#ident	"@(#)sgs-inc:common/dprof.h	1.4"
#include <libelf.h>
#include <stddef.h>

#define FCNTOT		600
#define NULL	 	0
#define MON_OUT  	"mon.out"

#define ONAMESIZE	128		/* maximum object file name size */

#define MUNKNOWN	0		/* unknown machine type */
#define MVAX		1 		/* VAX machine type */
#define MSIMPLEX	2		/* 3B20S machine type */
#define M32		3		/* 3B5 and 3B2 machine type */
#define M386            4               /* 386 machine type */
#define M860		5		/* 860 machine type */
#define M68K		6		/* 68K machine type */
#define M88K		7		/* 88K machine type */
#define MSPARC		8		/* SPARC machine type */

#define VERSION		4		/* current version number */

typedef unsigned short	WORD;		/* histogram bucket */
typedef unsigned long	caCOVWORD;	/* coverage counter size */
typedef struct cnt	Cnt;		/* call counter structure */
typedef struct cntb	Cntb;		/* holds a bunch of Cnt's */
typedef struct soentry	SOentry;	/* shared object information */

struct caHEADER {
	unsigned char	size;		/* # bytes in this header */
	unsigned char	mach;		/* Mxxx from above */
	unsigned char	vers;		/* VERSION from above */
	unsigned char	okay;		/* nonzero when done writing */
	unsigned char	ident[EI_NIDENT]; /* copied from elf file header */
	char		name[ONAMESIZE]; /* [truncated] elf file pathname */
	unsigned long	type;		/* elf "type" (ET_xxx) */
	caCOVWORD	ncov;		/* number of coverage entries */
	time_t		time;		/* modification time of elf file */
};

struct caCNTMAP {
	struct caHEADER	*head;		/* points to front, too */
	caCOVWORD	*cov;		/* start of coverage data */
	caCOVWORD	*aft;		/* just past last coverage */
	size_t		nbyte;		/* total number of bytes in file */
};

struct cnt {
	unsigned long	*fnpc;		/* text address owning this counter */
	long		mcnt;		/* actual counter */
	SOentry		*_SOptr;	/* containing SO's entry */
};

struct cntb {
	Cntb	*next;		/* pointer to next call count buffer */
	Cnt	cnts[FCNTOT];	/* call count cells */
};

struct soentry {
	char		*SOpath;	/* pathname of SO */
	char		*tmppath;	/* lprof temporary dump file */
	WORD		*tcnt;		/* start of histogram buckets */
	SOentry		*prev_SO;	/* previous (only when active) */
	SOentry		*next_SO;	/* next in list */
	unsigned long	baseaddr;	/* address where mapping starts */
	unsigned long	textstart;	/* low text address */
	unsigned long	endaddr;	/* just after high text address */
	unsigned long	size;		/* number of histogram buckets */
	unsigned long	ccnt;		/* number of call counters */
};

#define LPO_NOPROFILE	0x01	/* don't dump line profiling data */
#define LPO_MERGE	0x02	/* merge line profiling data */
#define LPO_VERBOSE	0x04	/* announce dumping operations */
#define LPO_USEPID	0x08	/* include the PID in the filename */

struct options {
	char		*dir;	/* this directory instead of current */
	char		*file;	/* this filename instead of default */
	unsigned long	flags;	/* LPO_xxx bits */
};

extern char	**___Argv;	/* points to argv; set by crt's */
extern SOentry	*_curr_SO;	/* most recent SO; null for line profiling */
extern SOentry	*_last_SO;	/* tail of active SO list */
extern SOentry	*_act_SO;	/* head of active SO list */
extern SOentry	*_inact_SO;	/* head of the inactive SO list */
extern long	_out_tcnt;	/* out-of-bounds PC bucket */
extern Cntb	*_cntbuffer;	/* current call counter block */
extern int	_countbase;	/* counters now available in _cntbuffer */

void	_newmon(unsigned long);		/* start/stop regular profiling */
void	_mcountNewent(long **, unsigned long); /* allocate new counter */
void	_SOin(unsigned long);		/* process linkmap changes */
void	*_search(unsigned long);	/* SO matching PC value */
void	_mnewblock(void);		/* allocate new Cntb (_cntbuffer) */
void	_dprofil(int);			/* start/stop SIGPROF events */
void	_CAstartSO(unsigned long);	/* start up line profiling */
void	_CAstopSO(SOentry *);		/* dump line profiling data for SO */
void	_CAnewdump(void);		/* shutdown line profiling */
int	_CAmapcntf(struct caCNTMAP *, const char *, int); /* map cnt file */
int	_CAcntmerge(const char *, const char *, int); /* add 2nd into 1st */
