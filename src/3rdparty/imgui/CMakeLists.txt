get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


include_directories("${CURRENT_DIR}")


file(GLOB imgui_SOURCES 
    "${CURRENT_DIR}/imconfig.h"
    "${CURRENT_DIR}/imgui_draw.cpp"
    "${CURRENT_DIR}/imgui_demo.cpp"
    "${CURRENT_DIR}/imgui_internal.h"
    "${CURRENT_DIR}/imgui_tables.cpp"
    "${CURRENT_DIR}/imgui_widgets.cpp"
    "${CURRENT_DIR}/imgui.cpp"
    "${CURRENT_DIR}/imgui.h"
    "${CURRENT_DIR}/imstb_rectpack.h"
    "${CURRENT_DIR}/imstb_textedit.h"
    "${CURRENT_DIR}/imstb_truetype.h"
    )
FilterFolder("${imgui_SOURCES}")

list(APPEND SOURCES ${imgui_SOURCES})

# 生成链接库
add_library(imgui ${SOURCES})

