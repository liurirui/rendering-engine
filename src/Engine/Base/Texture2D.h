#pragma once

#include <glad.h>
#include "Object.h"
#include<string>
#include<glm/vec4.hpp>

enum TextureFormat
{
    RGBA,
    RGBA32F,
    RGB,
    RG,
    R,
    GRAY,
    Depth24_Stencil8
};

enum TextureUsage
{
    ShaderRead,
    ShaderWrite,
    RenderTarget,
    DepthStencil
};


NAMESPACE_START


enum class TexelFilter : uint32_t {
    Nearest,
    Linear
};

enum class MipmapMode : uint32_t {
    None,
    Nearest,
    Linear
};

enum class WrapMode : uint32_t {
    ClampToEdge = 0,
    ClampToOpaqueBlack,
    ClampToOpaqueWhite
};

enum class SamplerAddressMode : uint32_t {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge
};

enum class CompareOp : uint32_t {
    None,
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

struct SamplerInfo {

    TexelFilter minFilter = TexelFilter::Linear;
    MipmapMode mipmapMode = MipmapMode::None;
    TexelFilter magFilter = TexelFilter::Linear;

    SamplerAddressMode addressModeS = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressModeT = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode addressModeR = SamplerAddressMode::ClampToEdge;

    float maxAnisotropy = 1.0f;

    CompareOp compareOp = CompareOp::None;
    glm::vec4 borderColor;

    bool operator==(const SamplerInfo& right) const {
        return minFilter == right.minFilter && mipmapMode == right.mipmapMode &&
            magFilter == right.magFilter && addressModeS == right.addressModeS &&
            addressModeT == right.addressModeT && addressModeR == right.addressModeR &&
            maxAnisotropy == right.maxAnisotropy && compareOp == right.compareOp;
    }

    bool operator!=(const SamplerInfo& right) const {
        return minFilter != right.minFilter || mipmapMode != right.mipmapMode ||
            magFilter != right.magFilter || addressModeS != right.addressModeS ||
            addressModeT != right.addressModeT || addressModeR != right.addressModeR ||
            maxAnisotropy != right.maxAnisotropy || compareOp != right.compareOp;
    }
};

static SamplerInfo defaultSampler;

class Texture2D : public Object
{
public:

    unsigned int id = -1;

    unsigned int width = 0;
    unsigned int height = 0;
    
    TextureFormat textureFormat = TextureFormat::RGBA;

    TextureUsage textureUsage = TextureUsage::ShaderRead;

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Texture2D(const char* path);

    Texture2D(const TextureUsage & usage, const TextureFormat & textureFormat, const int width, const int height, const unsigned char* data = nullptr);

    void initTexture(const TextureUsage& usage, const TextureFormat& textureFormat, const int width, const int height, const unsigned char* data = nullptr);

    ~Texture2D();

};

NAMESPACE_END