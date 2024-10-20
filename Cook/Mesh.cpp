#include "Mesh.hpp"


Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice,
           VkDevice newDevice,
           VkQueue transferQueue,
           VkCommandPool transferCommandPool,
           std::vector<Vertex>* vertices,
           std::vector<uint32_t> * indices)
{
    _vertexCount = static_cast<int>(vertices->size());
    _indexCount = static_cast<int>(indices->size());
    _physicalDevice = newPhysicalDevice;
    _device = newDevice;
    createVertexBuffer(transferQueue, transferCommandPool, vertices);
    createIndexBuffer(transferQueue, transferCommandPool, indices);
}

int Mesh::getVertexCount()
{
    return _vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
    return _vertexVkBuffer;
}

int Mesh::getIndexCount()
{
    return _indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
    return _indexVkBuffer;
}

void Mesh::destroyBuffers()
{
    vkDestroyBuffer(_device, _vertexVkBuffer, nullptr);
    vkFreeMemory(_device, _vertexVkDeviceMemory, nullptr);
    vkDestroyBuffer(_device, _indexVkBuffer, nullptr);
    vkFreeMemory(_device, _indexVkDeviceMemory, nullptr);
}


Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
    // Get size of buffer needed for vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

    // Temporary buffer to "stage" vertex data before transferring to GPU
    VkBuffer stagingVkBuffer;
    VkDeviceMemory stagingVkDeviceMemory;

    // Create Staging Buffer and Allocate Memory to it
    createBuffer(_physicalDevice, _device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingVkBuffer, &stagingVkDeviceMemory);

    // MAP MEMORY TO VERTEX BUFFER
    void * data; // 1. Create pointer to a point in normal memory
    vkMapMemory(_device,
                stagingVkDeviceMemory,
                0, bufferSize,
                0, &data); // 2. "Map" the vertex buffer memory to that point
    memcpy(data,
           vertices->data(),
           (size_t)bufferSize); // 3. Copy memory from vertices vec to the point
    
    vkUnmapMemory(_device, stagingVkDeviceMemory); // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not CPU (host)
    createBuffer(_physicalDevice,
                 _device, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &_vertexVkBuffer,
                 &_vertexVkDeviceMemory);

    // Copy staging buffer to vertex buffer on GPU
    copyBuffer(_device, transferQueue, transferCommandPool, stagingVkBuffer, _vertexVkBuffer, bufferSize);

    // Clean up staging buffer parts
    vkDestroyBuffer(_device, stagingVkBuffer, nullptr);
    vkFreeMemory(_device, stagingVkDeviceMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
    // Get size of buffer needed for indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();
    
    // Temporary buffer to "stage" index data before transferring to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(_physicalDevice,
                 _device, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingBufferMemory);

    // MAP MEMORY TO INDEX BUFFER
    void * data;
    vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices->data(), (size_t)bufferSize);
    vkUnmapMemory(_device, stagingBufferMemory);

    // Create buffer for INDEX data on GPU access only area
    createBuffer(_physicalDevice, _device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_indexVkBuffer, &_indexVkDeviceMemory);

    // Copy from staging buffer to GPU access buffer
    copyBuffer(_device, transferQueue, transferCommandPool, stagingBuffer, _indexVkBuffer, bufferSize);

    // Destroy + Release Staging Buffer resources
    vkDestroyBuffer(_device, stagingBuffer, nullptr);
    vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

