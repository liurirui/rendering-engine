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
    const float near_plane = 1.0f, far_plane = 25.0f;
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;


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
    glm::vec3 colorOrigin;      // ��¼�⿪��ʱ����ɫ
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
    glm::mat4 calculateLightSpaceMatrix();
    
private:
    glm::vec3 direction;  // �����ķ���
};


class PointLight :public Light {
public:
    PointLight(const glm::vec3& position, const glm::vec3& color, float intensity);
    ~PointLight();

    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& position);

    float getConstantAttenuation() const;
    float getLinearAttenuation() const;
    float getQuadraticAttenuation() const;
    void setAttenuation(float constant, float linear, float quadratic);

    glm::mat4 calculateShadowViewMatrix(int faceIndex) {
        float aspect = shadow->SCR_WIDTH / shadow->SCR_HEIGHT;
        glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), aspect, 1.0f, 25.0f);
        glm::vec3 up;
        glm::vec3 target;
        // ���Դ����������+X, -X, +Y, -Y, +Z, -Z
        switch (faceIndex) {
        case 0: target = position + glm::vec3(1.0f, 0.0f, 0.0f); up = glm::vec3(0.0f, -1.0f, 0.0f); break; // +X
        case 1: target = position + glm::vec3(-1.0f, 0.0f, 0.0f); up = glm::vec3(0.0f, -1.0f, 0.0f); break; // -X
        case 2: target = position + glm::vec3(0.0f, 1.0f, 0.0f); up = glm::vec3(0.0f, 0.0f, 1.0f); break;  // +Y
        case 3: target = position + glm::vec3(0.0f, -1.0f, 0.0f); up = glm::vec3(0.0f, 0.0f, -1.0f); break; // -Y
        case 4: target = position + glm::vec3(0.0f, 0.0f, 1.0f); up = glm::vec3(0.0f, -1.0f, 0.0f); break;  // +Z
        case 5: target = position + glm::vec3(0.0f, 0.0f, -1.0f); up = glm::vec3(0.0f, -1.0f, 0.0f); break; // -Z
        }
        glm::mat4 lightViewMatrix = glm::lookAt(position, target, up);
        return lightProjection * lightViewMatrix;
    }
    
private:
    glm::vec3 position;   // ���Դ��λ��
    float constant = 1.0f;       // ˥��������
    float linear = 0.09f;         // ˥��������
    float quadratic = 0.032;      // ˥��������
};


NAMESPACE_END