aux_source_directory(. SRC)
add_library(dynamic_bylSocket SHARED ${SRC})
add_library(static_bylSocket STATIC ${SRC})
SET_TARGET_PROPERTIES(dynamic_bylSocket static_bylSocket
        PROPERTIES OUTPUT_NAME "bylsocket")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)