#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <argp.h>

#include <opencv2/opencv.hpp>


void setup_vbo(GLuint*, GLshort*, GLuint);
void setup_vao(GLuint*);
void glfw_error_callback(int, const char*);
void glfw_key_callback(GLFWwindow*, int, int, int, int);
void cleanup(GLFWwindow*);
static int parse_opt(int, char*, struct argp_state*);
uint8_t setup_shaders(GLuint*);
uint8_t initialize_glfw(void);

int main(int argc, char *argv[]) {
    struct argp_option options[] = { { 0 } };

    static const char* doc = "OpenGL Image Viwer";
    struct argp argp = { options, parse_opt, "[FILE]", doc, 0, 0, 0 };

    int arg_count = 1;
    argp_parse(&argp, argc, argv, 0, 0, &arg_count);

    cv::Mat image = cv::imread(argv[1], cv::IMREAD_UNCHANGED); // use UNCHANGED for extracting alpha channel from png files
    if (image.empty()) {
        printf("OpenCV error: Could not open image %s\n", argv[1]);
        return -1;
    }
    GLint width, height, channels;
    width = image.cols;
    height = image.rows;
    channels = image.channels();
    printf("Image name: %s\n", argv[1]);
    printf("Image size: %dx%d\n", width, height);
    printf("Image channels: %d\n", channels);

    unsigned char *image_raw = image.data;

    uint8_t rv;
    if ((rv = initialize_glfw())) {
        printf("%s%" PRIu8 "\n", "initialize_glfw() failed with code ", rv);
        return -1;
    }

    glfwSetErrorCallback(glfw_error_callback);

    GLFWmonitor *primary = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary);
    const GLint screen_width = mode->width, screen_height = mode->height;

    GLFWwindow *window;
    if (!(window = glfwCreateWindow(width < screen_width ? width : screen_width,
                    height < screen_height ? height : screen_height, "OpenGL Window",
                    NULL,
                    NULL))) {
        glfwTerminate();
        printf("%s\n", "glfwCreateWindow failed");
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, glfw_key_callback);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        cleanup(window);
        printf("%s\n", "glewInit() failed");
        return -1;
    }

    printf("OpenGL %s\n", glGetString(GL_VERSION));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Canvas for image (two triangles)
    GLshort vertices[] = {
                    -1, 1, 0,
                     1, 1, 0,
                    -1,-1, 0,
                     1,-1, 0 };

    // Texture coords for the two triangles on canvas
    GLshort texcoords[] = {
                    0, 0,
                    1, 0,
                    0, 1,
                    1, 1 };

    GLuint vertices_vbo = 0;
    setup_vbo(&vertices_vbo, vertices, sizeof(vertices));
    GLuint texcoords_vbo = 0;
    setup_vbo(&texcoords_vbo, texcoords, sizeof(texcoords));

    glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);

    GLuint vao = 0;
    setup_vao(&vao);

    GLuint shader_program = 0;
    if (setup_shaders(&shader_program)) {
        cleanup(window);
        printf("%s\n", "shader compilation failed");
        return -1;
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);

    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (channels==4) // if image has alpha
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image_raw);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, image_raw);

    {
        GLenum error_code;
        if ((error_code = glGetError()) != GL_NO_ERROR) {
            printf("OpenGL Error: %d\n", error_code);
            return -1;
        }
    }
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader_program);
        glBindVertexArray(vertices_vbo);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &texture_id);
    cleanup(window);
}

void setup_vbo(GLuint *vbo, GLshort *vbo_array, GLuint size) {
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, size, vbo_array, GL_STATIC_DRAW);
}

void setup_vao(GLuint *vao) {
    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);
}

uint8_t setup_shaders(GLuint *shader_program) {
    const GLchar *vs_source = "#version 440\n"
                    "layout(location = 0) in vec3 vp;"
                    "layout(location = 1) in vec2 vt;"
                    "out vec2 texcoord;"
                    "void main() { gl_Position = vec4(vp, 1.0);"
                    "texcoord = vt; }", *fs_source = "#version 440\n"
                    "out vec4 frag_color;"
                    "in vec2 texcoord;"
                    "uniform sampler2D texture;"
                    "void main() { frag_color = texture2D(texture, texcoord); }";

    GLint compile_ok = GL_FALSE;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_source, NULL);
    glCompileShader(vs);

    glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok != GL_TRUE) {
        glDeleteShader(vs);
        return 1;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_source, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok != GL_TRUE) {
        glDeleteShader(fs);
        return 1;
    }

    *shader_program = glCreateProgram();
    glAttachShader(*shader_program, fs);
    glAttachShader(*shader_program, vs);
    glLinkProgram(*shader_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    return 0;
}

uint8_t initialize_glfw(void) {
    int major = 0, minor = 0, rev = 0;

    glfwGetVersion(&major, &minor, &rev);
    printf("GLFW %d.%d rev %d\n", major, minor, rev);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, minor);
    glfwWindowHint(GLFW_CONTEXT_REVISION, rev);
    glfwWindowHint(GLFW_SAMPLES, 8);

    if (!glfwInit())
        return 1;

    return 0;
}

void glfw_error_callback(int error, const char *description) {
    printf("glfw error callback triggered\n");
    fprintf(stderr, "%d: %s\n", error, description);
}

void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void cleanup(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
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

