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
#include "shader.hpp"
#include "texture.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace std;
#define SHOW_IMAGE     1
#define SHOW_BBOX      1
#define SHOW_TEXT      1

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
        DetectionWindow::done = true;
    }

}

bool DetectionWindow::done = false;

int DetectionWindow::initializeGLFW(void) {
    if (glfwInit() == GL_FALSE)
        return GL_FALSE;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
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
    if (mWindow == NULL) {
        glfwTerminate();
        printf("Failed to create window");
        return GL_FALSE;
    }

    glfwMakeContextCurrent(mWindow);
    glfwSetKeyCallback(mWindow, glfw_key_callback);

    glewExperimental = GL_TRUE; // *Needed* for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        printf("%s\n", "glewInit() failed");
        return GL_FALSE;
    }
    // glewInit() may cause an OpenGL error; we just want to clear the error. Ignore error.
    GLenum error_code;
    if ((error_code = glGetError()) != GL_NO_ERROR) {
        if (error_code != GL_INVALID_ENUM)
            printf("OpenGL Error: %d (line %d)\n", error_code, __LINE__);
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    return initBuffers();
}

int DetectionWindow::initBuffers(void) {
    if (mWindow == NULL) {
        printf("Window is not created yet!\n");
        return GL_FALSE;
    }
    mVAO = createVertexArray();
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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Enable transparency (for box lines, e.g.)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

int DetectionWindow::initImageBuffers(void) {
    // Canvas for image (two triangles)
    const GLshort mVertices[] = {
                   -1, 1, 0,
                    1, 1, 0,
                   -1,-1, 0,
                    1,-1, 0 };

    // UV coordinates for the two triangles on canvas
    const GLshort mUVs[] = {
                    0, 0,
                    1, 0,
                    0, 1,
                    1, 1 };

    mVertexBuffer = createVertexBuffer(mVertices, sizeof(mVertices));
    mUVBuffer = createVertexBuffer(mUVs, sizeof(mUVs));

    GLint ret = createImageShaders(&mImageShaderProgram);
    if (ret == GL_FALSE) {
        cleanup();
        printf("Image shader compilation failed\n");
        return GL_FALSE;
    }

    glGenTextures(1, &mImageTexID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mImageTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    mBBoxColorLOC = glGetUniformLocation(mBBoxShaderProgram, "BBCOLOR");
    glLineWidth(mLineWidth);

    return checkError();
}

int DetectionWindow::initTextBuffers(void) {
    // Initialize text with the Holstein font
    mTextTextID = loadDDS("../data/Holstein.DDS");

    // Initialize VBO
    glGenBuffers(1, &mTextVertexBuffer);
    glGenBuffers(1, &mTextUVBuffer);

    // Initialize Shader
    //mTextShaderProgram = LoadShaders("../shaders/TextVertexShader.vertexshader", "../shaders/TextVertexShader.fragmentshader");
    GLint ret = createTextShaders(&mTextShaderProgram);
    if (ret == GL_FALSE) {
        cleanup();
        printf("Image shader compilation failed\n");
        return GL_FALSE;
    }


    // Initialize uniforms' IDs
    mTextUniTexSampler = glGetUniformLocation(mTextShaderProgram, "texSampler");

    if (loadFonts() == GL_FALSE)
        return GL_FALSE;

    return checkError();
}

int DetectionWindow::display(const unsigned char* img, GLuint format) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBindVertexArray(mVAO);

#if SHOW_TEXT
    showText();
#endif
#if SHOW_IMAGE
    showImage(img, format);
#endif
#if SHOW_BBOX
    showBBox();
#endif

    glfwSwapBuffers(mWindow);
    glfwPollEvents();
    glfwSetWindowTitle(mWindow, "New OpenGL Window");
}


int DetectionWindow::display(cv::cuda::GpuMat& img) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBindVertexArray(mVAO);

#if SHOW_TEXT
    showText();
#endif
#if SHOW_IMAGE
    showImage(img);
#endif
#if SHOW_BBOX
    showBBox();
#endif

    glfwSwapBuffers(mWindow);
    glfwPollEvents();
    glfwSetWindowTitle(mWindow, "New OpenGL Window");
}


