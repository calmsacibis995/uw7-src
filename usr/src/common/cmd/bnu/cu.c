/*		copyright	"%c%" 	*/

#ident	"@(#)cu.c	1.8"
#ident  "$Header$"

/********************************************************************
 *cu [-cdevice] [-sspeed] [-lline] [-bbits] [-h] [-t] [-d] [-n] 
 *		[-o|-e|-p] telno | systemname
 *
 *	legal baud rates: 300, 1200, 2400, 4800, 9600.
 *
 *	-c is used to specify which device will be used for making the
 *		call.  The device argument is compared to the Type (first)
 *		field in the Devices file, and only those records that
 *		match will be used to make the call.  Either -d or -t
 *		would be more intuitive options designations, but they
 *		are already in use.
 *	-l is for specifying a line unit from the file whose
 *		name is defined in /etc/uucp/Devices.
 *	-b is for forcing the number of bits per character processed on
 *		the connection. Valid values are '7' or '8'.
 *	-h is for half-duplex (local echoing).
 *	-t is for adding CR to LF on output to remote (for terminals).
 *	-d can be used  to get some tracing & diagnostics.
 *	-o or -e or -p is for odd or even or none parity on transmission 
 *			  to remote.
 *	-n will request the phone number from the user.
 *	Telno is a telephone number with `=' for secondary dial-tone.
 *	If "-l dev" is used, speed is taken from /etc/uucp/Devices.
 *	Only systemnames that are included in /etc/uucp/Systems may
 *	be used.
 *
 *	Escape with `~' at beginning of line:
 *
 *	~.	quit,
 *
 *	~![cmd]			execute shell (or 'cmd') locally,
 *
 *	~$cmd			execute 'cmd' locally, stdout to remote,
 *
 *	~%break	(alias ~%b)	transmit BREAK to remote,
 *	~%cd [dir]		change directory to $HOME (or 'dir'),
 *	~%debug (alias ~%d)	toggles on/off the program debug trace,
 *	~%divert		allow unsolicited diversions to files,
 *	~%ifc (alias ~%nostop)	toggles on/off the DC3/DC1 input control,
 *	~%ofc (alias ~%noostop)	toggles on/off the DC3/DC1 output control,
 *		(certain remote systems cannot cope with DC3 or DC1).
 *	~%old			recognize old style silent diversions,
 *	~%put from [to]		put file from local to remote,
 *	~%take from [to]	take file from remote to local,
 *
 *	~l			dump communication line ioctl settings,
 *	~t			dump terminal ioctl settings.
 *
 *	Silent diversions are enabled only for use with the ~%take
 *	command by default for security reasons. Unsolicited diversions
 *	may be enabled using the ~%divert toggle. The 'new-style'
 *	diversion syntax is "~[local]>:filename", and is terminaled
 *	by "~[local]>", where 'local' is the nodename of the local
 *	system. This enables ~%take to operate properly when cu
 *	is used over multiple hops. 'old-style' diversion syntax may
 *	be enabled using the ~%old toggle. ('old-style' diversion
 *	should be avoided!)
 *
 *	cu uses the connection server via dials() to reach the remote.
 *	dials()/the connection server takes care of synchronizing device
 *	usage. undials() is called instead of close() to allow whatever
 *	cleanup is necessary.
 *
 *	cu controls line settings by passing a termio structure to
 *	dials(). The termio values are based on the current terminal
 *	settings, modified by command line options.
 ********************************************************************/

/*  "VERBOSE_CU()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The catalog name the string is output using <severity>.
 */

#include "uucp.h"
#include <dial.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <wait.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>

#define	CU_DEF_FILE	"/etc/uucp/Config"

#define MID	BUFSIZ/2	/* mnemonic */
#define	RUB	'\177'		/* mnemonic */
#define	XON	'\21'		/* mnemonic */
#define	XOFF	'\23'		/* mnemonic */
#define	TTYIN	0		/* mnemonic */
#define	TTYOUT	1		/* mnemonic */
#define	TTYERR	2		/* mnemonic */
#define	YES	1		/* mnemonic */
#define	NO	0		/* mnemonic */
#define	MAXPATH	100
#define	NPL	50

int Sflag=0;
int Cn;				/*fd for remote comm line */
jmp_buf Sjbuf;			/*needed by uucp routines*/

/*	io buffering	*/
/*	Wiobuf contains, in effect, 3 write buffers (to remote, to tty	*/
/*	stdout, and to tty stderr) and Riobuf contains 2 read buffers	*/
/*	(from remote, from tty).  [WR]IOFD decides which one to use.	*/
/*	[RW]iop holds current position in each.				*/
#define WIOFD(fd)	(fd == TTYOUT ? 0 : (fd == Cn ? 1 : 2))
#define RIOFD(fd)	(fd == TTYIN ? 0 : 1)
#define WMASK(fd)	(fd == Cn ? line_mask : term_mask)
#define RMASK(fd)	(fd == Cn ? line_mask : term_mask)
#define VERBOSE_CU(sev,fmt,str)\
	{ if(Verbose>0) pfmt(stderr, sev, fmt, str); }
#define WRIOBSZ 256
static char Riobuf[2*WRIOBSZ];
static char Wiobuf[3*WRIOBSZ];
static int Riocnt[2]={0,0};
static char *Riop[2]; 
static char *Wiop[3]; 

extern int optind;		/* variable in getopt() */

extern char *optarg;

calls_t call;		/* call structure for dials() */
calls_t *ret_call;	/* call structure returned from dials */
dial_status_t dstatus;	/* dialer status returned from dial()*/

static int Saved_tty;		/* was TCGETS of _Tv0 successful?	*/
static struct termios _Tv, _Tv0; /* for saving, changing TTY atributes */
static struct termios _Lv;	/* attributes for the line to remote */
static struct utsname utsn; 
static char prompt[sizeof (struct utsname) + 3]= "[";

static char filename[BUFSIZ] = "/dev/null";

static char
	_Cxc,			/* place into which we do character io*/
	_Tintr,			/* current input INTR */
	_Tquit,			/* current input QUIT */
	_Terase,		/* current input ERASE */
	_Tkill,			/* current input KILL */
	_Teol,			/* current secondary input EOL */
	_Myeof,			/* current input EOF */
	term_mask,		/* mask value for local terminal */
	line_mask;		/* mask value for remote line */
				/* either '0177' or '0377' */

int
	Echoe,			/* save users ECHOE bit */
	Echok,			/* save users ECHOK bit */
	Intrupt=NO,		/* interrupt indicator */
	Ifc=YES,		/* NO means remote can't XON/XOFF */
	Ofc=YES,		/* NO means local can't XON/XOFF */
	Rtn_code=0,		/* default return code */
	Divert=NO,		/* don't allow unsolicited redirection */
	OldStyle=NO,		/* don't handle old '~>:filename' syntax */
				/* this will be mandatory in SVR4.1 */
	Takeflag=NO;		/* indicates a ~%take is in progress */

extern int			/* These are initialized in line.c */
	Terminal,		/* flag; remote is a terminal */
	Oddflag,		/* flag- odd parity option*/
	Evenflag,		/* flag- even parity option*/
	Noneflag,		/* flag- none parity option*/
	Duplex,			/* Unix= full duplex=YES; half = NO */ 
	term_8bit,		/* is terminal set for 8 bit processing */
	line_8bit;		/* is line set for 8 bit processing */

