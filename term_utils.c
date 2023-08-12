#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <grp.h>
#include <errno.h>
#include <fcntl.h>

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

char * get_curr_tty_name()
{
  return ttyname(STDIN_FILENO);
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



/* From agetty.c                         */

#define	MAX_SPEED	10

#define	F_PARSE		(1<<0)	/* process modem status messages */
#define	F_ISSUE		(1<<1)	/* display /etc/issue */
#define	F_RTSCTS	(1<<2)	/* enable RTS/CTS flow control */

#define F_INITSTRING    (1<<4)	/* initstring is set */
#define F_WAITCRLF	(1<<5)	/* wait for CR or LF */
#define F_CUSTISSUE	(1<<6)	/* give alternative issue file */
#define F_NOPROMPT	(1<<7)	/* do not ask for login name! */
#define F_LCUC		(1<<8)	/* support for *LCUC stty modes */
#define F_KEEPSPEED	(1<<9)	/* follow baud rate from kernel */
#define F_KEEPCFLAGS	(1<<10)	/* reuse c_cflags setup from kernel */
#define F_EIGHTBITS	(1<<11)	/* Assume 8bit-clean tty */
#define F_VCONSOLE	(1<<12)	/* This is a virtual console */
#define F_HANGUP	(1<<13)	/* Do call vhangup(2) */
#define F_UTF8		(1<<14)	/* We can do UTF8 */
#define F_LOGINPAUSE	(1<<15)	/* Wait for any key before dropping login prompt */
#define F_NOCLEAR	(1<<16) /* Do not clear the screen before prompting */
#define F_NONL		(1<<17) /* No newline before issue */
#define F_NOHOSTNAME	(1<<18) /* Do not show the hostname */
#define F_LONGHNAME	(1<<19) /* Show Full qualified hostname */
#define F_NOHINTS	(1<<20) /* Don't print hints */
#define F_REMOTE	(1<<21) /* Add '-h fakehost' to login(1) command line */

#ifdef __linux__
#  include <sys/kd.h>
#  define USE_SYSLOG
#  ifndef DEFAULT_VCTERM
#    define DEFAULT_VCTERM "linux"
#  endif
#  if defined (__s390__) || defined (__s390x__)
#    define DEFAULT_TTYS0  "dumb"
#    define DEFAULT_TTY32  "ibm327x"
#    define DEFAULT_TTYS1  "vt220"
#  endif
#  ifndef DEFAULT_STERM
#    define DEFAULT_STERM  "vt102"
#  endif
#elif defined(__GNU__)
#  define USE_SYSLOG
#  ifndef DEFAULT_VCTERM
#    define DEFAULT_VCTERM "hurd"
#  endif
#  ifndef DEFAULT_STERM
#    define DEFAULT_STERM  "vt102"
#  endif
#else
#  ifndef DEFAULT_VCTERM
#    define DEFAULT_VCTERM "vt100"
#  endif
#  ifndef DEFAULT_STERM
#    define DEFAULT_STERM  "vt100"
#  endif
#endif

#define log_err(fmt, ...) { fprintf(stderr, fmt, ##__VA_ARGS__); \
                            return 1; }
                            
#define log_warn(fmt, ...) { fprintf(stderr, fmt, ##__VA_ARGS__); \
                           }

