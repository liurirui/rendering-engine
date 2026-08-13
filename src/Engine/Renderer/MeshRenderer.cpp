#include "MeshRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "Base/Light.h"
#include "RenderGraph/RenderGraph.h"
#include "Base/Material.h"
#include "Base/AssetManager.h"
#include "Base/Logger.h"
#include "Renderer/MaterialSystem.h"
#include "Renderer/IBLSystem.h"
#include "Renderer/RenderTarget.h"
#include "Renderer/RenderQueueBuilder.h"
#include "Base/Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad.h>

NAMESPACE_START


static Shader& resolveMaterialShader(AssetManager& assetManager, const RenderItem& item, const std::shared_ptr<Shader>& fallbackShader) {
    // 材质只保存 ShaderHandle；实际 program 从 ShaderLibrary 查找，找不到时回退默认 lit。
    if (item.materialAsset && item.materialAsset->shader.isValid()) {
        if (std::shared_ptr<Shader> shader = assetManager.getShaderLibrary().get(item.materialAsset->shader)) {
            return *shader;
        }
    }
    return *fallbackShader;
}

static void setupLitShaderCommon(Shader& shader, Scene& scene, Camera* camera, DirectionLight* mainLight, const glm::mat4& projection, const IBLSystem* iblSystem) {
    // 这些是当前 pass 的共享参数：相机、主方向光、点光源、阴影和 IBL。
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", camera->GetViewMatrix());
    shader.setVec3("viewPos", camera->Position);

    shader.setMat4("lightSpaceMatrix", mainLight ? mainLight->LightSpaceMatrix : glm::mat4(1.0f));
    shader.setVec3("light.direction", mainLight ? mainLight->getDirection() : glm::vec3(0.0f, -1.0f, 0.0f));
    shader.setVec3("light.color", mainLight ? mainLight->getColor() : glm::vec3(0.0f));
    shader.setFloat("light.intensity", mainLight ? mainLight->getIntensity() : 0.0f);

    shader.setVec3("ambient", 0.3f, 0.3f, 0.3f);
    shader.setVec3("diffuse", 0.6f, 0.6f, 0.6f);
    shader.setVec3("specular", 1.0f, 1.0f, 1.0f);
    shader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
    shader.setVec3("ambientColor", 0.03f, 0.03f, 0.03f);
    shader.setFloat("shininess", 32.0f);
    shader.setBool("isMirror", false);
    shader.setBool("isGlass", false);
    shader.setBool("receiveShadows", true);
    shader.setInt("shadowMap", 1);
    if (iblSystem) {
        iblSystem->bindToShader(shader);
    }
    else {
        shader.setBool("hasIBL", false);
        shader.setFloat("iblIntensity", 0.0f);
    }

    int pointLightCount = 0;
    for (Light* lightBase : scene.lights) {
        PointLight* light = dynamic_cast<PointLight*>(lightBase);
        if (!light || pointLightCount >= 4) {
            continue;
        }

        const std::string prefix = "point[" + std::to_string(pointLightCount) + "].";
        shader.setVec3((prefix + "position").c_str(), light->getPosition());
        shader.setVec3((prefix + "color").c_str(), light->getColor());
        shader.setFloat((prefix + "intensity").c_str(), light->getIntensity());
        shader.setFloat((prefix + "constant").c_str(), light->getConstantAttenuation());
        shader.setFloat((prefix + "linear").c_str(), light->getLinearAttenuation());
        shader.setFloat((prefix + "quadratic").c_str(), light->getQuadraticAttenuation());
        shader.setFloat((prefix + "range").c_str(), light->getRange());
        pointLightCount++;
    }
    shader.setInt("pointLightCount", pointLightCount);
}

