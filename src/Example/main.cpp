#include <glad.h>
#include <glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Engine/Base/Camera.h>
#include<Engine/Base/Light.h>
#include <Engine/RHI/OpenGL/OpenGLRenderContext.h>
#include<Engine/Renderer/RenderGraph/RenderGraph.h>
#include<Engine/Renderer/BasePassRenderer.h>
#include<Engine/Renderer/MeshRenderer.h>
#include<Engine/Renderer/PostProcessRenderer.h>

#include <iostream>

#include<Engine/Base/Constants.h>

#include "Windows.h"
#include <windows.h>

#include <assimp/camera.h>
#include <assimp/mesh.h>
#include <Engine/Base/ShaderCode.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

using namespace realtimerenderingengine;

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

//#include <direct.h>
//int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
int asdasdasdsa(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{

    //char* path = NULL;
    //path = _getcwd(NULL, 1);
    //puts(path);
    //delete path;

    Scene::rootPath = "F:/liurirui";
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "RenderEngine", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    screenShader = new Shader(Vert_quad, Frag_quad);
    screenShader->use();
    screenShader->setInt("screenTexture", 0);
    int errorCode = glGetError();
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

    BasePassRenderer* basePassRenderer = new BasePassRenderer;
    std::string texturepath = Scene::rootPath + "/resources/textures/background.jpg";
    Texture2D* texture = new Texture2D(texturepath.c_str());

    std::string objpath = Scene::rootPath + "/resources/objects/nanosuit/nanosuit.obj";
    MeshRenderer* meshRenderer = new MeshRenderer(objpath);
    PostProcessRenderer* postprocessRenderer = new PostProcessRenderer;
    
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
 
        // input
        // -----
        processInput(window);

        RenderGraph renderGraph;
        
        //basePassRenderer->render(&camera, renderGraph);
        meshRenderer->render(&camera, renderGraph);
        postprocessRenderer->render(renderGraph, meshRenderer->getTargetFrameBuffer());
        renderGraph.execute(openGLRenderContext);

        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        screenShader->use();
        glBindVertexArray(quadVAO);

        glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, basePassRenderer->getTargetColorTexture(0));	// use the color attachment texture as the texture of the quad plane
        glBindTexture(GL_TEXTURE_2D, meshRenderer->getTargetColorTextureID(0));
        //glBindTexture(GL_TEXTURE_2D, postprocessRenderer->getTargetColorTextureID(0));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete basePassRenderer;
    delete texture;
    delete meshRenderer;
    delete postprocessRenderer;
    delete openGLRenderContext;

    glDeleteVertexArrays(1, &quadVAO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
//int main(int, char**)
int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    Scene::rootPath = "E:/";

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
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
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
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
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
    
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    screenShader = new Shader(Vert_quad, Frag_quad);
    screenShader->use();
    screenShader->setInt("screenTexture", 0);
    int errorCode = glGetError();
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

    BasePassRenderer* basePassRenderer = new BasePassRenderer;
    std::string texturepath = Scene::rootPath + "/resources/textures/background.jpg";
    Texture2D* texture = new Texture2D(texturepath.c_str());

    std::string objpath = Scene::rootPath + "/resources/objects/nanosuit/nanosuit.obj";
    MeshRenderer* meshRenderer = new MeshRenderer(objpath);
    PostProcessRenderer* postprocessRenderer = new PostProcessRenderer;

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
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world! Hold down the 'Alt' key");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

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
            
            //Provides option to turn directional lights on and off
            ImGui::Text("Switch of Direction Light");
            if (ImGui::RadioButton("ON", meshRenderer->directionLight->Switch)) {
                if (!meshRenderer->directionLight->Switch) {
                    meshRenderer->directionLight->turnOn();
                }
            }

            ImGui::SameLine(0, 10);

            if (ImGui::RadioButton("OFF", !meshRenderer->directionLight->Switch)) {
                if (meshRenderer->directionLight->Switch) {
                    meshRenderer->directionLight->turnOff();
                }
            }
            
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
        postprocessRenderer->time = lastFrame;
        // input
        // -----
        processInput(window);
        RenderGraph renderGraph;

        //basePassRenderer->render(&camera, renderGraph);
        meshRenderer->render(&camera, renderGraph);
        if(useEffect!=0)
        postprocessRenderer->render(renderGraph, meshRenderer->getTargetFrameBuffer());
        renderGraph.execute(openGLRenderContext);

        glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        screenShader->use();
        glBindVertexArray(quadVAO);

        glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, basePassRenderer->getTargetColorTexture(0));	// use the color attachment texture as the texture of the quad plane
        if(useEffect==0)
        glBindTexture(GL_TEXTURE_2D, meshRenderer->getTargetColorTextureID(0));
        else 
        glBindTexture(GL_TEXTURE_2D, postprocessRenderer->getTargetColorTextureID(0,useEffect));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    delete basePassRenderer;
    delete texture;
    delete meshRenderer;
    delete postprocessRenderer;
    delete openGLRenderContext;

    glDeleteVertexArrays(1, &quadVAO);


    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
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

    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        if (!isCursorVisible)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show Cursor
            isCursorVisible = true;
        }
    }
    else
    {
        if (isCursorVisible)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide  Cursor
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
