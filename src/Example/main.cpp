#include <glad.h>
#include <glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include<Engine/Base/Scene.h>
#include <Engine/Base/Camera.h>
#include<Engine/Base/Light.h>
#include <Engine/RHI/OpenGL/OpenGLRenderContext.h>
#include<Engine/Renderer/RenderGraph/RenderGraph.h>
#include<Engine/Renderer/MeshRenderer.h>
#include<Engine/Renderer/PostProcessRenderer.h>
#include<Engine/Renderer/Renderer.h>
#include<Engine/Base/AssetManager.h>
#include<Engine/Base/Logger.h>
#include <Engine/Base/Shader.h>

#include <iostream>
#include<array>
#include<algorithm>

#include<Engine/Base/Constants.h>

#include "Windows.h"
#include <windows.h>

#include <assimp/camera.h>
#include <assimp/mesh.h>
#include <ImGuiFileDialog.h>
#include <Engine/Base/ShaderCode.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

using namespace realtimerenderingengine;
using std::to_string;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SRC_WIDTH = 800;
const unsigned int SRC_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SRC_WIDTH / 2.0f;
float lastY = SRC_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);


//Controls whether the cursor is visible
static bool isCursorVisible = false;
static bool isWindowOpen = false;

//int main(int argc, char* argv[])

bool debugGPU = true;

float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates. NOTE that this plane is now much smaller and at the top of the screen
        // positions   // texCoords
        -1.0f,  -1.0f,  0.0f, 0.0f,
        1.0f,  -1.0f,  1.0f, 0.0f,
         -1.0f,  1.0f,  0.0f, 1.0f,

        1.0f,  -1.0f,  1.0f, 0.0f,
         -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f
};

Shader* screenShader = nullptr;

static bool directoryExists(const std::string& path)
{
    DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static std::string findProjectRoot()
{
    char currentDir[MAX_PATH] = {};
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    std::string path = currentDir;
    std::replace(path.begin(), path.end(), '\\', '/');

    for (int i = 0; i < 8; ++i) {
        if (directoryExists(path + "/resources")) {
            return path;
        }
        size_t slash = path.find_last_of('/');
        if (slash == std::string::npos) {
            break;
        }
        path = path.substr(0, slash);
    }

    return ".";
}

static void glfw_error_callback(int error, const char* description)
{
    Logger::Error(std::string("GLFW Error ") + std::to_string(error) + ": " + description);
}


static void APIENTRY opengl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    std::string level = severity == GL_DEBUG_SEVERITY_HIGH ? "HIGH" :
        severity == GL_DEBUG_SEVERITY_MEDIUM ? "MEDIUM" :
        severity == GL_DEBUG_SEVERITY_LOW ? "LOW" : "UNKNOWN";

    Logger::Warn("OpenGL debug message. severity=" + level +
        ", source=" + std::to_string(source) +
        ", type=" + std::to_string(type) +
        ", id=" + std::to_string(id) +
        ", message=" + std::string(message, length));
}

