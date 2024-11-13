#include "Circle.hpp"
#include <iostream>
#include <cmath>
#include <numbers>

const float  PI_F=3.14159265358979f;
using namespace std;

Circle::Circle (float x, float y, float z, float radius, uint32_t numOfTriangles)
:   _x{x}, _y{y}, _z{z}, _radius{radius}, _numOfT{numOfTriangles}
{
    float radiansPerTriangle = 2.0f * PI_F/_numOfT;
    float multiplier = .5f/_radius;
    _vertices.push_back({ {0.0f + _x, 0.0f + _y, _z}, {1.0f, 0.0f, 0.0f}, {0.5f, 0.5f} });
    for(int ii=0; ii<_numOfT; ++ii)
    {
        float x = (cos(ii*radiansPerTriangle) * _radius);
        float y = (sin(ii*radiansPerTriangle) * _radius);
        _vertices.push_back({ { x + _x, y + _y, _z },
                              { 1.0f, 0.0f, 0.0f },
                              { ( 1.0f - ((x*multiplier) + 0.5f) ),
                                ((y*multiplier) + 0.5f)}
                            });
        int index = ii+1;
        if(index < _numOfT)
        {
            _indices.push_back(0);
            _indices.push_back(index);
            _indices.push_back(index+1);
        }
        else
        {
            _indices.push_back(0);
            _indices.push_back(index);
            _indices.push_back(1);
        }
        
    }
}

vector<glm::vec3>  Circle::getOutlineVertices()
{
    vector<glm::vec3> outline(_vertices.size()-1);
    for(int ii=1; ii<_vertices.size(); ++ii)
    {
        outline[ii-1] = _vertices[ii].pos;
    }
    return outline;
}

vector<Vertex> Circle::getVertices ()
{
    return _vertices;
}

vector<uint32_t> Circle::getIndices()
{
    return _indices;
}