static void renderItemsWithMaterials(RenderContext* renderContext, AssetManager& assetManager, Scene& scene, Camera* camera, DirectionLight* mainLight, const glm::mat4& projection, GraphicsPipeline basePipeline, const std::shared_ptr<Shader>& fallbackShader, const IBLSystem* iblSystem, const std::vector<RenderItem>& items) {
    // RenderQueue 已按 Shader/Material/Mesh 排序，同一 Shader 连续绘制时可复用 pass 公共状态。
    Shader* boundShader = nullptr;
    for (const RenderItem& item : items) {
        if (!item.mesh) {
            continue;
        }

        Shader& shader = resolveMaterialShader(assetManager, item, fallbackShader);
        if (boundShader != &shader) {
            basePipeline.shader = &shader;
            renderContext->bindPipeline(basePipeline);
            setupLitShaderCommon(shader, scene, camera, mainLight, projection, iblSystem);
            if (mainLight) {
                renderContext->bindTexture(mainLight->getShadow()->depthMap->id, 1);
            }
            boundShader = &shader;
        }

        shader.setBool("receiveShadows", item.receiveShadows && mainLight != nullptr);
        MaterialSystem::bindMaterialAsset(item.materialAsset.get(), shader, 2);
        shader.setMat4("model", item.worldMatrix);
        item.mesh->draw();
    }
}
MeshRenderer::MeshRenderer(RenderContext& renderContext, AssetManager& assetManager)
    : renderContext_(renderContext), assetManager_(assetManager) {
    depthStencilState.depthTest = true;
    depthStencilState.depthWrite = true;

    //Shader
    depthMapShader = assetManager_.getShaderLibrary().getOrCreate(ShaderLibrary::DepthOnlyShaderName());
    defaultLitShader = assetManager_.getShaderLibrary().getOrCreate(ShaderLibrary::DefaultLitShaderName());
    lightDebugShader = assetManager_.getShaderLibrary().getOrCreate(ShaderLibrary::LightDebugShaderName());

    iblSystem_.reset(new IBLSystem());
    iblSystem_->initialize(assetManager_.resolvePath("resources/textures/hdr/newport_loft.hdr"));

    floorMaterial_.reset(new MaterialAsset());
    floorMaterial_->name = "engine/floor";
    floorMaterial_->material.name = floorMaterial_->name;
    floorMaterial_->material.diffuseColor = glm::vec3(1.0f);
    floorMaterial_->material.roughness = 0.75f;
    floorMaterial_->material.metallic = 0.0f;

    // Main scene HDR target.
    SamplerInfo depthSampler;
    depthSampler.mipmapMode = MipmapMode::None;
    RenderTargetDesc sceneTargetDesc;
    sceneTargetDesc.debugName = "scene/hdr";
    sceneTargetDesc.width = renderContext_.windowsWidth;
    sceneTargetDesc.height = renderContext_.windowsHeight;
    RenderTargetColorDesc sceneColor;
    sceneColor.format = TextureFormat::RGBA32F;
    sceneColor.clearColor = glm::vec4(0.1f, 0.05f, 0.15f, 1.0f);
    sceneTargetDesc.colors.emplace_back(sceneColor);
    sceneTargetDesc.depth.enabled = true;
    sceneTargetDesc.depth.format = TextureFormat::Depth24_Stencil8;
    sceneTargetDesc.depth.sampler = depthSampler;
    sceneTarget_.reset(new RenderTarget(renderContext_, sceneTargetDesc));
    graphicsPipeline.shader = defaultLitShader.get();
    PipelineColorBlendAttachment pipelineColorBlendAttachment;
    pipelineColorBlendAttachment.blendState.enabled = true;
    graphicsPipeline.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);


    graphicsPipeline_DepthMap.shader = depthMapShader.get();
    PipelineColorBlendAttachment pipelineColorBlendAttachment_DepthMap;
    pipelineColorBlendAttachment_DepthMap.blendState.enabled = true;
    graphicsPipeline_DepthMap.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);
    graphicsPipeline_DepthMap.rasterizationState.cullMode = CullMode::Back;

    

    if (!cubeVBO) {
        cubeVBO = RenderContext::getInstance()->createVertexBuffer(cubeVertices, sizeof(cubeVertices));
    }
    if (!cubeVAO) {
       cubeVAO = RenderContext::getInstance()->createVertexArray(cubeVBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 1, 3);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 2, 8 * sizeof(float), 2, 6);
    }
    if (!planeVBO) {
        planeVBO = RenderContext::getInstance()->createVertexBuffer(planeVertices, sizeof(planeVertices));
    }
    if (!planeVAO) {
        planeVAO = RenderContext::getInstance()->createVertexArray(planeVBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(planeVBO, planeVAO, 3, 8 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(planeVBO, planeVAO, 3, 8 * sizeof(float), 1, 3);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(planeVBO, planeVAO, 2, 8 * sizeof(float), 2, 6);
    }
}
void MeshRenderer::setFloorTexture(const std::shared_ptr<Texture2D>& texture) {
    if (!floorMaterial_) {
        floorMaterial_.reset(new MaterialAsset());
        floorMaterial_->name = "engine/floor";
        floorMaterial_->material.name = floorMaterial_->name;
    }
    floorMaterial_->material.setDiffuseMap(texture);
}

void MeshRenderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (sceneTarget_) {
        sceneTarget_->resize(width, height);
    }
}

