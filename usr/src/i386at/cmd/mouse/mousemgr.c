#ident	"@(#)mousemgr.c	1.7"
#ident	"$Header$"

/*
 *					  Mouse Manager
 *
 * This process waits for commands from the mouse driver.
 * As a result of these commands, it I_PUSH's or I_POP's
 * or I_PLINK's driver/modules, to create the physical mouse 
 * device's data stream. 
 * Eg. tty00 : open the port in O_NDELAY mode, saving the old terminal
 * settings (pointless since the O_EXCL open mode requires that the old
 * state is closed, hence no settings to save. Then set the correct mouse 
 * according to the autodetection results (Baud, Data etc) and I_POP the
 * autopushed modules (/etc/ap/chan.ap : ttcompat & ldterm), then I_PUSH
 * the serial mouse module, "smse", onto the port. Then link the iasy/smse
 * stream onto the lower side of the cmux module. 
 * The complexity of the mouse system (mousemgr, mse, smse, bmse, m320,
 * /etc/default/mouse, /usr/lib/mousetab) allows applications to open 
 * the mse driver's nodes (/dev/mouse[-|cfg|mon]) and have the required
 * STREAMS initialised automatically. The mouse event stream is accessible 
 * by various means: direct calls to /dev/mousemon MOUSEIOCMON ioctls,
 * events from /dev/event, using the libevent interface to have the data
 * written into user data space; the cmux channel multiplexor ensures 
 * events follow the active virtual terminal. 
 * Modules involved are bmse, m320, (iasy + smse) generating events, 
 * and the char, xque and event modules which select the mouse events 
 * from the upstream cmux data streams.
 *
 * L001 philk@sco.com 12/11/97
 * Added code to read the /etc/default/mouse file to retrieve the
 * MOUSEBUTTONS parameter: used for third button emulation. Calls 
 * the MOUSEIOC3BE to set the value in the mse driver.
 * Generally tidied up a bit. Added debugging/logging calls but #ifdef
 * out, too risky at BL15.
 *
 * L002 philk@sco.com 25/11/97
 * Make mouse initialisation less fussy about emulation. If emulation 
 * data file (/etc/default/mouse) is absent (eg. ISL) don't give up on
 * the mouse, just don't do emulation.
 *
 * L003 brendank@sco.com 17/12/97
 * Change to prevent the X server getting a device busy error when it
 * tries to open the mouse on startup, after X was previously killed
 * with a forced VT switch (<ALT><SYSREQ>f).
 * Added code to retry the unlinking of the mouse and VT streams in the
 * close processing.  The <ALT><SYSREQ>f key sequence kills the process
 * on the VT and sends a hangup up stream.  This hangup prevents the 
 * VT stream being used again until it is closed, in particular, it
 * prevents the streams being unlinked.  The retry processing will
 * retry the unlink up to UNLINK_MAX_RETRIES times, waiting 
 * UNLINK_RETRY_DELAY before each retry apart from the first.
 * The retries give any other processes that have the VT stream
 * open time to see the hangup - the hangup won't get cleared until
 * everyone has closed the stream.
 *
 * TODO 
 * ANSIfy, ie. K&R2 prototypes etc. 
 * The entire mouse subsystem seems to duplicate an awful lot of data 
 * consodering that the only mapping needed is a mouse device to a 
 * terminal device. 
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <locale.h>
#include "sys/types.h"
#include "sys/termio.h"
#include "sys/fcntl.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/mman.h"
#include "sys/stream.h"
#include "sys/open.h"
#include "sys/vt.h"
#include <sys/kd.h>
#include "sys/stropts.h"
#include <errno.h>
#include <string.h>
#include <sys/mouse.h>
#include <sys/cm_i386at.h>
#include <sys/mse.h>
#include <varargs.h>
#include <grp.h>
#include <ulimit.h>						/* L001 */
#include "sys/stat.h"


