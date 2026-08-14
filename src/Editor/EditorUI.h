#pragma once

#include <glm/vec4.hpp>

struct GLFWwindow;

namespace realtimerenderingengine {

class AssetManager;
class Renderer;
class Scene;

// 编辑器 UI 只负责采集和展示开发期控制项，不参与 Engine 的资源或渲染生命周期。
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
