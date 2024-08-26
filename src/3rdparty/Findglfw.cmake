get_filename_component(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


set(glfw_INCLUDE_DIRS "${THIRDPARTY_DIR}/glfw/include")
set(glfw_LIBRARIES "${THIRDPARTY_DIR}/glfw/lib/glfw3.lib")
message("findglfw ${glfw_INCLUDE_DIRS} ${glfw_LIBRARIES}" )

