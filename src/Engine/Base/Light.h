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
    Shadow(LightType type) {
        switch (type) {
        case LightType::Direction:   initDirection(); break;
        case LightType::Point:        initPoint(); break;
        }

    };

    FrameBufferInfo DepthMapFramebuffer;
    Texture2D* depthMap = nullptr;
    const float near_plane = 1.0f, far_plane = 25.0f;
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;
    ~Shadow() { };

private:
    void initDirection() {
        depthMap = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth, SCR_WIDTH, SCR_HEIGHT);
        DepthMapFramebuffer.depthStencilAttachment.texture = depthMap;
        DepthMapFramebuffer.depthStencilAttachment.useStencil = false;
    }
    void initPoint() {
        depthMap = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth, SCR_WIDTH, SCR_HEIGHT, true);
        DepthMapFramebuffer.depthStencilAttachment.texture = depthMap;
        DepthMapFramebuffer.depthStencilAttachment.useStencil = false;
    }


};

class Light {
public:
    //initialization
    Light(LightType type,const glm::vec3& color, float intensity) : type(type),color(color), intensity(intensity), shadow(new Shadow(type)) {}

    //get light type
    LightType getType() const { return type; }

    //get light color
    void setColor(const glm::vec3& color) { this->color = color; }
    glm::vec3 getColor() const { return color; }

    //get light intensity
    void setIntensity(float intensity) { this->intensity = intensity; }
    float getIntensity() const { return intensity; }

    //get shadow
    Shadow* getShadow() const { return shadow; }
    ~Light() {
        delete shadow;
    }

protected:
    LightType type;             // �������
    glm::vec3 color;            // �����ɫ
    float intensity;            // ���ǿ��
    Shadow* shadow = nullptr;   // ��Ӱ
};


class DirectionLight : public Light {
public:
    DirectionLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
        : Light(LightType::Direction, color, intensity), direction(direction){}

    void setDirection(const glm::vec3& direction) { this->direction = direction; }
    glm::vec3 getDirection() const { return direction; }

    // ʵ�ֶ�������Ӱ��ͼ�������
    glm::mat4 calculateLightSpaceMatrix() {
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, shadow->near_plane, shadow->far_plane);

        glm::vec3 lightPos = -direction * 10.0f;  // ��Դ��λ�ÿ��Ը��ݷ�����Զһ���ľ���
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);  // ���򳡾�����
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // �Ϸ���Y�����ϣ�

        glm::mat4 lightViewMatrix = glm::lookAt(lightPos, target, up);
        return lightProjection* lightViewMatrix;
    }
    ~DirectionLight() {}
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
    ~PointLight() {}
private:
    glm::vec3 position;   // ���Դ��λ��
    float constant;       // ˥��������
    float linear;         // ˥��������
    float quadratic;      // ˥��������
};


NAMESPACE_END