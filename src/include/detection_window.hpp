/*
 * detection_window.hpp
 *
 *      Author: maheriya
 * Description: Detection window usingOpenGL that can show an image and bounding boxes
 */

#ifndef __DETECTION_WINDOW_HPP_
#define __DETECTION_WINDOW_HPP_
#include <inttypes.h>
#include <map>
//#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// GL includes
//#include "Shader.h"

#include <string>
#include <opencv2/opencv.hpp>

using namespace std;

struct Character {
    GLuint     TextureID;  // ID handle of the glyph texture
    glm::ivec2 Size;       // Size of glyph
    glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
    GLuint     Advance;    // Offset to advance to next glyph
};

struct Detection {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    glm::vec3 color;
    string label;
    float score;
};

class DetectionWindow {
public:

    DetectionWindow(void) :
        mWidth(1280),
        mHeight(720),
        mScreenWidth(1280),
        mScreenHeight(720),
        //
        mImageVAO(-1),
        mImageVertexBuffer(-1),
        mImageTexID(-1),
        mImageShaderProgram(-1),
        //
        mLineWidth(2.6f),
        mBBoxVAO(-1),
        mBBoxUniColor(-1),
        mBBoxVertexBuffer(-1),
        mBBoxShaderProgram(-1),
        //
        mTextVAO(-1),
        mTextVertexBuffer(-1),
        mTextUVBuffer(-1),
        mTextUniTexSampler(-1),
        mTextUniTextColor(-1),
        mTextTextID(-1),
        mTextShaderProgram(-1),
        mWindow(NULL) { }

    int createWindow(int width, int height, string winname="OpenGL Window");
    int display(const unsigned char* img, GLuint format);
    int display(cv::cuda::GpuMat& img);
    void setTitle(char* title);

    // Adds a detection to a list of detections. No visual processing is involved.
    inline void addDetection(Detection& det) {
        detections.push_back(det);
    }
    inline void delDetections(void) {
        detections.clear();
        detections.shrink_to_fit();
    }

    void cleanup(void);

    inline GLFWwindow* win(void) { return mWindow; }
    int _checkError(char *file, int line);

private:
    // Window setup
    GLint mWidth;
    GLint mHeight;
    GLint mScreenWidth;
    GLint mScreenHeight;

    // Image setup
    GLFWwindow* mWindow;
    GLuint mImageVAO;
    GLuint mImageVertexBuffer;
    GLuint mImageShaderProgram;
    GLuint mImageTexID;

    // Bounding box setup
    GLfloat mLineWidth;
    GLuint mBBoxVAO;
    GLuint mBBoxUniColor;
    GLuint mBBoxVertexBuffer;
    GLuint mBBoxShaderProgram;
    vector<Detection> detections;

    // Text setup (for labels)
    GLuint mTextVAO;
    GLuint mTextTextID;
    GLuint mTextVertexBuffer;
    GLuint mTextUVBuffer;
    GLuint mTextShaderProgram;
    GLuint mTextUniTexSampler;
    GLuint mTextUniTextColor;
    map<GLchar, Character> mCharacters;

    int initializeGLFW(void);
    int initBuffers(void);
    int initImageBuffers(void);
    int initBBoxBuffers(void);
    int initTextBuffers(void);

    GLuint createVertexBuffer(const void *vertex_buffer, GLuint vbsize, bool dstatic=true);
    GLuint createVertexArray(void);
    int createImageShaders(GLuint*);
    int createBBoxShaders(GLuint*);
    int createTextShaders(GLuint*);

    int showImage(cv::cuda::GpuMat& img);
    int showBBox(void);
    int showText(void);

    int loadFonts(void);
    void renderTextTrueType(string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);

    inline void unBindBuffers(void) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }

};

#endif /* __DETECTION_WINDOW_HPP_ */
