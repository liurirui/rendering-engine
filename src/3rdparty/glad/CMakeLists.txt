get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


include_directories("${CURRENT_DIR}" "${CURRENT_DIR}/include" "${CURRENT_DIR}/include/KHR")

file(GLOB glad_SOURCES
        "${CURRENT_DIR}/glad.c"
        )
FilterFolder("${glad_SOURCES}")

list(APPEND SOURCES ${glad_SOURCES})

# 生成链接库
add_library(glad ${SOURCES})

