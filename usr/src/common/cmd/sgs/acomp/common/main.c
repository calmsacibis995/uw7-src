#ident	"@(#)acomp:common/main.c	55.1.3.45"
/* main.c */

/* Driver code for ANSI C compiler.  Handle option processing,
** call parser, clean up.
*/

#include "p1.h"
#include "lang.h"
#include <signal.h>
#include <string.h>
#include <locale.h>
#include <pfmt.h>
#include <unistd.h>
#include "sgs.h"			/* For RELEASE string. */
#ifdef	MERGED_CPP
#include "interface.h"			/* CPP interface */
#endif
#ifdef	IEEE_HOST
/* CG's headers defined the macro FP_NEG, which happens to
** be the name of one of the symbols in ieeefp.h.  Since we
** don't need CG's FP_NEG here (or ieeefp.h's, for that matter),
** remove CG's to avoid a conflict.
*/
#undef	FP_NEG
#include <ieeefp.h>			/* host has IEEE floating point */
#endif

#ifndef	CPL_PKG
#define	CPL_PKG "package"
#endif
#ifndef	CPL_REL
#define	CPL_REL	"release"
#endif


int d1debug;				/* declaration processing debug */
int i1debug;				/* initialization debug */
#ifdef C_CHSIGN
int chars_signed = 1;			/* Default treatment of "plain" chars */
#else
int chars_signed = 0;
#endif
int version = V_CI4_1;			/* Version flag:  default is CI 4.1 */
int verbose;				/* be noisy about warnings if non-0 */
int suppress_warnings;			/* suppress warnings if non-0 */
#ifndef LINT
static int vstamp = 0;			/* 0:  no version stamp, 1:  stamp */
#endif

#ifdef MERGED_CPP
extern int	will_allow_dollar;	/* boolean: option to allow '$" seen. */
extern int 	dollar_allowed_in_id;	/* boolean: acceptance of '$' fully
					**	    activated and diagnostic
					**	    issued if needed. */
#else
int	will_allow_dollar;		/* boolean: option to allow '$" seen. */
int	dollar_allowed_in_id;		/* boolean: acceptance of '$' fully
					**	    activated and diagnostic
					**	    issued if needed. */
#endif

#ifdef __STDC__
extern void allow_dollar_in_id(void);
#else
extern void allow_dollar_in_id();
#endif

extern void earlyexit();
extern void exit();
#ifndef LINT
static void dovstamp();
#endif
static int setup();
static int process();
static int cleanup();

#ifdef LINT
#define p2flags(f)			/* doesn't exist for lint */
#define amigo_flags(f)			/* doesn't exist for lint */
#else
extern void amigo_flags();
extern int const_strings;
extern int same_erettype;
#endif


#include <sys/types.h>
#include <sys/times.h>
#include <sys/mman.h>
static mprotect_flag;

int
main(argc,argv)
int argc;
char * argv[];
{
    int retval;
    char  label[256];

#ifdef TIMEIT
    int the_time;
    struct tms tbuf;
    times(&tbuf);
    the_time = tbuf.tms_utime;
#endif
	
    (void)setlocale(LC_MESSAGES,"");
    (void)setcat("uxcds");
    (void)sprintf(label,"UX:%sacomp",SGS);
    (void)setlabel(label);
    language = C_language;
    retval = setup(argc, argv);
    if(mprotect_flag) {
	int *p;
	p = 0;  p = (int *)(*p);
	mprotect(0,4096,0);
    }
    retval |= process();
    retval |= cleanup();

#ifdef TIMEIT
    times(&tbuf);
    the_time = tbuf.tms_utime - the_time;
    pfmt(stderr, MM_NOSTD,":379:THE_TIME %d\n",the_time);
#endif

    return( retval ? 2 : 0 );
}

static int pponly = 0;			/* 1 if only preprocess */

