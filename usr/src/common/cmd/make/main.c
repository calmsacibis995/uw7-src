#ident	"@(#)make:main.c	1.24.2.2"
#include "defs"
#include <signal.h>
#include <ccstypes.h>
#include <errno.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

/* NAME
**	make - maintain, update and regenerate groups of programs.
**
** OPTIONS for make
** 	'-f'  The description file is the next argument;
** 	     	(makefile is the default)
** 	'-p'  Print out a complete set of macro definitions and target
**		descriptions
** 	'-i'  Ignore error codes returned by invoked commands
** 	'-S'  stop after any command fails (normally do parallel work)
** 	'-s'  Silent mode.  Do not print command lines before invoking
**	'-n'  No execute mode.  Print commands, but do not execute them.
** 	'-t'  Touch the target files but don't issue command
** 	'-q'  Question.  Check if object is up-to-date;
** 	      	returns exit code 0 if up-to-date, -1 if not
** 	'-u'  unconditional flag.  Ignore timestamps.
**	'-d'  Debug.  No-op unless compiled with -DMKDEBUG
**	'-w'  Set off warning msgs.
**	'-P'	Set on parallel.
**	'-B'	Block the taget's output.
*/

#define TURNOFF(a)	(Mflags &= ~(a))
#define TURNON(a)	(Mflags |= (a))


static char	makefile[] = "makefile",
	Makefile[] = "Makefile",
	Makeflags[] = "MAKEFLAGS",
	RELEASE[] = "RELEASE";

char	Nullstr[] = "",
	funny[CHAR_LIST];

extern CHARSTAR builtin[];
extern errno;
CHARSTAR *linesptr = builtin;

int parallel = PARALLEL ; /* The max. no. of parallel proc. that make will fork */
char tmp_block[30];
char * cur_makefile;
char * cur_wd;			/* Current working directory */
int nproc = 0 ;		  /* No. of process currently running */

FILE *fin;

struct nameblock pspace;
NAMEBLOCK curpname = &pspace,
	mainname,
	firstname;

LINEBLOCK sufflist;
VARBLOCK firstvar;
PATTERN firstpat ;
OPENDIR firstod;


CHAIN	lnkd_files,		/* list of files linked to directory */
	last_lnkdfs;		/* backward pointer of above */
int	lib_cpd = NO;		/* library copied into the node ? */


void	(*intstat) (),
	(*quitstat) (),
	(*hupstat) (),
	(*termstat) ();

/*
**	Declare local functions and make LINT happy.
*/

static int	rddescf();
static int	rdd1();
static void	printdesc();
static void	prname();
static void	getmflgs();
static void	setflags();
static void	optswitch();
static void	usage();
static void	setmflgs();
static void	readenv();
static int	eqsign();
static void	callyacc();

static char temp_path[MAXPATHLEN];	/* max temp pathname */

int	Mflags = MH_DEP;

int	okdel = YES;


