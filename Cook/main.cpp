#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.hpp"

GLFWwindow* window;
VulkanRenderer vulkanRenderer;

void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
{
    // Initialise GLFW
    glfwInit();

    // Set GLFW to NOT work with OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}
//
int main()
{
    // Create Window
    initWindow("Test Window", 1366, 768);

    // Create Vulkan Renderer instance
    if (vulkanRenderer.init(window) == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    float angle     = 0.0f;
    float deltaTime = 0.0f;
    float lastTime  = 0.0f;
    
    // Loop until closed
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        float now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;
        
        angle += 10.0f * deltaTime;
        if(angle > 360.0f)
        {
            angle -= 360.0f;
        }
        glm::mat4 firstModel(1.0f);
        glm::mat4 secondModel(1.0f);
//
        firstModel = glm::translate(firstModel, glm::vec3(-1.0f, 0.0f, -1.5f)); // red
        firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

        secondModel = glm::translate(secondModel, glm::vec3(1.0f, 0.0f, -3.0f)); // blue
        secondModel = glm::rotate(secondModel, glm::radians(-angle * 25), glm::vec3(0.0f, 0.0f, 1.0f));

        vulkanRenderer.updateModel(0, firstModel);
        vulkanRenderer.updateModel(1, secondModel);

        vulkanRenderer.draw();
    }

    vulkanRenderer.cleanup();

    // Destroy GLFW window and stop GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
