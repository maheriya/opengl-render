/*
 * detection_window.cpp
 *
 *      Author: maheriya
 * Description: Detection window usingOpenGL that can show an image and bounding boxes
 */

#include <stdio.h>
#include <stdlib.h>
#include "detection_window.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/core/opengl.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace std;
#define SHOW_IMAGE       1
#define SHOW_BBOX        1
#define SHOW_TEXT        1
#define NUM_BOX_VERTICES 4

#define checkError() _checkError(__FILE__, __LINE__)


inline int DetectionWindow::_checkError(char *file, int line) {
    GLenum error_code = glGetError();
    if (error_code != GL_NO_ERROR) {
        printf("OpenGL Error: %d (%s:%d)\n", error_code, file, line);
        return GL_FALSE;
    }
    return GL_TRUE;
}

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

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
static void glfw_fb_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

int DetectionWindow::initializeGLFW(void) {
    if (glfwInit() == GL_FALSE)
        return GL_FALSE;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 4.2 works too
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 1);
    glfwSetErrorCallback(glfw_error_callback);

    return GL_TRUE;
}

int DetectionWindow::createWindow(int width, int height, string winname) {
    if (initializeGLFW() == GL_FALSE) {
        printf("Failed to initialize GLFW\n");
        return GL_FALSE;
    }
    GLFWmonitor *primary = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary);
    mScreenWidth = mode->width;
    mScreenHeight = mode->height;

    mWidth = (width <= mScreenWidth) ? width : mScreenWidth;
    mHeight = (height <= mScreenHeight) ? height : mScreenHeight;

    mWindow = glfwCreateWindow(mWidth, mHeight, winname.c_str(), NULL, NULL);
    printf("Window size (created): %dx%d\n", mWidth, mHeight);
    if (mWindow == NULL) {
        glfwTerminate();
        printf("Failed to create window");
        return GL_FALSE;
    }

    glfwMakeContextCurrent(mWindow);
    glfwSetKeyCallback(mWindow, glfw_key_callback);
    glfwSetFramebufferSizeCallback(mWindow, glfw_fb_size_callback);


    glewExperimental = GL_TRUE; // *Needed* for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        printf("%s\n", "glewInit() failed");
        return GL_FALSE;
    }

    // Define the viewport dimensions
    glViewport(0, 0, mWidth, mHeight);

    // glewInit() may cause an OpenGL error; we just want to clear the error. Ignore error.
    GLenum error_code = glGetError();
    if ((error_code != GL_NO_ERROR) && (error_code != GL_INVALID_ENUM)) {
        printf("OpenGL Error: %d (line %d)\n", error_code, __LINE__);
    }
    glDisable(GL_DEPTH_TEST); // Ignore z values enforce ordered drawing
    glDepthFunc(GL_NEVER);
    // Enable transparency (for box lines, text, e.g.)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.2f, 0.2f , 0.2f);

    return initBuffers();
}

void DetectionWindow::setTitle(char* title) {
    glfwSetWindowTitle(mWindow, title);
}


int DetectionWindow::initBuffers(void) {
    if (mWindow == NULL) {
        printf("Window is not created yet!\n");
        return GL_FALSE;
    }
#if SHOW_IMAGE
    if (initImageBuffers() == GL_FALSE) {
        glfwTerminate();
        return GL_FALSE;
    }
#endif

#if SHOW_BBOX
    if (initBBoxBuffers()  == GL_FALSE) {
        glfwTerminate();
        return GL_FALSE;
    }
#endif

#if SHOW_TEXT
    if (initTextBuffers()  == GL_FALSE) {
        glfwTerminate();
        return GL_FALSE;
    }
#endif

    return GL_TRUE;
}

int DetectionWindow::initImageBuffers(void) {
    mImageVAO = createVertexArray();
    GLint ret = createImageShaders(&mImageShaderProgram);
    if (ret == GL_FALSE) {
        cleanup();
        printf("Image shader compilation failed\n");
        return GL_FALSE;
    }

    // Canvas for image (triangles strip to make a quad)
    const GLshort mVertices[] = {
             // position   texture coords (note inverted v as OpenCV images are scanned up to down)
             //     x  y   u  v
                   -1, 1,  0, 0,
                    1, 1,  1, 0,
                   -1,-1,  0, 1,
                    1,-1,  1, 1};


    mImageVertexBuffer = createVertexBuffer(mVertices, sizeof(mVertices));
    glBindBuffer(GL_ARRAY_BUFFER, mImageVertexBuffer);
    glEnableVertexAttribArray(0);
    //                 index​, size​,     type​, normalized​, stride​, *offset​
    glVertexAttribPointer(0,     4, GL_SHORT,   GL_FALSE,      0,    NULL);

    // cleanup
    unBindBuffers();

    return checkError();
}

