global print

section .text


# print(char * s, int len);
# [ebp+8] => s
# [ebp+12] => len
print:
	push ebp
	mov	 ebp, esp
	mov  edx, [ebp+12]
	mov  ecx, [ebp+8]
	mov  ebx, 1
	mov  eax, 4 
	int  80h
	pop  ebp
	ret
