#include <stdio.h>
#include <unistd.h>

int open_tty(char * tty);

void set_terminal_mode(void);
char * get_curr_tty_name();
int blocking_read_multibytes_from_term(char *);

int main(int argc, char * argv[])
{
  int i = 0;
  char buff[10];
  int r;
  
  fprintf(stderr, "main(): Opening tty %s\n", argv[2]);
  fprintf(stderr, "open_tty() returns %d\n", open_tty(argv[2]));

  fprintf(stderr, "%s\n", get_curr_tty_name());
  set_terminal_mode();
  
  for (i=0; i<10; i++)
  {
    r = blocking_read_multibytes_from_term(buff);
    fprintf(stderr, " cicle %d: got %d bytes, first %c\n", i, r, buff[0]);
  }
  
  fprintf(stderr, "Bye!\n");

  return 0;
}
