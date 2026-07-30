#include "Triangle.h"

// std 
#include <glad/glad.h>  // CRUCIAL: GLAD must always be included first!
#include <GLFW/glfw3.h>
#include <iostream>

// FUNCTIONS:
// Function to handle window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Input function 
void processInput(GLFWwindow* window) {
    // if the user presses the Escape key tell glfw the window to close
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// APP 
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW (OpenGL 3.3, Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the window and define it 
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window Renderer", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLAD before calling any OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Initialize object Triangle 
    Triangle myTriangle;

    // Render loop every frame update 
    while (!glfwWindowShouldClose(window)) {

        // Process inputs first thing every frame 
        processInput(window);

        // Clear the screen with a dark green colour
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // store color RGBA Dark Green value in memory 
        glClear(GL_COLOR_BUFFER_BIT); // bit or light weight switch , take array RGBA and overwrite every single pixel inside the achive window color buffer

        // Draw triangle every frame 
        myTriangle.Draw();

        // Swap buffers and poll OS events (keys pressed, mouse moved)
        glfwSwapBuffers(window); // Swap the buffer out 
        glfwPollEvents();
    }

    // Clean up and exit
    glfwTerminate();
    return 0;
}
