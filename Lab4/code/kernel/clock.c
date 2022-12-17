
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               clock.c
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


/*======================================================================*
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	if(debug) print_str("\n");
	ticks++; // 时钟中断次数+1
	if(debug){
		print_int(ticks);
		print_str(" ");
	}
	// p_proc_ready->ticks--; // 该进程剩余时间片-1
	for(PROCESS* p = proc_table + 1; p < proc_table + NR_TASKS; p++){
		// 所有在运行的进程时间片--
		if(p->state == RUNNING){
			p->ticks--;
		}
	}

	// if (k_reenter != 0) { // 上一个中断未处理完成之前又发生了一次中断
	// 	return;
	// }

	// if (p_proc_ready->ticks > 0) { // 如果当前进程还有剩余时间片，则继续执行
	// 	return;
	// }

	schedule(); // 调度进程

}

/*======================================================================*
                              milli_delay
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / HZ) < milli_sec) {}
}