pid_t
	Child,			/* pid for receive process */
	Shell;			/* pid for escape process */

static pid_t
	dofork();		/* fork and return pid */

static int
	r_char(),		/* local io routine */
	w_char(),		/* local io routine */
	wioflsh();

static void
	_onintrpt(),		/* interrupt routines */
	_rcvdead(),
	_quit(),
	_sighand(),
	_bye();

extern void	ttygenbrk();
void	(*genbrk)() = ttygenbrk;
extern void	cleanup();
extern void	tdmp();
static int transmit(), tilda();

static void
	recfork(),
	sysname(),
	blckcnt(),
	_flush(),
	_shell(),
	_dopercen(),
	_receive(),
	_mode(),
	_w_str();

static char *P_USAGE= ":1:Usage: cu [-c device] [-s speed] [-l line] [-b 7|8] [-h] [-n] [-t] [-d] [-x] [-o|-e|-p] telno | systemname\n";
static char *P_CON_FAILED = ":2:connect failed: %s\r\n";
static char *P_Ct_OPEN = ":3:Cannot open: %s\r\n";
static char *P_LINE_GONE = ":4:remote line gone\r\n";
static char *P_Ct_EXSH = ":5:cannot execute shell\r\n";
static char *P_Ct_DIVERT = ":6:Cannot divert to %s\r\n";
static char *P_Ct_UNDIVERT = ":7:Cannot end diversion to %s\r\n";
static char *P_Bad_DIVERT = ":8:Will not divert to %s. Unsolicited.\r\n";
static char *P_STARTWITH = ":9:Use `~~' to start line with `~'\r\n";
static char *P_CNTAFTER = ":10:after %ld bytes\r\n";
static char *P_CNTLINES = ":11:%d lines/";
static char *P_CNTCHAR = ":12:%ld characters\r\n";
static char *P_FILEINTR = ":13:File transmission interrupted\r\n";
static char *P_Ct_FK = ":14:Cannot fork -- try later\r\n";
static char *P_Ct_SPECIAL = ":15:\r\nCannot transmit special character `%#o'\r\n";
static char *P_TOOLONG = ":16:Line too long\r\n";
static char *P_IOERR = ":17:\r\nLine write error\r\n";
static char *P_USECMD = ":18:Use `~$'cmd \r\n"; 
static char *P_USEPLUSCMD =":19:Use `~+'cmd \r\n";
static char *P_TELLENGTH = ":20:telno cannot exceed 58 digits!\r\n";
static char *P_Ct_WRITE = ":21:cannot write to the local terminal\r\n";

/***************************************************************
 *	main: get command line args, establish connection, and fork.
 *	Child invokes "receive" to read from remote & write to TTY.
 *	Main line invokes "transmit" to read TTY & write to remote.
 ***************************************************************/

