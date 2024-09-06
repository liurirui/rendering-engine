#pragma once
#include "Object.h"
#include <glm/glm.hpp>
NAMESPACE_START
enum class LightType {
    Direction,
    Point,
    Spot
};
class Light {
public:
    //initialization
    Light(LightType type,const glm::vec3& color, float intensity) : type(type),color(color), intensity(intensity) {}

    //get light type
    LightType getType() const { return type; }

    //get light color
    void setColor(const glm::vec3& color) { this->color = color; }
    glm::vec3 getColor() const { return color; }

    //get light intensity
    void setIntensity(float intensity) { this->intensity = intensity; }
    float getIntensity() const { return intensity; }

protected:
    LightType type;       // �������
    glm::vec3 color;      // �����ɫ
    float intensity;      // ���ǿ��
};


class DirectionLight : public Light {
public:
    DirectionLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
        : Light(LightType::Direction, color, intensity), direction(direction) {}

    void setDirection(const glm::vec3& direction) { this->direction = direction; }
    glm::vec3 getDirection() const { return direction; }

private:
    glm::vec3 direction;  // �����ķ���
};


class PointLight :public Light {
public:
    PointLight(const glm::vec3& position, const glm::vec3& color, float intensity)
        : Light(LightType::Point,color, intensity), position(position), constant(1.0f), linear(0.09f), quadratic(0.032f) {}

    void setPosition(const glm::vec3& position) { this->position = position; }
    glm::vec3 getPosition() const { return position; }

    void setAttenuation(float constant, float linear, float quadratic) {
        this->constant = constant;
        this->linear = linear;
        this->quadratic = quadratic;
    }
    float getConstantAttenuation() const { return constant; }
    float getLinearAttenuation() const { return linear; }
    float getQuadraticAttenuation() const { return quadratic; }

private:
    glm::vec3 position;   // ���Դ��λ��
    float constant;       // ˥��������
    float linear;         // ˥��������
    float quadratic;      // ˥��������
};


NAMESPACE_END