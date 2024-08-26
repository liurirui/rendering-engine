#include "MeshRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "RenderGraph/RenderGraph.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

NAMESPACE_START
MeshRenderer::MeshRenderer(const std::string& modelPath) {
    depthStencilState.depthTest = true;
    modelSample = new Model(modelPath);
    string modelPath1 = "E:/download_model/cat_mask.fbx";
    modelSample1 = new Model(modelPath1);
    string modelPath2 = "E:/download_model/glass_11_2.fbx";
    modelSample2 = new Model(modelPath2);
    lightingShader = TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_lighting));
    lightingShader_cube= TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_cube));
    hight_lightingShader= TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_highlight));
    cartoonShader= TRefCountPtr<Shader>(new Shader(Vert_quad, Fragmodel_cartoon));
    blurShader = TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_blur));
    lastShader= TRefCountPtr<Shader>(new Shader(Vert_quad, Frag_last));
    ColorTextureMap["hands"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/objects/nanosuit/hand_dif.png");
    ColorTextureMap["Visor"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/objects/nanosuit/glass_dif.png");
    ColorTextureMap["Body"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/objects/nanosuit/body_dif.png");
    ColorTextureMap["Helmet"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/objects/nanosuit/helmet_diff.png");
    ColorTextureMap["Legs"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/objects/nanosuit/leg_dif.png");
    ColorTextureMap["Arms"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/objects/nanosuit/arm_dif.png");
    ColorTextureMap["glass"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/textures/skybox/back.jpg");
    ColorTextureMap["app"] = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/textures/background.jpg");

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
    graphicsPipeline1.shader = lightingShader.getPtr();
    PipelineColorBlendAttachment pipelineColorBlendAttachment1;
    pipelineColorBlendAttachment1.blendState.enabled = true;
    graphicsPipeline1.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment1);

    //set HightLightFramebuffer's Texture Attachments
    fboBrightTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment brightAttachment;
    brightAttachment.attachment = 0;
    brightAttachment.texture = fboBrightTexture;
    brightAttachment.clearColor = glm::vec4(0.2, 0.1, 0.3, 1);
    HightLightFramebuffer.colorAttachments.emplace_back(std::move(brightAttachment));

    // set cartoonFramebuffer's Texture Attachments
    fboCartoonColorTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment cartoonColorAttachment;
    cartoonColorAttachment.attachment = 0;
    cartoonColorAttachment.texture = fboCartoonColorTexture;
    cartoonColorAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    CartoonFramebuffer.colorAttachments.emplace_back(std::move(cartoonColorAttachment));

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

    //set lastFramebuffer's Texture Attachments
    fbolastColorTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);
    ColorAttachment lastColorAttachment;
    lastColorAttachment.attachment = 0;
    lastColorAttachment.texture = fbolastColorTexture;
    lastColorAttachment.clearColor = glm::vec4(1, 1, 1, 1);
    LastFramebuffer.colorAttachments.emplace_back(std::move(lastColorAttachment));

    if (!VBO) {
        VBO = RenderContext::getInstance()->createVertexBuffer(quadVertices, sizeof(quadVertices));
    }
    if (!quadVAO) {
        quadVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(VBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(VBO, quadVAO, 2, 4 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(VBO, quadVAO, 2, 4 * sizeof(float), 1, 2);
    }
    if (!cubeVBO) {
        cubeVBO = RenderContext::getInstance()->createVertexBuffer(cubevertices, sizeof(cubevertices));
    }
    if (!cubeVAO) {
        cubeVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(cubeVBO);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 0, 0);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 3, 8 * sizeof(float), 1, 3);
        RenderContext::getInstance()->setUpVertexBufferLayoutInfo(cubeVBO, cubeVAO, 2, 8 * sizeof(float), 2, 6);
    }
    lightPositions.push_back(glm::vec3(0.0f, 15.0f, 1.5f));
    lightPositions.push_back(glm::vec3(-2.0f, 0.5f, -3.0f));
    lightPositions.push_back(glm::vec3(3.0f, 8.5f, 1.0f));
    lightPositions.push_back(glm::vec3(-8.0f, 2.4f, -1.0f));
    lightColors.push_back(glm::vec3(50.0f, 50.0f, 50.0f));
    lightColors.push_back(glm::vec3(100.0f, 0.0f, 0.0f));
    lightColors.push_back(glm::vec3(0.0f, 0.0f, 150.0f));
    lightColors.push_back(glm::vec3(0.0f, 50.0f, 0.0f));
}

void MeshRenderer::render(Camera* camera, RenderGraph& rg) {
    const char* passName = "modelPass";
    rg.addPass(passName, camera, [this, camera](RenderContext* renderContext) {
        renderContext->beginRendering(OriginFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline1);
        glm::mat4 light_model=glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera->GetViewMatrix();
        int errorCode = glGetError();
        lightingShader_cube.getPtr()->use();
        errorCode = glGetError();
        lightingShader_cube.getPtr()->setMat4("projection", projection);
        lightingShader_cube.getPtr()->setMat4("view", view);
        for (int i = 0; i < lightPositions.size(); i++) {
            light_model = glm::mat4(1.0f);
            light_model = glm::translate(light_model, glm::vec3(lightPositions[i]));
            light_model = glm::scale(light_model, glm::vec3(0.25f, 0.25f, 0.25f));
            lightingShader_cube.getPtr()->setMat4("model", light_model); 
            lightingShader_cube.getPtr()->setVec3("lightColor", lightColors[i]);
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
        lightingShader.getPtr()->setVec3("light.direction", -0.5f, -0.8f, -0.3f);
        lightingShader.getPtr()->setVec3("light.ambient", 0.3f, 0.3f, 0.3f);
        lightingShader.getPtr()->setVec3("light.diffuse", 0.6f, 0.6f, 0.6f);
        lightingShader.getPtr()->setVec3("light.specular", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setFloat("shininess", 32.0f);
        lightingShader.getPtr()->setBool("isMirror", false);
        lightingShader.getPtr()->setVec3("point[0].Position", lightPositions[0]);
        lightingShader.getPtr()->setVec3("point[1].Position", lightPositions[1]);
        lightingShader.getPtr()->setVec3("point[2].Position", lightPositions[2]);
        lightingShader.getPtr()->setVec3("point[3].Position", lightPositions[3]);
        lightingShader.getPtr()->setVec3("point[0].Color", lightColors[0]);
        lightingShader.getPtr()->setVec3("point[1].Color", lightColors[1]);
        lightingShader.getPtr()->setVec3("point[2].Color", lightColors[2]);
        lightingShader.getPtr()->setVec3("point[3].Color", lightColors[3]);
        bool IsGlass = false;
        lightingShader.getPtr()->setInt("baseTexture", 0);
        errorCode = glGetError();
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
            // 清除之前绑定的纹理和缓冲区
            renderContext->bindVertexBuffer(0);
            renderContext->bindIndexBuffer(0);

            lightingShader.getPtr()->setBool("isGlass", IsGlass);
            // 设置纹理和缓冲区
            if (baseTexture) {
                renderContext->bindTexture(baseTexture->id, 0);
            }
            renderContext->bindVertexBuffer(mesh->vertexAttributeBufferID);
            renderContext->bindIndexBuffer(mesh->indexBufferID);

            // 渲染当前网格
            renderContext->drawElements(mesh->numTriangle * 3, 0);
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

        //Achieving cartoon effects
        /*renderContext->beginRendering(Cartoonframebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        cartoonShader.getPtr()->use();
        errorCode = glGetError();
        renderContext->bindVertexBuffer(quadVAO);
        cartoonShader.getPtr()->setInt("cartoonTexture", 0);
        renderContext->bindTexture(Originframebuffer.colorAttachments[0].texture->id, 0);
        renderContext->drawArrays(0, 6);*/

        //Extract highlight part
        renderContext->beginRendering(HightLightFramebuffer);
        hight_lightingShader.getPtr()->use();
        errorCode = glGetError();
        hight_lightingShader.getPtr()->setInt("scene", 0);
        renderContext->bindTexture(OriginFramebuffer.colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        //Gaussian Blur
        bool horizontal = true;
        renderContext->beginRendering(PingpongFramebuffer[0]);
        renderContext->setDepthStencilState(depthStencilState);
        blurShader.getPtr()->use();
        errorCode = glGetError();
        blurShader.getPtr()->setInt("horizontal", horizontal);
        blurShader.getPtr()->setInt("image", 0);
        renderContext->bindTexture(HightLightFramebuffer.colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);

        renderContext->beginRendering(PingpongFramebuffer[1]);
        renderContext->setDepthStencilState(depthStencilState);
        blurShader.getPtr()->use();
        errorCode = glGetError();
        blurShader.getPtr()->setInt("horizontal", !horizontal);
        blurShader.getPtr()->setInt("image", 0);
        renderContext->bindTexture(PingpongFramebuffer[0].colorAttachments[0].texture->id, 0);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();

        //Calculate the final color
        renderContext->beginRendering(LastFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        lastShader.getPtr()->use();
        errorCode = glGetError();
        lastShader.getPtr()->setInt("scene", 0);
        lastShader.getPtr()->setInt("bloomBlur", 1);
        renderContext->bindTexture(OriginFramebuffer.colorAttachments[0].texture->id, 0);
        renderContext->bindTexture(PingpongFramebuffer[1].colorAttachments[0].texture->id, 1);
        renderContext->bindVertexBuffer(quadVAO);
        renderContext->drawArrays(0, 6);
        renderContext->endRendering();
        });
}

unsigned int MeshRenderer::getTargetColorTexture(int  attachment) {

    if (attachment >= LastFramebuffer.colorAttachments.size()) {
        return 0;
    }
    return LastFramebuffer.colorAttachments[attachment].texture->id;
}

MeshRenderer::~MeshRenderer() {
    delete modelSample;
    delete modelSample1;
    delete modelSample2;
}
NAMESPACE_END