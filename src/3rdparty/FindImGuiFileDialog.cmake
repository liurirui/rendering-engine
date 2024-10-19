# 获取当前 CMake 文件的路径
get_filename_component(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

# 设置 ImGuiFileDialog 的包含目录
set(ImGuiFileDialog_INCLUDE_DIRS "${THIRDPARTY_DIR}/ImGuiFileDialog")

# 设置 ImGuiFileDialog 库的源文件
set(ImGuiFileDialog_SOURCES "${THIRDPARTY_DIR}/ImGuiFileDialog/ImGuiFileDialog.cpp")

# 设置库名称为 ImGuiFileDialog
set(ImGuiFileDialog_LIBRARIES ImGuiFileDialog)

# 输出调试信息
message(STATUS "FindImGuiFileDialog: ${ImGuiFileDialog_INCLUDE_DIRS}")
