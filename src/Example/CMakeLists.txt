get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


include_directories("${CURRENT_DIR}/..")


file(GLOB Main_SOURCES
        "${CURRENT_DIR}/*.cpp"
        "${CURRENT_DIR}/*.h"
        )

FilterFolder("${Main_SOURCES}")
list(APPEND SOURCES ${Main_SOURCES})

# 生成链接库
#add_library(engine ${SOURCES})
#add_executable(example ${SOURCES})
add_executable(example WIN32 ${SOURCES})

find_package(engine)
include_directories("${engine_INCLUDE_DIRS}")

find_package(glad)
include_directories("${glad_INCLUDE_DIRS}")

find_package(glfw)
include_directories("${glfw_INCLUDE_DIRS}")

find_package(stb_image)
include_directories("${stb_image_INCLUDE_DIRS}")

find_package(glm)
include_directories("${glm_INCLUDE_DIRS}")

find_package(assimp)
include_directories(${assimp_INCLUDE_DIRS})


target_link_libraries(example ${engine_LIBRARIES} ${glad_LIBRARIES} ${glfw_LIBRARIES} ${stb_image_LIBRARIES} ${assimp_LIBRARIES})
