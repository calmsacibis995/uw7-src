#ident	"@(#)m4:common/m4ext.c	1.9"
#include	"stdio.h"
#include	"m4.h"


/* storage params */
int	hshsize 	= 199;		/* hash table size (prime) */
int	bufsize 	= 4096;		/* pushback & arg text buffers */
int	stksize 	= 100;		/* call stack */
int	toksize 	= 512;		/* biggest word ([a-z_][a-z0-9_]*) */

/* pushback buffer */
WCHAR_T	*ibuf;				/* buffer */
WCHAR_T	*ibuflm;			/* highest buffer addr */
WCHAR_T	*ip;				/* current position */
WCHAR_T	*ipflr;				/* buffer floor */
WCHAR_T 	*ipstk[10];			/* stack for "ipflr"s */


/* arg collection buffer */
WCHAR_T	*obuf;				/* buffer */
WCHAR_T	*obuflm;			/* high address */
WCHAR_T	*op;				/* current position */


/* call stack */
struct call	*callst;		/* stack */
struct call	*Cp 	= NULL;		/* position */


/* token storage */
WCHAR_T	*token;				/* buffer */
WCHAR_T	*toklm;				/* high addr */


/* file name and current line storage for line sync and diagnostics */
char	fnbuf[512];			/* holds file name strings */
char	*fnbufend	= &fnbuf[512];	/* prevent overrunning of file names */
char	*fname[11] 	= {fnbuf};	/* file name ptr stack */
int	fline[10];			/* current line nbr stack */


/* input file stuff for "include"s */
FILE	*ifile[10] 	= {stdin};	/* stack */
int	ifx;				/* stack index */


/* stuff for output diversions */
FILE	*cf 	= stdout;		/* current output file */
FILE	*ofile[11] 	= {stdout};	/* output file stack */
int	ofx;				/* stack index */


/* comment markers */
WCHAR_T	lcom[MAXSYM+1] 	= l_type("#");
WCHAR_T	rcom[MAXSYM+1] 	= l_type("\n");


/* quote markers */
WCHAR_T	lquote[MAXSYM+1] 	= l_type("`");
WCHAR_T	rquote[MAXSYM+1] 	= l_type("'");


/* argument ptr stack */
WCHAR_T	**argstk;
WCHAR_T	*astklm;			/* high address */
WCHAR_T	**Ap;				/* current position */


/* symbol table */
struct nlist	**hshtab;		/* hash table */
int	hshval;				/* last hash val */


/* misc */
char	*procnam;			/* argv[0] */
char	*tempfile;			/* used for diversion files */
WCHAR_T	*Wrapstr;			/* last pushback string for "m4wrap" */
WCHAR_T nullstr[] = l_type("");
int	C;				/* see "m4.h" macros */
int	nflag 	= 1;			/* name flag, used for line sync code */
int	sflag;				/* line sync flag */
int	sysrval;			/* return val from syscmd */
int	trace;				/* global trace flag */


char	aofmsg[] 	= "more than %d chars of argument text";
char	astkof[] 	= "more than %d items on argument stack";
char	badfile[] 	= "can't open file";
char	nocore[] 	= "out of storage";
char	pbmsg[] 	= "pushed back more than %d chars";
