/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/codes.c	1.14.11.1"
#ident  "$Header$"

#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include "pcontrol.h"
#include "ioc.h"

#include <ctype.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <dshm.h>
#include <sys/fstyp.h>

#include <sys/unistd.h>
#if 0
#include <sys/vtoc.h>
#endif

#include <sys/stream.h>
#include <sys/timod.h>
#include <sys/termios.h>
#include <sys/termiox.h>
#ifdef XT_SUPPORT
#include <sys/jioctl.h>
#endif /* XT_SUPPORT */
#include <sys/filio.h>
#include <sys/stropts.h>
#ifdef RFS_SUPPORT
#include <sys/nserve.h>
#include <sys/rf_sys.h>
#endif /* RFS_SUPPORT */
#include <fcntl.h>
#include <sys/termio.h>
#include <sys/stermio.h>
#include <sys/ttold.h>

#include <sys/lock.h>
#include <sys/mount.h>
#include <sys/utssys.h>
#include <sys/sysconfig.h>
#include <sys/statvfs.h>

#include "codes.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"

#define	FCNTLMIN	F_DUPFD
#define	FCNTLMAX	F_RSETLKW
static CONST char * CONST FCNTLname[] = {
	"F_DUPFD",
	"F_GETFD",
	"F_SETFD",
	"F_GETFL",
	"F_SETFL",
	"F_O_GETLK",
	"F_SETLK",
	"F_SETLKW",
	NULL,
	NULL,
	"F_ALLOCSP",
	"F_FREESP",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"F_RSETLK",
	"F_RGETLK",
	"F_RSETLKW"
};

#define	RFSYSMIN	RF_FUMOUNT
#define	RFSYSMAX	RF_DEBUG
static CONST char * CONST RFSYSname[] = {
	"RF_FUMOUNT",
	"RF_SENDUMSG",
	"RF_GETUMSG",
	"RF_LASTUMSG",
	"RF_SETDNAME",
	"RF_GETDNAME",
	"RF_SETIDMAP",
	"RF_FWFD",
	"RF_VFLAG",
	"RF_VERSION",
	"RF_RUNSTATE",
	"RF_TUNEABLE",
	"RF_CLIENTS",
	"RF_RESOURCES",
	"RF_ADVFS",
	"RF_UNADVFS",
	"RF_START",
	"RF_STOP",
	"RF_DEBUG"
};

#define	SYSFSMIN	GETFSIND
#define	SYSFSMAX	GETNFSTYP
static CONST char * CONST SYSFSname[] = {
	"GETFSIND",
	"GETFSTYP",
	"GETNFSTYP"
};

#define	PLOCKMIN	UNLOCK
#define	PLOCKMAX	DATLOCK
static CONST char * CONST PLOCKname[] = {
	"UNLOCK",
	"PROCLOCK",
	"TXTLOCK",
	 NULL,
	"DATLOCK"
};

#define	SCONFMIN	_CONFIG_NGROUPS
#define	SCONFMAX	_CONFIG_CACHE_LINE
static CONST char * CONST SCONFname[] = {
	"NGROUPS",
	"CHILD_MAX",
	"OPEN_FILES",
	"POSIX_VER",
	"PAGESIZE",
	"CLK_TCK",
	"XOPEN_VER",
	"NACLS_MAX",
	"ARG_MAX",
	"NPROC",
	"NENGINE",
	"NENGINE_ONLN",
	"TOTAL_MEMORY",
	"USEABLE_MEMORY",
	"GENERAL_MEMORY",
	"DEDICATED_MEMORY",
	"NCGS_CONF",
	"NCGS_ONLN",
	"MAX_ENG_PER_CG",
	"CACHE_LINE",
};

#define	PATHCONFMIN	_PC_LINK_MAX
#define	PATHCONFMAX	_PC_CHOWN_RESTRICTED
static CONST char * CONST PATHCONFname[] = {
	"LINK_MAX",
	"MAX_CANON",
	"MAX_INPUT",
	"NAME_MAX",
	"PATH_MAX",
	"PIPE_BUF",
	"NO_TRUNC",
	"VDISABLE",
	"CHOWN_RESTRICTED"
};

CONST char *
ioctlname(code)
register int code;
{
	register CONST struct ioc *ip;
	register CONST char * str = NULL;
	register int c;

	for (ip = &ioc[0]; ip->name; ip++) {
		if (code == ip->code) {
			str = ip->name;
			break;
		}
	}

	if (str == NULL) {
		c = code >> 8;
		if (isascii(c) && isprint(c))
			(void) sprintf(code_buf, "(('%c'<<8)|%d)",
				c, code & 0xff);
		else
			(void) sprintf(code_buf, "0x%.4X", code);
		str = code_buf;
	}

	return str;
}

