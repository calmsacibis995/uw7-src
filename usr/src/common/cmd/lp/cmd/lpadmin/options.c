/*		copyright	"%c%" 	*/


#ident	"@(#)options.c	1.2"
#ident  "$Header$"

#include <sys/param.h>
#include <mac.h>
#include <audit.h>
#include "ctype.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "lp.h"
#include "printers.h"

#define	WHO_AM_I	I_AM_LPADMIN
#include "oam.h"

#include "lpadmin.h"

#ifdef	NETWORKING
#if	defined(CAN_DO_MODULES)
#define	OPT_LIST	"A:ac:d:D:e:f:F:H:hi:I:lm:Mo:O:p:Q:r:R:S:s:T:u:U:v:W:x:"
#else
#define	OPT_LIST	"A:ac:d:D:e:f:F:hi:I:lm:Mo:O:p:Q:r:R:S:s:T:u:U:v:W:x:"
#endif	/*  CAN_DO_MODULES  */
#else	/*  NETWORKING  */
#if	defined(CAN_DO_MODULES)
#define	OPT_LIST	"A:ac:d:D:e:f:F:H:hi:I:lm:Mo:O:p:Q:r:S:T:u:v:W:x:"
#else
#define	OPT_LIST	"A:ac:d:D:e:f:F:hi:I:lm:Mo:O:p:Q:r:S:T:u:v:W:x:"
#endif	/*  CAN_DO_MODULES  */
#endif	/*  NETWORKING  */

#define	MALLOC(pointer) \
	if (!(pointer = strdup(optarg))) { \
		LP_ERRMSG (ERROR, E_LP_MALLOC); \
		done (1); \
	} else

#define	REALLOC(pointer) \
	if (!(pointer = realloc(pointer, (unsigned) (strlen(pointer) + 1 + strlen(optarg) + 1)))) { \
		LP_ERRMSG (ERROR, E_LP_MALLOC); \
		done (1); \
	} else if (strcat(pointer, " ")) \
		(void)strcat (pointer, optarg); \
	else

extern char		*optarg;

extern int		optind,
			opterr,
			optopt;

extern double		strtod();

extern long		strtol();

int			a	= 0,	/* alignment needed for mount */
			banner	= -1,	/* allow/don't-allow nobanner */
#if	defined(DIRECT_ACCESS)
			C	= 0,	/* direct a.o.t. normal access */
#endif
			filebreak	= 0,
			h	= 0,	/* hardwired terminal */
			j	= 0,	/* do -F just for current job */
			l	= 0,	/* login terminal */
			M	= 0,	/* do mount */
			o	= 0,	/* some -o options given */
			Q	= -1,	/* queue threshold for alert */
			W	= -1;	/* alert interval */

char			*A	= 0,	/* alert type */
			*c	= 0,	/* class name */
			*cpi	= 0,	/* string value of -o cpi= */
			*d	= 0,	/* default destination */
			*O	= 0,	/* default copy mode */
			*D	= 0,	/* description */
			*e	= 0,	/* copy existing interface */
			*f	= 0,	/* forms list - allow/deny */
			*F	= 0,	/* fault recovery */
			**H	= 0,	/* list of modules to push */
			*i	= 0,	/* interface pathname */
			**I	= 0,	/* content-type-list */
			*length	= 0,	/* string value of -o length= */
			*lpi	= 0,	/* string value of -o lpi= */
			*m	= 0,	/* model name */
			modifications[128], /* list of mods to make */
			*p	= 0,	/* printer name */
			*r	= 0,	/* class to remove printer from */
			*stty	= 0,	/* string value of -o stty= */
			**S	= 0,	/* -set/print-wheel list */
			**T	= 0,	/* terminfo names */
			*u	= 0,	/* user allow/deny list */
			*U	= 0,	/* dialer_info */
			*v	= 0,	/* device pathname */
			*width	= 0,	/* string value of -o width= */
			*x	= 0;	/* destination to be deleted */

char			*s	= 0,	/* system printer is on */
			**R	= 0;	/* level range for remote printer*/

#ifdef	NETWORKING
			/*  Level range for the R */
