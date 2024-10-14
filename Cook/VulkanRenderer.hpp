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
        
        bool                            _enableValidationLayers;
        GLFWwindow*                     _window;
        VkInstance                      _instance;
        VkDebugUtilsMessengerEXT        _debugMessenger;
        VkQueue                         _graphicsQueue;
        const std::vector<const char *> _validationLayers = {"VK_LAYER_KHRONOS_validation"};

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
    
        std::vector<const char *> getRequiredExtensions();

        // -- Getter Functions
        QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
    
        bool checkValidationLayerSupport();
        void setupDebugMessenger();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);


};