int DetectionWindow::initBBoxBuffers(void) {

    GLint ret = createBBoxShaders(&mBBoxShaderProgram);
    if (ret == GL_FALSE) {
        cleanup();
        printf("BBox shader compilation failed\n");
        return GL_FALSE;
    }
    // Get a handle for BBox color uniform
    mBBoxUniColor = glGetUniformLocation(mBBoxShaderProgram, "BBCOLOR");
    glLineWidth(mLineWidth);

    mBBoxVAO = createVertexArray();
    mBBoxVertexBuffer = createVertexBuffer(NULL, sizeof(GLfloat) * NUM_BOX_VERTICES * 2, false);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    // cleanup
    unBindBuffers();

    return checkError();
}

int DetectionWindow::initTextBuffers(void) {
    // Initialize Shader
    GLint ret = createTextShaders(&mTextShaderProgram);
    if (ret == GL_FALSE) {
        cleanup();
        printf("Text shader compilation failed\n");
        return GL_FALSE;
    }


    // Initialize uniforms' IDs
    mTextUniTexSampler = glGetUniformLocation(mTextShaderProgram, "texSampler");
    mTextUniTextColor = glGetUniformLocation(mTextShaderProgram, "textColor");

    // Orthographic projection (for text). Set up to allow specifying coordinates in screen pixels units
    glm::mat4 projection = glm::ortho(0.0f, (float)mWidth, 0.0f, (float)mHeight);
    //glm::mat4 projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f); // 0..1 x, 1..0 y. OpenCV convention
    GLuint textUniProjection = glGetUniformLocation(mTextShaderProgram, "projection");

    glUseProgram(mTextShaderProgram);
    glUniformMatrix4fv(textUniProjection, 1, GL_FALSE, glm::value_ptr(projection));

    if (loadFonts() == GL_FALSE)
        return GL_FALSE;

    mTextVAO = createVertexArray();
    mTextVertexBuffer = createVertexBuffer(NULL, sizeof(GLfloat) * NUM_BOX_VERTICES * 4, false);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);

    // cleanup
    unBindBuffers();

    return checkError();
}

int DetectionWindow::display(cv::cuda::GpuMat& img) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if SHOW_IMAGE
    showImage(img);
#endif
#if SHOW_BBOX
    showBBox();
#endif
#if SHOW_TEXT
    showText();
#endif

    glfwSwapBuffers(mWindow);
    glfwPollEvents();
    delDetections();
}


int DetectionWindow::showImage(cv::cuda::GpuMat& img) {
    glBindVertexArray(mImageVAO);
    glUseProgram(mImageShaderProgram);

    glBindBuffer(GL_ARRAY_BUFFER, mImageVertexBuffer);
    glEnableVertexAttribArray(0);
    //                 index​, size​,     type​, normalized​, stride​, *offset​
    glVertexAttribPointer(0,     4, GL_SHORT,   GL_FALSE,      0,    NULL);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mImageTexID);
    cv::ogl::Texture2D tex(img);
    tex.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Cleanup
    unBindBuffers();

    return GL_TRUE;
}