level_t			R_hilevel = PR_DEFAULT_HILEVEL,
			R_lolevel = PR_DEFAULT_LOLEVEL;
#endif
SCALED			cpi_sdn = { 0, 0 },
			length_sdn = { 0, 0 },
			lpi_sdn = { 0, 0 },
			width_sdn = { 0, 0 };

static char		*modp	= modifications;

static void		oparse();

static char *		empty_list[] = { 0 };

/*
 * Procedure:     options
 *
 * Restrictions:
 *               lvlin: None
 *               CutAuditRec: None
 * notes - PARSE COMMAND LINE ARGUMENTS INTO OPTIONS
 */

void			options (argc, argv)
	int			argc;
	char			*argv[];
{
	int			optsw,
				ac;

	char			*cp,
				*rest,
				**av;
	char	*argvp;
	static char	*cmdline = (char *)0;
	char	*cmdname= "lpadmin";

#if	defined(__STDC__)
	typedef char * const *	stupid;	
#else
	typedef char **		stupid;
#endif

	
	/* save command line arguments for auditing*/
	if (( cmdline = (char *)argvtostr(argv)) == NULL) {
                printf("failed argvtostr\n");
                done(1);
        }


	/*
	 * Add a fake value to the end of the "argv" list, to
	 * catch the case that a valued-option comes last.
	 */
	av = malloc((argc + 2) * sizeof(char *));
	for (ac = 0; ac < argc; ac++)
		av[ac] = argv[ac];
	av[ac++] = "--";

	opterr = 0;
	while ((optsw = getopt(ac, (stupid)av, OPT_LIST)) != EOF) {
	switch (optsw) {
	/*
	 * These options MAY take a value. Check the value;
	 * if it begins with a '-', assume it's really the next
	 * option.
	 */
	case 'd':
	case 'p':	/* MR bl87-27863 */
	case 'I':
#if	defined(CAN_DO_MODULES)
	case 'H':
#endif
		if (*optarg == '-') {
			/*
			 * This will work if we were given
			 *
			 *	-x -foo
			 *
			 * but would fail if we were given
			 *
			 *	-x-foo
			 */
			optind--;
			switch (optsw) {
			case 'd':
			case 'O':
#if	defined(CAN_DO_MODULES)
			case 'H':
#endif
				optarg = NAME_NONE;
				break;
			case 'p':
				optarg = NAME_ALL;
				break;
			case 'I':
				optarg = 0;
				break;
			}
		}
		break;

	/*
	 * These options MUST have a value. Check the value;
	 * if it begins with a dash or is null, complain.
	 */
	case 'Q':
	case 'W':
		/*
		 * These options take numeric values, which might
		 * be negative. Negative values are handled later,
		 * but here we just screen them.
		 */
		(void)strtol(optarg, &rest, 10);
		if (!rest || !*rest)
			break;
		/*FALLTHROUGH*/
	case 'A':
	case 'c':
	case 'e':
	case 'f':
	case 'F':
	case 'i':
	case 'm':
	case 'o':
/*	case 'p': */	/* MR bl87-27863 */
	case 'r':
	case 'R':
	case 'S':
	case 's':
	case 'T':
	case 'u':
	case 'U':
	case 'v':
	case 'x':
	case 'O':	/* MR ul90-15108 */
		/*
		 * These options also must have non-null args.
		 */
		if (!*optarg)
		{
			(cp = "-X")[1] = optsw;
			LP_ERRMSG1 (ERROR, E_LP_NULLARG, cp);
			done (1);
		}
		if (*optarg == '-')
		{
			(cp = "-X")[1] = optsw;
			LP_ERRMSG1 (ERROR, E_LP_OPTARG, cp);
			done (1);
		}
		break;
	case 'D':
		/*
		 * These options can have a null arg.
		 */
		if (*optarg == '-')
		{
			(cp = "-X")[1] = optsw;
			LP_ERRMSG1 (ERROR, E_LP_OPTARG, cp);
			done (1);
		}
		break;
	}  /*  switch  */
	
	switch (optsw) {
	case 'a':	/* alignment pattern needed for mount */
		a = 1;
		break;

	case 'A':	/* alert type */
		if (A)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'A');
		MALLOC(A);
		if (!STREQU(A, NAME_QUIET) && !STREQU(A, NAME_LIST))
			*modp++ = 'A';
		break;

	case 'c':	/* class to insert printer p */
		if (c)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'c');
		MALLOC(c);
	break;

#if	defined(DIRECT_ACCESS)
	case 'C':	
		C = 1;
		break;
#endif

	case 'd':	/* system default destination */
		if (d)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'd');
		MALLOC(d);
		break;

	case 'O':	/* system default destination */
		if (O)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'O');
		MALLOC(O);
		if ( !STREQU(optarg, "copy") && !STREQU(optarg, "nocopy")) {
			/* Bad argument given with -O option */
			LP_ERRMSG (ERROR, E_ADM_BADARG);
			done(1);
		} 
		break;

	case 'D':	/* description */
		if (D)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'D');
		MALLOC(D);
		*modp++ = 'D';
		break;

	case 'e':	/* existing printer interface */
		if (e)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'e');
		MALLOC(e);
		*modp++ = 'e';
		break;

	case 'f':	/* set up forms allow/deny */
		if (f)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'f');
		MALLOC(f);
		break;

	case 'F':	/* fault recovery */
		if (F)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'F');
		MALLOC(F);
		*modp++ = 'F';
		break;

