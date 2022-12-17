
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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
                              schedule
调度策略：优先选择A进程，如果A进程在睡觉，则依次调度BCDEF其他五个进程
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p = proc_table;

	if(isRunnable(p)){ // 优先选择A
		p_proc_ready = p;
	} else {
		// 先去找刚睡醒的进程，尝试调度他们
		for (PROCESS* i = proc_table + 1; i < proc_table + NR_TASKS; i++){
			if (i->state == SLEEPING && i->wake_tick <= get_ticks()){
				i->state = RUNNING;
				i->run_after_sleep = 1;
				p_proc_ready = i;
				if(debug){
					print_int(p_proc_ready->ticks);
					print_str("w ");
				}
				return;
			}
		}
		// 再找到tick=0的进程 去释放资源
		for (PROCESS* i = proc_table + 1; i < proc_table + NR_TASKS; i++){
			if (i->ticks == 0){
				p_proc_ready = i;
				if(debug){
					print_int(p_proc_ready->ticks);
					print_str("t ");
				}
				return;
			}
		}
		// 调度waiting的进程 不用等信号量的进程
		for (PROCESS* i = proc_table + 1; i < proc_table + NR_TASKS; i++){
			if(i->state == WAITING){
				i->state = RUNNING;
				p_proc_ready = i;
				if (debug){
					print_int(p_proc_ready->ticks);
					print_str("n ");
				}
				return;
			}
		}
		// 都不存在按之前的顺序继续调度
		p = prev_proc + 1;
		while(!isRunnable(p)){
			p++;
			if(p >= proc_table + NR_TASKS){
				p = proc_table;
			}
		}
		p_proc_ready = p;
		prev_proc = p;
	}

	if(debug){
		print_int(p_proc_ready->ticks);
		print_str("s ");
	}

	p_proc_ready->state = RUNNING;
}

PUBLIC int isRunnable(PROCESS* p){
	if(p->wake_tick <= get_ticks() && p->state != BLOCKING){
		return 1;
	} else {
		return 0;
	}
}


/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_print_str(char* s){
	disp_str(s);
}

PUBLIC void sys_print_color_str(char* s, int color){
	disp_color_str(s, color);
}

PUBLIC void print_int(int n){
	if(n == 0) print_str("0");
	int len = 0;
	int tmp = n;
	while(tmp != 0){
		tmp /= 10;
		len ++;
	}
	char nums[10];
	nums[len] = '\0';
	int tmp0 = n;
	while(len > 0){
		nums[len-1] = (tmp0 % 10) - 0 + '0';
		tmp0 /= 10;
		len --;
	}
	print_str(nums);
}

PUBLIC void sys_sleep(int milli_seconds){
	// 设置该进程wake的时间
	p_proc_ready->state = SLEEPING;
	p_proc_ready->wake_tick = get_ticks() + milli_seconds / (1000 / HZ);
	// 重置进程时间片
	p_proc_ready->ticks = p_proc_ready->priority;
	schedule();
}

PUBLIC void sys_P(SEMAPHORE* s){
	disable_irq(CLOCK_IRQ);
	s->value--;
	if(s->value < 0){
		block(s);
	}
	enable_irq(CLOCK_IRQ);
}

PUBLIC void block(SEMAPHORE* s){
	s->waiting_queue[s->tail] = p_proc_ready;
	s->tail = (s->tail + 1) % WAITING_MAX;
	p_proc_ready->run_after_sleep = 0;
	p_proc_ready->state = BLOCKING;
	schedule();
}

PUBLIC void sys_V(SEMAPHORE* s){
	disable_irq(CLOCK_IRQ);
	s->value++;
	if(s->value <= 0){
		wake(s);
	}
	enable_irq(CLOCK_IRQ);
}

PUBLIC void wake(SEMAPHORE* s){
	s->waiting_queue[s->head]->state = WAITING;
	s->head = (s->head + 1) % WAITING_MAX;
}