#ifndef MOUSEIOC3BE 
/* 
 * Temporary define for now, until mse.h updated
 */
	#define MOUSEIOC3BE	(('M'<<8)|110)
#endif

#ifdef MMGR_DEBUG 
	#undef MMGR_DEBUG 
#endif 

/* Local definitions */

#define MGR_NAME		"/dev/mousemon"
#define MOUSETAB		"/usr/lib/mousetab"
#define MPARM		   "MOUSEBUTTONS="
#define MOUSEDEF		"/etc/default/mouse"

int			 	mgr_fd;
struct mse_mon  command;
char			errbuf[1024];

#define MAX_DEV		 100
#define MAXDEVNAME	  64

					/* L003 vvv */
#define	UNLINK_MAX_RETRIES	15
#define	UNLINK_RETRY_DELAY	2
					/* L003 ^^^ */
struct mousemap map[MAX_DEV];

struct mtable {
		int	 mse_fd;
		int	 disp_fd[16];
		int	 type;
		int	 linkid;
		int	 link_vt;
		struct termio   saveterm;
		char	name[MAXDEVNAME];
		char	linkname[MAXDEVNAME];
		char	dname[MAXDEVNAME];
} ;

/* 
 * L001 
 * Define debug diagnostic calls. DBGPRT prints to debug log, NFERROR is a 
 * non-fatal error (also logged) and FERROR is a fatal error (log and exit).
 * DBGPRT only if DEBUG, others all the time. The log file always exists, 
 * the debug messages only for MMGR_DEBUG builds and the debug flag.
 */

#ifdef MMGR_DEBUG 

	#define DBGPRT(a)	   dbgprt(a)
	#define NFERROR(a)		nferror(a)
	#define FERROR(a)		ferror(a)
	#ifdef STATIC 
		#undef STATIC 
	#endif 
	#define STATIC 

#else 

	#define DBGPRT(a)
	#define NFERROR(a)
	#define FERROR(a)
	#ifdef STATIC 
		#undef STATIC 
	#endif 
	#define STATIC static 

#endif 

/* 
 * Global data 
 */

struct mtable 	table[MAX_DEV];
static int	  n_dev = 0;
static int 		disp_vt= 0;
time_t  		tab_time;

/* 
 * Prototypes
 */

void 	do_open(), do_close();
void 	load_table(), download_table();
int 	read_dfile();

int	msebtns = -1;

#ifdef MMGR_DEBUG 

void 	nferror(),dbgprt(), ferror();

/* 
 * Static data 
 */

STATIC int 	logfd = -1;
STATIC int 	logfsz = 0;

/* 
 * int openlog ( char *name, int size )
 * 
 * Calling/Exit:
 *	  Called with an optional file name, and max size. If not specified 
 *	  defaults to "log" and 1MB, file wraps at 1MB. Sets global logfd to
 *	  file descriptor.
 */

int 
openlog(char *logfilename, int logfilesz)
{
		struct stat	 statb;
		struct group	*gp;
		int			 fd,fl; 
		char			*fname = 0;

		/* 
		 * For log file names passed as a null pointer or a pointer
		 * to NUL, open the default logfile "log". 
		 */ 

		if (!logfilename || *logfilename == '\0') 
				fname = "log"; 
		else 
				fname = logfilename; 

		if (strlen(fname) > 256) { 
				printf("ERROR: Log file name too long\n");
				return -1;
		}

		/* 
		 * Open log file in current directory?
		 */

		if ((fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC,0644)) < 0) { 
				printf("ERROR: Cant open logfile %s",fname); 
				return -1;
		}

		/* 
		 * Set close on exec flag to prevent inheritance. 
		 */

		fl = fcntl(fd, F_GETFD, 0);
		if ((fl < 0) || ( fcntl(fd, F_SETFD, fl|FD_CLOEXEC) < 0)) {
				close(fd);
				return -1; 
		}

		logfd = fd; 
		if (logfilesz > 0)
				logfsz = logfilesz; 
		else 
				logfsz = (1024 * 1024);		 /* 1MB  */

		/* 
		 * Set owner and group IDs.
		 */

		/* gp = getgrnam("sys");		*/
		if (!gp) { 
				printf("ERROR: Cant get group \"sys\" group ID\n"); 
				close(logfd); 
				logfd = -1;
				errno = EINVAL;
				return -1; 
		}

		/* 
		 * Chown if file not owned by root/sys.
		 */
		
		if (fstat (logfd, &statb) < 0){ 
				printf("ERROR: Cant stat(2) logfile\n");
				close (logfd);
				fd = logfd = -1;
				return -1;
		}

		if ((statb.st_uid != 0) || (statb.st_gid != gp->gr_gid)) 
				fchown(fd, 0, gp->gr_gid); 

		return 0;
}


