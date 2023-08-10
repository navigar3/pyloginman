#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <string.h>
#include <stdlib.h>


//int drmDropMaster(int);
//int drmSetMaster(int);

int (* video_set_master_mode)() = NULL;
int (* video_drop_master_mode)() = NULL;


struct vt_mode orig_vtmode;
struct termios orig_termios;
char *curr_tty;
int curr_tty_num;

void restore_terminal_mode(void)
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  
  if (ioctl(STDIN_FILENO, VT_SETMODE, &orig_vtmode) == -1)
    fprintf(stderr, "ioctl() SETMODE failed!\n");
}

/* Set terminal parameters and disable Ctrl+Alt+Fn VT switch. */
void set_terminal_mode(void)
{
  /* Try to tty number. */
  curr_tty = ttyname(STDIN_FILENO);
  fprintf(stderr, "ttyname() returns %s\n", curr_tty);
  
  if (!memcmp(curr_tty, "/dev/tty", 8))
  {
    curr_tty_num = strtol(curr_tty+8, NULL, 10);
    fprintf(stderr, "ttynum = %d\n", curr_tty_num);
  }
  else
  {
    fprintf(stderr, "Error: could not determine tty number!\n");
    exit(1);
  }
  
  
  /* Store current VT parameters. */
  if (ioctl(STDIN_FILENO, VT_GETMODE, &orig_vtmode) == -1)
  {
    fprintf(stderr, "ioctl() VT_GETMODE failed!\n");
    exit(1);
  }
  
  /* Set call to function to restore original terminal params 
   * on exit. */
  atexit(restore_terminal_mode);
  
  
  /* Disable Ctrl+Alt+Fn Virtual Terminal Switch. */
  struct vt_mode new_vtmode = orig_vtmode;
  
  new_vtmode.mode = VT_PROCESS;
  
  if (ioctl(STDIN_FILENO, VT_SETMODE, &new_vtmode) == -1)
  {
    fprintf(stderr, "ioctl() VT_SETMODE failed!\n");
    exit(1);
  }
  fprintf(stderr, "ioctl() VT_SETMODE succeded.\n");
  
  
  /* Set new terminal parameters. */
  tcgetattr(STDIN_FILENO, &orig_termios);
  
  struct termios new_term_mode = orig_termios;
  
  new_term_mode.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  new_term_mode.c_oflag &= ~(OPOST);
  new_term_mode.c_cflag |= (CS8);
  new_term_mode.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);
  
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term_mode);
}


/* Read a multibyte sequence originated from buffered terminal
 * key stroke. Store in buff and return bytes read. */
int blocking_read_multibytes_from_term(char * buff)
{
    
  tcflush(STDIN_FILENO, TCIOFLUSH);
  
  read(STDIN_FILENO, buff, 1);
  
  int n;
  
  ioctl(STDIN_FILENO, FIONREAD, &n);
  
  if (n>0)
    read(STDIN_FILENO, buff+1, n);
  
  return n+1;
}


int get_curr_tty_num()
{
  return curr_tty_num;
}


/* Handle Virtual Terminal switch manually. 
 *  int to_tty: ttynumber to activate. */
void switch_tty_and_wait(int to_tty)
{
  struct vt_mode vtmode = orig_vtmode;
  
  if (to_tty == curr_tty_num)
    return;
  
  /* First, drop Direct Rendering Manager Master Mode in order
   * to allow other process to load their FrameBuffers. */
  if (video_drop_master_mode)
    video_drop_master_mode();
  
  /* Restore VT Auto switch handler. */
  vtmode.mode = VT_AUTO;
  
  if (ioctl(STDIN_FILENO, VT_SETMODE, &vtmode) == -1)
  {
    fprintf(stderr, "ioctl() VT_SETMODE failed!\n");
    exit(1);
  }
  
  
  /* Switch to the requested ttynum. */
  if (ioctl(STDIN_FILENO, VT_ACTIVATE, to_tty) == -1)
  {
    fprintf(stderr, "ioctl() VT_ACTIVATE failed!\n");
    goto own_tty;
  }
  
  //fprintf(stderr, "Launching ioctl() VT_WAITACTIVE 1 \n");
  
  if (ioctl(STDIN_FILENO, VT_WAITACTIVE, to_tty) == -1)
  {
    fprintf(stderr, "ioctl() VT_WAITACTIVE %d failed!\n", to_tty);
  }
  
  //fprintf(stderr, "Launched ioctl() VT_WAITACTIVE 1 \n");
  

own_tty:
  /* Disable Ctrl+Alt+Fn again. */
  vtmode.mode = VT_PROCESS;
  
  if (ioctl(STDIN_FILENO, VT_SETMODE, &vtmode) == -1)
  {
    fprintf(stderr, "ioctl() VT_SETMODE failed!\n");
    exit(1);
  }
  
  //fprintf(stderr, "Launching ioctl() VT_WAITACTIVE 2 \n");
  
  /* Wait until this tty become active again. */
  if (ioctl(STDIN_FILENO, VT_WAITACTIVE, curr_tty_num) == -1)
  {
    fprintf(stderr, "ioctl() VT_WAITACTIVE %d failed!\n", 2);
    exit(1);
  }
  
  //fprintf(stderr, "Launched ioctl() VT_WAITACTIVE 2 \n");
  
  /* Set DRM Master Mode in order to allow rendering operations. */
  if (video_set_master_mode)
    video_set_master_mode();
}
