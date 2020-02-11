use64

mov rbx, [rcx+8]
mov rsp, [rcx+48]
mov rbp, [rcx+56]

; ip
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

; NT_TIB
; mov r10, [gs:030h] ; I'm using FASM was unable to output correct code.
mov rax, 030h
mov rdi, [gs:rax]

add rdi, 8
lea rsi, [rcx+152]
mov rax, rcx
mov rcx, 2
rep movsq

mov rcx, [rax+16]
mov rsi, [rax+32]
mov rdi, [rax+40]
mov rax, 1
ret
