project(glibc LANGUAGES C)

file(GLOB_RECURSE C_LIST "*.c" "*.h" "*.S")
add_library(${PROJECT_NAME} ${C_LIST})

option(KERNEL "build in kernel" 0)

if(${KERNEL})
    add_definitions(-DKERNEL)
endif()

# add_custom_command(
#     TARGET ${PROJECT_NAME}
#     POST_BUILD
#     COMMAND ${AR_TOOL} x ${CMAKE_BINARY_DIR}/common/libcommon.a
#     COMMAND ${AR_TOOL} rcs ${CMAKE_BINARY_DIR}/glibc/libglibc.a ${CMAKE_BINARY_DIR}/glibc/*.obj
# )
add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${OBJDUMP_TOOL} -x -d -S -m i386 ${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}.a > ${PROJECT_NAME}_dis.txt
    COMMAND ${READELF_TOOL} -a ${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}.a > ${PROJECT_NAME}_elf.txt
)