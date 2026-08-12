#pragma once

#include <Base/Constants.h>
#include <Base/Texture2D.h>
#include <RHI/RenderContext.h>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>

NAMESPACE_START

struct RenderTargetColorDesc {
    TextureFormat format = TextureFormat::RGBA32F;
    SamplerInfo sampler;
    AttachmentAction loadAction = AttachmentAction::Clear;
    glm::vec4 clearColor = glm::vec4(0.0f);
};

struct RenderTargetDepthDesc {
    bool enabled = false;
    TextureFormat format = TextureFormat::Depth24_Stencil8;
    SamplerInfo sampler;
    AttachmentAction loadAction = AttachmentAction::Clear;
    double clearDepth = 1.0;
    uint32_t clearStencil = 0;
    bool useStencil = true;
};

struct RenderTargetDesc {
    std::string debugName;
    int width = 1;
    int height = 1;
    std::vector<RenderTargetColorDesc> colors;
    RenderTargetDepthDesc depth;
};

// RenderTarget 拥有一个 framebuffer 的 attachment 纹理，并统一维护尺寸和生命周期。
class RenderTarget final {
public:
    RenderTarget(RenderContext& renderContext, const RenderTargetDesc& desc);
    ~RenderTarget() = default;

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    void resize(int width, int height);

    FrameBufferInfo& framebuffer() { return framebuffer_; }
    const FrameBufferInfo& framebuffer() const { return framebuffer_; }

    Texture2D* colorTexture(size_t index = 0);
    const Texture2D* colorTexture(size_t index = 0) const;
    Texture2D* depthTexture() { return depthTexture_.get(); }
    const Texture2D* depthTexture() const { return depthTexture_.get(); }

    int width() const { return desc_.width; }
    int height() const { return desc_.height; }
    const std::string& debugName() const { return desc_.debugName; }

private:
    void createAttachments();

    RenderContext& renderContext_;
    RenderTargetDesc desc_;
    FrameBufferInfo framebuffer_;
    std::vector<std::unique_ptr<Texture2D>> colorTextures_;
    std::unique_ptr<Texture2D> depthTexture_;
};

NAMESPACE_END
