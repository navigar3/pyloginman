#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include "cobj.h"

#include "drm-splash4slack.h"
#include "kbd-handler.h"

#include "tools/hashtable.h"

struct glyph_s 
{
  uint32_t offx;
  uint32_t offy;
  uint32_t gw;
  uint32_t gh;
};

int main(int argc, char *argv[])
{
  //int ret;
  //char *card = "/dev/dri/card0";

  
  // Set Terminal mode
  set_terminal_mode();
  
  // Init graphical hw
  //ret = drm_init(card);
  //if (ret)
  //  return ret;
  
  /* open fonts file */
  htables * fonts = new(htables, NULL);
  if (CALL(fonts, loadtables, "fontsgen/fonts2.baf"))
  {
    fprintf(stderr, "Cannot load fonts!\r\n");
    return 1;
  }
  
  printf("Num of tables=%d\r\n", CALL(fonts, get_num_of_tables));
  
  hashtable * glyphs = CALL(fonts, lookup, ".glyphs");
  if (!glyphs)
  {
    printf("table .glyphs not found!\n");
    return 1;
  }
  
  hashtable * metrics = CALL(fonts, lookup, ".metrics");
  if (!metrics)
  {
    printf("table .metrics not found!\n");
    return 1;
  }
  
  struct lookup_res metrics_res;
  CALL(metrics, lookup, 8, "boxwidth", NULL, &metrics_res);
  uint32_t width = *((uint32_t *)metrics_res.content);
  printf("width = %d\r\n", width);
  CALL(metrics, lookup, 9, "boxheight", NULL, &metrics_res);
  uint32_t height = *((uint32_t *)metrics_res.content);
  printf("height = %d\r\n", height);
  
  // Loop
  while (1) {
    char widech[10];
    
    int n = blocking_read_multibytes_from_term(widech);
    
    if ((*widech=='q') && (n==1)) break;
    
    *(widech+n) = '\0';
    
    
    
    // Draw
    //modeset_draw(drm_fd);
    
    if (iscntrl(*widech)) {
      printf("Not printable %s\r\n", widech);
    } else {
      printf("Printable %s\r\n", widech);
      struct lookup_res glyph_res;
      if(CALL(glyphs, lookup, n, widech, NULL, &glyph_res))
        printf("glyph is not indexed!\r\n");
      else
      {
        struct glyph_s * glyph;
        glyph = (struct glyph_s *)glyph_res.content;
        printf("glyph offx = %d\r\n", glyph->offx);
        printf("glyph offy = %d\r\n", glyph->offy);
        printf("glyph gw = %d\r\n", glyph->gw);
        printf("glyph gh = %d\r\n", glyph->gh);
        
        char * px = ((char *)glyph) + sizeof(struct glyph_s);
        char sw = 0;
        for (int row = 0; row < glyph->gh; row++)
        {
          for (int col = 0; col < glyph->gw; col++)
          {
            if (sw)
            {
              if (*px & 0x0f) printf("&"); else printf(" ");
              px++;
              sw = 0;
            }
            else
            {
              if (*px & 0xf0) printf("&"); else printf(" ");
              sw = 1;
            }
          }
          printf("\r\n");
        }
      }
      //if (c == 'y')
      //  switch_tty_and_wait(drm_fd, 1);
    }
    
    //modeset_draw_once(drm_fd);
    
    fprintf(stderr, "\r\n");
  }
  
  //Cleanup and close
  //modeset_cleanup(drm_fd);
  //close(drm_fd);
  
  return 0;
}