main(argc, argv)
char *argv[];
{
    char s[MAXPH];
    char *string;
    int i;
    int errflag=0;
    int lflag=0;
    int nflag=0;
    int systemname = 0;
    int code;
    char *errmsg;

    FILE * cu_def_file;
    char cu_def_info[BUFSIZ], *tok;
    int  OddDefFlag=0, EvenDefFlag=0, NoneDefFlag=0 ;

    (void)setlocale(LC_ALL,"");
    (void)setcat("uxcu");
    (void)setlabel("UX:cu");

    Riop[0] = &Riobuf[0];
    Riop[1] = &Riobuf[WRIOBSZ];
    Wiop[0] = &Wiobuf[0];
    Wiop[1] = &Wiobuf[WRIOBSZ];
    Wiop[2] = &Wiobuf[2*WRIOBSZ];

    Verbose = 1;		/*for uucp callers,  dialers feedback*/
    strcpy(Progname,"cu");

	/* save initial tty state */

	Saved_tty = ( ioctl(TTYIN, TCGETS, &_Tv0) >= 0 );

	if (Saved_tty) {
		/* start with line the same as terminal */
		_Lv = _Tv0;
	} else {
		/* set up some default settings for cu */
		_Lv.c_iflag = (IGNPAR | IGNBRK | IXON | IXOFF);
		_Lv.c_oflag = 0;
		_Lv.c_cflag = (CS8 | CREAD | HUPCL);
		_Lv.c_lflag = 0;
		/*_Lv.c_line = 0; */
		_Lv.c_cc[VINTR]  = CINTR;
		_Lv.c_cc[VQUIT]  = CQUIT;
		_Lv.c_cc[VERASE] = CERASE;
		_Lv.c_cc[VKILL]  = CKILL;
		_Lv.c_cc[VSWTCH] = CNSWTCH;
	}

	/* turn off input mappings */
	_Lv.c_iflag &= ~(INLCR | IGNCR | ICRNL | IUCLC);

	/* set up corresponding input flow control */
	if (Ifc = (_Tv0.c_iflag & IXON))
		_Lv.c_iflag |= IXOFF;
	else
		_Lv.c_iflag &= ~IXOFF;

	/* set up corresponding output flow control */
	if (Ofc = (_Tv0.c_iflag & IXOFF))
		_Lv.c_iflag |= IXON;
	else
		_Lv.c_iflag &= ~IXON;

	/* turn off output post-processing */
	_Lv.c_oflag &= ~OPOST;

	/* turn off cannonical processing */
	_Lv.c_lflag &= ~(ISIG | ICANON | ECHO);

	/* set VMIN */
	_Lv.c_cc[VMIN] = 1;

	call.attr = &_Lv;
	call.speed = -1;	/* -1 means Any speed */
	call.device_name = NULL;	/* line name if direct */
	call.telno = NULL;	/* telno */
	call.caller_telno = NULL; /* no caller telno */
	call.sysname = NULL;	/* system name */

	call.function = FUNC_NULL; /* default functions only, no chat */
	call.class = NULL;
	call.protocol = NULL;
	call.pinfo_len = 0;
	call.pinfo = NULL;
	
        cu_def_file = fopen(CU_DEF_FILE, "r");
        if (cu_def_file) {  /* Check the config file for PARITY and CHARSIZE */
 	    while (getline(cu_def_file, cu_def_info)) {
		/* got a line; so ... */
		tok = strtok(cu_def_info, " \t");
		if ( (tok != NULL) && (*tok != '#') ) {
	           if (strcmp(tok,"PARITY=even") == 0) {
		      if ( OddDefFlag || NoneDefFlag ) {
		            (void)pfmt(stderr, MM_ERROR,
		  	    ":76:cannot specify more than one parity setting in \n\t      %s file\n", CU_DEF_FILE);
        		    fclose(cu_def_file);
		            exit(37);
		      }
		      Evenflag = 1; EvenDefFlag = 1;
		      _Lv.c_cflag |= PARENB;
		      _Lv.c_cflag &= ~PARODD;
	           }
	           else if (strcmp(tok,"PARITY=odd") == 0) {
		         if ( EvenDefFlag || NoneDefFlag ) {
		               (void)pfmt(stderr, MM_ERROR,
		  	       ":76:cannot specify more than one parity setting in \n\t      %s file\n", CU_DEF_FILE);
        		       fclose(cu_def_file);
		               exit(37);
		         }
		         Oddflag = 1; OddDefFlag = 1;
		         _Lv.c_cflag |= PARENB;
		         _Lv.c_cflag |= PARODD;
	             }
	           else if (strcmp(tok,"PARITY=none") == 0) {
		         if ( EvenDefFlag || OddDefFlag) {
		               (void)pfmt(stderr, MM_ERROR,
		  	       ":76:cannot specify more than one parity setting in \n\t      %s file\n", CU_DEF_FILE);
        		       fclose(cu_def_file);
		               exit(37);
		         }
		         Noneflag = 1; NoneDefFlag = 1;
		         _Lv.c_cflag &= ~PARENB;
		         _Lv.c_cflag &= ~PARODD;
	             }
	           else if (strcmp(tok,"CHARSIZE=8") == 0) {
		         line_8bit =  YES ;
		         _Lv.c_cflag = (_Lv.c_cflag & ~CSIZE) | (line_8bit ? CS8 : CS7);
	             }
	           else if (strcmp(tok,"CHARSIZE=7") == 0) {
		         line_8bit =  NO ;
		         _Lv.c_cflag = (_Lv.c_cflag & ~CSIZE) | (line_8bit ? CS8 : CS7);
	             }
		}
	    }
	
            fclose(cu_def_file);
        }

/*Flags for -h, -t, -e, and -o options set here; corresponding line attributes*/
/*are set in fixline() in culine.c before remote connection is made	   */

    while((i = getopt(argc, argv, "dhteonps:l:c:b:x")) != EOF)
	switch(i) {
	    case 'd':
		Debug = 9; /*turns on uucp debugging-level 9*/
		cs_debug(TTYERR);
		break;
	    case 'h':
		Duplex  = NO;
		Ifc = NO;
		_Lv.c_iflag &= ~IXOFF;
		Ofc = NO;
		_Lv.c_iflag &= ~IXON;
		break;
	    case 't':
		Terminal = YES;
		_Lv.c_oflag |= (OPOST | ONLCR);
		break;
	    case 'e':
		if (( Oddflag &&  !OddDefFlag ) ||
		    ( Noneflag &&  !NoneDefFlag )) {
		      (void)pfmt(stderr, MM_ERROR,
		  	":77:cannot specify more than one parity setting\n");
		      exit(37);
		    }
		Oddflag = 0; Evenflag = 1; EvenDefFlag = 0;
		Noneflag =0; NoneDefFlag = 0;
		_Lv.c_cflag |= PARENB;
		_Lv.c_cflag &= ~PARODD;
		break;
	    case 'o':
		if (( Evenflag  && !EvenDefFlag ) ||
		    ( Noneflag &&  !NoneDefFlag )) {
		       (void)pfmt(stderr, MM_ERROR,
		  	":77:cannot specify more than one parity setting\n");
		       exit(37);
		    }
		Oddflag = 1; OddDefFlag = 0; Evenflag = 0;
		Noneflag =0; NoneDefFlag = 0;
		_Lv.c_cflag |= PARENB;
		_Lv.c_cflag |= PARODD;
		break;
	    case 'p':
		if (( Evenflag  && !EvenDefFlag ) ||
		    ( Oddflag &&  !OddDefFlag ) ) {
		       (void)pfmt(stderr, MM_ERROR,
		  	":77:cannot specify more than one parity setting\n");
		       exit(37);
		    }
		Oddflag = 0; OddDefFlag = 0; 
		Evenflag = 0; EvenDefFlag = 0;
		Noneflag = 1; NoneDefFlag = 0;

		_Lv.c_cflag &= ~PARENB;
		_Lv.c_cflag &= ~PARODD; 
		break;
	    case 'n':
		nflag++;
		pfmt(stdout, MM_NOSTD,
			":23:Please enter the number: ");
		fgets(s,MAXPH,stdin);
		s[strlen(s)-1] = '\0';
		break;
	    case 's':
		Sflag++;
		call.speed = atoi(optarg);
		tcsetspeed(TCS_ALL,&_Lv,call.speed);
		break;
	    case 'l':
		lflag++;
		call.device_name = optarg;
		break;
	    case 'c':
		call.class = optarg;
		break;
	    case 'b':
		line_8bit = ((*optarg=='7') ? NO : ((*optarg=='8') ? YES : -1));
		if ( line_8bit == -1 ) {
		    (void) pfmt(stderr, MM_ERROR,
			":24:b option value must be '7' or '8'\n");
		    exit(38);
		}
		_Lv.c_cflag = (_Lv.c_cflag & ~CSIZE) | (line_8bit ? CS8 : CS7);
		break;
	    case 'x':
		call.function = FUNC_CHAT; /* run chat */
		break;
	    case '?':
		++errflag;
	}

    if((optind < argc && optind > 0) || (nflag && optind > 0)) {  
	if(nflag) 
	    string=s;
	else
	    string = strdup(argv[optind]);
	call.telno = string;
	if ( strlen(string) != strspn(string, "0123456789=-*#") ) {
	    /* if it's not a legitimate telno, then it should be a systemname */
	    if ( nflag ) {
		(void)pfmt(stderr, MM_ERROR,
		":25:bad phone number <%s>\nPhone numbers may contain only the digits 0 through 9 and the special\ncharacters =, -, * and #.\n",
		 string );
		exit(39);
	    }
	    systemname++;
	    call.sysname = string;
	    call.telno = CNULL;
	}
    } else
	if(call.device_name == CNULL)   /*if none of above, must be direct */
	    ++errflag;
    
    if(errflag) {
	VERBOSE_CU(MM_ACTION, P_USAGE, "");
	exit(40);
    }

    if ((call.telno != CNULL) &&
		(strlen(call.telno) >= (size_t)(MAXPH - 1))) {
	VERBOSE_CU(MM_ERROR, P_TELLENGTH, "");
	exit(41);
    }

    /* I am not sure why  Oddflag, Evenflag are set here as they
       will not be used later. Any way, to be consistent with, 
       setting the Noneflag also.  */

    if (Saved_tty) {
	term_8bit = ( (_Tv0.c_cflag & CS8) && !(_Tv0.c_iflag & ISTRIP) );
	if ( !Oddflag && !Evenflag && !Noneflag ) {
	    if (_Tv0.c_cflag & PARENB)
		if (_Tv0.c_cflag & PARODD)
		    Oddflag = 1;
		else
		    Evenflag = 1;
	    else
		Noneflag = 1;
	}
    }

    if (line_8bit == -1)
	line_8bit = term_8bit;

    term_mask = ( term_8bit ? 0377 : 0177 );
    line_mask = ( line_8bit ? 0377 : 0177 );

    /* if not set, use the POSIX disabled designation */
    _Tintr = _Tv0.c_cc[VINTR] ? _Tv0.c_cc[VINTR] : _POSIX_VDISABLE;
    _Tquit = _Tv0.c_cc[VQUIT] ? _Tv0.c_cc[VQUIT] : _POSIX_VDISABLE;
    _Terase = _Tv0.c_cc[VERASE] ? _Tv0.c_cc[VERASE] : _POSIX_VDISABLE;
    _Tkill = _Tv0.c_cc[VKILL] ? _Tv0.c_cc[VKILL] : _POSIX_VDISABLE;
    _Teol = _Tv0.c_cc[VEOL] ? _Tv0.c_cc[VEOL] : _POSIX_VDISABLE;
    _Myeof = _Tv0.c_cc[VEOF] ? _Tv0.c_cc[VEOF] : CEOF;
    Echoe = _Tv0.c_lflag & ECHOE;
    Echok = _Tv0.c_lflag & ECHOK;

    (void)signal(SIGHUP, _sighand);
    (void)signal(SIGQUIT, _sighand);
    (void)signal(SIGINT, _sighand);

/* place call to system; if "cu systemname", use conn() from uucp
   directly.  Otherwise, use altconn() which dummies in the
   Systems file line.
*/

    if(systemname) {
	if ( lflag )
	    (void)pfmt(stderr, MM_WARNING,
	    ":26:-l flag ignored when system name used\n");
	if ( Sflag )
	    (void)pfmt(stderr, MM_WARNING,
	    ":27:-s flag ignored when system name used\n");
    }
    Cn = dials(&call, &ret_call, &dstatus);
    Euid = geteuid();
    Egid = getegid();
    /* get rid of "uucp"-ness */
    if ((seteuid(getuid()) < 0) || (setegid(getgid()) < 0)) {
	VERBOSE_CU(MM_ERROR,":28:unable to setuid/setgid\n", "");
	cleanup(42);
    }

    if(Cn < 0) {
	code = dialerr(Cn);
	VERBOSE_CU(MM_ERROR, P_CON_FAILED, UERRORTEXT);
	cleanup(code);
    } else {
	struct stat Cnsbuf;
	if ( fstat(Cn, &Cnsbuf) == 0 )
	    Dev_mode = Cnsbuf.st_mode;
	else
	    Dev_mode = R_DEVICEMODE;
	fchmod(Cn, M_DEVICEMODE);
    }

    if(Debug)
	tdmp(Cn); 

    /* At this point succeeded in getting an open communication line	*/
    /* Conn() takes care of closing the Systems file			*/

    (void)signal(SIGINT,_onintrpt);
    _mode(1);			/* put terminal in `raw' mode */
    VERBOSE_CU(MM_NOSTD,":29:Connected\007\r\n%s", "");/*bell!*/

    /*	must catch signals before fork.  if not and if _receive()	*/
    /*	fails in just the right (wrong?) way, _rcvdead() can be		*/
    /*	called and do "kill(getppid(),SIGUSR1);" before parent		*/
    /*	has done calls to signal() after recfork().			*/
    (void)signal(SIGUSR1, _bye);
    (void)signal(SIGHUP, _sighand);
    (void)signal(SIGQUIT, _onintrpt);

    sysname(&prompt[1]);	/* set up system name prompt */
    (void) strcat(prompt, "]");

    recfork();		/* checks for child == 0 */

    if(Child > 0) {
	Rtn_code = transmit();
	_quit(Rtn_code);
    } else {
	cleanup(20);
    }
    /*NOTREACHED*/
}

