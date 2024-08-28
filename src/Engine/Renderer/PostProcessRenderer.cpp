#include "PostProcessRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "RenderGraph/RenderGraph.h"
NAMESPACE_START
PostProcessRenderer::PostProcessRenderer() {
	PostProcessRenderer_depthStencilState.depthTest = false;
    PostProcessRenderer_depthStencilState.depthWrite = false;
    HightLightShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_highlight));
    BlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_blur));
    BloomShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_bloom));
    RadialBlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_Radialblur));
    MotionBlurShader= TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_Motionblur));
    CartoonShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Fragmodel_cartoon));

    //set HightLightFramebuffer's Texture Attachments
    fboBrightTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment brightAttachment;
    brightAttachment.attachment = 0;
    brightAttachment.texture = fboBrightTexture;
    brightAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    HightLightFramebuffer.colorAttachments.emplace_back(std::move(brightAttachment));
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
        pingpongColorAttachment[i].clearColor = glm::vec4(1, 1, 1, 1);
        PingpongFramebuffer[i].colorAttachments.emplace_back(std::move(pingpongColorAttachment[i]));
    }

    //set BloomFramebuffer's Texture Attachments
    fboBloomTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment bloomColorAttachment;
    bloomColorAttachment.attachment = 0;
    bloomColorAttachment.texture = fboBloomTexture;
    bloomColorAttachment.clearColor = glm::vec4(1, 1, 1, 1);
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
    fboMotionTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment motionAttachment;
    motionAttachment.attachment = 0;
    motionAttachment.texture = fboMotionTexture;
    motionAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    MotionFramebuffer.colorAttachments.emplace_back(std::move(motionAttachment));

    // set cartoonFramebuffer's Texture Attachments
    fboCartoonTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment cartoonColorAttachment;
    cartoonColorAttachment.attachment = 0;
    cartoonColorAttachment.texture = fboCartoonTexture;
    cartoonColorAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    CartoonFramebuffer.colorAttachments.emplace_back(std::move(cartoonColorAttachment));

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

void PostProcessRenderer::render(RenderGraph& rg, Texture2D* sceneTexture) {
    ////Bloom
    const char* bloomPassName = "bloomPass";
    rg.addPass(bloomPassName, sceneTexture, [this,sceneTexture](RenderContext* renderContext) {
        //get high light
        renderContext->beginRendering(HightLightFramebuffer);
        renderContext->setDepthStencilState(PostProcessRenderer_depthStencilState);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        HightLightShader.getPtr()->use();
        HightLightShader.getPtr()->setInt("scene", 0);
        renderContext->bindTexture(sceneTexture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        //horizontal blur
        bool horizontal = true;
        renderContext->beginRendering(PingpongFramebuffer[0]);
        BlurShader.getPtr()->use();
        BlurShader.getPtr()->setInt("horizontal", horizontal);
        BlurShader.getPtr()->setInt("image", 0);
        renderContext->bindTexture(HightLightFramebuffer.colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
    
        //vertical blur
        renderContext->beginRendering(PingpongFramebuffer[1]);
        BlurShader.getPtr()->use();
        BlurShader.getPtr()->setInt("horizontal", !horizontal);
        BlurShader.getPtr()->setInt("image", 0);
        renderContext->bindTexture(PingpongFramebuffer[0].colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        //Calculate the final color
        renderContext->beginRendering(BloomFramebuffer);
        BloomShader.getPtr()->use();
        BloomShader.getPtr()->setInt("scene", 0);
        BloomShader.getPtr()->setInt("bloomBlur", 1);
        renderContext->bindTexture(sceneTexture->id, 0);
        renderContext->bindTexture(PingpongFramebuffer[1].colorAttachments[0].texture->id, 1);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });

    ////Radial Blur
    //const char* RadialPassName = "RadialPass";
    //rg.addPass(RadialPassName, sceneTexture, [this, sceneTexture](RenderContext* renderContext) {
    //    //Radial Blur
    //    renderContext->beginRendering(RadialFramebuffer);
    //    RadialBlurShader.getPtr()->use();
    //    RadialBlurShader.getPtr()->setInt("sceneTexture", 0);
    //    RadialBlurShader.getPtr()->setVec2("center", SCR_WIDTH/2, SCR_HEIGHT/2);
    //    RadialBlurShader.getPtr()->setFloat("strength", 1.0);
    //    renderContext->bindTexture(sceneTexture->id, 0);
    //    renderContext->bindVertexBuffer(quadVAO);
    //    renderContext->drawArrays(0, 6);
    //    renderContext->endRendering();
    //});

    ////Motion Blur
    //const char* MotionPassName = "MotionPass";
    //rg.addPass(MotionPassName, sceneTexture, [this, sceneTexture](RenderContext* renderContext) {
    //    //Radial Blur
    //    renderContext->beginRendering(MotionFramebuffer);
    //    MotionBlurShader.getPtr()->use();
    //    MotionBlurShader.getPtr()->setInt("sceneTexture", 0);
    //    renderContext->bindTexture(sceneTexture->id, 0);
    //    renderContext->bindVertexBuffer(quadVAO);
    //    renderContext->drawArrays(0, 6);
    //    renderContext->endRendering();
    //    });


    ////Cartoon effect
    //const char* CartoonPassName = "CartoonPass";
    //rg.addPass(CartoonPassName, sceneTexture, [this, sceneTexture](RenderContext* renderContext) {
    //    //Radial Blur
    //    renderContext->beginRendering(CartoonFramebuffer);
    //    CartoonShader.getPtr()->use();
    //    renderContext->bindVertexBuffer(quadVAO);
    //    CartoonShader.getPtr()->setInt("cartoonTexture", 0);
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