#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <set>
#include <array>

#include "stb_image.hpp"
#include "Utilities.hpp"
#include "Mesh.hpp"

class VulkanRenderer
{
    public:
        VulkanRenderer();

        int init(GLFWwindow * newWindow);
        void updateModel(int modelId, glm::mat4 newModel);
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
        VkImageView                     _depthBufferVkImageView;
        VkImage                         _depthBufferVkImage;
        VkDeviceMemory                  _depthBufferImageVkDeviceMemory;
        VkPipeline                      _graphicsPipeline;
        VkPipelineLayout                _pipelineLayout;
        VkRenderPass                    _renderPass;
        VkExtent2D                      _swapChainExtent;
        bool                            _enableValidationLayers;
        std::vector<VkFramebuffer>      _swapChainFramebuffers;
        VkCommandPool                   _graphicsCommandPool;
        std::vector<VkCommandBuffer>    _commandBuffers;
        std::vector<VkSemaphore>        _imageAvailableVkSemaphores;
        std::vector<VkSemaphore>        _renderFinishedVkSemaphores;
        std::vector<VkFence>            _drawVkFences;
        const std::vector<const char *> _validationLayers = {"VK_LAYER_KHRONOS_validation"};
        std::vector<Mesh>               _meshList;
        VkDescriptorSetLayout           _vkDescriptorSetLayout;
        VkDescriptorSetLayout           _vkSamplerDescriptorSetLayout;
        VkPushConstantRange             _pushConstantRange;
        VkDescriptorPool                _descriptorPool;
        VkDescriptorPool                _samplerDescriptorPool;
        std::vector<VkDescriptorSet>    _vkDescriptorSets;
        std::vector<VkDescriptorSet>    _vkSamplerDescriptorSets;
        std::vector<VkBuffer>           _vpUniformBuffer;
        std::vector<VkDeviceMemory>     _vpUniformBufferMemory;
        VkSampler                       _textureSampler;
        std::vector<VkImage>            _vkTextureImages;
        std::vector<VkDeviceMemory>     _vkTextureImageDeviceMemory;
        std::vector<VkImageView>        _vkTextureImageViews;
        
        
        struct UboViewProjection
        {
            glm::mat4 projection;
            glm::mat4 view;
        } _uboViewProjection;
        

        void createInstance();
        void createLogicalDevice();
        void createSurface();
        void createSwapChain();
        void createGraphicsPipeline();
        void createRenderPass();
        void createDescriptorSetLayout();
        void createPushConstantRange();
        void createDepthBufferImage();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void createSynchronization();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void updateUniformBuffers(uint32_t imageIndex);
        void getPhysicalDevice();
        void allocateDynamicBufferTransferSpace();
        void setupDebugMessenger();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
        void recordCommands(uint32_t currentImage);
        void createTextureSampler();
    
        bool                      checkInstanceExtensionSupport(std::vector<const char*> * checkExtensions);
        bool                      checkDeviceExtensionSupport(VkPhysicalDevice device);
        bool                      checkPhysicalDeviceSuitable(VkPhysicalDevice device);
        std::vector<const char *> getRequiredExtensions();
        QueueFamilyIndices        getQueueFamilies(VkPhysicalDevice device);
        SwapChainDetails          getSwapChainDetailsPerPhysicalDevice(VkPhysicalDevice device);
        VkSurfaceFormatKHR        chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR          chooseBestPresentMode(const std::vector<VkPresentModeKHR> presentModes);
        VkExtent2D                chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
        VkFormat                  chooseSupportedFormat(const std::vector<VkFormat> &formats,
                                                        VkImageTiling tiling,
                                                        VkFormatFeatureFlags featureFlags);
        bool                      checkValidationLayerSupport();
        VkImageView               createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        VkShaderModule            createShaderModule(const std::vector<char>& code);
        VkImage                   createImage(uint32_t width,
                                              uint32_t height,
                                              VkFormat format,
                                              VkImageTiling tiling,
                                              VkImageUsageFlags useFlags,
                                              VkMemoryPropertyFlags propFlags,
                                              VkDeviceMemory *imageVkDeviceMemory);
        stbi_uc*                  loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize);
        int                       createTextureImage(std::string fileName);
        int                       createTexture(std::string fileName);
        int                       createTextureDescriptor(VkImageView textureImage);

};
