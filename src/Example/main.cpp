#include <glad.h>
#include <glfw3.h>

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
#include <Editor/EditorUI.h>

#include <iostream>
#include<algorithm>

#include<Engine/Base/Constants.h>

#include "Windows.h"
#include <windows.h>

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


// 应用输入层维护相机鼠标捕获状态；按住 Alt 时显示鼠标光标。
static bool isCursorVisible = false;

//int main(int argc, char* argv[])

bool debugGPU = true;

struct WindowRenderState {
    // GLFW callback 通过 user pointer 找到 Renderer，再统一转发窗口 resize。
    Renderer* renderer = nullptr;
};

float quadVertices[] = { // 全屏 NDC 四边形：位置和 UV 交错排列，用于最终屏幕合成。
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

    // 选择当前平台的 OpenGL Context 和 GLSL 版本。
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
    // 旧版 OpenGL 3.0 / GLSL 130 配置已停用，仅保留注释供后续跨平台扩展参考。
    //const char* glsl_version = "#version 130";
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
     // 当前 Windows 路径创建 OpenGL 4.2 Core Context，Shader 使用 GLSL 330。
      const char* glsl_version = "#version 330";
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "Render Engine", nullptr, nullptr);
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

    /* EditorUI owns Dear ImGui context and backend initialization. */
    EditorUI editorUI;
    if (!editorUI.initialize(window, glsl_version)) {
        Logger::Error("Failed to initialize EditorUI.");
        glfwDestroyWindow(window);
        Logger::Shutdown();
        glfwTerminate();
        return 1;
    }

    //Decide which effect to use
    int useEffect = 0;
    float exposure = 1.35f;
    
    // 清屏颜色属于窗口/应用状态，不属于 EditorUI。
    glm::vec4 clearColor(0.45f, 0.55f, 0.60f, 1.00f);


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
    assetManager->getShaderLibrary().registerPass(ShaderLibrary::ScreenShaderName(), ShaderPassType::Forward,
        { "resources/shaders/fullscreen.vert", "resources/shaders/screen_tonemap.frag", "" });
    std::shared_ptr<Shader> sharedScreenShader = assetManager->getShaderLibrary().getPass(
        ShaderHandle(ShaderLibrary::ScreenShaderName()), ShaderPassType::Forward);
    screenShader = sharedScreenShader.get();
    screenShader->use();
    screenShader->setInt("screenTexture", 0);
    screenShader->setFloat("exposure", exposure);
    GLenum screenShaderError = glGetError();
    if (screenShaderError != GL_NO_ERROR) {
        Logger::Warn("OpenGL error after screen shader setup. error=" + std::to_string(screenShaderError));
    }
    Renderer* renderer = new Renderer(*openGLRenderContext, *assetManager);
    WindowRenderState windowRenderState;
    windowRenderState.renderer = renderer;
    glfwSetWindowUserPointer(window, &windowRenderState);
    int initialFramebufferWidth = 0;
    int initialFramebufferHeight = 0;
    glfwGetFramebufferSize(window, &initialFramebufferWidth, &initialFramebufferHeight);
    renderer->resize(initialFramebufferWidth, initialFramebufferHeight);

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
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        
        /////
        // 轮询 GLFW 事件并处理窗口尺寸与相机输入。
        // EditorUI 在自己的模块中处理 ImGui Frame；应用层这里只轮询 GLFW 事件并处理相机输入。
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            glfwWaitEventsTimeout(0.01);
            continue;
        }

        // Rendering
        
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);



        // per-frame time logic
       // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        renderer->getPostProcessRenderer().time = lastFrame;
        // 场景最终显示的是 HDR Scene Target，而不是默认 framebuffer；因此清屏颜色必须同步到 Renderer。
        renderer->getMeshRenderer().setSceneClearColor(clearColor);
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
        editorUI.beginFrame();
        editorUI.draw(*renderer, *scene, *assetManager, clearColor, exposure, useEffect);
        editorUI.render();

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
    glfwSetWindowUserPointer(window, nullptr);
    windowRenderState.renderer = nullptr;
    delete scene;
    delete renderer;
    screenShader = nullptr;
    sharedScreenShader.reset();
    delete assetManager;
    glDeleteVertexArrays(1, &quadVAO);


    // Cleanup
    editorUI.shutdown();

    // Release all OpenGL-backed resources before destroying the GLFW context.
    delete openGLRenderContext;
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
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    // 应用输入层负责鼠标捕获；按住 Alt 时显示光标，松开后恢复相机控制。
    {
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
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    WindowRenderState* state = static_cast<WindowRenderState*>(glfwGetWindowUserPointer(window));
    if (state && state->renderer) {
        state->renderer->resize(width, height);
    }
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
