use64

mov [rdx], rax
mov [rdx+8], rbx
mov [rdx+16], rcx
mov [rdx+24], rdx
mov [rdx+32], rsi
mov [rdx+40], rdi
mov [rdx+48], rsp
mov [rdx+56], rbp

; caller_ip
mov rax, [rsp]
mov [rdx+64], rax

; flags
; pushfq
; pop rax
; mov [rdx+192], rax

mov [rdx+80], r8
mov [rdx+88], r9
mov [rdx+96], r10
mov [rdx+104], r11
mov [rdx+112], r12
mov [rdx+120], r13
mov [rdx+128], r14
mov [rdx+136], r15

stmxcsr [rdx+144]
fnstcw [rdx+148]

; load NT_TIB
; mov r10, [gs:030h] ; I'm using FASM was unable to output correct code.
mov rax, 030h
mov r10, [gs:rax]

; deallocation stack
mov rax, [r10+01478h]
mov [rdx+152], rax

; stack limit
mov rax, [r10+010h]
mov [rdx+160], rax

; stack base
mov rax, [r10+008h]
mov [rdx+168], rax

; exception list
mov rax, [r10]
mov [rdx+176], rax

; fiber local storage
mov rax, [r10+020h]
mov [rdx+184], rax

;;;;;

mov rbx, [rcx+8]
mov rsi, [rcx+32]
mov rdi, [rcx+40]
mov rsp, [rcx+48]
mov rbp, [rcx+56]

; caller_ip
mov rax, [rcx+64]
mov [rsp], rax

; flags
; mov rax, [rcx+192]
; push rax
; popfq

mov r12, [rcx+112]
mov r13, [rcx+120]
mov r14, [rcx+128]
mov r15, [rcx+136]

ldmxcsr [rcx+144]
fldcw [rcx+148]

; load NT_TIB
; mov r10, [gs:030h] ; I'm using FASM was unable to output correct code.
mov rax, 030h
mov r10, [gs:rax]

; deallocation stack
mov rax, [rcx+152]
mov [r10+01478h], rax

; stack limit
mov rax, [rcx+160]
mov [r10+010h], rax

; stack base
mov rax, [rcx+168]
mov [r10+008h], rax

; exception list
mov rax, [rcx+176]
mov [r10], rax

; fiber local storage
mov rax, [rcx+184]
mov [r10+020h], rax

mov rcx, [rcx+16]
mov rax, 0
ret
