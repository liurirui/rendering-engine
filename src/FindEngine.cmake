get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

set(engine_INCLUDE_DIRS "${CURRENT_DIR}" "${CURRENT_DIR}/Engine/Renderer/.." "${CURRENT_DIR}/Engine/RHI/.." "${CURRENT_DIR}/Engine/RHI/OpenGL/.." "${CURRENT_DIR}/Engine/Base/..")
set(engine_LIBRARIES engine)