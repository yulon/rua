use32

mov eax, [esp+4]

mov ebx, [eax+4]
mov esp, [eax+24]
mov ebp, [eax+28]

; ip
mov edx, [eax+32]
mov [esp], edx

; flags
; mov ecx, [eax+36]
; push ecx
; popfd

ldmxcsr [eax+40]
fldcw [eax+44]

; NT_TIB
mov edx, 018h
mov edi, [fs:edx]

add edi, 4
lea esi, [eax+48]
mov ecx, 2
rep movsd

mov esi, [eax+16]
mov edi, [eax+20]
mov eax, 0
ret
