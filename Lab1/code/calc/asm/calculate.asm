; the nasm program to do add or mul the two big number

; the input format: <first_number><operator><second_number>
; the output format: <add_result> / <mul_result> / <invalid>

; the input prompt: 'please give the input: '
; the output prompt: 'the result is: '

; the steps:
; 1. print the prompt and read the input and judge if exit : done
; 2. parse the input to the two numbers and an operator : done
; 3. deal with the exception
; 4. do add : done
; 5. do mul : done
; 6. print the result : done
; 7. loop : done


; define the macro
%define SYS_READ 	3
%define SYS_WRITE 	4

%define STDIN 		0
%define STDOUT 		1
%define STDERR		2

%define N 		255

section .bss
	input	resb	N
	first_number	resb	N
	second_number	resb	N
	op	resb	1
	flag	resb 	1
	res	resb	N
	

section .data
	input_prompt	db	'please give the input: ', 0h
	output_prompt	db	'the result is: ', 0h
	operand_error	db	'operand_error: need two operand', 0h
	operator_error	db	'operator: the operator must be add or mul', 0h
	exit_sign	equ	'q'	

section .text
	global main


; the string is in the %eax
strlen:
	push ebx
	mov ebx, eax
	.loop:
	cmp byte[ebx], 0
	jz .end
	inc ebx
	jmp .loop
	.end:
	sub ebx, eax
	mov eax, ebx
	pop ebx
	ret
	
; the string is in the %eax
print_string:
	push eax
	push ebx
	push ecx
	push edx
	
	mov ecx, eax ; put string in %ecx
	call strlen
	mov edx, eax ; put the len in edx
	mov ebx, STDOUT
	mov eax, SYS_WRITE
	int 80h

	pop edx
	pop ecx
	pop ebx
	pop eax

	ret

; print the char
; char in %eax
print_char:
	push edx
	push ecx
	push ebx
	push eax

	mov eax, SYS_WRITE
	mov ebx, STDOUT
	mov ecx, esp 
	mov edx, 1
	int 80h

	pop eax
	pop ebx
	pop ecx
	pop edx
	
	ret


parse_input:
	push ebx
	push ecx
	push edx

	mov edx, 0 ; the flag, signify the parse phase
	mov ebx, esi ; the address of first_number put into %ebx
	
	.parse_loop:
	cmp byte[eax], 43 ; == '+'
	jz .set_add

	cmp byte[eax], 42 ; == '*'
	jz .set_mul

	cmp byte[eax], 10 ; == 'CL'
	jz .exit_parse
	
	jmp .parsing

	.set_add:
	mov byte[op], 43
	add edx, 1
	mov ebx, edi
	inc eax
	jmp .parse_loop

	.set_mul:
	mov byte[op], 42
	add edx, 1
	mov ebx, edi
	inc eax
	jmp .parse_loop

	.parsing:
	mov cl, byte[eax]
	mov byte[ebx], cl
	inc eax
	inc ebx

	jmp .parse_loop

	.exit_parse:
	add edx, 1
	mov eax, edx
	pop edx
	pop ecx
	pop ebx

	ret
; address put in the %eax
reset_memory:
	.reset_loop:
	cmp byte[eax], 0
	jz .reset_end
	mov byte[eax], 0
	inc eax
	jmp .reset_loop	
	
	.reset_end:
	ret

; first_operand in %esi
; second_operand in %edi
; res in %edx
add:
	push ecx
	push ebx
	push eax
	
	;jmp .add_exit
	
	add edx, N
	mov byte[edx], 0
	sub edx, 1
	
	mov eax, esi
	call strlen
	add esi, eax
	sub esi, 1
 
	mov eax, edi
	call strlen
	add edi, eax
	sub edi, 1

	mov cl, 0
	;jmp .add_exit
	.add_loop:
	cmp esi, first_number
	jl .first_number_end
	cmp edi, second_number
	jl .second_number_end
	; add two nums
	mov al, byte[esi]
	sub al, 48
	add al, byte[edi]
	add al, cl ; add the carry
	;jmp .add_exit
	.judge_overflow:
	cmp al, 57
	jg .overflow
	jmp .noflow

	.overflow:
	mov cl, 1
	sub al, 10
	mov byte[edx], al
	jmp .add_update_address

	.noflow:
	mov cl, 0
	mov byte[edx], al
	jmp .add_update_address

	.first_number_end:
	cmp edi, second_number
	jl .add_end
	; just second number
	mov al, byte[edi]
	add al, cl
	jmp .judge_overflow

	.second_number_end:
	cmp esi, first_number
	jl .add_end
	; just first number
	mov al, byte[esi]
	add al, cl
	jmp .judge_overflow

	.add_update_address:
	dec esi
	dec edi
	dec edx
	jmp .add_loop

	.add_highest_bit:
	mov byte[edx], 49
	jmp .add_exit

	.add_end:
	cmp cl, 1
	jz .add_highest_bit
	inc edx
	.add_exit:
	pop eax
	pop ebx
	pop ecx
	
	ret
	
