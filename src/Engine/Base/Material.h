#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <Base/Texture2D.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "ShaderCode.h"
#include "Camera.h"
#include "Light.h"
#include <RHI/RenderContext.h>
NAMESPACE_START
class Shader;
class RenderContext;

class Material {
public:
    Shader shader;
    Material() {}
    // 纹理贴图
    std::shared_ptr<Texture2D> diffuseMap;
    std::shared_ptr<Texture2D> normalMap;
    std::shared_ptr<Texture2D> specularMap;
    std::shared_ptr<Texture2D> metallicMap;
    std::shared_ptr<Texture2D> roughnessMap;
    std::shared_ptr<Texture2D> aoMap;
    std::shared_ptr<Texture2D> emissiveMap;
    std::shared_ptr<Texture2D> heightMap;

    // 材质属性（默认值）
    glm::vec3 ambientColor = glm::vec3(0.1f);
    glm::vec3 diffuseColor = glm::vec3(1.0f);
    glm::vec3 specularColor = glm::vec3(1.0f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float shininess = 32.0f;
    float opacity = 1.0f;
    float refractiveIndex = 1.0f;

    // 标识位，用于Shader生成
    bool hasDiffuseMap() const { return diffuseMap != nullptr; }
    bool hasNormalMap() const { return normalMap != nullptr; }
    bool hasSpecularMap() const { return specularMap != nullptr; }
    bool hasMetallicMap() const { return metallicMap != nullptr; }
    bool hasRoughnessMap() const { return roughnessMap != nullptr; }
    bool hasAoMap() const { return aoMap != nullptr; }
    bool hasEmissiveMap() const { return emissiveMap != nullptr; }
    bool hasHeightMap() const { return heightMap != nullptr; }

    // 设置纹理辅助函数
    void setDiffuseMap(std::shared_ptr<Texture2D> diffuse) { 
        this->diffuseMap = diffuse;
    }
    void setNormalMap(std::shared_ptr<Texture2D> normal) { 
        this->normalMap = normal;
    }
    void setSpecularMap(std::shared_ptr<Texture2D> specular) { 
        this->specularMap = specular;
    }
    void setMetallicMap(std::shared_ptr<Texture2D> metallic) { 
        this->metallicMap = metallic;
    }
    void setRoughnessMap(std::shared_ptr<Texture2D> roughness) { 
        this->roughnessMap = roughness;
    }
    void setAoMap(std::shared_ptr<Texture2D> ao) { 
        this->aoMap = ao;
    }
    void setEmissiveMap(std::shared_ptr<Texture2D> emissive) { 
        this->emissiveMap = emissive;
    }
    void setHeightMap(std::shared_ptr<Texture2D> height) { 
        this->heightMap = height;
    }
    void setUniform(){
        shader.use();
        shader.setVec3("ambientColor", ambientColor);
        shader.setVec3("diffuseColor", diffuseColor);  
        shader.setVec3("specular", specularColor);
        shader.setVec3("emissiveColor", emissiveColor);
        shader.setFloat("metallic", metallic);
        shader.setFloat("roughness", roughness);
        int slot = 0;

        if (diffuseMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, diffuseMap->id);
            shader.setInt("diffuseMap", slot); 
            slot++;
        }
        if (normalMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, normalMap->id);
            shader.setInt("normalMap", slot);  
            slot++;
        }
        if (specularMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, specularMap->id);
            shader.setInt("specularMap", slot); 
            slot++;
        }
        if (metallicMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, metallicMap->id);
            shader.setInt("metallicMap", slot); 
            slot++;
        }
        if (roughnessMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, roughnessMap->id);
            shader.setInt("roughnessMap", slot); 
            slot++;
        }
        if (aoMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, aoMap->id);
            shader.setInt("aoMap", slot); 
            slot++;
        }
        if (emissiveMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, emissiveMap->id);
            shader.setInt("emissiveMap", slot);  
            slot++;
        }
        if (heightMap != nullptr) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, heightMap->id);
            shader.setInt("heightMap", slot);  
            slot++;
        }
    }
    void generateShader(){
        // 根据材质属性生成着色器代码
             std::string shaderCode = "#version 420 core\n";
            /*shaderCode += "struct Material {\n";
            shaderCode += "    vec3 ambient;\n";
            shaderCode += "    vec3 diffuse;\n";
            shaderCode += "    vec3 specular;\n";
            shaderCode += "    vec3 emissive;\n";
            shaderCode += "    float metallic;\n";
            shaderCode += "    float roughness;\n";
            shaderCode += "};\n\n";*/
            shaderCode += "#define USE_PBR  \n";
            if (hasDiffuseMap()) {
                shaderCode += "#define HAS_DIFFUSE_MAP\n";
                //shaderCode += "uniform sampler2D diffuseMap;\n";
            }
            if (hasNormalMap()) {
                shaderCode += "#define HAS_NORMAL_MAP\n";
                //shaderCode += "uniform sampler2D normalMap;\n";
            }
            if (hasSpecularMap()) {
                shaderCode += "#define HAS_SPECULAR_MAP\n";
                //shaderCode += "uniform sampler2D specularMap;\n";
            }
            if (hasMetallicMap()) {
                shaderCode += "#define HAS_METALLIC_MAP\n";
                //shaderCode += "uniform sampler2D metallicMap;\n";
            }
            if (hasRoughnessMap()) {
                shaderCode += "#define HAS_ROUGHNESS_MAP\n";
                //shaderCode += "uniform sampler2D roughnessMap;\n";
            }
            if (hasAoMap()) {
                shaderCode += "#define HAS_AO_MAP\n";
                //shaderCode += "uniform sampler2D aoMap;\n";
            }
            if (hasEmissiveMap()) {
                shaderCode += "#define HAS_EMISSIVE_MAP\n";
                //shaderCode += "uniform sampler2D emissiveMap;\n";
            }
            if (hasHeightMap()) {
                shaderCode += "#define HAS_HEIGHT_MAP\n";
                //shaderCode += "uniform sampler2D heightMap;\n";
            }
     
            // 添加其他着色器代码（如光照计算等）
            
            // 创建Shader对象
            this->shader = Shader(std::string(shaderCode + general_pbr_vert).c_str(), std::string(shaderCode + general_pbr_frag).c_str());

            // 绑定Uniform Block到正确的绑定点
            RenderContext* renderContext = RenderContext::getInstance();
            if (renderContext) {
                renderContext->bindUniformBlock(this->shader.ID, "CameraData", Camera::UBO_BINDING_POINT);
                renderContext->bindUniformBlock(this->shader.ID, "LightData", Light::UBO_BINDING_POINT);
            }
    }

    // 绑定纹理到着色器（根据纹理存在性设置uniform和绑定纹理单元）
    void bindTextures(RenderContext* context, Shader* shader);

};

NAMESPACE_END