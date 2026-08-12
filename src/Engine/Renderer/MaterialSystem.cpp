#include "MaterialSystem.h"

#include <Base/Material.h>
#include <Base/Shader.h>
#include <Base/Texture2D.h>
#include <glad.h>

NAMESPACE_START

static bool bindTextureIfPresent(const std::shared_ptr<Texture2D>& texture, Shader& shader, const char* uniformName, int& slot, int* boundSlot = nullptr) {
    // 只有存在的贴图才占用 texture unit；slot 从材质起始位置递增，避开 shadow/IBL 保留槽位。
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
    // MaterialSystem 是材质数据到 Shader uniform/texture 的唯一绑定入口。
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
    const bool usesMetallicRoughness = material.workflow == MaterialWorkflow::MetallicRoughness;
    const bool usesSpecularGlossiness = material.workflow == MaterialWorkflow::SpecularGlossiness;
    shader.setBool("usesSpecularGlossinessWorkflow", usesSpecularGlossiness);

    shader.setBool("hasDiffuseMap", material.hasDiffuseMap());
    shader.setBool("hasNormalMap", material.hasNormalMap());
    shader.setBool("hasSpecularMap", material.hasSpecularMap());
    shader.setBool("hasReflectionMap", usesSpecularGlossiness && material.hasReflectionMap());
    shader.setBool("hasMetallicMap", usesMetallicRoughness && material.hasMetallicMap());
    shader.setBool("hasRoughnessMap", usesMetallicRoughness && material.hasRoughnessMap());
    shader.setBool("hasMetallicRoughnessMap", usesMetallicRoughness && material.hasMetallicRoughnessMap());
    shader.setBool("hasAoMap", material.hasAoMap());
    shader.setBool("hasEmissiveMap", material.hasEmissiveMap());
    shader.setBool("hasHeightMap", material.hasHeightMap());

    int slot = firstTextureSlot;
    int diffuseSlot = -1;
    if (bindTextureIfPresent(material.diffuseMap, shader, "diffuseMap", slot, &diffuseSlot)) {
        shader.setInt("baseTexture", diffuseSlot);
    }
    bindTextureIfPresent(material.normalMap, shader, "normalMap", slot);
    bindTextureIfPresent(material.specularMap, shader, "specularMap", slot);
    bindTextureIfPresent(material.reflectionMap, shader, "reflectionMap", slot);
    bindTextureIfPresent(material.metallicMap, shader, "metallicMap", slot);
    bindTextureIfPresent(material.roughnessMap, shader, "roughnessMap", slot);
    bindTextureIfPresent(material.metallicRoughnessMap, shader, "metallicRoughnessMap", slot);
    bindTextureIfPresent(material.aoMap, shader, "aoMap", slot);
    bindTextureIfPresent(material.emissiveMap, shader, "emissiveMap", slot);
    bindTextureIfPresent(material.heightMap, shader, "heightMap", slot);
}

void MaterialSystem::bindMaterialAsset(const MaterialAsset* materialAsset, Shader& shader, int firstTextureSlot) {
    if (!materialAsset) {
        static Material defaultMaterial;
        bindMaterial(defaultMaterial, shader, firstTextureSlot);
        return;
    }
    bindMaterial(materialAsset->material, shader, firstTextureSlot);
}

NAMESPACE_END
