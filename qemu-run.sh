#!/bin/bash
# -netdev vmnet-bridged,id=vmnet,ifname=en0 -device e1000-82540em,netdev=vmnet
# -nic vmnet-bridged,model=rtl8139,ifname=en0
qemu-system-i386 -m 4G $1 $2 -serial stdio -nic vmnet-bridged,model=rtl8139,ifname=en0,mac=52:54:00:c9:18:27 -drive file=./build/clean_os.dmg,index=0,media=disk,format=raw -drive file=./file_fat16.dmg,index=1,media=disk,format=raw -drive file=./file_ext2.dmg,index=2,media=disk,format=raw  -d pcall,page,cpu_reset,guest_errors,page,trace:ps2_keyboard_set_translation
                                                                                    