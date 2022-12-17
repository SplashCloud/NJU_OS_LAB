
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "global.h"


PUBLIC	PROCESS			proc_table[NR_TASKS];

PUBLIC	char			task_stack[STACK_SIZE_TOTAL];

PUBLIC	TASK	task_table[NR_TASKS] = {
    {CommonA, STACK_SIZE_COMMONA, "CommonA"},
	{ReadB, STACK_SIZE_READB, "ReadB"},
	{ReadC, STACK_SIZE_READC, "ReadC"},
    {ReadD, STACK_SIZE_READD, "ReadD"},
    {WriteE, STACK_SIZE_WRITEE, "WriteE"},
    {WriteF, STACK_SIZE_WRITEF, "WriteF"},
};

PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {
                sys_get_ticks,
                sys_print_str,
                sys_print_color_str,
                sys_sleep,
                sys_P,
                sys_V,
                sys_do_schedule
    };


PUBLIC reader_func readers[STRATEGY] = {
    reader_first_r,
    writer_first_r,
    rw_equality_r
};

PUBLIC writer_func writers[STRATEGY] = {
    reader_first_w,
    writer_first_w,
    rw_equality_w
};
