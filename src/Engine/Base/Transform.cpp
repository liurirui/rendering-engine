#include"Transform.h"
NAMESPACE_START
Transform::Transform() {}

Transform::~Transform() {}

void Transform::setTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
	modelMatrix = glm::mat4(1.0f);
	Position = position;
	Rotation = rotation;
	Scale = scale;
	modelMatrix = glm::translate(modelMatrix, position);
	modelMatrix = glm::scale(modelMatrix, Scale);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.z), glm::vec3(0, 0, 1)); // Roll
	modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.y), glm::vec3(0, 1, 0)); // Yaw
	modelMatrix = glm::rotate(modelMatrix, glm::radians(Rotation.x), glm::vec3(1, 0, 0)); // Pitch
}

NAMESPACE_END