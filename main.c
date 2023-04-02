#include <stdio.h>
#include <unistd.h>

#include "drm-splash4slack.h"


int main(int argc, char *argv[])
{
  int ret;
  char *card = "/dev/dri/card0";
  
  // Init hw
  ret = drm_init(card);
  if (ret)
    return ret;
  
  // Draw
  modeset_draw(drm_fd);
  
  //Cleanup and close
  modeset_cleanup(drm_fd);
  close(drm_fd);
  
  return ret;
}
