
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                tty.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                        Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_TTY_H_
#define _ORANGES_TTY_H_

#define TTY_IN_BYTES 256 /* tty input queue size */
#define BYTES_IN_BUF 0x1555
#define MAX_ACTION   1000

#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

struct s_console;

/* TTY */
typedef struct s_tty {
    u32 in_buf[TTY_IN_BYTES]; /* TTY 输入缓冲区 */
    u32* p_inbuf_head;        /* 指向缓冲区中下一个空闲位置 */
    u32* p_inbuf_tail;        /* 指向键盘任务应处理的键值 */
    int inbuf_count;          /* 缓冲区中已经填充了多少 */

    struct s_console* p_console;
} TTY;

typedef struct char_in_screen {
    char val; // 字符的值
    char color; // 字符颜色
    unsigned int before_cursor; // 输出该字符前的光标位置
    unsigned int after_cursor; // 输出该字符后的光标位置
} CHAR;

typedef struct char_buf {
    CHAR buf[BYTES_IN_BUF];  // 存储当前所有显示的字符
    int buf_cur_idx;         // 指向当前显示的字符
} C_BUF;

typedef struct action {
    CHAR this;
    CHAR relate;
} ACTION;

typedef struct actoin_list{
    ACTION list[MAX_ACTION];
    int idx;
} ACT_LIST;

#endif /* _ORANGES_TTY_H_ */
