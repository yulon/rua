use64

mov rbx, [rdi+8]
mov rcx, [rdi+16]
mov rsi, [rdi+32]
mov rsp, [rdi+48]
mov rbp, [rdi+56]
mov rdx, [rdi+64]
push rdx
mov rdx, [rdi+72]
push rdx
popfq
mov r8, [rdi+80]
mov r9, [rdi+88]
mov r10, [rdi+96]
mov r11, [rdi+104]
mov r12, [rdi+112]
mov r13, [rdi+120]
mov r14, [rdi+128]
mov r15, [rdi+136]
mov rdx, [rdi+24]
ldmxcsr [rdi+144]
fldenv [rdi+152]
mov rdi, [rdi+40]
mov rax, 0
ret
