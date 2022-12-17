
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int		ticks;

EXTERN	int		disp_pos;
EXTERN	u8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32		k_reenter;

EXTERN	TSS		tss;
EXTERN	PROCESS*	p_proc_ready;

extern	PROCESS		proc_table[];
extern	char		task_stack[];
extern  TASK            task_table[];
extern	irq_handler	irq_table[];

// semaphore
SEMAPHORE rw_mutex; // 读写、写写互斥量
SEMAPHORE r_cnt_mutex; // 控制count的使用
SEMAPHORE r_mutex; // 控制读者个数
SEMAPHORE w_mutex; // 控制w_count的使用
SEMAPHORE mutex; // 写者优先 在有写者时限制读者
SEMAPHORE ww_mutex; // 使得等待的写进程不直接在rw_mutex上排队
int count;
int w_count;
PROCESS* prev_proc;

int strategy; // 选择的读写策略

int debug;

extern  reader_func readers[];
extern  writer_func writers[];