#pragma once

#include <memory>
#include <string>
#include <Base/Texture2D.h>
#include <Base/ShaderLibrary.h>
#include <glm/glm.hpp>

NAMESPACE_START

enum class MaterialWorkflow {
    EngineDefault,
    MetallicRoughness,
    SpecularGlossiness
};

class Material {
public:
    std::string name;

    std::shared_ptr<Texture2D> diffuseMap;
    std::shared_ptr<Texture2D> normalMap;
    std::shared_ptr<Texture2D> specularMap;
    std::shared_ptr<Texture2D> reflectionMap;
    std::shared_ptr<Texture2D> metallicMap;
    std::shared_ptr<Texture2D> roughnessMap;
    std::shared_ptr<Texture2D> metallicRoughnessMap;
    std::shared_ptr<Texture2D> aoMap;
    std::shared_ptr<Texture2D> emissiveMap;
    std::shared_ptr<Texture2D> heightMap;

    glm::vec3 ambientColor = glm::vec3(0.1f);
    glm::vec3 diffuseColor = glm::vec3(1.0f);
    glm::vec3 specularColor = glm::vec3(1.0f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float metallic = 0.0f;
    float roughness = 1.0f;
    float shininess = 32.0f;
    float opacity = 1.0f;
    float refractiveIndex = 1.0f;
    MaterialWorkflow workflow = MaterialWorkflow::EngineDefault;

    bool hasDiffuseMap() const { return diffuseMap != nullptr; }
    bool hasNormalMap() const { return normalMap != nullptr; }
    bool hasSpecularMap() const { return specularMap != nullptr; }
    bool hasReflectionMap() const { return reflectionMap != nullptr; }
    bool hasMetallicMap() const { return metallicMap != nullptr; }
    bool hasRoughnessMap() const { return roughnessMap != nullptr; }
    bool hasMetallicRoughnessMap() const { return metallicRoughnessMap != nullptr; }
    bool hasAoMap() const { return aoMap != nullptr; }
    bool hasEmissiveMap() const { return emissiveMap != nullptr; }
    bool hasHeightMap() const { return heightMap != nullptr; }

    void setDiffuseMap(std::shared_ptr<Texture2D> diffuse) { diffuseMap = diffuse; }
    void setNormalMap(std::shared_ptr<Texture2D> normal) { normalMap = normal; }
    void setSpecularMap(std::shared_ptr<Texture2D> specular) { specularMap = specular; }
    void setReflectionMap(std::shared_ptr<Texture2D> reflection) { reflectionMap = reflection; }
    void setMetallicMap(std::shared_ptr<Texture2D> metallicTexture) { metallicMap = metallicTexture; }
    void setRoughnessMap(std::shared_ptr<Texture2D> roughnessTexture) { roughnessMap = roughnessTexture; }
    void setMetallicRoughnessMap(std::shared_ptr<Texture2D> metallicRoughnessTexture) { metallicRoughnessMap = metallicRoughnessTexture; }
    void setAoMap(std::shared_ptr<Texture2D> ao) { aoMap = ao; }
    void setEmissiveMap(std::shared_ptr<Texture2D> emissive) { emissiveMap = emissive; }
    void setHeightMap(std::shared_ptr<Texture2D> height) { heightMap = height; }
};

struct MaterialAsset {
    std::string name;
    ShaderHandle shader = ShaderHandle(ShaderLibrary::DefaultLitShaderName());
    Material material;
};

NAMESPACE_END
