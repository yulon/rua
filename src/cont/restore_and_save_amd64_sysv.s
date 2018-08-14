use64

mov [rsi], rax
mov [rsi+8], rbx
mov [rsi+16], rcx
mov [rsi+24], rdx
mov [rsi+32], rsi
mov [rsi+40], rdi
mov [rsi+48], rsp
mov [rsi+56], rbp

; caller_ip
mov rax, [rsp]
mov [rsi+64], rax

mov [rsi+80], r8
mov [rsi+88], r9
mov [rsi+96], r10
mov [rsi+104], r11
mov [rsi+112], r12
mov [rsi+120], r13
mov [rsi+128], r14
mov [rsi+136], r15

stmxcsr [rsi+144]
fnstcw [rsi+148]

;;;;;

mov rbx, [rdi+8]
mov rsp, [rdi+48]
mov rbp, [rdi+56]

; caller_ip
mov rax, [rdi+64]
mov [rsp], rax

mov r12, [rdi+112]
mov r13, [rdi+120]
mov r14, [rdi+128]
mov r15, [rdi+136]

ldmxcsr [rdi+144]
fldcw [rdi+148]

mov rdi, [rdi+40]
mov rax, 0
ret
