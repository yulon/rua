use32

mov [esp-4], eax
mov eax, [esp+4]
mov [eax+4], ebx
mov ebx, [esp-4]
mov [eax], ebx
mov [eax+8], ecx
mov [eax+12], edx
mov [eax+16], esi
mov [eax+20], edi
mov [eax+24], esp
mov [eax+28], ebp

; caller_ip
mov ebx, [esp]
mov [eax+32], ebx

; flags
pushfd
pop ebx
mov [eax+36], ebx

stmxcsr [eax+40]
fnstenv [eax+44]

; load NT_TIB
mov ebx, 018h
mov ecx, [fs:ebx]

; fiber local storage
mov ebx, [ecx+010h]
mov [eax+72], ebx

; deallocation stack
mov ebx, [ecx+0e0ch]
mov [eax+76], ebx

; stack limit
mov ebx, [ecx+08h]
mov [eax+80], ebx

; stack base
mov ebx, [ecx+04h]
mov [eax+84], ebx

; exception list
mov ebx, [ecx]
mov [eax+88], ebx

mov ecx, [eax+8]

mov ebx, [eax+4]
mov eax, 1
ret
