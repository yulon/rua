use32

mov [esp - 4], eax
mov eax, [esp + 4]
mov [eax + 4], ebx
mov ebx, [esp - 4]
mov [eax], ebx
mov [eax + 8], ecx
mov [eax + 12], edx
mov [eax + 16], esi
mov [eax + 20], edi
mov [eax + 24], esp
mov [eax + 28], ebp
mov ebx, [esp]
mov [eax + 32], ebx
pushfd
pop ebx
mov [eax + 36], ebx
fnstenv [eax + 44]
mov ebx, [eax + 4]
mov eax, 1
ret
