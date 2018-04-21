use64

mov [rdi + 24], rdx
mov [rdi], rax
mov [rdi+8], rbx
mov [rdi+16], rcx
mov [rdi+32], rsi
mov [rdi+40], rdi
lea rdx, [rsp+8]
mov [rdi+48], rdx
mov [rdi+56], rbp
mov rdx, [rsp]
mov [rdi+64], rdx
pushfq
pop rdx
mov [rdi+72], rdx
mov [rdi+80], r8
mov [rdi+88], r9
mov [rdi+96], r10
mov [rdi+104], r11
mov [rdi+112], r12
mov [rdi+120], r13
mov [rdi+128], r14
mov [rdi+136], r15
mov rdx, [rdi+24]
stmxcsr [rdi+144]
fnstenv [rdi+152]
mov rax, 1
ret