; first_number in %esi
; second_number in %edi
; res in %edx
; the mul cmd => one operand in %al and another is designated by the '<mul> <register>'
; the res is in the %ax, the high 8 bit in the %ah while the low 8 bit in the %al
mul:
	push ecx
	push ebx
	push eax

	; put the ptr point to the end of res	
	add edx, N
	mov byte[edx], 0
	dec edx
	; put the ptr1 point to the end number of the number1
	mov eax, esi
	call strlen
	add esi, eax
	sub esi, 1
	; put the ptr2 point to the end number of the number2
	mov eax, edi
	call strlen
	add edi, eax
	sub edi, 1
	
	mov cl, 0 ; store the carry number
	
	jmp .mul_inner_loop
	
	.mul_outer_loop:
	dec edi ; update the second_number ptr
	cmp edi, second_number
	jl .mul_end ; if every number in numbers has multiple the number1, mul end

	; the ptr1 point to the end number of number1 again
	mov esi, first_number
	mov eax, esi
	call strlen
	add esi, eax
	sub esi, 1

	.update_res_ptr:
	
	mov edx, res
	add edx, N 
	sub edx, 1 ; firstly put the ptr point to the end of res

	mov eax, second_number
	mov ebx, second_number
	call strlen
	add ebx, eax
	sub ebx, 1
	sub ebx, edi ; secondly, calc the length of the ptr2 move

	sub edx, ebx ; update the res ptr: move the same length as the ptr2
	

	.mul_inner_loop:
	cmp esi, first_number
	jl .end_inner_loop ; the inner_loop end 

	mov al, byte[esi]
	sub al, 48

	mov bl, byte[edi]
	sub bl, 48

	; multiple the value in %al and the value in %bl => result is in the %ax (%ah + %al)
	; because the value in byte[esi] and byte[edi] is in [0, 9] so the result is in [0, 81], so the %ah must be 0
	mul bl
	add al, cl ; add the carry	

	; do div get the carry and the value in byte[edx]
	mov ah, 0
	mov bl, 10
	div bl
	; the div result is in %al and the remainer is in the %ah
	; the div remainer should be put in the byte[edx] and the result should be put in the byte[edx - 1]
	
	mov cl, al ; mov the carry into %cl
	add ah, byte[edx] ; attention: byte[edx] maybe has the value!

	; judge if overflow
	cmp ah, 9
	jg .overflow
	mov byte[edx], ah
	
	.update_ptr:
	dec esi
	dec edx
	jmp .mul_inner_loop

	.overflow:
	add cl, 1
	sub ah, 10
	mov byte[edx], ah
	jmp .update_ptr

	.end_inner_loop:
	mov byte[edx], cl ; maybe the carry != 0, so byte[edx] should add the value
	mov cl, 0 ; clear the carry
	jmp .mul_outer_loop

	.mul_end:
	cmp byte[edx], 0 ; judge the highest if == 0
	jnz .mul_exit
	inc edx
	.mul_exit:
	pop eax
	pop ebx
	pop ecx

	ret

; the len of res store in %eax
; the res store in edx
; convert the digit to char
deal_the_format:

	push ebx
	mov ebx, edx

	.format_loop:
	cmp eax, 0
	jz .format_end
	add byte[ebx], 48
	inc ebx
	sub eax, 1
	jmp .format_loop

	.format_end:
	pop ebx
	ret
	


main:
	.print_prompt:
		mov eax, input_prompt 
		call print_string

	.read_input:
		mov eax, SYS_READ
		mov ebx, STDIN
		mov ecx, input
		mov edx, N
		int 80h

	.jugde_exit:
		cmp byte[input], exit_sign
		jz .exit

	.parse_input:
		mov eax, input
		mov esi, first_number
		mov edi, second_number
		mov edx, res
		call parse_input
		
		cmp eax, 2
		jnz .execption		

		mov esi, first_number
		mov edi, second_number
		mov edx, res

		cmp byte[op], 43
		jz .add
		cmp byte[op], 42
		jz .mul

	.add:	
		call add

		jmp .print_res

		
	.mul:
		call mul
		
		; count the length of the res
		mov ebx, edx
		sub ebx, res
		mov eax, N
		sub  eax, ebx; the len in %eax
		call deal_the_format

		jmp .print_res
	
		
	

	.print_res:
		mov eax, output_prompt
		call print_string

		mov eax, edx
		call print_string

		mov eax, 0Ah
		call print_char
		
	.reset:
		mov eax, input
		call reset_memory
		mov eax, first_number
		call reset_memory
		mov eax, second_number
		call reset_memory
		mov eax, edx
		call reset_memory		
		jmp .print_prompt

	.exit:
		mov eax, 1
		mov ebx, 0
		int 80h

	.execption:
		mov eax, operand_error
		call print_string
		
		mov eax, 0Ah
		call print_char
		
		jmp .reset