int DetectionWindow::showImage(const unsigned char* img, GLuint format) {
    glUseProgram(mImageShaderProgram);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glEnableVertexAttribArray(0);
    //                 index​, size​,     type​, normalized​, stride​, *offset​
    glVertexAttribPointer(0,     3, GL_SHORT,   GL_FALSE,      0,    NULL);

    glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0, NULL);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mImageTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (format == GL_BGRA)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, img);
    else if (format == GL_RGBA)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    else if (format == GL_BGR)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, img);
    else {
        printf("Unsupported format %" PRIu16 "\n", format);
        return GL_FALSE;
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return checkError();
}

int DetectionWindow::showImage(cv::cuda::GpuMat& img) {
    glUseProgram(mImageShaderProgram);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glEnableVertexAttribArray(0);
    //                 index​, size​,     type​, normalized​, stride​, *offset​
    glVertexAttribPointer(0,     3, GL_SHORT,   GL_FALSE,      0,    NULL);

    glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0, NULL);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mImageTexID);
    cv::ogl::Texture2D tex(img);
    tex.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    return GL_TRUE;
}

int DetectionWindow::showBBox(void) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(mBBoxShaderProgram);
    for (auto& det: detections) {
        glUniform3fv(mBBoxColorLOC, 1, det.color);
        // Create 2D bounding box
        const GLfloat bboxVertices[] = {
                        det.xmin, det.ymin,
                        det.xmax, det.ymin,
                        det.xmax, det.ymax,
                        det.xmin, det.ymax };
        mBBoxVertexBuffer = createVertexBuffer(bboxVertices, sizeof(bboxVertices), false);

        glBindBuffer(GL_ARRAY_BUFFER, mBBoxVertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        glDisableVertexAttribArray(0);
    }
    glDisable(GL_BLEND);

    return GL_TRUE;
}

// Render text
int DetectionWindow::showText(void) {
    glClearColor(0.0f, 0.7f, 0.7f , 0.0f);

    char text[256];
    for (auto& det: detections) {
        renderText2D(det.label.c_str(), det.xmin, det.ymin, 20.f/mHeight);
    }
    return GL_TRUE;
}