PUBLIC void sys_do_schedule(){
	schedule();
}

PUBLIC void reader_first_r(){
	while(1){
		if(debug) begin();

		P(&r_cnt_mutex);
		if(count == 0){
			P(&rw_mutex);
		}
		count ++;
		V(&r_cnt_mutex);

		P(&r_mutex);
		// read
		reading();
		V(&r_mutex);

		P(&r_cnt_mutex);
		count --;
		if(count == 0){
			V(&rw_mutex);
		}
		V(&r_cnt_mutex);
		if(debug) end();

		sleep(p_proc_ready->sleep_time * 10);
	}
}

PUBLIC void reader_first_w(){
	while(1){
		if(debug) begin();
		P(&ww_mutex);
		P(&rw_mutex);

		// write
		writing();

		V(&rw_mutex);
		V(&ww_mutex);
		if(debug) end();

		sleep(p_proc_ready->sleep_time * 10);
	}
}

PUBLIC void writer_first_r(){
	while(1){

		if(debug) begin();

		P(&mutex);
		P(&r_mutex);
		P(&r_cnt_mutex);
		count++;
		if(count == 1){
			P(&rw_mutex);
		}
		V(&r_cnt_mutex);
		V(&mutex);

		// reading...
		reading();

		P(&r_cnt_mutex);
		count--;
		if(count == 0){
			V(&rw_mutex);
		}
		V(&r_cnt_mutex);
		V(&r_mutex);

		if(debug) end();
		sleep(p_proc_ready->sleep_time * 10);
	}
}

PUBLIC void writer_first_w(){
	while(1){
		if(debug) begin();

		P(&w_mutex);
		w_count++;
		if(w_count == 1){
			P(&mutex);
		}
		V(&w_mutex);

		P(&rw_mutex);
		// write...
		writing();

		V(&rw_mutex);

		P(&w_mutex);
		w_count--;
		if(w_count == 0){
			V(&mutex);
		}
		V(&w_mutex);
		if(debug) end();
		sleep(p_proc_ready->sleep_time * 10);
	}
}

PUBLIC void rw_equality_r(){
	while(1){
		if(debug) begin();

		P(&mutex);
		P(&r_mutex);
		P(&r_cnt_mutex);
		count++;
		if(count == 1){
			P(&rw_mutex);
		}
		V(&r_cnt_mutex);
		V(&mutex);

		// reading...
		reading();

		P(&r_cnt_mutex);
		count--;
		if(count == 0){
			V(&rw_mutex);
		}
		V(&r_cnt_mutex);
		V(&r_mutex);
		
		if(debug) end();
		sleep(p_proc_ready->sleep_time * 10);
	}
}

PUBLIC void rw_equality_w(){
	while(1){
		if(debug) begin();

		P(&mutex);
		P(&rw_mutex);

		// writing
		writing();

		V(&rw_mutex);
		V(&mutex);
		if(debug) end();
		sleep(p_proc_ready->sleep_time * 10);
	}
}

void begin(){
	print_str(p_proc_ready->p_name);
	print_str(" begin. ");
}

void reading(){
	while(p_proc_ready->ticks > 0){
		if(debug){
			print_str(p_proc_ready->p_name);
			print_str(" reading.");
		}
		if (p_proc_ready->run_after_sleep){
			p_proc_ready->run_after_sleep = 0;
			do_schedule();
		}
		if (p_proc_ready->ticks <= 0) break;
		milli_delay(10);
	}
}

void writing(){
	while(p_proc_ready->ticks > 0){
		if(debug){
			print_str(p_proc_ready->p_name);
			print_str(" writing. ");
		}
		if (p_proc_ready->run_after_sleep) {
			p_proc_ready->run_after_sleep = 0;
			do_schedule();
		}
		if (p_proc_ready->ticks <= 0) break;
		milli_delay(10);
	}
}

void end(){
	print_str(p_proc_ready->p_name);
	print_str(" endd. ");
}