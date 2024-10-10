#pragma once
#include "Object.h"
#include <RHI/RenderContext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
NAMESPACE_START
class Transform {
public:
	Transform();
	~Transform();
	//Update data and model matrix
	void setTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);

	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
	glm::mat4 modelMatrix=glm::mat4(1.0f);
};
NAMESPACE_END