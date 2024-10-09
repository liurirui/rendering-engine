#include"OpenGLRenderContext.h"
#include <glad.h>
#include<Base/Texture2D.h>


NAMESPACE_START

OpenGLRenderContext::OpenGLRenderContext(){
    this->setCurrentRenderContext(this);
}

Texture2D* OpenGLRenderContext::createTexture2D(const TextureUsage& usage, const TextureFormat& textureFormat, const int width, const int height, bool useCubeMap) {

    Texture2D* texture2D = new Texture2D(usage, textureFormat, width, height ,nullptr,useCubeMap);
    
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

    if (!fbo.id) {

        glGenFramebuffers(1, &fbo.id);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);
        for (auto& colorAttachment : fbo.colorAttachments) {
            glBindTexture(GL_TEXTURE_2D, colorAttachment.texture->id);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachment.attachment, GL_TEXTURE_2D, colorAttachment.texture->id, 0);
        }

        //depth texture
        if (fbo.depthStencilAttachment.texture) {
            glBindTexture(GL_TEXTURE_2D, fbo.depthStencilAttachment.texture->id);
            if (fbo.depthStencilAttachment.useStencil)  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fbo.depthStencilAttachment.texture->id, 0);
            else glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo.depthStencilAttachment.texture->id, 0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);
    static const GLenum kDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6,GL_COLOR_ATTACHMENT7 };
    glDrawBuffers(fbo.colorAttachments.size(), kDrawBuffers); // 指定绘制到两个颜色附件
    for (auto& colorAttachment : fbo.colorAttachments) {
        if (colorAttachment.action == AttachmentAction::Clear) {
            GLuint attachments[] = { GL_COLOR_ATTACHMENT0 + colorAttachment.attachment };
            glClearBufferfv(GL_COLOR, colorAttachment.attachment, &colorAttachment.clearColor.x); // 对附件0使用清除颜色
        }
    }

    if (fbo.depthStencilAttachment.texture) {
        if (fbo.depthStencilAttachment.action == AttachmentAction::Clear) {
            glDepthMask(GL_TRUE);
            glClearDepth(fbo.depthStencilAttachment.depthClearValue); // 设置深度值，1.0 表示最远
            glClearStencil(fbo.depthStencilAttachment.stencilClearValue);
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // 清除深度缓冲区
            glDepthMask(GL_FALSE);
        }
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
}

void OpenGLRenderContext::bindPipeline(GraphicsPipeline& pipeline) {

    if (!pipeline.shader) {
        return;
    }

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

unsigned int OpenGLRenderContext::createVertexBufferLayoutInfo(unsigned int vertexBufferID) {

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);

    unsigned int vertextBufferAtributeID;
    glGenVertexArrays(1, &vertextBufferAtributeID);
    glBindVertexArray(vertextBufferAtributeID);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vertextBufferAtributeID;
}

void OpenGLRenderContext::setUpVertexBufferLayoutInfo(unsigned int vertexBufferID, unsigned int vertexBufferLayoutID, int size, int stride, int location, int offset) {

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    if (vertexBufferLayoutID > 0) {
        glBindVertexArray(vertexBufferLayoutID);
    }

    // position attribute
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, stride, (void*)(offset * sizeof(float)));
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

    glBindVertexArray(bufferID);
}

OpenGLRenderContext::~OpenGLRenderContext() {
    this->setCurrentRenderContext(nullptr);
}

NAMESPACE_END