/*
 *	Kill the present child, if it exists, then fork a new one.
 */

static void
recfork()
{
    int ret, status;
    if (Child) {
	kill(Child, SIGKILL);
	while ( (ret = wait(&status)) != Child )
	    if (ret == -1 && errno != EINTR)
		break;
    }
    Child = dofork();
    if(Child == 0) {
	(void)signal(SIGUSR1, SIG_DFL);
	(void)signal(SIGHUP, _rcvdead);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);

	_receive();	/* This should run until killed */
	/*NOTREACHED*/
    }
    return;
}

/***************************************************************
 *	transmit: copy stdin to remote fd, except:
 *	~.	terminate
 *	~!	local login-style shell
 *	~!cmd	execute cmd locally
 *	~$proc	execute proc locally, send output to line
 *	~%cmd	execute builtin cmd (put, take, or break)
 *	~+proc	execute locally, with stdout to and stdin from line.
 ******************************************************************/

int
transmit()
{
    char b[BUFSIZ];
    register char *p;
    register int escape;
    register int id = 0;  /*flag for systemname prompt on tilda escape*/

    CDEBUG(4,"transmit started\n\r%s", "");

    /* In main loop, always waiting to read characters from	*/
    /* keyboard; writes characters to remote, or to TTYOUT	*/
    /* on a tilda escape					*/

    for (;;) {
	p = b;
	while(r_char(TTYIN) == YES) {
	    if(p == b)  	/* Escape on leading  ~    */
		escape = (_Cxc == '~');
	    if(p == b+1)   	/* But not on leading ~~   */
		escape &= (_Cxc != '~');
	    if(escape) {
		 if(_Cxc == '\n' || _Cxc == '\r' || _Cxc == _Teol) {
		    *p = '\0';
		    if(tilda(b+1) == YES)
			return(0);
		    id = 0;
		    break;
		}
		if(_Cxc == _Tintr || _Cxc == _Tkill || _Cxc == _Tquit ||
			(Intrupt && _Cxc == '\0')) {
		    if(_Cxc == _Tkill) {
			if(Echok)
			    VERBOSE("\r\n%s", "");
		    } else {
			_Cxc = '\r';
			if( w_char(Cn) == NO) {
			    VERBOSE_CU(MM_ERROR, P_LINE_GONE, "");
			    return(45);
			}
			id=0;
		    }
		    break;
		}
		if((p == b+1) && (_Cxc != _Terase) && (!id)) {
		    id = 1;
		    VERBOSE("%s", prompt);
		}
		if(_Cxc == _Terase) { 
		    p = (--p < b)? b:p;
		    if(p > b)
			if(Echoe) {
			    VERBOSE("\b \b%s", "");
			} else 
			    (void)w_char(TTYOUT);
		} else {
		    (void)w_char(TTYOUT);
		    if(p-b < BUFSIZ) 
			*p++ = _Cxc;
		    else {
			putc('\n', stderr);
			VERBOSE_CU(MM_ERROR,P_TOOLONG, "");
			break;
		    }
		}
    /*not a tilda escape command*/
	    } else {
		if(Intrupt && _Cxc == '\0') {
		    CDEBUG(4,"got break in transmit\n\r%s", "");
		    Intrupt = NO;
		    (*genbrk)(Cn);
		    _flush();
		    break;
		}
		if(w_char(Cn) == NO) {
		    VERBOSE_CU(MM_ERROR, P_LINE_GONE, "");
		    return(45);
		}
		if(Duplex == NO) {
		    if((w_char(TTYERR) == NO) || (wioflsh(TTYERR) == NO)) {
			VERBOSE_CU(MM_ERROR, P_Ct_WRITE, "");
			return(46);
		    }
		}
		if ((_Cxc == _Tintr) || (_Cxc == _Tquit) ||
		     ( (p==b) && (_Cxc == _Myeof) ) ) {
		    CDEBUG(4,"got a tintr\n\r%s", "");
		    _flush();
		    break;
		}
		if(_Cxc == '\n' || _Cxc == '\r' ||
		    _Cxc == _Teol || _Cxc == _Tkill) {
		    id=0;
		    Takeflag = NO;
		    break;
		}
		p = (char*)0;
	    }
	}
    }
}

/***************************************************************
 *	routine to halt input from remote and flush buffers
 ***************************************************************/
static void
_flush()
{
    (void)ioctl(TTYOUT, TCXONC, 0);	/* stop tty output */
    (void)Ioctl(Cn, TCFLSH, 0);		/* flush remote input */
    (void)ioctl(TTYOUT, TCFLSH, 1);	/* flush tty output */
    (void)ioctl(TTYOUT, TCXONC, 1);	/* restart tty output */
    if(Takeflag == NO) {
	return;		/* didn't interupt file transmission */
    }
    VERBOSE_CU(MM_ERROR,P_FILEINTR, "");
    (void)sleep(3);
    _w_str("echo '\n~>\n';mesg y;stty echo\n");
    Takeflag = NO;
    return;
}

