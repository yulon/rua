use64

mov rbx, [rcx+8]
mov rdx, [rcx+24]
mov rsi, [rcx+32]
mov rdi, [rcx+40]
mov rsp, [rcx+48]
mov rbp, [rcx+56]

; caller_ip
mov rax, [rcx+64]
mov [rsp], rax

; flags
mov rax, [rcx+72]
push rax
popfq

mov r8, [rcx+80]
mov r9, [rcx+88]
mov r10, [rcx+96]
mov r11, [rcx+104]
mov r12, [rcx+112]
mov r13, [rcx+120]
mov r14, [rcx+128]
mov r15, [rcx+136]
mov rdx, [rcx+24]

ldmxcsr [rcx+144]
fldenv [rcx+148]

mov rcx, [rcx+16]
mov rax, 0
ret
