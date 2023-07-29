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
  
  CALL(dv, setup_monitor, 0);
  CALL(dv, enable_monitor, 0);
  
  monitor * mon0 = CALL(dv, get_monitor_by_ID, 0);
  if (!mon0)
    goto main_quit;
  
  CALL(dv, setup_monitor, 1);
  monitor * mon1 = CALL(dv, get_monitor_by_ID, 1);
  if (!mon1)
    goto main_quit;
  
  fprintf(stderr, "TEST: width=%d, height=%d\n",
                  CALL(mon1, get_monitor_width), 
                  CALL(mon1, get_monitor_height));
  CALL(mon1, destroy);
  
  CALL(dv, load_font_from_file, "fontsgen/fonts2.baf");
  
  videoterm * vt0 = CALL(mon0, videoterminal, 0);
  
  if (!vt0)
    goto main_quit;
  
  CALL(mon0, draw_rectangle, 0, 0, 1919, 1079, 0x0000aaaa);
  
  sleep(1);
  
  CALL(mon0, draw_rectangle, 500, 300, 800, 600, 0xff << 8);
  
  sleep(1);
  
  CALL(mon0, draw_rectangle, 0, 0, 32, 65, 0xaa << 14);
  
  sleep(1);
  
  CALL(mon0, draw_rectangle, 32, 0, 32, 65, 0xff << 14);
  
  sleep(1);
  
  CALL(vt0, putc_advance_and_sync, 1, 1, "g");
  
  for (int k=0; k<450; k++)
  {
    uint8_t c = (k % 93) + 33;
    CALL(vt0, putc_advance_and_sync, 0, 1, (char *)(&c));
  }
  
  CALL(vt0, putc_advance_and_sync, 1, 1, "p");
  
  sleep(2);
  
  CALL(mon0, draw_rectangle, 200, 500, 400, 300, 0x000000dd); /* Blue */
  
  sleep(2);


main_quit:
  
  CALL(dv, destroy);
  
  fprintf(stderr, "Going out...\n");
  
  
  return 0;
}