/**************************************************************
 *	command interpreter for escape lines
 **************************************************************/
int
tilda(cmd)
register char	*cmd;
{

    VERBOSE("\r\n%s", "");
    CDEBUG(4,"call tilda(%s)\r\n", cmd);

    switch(cmd[0]) {
	case CSUSP:
	case CDSUSP:
	    _mode(0);
	    kill(cmd[0] == CDSUSP ? getpid() : (pid_t) 0, SIGTSTP);
	    _mode(1);
	    break;
	case '.':
/*	    if(call.telno == CNULL)            */
		if(cmd[1] != '.') {
		    _w_str("\04\04\04\04\04");
		    if (Child)
			kill(Child, SIGKILL);
		    (void) Ioctl (Cn, TCGETS, &_Lv);
		    if (_Lv.c_cflag & CLOCAL)
		    	_Lv.c_cflag &= ~CLOCAL;
		    /* speed to zero for hangup */ 
		    tcsetspeed(TCS_ALL, &_Lv, 0);
		    (void) Ioctl (Cn, TCSETSW, &_Lv);
		    (void) sleep (2);
		}
	    return(YES);
	case '!':
	    _shell(cmd);	/* local shell */
	    VERBOSE("\r%c\r\n", *cmd);
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    break;
	case '$':
	    if(cmd[1] == '\0') {
		VERBOSE_CU(MM_ERROR,P_USECMD,"");
		VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    } else {
		_shell(cmd);	/*Local shell  */
		VERBOSE("\r%c\r\n", *cmd);
	    }
	    break;	

	case '+':
	    if(cmd[1] == '\0') {
		VERBOSE_CU(MM_ERROR,P_USEPLUSCMD, "");
		VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    } else {
		/* must suspend receive() to use line for command */
		kill(Child, SIGKILL);
		_shell(cmd);	/* Local shell */
		recfork();
		VERBOSE("\r%c\r\n", *cmd);
	    }
	    break;

	case '%':
	    _dopercen(++cmd);
	    break;
	    
	case 't':
	    tdmp(TTYIN);
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    break;
	case 'l':
	    tdmp(Cn);
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    break;
	
	default:
	    VERBOSE_CU(MM_ERROR,P_STARTWITH,"");
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    break;
    }
    return(NO);
}

/***************************************************************
 *	The routine "shell" takes an argument starting with
 *	either "!", "$", or "+" and terminated with '\0'.
 *	If $arg, arg is the name of a local shell file which
 *	is executed. For "!" both stdin and stdout are attached
 *	to the terminal. For "$" stdin is attached to the
 *	terminal but stdout is attached to the line. For "+"
 *	both stdin and stdout are attached to the line.
 *	In any case, '^D' will kill the escape status.
 **************************************************************/

static void
_shell(str)
char	*str;
{
    pid_t	fk, w_ret;
    void	(*xx)(), (*yy)();

    CDEBUG(4,"call _shell(%s)\r\n", str);
    fk = dofork();
    if(fk < 0)
	return;
    Shell = fk;
    _mode(0);	/* restore normal tty attributes */
    xx = signal(SIGINT, SIG_IGN);
    yy = signal(SIGQUIT, SIG_IGN);
    if(fk == 0) {
	char *shell;

	if( (shell = getenv("SHELL")) == NULL)
	    /* use default if user's shell is not set */
	    shell = SHELL;

	/***********************************************
	 * Hook-up our "standard output"
	 * to the tty for '!' or the line
	 * for '$' or '+'.
	 **********************************************/

	(void)close(TTYOUT);
	if (*str == '!')
		(void)fcntl(TTYERR, F_DUPFD, TTYOUT);
	else
		(void)fcntl(Cn, F_DUPFD, TTYOUT);

	/*************************************************
	 * Hook-up "standard input" to the line for '+'.
	 * Otherwise leave "standard input" from the
	 * terminal.
	 ***********************************************/

	if (*str == '+') {
	    (void)close(TTYIN);
	    (void)fcntl(Cn,F_DUPFD,TTYIN);
	}

	(void)close(Cn);   	/*parent still has Cn*/
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGHUP, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGUSR1, SIG_DFL);
	if(*++str == '\0')
	    (void)execl(shell,shell,(char*) 0,(char*) 0,(char *) 0);
	else
	    (void)execl(shell,"sh","-c",str,(char *) 0);
	VERBOSE_CU(MM_ERROR, P_Ct_EXSH, "");
	exit(9);
    }
    while ((w_ret = wait((int*)0)) != fk)
	if (w_ret == -1 && errno != EINTR)
	    break;
    Shell = 0;
    (void)signal(SIGINT, xx);
    (void)signal(SIGQUIT, yy);
    _mode(1);
    return;
}


/***************************************************************
 *	This function implements the 'put', 'take', 'break', 
 *	'ifc' (aliased to nostop) and 'ofc' (aliased to noostop)
 *	commands which are internal to cu.
 ***************************************************************/

