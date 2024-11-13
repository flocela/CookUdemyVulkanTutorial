#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <chrono>
#include <thread>

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.hpp"
#include "Circle.hpp"

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
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
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
    
    Circle outlineCircle{4.0f, 0.0f, 0.0f, 2.5, 720};
    
    std::vector<Vertex> outlineVertices = outlineCircle.getVertices();
    std::cout << "outlineVertices size: " << outlineVertices.size() << std::endl;
    for(int ii=0; ii<outlineVertices.size(); ++ii)
    {
        std::cout << "(" << outlineVertices[ii].pos.x << ", " << outlineVertices[ii].pos.y << ")" << std::endl;
    }
    int outlineCounter = 0;
    
    // Loop until closed
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        //
        float now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;
        
        angle += 20.0f * deltaTime;
        if(angle > 720.0f)
        {
            std::cout << " -760" << std::endl;
            angle -= 760.0f;
        }
        
        glm::mat4 zeroithModel(1.0f);
        zeroithModel = glm::translate(zeroithModel, glm::vec3(-3.0f, 0.0f, 0.0f));
        zeroithModel = glm::rotate(zeroithModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
        
        glm::mat4 firstModel(1.0f);
        firstModel = glm::translate(firstModel, glm::vec3(0.0f, 0.0f, 0.0f));
        firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
        
        glm::mat4 secondModel(1.0f);
        secondModel = glm::translate(secondModel, glm::vec3(4.0f, 0.0f, 0.0f));
        secondModel = glm::rotate(secondModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
        
        glm::mat4 thirdModel(1.0f);
        float twiceAngle = angle * 1.5f;
        if(twiceAngle > 720.0f){twiceAngle -= 720.0;}
        std::cout << "deltaTime, angle, twiceAngle: " << "(" << deltaTime <<", " << angle << ", " << twiceAngle << ");  (" << outlineVertices[(int)twiceAngle].pos.x << ", " << outlineVertices[(int)twiceAngle].pos.y << ")" << std::endl;
        thirdModel = glm::translate(thirdModel, outlineVertices[(int)twiceAngle].pos);
        std::cout << "angle: " << angle << ": " << std::endl;
        
        //thirdModel = glm::rotate(thirdModel, glm::radians(3*angle), glm::vec3(0.0f, 0.0f, 1.0f));

        vulkanRenderer.updateModel(0, zeroithModel);
        vulkanRenderer.updateModel(1, firstModel);
        vulkanRenderer.updateModel(2, secondModel);
        vulkanRenderer.updateModel(3, thirdModel);

        vulkanRenderer.draw();
    }
    
    vulkanRenderer.cleanup();

    // Destroy GLFW window and stop GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
