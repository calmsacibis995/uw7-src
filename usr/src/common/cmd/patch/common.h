/*	copyright	"%c%"	*/
/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)patch_p2:common.h	1.2"

/*
 * OSF/1 1.2
 */
/* @(#)$RCSfile$ $Revision$ (OSF) $Date$ */

/* Header: common.h,v 2.0.1.2 88/06/22 20:44:53 lwall Locked
 *
 * Log:	common.h,v
 * Revision 2.0.1.2  88/06/22  20:44:53  lwall
 * patch12: sprintf was declared wrong
 * 
 * Revision 2.0.1.1  88/06/03  15:01:56  lwall
 * patch10: support for shorter extensions.
 * 
 * Revision 2.0  86/09/17  15:36:39  lwall
 * Baseline for netwide release.
 * 
 */

#define DEBUGGING


/* shut lint up about the following when return value ignored */

#define Signal (void)signal
#define Unlink (void)unlink
#define Lseek (void)lseek
#define Fseek (void)fseek
#define Fstat (void)fstat
#define Pclose (void)pclose
#define Close (void)close
#define Fclose (void)fclose
#define Fflush (void)fflush
#define Sprintf (void)sprintf
#define Mktemp (void)mktemp
#define Strcpy (void)strcpy
#define Strcat (void)strcat


/* constants */

#undef TRUE
#undef FALSE
#define TRUE (1)
#define FALSE (0)

#define MAXHUNKSIZE 100000		/* is this enough lines? */
#define INITHUNKMAX 125			/* initial dynamic allocation size */
#define MAXLINELEN 1024
#define BUFFERSIZE 1024
#define SCCSPREFIX "s."
#define GET "get -e %s"
#define RCSSUFFIX ",v"
#define CHECKOUT "co -l %s"

#ifdef FLEXFILENAMES
#define ORIGEXT ".orig"
#define REJEXT ".rej"
#else
#define ORIGEXT "~"
#define REJEXT "#"
#endif

/* handy definitions */

#define Null(t) ((t)0)
#define Nullch (char *)NULL
#define Nullfp (FILE *)NULL
#define Nulline (LINENUM)NULL

#define Ctl(ch) ((ch) & 037)

#define strNE(s1,s2) (strcmp(s1, s2))
#define strEQ(s1,s2) (!strcmp(s1, s2))
#define strnNE(s1,s2,l) (strncmp(s1, s2, l))
#define strnEQ(s1,s2,l) (!strncmp(s1, s2, l))

/* typedefs */

typedef char bool;
typedef long LINENUM;			/* must be signed */
typedef unsigned MEM;			/* what to feed malloc */

/* globals */

EXT int Argc;				/* guess */
EXT char **Argv;
EXT int Argc_last;			/* for restarting plan_b */
EXT char **Argv_last;

extern struct stat filestat;		/* file statistics area */
extern int filemode;

EXT char buf[MAXLINELEN];		/* general purpose buffer */
extern FILE *ofp;			/* output file pointer */
extern FILE *rejfp;			/* reject file pointer */

extern bool using_plan_a;		/* try to keep everything in memory */
extern bool out_of_mem;	/* ran out of memory in plan a */

#define MAXFILEC 2
extern int filec;			/* how many file arguments? */
EXT char *filearg[MAXFILEC];
extern bool ok_to_create_file;
extern char *bestguess;			/* guess at correct filename */

extern char *outname;
extern char *oopt;			/* -o option-argument */
extern int first_append;
EXT char rejname[128];

extern char *origprae;

extern char TMPOUTNAME[];
extern char TMPINNAME[];		/* might want /usr/tmp here */
extern char TMPREJNAME[];
extern char TMPPATNAME[];
extern bool toutkeep;
extern bool trejkeep;

extern LINENUM last_offset;
#ifdef DEBUGGING
extern int debug;
#endif
extern LINENUM maxfuzz;
extern bool force;
extern bool verbose;
extern bool reverse;
extern bool noreverse;
extern bool skip_rest_of_patch;
extern int strippath;
extern bool canonicalize;
extern bool saveorig;

#define CONTEXT_DIFF 1
#define NORMAL_DIFF 2
#define ED_DIFF 3
#define NEW_CONTEXT_DIFF 4
extern int diff_type;

extern bool do_defines;			/* patch using ifdef, ifndef, etc. */
EXT char if_defined[128];		/* #ifdef xyzzy */
EXT char not_defined[128];		/* #ifndef xyzzy */
extern char else_defined[];		/* #else */
EXT char end_defined[128];		/* #endif xyzzy */

extern char *revision;			/* prerequisite revision, if any */

/*
char *malloc();
char *realloc();
char *strcpy();
char *strcat();
long atol();
long lseek();
char *mktemp();
#ifdef CHARSPRINTF
char *sprintf();
#else
int sprintf();
#endif

*/
