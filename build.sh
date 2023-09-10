# cmake
if [ ! -d "build" ];then
    mkdir ./build
fi
cd build
cmake ../
make
os_img=clean_os.dmg
# create os img if not exist
if [ ! -f "$os_img" ];then
    qemu-img create -f raw $os_img 64M
fi

binFiles=(boot.bin 0 loader.bin 1 kernel.elf 100)
# write os data into img 
for (( i=0; i < ${#binFiles[@]}; i=i+2 ));do
    if [ ! -f "${binFiles[i]}" ]; then
        echo "\033[31m file ${binFiles[i]} not found \033[0m"
        break
    fi
    echo ${binFiles[i]}-${binFiles[i+1]}
    dd if=${binFiles[i]} of=$os_img bs=512 conv=notrunc seek=${binFiles[i+1]}
done
