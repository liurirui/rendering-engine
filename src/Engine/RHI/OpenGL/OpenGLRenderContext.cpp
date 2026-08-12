#include"OpenGLRenderContext.h"
#include <glad.h>
#include<Base/Texture2D.h>
#include<Base/Logger.h>
#include<Base/Shader.h>
#include<iostream>
#include<string>


NAMESPACE_START

OpenGLRenderContext::OpenGLRenderContext(){
    this->setCurrentRenderContext(this);
}

Texture2D* OpenGLRenderContext::createTexture2D(const TextureUsage& usage, const TextureFormat& textureFormat, const int width, const int height, const SamplerInfo& samplerInfo) {

    Texture2D* texture2D = new Texture2D(usage, textureFormat, samplerInfo, width, height, nullptr);
    
    return texture2D;
}

Texture2D * OpenGLRenderContext::loadTexture2D(const char* path) {

    Texture2D* texture2D = new Texture2D(path);

    return texture2D;
}

void OpenGLRenderContext::bindTexture(unsigned int bufferID, unsigned int bindingIndex) {
    glActiveTexture(GL_TEXTURE0 + bindingIndex);
    glBindTexture(GL_TEXTURE_2D, bufferID);
}

FrameBufferInfo::~FrameBufferInfo() {

    if (id > 0) {
        glDeleteFramebuffers(1, &id);
    }
}

void OpenGLRenderContext::beginRendering(FrameBufferInfo& fbo) {
	// FrameBufferInfo 由上层描述 attachment，OpenGL backend 在这里懒创建 FBO 并检查完整性。

    if (!fbo.id) {
        glGenFramebuffers(1, &fbo.id);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);

        for (auto& colorAttachment : fbo.colorAttachments) {
            if (!colorAttachment.texture) {
                Logger::Error("Framebuffer color attachment has null texture. attachment=" + std::to_string(colorAttachment.attachment));
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, colorAttachment.texture->id);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachment.attachment, GL_TEXTURE_2D, colorAttachment.texture->id, 0);
        }

        if (fbo.depthStencilAttachment.texture) {
            Texture2D* depthTexture = fbo.depthStencilAttachment.texture;
            glBindTexture(depthTexture->useCubeMap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, depthTexture->id);

            GLenum attachment = fbo.depthStencilAttachment.useStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
            if (depthTexture->useCubeMap) {
                glFramebufferTexture(GL_FRAMEBUFFER, attachment, depthTexture->id, 0);
            }
            else {
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, depthTexture->id, 0);
            }
        }

        GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
            Logger::Error("Framebuffer incomplete. id=" + std::to_string(fbo.id) + ", status=0x" + std::to_string(framebufferStatus));
        }
        else {
            Logger::Info("Framebuffer created. id=" + std::to_string(fbo.id));
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);

    if (fbo.colorAttachments.empty()) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else {
        static const GLenum kDrawBuffers[] = {
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
            GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7
        };
        glDrawBuffers(static_cast<GLsizei>(fbo.colorAttachments.size()), kDrawBuffers);
    }

    for (auto& colorAttachment : fbo.colorAttachments) {
        if (colorAttachment.action == AttachmentAction::Clear) {
            glClearBufferfv(GL_COLOR, colorAttachment.attachment, &colorAttachment.clearColor.x);
        }
    }

    if (fbo.depthStencilAttachment.texture && fbo.depthStencilAttachment.action == AttachmentAction::Clear) {
        glDepthMask(GL_TRUE);
        glClearDepth(fbo.depthStencilAttachment.depthClearValue);
        glClearStencil(fbo.depthStencilAttachment.stencilClearValue);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDepthMask(GL_FALSE);
    }

}

void OpenGLRenderContext::endRendering() {

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLenum getBlendFactor(BlendFactor & factor) {

    switch (factor) {
    case BlendFactor::Zero:
        return GL_ZERO;
    case BlendFactor::One:
        return GL_ONE;
    case BlendFactor::SrcColor:
        return GL_SRC_COLOR;
    case BlendFactor::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case BlendFactor::DstColor:
        return GL_DST_COLOR;
    case BlendFactor::OneMinusDstColor:
        return GL_ONE_MINUS_DST_COLOR;
    case BlendFactor::SrcAlpha:
        return GL_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DstAlpha:
        return GL_DST_ALPHA;
    case BlendFactor::OneMinusDstAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case BlendFactor::ConstantColor:
        return GL_CONSTANT_COLOR;
    case BlendFactor::OneMinusConstantColor:
        return GL_ONE_MINUS_CONSTANT_COLOR;
    case BlendFactor::ConstantAlpha:
        return GL_CONSTANT_ALPHA;
    case BlendFactor::OneMinusConstantAlpha:
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    case BlendFactor::SrcAlphaSaturate:
        return GL_SRC_ALPHA_SATURATE;
    case BlendFactor::Src1Color:
#ifdef GL_SRC1_COLOR
        return GL_SRC1_COLOR;
#else
        return GL_CONSTANT_COLOR;
#endif
    case BlendFactor::OneMinusSrc1Color:
#ifdef GL_ONE_MINUS_SRC1_COLOR
        return GL_ONE_MINUS_SRC1_COLOR;
#else
        return GL_ONE_MINUS_SRC_COLOR;
#endif
    case BlendFactor::Src1Alpha:
#ifdef GL_SRC1_ALPHA
        return GL_SRC1_ALPHA;
#else
        return GL_SRC_ALPHA;
#endif
    case BlendFactor::OneMinusSrc1Alpha:
#ifdef GL_ONE_MINUS_SRC1_ALPHA
        return GL_ONE_MINUS_SRC1_ALPHA;
#else
        return GL_ONE_MINUS_SRC_ALPHA;
#endif
    }

    return GL_ONE;
}

void OpenGLRenderContext::bindPipeline(GraphicsPipeline& pipeline) {

    if (!pipeline.shader) {
        Logger::Error("bindPipeline called with null shader.");
        return;
    }

    pipeline.shader->use();

    if (pipeline.rasterizationState.cullMode != CullMode::None) {

        glEnable(GL_CULL_FACE);
        glCullFace(pipeline.rasterizationState.cullMode == CullMode::Back ? GL_BACK : GL_FRONT);
        glFrontFace(pipeline.rasterizationState.frontFaceDir == FrontFaceDir::CCW ? GL_CCW : GL_CW);
    }

    for(auto & attachment : pipeline.rasterizationState.blendState.attachmentsBlendState) {
        if (attachment.blendState.enabled) {
            glEnable(GL_BLEND);
            //glBlendFuncSeparatei(attachment.attachment, getBlendFactor(attachment.blendState.srcColor),
            //    getBlendFactor(attachment.blendState.destColor),
            //    getBlendFactor(attachment.blendState.srcAlpha),
            //    getBlendFactor(attachment.blendState.destAlpha)
            //    );
            // don't support multiple attachment blend state
            glBlendFuncSeparate(getBlendFactor(attachment.blendState.srcColor),
                getBlendFactor(attachment.blendState.destColor),
                getBlendFactor(attachment.blendState.srcAlpha),
                getBlendFactor(attachment.blendState.destAlpha)
            );
        }
        break;
    }
    
}

void OpenGLRenderContext::bindIndexBuffer(unsigned int bufferID) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
}

