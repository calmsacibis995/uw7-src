#ident	"@(#)sgs-cmd:common/cc.h	1.22"

#include	<stdio.h>
#include	<string.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<ccstypes.h>
#include	<unistd.h>
#include	<malloc.h>
#include	<locale.h>
#include	<varargs.h>
#include	<pfmt.h>
#ifdef __STDC__
#include	<stdlib.h>
#include	<wait.h>
#else
#define	WCOREFLG	0200
#endif
#include	"paths.h"
#include	"sgs.h"
#include	"machdep.h"

/* The PARGS macro is used to print out arguments before a callsys(),
 * when the -# option is in effect.  Note that output goes to stdout
 * for C, stderr for C++; the former is probably a bug (since -## path
 * output goes to stderr), but the cc test suite and possibly other
 * things depend on it.  
 */
#define	PARGS	if(debug) { int j;					\
			(void)fprintf(CCflag ? stderr : stdout, "%scc: ", prefix);		\
			for(j=0;j<nlist[AV];j++)			\
				(void)fprintf(CCflag ? stderr : stdout," '%s'",list[AV][j]);	\
			(void)fprintf(CCflag ? stderr : stdout,"\n");				\
		}

/* performance statistics */

#ifdef PERF

#define	STATS( s )	if (pstat == 1) {			\
				stats[ii].module = s;		\
				stats[ii].ttim = ttime;		\
				stats[ii++].perform = ptimes;	\
				if (ii > 25)			\
					pexit();		\
			}

extern	long	times();

struct	tbuffer {
	long	proc_user_time;
	long	proc_system_time;
	long	child_user_time;
	long	child_system_time;
};

struct	perfstat {
	char	*module;
	long	ttim;
	struct	tbuffer	perform;
};

extern struct tbuffer	ptimes;
extern struct perfstat	stats[30];
extern int	pstat;
extern int	ii;
extern long	ttime;

#endif	/* PERF */

#define cunlink(x)	if (x)	(void)unlink(x);

#ifndef	ADDassemble
#define	ADDassemble()			/* empty */
#endif

/* tool names */

#define CRT1	"crt1.o"
#define CRTI	"crti.o"
#define CCRTI	"Crti.o"
#define VALUES	"values"
#define MCRT1	"mcrt1.o"
#define CRTN	"crtn.o"
#define CCRTN	"Crtn.o"

#define PCRT1   "pcrt1.o"

#define OSRCRT1	"osrcrt1.o"

#define N_FE	"c++fe"
#define N_FS	"fsipp"

#ifndef N_C0
#define N_C0    "acomp"
#endif

#define N_BE	"c++be"

#ifndef	N_OPTIM
#define	N_OPTIM	"optim"
#endif

#if ASFILT
#define N_ASFILT "asfilt"
#endif

#define N_PROF	"basicblk"
#define	N_AS	"as"
#define	N_LD	"ld"
#define	N_DE	"c++filt"
#if AUTOMATIC_TEMPLATE_INSTANTIATION
#define N_PT	"prelink"
#endif


/* list indexes */

extern int Xcp;		/* index of option list for cpp, depends on CCflag */
#define	Xcmd	0	/* index of list containing all command line options */
#define	Xc0	1	/* index of list containing options to the compiler */
#define	Xc2	2	/* index of list containing options to the optimizer */
#define Xbb	3	/* index of list containing options to basicblk */
#define	Xas	4	/* index of list containing options to the assembler */
#define	Xld	5	/* index of list containing options to ld */
#define	AV	6	/* index of the argv list to be supplied to execvp */
#define	CF	7	/* index of list containing the names of files to be 
				compiled and assembled */
#define Xfe	10	/* index of list containing options to the front end */
#if AUTOMATIC_TEMPLATE_INSTANTIATION
#define	Xpt	11	/* index of list containing options to the pre-linker */
#define	XCC	12	/* index of list containing options for recursive calls
				to CC for template instantiation */
#endif
#ifndef	NLIST
#define	NLIST	13	/* total number of lists, can be overwritten in machdep.h */
#endif
#ifndef	Xoptim
#define	Xoptim	Xc2	/* adjust some options not to be appended to optimizer */
#endif

/* option string for getopt():
 *	OPTSTR     == machine independent options
 *	CCOPTSTR   == machine independent options for C++ mode
 *	MACHOPTSTR == machine dependent options
 */

#define OPTSTR "01234A:b:B:Ccd:D:e:EgGh:HI:K:l:L:o:OpPq:Q:Su:U:vVwW:X:Y:z:#"
#ifdef IL_SHOULD_BE_WRITTEN_TO_FILE
#define CCOPTSTR	"NfFR:T:J:"
#else
#define CCOPTSTR	"fFR:T:J:"
#endif
#ifndef MACHOPTSTR
#define MACHOPTSTR	""
#endif

#define OPTPTR(s)	(void)fprintf(stderr, s);

/*
 * colon-separated list of allowable suffixes for C++ source files
 * (this must match DEFAULT_INSTANTIATION_FILE_SUFFIX_LIST in the front end)
 */

