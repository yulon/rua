use64

mov [rcx], rax
mov [rcx+8], rbx
mov [rcx+16], rcx
mov [rcx+24], rdx
mov [rcx+32], rsi
mov [rcx+40], rdi
mov [rcx+48], rsp
mov [rcx+56], rbp

; ip
mov rax, [rsp]
mov [rcx+64], rax

; flags
; pushfq
; pop rax
; mov [rcx+192], rax

mov [rcx+80], r8
mov [rcx+88], r9
mov [rcx+96], r10
mov [rcx+104], r11
mov [rcx+112], r12
mov [rcx+120], r13
mov [rcx+128], r14
mov [rcx+136], r15

stmxcsr [rcx+144]
fnstcw [rcx+148]

; NT_TIB
; mov r10, [gs:030h] ; I'm using FASM was unable to output correct code.
mov rax, 030h
mov rsi, [gs:rax]

add rsi, 8
lea rdi, [rcx+152]
mov rax, rcx
mov rcx, 2
rep movsq

mov rsi, [rax+32]
mov rdi, [rax+40]
mov rax, 1
ret