/* Set up tty as stdin, stdout & stderr. */
int open_tty(char *tty)
{
	const pid_t pid = getpid();
	int closed = 0;
#ifndef KDGKBMODE
	int serial;
#endif

  struct termios rtp;
  struct termios *tp = &rtp;

	/* Set up new standard input, unless we are given an already opened port. */

	if (strcmp(tty, "-") != 0) {
		char buf[100];
		struct group *gr = NULL;
		struct stat st;
		int fd, len;
		pid_t tid;
		gid_t gid = 0;

		/* Use tty group if available */
		if ((gr = getgrnam("tty")))
			gid = gr->gr_gid;

		if (((len = snprintf(buf, sizeof(buf), "/dev/%s", tty)) >=
		     (int)sizeof(buf)) || (len < 0))
			log_err( "/dev/%s: cannot open as standard input %m", tty)

		/*
		 * There is always a race between this reset and the call to
		 * vhangup() that s.o. can use to get access to your tty.
		 * Linux login(1) will change tty permissions. Use root owner and group
		 * with permission -rw------- for the period between getty and login.
		 */
		if (chown(buf, 0, gid) || chmod(buf, (gid ? 0620 : 0600))) {
			if (errno == EROFS)
				log_warn( "%s: %m", buf)
			else
				log_err( "%s: %m", buf)
		}

		/* Open the tty as standard input. */
		if ((fd = open(buf, O_RDWR|O_NOCTTY|O_NONBLOCK, 0)) < 0)
			log_err( "/dev/%s: cannot open as standard input: %m", tty)

		/* Sanity checks... */
		if (fstat(fd, &st) < 0)
			log_err( "%s: %m", buf)
		if ((st.st_mode & S_IFMT) != S_IFCHR)
			log_err( "/dev/%s: not a character device", tty)
		if (!isatty(fd))
			log_err( "/dev/%s: not a tty", tty)
		if (((tid = tcgetsid(fd)) < 0) || (pid != tid)) {
			if (ioctl(fd, TIOCSCTTY, 1) == -1)
				log_warn( "/dev/%s: cannot get controlling tty: %m", tty)
		}

		close(STDIN_FILENO);
		errno = 0;

		if (0) {

			if (ioctl(fd, TIOCNOTTY))
				log_warn( "TIOCNOTTY ioctl failed\n")

			/*
			 * Let's close all file decriptors before vhangup
			 * https://lkml.org/lkml/2012/6/5/145
			 */
			close(fd);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);
			errno = 0;
			closed = 1;

			if (vhangup())
				log_err( "/dev/%s: vhangup() failed: %m", tty)
		} else
			close(fd);

		//debug("open(2)\n");
		//if (open(buf, O_RDWR|O_NOCTTY|O_NONBLOCK, 0) != 0)
    if (open(buf, O_RDWR|O_NOCTTY, 0) != 0)
			log_err( "/dev/%s: cannot open as standard input: %m", tty)

		if (((tid = tcgetsid(STDIN_FILENO)) < 0) || (pid != tid)) {
			if (ioctl(STDIN_FILENO, TIOCSCTTY, 1) == -1)
				log_warn( "/dev/%s: cannot get controlling tty: %m", tty)
		}

	} else {

		/*
		 * Standard input should already be connected to an open port. Make
		 * sure it is open for read/write.
		 */

		if ((fcntl(STDIN_FILENO, F_GETFL, 0) & O_RDWR) != O_RDWR)
    {
			log_warn( "%s: not open for read/write", tty)
    }
	}

	if (tcsetpgrp(STDIN_FILENO, pid))
		log_warn( "/dev/%s: cannot set process group: %m", tty)

	/* Get rid of the present outputs. */
	if (!closed) {
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		errno = 0;
	}

	/* Set up standard output and standard error file descriptors. */
	//debug("duping\n");

	/* set up stdout and stderr */
	if (dup(STDIN_FILENO) != 1 || dup(STDIN_FILENO) != 2)
		log_err( "%s: dup problem: %m", tty)

	/* make stdio unbuffered for slow modem lines */
	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * The following ioctl will fail if stdin is not a tty, but also when
	 * there is noise on the modem control lines. In the latter case, the
	 * common course of action is (1) fix your cables (2) give the modem
	 * more time to properly reset after hanging up.
	 *
	 * SunOS users can achieve (2) by patching the SunOS kernel variable
	 * "zsadtrlow" to a larger value; 5 seconds seems to be a good value.
	 * http://www.sunmanagers.org/archives/1993/0574.html
	 */
	memset(tp, 0, sizeof(struct termios));
	if (tcgetattr(STDIN_FILENO, tp) < 0)
		log_err( "%s: failed to get terminal attributes: %m", tty)

	/*
	 * Detect if this is a virtual console or serial/modem line.
	 * In case of a virtual console the ioctl KDGKBMODE succeeds
	 * whereas on other lines it will fails.
	 */

/*
int kbmode;
#ifdef KDGKBMODE
	if (ioctl(STDIN_FILENO, KDGKBMODE, &kbmode) == 0)
#else
	if (ioctl(STDIN_FILENO, TIOCMGET, &serial) < 0 && (errno == EINVAL))
#endif
	{
		op->flags |= F_VCONSOLE;
		if (!op->term)
			op->term = DEFAULT_VCTERM;
	} else {
#ifdef K_RAW
		op->kbmode = K_RAW;
#endif
		if (!op->term)
			op->term = DEFAULT_STERM;
	}

	setenv("TERM", op->term, 1);
*/
  
  return 0;
}
