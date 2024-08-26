#pragma once

#include<Base/Constants.h>
#include<Base/TRefCountPtr.h>

#include<RHI/RenderContext.h>

class RenderGraph;

NAMESPACE_START

    class Camera;
    class Shader;
    class Mesh;

    class BasePassRenderer
    {
    public:

        BasePassRenderer();
        virtual ~BasePassRenderer();
        virtual void render(Camera * camera, RenderGraph & rg);

        unsigned int getTargetColorTexture(int  attachment);

    private:

        TRefCountPtr<Shader> lightingShader;

        Texture2D* baseTexture = nullptr;
        Texture2D* normalTexture = nullptr;
        FrameBufferInfo framebuffer;
        GraphicsPipeline graphicsPipeline;
        DepthStencilState depthStencilState;

        Texture2D* fboColorTexture = nullptr;
        Texture2D* fboDepthTexture = nullptr;

        const unsigned int SCR_WIDTH = 800;
        const unsigned int SCR_HEIGHT = 600;

        Mesh* mesh = nullptr;

    };


NAMESPACE_END

