#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.hpp"

class Mesh
{
    public:
        Mesh();
        Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex> * vertices);

        int getVertexCount();
        VkBuffer getVertexBuffer();

        void destroyVertexBuffer();

        ~Mesh();

    private:
        int              _vertexCount;
        VkBuffer         _vkBuffer;
        VkDeviceMemory   _vkDeviceMemory;

        VkPhysicalDevice _physicalDevice;
        VkDevice         _device;

        void createVertexBuffer(std::vector<Vertex> * vertices);
    
        uint32_t findMemoryTypeIndex(uint32_t requiredTypes, VkMemoryPropertyFlags properties);
};
