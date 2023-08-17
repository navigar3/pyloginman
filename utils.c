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

int wait_for_tty_active(int tty_num)
{
  /* Wait until this tty become active again. */
  if (ioctl(STDIN_FILENO, VT_WAITACTIVE, tty_num) == -1)
  {
    fprintf(stderr, "ioctl() VT_WAITACTIVE %d failed!\n", 2);
    exit(1);
  }
  
  return 0;
}

int detach_tty(void)
{
  ioctl(0, TIOCNOTTY, NULL);
  
  return 0;
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
		int fd, len, flags;
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

    close(fd);
    
    fd = open(buf, O_RDWR|O_NOCTTY|O_NONBLOCK, 0);
    
    if (fd != 0)
			log_err( "/dev/%s: cannot open as standard input: %m", tty)

		if (((tid = tcgetsid(STDIN_FILENO)) < 0) || (pid != tid)) {
			if (ioctl(STDIN_FILENO, TIOCSCTTY, 1) == -1)
				log_warn( "/dev/%s: cannot get controlling tty: %m", tty)
		}
    
    flags = fcntl(fd, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

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
  
  return 0;
}

#include <sys/file.h>
#include <time.h>
#include <utmp.h>

#ifdef LOGIN_PROCESS
#  define SYSV_STYLE
#endif

#ifdef	SYSV_STYLE

#define _PATH_WTMPLOCK		"/etc/wtmplock"

static inline int write_all(int fd, const void *buf, size_t count)
{
  while (count) {
    ssize_t tmp;

    errno = 0;
    tmp = write(fd, buf, count);
    if (tmp > 0) {
      count -= tmp;
      if (count)
        buf = (void *) ((char *) buf + tmp);
    } else if (errno != EINTR && errno != EAGAIN)
      return -1;
    if (errno == EAGAIN)    /* Try later, *sigh* */
      usleep(250000);
  }
  return 0;
}

/* Update our utmp entry. */
void update_utmp(char * line)
{
	struct utmp ut;
	time_t t;
	pid_t pid = getpid();
	pid_t sid = getsid(0);
	struct utmp *utp;
  
  static char * fakehost = NULL;
  
  char * vcline;
  char * null_vcl = '\0';
  
  /* On virtual console remember the line which is used for */
	if (strncmp(line, "tty", 3) == 0 &&
	    strspn(line + 3, "0123456789") == strlen(line+3))
		vcline = line+3;
  else
    vcline = null_vcl;

	/*
	 * The utmp file holds miscellaneous information about things started by
	 * /sbin/init and other system-related events. Our purpose is to update
	 * the utmp entry for the current process, in particular the process type
	 * and the tty line we are listening to. Return successfully only if the
	 * utmp file can be opened for update, and if we are able to find our
	 * entry in the utmp file.
	 */
	utmpname(_PATH_UTMP);
	setutent();

	/*
	 * Find my pid in utmp.
	 *
	 * FIXME: Earlier (when was that?) code here tested only utp->ut_type !=
	 * INIT_PROCESS, so maybe the >= here should be >.
	 *
	 * FIXME: The present code is taken from login.c, so if this is changed,
	 * maybe login has to be changed as well (is this true?).
	 */
	while ((utp = getutent()))
		if (utp->ut_pid == pid
				&& utp->ut_type >= INIT_PROCESS
				&& utp->ut_type <= DEAD_PROCESS)
			break;

	if (utp) {
		memcpy(&ut, utp, sizeof(ut));
	} else {
		/* Some inits do not initialize utmp. */
		memset(&ut, 0, sizeof(ut));
		if (vcline && *vcline)
			/* Standard virtual console devices */
			strncpy(ut.ut_id, vcline, sizeof(ut.ut_id));
		else {
			size_t len = strlen(line);
			char * ptr;
			if (len >= sizeof(ut.ut_id))
				ptr = line + len - sizeof(ut.ut_id);
			else
				ptr = line;
			strncpy(ut.ut_id, ptr, sizeof(ut.ut_id));
		}
	}

	strncpy(ut.ut_user, "LOGIN", sizeof(ut.ut_user));
	strncpy(ut.ut_line, line, sizeof(ut.ut_line));
	if (fakehost)
		strncpy(ut.ut_host, fakehost, sizeof(ut.ut_host));
	time(&t);
#if defined(_HAVE_UT_TV)
	ut.ut_tv.tv_sec = t;
#else
	ut.ut_time = t;
#endif
	ut.ut_type = LOGIN_PROCESS;
	ut.ut_pid = pid;
	ut.ut_session = sid;

	pututline(&ut);
	endutent();

	{
#ifdef HAVE_UPDWTMP
		updwtmp(_PATH_WTMP, &ut);
#else
		int ut_fd;
		int lf;

		if ((lf = open(_PATH_WTMPLOCK, O_CREAT | O_WRONLY, 0660)) >= 0) {
			flock(lf, LOCK_EX);
			if ((ut_fd =
			     open(_PATH_WTMP, O_APPEND | O_WRONLY)) >= 0) {
				write_all(ut_fd, &ut, sizeof(ut));
				close(ut_fd);
			}
			flock(lf, LOCK_UN);
			close(lf);
		}
#endif				/* HAVE_UPDWTMP */
	}
}

#endif				/* SYSV_STYLE */


/* caller guarantees n > 0 */
static inline void xstrncpy(char *dest, const char *src, size_t n)
{
        strncpy(dest, src, n-1);
        dest[n-1] = 0;
}

/* Log utmp user login */
void log_utmp(int pid, char * tty_name, char * username, 
              char * hostname, char * hostaddress)
{
	struct utmp ut;
	struct utmp *utp;
	struct timeval tv;

	utmpname(_PATH_UTMP);
	setutent();
  
  char * tty_number;
  char * null_tty_num = '\0';
  
  /* On virtual console remember the line which is used for */
	if (strncmp(tty_name, "tty", 3) == 0 &&
	    strspn(tty_name + 3, "0123456789") == strlen(tty_name+3))
		tty_number = tty_name+3;
  else
    tty_number = null_tty_num;

	/* Find pid in utmp.
	 *
	 * login sometimes overwrites the runlevel entry in /var/run/utmp,
	 * confusing sysvinit. I added a test for the entry type, and the
	 * problem was gone. (In a runlevel entry, st_pid is not really a pid
	 * but some number calculated from the previous and current runlevel.)
	 * -- Michael Riepe <michael@stud.uni-hannover.de>
	 */
	while ((utp = getutent()))
		if (utp->ut_pid == pid
		    && utp->ut_type >= INIT_PROCESS
		    && utp->ut_type <= DEAD_PROCESS)
			break;

	/* If we can't find a pre-existing entry by pid, try by line.
	 * BSD network daemons may rely on this. */
	if (utp == NULL && tty_name) {
		setutent();
		ut.ut_type = LOGIN_PROCESS;
		strncpy(ut.ut_line, tty_name, sizeof(ut.ut_line));
		utp = getutline(&ut);
	}

	/* If we can't find a pre-existing entry by pid and line, try it by id.
	 * Very stupid telnetd daemons don't set up utmp at all. (kzak) */
	if (utp == NULL && tty_number) {
	     setutent();
	     ut.ut_type = DEAD_PROCESS;
	     strncpy(ut.ut_id, tty_number, sizeof(ut.ut_id));
	     utp = getutid(&ut);
	}

	if (utp)
		memcpy(&ut, utp, sizeof(ut));
	else
		/* some gettys/telnetds don't initialize utmp... */
		memset(&ut, 0, sizeof(ut));

	if (tty_number && ut.ut_id[0] == 0)
		strncpy(ut.ut_id, tty_number, sizeof(ut.ut_id));
	if (username)
		strncpy(ut.ut_user, username, sizeof(ut.ut_user));
	if (tty_name)
		xstrncpy(ut.ut_line, tty_name, sizeof(ut.ut_line));

#ifdef _HAVE_UT_TV		/* in <utmpbits.h> included by <utmp.h> */
	gettimeofday(&tv, NULL);
	ut.ut_tv.tv_sec = tv.tv_sec;
	ut.ut_tv.tv_usec = tv.tv_usec;
#else
	{
		time_t t;
		time(&t);
		ut.ut_time = t;	/* ut_time is not always a time_t */
				/* glibc2 #defines it as ut_tv.tv_sec */
	}
#endif
	ut.ut_type = USER_PROCESS;
	ut.ut_pid = pid;
	if (hostname) {
		xstrncpy(ut.ut_host, hostname, sizeof(ut.ut_host));
		if (hostaddress)
			memcpy(&ut.ut_addr_v6, hostaddress,
			       sizeof(ut.ut_addr_v6));
	}

	pututline(&ut);
	endutent();

	updwtmp(_PATH_WTMP, &ut);
}


#define _PATH_BTMP		"/var/log/btmp"

/*
 * Logs failed login attempts in _PATH_BTMP, if it exists.
 * Must be called only with username the name of an actual user.
 * The most common login failure is to give password instead of username.
 */
void log_btmp(int _pid, char * tty_name, char * username, 
              char * hostname, char * hostaddress)
{
	struct utmp ut;
#if defined(_HAVE_UT_TV)        /* in <utmpbits.h> included by <utmp.h> */
	struct timeval tv;
#endif

  char * tty_number;
  char * null_tty_num = '\0';
  
  pid_t pid = (_pid) ? _pid : getpid(); 
  
  /* On virtual console remember the line which is used for */
	if (strncmp(tty_name, "tty", 3) == 0 &&
	    strspn(tty_name + 3, "0123456789") == strlen(tty_name+3))
		tty_number = tty_name+3;
  else
    tty_number = null_tty_num;
    

	memset(&ut, 0, sizeof(ut));

	strncpy(ut.ut_user,
		username ? username : "(unknown)",
		sizeof(ut.ut_user));

	if (tty_number)
		strncpy(ut.ut_id, tty_number, sizeof(ut.ut_id));
	if (tty_name)
		xstrncpy(ut.ut_line, tty_name, sizeof(ut.ut_line));

#if defined(_HAVE_UT_TV)	/* in <utmpbits.h> included by <utmp.h> */
	gettimeofday(&tv, NULL);
	ut.ut_tv.tv_sec = tv.tv_sec;
	ut.ut_tv.tv_usec = tv.tv_usec;
#else
	{
		time_t t;
		time(&t);
		ut.ut_time = t;	/* ut_time is not always a time_t */
	}
#endif

	ut.ut_type = LOGIN_PROCESS;	/* XXX doesn't matter */
	ut.ut_pid = pid;

	if (hostname) {
		xstrncpy(ut.ut_host, hostname, sizeof(ut.ut_host));
		if (hostaddress)
			memcpy(&ut.ut_addr_v6, hostaddress,
			       sizeof(ut.ut_addr_v6));
	}

	updwtmp(_PATH_BTMP, &ut);
}
