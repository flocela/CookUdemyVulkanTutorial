#include "VulkanRenderer.hpp"
#include <iostream>
#include <algorithm>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT     messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT            messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void                                       *pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance                               instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks              *pAllocator,
    VkDebugUtilsMessengerEXT                 *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance,
      "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance                  instance,
    VkDebugUtilsMessengerEXT    debugMessenger,
    const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance,
      "vkDestroyDebugUtilsMessengerEXT");
    
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

VulkanRenderer::VulkanRenderer()
{}

VulkanRenderer::~VulkanRenderer()
{}
//
int VulkanRenderer::init(GLFWwindow * newWindow)
{
    _window = newWindow;

    try
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        getPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        std::vector<Vertex> meshVertices = {
            { { -0.1, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },   // 0
            { { -0.1, 0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } },    // 1
            { { -0.9, 0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
            { { -0.9, -0.4, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
        };

        std::vector<Vertex> meshVertices2 = {
            { { 0.9, -0.3, 0.0 },{ 1.0f, 0.0f, 0.0f } },   // 0
            { { 0.9, 0.3, 0.0 },{ 0.0f, 1.0f, 0.0f } },    // 1
            { { 0.1, 0.3, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
            { { 0.1, -0.3, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
        };

        // Index Data
        std::vector<uint32_t> meshIndices = {
            0, 1, 2,
            2, 3, 0
        };
        Mesh firstMesh = Mesh(_mainDevice.physicalDevice,
                              _mainDevice.logicalDevice,
                              _graphicsQueue,
                              _graphicsCommandPool,
                              &meshVertices, &meshIndices);
        Mesh secondMesh = Mesh(_mainDevice.physicalDevice,
                               _mainDevice.logicalDevice,
                               _graphicsQueue,
                               _graphicsCommandPool,
                               &meshVertices2, &meshIndices);
        
        _meshList.push_back(firstMesh);
        _meshList.push_back(secondMesh);
        
        createCommandBuffers();
        recordCommands();
        createSynchronization();
    }
    catch (const std::runtime_error &e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

void VulkanRenderer::cleanup()
{
    vkDeviceWaitIdle(_mainDevice.logicalDevice);
    for (size_t i = 0; i < _meshList.size(); i++)
    {
        _meshList[i].destroyBuffers();
    }
    
    for(size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        vkDestroySemaphore(_mainDevice.logicalDevice, _renderFinishedVkSemaphores[i], nullptr);
        vkDestroySemaphore(_mainDevice.logicalDevice, _imageAvailableVkSemaphores[i], nullptr);
        vkDestroyFence(_mainDevice.logicalDevice, _drawVkFences[i], nullptr);
    }
    
    
    vkDestroyCommandPool(_mainDevice.logicalDevice, _graphicsCommandPool, nullptr);
    for(VkFramebuffer framebuffer : _swapChainFramebuffers)
    {
        vkDestroyFramebuffer(_mainDevice.logicalDevice, framebuffer, nullptr);
    }
    vkDestroyPipeline(_mainDevice.logicalDevice, _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(_mainDevice.logicalDevice, _pipelineLayout, nullptr);
    vkDestroyRenderPass(_mainDevice.logicalDevice, _renderPass, nullptr);
    for (auto swapchainImage : _swapChainImages)
    {
        vkDestroyImageView(_mainDevice.logicalDevice, swapchainImage.vkImageView, nullptr);
    }
    vkDestroySwapchainKHR(_mainDevice.logicalDevice, _swapchain, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_mainDevice.logicalDevice, nullptr);
    
    if (_enableValidationLayers)
    {
    DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }
    
    vkDestroyInstance(_instance, nullptr);
}

void VulkanRenderer::createInstance()
{
    if (_enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    
    // Information about the application itself.
    // Most data here doesn't affect the program and is for developer convenience
    VkApplicationInfo appInfo = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Vulkan App";                  // Custom name of the application
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);      // Custom version of the application
    appInfo.pEngineName        = "No Engine";                   // Custom engine name
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);      // Custom engine version
    appInfo.apiVersion         = VK_API_VERSION_1_0;            // The Vulkan Version

    // Creation information for a VkInstance (Vulkan Instance)
    VkInstanceCreateInfo instanceCI = {};
    instanceCI.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.flags                = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; // macOS fix
    instanceCI.pApplicationInfo     = &appInfo;

    // Create list to hold instance extensions
    std::vector<const char*> instanceExtensions = std::vector<const char*>();

    // Set up extensions Instance will use
    uint32_t glfwExtensionCount = 0;                // GLFW may require multiple extensions
    const char** glfwExtensions;                    // Extensions passed as array of cstrings, so need pointer (the array) to pointer (the cstring)

    // Get GLFW extensions
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Add GLFW extensions to list of extensions
    for (size_t i = 0; i < glfwExtensionCount; i++)
    {
        instanceExtensions.push_back(glfwExtensions[i]);
    }
    
    instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME); // macOS fix
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME); // macOS fix

    // Check Instance Extensions supported...
    if (!checkInstanceExtensionSupport(&instanceExtensions))
    {
        throw std::runtime_error("VkInstance does not support required extensions!");
    }

    instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCI.ppEnabledExtensionNames = instanceExtensions.data();

    if (_enableValidationLayers)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        instanceCI.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size());
        instanceCI.ppEnabledLayerNames = _validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        instanceCI.enabledLayerCount = 0;
        instanceCI.ppEnabledLayerNames = nullptr;
    }
    
    // Create instance
    VkResult result = vkCreateInstance(&instanceCI, nullptr, &_instance);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vulkan Instance!");
    }
}

void VulkanRenderer::createSurface()
{
    // Create Surface (creates a surface create info struct, runs the create surface function, returns result)
    VkResult result = glfwCreateWindowSurface(_instance, _window, nullptr, &_surface);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a surface!");
    }
}

void VulkanRenderer::createLogicalDevice()
{
    //Get the queue family indices for the chosen Physical Device
    QueueFamilyIndices indices = getQueueFamilies(_mainDevice.physicalDevice);
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = {indices.graphicsFamily, indices.presentationFamily};

    // Queue the logical device needs to create and info to do so (Only 1 for now, will add more later!)
    for (int queueFamilyIndex : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;                // The index of the family to create a queue from
        queueCreateInfo.queueCount = 1;                                     // Number of queues to create
        float priority = 1.0f;
        queueCreateInfo.pQueuePriorities = &priority;                       // Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)
        queueCreateInfos.push_back(queueCreateInfo);
    }
    

    // Information to create logical device (sometimes called "device")
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount =  static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    // Physical Device Features the Logical Device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // Create the logical device for the given physical device
    VkResult result = vkCreateDevice(_mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &_mainDevice.logicalDevice);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Logical Device!");
    }

    // Queues are created at the same time as the device...
    // So we want handle to queues
    // From given logical device, of given Queue Family, of given Queue Index (0 since only one queue), place reference in given VkQueue
    vkGetDeviceQueue(_mainDevice.logicalDevice, indices.graphicsFamily, 0, &_graphicsQueue);
    vkGetDeviceQueue(_mainDevice.logicalDevice, indices.presentationFamily, 0, &_presentationQueue);
}

void VulkanRenderer::getPhysicalDevice()
{
    // Enumerate Physical devices the vkInstance can access
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    // If no devices available, then none support Vulkan!
    if (deviceCount == 0)
    {
        throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");
    }

    // Get list of Physical Devices
    std::vector<VkPhysicalDevice> physicalDeviceList(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDeviceList.data());

    for (const auto &physicalDevice : physicalDeviceList)
    {
        if (checkPhysicalDeviceSuitable(physicalDevice))
        {
            _mainDevice.physicalDevice = physicalDevice;
            break;
        }
    }
}

void VulkanRenderer::createSwapChain()
{
    // Get Swap Chain details so we can pick best settings
    SwapChainDetails swapChainDetails = getSwapChainDetailsPerPhysicalDevice(_mainDevice.physicalDevice);

    // Find optimal surface values for our swap chain
    VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentMode     = chooseBestPresentMode(swapChainDetails.presentationModes);
    VkExtent2D extent                = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

    // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    if(swapChainDetails.surfaceCapabilities.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, swapChainDetails.surfaceCapabilities.maxImageCount);
    }

    // Creation information for swap chain
    VkSwapchainCreateInfoKHR vkSwapChainCI = {};
    vkSwapChainCI.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkSwapChainCI.surface          = _surface;                                              // Swapchain surface
    vkSwapChainCI.imageFormat      = surfaceFormat.format;                                  // Swapchain format
    vkSwapChainCI.imageColorSpace  = surfaceFormat.colorSpace;                              // Swapchain colour space
    vkSwapChainCI.presentMode      = presentMode;                                           // Swapchain presentation mode
    vkSwapChainCI.imageExtent      = extent;                                                // Swapchain image extents
    vkSwapChainCI.minImageCount    = imageCount;                                            // Minimum images in swapchain
    vkSwapChainCI.imageArrayLayers = 1;                                                     // Number of layers for each image in chain
    vkSwapChainCI.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                   // What attachment images will be used as
    vkSwapChainCI.preTransform     = swapChainDetails.surfaceCapabilities.currentTransform; // Transform to perform on swapchain images
    vkSwapChainCI.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                     // How to handle blending images
    vkSwapChainCI.clipped          = VK_TRUE;                                               // Whether to clip parts of image

    // Get Queue Family Indices
    QueueFamilyIndices indices = getQueueFamilies(_mainDevice.physicalDevice);

    // If Graphics and Presentation families are different, then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        // Queues to share between
        uint32_t queueFamilyIndices[] = {
            (uint32_t)indices.graphicsFamily,
            (uint32_t)indices.presentationFamily
        };

        vkSwapChainCI.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;   // Image share handling
        vkSwapChainCI.queueFamilyIndexCount = 2;                            // Number of queues to share images between
        vkSwapChainCI.pQueueFamilyIndices   = queueFamilyIndices;           // Array of queues to share between
    }
    else
    {
        vkSwapChainCI.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        vkSwapChainCI.queueFamilyIndexCount = 0;
        vkSwapChainCI.pQueueFamilyIndices   = nullptr;
    }

    // IF old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
    vkSwapChainCI.oldSwapchain = VK_NULL_HANDLE;

    // Create Swapchain
    VkResult result = vkCreateSwapchainKHR(_mainDevice.logicalDevice, &vkSwapChainCI, nullptr, &_swapchain);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Swapchain!");
    }

    // Store for later reference
    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent      = extent;

    // Populate VkImages
    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(_mainDevice.logicalDevice, _swapchain, &swapChainImageCount, nullptr);
    std::vector<VkImage> vkImages(swapChainImageCount);
    vkGetSwapchainImagesKHR(_mainDevice.logicalDevice, _swapchain, &swapChainImageCount, vkImages.data());

    for (VkImage vkImage : vkImages)
    {
        // Store image handle
        SwapchainImage swapChainImage = {};
        swapChainImage.vkImage        = vkImage;
        swapChainImage.vkImageView    = createImageView(vkImage, _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        // Add to swapchain image list
        _swapChainImages.push_back(swapChainImage);
    }
}

void VulkanRenderer::createGraphicsPipeline()
{
    // Read in SPIR-V code of shaders
    auto vertexShaderCode   = readFile("/Users/flo/LocalDocuments/Projects/VulkanLearning/Cook/Cook/shaders/simple_shader.vert.spv");
    auto fragmentShaderCode = readFile("/Users/flo/LocalDocuments/Projects/VulkanLearning/Cook/Cook/shaders/simple_shader.frag.spv");

    // Create Shader Modules
    VkShaderModule vertexShaderModule   = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    // -- SHADER STAGE CREATION INFORMATION --
    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCI   = {};
    vertexShaderCI.sType                             = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCI.stage                             = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCI.module                            = vertexShaderModule;
    vertexShaderCI.pName                             = "main";

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCI = {};
    fragmentShaderCI.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCI.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCI.module                          = fragmentShaderModule;
    fragmentShaderCI.pName                           = "main";

    // Put shader stage creation info in to array
    // Graphics Pipeline creation info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCI, fragmentShaderCI };

    // CREATE PIPELINE
    
    // How the data for a single vertex (including info such as position, color, texture coords, normals, ...etc) is as a whole.
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding                         = 0;
    bindingDescription.stride                          = sizeof(Vertex);
    // Choose between VK_VERTEX_INPUT_RATE_INDEX and VK_VERTEX_INPUT_RATE_INSTANCE
    bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
    
    // VkVertexInputBindingDescription has many VkVertexInputAttributeDescriptions.
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
    attributeDescriptions[0].binding                   = 0;       // Which binding the data is at. (should be the same as above.)
    attributeDescriptions[0].location                  = 0;       // Location in shader where data will be read from.
    attributeDescriptions[0].format                    = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset                    = offsetof(Vertex, pos);
    
    attributeDescriptions[1].binding                   = 0;
    attributeDescriptions[1].location                  = 1;
    attributeDescriptions[1].format                    = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset                    = offsetof(Vertex, col);
    
    // -- VERTEX INPUT (TODO: Put in vertex descriptions when resources created) --
    VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
    vertexInputCI.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCI.vertexBindingDescriptionCount        = 1;
    vertexInputCI.pVertexBindingDescriptions           = &bindingDescription; // List of Vertex Binding Descriptions (data spacing/stride information)
    vertexInputCI.vertexAttributeDescriptionCount      = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCI.pVertexAttributeDescriptions         = attributeDescriptions.data(); // List of Vertex Attribute Descriptions (data format and where to bind to/from)

    // -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI = {};
    inputAssemblyCI.sType                               = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCI.topology                            = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCI.primitiveRestartEnable              = VK_FALSE;

    // -- VIEWPORT & SCISSOR --
    // Create a viewport info struct
    VkViewport viewport = {};
    viewport.x        = 0.0f;                            // x start coordinate
    viewport.y        = 0.0f;                            // y start coordinate
    viewport.width    = (float)_swapChainExtent.width;   // width of viewport
    viewport.height   = (float)_swapChainExtent.height;  // height of viewport
    viewport.minDepth = 0.0f;                            // min framebuffer depth
    viewport.maxDepth = 1.0f;                            // max framebuffer depth

    // Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset   = { 0,0 };                          // Offset to use region from
    scissor.extent   = _swapChainExtent;                 // Extent to describe region to use, starting at offset

    VkPipelineViewportStateCreateInfo viewportStateCI = {};
    viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCI.viewportCount                     = 1;
    viewportStateCI.pViewports                        = &viewport;
    viewportStateCI.scissorCount                      = 1;
    viewportStateCI.pScissors                         = &scissor;


    // -- DYNAMIC STATES --
    // Dynamic states to enable
    //std::vector<VkDynamicState> dynamicStateEnables;
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);    // Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);    // Dynamic Scissor    : Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

    //// Dynamic State creation info
    //VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    //dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    //dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

    // -- RASTERIZER --
    VkPipelineRasterizationStateCreateInfo rasterizerCI = {};
    rasterizerCI.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCI.depthClampEnable        = VK_FALSE;
    rasterizerCI.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCI.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizerCI.lineWidth               = 1.0f;
    rasterizerCI.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizerCI.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizerCI.depthBiasEnable         = VK_FALSE;

    // -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisamplingCI = {};
    multisamplingCI.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCI.sampleShadingEnable                  = VK_FALSE;
    multisamplingCI.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

    // -- BLENDING --
    // Blending decides how to blend a new colour being written to a fragment, with the old value

    // Blend Attachment State (how blending is handled)
    VkPipelineColorBlendAttachmentState colourState = {};
    colourState.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT |
                                                      VK_COLOR_COMPONENT_G_BIT |    // Colours to apply blending to
                                                      VK_COLOR_COMPONENT_B_BIT |
                                                      VK_COLOR_COMPONENT_A_BIT;
    colourState.blendEnable                         = VK_TRUE;
    colourState.srcColorBlendFactor                 = VK_BLEND_FACTOR_SRC_ALPHA;
    colourState.dstColorBlendFactor                 = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colourState.colorBlendOp                        = VK_BLEND_OP_ADD;

    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
    //               (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

    colourState.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
    colourState.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
    colourState.alphaBlendOp                        = VK_BLEND_OP_ADD;
    // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

    VkPipelineColorBlendStateCreateInfo colourBlendingCI = {};
    colourBlendingCI.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlendingCI.logicOpEnable   = VK_FALSE;
    colourBlendingCI.attachmentCount = 1;
    colourBlendingCI.pAttachments    = &colourState;


    // -- PIPELINE LAYOUT (TODO: Apply Future Descriptor Set Layouts) --
    VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
    pipelineLayoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount         = 0;
    pipelineLayoutCI.pSetLayouts            = nullptr;
    pipelineLayoutCI.pushConstantRangeCount = 0;
    pipelineLayoutCI.pPushConstantRanges    = nullptr;

    VkResult result = vkCreatePipelineLayout(_mainDevice.logicalDevice, &pipelineLayoutCI, nullptr, &_pipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Pipeline Layout!");
    }


    // -- DEPTH STENCIL TESTING --
    // TODO: Set up depth stencil testing


    // -- GRAPHICS PIPELINE CREATION --
    VkGraphicsPipelineCreateInfo pipelineCI = {};
    pipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount          = 2;                     // Number of shader stages
    pipelineCI.pStages             = shaderStages;          // List of shader stages
    pipelineCI.pVertexInputState   = &vertexInputCI;        // All the fixed function pipeline states
    pipelineCI.pInputAssemblyState = &inputAssemblyCI;
    pipelineCI.pViewportState      = &viewportStateCI;
    pipelineCI.pDynamicState       = nullptr;
    pipelineCI.pRasterizationState = &rasterizerCI;
    pipelineCI.pMultisampleState   = &multisamplingCI;
    pipelineCI.pColorBlendState    = &colourBlendingCI;
    pipelineCI.pDepthStencilState  = nullptr;
    pipelineCI.layout              = _pipelineLayout;      // Pipeline Layout pipeline should use
    pipelineCI.renderPass          = _renderPass;          // Render pass description the pipeline is compatible with
    pipelineCI.subpass             = 0;                    // Subpass of render pass to use with pipeline

    // Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
    pipelineCI.basePipelineHandle  = VK_NULL_HANDLE;       // Existing pipeline to derive from...
    pipelineCI.basePipelineIndex   = -1;                     // or index of pipeline being created to derive from (in case creating multiple at once)

    result = vkCreateGraphicsPipelines(_mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &_graphicsPipeline);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Graphics Pipeline!");
    }
    
    // Destroy Shader Modules, no longer needed after Pipeline created
    vkDestroyShaderModule(_mainDevice.logicalDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(_mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::createRenderPass()
{
    // Colour attachment of render pass
    VkAttachmentDescription colourAttachment = {};
    colourAttachment.format         = _swapChainImageFormat;        // Format to use for attachment
    colourAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;        // Number of samples to write for multisampling
    colourAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;       // Describes what to do with attachment before rendering
    colourAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;      // Describes what to do with attachment after rendering
    colourAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Describes what to do with stencil before rendering
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colourAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;     // Image data layout before render pass starts
    colourAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Image data layout after render pass (to change to)

    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colourAttachmentReference = {};
    colourAttachmentReference.attachment            = 0;
    colourAttachmentReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Information about a particular subpass the Render Pass is using
    VkSubpassDescription subpass                    = {};
    subpass.pipelineBindPoint                       = VK_PIPELINE_BIND_POINT_GRAPHICS; // Pipeline type subpass is to be bound to
    subpass.colorAttachmentCount                    = 1;
    subpass.pColorAttachments                       = &colourAttachmentReference;

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after...
    subpassDependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL; // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;  // Pipeline stage
    subpassDependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;            // Stage access mask (memory access)
    // But must happen before...
    subpassDependencies[0].dstSubpass      = 0;
    subpassDependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;

    // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass      = 0;
    subpassDependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // But must happen before...
    subpassDependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments    = &colourAttachment;
    renderPassCreateInfo.subpassCount    = 1;
    renderPassCreateInfo.pSubpasses      = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies   = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(_mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &_renderPass);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Render Pass!");
    }
}

void VulkanRenderer::createFramebuffers()
{
    // Resize framebuffer count to equal swap chain image count
    _swapChainFramebuffers.resize(_swapChainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < _swapChainFramebuffers.size(); i++)
    {
        std::array<VkImageView, 1> vkImageViewAttachments = {
            _swapChainImages[i].vkImageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass      = _renderPass;   // Render Pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(vkImageViewAttachments.size());
        framebufferCreateInfo.pAttachments    = vkImageViewAttachments.data(); // List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width           = _swapChainExtent.width;
        framebufferCreateInfo.height          = _swapChainExtent.height;
        framebufferCreateInfo.layers          = 1;

        VkResult result = vkCreateFramebuffer(_mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &_swapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Framebuffer!");
        }
    }
}

void VulkanRenderer::createCommandPool()
{
    // Get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(_mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex        = queueFamilyIndices.graphicsFamily;    // matches Queue Family type

    // Create a Graphics Queue Family Command Pool
    VkResult result = vkCreateCommandPool(_mainDevice.logicalDevice, &poolInfo, nullptr, &_graphicsCommandPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Command Pool!");
    }
}

void VulkanRenderer::createCommandBuffers()
{
    // Resize command buffer count to have one for each framebuffer
    _commandBuffers.resize(_swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool                 = _graphicsCommandPool;
    cbAllocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount          = static_cast<uint32_t>(_commandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(_mainDevice.logicalDevice, &cbAllocInfo, _commandBuffers.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Command Buffers!");
    }
}

void VulkanRenderer::recordCommands()
{
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo vkCommandBufferBI = {};
    vkCommandBufferBI.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //vkCommandBufferBI.flags                  = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;    // Buffer can be resubmitted when it has already been submitted and is awaiting execution. Not needed. Only have 2 frame draws.

    // Information about how to begin a render pass (only needed for graphical applications)
    VkClearValue clearValues[] = {{0.6f, 0.65f, 0.4, 1.0f}};
    VkRenderPassBeginInfo vkRenderPassBI       = {};
    vkRenderPassBI.sType                       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vkRenderPassBI.renderPass                  = _renderPass;
    vkRenderPassBI.renderArea.offset           = { 0, 0 };
    vkRenderPassBI.renderArea.extent           = _swapChainExtent;
    vkRenderPassBI.pClearValues                = clearValues;  // (TODO: Depth Attachment Clear Value)
    vkRenderPassBI.clearValueCount             = 1;

    for (size_t i = 0; i < _commandBuffers.size(); i++)
    {
        vkRenderPassBI.framebuffer = _swapChainFramebuffers[i];

        // Start recording commands to command buffer!
        VkResult result = vkBeginCommandBuffer(_commandBuffers[i], &vkCommandBufferBI);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to start recording a Command Buffer!");
        }

        vkCmdBeginRenderPass(_commandBuffers[i], &vkRenderPassBI, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
        for (size_t j = 0; j < _meshList.size(); j++)
        {
            VkBuffer vertexBuffers[] = { _meshList[j].getVertexBuffer() };                // Buffers to bind
            VkDeviceSize offsets[] = { 0 };                                               // Offsets into buffers being bound
            vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);    // Command to bind vertex buffer before drawing with them

            // Bind mesh index buffer, with 0 offset and using the uint32 type
            vkCmdBindIndexBuffer(_commandBuffers[i], _meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            // Execute pipeline
            vkCmdDrawIndexed(_commandBuffers[i], _meshList[j].getIndexCount(), 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(_commandBuffers[i]);

        // Stop recording to command buffer
        result = vkEndCommandBuffer(_commandBuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to stop recording a Command Buffer!");
        }
    }
}

void VulkanRenderer::createSynchronization()
{
    _imageAvailableVkSemaphores.resize(MAX_FRAME_DRAWS);
    _renderFinishedVkSemaphores.resize(MAX_FRAME_DRAWS);
    _drawVkFences              .resize(MAX_FRAME_DRAWS);

    // Semaphore creation information
    VkSemaphoreCreateInfo semaphoreCI = {};
    semaphoreCI.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence creation information
    VkFenceCreateInfo fenceCI = {};
    fenceCI.sType                     = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags                     = VK_FENCE_CREATE_SIGNALED_BIT; // Fence starts off open.
    

    for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
    {
        if (vkCreateSemaphore(_mainDevice.logicalDevice, &semaphoreCI, nullptr, &_imageAvailableVkSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_mainDevice.logicalDevice, &semaphoreCI, nullptr, &_renderFinishedVkSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence    (_mainDevice.logicalDevice, &fenceCI, nullptr, &_drawVkFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Semaphore and/or Fence!");
        }
    }
}
//
void VulkanRenderer::draw()
{
    // -- GET NEXT IMAGE --
    // Wait for given fence to signal (open) from last draw before continuing
    vkWaitForFences(_mainDevice.logicalDevice, 1, &_drawVkFences[_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    vkResetFences(_mainDevice.logicalDevice, 1, &_drawVkFences[_currentFrame]);

    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex;
    vkAcquireNextImageKHR(_mainDevice.logicalDevice, _swapchain, std::numeric_limits<uint64_t>::max(), _imageAvailableVkSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue submission information
    VkSubmitInfo submitInfo           = {};
    submitInfo.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount     = 1;                                             // Number of semaphores to wait on
    submitInfo.pWaitSemaphores        = &_imageAvailableVkSemaphores[_currentFrame];   // List of semaphores to wait on
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // Can't do this stage until .pWaitSemaphores are open.
    submitInfo.pWaitDstStageMask      = waitStages;                                    // Stages to check semaphores at
    submitInfo.commandBufferCount     = 1;                                             // Number of command buffers to submit
    submitInfo.pCommandBuffers        = &_commandBuffers[imageIndex];                  // Command buffer to submit
    submitInfo.signalSemaphoreCount   = 1;                                             // Number of semaphores to signal
    submitInfo.pSignalSemaphores      = &_renderFinishedVkSemaphores[_currentFrame];   // Semaphores to signal command buffer finished

    // Submit command buffer to queue, signal fence when finished.
    VkResult result = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _drawVkFences[_currentFrame]);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Command Buffer to Queue!");
    }

    // -- PRESENT RENDERED IMAGE TO SCREEN --
    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;                                        // Number of semaphores to wait on
    presentInfo.pWaitSemaphores    = &_renderFinishedVkSemaphores[_currentFrame];          // Semaphores to wait on
    presentInfo.swapchainCount     = 1;                                        // Number of swapchains to present to
    presentInfo.pSwapchains        = &_swapchain;                              // Swapchains to present images to
    presentInfo.pImageIndices      = &imageIndex;                              // Index of images in swapchains to present

    // Present image
    result = vkQueuePresentKHR(_presentationQueue, &presentInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present Image!");
    }

    // Get next frame (use % MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
    _currentFrame = (_currentFrame + 1) % MAX_FRAME_DRAWS;
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
    // Need to get number of extensions to create array of correct size to hold extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // Create a list of VkExtensionProperties using count
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    // Check if given extensions are in list of available extensions
    for (const auto &checkExtension : *checkExtensions)
    {
        bool hasExtension = false;
        for (const auto &extension : extensions)
        {
            if (strcmp(checkExtension, extension.extensionName))
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    // Get device extension count
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // If no extensions found, return failure
    if (extensionCount == 0)
    {
        return false;
    }

    std::vector<VkExtensionProperties> actualDeviceExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, actualDeviceExtensions.data());

    for (const auto &requiredDeviceExtension : requiredDeviceExtensions)
    {
        bool hasExtension = false;
        for (const auto &extension : actualDeviceExtensions)
        {
            if (strcmp(requiredDeviceExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            return false;
        }
    }

    return true;
}

bool VulkanRenderer::checkPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    /*
    // Information about the device itself (ID, name, type, vendor, etc)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Information about what the device can do (geo shader, tess shader, wide lines, etc)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    */

    bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);
    if(!extensionsSupported)
    {
        return false;
    }
    
    SwapChainDetails swapChainDetails = getSwapChainDetailsPerPhysicalDevice(physicalDevice);
    bool swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
    if(!swapChainValid)
    {
        return false;
    }
    
    QueueFamilyIndices indices = getQueueFamilies(physicalDevice);
    return indices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
    
    // Get all Queue Family Property info for the given device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    // Go through each queue family and check if it has at least 1 of the required types of queue
    QueueFamilyIndices indices;
    int i = 0;
    for (const auto& queueFamily : queueFamilyList)
    {
        // First check if queue family has at least 1 queue in that family (could have no queues)
        // Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if has required type
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;        // If queue family is valid, then get index
        }
        
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentationSupport);
        if(queueFamily.queueCount > 0 && presentationSupport)
        {
            indices.presentationFamily = i;
        }
        
        // Check if queue family indices are in a valid state, stop searching if so
        if (indices.isValid())
        {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainDetails VulkanRenderer::getSwapChainDetailsPerPhysicalDevice(VkPhysicalDevice device)
{
    SwapChainDetails swapChainDetails;

    // -- CAPABILITIES --
    // Get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &swapChainDetails.surfaceCapabilities);

    // -- FORMATS --
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    // If formats returned, get list of formats
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, swapChainDetails.formats.data());
    }

    // -- PRESENTATION MODES --
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentationCount, nullptr);

    // If presentation modes returned, get list of presentation modes
    if (presentationCount != 0)
    {
        swapChainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentationCount, swapChainDetails.presentationModes.data());
    }

    return swapChainDetails;
}

void VulkanRenderer::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;  // Optional
}

void VulkanRenderer::setupDebugMessenger()
{
    if (!_enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

bool VulkanRenderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : _validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If restricted, search for optimal format
    for (const auto &format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // If can't find optimal format, then just return first format
    return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentMode(const std::vector<VkPresentModeKHR> presentModes)
{
    // Look for Mailbox presentation mode
    for (const auto &presentationMode : presentModes)
    {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentationMode;
        }
    }

    // If can't find, use FIFO as Vulkan spec says it must be present
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
    // If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        // If value can vary, need to set manually

        // Get window size
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        // Create new extent using window size
        VkExtent2D newExtent = {};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        // Surface also defines max and min, so make sure within boundaries by clamping value
        newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

        return newExtent;
    }
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo vkImageViewCI = {};
    vkImageViewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkImageViewCI.image                           = image;                         // Image to create view for
    vkImageViewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;         // Type of image (1D, 2D, 3D, Cube, etc)
    vkImageViewCI.format                          = format;                        // Format of image data
    vkImageViewCI.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vkImageViewCI.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vkImageViewCI.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    vkImageViewCI.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources allow the view to view only a part of an image
    vkImageViewCI.subresourceRange.aspectMask     = aspectFlags;  // Aspect of image to view (e.g. COLOR_BIT for viewing colour)
    vkImageViewCI.subresourceRange.baseMipLevel   = 0;            // Start mipmap level to view from
    vkImageViewCI.subresourceRange.levelCount     = 1;            // Number of mipmap levels to view
    vkImageViewCI.subresourceRange.baseArrayLayer = 0;            // Start array level to view from
    vkImageViewCI.subresourceRange.layerCount     = 1;            // Number of array levels to view

    VkImageView imageView;
    VkResult result = vkCreateImageView(_mainDevice.logicalDevice, &vkImageViewCI, nullptr, &imageView);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image View!");
    }

    return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
    // Shader Module creation information
    VkShaderModuleCreateInfo shaderModuleCI = {};
    shaderModuleCI.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCI.codeSize                 = code.size();
    shaderModuleCI.pCode                    = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(_mainDevice.logicalDevice, &shaderModuleCI, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a shader module!");
    }

    return shaderModule;
}