static void 
_cmnerr(const char *fmt, va_list ap)
{
		int							 err, bufsz; 
		struct stat			 statb; 

		/* Save current errno   */
		err = errno;

		/* Copy data into buffer		*/
		vsprintf(errbuf,fmt,ap); 

		/* Add system error code description	*/
		if (err)
				sprintf(errbuf + strlen(errbuf),": %s",strerror(err)); 

		/* Add newline  */
		strcat (errbuf,"\n"); 

		bufsz = strlen(errbuf); 

		/* Check logfile size and truncate if necessary */
		if ( fstat(logfd, &statb) < 0 ) 
				return ;

		if (statb.st_size > logfsz) 
				ftruncate ( logfd, 0);

		/* Send to the logfile  */
		if (bufsz > write(logfd,errbuf,strlen(errbuf))) { 

				/* What can be done ? Write meta error to meta log	  */

		}

		return;
}


/* 
 * void dbgprt(char *,...)
 * 
 * Called to display a debug message. Can use the invocation params to 
 * actually name a debug log, otherwise write to stderr, since we can run 
 * the process as a foreground process. 
 */

void 
dbgprt(char * fmt,...)
{
		return;
}

/* 
 * void ferror(int exitcode, char * fmt,...)
 * 
 * Called on a fatal error. Print message to stderr. 
 */

void 
ferror(int ecode, char * fmt,...)
{
		return;
}

/* 
 * void nferror(char *,...)
 * 
 * Called on a non-fatal error. Write the error message and the strerror() 
 * message (if errno) to the logfile and return.
 */

void 
nferror(char * fmt,...)
{
		return;
}

#endif 


/* 
 *	mousemgr (1M) is the mouse system daemon. It is used to set up the mouse
 * 	hardware device when processes request data fron the generic mouse driver
 *	mse. mse issues requests to the mousemgr via the MOUSEIOCMON ioctl to open
 *	and close the mouse device and mousemgr looks up the hardware details from
 *	the mousetab map table, initialises the hardware devices and configures 
 *	the STREAMS stacks used to merge the input event streams. It allows the 
 *	processes requesting mouse events to be independent of any explicit 
 *	knowledge of the mouse devices. 
 *		On the downside, the proliferation of control entities means that much 
 *	configuration data is duplicated (races on changes, wasting space, extra
 *	complexity, maintenance problems) and causes of errors are multipled. The
 *	error handling is poor and reporting non-existent, leading to difficulty 
 * 	in diagnosing problems. The test/configuration programs use different 
 *	mechanisms for mouse data retrieval to those use by 'real' clients (eg. 
 *	the X server, usemouse(1) : scenarios where the system configures and tests
 *	OK but the X server receives no mouse input are not uncommon. 
 *		
 *	TODO
 *		Redesign/rewrite mouse subsystem, eliminating duplicated information, 
 *	and adding plentiful error reporting (eg. mousemgr debug/log). Maybe the 
 *	entire mousemgr daemon could be discarded ? 
 *		Make the mse driver loadable ? 
 *		Improve test programs to use X/usemouse interfaces.
 *		
 */

