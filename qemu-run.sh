#!/bin/bash
qemu-system-i386 -m 4G $1 $2 -serial stdio -drive file=./build/clean_os.dmg,media=disk,format=raw -d pcall,page,cpu_reset,guest_errors,page,trace:ps2_keyboard_set_translation
                                                                                    