#if	defined(CAN_DO_MODULES)
	case 'H':
		if (H)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'H');
		if (!optarg || !*optarg || STREQU(NAME_NONE, optarg))
			H = empty_list;
		if (!(H = getlist(optarg, LP_WS, LP_SEP))) {
			LP_ERRMSG (ERROR, E_LP_MALLOC);
			done(1);
		}
		*modp++ = 'H';
		break;
#endif
			
	case 'h':	/* hardwired terminal */
		h = 1;
		*modp++ = 'h';
		break;

	case 'i':	/* interface pathname */
		if (i)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'i');
		MALLOC(i);
		*modp++ = 'i';
		break;

	case 'I':	/* content-type-list */
		if (I)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'I');
		if (!optarg || !*optarg || STREQU(NAME_NONE, optarg))
			I = empty_list;
		else if (!(I = getlist(optarg, LP_WS, LP_SEP))) {
			LP_ERRMSG (ERROR, E_LP_MALLOC);
			done (1);
		}
		*modp++ = 'I';
		break;
#ifdef	NETWORKING
	/*
	**  level range of remote printer
	*/
	case 'R':
		if (R)
		{
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'R');
		}
		else
		if (!(R = getlist (optarg, LP_WS, LP_SEP)))
		{
			LP_ERRMSG (ERROR, E_LP_MALLOC);
			done (1);
		}
		if (lenlist (R) > 2)
		{
			LP_ERRMSG (ERROR, E_ADM_2MANY_RANGES);
			done (1);
		}
		if (lvlin (*R, &R_hilevel) < 0)
		{
			LP_ERRMSG1 (ERROR, E_ADM_INVALID_LEVEL, *R);
			/* ul90-35225 if both levels are bad report it
			 * now instead of after user fixes the 1st. abs s19
			 */
			if (*(R+1) && lvlin (*(R+1),&R_lolevel) < 0 )
			    LP_ERRMSG1 (ERROR, E_ADM_INVALID_LEVEL, *(R+1));
			done (1);
		}
		if (! *(++R))
		{
			R_lolevel = R_hilevel;
			R--;
			*modp++ = 'R';
			break;
		}
		/* ul90-35225 give invalid msg not dominate msg for 
		 * illegal R_lolevel.  abs s19
		 */
		if (lvlin (*R, &R_lolevel) < 0 )
		{
			LP_ERRMSG1 (ERROR, E_ADM_INVALID_LEVEL, *R);
			done (1);
		}
		if (lvldom (&R_hilevel, &R_lolevel) != 1)
		{
			LP_ERRMSG2 (ERROR, E_ADM_NO_DOMINATE, *(R-1), *R);
			done (1);
		}
		R--;
		*modp++ = 'R';
		break;
#endif