static int
setup(argc, argv)
int argc;
char * argv[];
/* Do initializations of compiler and, if appropriate, cpp
** and lint.
*/
{
    static void setsig();
    extern void sh_proc();
    extern char * optarg;
    extern int optind;
    int c;
    char * infname = "";		/* Presumed input filename (standard in) */
#ifndef MERGED_CPP
    char * outfname = "";		/* Presumed output filename */
    char * errfname = "";		/* Filename for error messages. */
#endif

#ifndef LINT
#define	COMP_ARGS "1:2:G:OF:X:L:Vac:d:bf:i:o:vwpQ:R:Z:$"
#else
#define	COMP_ARGS "1:2:G:OX:L:Vac:d:f:i:o:vwpQ:R:Z:"
#endif
    static char args[100] = COMP_ARGS;	/* may append CPP args, too */

#ifdef	MERGED_CPP
    if( PP_INIT(argv[0], sh_proc) == PP_FAILURE)
	return( 1 );			/* initialization failure */
    (void) strcat(args, PP_OPTSTRING()); /* add in CPP options, which are
					** ignored
					*/
#endif
#ifdef	LINT
    (void) strcat(args, ln_optstring()); /* add in lint options */
#else /*!LINT*/
#ifndef NODBG
    {   /* Hackwork to control inlining from environment */
#ifndef __STDC__
	long strtol();
	extern char *getenv();
#endif
	extern inline_expansion;
	char *str, *ptr;
	str = getenv("IL_EXPANSION");
	if(str) {
		long val = strtol(str,&ptr,10);
		if(ptr != str)
			inline_expansion = (int)val;
	}
    }
#endif /*NODBG*/
#endif /*LINT*/

    while (optind < argc) {
	c = getopt(argc, argv, args);
	switch( c ) {
	    char * cp;			/* for walking optarg */
	case '1':
	    /* Pass 1 flags. */
	    cp = optarg;
	    while(*cp) {
		switch( *cp ) {
		case 'a':	++a1debug; break;
#ifndef LINT
		case 'c':	++const_strings; break;
		case 'e':	same_erettype = *++cp - '0'; break;
#endif
		case 'b':	++b1debug; break;
		case 'd':	++d1debug; break;
		case 'f': {
			extern int suppress_div_to_mul;
				/* Don't do the div to
				** mul conversion even if
				** under noieee
				*/
			suppress_div_to_mul = 1;
			break;
		}
		case 'i':	++i1debug; break;
		case 'm':	++mprotect_flag; break;
		case 'o':	++o1debug; break;
		case 'r':	++r1debug; break;
		case 's':	++s1debug; break;
		{
		    extern int yydebug;		/* yacc's debug flag */
		    extern int print_cgq_flag;
		case 'y':	++yydebug; break;
#ifndef NODBG
		case 't': ++print_cgq_flag; break;
#endif
		}
		case 'z':
		    p2flags("z");		/* really a Pass 2 flag */
		    break;
#ifndef LINT
		case 'I':
		    inline_flags(++cp);
		    break;

		case 'R':
		    ++cp;
		    set_parms_in_regs_flag(cp);
		    break;
#endif
		}
		++cp;
	    }
	    break;
	case '2':
	    /* Pass 2 (CG) debugging flags. */
	    p2flags(optarg);
	    break;
#ifndef NO_AMIGO
	case 'G':
	    amigo_flags(optarg);
	    break;
	case 'O':
	    {
	    extern do_amigo_flag;	
#ifndef NODBG
	    extern void init_save_aflags();
	    init_save_aflags();
#endif
#ifndef LINT
	    inline_flags("8" /* REM_UNREF_STAT */);
#endif
	    do_amigo = do_amigo_flag;	/*
					** If -G was seen first and the no_amigo
					** option was parsed ( ~all ) we
					** don't want do_amigo turned back
					** on just because cc passed us the
					** -O flag.
					*/
	    break;
	    }
#endif
	case 'a':
	{
	    extern int err_dump;
	    err_dump = 1;		/* force dump on compiler error */
	    break;
	}
#ifndef LINT
	case 'V':
	    pfmt(stderr,MM_INFO,":380:%s %s\n", CPL_PKG,CPL_REL);
	    break;
	case 'Q':
	    if (optarg[1] == '\0') {
		if (optarg[0] == 'y') {
		    vstamp = 1;
		    break;
		}
		else if (optarg[0] == 'n') {
		    vstamp = 0;
		    break;
		}
	    }
	    UERROR(gettxt(":265","bad -Q"));
	    break;
#endif
	case 'R':
	    /* Choose register allocation style. */
#ifdef FAT_ACOMP
	    switch (*optarg) {
	    case 'g':	al_regallo = RA_GLOBAL; break;
	    case 'o':	al_regallo = RA_OLDSTYLE; break;
	    case 'n':	al_regallo = RA_NONE; break;
	    default:	WERROR(gettxt(":266","unknown allocation style '%c'"), *optarg); break;
	    }
#endif
	    break;
	case 'X':
	    /* Choose language version. */
	    switch (*optarg) {
	    case 't':	version = V_CI4_1; break;	/* Transition */
	    case 'a':	version = V_ANSI; break;	/* ANSI interp. */
	    case 'c':	version = (V_ANSI|V_STD_C); break; /* strict */
	    default:	UERROR(gettxt(":267","unknown language version '%c'"), *optarg);
	    }
	    if (optarg[1] != '\0')
		UERROR(gettxt(":268","language version \"%s\"?"), optarg);
	    break;
	case 'Z':
#ifdef	PACK_PRAGMA
	    if (optarg[0] == 'p' && optarg[1] != '\0')
		Pack_align = Pack_default = Pack_string(&optarg[1]);
	    else
#endif
	    {
		WERROR(gettxt(":269","invalid -Z"));
	    }
	    break;
	case 'v':
	    ++verbose; break;
	case 'w':
	    ++suppress_warnings; break;
	case 'p':			/* Turn on profiling. */
	    cg_profile(); break;
	case 'L':
	{
	    int code;

	    /* Select loop generation code. */
	    switch (optarg[0]) {
	    case 't':	code = LL_TOP; break;
	    case 'b':	code = LL_BOT; break;
	    case 'd':	code = LL_DUP; break;
	    default:
		WERROR(gettxt(":270","loop code type %c?"), *optarg);
		goto noselect;
	    }
	    if (optarg[1] == ',') {
		switch( optarg[2] ) {
		case 'w':	sm_while_loop_code = code; break;
		case 'f':	sm_for_loop_code = code; break;
		default:
		    WERROR(gettxt(":271","loop type %c?"), optarg[2]);
		}
	    }
	    else
		sm_while_loop_code = sm_for_loop_code = code;
noselect:;
	    break;
	}
	case 'c':
	    /* Specify "signedness" of "plain" chars */
	    switch (*optarg) {
	    case 's':	chars_signed = 1; break;
	    case 'u':	chars_signed = 0; break;
	    default:	UERROR(gettxt(":1622","illegal -c option to acomp, '%c'"), *optarg);
	    }
	    if (optarg[1] != '\0')
		UERROR(gettxt(":1623","-c option to acomp ? \"%s\"?"), optarg);
	    break;
	case 'd':
	{
	    for (cp = optarg; *cp; ++cp) {
		switch(*cp) {
		/* These affect debugging level.  These values are
		** implicit here.
		**	0	DB_LEVEL2
		**	1	DB_LEVEL0
		**	>=2	DB_LEVEL1
		*/
		case 'l':
#ifndef LINT
		    ++db_linelevel;
#endif
		    break;
		case 's':
#ifndef LINT
		    ++db_symlevel;
#endif
		    break;
		case '1':
#ifndef LINT
		    db_format = DB_DWARF_1;
#endif
		    break;
		case '2':
#ifndef LINT
		    db_format = DB_DWARF_2;
		    set_module_at_a_time();
#endif
		    break;
		case 'n':	/* lookup by name option */
#ifndef LINT
		    if (IS_DWARF_1()) {
			WERROR(gettxt(":1624", "option -d%c is valid only for Dwarf 2"), (int)'n');
		    }  else {
			db_name_lookup = 1;
		    }  /* if */
#endif
		    break;
		case 'a':	/* generate address ranges option */
#ifndef LINT
		    if (IS_DWARF_1()) {
			WERROR(gettxt(":1624", "option -d%c is valid only for Dwarf 2"), (int)'a');
		    }  else {
			db_addr_ranges = 1;
		    }  /* if */
#endif
		    break;
		case 't':	/* generate abbreviation table option */
#ifndef LINT
		    if (IS_DWARF_1()) {
			WERROR(gettxt(":1624", "option -d%c is valid only for Dwarf 2"), (int)'t');
		    }  else {
			db_abbreviations = 1;
		    }  /* if */
#endif
		    break;
		default:
		    WERROR(gettxt(":272","-d%c?"), *cp);
		}
	    }
	}
	    break;
	case 'i':			/* Set input filename. */
	    infname = optarg; break;
	case 'o':			/* Set output filename. */
#ifndef MERGED_CPP
	    outfname = optarg; 
#endif
	    break;
	case 'f':
#ifndef MERGED_CPP
	    errfname = optarg; 
#endif
	    break;
	case 'E':
	case 'P':
	    pponly = 1; break;		/* only possible if MERGED_CPP */
#ifndef LINT
	case '$':
	    allow_dollar_in_id();
	    break;
#endif
	default:
	    break;			/* ignore other options */
	} /* end switch */

#ifndef NO_AMIGO
	if (do_amigo) sm_while_loop_code = sm_for_loop_code = LL_DUP;
#endif
	
#ifdef	MERGED_CPP
	/* Pass options to CPP. */
	if (PP_OPT(c, c == EOF ? argv[optind] : optarg) == PP_FAILURE)
	    return( 3 );		/* error processing args. */
#else
	if (c == EOF) {
	    /* Assume there may be further arguments, but that what
	    ** we're looking at are filename arguments.
	    */
	    if (*infname == '\0')
		infname = argv[optind];
	    else if (*outfname == '\0')
		outfname = argv[optind];
	}
#endif
#ifdef	LINT
	if (ln_opt(c, c == EOF ? argv[optind] : optarg))
	    return( 4 );
#endif
	if (c == EOF)
	    ++optind;			/* bump past current arg. */
    } /* end while */

    /* If the debug format is Dwarf 2 (selected or default) and debugging
       information is to be generated, the compilation must be done
       a-module-at-a-time in order to capture all the source file
       info.
    */
#ifndef LINT
    if (   db_symlevel == DB_LEVEL2
        && db_format == DB_DWARF_2) {
    	set_module_at_a_time();
    }
#endif

#ifdef	MERGED_CPP			/* CPP does file handling */
    if (PP_OPT(EOF, (char *) 0) == PP_FAILURE)
	exit(5);			/* signal end of options */
    infname = fl_curname();		/* get current CPP filename */
#else
    if (*infname && freopen(infname, "r", stdin) == NULL) {
	UERROR(gettxt(":274","cannot open %s"), infname);
	printf("UERROR=%s\n",gettxt(":274","cannot open %s"));
	exit( 2 );
    }
    if (*outfname && freopen(outfname, "w", stdout) == NULL) {
	UERROR(gettxt(":274","cannot open %s"), outfname);
	printf("UERROR=%s\n",gettxt(":274","cannot open %s"));
	exit( 2 );
    }
    if (*errfname)
	infname = errfname;		/* for reporting purposes */

#endif	/* def MERGED_CPP */
#ifdef	LINT
    if (ln_opt(EOF, (char *) 0))
	return( 6 );
#endif
    op_numinit();			/* initialize numeric stuff */
    if (! pponly) {
	setsig();			/* set up signal handling */
	p2init();			/* initialize CG stuff */
	tt_init();			/* initialize types */
	DB_S_FILE(infname);		/* do initial debug stuff */
	cg_filename(infname);		/* Set filename for CG. */
	er_filename(infname,1);		/* Set filename for error processing. */
#ifndef LINT
	infname = getcwd((char *) 0, PATH_MAX + 1);
	DB_S_CWD(infname);
	free(infname);
#ifdef	MERGED_CPP			/* CPP does file handling */
	while (infname = fl_incdir_name()) {
	    DB_REG_INCDIR(infname);
	}  /* while */
#endif	/* def MERGED_CPP */
#endif
    }  /* if */
    return( 0 );
}

