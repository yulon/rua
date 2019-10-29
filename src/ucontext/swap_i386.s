use32

mov [esp-4], eax
mov eax, [esp+4]

mov [eax+12], edx
mov edx, [esp-4]
mov [eax], edx
mov [eax+4], ebx
mov [eax+8], ecx
mov [eax+16], esi
mov [eax+20], edi
mov [eax+24], esp
mov [eax+28], ebp

; ip
mov edx, [esp]
mov [eax+32], edx

; flags
; pushfd
; pop edx
; mov [eax+36], edx

stmxcsr [eax+40]
fnstcw [eax+44]

mov edx, 0
mov [eax+48], edx
mov [eax+52], edx

;;;;;

mov eax, [esp+8]

mov ebx, [eax+4]
mov esi, [eax+16]
mov edi, [eax+20]
mov esp, [eax+24]
mov ebp, [eax+28]

; ip
mov edx, [eax+32]
mov [esp], edx

; flags
; mov edx, [eax+36]
; push edx
; popfd

ldmxcsr [eax+40]
fldcw [eax+44]

mov eax, 0
ret
