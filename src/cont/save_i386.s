use32

mov [esp-4], eax
mov eax, [esp+4]
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

mov eax, 1
ret
