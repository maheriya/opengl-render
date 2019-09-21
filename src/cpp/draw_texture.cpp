//-///////////////////////////////////////////////////////////////////////////////////////////
//
//-///////////////////////////////////////////////////////////////////////////////////////////
//#include <opencv2/opencv.hpp>
//#include <opencv2/core/opengl.hpp>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <signal.h>
// Include GLEW. Always before gl.h / glfw3.h
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.hpp"
#include "texture.hpp"

using namespace std;
using namespace glm;

GLFWwindow* openGLWindow(void) {
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    GLFWwindow* win = glfwCreateWindow(1280, 720, "OpenGL Window", NULL, NULL);
    if (win == NULL) {
        fprintf(stderr,"Failed to open GLFW window.\n");
        glfwTerminate();
        return win;
    }
    glfwMakeContextCurrent(win);
    return win;
}

GLFWwindow* openWindow(void) {
    GLFWwindow* win;

    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return NULL;
    }

    if (!(win = openGLWindow()))
        return NULL;

    // Initialize GLEW
    glewExperimental = true; // *Needed* for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        return NULL;
    }

    // Ensure we can capture the escape key
    glfwSetInputMode(win, GLFW_STICKY_KEYS, GL_TRUE);

    // background color
    glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

    cout << "Opened a window\n";
    return win;
}
int main(void) {
    GLFWwindow* window;

    if (!(window = openWindow()))
        return -1;

    // Following two enable Z-buffer so that fragments that cannot be seen are not drawn
    glEnable(GL_DEPTH_TEST); // Enable depth test
    glDepthFunc(GL_LESS);    // Accept fragment if it closer to the camera than the former one


    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLuint programID = LoadShaders("../shaders/TransformVertexShader2.vertexshader", "../shaders/TextureFragmentShader.fragmentshader");
    if (!programID) {
        glfwTerminate();
        return -1;
    }

    // Get a handle for our "MVP" uniform
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    // Projection matrix : 45° Field of View, 16:9 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    // Camera matrix
    glm::mat4 View = glm::lookAt(glm::vec3(4,3,3), // Camera is at (4,3,3), in World Space
                                 glm::vec3(0,0,0), // and looks at the origin
                                 glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                                 );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection
    glm::mat4 MVP1   = Projection * View * Model;

    // Load the texture using any two methods
    //GLuint Texture = loadBMP_custom("../data/uvtemplate.bmp");
    GLuint Texture = loadDDS("../data/uvtemplate.DDS");

    // Get a handle for our "myTextureSampler" uniform
    GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");


    // A cube. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
    // A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f,-1.0f,-1.0f, // triangle 1 : begin
        -1.0f,-1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f, // triangle 1 : end
         1.0f, 1.0f,-1.0f, // triangle 2 : begin
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f, // triangle 2 : end
         1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,
         1.0f, 1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
         1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,
         1.0f,-1.0f,-1.0f,
         1.0f, 1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,
         1.0f, 1.0f, 1.0f,
         1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,
         1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f,
         1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
         1.0f,-1.0f, 1.0f
    };

    // Two UV coordinates for each vertex. They were created with Blender.
    static const GLfloat g_uv_buffer_data[] = {
        0.000059f, 0.000004f,
        0.000103f, 0.336048f,
        0.335973f, 0.335903f,
        1.000023f, 0.000013f,
        0.667979f, 0.335851f,
        0.999958f, 0.336064f,
        0.667979f, 0.335851f,
        0.336024f, 0.671877f,
        0.667969f, 0.671889f,
        1.000023f, 0.000013f,
        0.668104f, 0.000013f,
        0.667979f, 0.335851f,
        0.000059f, 0.000004f,
        0.335973f, 0.335903f,
        0.336098f, 0.000071f,
        0.667979f, 0.335851f,
        0.335973f, 0.335903f,
        0.336024f, 0.671877f,
        1.000004f, 0.671847f,
        0.999958f, 0.336064f,
        0.667979f, 0.335851f,
        0.668104f, 0.000013f,
        0.335973f, 0.335903f,
        0.667979f, 0.335851f,
        0.335973f, 0.335903f,
        0.668104f, 0.000013f,
        0.336098f, 0.000071f,
        0.000103f, 0.336048f,
        0.000004f, 0.671870f,
        0.336024f, 0.671877f,
        0.000103f, 0.336048f,
        0.336024f, 0.671877f,
        0.335973f, 0.335903f,
        0.667969f, 0.671889f,
        1.000004f, 0.671847f,
        0.667979f, 0.335851f
        //
//        0.000059f, 1.0f-0.000004f,
//        0.000103f, 1.0f-0.336048f,
//        0.335973f, 1.0f-0.335903f,
//        1.000023f, 1.0f-0.000013f,
//        0.667979f, 1.0f-0.335851f,
//        0.999958f, 1.0f-0.336064f,
//        0.667979f, 1.0f-0.335851f,
//        0.336024f, 1.0f-0.671877f,
//        0.667969f, 1.0f-0.671889f,
//        1.000023f, 1.0f-0.000013f,
//        0.668104f, 1.0f-0.000013f,
//        0.667979f, 1.0f-0.335851f,
//        0.000059f, 1.0f-0.000004f,
//        0.335973f, 1.0f-0.335903f,
//        0.336098f, 1.0f-0.000071f,
//        0.667979f, 1.0f-0.335851f,
//        0.335973f, 1.0f-0.335903f,
//        0.336024f, 1.0f-0.671877f,
//        1.000004f, 1.0f-0.671847f,
//        0.999958f, 1.0f-0.336064f,
//        0.667979f, 1.0f-0.335851f,
//        0.668104f, 1.0f-0.000013f,
//        0.335973f, 1.0f-0.335903f,
//        0.667979f, 1.0f-0.335851f,
//        0.335973f, 1.0f-0.335903f,
//        0.668104f, 1.0f-0.000013f,
//        0.336098f, 1.0f-0.000071f,
//        0.000103f, 1.0f-0.336048f,
//        0.000004f, 1.0f-0.671870f,
//        0.336024f, 1.0f-0.671877f,
//        0.000103f, 1.0f-0.336048f,
//        0.336024f, 1.0f-0.671877f,
//        0.335973f, 1.0f-0.335903f,
//        0.667969f, 1.0f-0.671889f,
//        1.000004f, 1.0f-0.671847f,
//        0.667979f, 1.0f-0.335851f

    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);


    do {
        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        // Send our transformation to the currently bound shader, in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(TextureID, 0);

        // 1st Attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // 3 for vec3 vertex

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0); // 2 for vec2 UV

        // Draw the cube
        glDrawArrays(GL_TRIANGLES, 0, 12*3); // 12*3 indices, start at 0 (will create on triangle)

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Swap buffers (update display)
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteProgram(programID);
    glDeleteTextures(1, &Texture);
    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

