
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_print_str		equ	1
_NR_print_color_str equ 2
_NR_sleep			equ	3
_NR_P				equ	4
_NR_V				equ	5
_NR_do_schedule		equ	6
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global 	print_str
global print_color_str
global 	sleep
global	P
global	V
global do_schedule
bits 32
[section .text]

; ====================================================================
;                              get_ticks
;										获得当前总共发生了多少次时钟中断
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

print_str:
	mov eax, _NR_print_str
	mov ebx, [esp + 4] ; char * str
	int INT_VECTOR_SYS_CALL
	ret

print_color_str:
	mov eax, _NR_print_color_str
	mov ebx, [esp + 4] ; char * str
	mov ecx, [esp + 8] ; int color
	int INT_VECTOR_SYS_CALL
	ret

sleep:
	mov eax, _NR_sleep
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

P:
	mov eax, _NR_P
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret
V:
	mov eax, _NR_V
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	ret
do_schedule:
	mov	eax, _NR_do_schedule
	int	INT_VECTOR_SYS_CALL
	ret