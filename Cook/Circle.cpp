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
    float multiplier = .5f/_radius;
    _vertices.push_back({ {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 0.5f} });
    for(int ii=0; ii<_numOfT; ++ii)
    {
        float x = cos(ii*radiansPerTriangle) * _radius;
        float y = sin(ii*radiansPerTriangle) * _radius;
        //cout << ii << ": " << ( (x/(2*_radius)) + _radius ) << endl;
        _vertices.push_back({ { x, y, 0.0f },
                              { 1.0f, 0.0f, 0.0f },
                              { ( 1.0f - ((x*multiplier) + 0.5f) ),
                                ((y*multiplier) + 0.5f)}
                            });
                            
        std::cout << "(" << _vertices[ii].pos[0] << ", " << _vertices[ii].pos[1] << ", " << _vertices[ii].pos[2] << ") (" << _vertices[ii].tex[0] << ", " << _vertices[ii].tex[1] << ")" <<endl;
        
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

vector<Vertex> Circle::getVertices ()
{
    for(Vertex v : _vertices)
    {
        std::cout << "(" << v.pos[0] << ", " << v.pos[1] << ", " << v.pos[2] << ") (" << v.tex[0] << ", " << v.tex[1] << ")" <<endl;
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