main()
{
	int	 lastfd = ulimit(UL_GDESLIM) - 1;

	/* Close all inherited fds > stderr	 */

	for (mgr_fd = lastfd; --mgr_fd > 2;) {
		close(mgr_fd);
	}

	/* Set the internationalisation message catalogues	  */

	(void)setlocale(LC_ALL, "");
	(void)setcat("mousemgr");

	/* 
	 * Open the mouse configuration device (provided by mse driver): the
	 * mse driver will pass commands to the mousemgr daemon through this 
	 * port by setting the data returned from the MOUSEIOCMON ioctl(2)s.
	 */

	if((mgr_fd = open(MGR_NAME, O_RDWR)) < 0){
		if( errno == EBUSY ) {
			FERROR((0,gettxt("open mse cfg port %s failed (busy)",MGR_NAME))); 
			exit (-1);
			/* NOTREACHED */
		} else {
			FERROR((1,gettxt(":76","mousemgr: /dev/mousemon open failed")));
			exit (-1);
			/* NOTREACHED */
		}
	}

	/* Close stdin and stdout	   */
	close(0); 
	close(1);

	/* 
	 * Set the highest numbered file descriptor (say 63) to the 
	 * mouse configuration port.
	 */

	if(dup2(mgr_fd, lastfd) >= 0) {
		close(mgr_fd);
		mgr_fd = lastfd;
	}

	load_table();
	download_table();


	/* 
	 * This is the mouse service handling loop. mousemgr sleeps on the 
	 * MOUSEIOCMON ioctl(2) call, until the mouse driver requires service
	 * when it fills the ioctl(2) return data with the service data and 
	 * wakes the mousemgr.
	 */

	for (;;) {

		if (ioctl(mgr_fd, MOUSEIOCMON, &command) < 0) {
			perror(gettxt(":77", "mousemgr: MOUSEIOCMON ioctl failed"));
			exit(1);
		}

		command.errno = 0;

		switch (command.cmd & 7) {

			case MSE_MGR_OPEN:
				do_open();
				break;

			case MSE_MGR_CLOSE:
			case MSE_MGR_LCLOSE:
				do_close();
				break;

			default:
				fprintf(stderr, 
					gettxt(":78","mousemgr: Unknown cmd: %d\n"),command.cmd);
				break;

		} 

	}

	/* NOTREACHED */	

}

/* 
 * L001 
 * Read the /etc/default/mouse file to get the MOUSEBUTTONS variable, used
 * to enable the 3rd button emulation. Value is written by the mouseadmin
 * utility. If this cannot be done (file not found, etc. then return -1 
 * to indicate an error, but don't exit (killing the daemon): it's not that 
 * important.
 */

int
read_dfile()
{
	int			fd;
	void		*fp;
	char 		*cp; 
	struct stat	statb;
	char		str[] = MPARM;	/* MPARM is MOUSEBUTTONS=	*/
	size_t		fsz;


	if ((fd = open(MOUSEDEF,O_RDONLY)) < 0) {		/* /etc/default/mouse 	*/
		NFERROR(("Cant open file %s\n",MOUSEDEF));
		return (-1); 
	}

	if (fstat(fd, &statb) < 0) {
		NFERROR(("Cant stat file %s\n",MOUSEDEF));
		return (-1); 
	}

	fsz = statb.st_size;

	/* 
 	 * /etc/default/mouse is 12 bytes long presently, it holds 
	 * only MOUSEBUTTONS=n string. This code will be OK for file
	 * expansion to a page.
	 */

	if ((fp=mmap(0, fsz, PROT_READ, MAP_SHARED, fd, 0)) == 0) {
		NFERROR(("Cant mmap file; %s\n",MOUSEDEF));
		return (-1); 
	}

	/* Use strstr(2) to search for MOUSEBUTTONS=, OK for no embedded NULs. */
	if ((cp = strstr((char *)fp, str)) == 0) {
		NFERROR(("Cant find %s string",str));
		return (-1);
	}

	/* If not in first page quit with oversized files. */
	if (fsz > 4096) { 
		NFERROR(("File %s too large %d\n",str,fsz)); 
		return (-1); 
	}

	/* Bump pointer to end of the definition	*/
	cp += strlen(MPARM);

	/* Look for value in decimal, to end of file	*/

	while (((cp - (char *)fp) < fsz) && !isdigit((int)*cp))
		++cp; 	

	msebtns = *cp - '0'; 

	if ((msebtns != 2) && (msebtns != 3)) {
		NFERROR(("Bad mousebuttons value %d\n",msebtns));
		return (-1);
	}

	munmap(fp,statb.st_size);
	return (0);

}


