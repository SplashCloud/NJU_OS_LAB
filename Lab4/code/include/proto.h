
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void CommonA();
void ReadB();
void ReadC();
void ReadD();
void WriteE();
void WriteF();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);


/* 以下是系统调用相关 */

/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
PUBLIC  void    sys_print_str(char* s);
PUBLIC  void    sys_print_color_str(char* s, int color);
PUBLIC  void    sys_sleep(int milli_seconds);
PUBLIC  void    sys_P(SEMAPHORE* s);
PUBLIC  void    sys_V(SEMAPHORE* s);
PUBLIC  void    sys_do_schedule();


PUBLIC  void    print_int(int n);

PUBLIC  void    begin();
PUBLIC  void    end();
PUBLIC  void    reading();
PUBLIC  void    writing();

PUBLIC  void    reader_first_r();
PUBLIC  void    reader_first_w();
PUBLIC  void    writer_first_r();
PUBLIC  void    writer_first_w();
PUBLIC  void    rw_equality_r();
PUBLIC  void    rw_equality_w();

/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();
PUBLIC  void    print_str(char* s);
PUBLIC  void    print_color_str(char* s, int color);
PUBLIC  void    sleep(int milli_seconds);
PUBLIC  void    P(SEMAPHORE* s);
PUBLIC  void    V(SEMAPHORE* s);
PUBLIC  void    do_schedule();

