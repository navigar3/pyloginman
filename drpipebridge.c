#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "log.h"
#include "cobj.h"
//#include "drm-doublebuff.h"

#define  SIZE_L  sizeof(size_t)
#define  MAX_SZ  512
#define  MAX_NARGS  10

struct payload_s
{
  size_t sz;
  char hrsz[4];
  char payl[MAX_SZ];
};

int compose_msg(struct payload_s * payload, char * msg)
{
  size_t msg_len = strlen(msg);
  
  if (msg_len >= MAX_SZ)
    return 1;
  
  memcpy(payload->payl, msg, msg_len);
  payload->sz = msg_len;
  
  sprintf(payload->hrsz, "%3d", msg_len);
  
  return 0;
}

int cmd_match(struct payload_s *p, char * cmddef)
{
  return memcmp(p->payl, cmddef, strlen(cmddef));
}

int pop_from_pipe(int fd, struct payload_s * payload)
{
  int rb = read(fd, payload->hrsz, 4);
  if (rb != 4)
    exit(EXIT_FAILURE);
  
  sscanf(payload->hrsz, "%3d", &(payload->sz));
  
  rb = read(fd, payload->payl, payload->sz);
  if (rb != payload->sz)
    exit(EXIT_FAILURE);
  
  return 0;
}

int push_into_pipe(int fd, struct payload_s * payload)
{
  if (write(fd, payload->hrsz, 4) == -1)
    exit(EXIT_FAILURE);
    
  if(write(fd, payload->payl, payload->sz) == -1)
    exit(EXIT_FAILURE);
    
  return 0;
}

int pc_say(int rfd, int wfd, 
           char * cmd, char * cmd_args[])
{
  struct payload_s pw, pr;
  
  /* Send message to child */
  compose_msg(&pw, cmd);
  push_into_pipe(wfd, &pw);
  
  /* Acknowledged? */
  pop_from_pipe(rfd, &pr);
  if (cmd_match(&pr, "ACK"))
  {
    fprintf(stderr, "[MAIN] Error while comunicating with child.\n");
    exit(EXIT_FAILURE);
  }
  
  return 0;
}

int pc_listen(int rfd, int wfd,
              char * ans)
{
  struct payload_s pw, pr;
  
  /* Get message from child */
  pop_from_pipe(rfd, &pr);
  
  memcpy(ans, pr.payl, pr.sz);
  ans[pr.sz] = '\0';
  
  return 0;
}

int video_manager(int rfd, int wfd,
                  char * card, 
                  char * font_file, int font_color, 
                  char * backg_file)
{
  struct payload_s p;
  
  compose_msg(&p, "READY");
  push_into_pipe(wfd, &p);
  
  /* Video Manager Loop */
  while (1)
  {
    pop_from_pipe(rfd, &p);
    
    if (!cmd_match(&p, "QUIT"))
    {
      compose_msg(&p, "ACK");
      push_into_pipe(wfd, &p);
      
      fprintf(stderr, "[VIDEO] Bye!\n");
      break;
    }
    
    fprintf(stderr, "[VIDEO] Load \'");
    write(2, p.payl, p.sz);
    fprintf(stderr, "\'\n");
    
    compose_msg(&p, "ACK");
    push_into_pipe(wfd, &p);
  }
  
  exit(EXIT_SUCCESS);
}

void engine(int nargs, char * args[])
{
  execvp(args[0], args);
}