CONST char *
fcntlname(code)
register int code;
{
	register CONST char * str = NULL;

	if (code >= FCNTLMIN && code <= FCNTLMAX)
		str = FCNTLname[code-FCNTLMIN];
	return str;
}

CONST char *
sfsname(code)
register int code;
{
	register CONST char * str = NULL;

	if (code >= SYSFSMIN && code <= SYSFSMAX)
		str = SYSFSname[code-SYSFSMIN];
	return str;
}

CONST char *
plockname(code)
register int code;
{
	register CONST char * str = NULL;

	if (code >= PLOCKMIN && code <= PLOCKMAX)
		str = PLOCKname[code-PLOCKMIN];
	return str;
}

#ifdef RFS_SUPPORT
CONST char *
rfsysname(code)
register int code;
{
	register CONST char * str = NULL;

	if (code >= RFSYSMIN && code <= RFSYSMAX)
		str = RFSYSname[code-RFSYSMIN];
	return str;
}
#endif /* RFS_SUPPORT */

CONST char *
utscode(code)
register int code;
{
	register CONST char * str = NULL;

	switch (code) {
	case UTS_UNAME:		str = "UNAME";	break;
	case UTS_USTAT:		str = "USTAT";	break;
	case UTS_FUSERS:	str = "FUSERS";	break;
	}

	return str;
}

CONST char *
sconfname(code)
register int code;
{
	register CONST char * str = NULL;

	if (code >= SCONFMIN && code <= SCONFMAX)
		str = SCONFname[code-SCONFMIN];
	return str;
}

CONST char *
pathconfname(code)
register int code;
{
	register CONST char * str = NULL;

	if (code >= PATHCONFMIN && code <= PATHCONFMAX)
		str = PATHCONFname[code-PATHCONFMIN];
	return str;
}

CONST char *
sigarg(arg)
register int arg;
{
	register char * str = NULL;
	register int sig = (arg & SIGNO_MASK);

	str = code_buf;
	arg &= ~SIGNO_MASK;
	if (arg & ~(SIGDEFER|SIGHOLD|SIGRELSE|SIGIGNORE|SIGPAUSE))
		(void) sprintf(str, "%s|0x%X", signame(sig), arg);
	else {
		(void) strcpy(str, signame(sig));
		if (arg & SIGDEFER)
			(void) strcat(str, "|SIGDEFER");
		if (arg & SIGHOLD)
			(void) strcat(str, "|SIGHOLD");
		if (arg & SIGRELSE)
			(void) strcat(str, "|SIGRELSE");
		if (arg & SIGIGNORE)
			(void) strcat(str, "|SIGIGNORE");
		if (arg & SIGPAUSE)
			(void) strcat(str, "|SIGPAUSE");
	}

	return (CONST char *)str;
}

CONST char *
openarg(arg)
register int arg;
{
	register char * str = code_buf;

	switch (arg & ~(O_NDELAY|O_APPEND|O_SYNC|O_CREAT|O_TRUNC|O_EXCL))
	{
	default:
		return (char *)NULL;
	case O_RDONLY:
		(void) strcpy(str, "O_RDONLY");
		break;
	case O_WRONLY:
		(void) strcpy(str, "O_WRONLY");
		break;
	case O_RDWR:
		(void) strcpy(str, "O_RDWR");
		break;
	}

	if (arg & O_NDELAY)
		(void) strcat(str, "|O_NDELAY");
	if (arg & O_APPEND)
		(void) strcat(str, "|O_APPEND");
	if (arg & O_SYNC)
		(void) strcat(str, "|O_SYNC");
	if (arg & O_CREAT)
		(void) strcat(str, "|O_CREAT");
	if (arg & O_TRUNC)
		(void) strcat(str, "|O_TRUNC");
	if (arg & O_EXCL)
		(void) strcat(str, "|O_EXCL");

	return (CONST char *)str;
}

#define	IPC_FLAGS	(IPC_ALLOC|IPC_CREAT|IPC_EXCL|IPC_NOWAIT)

static char *
ipcflags(arg)
register int arg;
{
	register char * str = code_buf;

	if (arg&0777)
		(void) sprintf(str, "0%.3o", arg&0777);
	else
		*str = '\0';

	if (arg & IPC_ALLOC)
		(void) strcat(str, "|IPC_ALLOC");
	if (arg & IPC_CREAT)
		(void) strcat(str, "|IPC_CREAT");
	if (arg & IPC_EXCL)
		(void) strcat(str, "|IPC_EXCL");
	if (arg & IPC_NOWAIT)
		(void) strcat(str, "|IPC_NOWAIT");

	return str;
}