// Main code
//int main(int, char**)
int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    std::string rootPath = findProjectRoot();
    Logger::Initialize(rootPath + "/logs", true);
    Logger::Info("RenderEngine starting. Project root: " + rootPath);

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        Logger::Error("glfwInit failed.");
        Logger::Shutdown();
        return 1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    //const char* glsl_version = "#version 130";
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
     // GL 3.3 + GLSL 330
      const char* glsl_version = "#version 330";
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr) {
        Logger::Error("Failed to create GLFW window.");
        Logger::Shutdown();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1); // Enable vsync

        // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        Logger::Error("Failed to initialize GLAD");
        Logger::Shutdown();
        return -1;
    }

    if (glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(opengl_debug_message_callback, nullptr);
        Logger::Info("OpenGL debug callback enabled.");
    }
    else {
        Logger::Warn("OpenGL debug callback is not available on this driver/context.");
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    //Decide which effect to use
    int useEffect = 0;
    float exposure = 1.35f;
    
    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    screenShader = new Shader(Vert_quad, Frag_quad, nullptr, "engine/screen-quad");
    screenShader->use();
    screenShader->setInt("screenTexture", 0);
    screenShader->setFloat("exposure", exposure);
    GLenum screenShaderError = glGetError();
    if (screenShaderError != GL_NO_ERROR) {
        Logger::Warn("OpenGL error after screen shader setup. error=" + std::to_string(screenShaderError));
    }
    // screen quad VAO
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    OpenGLRenderContext* openGLRenderContext = new OpenGLRenderContext();
    openGLRenderContext->windowsWidth = SRC_WIDTH;
    openGLRenderContext->windowsHeight = SRC_HEIGHT;
    AssetManager* assetManager = new AssetManager(*openGLRenderContext, rootPath);
    Renderer* renderer = new Renderer(*openGLRenderContext, *assetManager);

    std::string texturepath = rootPath + "/resources/textures/background.jpg";
    
    Scene* scene = new Scene("scene");
    scene->setAssetManager(assetManager);
    scene->Start();
    scene->AddLight(LightType::Direction, glm::vec3(-0.5f, -0.8f, -0.5f), glm::vec3(1.0f), 2.0f);
    auto addPointLight = [scene](const glm::vec3& position, const glm::vec3& color, float intensity, float range) {
        PointLight* light = static_cast<PointLight*>(scene->AddLight(LightType::Point, position, color, intensity));
        light->setRange(range);
    };
    // Low lighting intensity with bounded influence; visible light cubes stay HDR-emissive for bloom.
    addPointLight(glm::vec3(0.0f, 6.0f, 5.0f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 7.0f);
    addPointLight(glm::vec3(-2.0f, 1.0f, -3.0f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 4.0f);
    addPointLight(glm::vec3(3.0f, 8.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 1.2f, 7.0f);
    addPointLight(glm::vec3(-8.0f, 3.0f, -1.0f), glm::vec3(1.0f), 0.9f, 5.0f);

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        
        /////
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            ImGui::SetNextWindowSize(ImVec2(400, 300));
            // Flag for file dialog
            static bool openModelDialog = false, openTextureDialog = false;
            // store selected file path
            static std::string selectedFilePath = "";

            // Create a window called "Hello, world!" and append into it.
            isWindowOpen = ImGui::Begin("Hello, world! Hold down the 'Alt' key");
            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::ColorEdit3("Background Color", (float*)&(renderer->getMeshRenderer().getTargetFrameBuffer()->colorAttachments[0].clearColor)); // Edit 3 floats representing a color
            ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f, "%.2f");

            ImVec4 buttonColor = ImVec4(1.0f, 0.4f, 0.f, 1.0f);      // Color of button
            ImVec4 hoveredColor = ImVec4(0.4f, 0.15f, 0.15f, 1.0f);  // Color on hover
            ImVec4 activeColor = ImVec4(0.8f, 0.15f, 0.0f, 1.0f);    // Color when pressed

           // Reusable function to apply button colors
            auto setButtonStyle = [&]() {
                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
            };

            setButtonStyle();
            if (ImGui::Button("Load Model", ImVec2(160, 30)))  openModelDialog = true;
            ImGui::PopStyleColor(3);

            if (openModelDialog) {
                IGFD::FileDialogConfig config;
                config.path = assetManager->resolvePath("resources/objects");                // Default folder path for model selection
                config.countSelectionMax = 1;                       // Allow only one file to be selected
                config.flags = ImGuiFileDialogFlags_None;           // No special flags

                // Open model file dialog
                ImGuiFileDialog::Instance()->OpenDialog("ChooseModelDlgKey", "Choose Model", ".obj,.fbx,.glb,.gltf,.dae", config);
                openModelDialog = false;
            }

            // Process selected model file
            if (ImGuiFileDialog::Instance()->Display("ChooseModelDlgKey")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    scene->createModel(selectedFilePath);   // Add model to the scene
                }
                ImGuiFileDialog::Instance()->Close();
            }
            
            setButtonStyle();
            if (ImGui::Button("Change Floor Texture", ImVec2(160, 30))) openTextureDialog = true;
            ImGui::PopStyleColor(3);

            if (openTextureDialog) {
                IGFD::FileDialogConfig config;
                config.path = assetManager->resolvePath("resources/textures");               // Default folder path for texture selection
                config.countSelectionMax = 1;                       // Allow only one file to be selected
                config.flags = ImGuiFileDialogFlags_None;           // No special flags

                // Open texture file dialog
                ImGuiFileDialog::Instance()->OpenDialog("ChooseTextureDlgKey", "Choose Texture", ".png,.jpg,.jpeg,.bmp", config);
                openTextureDialog = false;
            }

            // Process selected texture file
            if (ImGuiFileDialog::Instance()->Display("ChooseTextureDlgKey")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    renderer->getMeshRenderer().setFloorTexture(assetManager->loadTexture2D(selectedFilePath));
                }
                ImGuiFileDialog::Instance()->Close();
            }

            if (!scene->root->child.empty()) {
                if (ImGui::CollapsingHeader("Model Control")) {
                    ImGui::Indent(ImGui::GetFontSize() * 2);
                    for (int i = 0; i < scene->root->child.size(); i++) {
                        if (ImGui::CollapsingHeader(("Model " + to_string(i + 1) + " Control").c_str())) {
                            ImGui::Indent(ImGui::GetFontSize() * 2);
                            if (ImGui::CollapsingHeader(("Model " + to_string(i + 1) + " Position").c_str())) {
                                if (ImGui::SliderFloat(("Position X##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localPosition).x), -10.0f, 10.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                                if (ImGui::SliderFloat(("Position Y##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localPosition).y), 0.0f, 10.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                                if (ImGui::SliderFloat(("Position Z##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localPosition).z), -10.0f, 10.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                            }
                            if (ImGui::CollapsingHeader(("Model " + to_string(i + 1) + " Rotation").c_str())) {
                                if (ImGui::SliderFloat(("Rotation X##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localRotation).x), -90.0f, 90.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                                if (ImGui::SliderFloat(("Rotation Y##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localRotation).y), -90.0f, 90.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                                if (ImGui::SliderFloat(("Rotation Z##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localRotation).z), -90.0f, 90.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                            }
                            if (ImGui::CollapsingHeader(("Model " + to_string(i + 1) + " Scale").c_str())) {
                                if (ImGui::SliderFloat(("Scale X##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localScale).x), 0.0f, 5.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                                if (ImGui::SliderFloat(("Scale Y##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localScale).y), 0.0f, 5.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                                if (ImGui::SliderFloat(("Scale Z##" + to_string(i)).c_str(), &((scene->root->child[i]->GetTransform()->localScale).z), 0.0f, 5.0f))  scene->root->child[i]->GetTransform()->SetDirty();
                            }
                            ImGui::Unindent();
                        }
                    }
                    ImGui::Unindent();
                }
            }

            //Provides the option to select post-processing
            ImGui::Text("Select Post-processing Effect");
            ImGui::RadioButton("Origin", &useEffect, 0);
            ImGui::SameLine(0, 10);
            ImGui::RadioButton("Bloom", &useEffect, 1);
            ImGui::SameLine(0, 10);
            ImGui::RadioButton("Radial", &useEffect, 2);
            ImGui::SameLine(0, 10);
            ImGui::RadioButton("Motion", &useEffect, 3);
            ImGui::SameLine(0, 10);
            ImGui::RadioButton("Cartoon", &useEffect, 4);

            ImGui::RadioButton("Ripple", &useEffect, 5);
            
            /*if (ImGui::CollapsingHeader("Lights Control")) {
                //directionLight 
                ImGui::Text("Directional Light:");
                ImGui::Indent(ImGui::GetFontSize() * 2);
                if (ImGui::Checkbox("Directional Light", &(scene->meshRenderer->directionLight->Switch))) {
                    if (scene->meshRenderer->directionLight->Switch) scene->meshRenderer->directionLight->turnOn();
                    else   scene->meshRenderer->directionLight->turnOff();
                }
                ImGui::Unindent();

                //pointLight
                ImGui::Text("Point Lights:");
                ImGui::Indent(ImGui::GetFontSize() * 2);
                // 4 pointLights switch
                static const std::array<std::string, 4> nameLight = { "red", "green", "blue", "white" };
                string name = "light ";
                for (int i = 0; i < 4; i++) {
                    if (ImGui::CollapsingHeader((name + nameLight[i]).c_str())) {
                        //Control the light source on and off
                        if (ImGui::Checkbox(("On / Off##" + nameLight[i]).c_str(), &(scene->meshRenderer->pointLights[i]->Switch))) {
                            if (scene->meshRenderer->pointLights[i]->Switch)    scene->meshRenderer->pointLights[i]->turnOn();
                            else  scene->meshRenderer->pointLights[i]->turnOff();
                        }
                        //Control the position of light sources
                        ImGui::SliderFloat((name + nameLight[i] + "X").c_str(), &((scene->meshRenderer->pointLights[i]->getPosition()).x), -10.0f, 10.0f);
                        ImGui::SliderFloat((name + nameLight[i] + "Y").c_str(), &((scene->meshRenderer->pointLights[i]->getPosition()).y), 0.0f, 10.0f);
                        ImGui::SliderFloat((name + nameLight[i] + "Z").c_str(), &((scene->meshRenderer->pointLights[i]->getPosition()).z), -10.0f, 10.0f);
                    }
                }
                ImGui::Unindent();
            }*/
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
        

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);



        // per-frame time logic
       // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        renderer->getPostProcessRenderer().time = lastFrame;
        // input
        // -----
        processInput(window);
        renderer->render(*scene, camera, useEffect);

        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        screenShader->use();
        screenShader->setFloat("exposure", exposure);
        glBindVertexArray(quadVAO);

        glActiveTexture(GL_TEXTURE0);
        if(useEffect==0)
        glBindTexture(GL_TEXTURE_2D, renderer->getMeshRenderer().getTargetColorTextureID(0));
        else 
        glBindTexture(GL_TEXTURE_2D, renderer->getPostProcessRenderer().getTargetColorTextureID(0,useEffect));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
    delete scene;
    delete renderer;
    delete assetManager;
    delete openGLRenderContext;

    glDeleteVertexArrays(1, &quadVAO);


    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    Logger::Info("RenderEngine shutdown.");
    Logger::Shutdown();
    glfwTerminate();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    //Control whether the cursor should be hidden
    if (!isWindowOpen) {             //When the window is not expanded
        if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            if (!isCursorVisible)    //Press "Alt" when the cursor is not visible
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show Cursor
                isCursorVisible = true;
            }
        }
        else if (isCursorVisible)   //Not holding "Alt"
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);         // Hide  Cursor
            isCursorVisible = false;
        }
    }
    else if (!isCursorVisible) {    //When the window is expanded
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);              //Show Currsor
        isCursorVisible = true;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!isCursorVisible) {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
