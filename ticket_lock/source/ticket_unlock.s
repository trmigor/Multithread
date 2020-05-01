.intel_syntax noprefix

.text
.globl ticket_unlock

ticket_unlock:
    mov rcx, rdi
    mov rdx, 1
    XRELEASE lock xadd qword ptr [rcx], rdx
    xor rax, rax
    ret
