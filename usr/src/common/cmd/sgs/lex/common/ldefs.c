#ident	"@(#)lex:common/ldefs.c	1.15"
# include <stdio.h>
# include <sgs.h>
#ifdef __STDC__
# include <stdlib.h>
#endif

# define NCH 256
# define TOKENSIZE 20000
# define DEFSIZE 8000
# define DEFCHAR 16000
# define BUF_SIZ 16000
# define STARTCHAR 10240
# define STARTSIZE 800
# define CCLSIZE 8000

# define TREESIZE 8000
# define NSTATES 4000
# define MAXPOS 20000
# define NTRANS 16000
# define NOUTPUT 24000
# define NACTIONS 800
# define ALITTLEEXTRA 2400

# define RCCL NCH+90
# define RNCCL NCH+91
# define RSTR NCH+92
# define RSCON NCH+93
# define RNEWE NCH+94
# define FINAL NCH+95
# define RNULLS NCH+96
# define RCAT NCH+97
# define STAR NCH+98
# define PLUS NCH+99
# define QUEST NCH+100
# define DIV NCH+101
# define BAR NCH+102
# define CARAT NCH+103
# define S1FINAL NCH+104
# define S2FINAL NCH+105

# define DEFSECTION 1
# define RULESECTION 2
# define ENDSECTION 5
# define TRUE 1
# define FALSE 0

# ifdef DEBUG
# define LINESIZE 110
extern int yydebug;
extern int debug;		/* 1 = on */
extern int charc;
# endif

# ifndef DEBUG
# define freturn(s) s
# endif


extern int optind;
extern int no_input;
extern int sargc;
extern char **sargv;
extern char *v_stmp;
extern char buf[];
extern int ratfor;		/* 1 = ratfor, 0 = C */
extern int fatal;
extern int n_error;
extern int copy_line;
extern int yyline;		/* line number of file */
extern int sect;
extern int eof;
extern int lgatflg;
extern int divflg;
extern int funcflag;
extern int pflag;
extern int casecount;
extern int chset;	/* 1 = char set modified */
extern FILE *fin, *fout, *fother, *errorf;
extern int fptr;
extern char *ratname, *cname;
extern int prev;	/* previous input character */
extern int pres;	/* present input character */
extern int peek;	/* next input character */
extern int *name;
extern int *left;
extern int *right;
extern int *parent;
extern char *nullstr;
extern int tptr;
extern int pushc[TOKENSIZE];
extern int *pushptr;
extern char slist[STARTSIZE];
extern char *slptr;
extern char **def, **subs, *dchar;
extern char **sname, *schar;
extern char *ccl;
extern char *ccptr;
extern char *dp, *sp;
extern int dptr, sptr;
extern char *bptr;		/* store input position */
extern char *tmpstat;
extern int count;
extern int **foll;
extern int *nxtpos;
extern int *positions;
extern int *gotof;
extern int *nexts;
extern unsigned char *nchar;
extern int **state;
extern int *sfall;		/* fallback state num */
extern char *cpackflg;		/* true if state has been character packed */
extern int *atable, aptr;
extern int nptr;
extern char symbol[NCH];
extern unsigned char cindex[NCH];
extern int xstate;
extern int stnum;
extern int ctable[];
extern int ZCH;
extern int ccount;
extern unsigned char match[NCH];
extern char extra[NACTIONS];
extern unsigned char *pcptr, *pchar;
extern int pchlen;
extern int nstates, maxpos;
extern int yytop;
extern int report;
extern int ntrans, treesize, outsize;
extern long rcount;
extern int optim;
extern int *verify, *advance, *stoff;
extern int scon;
extern char *psave;
extern char *getl();
#ifdef __STDC__
extern char *myalloc();
#else
extern char *calloc(), *myalloc();
#endif
extern int buserr(), segviol();
extern void error_tail();
extern void error();
extern void warning();
extern int printable();
extern void cfree();
extern void layout();
extern void mkmatch();
extern void cgoto();
extern void cfoll();
extern void phead1();
extern void statistics();
extern void ptail();
extern int gch();
extern int slength();
extern void scopy();
extern void lgate();
extern void cclinter();
extern void cpycom();
extern void munput();
