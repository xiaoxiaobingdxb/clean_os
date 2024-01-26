
project(common LANGUAGES C)

file(GLOB_RECURSE C_LIST "*.c" "*.h" "*.S")

set(LOG_LEVEL 0)
set(LOG_SHOW_PATH 0)
configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_SOURCE_DIR}/config.h"
)

add_library(${PROJECT_NAME} ${C_LIST})

add_custom_command(TARGET ${PROJECT_NAME}
POST_BUILD
COMMAND ${OBJDUMP_TOOL} -x -d -S -m i386 ${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}.a > ${PROJECT_NAME}_dis.txt
COMMAND ${READELF_TOOL} -a ${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}.a > ${PROJECT_NAME}_elf.txt
)