void MeshRenderer::addShadowPass(Scene& scene, const std::shared_ptr<RenderQueue>& renderQueue, RenderGraph& rg) {
    // Shadow pass 只写深度和 model/lightSpaceMatrix，不绑定完整材质贴图。
    if (!scene.GetMainDirectionalLight()) {
        return;
    }

    rg.addPass("shadowPass", renderQueue.get(), [this, &scene, renderQueue](RenderContext* renderContext) {
        DirectionLight* light = scene.GetMainDirectionalLight();
        Shadow* shadow = light->getShadow();
        GLint previousViewport[4];
        glGetIntegerv(GL_VIEWPORT, previousViewport);
        const GLboolean polygonOffsetWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);

        depthStencilState.depthTest = true;
        depthStencilState.depthWrite = true;
        renderContext->beginRendering(shadow->DepthMapFramebuffer);
        glViewport(0, 0, static_cast<GLsizei>(shadow->SHADOW_WIDTH), static_cast<GLsizei>(shadow->SHADOW_HEIGHT));
        if (!polygonOffsetWasEnabled) {
            glEnable(GL_POLYGON_OFFSET_FILL);
        }
        // Offset depth-map rasterization to prevent a receiver from shadowing itself.
        glPolygonOffset(2.0f, 4.0f);

        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline_DepthMap);
        depthMapShader->setMat4("lightSpaceMatrix", light->LightSpaceMatrix);
        depthMapShader->setMat4("model", glm::mat4(1.0f));
        renderContext->bindVertexArray(planeVAO);
        renderContext->drawArrays(0, 6);
        for (const RenderItem& item : renderQueue->shadowCasters) {
            if (!item.mesh) {
                continue;
            }
            depthMapShader->setMat4("model", item.worldMatrix);
            item.mesh->draw();
        }
        renderContext->endRendering();

        if (!polygonOffsetWasEnabled) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        });
}