int main(int argc, char * argv[])
{
  char * card = "/dev/dri/card0";
  char * font_file = NULL;
  int font_color = 0;
  char * backg_file = NULL;
  
  int vcpid;
  int ecpid;
  
  int eng_pc_pipefd[2];
  int eng_cp_pipefd[2];
  int vm_pc_pipefd[2];
  int vm_cp_pipefd[2];
  
  /* Open engine parent to child pipe */
  if (pipe(eng_pc_pipefd) == -1)
  {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  
  /* Open engine child to parent pipe */
  if (pipe(eng_cp_pipefd) == -1)
  {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  
  /* Open video manager parent to child pipe */
  if (pipe(vm_pc_pipefd) == -1)
  {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  
  /* Open video manager child to parent pipe */
  if (pipe(vm_cp_pipefd) == -1)
  {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
  
  /* Fork video manager */
  vcpid = fork();
  if (vcpid == -1) 
  {
   perror("fork");
   exit(EXIT_FAILURE);
  }
  
  if (vcpid == 0)
  {
    /* Child, setup pipes and try to launch video manager */
    int ret;
    
    /* Close parent to child pipe write end. */
    close(vm_pc_pipefd[1]);
    
    /* Close child to parent pipe read end. */
    close(vm_cp_pipefd[0]);
    
    /* Launch video manager */
    ret = video_manager(vm_pc_pipefd[0], vm_cp_pipefd[1],
                        card, font_file, font_color, backg_file);
    
    if (ret)
      exit(EXIT_FAILURE);
  }
  
  /* Parent */
  
  /* Close parent to child pipe read end. */
  close(vm_pc_pipefd[0]);
  
  /* Close child to parent pipe write end. */
  close(vm_cp_pipefd[1]);
  
  char vc_ans[MAX_SZ+1];
  
  /* Check if video manager successfully start */
  pc_listen(vm_cp_pipefd[0], vm_pc_pipefd[1], vc_ans);
  
  if (strcmp(vc_ans, "READY"))
  {
    fprintf(stderr, "[MAIN] Error while starting video.\n");
    exit(EXIT_FAILURE);
  }
  else
    fprintf(stderr, "[MAIN] video started.\n");
  
  /* Fork engine */
  ecpid = fork();
  if (ecpid == -1) 
  {
   perror("fork");
   exit(EXIT_FAILURE);
  }
  
  if (ecpid == 0)
  {
    /* Child, setup pipes and try to launch video manager */
    int ret;
    
    /* Close parent to child pipe write end. */
    close(eng_pc_pipefd[1]);
    
    /* Close child to parent pipe read end. */
    close(eng_cp_pipefd[0]);
    
    close(0);
    close(1);
    
    if (dup2(eng_pc_pipefd[0], 0) == -1)
    {
      fprintf(stderr, "Cannot dup2() STDIN!\n");
      exit(EXIT_FAILURE);
    }
    
    if (dup2(eng_cp_pipefd[1], 1) == -1)
    {
      fprintf(stderr, "Cannot dup2() STDOUT!\n");
      exit(EXIT_FAILURE);
    }
    
    /* Launch engine */
    char * args[] = {"./engine.sh", NULL};
    engine(1, args);
      
    exit(EXIT_SUCCESS);
  }
  
  /* Parent again */
  
  /* Close parent to child pipe read end. */
  close(eng_pc_pipefd[0]);
  
  /* Close child to parent pipe write end. */
  close(eng_cp_pipefd[1]);
  
  char ec_ans[MAX_SZ+1];
  
  /* Check if engine successfully start */
  pc_listen(eng_cp_pipefd[0], eng_pc_pipefd[1], ec_ans);
  
  if (strcmp(ec_ans, "READY"))
  {
    fprintf(stderr, "[MAIN] Error while starting engine.\n");
    exit(EXIT_FAILURE);
  }
  else
    fprintf(stderr, "[MAIN] engine is ready.\n");
  
  sleep(3);
  
  pc_say(vm_cp_pipefd[0], vm_pc_pipefd[1], "Test from Parent.", NULL);
  
  sleep(3);
  
  pc_say(eng_cp_pipefd[0], eng_pc_pipefd[1], "To engine.\n", NULL);
  
  sleep(3);
  
  pc_say(vm_cp_pipefd[0], vm_pc_pipefd[1], "QUIT", NULL);
  
  pc_say(eng_cp_pipefd[0], eng_pc_pipefd[1], "QUIT", NULL);
  
  sleep(2);
  
  exit(EXIT_SUCCESS);
}
