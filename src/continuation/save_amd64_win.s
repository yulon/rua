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

; load NT_TIB
; mov r10, [gs:030h] ; I'm using FASM was unable to output correct code.
mov rax, 030h
mov r10, [gs:rax]

; deallocation stack
mov rax, [r10+01478h]
mov [rcx+152], rax

; stack limit
mov rax, [r10+010h]
mov [rcx+160], rax

; stack base
mov rax, [r10+008h]
mov [rcx+168], rax

; exception list
mov rax, [r10]
mov [rcx+176], rax

; fiber local storage
; mov rax, [r10+020h]
; mov [rcx+184], rax

mov rax, 1
ret
