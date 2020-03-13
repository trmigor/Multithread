.intel_syntax noprefix

.text
.globl spin_lock
#.type spin_lock, @function

spin_lock:
    mov rcx, rdi
    mov rdx, 1
retry:
    xor rax, rax
    XACQUIRE lock cmpxchg qword ptr [rcx], rdx
    je out 

pause:
    mov rax, [rcx]
    test rax, rax 
    jz retry
    rep nop 

    jmp pause

out:
    xor rax, rax
    ret 
