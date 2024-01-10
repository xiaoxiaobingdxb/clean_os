dd if=/dev/zero of=file_ext2.dmg bs=1M count=64
$(brew --prefix e2fsprogs)/sbin/mkfs.ext2 file_ext2.dmg