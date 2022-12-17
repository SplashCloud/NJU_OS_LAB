
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                           tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                                                                        Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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


PRIVATE void init_char_buf();
PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

PRIVATE void init_search();
PRIVATE void exit_search(CONSOLE* p_con);
PRIVATE void do_match(CONSOLE* p_con);

PRIVATE void init_act_list();

/*======================================================================*
                                                   task_tty
 *======================================================================*/
PUBLIC void task_tty() {
    TTY* p_tty;

    init_keyboard();

    for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
        init_tty(p_tty);
    }

    init_char_buf();
    init_act_list();
    cur_mode = INIT;

    select_console(0);
    while (1) {
        for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) {
            tty_do_read(p_tty);
            tty_do_write(p_tty);
        }
    }
}

/*======================================================================*
                           init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty) {
    p_tty->inbuf_count = 0;
    p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

    init_screen(p_tty);
}
/*======================================================================*
                           init_char_buff
 *======================================================================*/
PRIVATE void init_char_buf() {
    p_cbuf = &cbuf;
    p_cbuf->buf_cur_idx = 0;
}
/*======================================================================*
                           init_act_list
 *======================================================================*/
PRIVATE void init_act_list(){
    p_act_list = &act_list;
    p_act_list->idx = 0;
}
/*======================================================================*
                           init_search
 *======================================================================*/
PRIVATE void init_search(){
    match_str_idx = 0;
}
/*======================================================================*
                           exit_search
 *======================================================================*/
PRIVATE void exit_search(CONSOLE* p_con){
    // 删除搜索字符串
    for(int i = 0; i < match_str_idx; i++){
        out_char(p_con, '\b');
    }
    // 改变字体颜色
    int len = p_cbuf->buf_cur_idx;
    for(int i = 0; i < len; i++){
        if(p_cbuf->buf[i].color == RED_IN_BLACK){
            p_cbuf->buf[i].color = DEFAULT_CHAR_COLOR;
        }
    }
    // 从头显示
    p_con->cursor = p_con->original_addr;
    p_cbuf->buf_cur_idx = 0;
    for(int i = 0; i < len; i++){
        out_char(p_con, p_cbuf->buf[i].val);
    }
}
/*======================================================================*
                           do_match
 *======================================================================*/
PRIVATE void do_match(CONSOLE* p_con){
    int i = 0; int len = p_cbuf->buf_cur_idx;
    int len0 = len - match_str_idx;
    while(i < len0){
        int k = i; int j = 0;
        // 匹配搜索字符串
        for(; j < match_str_idx; j++){
            if(p_cbuf->buf[i].val != match_str[j]){
                break;
            } else {
                i++;
                if(i >= len0 && j != match_str_idx-1) break;
            }
        }
        if(j == match_str_idx){
            // 改颜色
            for(int m = k; m < i; m++){
                p_cbuf->buf[m].color = RED_IN_BLACK;
            }
            i = k + 1;
        } else {
            i = k + 1;
        }
    }
    // 重新输出
    p_con->cursor = p_con->original_addr;
    p_cbuf->buf_cur_idx = 0;
    for(int t = 0; t < len; t++){
        out_char(p_con, p_cbuf->buf[t].val);
    }
}
/*======================================================================*
                                in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key) {
    // 如果处于匹配阶段 不处理除esc外任何输入
    if(cur_mode == MATCH){
        if((key & MASK_RAW) == ESC){
            exit_search(p_tty->p_console);
            cur_mode = INIT;
        }
        return;
    }
    char output[2] = {'\0', '\0'};
    // 可显示字符
    if (!(key & FLAG_EXT)) {
        char c = key;
        if( c == 'z' ){
            // 按下 Ctrl + z
            if((key & FLAG_CTRL_L) || (key & FLAG_CTRL_R)){
                if(p_act_list->idx > 0){
                    ACTION last_action = p_act_list->list[p_act_list->idx-1];
                    if(last_action.this.val == '\b'){ // 如果上一个动作是退格
                        out_char(p_tty->p_console, last_action.relate.val);
                    } else { // 如果上一个动作是显示
                        out_char(p_tty->p_console, '\b');
                    }
                    p_act_list->idx --;
                }
                return;
            }
        }
        put_key(p_tty, key);
    } else {  // 不可显示字符
        int raw_code = key & MASK_RAW;
        switch (raw_code) {
            case ENTER:
                if(cur_mode == SEARCH){
                    cur_mode = MATCH;
                    do_match(p_tty->p_console);
                } else {
                    put_key(p_tty, '\n');
                }
                break;
            case BACKSPACE:
                put_key(p_tty, '\b');
                break;
            case TAB:
                put_key(p_tty, '\t');
                break;
            case UP:
                if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                    scroll_screen(p_tty->p_console, SCR_DN);
                }
                break;
            case DOWN:
                if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
                    scroll_screen(p_tty->p_console, SCR_UP);
                }
                break;
            case ESC:
                if(cur_mode == INIT) {
                    cur_mode = SEARCH;
                    init_search();
                }
                break;
            case F1:
            case F2:
            case F3:
            case F4:
            case F5:
            case F6:
            case F7:
            case F8:
            case F9:
            case F10:
            case F11:
            case F12:
                /* Alt + F1~F12 */
                if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
                    select_console(raw_code - F1);
                }
                break;
            default:
                break;
        }
    }
}

/*======================================================================*
                                  put_key
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key) {
    if (p_tty->inbuf_count < TTY_IN_BYTES) {
        *(p_tty->p_inbuf_head) = key;
        p_tty->p_inbuf_head++;
        if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
            p_tty->p_inbuf_head = p_tty->in_buf;
        }
        p_tty->inbuf_count++;
    }
}

/*======================================================================*
                                  tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty) {
    if (is_current_console(p_tty->p_console)) {
        keyboard_read(p_tty);
    }
}

/*======================================================================*
                                  tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty) {
    if (p_tty->inbuf_count) {
        char ch = *(p_tty->p_inbuf_tail);
        p_tty->p_inbuf_tail++;
        if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
            p_tty->p_inbuf_tail = p_tty->in_buf;
        }
        p_tty->inbuf_count--;

        out_char(p_tty->p_console, ch);

        // 向ACTION列表中增加ACTION
        ACTION action;
        if(ch == '\b'){
            action.this = p_cbuf->buf[p_cbuf->buf_cur_idx+1];
            action.relate = p_cbuf->buf[p_cbuf->buf_cur_idx];
        } else {
            action.this = p_cbuf->buf[p_cbuf->buf_cur_idx-1];
        }
        p_act_list->list[p_act_list->idx] = action;
        p_act_list->idx++;
    }
}

/*======================================================================*
                                                          tty_write
*======================================================================*/
PUBLIC void tty_write(TTY* p_tty, char* buf, int len) {
    char* p = buf;
    int i = len;

    while (i) {
        out_char(p_tty->p_console, *p++);
        i--;
    }
}

/*======================================================================*
                                                          sys_write
*======================================================================*/
PUBLIC int sys_write(char* buf, int len, PROCESS* p_proc) {
    tty_write(&tty_table[p_proc->nr_tty], buf, len);
    return 0;
}
