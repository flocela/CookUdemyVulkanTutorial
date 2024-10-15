#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>

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
        
        
        GLFWwindow*                     _window;
        VkInstance                      _instance;
        VkDebugUtilsMessengerEXT        _debugMessenger;
        VkQueue                         _graphicsQueue;
        VkQueue                         _presentationQueue;
        VkSurfaceKHR                    _surface;
        VkSwapchainKHR                  _swapchain;
        std::vector<SwapchainImage>     _swapChainImages;
        VkFormat                        _swapChainImageFormat;
        VkExtent2D                      _swapChainExtent;
        bool                            _enableValidationLayers;
        const std::vector<const char *> _validationLayers = {"VK_LAYER_KHRONOS_validation"};
        

        void createInstance();
        void createLogicalDevice();
        void createSurface();
        void createSwapChain();
        void getPhysicalDevice();
        void setupDebugMessenger();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    
        bool                      checkInstanceExtensionSupport(std::vector<const char*> * checkExtensions);
        bool                      checkDeviceExtensionSupport(VkPhysicalDevice device);
        bool                      checkPhysicalDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char *> getRequiredExtensions();
        QueueFamilyIndices        getQueueFamilies(VkPhysicalDevice device);
        SwapChainDetails          getSwapChainDetailsPerPhysicalDevice(VkPhysicalDevice device);
        VkSurfaceFormatKHR        chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR          chooseBestPresentMode(const std::vector<VkPresentModeKHR> presentModes);
        VkExtent2D                chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
        bool                      checkValidationLayerSupport();
        VkImageView               createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        


};