int DetectionWindow::showBBox(void) {
    glBindVertexArray(mBBoxVAO);
    glUseProgram(mBBoxShaderProgram);

    for (auto& det: detections) {
        //glUniform3f(mBBoxUniColor, det.color.x, det.color.y, det.color.z);
        glUniform3fv(mBBoxUniColor, 1, glm::value_ptr(det.color));
        // Create 2D bounding box
        const GLfloat bboxVertices[] = {
                        det.xmin, det.ymin,
                        det.xmax, det.ymin,
                        det.xmax, det.ymax,
                        det.xmin, det.ymax };

        glBindBuffer(GL_ARRAY_BUFFER, mBBoxVertexBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bboxVertices), bboxVertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    // Cleanup
    unBindBuffers();

    return GL_TRUE;
}

// Render text
int DetectionWindow::showText(void) {
    for (auto& det: detections) {
        string label = det.label; // TODO: add det.score
        renderTextTrueType(label, det.xmin, det.ymin, 0.35f, det.color);
    }

    // Cleanup
    unBindBuffers();

    return GL_TRUE;
}


GLuint DetectionWindow::createVertexBuffer(const void *vertex_buffer, GLuint vbsize, bool dstatic) {
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (dstatic)
        glBufferData(GL_ARRAY_BUFFER, vbsize, vertex_buffer, GL_STATIC_DRAW);
    else
        glBufferData(GL_ARRAY_BUFFER, vbsize, vertex_buffer, GL_DYNAMIC_DRAW);
    return vbo;
}

GLuint DetectionWindow::createVertexArray() {
    GLuint VAID = 0;
    glGenVertexArrays(1, &VAID);
    glBindVertexArray(VAID);
    return VAID;
}

void DetectionWindow::cleanup(void) {
#if SHOW_IMAGE
    // Image
    glDeleteVertexArrays(1, &mImageVAO);
    glDeleteBuffers(1, &mImageVertexBuffer);
    glDeleteTextures(1, &mImageTexID);
    glDeleteProgram(mImageShaderProgram);
#endif

#if SHOW_BBOX
    // BBox
    glDeleteVertexArrays(1, &mBBoxVAO);
    glDeleteBuffers(1, &mBBoxVertexBuffer);
    glDeleteProgram(mBBoxShaderProgram);
#endif

#if SHOW_TEXT
    // Text
    glDeleteVertexArrays(1, &mTextVAO);
    glDeleteBuffers(1, &mTextVertexBuffer);
    glDeleteBuffers(1, &mTextUVBuffer);
    glDeleteTextures(1, &mTextTextID);
    glDeleteProgram(mTextShaderProgram);
#endif

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

int DetectionWindow::createImageShaders(GLuint* shader_program_id) {
    const GLchar* vs_source = R"(#version 330

layout(location = 0) in vec4 vertex;

out vec2 texUV;

void main() {
  gl_Position = vec4(vertex.xy, 0.0f, 1.0f);
  texUV = vertex.zw;
}
)";
    const GLchar* fs_source = R"(#version 440

out vec4 frag_color;
in vec2 texUV;

uniform sampler2D texture;
void main() {
  frag_color = texture2D(texture, texUV);
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

int DetectionWindow::createBBoxShaders(GLuint* shader_program_id) {
    // This shader takes xmin,ymin,xmax,ymax format box input, and converts into OpenGL convention
    const GLchar* vs_source = R"(#version 330 core

layout(location = 0) in vec3 vertexPosition;

void main() {
  vec3 position;
  position = vertexPosition * 2 - 1.0f;
  position.y *= -1.0f;
  gl_Position = vec4(position, 1.0f);
}
)";

    const GLchar* fs_source = R"(#version 440 core

out vec4 frag_color;

uniform vec3 BBCOLOR;
void main() {
  frag_color = vec4(BBCOLOR, 0.8f);
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

int DetectionWindow::createTextShaders(GLuint* shader_program_id) {
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
    color = vec4(textColor, 0.8) * sampled;
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

// render text
// params
// _x: bottom-left x position for text. [0..1] == [left..right]
// _y: bottom-left y position for text. [0..1] == [top..bottom]
void DetectionWindow::renderTextTrueType(string text, GLfloat _x, GLfloat _y, GLfloat scale, glm::vec3 color) {

#if SHOW_BBOX
    // First render a solid box behind text
    GLfloat tw = 10.0f; // total text width
    for (auto ch: text)
        tw += (mCharacters[ch].Advance >> 6);
    tw *= (scale/mWidth); // fraction of screen width
    GLfloat bx = _x;
    GLfloat by;
    GLfloat h = (mCharacters['X'].Size.y *1.7f) * scale / mHeight;
    if (_y < 20.0f/mHeight) {
        h *= -1.0f;
        by = _y + 1.0f/mHeight;
    } else {
        by = _y - 1.0f/mHeight;
    }
    GLfloat boxVertices[] = {
        // triangle strip
        bx,        by - h,
        bx + tw,   by - h,
        bx,        by,
        bx + tw,   by
    };
    // We will use BBox shaders to draw a solid bg box for text
    glBindVertexArray(mBBoxVAO);
    glUseProgram(mBBoxShaderProgram);
    glUniform3fv(mBBoxUniColor, 1, glm::value_ptr(color));
    glBindBuffer(GL_ARRAY_BUFFER, mBBoxVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(boxVertices), boxVertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, NUM_BOX_VERTICES);
    glBindVertexArray(0);
#endif
#if 1
    // Convert to pixel units and add margins
    // y : [mHeight..0] == [top..bottom]
    // x : [0..mWidth] == [left..right]
    GLfloat y = mHeight-_y*mHeight + 6.0f;
    if (y > (mHeight - 15))
        y = (mHeight - 15);
    GLfloat x = _x*mWidth + 1.0f;

    // Render text characters
    glBindVertexArray(mTextVAO);
    glUseProgram(mTextShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glUniform3f(mTextUniTextColor, 1.0f - color.x, 1.0f - color.y, 1.0f - color.z);
    string::const_iterator c;
    for (auto c: text) {
        Character ch = mCharacters[c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character

        GLfloat vertices[NUM_BOX_VERTICES][4] = {
            // triangle strip (4 vertices)
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos + h,   1.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, mTextVertexBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLE_STRIP, 0, NUM_BOX_VERTICES);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bit-shift by 6 to get value in pixels (2^6 = 64)
    }
#endif
}

int DetectionWindow::loadFonts(void) {
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
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return GL_TRUE;
}
