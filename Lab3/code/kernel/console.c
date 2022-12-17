
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                  console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                        Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
        回车键: 把光标移到第一列
        换行键: 把光标前进到下一行
*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PRIVATE void clean_screen();
/*======================================================================*
                           task_clear
                                            每隔20s清空屏幕的任务
 *======================================================================*/
PUBLIC void task_clear_screen(){
    while(1){
        if(cur_mode == INIT)
            clean_screen();
        milli_delay(50000);
    }
}
/*======================================================================*
                           clean_screen
 *======================================================================*/
PRIVATE void clean_screen(){
    TTY* p_tty;
    for(p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++){
        if(is_current_console(p_tty->p_console)){
            while(p_tty->p_console->cursor != p_tty->p_console->original_addr){
                out_char(p_tty->p_console, '\b');
            }
            p_cbuf->buf_cur_idx = 0; // 清空显示字符buf
            p_act_list->idx = 0; // 清空动作列表
        }
    }


}

/*======================================================================*
                           init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty) {
    int nr_tty = p_tty - tty_table;
    p_tty->p_console = console_table + nr_tty;

    int v_mem_size = V_MEM_SIZE >> 1; /* 显存总大小 (in WORD) */

    int con_v_mem_size = v_mem_size / NR_CONSOLES;
    p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
    p_tty->p_console->v_mem_limit = con_v_mem_size;
    p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

    /* 默认光标位置在最开始处 */
    p_tty->p_console->cursor = p_tty->p_console->original_addr;

    if (nr_tty == 0) {
        /* 第一个控制台沿用原来的光标位置 */
        p_tty->p_console->cursor = disp_pos / 2;
        disp_pos = 0;
    } else {
        out_char(p_tty->p_console, nr_tty + '0');
        out_char(p_tty->p_console, '#');
    }

    set_cursor(p_tty->p_console->cursor);
}

/*======================================================================*
                           is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con) {
    return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
                           disp_console_vals
                                                调试使用
 *======================================================================*/
PUBLIC void disp_console_vals(CONSOLE* p_con) {
    disp_color_str("current_start_addr: ", 0x0C);
    disp_int(p_con->current_start_addr);
    disp_str("                         \n");
    disp_color_str("original_addr: ", 0x0C);
    disp_int(p_con->original_addr);
    disp_str("                         \n");
    disp_color_str("v_mem_limit: ", 0x0C);
    disp_int(p_con->v_mem_limit);
    disp_str("                         \n");
    disp_color_str("cursor: ", 0x0C);
    disp_int(p_con->cursor);
    disp_str("                         \n");
}

/*======================================================================*
                           out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch) {

    // 匹配状态下调用out_char不要将字符加入C_BUF的buf中
    if(cur_mode != MATCH){
        CHAR c;
        c.val = ch; 
        c.color = cur_mode == INIT ? DEFAULT_CHAR_COLOR : RED_IN_BLACK;
        p_cbuf->buf[p_cbuf->buf_cur_idx] = c;
    }

    // 搜索状态要将搜索字符串记录下来
    if(cur_mode == SEARCH){
        if(ch == '\b'){
            if(match_str_idx > 0) match_str_idx--;
            else return;
        } else {
            match_str[match_str_idx++] = ch;
        }
    }

    u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

    switch (ch) {
        case '\n':
            // 当前cursor所在位置需要距离结束位置多于80个字符，多出一行可以换行
            if (p_con->cursor < p_con->original_addr +
                                    p_con->v_mem_limit - SCREEN_WIDTH) {
                p_cbuf->buf[p_cbuf->buf_cur_idx].before_cursor = p_con->cursor; // 显示该字符前的光标位置
                p_con->cursor = p_con->original_addr + SCREEN_WIDTH *
                                                           ((p_con->cursor - p_con->original_addr) /
                                                                SCREEN_WIDTH +
                                                            1);
                p_cbuf->buf[p_cbuf->buf_cur_idx].after_cursor = p_con->cursor; // 显示该字符后的光标位置
            }
            break;
        case '\b':
            // 光标位置需要在多于起始位置
            if (p_con->cursor > p_con->original_addr) {
				if(p_cbuf->buf_cur_idx > 0)
                	p_con->cursor = p_cbuf->buf[p_cbuf->buf_cur_idx-1].before_cursor;
                else
					p_con->cursor--;
				*(p_vmem - 2) = ' ';
                *(p_vmem - 1) = DEFAULT_CHAR_COLOR;
            }
            break;
        case '\t':
            if (p_con->cursor <
                p_con->original_addr + p_con->v_mem_limit - 1) {
                p_cbuf->buf[p_cbuf->buf_cur_idx].before_cursor = p_con->cursor;
                for (int i = 0; i < TAB_SIZE; i++) {
                    *p_vmem++ = ' ';
                    *p_vmem++ = p_cbuf->buf[p_cbuf->buf_cur_idx].color;
                    p_con->cursor++;
                }
                p_cbuf->buf[p_cbuf->buf_cur_idx].after_cursor = p_con->cursor;
            }
            break;
        default:
            if (p_con->cursor <
                p_con->original_addr + p_con->v_mem_limit - 1) {
                p_cbuf->buf[p_cbuf->buf_cur_idx].before_cursor = p_con->cursor;
                *p_vmem++ = ch;
                *p_vmem++ = p_cbuf->buf[p_cbuf->buf_cur_idx].color;
                p_con->cursor++;
                p_cbuf->buf[p_cbuf->buf_cur_idx].after_cursor = p_con->cursor;
            }
            break;
    }

    if (ch == '\b') {
        if(p_cbuf->buf_cur_idx > 0)
            p_cbuf->buf_cur_idx --;
    } else {
        p_cbuf->buf_cur_idx ++;
    }

    while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
        scroll_screen(p_con, SCR_DN);
    }

    flush(p_con);
}

/*======================================================================*
                                                   flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con) {
    if (is_current_console(p_con)) {
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
    }
}

/*======================================================================*
                                set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position) {
    disable_int();
    out_byte(CRTC_ADDR_REG, CURSOR_H);
    out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, CURSOR_L);
    out_byte(CRTC_DATA_REG, position & 0xFF);
    enable_int();
}

/*======================================================================*
                          set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr) {
    disable_int();
    out_byte(CRTC_ADDR_REG, START_ADDR_H);
    out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, START_ADDR_L);
    out_byte(CRTC_DATA_REG, addr & 0xFF);
    enable_int();
}

/*======================================================================*
                           select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console) /* 0 ~ (NR_CONSOLES - 1) */
{
    if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
        return;
    }

    nr_current_console = nr_console;

    flush(&console_table[nr_console]);
}

/*======================================================================*
                           scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
        SCR_UP	: 向上滚屏
        SCR_DN	: 向下滚屏
        其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction) {
    if (direction == SCR_UP) {
        if (p_con->current_start_addr > p_con->original_addr) {
            p_con->current_start_addr -= SCREEN_WIDTH;
        }
    } else if (direction == SCR_DN) {
        if (p_con->current_start_addr + SCREEN_SIZE <
            p_con->original_addr + p_con->v_mem_limit) {
            p_con->current_start_addr += SCREEN_WIDTH;
        }
    } else {
    }

    flush(p_con);
}
