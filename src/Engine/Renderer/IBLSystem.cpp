#include "IBLSystem.h"

#include <Base/Logger.h>
#include <Base/Shader.h>
#include <glad.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

NAMESPACE_START

namespace {

const char* kCubemapVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 WorldPos;
uniform mat4 projection;
uniform mat4 view;
void main()
{
    WorldPos = aPos;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

const char* kEquirectangularToCubemapShader = R"(
#version 330 core
out vec4 FragColor;
in vec3 WorldPos;
uniform sampler2D equirectangularMap;
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
void main()
{
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}
)";

const char* kIrradianceShader = R"(
#version 330 core
out vec4 FragColor;
in vec3 WorldPos;
uniform samplerCube environmentMap;
const float PI = 3.14159265359;
void main()
{
    vec3 N = normalize(WorldPos);
    vec3 irradiance = vec3(0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / nrSamples);
    FragColor = vec4(irradiance, 1.0);
}
)";

const char* kPrefilterShader = R"(
#version 330 core
out vec4 FragColor;
in vec3 WorldPos;
uniform samplerCube environmentMap;
uniform float roughness;
const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughnessValue)
{
    float a = roughnessValue * roughnessValue;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / max(denom, 0.001);
}
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughnessValue)
{
    float a = roughnessValue * roughnessValue;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}
void main()
{
    vec3 N = normalize(WorldPos);
    vec3 R = N;
    vec3 V = R;
    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefilteredColor += texture(environmentMap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / max(totalWeight, 0.001);
    FragColor = vec4(prefilteredColor, 1.0);
}
)";

const char* kQuadVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* kBRDFShader = R"(
#version 330 core
out vec2 FragColor;
in vec2 TexCoords;
const float PI = 3.14159265359;
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / max(denom, 0.001);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}
vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(max(1.0 - NdotV * NdotV, 0.0));
    V.y = 0.0;
    V.z = NdotV;
    float A = 0.0;
    float B = 0.0;
    vec3 N = vec3(0.0, 0.0, 1.0);
    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if(NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / max(NdotH * NdotV, 0.001);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}
void main()
{
    FragColor = IntegrateBRDF(TexCoords.x, TexCoords.y);
}
)";

const glm::mat4 kCaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
const glm::mat4 kCaptureViews[] = {
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};

std::string framebufferStatusToString(GLenum status) {
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
    case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
    case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
    default: return "UNKNOWN_FRAMEBUFFER_STATUS_" + std::to_string(status);
    }
}

bool checkFramebufferComplete(const std::string& context) {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        return true;
    }

    Logger::Error("IBL framebuffer incomplete. context=" + context + ", status=" + framebufferStatusToString(status));
    return false;
}

void logGLErrorIfAny(const std::string& context) {
    bool hasError = false;
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
        hasError = true;
        Logger::Warn("OpenGL error during IBL. context=" + context + ", error=" + std::to_string(error));
    }
    if (hasError) {
        Logger::Warn("IBL OpenGL error check finished. context=" + context);
    }
}

}

IBLSystem::IBLSystem() = default;

IBLSystem::~IBLSystem() {
    destroy();
}