#ifdef __STDC__
void allow_dollar_in_id(void)
#else
void allow_dollar_in_id()
#endif
/* Note that the user as rquested that '$' be allowed in identifiers. */
{
    will_allow_dollar = 1;
}

static int
process()
/* Do whatever processing is called for.  Return non-zero on error. */
{
    extern int yyparse();

#ifdef MERGED_CPP
    if (pponly) {
	lx_input();			/* preprocess, flush to output */
	return( fl_numerrors() );
    }
#endif
    if (yyparse() || nerrors)		/* quit on errors */
	return( 1 );
#ifdef MERGED_CPP
    if ( fl_numerrors() )
	return( 2 );			/* had preprocessing errors */
#endif
    return( 0 );
}

static int
cleanup()
/* Clean up after processing, check for finaly errors.
** Return non-zero on errors.
*/
{
#ifndef	LINT
    if (vstamp)
	dovstamp();			/* put out version stamp */
#endif
#ifdef	MERGED_CPP
    if (pponly)
	/* PP_CLEANUP() might produce new error messages, but not a failure. */
	return( PP_CLEANUP() == PP_FAILURE || fl_numerrors() );
#endif

    sy_clear(SL_EXTERN);		/* flush symbols at external level */
    DB_E_FILE();			/* do debug stuff for end of file */

#ifndef LINT
    if( p2done() )			/* close off CG stuff */
	/* p2done() returns non-zero if file I/O errors occurred. */
	UERROR(gettxt(":275","error writing output file"));
#endif

    if (nerrors) {
#ifdef LINT
	(void) ln_cleanup();
#endif
	return( 1 );			/* could have new errors */
    }

    if (tcheck()) {
	tshow();			/* check for lost nodes */
	return( 2 );
    }
#ifdef	LINT
    if (ln_cleanup())
	return( 3 );
#endif

#ifndef NODBG
#ifndef NO_AMIGO
	{ extern void amigo_time_print(); amigo_time_print(); }
#endif
#endif

    return( 0 );
}