/* 
 * void load_table
 * 
 * Reads the /usr/lib/mousetab and verifies that the specified devices
 * are correct, ie. character special devices, recognisable mouse devs,
 *
 */

void
load_table()
{
	FILE			*tabf;
	char			dname[MAXDEVNAME], mname[MAXDEVNAME];
	struct stat	 	statb;

	if ((tabf = fopen(MOUSETAB, "r")) == NULL)
		return;

	fstat(fileno(tabf), &statb);

	tab_time = statb.st_mtime;

	/* Format is:
	 *	  disp_name	   mouse_name
	 */

	n_dev = 0;
	strcpy(dname, "/dev/");

	while (fscanf(tabf, "%s %s", dname + 5, mname) > 0) {

		if (stat(dname, &statb) == -1)
			continue;

		if ((statb.st_mode & S_IFMT) != S_IFCHR)
			continue;

		map[n_dev].disp_dev = statb.st_rdev;

		if (!strncmp(mname, "m320", 4))
			table[n_dev].type = map[n_dev].type = M320;
		else if (!strncmp(mname, "bmse", 4))
			table[n_dev].type = map[n_dev].type = MBUS;
		else
			table[n_dev].type = map[n_dev].type = MSERIAL;

		strcpy(table[n_dev].name, "/dev/");
		strcat(table[n_dev].name, mname);
		strcpy(table[n_dev].dname, dname);

		if (stat(table[n_dev].name, &statb) == -1)
			continue;

		if ((statb.st_mode & S_IFMT) != S_IFCHR)
			continue;

		map[n_dev].mse_dev = statb.st_rdev;
		table[n_dev++].mse_fd = -1;
	}

	return;

}

/* 
 * void download_table(void)
 * 
 * Copy the table down to the mse driver (it will fill in missing 
 * data, as devices are opened by the controlling processes. 
 */

void
download_table()
{
	struct mse_cfg  mse_cfg;

	mse_cfg.mapping = map;
	mse_cfg.count = n_dev;
	ioctl(mgr_fd, MOUSEIOCCONFIG, &mse_cfg);

	return;
}

/* 
 * int lookup_dev ( void )
 *
 * Search through mousetab mappings for the entry corresponding to 
 * the controlling tty of the process that opened the mouse device, 
 * returned by the ws_getctty() call. 
 * Each mapping holds { terminal dev_t, mouse dev_t, mouse type }
 */

int
lookup_dev()
{
	int	 		idx;
	struct stat	 statb;

	/* Check if table needs to be reloaded */

	if (stat(MOUSETAB, &statb) != -1 && statb.st_mtime > tab_time){
#ifdef DEBUG
		fprintf(stderr,"mousemgr: reloading map table\n");
#endif
		load_table();
	}

	for (idx = 0; idx < n_dev; idx++) {
		if (map[idx].mse_dev == command.mdev)
		return idx;
	}

	return -1;
}

/* 
 * getchan() takes the map[] index of the opening processes ctty, 
 * the device names are expected to be of the form /dev/vtnn. 
 */