static void
_dopercen(cmd)
register char *cmd;
{
    char	*arg[5];
    char	*getpath;
    char	mypath[MAXPATH];
    int	narg;

    blckcnt((long)(-1));

    CDEBUG(4,"call _dopercen(\"%s\")\r\n", cmd);

    arg[narg=0] = strtok(cmd, " \t\n");

    /* following loop breaks out the command and args */
    while((arg[++narg] = strtok((char*) NULL, " \t\n")) != NULL) {
	if(narg < 4)
	    continue;
	else
	    break;
    }

    /* ~%take file option */
    if(EQUALS(arg[0], "take")) {
	if(narg < 2 || narg > 3) {
	    VERBOSE_CU(MM_ACTION,
		":31:Usage: ~%%take from [to]\r\n%s", "");
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    return;
	}
	if(narg == 2)
	    arg[2] = arg[1];
	(void) strcpy(filename, arg[2]);
	recfork();	/* fork so child (receive) knows filename */

	/*
	 * be sure that the remote file (arg[1]) exists before 
	 * you try to take it.   otherwise, the error message from
	 * cat will wind up in the local file (arg[2])
	 *
	 * what we're doing is:
	 *	stty -echo; \
	 *	if test -r arg1
	 *	then (echo '~[local]'>arg2; cat arg1; echo '~[local]'>)
	 *	else echo can't open: arg1
	 *	fi; \
	 *	stty echo
	 *
	 */
	_w_str("stty -echo;if test -r ");
	_w_str(arg[1]);
	_w_str("; then (echo '~");
	_w_str(prompt);
	_w_str(">'");
	_w_str(arg[2]);
	_w_str(";cat ");
	_w_str(arg[1]);
	_w_str(";echo '~");
	_w_str(prompt);
	_w_str(">'); else echo cant\\'t open: ");
	_w_str(arg[1]);
	_w_str("; fi;stty echo\n");
	Takeflag = YES;
	return;
    }
    /* ~%put file option*/
    if(EQUALS(arg[0], "put")) {
	FILE	*file;
	char	ch, buf[BUFSIZ], spec[NCC+1], *b, *p, *q;
	int	i, j, len, tc=0, lines=0;
	long	chars=0L;

	if(narg < 2 || narg > 3) {
	    VERBOSE_CU(MM_ACTION,
		":32:Usage: ~%%put from [to]\r\n%s", "");
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    return;
	}
	if(narg == 2)
	    arg[2] = arg[1];

	if((file = fopen(arg[1], "r")) == NULL) {
	    VERBOSE_CU(MM_ERROR,P_Ct_OPEN, arg[1]);
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    return;
	}
	/*
	 * if cannot write into file on remote machine, write into
	 * /dev/null
	 *
	 * what we're doing is:
	 *	stty -echo
	 *	(cat - > arg2) || cat - > /dev/null
	 *	stty echo
	 */
	_w_str("stty -echo;(cat - >");
	_w_str(arg[2]);
	_w_str(")||cat - >/dev/null;stty echo\n");
	Intrupt = NO;
	for(i=0,j=0; i < NCC; ++i)
	    if((ch=_Tv0.c_cc[i]) != '\0')
		spec[j++] = ch;
	spec[j] = '\0';
	_mode(2);	/*accept interrupts from keyboard*/
	(void)sleep(5);	/*hope that w_str info digested*/

	/* Read characters line by line into buf to write to	*/
	/* remote with character and line count for blckcnt	*/
	while(Intrupt == NO &&
		fgets(b= &buf[MID],MID,file) != NULL) {
	    /* worse case is each char must be escaped*/
	    len = strlen(b);
	    chars += len;		/* character count */
	    p = b;
	    while(q = strpbrk(p, spec)) {
		if(*q == _Tintr || *q == _Tquit || *q == _Teol) {
		    VERBOSE_CU(MM_NOSTD,P_Ct_SPECIAL, *q);
		    (void)strcpy(q, q+1);
		    Intrupt = YES;
		} else {
		    b = strncpy(b-1, b, q-b);
		    *(q-1) = '\\';
		}
		p = q+1;
	    }
	    if((tc += len) >= MID) {
		(void)sleep(1);
		tc = len;
	    }
	    if(write(Cn, b, (unsigned)strlen(b)) < 0) {
		VERBOSE_CU(MM_ERROR,P_IOERR,"");
		Intrupt = YES;
		break;
	    }
	    ++lines;		/* line count */
	    blckcnt((long)chars);
	}
	_mode(1);
	blckcnt((long)(-2));		/* close */
	(void)fclose(file);
	if(Intrupt == YES) {
	    Intrupt = NO;
	    VERBOSE_CU(MM_ERROR,P_FILEINTR,"");
	    _w_str("\n");
	    VERBOSE_CU(MM_NOSTD,P_CNTAFTER, ++chars);
	} else {
	    VERBOSE_CU(MM_NOSTD,P_CNTLINES, lines);
	    VERBOSE_CU(MM_NOSTD, P_CNTCHAR, chars);
	}
	(void)sleep(3);
	_w_str("\04");
	return;
    }

	/*  ~%b or ~%break  */
    if(EQUALS(arg[0], "b") || EQUALS(arg[0], "break")) {
	(*genbrk)(Cn);
	return;
    }
	/*  ~%d or ~%debug toggle  */
    if(EQUALS(arg[0], "d") || EQUALS(arg[0], "debug")) {
	if(Debug == 0)
	    Debug = 9;
	else
	    Debug = 0;
	VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	return;
    }
	/*  ~%[ifc|nostop]  toggles start/stop input control  */
    if( EQUALS(arg[0], "ifc") || EQUALS(arg[0], "nostop") ) {
	(void)Ioctl(Cn, TCGETS, &_Tv);
	Ifc = !Ifc;
	if(Ifc == YES)
	    _Tv.c_iflag |= IXOFF;
	else
	    _Tv.c_iflag &= ~IXOFF;
	(void)Ioctl(Cn, TCSETSW, &_Tv);
	_mode(1);
	VERBOSE_CU(MM_ERROR,":33:(ifc %s)",
		(Ifc ? gettxt(":34","enabled") : gettxt(":35","disabled")));
	VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	return;
    }
	/*  ~%[ofc|noostop]  toggles start/stop output control  */
    if( EQUALS(arg[0], "ofc") || EQUALS(arg[0], "noostop") ) {
	(void)Ioctl(Cn, TCGETS, &_Tv);
	Ofc = !Ofc;
	if(Ofc == YES)
	    _Tv.c_iflag |= IXON;
	else
	    _Tv.c_iflag &= ~IXON;
	(void)Ioctl(Cn, TCSETSW, &_Tv);
	_mode(1);
	VERBOSE_CU(MM_ERROR,":36:(ofc %s)",
		(Ofc ? gettxt(":34","enabled") : gettxt(":35","disabled")));
	VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	return;
    }
	/*  ~%divert toggles unsolicited redirection security */
    if( EQUALS(arg[0], "divert") ) {
	Divert = !Divert;
	recfork();	/* fork a new child so it knows about change */
	VERBOSE_CU(MM_ERROR,":37:(unsolicited diversion %s)",
		(Divert ? gettxt(":34","enabled") : gettxt(":35","disabled")));
	VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	return;
    }
	/*  ~%old toggles recognition of old-style '~>:filename' */
    if( EQUALS(arg[0], "old") ) {
	OldStyle = !OldStyle;
	recfork();	/* fork a new child so it knows about change */
	VERBOSE_CU(MM_ERROR,":38:(old-style diversion %s)",
		(OldStyle ? gettxt(":34","enabled") : gettxt(":35","disabled")));
	VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	return;
    }
	/* Change local current directory */
    if(EQUALS(arg[0], "cd")) {
	if (narg < 2) {
 	    if((getpath = getenv("HOME")) == NULL)
 		getpath = "";
	    strcpy(mypath, getpath);
	    if(chdir(mypath) < 0) {
		VERBOSE_CU(MM_ERROR,
			":39:Cannot change to %s\r\n", mypath);
		VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
		return;
	    }
	} else if (chdir(arg[1]) < 0) {
	    VERBOSE_CU(MM_ERROR,":39:Cannot change to %s\r\n", arg[1]);
	    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	    return;
	}
	recfork();	/* fork a new child so it knows about change */
	VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
	return;
    }
    if (arg[0]) {
	VERBOSE_CU(MM_ERROR, ":40:~%%%s unknown to cu\r\n", arg[0]);
    } else {
	VERBOSE_CU(MM_ERROR, ":40:~%%%s unknown to cu\r\n","");
    }
    VERBOSE_CU(MM_NOSTD,":30:(continue)%s", "");
    return;
}

/***************************************************************
 *	receive: read from remote line, write to fd=1 (TTYOUT)
 *	catch:
 *	~>[>]:file
 *	.
 *	. stuff for file
 *	.
 *	~>	(ends diversion)
 ***************************************************************/

