use64

mov rbx, [rcx+8]
mov rsi, [rcx+32]
mov rdi, [rcx+40]
mov rsp, [rcx+48]
mov rbp, [rcx+56]

; caller_ip
mov rax, [rcx+64]
mov [rsp], rax

; flags
; mov rax, [rdi+192]
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
; mov rax, [rcx+184]
; mov [r10+020h], rax

mov rcx, [rcx+16]
mov rax, 0
ret