main(argc, argv)
int	argc;
CHARSTAR argv[];
{
	void	intrupt();
	register NAMEBLOCK p;
	register CHARSTAR s;
	register int i;
	time_t	tjunk;
	char	*getenv(), *sname(),
		*m_getcwd(), *vptr ;
	void	fatal(), fatal1(), setvar(), enbint(), pwait();
	int	nfargs, chdir(), doname(), isdir(),
		descset = 0;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxepu");	
	(void)setlabel("UX:make");

	for (s = "#|=^();&<>*?[]:$`'\"\\\n" ; *s ; ++s)
		funny[(unsigned char) *s] |= META;
	for (s = "\n\t :=;{}&>|" ; *s ; ++s)
		funny[(unsigned char) *s] |= TERMINAL;
	funny[(unsigned char) '\0'] |= TERMINAL;

        if ((lefts = (struct nameblock * *) malloc(NLEFTS_BASE *
                         sizeof(struct nameblock *))) == NULL) {
                fatal(":311:out of memory") ;
        }

        if ((yytext = malloc(BUFF_BASE)) == NULL) {
                fatal(":311:out of memory") ;
        }


        yytextEnd = yytext + BUFF_BASE ;
        yytextLen = BUFF_BASE ;

	TURNON(INTRULE);	/* Default internal rules, turned on */

	init_lex();

	builtin[1] = "MAKE=make";

	cur_wd = m_getcwd();

	getmflgs();			/* Init $(MAKEFLAGS) variable */
	setflags(argc, argv);		/* Set command line flags */

	setvar("$", "$");

	/*	Read command line "=" type args and make them readonly.	*/

	TURNON(INARGS | EXPORT);
#ifdef MKDEBUG
	if (IS_ON(DBUG)) 
		printf("Reading \"=\" args on command line.\n");
#endif
	for (i = 1; i < argc; ++i)
		if ( argv[i] && argv[i][0] != MINUS &&
		     eqsign(argv[i]) )
			argv[i] = 0;

	TURNOFF(INARGS | EXPORT);

	if (IS_ON(INTRULE)) {	/* Read internal definitions and rules. */
#ifdef MKDEBUG
		if (IS_ON(DBUG))
			printf("Reading internal rules.\n");
#endif
		(void)rdd1((FILE *) NULL);
	}
	TURNOFF(INTRULE);	/* Done with internal rules, now. */

	/* Read environment args.
	 * Let file args which follow override, unless 'e'
	 * in MAKEFLAGS variable is set.
	 */
	if (ANY((varptr(Makeflags))->varval.charstar, 'e') )
		TURNON(ENVOVER);
#ifdef MKDEBUG
	if (IS_ON(DBUG))
		printf("Reading environment.\n");
#endif
	TURNON(EXPORT);
	readenv();
	TURNOFF(EXPORT | ENVOVER);

	if(IS_ON(PAR) && IS_ON(NOEX))
		TURNOFF(PAR);


	if(IS_ON(PAR)){

		if(IS_ON(BLOCK))
			sprintf(tmp_block,"/usr/tmp/make%d", getpid());

		if ((vptr = getenv("PARALLEL")) && !(STREQ(vptr, "")))
			if((parallel = atoi(vptr)) <= 0 )
				parallel = 1;
	}else
		TURNOFF(BLOCK);

	/*	Get description file in the following order:
	**	 - command line '-f' parameters
	**	 - default names (makefile, Makefile, s.makefile, s.Makefile)
	*/
	for (i = 1; i < argc; i++)
		if ( argv[i] && (argv[i][0] == MINUS) &&
		    (argv[i][1] == 'f') && (argv[i][2] == CNULL)) {
			argv[i] = 0;
			if (i >= argc - 1 || argv[i+1] == 0)
				fatal(":302:no description file after -f flag (bu1)");
			if ( rddescf(argv[++i], YES) )
				fatal1(":303:cannot open %s", argv[i]);
			argv[i] = 0;
			++descset;
		}
	if ( !descset )
		if ( rddescf(makefile, NO))
			if ( rddescf(Makefile, NO) )
				if ( rddescf(makefile, YES))
					(void)rddescf(Makefile, YES);

	if (IS_ON(PRTR)) 
		printdesc(NO);

	if ( SRCHNAME(".IGNORE") ) 
		TURNON(IGNERR);
	if ( SRCHNAME(".SILENT") ) 
		TURNON(SIL);
	if (p = SRCHNAME(".SUFFIXES")) 
		sufflist = p->linep;
	if ( !sufflist ) 
		if(IS_OFF(WARN))
			pfmt(stderr, MM_WARNING,
				":292:No suffix list.\n");

	intstat = (void(*)()) signal(SIGINT, SIG_IGN);
	quitstat = (void(*)()) signal(SIGQUIT, SIG_IGN);
	hupstat = (void(*)()) signal(SIGHUP, SIG_IGN);
	termstat = (void(*)()) signal(SIGTERM, SIG_IGN);
	enbint(intrupt);

	nfargs = 0;


		

	for (i = 1; i < argc; ++i)
		if ( s = argv[i] ) {
			if ( !(p = SRCHNAME(s)) )
				p = makename(s);

			++nfargs;

			/* another entry in list of files linked/copied */

			last_lnkdfs = lnkd_files = ALLOC(chain);
			lnkd_files->nextchain = NULL;
			(void)doname(p, 0, &tjunk );
			if(IS_ON(PAR)){
#ifdef MKDEBUG
			if (IS_ON(DBUG)) 
				printdesc(YES);
#endif
				pwait(0);
			}
#ifdef MKDEBUG
			if (IS_ON(DBUG)) 
				printdesc(YES);
#endif
		}

	/*
	 * If no file arguments have been encountered, make the first
	 * name encountered that doesn't start with a dot
	 */
	if ( !nfargs )
		if ( !mainname )
			fatal(":304:no arguments or description file (bu7)");
		else {
			last_lnkdfs = lnkd_files = ALLOC(chain);
			lnkd_files->nextchain = NULL;
			(void)doname(mainname, 0, &tjunk);
			if(IS_ON(PAR)){
#ifdef MKDEBUG
			if (IS_ON(DBUG)) 
				printdesc(YES);
#endif
				pwait(0);
			}
#ifdef MKDEBUG
			if (IS_ON(DBUG)) 
				printdesc(YES);
#endif
		}

	mkexit(0);	/* make succeeded; no fatal errors */
	/*NOTREACHED*/
}




