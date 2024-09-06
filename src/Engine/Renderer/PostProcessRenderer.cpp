#include "PostProcessRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "RenderGraph/RenderGraph.h"
NAMESPACE_START
PostProcessRenderer::PostProcessRenderer() {
	PostProcessRenderer_depthStencilState.depthTest = false;
    PostProcessRenderer_depthStencilState.depthWrite = false;
    //PostProcessRenderer_depthStencilState.depthWrite = false;
    HightLightShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_highlight));
    BlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_blur));
    BloomShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_bloom));
    RadialBlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_Radialblur));
    MotionBlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_Motionblur));
    CartoonShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_cartoon));
    RippleShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_ripple));

    lastTexture=RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);

    //set HightLightFramebuffer's Texture Attachments
    fboBrightTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment brightAttachment;
    brightAttachment.attachment = 0;
    brightAttachment.texture = fboBrightTexture;
    brightAttachment.clearColor = glm::vec4(0, 1, 0, 1);
    HighLightFramebuffer.colorAttachments.emplace_back(std::move(brightAttachment));
    PostProcessRenderer_graphicsPipeline.shader = HightLightShader.getPtr();
    PipelineColorBlendAttachment pipelineColorBlendAttachment;
    pipelineColorBlendAttachment.blendState.enabled = false;
    PostProcessRenderer_graphicsPipeline.rasterizationState.cullMode = CullMode::None;
    PostProcessRenderer_graphicsPipeline.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);

    //set pingpongFramebuffer's Texture Attachments
    ColorAttachment pingpongColorAttachment[2];
    for (int i = 0; i < 2; i++) {
        fbopingpongColorTexture[i] = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
            RenderContext::getInstance()->windowsHeight);
        pingpongColorAttachment[i].attachment = 0;
        pingpongColorAttachment[i].texture = fbopingpongColorTexture[i];
        pingpongColorAttachment[i].clearColor = glm::vec4(1, 0, 0, 1);
        PingpongFramebuffer[i].colorAttachments.emplace_back(std::move(pingpongColorAttachment[i]));
    }

    //set BloomFramebuffer's Texture Attachments
    fboBloomTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment bloomColorAttachment;
    bloomColorAttachment.attachment = 0;
    bloomColorAttachment.texture = fboBloomTexture;
    bloomColorAttachment.clearColor = glm::vec4(0, 0, 1, 1);
    BloomFramebuffer.colorAttachments.emplace_back(std::move(bloomColorAttachment));

    //set RadialFramebuffer's Texture Attachments
    fboRadialTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment radialAttachment;
    radialAttachment.attachment = 0;
    radialAttachment.texture = fboRadialTexture;
    radialAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    RadialFramebuffer.colorAttachments.emplace_back(std::move(radialAttachment));

    //set MotionFramebuffer's Texture Attachments
    fboMotionTextureA = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment motionAttachmentA;
    motionAttachmentA.attachment = 0;
    motionAttachmentA.texture = fboMotionTextureA;
    motionAttachmentA.clearColor = glm::vec4(1, 1, 1, 1);
    MotionFramebufferA.colorAttachments.emplace_back(std::move(motionAttachmentA));
    fboMotionTextureB = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment motionAttachmentB;
    motionAttachmentB.attachment = 0;
    motionAttachmentB.texture = fboMotionTextureB;
    motionAttachmentB.clearColor = glm::vec4(1, 1, 1, 1);
    MotionFramebufferB.colorAttachments.emplace_back(std::move(motionAttachmentB));

    // set cartoonFramebuffer's Texture Attachments
    fboCartoonTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment cartoonColorAttachment;
    cartoonColorAttachment.attachment = 0;
    cartoonColorAttachment.texture = fboCartoonTexture;
    cartoonColorAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    CartoonFramebuffer.colorAttachments.emplace_back(std::move(cartoonColorAttachment));

    // set cartoonFramebuffer's Texture Attachments
    fboRippleTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment rippleColorAttachment;
    rippleColorAttachment.attachment = 0;
    rippleColorAttachment.texture = fboCartoonTexture;
    rippleColorAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    RippleFramebuffer.colorAttachments.emplace_back(std::move(rippleColorAttachment));

    //set VAO and VBO
    if (!VBO) {
        VBO = RenderContext::getInstance()->createVertexBuffer(quadVertices, sizeof(quadVertices));
    }
    if (!quadVAO) {
        quadVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(VBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(VBO, quadVAO, 2, 4 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(VBO, quadVAO, 2, 4 * sizeof(float), 1, 2);
    }

}

void PostProcessRenderer::render(RenderGraph& rg, FrameBufferInfo* sceneFBO) {
    ////Bloom
    const char* bloomPassName = "bloomPass";
    rg.addPass(bloomPassName, sceneFBO, [this, sceneFBO](RenderContext* renderContext) {
        //get high light
        PostProcessRenderer_graphicsPipeline.shader = HightLightShader.getPtr();
        renderContext->beginRendering(HighLightFramebuffer);
        renderContext->setDepthStencilState(PostProcessRenderer_depthStencilState);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        HightLightShader.getPtr()->use();
        HightLightShader.getPtr()->setInt("scene", 0);
        renderContext->bindTexture(sceneFBO->colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        //horizontal blur
        PostProcessRenderer_graphicsPipeline.shader = BlurShader.getPtr();
        bool horizontal = true;
        renderContext->beginRendering(PingpongFramebuffer[0]);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        BlurShader.getPtr()->use();
        BlurShader.getPtr()->setInt("horizontal", horizontal);
        BlurShader.getPtr()->setInt("image", 0);
        renderContext->bindTexture(HighLightFramebuffer.colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
    
        //vertical blur
        PostProcessRenderer_graphicsPipeline.shader = BlurShader.getPtr();
        renderContext->beginRendering(PingpongFramebuffer[1]);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        int errorCode = glGetError();
        BlurShader.getPtr()->use();
        errorCode = glGetError();
        BlurShader.getPtr()->setInt("horizontal", !horizontal);
        BlurShader.getPtr()->setInt("image", 0);
        renderContext->bindTexture(PingpongFramebuffer[0].colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        //Calculate the final color
        PostProcessRenderer_graphicsPipeline.shader = BloomShader.getPtr();
        renderContext->beginRendering(BloomFramebuffer);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        BloomShader.getPtr()->use();
        errorCode = glGetError();
        BloomShader.getPtr()->setInt("scene", 0);
        BloomShader.getPtr()->setInt("bloomBlur", 1);
        renderContext->bindTexture(sceneFBO->colorAttachments[0].texture->id, 0);
        renderContext->bindTexture(PingpongFramebuffer[1].colorAttachments[0].texture->id, 1);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        bloomTexture = BloomFramebuffer.colorAttachments[0].texture;
        renderContext->endRendering();
        });
    
    //Radial Blur
    PostProcessRenderer_graphicsPipeline.shader = RadialBlurShader.getPtr();
    const char* RadialPassName = "RadialPass";
    rg.addPass(RadialPassName, bloomTexture, [this](RenderContext* renderContext) {
        //Radial Blur
        renderContext->beginRendering(RadialFramebuffer);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        int errorCode = glGetError();
        RadialBlurShader.getPtr()->use();
        errorCode = glGetError();
        RadialBlurShader.getPtr()->setInt("sceneTexture", 0);
        RadialBlurShader.getPtr()->setVec2("center", 0.5, 0.5);
        RadialBlurShader.getPtr()->setFloat("strength", 0.3);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
    });

    //Motion Blur
    const char* MotionPassName = "MotionPass";
    PostProcessRenderer_graphicsPipeline.shader = MotionBlurShader.getPtr();
    rg.addPass(MotionPassName, bloomTexture, [this](RenderContext* renderContext) {
        NowFramebuffer = useFramebufferA ? &MotionFramebufferA: &MotionFramebufferB;
        if (firstRender) {
            lastTexture = bloomTexture;
            firstRender = false;
        }
        else lastTexture = useFramebufferA ? MotionFramebufferB.colorAttachments[0].texture : MotionFramebufferA.colorAttachments[0].texture;
        renderContext->beginRendering(*NowFramebuffer);
        renderContext->setDepthStencilState(PostProcessRenderer_depthStencilState);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        MotionBlurShader.getPtr()->use();
        int errorCode = glGetError();
        renderContext->bindVertexBuffer(quadVAO);
        MotionBlurShader.getPtr()->setInt("sceneTexture", 0);
        MotionBlurShader.getPtr()->setInt("lastTexture", 1);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->bindTexture(lastTexture->id, 1);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        useFramebufferA = !useFramebufferA;
    });


    //Cartoon effect
    const char* CartoonPassName = "CartoonPass";
    rg.addPass(CartoonPassName, sceneFBO, [this, sceneFBO](RenderContext* renderContext) {
        //Radial Blur
        renderContext->beginRendering(CartoonFramebuffer);
        CartoonShader.getPtr()->use();
       int errorCode = glGetError(); 
        renderContext->bindVertexBuffer(quadVAO);
        CartoonShader.getPtr()->setInt("sceneTexture", 0);
        renderContext->bindTexture(sceneFBO->colorAttachments[0].texture->id, 0);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });

    ////Ripple effect
    //const char* RipplePassName = "RipplePass";
    //rg.addPass(RipplePassName, sceneTexture, [this, sceneTexture](RenderContext* renderContext) {
    //    //Radial Blur
    //    renderContext->beginRendering(RippleFramebuffer);
    //    int errorCode = glGetError();
    //    RippleShader.getPtr()->use();
    //    errorCode = glGetError();
    //    renderContext->bindVertexBuffer(quadVAO);
    //    RippleShader.getPtr()->setFloat("iTime", time);
    //    RippleShader.getPtr()->setInt("sceneTexture", 0);
    //    renderContext->bindTexture(sceneTexture->id, 0);
    //    renderContext->drawArrays(0, 6);
    //    renderContext->endRendering();
    //    });

   
}
unsigned int PostProcessRenderer::getTargetColorTextureID(int  attachment) {

    if (attachment >= BloomFramebuffer.colorAttachments.size()) {
        return 0;
    }
    return BloomFramebuffer.colorAttachments[attachment].texture->id;
}

PostProcessRenderer::~PostProcessRenderer() {

}





NAMESPACE_END