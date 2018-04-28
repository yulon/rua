use32

mov eax, [esp+4]
mov edx, [eax+12]
mov esi, [eax+16]
mov edi, [eax+20]
mov esp, [eax+24]
mov ebp, [eax+28]

; caller_ip
mov ebx, [eax+32]
mov [esp], ebx

; flags
mov ebx, [eax+36]
push ebx
popfd

ldmxcsr [eax+40]
fldenv [eax+44]

; load NT_TIB
mov ebx, 018h
mov ecx, [fs:ebx]

; fiber local storage
mov ebx, [eax+72]
mov [ecx+010h], ebx

; deallocation stack
mov ebx, [eax+76]
mov [ecx+0e0ch], ebx

; stack limit
mov ebx, [eax+80]
mov [ecx+08h], ebx

; stack base
mov ebx, [eax+84]
mov [ecx+04h], ebx

; exception list
mov ebx, [eax+88]
mov [ecx], ebx

mov ecx, [eax+8]

mov ebx, [eax+4]
mov eax, 0
ret
