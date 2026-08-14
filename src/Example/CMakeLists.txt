get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


include_directories("${CURRENT_DIR}/..")


set(Main_SOURCES
        "${CURRENT_DIR}/main.cpp"
        )

FilterFolder("${Main_SOURCES}")
list(APPEND SOURCES ${Main_SOURCES})

# 生成链接库
#add_library(engine ${SOURCES})
#add_executable(example ${SOURCES})
add_executable(example WIN32 ${SOURCES})

# Example 同样包含中文注释和字符串，统一要求 MSVC 使用 UTF-8 解析源码。
if(MSVC)
    target_compile_options(example PRIVATE /utf-8)
endif()

target_include_directories(example PRIVATE "${CURRENT_DIR}/..")
target_link_libraries(example PRIVATE engine editor_ui)

set(ASSIMP_RUNTIME_DLL "${CMAKE_SOURCE_DIR}/src/3rdparty/assimp/lib/dll/assimp-vc142-mt.dll")

add_custom_target(copy_example_runtime_dlls ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:example>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ASSIMP_RUNTIME_DLL}"
        $<TARGET_FILE_DIR:example>
    COMMENT "Copying runtime DLLs for example"
    VERBATIM)

add_custom_command(TARGET example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ASSIMP_RUNTIME_DLL}"
        $<TARGET_FILE_DIR:example>
    VERBATIM)
