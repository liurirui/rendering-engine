#include "EditorUI.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuiFileDialog.h>

#include <Engine/Base/AssetManager.h>
#include <Engine/Base/Scene.h>
#include <Engine/Renderer/MeshRenderer.h>
#include <Engine/Renderer/Renderer.h>

#include <glm/vec4.hpp>

#include <string>

namespace realtimerenderingengine {

bool EditorUI::initialize(GLFWwindow* window, const char* glslVersion) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true) || !ImGui_ImplOpenGL3_Init(glslVersion)) {
        ImGui::DestroyContext();
        return false;
    }
    initialized_ = true;
    return true;
}

void EditorUI::beginFrame() {
    if (!initialized_) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorUI::draw(Renderer& renderer, Scene& scene, AssetManager& assetManager,
    glm::vec4& clearColor, float& exposure, int& postProcessEffect) {
    if (!initialized_) return;

    if (showDemoWindow_) ImGui::ShowDemoWindow(&showDemoWindow_);

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Render Engine Editor")) {
        ImGui::Checkbox("ImGui Demo", &showDemoWindow_);
        ImGui::Checkbox("Auxiliary Window", &showAuxiliaryWindow_);

        ImGui::ColorEdit3("Window Clear Color", &clearColor.x);
        ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f, "%.2f");

        if (ImGui::CollapsingHeader("Render Statistics")) {
            const RenderStats& stats = renderer.getMeshRenderer().getRenderStats();
            ImGui::Text("Submitted: %u", stats.submittedItems);
            ImGui::Text("Visible: %u", stats.visibleItems);
            ImGui::Text("Culled: %u", stats.culledItems);
            ImGui::Text("Invalid Bounds: %u", stats.invalidBoundsItems);
            ImGui::Text("Shadow / Opaque / Transparent: %u / %u / %u",
                stats.shadowItems, stats.opaqueItems, stats.transparentItems);
            ImGui::Text("Visible Triangles: %llu", static_cast<unsigned long long>(stats.visibleTriangles));
        }

        if (ImGui::CollapsingHeader("Shader System")) {
            const ShaderLibraryStats stats = assetManager.getShaderLibrary().getStats();
            ImGui::Text("Techniques: %llu", static_cast<unsigned long long>(stats.techniqueCount));
            ImGui::Text("Programs / Variants: %llu / %llu",
                static_cast<unsigned long long>(stats.programCount),
                static_cast<unsigned long long>(stats.variantCount));
            ImGui::Text("Reload Success / Failed: %llu / %llu",
                static_cast<unsigned long long>(stats.successfulReloads),
                static_cast<unsigned long long>(stats.failedReloads));
            if (ImGui::Button("Reload Shaders", ImVec2(160, 26))) assetManager.getShaderLibrary().requestReload();
        }

        if (ImGui::CollapsingHeader("RenderGraph")) {
            const RenderGraph::Stats& stats = renderer.getLastRenderGraphStats();
            ImGui::Text("Passes: %llu", static_cast<unsigned long long>(stats.passCount));
            ImGui::Text("Dependency Edges: %llu", static_cast<unsigned long long>(stats.dependencyEdgeCount));
            ImGui::Text("Reordered / Cycles: %llu / %llu",
                static_cast<unsigned long long>(stats.reorderedPassCount),
                static_cast<unsigned long long>(stats.cycleCount));
        }

        const auto pushActionButtonStyle = []() {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.15f, 0.0f, 1.0f));
        };

        pushActionButtonStyle();
        if (ImGui::Button("Load Model", ImVec2(160, 30))) openModelDialog_ = true;
        ImGui::PopStyleColor(3);
        if (openModelDialog_) {
            IGFD::FileDialogConfig config;
            config.path = assetManager.resolvePath("resources/objects");
            config.countSelectionMax = 1;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseModelDlgKey", "Choose Model", ".obj,.fbx,.glb,.gltf,.dae", config);
            openModelDialog_ = false;
        }
        if (ImGuiFileDialog::Instance()->Display("ChooseModelDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) scene.createModel(ImGuiFileDialog::Instance()->GetFilePathName());
            ImGuiFileDialog::Instance()->Close();
        }

        pushActionButtonStyle();
        if (ImGui::Button("Change Floor Texture", ImVec2(160, 30))) openTextureDialog_ = true;
        ImGui::PopStyleColor(3);
        if (openTextureDialog_) {
            IGFD::FileDialogConfig config;
            config.path = assetManager.resolvePath("resources/textures");
            config.countSelectionMax = 1;
            ImGuiFileDialog::Instance()->OpenDialog("ChooseTextureDlgKey", "Choose Texture", ".png,.jpg,.jpeg,.bmp", config);
            openTextureDialog_ = false;
        }
        if (ImGuiFileDialog::Instance()->Display("ChooseTextureDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                renderer.getMeshRenderer().setFloorTexture(assetManager.loadTexture2D(ImGuiFileDialog::Instance()->GetFilePathName()));
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (!scene.root->child.empty() && ImGui::CollapsingHeader("Model Control")) {
            for (size_t index = 0; index < scene.root->child.size(); ++index) {
                GameObject* object = scene.root->child[index];
                if (!object) continue;
                const std::string label = "Model " + std::to_string(index + 1);
                if (ImGui::TreeNode(label.c_str())) {
                    Transform* transform = object->GetTransform();
                    bool changed = false;
                    changed |= ImGui::SliderFloat(("Position X##" + std::to_string(index)).c_str(), &transform->localPosition.x, -10.0f, 10.0f);
                    changed |= ImGui::SliderFloat(("Position Y##" + std::to_string(index)).c_str(), &transform->localPosition.y, 0.0f, 10.0f);
                    changed |= ImGui::SliderFloat(("Position Z##" + std::to_string(index)).c_str(), &transform->localPosition.z, -10.0f, 10.0f);
                    changed |= ImGui::SliderFloat(("Rotation X##" + std::to_string(index)).c_str(), &transform->localRotation.x, -90.0f, 90.0f);
                    changed |= ImGui::SliderFloat(("Rotation Y##" + std::to_string(index)).c_str(), &transform->localRotation.y, -90.0f, 90.0f);
                    changed |= ImGui::SliderFloat(("Rotation Z##" + std::to_string(index)).c_str(), &transform->localRotation.z, -90.0f, 90.0f);
                    changed |= ImGui::SliderFloat(("Scale X##" + std::to_string(index)).c_str(), &transform->localScale.x, 0.0f, 5.0f);
                    changed |= ImGui::SliderFloat(("Scale Y##" + std::to_string(index)).c_str(), &transform->localScale.y, 0.0f, 5.0f);
                    changed |= ImGui::SliderFloat(("Scale Z##" + std::to_string(index)).c_str(), &transform->localScale.z, 0.0f, 5.0f);
                    if (changed) transform->SetDirty();
                    ImGui::TreePop();
                }
            }
        }

        ImGui::Text("Post-processing");
        ImGui::RadioButton("Origin", &postProcessEffect, 0); ImGui::SameLine();
        ImGui::RadioButton("Bloom", &postProcessEffect, 1); ImGui::SameLine();
        ImGui::RadioButton("Radial", &postProcessEffect, 2); ImGui::SameLine();
        ImGui::RadioButton("Motion", &postProcessEffect, 3); ImGui::SameLine();
        ImGui::RadioButton("Cartoon", &postProcessEffect, 4); ImGui::SameLine();
        ImGui::RadioButton("Ripple", &postProcessEffect, 5);
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
    ImGui::End();

    if (showAuxiliaryWindow_) {
        ImGui::Begin("Auxiliary Window", &showAuxiliaryWindow_);
        ImGui::Text("Editor tool window.");
        ImGui::End();
    }
}

void EditorUI::render() {
    if (!initialized_) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorUI::shutdown() {
    if (!initialized_) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

} // namespace realtimerenderingengine