#if	defined(J_OPTION)
	case 'j':	/* fault recovery just for current job */
		j = 1;
(void) printf ("Sorry, the -j option is currently broken\n");
		break;
#endif

	case 'l':	/* login terminal */
		l = 1;
		*modp++ = 'l';
		break;

	case 'm':	/* model interface */
		if (m) 
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'm');
		MALLOC(m);
		*modp++ = 'm';
		break;

	case 'M':	/* a mount request */
		M = 1;
		break;

	case 'o':	/* several different options */
		oparse (optarg);
		o = 1;
		break;

	case 'p':	/* printer name */
		if (p) 
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'p');
		MALLOC(p);
		break;

	case 'Q':
		if (Q != -1)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'Q');
		if (STREQU(NAME_ANY, optarg))
			Q = 1;
		else {
			Q = strtol(optarg, &rest, 10);
			if (Q < 0) {
				LP_ERRMSG1 (ERROR, E_LP_NEGARG, 'Q');
				done (1);
			}
			if (rest && *rest) {
				LP_ERRMSG1 (ERROR, E_LP_GARBNMB, 'Q');
				done (1);
			}
			if (Q == 0) {
				LP_ERRMSG1 (ERROR, E_ADM_ZEROARG, 'Q');
				done (1);
			}
		}
		*modp++ = 'Q';
		break;

	case 'r':	/* class to remove p from */
		if (r) 
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'r');
		MALLOC(r);
		break;

	case 'S':	/* char_set/print-wheels */
		if (S) 
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'S');
		if (!(S = getlist(optarg, LP_WS, LP_SEP))) {
			LP_ERRMSG (ERROR, E_LP_MALLOC);
			done (1);
		}
		*modp++ = 'S';
		break;

	case 's':
		if (s)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 's');

		if ((cp = strchr(optarg, '!')))
			*cp = '\0';

		if (STREQU(optarg, NAME_NONE))
			s = Local_System;
		else
		if (STREQU (optarg, Local_System))
		{
			if (cp)
			{
				LP_ERRMSG (ERROR, E_ADM_NAMEONLOCAL);
				done(1);
			}
			else
				s = Local_System;
		}
		else
		{
			if (cp)
			    *cp = '!';

			MALLOC(s);
		}

		*modp++ = 'Z';	/* 's' already used for stty 'R' for remote? */
		break;

	case 'T':	/* terminfo names for p */
		if (T)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'T');
		if (!(T = getlist(optarg, LP_WS, LP_SEP))) {
			LP_ERRMSG (ERROR, E_LP_MALLOC);
			done (1);
		}
		*modp++ = 'T';
		break;

	case 'u':	/* user allow/deny list */
		if (u)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'u');
		MALLOC(u);
		break;

	case 'U':	/* dialer_info */
		if (U)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'U');
		MALLOC(U);
		*modp++ = 'U';
		break;

	case 'v':	/* device pathname */
		if (v)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'v');
		MALLOC(v);
		*modp++ = 'v';
		break;

	case 'W':	/* alert interval */
		if (W != -1)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'W');
		if (STREQU(NAME_ONCE, optarg))
			W = 0;
		else {
			W = strtol(optarg, &rest, 10);
			if (W < 0) {
				LP_ERRMSG1 (ERROR, E_LP_NEGARG, 'W');
				done (1);
			}
			if (rest && *rest) {
				LP_ERRMSG1 (ERROR, E_LP_GARBNMB, 'W');
				done (1);
			}
		}
		*modp++ = 'W';
		break;

	case 'x':	/* destination to be deleted */
		if (x)
			LP_ERRMSG1 (WARNING, E_LP_2MANY, 'x');
		MALLOC(x);
		break;

	default:
		if (optopt == '?')
		{
			usage ();
	        	CutAuditRec(ADT_LP_ADMIN,0,strlen(cmdline),cmdline);
			done (0);

		} else
		{
			(cp = "-X")[1] = optopt;
			if (strchr(OPT_LIST, optopt))
				LP_ERRMSG1 (ERROR, E_LP_OPTARG, cp);
			else
				LP_ERRMSG1 (ERROR, E_LP_OPTION, cp);
			done (1);
		}
	}  /*  switch  */
	}  /*  while  */
	

	if (optind < argc)
		LP_ERRMSG1 (WARNING, E_LP_EXTRA, argv[optind]);

	return;
}

