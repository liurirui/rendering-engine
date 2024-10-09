#pragma once

#include<Base/Constants.h>
#include<Base/Texture2D.h>
#include <mutex>
#include <vector>

NAMESPACE_START

class Shader;

struct DepthStencilState {
    bool depthTest = true;
    bool depthWrite = true;
};

enum class AttachmentAction {
    None,
    Clear
};

struct ColorAttachment {
    unsigned int attachment = 0;
    Texture2D* texture = nullptr;
    uint32_t faceIndex = 0;
    AttachmentAction action = AttachmentAction::Clear;
    glm::vec4 clearColor;
};

struct DepthStencilAttachment {
    unsigned int attachment = 0;
    Texture2D* texture = nullptr;
    AttachmentAction action = AttachmentAction::Clear;
    double depthClearValue = 1.0;
    uint32_t stencilClearValue = 0;
    bool useStencil = true;

};

enum class CullMode : uint32_t {
    None,
    Back,
    Front
};

enum class FrontFaceDir : uint32_t {
    CW,
    CCW
};

enum class BlendFactor : uint32_t {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha
};

enum class BlendOp : uint32_t {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

struct BlendState {

    bool enabled = false;

    BlendFactor srcColor = BlendFactor::One;
    BlendFactor destColor = BlendFactor::OneMinusSrcAlpha;
    BlendOp colorOp = BlendOp::Add;

    BlendFactor srcAlpha = BlendFactor::One;
    BlendFactor destAlpha = BlendFactor::OneMinusSrcAlpha;
    BlendOp alphaOp = BlendOp::Add;

    BlendState() = default;
    BlendState(BlendFactor srcColor, BlendFactor destColor, BlendFactor srcAlpha, BlendFactor destAlpha, BlendOp colorOp = BlendOp::Add, BlendOp alphaOp = BlendOp::Add) : enabled(true), srcColor(srcColor), destColor(destColor), colorOp(colorOp), srcAlpha(srcAlpha), destAlpha(destAlpha), alphaOp(alphaOp) {}
};

struct PipelineColorBlendAttachment {
    uint32_t attachment = 0;
    BlendState blendState;
};

struct PipelineColorBlendState {
    std::vector<PipelineColorBlendAttachment> attachmentsBlendState;
};

struct PipelineRasterizationState {
    CullMode cullMode = CullMode::Back;
    FrontFaceDir frontFaceDir = FrontFaceDir::CCW;
    PipelineColorBlendState blendState;
};

struct GraphicsPipeline {
    Shader* shader = nullptr;
    PipelineRasterizationState rasterizationState;
};

struct FrameBufferInfo {

    unsigned int id = 0;
    std::vector<ColorAttachment> colorAttachments;
    DepthStencilAttachment depthStencilAttachment;
    FrameBufferInfo& operator=(FrameBufferInfo&& other) noexcept {
    
        this->id = other.id;
        this->colorAttachments.swap(other.colorAttachments);
        this->depthStencilAttachment = other.depthStencilAttachment;
        other.id = -1;
        other.colorAttachments.clear();
    
        return *this;
    }
    FrameBufferInfo() {
    }
    ~FrameBufferInfo();

};

class RenderContext
{

protected:
    static RenderContext* instance;
    RenderContext() {};

public:

    void setCurrentRenderContext(RenderContext* renderContext) {
        instance = renderContext;
    };

    static RenderContext* getInstance() {
        return instance;
    }

    virtual Texture2D * createTexture2D(const TextureUsage& usage, const TextureFormat& textureFormat, const int width, const int height,bool useCubeMap=false) = 0;

    virtual Texture2D * loadTexture2D(const char* path) = 0;

    virtual void bindTexture(unsigned int bufferID, unsigned int bindingIndex) = 0;

    virtual void beginRendering(FrameBufferInfo& fbo) = 0;

    virtual void endRendering() = 0;

   // virtual void setClearColor(float r, float g, float b, float a) = 0;

    virtual void setDepthStencilState(const DepthStencilState& depthStencilState) = 0;

    virtual void bindPipeline(GraphicsPipeline & pipeline) = 0;

  //  virtual void setClearAction(unsigned int action) = 0;

    virtual void bindVertexBuffer(unsigned int bufferID) = 0;

    virtual unsigned int createVertexBuffer(const void * data, int sizeInByte) = 0;

    virtual unsigned int createVertexBufferLayoutInfo(unsigned int vertexBufferID) = 0;

    virtual void bindIndexBuffer(unsigned int bufferID) = 0;

    virtual unsigned int createIndexBuffer(const void* data, int sizeInByte) = 0;

    virtual void setUpVertexBufferLayoutInfo(unsigned int vertexBufferID, unsigned int vertexBufferLayoutID, int size, int stride, int location, int offset = 0) = 0;

    virtual void drawArrays(int first, int numVertex) = 0;

    virtual void drawElements(unsigned int count, const void* indices) = 0;

    virtual ~RenderContext() {};

    int windowsWidth = 0;
    int windowsHeight = 0;

private:

};


NAMESPACE_END

