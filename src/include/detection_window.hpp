/*
 * detection_window.hpp
 *
 *      Author: maheriya
 * Description: Detection window usingOpenGL that can show an image and bounding boxes
 */

#ifndef __DETECTION_WINDOW_HPP_
#define __DETECTION_WINDOW_HPP_
#include <inttypes.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <opencv2/opencv.hpp>

using namespace std;

struct Detection {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float color[3];
    string label;
    float score;
};

class DetectionWindow {
public:
    static bool done; // To indicate we are done via callbacks outside the class

    DetectionWindow(void) :
        mWidth(1280),
        mHeight(720),
        mScreenWidth(1280),
        mScreenHeight(720),
        mVAO(-1),
        //
        mVertexBuffer(-1),
        mUVBuffer(-1),
        mImageTexID(-1),
        mImageShaderProgram(-1),
        //
        mLineWidth(2.6f),
        mBBoxColorLOC(-1),
        mBBoxVertexBuffer(-1),
        mBBoxShaderProgram(-1),
        //
        //mTextColorLOC(-1),
        mTextVertexBuffer(-1),
        mTextUVBuffer(-1),
        mTextTextID(-1),
        mTextShaderProgram(-1),
        mWindow(NULL) { }

    int createWindow(int width, int height, string winname="OpenGL Window");
    int display(const unsigned char* img, GLuint format);
    int display(cv::cuda::GpuMat& img);
    void addDetection(Detection& det);

    void cleanup(void);

    inline GLFWwindow* win(void) { return mWindow; }
    int _checkError(char *file, int line);

private:
    // Window setup
    GLint mWidth;
    GLint mHeight;
    GLint mScreenWidth;
    GLint mScreenHeight;

    GLuint mVAO;
    // Image setup
    GLFWwindow* mWindow;
    GLuint mVertexBuffer;
    GLuint mUVBuffer;
    GLuint mImageShaderProgram;
    GLuint mImageTexID;

    // Bounding box setup
    GLfloat mLineWidth;
    GLuint mBBoxColorLOC;
    GLuint mBBoxVertexBuffer;
    GLuint mBBoxShaderProgram;
    vector<Detection> detections;

    // Text setup (for labels)
    GLuint mTextTextID;
    GLuint mTextVertexBuffer;
    GLuint mTextUVBuffer;
    GLuint mTextShaderProgram;
    GLuint mTextUniTexSampler;


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

    int showImage(const unsigned char* img, GLuint format);
    int showImage(cv::cuda::GpuMat& img);
    int showBBox(void);
    int showText(void);
    int renderText2D(const char * text, GLfloat x, GLfloat y, GLfloat size);
    int loadFonts(void);
};

#endif /* __DETECTION_WINDOW_HPP_ */