#define CPLUSPLUS_FILE_SUFFIX_LIST "c:C:cpp:CPP:cxx:CXX:cc:CC:c++:C++"

/* various arguments to callsys() concerning I/O redirection */
 
#define NO_TMP4_REDIRECTION -1
#define STDIN_FROM_TMP4 STDIN_FILENO
#define STDOUT_TO_TMP4 STDOUT_FILENO
#define STDERR_TO_TMP4 STDERR_FILENO
 
#define NORMAL_STDOUT 0
#define STDOUT_TO_STDERR 1


/* file names */

extern
char	*fe_out,	/* front end output */
	*c_out,		/* compiler output */
	*as_in,		/* assembler input */
	*tmp2,
	*tmp3,
	*tmp4,
	*tmp5,
	*tmp6,
	*tmp7;

#if ASFILT
extern
char	*tmp8;
#endif


/* path names of the various tools and directories */

extern char	
	*passfe,
	*passde,
	*passc0,
	*passc2,
#if ASFILT
	*passasfilt,
#endif
	*passprof,
	*passas,
	*passld,
	*crt,
	*crtdir,
	*ccrtdir,
	*fplibdir,
	*libpath;


/* flags: ?flag corresponds to ? option */
extern
int	cflag,		/* compile and assemble only to .o; no load */
	Tflag,		/* user specified -T option on command line */
	Fflag,		/* run front end only, leave output in .cll file */
	Nflag,		/* run front end only, leave output in .cil file */
	kflag,		/* suppress virtual function tables */
	vflag,		/* enable remarks */
	wflag,		/* suppress warnings */
	Oflag,		/* optimizer request */
	Sflag,		/* leave assembly output in .s file */
	Vflag,		/* "-V" option flag */
	eflag,		/* error count */
	dsflag,		/* turn off symbol attribute output in compiler */
	dlflag,		/* turn of line number output in compiler */
	ilflag,		/* if *.il is present */
	pflag,		/* profile request for standard profiler */
	qpflag,		/* if = 1, then -p. if = 2, then -qp. else none */
	qarg,		/* profile request for xprof or lprof */
	gflag,		/* include libg.a on load line */
	debug,		/* cc command debug flag (prints command lines) */
	Eflag,		/* preprocess only, output to stdout */
	Pflag;		/* preprocess only, output to file.i */

extern char	*ilfile;/* will hold the .il file name */
extern char    *Parg;	/* will hold the -KPIC or -Kpic argument */

/* lists */

extern char	**list[NLIST];	/* list of lists */
extern int
	nlist[NLIST],	/* current index for each list */
	limit[NLIST];	/* length of each list */


extern char	*prefix;
extern int	sfile;		/* indicates current file in list[CF] is .s */

extern int	inlineflag;	/* inline file indicator. 1==ON */
extern int	independ_optim;	/* 1 indicates independent optimizor */
extern int	Ocount;		/* Oflag counting flag */
extern int	Add_Xc2_Qy;	/* Add -Qy to Xc2 ? 1== YES (default), 0== NO */
extern char	Xc0tmp[4];	/* "-KPIC" option for Xc0 (acomp) */

/* controls for C++ compilation method */
enum cplusplus_mode_type {via_c, via_glue};
extern enum cplusplus_mode_type cplusplus_mode;

/* functions */

#ifndef __STDC__
extern  int     optind;         /* arg list index */
extern	int	opterr;         /* turn off error message */
extern	int	optopt;         /* current option char */
extern	int	access();
extern	int	unlink();
extern	int	execvp();
extern	int	creat();
extern	int	getopt();
extern  void    exit();
extern  char    *getenv();
extern  int     putenv();
extern  char    *optarg;        /* current option argument */
extern	int	pfmt();
extern	int	setlabel();
#endif	/* __STDC__ */

/* machine independent routines */

extern char	*stralloc(); 
extern char	*setsuf();
extern char	*passname();
extern void	addopt();
extern void	error();
extern int	callsys(char f[], char *v[], int fd, int one_onto_two);
			/* prototype used to force compilation failure
			   for not-yet-updated machdep.c's  */

/* machine dependent routines */

extern int	optimize();	/* pass arguments to optimizor */
extern int	optelse();	/* more legal options? 1==TRUE,0==FALSE */
extern int	Kelse();	/* more legal '-K xxx' options? 1==TRUE,0==FALSE */
extern int	Yelse();	/* more legal '-Y xxx' options? 1==TRUE,0==FALSE */
extern int	Welse();	/* more legal '-W xxx' options? */
				/* (-1)==FALSE, otherwise, returns optarg value */

extern void	init_mach_opt();/* add more machine dependent options, stage 1 */
extern void	add_mach_opt();	/* add more machine dependent options, stage 2 */
extern void	initvars();	/* machine dependent initialization routine */
extern void	mach_defpath();	/* make machine dependent default path */
extern void	AVmore();	/* append machine dependent options to acomp */
extern void	LDmore();	/* append machine dependent options to ld */
extern void	option_mach();/* machine dependent option usage messages */

