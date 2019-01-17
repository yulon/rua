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
mov rax, [r10+020h]
mov [rcx+184], rax

;;;;;

mov rbx, [rdx+8]
mov rsi, [rdx+32]
mov rdi, [rdx+40]
mov rsp, [rdx+48]
mov rbp, [rdx+56]

; caller_ip
mov rax, [rdx+64]
mov [rsp], rax

; flags
; mov rax, [rdx+192]
; push rax
; popfq

mov r12, [rdx+112]
mov r13, [rdx+120]
mov r14, [rdx+128]
mov r15, [rdx+136]

ldmxcsr [rdx+144]
fldcw [rdx+148]

; load NT_TIB
; mov r10, [gs:030h] ; I'm using FASM was unable to output correct code.
mov rax, 030h
mov r10, [gs:rax]

; deallocation stack
mov rax, [rdx+152]
mov [r10+01478h], rax

; stack limit
mov rax, [rdx+160]
mov [r10+010h], rax

; stack base
mov rax, [rdx+168]
mov [r10+008h], rax

; exception list
mov rax, [rdx+176]
mov [r10], rax

; fiber local storage
mov rax, [rdx+184]
mov [r10+020h], rax

mov rcx, [rdx+16]
mov rdx, [rdx+24]
mov rax, 0
ret
