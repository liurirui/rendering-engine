#pragma once

#include"Constants.h"
#include<vector>
#include"Texture2D.h"
#include"Material.h"

#include<glm/vec3.hpp>
#include<glm/vec2.hpp>
#include<string>

NAMESPACE_START

    class Mesh
    {
    public:
        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoords;
            glm::vec3 tangent;
            glm::vec3 bitangent;
        };

        Mesh();
        virtual ~Mesh();

        void setupMesh();

        void draw();

        void createVertexBuffer(unsigned int numVertex, glm::vec3* position, glm::vec3* normal, glm::vec2* uv);

        void createIndexBuffer(unsigned int numIndex, unsigned int* indices);

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        bool hasNormals = false;
        bool hasTexCoords = false;
        bool hasTangents = false;
        unsigned int numVertex = 0;
        unsigned int vao = 0;   // 顶点数组对象
        unsigned int vbo = 0;   // 顶点缓冲
        unsigned int ibo = 0;   // 索引缓冲
        std::string name;
        Material* material = nullptr;
         
    };


NAMESPACE_END

