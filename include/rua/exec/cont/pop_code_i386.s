use32

mov eax, [esp+4]
mov ecx, [eax+8]
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

mov ebx, [eax+4]
mov eax, 0
ret
