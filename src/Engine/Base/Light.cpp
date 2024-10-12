#include"Light.h"
NAMESPACE_START
Shadow::Shadow(LightType type) {
    switch (type) {
    case LightType::Direction:   initDirection(); break;
    case LightType::Point:        initPoint(); break;
    }
}

Shadow::~Shadow(){}

void Shadow::initDirection() {
    depthMap = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth, SCR_WIDTH, SCR_HEIGHT);
    DepthMapFramebuffer.depthStencilAttachment.texture = depthMap;
    DepthMapFramebuffer.depthStencilAttachment.useStencil = false;
}

void Shadow::initPoint() {
    depthMap = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth, SCR_WIDTH, SCR_HEIGHT, true);
    DepthMapFramebuffer.depthStencilAttachment.texture = depthMap;
    DepthMapFramebuffer.depthStencilAttachment.useStencil = false;
}

Light::Light(LightType type, const glm::vec3& color, float intensity) : type(type), color(color), intensity(intensity), shadow(new Shadow(type)) {
    this->type = type;
    this->color = color;
    this->colorOrigin = color;
    this->intensity = intensity;
    this->intensityOrigin = intensity;
    this->shadow = new Shadow(type);
}

Light::~Light() {
    delete shadow;
}

LightType Light::getType() const { 
    return type; 
}

glm::vec3 Light::getColor() const {
    return color;
}

float Light::getIntensity() const { 
    return intensity; 
}

Shadow* Light::getShadow() const { 
    return shadow; 
}

void Light::setColor(const glm::vec3& color) { 
    this->color = color; 
}

void Light::setIntensity(float intensity) { 
    this->intensity = intensity; 
}

void Light::turnOn() {
    setColor(colorOrigin);
    setIntensity(intensityOrigin);
    Switch = true;
}

void Light::turnOff() {
    setColor(glm::vec3(0.1f)*color);
    setIntensity(0.0f);
    Switch = false;
}

DirectionLight::DirectionLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
    : Light(LightType::Direction, color, intensity) {
    this->direction = direction;
}

DirectionLight::~DirectionLight() {}

glm::vec3 DirectionLight::getDirection() const { 
    return direction; 
}

void DirectionLight::setDirection(const glm::vec3& direction) { 
    this->direction = direction; 
}

glm::mat4 DirectionLight::calculateLightSpaceMatrix() {
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, shadow->near_plane, shadow->far_plane);

    glm::vec3 lightPos = -direction * 10.0f;  // 光源的位置可以根据方向拉远一定的距离
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);  // 看向场景中心
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // 上方向（Y轴向上）

    glm::mat4 lightViewMatrix = glm::lookAt(lightPos, target, up);
    return lightProjection * lightViewMatrix;
}

PointLight::PointLight(const glm::vec3& position, const glm::vec3& color, float intensity)
    : Light(LightType::Point, color, intensity){
    this->position = position;
}

PointLight::~PointLight() {}

glm::vec3 PointLight::getPosition() const { 
    return position; 
}

void PointLight::setPosition(const glm::vec3& position) { 
    this->position = position; 
}

float PointLight::getConstantAttenuation() const { 
    return constant; 
}

float PointLight::getLinearAttenuation() const { 
    return linear; 
}

float PointLight::getQuadraticAttenuation() const { 
    return quadratic; 
}

void PointLight::setAttenuation(float constant, float linear, float quadratic) {
    this->constant = constant;
    this->linear = linear;
    this->quadratic = quadratic;
}
NAMESPACE_END