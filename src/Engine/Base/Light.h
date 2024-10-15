#pragma once
#include "Object.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <RHI/RenderContext.h>
NAMESPACE_START
enum class LightType {
    Direction,
    Point,
    Spot
};

class Shadow {
public:
    Shadow(LightType type);
    ~Shadow();
    FrameBufferInfo DepthMapFramebuffer;   
    Texture2D* depthMap = nullptr;
    float near_plane = 1.0f, far_plane = 25.0f;
    const unsigned int SHADOW_WIDTH = 800;
    const unsigned int SHADOW_HEIGHT = 600;
    const unsigned int SHADOW_CUBE = 1024;


private:
    void initDirection();   //Creating a shadow map for a directional light
    void initPoint();       //Creating a cubic shadow map for a point light

};

class Light {
public:
    //initialization
    Light(LightType type, const glm::vec3& color, float intensity);

    ~Light();

    LightType getType() const;    //get light type

    glm::vec3 getColor() const;   //get light color

    float getIntensity() const;   //get light intensity

    Shadow* getShadow() const;    //get shadow
    
    void setColor(const glm::vec3& color);
    
    void setIntensity(float intensity);

    void turnOn();

    void turnOff();

    bool Switch = true;         // 灯的状态

protected:
    LightType type;             // 光的类型
    glm::vec3 color;            // 光的颜色
    glm::vec3 colorOn;          // 光开启时的颜色
    glm::vec3 colorOff;          // 光关闭时的颜色
    float intensity;            // 光的强度
    float intensityOrigin;      // 记录光开启时的强度
    Shadow* shadow = nullptr;   // 阴影
};


class DirectionLight : public Light {
public:
    DirectionLight(const glm::vec3& direction, const glm::vec3& color, float intensity);
    ~DirectionLight();

    void setDirection(const glm::vec3& direction);
    glm::vec3 getDirection() const;

    // 实现定向光的阴影视图矩阵计算
    void calculateLightSpaceMatrix();
    glm::mat4 LightSpaceMatrix;
    
private:
    glm::vec3 direction;  // 定向光的方向
};


class PointLight :public Light {
public:
    PointLight(const glm::vec3& position, const glm::vec3& color, float intensity);
    ~PointLight();

    glm::vec3& getPosition();
    void setPosition(const glm::vec3& position);

    float getConstantAttenuation() const;
    float getLinearAttenuation() const;
    float getQuadraticAttenuation() const;
    void setAttenuation(float constant, float linear, float quadratic);

    glm::mat4 calculateShadowViewMatrix(int faceIndex);
    std::vector<glm::mat4> shadowTransforms;
    
private:
    glm::vec3 position;   // 点光源的位置
    float constant = 1.0f;       // 衰减常数项
    float linear = 0.09f;         // 衰减线性项
    float quadratic = 0.032;      // 衰减二次项
};


NAMESPACE_END