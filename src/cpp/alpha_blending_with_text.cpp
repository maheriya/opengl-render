/*
 * opengl_basic_rendering.cpp
 *
 *  Created on: Sep 23, 2019
 *      Author: maheriya
 */


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <map>
//#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <opencv2/opencv.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define DEBUG 2

using namespace std;

struct Character {
    GLuint     TextureID;  // ID handle of the glyph texture
    glm::ivec2 Size;       // Size of glyph
    glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
    GLuint     Advance;    // Offset to advance to next glyph
};

map<GLchar, Character> mCharacters;

// Callback for errors
static void glfw_error_callback(int error, const char* desc) {
    fprintf(stderr, "glfw_error_callback(): Error %d: %s\n", error, desc);
}

// Callback for keys (ESCAPE key will cause closing the window)
static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

}

// glfw: whenever the window size is changed (by OS or user resize) this callback function executes
static void glfw_fb_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on high-DPI displays.
    glViewport(0, 0, width, height);
}

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = R"(#version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() {
       FragColor = vec4(color, 0.6f); //vec4(1.0f, 0.5f, 0.2f, 1.0f);
    }
    )";


int loadFonts(void) {
    // Load TrueType fonts
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        cout << "ERROR::FREETYPE: Could not init FreeType Library" << endl;
        return GL_FALSE;
    }

    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-R.ttf", 0, &face)) {
        cout << "ERROR::FREETYPE: Failed to load font" << endl;
        return GL_FALSE;
    }
    FT_Set_Pixel_Sizes(face, 0, 48); // width decided by lib

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    for (GLubyte c = 0; c < 128; c++) {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            cout << "ERROR::FREETYTPE: Failed to load Glyph" << endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RED,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     0,
                     GL_RED,
                     GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer
        );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint)face->glyph->advance.x
        };
        mCharacters.insert(pair<GLchar, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return GL_TRUE;
}

int createTextShaders(GLuint* shader_program_id) {
    // Shaders for TrueType fonts rendering
    const GLchar* vs_source = R"(#version 330 core

layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 UV;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    UV = vertex.zw;
}
)";

    const GLchar* fs_source = R"(#version 440 core

in vec2 UV;
out vec4 color;

uniform sampler2D texSampler;
uniform vec3      textColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texSampler, UV).r);
    color = vec4(textColor, 0.4) * sampled;
}
)";

    GLint compile_ok = GL_FALSE;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_source, NULL);
    glCompileShader(vs);

    glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE) {
        glDeleteShader(vs);
        return GL_FALSE;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_source, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE) {
        glDeleteShader(fs);
        return GL_FALSE;
    }

    *shader_program_id = glCreateProgram();
    glAttachShader(*shader_program_id, fs);
    glAttachShader(*shader_program_id, vs);
    glLinkProgram(*shader_program_id);
    glDeleteShader(vs);
    glDeleteShader(fs);

    return GL_TRUE;
}

void renderTextTrueType(GLuint VBO, string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    // Iterate through all characters
    string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = mCharacters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
}

