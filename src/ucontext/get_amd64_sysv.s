use64

mov [rdi], rax
mov [rdi+8], rbx
mov [rdi+16], rcx
mov [rdi+24], rdx
mov [rdi+32], rsi
mov [rdi+40], rdi
mov [rdi+48], rsp
mov [rdi+56], rbp

; ip
mov rax, [rsp]
mov [rdi+64], rax

; flags
; pushfq
; pop rax
; mov [rdi+72], rax

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

mov rax, 0
mov [rdi+152], rax
mov [rdi+160], rax

mov rax, 1
ret