// Adds a detection to a list of detections. No visual processing is involved.
void DetectionWindow::addDetection(Detection& det) {
    detections.push_back(det);
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
    glDeleteVertexArrays(1, &mVAO);
#if SHOW_IMAGE
    // Image
    glDeleteBuffers(1, &mVertexBuffer);
    glDeleteBuffers(1, &mUVBuffer);
    glDeleteTextures(1, &mImageTexID);
    glDeleteProgram(mImageShaderProgram);
#endif

#if SHOW_BBOX
    // BBox
    glDeleteBuffers(1, &mBBoxVertexBuffer);
    glDeleteProgram(mBBoxShaderProgram);
#endif

#if SHOW_TEXT
    // Text
    glDeleteBuffers(1, &mTextVertexBuffer);
    glDeleteBuffers(1, &mTextUVBuffer);
    glDeleteTextures(1, &mTextTextID);
    glDeleteProgram(mTextShaderProgram);
#endif

    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

int DetectionWindow::createImageShaders(GLuint* shader_program_id) {
    const GLchar* vs_source = R"(#version 440

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;

out vec2 texUV;

void main() {
  gl_Position = vec4(vertexPosition, 1.0f);
  texUV = vertexUV;
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
    const GLchar* vs_source = R"(#version 440 core

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
    // This shader takes xmin,ymin,xmax,ymax format vertex input, and converts into OpenGL convention
    // Maps [0..1][0..1] OpenCV coordinates (x left to right, y top to down) to
    // [-1..1][-1..1] OpenGL coordinates with x left to right and y down to up (inverted)
    const GLchar* vs_source = R"(#version 440 core

layout(location = 0) in vec2 position_opencv;
layout(location = 1) in vec2 vertexUV;

out vec2 UV;

void main(){
    vec2 position_opengl;
    position_opengl = 2.0 * position_opencv - 1.0f; // [0..1] -> [-1..1]
    position_opengl.y *= -1.0; // invert y

    gl_Position =  vec4(position_opengl,0,1);
    UV = vertexUV;
}
)";
    const GLchar* fs_source = R"(#version 440 core

in vec2 UV;
out vec4 color;

uniform sampler2D texSampler;

void main(){
    vec4 _color = vec4(1.0f, 0.8f, 0.2f, 1.0f);
    //vec4 _color = vec4(0.8f, 0.0f, 0.0f, 1.0f);

    color = texture( texSampler, UV );
    color = _color * color;
    
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

int DetectionWindow::renderText2D(const char * text, GLfloat x, GLfloat y, GLfloat size) {

    unsigned int length = strlen(text);

    // Fill buffers
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> UVs;
    for (unsigned int i = 0; i < length; i++) {

        // x low to high is left to right, ; y low to high is up to down [OpenCV convention input]
        glm::vec2 vertex_down_left  = glm::vec2( x+i*size     , y+size );
        glm::vec2 vertex_down_right = glm::vec2( x+i*size+size, y+size );
        glm::vec2 vertex_up_right   = glm::vec2( x+i*size+size, y      );
        glm::vec2 vertex_up_left    = glm::vec2( x+i*size     , y      );

//        // x low to high is left to right, ; y low to high is down to up [OpenGL convention input]
//        glm::vec2 vertex_up_left    = glm::vec2( x+i*size     , y+size );
//        glm::vec2 vertex_up_right   = glm::vec2( x+i*size+size, y+size );
//        glm::vec2 vertex_down_right = glm::vec2( x+i*size+size, y      );
//        glm::vec2 vertex_down_left  = glm::vec2( x+i*size     , y      );

        vertices.push_back(vertex_up_left);
        vertices.push_back(vertex_down_left);
        vertices.push_back(vertex_up_right);

        vertices.push_back(vertex_down_right);
        vertices.push_back(vertex_up_right);
        vertices.push_back(vertex_down_left);

        char character = text[i];
        float uv_x = (character%16)/16.0f;
        float uv_y = (character/16)/16.0f;

        glm::vec2 uv_up_left    = glm::vec2( uv_x           , uv_y );
        glm::vec2 uv_up_right   = glm::vec2( uv_x+1.0f/16.0f, uv_y );
        glm::vec2 uv_down_right = glm::vec2( uv_x+1.0f/16.0f, (uv_y + 1.0f/16.0f) );
        glm::vec2 uv_down_left  = glm::vec2( uv_x           , (uv_y + 1.0f/16.0f) );
        UVs.push_back(uv_up_left);
        UVs.push_back(uv_down_left);
        UVs.push_back(uv_up_right);

        UVs.push_back(uv_down_right);
        UVs.push_back(uv_up_right);
        UVs.push_back(uv_down_left);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mTextVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mTextUVBuffer);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs[0], GL_STATIC_DRAW);

    // Bind shader
    glUseProgram(mTextShaderProgram);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextTextID);
    // Set our "myTextureSampler" sampler to use Texture Unit 0
    glUniform1i(mTextUniTexSampler, 0);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, mTextVertexBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );

    // 2nd attribute buffer : UVs
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, mTextUVBuffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw call
    glDrawArrays(GL_TRIANGLES, 0, vertices.size() );

    glDisable(GL_BLEND);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    return checkError();
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

    // check that we can load font for 'X'
    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
        cout << "ERROR::FREETYTPE: Failed to load Glyph" << endl;


    return GL_TRUE;
}
