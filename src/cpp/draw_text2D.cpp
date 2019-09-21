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
#include "controls.hpp"
#include "objloader.hpp"
#include "text2D.hpp"

using namespace std;

GLFWwindow* window;
double scroll_x = 0;
double scroll_y = 0;

// Mouse Wheel callback
void glfwGetMouseWheel(GLFWwindow *w, double x, double y) {
    printf("Scroll: x:%6.2f, y:%6.2f\n", x, y);
    scroll_x += x;
    scroll_y += y;
    return;
}

GLFWwindow* openGLWindow(void) {
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    GLFWwindow* win = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "OpenGL Window", NULL, NULL);
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

    // Initialize GLFW
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
    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(win, glfwGetMouseWheel);

    // background color
    glClearColor(0.1f, 0.1f, 0.3f, 0.0f);

    cout << "Opened a window\n";
    return win;
}

int main(void) {

    if (!(window = openWindow()))
        return -1;

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, WIN_WIDTH/2, WIN_HEIGHT/2);

    // Following two enable Z-buffer so that fragments that cannot be seen are not drawn
    glEnable(GL_DEPTH_TEST); // Enable depth test
    glDepthFunc(GL_LESS);    // Accept fragment if it closer to the camera than the former one
    glEnable(GL_CULL_FACE);  // Cull triangles whose normal is not towards the camera

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLuint programID = LoadShaders("../shaders/TransformVertexShader2.vertexshader", "../shaders/TextureFragmentShader.fragmentshader");
    if (!programID) {
        glfwTerminate();
        return -1;
    }

    GLuint MatrixID  = glGetUniformLocation(programID, "MVP");
    GLuint Texture   = loadDDS("../data/uvmap.DDS");
    GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

    // Read our .obj file
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals; // Won't be used at the moment.
    bool res = loadOBJ("../data/cube.obj", vertices, uvs, normals);

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer); // generate new VBO, return the ID (vertexbuffer). Do once.
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer); // set vertexbuffer as active VBO. Do as many times as need to operate on VBO.
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

    // Initialize text with the Holstein font
    initText2D( "../data/Holstein.DDS" );

    glfwPollEvents();
    glfwSetCursorPos(window, WIN_WIDTH/2.f, WIN_HEIGHT/2.f);
    do {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        // Compute the MVP matrix from keyboard and mouse input
        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();
        glm::mat4 ModelMatrix = glm::mat4(1.0);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        // Send our transformation to the currently bound shader, in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

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
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); // 2 for vec2 UV

        // Draw the cube
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Now render text
        char text[256];
        sprintf(text,"%.2f sec", glfwGetTime() );
        printText2D(text, 50, 600, 100);





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