CONST char *
msgflags(arg)
register int arg;
{
	register char * str;

	if (arg == 0 || (arg & ~(IPC_FLAGS|MSG_NOERROR|0777)) != 0)
		return (char *)NULL;

	str = ipcflags(arg);

	if (arg & MSG_NOERROR)
		(void) strcat(str, "|MSG_NOERROR");

	if (*str == '|')
		str++;
	return (CONST char *)str;
}

CONST char *
semflags(arg)
register int arg;
{
	register char * str;

	if (arg == 0 || (arg & ~(IPC_FLAGS|SEM_UNDO|0777)) != 0)
		return (char *)NULL;

	str = ipcflags(arg);

	if (arg & SEM_UNDO)
		(void) strcat(str, "|SEM_UNDO");

	if (*str == '|')
		str++;
	return (CONST char *)str;
}

CONST char *
shmflags(arg)
register int arg;
{
	register char * str;

	if (arg == 0 || (arg & ~(IPC_FLAGS|SHM_RDONLY|SHM_RND|0777)) != 0)
		return (char *)NULL;

	str = ipcflags(arg);

	if (arg & SHM_RDONLY)
		(void) strcat(str, "|SHM_RDONLY");
	if (arg & SHM_RND)
		(void) strcat(str, "|SHM_RND");

	if (*str == '|')
		str++;
	return (CONST char *)str;
}

CONST char *
dshmflags(arg)
register int arg;
{
	register char * str;

	if (arg == 0 || (arg & ~(IPC_FLAGS|SHM_RDONLY|0777)) != 0)
		return (char *)NULL;
	
	str = ipcflags(arg);

	if (arg & SHM_RDONLY)
		(void) strcat(str, "DSHM_RDONLY");

	return (CONST char *)str;
}

#define	MSGCMDMIN	IPC_RMID
#define	MSGCMDMAX	IPC_STAT
static CONST char * CONST MSGCMDname[MSGCMDMAX+1] = {
	"IPC_RMID",
	"IPC_SET",
	"IPC_STAT",
};

#define	DSHMCMDMIN	IPC_RMID
#define	DSHMCMDMAX	IPC_STAT
static CONST char * CONST DSHMCMDname[DSHMCMDMAX+1] = {
	"IPC_RMID",
	"IPC_SET",
	"IPC_STAT",
};

#define	SEMCMDMIN	IPC_RMID
#define	SEMCMDMAX	SETALL
static CONST char * CONST SEMCMDname[SEMCMDMAX+1] = {
	"IPC_RMID",
	"IPC_SET",
	"IPC_STAT",
	"GETNCNT",
	"GETPID",
	"GETVAL",
	"GETALL",
	"GETZCNT",
	"SETVAL",
	"SETALL",
};

#define	SHMCMDMIN	IPC_RMID
#ifdef	SHM_UNLOCK
#	define	SHMCMDMAX	SHM_UNLOCK
#else
#	define	SHMCMDMAX	IPC_STAT
#endif
static CONST char * CONST SHMCMDname[SHMCMDMAX+1] = {
	"IPC_RMID",
	"IPC_SET",
	"IPC_STAT",
#ifdef	SHM_UNLOCK
	"SHM_LOCK",
	"SHM_UNLOCK",
#endif
};

CONST char *
msgcmd(arg)
register int arg;
{
	register CONST char * str = NULL;

	if (arg >= MSGCMDMIN && arg <= MSGCMDMAX)
		str = MSGCMDname[arg-MSGCMDMIN];
	return str;
}

CONST char *
semcmd(arg)
register int arg;
{
	register CONST char * str = NULL;

	if (arg >= SEMCMDMIN && arg <= SEMCMDMAX)
		str = SEMCMDname[arg-SEMCMDMIN];
	return str;
}

CONST char *
shmcmd(arg)
register int arg;
{
	register CONST char * str = NULL;

	if (arg >= SHMCMDMIN && arg <= SHMCMDMAX)
		str = SHMCMDname[arg-SHMCMDMIN];
	return str;
}

CONST char *
dshmcmd(arg)
register int arg;
{
	register CONST char * str = NULL;

	if (arg >= DSHMCMDMIN && arg <= DSHMCMDMAX)
		str = DSHMCMDname[arg-DSHMCMDMIN];
	return str;
}