void
intrupt()
{
	time_t	exists();
	int	isprecious(), member_ar(), unlink();
	void	mkexit();
	int	lev, ret_isdir;
	CHARSTAR p;

	NAMEBLOCK lookup_name();

	if (okdel && IS_OFF(NOEX) && IS_OFF(TOUCH) && 
	    (p = (varptr("@")->varval.charstar)) &&
	    (exists( lookup_name(p), &lev) != -1) && 
	    (!isprecious(p)) &&
	    (!member_ar(p)))	/* don't remove archives generated during make */
	{
		if ((ret_isdir = isdir(p)) == 1)
			pfmt(stderr, MM_NOSTD,
				":293:\n*** %s NOT removed.\n", p);
		else if ( !( ( ret_isdir == 2 ) || unlink(p) ) )
			pfmt(stderr, MM_NOSTD,
				":294:\n*** %s removed.\n", p);
	}
	fprintf(stderr, "\n");
	mkexit(2);
	/*NOTREACHED*/
}


void
enbint(onintr)
void (*onintr)();
{
	if (intstat == (void(*)())SIG_DFL)
		(void)signal(SIGINT, onintr);
	if (quitstat == (void(*)())SIG_DFL)
		(void)signal(SIGQUIT, onintr);
	if (hupstat == (void(*)())SIG_DFL)
		(void)signal(SIGHUP, onintr);
	if (termstat == (void(*)())SIG_DFL)
		(void)signal(SIGTERM, onintr);
}




static int
rddescf(descfile, sflg)		/* read and parse description file */
CHARSTAR descfile;
int	sflg;		/* if YES try s.descfile */
{
	FILE *k;

	if (STREQ(descfile, "-")){
		cur_makefile = descfile;
		return( rdd1(stdin) );
	}

retry:	if (k = fopen(descfile, "r")) {
#ifdef MKDEBUG
		if (IS_ON(DBUG))
			printf("Reading %s\n", descfile);
#endif
		cur_makefile = descfile;
		return( rdd1(k) );
	}

	if ( !sflg || !get(descfile, varptr(RELEASE)->varval.charstar) )
		return(1);
	sflg = NO;
	goto retry;
}


/**	used by yyparse		**/
extern int	yylineno;
extern CHARSTAR zznextc;


static int
rdd1(k)
FILE *k;
{
	int yyparse();
	void	fatal();

	fin = k;
	yylineno = 0;
	zznextc = 0;

	if ( yyparse() )
		fatal(":305:description file error (bu9)");

	if ( ! (fin == NULL || fin == stdin) )
		(void)fclose(fin);

	return(0);
}


