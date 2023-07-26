#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "cobj.h"
#include "drm-doublebuff.h"

int main(int argc, char * argv[])
{
  if (argc < 2)
  {
    fprintf(stderr, "Which card?\n");
    return 1;
  }
  
  drmvideo * dv = new(drmvideo, argv[1]);
  
  if (!dv)
  {
    fprintf(stderr, "Something wrong!\n");
    return 1;
  }
  
  CALL(dv, modeset_prepare_monitor, 0);
  CALL(dv, enable_monitor, 0);
  
  sleep(2);
  
  fprintf(stderr, "Going out...\n");
  
  return 0;
}
