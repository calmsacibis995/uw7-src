#ifndef _SVC_SCO_H	/* wrapper symbol for kernel use */
#define _SVC_SCO_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/sco.h	1.10.3.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/termios.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/termios.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

/* XENIX Support */

#define XF_RDLCK	3	/* XENIX F_RDLCK */
#define XF_WRLCK	1	/* XENIX F_WRLCK */
#define XF_UNLCK	0	/* XENIX F_UNLCK */

	/* Maps XENIX 386 fcntl lock type value onto UNIX lock type value */
#define XMAP_TO_LTYPE(t) (t==XF_UNLCK?F_UNLCK:(t==XF_RDLCK?F_RDLCK:\
				(t==XF_WRLCK?F_WRLCK:t)))

	/* Maps UNIX fcntl lock type value onto XENIX 386 lock type value */
#define XMAP_FROM_LTYPE(t) (t==F_UNLCK?XF_UNLCK:(t==F_RDLCK?XF_RDLCK:\
				(t==F_WRLCK?XF_WRLCK:t)))

#define XENIX_SIGPOLL	20
/* End XENIX Support */


/* Enhanced Application Compatibility Support */

#define SCO_NCCS	NCC+5
#define SCO_8		8		/* Not used by SCO */
#define SCO_9		9		/* Not used by SCO */
#define SCO_VSUSP	10
#define SCO_VSTART	11		/* These conflict with SVR4 */
#define SCO_VSTOP	12		/* These conflict with SVR4 */

typedef	unsigned short	sco_tcflag_t;

struct sco_termios {
	sco_tcflag_t	c_iflag;
	sco_tcflag_t	c_oflag;
	sco_tcflag_t	c_cflag;
	sco_tcflag_t	c_lflag;
	char		c_line;
	unsigned char	c_cc[SCO_NCCS];
	char	c_ispeed;
	char	c_ospeed;
};

#define IS_SCOEXEC	((!ISC_USES_POSIX) && (isSCO || VIRTUAL_XOUT))
#define SCO_DOWAITPID(eflgs)   (((eflgs)->fl_of) && \
			       ((eflgs)->fl_pf) && \
				((eflgs)->fl_zf) && \
				((eflgs)->fl_sf))

/* POSIX wait()/waidpid() defines for SCO binaries */
#define	SCO_WNOHANG	1
#define	SCO_WUNTRACED	2
#define	SCO_WEXITED	0400
#define SCO_WTRACED	0020

/* POSIX 1003.1 termios defines */
#define SCO_TIOC	('T'<<8)
#define	SCO_TIOCSPGRP	(SCO_TIOC|118)
#define	SCO_TIOCGPGRP	(SCO_TIOC|119)

#define	SCO_OXIOC	('x' << 8)
#define SCO_OXCGETA	(SCO_OXIOC | 1)
#define SCO_OXCSETA	(SCO_OXIOC | 2)
#define SCO_OXCSETAW	(SCO_OXIOC | 3)
#define SCO_OXCSETAF	(SCO_OXIOC | 4)

#define SCO_OTCSANOW	SCO_OXCSETA
#define	SCO_OTCSADRAIN	SCO_OXCSETAW
#define	SCO_OTCSAFLUSH	SCO_OXCSETAF
#define	SCO_OTCSADFLUSH	SCO_OXCSETAF

/* Translate SCO POSIX 1003.1 conflicts to BCS Numbers */

#define	SCO_XIOC	(('i' << 24) | ('X' << 16))
#define SCO_XCGETA	(SCO_XIOC | 1)
#define SCO_XCSETA	(SCO_XIOC | 2)
#define SCO_XCSETAW	(SCO_XIOC | 3)
#define SCO_XCSETAF	(SCO_XIOC | 4)

#define SCO_TCSANOW	SCO_XCSETA
#define	SCO_TCSADRAIN	SCO_XCSETAW
#define	SCO_TCSAFLUSH	SCO_XCSETAF
#define	SCO_TCSADFLUSH	SCO_XCSETAF

#define SHNSLPATH	"/shlib/libnsl_s"
#define SCO_SHNSLPATH	"/shlib/libNSL_s"

#define SCO_OMF_NOTE	"Converted OMF object(s), use XENIX semantics!"

/*
 * SCO functions to be included in cxentry[].
 * No need to change them to prototype style.
 */

