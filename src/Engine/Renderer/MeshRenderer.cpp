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
MeshRenderer::MeshRenderer(const std::vector<Renderable*>& translucentMeshes, const std::vector<Renderable*>& opaqueMeshes)
    : translucentMeshes(translucentMeshes), opaqueMeshes(opaqueMeshes){}

void MeshRenderer::render(Camera* camera, RenderGraph& rg) {
    const char* passName = "modelPass";
    rg.addPass(passName, camera, [this, camera](RenderContext* renderContext) {
        //Rendering shadow maps
        depthStencilState.depthTest = true;
        depthStencilState.depthWrite = true;
        int errorCode = glGetError();
        renderContext->beginRendering(directionLight->getShadow()->DepthMapFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline_DepthMap);
        errorCode = glGetError();
        depthMapShader.getPtr()->use();
        errorCode = glGetError();
        glm::mat4 model = glm::mat4(1.0f);
        depthMapShader.getPtr()->setMat4("lightSpaceMatrix",directionLight->LightSpaceMatrix);
        
        //plane(shadow)
        depthMapShader.getPtr()->setMat4("model", model);
        renderContext->bindVertexBuffer(planeVAO);
        renderContext->drawArrays(0, 6);

        for (Renderable* renderable : opaqueMeshes) {
            depthMapShader.getPtr()->setMat4("model", renderable->transform->modelMatrix);
            renderContext->bindVertexBuffer(renderable->mesh->vertexAttributeBufferID);
            renderContext->bindIndexBuffer(renderable->mesh->indexBufferID);
            renderContext->drawElements(renderable->mesh->numTriangle * 3, 0);
        }

        for (Renderable* renderable : translucentMeshes) {
            depthMapShader.getPtr()->setMat4("model", renderable->transform->modelMatrix);
            renderContext->bindVertexBuffer(renderable->mesh->vertexAttributeBufferID);
            renderContext->bindIndexBuffer(renderable->mesh->indexBufferID);
            renderContext->drawElements(renderable->mesh->numTriangle * 3, 0);
        }
        renderContext->endRendering();
        
        // Start render scene
        renderContext->beginRendering(OriginFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline);
        glm::mat4 light_model=glm::mat4(1.0f);
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera->GetViewMatrix();
        lightingShader_cube.getPtr()->use();
        lightingShader_cube.getPtr()->setMat4("projection", projection);
        lightingShader_cube.getPtr()->setMat4("view", view);

        //Render a large point light source to act as a unidirectional light source
        static glm::vec3 directionPos = directionLight->getDirection()*(-80.0f);
        light_model = glm::translate(light_model, directionPos);
        light_model = glm::scale(light_model, glm::vec3(3.0f, 3.0f, 3.0f));
        lightingShader_cube.getPtr()->setMat4("model", light_model);
        lightingShader_cube.getPtr()->setVec3("lightColor", directionLight->getColor());
        renderContext->bindVertexBuffer(cubeVAO);
        renderContext->drawArrays(0, 36);

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
        lightingShader.getPtr()->setMat4("projection", projection);
        lightingShader.getPtr()->setMat4("view", view);
        // light properties
        lightingShader.getPtr()->setVec3("viewPos", camera->Position);
        lightingShader.getPtr()->setVec3("lightPos", directionLight->getDirection()*(-10.0f));
        lightingShader.getPtr()->setMat4("lightSpaceMatrix", directionLight->LightSpaceMatrix);
        lightingShader.getPtr()->setVec3("light.direction", directionLight->getDirection());
        lightingShader.getPtr()->setVec3("light.color", directionLight->getColor());
        lightingShader.getPtr()->setFloat("light.intensity", directionLight->getIntensity());
        lightingShader.getPtr()->setVec3("ambient", 0.3f, 0.3f, 0.3f);
        lightingShader.getPtr()->setVec3("diffuse", 0.6f, 0.6f, 0.6f);
        lightingShader.getPtr()->setVec3("specular", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setFloat("shininess", 32.0f);
        lightingShader.getPtr()->setBool("isMirror", false);
        lightingShader.getPtr()->setBool("isGlass", false);
        bool IsGlass = false;
        lightingShader.getPtr()->setInt("baseTexture", 0);
        lightingShader.getPtr()->setInt("shadowMap", 1);
        renderContext->bindTexture(directionLight->getShadow()->depthMap->id, 1);
        for (int i = 0; i<pointLights.size(); i++) {
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i) + "].position").c_str(), pointLights[i]->getPosition());
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i) + "].color").c_str(), pointLights[i]->getColor());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].intensity").c_str(), pointLights[i]->getIntensity());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].constant").c_str(), pointLights[i]->getConstantAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].linear").c_str(), pointLights[i]->getLinearAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i) + "].quadratic").c_str(), pointLights[i]->getQuadraticAttenuation());
        }
        
        //render plane
        lightingShader.getPtr()->setMat4("model", model);
        renderContext->bindVertexBuffer(planeVAO);
        renderContext->bindTexture(floor->id, 0);
        renderContext->drawArrays(0, 6);

        //Rendering Opaque Meshes
        for (Renderable* renderable : opaqueMeshes) {
            if (renderable->modelNumber != 1 && renderable->modelNumber != 2 && ColorTextureMap.find(renderable->mesh->nowName) != ColorTextureMap.end()) {
                if (renderable->mesh->nowName == "Visor")  IsGlass = true;
                else  IsGlass = false;
                baseTexture = ColorTextureMap[renderable->mesh->nowName];
                lightingShader.getPtr()->setBool("isGlass", IsGlass);
            }

            else if (renderable->modelNumber == 1)  baseTexture = ColorTextureMap["app"];

            else if (renderable->modelNumber == 8)  baseTexture = ColorTextureMap["Backpack"];

            if (baseTexture) {
                lightingShader.getPtr()->setMat4("model", renderable->transform->modelMatrix);
                renderContext->bindTexture(baseTexture->id, 0);
                renderContext->bindVertexBuffer(renderable->mesh->vertexAttributeBufferID);
                renderContext->bindIndexBuffer(renderable->mesh->indexBufferID);
                renderContext->drawElements(renderable->mesh->numTriangle * 3, 0);
            }
            baseTexture = nullptr;
        }

        //Using the method of disabling depth writingand then rendering the transparent grid instead of sorting the transparent grid 
        // can also correctly render the transparent grid
        //scene->sortTranslucentMeshes(camera->Position);
        depthStencilState.depthWrite = false;
        renderContext->setDepthStencilState(depthStencilState);

        //Rendering Translucent Meshes
        lightingShader.getPtr()->setBool("isGlass",true);
        lightingShader.getPtr()->setBool("isMirror", true);
        for (Renderable* renderable : translucentMeshes) {
            if (renderable->modelNumber == 1) {
                lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
                baseTexture = ColorTextureMap["glass"];
            }
            else if (renderable->modelNumber == 2) {
                lightingShader.getPtr()->setVec3("objectColor", 0.0f, 0.0f, 10.0f);
                baseTexture = ColorTextureMap["glass"];
            }
            if (baseTexture) {
                lightingShader.getPtr()->setMat4("model", renderable->transform->modelMatrix);
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
    delete fboColorTexture;
    delete fboDepthTexture;
    delete baseTexture;
    delete directionLight;
    for (PointLight* ptr : pointLights) {
        delete ptr;
    }
    pointLights.clear();
}
NAMESPACE_END