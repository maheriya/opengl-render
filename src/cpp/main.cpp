#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <argp.h>
#include "detection_window.hpp"
#include <opencv2/opencv.hpp>

static int parse_opt(int, char*, struct argp_state*);
#define DEBUG 2

using namespace std;

int main(int argc, char *argv[]) {
    struct argp_option options[] = { { 0 } };

    static const char* doc = "OpenGL Image Viwer";
    struct argp argp = { options, parse_opt, "[FILE]", doc, 0, 0, 0 };

    int arg_count = 1;
    argp_parse(&argp, argc, argv, 0, 0, &arg_count);

    char* imgfile[120];
    cv::Mat img = cv::imread(argv[1], cv::IMREAD_COLOR); //cv::IMREAD_UNCHANGED); // use UNCHANGED for extracting alpha channel from png files
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

    DetectionWindow detectionWin;
    int ret = detectionWin.createWindow(width, height, "OpenGL Detections Viewer");
    if (ret == GL_FALSE) {
        printf("Could not create detection window.\n");
        glfwTerminate();
        return -1;
    }

    // Create a fake detection results
    string label1 = "Object 1";
    Detection det1 = {(1.0f/width), (1.0f/height), 0.5f, 0.5f,
                      glm::vec3(1.0f, 0.2f, 0.2f), label1, 0.98f };
    string label2 = "Object 2";
    Detection det2 = {0.5f, 0.5f, 1.0f-(1.0f/width), 1.0f-(1.f/height),
                    glm::vec3(0.2f, 1.0f, 0.2f), label2, 0.7f };
    string label3 = "Object 3";
    Detection det3 = {0.15f, 0.25f, 0.85f, 0.75f,
                    glm::vec3(0.2f, 0.2f, 1.0f), label3, 0.54f };


    int64 cnt = 0;
    char str[100];
    // simulate active detections
    while (!glfwWindowShouldClose(detectionWin.win())) {
        cnt++;
        if (cnt >= 100)
            detectionWin.addDetection(det1);
        if (cnt >= 200)
            detectionWin.addDetection(det2);
        if (cnt >= 300)
            detectionWin.addDetection(det3);

        detectionWin.display(imgGPU);
        sprintf(str, "Frame %ld" , cnt);

        detectionWin.setTitle(str);

    }
    detectionWin.cleanup();
}

static int parse_opt(int key, char *arg, struct argp_state *state) {
    int *arg_count = (int*) state->input;

    switch (key) {
    case ARGP_KEY_ARG:
        --(*arg_count);
        break;

    case ARGP_KEY_END:
        if (*arg_count > 0)
            argp_failure(state, 1, 0, "too few arguments");
        break;
    }
    return 0;
}

