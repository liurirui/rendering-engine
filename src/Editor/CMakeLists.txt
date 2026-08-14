set(EDITOR_UI_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/EditorUI.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/EditorUI.h"
    "${CMAKE_SOURCE_DIR}/src/3rdparty/imgui/backends/imgui_impl_glfw.cpp"
    "${CMAKE_SOURCE_DIR}/src/3rdparty/imgui/backends/imgui_impl_glfw.h"
    "${CMAKE_SOURCE_DIR}/src/3rdparty/imgui/backends/imgui_impl_opengl3.cpp"
    "${CMAKE_SOURCE_DIR}/src/3rdparty/imgui/backends/imgui_impl_opengl3.h"
)

# EditorUI 是可选的开发工具层；Engine 核心不依赖 ImGui。
add_library(editor_ui STATIC ${EDITOR_UI_SOURCES})
target_include_directories(editor_ui
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/.."
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/Engine"
        "${CMAKE_SOURCE_DIR}/src/Engine/RHI"
        "${CMAKE_SOURCE_DIR}/src/Engine/Renderer"
        "${CMAKE_SOURCE_DIR}/src/Engine/Base"
        "${CMAKE_SOURCE_DIR}/src/3rdparty/glm"
        "${CMAKE_SOURCE_DIR}/src/3rdparty/imgui"
        "${CMAKE_SOURCE_DIR}/src/3rdparty/imgui/backends"
        "${CMAKE_SOURCE_DIR}/src/3rdparty/ImGuiFileDialog"
        ${glfw_INCLUDE_DIRS}
        ${glad_INCLUDE_DIRS}
)

find_package(glfw REQUIRED)
find_package(glad REQUIRED)

target_include_directories(editor_ui
    PUBLIC
        ${glfw_INCLUDE_DIRS}
        ${glad_INCLUDE_DIRS}
)

# 让官方 OpenGL3 backend 使用工程统一的 GLAD，避免它再内置一份 GL loader。
target_compile_definitions(editor_ui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(editor_ui
    PUBLIC
        imgui
        ImGuiFileDialog
        ${glfw_LIBRARIES}
        glad
)

if(MSVC)
    target_compile_options(editor_ui PRIVATE /utf-8)
endif()
