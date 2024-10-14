#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Utilities.hpp"

class VulkanRenderer
{
    public:
        VulkanRenderer();

        int init(GLFWwindow * newWindow);
        void cleanup();

        ~VulkanRenderer();

    private:
        struct
        {
            VkPhysicalDevice physicalDevice;
            VkDevice logicalDevice;
        }_mainDevice;
        
        GLFWwindow* _window;
        VkInstance  _instance;
        VkQueue     _graphicsQueue;


        // Vulkan Functions
        // - Create Functions
        void createInstance();
        void createLogicalDevice();

        // - Get Functions
        void getPhysicalDevice();

        // - Support Functions
        // -- Checker Functions
        bool checkInstanceExtensionSupport(std::vector<const char*> * checkExtensions);
        bool checkDeviceSuitable(VkPhysicalDevice device);

        // -- Getter Functions
        QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

};
