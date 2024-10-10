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
    modelSample1 = new Model(modelPath);
    string modelPath2 = "E:/download_model/cat_mask.fbx";
    modelSample2 = new Model(modelPath2);
    string modelPath3 = "E:/download_model/glass_11_2.fbx";
    modelSample3 = new Model(modelPath3);
   
    //Storing the model's mesh
    Renderable* renderable = nullptr;
    for (Mesh* mesh : modelSample1->meshes) {
        renderable=new Renderable(mesh, false);
        renderable->modelNumber = 1;
        renderable->transform.setTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
        scene->addRenderable(renderable);
    }
    for (Mesh* mesh : modelSample2->meshes) {
        renderable = new Renderable(mesh, true);
        renderable->modelNumber = 2;
        renderable->transform.setTransform(glm::vec3(3.0f, 1.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(25.0f, 25.0f, 25.0f));
        scene->addRenderable(renderable);
    }
    for (Mesh* mesh : modelSample3->meshes) {
        if (mesh->nowName == "frame")  renderable = new Renderable(mesh, false);
        else  renderable = new Renderable(mesh, true);
        renderable->modelNumber = 3;
        renderable->transform.setTransform(glm::vec3(3.0f, 1.0f, -0.6f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.01f, 0.01f, 0.01f));
        scene->addRenderable(renderable);
    }
    depthMapShader = TRefCountPtr<Shader>(new Shader(Vert_depth_map, Frag_depth_map));
    lightingShader = TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_lighting));
    lightingShader_cube= TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_cube));

    ColorTextureMap["hands"] = RenderContext::getInstance()->loadTexture2D("E:/resources/objects/nanosuit/hand_dif.png");
    ColorTextureMap["Visor"] = RenderContext::getInstance()->loadTexture2D("E:/resources/objects/nanosuit/glass_dif.png");
    ColorTextureMap["Body"] = RenderContext::getInstance()->loadTexture2D("E:/resources/objects/nanosuit/body_dif.png");
    ColorTextureMap["Helmet"] = RenderContext::getInstance()->loadTexture2D("E:/resources/objects/nanosuit/helmet_diff.png");
    ColorTextureMap["Legs"] = RenderContext::getInstance()->loadTexture2D("E:/resources/objects/nanosuit/leg_dif.png");
    ColorTextureMap["Arms"] = RenderContext::getInstance()->loadTexture2D("E:/resources/objects/nanosuit/arm_dif.png");
    ColorTextureMap["glass"] = RenderContext::getInstance()->loadTexture2D("E:/resources/textures/skybox/back.jpg");
    ColorTextureMap["app"] = RenderContext::getInstance()->loadTexture2D("E:/resources/textures/background.jpg");
    ColorTextureMap["plane"]= RenderContext::getInstance()->loadTexture2D("E:/resources/textures/wood.png");

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

    depthMap = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    DepthMapFramebuffer.depthStencilAttachment.texture = depthMap;
    DepthMapFramebuffer.depthStencilAttachment.useStencil = false;
    graphicsPipeline_DepthMap.shader = depthMapShader.getPtr();
    //graphicsPipeline_DepthMap.rasterizationState.cullMode = CullMode::Front;
    

    if (!cubeVBO) {
        cubeVBO = RenderContext::getInstance()->createVertexBuffer(cubeVertices, sizeof(cubeVertices));
    }
    if (!cubeVAO) {
        cubeVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(cubeVBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 1, 3);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 2, 8 * sizeof(float), 2, 6);
    }
    if (!planeVBO) {
        planeVBO = RenderContext::getInstance()->createVertexBuffer(planeVertices, sizeof(planeVertices));
    }
    if (!planeVAO) {
        planeVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(planeVBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(planeVBO, planeVAO, 3, 8 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(planeVBO, planeVAO, 3, 8 * sizeof(float), 1, 3);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(planeVBO, planeVAO, 2, 8 * sizeof(float), 2, 6);
    }
    directiontLight = new DirectionLight(glm::vec3(-0.5f, -0.8f, -0.5f), glm::vec3(1.5f, 1.5f, 1.5f), 1.0f);
    pointLights.push_back(new PointLight(glm::vec3(0.0f, 12.0f, 1.5f), glm::vec3(10.0f, 0.0f, 0.0f), 0.4f));
    pointLights.push_back(new PointLight(glm::vec3(-2.0f, 0.5f, -3.0f), glm::vec3(0.0f, 10.0f, 0.0f), 0.4f));
    pointLights.push_back(new PointLight(glm::vec3(3.0f, 8.5f, 1.0f), glm::vec3(0.0f, 0.0f, 12.0f), 0.4f));
    pointLights.push_back(new PointLight(glm::vec3(-8.0f, 2.4f, -1.0f), glm::vec3(10.0f, 10.0f, 10.0f), 0.2f));
}

void MeshRenderer::render(Camera* camera, RenderGraph& rg) {
    const char* passName = "modelPass";
    rg.addPass(passName, camera, [this, camera](RenderContext* renderContext) {
        //Rendering shadow maps
        renderContext->beginRendering(directiontLight->getShadow()->DepthMapFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline_DepthMap);
        depthMapShader.getPtr()->use();
        int errorCode = glGetError();
        glm::mat4 model = glm::mat4(1.0f);
        depthMapShader.getPtr()->setMat4("lightSpaceMatrix",directiontLight->calculateLightSpaceMatrix());
        depthMapShader.getPtr()->setMat4("model", model);

        //plane(shadow)
        renderContext->bindVertexBuffer(planeVAO);
        renderContext->drawArrays(0, 6);

        for (Renderable* renderable : scene->Opaque) {
            depthMapShader.getPtr()->setMat4("model", renderable->transform.modelMatrix);
            renderContext->bindVertexBuffer(renderable->mesh->vertexAttributeBufferID);
            renderContext->bindIndexBuffer(renderable->mesh->indexBufferID);
            renderContext->drawElements(renderable->mesh->numTriangle * 3, 0);
        }

        for (Renderable* renderable : scene->Translucent) {
            depthMapShader.getPtr()->setMat4("model", renderable->transform.modelMatrix);
            renderContext->bindVertexBuffer(renderable->mesh->vertexAttributeBufferID);
            renderContext->bindIndexBuffer(renderable->mesh->indexBufferID);
            renderContext->drawElements(renderable->mesh->numTriangle * 3, 0);
        }

        // Start render scene
        renderContext->beginRendering(OriginFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline);
        glm::mat4 light_model=glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera->GetViewMatrix();
        errorCode = glGetError();
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
        // light properties
        lightingShader.getPtr()->setVec3("viewPos", camera->Position);
        lightingShader.getPtr()->setVec3("lightPos", directiontLight->getDirection()*(-10.0f));
        lightingShader.getPtr()->setMat4("lightSpaceMatrix", directiontLight->calculateLightSpaceMatrix());
        lightingShader.getPtr()->setVec3("light.direction", directiontLight->getDirection());
        lightingShader.getPtr()->setVec3("light.color", directiontLight->getColor());
        lightingShader.getPtr()->setFloat("light.intensity", directiontLight->getIntensity());
        lightingShader.getPtr()->setVec3("ambient", 0.3f, 0.3f, 0.3f);
        lightingShader.getPtr()->setVec3("diffuse", 0.6f, 0.6f, 0.6f);
        lightingShader.getPtr()->setVec3("specular", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setFloat("shininess", 32.0f);
        lightingShader.getPtr()->setBool("isMirror", false);
        bool IsGlass = false;
        lightingShader.getPtr()->setInt("baseTexture", 0);
        lightingShader.getPtr()->setInt("shadowMap", 1);
        renderContext->bindTexture(directiontLight->getShadow()->depthMap->id, 1);
        for (int i = 0; i<pointLights.size(); ++i) {
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i) + "].position").c_str(), pointLights[i]->getPosition());
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i) + "].color").c_str(), pointLights[i]->getColor());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].intensity").c_str(), pointLights[i]->getIntensity());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].constant").c_str(), pointLights[i]->getConstantAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].linear").c_str(), pointLights[i]->getLinearAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].quadratic").c_str(), pointLights[i]->getQuadraticAttenuation());
        }
        
        //render plane
        lightingShader.getPtr()->setMat4("model", model);
        baseTexture = ColorTextureMap["plane"];
        renderContext->bindVertexBuffer(planeVAO);
        renderContext->bindTexture(baseTexture->id, 0);
        renderContext->drawArrays(0, 6);

        //Rendering Opaque Meshes
        lightingShader.getPtr()->setBool("isMirror", false);
        for (Renderable* renderable : scene->Opaque) {
            if (renderable->modelNumber == 1&& ColorTextureMap.find(renderable->mesh->nowName) != ColorTextureMap.end()) {
                if (renderable->mesh->nowName == "Visor")  IsGlass = true;
                else  IsGlass = false;
                baseTexture = ColorTextureMap[renderable->mesh->nowName];
                lightingShader.getPtr()->setBool("isGlass", IsGlass);
            }
            else if (renderable->modelNumber == 3)  baseTexture = ColorTextureMap["app"];
           
            if (baseTexture) {
                lightingShader.getPtr()->setMat4("model", renderable->transform.modelMatrix);
                renderContext->bindTexture(baseTexture->id, 0);
                renderContext->bindVertexBuffer(renderable->mesh->vertexAttributeBufferID);
                renderContext->bindIndexBuffer(renderable->mesh->indexBufferID);
                renderContext->drawElements(renderable->mesh->numTriangle * 3, 0);
            }
            baseTexture = nullptr;
        }

        //Sort transparent mesh from far to near
        scene->sortTranslucentMeshes(camera->Position);

        //Rendering Translucent Meshes
        lightingShader.getPtr()->setBool("isGlass",true);
        lightingShader.getPtr()->setBool("isMirror", true);
        for (Renderable* renderable : scene->Translucent) {
           if (renderable->modelNumber == 2) {
                lightingShader.getPtr()->setVec3("objectColor", 1.0f, 0.0f, 0.0f);
                baseTexture = ColorTextureMap["glass"];
            }
            else if (renderable->modelNumber == 3) {
                lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                baseTexture = ColorTextureMap["glass"];
            }
            if (baseTexture) {
                lightingShader.getPtr()->setMat4("model", renderable->transform.modelMatrix);
                renderContext->bindTexture(baseTexture->id, 0);
                renderContext->bindVertexBuffer(renderable->mesh->vertexAttributeBufferID);
                renderContext->bindIndexBuffer(renderable->mesh->indexBufferID);
                renderContext->drawElements(renderable->mesh->numTriangle * 3, 0);
            }
            baseTexture = nullptr;
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
    delete modelSample1;
    delete modelSample2;
    delete modelSample3;
    delete fboColorTexture;
    delete fboDepthTexture;
    delete baseTexture;
    delete normalTexture;
    delete depthMap;
    delete directiontLight;
    for (PointLight* ptr : pointLights) {
        delete ptr;
    }
    pointLights.clear();
}
NAMESPACE_END