static void
_receive()
{
    register silent = NO, file = -1;
    register char *p;
    int	tic = 0;
    int for_me = NO;
    char	b[BUFSIZ];
    char	*b_p;
    long	count = 0;

    CDEBUG(4,"_receive started\r\n%s", "");

    b[0] = '\0';
    b_p = p = b;

    while(r_char(Cn) == YES) {
	if(silent == NO)    /* ie., if not redirecting from screen */
	    if(w_char(TTYOUT) == NO) {
		VERBOSE_CU(MM_ERROR, P_Ct_WRITE, "");
		_rcvdead(46);    /* this will exit */
	    }
	/* remove CR's and fill inserted by remote */
	if(_Cxc == '\0' || _Cxc == RUB || _Cxc == '\r')
	    continue;
	*p++ = _Cxc;
	if(_Cxc != '\n' && (p-b) < BUFSIZ)
	    continue;
	/* ****************************************** */
	/* This code deals with ~%take file diversion */
	/* ****************************************** */
	if (b[0] == '~') {
	    int    append;

	    if (EQUALSN(&b[1],prompt,strlen(prompt))) {
		b_p = b + 1 + strlen(prompt);
		for_me = YES;
	    } else {
		b_p = b + 1;
		for_me = NO;
	    }
	    if ( (for_me || OldStyle) && (*b_p == '>') ) {
		/* This is an acceptable '~[uname]>' line */
		b_p++;
		if ( (*b_p == '\n') && (silent == YES) ) {
		    /* end of diversion */
		    *b_p = '\0';
		    (void) strcpy(filename, "/dev/null");
		    if ( file >= 0 && close(file) ) {
			VERBOSE_CU(MM_ERROR,P_Ct_UNDIVERT, b_p);
			pfmt(stderr, MM_ERROR,
				":41:close failed: %s\n", strerror(errno));
			VERBOSE("%s","\r");
		    }
		    silent = NO;
		    blckcnt((long)(-2));
		    VERBOSE("%s\r\n", b);
		    VERBOSE_CU(MM_NOSTD,P_CNTLINES, tic);
		    VERBOSE_CU(MM_NOSTD, P_CNTCHAR, count);
		    file = -1;
		    p = b;
		    continue;
		} else if (*b_p != '\n') {
		    if ( *b_p == '>' ) {
			append = 1;
			b_p++;
		    }
		    if ( (for_me || (OldStyle && (*b_p == ':'))) && (silent == NO) ) {
			/* terminate filename string */
			*(p-1) = '\0';
			if ( *b_p == ':' )
			    b_p++;
			if ( !EQUALS(filename, b_p) ) {
			    if ( !Divert  || !EQUALS(filename, "/dev/null") ) {
				VERBOSE_CU(MM_ERROR,P_Bad_DIVERT, b_p);
				(void) strcpy(filename, "/dev/null");
				append = 1;
			    } else {
				(void) strcpy(filename, b_p);
			    }
			}
			if ( append && ((file=open(filename,O_WRONLY)) >= 0) )
			    (void)lseek(file, 0L, 2);
			else
			    file = creat(filename, PUB_FILEMODE);
			if (file < 0) {
			    VERBOSE_CU(MM_ERROR,P_Ct_DIVERT,filename);
			    pfmt(stderr, MM_ERROR,
				":42:open|creat failed: %s\n", strerror(errno));
			    VERBOSE("%s","\r");
			    (void)sleep(5); /* 10 seemed too long*/
			}
			silent = YES; 
			count = tic = 0;
			p = b;
			continue;
		    }
		}
	    }
	}
	/* Regular data, divert if appropriate */
	if ( silent == YES ) {
	    if ( file >= 0)
		(void)write(file, b, (unsigned)(p-b));
	    count += p-b;	/* tally char count */
	    ++tic;		/* tally lines */
	    blckcnt((long)count);
	}
	p = b;
    }
    /*
     * we used to tell of lost carrier here, but now
     * defer to _bye() so that escape processes are
     * not interrupted.
     */
    _rcvdead(47);
    return;
}

/***************************************************************
 *	change the TTY attributes of the users terminal:
 *	0 means restore attributes to pre-cu status.
 *	1 means set `raw' mode for use during cu session.
 *	2 means like 1 but accept interrupts from the keyboard.
 ***************************************************************/
static void
_mode(arg)
{
    CDEBUG(4,"call _mode(%d)\r\n", arg);
    if(arg == 0) {
	if ( Saved_tty )
	    (void)ioctl(TTYIN, TCSETSW, &_Tv0);
    } else {
	(void)ioctl(TTYIN, TCGETS, &_Tv);
	if(arg == 1) {
	    _Tv.c_iflag &= ~(INLCR | ICRNL | IGNCR | IUCLC);
	    if ( !term_8bit )
		_Tv.c_iflag |= ISTRIP;
	    _Tv.c_oflag |=  OPOST;
	    _Tv.c_oflag &= ~(OLCUC | ONLCR | OCRNL | ONOCR | ONLRET);
	    _Tv.c_lflag &= ~(ICANON | ISIG | ECHO);
	    if(Ifc == NO)
		_Tv.c_iflag &= ~IXON;
	    else
		_Tv.c_iflag |= IXON;
	    if(Ofc == NO)
		_Tv.c_iflag &= ~IXOFF;
	    else
		_Tv.c_iflag |= IXOFF;
	    if(Terminal) {
		_Tv.c_oflag |= ONLCR;
		_Tv.c_iflag |= ICRNL;
	    }
	    _Tv.c_cc[VMIN] = 1;
	    _Tv.c_cc[VTIME] = 0;
	}
	if(arg == 2) {
	    _Tv.c_iflag |= IXON;
	    _Tv.c_lflag |= ISIG;
	}
	(void)ioctl(TTYIN, TCSETSW, &_Tv);
    }
    return;
}


static pid_t
dofork()
{
    register int i;
    pid_t x;

    for(i = 0; i < 6; ++i) {
	if((x = fork()) >= 0) {
	    return(x);
	}
    }

    if(Debug) perror("dofork");

    VERBOSE_CU(MM_ERROR, P_Ct_FK, "");
    return(x);
}

static int
r_char(fd)
{
    int rtn = 1, rfd;
    char *riobuf;

    /* find starting pos in correct buffer in Riobuf	*/
    rfd = RIOFD(fd);
    riobuf = &Riobuf[rfd*WRIOBSZ];

    if (Riop[rfd] >= &riobuf[Riocnt[rfd]]) {
	/* empty read buffer - refill it	*/

	/*	flush any waiting output	*/
	if ( (wioflsh(Cn) == NO ) || (wioflsh(TTYOUT) == NO) )
	    return(NO);

	while((rtn = read(fd, riobuf, WRIOBSZ)) < 0){
	    if(errno == EINTR) {
		/* onintrpt() called asynchronously before this line */
		if(Intrupt == YES) {
		    /* got a BREAK */
		    _Cxc = '\0';
		    return(YES);
		} else {
		    /*a signal other than interrupt*/ 
		    /*received during read*/
		    continue;
		}
	    } else {
		CDEBUG(4,"got read error, not EINTR\n\r%s", "");
		break;			/* something wrong */
	    }
	}
	if (rtn > 0) {
	    /* reset current position in buffer	*/
	    /* and count of available chars		*/
	    Riop[rfd] = riobuf;
	    Riocnt[rfd] = rtn;
	}
    }

    if ( rtn > 0 ) {
	_Cxc = *(Riop[rfd]++) & RMASK(fd);	/* mask off appropriate bits */
	return(YES);
    } else {
	_Cxc = '\0';
	return(NO);
    }
}

static int
w_char(fd)
{
    int wfd;
    char *wiobuf;

    /* find starting pos in correct buffer in Wiobuf	*/
    wfd = WIOFD(fd);
    wiobuf = &Wiobuf[wfd*WRIOBSZ];

    if (Wiop[wfd] >= &wiobuf[WRIOBSZ]) {
	/* full output buffer - flush it */
	if ( wioflsh(fd) == NO )
	    return(NO);
    }
    *(Wiop[wfd]++) = _Cxc & WMASK(fd);	/* mask off appropriate bits */
    return(YES);
}