/* System calls */
extern int	sco_tbd();
extern int	select_sco();
extern int	eaccess_sco();
extern int	sigaction_sco();
extern int	sigprocmask_sco();
extern int	sigpending_sco();
extern int	sigsuspend_sco();
extern int	getgroups_sco();
extern int	setgroups_sco();
extern int	sysconf_sco();
extern int	pathconf_sco();
extern int	fpathconf_sco();
extern int	rename_sco();
extern int	wait_sco();
extern int	scoinfo();

/* Errno Numbers translations for COFF based executables */
#define OELBIN		75
#define OEDOTDOT	76
#define OEWOULDBLOCK	90
#define OENOTSOCK	93
#define OEDESTADDRREQ	94
#define OEMSGSIZE	95
#define OEPROTOTYPE	96
#define OENOPROTOOPT	118
#define OEPROTONOSUPPORT 97
#define OESOCKTNOSUPPORT 98
#define OEOPNOTSUPP	99
#define OEPFNOSUPPORT	100
#define OEAFNOSUPPORT	101
#define OEADDRINUSE	102
#define OEADDRNOTAVAIL	103
#define OENETDOWN	104
#define OENETUNREACH	105
#define OENETRESET	106
#define OECONNABORTED	107
#define OECONNRESET	108
#define OENOBUFS	109
#define OEISCONN	110
#define OENOTCONN	111
#define OESHUTDOWN	112
#define OETOOMANYREFS	113
#define OETIMEDOUT	114
#define OECONNREFUSED	115
#define OEHOSTDOWN	116
#define OEHOSTUNREACH	117
#define OEALREADY	92
#define OEINPROGRESS	91
#define OELOOP		150
#define OERESTART	152
#define OESTRPIPE	153
#define OENOTEMPTY	145


/* The following definitions were copied from values.h */
#define BITS(type)	(NBBY * (int)sizeof(type))

/* short, regular and long ints with only the high-order bit turned on */
#define HIBITS	((short)(1 << BITS(short) - 1))

#define HIBITI	(1U << BITS(int) - 1)
#define HIBITL	(1UL << BITS(long) - 1)

/* largest short, regular and long int */
#define MAXSHORT	((short)~HIBITS)
#define MAXINT		((int)(~HIBITI))
#define MAXLONG		((long)(~HIBITL))
/* The above definitions were copied from values.h */

/* SCO_STAT_VER struct for sco lxstat, xstat and fxstat */
#define _SCO_STAT_VER	51
#define _SCO_ST_FSTYPSZ	16

struct sco_st_stat32 {		/* New 32-bit stat (always defined)	*/
	short	st_dev;		/* id of device containing dir entry	*/
	long	st_pad1[3];
	ino_t	st_ino;		/* i-node number (32 bits-worth)	*/
	ushort_t 	st_mode;	/* file mode (permissions and type)	*/
	short	st_nlink;	/* number of links			*/
	ushort_t 	st_uid;		/* user id of the file's owner		*/
	ushort_t 	st_gid;		/* group id of the file's owner		*/
	short	st_rdev;	/* id of block/character special file	*/
	long	st_pad2[2];
	off32_t	st_size;	/* file size in bytes			*/
	long	st_pad3;
	time_t	st_atime;	/* time of last access			*/
	time_t	st_mtime;	/* time of last data modification	*/
	time_t	st_ctime;	/* time of last file status change	*/
	long	st_blksize;	/* preferred I/O block size in bytes	*/
	long	st_blocks;	/* number st_blksize blocks allocated	*/
	char	st_fstype[_SCO_ST_FSTYPSZ];	/* containing filesystem's type	*/
	long	st_pad4[7];
	long	st_sco_flags;	/* flags (_STAT_SCO_*)			*/
};

/* 
* st_sco_flags
*/
#define _STAT_SCO_ISREMOTE	0x1	/* inode is from a remote filesystem */

/*
 * taken from sco.c to make them global
 */
typedef long    sco_sigset_t;           /* SCO version of sigset_t */
typedef u_short sco_uid_t;              /* SCO version of uid_t */
typedef u_short sco_gid_t;              /* SCO version of gid_t */


#endif /* _KERNEL */

/* End Enhanced Application Compatibility Support */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_SCO_H */
