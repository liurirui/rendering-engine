#pragma once

#include"Constants.h"
#include<vector>

#include<glm/vec3.hpp>
#include<glm/vec2.hpp>
#include<string>

NAMESPACE_START

    class Mesh
    {
    public:

        Mesh();
        virtual ~Mesh();

        void createVertextBuffer(unsigned int numVertex, glm::vec3* position, glm::vec3* normal, glm::vec2* uv);

        void createTriangleIndexBuffer(unsigned int numTriangle, unsigned int* indices);

        struct Vertex {
            // position
            glm::vec3 position;
            // normal
            glm::vec3 normal;
            // texCoords
            glm::vec2 uv;
            // tangent
            glm::vec3 tangent;
        };

        std::string nowName;
        bool isTramslucent = false;
        int modelNumber=0;

        unsigned int numVertex = 0;
        Vertex * vertices = nullptr;
        unsigned int vertexBufferID = 0;
        unsigned int vertexAttributeBufferID = 0;

        unsigned int numTriangle = 0;
        unsigned int* indices = nullptr;
        unsigned int indexBufferID = 0;

    };


NAMESPACE_END

