#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <array>

#include "Utilities.hpp"

class VulkanRenderer
{
    public:
        VulkanRenderer();

        int init(GLFWwindow * newWindow);
        void draw();
        void cleanup();

        ~VulkanRenderer();

    private:
    
        struct
        {
            VkPhysicalDevice physicalDevice;
            VkDevice logicalDevice;
        }_mainDevice;
        
        int                             _currentFrame = 0;
        GLFWwindow*                     _window;
        VkInstance                      _instance;
        VkDebugUtilsMessengerEXT        _debugMessenger;
        VkQueue                         _graphicsQueue;
        VkQueue                         _presentationQueue;
        VkSurfaceKHR                    _surface;
        VkSwapchainKHR                  _swapchain;
        std::vector<SwapchainImage>     _swapChainImages;
        VkFormat                        _swapChainImageFormat;
        VkPipeline                      _graphicsPipeline;
        VkPipelineLayout                _pipelineLayout;
        VkRenderPass                    _renderPass;
        VkExtent2D                      _swapChainExtent;
        bool                            _enableValidationLayers;
        std::vector<VkFramebuffer>      _swapChainFramebuffers;
        VkCommandPool                   _graphicsCommandPool;
        std::vector<VkCommandBuffer>    _commandBuffers;
        std::vector<VkSemaphore>        _imageAvailable;
        std::vector<VkSemaphore>        _renderFinished;
        std::vector<VkFence>            _drawFences;
        const std::vector<const char *> _validationLayers = {"VK_LAYER_KHRONOS_validation"};
        

        void createInstance();
        void createLogicalDevice();
        void createSurface();
        void createSwapChain();
        void createGraphicsPipeline();
        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSynchronization();
        void getPhysicalDevice();
        void setupDebugMessenger();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void recordCommands();
    
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
        VkShaderModule            createShaderModule(const std::vector<char>& code);
        


};
