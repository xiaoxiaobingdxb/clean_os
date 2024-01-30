project(shell LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE ON)

# linker
set(LIBS_FLAGS "-L ${CMAKE_BINARY_DIR}/common/ -lcommon")
set(LIBS_FLAGS "${LIBS_FLAGS} -L ${CMAKE_BINARY_DIR}/applib/ -lapp")
set(LIBS_FLAGS "${LIBS_FLAGS} -L ${CMAKE_BINARY_DIR}/glibc/ -lglibc")
set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386 -T ${APP_LDS} ${LIBS_FLAGS}")
set(CMAKE_C_LINK_EXECUTABLE "${LINKER_TOOL} <OBJECTS> ${CMAKE_EXE_LINKER_FLAGS} -o ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf")

# link_directories(${PROJECT_BINARY_DIR}/common ${PROJECT_BINARY_DIR}/glibc)
# add_library(common_lib STATIC IMPORTED)
# add_library(glibc_lib STATIC IMPORTED)
# set_property(TARGET common_lib PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/common/libcommon.a)
# set_property(TARGET glibc_lib PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/glibc/libglibc.a)

file(GLOB_RECURSE C_LIST "*.c" "*.h")
add_executable(${PROJECT_NAME} ${C_LIST})

# target_link_libraries(${PROJECT_NAME} -Wl,--start-group common_lib glibc_lib -Wl,--end-group)

add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${OBJCOPY_TOOL} -S ${PROJECT_NAME}.elf ../${PROJECT_NAME}.bin
    COMMAND ${OBJDUMP_TOOL} -x -d -S -m i8086 ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_dis.txt
    COMMAND ${READELF_TOOL} -a ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf > ${PROJECT_NAME}_elf.txt
)