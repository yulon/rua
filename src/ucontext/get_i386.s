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

mov eax, 0
ret
