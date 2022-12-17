
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
#include "global.h"

PUBLIC void init_rw(){
	rw_mutex.value = 1; rw_mutex.head = rw_mutex.tail = 0;
	r_cnt_mutex.value = 1; r_cnt_mutex.head = r_cnt_mutex.tail = 0;
	r_mutex.value = READER_MAX; r_mutex.head = r_mutex.tail = 0;
	w_mutex.value = 1; w_mutex.head = w_mutex.tail = 0;
	mutex.value = 1; mutex.head = mutex.tail = 0;
	ww_mutex.value = 1; ww_mutex.head = ww_mutex.tail = 0;
	
	count = 0; // 排队读的读者数量
	w_count = 0;
	strategy = WRITER_FIRST; // 读写策略
	debug = 1;
}

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");
	// 清屏
	disp_pos = 0;
	for(int i = 0; i < 80 * 25; i++){
		disp_str(" ");
	}
	disp_pos = 0;

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority =  1; // 占用一个时间片进行输出
	proc_table[1].ticks = proc_table[1].priority =  2;
	proc_table[2].ticks = proc_table[2].priority =  3;
	proc_table[3].ticks = proc_table[3].priority =  3;
	proc_table[4].ticks = proc_table[4].priority =  3;
	proc_table[5].ticks = proc_table[5].priority =  4;


	for(PROCESS * p = proc_table; p < proc_table + NR_TASKS; p++){
		p->state = WAITING;
		p->run_after_sleep = 0;
	}

	// 设定每个进程的类型
	proc_table[0].type = COMMON;
	proc_table[1].type = proc_table[2].type = proc_table[3].type = READER;
	proc_table[4].type = proc_table[5].type = WRITER;

	// 设定每个进程的休息时间
	proc_table[1].sleep_time = SLEEPING_TIME + 1;
	proc_table[2].sleep_time = SLEEPING_TIME;
	proc_table[3].sleep_time = SLEEPING_TIME + 2;
	proc_table[4].sleep_time = SLEEPING_TIME + 1;
	proc_table[5].sleep_time = SLEEPING_TIME;

	init_rw();

	k_reenter = 0;
	ticks = 0;

	if(debug){
		print_int(ticks);
		print_str(" ");
	}

	p_proc_ready	= proc_table;
	prev_proc = p_proc_ready;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	restart();

	while(1){
	}
}

/*======================================================================*
                               普通进程A
 *======================================================================*/
int cnt = 1;
void CommonA()
{
	while (1) {
		int t = get_ticks();
		print_int(cnt);
		// 对齐
		if (cnt < 10){
			print_str(" ");
		}
		PROCESS* p = proc_table + 1;
		for(; p < proc_table + NR_TASKS; p++){
			switch(p->state){
				case BLOCKING:
					// disp_color_str(" B", WHITE);
					// break;
				case WAITING:
					print_color_str(" X", RED);
					break;
				case RUNNING:
					print_color_str(" O", GREEN);
					break;
				case SLEEPING:
					print_color_str(" Z", BLUE);
					break;
			}
		}
		if(!debug) print_str("\n");
		else print_str(" ");
		cnt++;
		if(!debug && cnt > 25){
			disable_irq(CLOCK_IRQ);
			while(1){}
		}
		sleep(10);
	}
}

/*======================================================================*
                               读者进程B
 *======================================================================*/
void ReadB()
{
	readers[strategy]();
}

/*======================================================================*
                               读者进程C
 *======================================================================*/
void ReadC()
{
	readers[strategy]();
}
/*======================================================================*
                               读者进程D
 *======================================================================*/
void ReadD(){
	readers[strategy]();
}

/*======================================================================*
                               写者进程E
 *======================================================================*/
void WriteE(){
	writers[strategy]();
}
/*======================================================================*
                               写者进程F
 *======================================================================*/
void WriteF(){
	writers[strategy]();
}
