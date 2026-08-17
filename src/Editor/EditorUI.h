#pragma once

#include <glm/vec4.hpp>

struct GLFWwindow;

namespace realtimerenderingengine {

class AssetManager;
class Renderer;
class Scene;

// 编辑器 UI 负责开发期控制和操作，但不拥有 Engine 资源或渲染对象的生命周期；
// 资源和渲染对象仍由 AssetManager、Scene 和 Renderer 管理。
class EditorUI {
public:
    bool initialize(GLFWwindow* window, const char* glslVersion);
    void beginFrame();
    void draw(Renderer& renderer, Scene& scene, AssetManager& assetManager,
        glm::vec4& clearColor, float& exposure, int& postProcessEffect);
    void render();
    void shutdown();

private:
    bool initialized_ = false;
    bool showDemoWindow_ = false;
    bool showAuxiliaryWindow_ = false;
    bool openModelDialog_ = false;
    bool openTextureDialog_ = false;
};

} // namespace realtimerenderingengine
