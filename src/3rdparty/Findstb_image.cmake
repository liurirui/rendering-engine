get_filename_component(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

set(stb_image_INCLUDE_DIRS "${THIRDPARTY_DIR}/stb_image" "${THIRDPARTY_DIR}/stb_image/include")

set(stb_image_LIBRARIES stb_image)