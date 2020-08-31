; Copyright 2017-2020 Siemens AG
; 
; Permission is hereby granted, free of charge, to any person obtaining
; a copy of this software and associated documentation files (the
; "Software"), to deal in the Software without restriction, including without
; limitation the rights to use, copy, modify, merge, publish, distribute,
; sublicense, and/or sell copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following conditions:
; 
;The above copyright notice and this permission notice shall be
;The included in all copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
; SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
; OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
; ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
; DEALINGS IN THE SOFTWARE.
; 
; Author(s): Thomas Riedmaier


IFDEF RAX

?mystrcmp_@@YAH_KPEBD1@Z PROTO C
?mymemcmp_@@YAH_KPEBX10@Z PROTO C
?my_stricmp_@@YAH_KPEBD1@Z PROTO C
?mystrcmpi_@@YAH_KPEBD1@Z PROTO C
?mystricmp_@@YAH_KPEBD1@Z PROTO C
?mystrncmp_@@YAH_KPEBD10@Z PROTO C

.CODE

mystrcmp PROC
	sub rsp, 32 ; create shadow space
	mov r8, rdx ; parameter3 of mystrcmp_ 
	mov rdx, rcx ; parameter2 of mystrcmp_
	pop rcx ; parameter1 of mystrcmp_
	push rcx ; restore stack
	call ?mystrcmp_@@YAH_KPEBD1@Z
	add rsp, 32; cleanup shadow space
	ret
mystrcmp ENDP

mymemcmp PROC
	sub rsp, 32 ; create shadow space
	mov r9, r8 ; parameter4 of mymemcmp_ 
	mov r8, rdx ; parameter3 of mymemcmp_ 
	mov rdx, rcx ; parameter2 of mymemcmp_
	pop rcx ; parameter1 of mymemcmp_
	push rcx ; restore stack
	call ?mymemcmp_@@YAH_KPEBX10@Z
	add rsp, 32; cleanup shadow space
	ret
mymemcmp ENDP

my_stricmp PROC
	sub rsp, 32 ; create shadow space
	mov r8, rdx ; parameter3 of my_stricmp_ 
	mov rdx, rcx ; parameter2 of my_stricmp_
	pop rcx ; parameter1 of my_stricmp_
	push rcx ; restore stack
	call ?my_stricmp_@@YAH_KPEBD1@Z
	add rsp, 32; cleanup shadow space
	ret
my_stricmp ENDP

mystrcmpi PROC
	sub rsp, 32 ; create shadow space
	mov r8, rdx ; parameter3 of mystrcmpi_ 
	mov rdx, rcx ; parameter2 of mystrcmpi_
	pop rcx ; parameter1 of mystrcmpi_
	push rcx ; restore stack
	call ?mystrcmpi_@@YAH_KPEBD1@Z
	add rsp, 32; cleanup shadow space
	ret
mystrcmpi ENDP

mystricmp PROC
	sub rsp, 32 ; create shadow space
	mov r8, rdx ; parameter3 of mystricmp_ 
	mov rdx, rcx ; parameter2 of mystricmp_
	pop rcx ; parameter1 of mystricmp_
	push rcx ; restore stack
	call ?mystricmp_@@YAH_KPEBD1@Z
	add rsp, 32; cleanup shadow space
	ret
mystricmp ENDP

mystrncmp PROC
	sub rsp, 32 ; create shadow space
	mov r9, r8 ; parameter4 of mystrncmp_ 
	mov r8, rdx ; parameter3 of mystrncmp_ 
	mov rdx, rcx ; parameter2 of mystrncmp_
	pop rcx ; parameter1 of mystrncmp_
	push rcx ; restore stack
	call ?mystrncmp_@@YAH_KPEBD10@Z
	add rsp, 32; cleanup shadow space
	ret
mystrncmp ENDP

ELSE

.MODEL FLAT, C

?mystrcmp_@@YAHIPBD0@Z PROTO SYSCALL
?mymemcmp_@@YAHIPBX0I@Z PROTO SYSCALL
?my_stricmp_@@YAHIPBD0@Z PROTO SYSCALL
?mystrcmpi_@@YAHIPBD0@Z PROTO SYSCALL
?mystricmp_@@YAHIPBD0@Z PROTO SYSCALL
?mystrncmp_@@YAHIPBD0I@Z PROTO SYSCALL

.CODE


mystrcmp PROC
	pop eax ; caller address
	pop edx ; str1
	pop ecx ; str2
	push ecx ; restore stack 1/3
	push edx ; restore stack 2/3
	push eax ; restore stack 3/3
	push ecx ; parameter3 of mystrcmp_ 
	push edx ; parameter2 of mystrcmp_
	push eax ; parameter1 of mystrcmp_
	call ?mystrcmp_@@YAHIPBD0@Z
	add esp, 12
	ret
mystrcmp ENDP

mymemcmp PROC
	sub esp, 16
	mov eax, [esp+16]
	mov [esp], eax ; parameter1 of mymemcmp_
	mov eax, [esp+20]
	mov [esp+4], eax ; parameter2 of mymemcmp_
	mov eax, [esp+24]
	mov [esp+8], eax ; parameter3 of mymemcmp_
	mov eax, [esp+28]
	mov [esp+12], eax ; parameter4 of mymemcmp_
	call ?mymemcmp_@@YAHIPBX0I@Z
	add esp, 16
	ret
mymemcmp ENDP

my_stricmp PROC
	pop eax ; caller address
	pop edx ; str1
	pop ecx ; str2
	push ecx ; restore stack 1/3
	push edx ; restore stack 2/3
	push eax ; restore stack 3/3
	push ecx ; parameter3 of my_stricmp_ 
	push edx ; parameter2 of my_stricmp_
	push eax ; parameter1 of my_stricmp_
	call ?my_stricmp_@@YAHIPBD0@Z
	add esp, 12
	ret
my_stricmp ENDP

mystrcmpi PROC
	pop eax ; caller address
	pop edx ; str1
	pop ecx ; str2
	push ecx ; restore stack 1/3
	push edx ; restore stack 2/3
	push eax ; restore stack 3/3
	push ecx ; parameter3 of mystrcmpi_ 
	push edx ; parameter2 of mystrcmpi_
	push eax ; parameter1 of mystrcmpi_
	call ?mystrcmpi_@@YAHIPBD0@Z
	add esp, 12
	ret
mystrcmpi ENDP

mystricmp PROC
	pop eax ; caller address
	pop edx ; str1
	pop ecx ; str2
	push ecx ; restore stack 1/3
	push edx ; restore stack 2/3
	push eax ; restore stack 3/3
	push ecx ; parameter3 of mystricmp_ 
	push edx ; parameter2 of mystricmp_
	push eax ; parameter1 of mystricmp_
	call ?mystricmp_@@YAHIPBD0@Z
	add esp, 12
	ret
mystricmp ENDP

mystrncmp PROC
	sub esp, 16
	mov eax, [esp+16]
	mov [esp], eax ; parameter1 of mystrncmp_
	mov eax, [esp+20]
	mov [esp+4], eax ; parameter2 of mystrncmp_
	mov eax, [esp+24]
	mov [esp+8], eax ; parameter3 of mystrncmp_
	mov eax, [esp+28]
	mov [esp+12], eax ; parameter4 of mystrncmp_
	call ?mystrncmp_@@YAHIPBD0I@Z
	add esp, 16
	ret
mystrncmp ENDP

ENDIF

END
