get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


include_directories("${CURRENT_DIR}" "${CURRENT_DIR}/include" "${CURRENT_DIR}/include/KHR")

file(GLOB stb_image_SOURCES
        "${CURRENT_DIR}/stb_image.cpp"
        )
FilterFolder("${stb_image_SOURCES}")

list(APPEND SOURCES ${stb_image_SOURCES})

# 生成链接库
add_library(stb_image ${SOURCES})

