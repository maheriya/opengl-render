#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <argp.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"


typedef struct _image {
    GLint width, height, bpp;
    uint32_t current_image, next_image;
} image_t;

void setup_vbo(GLuint*, GLshort*, GLuint);
void setup_vao(GLuint*);
void glfw_error_callback(int, const char*);
void glfw_key_callback(GLFWwindow*, int, int, int, int);
void cleanup(GLFWwindow*);
static int parse_opt(int, char*, struct argp_state*);
uint8_t setup_shaders(GLuint*);
uint8_t initialize_glfw(void);
unsigned char* load_image(char*, image_t*);

int main(int argc, char *argv[]) {
    struct argp_option options[] = { { 0 } };

    static const char* doc = "OpenGL Image Viwer";
    struct argp argp = { options, parse_opt, "[FILE]", doc, 0, 0, 0 };

    int arg_count = 1;
    argp_parse(&argp, argc, argv, 0, 0, &arg_count);

    image_t image = { 0, 0, 0, 1, 2 };
    unsigned char *image_raw = load_image(argv[image.current_image], &image);

    if (!image_raw) {
        printf("stbi error: %s\n", stbi_failure_reason());
        return -1;
    }


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
    if (!(window = glfwCreateWindow(image.width < screen_width ? image.width : screen_width,
                    image.height < screen_height ? image.height : screen_height, "OpenGL Window",
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_raw);
    stbi_image_free(image_raw);

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

unsigned char* load_image(char *name, image_t *image) {
    return stbi_load(name, &image->width, &image->height, &image->bpp, STBI_rgb_alpha);
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

////-///////////////////////////////////////////////////////////////////////////////////////////
////
////-///////////////////////////////////////////////////////////////////////////////////////////
////#include <opencv2/opencv.hpp>
////#include <opencv2/core/opengl.hpp>
////#include <opencv2/core/core.hpp>
////#include <opencv2/highgui/highgui.hpp>
//
//#include <iostream>
//#include <signal.h>
//// Include GLEW. Always before gl.h / glfw3.h
//#include <GL/glew.h>
//#include <GLFW/glfw3.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include "shader.hpp"
//#include "texture.hpp"
//#include "controls.hpp"
//#include "objloader.hpp"
//#include "text2D.hpp"
//
//using namespace std;
//
//GLFWwindow* window;
//double scroll_x = 0;
//double scroll_y = 0;
//
//// Mouse Wheel callback
//void glfwGetMouseWheel(GLFWwindow *w, double x, double y) {
//    printf("Scroll: x:%6.2f, y:%6.2f\n", x, y);
//    scroll_x += x;
//    scroll_y += y;
//    return;
//}
//
//GLFWwindow* openGLWindow(void) {
//    glfwWindowHint(GLFW_SAMPLES, 4);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//    // Open a window and create its OpenGL context
//    GLFWwindow* win = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "OpenGL Window", NULL, NULL);
//    if (win == NULL) {
//        fprintf(stderr,"Failed to open GLFW window.\n");
//        glfwTerminate();
//        return win;
//    }
//    glfwMakeContextCurrent(win);
//    return win;
//}
//
//GLFWwindow* openWindow(void) {
//    GLFWwindow* win;
//
//    // Initialize GLFW
//    if (!glfwInit()) {
//        fprintf(stderr, "Failed to initialize GLFW\n");
//        return NULL;
//    }
//
//    if (!(win = openGLWindow()))
//        return NULL;
//
//    // Initialize GLEW
//    glewExperimental = true; // *Needed* for core profile
//    if (glewInit() != GLEW_OK) {
//        fprintf(stderr, "Failed to initialize GLEW\n");
//        glfwTerminate();
//        return NULL;
//    }
//
//    // Ensure we can capture the escape key
//    glfwSetInputMode(win, GLFW_STICKY_KEYS, GL_TRUE);
//    // Hide the mouse and enable unlimited movement
//    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//    glfwSetScrollCallback(win, glfwGetMouseWheel);
//
//    // background color
//    glClearColor(0.1f, 0.1f, 0.3f, 0.0f);
//
//    cout << "Opened a window\n";
//    return win;
//}
//
//int main(void) {
//
//    if (!(window = openWindow()))
//        return -1;
//
//    // Set the mouse at the center of the screen
//    glfwPollEvents();
//    glfwSetCursorPos(window, WIN_WIDTH/2, WIN_HEIGHT/2);
//
//    // Following two enable Z-buffer so that fragments that cannot be seen are not drawn
//    glEnable(GL_DEPTH_TEST); // Enable depth test
//    glDepthFunc(GL_LESS);    // Accept fragment if it closer to the camera than the former one
//    glEnable(GL_CULL_FACE);  // Cull triangles whose normal is not towards the camera
//
//    GLuint VertexArrayID;
//    glGenVertexArrays(1, &VertexArrayID);
//    glBindVertexArray(VertexArrayID);
//
//    GLuint programID = LoadShaders("../shaders/TransformVertexShader2.vertexshader", "../shaders/TextureFragmentShader.fragmentshader");
//    if (!programID) {
//        glfwTerminate();
//        return -1;
//    }
//
//    GLuint MatrixID  = glGetUniformLocation(programID, "MVP");
//    GLuint Texture   = loadDDS("../data/uvmap.DDS");
//    GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");
//
//    // Read our .obj file
//    std::vector<glm::vec3> vertices;
//    std::vector<glm::vec2> uvs;
//    std::vector<glm::vec3> normals; // Won't be used at the moment.
//    bool res = loadOBJ("../data/cube.obj", vertices, uvs, normals);
//
//    GLuint vertexbuffer;
//    glGenBuffers(1, &vertexbuffer); // generate new VBO, return the ID (vertexbuffer). Do once.
//    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer); // set vertexbuffer as active VBO. Do as many times as need to operate on VBO.
//    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
//
//    GLuint uvbuffer;
//    glGenBuffers(1, &uvbuffer);
//    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
//    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
//
//    // Initialize text with the Holstein font
//    initText2D( "../data/Holstein.DDS" );
//
//    glfwPollEvents();
//    glfwSetCursorPos(window, WIN_WIDTH/2.f, WIN_HEIGHT/2.f);
//    do {
//        // Clear the screen
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//        // Use our shader
//        glUseProgram(programID);
//
//        // Compute the MVP matrix from keyboard and mouse input
//        computeMatricesFromInputs();
//        glm::mat4 ProjectionMatrix = getProjectionMatrix();
//        glm::mat4 ViewMatrix = getViewMatrix();
//        glm::mat4 ModelMatrix = glm::mat4(1.0);
//        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
//
//        // Send our transformation to the currently bound shader, in the "MVP" uniform
//        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
//
//        // Bind our texture in Texture Unit 0
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, Texture);
//        // Set our "myTextureSampler" sampler to use Texture Unit 0
//        glUniform1i(TextureID, 0);
//
//        // 1st Attribute buffer : vertices
//        glEnableVertexAttribArray(0);
//        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
//        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // 3 for vec3 vertex
//
//        // 2nd attribute buffer : UVs
//        glEnableVertexAttribArray(1);
//        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
//        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); // 2 for vec2 UV
//
//        // Draw the cube
//        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
//
//        glDisableVertexAttribArray(0);
//        glDisableVertexAttribArray(1);
//
//        // Now render text
//        char text[256];
//        sprintf(text,"%.2f sec", glfwGetTime() );
//        printText2D(text, 50, 600, 100);
//
//
//
//
//
//        // Swap buffers (update display)
//        glfwSwapBuffers(window);
//        glfwPollEvents();
//
//    } // Check if the ESC key was pressed or the window was closed
//    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
//
//    // Cleanup VBO
//    glDeleteBuffers(1, &vertexbuffer);
//    glDeleteBuffers(1, &uvbuffer);
//    glDeleteProgram(programID);
//    glDeleteTextures(1, &Texture);
//    glDeleteVertexArrays(1, &VertexArrayID);
//
//    // Close OpenGL window and terminate GLFW
//    glfwTerminate();
//
//    return 0;
//}
//