#ifndef LINT
static void
dovstamp()
/* Output version stamp into output file.  cg_ident()
** expects "-enclosed string.
*/
{
    char va[200];			/* presumed big enough */
    sprintf(va, "\"acomp: %s\"", CPL_REL);
#ifdef	MERGED_CPP
    if (pponly)
	CG_PRINTF(("#ident %s\n", va));
    else
#endif
	cg_ident(va);			/* generate .ident for this string */
    return;
}
#endif

static void
setsig()
/* catch signals if they're not now being ignored */
{
    static void catch_fpe();

    if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
	(void) signal(SIGHUP, earlyexit);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	(void) signal(SIGINT, earlyexit);
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	(void) signal(SIGQUIT, earlyexit);
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
	(void) signal(SIGTERM, earlyexit);
    
    /* catch floating error */

#ifdef	IEEE_HOST
    /* Guarantee traps on overflow, invalid operations, divide by 0, etc. */
    (void) fpsetmask(FP_X_OFL|FP_X_INV|FP_X_DZ);
#endif
    (void) signal(SIGFPE, catch_fpe);
    return;
}

void
earlyexit()
/* Exit compiler early with error return code.  Clean up
** CG interface first.
*/
{
#ifdef LINT
    (void) ln_cleanup();
#endif
    p2abort();
    exit(10);				/* random non-0 number */
    /*NOTREACHED*/
}

/* Floating point exception handling */
static void (*fpe_func)();

void
save_fpefunc(p)
void (*p)();
/* Save the name of the fp exception handler routine. */
{
    fpe_func = p;
    return;
}

/*ARGSUSED*/
static void
catch_fpe(sig)
int sig;
/* Catch floating-point exceptions and handle them gracefully. */
{
    if (fpe_func){	/* handle floating exception as setup in optim.c */
	if (signal(SIGFPE, SIG_IGN) != SIG_IGN)		/* reset signal */
	    (void) signal(SIGFPE, catch_fpe);
	(*fpe_func)();			/* call fpe handler */
    }
    else UERROR(gettxt(":276","floating-point constant folding causes exception"));
    earlyexit();
}