int main(int argc, char *argv[]) {
    string imgfile = "/home/maheriya/Pictures/c2371d98-GOPR0131-0569-z1.jpg";
    cv::Mat img = cv::imread(imgfile.c_str(), cv::IMREAD_COLOR); //cv::IMREAD_UNCHANGED); // use UNCHANGED for extracting alpha channel from png files
    if (img.empty()) {
        printf("OpenCV error: Could not open image %s\n", argv[1]);
        return -1;
    }
    GLint width = img.cols;
    GLint height = img.rows;
#if DEBUG>=1
    printf("Image file: %s\n", argv[1]);
    printf("Image size: %dx%d\n", width, height);
    printf("Image channels: %d\n", img.channels());
#endif
    cv::cuda::GpuMat imgGPU;
    imgGPU.upload(img);
#if DEBUG>=2
    printf("    img step: %d, elemSize: %d\n", (int)img.step, (int)img.elemSize());
    printf("GPU img step: %d, elemSize: %d\n", (int)imgGPU.step, (int)imgGPU.elemSize());
#endif

    // Create window
    if (glfwInit() == GL_FALSE) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 4.2 works too
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 1);
    glfwSetErrorCallback(glfw_error_callback);


    GLFWwindow* mWindow = glfwCreateWindow(width, height, "Opengl Rendering Window", NULL, NULL);
    printf("Window size (created): %dx%d\n", width, height);
    if (mWindow == NULL) {
        glfwTerminate();
        printf("Failed to create window");
        return -1;
    }

    glfwMakeContextCurrent(mWindow);
    glfwSetKeyCallback(mWindow, glfw_key_callback);
    glfwSetFramebufferSizeCallback(mWindow, glfw_fb_size_callback);


    glewExperimental = GL_TRUE; // *Needed* for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        printf("%s\n", "glewInit() failed");
        return -1;
    }
    glViewport(0, 0, width, height);


    // Compile shaders
    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //-///////////////////////////////////////////////////////////////////////////////////////
    // Triangles setup start
    //-///////////////////////////////////////////////////////////////////////////////////////
    // Triangle #1
    // set up a triangle shape
    // ------------------------------------------------------------------
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // left
         0.5f, -0.5f, 0.0f, // right
         0.0f,  0.5f, 0.0f  // top
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // un-bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Triangle #2
    // set up a triangle shape
    // ------------------------------------------------------------------
    float vertices2[] = {
                    0.0f, -0.8f, 0.0f, // left
                    0.8f, -0.8f, 0.0f, // right
                    0.8f,  0.8f, 0.0f  // top
    };

    unsigned int VAO2, VBO2;
    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO2);
    glBindVertexArray(VAO2);

    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Triangle #3
    // set up a triangle shape
    // ------------------------------------------------------------------
    float vertices3[] = {
                   -0.8f,  0.0f, 0.0f, // left
                    0.1f,  0.0f, 0.0f, // right
                    0.1f,  0.8f, 0.0f  // top
    };

    unsigned int VAO3, VBO3;
    glGenVertexArrays(1, &VAO3);
    glGenBuffers(1, &VBO3);
    glBindVertexArray(VAO3);

    glBindBuffer(GL_ARRAY_BUFFER, VBO3);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3), vertices3, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // un-bind VBO3 and VAO3
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //-///////////////////////////////////////////////////////////////////////////////////////
    // Triangles setup end
    //-///////////////////////////////////////////////////////////////////////////////////////




    //-///////////////////////////////////////////////////////////////////////////////////////
    // Text setup start
    //-///////////////////////////////////////////////////////////////////////////////////////
    GLuint mTextShaderProgram;
    GLint ret = createTextShaders(&mTextShaderProgram);
    if (ret == GL_FALSE) {
        glfwTerminate();
        printf("Text shader compilation failed\n");
        return -1;
    }

    // Initialize uniforms' IDs
    GLuint mTextUniTexSampler = glGetUniformLocation(mTextShaderProgram, "texSampler");
    GLuint mTextUniTextColor = glGetUniformLocation(mTextShaderProgram, "textColor");

    glm::mat4 mProjection = glm::ortho(0.0f, (float)width, 0.0f, (float)height);
    GLuint textUniProjection = glGetUniformLocation(mTextShaderProgram, "projection");

    glUseProgram(mTextShaderProgram);
    glUniformMatrix4fv(textUniProjection, 1, GL_FALSE, glm::value_ptr(mProjection));

    if (loadFonts() == GL_FALSE)
        return -1;

    unsigned int VAO4, VBO4;
    glGenVertexArrays(1, &VAO4);
    glGenBuffers(1, &VBO4);
    glBindVertexArray(VAO4);

    glBindBuffer(GL_ARRAY_BUFFER, VBO4);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    // un-bind VBO4 and VAO4
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    //-///////////////////////////////////////////////////////////////////////////////////////
    // Text setup end
    //-///////////////////////////////////////////////////////////////////////////////////////


    glDisable(GL_DEPTH_TEST); // Disable depth test to enable depth-independent, ordered drawing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint uniColor = glGetUniformLocation(shaderProgram, "color");
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    while (!glfwWindowShouldClose(mWindow)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


#if 1
        //-///////////////////////////////////////////////////////////////////////////////////////
        // Draw triangles
        //-///////////////////////////////////////////////////////////////////////////////////////
        // draw the red triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        float clr[3] = {1.0f, 0.0f, 0.0f};
        glUniform3fv(uniColor, 1, clr);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // draw the green triangle
        glBindVertexArray(VAO2);
        float clr2[3] = {0.0f, 1.0f, 0.0f};
        glUniform3fv(uniColor, 1, clr2);
        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // draw the blue triangle
        glBindVertexArray(VAO3);
        float clr3[3] = {0.0f, 0.0f, 1.0f};
        glUniform3fv(uniColor, 1, clr3);
        glBindBuffer(GL_ARRAY_BUFFER, VBO3);
        glDrawArrays(GL_TRIANGLES, 0, 3);
#endif

        //-///////////////////////////////////////////////////////////////////////////////////////
        // Draw text
        //-///////////////////////////////////////////////////////////////////////////////////////
        glUseProgram(mTextShaderProgram);
        glm::vec3 color(0.5f, 0.8f, 0.2f);
        glUniform3f(mTextUniTextColor, color.x, color.y, color.z);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO4);

        renderTextTrueType(VBO4, "Text at BOTTOM LEFT", 50.0f, 20.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
        renderTextTrueType(VBO4, "This is TTF Text", 400.0f, 500.0f, 1.0f, glm::vec3(0.8, 0.8f, 0.2f));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(mWindow);
        glfwPollEvents();

    }
    glfwTerminate();
}