bool IBLSystem::initialize(const std::string& hdrPath) {
    // 初始化阶段一次性预计算环境 cubemap、irradiance、prefilter 和 BRDF LUT，
    // 运行时 Shader 只做查表采样。
    if (initialized_) {
        return true;
    }

    Logger::Info("Initializing IBL. hdr=" + hdrPath);

    hdrTexture_ = loadHDRTexture(hdrPath);
    if (hdrTexture_ == 0) {
        Logger::Error("IBL initialization failed: HDR texture could not be loaded. path=" + hdrPath);
        return false;
    }

    createCaptureResources();
    createCubeGeometry();
    createQuadGeometry();

    equirectangularToCubemapShader_.reset(new Shader(kCubemapVertexShader, kEquirectangularToCubemapShader, nullptr, "engine/ibl-equirectangular-to-cubemap"));
    irradianceShader_.reset(new Shader(kCubemapVertexShader, kIrradianceShader, nullptr, "engine/ibl-irradiance"));
    prefilterShader_.reset(new Shader(kCubemapVertexShader, kPrefilterShader, nullptr, "engine/ibl-prefilter"));
    brdfShader_.reset(new Shader(kQuadVertexShader, kBRDFShader, nullptr, "engine/ibl-brdf-lut"));

    GLint previousViewport[4] = {};
    GLint previousDepthFunc = GL_LESS;
    GLboolean wasCullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
    auto restoreCaptureState = [&]() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        glDepthFunc(previousDepthFunc);
        if (wasCullFaceEnabled) {
            glEnable(GL_CULL_FACE);
        }
        else {
            glDisable(GL_CULL_FACE);
        }
    };

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    const unsigned int environmentResolution = 512;
    glGenTextures(1, &environmentMap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap_);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, environmentResolution, environmentResolution, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO_);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, environmentResolution, environmentResolution);

    equirectangularToCubemapShader_->use();
    equirectangularToCubemapShader_->setInt("equirectangularMap", 0);
    equirectangularToCubemapShader_->setMat4("projection", kCaptureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture_);
    glViewport(0, 0, environmentResolution, environmentResolution);
    for (unsigned int i = 0; i < 6; ++i) {
        equirectangularToCubemapShader_->setMat4("view", kCaptureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environmentMap_, 0);
        if (!checkFramebufferComplete("environment cubemap face=" + std::to_string(i))) {
            restoreCaptureState();
            return false;
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap_);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    logGLErrorIfAny("environment cubemap generation");

    const unsigned int irradianceResolution = 32;
    glGenTextures(1, &irradianceMap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap_);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, irradianceResolution, irradianceResolution, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradianceResolution, irradianceResolution);
    irradianceShader_->use();
    irradianceShader_->setInt("environmentMap", 0);
    irradianceShader_->setMat4("projection", kCaptureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap_);
    glViewport(0, 0, irradianceResolution, irradianceResolution);
    for (unsigned int i = 0; i < 6; ++i) {
        irradianceShader_->setMat4("view", kCaptureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap_, 0);
        if (!checkFramebufferComplete("irradiance cubemap face=" + std::to_string(i))) {
            restoreCaptureState();
            return false;
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();
    }
    logGLErrorIfAny("irradiance cubemap generation");

    const unsigned int prefilterResolution = 128;
    const unsigned int maxMipLevels = 5;
    glGenTextures(1, &prefilterMap_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap_);
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = static_cast<unsigned int>(prefilterResolution * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(prefilterResolution * std::pow(0.5, mip));
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip, GL_RGBA16F, mipWidth, mipHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxMipLevels - 1);

    prefilterShader_->use();
    prefilterShader_->setInt("environmentMap", 0);
    prefilterShader_->setMat4("projection", kCaptureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap_);
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = static_cast<unsigned int>(prefilterResolution * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(prefilterResolution * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);
        float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
        prefilterShader_->setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i) {
            prefilterShader_->setMat4("view", kCaptureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap_, mip);
            if (!checkFramebufferComplete("prefilter cubemap mip=" + std::to_string(mip) + ", face=" + std::to_string(i))) {
                restoreCaptureState();
                return false;
            }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    logGLErrorIfAny("prefilter cubemap generation");

    const unsigned int brdfResolution = 512;
    glGenTextures(1, &brdfLUTTexture_);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdfResolution, brdfResolution, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO_);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, brdfResolution, brdfResolution);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture_, 0);
    if (!checkFramebufferComplete("BRDF LUT")) {
        restoreCaptureState();
        return false;
    }
    glViewport(0, 0, brdfResolution, brdfResolution);
    brdfShader_->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();
    logGLErrorIfAny("BRDF LUT generation");

    restoreCaptureState();

    initialized_ = true;
    Logger::Info("IBL initialized. environmentMap=" + std::to_string(environmentMap_) +
        ", irradianceMap=" + std::to_string(irradianceMap_) +
        ", prefilterMap=" + std::to_string(prefilterMap_) +
        ", brdfLUT=" + std::to_string(brdfLUTTexture_));
    return true;
}