/* wioflsh	flush output buffer	*/
static int
wioflsh(fd)
int fd;
{
    int wfd;
    char *wiobuf;

    /* find starting pos in correct buffer in Wiobuf	*/
    wfd = WIOFD(fd);
    wiobuf = &Wiobuf[wfd*WRIOBSZ];

    if (Wiop[wfd] > wiobuf) {
	/* there's something in the buffer */
	while(write(fd, wiobuf, (Wiop[wfd] - wiobuf)) < 0) {
	    if(errno == EINTR) {
		if(Intrupt == YES) {
		    VERBOSE_CU(MM_ERROR, ":43:\noutput blocked\r\n%s", "");

		    _quit(48);
		} else
		    continue;	/* alarm went off */
	    } else {
		Wiop[wfd] = wiobuf;
		return(NO);			/* bad news */
	    }
	}
    }
    Wiop[wfd] = wiobuf;
    return(YES);
}


static void
_w_str(string)
register char *string;
{
    int len;

    len = strlen(string);
    if ( write(Cn, string, (unsigned)len) != len )
	VERBOSE_CU(MM_ERROR,P_LINE_GONE,"");
    return;
}

/* ARGSUSED */
static void
_onintrpt(sig)
int sig;
{
    (void)signal(SIGINT, _onintrpt);
    (void)signal(SIGQUIT, _onintrpt);
    Intrupt = YES;
    return;
}

static void
_rcvdead(arg)	/* this is executed only in the receive process */
int arg;
{
    CDEBUG(4,"call _rcvdead(%d)\r\n", arg);
    (void)kill(getppid(), SIGUSR1);
    exit(arg);
    /*NOTREACHED*/
}

static void
_quit(arg)	/* this is executed only in the parent process */
int arg;
{
    CDEBUG(4,"call _quit(%d)\r\n", arg);
    (void)kill(Child, SIGKILL);
    _bye(arg);
    /*NOTREACHED*/
}

static void
_bye(arg)	/* this is executed only in the parent proccess */
int arg;
{
    int status, childstat;
    pid_t obit;
    int rcvexit = 0;

    if ( Shell > 0 )
	while ((obit = wait(&status)) != Shell) {
	    if (obit == -1 && errno != EINTR)
		break;
	    /* _receive (Child) may have ended - check it out */
	    if (obit == Child) {
		childstat = status;
		rcvexit = 1;
	    }
	}

    /* give user customary message after escape command returns */
    CDEBUG(4,"call _bye(%d)\r\n", arg);

    (void)signal(SIGINT, SIG_IGN);
    (void)signal(SIGQUIT, SIG_IGN);

    /* if _receive() ended already, don't wait for it again */
    if (!rcvexit)
	while ((obit = wait(&childstat)) != Child)
	    if (obit == -1 && errno != EINTR)
		break;

    if (arg == SIGUSR1) {
	VERBOSE_CU(MM_INFO, ":44:\r\nLost Carrier\r\n%s", "");
	arg = childstat >> 8;
    }

    VERBOSE_CU(MM_INFO,":45:\r\nDisconnected\007\r\n%s", "");

    cleanup(arg);
/*NOTREACHED*/
}



void
cleanup(code) 	/*this is executed only in the parent process*/
int code;	/*Closes device; removes lock files	  */
{

    CDEBUG(4,"call cleanup(%d)\r\n", code);

    (void) seteuid(Euid);
    (void) setegid(Egid);
    if(Cn > 0) {
    	fchmod(Cn, Dev_mode);
    	(void)undials(Cn, ret_call);
    }

    _mode(0);

    exit(code);		/* code=negative for signal causing disconnect*/
}



void
tdmp(arg)
int arg;
{

struct termios xv;
int i;

    VERBOSE_CU(MM_NOSTD,":46:\rdevice status for fd=%d\r\n", arg);
    VERBOSE_CU(MM_NOSTD,":47:F_GETFL=%o,", fcntl(arg, F_GETFL,1));
    if(ioctl(arg, TCGETS, &xv) < 0) {
	putc('\r', stderr);
	pfmt(stderr, MM_ERROR,
		":48:tdmp for fd=%o: %s\n", arg, strerror(errno));
	return;
    }
    VERBOSE("iflag=`%lo',", xv.c_iflag);
    VERBOSE("oflag=`%lo',", xv.c_oflag);
    VERBOSE("cflag=`%lo',", xv.c_cflag);
    VERBOSE("lflag=`%lo',", xv.c_lflag);
    VERBOSE("cc[0]=`%o',",  xv.c_cc[0]);
    for(i=1; i<NCCS; ++i) {
	VERBOSE("[%d]=", i);
	VERBOSE("`%o',",xv.c_cc[i]);
    }
    VERBOSE("\r\n%s", "");
    return;
}

static void
sysname(name)
char * name;
{

    register char *s;

    if(uname(&utsn) < 0)
	s = "Local";
    else
	s = utsn.nodename;

    strncpy(name,s,strlen(s) + 1);
    *(name + strlen(s) + 1) = '\0';
    return;
}


static void
blckcnt(count)
long count;
{
    static long lcharcnt = 0;
    register long c1, c2;
    register int i;
    char c;

    if(count == (long) (-1)) {	/* initialization call */
	lcharcnt = 0;
	return;
    }
    c1 = lcharcnt/BUFSIZ;
    if(count != (long)(-2)) {	/* regular call */
	c2 = count/BUFSIZ;
	for(i = c1; i++ < c2;) {
	    c = '0' + i%10;
	    write(2, &c, 1);
	    if(i%NPL == 0)
		write(2, "\n\r", 2);
	}
	lcharcnt = count;
    } else {
	c2 = (lcharcnt + BUFSIZ -1)/BUFSIZ;
	if(c1 != c2)
	    write(2, "+\n\r", 3);
	else if(c2%NPL != 0)
	    write(2, "\n\r", 2);
	lcharcnt = 0;
    }
    return;
}

static void
_sighand(sig)
int sig;
{
	VERBOSE_CU(MM_ERROR, ":49:received signal <%d>\n", sig);
	cleanup(29);
}

/*VARARGS*/
/*ARGSUSED*/
void
assert (s1, s2, i1, s3, i2)
char *s1, *s2, *s3;
int i1, i2;
{ 
	fprintf(stderr,"cu: %s %s %d\n", s2, s1, i1);  /* for ASSERT in uucp.h*/
}

/*ARGSUSED*/
void
logent (s1, s2)
char *s1, *s2;
{ }		/* so we can load ulockf() */

/*
 *  given a file pointer and buffer, construct logical line in buffer
 *  (i.e., concatenate lines ending in '\').  return length of line
 *  ASSUMES that buffer is BUFSIZ long!
 */

static int
getline(f, line)
FILE *f;
char *line;
{	char *lptr, *lend;

	lptr = line;
	while (fgets(lptr, (line + BUFSIZ) - lptr, f) != NULL) {
		lend = lptr + strlen(lptr);
		if (lend == lptr || lend[-1] != '\n')	
			/* empty buf or line too long! */
			break;
		*--lend = '\0'; /* lop off ending '\n' */
		if ( lend == line ) /* empty line - ignore */
			continue;
		lptr = lend;
		if (lend[-1] != '\\')
			break;
		/* continuation */
		lend[-1] = ' ';
	}
	return(lptr - line);
}
