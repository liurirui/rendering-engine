#pragma once

#include "RenderContext.h"

NAMESPACE_START

class Shader;
class Camera;

class OpenGLRenderContext : public RenderContext
{
public:

    OpenGLRenderContext();

    virtual Texture2D* createTexture2D(const TextureUsage& usage, const TextureFormat& textureFormat, const int width, const int height) override;
    virtual Texture2D* loadTexture2D(const char* path) override;
    virtual void bindTexture(unsigned int bufferID, unsigned int bindingIndex) override;

    virtual void beginRendering(FrameBufferInfo& fbo) override;

    virtual void endRendering() override;

    virtual void drawArrays(int first, int numVertex) override;

    virtual void drawElements(unsigned int count, const void* indices) override;

    //virtual void setClearColor(float r, float g, float b, float a) override;

    virtual void setDepthStencilState(const DepthStencilState& depthStencilState) override;

    virtual void bindPipeline(GraphicsPipeline& pipeline) override;

    virtual unsigned int createVertexBuffer(const void* data, int sizeInByte) override;

    virtual unsigned int createVertexBufferLayoutInfo(unsigned int vertexBufferID) override;

    virtual void bindIndexBuffer(unsigned int bufferID) override;

    virtual unsigned int createIndexBuffer(const void* data, int sizeInByte) override;

    virtual void setUpVertexBufferLayoutInfo(unsigned int vertexBufferID, unsigned int vertexBufferLayoutID, int size, int stride, int location, int offset = 0) override;

   // virtual void setClearAction(unsigned int action) override;

    virtual void bindVertexBuffer(unsigned int bufferID) override;

    virtual ~OpenGLRenderContext();

    

};


NAMESPACE_END