unsigned int OpenGLRenderContext::createIndexBuffer(const void* data, int sizeInByte) {
    glBindVertexArray(0);
    unsigned int bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeInByte, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return bufferID;
}

unsigned int OpenGLRenderContext::createVertexBuffer(const void* data, int sizeInByte) {

    unsigned int bufferID;
    glGenBuffers(1, &bufferID);

    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeInByte, data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return bufferID;
}

unsigned int OpenGLRenderContext::createVertexArray(unsigned int vertexBufferID) {

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);

    unsigned int vertextBufferAtributeID;
    glGenVertexArrays(1, &vertextBufferAtributeID);
    glBindVertexArray(vertextBufferAtributeID);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vertextBufferAtributeID;
}

unsigned int OpenGLRenderContext::createVertexArray(unsigned int vertexBufferID, unsigned int indexBufferID) {
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return vaoID;
}

void OpenGLRenderContext::setUpVertexBufferLayoutInfo(unsigned int vertexBufferID, unsigned int vertexBufferLayoutID, int size, int stride, int location, int offset) {

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    if (vertexBufferLayoutID > 0) {
        glBindVertexArray(vertexBufferLayoutID);
    }

    // position attribute
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, stride, (void*)(offset * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLRenderContext::drawLines(int first, int numVertex) {
    glDrawArrays(GL_LINES, first, numVertex);
}

void OpenGLRenderContext::drawArrays(int first, int numVertex) {

    glDrawArrays(GL_TRIANGLES, first, numVertex);
}

void OpenGLRenderContext::drawElements(unsigned int count, const void* indices) {

    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
}

void OpenGLRenderContext::setDepthStencilState(const DepthStencilState& depthStencilState) {

    depthStencilState.depthTest ?glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    depthStencilState.depthWrite ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);

}

//void OpenGLRenderContext::setClearAction(unsigned int action) {
//    glClear(action);
//}
//
//void OpenGLRenderContext::setClearColor(float r, float g, float b, float a) {
//    glClearColor(r, g, b, a);
//}

void OpenGLRenderContext::bindVertexBuffer(unsigned int bufferID) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
}

void OpenGLRenderContext::bindVertexArray(unsigned int vaoID) {
    glBindVertexArray(vaoID);
}

void OpenGLRenderContext::deleteVertexBuffer(unsigned int bufferID) {
    glDeleteBuffers(1, &bufferID);
}

void OpenGLRenderContext::deleteIndexBuffer(unsigned int bufferID) {
    glDeleteBuffers(1, &bufferID);
}

void OpenGLRenderContext::deleteVertexArray(unsigned int vaoID) {
    glDeleteVertexArrays(1, &vaoID);
}

// Uniform Buffer Object (UBO) methods implementation
unsigned int OpenGLRenderContext::createUniformBuffer(const void* data, size_t size) {
    unsigned int uboID;
    glGenBuffers(1, &uboID);
    glBindBuffer(GL_UNIFORM_BUFFER, uboID);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return uboID;
}

void OpenGLRenderContext::updateUniformBuffer(unsigned int uboID, const void* data, size_t size, size_t offset) {
    glBindBuffer(GL_UNIFORM_BUFFER, uboID);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGLRenderContext::bindUniformBuffer(unsigned int uboID, unsigned int bindingPoint) {
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, uboID);
}

void OpenGLRenderContext::deleteUniformBuffer(unsigned int uboID) {
    glDeleteBuffers(1, &uboID);
}

void OpenGLRenderContext::bindUniformBlock(unsigned int programID, const char* blockName, unsigned int bindingPoint) {
    unsigned int blockIndex = glGetUniformBlockIndex(programID, blockName);
    if (blockIndex != GL_INVALID_INDEX) {
        glUniformBlockBinding(programID, blockIndex, bindingPoint);
    }
}

OpenGLRenderContext::~OpenGLRenderContext() {
    this->setCurrentRenderContext(nullptr);
}

NAMESPACE_END
