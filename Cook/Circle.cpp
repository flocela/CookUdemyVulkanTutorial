#include "Circle.hpp"
#include <iostream>
#include <cmath>
#include <numbers>

const float  PI_F=3.14159265358979f;
using namespace std;

Circle::Circle (float radius, uint32_t numOfTriangles)
:   _radius{radius}, _numOfT{numOfTriangles}
{
    float radiansPerTriangle = 2.0f * PI_F/_numOfT;
    
    _vertices.push_back({ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} });
    for(int ii=0; ii<_numOfT; ++ii)
    {
        _vertices.push_back({ { cos(ii*radiansPerTriangle) * _radius,
                                -sin(ii*radiansPerTriangle) * _radius, // y points downard.
                                0.0f },
                                { 1.0f, 0.0f, 0.0f } });
        
        if(ii < _numOfT-1)
        {
            _indices.push_back(0);
            _indices.push_back(ii+2);
            _indices.push_back(ii+1);
        }
        else
        {
            _indices.push_back(0);
            _indices.push_back(1);
            _indices.push_back(ii+1);
        }
        
    }
    
}

vector<Vertex> Circle::getVertices ()
{
    for(Vertex v : _vertices)
    {
        std::cout << v.pos[0] << ", " << v.pos[1] << ", " << v.pos[2] << endl;
    }
    
    cout << endl << endl;
    
    return _vertices;
}

vector<uint32_t> Circle::getIndices()
{
    int count = 0;
    for(uint32_t x: _indices)
    {
        std::cout << x;
        
        ++count;
        if (count%3 == 0)
        {
            cout << endl;
        }
    }
    return _indices;
}
