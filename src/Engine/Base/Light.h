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
    LightType type;       // 光的类型
    glm::vec3 color;      // 光的颜色
    float intensity;      // 光的强度
};


class DirectionLight : public Light {
public:
    DirectionLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
        : Light(LightType::Direction, color, intensity), direction(direction) {}

    void setDirection(const glm::vec3& direction) { this->direction = direction; }
    glm::vec3 getDirection() const { return direction; }

private:
    glm::vec3 direction;  // 定向光的方向
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
    glm::vec3 position;   // 点光源的位置
    float constant;       // 衰减常数项
    float linear;         // 衰减线性项
    float quadratic;      // 衰减二次项
};


NAMESPACE_END