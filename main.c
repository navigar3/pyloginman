#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>

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

int test(void)
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

int go_background(int pipesfds[2], char * pipesnames[2])
{
  pid_t cpid;
  
  if (pipesfds[0] == -1)
  {
    if (!pipesnames[0])
    {
      fprintf(stderr, "Failed to handle read end pipe.\n");
      exit(EXIT_FAILURE);
    }
    
    struct stat sb;
    
    if (stat(pipesnames[0], &sb) == -1)
    {
      perror("Failed to open read pipe");
      exit(EXIT_FAILURE);
    }
    if (!(sb.st_mode & S_IFIFO))
    {
      fprintf(stderr, "Read end must be a pipe!\n");
      exit(EXIT_FAILURE);
    }
  }
  
  if (pipesfds[1] == -1)
  {
    if (!pipesnames[1])
    {
      fprintf(stderr, "Failed to handle write end pipe.\n");
      exit(EXIT_FAILURE);
    }
    
    struct stat sb;
    
    if (stat(pipesnames[1], &sb) == -1)
    {
      perror("Failed to open write pipe");
      exit(EXIT_FAILURE);
    }
    if (!(sb.st_mode & S_IFIFO))
    {
      fprintf(stderr, "Write end must be a pipe!\n");
      exit(EXIT_FAILURE);
    }
  }
  
  fprintf(stderr, "Going background ...\n");
  
  cpid = fork();
  if (cpid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  
  if (cpid == 0)             /* Child */
  {
    
    int readpipe;
    int writepipe;
    
    if (pipesfds[0] != -1)
      readpipe = pipesfds[0];
    else
    {
      readpipe = open(pipesnames[0], O_RDONLY);
      if (readpipe<=0)
      {
        perror("Failed to open read pipe");
        exit(EXIT_FAILURE);
      }
    }
    
    if (pipesfds[1] != -1)
      writepipe = pipesfds[1];
    else
    {
      writepipe = open(pipesnames[1], O_WRONLY);
      if (writepipe<=0)
      {
        perror("Failed to open write pipe");
        exit(EXIT_FAILURE);
      }
    }
    
    // Main Loop
    //  1. Read command from read pipeline
    //  2. Exec job
    //  3. Write answer to write pipeline
    //  4. Come back to 1.
    
    char buf[100];
    int n = 0;
    
    while (read(readpipe, buf+n, 1) > 0)
      n++;
    buf[n] = '\0';
    
    char outstr[] = " From Background: ";
    
    write(writepipe, outstr, strlen(outstr));
    write(writepipe, buf, strlen(buf));
    //
    
    _exit(EXIT_SUCCESS);
  }
    
    else                    /* Parent exits immediately */
      exit(EXIT_SUCCESS);
}

int scan_cmd_arg(int argc, char * argv[], int * argi, 
                 char * fmt, void * parsed)
{
  if (*argi+1<argc)
  {
    if (sscanf(argv[*argi+1], fmt, parsed)<=0)
    {
      fprintf(stderr, "%s is not valid %s value!\n", argv[*argi+1], argv[*argi]);
      exit(1);
    }
  }
  else
  {
    fprintf(stderr, "Argument %s require a value!\n", argv[*argi]);
    exit(1);
  }
  
  (*argi) += 2;
  
  return 0;
}

int main(int argc, char * argv[])
{
  int pipefd[2] = {-1, -1};
  char * pipename[2] = {NULL, NULL};
  
  // Parse command line args
  for (int i=1; i<argc; )
  {
    if (strcmp(argv[i], "--pipe-read-fd") == 0)
      scan_cmd_arg(argc, argv, &i, "%d", pipefd);
    else if (strcmp(argv[i], "--pipe-write-fd") == 0)
      scan_cmd_arg(argc, argv, &i, "%d", pipefd+1);
    else if (strcmp(argv[i], "--pipe-read-name") == 0)
      scan_cmd_arg(argc, argv, &i, "%ms", pipename);
    else if (strcmp(argv[i], "--pipe-write-name") == 0)
      scan_cmd_arg(argc, argv, &i, "%ms", pipename+1);
    else
    {
      fprintf(stderr, "Unknown arg %s!\n", argv[i]);
      exit(1);
    }
  }
  
  go_background(pipefd, pipename);
  
  exit(EXIT_SUCCESS);
}
