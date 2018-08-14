use32

mov [esp-4], eax
mov eax, [esp+8]

mov [eax+8], ecx
mov ecx, [esp-4]
mov [eax], ecx
mov [eax+4], ebx
mov [eax+12], edx
mov [eax+16], esi
mov [eax+20], edi
mov [eax+24], esp
mov [eax+28], ebp

; caller_ip
mov ecx, [esp]
mov [eax+32], ecx

; flags
; pushfd
; pop ecx
; mov [eax+36], ecx

stmxcsr [eax+40]
fnstcw [eax+44]

;;;;;

mov eax, [esp+4]

mov ebx, [eax+4]
mov esi, [eax+16]
mov edi, [eax+20]
mov esp, [eax+24]
mov ebp, [eax+28]

; caller_ip
mov ecx, [eax+32]
mov [esp], ecx

; flags
; mov ecx, [eax+36]
; push ecx
; popfd

ldmxcsr [eax+40]
fldcw [eax+44]

mov eax, 0
ret
