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
mov ecx, [edx+010h]
mov [eax+64], ecx

mov eax, 1
ret
