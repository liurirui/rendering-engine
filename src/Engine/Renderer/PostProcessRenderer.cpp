#include "PostProcessRenderer.h"
#include <Base/Logger.h>
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderTarget.h"
#include <algorithm>
NAMESPACE_START


static void LogOpenGLErrorIfAny(const char* context) {
    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR) {
        realtimerenderingengine::Logger::Warn(std::string("OpenGL error in ") + context + ". error=" + std::to_string(errorCode));
    }
}
static std::unique_ptr<RenderTarget> CreateColorTarget(RenderContext& renderContext, const std::string& name,
    int width, int height, const SamplerInfo& sampler, const glm::vec4& clearColor) {
    RenderTargetDesc desc;
    desc.debugName = name;
    desc.width = width;
    desc.height = height;
    RenderTargetColorDesc color;
    color.format = TextureFormat::RGBA32F;
    color.sampler = sampler;
    color.clearColor = clearColor;
    desc.colors.emplace_back(color);
    return std::unique_ptr<RenderTarget>(new RenderTarget(renderContext, desc));
}

PostProcessRenderer::PostProcessRenderer(RenderContext& renderContext)
    : renderContext_(renderContext) {
	PostProcessRenderer_depthStencilState.depthTest = false;
    PostProcessRenderer_depthStencilState.depthWrite = false;
    //PostProcessRenderer_depthStencilState.depthWrite = false;
    HightLightShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_highlight, nullptr, "post/highlight"));
    BlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_blur, nullptr, "post/blur"));
    DownSampleShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_DownSample, nullptr, "post/downsample"));
    UpSampleShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_UpSample, nullptr, "post/upsample"));
    BloomShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_bloom, nullptr, "post/bloom"));
    RadialBlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_Radialblur, nullptr, "post/radial-blur"));
    MotionBlurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_Motionblur, nullptr, "post/motion-blur"));
    CartoonShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_cartoon, nullptr, "post/cartoon"));
    RippleShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_ripple, nullptr, "post/ripple"));

    SamplerInfo bloomSampler;
    bloomSampler.mipmapMode = MipmapMode::None;
    bloomSampler.addressMode = SamplerAddressMode::ClampToEdge;
    const int width = renderContext_.windowsWidth;
    const int height = renderContext_.windowsHeight;
    highLightTarget_ = CreateColorTarget(renderContext_, "post/highlight", width, height, bloomSampler, glm::vec4(0.0f));
    PostProcessRenderer_graphicsPipeline.shader = HightLightShader.getPtr();
    PipelineColorBlendAttachment pipelineColorBlendAttachment;
    pipelineColorBlendAttachment.blendState.enabled = false;
    PostProcessRenderer_graphicsPipeline.rasterizationState.cullMode = CullMode::None;
    PostProcessRenderer_graphicsPipeline.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);

    for (int i = 0; i < bloomLevel; i++) {
        const int mipWidth = std::max(1, width / (1 << (i + 1)));
        const int mipHeight = std::max(1, height / (1 << (i + 1)));
        downSampleTargets_[i] = CreateColorTarget(renderContext_, "post/bloom-down-" + std::to_string(i),
            mipWidth, mipHeight, bloomSampler, glm::vec4(0.0f));
        upSampleTargets_[i] = CreateColorTarget(renderContext_, "post/bloom-up-" + std::to_string(i),
            mipWidth, mipHeight, bloomSampler, glm::vec4(0.0f));
    }

    bloomTarget_ = CreateColorTarget(renderContext_, "post/bloom", width, height, bloomSampler, glm::vec4(0.0f));
    radialTarget_ = CreateColorTarget(renderContext_, "post/radial", width, height, bloomSampler, glm::vec4(0.0f));
    motionTargetA_ = CreateColorTarget(renderContext_, "post/motion-a", width, height, bloomSampler, glm::vec4(0.0f));
    motionTargetB_ = CreateColorTarget(renderContext_, "post/motion-b", width, height, bloomSampler, glm::vec4(0.0f));
    cartoonTarget_ = CreateColorTarget(renderContext_, "post/cartoon", width, height, bloomSampler, glm::vec4(0.0f));
    rippleTarget_ = CreateColorTarget(renderContext_, "post/ripple", width, height, bloomSampler, glm::vec4(0.0f));

    //set VAO and VBO
    if (!VBO) {
        VBO = RenderContext::getInstance()->createVertexBuffer(quadVertices, sizeof(quadVertices));
    }
    if (!quadVAO) {
        quadVAO = RenderContext::getInstance()->createVertexArray(VBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(VBO, quadVAO, 2, 4 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(VBO, quadVAO, 2, 4 * sizeof(float), 1, 2);
    }

}

void PostProcessRenderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    highLightTarget_->resize(width, height);
    bloomTarget_->resize(width, height);
    radialTarget_->resize(width, height);
    motionTargetA_->resize(width, height);
    motionTargetB_->resize(width, height);
    cartoonTarget_->resize(width, height);
    rippleTarget_->resize(width, height);

    for (int i = 0; i < bloomLevel; ++i) {
        const int mipWidth = std::max(1, width / (1 << (i + 1)));
        const int mipHeight = std::max(1, height / (1 << (i + 1)));
        downSampleTargets_[i]->resize(mipWidth, mipHeight);
        upSampleTargets_[i]->resize(mipWidth, mipHeight);
    }

    firstRender = true;
    useFramebufferA = true;
    nowTarget_ = nullptr;
    lastTexture = nullptr;
}