char *
getchan(ndx)
register int ndx;
{
	static char		tmp[MAXDEVNAME], tmp1[MAXDEVNAME];
	struct stat		statb;
	char			*end;
	int 			i;

	/* end is last letter of /dev/vt 	*/
	end = strchr(table[ndx].dname, 't');

	/* copy the /dev/vt root into the display name of table[]	*/
	strncpy(tmp, table[ndx].dname,  (end - table[ndx].dname) + 1 );

	tmp[end - table[ndx].dname + 1] = NULL;

	for(i=0;i<15; i++){

		/* copy into buffer	*/

		sprintf(tmp1, "%s%02d", tmp, i );

		/* if file exists 	*/

		if (stat(tmp1, &statb) == 0 ){

			/* if device matches (major/minor)	*/

			if(statb.st_rdev == command.dev){

				/* set disp_vt to index of display specified in mousetab */

				disp_vt = i;
				return(tmp1);
			}

		}
	}

	return((char *) NULL);

}

/* 
 * void do_open ( void )
 * 
 * Mouse driver requests the mousemgr daemon to open the specific mouse 
 * hardware soecified in the command data, whatever that requires. At 
 * minimum, opening the devices node, and the terminal's node, and calling
 * a STREAMS I_PLINK to attach one to another. Some mice (serial) require
 * extra processing (opening ports, I_POP and I_PUSH modules, etc). The 
 * mousemgr daemon has root privileges and can perform various tasks that 
 * would be denied the average user. 
 */

