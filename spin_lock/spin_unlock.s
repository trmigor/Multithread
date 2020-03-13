.intel_syntax noprefix

.text
.globl spin_unlock
#.type spin_unlock, @function

spin_unlock:
    mov rcx, rdi
    XRELEASE mov qword ptr [rcx], 0
    xor rax, rax
    ret
