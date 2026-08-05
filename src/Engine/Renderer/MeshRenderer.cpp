#include "MeshRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "Base/Light.h"
#include "RenderGraph/RenderGraph.h"
#include "Base/Material.h"
#include "Base/AssetManager.h"
#include "Renderer/MaterialSystem.h"
#include "Renderer/IBLSystem.h"
#include "Base/Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad.h>

NAMESPACE_START


static Shader& resolveMaterialShader(AssetManager& assetManager, Mesh* mesh, const std::shared_ptr<Shader>& fallbackShader) {
    if (mesh && mesh->materialAsset && mesh->materialAsset->shader.isValid()) {
        if (std::shared_ptr<Shader> shader = assetManager.getShaderLibrary().get(mesh->materialAsset->shader)) {
            return *shader;
        }
    }
    return *fallbackShader;
}

static void setupLitShaderCommon(Shader& shader, Scene& scene, Camera* camera, DirectionLight* mainLight, const glm::mat4& projection, const IBLSystem* iblSystem) {
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", camera->GetViewMatrix());
    shader.setVec3("viewPos", camera->Position);

    shader.setMat4("lightSpaceMatrix", mainLight->LightSpaceMatrix);

    shader.setVec3("light.direction", mainLight->getDirection());
    shader.setVec3("light.color", mainLight->getColor());
    shader.setFloat("light.intensity", mainLight->getIntensity());

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

static void renderSceneObjectsWithMaterials(RenderContext* renderContext, AssetManager& assetManager, Scene& scene, Camera* camera, DirectionLight* mainLight, const glm::mat4& projection, GraphicsPipeline basePipeline, const std::shared_ptr<Shader>& fallbackShader, const IBLSystem* iblSystem) {
    Shader* boundShader = nullptr;

    for (auto go : scene.GetRenderableObjects()) {
        if (!go || !go->GetTransform()) {
            continue;
        }
        for (auto mesh : go->meshes) {
            if (!mesh) {
                continue;
            }

            Shader& shader = resolveMaterialShader(assetManager, mesh, fallbackShader);
            if (boundShader != &shader) {
                basePipeline.shader = &shader;
                renderContext->bindPipeline(basePipeline);
                setupLitShaderCommon(shader, scene, camera, mainLight, projection, iblSystem);
                renderContext->bindTexture(mainLight->getShadow()->depthMap->id, 1);
                boundShader = &shader;
            }

            MaterialSystem::bindMaterialAsset(mesh->materialAsset.get(), shader, 2);
            shader.setMat4("model", go->GetTransform()->worldMaterix);
            mesh->draw();
        }
    }
}
MeshRenderer::MeshRenderer(AssetManager& assetManager)
    : assetManager_(assetManager) {
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

    //set  Originframebuffer's Texture Attachments
    SamplerInfo depthSampler;
    depthSampler.mipmapMode = MipmapMode::None;
    fboColorTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    fboDepthTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth24_Stencil8, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight, depthSampler);
    ColorAttachment colorAttachment;
    colorAttachment.attachment = 0;
    colorAttachment.texture = fboColorTexture;
    colorAttachment.clearColor = glm::vec4(0.1, 0.05, 0.15, 1);
    OriginFramebuffer.colorAttachments.emplace_back(std::move(colorAttachment));
    OriginFramebuffer.depthStencilAttachment.texture = fboDepthTexture;
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
    if (fboColorTexture) {
        fboColorTexture->resize(width, height);
    }
    if (fboDepthTexture) {
        fboDepthTexture->resize(width, height);
    }
}

void MeshRenderer::addShadowPass(Scene& scene, Camera* camera, RenderGraph& rg) {
    if (!scene.GetMainDirectionalLight()) {
        return;
    }

    rg.addPass("shadowPass", &camera, [this, &scene, camera](RenderContext* renderContext) {
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
        scene.DrawObjects(*depthMapShader);
        renderContext->endRendering();

        if (!polygonOffsetWasEnabled) {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        });
}

void MeshRenderer::render(Scene& scene, Camera* camera, RenderGraph& rg) {
    addShadowPass(scene, camera, rg);

    rg.addPass("scenePass", &camera, [this, &scene, camera](RenderContext* renderContext) {
        DirectionLight* mainLight = scene.GetMainDirectionalLight();
        if (!mainLight) {
            return;
        }

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

        renderContext->beginRendering(OriginFramebuffer);
        glViewport(0, 0, renderWidth, renderHeight);
        renderContext->setDepthStencilState(depthStencilState);
        GraphicsPipeline lightDebugPipeline = graphicsPipeline;
        lightDebugPipeline.shader = lightDebugShader.get();
        renderContext->bindPipeline(lightDebugPipeline);
        glm::mat4 light_model = glm::mat4(1.0f);
        lightDebugShader->use();
        lightDebugShader->setMat4("projection", projection);
        lightDebugShader->setMat4("view", camera->GetViewMatrix());
        //Render a large point light source to act as a unidirectional light source
        glm::vec3 directionPos = mainLight->getDirection() * (-80.0f);
        light_model = glm::translate(light_model, directionPos);
        light_model = glm::scale(light_model, glm::vec3(3.0f, 3.0f, 3.0f));
        lightDebugShader->setMat4("model", light_model);
        lightDebugShader->setVec3("lightColor", mainLight->getColor());
        lightDebugShader->setFloat("lightVisualIntensity", 4.0f);
        renderContext->bindVertexArray(cubeVAO);
        renderContext->drawArrays(0, 36);

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
        renderContext->bindTexture(mainLight->getShadow()->depthMap->id, 1);

        //render plane
        MaterialSystem::bindMaterialAsset(floorMaterial_.get(), *defaultLitShader, 2);
        defaultLitShader->setMat4("model", glm::mat4(1.0f));
        renderContext->bindVertexArray(planeVAO);
        renderContext->drawArrays(0, 6);

        renderSceneObjectsWithMaterials(renderContext, assetManager_, scene, camera, mainLight, projection, graphicsPipeline, defaultLitShader, iblSystem_.get());

        renderContext->bindVertexArray(0);
        renderContext->endRendering();
        });
}

unsigned int MeshRenderer::getTargetColorTextureID(int  attachment) {

    if (attachment >= OriginFramebuffer.colorAttachments.size()) {
        return 0;
    }
    return OriginFramebuffer.colorAttachments[attachment].texture->id;

}

FrameBufferInfo* MeshRenderer::getTargetFrameBuffer() {
    return &OriginFramebuffer;
}

MeshRenderer::~MeshRenderer() {
    delete fboColorTexture;
    delete fboDepthTexture;
}
NAMESPACE_END
