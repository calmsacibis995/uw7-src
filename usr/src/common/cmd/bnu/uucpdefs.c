/*		copyright	"%c%" 	*/

#ident	"@(#)uucpdefs.c	1.2"
#ident  "$Header$"

#include "uucp.h"
#include <sys/stat.h>
#include <crypt.h>

/* Configurable parameters */

char	_ProtoCfg[40]="";	/* protocol string from Config file */
char	_KeysCfg[MAXBASENAME]="";
int	_AuthCfg = FALSE;
int	_CryptCfg = X_DES;

/* Non-configurable parameters */

int	Ifn, Ofn;
int 	Sgrades = FALSE;
int	Debug = 0;
int	SizeCheck = 0;		/* Ulimit checking supported flag */
long	RemUlimit = 0;		/* Ulimit of remote if supported */
int	Restart = 0;		/* checkpoint restart supported flag */
uid_t	Uid, Euid;		/* user-id and effective-uid */
uid_t	Gid, Egid;		/* group-id and effective-gid */
long	Ulimit;
mode_t	Dev_mode;		/* save device mode here */
char	Progname[NAMESIZE];
char	Pchar;
char	Grade = 'Z';
char	Rmtname[MAXFULLNAME];
char	JobGrade[MAXBASENAME+1] = { NULLCHAR };
char	RemSpool[MAXFULLNAME];	/* spool subdirectory for remote system */
char	User[MAXFULLNAME];
char	Uucp[NAMESIZE];
char	Loginuser[NAMESIZE];
char	Myname[MAXBASENAME+1];
char	Keys[MAXBASENAME+1];
char	Wrkdir[MAXFULLNAME];
char	Logfile[MAXFULLNAME];
char	*Spool = SPOOL;
char	*Pubdir = PUBDIR;
char	**Env;

extern int	read();
extern int	write();

long	Retrytime = 0;
struct	nstat Nstat;
char	Dc[50];			/* line name				*/
int	Seqn;			/* sequence #				*/
int	Role;
char	*Bnptr;			/* used when BASENAME macro is expanded */
char	Jobid[NAMESIZE] = "";	/* Jobid of current C. file */
int	Uerror;			/* global error code */

int	Verbose = 0;	/* only for cu and ct to change */

/* used for READANY and READSOME macros */
struct stat __s_;

/* messages */
char	*Ct_OPEN =	"CAN'T OPEN";
char	*Ct_WRITE =	"CAN'T WRITE";
char	*Ct_READ =	"CAN'T READ";
char	*Ct_CREATE =	"CAN'T CREATE";
char	*Ct_ALLOCATE =	"CAN'T ALLOCATE";
char	*Ct_LOCK =	"CAN'T LOCK";
char	*Ct_STAT =	"CAN'T STAT";
char	*Ct_CHOWN =	"CAN'T CHOWN";
char	*Ct_CHMOD =	"CAN'T CHMOD";
char	*Ct_LINK =	"CAN'T LINK";
char	*Ct_CHDIR =	"CAN'T CHDIR";
char	*Ct_UNLINK =	"CAN'T UNLINK";
char	*Wr_ROLE =	"WRONG ROLE";
char	*Ct_CORRUPT =	"CAN'T MOVE TO CORRUPTDIR";
char	*Ct_CLOSE =	"CAN'T CLOSE";
char	*Ct_FORK =	"CAN'T FORK";
char	*Fl_EXISTS =	"FILE EXISTS";
char	*Ct_BADOWN =	"BAD OWNER/PERMS";

struct MsgNo UerrorText[] = {
  /* SS_OK			0 */ "uxcu:51","SUCCESSFUL",
  /* SS_NO_DEVICE		1 */ "uxcu:52","NO DEVICES AVAILABLE",
  /* SS_TIME_WRONG		2 */ "uxcu:53","WRONG TIME TO CALL",
  /* SS_INPROGRESS		3 */ "uxcu:54","TALKING",
  /* SS_CONVERSATION		4 */ "uxcu:55","CONVERSATION FAILED",
  /* SS_SEQBAD			5 */ "uxcu:56","BAD SEQUENCE CHECK",
  /* SS_LOGIN_FAILED		6 */ "uxcu:57","LOGIN FAILED",
  /* SS_DIAL_FAILED		7 */ "uxcu:58","DIAL FAILED",
  /* SS_BAD_LOG_MCH		8 */ "uxcu:59","BAD LOGIN/MACHINE COMBINATION",
  /* SS_LOCKED_DEVICE		9 */ "uxcu:60","DEVICE LOCKED",
  /* SS_ASSERT_ERROR		10 */ "uxcu:61","ASSERT ERROR",
  /* SS_BADSYSTEM		11 */ "uxcu:62","SYSTEM NOT IN Systems FILE",
  /* SS_CANT_ACCESS_DEVICE	12 */ "uxcu:63","CANNOT ACCESS DEVICE",
  /* SS_DEVICE_FAILED		13 */ "uxcu:64","DEVICE FAILED",
  /* SS_WRONG_MCH		14 */ "uxcu:65","WRONG MACHINE NAME",
  /* SS_CALLBACK		15 */ "uxcu:66","CALLBACK REQUIRED",
  /* SS_RLOCKED			16 */ "uxcu:67","REMOTE HAS A LCK FILE FOR ME",
  /* SS_RUNKNOWN		17 */ "uxcu:68","REMOTE DOES NOT KNOW ME",
  /* SS_RLOGIN			18 */ "uxcu:69","REMOTE REJECT AFTER LOGIN",
  /* SS_UNKNOWN_RESPONSE	19 */ "uxcu:70","REMOTE REJECT, UNKNOWN MESSAGE",
  /* SS_STARTUP			20 */ "uxcu:71","STARTUP FAILED",
  /* SS_CHAT_FAILED		21 */ "uxcu:72","CALLER SCRIPT FAILED",
  /* SS_CALLBACK_LOOP		22 */ "uxcu:73","CALLBACK REQUIRED - LOOP",
  /* SS_INVOKE_FAILED		23 */ "uxcu:74","INVOKE(SCHEME) FAILED",
  /* SS_CS_PROBLEM		24 */ "uxcu:75","CONNECTION SERVER PROBLEM",
};
