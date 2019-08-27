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

; load NT_TIB
mov ecx, 018h
mov edx, [fs:ecx]

; deallocation stack
mov ecx, [edx+0e0ch]
mov [eax+48], ecx

; stack limit
mov ecx, [edx+08h]
mov [eax+52], ecx

; stack base
mov ecx, [edx+04h]
mov [eax+56], ecx

; exception list
mov ecx, [edx]
mov [eax+60], ecx

; fiber local storage
; mov ecx, [edx+010h]
; mov [eax+64], ecx

;;;;;

mov eax, [esp+8]

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
