.intel_syntax noprefix

.data
a:
    .quad 0

.text
.globl ticket_lock

ticket_lock:
    mov rcx, rdi

    add rcx, 64
    mov rdx, 1
    lock xadd qword ptr [rcx], rdx
    sub rcx, 64

    mov rsi, rdx
retry:
    mov rax, rsi
    mov rdx, [rcx]
    sub rax, rdx
    je out

    lea rax, [rax + 8 * rax]
    lea rax, [rax + 8 * rax]
    lea rax, [rax + 8 * rax]

backoff:
    rep nop
    dec rax
    jnz backoff

    jmp retry

out:
    xor rax, rax
    ret
