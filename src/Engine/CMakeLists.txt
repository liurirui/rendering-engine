get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


include_directories("${CURRENT_DIR}/.." "${CURRENT_DIR}/RHI/.." "${CURRENT_DIR}/RHI/OpenGL/.." "${CURRENT_DIR}/Renderer/.." "${CURRENT_DIR}/Renderer/RenderGraph/.." "${CURRENT_DIR}/Base/..")


file(GLOB Base_SOURCES
        "${CURRENT_DIR}/Base/*.cpp"
        "${CURRENT_DIR}/Base/*.h"
        )
FilterFolder("${Base_SOURCES}")

file(GLOB Renderer_SOURCES
        "${CURRENT_DIR}/Renderer/*.cpp"
        "${CURRENT_DIR}/Renderer/*.h"
        "${CURRENT_DIR}/Renderer/RenderGraph/*.cpp"
        "${CURRENT_DIR}/Renderer/RenderGraph/*.h"
        "${CURRENT_DIR}/Renderer/RenderGraph/*.inl"
        )

FilterFolder("${Renderer_SOURCES}")

file(GLOB RHI_SOURCES
        "${CURRENT_DIR}/RHI/*.cpp"
        "${CURRENT_DIR}/RHI/*.h"
        "${CURRENT_DIR}/RHI/OpenGL/*.cpp"
        "${CURRENT_DIR}/RHI/OpenGL/*.h"
)
FilterFolder("${RHI_SOURCES}")

list(APPEND SOURCES ${Renderer_SOURCES} ${Base_SOURCES} ${RHI_SOURCES})

# 生成链接库
add_library(engine ${SOURCES})

find_package(glad)
include_directories("${glad_INCLUDE_DIRS}")

find_package(stb_image)
include_directories("${stb_image_INCLUDE_DIRS}")

find_package(glm)
include_directories("${glm_INCLUDE_DIRS}")

find_package(assimp)
include_directories("${assimp_INCLUDE_DIRS}")

target_link_libraries(engine ${glad_LIBRARIES})
