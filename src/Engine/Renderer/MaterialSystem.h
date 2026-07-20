#pragma once

#include <Base/Constants.h>

NAMESPACE_START

class Material;
struct MaterialAsset;
class Shader;

class MaterialSystem {
public:
    static void bindMaterial(const Material& material, Shader& shader, int firstTextureSlot = 0);
    static void bindMaterialAsset(const MaterialAsset* materialAsset, Shader& shader, int firstTextureSlot = 0);
};

NAMESPACE_END