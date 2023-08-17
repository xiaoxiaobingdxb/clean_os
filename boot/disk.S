read_disk:
    pusha
    mov $0x8000, %bx	// 读取到的内存地址
	mov $0x2, %cx		// ch:磁道号,cl起始扇区号
	mov $0x2, %ah		// ah: 0x2读磁盘命令
	mov $64, %al		// al: 读取的扇区数量, 必须小于128,暂设置成32KB
	mov $0x0080, %dx	// dh: 磁头号,dl驱动器号0x80(磁盘1)
	int $0x13
    popa
    ret