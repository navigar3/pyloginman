#ifndef _LOG_H

#define _LOG_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define  MAX_MSG_LEN  200

#define LogMsg(fmt, ...) \
  {  \
    char msg[MAX_MSG_LEN]; \
    snprintf(msg, MAX_MSG_LEN, fmt, ##__VA_ARGS__); \
    log_msg(msg); \
  }

#define LogWarn(fmt, ...) \
  {  \
    char msg[MAX_MSG_LEN]; \
    snprintf(msg, MAX_MSG_LEN, fmt, ##__VA_ARGS__); \
    log_warn(msg); \
  }
  
#define LogErr(fmt, ...) \
  {  \
    char msg[MAX_MSG_LEN]; \
    snprintf(msg, MAX_MSG_LEN, fmt, ##__VA_ARGS__); \
    log_err(msg); \
  }

#define LogDbg(l, fmt, ...) \
  {  \
    char msg[MAX_MSG_LEN]; \
    snprintf(msg, MAX_MSG_LEN, fmt, ##__VA_ARGS__); \
    log_dbg(l, msg); \
  }

int _dont_log = 1;
int * log_fd = &_dont_log;

int _dont_dbg = 0;
int * dbg_level = &_dont_dbg;


char * _Mnm[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* Log a message into logfile */
int log_msg(char * msg)
{
  if (*log_fd < 0)
    return 0;
  
  char full_msg[MAX_MSG_LEN];
  
  /* Get local time */
  time_t tic = time(NULL);
  struct tm * t = localtime(&tic);
  
  /* Compose message string */
  int msglen =
    snprintf(full_msg, MAX_MSG_LEN-1,
             "[%4d-%s-%02d %02d:%02d:%02d] %s\n", 
             1900+t->tm_year,
             _Mnm[t->tm_mon],
             t->tm_mday,
             t->tm_hour,
             t->tm_min,
             t->tm_sec,
             msg);
  
  return write(*log_fd, full_msg, msglen);
}


int log_warn(char * msg)
{
  if (*log_fd < 0)
    return 0;
  
  char full_msg[MAX_MSG_LEN];
  
  /* Get local time */
  time_t tic = time(NULL);
  struct tm * t = localtime(&tic);
  
  /* Compose message string */
  int msglen =
    snprintf(full_msg, MAX_MSG_LEN-1,
             "[%4d-%s-%02d %02d:%02d:%02d] WARNING: %s\n", 
             1900+t->tm_year,
             _Mnm[t->tm_mon],
             t->tm_mday,
             t->tm_hour,
             t->tm_min,
             t->tm_sec,
             msg);
  
  return write(*log_fd, full_msg, msglen);
}

int log_err(char * msg)
{
  if (*log_fd < 0)
    return 0;
  
  char full_msg[MAX_MSG_LEN];
  
  /* Get local time */
  time_t tic = time(NULL);
  struct tm * t = localtime(&tic);
  
  /* Compose message string */
  int msglen =
    snprintf(full_msg, MAX_MSG_LEN-1,
             "[%4d-%s-%02d %02d:%02d:%02d] ERROR: %s\n", 
             1900+t->tm_year,
             _Mnm[t->tm_mon],
             t->tm_mday,
             t->tm_hour,
             t->tm_min,
             t->tm_sec,
             msg);
  
  return write(*log_fd, full_msg, msglen);
}

int log_dbg(int level, char * msg)
{
  if (*dbg_level < level)
    return 0;
    
  if (*log_fd < 0)
    return 0;
  
  char full_msg[MAX_MSG_LEN];
  
  /* Get local time */
  time_t tic = time(NULL);
  struct tm * t = localtime(&tic);
  
  /* Compose message string */
  int msglen =
    snprintf(full_msg, MAX_MSG_LEN-1,
             "[%4d-%s-%02d %02d:%02d:%02d] DEBUG: %s\n", 
             1900+t->tm_year,
             _Mnm[t->tm_mon],
             t->tm_mday,
             t->tm_hour,
             t->tm_min,
             t->tm_sec,
             msg);
  
  return write(*log_fd, full_msg, msglen);
}


int init_logger(char * filename)
{
  int fd = open(filename, 
                O_CREAT|O_WRONLY|O_APPEND, 
                S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  
  if (fd < 0)
  {
    fprintf(stderr, "Error while opening %s for writing!\n", filename);
    exit(1);
  }
  
  return fd;
}


int set_log_fd(int * des)
{
  log_fd = des;
  
  return 0;
}

int set_log_dbg_level(int * des)
{
  dbg_level = des;
  
  return 0;
}

#endif
