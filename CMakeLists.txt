cmake_minimum_required(VERSION 3.5.0)

set(CMAKE_VERBOSE_MAKEFILE on)

if (CMAKE_HOST_APPLE)
    set(CMAKE_SYSTEM_NAME macOS)
    set(TOOL_PREFIX "x86_64-elf-")
elseif (CMAKE_HOST_WIN32)
    set(TOOL_PREFIX "x86_64-linux-gnu-")
endif ()

# c compiler gcc
set(CMAKE_C_COMPILER "${TOOL_PREFIX}gcc")
set(CMAKE_C_FLAGS "-g -c -O0 -m32 -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables")

# assembly compiler gcc
set(CMAKE_ASM_COMPILER "${TOOL_PREFIX}gcc")
set(CMAKE_ASM_FLAGS "-m32 -g")
set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS "asm")

set(LINKER_TOOL "${TOOL_PREFIX}ld")

# other build tools
set(OBJCOPY_TOOL "${TOOL_PREFIX}objcopy")
set(OBJDUMP_TOOL "${TOOL_PREFIX}objdump")
set(READELF_TOOL "${TOOL_PREFIX}readelf")

project(clean_os LANGUAGES C)
enable_language(ASM)

# search h file
include_directories(
    ${PROJECT_SOURCE_DIR}
)

add_subdirectory(common)
add_subdirectory(boot)
add_subdirectory(loader)
add_subdirectory(kernel)

add_dependencies(boot common)
add_dependencies(loader common)
add_dependencies(kernel common)

