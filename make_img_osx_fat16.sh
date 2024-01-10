hdiutil create -fs "MS-DOS FAT16" -volname sdba -size 64M -layout MBRSPUD -ov file.dmg

mkdir mp
hdiutil attach -mountpoint mp file.dmg
cd mp
echo long\nlong\nlong\ntest\n >> longlonglong_file.txt
mkdir sub_dir
cd sub_dir
echo sub_test >> sub_test.txt
cd ../..
hdiutil detach mp
rm -rf mp
 
