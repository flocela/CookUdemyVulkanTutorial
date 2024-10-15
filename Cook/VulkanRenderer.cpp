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


VulkanRenderer::~VulkanRenderer()
{}

void VulkanRenderer::createInstance()
{
    if (_enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    
    // Information about the application itself
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
    else{
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

void VulkanRenderer::createSurface()
{
    // Create Surface (creates a surface create info struct, runs the create surface function, returns result)
    VkResult result = glfwCreateWindowSurface(_instance, _window, nullptr, &_surface);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a surface!");
    }
}

void VulkanRenderer::createSwapChain()
{
    // Get Swap Chain details so we can pick best settings
    SwapChainDetails swapChainDetails = getSwapChainDetails(_mainDevice.physicalDevice);

    // Find optimal surface values for our swap chain
    VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentMode = chooseBestPresentMode(swapChainDetails.presentationModes);
    VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

    // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    // If imageCount higher than max, then clamp down to max
    // If 0, then limitless
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
        swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
    {
        imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
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

        vkSwapChainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;        // Image share handling
        vkSwapChainCI.queueFamilyIndexCount = 2;                            // Number of queues to share images between
        vkSwapChainCI.pQueueFamilyIndices = queueFamilyIndices;            // Array of queues to share between
    }
    else
    {
        vkSwapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkSwapChainCI.queueFamilyIndexCount = 0;
        vkSwapChainCI.pQueueFamilyIndices = nullptr;
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
    _swapChainExtent = extent;

    // Populate VkImages
    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(_mainDevice.logicalDevice, _swapchain, &swapChainImageCount, nullptr);
    std::vector<VkImage> vkImages(swapChainImageCount);
    vkGetSwapchainImagesKHR(_mainDevice.logicalDevice, _swapchain, &swapChainImageCount, vkImages.data());

    for (VkImage vkImage : vkImages)
    {
        // Store image handle
        SwapchainImage swapChainImage = {};
        swapChainImage.vkImage = vkImage;
        swapChainImage.vkImageView = createImageView(vkImage, _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        // Add to swapchain image list
        _swapChainImages.push_back(swapChainImage);
    }
    
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
    
    SwapChainDetails swapChainDetails = getSwapChainDetails(physicalDevice);
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

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
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
