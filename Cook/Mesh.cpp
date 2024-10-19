#include "Mesh.hpp"


Mesh::Mesh()
{
}
Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
    _vertexCount = static_cast<int>(vertices->size());
    _physicalDevice = newPhysicalDevice;
    _device = newDevice;
    createVertexBuffer(vertices);
}

int Mesh::getVertexCount()
{
    return _vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
    return _vkBuffer;
}

void Mesh::destroyVertexBuffer()
{
    vkDestroyBuffer(_device, _vkBuffer, nullptr);
    vkFreeMemory(_device, _vkDeviceMemory, nullptr);
}


Mesh::~Mesh()
{
}

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
    // CREATE VkBuffer, which will hold Vertex struct data
    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo vkBufferCI = {};
    vkBufferCI.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCI.size               = sizeof(Vertex) * vertices->size(); // Size of buffer (size of 1 vertex * number of vertices)
    vkBufferCI.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Multiple types of buffer possible, we want Vertex Buffer
    vkBufferCI.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;         // Similar to Swap Chain images, can share vertex buffers

    VkResult result = vkCreateBuffer(_device, &vkBufferCI, nullptr, &_vkBuffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vertex Buffer!");
    }

    // Get buffer memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _vkBuffer, &memRequirements);

    // Allocate memory to buffer.
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize  = memRequirements.size;
         // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT    : CPU can interact with memory.
         //VK_MEMORY_PROPERTY_HOST_COHERENT_BIT    : Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
    memoryAllocInfo.memoryTypeIndex = 
        findMemoryTypeIndex(memRequirements.memoryTypeBits, // Index of memory type on Physical Device that has required bit flags
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    // ALLOCATE MEMORY TO VKDEVICEMEMORY
    result = vkAllocateMemory(_device, &memoryAllocInfo, nullptr, &_vkDeviceMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
    }

    // Bind vkBuffer and vkDeviceMemory
    vkBindBufferMemory(_device, _vkBuffer, _vkDeviceMemory, 0);

    // MAP *data TO _vkDeviceMemory
    void * data;                                                                // 1. Create pointer to a point in normal memory
    vkMapMemory(_device, _vkDeviceMemory, 0, vkBufferCI.size, 0, &data);        // 2. "Map" the vertex buffer memory to that point
    
    // COPY vertices VECTOR TO data.
    memcpy(data, vertices->data(), (size_t)vkBufferCI.size);                    // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(_device, _vkDeviceMemory);                                    // 4. Unmap the vertex buffer memory
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t requiredTypes, VkMemoryPropertyFlags requiredProperties)
{
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((requiredTypes & (1 << i)) &&                       // Index of memory type must match corresponding bit in requiredTypes
            (memoryProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties)    // Desired property bit flags are part of memory type's property flags
        {
            // This memory type is valid, so return its index
            return i;
        }
    }
    return -1;
}
