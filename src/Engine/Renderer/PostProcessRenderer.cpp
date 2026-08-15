#include "PostProcessRenderer.h"
#include <Base/AssetManager.h>
#include <Base/Logger.h>
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderTarget.h"
#include <algorithm>
NAMESPACE_START


static void LogOpenGLErrorIfAny(const char* context) {
    // 局部 pass 检查 OpenGL 状态，日志带上下文名称，避免黑屏时只能看到错误码。
    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR) {
        realtimerenderingengine::Logger::Warn(std::string("OpenGL error in ") + context + ". error=" + std::to_string(errorCode));
    }
}
static std::unique_ptr<RenderTarget> CreateColorTarget(RenderContext& renderContext, const std::string& name,
    int width, int height, const SamplerInfo& sampler, const glm::vec4& clearColor) {
    // 所有屏幕后处理目标都通过同一个工厂构造，避免格式、采样器和清屏参数不一致。
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

PostProcessRenderer::PostProcessRenderer(RenderContext& renderContext, AssetManager& assetManager)
    : renderContext_(renderContext), assetManager_(assetManager) {
	// 后处理目标只保存中间图像；最终 tone mapping 仍由 Example 的屏幕 pass 完成。
	PostProcessRenderer_depthStencilState.depthTest = false;
    PostProcessRenderer_depthStencilState.depthWrite = false;
    //PostProcessRenderer_depthStencilState.depthWrite = false;
    ShaderLibrary& shaders = assetManager_.getShaderLibrary();
    const std::string technique = ShaderLibrary::PostProcessShaderName();
    auto loadPass = [&](const char* variant, const char* fragment) {
        const std::string passTechnique = technique + "/" + variant;
        shaders.registerPass(passTechnique, ShaderPassType::Forward,
            { "resources/shaders/fullscreen.vert", fragment, "" });
        return shaders.getPass(ShaderHandle(passTechnique), ShaderPassType::Forward);
    };
    HightLightShader = loadPass("highlight", "resources/shaders/post_highlight.frag");
    BlurShader = loadPass("blur", "resources/shaders/post_blur.frag");
    DownSampleShader = loadPass("downsample", "resources/shaders/post_downsample.frag");
    UpSampleShader = loadPass("upsample", "resources/shaders/post_upsample.frag");
    BloomShader = loadPass("bloom", "resources/shaders/post_bloom.frag");
    RadialBlurShader = loadPass("radial-blur", "resources/shaders/post_radial_blur.frag");
    MotionBlurShader = loadPass("motion-blur", "resources/shaders/post_motion_blur.frag");
    CartoonShader = loadPass("cartoon", "resources/shaders/post_cartoon.frag");
    RippleShader = loadPass("ripple", "resources/shaders/post_ripple.frag");

    SamplerInfo bloomSampler;
    bloomSampler.mipmapMode = MipmapMode::None;
    bloomSampler.addressMode = SamplerAddressMode::ClampToEdge;
    const int width = renderContext_.windowsWidth;
    const int height = renderContext_.windowsHeight;
    highLightTarget_ = CreateColorTarget(renderContext_, "post/highlight", width, height, bloomSampler, glm::vec4(0.0f));
    PostProcessRenderer_graphicsPipeline.shader = HightLightShader.get();
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
    // Bloom mip 链必须按各自目标尺寸 resize，不能只修改默认 viewport。
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
    // Bloom 是公共前置 pass；effectNo 决定是否继续追加一个后处理效果。
    ////Bloom
    const char* bloomPassName = "bloomPass";
    rg.addPass(bloomPassName, sceneFBO,
        { {"scene.color", RenderGraphAccess::Read}, {"post.bloom", RenderGraphAccess::Write} },
        [this, sceneFBO](RenderContext* renderContext) {
        //get high light
        PostProcessRenderer_graphicsPipeline.shader = HightLightShader.get();
        renderContext->beginRendering(highLightTarget_->framebuffer());
        glViewport(0, 0, highLightTarget_->width(), highLightTarget_->height());
        renderContext->setDepthStencilState(PostProcessRenderer_depthStencilState);
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        HightLightShader.get()->use();
        HightLightShader.get()->setInt("scene", 0);
        renderContext->bindTexture(sceneFBO->colorAttachments[0].texture->id, 0);
        renderContext->bindVertexArray(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        // 逐级降采样提取更大范围的高光，减少全分辨率模糊成本。
        for (int i = 0; i < bloomLevel; i++) {
            RenderTarget& downTarget = *downSampleTargets_[i];
            renderContext->beginRendering(downTarget.framebuffer());
            PostProcessRenderer_graphicsPipeline.shader = DownSampleShader.get();
            glViewport(0, 0, downTarget.width(), downTarget.height());
            renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
            DownSampleShader.get()->use();
            glm::vec2 textureSize(1.0f / static_cast<float>(downTarget.width()),
                1.0f / static_cast<float>(downTarget.height()));
            DownSampleShader.get()->setVec2("textureSize", textureSize);
            DownSampleShader.get()->setInt("u_texture", 0);
            if (i == 0) renderContext->bindTexture(highLightTarget_->colorTexture()->id, 0);
            else renderContext->bindTexture(downSampleTargets_[i - 1]->colorTexture()->id, 0);
            renderContext->bindVertexArray(quadVAO);
            renderContext->drawArrays(0, 6);
        }

        // 自底向上合并 mip，恢复到屏幕尺寸附近的柔和 Bloom。
        for (int i = bloomLevel - 2; i >= 0; i--) {
            RenderTarget& upTarget = *upSampleTargets_[i];
            renderContext->beginRendering(upTarget.framebuffer());
            PostProcessRenderer_graphicsPipeline.shader = UpSampleShader.get();
            glViewport(0, 0, upTarget.width(), upTarget.height());
            renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
            UpSampleShader.get()->use();
            glm::vec2 textureSize(1.0f / static_cast<float>(upTarget.width()),
                1.0f / static_cast<float>(upTarget.height()));
            UpSampleShader.get()->setVec2("textureSize", textureSize);
            UpSampleShader.get()->setInt("curMipDownSampletexture", 0);
            renderContext->bindTexture(downSampleTargets_[i]->colorTexture()->id, 0);
            UpSampleShader.get()->setInt("lastMipUpSampletexture", 1);
            if(i == bloomLevel - 2) renderContext->bindTexture(downSampleTargets_[i + 1]->colorTexture()->id, 1);
            else renderContext->bindTexture(upSampleTargets_[i + 1]->colorTexture()->id, 1);
            renderContext->bindVertexArray(quadVAO);
            renderContext->drawArrays(0, 6);
        }

        //Calculate the final color
        PostProcessRenderer_graphicsPipeline.shader = BloomShader.get();
        renderContext->beginRendering(bloomTarget_->framebuffer());
        glViewport(0, 0, bloomTarget_->width(), bloomTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        BloomShader.get()->use();
        BloomShader.get()->setInt("scene", 0);
        BloomShader.get()->setInt("bloomBlur", 1);
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
        PostProcessRenderer_graphicsPipeline.shader = RadialBlurShader.get();
        const char* RadialPassName = "RadialPass";
        rg.addPass(RadialPassName, bloomTexture,
            { {"post.bloom", RenderGraphAccess::Read}, {"post.radial", RenderGraphAccess::Write} },
            [this](RenderContext* renderContext) {
        //Radial Blur
        PostProcessRenderer_graphicsPipeline.shader = RadialBlurShader.get();
        renderContext->beginRendering(radialTarget_->framebuffer());
        glViewport(0, 0, radialTarget_->width(), radialTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        LogOpenGLErrorIfAny("post radial after bind pipeline");
        RadialBlurShader.get()->use();
        LogOpenGLErrorIfAny("post radial after use shader");
        RadialBlurShader.get()->setInt("sceneTexture", 0);
        RadialBlurShader.get()->setVec2("center", 0.5f, 0.5f);
        RadialBlurShader.get()->setFloat("strength", 0.3f);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->bindVertexArray(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });
        return;
    }

    // Motion Blur 使用 A/B ping-pong 保存上一帧输出，resize 后必须重置历史状态。
    if (effectNo == 3) {
        const char* MotionPassName = "MotionPass";
        PostProcessRenderer_graphicsPipeline.shader = MotionBlurShader.get();
        rg.addPass(MotionPassName, bloomTexture,
            { {"post.bloom", RenderGraphAccess::Read}, {"post.motion.history", RenderGraphAccess::ReadWrite} },
            [this](RenderContext* renderContext) {
        PostProcessRenderer_graphicsPipeline.shader = MotionBlurShader.get();
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
        MotionBlurShader.get()->use();
        LogOpenGLErrorIfAny("post motion after use shader");
        renderContext->bindVertexArray(quadVAO);
        MotionBlurShader.get()->setInt("sceneTexture", 0);
        MotionBlurShader.get()->setInt("lastTexture", 1);
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
        rg.addPass(CartoonPassName, bloomTexture,
            { {"post.bloom", RenderGraphAccess::Read}, {"post.cartoon", RenderGraphAccess::Write} },
            [this](RenderContext* renderContext) {
        PostProcessRenderer_graphicsPipeline.shader = CartoonShader.get();
        renderContext->beginRendering(cartoonTarget_->framebuffer());
        glViewport(0, 0, cartoonTarget_->width(), cartoonTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        CartoonShader.get()->use();
        LogOpenGLErrorIfAny("post cartoon");
        renderContext->bindVertexArray(quadVAO);
        CartoonShader.get()->setInt("sceneTexture", 0);
        renderContext->bindTexture(bloomTexture->id, 0);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });
        return;
    }

    //Ripple effect
    if (effectNo == 5) {
        const char* RipplePassName = "RipplePass";
        rg.addPass(RipplePassName, bloomTexture,
            { {"post.bloom", RenderGraphAccess::Read}, {"post.ripple", RenderGraphAccess::Write} },
            [this](RenderContext* renderContext) {
        //Radial Blur
        PostProcessRenderer_graphicsPipeline.shader = RippleShader.get();
        renderContext->beginRendering(rippleTarget_->framebuffer());
        glViewport(0, 0, rippleTarget_->width(), rippleTarget_->height());
        renderContext->bindPipeline(PostProcessRenderer_graphicsPipeline);
        LogOpenGLErrorIfAny("post ripple after bind pipeline");
        RippleShader.get()->use();
        LogOpenGLErrorIfAny("post ripple after use shader");
        renderContext->bindVertexArray(quadVAO);
        RippleShader.get()->setVec2("rippleCenter", glm::vec2(0.5f, 0.5f));
        RippleShader.get()->setFloat("time", time);
        RippleShader.get()->setFloat("waveAmplitude", 0.02f);
        RippleShader.get()->setFloat("waveFrequency", 20.0f);
        RippleShader.get()->setFloat("waveSpeed", 5.0f);
        RippleShader.get()->setInt("sceneTexture", 0);
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
