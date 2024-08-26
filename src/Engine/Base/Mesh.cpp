#include "Mesh.h"
#include<RHI/RenderContext.h>

NAMESPACE_START
Mesh::Mesh() {

}

void Mesh::createVertextBuffer(unsigned int numVertex, glm::vec3* position, glm::vec3* normal, glm::vec2* uv) {

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

	if (!vertexBufferID) {
		vertexBufferID = renderContext->createVertexBuffer(vertices, numVertex * sizeof(Vertex));
	}

	if (!vertexAttributeBufferID) {
		vertexAttributeBufferID = renderContext->createVertexBufferLayoutInfo(vertexBufferID);
		//position
		renderContext->setUpVertexBufferLayoutInfo(vertexBufferID, vertexAttributeBufferID, 3, sizeof(Vertex), 0, 0);

		//normal
		renderContext->setUpVertexBufferLayoutInfo(vertexBufferID, vertexAttributeBufferID, 3, sizeof(Vertex), 1, 3);

		//uv
		renderContext->setUpVertexBufferLayoutInfo(vertexBufferID, vertexAttributeBufferID, 2, sizeof(Vertex), 2, 6);
	}

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

void Mesh::createTriangleIndexBuffer(unsigned int numTriangle, unsigned int* indices) {

	RenderContext* renderContext = RenderContext::getInstance();
	if (!renderContext) {
		return;
	}

	this->numTriangle = numTriangle;
	this->indices = indices;
	if (!indexBufferID) {
		indexBufferID = renderContext->createIndexBuffer(indices, numTriangle * 3 * sizeof(unsigned int));
	}
}

Mesh::~Mesh() {

	if (vertices) {
		delete[] vertices;
		vertices = nullptr;
	}

	if (indices) {
		delete[] indices;
		indices = nullptr;
	}

	//glDeleteVertexArrays(1, &cubeVAO);
	//glDeleteVertexArrays(1, &lightCubeVAO);
	//glDeleteBuffers(1, &VBO);

}

NAMESPACE_END



