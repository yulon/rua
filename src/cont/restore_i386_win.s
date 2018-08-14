use32

mov eax, [esp+4]

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

; load NT_TIB
mov ecx, 018h
mov edx, [fs:ecx]

; deallocation stack
mov ecx, [eax+48]
mov [edx+0e0ch], ecx

; stack limit
mov ecx, [eax+52]
mov [edx+08h], ecx

; stack base
mov ecx, [eax+56]
mov [edx+04h], ecx

; exception list
mov ecx, [eax+60]
mov [edx], ecx

; fiber local storage
; mov ecx, [eax+64]
; mov [edx+010h], ecx

mov eax, 0
ret
