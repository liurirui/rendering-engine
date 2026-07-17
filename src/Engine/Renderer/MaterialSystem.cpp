#include "MaterialSystem.h"

#include <Base/Material.h>
#include <Base/Shader.h>
#include <Base/Texture2D.h>
#include <glad.h>

NAMESPACE_START

static bool bindTextureIfPresent(const std::shared_ptr<Texture2D>& texture, Shader& shader, const char* uniformName, int& slot, int* boundSlot = nullptr) {
    if (!texture) {
        return false;
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    shader.setInt(uniformName, slot);
    if (boundSlot) {
        *boundSlot = slot;
    }
    slot++;
    return true;
}

void MaterialSystem::bindMaterial(const Material& material, Shader& shader, int firstTextureSlot) {
    shader.use();
    shader.setVec3("ambientColor", material.ambientColor);
    shader.setVec3("diffuseColor", material.diffuseColor);
    shader.setVec3("specular", material.specularColor);
    shader.setVec3("specularColor", material.specularColor);
    shader.setVec3("emissiveColor", material.emissiveColor);
    shader.setFloat("metallic", material.metallic);
    shader.setFloat("roughness", material.roughness);
    shader.setFloat("shininess", material.shininess);
    shader.setFloat("opacity", material.opacity);

    int slot = firstTextureSlot;
    int diffuseSlot = -1;
    if (bindTextureIfPresent(material.diffuseMap, shader, "diffuseMap", slot, &diffuseSlot)) {
        shader.setInt("baseTexture", diffuseSlot);
    }
    bindTextureIfPresent(material.normalMap, shader, "normalMap", slot);
    bindTextureIfPresent(material.specularMap, shader, "specularMap", slot);
    bindTextureIfPresent(material.metallicMap, shader, "metallicMap", slot);
    bindTextureIfPresent(material.roughnessMap, shader, "roughnessMap", slot);
    bindTextureIfPresent(material.aoMap, shader, "aoMap", slot);
    bindTextureIfPresent(material.emissiveMap, shader, "emissiveMap", slot);
    bindTextureIfPresent(material.heightMap, shader, "heightMap", slot);
}

void MaterialSystem::bindMaterialAsset(const MaterialAsset* materialAsset, Shader& shader, int firstTextureSlot) {
    if (!materialAsset) {
        shader.use();
        return;
    }
    bindMaterial(materialAsset->material, shader, firstTextureSlot);
}

NAMESPACE_END