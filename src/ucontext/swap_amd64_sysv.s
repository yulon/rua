use64

mov [rdi], rax
mov [rdi+8], rbx
mov [rdi+16], rcx
mov [rdi+24], rdx
mov [rdi+32], rsi
mov [rdi+40], rdi
mov [rdi+48], rsp
mov [rdi+56], rbp

; caller_ip
mov rax, [rsp]
mov [rdi+64], rax

mov [rdi+80], r8
mov [rdi+88], r9
mov [rdi+96], r10
mov [rdi+104], r11
mov [rdi+112], r12
mov [rdi+120], r13
mov [rdi+128], r14
mov [rdi+136], r15

stmxcsr [rdi+144]
fnstcw [rdi+148]

;;;;;

mov rbx, [rsi+8]
mov rsp, [rsi+48]
mov rbp, [rsi+56]

; caller_ip
mov rax, [rsi+64]
mov [rsp], rax

mov r12, [rsi+112]
mov r13, [rsi+120]
mov r14, [rsi+128]
mov r15, [rsi+136]

ldmxcsr [rsi+144]
fldcw [rsi+148]

mov rdi, [rsi+40]
mov rsi, [rsi+32]
mov rax, 0
ret
