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

    bool Switch = true;         // �Ƶ�״̬

protected:
    LightType type;             // �������
    glm::vec3 color;            // �����ɫ
    glm::vec3 colorOn;          // �⿪��ʱ����ɫ
    glm::vec3 colorOff;          // ��ر�ʱ����ɫ
    float intensity;            // ���ǿ��
    float intensityOrigin;      // ��¼�⿪��ʱ��ǿ��
    Shadow* shadow = nullptr;   // ��Ӱ
};


class DirectionLight : public Light {
public:
    DirectionLight(const glm::vec3& direction, const glm::vec3& color, float intensity);
    ~DirectionLight();

    void setDirection(const glm::vec3& direction);
    glm::vec3 getDirection() const;

    // ʵ�ֶ�������Ӱ��ͼ�������
    void calculateLightSpaceMatrix();
    glm::mat4 LightSpaceMatrix;
    
private:
    glm::vec3 direction;  // �����ķ���
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
    glm::vec3 position;   // ���Դ��λ��
    float constant = 1.0f;       // ˥��������
    float linear = 0.09f;         // ˥��������
    float quadratic = 0.032;      // ˥��������
};


NAMESPACE_END