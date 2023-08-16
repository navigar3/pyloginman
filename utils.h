#ifndef _TERM_UTILS_H

#define _TERM_UTILS_H


void set_terminal_mode(void);
void switch_tty_and_wait(int, int);
int blocking_read_multibytes_from_term(char *);

#endif
