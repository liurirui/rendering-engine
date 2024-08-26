get_filename_component(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

set(glad_INCLUDE_DIRS "${THIRDPARTY_DIR}/glad" "${THIRDPARTY_DIR}/glad/include")

set(glad_LIBRARIES glad)
message("findglad ${glad_INCLUDE_DIRS}")