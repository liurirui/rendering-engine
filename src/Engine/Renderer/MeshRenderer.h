#pragma once
#include <Base/Constants.h>
#include <RHI/RenderContext.h>
#include <unordered_map>
#include <memory>
#include"Base/Renderable.h"
class RenderGraph;

NAMESPACE_START

class AssetManager;
class Camera;
class Shader;
class DirectionLight;
class PointLight;
class Scene;
class MeshRenderer {
public:
    friend class Scene;
    explicit MeshRenderer(AssetManager& assetManager);
    ~MeshRenderer();
    virtual void render(Scene& scene, Camera* camera, RenderGraph& rg);
    void addShadowPass(Scene& scene, Camera* camera, RenderGraph& rg);
    void setFloorTexture(Texture2D* texture);
    unsigned int getTargetColorTextureID(int  attachment);
    FrameBufferInfo* getTargetFrameBuffer();

    //Model mesh textures that need to be stored in advance
    std::unordered_map<std::string, Texture2D*> ColorTextureMap;
    Texture2D* floor = nullptr;

private:
    //shader
    AssetManager& assetManager_;
    std::shared_ptr<Shader> defaultLitShader;
    std::shared_ptr<Shader> lightDebugShader;
    std::shared_ptr<Shader> depthMapShader;

    //FBO
    FrameBufferInfo OriginFramebuffer;
    FrameBufferInfo DepthMapFramebuffer;

    //Texture
    Texture2D* fboColorTexture = nullptr;
    Texture2D* fboDepthTexture = nullptr;
    Texture2D* baseTexture = nullptr;

  

    //Render pipeline status
    GraphicsPipeline graphicsPipeline, graphicsPipeline_DepthMap;
    DepthStencilState depthStencilState;

    //Accept  Scene's RenderableContainer reference
    const std::vector<Renderable*> translucentMeshes;  
    const std::vector<Renderable*> opaqueMeshes;       
    
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    //cube
    unsigned int cubeVAO=0,cubeVBO=0;
    float cubeVertices[288] = {
        // back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
        // bottom face
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
         1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
    };
    unsigned int planeVAO = 0, planeVBO = 0;
    float planeVertices[48] = {
        // positions            // normals         // texcoords
        -25.0f, -0.01f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
        -25.0f, -0.01f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
         25.0f, -0.01f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,

         -25.0f, -0.01f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
         25.0f, -0.01f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
         25.0f, -0.01f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
    };
};

NAMESPACE_END