void
do_open()
{
	struct termio   cb;
	int			 idx, flags;
	int	 		fd;
	char			*dispname;
	int	 		xxcompatmode;
	int			b3dly;	

	DBGPRT((stderr,"mousemgr: entered do_open\n"));

	if ((idx = lookup_dev()) < 0) {
		command.errno = ENXIO;
		NFERROR(("mousemgr:do_open: failed lookup_dev()\n"));
		return;
	}

	/* If table entry for the device has a mouse attached, exit	 */

	if(table[idx].mse_fd != -1) { 

		errno = 0;
		NFERROR(("mousemgr:do_open: dev attached to mouse fd %d\n", \
													table[idx].mse_fd)); 
		return;

	}

	DBGPRT((stderr,"mousemgr:%d: idx = %d\n", __LINE__, idx));

	if((dispname = getchan(idx)) == (char *)NULL) {

		command.errno = ENXIO;
		NFERROR(("mousemgr: do_open() - getchan failed\n"));
		return;

	}

	/* open primary display channel */

	if((fd = table[idx].disp_fd[disp_vt] = open(dispname, O_RDWR)) < 0){
		NFERROR(("mousemgr: open dev= %s failed\n",dispname));
		command.errno = errno;
		return;
	}

	DBGPRT(("mousemgr: open mouse= %s \n",table[idx].name));

	/* 
	 * open(2) the mouse device, O_NDELAY prevents blocking. 
	 */

	if((table[idx].mse_fd = open(table[idx].name, O_RDWR|O_NDELAY|O_EXCL))<0){

		NFERROR(("mousemgr: open mouse= %s failed\n",table[idx].name));
		command.errno = errno;
		close(fd);
		table[idx].disp_fd[disp_vt] = 0;
		return;

	}

	DBGPRT(("mousemgr:%d: TYPE = %d\n",table[idx].type));

										
	/* 
 	 * Special case serial mouse processing, open and set serial port
 	 * parameters and pop terminal modules, push serial mouse module.
	 */

	if(table[idx].type == MSERIAL){

		/* 
		 * Polite users save the old terminal state for restoration 
		 * when they complete, even if they open the device O_EXCL 
		 * in which case their success implies a closed (and thus 
		 * unset) terminal port. 
		 */

		if (ioctl(table[idx].mse_fd, TCGETA, &cb) == -1) {
			
			command.errno = errno;
			close(table[idx].mse_fd);
			table[idx].mse_fd = -1;
			NFERROR(("mousemgr: TCGETA failed\n"));
			return;

		}
		
		table[idx].saveterm = cb;
		
		/*
		 * Put the serial device in raw mode at 1200 baud.
		 */

		cb.c_iflag = IGNBRK|IGNPAR;
		cb.c_oflag = 0;
		cb.c_cflag = B1200|CS8|CREAD|CLOCAL|PARENB|PARODD;
		cb.c_lflag = 0;
		cb.c_line = 0;
		cb.c_cc[VMIN] = 1;
		cb.c_cc[VTIME] = 0;

		if (ioctl(table[idx].mse_fd, TCSETAF, &cb) == -1) {
			command.errno = errno;
			close(table[idx].mse_fd);
			table[idx].mse_fd = -1;
			NFERROR(("mousemgr: TCSETAF failed\n"));
			return;
		}

		/* Clear the O_NDELAY flag now port open 	*/
		flags = fcntl(table[idx].mse_fd, F_GETFL, 0);
		fcntl(table[idx].mse_fd, F_SETFL, flags & ~O_NDELAY);

		/* Hook for XENIX compatibility */

		xxcompatmode = ioctl(table[idx].disp_fd[disp_vt], WS_GETXXCOMPAT,0);
		if (xxcompatmode == 1)
			(void) ioctl(table[idx].disp_fd[disp_vt], WS_CLRXXCOMPAT,0);

		/* I_POP all modules from asy stream */
		
		while(ioctl(table[idx].mse_fd, I_POP) != -1)
			;

		if(ioctl(table[idx].mse_fd, I_PUSH, "smse") < 0){

			command.errno = errno;
			NFERROR(("mousemgr: smse I_PUSH failed\n"));
			if (xxcompatmode == 1)
				(void) ioctl(table[idx].disp_fd[disp_vt], WS_SETXXCOMPAT,0);
				return;
			}

		if (xxcompatmode == 1)
			(void) ioctl(table[idx].disp_fd[disp_vt], WS_SETXXCOMPAT,0);

	}	   /* End of MSERIAL specialcasing		 */

	/*
	 * L002 
	 * Third button emulation code. If the device has only two buttons
	 * (in /etc/default/mouse) then set emulation mode on. If we can't 
	 * get the button count, don't give up, just carry on: no emulation
	 * but not the end of the world.
	 */

	if (read_dfile() == 0) { 

		/* 
		 * Ideally, write error message to error log. No log, so
		 * just don't do the emulation stuff. 
		 */

		if (msebtns == 2) {
			b3dly = -1;
		} else {
			b3dly = 0;
		}

		if (ioctl(table[idx].mse_fd,MOUSEIOC3BE,&b3dly) < 0) {

			/* 
			 * If ioctl fails, carry on. If the failure is due to a 
			 * major error then we will fail later anyway, otherwise 
			 * don't kill mouse use due to emulation support failure.
			 */

			NFERROR(("mousemgr: MOUSEIOC3BE on %s failed\n",table[idx].name));
		}

	}

	/* L002 end */

	/* 
 	 * Hook for XENIX compatibility. If XENIX compatibility is enabled
	 * then we should disable it temporarily during the PLINK and 
	 * reenable it afterwards, so the new modules can handle it as 
	 * correct for their new configuration. 
	 */

	xxcompatmode = ioctl(table[idx].disp_fd[disp_vt], WS_GETXXCOMPAT,0);

	if (xxcompatmode == 1)
		(void) ioctl(table[idx].disp_fd[disp_vt], WS_CLRXXCOMPAT,0);

	/* 
 	 * PLINK the mouse and terminal streams together, the terminal 
	 * device is the lower edge of the chanmux MUX module.
	 */

	if((table[idx].linkid = \
		ioctl(table[idx].disp_fd[disp_vt], I_PLINK, table[idx].mse_fd)) < 0){

		command.errno = errno;
		NFERROR(("mousemgr:%d: I_PLINK failed: %d\n",__LINE__,command.errno));
		close(table[idx].mse_fd);
		close(table[idx].disp_fd[disp_vt]);
		table[idx].mse_fd = -1;

		if (xxcompatmode == 1)
			(void) ioctl(table[idx].disp_fd[disp_vt], WS_SETXXCOMPAT,0);
		
		return;

	}


	DBGPRT(("mousemgr: do_open :disp_vt = %x, linkid= %x\n",	\
												disp_vt,table[idx].linkid));

	/* 
	 * Re-enable XENIX compatibility mode if it had been turned on 
	 * before we turned it off to do the I_PLINK
	 */

	if (xxcompatmode == 1)
		(void) ioctl(table[idx].disp_fd[disp_vt], WS_SETXXCOMPAT,0);

	strcpy(table[idx].linkname, dispname);
	table[idx].link_vt = table[idx].disp_fd[disp_vt];

	return;

}