void MeshRenderer::render(Scene& scene, Camera* camera, RenderGraph& rg) {
    // Forward PBR pass 将场景数据、材质数据和共享 GPU MeshResource 组合后提交绘制。
    const int queueWidth = renderContext_.windowsWidth;
    const int queueHeight = renderContext_.windowsHeight;
    if (!camera || queueWidth <= 0 || queueHeight <= 0) {
        return;
    }
    const glm::mat4 queueProjection = glm::perspective(glm::radians(camera->Zoom),
        static_cast<float>(queueWidth) / static_cast<float>(queueHeight), 0.1f, 1000.0f);
    std::shared_ptr<RenderQueue> renderQueue(new RenderQueue());
    RenderQueueBuilder().build(scene, *camera, queueProjection, *renderQueue);
    if (!hasLoggedRenderStats_ ||
        lastRenderStats_.submittedItems != renderQueue->stats.submittedItems ||
        lastRenderStats_.visibleItems != renderQueue->stats.visibleItems ||
        lastRenderStats_.culledItems != renderQueue->stats.culledItems ||
        lastRenderStats_.transparentItems != renderQueue->stats.transparentItems) {
        Logger::Info("Render queue stats. submitted=" + std::to_string(renderQueue->stats.submittedItems) +
            ", visible=" + std::to_string(renderQueue->stats.visibleItems) +
            ", culled=" + std::to_string(renderQueue->stats.culledItems) +
            ", shadow=" + std::to_string(renderQueue->stats.shadowItems) +
            ", opaque=" + std::to_string(renderQueue->stats.opaqueItems) +
            ", transparent=" + std::to_string(renderQueue->stats.transparentItems) +
            ", triangles=" + std::to_string(renderQueue->stats.visibleTriangles));
        hasLoggedRenderStats_ = true;
    }
    lastRenderStats_ = renderQueue->stats;
    addShadowPass(scene, renderQueue, rg);

    rg.addPass("scenePass", renderQueue.get(), [this, &scene, camera, renderQueue](RenderContext* renderContext) {
        DirectionLight* mainLight = scene.GetMainDirectionalLight();

        depthStencilState.depthTest = true;
        depthStencilState.depthWrite = true;
        // Update Camera UBO
        const int renderWidth = renderContext->windowsWidth;
        const int renderHeight = renderContext->windowsHeight;
        if (renderWidth <= 0 || renderHeight <= 0) {
            return;
        }
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom),
            static_cast<float>(renderWidth) / static_cast<float>(renderHeight), 0.1f, 1000.0f);
        if (camera->getUBOID() == 0) {
            camera->createUBO();
        }
        camera->setProjectionMatrix(projection);
        camera->updateUBO();
        camera->bindUBO(Camera::UBO_BINDING_POINT);

        // Update Light UBO
        if (Light::uboID == 0) {
            Light::createUBO();
        }
        Light::updateUBO(scene.lights);
        Light::bindUBO();

        renderContext->beginRendering(sceneTarget_->framebuffer());
        glViewport(0, 0, renderWidth, renderHeight);
        renderContext->setDepthStencilState(depthStencilState);
        GraphicsPipeline lightDebugPipeline = graphicsPipeline;
        lightDebugPipeline.shader = lightDebugShader.get();
        renderContext->bindPipeline(lightDebugPipeline);
        glm::mat4 light_model = glm::mat4(1.0f);
        lightDebugShader->use();
        lightDebugShader->setMat4("projection", projection);
        lightDebugShader->setMat4("view", camera->GetViewMatrix());
        // 方向光只是场景可选光源；没有主方向光时仍保留 IBL、点光源和 Emissive 渲染。
        if (mainLight) {
            glm::vec3 directionPos = mainLight->getDirection() * (-80.0f);
            light_model = glm::translate(light_model, directionPos);
            light_model = glm::scale(light_model, glm::vec3(3.0f));
            lightDebugShader->setMat4("model", light_model);
            lightDebugShader->setVec3("lightColor", mainLight->getColor());
            lightDebugShader->setFloat("lightVisualIntensity", 4.0f);
            renderContext->bindVertexArray(cubeVAO);
            renderContext->drawArrays(0, 36);
        }

        //render light cube
        std::vector<Light*> all_lights = scene.GetAllLights();
        for (int i = 0; i < all_lights.size(); i++) {
            auto light = all_lights[i];
            if (light->getType() == LightType::Point) {
                auto pointLight = dynamic_cast<PointLight*>(light);
                light_model = glm::mat4(1.0f); 
                light_model = glm::translate(light_model, glm::vec3(pointLight->getPosition()));
                light_model = glm::scale(light_model, glm::vec3(0.1f, 0.1f, 0.1f));
                lightDebugShader->setMat4("model", light_model);
                lightDebugShader->setVec3("lightColor", pointLight->getColor());
                lightDebugShader->setFloat("lightVisualIntensity", 4.0f);
                renderContext->bindVertexArray(cubeVAO);
                renderContext->drawArrays(0, 36);
            }
        }

        graphicsPipeline.shader = defaultLitShader.get();
        renderContext->bindPipeline(graphicsPipeline);
        setupLitShaderCommon(*defaultLitShader, scene, camera, mainLight, projection, iblSystem_.get());
        if (mainLight) {
            renderContext->bindTexture(mainLight->getShadow()->depthMap->id, 1);
        }

        //render plane
        MaterialSystem::bindMaterialAsset(floorMaterial_.get(), *defaultLitShader, 2);
        defaultLitShader->setMat4("model", glm::mat4(1.0f));
        renderContext->bindVertexArray(planeVAO);
        renderContext->drawArrays(0, 6);

        renderItemsWithMaterials(renderContext, assetManager_, scene, camera, mainLight, projection,
            graphicsPipeline, defaultLitShader, iblSystem_.get(), renderQueue->opaqueItems);

        // 透明队列保持深度测试但关闭深度写入，并按相机距离从远到近绘制。
        if (!renderQueue->transparentItems.empty()) {
            depthStencilState.depthWrite = false;
            renderContext->setDepthStencilState(depthStencilState);
            renderItemsWithMaterials(renderContext, assetManager_, scene, camera, mainLight, projection,
                graphicsPipeline, defaultLitShader, iblSystem_.get(), renderQueue->transparentItems);
            depthStencilState.depthWrite = true;
            renderContext->setDepthStencilState(depthStencilState);
        }

        renderContext->bindVertexArray(0);
        renderContext->endRendering();
        });
}

unsigned int MeshRenderer::getTargetColorTextureID(int  attachment) {

    Texture2D* texture = sceneTarget_ ? sceneTarget_->colorTexture(static_cast<size_t>(attachment)) : nullptr;
    return texture ? texture->id : 0;

}

FrameBufferInfo* MeshRenderer::getTargetFrameBuffer() {
    return sceneTarget_ ? &sceneTarget_->framebuffer() : nullptr;
}

MeshRenderer::~MeshRenderer() = default;
NAMESPACE_END
