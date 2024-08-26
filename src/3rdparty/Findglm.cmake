get_filename_component(THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)


set(glm_INCLUDE_DIRS "${THIRDPARTY_DIR}/glm")
message("findglm ${glm_INCLUDE_DIRS}" )

