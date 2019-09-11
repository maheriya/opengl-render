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

using namespace std;
using namespace glm;

GLFWwindow* window;
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

int main(void) {
    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    if (!(window = openGLWindow()))
        return -1;

    // Initialize GLEW
	glewExperimental = true; // *Needed* for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // background color
    glClearColor(0.0f, 0.4f, 0.4f, 0.0f);

    cout << "Opened a window\n";
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    //GLuint programID = LoadShaders("SimpleVertexShader.vertexshader", "SingleColor.fragmentshader" );
    GLuint programID = LoadShaders("SimpleTransform.vertexshader", "SingleColor.fragmentshader" );
    if (!programID) {
        glfwTerminate();
        return -1;
    }

    // Get a handle for our "MVP" uniform
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(35.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    // Or, for an ortho camera :
    //glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates

    // Camera matrix
    glm::mat4 View = glm::lookAt(glm::vec3(4,3,3), // Camera is at (4,3,3), in World Space
                                 glm::vec3(0,0,0), // and looks at the origin
                                 glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                                 );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP   = Projection * View * Model; // Remember, matrix multiplication is the other way around

    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         0.0f,  1.0f, 0.0f,
    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);


    do {
        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // 1st Attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0,         // match with shader
                              3,         // size (three vertices)
                              GL_FLOAT,  // type
                              GL_FALSE,  // normalized?
                              0,         // stride
                              (void*)0   // array buffer offset
        );

        // Draw the triangle using the vertexbuffer, g_vertex_buffer_data
        glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices, start at 0 (will create on triangle)
        glDisableVertexAttribArray(0);

        // Swap buffers (update display)
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