/* 
 * void do_close ( void ) 
 * 
 * Close the mouse device. Tear down the PLINKed streams built earlier
 * and discard all previous setup information. 
 * TODO : special processing for the serial port ?? close(2) actually 
 * is sufficient, this resets the modules etc.
 */

void
do_close()
{
	int		idx;
	char	*dispname;
	char	temp[2];
	int 	xxcompatmode;
	unsigned unlink_retries = 0;			/* L003	*/

	/* Check device correct */

	if ((idx = lookup_dev()) < 0){

		/* Bad device */
		NFERROR(("mousemgr:do_close - lookup_dev failed\n"));
		return;

	}

	/*	
	 * ?? No idea - philk 
	 */

	sprintf(temp,"");		/* hack for weird getchan() behavior */

	/* Find associated display devices	*/

	if((dispname = getchan(idx)) == (char *)NULL){

		command.errno = ENXIO;
		NFERROR(("mousemgr:do_close() - getchan failed\n"));
		return;
	}

	/* Check mouse device fd entry 	*/
	if (table[idx].mse_fd == -1){
		
		NFERROR(("mousemgr:do_close - mse_fd == -1\n"));
		return;
		
	}

	/* If command == LCLOSE close the LINK 	*/

	if(command.cmd & MSE_MGR_LCLOSE ){

unlink_retry:	/* L003 */
		if(strcmp(table[idx].linkname, dispname) != 0 || unlink_retries){	/* L003 */

			close(table[idx].link_vt); 
			table[idx].disp_fd[disp_vt] = open(table[idx].linkname, O_RDWR);
			table[idx].link_vt = table[idx].disp_fd[disp_vt];

		}

		/* Hook for XENIX compatibility */
	
		xxcompatmode = ioctl(table[idx].disp_fd[disp_vt], WS_GETXXCOMPAT,0);
		if (xxcompatmode == 1)
		(void) ioctl(table[idx].disp_fd[disp_vt], WS_CLRXXCOMPAT,0);

		/* 
 		 * Call I_PUNLINK to detach the mouse and terminal streams.
		 */

		if(ioctl(table[idx].disp_fd[disp_vt],I_PUNLINK,table[idx].linkid) < 0){

			command.errno = errno;
			NFERROR(("mousemgr: I_PUNLINK failed - disp_vt=%d, linkid=%x\n",\
													disp_vt,table[idx].linkid));

			if ( errno == ENXIO ) {
							/* L003 vvv */
				if (unlink_retries < UNLINK_MAX_RETRIES) {
					/* don't sleep on first retry	*/
					if (unlink_retries)
						sleep(UNLINK_RETRY_DELAY);
					unlink_retries++;
					goto unlink_retry;
				}
							/* L003 ^^^ */
				close(table[idx].link_vt);
			} else {
				if (xxcompatmode == 1)
					(void) ioctl(table[idx].disp_fd[disp_vt], WS_SETXXCOMPAT,0);
				return;
			}
		}

		if (xxcompatmode == 1)
			(void) ioctl(table[idx].disp_fd[disp_vt], WS_SETXXCOMPAT,0);

		close(table[idx].mse_fd);
		table[idx].mse_fd = -1;
	}

	close(table[idx].disp_fd[disp_vt]);
	table[idx].disp_fd[disp_vt] = -1;
	table[idx].link_vt = -1;

	return;

}
