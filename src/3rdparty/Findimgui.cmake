get_filename_component(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

set(imgui_INCLUDE_DIRS "${THIRDPARTY_DIR}/imgui")

set(imgui_LIBRARIES imgui)
message("findimgui ${imgui_INCLUDE_DIRS}")