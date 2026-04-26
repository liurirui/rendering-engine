#include"Light.h"
NAMESPACE_START

// Static member definition
unsigned int Light::uboID = 0;
Shadow::Shadow(LightType type) {
    switch (type) {
    case LightType::Direction:   initDirection(); break;
    case LightType::Point:        initPoint(); break;
    }
}

Shadow::~Shadow() { delete depthMap; }

void Shadow::initDirection() {
    SamplerInfo depthSampler;
    depthSampler.mipmapMode = MipmapMode::None;
    depthSampler.addressMode = SamplerAddressMode::ClampToBorder;
    depthMap = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth, SHADOW_WIDTH, SHADOW_HEIGHT, depthSampler);
    DepthMapFramebuffer.depthStencilAttachment.texture = depthMap;
    DepthMapFramebuffer.depthStencilAttachment.useStencil = false;
}

void Shadow::initPoint() {
    /*depthMap = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth, SHADOW_CUBE, SHADOW_CUBE, true);
    DepthMapFramebuffer.depthStencilAttachment.texture = depthMap;
    DepthMapFramebuffer.depthStencilAttachment.useStencil = false;*/
}

Light::Light(LightType type, const glm::vec3& color, float intensity) {
    this->type = type;
    this->color = color;
    this->colorOn = color;
    this->colorOff = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f))* 0.1f;
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
    setColor(colorOn);
    setIntensity(intensityOrigin);
}

void Light::turnOff() {
    setColor(colorOff);
    setIntensity(0.0f);
}

DirectionLight::DirectionLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
    : Light(LightType::Direction, color, intensity) {
    this->direction = direction;
    calculateLightSpaceMatrix();
}

DirectionLight::~DirectionLight() {}

glm::vec3 DirectionLight::getDirection() const { 
    return direction; 
}

void DirectionLight::setDirection(const glm::vec3& direction) { 
    this->direction = direction; 
}

void DirectionLight::calculateLightSpaceMatrix() {
    glm::mat4 lightProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, shadow->near_plane, shadow->far_plane);

    glm::vec3 lightPos = -glm::normalize(direction) * 40.0f;     
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f); 
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);     

    glm::mat4 lightViewMatrix = glm::lookAt(lightPos, target, up);
    LightSpaceMatrix = lightProjection * lightViewMatrix;
}

PointLight::PointLight(const glm::vec3& position, const glm::vec3& color, float intensity)
    : Light(LightType::Point, color, intensity){
    this->position = position;
    /*for (int i = 0; i < 6; i++) {
        shadowTransforms.push_back(calculateShadowViewMatrix(i));
    }*/
}

PointLight::~PointLight() {}

glm::vec3& PointLight::getPosition() { 
    return this->position; 
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

glm::mat4 PointLight::calculateShadowViewMatrix(int faceIndex) {
    float aspect = (float)shadow->SHADOW_CUBE / (float)shadow->SHADOW_CUBE;
    glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), aspect, shadow->near_plane, shadow->far_plane);
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

// Light UBO methods implementation
void Light::createUBO() {
    RenderContext* renderContext = RenderContext::getInstance();
    if (!renderContext) return;

    // Create UBO with initial zero data
    struct LightUBOData {
        // Direction light
        glm::vec4 directionLightDirection;
        glm::vec4 directionLightColor;
        float directionLightIntensity;
        glm::vec3 padding1;
        // Point lights
        int numPointLights;
        glm::vec3 padding2;
        struct PointLightData {
            glm::vec4 position;
            glm::vec4 color;
            float intensity;
            float constant;
            float linear;
            float quadratic;
            glm::vec2 padding;
        } pointLights[MAX_POINT_LIGHTS];
    };

    LightUBOData data = {};
    data.numPointLights = 0;
    // Initialize point lights with zero
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        data.pointLights[i].position = glm::vec4(0.0f);
        data.pointLights[i].color = glm::vec4(0.0f);
        data.pointLights[i].intensity = 0.0f;
        data.pointLights[i].constant = 1.0f;
        data.pointLights[i].linear = 0.0f;
        data.pointLights[i].quadratic = 0.0f;
    }

    uboID = renderContext->createUniformBuffer(&data, sizeof(LightUBOData));
}

void Light::updateUBO(const std::vector<Light*>& lights) {
    RenderContext* renderContext = RenderContext::getInstance();
    if (!renderContext || uboID == 0) return;

    struct PointLightData {
        glm::vec4 position;
        glm::vec4 color;
        float intensity;
        float constant;
        float linear;
        float quadratic;
        glm::vec2 padding;
    };

    struct LightUBOData {
        // Direction light
        glm::vec4 directionLightDirection;
        glm::vec4 directionLightColor;
        float directionLightIntensity;
        glm::vec3 padding1;
        // Point lights
        int numPointLights;
        glm::vec3 padding2;
        PointLightData pointLights[MAX_POINT_LIGHTS];
    };

    LightUBOData data = {};
    data.numPointLights = 0;

    // Fill direction light and point lights
    for (Light* light : lights) {
        if (light->getType() == LightType::Direction) {
            DirectionLight* dirLight = static_cast<DirectionLight*>(light);
            data.directionLightDirection = glm::vec4(dirLight->getDirection(), 0.0f);
            data.directionLightColor = glm::vec4(dirLight->getColor(), 1.0f);
            data.directionLightIntensity = dirLight->getIntensity();
        }
        else if (light->getType() == LightType::Point && data.numPointLights < MAX_POINT_LIGHTS) {
            PointLight* pointLight = static_cast<PointLight*>(light);
            data.pointLights[data.numPointLights].position = glm::vec4(pointLight->getPosition(), 1.0f);
            data.pointLights[data.numPointLights].color = glm::vec4(pointLight->getColor(), 1.0f);
            data.pointLights[data.numPointLights].intensity = pointLight->getIntensity();
            data.pointLights[data.numPointLights].constant = pointLight->getConstantAttenuation();
            data.pointLights[data.numPointLights].linear = pointLight->getLinearAttenuation();
            data.pointLights[data.numPointLights].quadratic = pointLight->getQuadraticAttenuation();
            data.numPointLights++;
        }
    }

    renderContext->updateUniformBuffer(uboID, &data, sizeof(LightUBOData));
}

void Light::bindUBO() {
    RenderContext* renderContext = RenderContext::getInstance();
    if (!renderContext || uboID == 0) return;

    renderContext->bindUniformBuffer(uboID, UBO_BINDING_POINT);
}

void Light::deleteUBO() {
    RenderContext* renderContext = RenderContext::getInstance();
    if (!renderContext || uboID == 0) return;

    renderContext->deleteUniformBuffer(uboID);
    uboID = 0;
}
NAMESPACE_END