static void
printdesc(prntflag)
int	prntflag;
{
	register NAMEBLOCK p;
	register VARBLOCK vp;
	register CHAIN pch;
	if (prntflag) {
		register OPENDIR od;
		pfmt(stdout, MM_NOSTD, ":295:Open directories:\n");
		for (od = firstod; od; od = od->nextopendir)
			fprintf(stdout, "\t%s\n", od->dirn);
	}
	if (firstvar) 
		pfmt(stdout, MM_NOSTD, ":296:Macros:\n");
	for (vp = firstvar; vp; vp = vp->nextvar)
		if ( !(vp->v_aflg) )
			printf("%s = %s\n", vp->varname,
				((vp->varval.charstar) == NULL? " ":vp->varval.charstar));
		else {
			pfmt(stdout, MM_NOSTD,
				":297:Lookup chain: %s\n\t", vp->varname);
			for (pch = (vp->varval.chain); pch; pch = pch->nextchain)
				fprintf(stdout, " %s", (pch->datap.nameblock)->namep);
			fprintf(stdout, "\n");
		}

	for (p = firstname; p; p = p->nextname)
		prname(p, prntflag);
	printf("\n");
	(void)fflush(stdout);
}


static void
prname(p, prntflag)
register NAMEBLOCK p;
{
	register DEPBLOCK dp;
	register SHBLOCK sp;
	register LINEBLOCK lp;

	if (p->linep)
		printf("\n%s:", p->namep);
	else
		fprintf(stdout, "\n%s:", p->namep);

	if (prntflag)
		fprintf(stdout, "  done=%d", p->done);

	if (p == mainname) 
		pfmt(stdout, MM_NOSTD, ":298:(MAIN NAME)");

	for (lp = p->linep; lp; lp = lp->nextline) {
		if ( dp = lp->depp ) {
			pfmt(stdout, MM_NOSTD, ":299:\n depends on:");
			for (; dp; dp = dp->nextdep)
				if ( dp->depname) {
					printf(" %s", dp->depname->namep);
					printf(" ");
				}
		}
		if (sp = lp->shp) {
			printf("\n");
			pfmt(stdout, MM_NOSTD, ":300:commands:\n");
			for ( ; sp; sp = sp->nextsh)
				printf("\t%s\n", sp->shbp);
		}
	}
}


static void
getmflgs()		/* export command line flags for future invocation */
{
	void	setvar();
	int	sindex();
	register CHARSTAR *pe, p;
	register VARBLOCK vpr = varptr(Makeflags);

	setvar(Makeflags, "ZZZZZZZZZZZZZZZZ");
	vpr->varval.charstar[0] = CNULL;
	vpr->envflg = YES;
	vpr->noreset = YES;
	/*optswitch('b');*/
	for (pe = environ; *pe; pe++)
		if ( !sindex(*pe, "MAKEFLAGS=") ) {
			for (p = (*pe) + sizeof Makeflags; *p; p++)
				optswitch(*p);
			return;
		}
}


static void
setflags(ac, av)
register int	ac;
CHARSTAR *av;
{
	register int	i, j;
	register char	c;
	int	flflg = 0; 		/* flag to note '-f' option. */

	for (i = 1; i < ac; ++i) {
		if (flflg ) {
			flflg = 0;
			continue;
		}
		if (av[i] && av[i][0] == MINUS) {
			if (ANY(av[i], 'f'))
				flflg++;

			for (j = 1 ; (c = av[i][j]) != CNULL ; ++j)
				optswitch(c);

			if (flflg)
				av[i] = "-f";
			else
				av[i] = 0;
		}
	}
}



static void
optswitch(c)	/* Handle a single character option */
char	c;
{

	switch (c) {

	case 's':	/* silent flag */
		TURNON(SIL);
		setmflgs(c);
		return;

	case 'n':	/* do not exec any commands, just print */
		TURNON(NOEX);
		setmflgs(c);
		return;

	case 'e':	/* environment override flag */
		setmflgs(c);
		return;

	case 'p':	/* print description */
		TURNON(PRTR);
		return;

	case 'i':	/* ignore errors */
		TURNON(IGNERR);
		setmflgs(c);
		return;

	case 'S':
		TURNOFF(KEEPGO);
		setmflgs(c);
		return;

	case 'k':
		TURNON(KEEPGO);
		setmflgs(c);
		return;

	case 'r':	/* turn off internal rules */
		TURNOFF(INTRULE);
		return;

	case 't':	/* touch flag */
		TURNON(TOUCH);
		setmflgs(c);
		return;

	case 'q':	/* question flag */
		TURNON(QUEST);
		setmflgs(c);
		return;

	case 'g':	/* turn default $(GET) of files not found */
		TURNON(GET);
		setmflgs(c);
		return;

	case 'b':	/* use MH version of test for whether cmd exists */
		TURNON(MH_DEP);
		setmflgs(c);
		return;

/* turn off -b flag */
/*
	case 'B':	
		TURNOFF(MH_DEP);
		setmflgs(c);
		return;
*/

	case 'd':	/* debug flag */
#ifdef MKDEBUG
		TURNON(DBUG);
		setmflgs(c);
#endif
		return;

	case 'm':	/* print memory map is not supported any more */
		return;

	case 'f':	/* named makefile: handled by setflags() */
		return;

	case 'u':	/* unconditional build indicator */
		TURNON(UCBLD);
		setmflgs(c);
		return;

	case 'w':	/* Set off warning msgs */
		TURNON(WARN);
		setmflgs(c);
		return;

	case 'P':	/* Set on parallel */
		TURNON(PAR);
		setmflgs(c);
		return;

	case 'B':	/* Set on target's output blocking */
		TURNON(BLOCK);
		setmflgs(c);
		return;
	}

	usage(c);
}