CONST char *
strrdopt(arg)		/* streams read option (I_SRDOPT I_GRDOPT) */
register int arg;
{
	register CONST char * str = NULL;

	switch (arg) {
	case RNORM:	str = "RNORM";		break;
	case RMSGD:	str = "RMSGD";		break;
	case RMSGN:	str = "RMSGN";		break;
	}

	return str;
}

CONST char *
strevents(arg)		/* bit map of streams events (I_SETSIG & I_GETSIG) */
register int arg;
{
	register char * str = code_buf;

	if (arg & ~(S_INPUT|S_HIPRI|S_OUTPUT|S_MSG|S_ERROR|S_HANGUP))
		return (char *)NULL;

	*str = '\0';
	if (arg & S_INPUT)
		(void) strcat(str, "|S_INPUT");
	if (arg & S_HIPRI)
		(void) strcat(str, "|S_HIPRI");
	if (arg & S_OUTPUT)
		(void) strcat(str, "|S_OUTPUT");
	if (arg & S_MSG)
		(void) strcat(str, "|S_MSG");
	if (arg & S_ERROR)
		(void) strcat(str, "|S_ERROR");
	if (arg & S_HANGUP)
		(void) strcat(str, "|S_HANGUP");

	return (CONST char *)(str+1);
}

CONST char *
strflush(arg)		/* streams flush option (I_FLUSH) */
register int arg;
{
	register CONST char * str = NULL;

	switch (arg) {
	case FLUSHR:	str = "FLUSHR";		break;
	case FLUSHW:	str = "FLUSHW";		break;
	case FLUSHRW:	str = "FLUSHRW";	break;
	}

	return str;
}

CONST char *
mountflags(arg)		/* bit map of mount syscall flags */
register int arg;
{
	register char * str = code_buf;

	if (arg & ~(MS_RDONLY|MS_FSS|MS_DATA|MS_NOSUID|MS_REMOUNT))
		return (char *)NULL;

	*str = '\0';
	if (arg & MS_RDONLY)
		(void) strcat(str, "|MS_RDONLY");
	if (arg & MS_FSS)
		(void) strcat(str, "|MS_FSS");
	if (arg & MS_DATA)
		(void) strcat(str, "|MS_DATA");
	if (arg & MS_NOSUID)
		(void) strcat(str, "|MS_NOSUID");
	if (arg & MS_REMOUNT)
		(void) strcat(str, "|MS_REMOUNT");
	return *str? (CONST char *)(str+1) : "0";
}

CONST char *
svfsflags(arg)		/* bit map of statvfs syscall flags */
register int arg;
{
	register char * str = code_buf;

	if (arg & ~(ST_RDONLY|ST_NOSUID|ST_NOTRUNC)) {
		(void) sprintf(str, "0x%x", arg);
		return str;
	}
	*str = '\0';
	if (arg & ST_RDONLY)
		(void) strcat(str, "|ST_RDONLY");
	if (arg & ST_NOSUID)
		(void) strcat(str, "|ST_NOSUID");
	if (arg & ST_NOTRUNC)
		(void) strcat(str, "|ST_NOTRUNC");
	return *str? (CONST char *)(str+1) : "0";
}

CONST char *
fuiname(arg)		/* fusers() input argument */
register int arg;
{
	register CONST char * str = NULL;

	switch (arg) {
	case F_FILE_ONLY:	str = "F_FILE_ONLY";		break;
	case F_CONTAINED:	str = "F_CONTAINED";		break;
	}

	return str;
}

CONST char *
fuflags(arg)		/* fusers() output flags */
register int arg;
{
	register char * str = code_buf;

	if (arg & ~(F_CDIR|F_RDIR|F_TEXT|F_MAP|F_OPEN|F_TRACE|F_TTY)) {
		(void) sprintf(str, "0x%x", arg);
		return str;
	}
	*str = '\0';
	if (arg & F_CDIR)
		(void) strcat(str, "|F_CDIR");
	if (arg & F_RDIR)
		(void) strcat(str, "|F_RDIR");
	if (arg & F_TEXT)
		(void) strcat(str, "|F_TEXT");
	if (arg & F_MAP)
		(void) strcat(str, "|F_MAP");
	if (arg & F_OPEN)
		(void) strcat(str, "|F_OPEN");
	if (arg & F_TRACE)
		(void) strcat(str, "|F_TRACE");
	if (arg & F_TTY)
		(void) strcat(str, "|F_TTY");
	return *str? (CONST char *)(str+1) : "0";
}
