use64

mov [rcx], rax
mov [rcx+8], rbx
mov [rcx+16], rcx
mov [rcx+24], rdx
mov [rcx+32], rsi
mov [rcx+40], rdi
mov [rcx+48], rsp
mov [rcx+56], rbp

; caller_ip
mov rax, [rsp]
mov [rcx+64], rax

; flags
pushfq
pop rax
mov [rcx+72], rax

mov [rcx+80], r8
mov [rcx+88], r9
mov [rcx+96], r10
mov [rcx+104], r11
mov [rcx+112], r12
mov [rcx+120], r13
mov [rcx+128], r14
mov [rcx+136], r15

stmxcsr [rcx+144]
fnstenv [rcx+148]

mov rax, 1
ret
