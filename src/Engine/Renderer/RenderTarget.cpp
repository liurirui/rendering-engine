#include "RenderTarget.h"

#include <Base/Logger.h>
#include <algorithm>

NAMESPACE_START

RenderTarget::RenderTarget(RenderContext& renderContext, const RenderTargetDesc& desc)
    : renderContext_(renderContext), desc_(desc) {
    desc_.width = (std::max)(1, desc_.width);
    desc_.height = (std::max)(1, desc_.height);
    createAttachments();
}

void RenderTarget::createAttachments() {
    // FrameBufferInfo 中的 texture 指针只是非拥有视图，真正所有权在下面的 unique_ptr 中。
    framebuffer_.colorAttachments.clear();
    colorTextures_.clear();
    colorTextures_.reserve(desc_.colors.size());

    for (size_t i = 0; i < desc_.colors.size(); ++i) {
        const RenderTargetColorDesc& colorDesc = desc_.colors[i];
        std::unique_ptr<Texture2D> texture(renderContext_.createTexture2D(
            TextureUsage::RenderTarget, colorDesc.format, desc_.width, desc_.height, colorDesc.sampler));
        if (!texture) {
            Logger::Error("RenderTarget color attachment creation failed. target=" + desc_.debugName +
                ", attachment=" + std::to_string(i));
            continue;
        }

        ColorAttachment attachment;
        attachment.attachment = static_cast<unsigned int>(i);
        attachment.texture = texture.get();
        attachment.action = colorDesc.loadAction;
        attachment.clearColor = colorDesc.clearColor;
        framebuffer_.colorAttachments.emplace_back(attachment);
        colorTextures_.emplace_back(std::move(texture));
    }

    framebuffer_.depthStencilAttachment = DepthStencilAttachment();
    if (desc_.depth.enabled) {
        depthTexture_.reset(renderContext_.createTexture2D(
            TextureUsage::DepthStencil, desc_.depth.format, desc_.width, desc_.height, desc_.depth.sampler));
        if (!depthTexture_) {
            Logger::Error("RenderTarget depth attachment creation failed. target=" + desc_.debugName);
        }
        else {
            framebuffer_.depthStencilAttachment.texture = depthTexture_.get();
            framebuffer_.depthStencilAttachment.action = desc_.depth.loadAction;
            framebuffer_.depthStencilAttachment.depthClearValue = desc_.depth.clearDepth;
            framebuffer_.depthStencilAttachment.stencilClearValue = desc_.depth.clearStencil;
            framebuffer_.depthStencilAttachment.useStencil = desc_.depth.useStencil;
        }
    }

    if (framebuffer_.colorAttachments.empty() && !framebuffer_.depthStencilAttachment.texture) {
        Logger::Error("RenderTarget has no valid attachments. target=" + desc_.debugName);
    }
}

void RenderTarget::resize(int width, int height) {
    // 保留 texture ID，只重新分配 storage；这样引用该 attachment 的 FBO 无需重建。
    width = (std::max)(1, width);
    height = (std::max)(1, height);
    if (desc_.width == width && desc_.height == height) {
        return;
    }

    desc_.width = width;
    desc_.height = height;
    for (const std::unique_ptr<Texture2D>& texture : colorTextures_) {
        texture->resize(width, height);
    }
    if (depthTexture_) {
        depthTexture_->resize(width, height);
    }
}

Texture2D* RenderTarget::colorTexture(size_t index) {
    return index < colorTextures_.size() ? colorTextures_[index].get() : nullptr;
}

const Texture2D* RenderTarget::colorTexture(size_t index) const {
    return index < colorTextures_.size() ? colorTextures_[index].get() : nullptr;
}

NAMESPACE_END
