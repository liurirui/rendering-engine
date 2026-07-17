#include "MeshRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "Base/Light.h"
#include "RenderGraph/RenderGraph.h"
#include "Base/Material.h"
#include "Base/AssetManager.h"
#include "Renderer/MaterialSystem.h"
#include "Base/Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

NAMESPACE_START


static Shader& resolveMaterialShader(AssetManager& assetManager, Mesh* mesh, const std::shared_ptr<Shader>& fallbackShader) {
    if (mesh && mesh->materialAsset && mesh->materialAsset->shader.isValid()) {
        if (std::shared_ptr<Shader> shader = assetManager.getShaderLibrary().get(mesh->materialAsset->shader)) {
            return *shader;
        }
    }
    return *fallbackShader;
}

static void setupLitShaderCommon(Shader& shader, Scene& scene, Camera* camera, DirectionLight* mainLight, const glm::mat4& projection) {
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", camera->GetViewMatrix());
    shader.setVec3("viewPos", camera->Position);
    shader.setVec3("lightPos", mainLight->getDirection() * (-10.0f));
    shader.setMat4("lightSpaceMatrix", mainLight->LightSpaceMatrix);
    shader.setVec3("light.direction", mainLight->getDirection());
    shader.setVec3("light.color", mainLight->getColor());
    shader.setFloat("light.intensity", mainLight->getIntensity());
    shader.setVec3("ambient", 0.3f, 0.3f, 0.3f);
    shader.setVec3("diffuse", 0.6f, 0.6f, 0.6f);
    shader.setVec3("specular", 1.0f, 1.0f, 1.0f);
    shader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
    shader.setFloat("shininess", 32.0f);
    shader.setBool("isMirror", false);
    shader.setBool("isGlass", false);
    shader.setInt("shadowMap", 1);

    for (int i = 1; i < scene.lights.size(); i++) {
        PointLight* light = dynamic_cast<PointLight*>(scene.lights[i]);
        if (!light) {
            continue;
        }
        shader.setVec3(("point[" + std::to_string(i - 1) + "].position").c_str(), light->getPosition());
        shader.setVec3(("point[" + std::to_string(i - 1) + "].color").c_str(), light->getColor());
        shader.setFloat(("point[" + std::to_string(i - 1) + "].intensity").c_str(), light->getIntensity());
        shader.setFloat(("point[" + std::to_string(i - 1) + "].constant").c_str(), light->getConstantAttenuation());
        shader.setFloat(("point[" + std::to_string(i - 1) + "].linear").c_str(), light->getLinearAttenuation());
        shader.setFloat(("point[" + std::to_string(i - 1) + "].quadratic").c_str(), light->getQuadraticAttenuation());
    }
}

static void renderSceneObjectsWithMaterials(RenderContext* renderContext, AssetManager& assetManager, Scene& scene, Camera* camera, DirectionLight* mainLight, const glm::mat4& projection, GraphicsPipeline basePipeline, const std::shared_ptr<Shader>& fallbackShader) {
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
                setupLitShaderCommon(shader, scene, camera, mainLight, projection);
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
void MeshRenderer::setFloorTexture(Texture2D* texture) {
    floor = texture;
}

void MeshRenderer::addShadowPass(Scene& scene, Camera* camera, RenderGraph& rg) {
    if (!scene.GetMainDirectionalLight()) {
        return;
    }

    rg.addPass("shadowPass", &camera, [this, &scene, camera](RenderContext* renderContext) {
        depthStencilState.depthTest = true;
        depthStencilState.depthWrite = true;
        renderContext->beginRendering(scene.GetMainDirectionalLight()->getShadow()->DepthMapFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline_DepthMap);
        depthMapShader->setMat4("lightSpaceMatrix", scene.GetMainDirectionalLight()->LightSpaceMatrix);
        //plane(shadow)
        depthMapShader->setMat4("model", glm::mat4(1.0f));
        renderContext->bindVertexArray(planeVAO);
        renderContext->drawArrays(0, 6);
        //gameObject(shadow)
        scene.DrawObjects(*depthMapShader);
        renderContext->endRendering();
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
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
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
                renderContext->bindVertexArray(cubeVAO);
                renderContext->drawArrays(0, 36);
            }
        }

        graphicsPipeline.shader = defaultLitShader.get();
        renderContext->bindPipeline(graphicsPipeline);
        setupLitShaderCommon(*defaultLitShader, scene, camera, mainLight, projection);
        defaultLitShader->setInt("baseTexture", 0);
        renderContext->bindTexture(mainLight->getShadow()->depthMap->id, 1);

        //render plane
        defaultLitShader->setMat4("model", glm::mat4(1.0));
        renderContext->bindVertexArray(planeVAO);
        if (floor) {
            renderContext->bindTexture(floor->id, 0);
        }
        renderContext->drawArrays(0, 6);

        renderSceneObjectsWithMaterials(renderContext, assetManager_, scene, camera, mainLight, projection, graphicsPipeline, defaultLitShader);

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
    delete baseTexture;
}
NAMESPACE_END
