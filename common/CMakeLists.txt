
project(common LANGUAGES C)

file(GLOB_RECURSE C_LIST "*.c" "*.h" "*.S")

set(LOG_LEVEL 0)
set(LOG_SHOW_PATH 0)
configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_SOURCE_DIR}/config.h"
)

add_library(${PROJECT_NAME} ${C_LIST})