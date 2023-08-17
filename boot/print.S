// char into al
print_char: 
    pusha
    mov $0x0e, %ah
    int $0x10
    popa
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

// dh: row, dl: column
mov_cursor:
    pusha
    mov $0x0200, %ax
    mov $0x0, %bx
    int $0x10
    popa
    ret

reset_cursor:
    pusha
    mov $0x0, %dx //mov cursor to [0, 0]
    call mov_cursor
    popa
    ret

// str address into es:bp, str len into cx, str position of scene into dx
print_str:
    pusha
    mov $0x01, %al
    mov $0x13, %ah
    mov $0x00, %bh
    mov $0x07, %bl // black background, white frontground
    int $0x10
    popa
    ret