use64

mov rbx, [rdi+8]
mov rsp, [rdi+48]
mov rbp, [rdi+56]

; ip
mov rax, [rdi+64]
mov [rsp], rax

; flags
; mov rax, [rdi+72]
; push rax
; popfq

mov r12, [rdi+112]
mov r13, [rdi+120]
mov r14, [rdi+128]
mov r15, [rdi+136]

ldmxcsr [rdi+144]
fldcw [rdi+148]

mov rdi, [rdi+40]
mov rax, 1
ret
