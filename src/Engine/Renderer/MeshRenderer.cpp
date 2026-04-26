#include "MeshRenderer.h"
#include "Base/ShaderCode.h"
#include "Base/Shader.h"
#include "Base/Camera.h"
#include "Base/Light.h"
#include "RenderGraph/RenderGraph.h"
#include "Base/Material.h"
#include "Base/Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

NAMESPACE_START
MeshRenderer::MeshRenderer() {}
MeshRenderer::MeshRenderer(const std::vector<Renderable*>& translucentMeshes, const std::vector<Renderable*>& opaqueMeshes)
    : translucentMeshes(translucentMeshes), opaqueMeshes(opaqueMeshes){}

void MeshRenderer::render(Camera* camera, RenderGraph& rg) {

    rg.addPass("shadowPass", &camera, [this, camera](RenderContext* renderContext) {
        depthStencilState.depthTest = true;
        depthStencilState.depthWrite = true;
        renderContext->beginRendering(scene->GetMainDirectionalLight()->getShadow()->DepthMapFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline_DepthMap);
        depthMapShader.getPtr()->setMat4("lightSpaceMatrix", scene->GetMainDirectionalLight()->LightSpaceMatrix);
        //plane(shadow)
        depthMapShader.getPtr()->setMat4("model", glm::mat4(1.0f));
        renderContext->bindVertexArray(planeVAO);
        renderContext->drawArrays(0, 6);
        //gameObject(shadow)
        scene->RenderObject();
        renderContext->endRendering();
    });

    rg.addPass("scenePass", &camera, [this, camera](RenderContext* renderContext) {
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
        Light::updateUBO(scene->lights);
        Light::bindUBO();

        renderContext->beginRendering(OriginFramebuffer);
        renderContext->setDepthStencilState(depthStencilState);
        renderContext->bindPipeline(graphicsPipeline);
        glm::mat4 light_model = glm::mat4(1.0f);
        lightingShader_cube.getPtr()->use();
        lightingShader_cube.getPtr()->setMat4("projection", projection);
        lightingShader_cube.getPtr()->setMat4("view", camera->GetViewMatrix());
        //Render a large point light source to act as a unidirectional light source
        static glm::vec3 directionPos = scene->GetMainDirectionalLight()->getDirection() * (-80.0f);
        light_model = glm::translate(light_model, directionPos);
        light_model = glm::scale(light_model, glm::vec3(3.0f, 3.0f, 3.0f));
        lightingShader_cube.getPtr()->setMat4("model", light_model);
        lightingShader_cube.getPtr()->setVec3("lightColor", scene->GetMainDirectionalLight()->getColor());
        renderContext->bindVertexArray(cubeVAO);
        renderContext->drawArrays(0, 36);

        //render light cube
        std::vector<Light*> all_lights = scene->GetAllLights();
        for (int i = 0; i < all_lights.size(); i++) {
            auto light = all_lights[i];
            if (light->getType() == LightType::Point) {
                auto pointLight = dynamic_cast<PointLight*>(light);
                light_model = glm::mat4(1.0f); 
                light_model = glm::translate(light_model, glm::vec3(pointLight->getPosition()));
                light_model = glm::scale(light_model, glm::vec3(0.1f, 0.1f, 0.1f));
                lightingShader_cube.getPtr()->setMat4("model", light_model); 
                lightingShader_cube.getPtr()->setVec3("lightColor", pointLight->getColor());
                renderContext->bindVertexArray(cubeVAO);
                renderContext->drawArrays(0, 36);
            }
        }

        lightingShader.getPtr()->use();
        lightingShader.getPtr()->setMat4("projection", projection);
        lightingShader.getPtr()->setMat4("view", camera->GetViewMatrix());
        // light properties
        lightingShader.getPtr()->setVec3("viewPos", camera->Position);
        lightingShader.getPtr()->setVec3("lightPos", scene->GetMainDirectionalLight()->getDirection()*(-10.0f));
        lightingShader.getPtr()->setMat4("lightSpaceMatrix", scene->GetMainDirectionalLight()->LightSpaceMatrix);
        lightingShader.getPtr()->setVec3("light.direction", scene->GetMainDirectionalLight()->getDirection());
        lightingShader.getPtr()->setVec3("light.color", scene->GetMainDirectionalLight()->getColor());
        lightingShader.getPtr()->setFloat("light.intensity", scene->GetMainDirectionalLight()->getIntensity());
        lightingShader.getPtr()->setVec3("ambient", 0.3f, 0.3f, 0.3f);
        lightingShader.getPtr()->setVec3("diffuse", 0.6f, 0.6f, 0.6f);
        lightingShader.getPtr()->setVec3("specular", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        lightingShader.getPtr()->setFloat("shininess", 32.0f);
        lightingShader.getPtr()->setBool("isMirror", false);
        lightingShader.getPtr()->setBool("isGlass", false);
        lightingShader.getPtr()->setInt("baseTexture", 0);
        lightingShader.getPtr()->setInt("shadowMap", 1);
        renderContext->bindTexture(scene->GetMainDirectionalLight()->getShadow()->depthMap->id, 1);
        for (int i = 1; i<scene->lights.size(); i++) {
            PointLight* light = dynamic_cast<PointLight*>(scene->lights[i]);
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i-1) + "].position").c_str(), light->getPosition());
            lightingShader.getPtr()->setVec3(("point[" + std::to_string(i - 1) + "].color").c_str(), light->getColor());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i - 1) + "].intensity").c_str(), light->getIntensity());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i - 1) + "].constant").c_str(), light->getConstantAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i - 1) + "].linear").c_str(), light->getLinearAttenuation());
            lightingShader.getPtr()->setFloat(("point[" + std::to_string(i - 1) + "].quadratic").c_str(), light->getQuadraticAttenuation());
        }
        
        //render plane
        lightingShader.getPtr()->setMat4("model", glm::mat4(1.0));
        renderContext->bindVertexArray(planeVAO);
        renderContext->bindTexture(floor->id, 0);
        renderContext->drawArrays(0, 6);

        scene->RenderObject();

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