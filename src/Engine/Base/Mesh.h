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

        void createVertexBuffer(unsigned int numVertex, glm::vec3* position, glm::vec3* normal, glm::vec2* uv);

        void createIndexBuffer(unsigned int numIndex, unsigned int* indices);

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

        unsigned int numVertex = 0;
        Vertex* vertices = nullptr;
        unsigned int vao = 0;   // 顶点数组对象
        unsigned int vbo = 0;   // 顶点缓冲
        unsigned int ibo = 0;   // 索引缓冲

        unsigned int indexCount = 0;
        unsigned int* indices = nullptr;

    };


NAMESPACE_END

