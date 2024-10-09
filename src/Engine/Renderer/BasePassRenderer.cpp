#include "BasePassRenderer.h"
#include"Base/ShaderCode.h"
#include"Base/Shader.h"
#include"Base/Camera.h"
#include"Base/Mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include"RenderGraph/RenderGraph.h"

NAMESPACE_START

template <typename T>
struct has_preRead {
    // 使用 if constexpr 来检查 T 类型是否具有 preRead 成员函数
    static constexpr bool value = [](){
        if constexpr (std::is_same<void, decltype(std::declval<T>().preRead(
            typename T::Desc{}, 0u, std::declval<void*>())) > ::value) {
            return true;
        }
        else {
            return false;
        }
    };
};

BasePassRenderer::BasePassRenderer() {

    depthStencilState.depthTest = true;

    mesh = new Mesh();
    glm::vec3* vertex = new glm::vec3[3]{ glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.5f, -0.5f, -0.5f) , glm::vec3(0.5f,  0.5f, -0.5f) };
    glm::vec3* normal = new glm::vec3[3]{ glm::vec3(0.0f,  0.0f, 1.0f), glm::vec3(0.0f,  0.0f, 1.0f) , glm::vec3(0.0f,  0.0f, 1.0f) };
    glm::vec2* uv = new glm::vec2[3]{ glm::vec2(0.0f,  0.0f), glm::vec2(1.0f,  0.0f) , glm::vec2(1.0f,  1.0f) };

    mesh->createVertextBuffer(3, vertex, normal, uv);

    unsigned int* indices = new unsigned int[3]{ 0,1,2 };//只有一个三角形
    mesh->createTriangleIndexBuffer(1, indices);

    lightingShader = TRefCountPtr<Shader>(new Shader(Vertbasic_lighting, Fragbasic_lighting));

    baseTexture = RenderContext::getInstance()->loadTexture2D("E:/learnRenderC++/resources/textures/awesomeface.png");
    normalTexture = RenderContext::getInstance()->loadTexture2D("E: / learnRenderC++ / resources / textures / brickwall_normal.jpg");

    fboColorTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);

    fboDepthTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth24_Stencil8, RenderContext::getInstance()->windowsWidth,
        RenderContext::getInstance()->windowsHeight);

    ColorAttachment colorAttachment;
    colorAttachment.attachment = 0;
    colorAttachment.texture = fboColorTexture;
    colorAttachment.clearColor = glm::vec4(1,1,1,1);
    framebuffer.colorAttachments.emplace_back(std::move(colorAttachment));

    framebuffer.depthStencilAttachment.texture = fboDepthTexture;

    graphicsPipeline.shader = lightingShader.getPtr();
    
    PipelineColorBlendAttachment pipelineColorBlendAttachment;
    pipelineColorBlendAttachment.blendState.enabled = true;
    
    graphicsPipeline.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);
}

void BasePassRenderer::render(Camera* camera, RenderGraph & rg){

    const char* passName = "basePass";

    rg.addPass(passName, camera,
        [this, camera](RenderContext * renderContext) {
        
        glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

        renderContext->beginRendering(framebuffer);

        // render
           // ------
        renderContext->setDepthStencilState(depthStencilState);
        
        renderContext->bindPipeline(graphicsPipeline);
        
        //renderContext->setClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        //renderContext->setClearAction(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        int errorCode = glGetError();
        // be sure to activate shader when setting uniforms/drawing objects
        lightingShader.getPtr()->use();

        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 0.5f, 0.31f);

        lightingShader.getPtr()->setVec3("lightColor", 1.0f, 1.0f, 1.0f);

        lightingShader.getPtr()->setVec3("lightPos", lightPos);
  
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera->GetViewMatrix();
        lightingShader.getPtr()->setMat4("projection", projection);
        lightingShader.getPtr()->setMat4("view", view);
       
        // world transformation
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.getPtr()->setMat4("model", model);

        lightingShader.getPtr()->setInt("baseTexture", 0);
        lightingShader.getPtr()->setInt("normalTexture", 1);
        
        renderContext->bindTexture(baseTexture->id, 0);
        renderContext->bindTexture(normalTexture->id, 1);
       
        errorCode = glGetError();
        // render the cube

        renderContext->bindVertexBuffer(mesh->vertexAttributeBufferID);

        renderContext->bindIndexBuffer(mesh->indexBufferID);

        renderContext->drawElements(mesh->numTriangle * 3, 0);

        renderContext->endRendering();
        
    });
}

unsigned int BasePassRenderer::getTargetColorTexture(int  attachment) {

    if (attachment >= framebuffer.colorAttachments.size()) {
        return 0;
    }
    return framebuffer.colorAttachments[attachment].texture->id;
}

BasePassRenderer::~BasePassRenderer() {
    
    if (baseTexture) {
        delete baseTexture;
    }
    baseTexture = nullptr;

    if (normalTexture) {
        delete normalTexture;
    }
    normalTexture = nullptr;

    
}

NAMESPACE_END



