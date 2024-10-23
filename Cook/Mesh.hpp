#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.hpp"

class Mesh
{
    public:
        Mesh();
        Mesh(VkPhysicalDevice newPhysicalDevice,
                 VkDevice newDevice,
                 VkQueue transferQueue,
                 VkCommandPool transferCommandPool,
                 std::vector<Vertex>* vertices,
                 std::vector<uint32_t> * indices);

        int getVertexCount();
        VkBuffer getVertexBuffer();
    
        int getIndexCount();
        VkBuffer getIndexBuffer();

        void destroyBuffers();

        ~Mesh();

    private:
        int              _vertexCount;
        int              _indexCount;
        VkBuffer         _vertexVkBuffer;
        VkDeviceMemory   _vertexVkDeviceMemory;
        VkBuffer         _indexVkBuffer;
        VkDeviceMemory   _indexVkDeviceMemory;
    
        VkPhysicalDevice _physicalDevice;
        VkDevice         _device;

        void createVertexBuffer(VkQueue transferQueue,
                                VkCommandPool transferCommandPool,
                                std::vector<Vertex>* vertices);
    
        void createIndexBuffer(VkQueue transferQueue,
                               VkCommandPool transferCommandPool,
                               std::vector<uint32_t>* indices);
};