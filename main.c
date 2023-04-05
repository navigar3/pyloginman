#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include "drm-splash4slack.h"
#include "kbd-handler.h"


int main(int argc, char *argv[])
{
  int ret;
  char *card = "/dev/dri/card0";

  
  // Set Terminal mode
  set_terminal_mode();
  
  // Init graphical hw
  ret = drm_init(card);
  if (ret)
    return ret;
  
  // Loop
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    // Draw
    //modeset_draw(drm_fd);
    
    if (iscntrl(c)) {
      printf("%d\n", c);
    } else {
      printf("%d ('%c')\n", c, c);
      if (c == 'y')
        switch_tty_and_wait(drm_fd, 1);
    }
    
    modeset_draw_once(drm_fd);
    
  }
  
  //Cleanup and close
  modeset_cleanup(drm_fd);
  close(drm_fd);
  
  return ret;
}
