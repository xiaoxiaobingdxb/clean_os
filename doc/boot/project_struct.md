# CMake工程构建与调试

为了后续方便更方便，工程还需要支持集成构建和调试功能。由于使用的是gcc编译构建链工具，而且CMake也完美支持了gcc，因此项目构建使用CMake。

# 编译

在clean_os工程根目录创建CMakeLists.txt文件，表示构建的总project，在里面定义和设置gcc全局配置，并且引入子project

```cmake
cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_VERBOSE_MAKEFILE on)

if (CMAKE_HOST_APPLE)
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

project(clean_os LANGUAGES C)
enable_language(ASM)

add_subdirectory(./boot)
```

这里主要是设置了gcc的c编译器和汇编编译器，并且引入了boot子project

同样也需要在boot目录创建一个CMakeLists.txt文件，在里面配置编译输出文件和连接方式

```cmake
project(boot LANGUAGES C)

# linker
set(CMAKE_EXE_LINKER_FLAGS "-m elf_i386 -Ttext=0x7c00 --section-start boot_end=0x7dfe")
set(CMAKE_C_LINK_EXECUTABLE "${LINKER_TOOL} <OBJECTS> ${CMAKE_EXE_LINKER_FLAGS} -o ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.elf")

file(GLOB C_LIST "*.c" "*.h")
add_executable(${PROJECT_NAME} start.s ${C_LIST})

add_custom_command(TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${OBJCOPY_TOOL} -O binary ${PROJECT_NAME}.elf ../${PROJECT_NAME}.bin
)
```

这里运用到了前面说到的ld参数，指定代码段地址和boot_end的位置，把生成的elf文件用objcopyp命令生成bin文件

# 调试

调试使用gdb，在vscode中装好gdb debuger插件后，需要在工程目录里创建文件.vscode/launch.json来配置gdb调试配置文件

```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(gdb) 启动",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceRoot}/build/boot/boot.elf",
            "stopAtEntry": false,
            "cwd": "${workspaceRoot}",
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerServerAddress": "127.0.0.1:1234",
            "targetArchitecture": "x86",
            "stopAtConnect": true,
            "miDebuggerPath": "x86_64-elf-gdb",
            "linux": {
                "miDebuggerPath": "gdb",
            },
            "customLaunchSetupCommands": [],
            "setupCommands": [
                {
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                },
            ],
            "postRemoteConnectCommands": [
                {
                    "text": "-file-symbol-file ./build/boot/boot.elf",
                    "ignoreFailures": false
                },
                {
                    "text": "-exec-until *0x7c00",
                    "ignoreFailures": false
                }
            ]
        },
    ],
}
```

这里gdb需要之前编译生成的elf来识别源文件，同时设置qemu执行到0x7c00这个内存位置时就暂停，相当于走到断点，gdb连接上qemu使用127.0.0.1:1234这个默认的remote，qemu启动时就需要添加上`-S -s`参数，因此为了方便也为qemu的启动写一个脚本qemu-debug.sh

```shell
qemu-system-i386 -S -s -drive file=./build/boot.bin,media=disk,format=raw
```

此时执行qemu-debug.sh可qemu会处于待连接的wait状态，启动vscode的gdb调试，这时就会断点在_start标记处，这个位置刚好也是在0x7c00内存位置。
