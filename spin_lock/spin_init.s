.intel_syntax noprefix

.text
.globl spin_init
#.type spin_init, @function

spin_init:
    mov rcx, rdi
    mov qword ptr [rcx], 0
    xor rax, rcx
    ret
