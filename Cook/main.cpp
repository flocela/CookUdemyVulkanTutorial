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
    float twiceAngle = 0.0f;
    float deltaTime = 0.0f;
    float lastTime  = 0.0f;
    
    Circle outlineCircle{4.0f, 0.0f, 0.0f, 2.5, 720};
    
    std::vector<glm::vec3> outlineVertices = outlineCircle.getOutlineVertices();
    std::cout << "outlineVertices size: " << outlineVertices.size() << std::endl;
    for(int ii=0; ii<outlineVertices.size(); ++ii)
    {
        std::cout << "(" << outlineVertices[ii].x << ", " << outlineVertices[ii].y << ")" << std::endl;
    }
    bool stop = false;
    
    // Loop until closed
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        if (!stop)
        {
            float now = glfwGetTime();
            deltaTime = now - lastTime;
            lastTime = now;
            
            angle += 30.0f * deltaTime;
            angle = std::fmod(angle, 360.0f);
            twiceAngle = angle * 3.0f;
            twiceAngle = std::fmod(twiceAngle, 360.0f);
            
            if(angle > 179.5f && angle <180.5f && twiceAngle > 179.5f && twiceAngle < 180.5f)
            {
                angle = 180.0f;
                twiceAngle = 180.0f;
                stop = true;
            }
            
            glm::mat4 zeroithModel(1.0f);
            zeroithModel = glm::translate(zeroithModel, glm::vec3(-3.0f, 0.0f, 0.0001f));
            zeroithModel = glm::rotate(zeroithModel, glm::radians(-angle), glm::vec3(0.0f, 0.0f, 1.0f));
            
            glm::mat4 firstModel(1.0f);
            firstModel = glm::translate(firstModel, glm::vec3(0.0f, 0.0f, 0.001f));
            firstModel = glm::rotate(firstModel, glm::radians(3*angle), glm::vec3(0.0f, 0.0f, 1.0f));
            
            glm::mat4 secondModel(1.0f);
            secondModel = glm::translate(secondModel, glm::vec3(4.0f, 0.0f, 0.01f));
            secondModel = glm::rotate(secondModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
            
            glm::mat4 thirdModel(1.0f);
            
            std::cout << "deltaTime, angle, twiceAngle: " << "(" << deltaTime <<", " << angle << ", " << twiceAngle << ");  (" << outlineVertices[(int)twiceAngle].x << ", " << outlineVertices[(int)twiceAngle].y << ")" << std::endl;
            thirdModel = glm::translate(thirdModel, {cos(glm::radians(twiceAngle))*2.5f + 4.0f, sin(glm::radians(twiceAngle))*2.5f + 0.0f, 0.1f});
            thirdModel = glm::rotate(thirdModel, glm::radians(-twiceAngle), glm::vec3(0.0f, 0.0f, 1.0f));

            vulkanRenderer.updateModel(0, zeroithModel);
            vulkanRenderer.updateModel(1, firstModel);
            vulkanRenderer.updateModel(2, secondModel);
            vulkanRenderer.updateModel(3, thirdModel);

            vulkanRenderer.draw();
        }
    }
    
    vulkanRenderer.cleanup();

    // Destroy GLFW window and stop GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
