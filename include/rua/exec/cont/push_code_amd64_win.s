use64

mov [rcx + 24], rdx
mov [rcx], rax
mov [rcx+8], rbx
mov [rcx+16], rcx
mov [rcx+32], rsi
mov [rcx+40], rdi
mov [rcx+48], rsp
mov [rcx+56], rbp
mov rdx, [rsp]
mov [rcx+64], rdx
pushfq
pop rdx
mov [rcx+72], rdx
mov [rcx+80], r8
mov [rcx+88], r9
mov [rcx+96], r10
mov [rcx+104], r11
mov [rcx+112], r12
mov [rcx+120], r13
mov [rcx+128], r14
mov [rcx+136], r15
mov rdx, [rcx+24]
stmxcsr [rcx+144]
fnstenv [rcx+152]
mov rax, 1
ret
