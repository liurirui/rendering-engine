#include "MeshRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "Base/Light.h"
#include "RenderGraph/RenderGraph.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

NAMESPACE_START
MeshRenderer::MeshRenderer(const std::string& modelPath) {
    depthStencilState.depthTest = true;
    depthStencilState.depthWrite = true;
    modelSample = new Model(modelPath);
    string modelPath1 = "E:/download_model/cat_mask.fbx";
    modelSample1 = new Model(modelPath1);
    string modelPath2 = "E:/download_model/glass_11_2.fbx";
    modelSample2 = new Model(modelPath2);

    lightingShader = TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_lighting));
    lightingShader_cube= TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_cube));

    ColorTextureMap["hands"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/objects/nanosuit/hand_dif.png");
    ColorTextureMap["Visor"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/objects/nanosuit/glass_dif.png");
    ColorTextureMap["Body"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/objects/nanosuit/body_dif.png");
    ColorTextureMap["Helmet"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/objects/nanosuit/helmet_diff.png");
    ColorTextureMap["Legs"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/objects/nanosuit/leg_dif.png");
    ColorTextureMap["Arms"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/objects/nanosuit/arm_dif.png");
    ColorTextureMap["glass"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/textures/skybox/back.jpg");
    ColorTextureMap["app"] = RenderContext::getInstance()->loadTexture2D("E:/rendering-engine/resources/textures/background.jpg");

    //set  Originframebuffer's Texture Attachments
    fboColorTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    fboDepthTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth24_Stencil8, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment colorAttachment;
    colorAttachment.attachment = 0;
    colorAttachment.texture = fboColorTexture;
    colorAttachment.clearColor = glm::vec4(0.2, 0.1, 0.3, 1);
    OriginFramebuffer.colorAttachments.emplace_back(std::move(colorAttachment));
    OriginFramebuffer.depthStencilAttachment.texture = fboDepthTexture;
    graphicsPipeline.shader = lightingShader.getPtr();
    PipelineColorBlendAttachment pipelineColorBlendAttachment;
    pipelineColorBlendAttachment.blendState.enabled = true;
    graphicsPipeline.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);

    if (!cubeVBO) {
        cubeVBO = RenderContext::getInstance()->createVertexBuffer(cubevertices, sizeof(cubevertices));
    }
    if (!cubeVAO) {
        cubeVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(cubeVBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 1, 3);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 2, 8 * sizeof(float), 2, 6);
    }

    directiontLight = new DirectionLight(glm::vec3(-0.5f, -0.8f, -0.3f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
    pointLights.push_back(new PointLight(glm::vec3(0.0f, 15.0f, 1.5f), glm::vec3(15.0f, 15.0f, 15.0f), 0.5f));
    pointLights.push_back(new PointLight(glm::vec3(-2.0f, 0.5f, -3.0f), glm::vec3(30.0f, 0.0f, 0.0f), 0.4f));
    pointLights.push_back(new PointLight(glm::vec3(3.0f, 8.5f, 1.0f), glm::vec3(0.0f, 0.0f, 45.0f), 0.3f));
    pointLights.push_back(new PointLight(glm::vec3(-8.0f, 2.4f, -1.0f), glm::vec3(0.0f, 15.0f, 0.0f), 0.3f));
}

void MeshRenderer::render(Camera* camera, RenderGraph& rg) {
    const char* passName = "modelPass";
    rg.addPass(passName, camera, [this, camera](RenderContext* renderContext) {
        renderContext->beginRendering(OriginFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline);
        glm::mat4 light_model=glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera->GetViewMatrix();
        int errorCode = glGetError();
        lightingShader_cube.getPtr()->use();
        errorCode = glGetError();
        lightingShader_cube.getPtr()->setMat4("projection", projection);
        lightingShader_cube.getPtr()->setMat4("view", view);

        //render light cube
        for (int i = 0; i < pointLights.size(); i++) {
            light_model = glm::mat4(1.0f);
            light_model = glm::translate(light_model, glm::vec3(pointLights[i]->getPosition()));
            light_model = glm::scale(light_model, glm::vec3(0.25f, 0.25f, 0.25f));
            lightingShader_cube.getPtr()->setMat4("model", light_model); 
            lightingShader_cube.getPtr()->setVec3("lightColor", pointLights[i]->getColor());
            renderContext->bindVertexBuffer(cubeVAO);
            renderContext->drawArrays(0, 36);
        }
        
        lightingShader.getPtr()->use();
        errorCode = glGetError();
        lightingShader.getPtr()->setMat4("projection", projection);
        lightingShader.getPtr()->setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        lightingShader.getPtr()->setMat4("model", model);
        // light properties
        lightingShader.getPtr()->setVec3("viewPos", camera->Position);
        lightingShader.getPtr()->setVec3("light.direction", directiontLight->getDirection());
        lightingShader.getPtr()->setVec3("light.color", directiontLight->getColor());
        lightingShader.getPtr()->setFloat("light.intensity", directiontLight->getIntensity());
        lightingShader.getPtr()->setVec3("ambient", 0.3f, 0.3f, 0.3f);
        lightingShader.getPtr()->setVec3("diffuse", 0.6f, 0.6f, 0.6f);
        lightingShader.getPtr()->setVec3("specular", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setFloat("shininess", 32.0f);
        lightingShader.getPtr()->setBool("isMirror", false);
        for (int i = 0; i<pointLights.size(); ++i) {
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i) + "].position").c_str(), pointLights[i]->getPosition());
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i) + "].color").c_str(), pointLights[i]->getColor());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].intensity").c_str(), pointLights[i]->getIntensity());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].constant").c_str(), pointLights[i]->getConstantAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].linear").c_str(), pointLights[i]->getLinearAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].quadratic").c_str(), pointLights[i]->getQuadraticAttenuation());
        }
        bool IsGlass = false;
        lightingShader.getPtr()->setInt("baseTexture", 0);
        //render first model
        for (const Mesh* mesh : modelSample->meshes) {
            if (ColorTextureMap.find(mesh->nowName) != ColorTextureMap.end()) {
                if (mesh->nowName == "Visor") {
                    IsGlass = true;
                }
                else {
                    IsGlass = false;
                }
                baseTexture = ColorTextureMap[mesh->nowName];
            }
            else {
                baseTexture = nullptr;
            }
            renderContext->bindVertexBuffer(0);
            renderContext->bindIndexBuffer(0);
            lightingShader.getPtr()->setBool("isGlass", IsGlass);
            if (baseTexture) {
                renderContext->bindTexture(baseTexture->id, 0);
                renderContext->bindVertexBuffer(mesh->vertexAttributeBufferID);
                renderContext->bindIndexBuffer(mesh->indexBufferID);
                renderContext->drawElements(mesh->numTriangle * 3, 0);
            }
        }

        lightingShader.getPtr()->setBool("isGlass", true);
        model = glm::translate(model, glm::vec3(4.0f, 4.0f, -2.0f));
        model = glm::mat4(25.0f);
        lightingShader.getPtr()->setMat4("model", model);
        lightingShader.getPtr()->setBool("isMirror", true);
        baseTexture = ColorTextureMap["glass"];
        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 0.0f, 0.0f);
        //first mask
        for (const Mesh* mesh : modelSample1->meshes) {
            renderContext->bindTexture(baseTexture->id, 0);
            renderContext->bindVertexBuffer(mesh->vertexAttributeBufferID);
            renderContext->bindIndexBuffer(mesh->indexBufferID);

            // 渲染当前网格
            renderContext->drawElements(mesh->numTriangle * 3, 0);
        }

        model = glm::mat4(0.01f);
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(-500.0f, 200.0f, 20.0f));
        lightingShader.getPtr()->setMat4("model", model);
        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        for (const Mesh* mesh : modelSample2->meshes) {       
            baseTexture = nullptr;
            if (mesh->nowName == "frame") {
                lightingShader.getPtr()->setBool("isMirror", false);
                baseTexture = ColorTextureMap["app"];
            }
            else {
                lightingShader.getPtr()->setBool("isMirror", true);
                baseTexture = ColorTextureMap["glass"];
            }
            renderContext->bindTexture(baseTexture->id, 0);
            renderContext->bindVertexBuffer(mesh->vertexAttributeBufferID);
            renderContext->bindIndexBuffer(mesh->indexBufferID);
            // 渲染当前网格
            renderContext->drawElements(mesh->numTriangle * 3, 0);
        }
        renderContext->bindVertexBuffer(0);
        renderContext->bindIndexBuffer(0);
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
    delete modelSample;
    delete modelSample1;
    delete modelSample2;
}
NAMESPACE_END