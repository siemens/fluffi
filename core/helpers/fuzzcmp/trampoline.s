# Copyright 2017-2020 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including without
# limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier

.global mystrcmp
.global mymemcmp
.global my_stricmp
.global mystrcmpi
.global mystricmp
.global mystrncmp
.global memcmp
.global strcmp
.global _stricmp
.global strcmpi
.global stricmp
.global strncmp

.text

strcmp:
	jmp mystrcmp

memcmp:
	jmp mymemcmp

_stricmp:
	jmp my_stricmp

strcmpi:
	jmp mystrcmpi

stricmp:
	jmp mystricmp

strncmp:
	jmp mystrncmp

.if ARCH == 64


mystrcmp:
	push %rbp
	mov %rsp,%rbp
	mov %rsi, %rdx # parameter3 of mystrcmp_ 
	mov %rdi, %rsi  # parameter2 of mystrcmp_
	mov 8(%rbp), %rdi # parameter1 of mystrcmp_
	call _Z9mystrcmp_mPKcS0_@plt 
	leave # restore stack
	ret

mymemcmp:
	push %rbp
	mov %rsp,%rbp
	mov %rdx, %rcx  # parameter4 of mymemcmp_ 
	mov %rsi, %rdx  # parameter3 of mymemcmp_ 
	mov %rdi, %rsi # parameter2 of mymemcmp_
	mov 8(%rbp), %rdi # parameter1 of mymemcmp_
	call _Z9mymemcmp_mPKvS0_m@plt 
	leave # restore stack
	ret

my_stricmp:
	push %rbp
	mov %rsp,%rbp
	mov %rsi, %rdx # parameter3 of my_stricmp_ 
	mov %rdi, %rsi  # parameter2 of my_stricmp_
	mov 8(%rbp), %rdi # parameter1 of my_stricmp_
	call _Z10mystrcmpi_mPKcS0_@plt 
	leave # restore stack
	ret

mystrcmpi:
	push %rbp
	mov %rsp,%rbp
	mov %rsi, %rdx # parameter3 of mystrcmpi_ 
	mov %rdi, %rsi  # parameter2 of mystrcmpi_
	mov 8(%rbp), %rdi # parameter1 of mystrcmpi_
	call _Z10mystrcmpi_mPKcS0_@plt 
	leave # restore stack
	ret

mystricmp:
	push %rbp
	mov %rsp,%rbp
	mov %rsi, %rdx # parameter3 of mystricmp_ 
	mov %rdi, %rsi  # parameter2 of mystricmp_
	mov 8(%rbp), %rdi # parameter1 of mystricmp_
	call _Z10mystricmp_mPKcS0_@plt 
	leave # restore stack
	ret

mystrncmp:
	push %rbp
	mov %rsp,%rbp
	mov %rdx, %rcx  # parameter4 of mystrncmp_ 
	mov %rsi, %rdx  # parameter3 of mystrncmp_ 
	mov %rdi, %rsi # parameter2 of mystrncmp_
	mov 8(%rbp), %rdi # parameter1 of mystrncmp_
	call _Z10mystrncmp_mPKcS0_m@plt 
	leave # restore stack
	ret

.else



mystrcmp:
	pop %eax # caller address
	pop %edx # str1
	pop %ecx # str2
	push %ecx # restore stack 1/3
	push %edx # restore stack 2/3
	push %eax # restore stack 3/3
	push %ecx # parameter3 of mystrcmp_ 
	push %edx # parameter2 of mystrcmp_
	push %eax # parameter1 of mystrcmp_
	call _Z9mystrcmp_jPKcS0_@plt 
	add $12, %esp 
	ret

mymemcmp:
	sub $16, %esp 
	mov 16(%esp), %eax
	mov %eax, (%esp) # parameter1 of mymemcmp_
	mov 20(%esp), %eax
	mov %eax, 4(%esp) # parameter2 of mymemcmp_
	mov 24(%esp), %eax 
	mov %eax, 8(%esp) # parameter3 of mymemcmp_
	mov 28(%esp), %eax
	mov %eax, 12(%esp) # parameter4 of mymemcmp_
	call _Z9mymemcmp_jPKvS0_j@plt 
	add $16, %esp
	ret

my_stricmp:
	pop %eax # caller address
	pop %edx # str1
	pop %ecx # str2
	push %ecx # restore stack 1/3
	push %edx # restore stack 2/3
	push %eax # restore stack 3/3
	push %ecx # parameter3 of my_stricmp_ 
	push %edx # parameter2 of my_stricmp_
	push %eax # parameter1 of my_stricmp_
	call _Z11my_stricmp_jPKcS0_@plt 
	add $12, %esp 
	ret

mystrcmpi:
	pop %eax # caller address
	pop %edx # str1
	pop %ecx # str2
	push %ecx # restore stack 1/3
	push %edx # restore stack 2/3
	push %eax # restore stack 3/3
	push %ecx # parameter3 of mystrcmpi_ 
	push %edx # parameter2 of mystrcmpi_
	push %eax # parameter1 of mystrcmpi_
	call _Z10mystrcmpi_jPKcS0_@plt 
	add $12, %esp 
	ret

mystricmp:
	pop %eax # caller address
	pop %edx # str1
	pop %ecx # str2
	push %ecx # restore stack 1/3
	push %edx # restore stack 2/3
	push %eax # restore stack 3/3
	push %ecx # parameter3 of mystricmp_ 
	push %edx # parameter2 of mystricmp_
	push %eax # parameter1 of mystricmp_
	call _Z10mystricmp_jPKcS0_@plt 
	add $12, %esp 
	ret

mystrncmp:
	sub $16, %esp 
	mov 16(%esp), %eax
	mov %eax, (%esp) # parameter1 of mystrncmp_
	mov 20(%esp), %eax
	mov %eax, 4(%esp) # parameter2 of mystrncmp_
	mov 24(%esp), %eax 
	mov %eax, 8(%esp) # parameter3 of mystrncmp_
	mov 28(%esp), %eax
	mov %eax, 12(%esp) # parameter4 of mystrncmp_
	call _Z10mystrncmp_jPKcS0_j@plt 
	add $16, %esp
	ret

.endif

