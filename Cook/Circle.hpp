#ifndef Circle_hpp
#define Circle_hpp

#include <vector>
#include "Utilities.hpp"

class Circle
{
    public:
        Circle(float x, float y, float z, float radius, uint32_t numOfTriangles);
        Circle() = delete;
        Circle(const Circle& o) = default;
        Circle(Circle&& o) noexcept = default;
        Circle& operator=(const Circle& o) = default;
        Circle& operator=(Circle&& o) noexcept = default;
    
        std::vector<Vertex> getVertices();
        std::vector<uint32_t> getIndices();
        std::vector<glm::vec3>  getOutlineVertices();
        
    private:
        std::vector<Vertex>   _vertices{};
        uint32_t              _numOfT; // number of triangles
        float                 _radius;
        std::vector<uint32_t> _indices;
        float                 _x;
        float                 _y;
        float                 _z;
    
};


#endif
