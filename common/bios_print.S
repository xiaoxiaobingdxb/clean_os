// print a char into scene
// param: char
print_char:
    push %ax
    push %bp
    mov %sp, %bp
    mov 0x06(%bp), %ax
    mov $0x0e, %ah
    int $0x10
    pop %bp
    pop %ax
    ret

// print a some char into scene
// param: size, char ...
print_nchar:
    push %ax
    push %cx
    push %bp
    mov %sp, %bp
    mov 0x08(%bp), %cx
    xor %ax, %ax
    mov $0x0a, %ax
print_nchar_loop:
    push 0x0(%ebp, %eax, 0x1)
    call print_char
    add $0x02, %sp
    add $0x02, %ax
    loop print_nchar_loop
    pop %bp
    pop %cx
    pop %ax
    ret

// next_line
print_nl:
    push %bp
    mov %sp, %bp
    push $0x0a // 换行
    call print_char
    add $0x02, %sp

    push $0x0d // 回车
    call print_char
    add $0x02, %sp

    pop %bp
    ret

// clean the scene
clean_scene:
    pusha
    mov $0x0600, %ax
    mov $0x0700, %bx
    mov $0, %cx
    mov $0x184f, %dx // position in scene
    int $0x10
    popa
    ret

// mov cursor into position
// param: row, cloumn
mov_cursor:
    push %ax
    push %bx
    push %dx
    push %bp
    mov %sp, %bp
    mov $0x0200, %ax
    mov $0x0, %bx
    mov 0x0a(%bp), %dh
    mov 0x0c(%bp), %dl
    int $0x10
    pop %bp
    pop %dx
    pop %bx
    pop %ax
    ret

reset_cursor:
    // mov cursor to [0, 0]
    push $0x0
    push $0x0
    call mov_cursor
    add $0x04, %sp
    ret

// read cursor
// return row,column
read_cursor:
    push %ax
    push %bx
    push %cx
    push %bp
    mov %sp, %bp
    mov $0x03, %ah
    xor %bh, %bh
    int $0x10
    mov %dx, 0x0a(%bp)
    pop %bp
    pop %cx
    pop %bx
    pop %ax
    ret


print_str:
    push %ax
    push %bx
    push %cx
    push %bp
    mov %sp, %bp
    sub $0x02, %sp
    call read_cursor
    pop %dx // position
    mov 0x0c(%bp), %cx
    mov 0x0a(%bp),%bp
    mov $0x1301, %ax
    mov $0007, %bx
    int $0x10
    pop %bp
    pop %cx
    pop %bx
    pop %ax
    ret