void PostProcessRenderer::render(RenderGraph& rg, FrameBufferInfo* sceneFBO, int effectNo) {
    ////Bloom
    const char* bloomPassName = "bloomPass";
    rg.addPass(bloomPassName, sceneFBO, [this, sceneFBO](RenderContext* renderContext) {
        //get high light
        PostProcessRenderer_graphicsPipeline.shader = HightLightShader.getPtr();
        renderContext->beginRendering(highLightTarget_->framebuffer());
        glViewport(0, 0, highLightTarget_->width(), highLightTarget_->height());
        renderContext->setDepthStencilState(PostProcessRenderer_depthStencilState);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        HightLightShader.getPtr()->use();
        HightLightShader.getPtr()->setInt("scene", 0);
        renderContext->bindTexture(sceneFBO->colorAttachments[0].texture->id, 0);
        renderContext->bindVertexArray(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        for (int i = 0; i < bloomLevel; i++) {
            RenderTarget& downTarget = *downSampleTargets_[i];
            renderContext->beginRendering(downTarget.framebuffer());
            PostProcessRenderer_graphicsPipeline.shader = DownSampleShader.getPtr();
            glViewport(0, 0, downTarget.width(), downTarget.height());
            renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
            DownSampleShader.getPtr()->use();
            glm::vec2 textureSize(1.0f / static_cast<float>(downTarget.width()),
                1.0f / static_cast<float>(downTarget.height()));
            DownSampleShader.getPtr()->setVec2("textureSize", textureSize);
            DownSampleShader.getPtr()->setInt("u_texture", 0);
            if (i == 0) renderContext->bindTexture(highLightTarget_->colorTexture()->id, 0);
            else renderContext->bindTexture(downSampleTargets_[i - 1]->colorTexture()->id, 0);
            renderContext->bindVertexArray(quadVAO);
            renderContext->drawArrays(0, 6);
        }

        for (int i = bloomLevel - 2; i >= 0; i--) {
            RenderTarget& upTarget = *upSampleTargets_[i];
            renderContext->beginRendering(upTarget.framebuffer());
            PostProcessRenderer_graphicsPipeline.shader = UpSampleShader.getPtr();
            glViewport(0, 0, upTarget.width(), upTarget.height());
            renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
            UpSampleShader.getPtr()->use();
            glm::vec2 textureSize(1.0f / static_cast<float>(upTarget.width()),
                1.0f / static_cast<float>(upTarget.height()));
            UpSampleShader.getPtr()->setVec2("textureSize", textureSize);
            UpSampleShader.getPtr()->setInt("curMipDownSampletexture", 0);
            renderContext->bindTexture(downSampleTargets_[i]->colorTexture()->id, 0);
            UpSampleShader.getPtr()->setInt("lastMipUpSampletexture", 1);
            if(i == bloomLevel - 2) renderContext->bindTexture(downSampleTargets_[i + 1]->colorTexture()->id, 1);
            else renderContext->bindTexture(upSampleTargets_[i + 1]->colorTexture()->id, 1);
            renderContext->bindVertexArray(quadVAO);
            renderContext->drawArrays(0, 6);
        }

        //Calculate the final color
        PostProcessRenderer_graphicsPipeline.shader = BloomShader.getPtr();
        renderContext->beginRendering(bloomTarget_->framebuffer());
        glViewport(0, 0, bloomTarget_->width(), bloomTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        BloomShader.getPtr()->use();
        BloomShader.getPtr()->setInt("scene", 0);
        BloomShader.getPtr()->setInt("bloomBlur", 1);
        renderContext->bindTexture(sceneFBO->colorAttachments[0].texture->id, 0);
        renderContext->bindTexture(upSampleTargets_[0]->colorTexture()->id, 1);
        renderContext->bindVertexArray(quadVAO);
        renderContext->drawArrays(0, 6);
        bloomTexture = bloomTarget_->colorTexture();
        renderContext->endRendering();
        });

    if (effectNo == 1) {
        return;
    }
    
    //Radial Blur
    if (effectNo == 2) {
        PostProcessRenderer_graphicsPipeline.shader = RadialBlurShader.getPtr();
        const char* RadialPassName = "RadialPass";
        rg.addPass(RadialPassName, bloomTexture, [this](RenderContext* renderContext) {
        //Radial Blur
        PostProcessRenderer_graphicsPipeline.shader = RadialBlurShader.getPtr();
        renderContext->beginRendering(radialTarget_->framebuffer());
        glViewport(0, 0, radialTarget_->width(), radialTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        LogOpenGLErrorIfAny("post radial after bind pipeline");
        RadialBlurShader.getPtr()->use();
        LogOpenGLErrorIfAny("post radial after use shader");
        RadialBlurShader.getPtr()->setInt("sceneTexture", 0);
        RadialBlurShader.getPtr()->setVec2("center", 0.5f, 0.5f);
        RadialBlurShader.getPtr()->setFloat("strength", 0.3f);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->bindVertexArray(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });
        return;
    }

    //Motion Blur
    if (effectNo == 3) {
        const char* MotionPassName = "MotionPass";
        PostProcessRenderer_graphicsPipeline.shader = MotionBlurShader.getPtr();
        rg.addPass(MotionPassName, bloomTexture, [this](RenderContext* renderContext) {
        PostProcessRenderer_graphicsPipeline.shader = MotionBlurShader.getPtr();
        nowTarget_ = useFramebufferA ? motionTargetA_.get() : motionTargetB_.get();
        if (firstRender) {
            lastTexture = bloomTexture;
            firstRender = false;
        }
        else lastTexture = useFramebufferA ? motionTargetB_->colorTexture() : motionTargetA_->colorTexture();
        renderContext->beginRendering(nowTarget_->framebuffer());
        glViewport(0, 0, nowTarget_->width(), nowTarget_->height());
        renderContext->setDepthStencilState(PostProcessRenderer_depthStencilState);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        MotionBlurShader.getPtr()->use();
        LogOpenGLErrorIfAny("post motion after use shader");
        renderContext->bindVertexArray(quadVAO);
        MotionBlurShader.getPtr()->setInt("sceneTexture", 0);
        MotionBlurShader.getPtr()->setInt("lastTexture", 1);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->bindTexture(lastTexture->id, 1);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        useFramebufferA = !useFramebufferA;
        });
        return;
    }


    //Cartoon effect
    if (effectNo == 4) {
        const char* CartoonPassName = "CartoonPass";
        rg.addPass(CartoonPassName, bloomTexture, [this](RenderContext* renderContext) {
        PostProcessRenderer_graphicsPipeline.shader = CartoonShader.getPtr();
        renderContext->beginRendering(cartoonTarget_->framebuffer());
        glViewport(0, 0, cartoonTarget_->width(), cartoonTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        CartoonShader.getPtr()->use();
        LogOpenGLErrorIfAny("post cartoon");
        renderContext->bindVertexArray(quadVAO);
        CartoonShader.getPtr()->setInt("sceneTexture", 0);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });
        return;
    }

    //Ripple effect
    if (effectNo == 5) {
        const char* RipplePassName = "RipplePass";
        rg.addPass(RipplePassName, bloomTexture, [this](RenderContext* renderContext) {
        //Radial Blur
        PostProcessRenderer_graphicsPipeline.shader = RippleShader.getPtr();
        renderContext->beginRendering(rippleTarget_->framebuffer());
        glViewport(0, 0, rippleTarget_->width(), rippleTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        LogOpenGLErrorIfAny("post ripple after bind pipeline");
        RippleShader.getPtr()->use();
        LogOpenGLErrorIfAny("post ripple after use shader");
        renderContext->bindVertexArray(quadVAO);
        RippleShader.getPtr()->setVec2("rippleCenter", glm::vec2(0.5f, 0.5f));
        RippleShader.getPtr()->setFloat("time", time);
        RippleShader.getPtr()->setFloat("waveAmplitude", 0.02f);
        RippleShader.getPtr()->setFloat("waveFrequency", 20.0f);
        RippleShader.getPtr()->setFloat("waveSpeed", 5.0f);
        RippleShader.getPtr()->setInt("sceneTexture", 0);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });
    }

   
}

RenderTarget* PostProcessRenderer::targetForEffect(int effectNo) {
    switch (effectNo) {
    case 1: return bloomTarget_.get();
    case 2: return radialTarget_.get();
    case 3: return nowTarget_ ? nowTarget_ : motionTargetA_.get();
    case 4: return cartoonTarget_.get();
    case 5: return rippleTarget_.get();
    default: return nullptr;
    }
}

unsigned int PostProcessRenderer::getTargetColorTextureID(int attachment, int effectNo) {
    RenderTarget* target = targetForEffect(effectNo);
    Texture2D* texture = target ? target->colorTexture(static_cast<size_t>(attachment)) : nullptr;
    return texture ? texture->id : 0;
}

PostProcessRenderer::~PostProcessRenderer() = default;





NAMESPACE_END