void IBLSystem::bindToShader(const Shader& shader, int irradianceSlot, int prefilterSlot, int brdfLutSlot) const {
    shader.use();
    shader.setBool("hasIBL", initialized_);
    shader.setFloat("iblIntensity", initialized_ ? 1.0f : 0.0f);
    if (!initialized_) {
        return;
    }

    shader.setInt("irradianceMap", irradianceSlot);
    shader.setInt("prefilterMap", prefilterSlot);
    shader.setInt("brdfLUT", brdfLutSlot);

    glActiveTexture(GL_TEXTURE0 + irradianceSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap_);
    glActiveTexture(GL_TEXTURE0 + prefilterSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap_);
    glActiveTexture(GL_TEXTURE0 + brdfLutSlot);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture_);
}

unsigned int IBLSystem::loadHDRTexture(const std::string& hdrPath) {
    stbi_set_flip_vertically_on_load(true);
    int width = 0;
    int height = 0;
    int components = 0;
    float* data = stbi_loadf(hdrPath.c_str(), &width, &height, &components, 3);
    stbi_set_flip_vertically_on_load(false);
    if (!data) {
        const char* reason = stbi_failure_reason();
        Logger::Error("HDR texture load failed. path=" + hdrPath + ", reason=" + std::string(reason ? reason : "unknown"));
        return 0;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    Logger::Info("HDR texture loaded. path=" + hdrPath + ", width=" + std::to_string(width) + ", height=" + std::to_string(height));
    return texture;
}

void IBLSystem::createCaptureResources() {
    glGenFramebuffers(1, &captureFBO_);
    glGenRenderbuffers(1, &captureRBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO_);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO_);
}

void IBLSystem::createCubeGeometry() {
    if (cubeVAO_ != 0) {
        return;
    }

    const float vertices[] = {
        -1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,  1.0f,-1.0f,-1.0f,
         1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
        -1.0f,-1.0f, 1.0f,  1.0f,-1.0f, 1.0f,  1.0f, 1.0f, 1.0f,
         1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,-1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f, -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,  1.0f,-1.0f, 1.0f,
         1.0f,-1.0f, 1.0f, -1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,-1.0f,
         1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &cubeVAO_);
    glGenBuffers(1, &cubeVBO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
    glBindVertexArray(0);
}

void IBLSystem::createQuadGeometry() {
    if (quadVAO_ != 0) {
        return;
    }

    const float vertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void IBLSystem::renderCube() const {
    glBindVertexArray(cubeVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void IBLSystem::renderQuad() const {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void IBLSystem::destroy() {
    if (hdrTexture_) glDeleteTextures(1, &hdrTexture_);
    if (environmentMap_) glDeleteTextures(1, &environmentMap_);
    if (irradianceMap_) glDeleteTextures(1, &irradianceMap_);
    if (prefilterMap_) glDeleteTextures(1, &prefilterMap_);
    if (brdfLUTTexture_) glDeleteTextures(1, &brdfLUTTexture_);
    if (captureRBO_) glDeleteRenderbuffers(1, &captureRBO_);
    if (captureFBO_) glDeleteFramebuffers(1, &captureFBO_);
    if (cubeVBO_) glDeleteBuffers(1, &cubeVBO_);
    if (cubeVAO_) glDeleteVertexArrays(1, &cubeVAO_);
    if (quadVBO_) glDeleteBuffers(1, &quadVBO_);
    if (quadVAO_) glDeleteVertexArrays(1, &quadVAO_);

    hdrTexture_ = 0;
    environmentMap_ = 0;
    irradianceMap_ = 0;
    prefilterMap_ = 0;
    brdfLUTTexture_ = 0;
    captureRBO_ = 0;
    captureFBO_ = 0;
    cubeVBO_ = 0;
    cubeVAO_ = 0;
    quadVBO_ = 0;
    quadVAO_ = 0;
    initialized_ = false;
}

NAMESPACE_END