static void
usage(c)
char c;
{
	pfmt(stderr, MM_ERROR, 
		":331:Illegal option -- %c\n",c); 
	pfmt(stderr, MM_ACTION,
         	":332:Usage: make [-f makefile] [-B] [-e] [-i] [-k] [-n]\n\t");
#ifdef MKDEBUG
	fprintf(stderr, "[-d] ");
#endif
	pfmt(stderr,MM_NOSTD,
		":333:[-p] [-P] [-q] [-r] [-s] [-t] [-u] [-w] [names]\n");
	mkexit(1);
}


static void
setmflgs(c)		/* set up the cmd line input flags for EXPORT. */
register char	c;
{
	register CHARSTAR p = (varptr(Makeflags))->varval.charstar;
	for (; *p; p++)
		if (*p == c)
			return;
	*p++ = c;
	*p = CNULL;
}


/*
 *	If a string like "CC=" occurs then CC is not put in environment.
 *	This is because there is no good way to remove a variable
 *	from the environment within the shell.
 */
static void
readenv()
{
	register CHARSTAR *ea, p;

	ea = environ;
	for (; *ea; ea++) {
		for (p = *ea; *p && *p != EQUALS; p++)
			;
		if ((*p == EQUALS) && *++p != '\0') {
			if (ANY(p, '$')) {
				char *s;
				int n = p - *ea; /* length through '=' */

				/*
				 * Protect incoming environment strings with
				 * dollars from being subsequently taken as
				 * introducing make macro substitutions by
				 * inserting an extra dollar for each such.
				 */
				if ((s = malloc(n + 2 * strlen(p))) == 0)
					fatal(":313:cannot alloc mem");
				(void)memcpy(s, *ea, n);
				*ea = s;
				s += n;
				do {
					if ((*s++ = *p) == '$')
						*s++ = '$';
				} while (*p++ != '\0');
			}
			(void)eqsign(*ea);
		}
	}
}


static int
eqsign(a)
register CHARSTAR a;
{
	register CHARSTAR p;

	for (p = ":;=$\n\t"; *p; p++)
		if (ANY(a, *p)) {
			callyacc(a);
			return(YES);
		}
	return(NO);
}


static void
callyacc(str)
register CHARSTAR str;
{
	CHARSTAR lines[2];
	FILE 	*finsave = fin;
	CHARSTAR *lpsave = linesptr;
	fin = 0;
	lines[0] = str;
	lines[1] = 0;
	linesptr = lines;
	(void)yyparse();
	fin = finsave;
	linesptr = lpsave;
}

NAMEBLOCK
lookup_name(namep)
CHARSTAR namep;
{
	NAMEBLOCK p;
	for (p = firstname; p; p = p->nextname) {
		if (STREQ(namep, p->namep)) 
			return (p);
	}
	return ( NULL );
}
char *
m_getcwd()
{
char *getcwd();
char *p;
	if(getcwd(temp_path, MAXPATHLEN) == NULL)
 		fatal(":306:getcwd (bu5)");

	p = ck_malloc(strlen(temp_path) + 1);
	strcpy(p, temp_path);
	return(p);
}