/**
 ** oparse() - PARSE -o OPTION
 **/

static void		oparse (optarg)
	char			*optarg;
{
	register char		**list	= dashos(optarg);


	if (!list)
		return;

	for ( ; (optarg = *list); list++)

		if (STREQU(optarg, "banner")) {
			if (banner != -1)
				LP_ERRMSG1 (

					/* lpadmin -o banner -o nobanner */
					WARNING,
					E_ADM_2MANY,
					"banner/nobanner"
				);
			banner = 1;
			*modp++ = 'b';

		} else if (STREQU(optarg, "nobanner")) {
			if (banner != -1)
				LP_ERRMSG1 (
					WARNING,
					E_ADM_2MANY,
					"banner/nobanner"
				);
			banner = 0;
			*modp++ = 'b';

		} else if (STRNEQU(optarg, "length=", 7)) {
			if (length)
				LP_ERRMSG1 (
					WARNING,
					E_ADM_2MANY,
					"length="
				);
			length = (optarg += 7);

			if (!*optarg) {
				length_sdn.val = 0;
				length_sdn.sc = 0;

			} else {
				length_sdn = _getsdn(optarg, &optarg, 0);
				if (errno == EINVAL) {
					LP_ERRMSG (ERROR, E_LP_BADSCALE);
					done (1);
				}
			}
			*modp++ = 'L';

		} else if (STRNEQU(optarg, "width=", 6)) {
			if (width)
				LP_ERRMSG1 (
					WARNING,
					E_ADM_2MANY,
					"width="
				);
			width = (optarg += 6);

			if (!*optarg) {
				width_sdn.val = 0;
				width_sdn.sc = 0;

			} else {
				width_sdn = _getsdn(optarg, &optarg, 0);
				if (errno == EINVAL) {
					LP_ERRMSG (ERROR, E_LP_BADSCALE);
					done (1);
				}
			}
			*modp++ = 'w';

		} else if (STRNEQU(optarg, "cpi=", 4)) {
			if (cpi)
				LP_ERRMSG1 (WARNING, E_ADM_2MANY, "cpi=");

			cpi = (optarg += 4);

			if (!*optarg) {
				cpi_sdn.val = 0;
				cpi_sdn.sc = 0;

			} else {
				cpi_sdn = _getsdn(optarg, &optarg, 1);
				if (errno == EINVAL) {
					LP_ERRMSG (ERROR, E_LP_BADSCALE);
					done (1);
				}
			}
			*modp++ = 'c';

		} else if (STRNEQU(optarg, "lpi=", 4)) {
			if (lpi)
				LP_ERRMSG1 (WARNING, E_ADM_2MANY, "lpi=");
			lpi = (optarg += 4);

			if (!*optarg) {
				lpi_sdn.val = 0;
				lpi_sdn.sc = 0;

			} else {
				lpi_sdn = _getsdn(optarg, &optarg, 0);
				if (errno == EINVAL) {
					LP_ERRMSG (ERROR, E_LP_BADSCALE);
					done (1);
				}
			}
			*modp++ = 'M';

		} else if (STRNEQU(optarg, "stty=", 5)) {

			optarg += 5;
			if (!*optarg)
				stty = 0;

			else {
				if (strchr(LP_QUOTES, *optarg)) {
					register int		len
							= strlen(optarg);

					if (optarg[len - 1] == *optarg)
						optarg[len - 1] = 0;
					optarg++;
				}
				if (stty)
					REALLOC (stty);
				else
					MALLOC (stty);
			}
			*modp++ = 's';

		} else if (STREQU(optarg, "filebreak")) {
			filebreak = 1;

		} else if (STREQU(optarg, "nofilebreak")) {
			filebreak = 0;

		} else if (*optarg) {
			LP_ERRMSG1 (ERROR, E_ADM_BADO, optarg);
			done (1);
		}

	return;
}
