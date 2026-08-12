#include "Mesh.h"
#include<RHI/RenderContext.h>
#include <Renderer/MeshResource.h>

NAMESPACE_START
Mesh::Mesh() {

}

void Mesh::createVertexBuffer(unsigned int numVertex, glm::vec3* position, glm::vec3* normal, glm::vec2* uv) {
	/*
	RenderContext* renderContext = RenderContext::getInstance();
	if (!renderContext) {
		return;
	}

	this->vertices = new Vertex[numVertex];
	for (int i = 0; i < numVertex; i++) {
		this->vertices[i].position = position[i];
		this->vertices[i].normal = normal[i];
		this->vertices[i].uv = uv[i];
	}

	if (!vbo) {
		vbo = renderContext->createVertexBuffer(vertices, numVertex * sizeof(Vertex));
	}


	if (!vao) {
		if (ibo != 0) {
			vao = renderContext->createVertexArray(vbo, ibo);
		}
		else {
			vao = renderContext->createVertexArray(vbo);
		}
		glBindVertexArray(6);
		//position
		renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 3, sizeof(Vertex), 0, 0);

		//normal
		renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 3, sizeof(Vertex), 1, 3);

		//uv
		renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 2, sizeof(Vertex), 2, 6);
		glBindVertexArray(6);
	}*/

	//// create buffers/arrays
	//glGenVertexArrays(1, &VAO);
	//glGenBuffers(1, &VBO);
	//
	//glBindVertexArray(VAO);
	//// load data into vertex buffers
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//// A great thing about structs is that their memory layout is sequential for all its items.
	//// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
	//// again translates to 3/2 floats which translates to a byte array.
	//glBufferData(GL_ARRAY_BUFFER, numVertex * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	//// set the vertex attribute pointers
	//// vertex Positions
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	//// vertex normals
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	//// vertex texture coords
	//glEnableVertexAttribArray(2);
	//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	//// vertex tangent
	//glEnableVertexAttribArray(3);
	//glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	//// vertex bitangent
	//glEnableVertexAttribArray(4);
	//glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
	//// ids
	//glEnableVertexAttribArray(5);
	//glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
	//
	//// weights
	//glEnableVertexAttribArray(6);
	//glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
	//glBindVertexArray(0);


}

void Mesh::createIndexBuffer(unsigned int numIndex, unsigned int* indices) {
	/*
	RenderContext* renderContext = RenderContext::getInstance();
	if (!renderContext || !indices) {
		return;
	}
	glBindVertexArray(6);
	GLint currentIBO;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentIBO);
	this->indices = indices;
	indexCount = numIndex;
	if (!ibo) {
		ibo = renderContext->createIndexBuffer(indices, indexCount * sizeof(unsigned int));
	}
	glBindVertexArray(6);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &currentIBO);*/
}

Mesh::~Mesh() {
	// 共享资源模式下，GPU 对象由 MeshResource 释放；fallback 模式才由 Mesh 自己释放。
	if (resource) {
		return;
	}
	if (vao) RenderContext::getInstance()->deleteVertexArray(vao);
	if (vbo) RenderContext::getInstance()->deleteVertexBuffer(vbo);
	if (ibo) RenderContext::getInstance()->deleteIndexBuffer(ibo);
}

void Mesh::setupMesh() {
	// 旧式调用路径仍可直接从 CPU 数组上传；正常 AssetManager 路径会提前绑定 resource。
	if (resource) {
		numVertex = static_cast<unsigned int>(resource->vertexCount());
		return;
	}
	RenderContext* renderContext = RenderContext::getInstance();
	vbo = renderContext->createVertexBuffer(vertices.data(), static_cast<int>(vertices.size() * sizeof(Vertex)));
	ibo = renderContext->createIndexBuffer(indices.data(), static_cast<int>(indices.size() * sizeof(unsigned int)));
	vao = renderContext->createVertexArray(vbo, ibo);
	numVertex = static_cast<unsigned int>(vertices.size());

	// position
	renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 3, sizeof(Vertex), 0, offsetof(Vertex, position) / sizeof(float));

	// normal
	if (hasNormals) renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 3, sizeof(Vertex), 1, offsetof(Vertex, normal) / sizeof(float));

	// uv
	if (hasTexCoords) renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 2, sizeof(Vertex), 2, offsetof(Vertex, texCoords) / sizeof(float));
	
	// tangent
	if (hasTangents) {
		renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 3, sizeof(Vertex), 3, offsetof(Vertex, tangent) / sizeof(float));
		renderContext->setUpVertexBufferLayoutInfo(vbo, vao, 3, sizeof(Vertex), 4, offsetof(Vertex, bitangent) / sizeof(float));
	}
}

void Mesh::draw() {
	// draw 不区分材质和 Transform，Renderer 在调用前负责绑定 Shader/uniform。
	if (resource) {
		resource->draw();
		return;
	}
	RenderContext::getInstance()->bindVertexArray(vao);
	RenderContext::getInstance()->drawElements(static_cast<unsigned int>(indices.size()), nullptr);
	RenderContext::getInstance()->bindVertexArray(0);
}


NAMESPACE_END



