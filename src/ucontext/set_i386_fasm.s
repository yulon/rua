use32

mov eax, [esp+4]

mov ebx, [eax+4]
mov esi, [eax+16]
mov edi, [eax+20]
mov esp, [eax+24]
mov ebp, [eax+28]

; ip
mov ecx, [eax+32]
mov [esp], ecx

; flags
; mov ecx, [eax+36]
; push ecx
; popfd

ldmxcsr [eax+40]
fldcw [eax+44]

mov eax, 1
ret
