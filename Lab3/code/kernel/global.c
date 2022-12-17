
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
													Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

PUBLIC PROCESS proc_table[NR_TASKS + NR_PROCS];

PUBLIC TASK task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "tty"},
	{task_clear_screen, STACK_SIZE_CLEAR, "clear_screen"}
};

PUBLIC TASK user_proc_table[NR_PROCS] = {
	{TestA, STACK_SIZE_TESTA, "TestA"},
	{TestB, STACK_SIZE_TESTB, "TestB"},
	{TestC, STACK_SIZE_TESTC, "TestC"}};

PUBLIC char task_stack[STACK_SIZE_TOTAL];

PUBLIC TTY tty_table[NR_CONSOLES];
PUBLIC CONSOLE console_table[NR_CONSOLES];
PUBLIC C_BUF cbuf;
PUBLIC C_BUF* p_cbuf;

PUBLIC irq_handler irq_table[NR_IRQ];

PUBLIC system_call sys_call_table[NR_SYS_CALL] = {sys_get_ticks, sys_write};

PUBLIC int cur_mode; // 记录当前处于什么状态 用于搜索 INIT SEARCH MATCH
PUBLIC char match_str[MATCH_LEN]; // 记录搜索字符串
PUBLIC int match_str_idx;

PUBLIC ACT_LIST act_list;
PUBLIC ACT_LIST* p_act_list;
