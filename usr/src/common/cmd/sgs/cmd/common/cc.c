#ident	"@(#)sgs-cmd:common/cc.c	1.232"

/*===================================================================*/
/*                                                                   */
/*         	        cc and CC Commands                           */
/*                                                                   */
/*           Drivers for C and C++ Compilation System                */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*     These commands parse the command line to set up command       */
/*     lines for and exec whatever processes (preprocessor,          */
/*     compiler, assembler, link editor, etc.) are required. 	     */
/*                                                                   */
/*     This source is conditionally compiled, via the CCflag         */
/*     preprocessor define, into either the cc driver (for C)	     */
/*     or the CC driver (for C++).  Note that the form "if (CCflag)" */
/*     is used, rather than "#ifdef CCflag", to allow the future     */
/*     possibility of run-time determination of C vs. C++ operation, */
/*     as was done in an earlier version.                            */
/*                                                                   */
/*===================================================================*/

#include	"cc.h"
#include	"abbrev.h"
#include        <unistd.h>
#include	<sys/stat.h>
#include 	<fcntl.h>			/* for open(2) */
#include	<limits.h>

#ifdef PERF
struct tbuffer	ptimes;
struct perfstat	stats[30];
int	pstat;
#endif	/* PERF */

char	*fe_out, *c_out, *as_in, *as_out;
char	*tmp2, *tmp3, *tmp4, *tmp5, *tmp6, *tmp7;
#if ASFILT
char	*tmp8;
char	*passasfilt;
#endif
char	*tmp9;

char	*passfe, *passfs, *passde, *passc0, *passc2, *passprof, *passas, 
	*passld, *crt, *crtdir, *ccrtdir, *fplibdir, *libpath;

int	Tnone	= 0;
int	Tused	= 0;
int	Tall	= 0;
int	Tlocal	= 0;
#if AUTOMATIC_TEMPLATE_INSTANTIATION
char	*passpt;
int	Tauto	= 0;
int	Tno_auto= 0;
#endif
#if INSTANTIATION_BY_IMPLICIT_INCLUSION
int	Timplicit    = 0;
int	Tno_implicit = 0;
#endif
int	Tprelink_objects = 0;

/* controls for pre-compiled headers */
int		Rflag;
const char	*Rcreate;
const char	*Ruse;

enum cplusplus_mode_type cplusplus_mode = via_glue;

enum link_mode_type {dynamic_link, static_link};
enum link_mode_type link_mode = dynamic_link;

int	cflag, Tflag, kflag, Oflag, Sflag, Vflag, eflag, dsflag,
	dlflag, ilflag, pflag, qpflag, qarg, gflag, debug;

int	Eflag	= 0;	/* preprocess only, output to stdout */
int	Pflag	= 0;	/* preprocess only, output to file.i */
int	Fflag	= 0;	/* front-end only, output to file.cll */
int	Nflag	= 0;	/* front-end only, output to file.cil */
int	vflag	= 0;	/* compiler, enable remarks */
int	wflag	= 0;	/* compiler, suppress warnings */
int	fflag	= 0;	/* C++ only, process for fs tool */
int	lflag	= 0;	/* whether -l specified */

int	Xcp;		/* index for preprocessor list, varies for cc/CC */

int	actual_cflag;	/* whether actual -c option used */

char	*ilfile, *Parg;
char	**list[NLIST];
int	nlist[NLIST], limit[NLIST];

char	*prefix;
int	c89 = 0;
int	sfile;
int	inlineflag;
int	independ_optim;
int	Ocount;
int	Add_Xc2_Qy;
char	Xc0tmp[4];

/* static common variables */

#ifdef PERF
static	FILE	*perfile;
long	ttime;
int	ii = 0;
#endif

/* number of args always passed to ld; used to determine whether
 * or not to run ld when no files are given */

#define	MINLDARGS	3

/* default options regarding ANSI conformance etc. for C and C++ */

#define DFT_X_OPT_C		"-Xa" 	/* the default for C */
#define DFT_X_OPT_CPLUSPLUS	"-Xd" 	/* the default for C++ */



static char	
	*out_name = NULL,	/* output file name, from -o option */
	*libdir = NULL,
	*llibdir = NULL;

static	char	*profdir= LIBDIR;

static	char	*ccincdir = CCINCDIR;

static int
	Qflag   = 1,    /* turn on version stamping - turn off if -Qn 
				is specified*/
	Xwarnfe	= 0,
	Xwarn	= 0,	/* warn user if more than one -X options were 
				given with differing arguments */	
	Gflag	= 0,	/* == 0 include start up routines else don't */
	strict  = 0,	/* strict POSIX2 conformance off by default */
	Kthread = 0;	/* 1 if Kthread is given */

static char	*Xargfe	= NULL; /* will hold the -X flag and argument for 
				  the C++ front end */
static char	*Xarg	= NULL; /* will hold the -X flag and argument for
				  the C compiler */
static	char	*earg	= NULL; /* will hold the -e flag and argument */
static	char	*harg	= NULL; /* will hold the -h flag and argument */
static	char	*nopt;		/* current -W flag option */

/* lists */

static int
	nxo	= 0,	/* number of .o files in list[Xld] */
	nxn	= 0,	/* number of non-.o files in list[Xld] */
	nxf	= 0,	/* number of files on command line */
	questmark = 0,	/* specify question mark on command line */
	argcount;	/* initial length of each list */


static	char	*values = VALUES;	/* name of the values-X[catdo].o file */
static	char	*optstr;	/* holds option string */

static	char
	*getsuf(),
	*getpref(),
	*makename(),
	*compat_YL_YU();

static void
	checkdir(),
	idexit(),
	initialize(),
	linkedit(),
	mk_defpaths(),
	chg_pathnames(),
	mktemps(), 
	process_lib_path(),
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	process_ii_file(),
#endif
	dexit(),
	option_CC(),
	option_indep();

static int
	CCoptelse(),
	nodup(),
	move(),
	valid_Cplusplus_suffix(const char *const s),
	getXpass(),
	frontend(),
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	ptlink(),
#endif
	compile(),
	profile(),
	assemble();

static char*
	chg_toolname(char* old_pathname, const char* old_tool, const char* new_tool);

static char psxerr[] = ":1273: cannot access %s\n";

static void
checkdir(dir)
char * dir;
{
	struct stat statbuf;

	if ((stat(dir, &statbuf) == -1) ||
		(!(S_ISDIR(statbuf.st_mode))) ||
	   	(!(statbuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))) {
			if (strict){
				 strict=2;	
				 (void) pfmt(stderr,MM_ERROR,psxerr,dir);
			} else
                              (void) pfmt(stderr,MM_WARNING,psxerr,dir);

		}
		
}


main (argc, argv)
	int argc;
	char *argv[];
{
	int	c;		/* current option char from getopt() */
	char	*t, *s, *p;	/* char ptr temporary */
	int	done;		/* used to indicate end of arguments passed by -W */
	int	i;		/* loop counter, array index */
	char 	*chpiece = NULL,	/* list of pieces whose default location has
					   been changed by the -Y option */
		*npath = NULL;	/* new location for pieces changed by -Y option */
	char   label[256];

	opterr = 0;
	
#ifdef PERF
	if ((perfile = fopen("cc.perf.info", "r")) != NULL) {
		fclose(perfile);
		pstat = 1;
	}
#endif

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcds");
	if(CCflag)
		(void)sprintf(label,"UX:%sCC",SGS);
	else
		(void)sprintf(label,"UX:%scc",SGS);
	(void)setlabel(label);
	prefix = getpref( argv[0] );

	/* initialize the lists */
	argcount = argc + 6;
	for (i = 0; i < NLIST; i++) {
		nlist[i] = 0;
		if ((list[i] = (char **)calloc((size_t)argcount,sizeof(char **))) == NULL) {
			error('e', gettxt(":35","Out of space\n"));
			dexit();
		}
		limit[i]= argcount;
	}
	initialize();

	setbuf(stdout, (char *) NULL);	/* unbuffered output */

#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if (CCflag) {
		/* point to this command so that recursive calls will work */
		addopt(XCC, argv[0]);
		addopt(XCC, " -c ");
	}
#endif

	while (optind < argc) {
#if AUTOMATIC_TEMPLATE_INSTANTIATION
		int add_to_XCC = 1;
#endif

		c = getopt(argc, argv, optstr);
		switch (c) {

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':	/* option levels -- OSR5 compatibility */
			/* OSR5 and other systems have option levels, such as
			   -O2, -w0, -g1, etc.  For makefile compatibility we
			   parse these as -O -2 etc. and ignore the level */ 
			break;

		case 'b':	/* file format -- OSR5 compatibility */
			/* -b elf accepted and ignored, -b anything else is an error */
			if (strcmp(optarg, "elf") != 0) {
				error('e', gettxt(":1638","Invalid object file format -- only ELF is supported\n"));
				exit(1);
			}
				
			break;

		case 'c':       /* produce .o files only */
			actual_cflag++;
                        cflag++;
#if AUTOMATIC_TEMPLATE_INSTANTIATION
			add_to_XCC=0;
#endif
                        break;

		case 'C':       /* tell compiler to leave comments in (lint) */
                        addopt(Xcp,"-C");
                        break;

		case 'D': 	/* Define name with value def, otherwise 1 */
			t = stralloc(strlen(optarg)+2);
                        (void) sprintf(t,"-%c%s",c,optarg);
                        addopt(Xcp,t);
                        break;

		case 'e':       /* Make name the new entry point in ld */
                                /* Take last occurrence */
                        earg = stralloc(strlen(optarg) + 3);
                        (void) sprintf(earg,"-%c%s",optopt,optarg);
                        break;
		case 'E':       /* run only preprocessor part of
                                   compiler, output to stdout */
                        Eflag++;
                        addopt(Xcp,"-E");
                        cflag++;
                        break;

		case 'g':       /* turn on symbols and line numbers */
                        dsflag = dlflag = 0;
                        gflag++;
                        break;

		case 'H':       /* preprocessor - print pathnames of 
				   included files on stderr */
                        addopt(Xcp,"-H");
                        break;

		case 'I':
			if (c89)
				checkdir(optarg);
                        t = stralloc(strlen(optarg)+2);
                        (void) sprintf(t,"-%c%s",c,optarg);
                        addopt(Xcp,t);
                        break;

		case 'l':       /* ld: include library */
			lflag++;
                        t = stralloc(strlen(optarg) + 3);
                        (void) sprintf(t,"-%c%s",optopt,optarg);
                        addopt(Xld,t);
                        break;
                case 'L':       /* ld: alternate lib path prefix */
			if (c89)
				checkdir(optarg);
                        t = stralloc(strlen(optarg) + 3);
                        (void) sprintf(t,"-%c%s",optopt,optarg);
                        addopt(Xld,t);
                        break;
		
		case 'o':       /* object file name */
                        if (!optarg[0])
				break;
			/* this option can rename .i, .s, .o, or a.out files */
			out_name = optarg;
#if 0
			s = getsuf(ld_out);
			if ( strcmp(s,"c")==0 || (CCflag && strcmp(s,"C")==0)
			    		 || strcmp(s,"i")==0 || strcmp(s,"s")==0
			     		 || (!CCflag && inlineflag && strcmp(s, "il")==0)
			     		 || (CCflag && strcmp(s,"cil")==0)
			     		 || (CCflag && strcmp(s,"cll")==0) ) {
				error('e', gettxt(":2","Illegal suffix for object file %s\n"),
					ld_out);
				exit(1);
			}
#endif
#if AUTOMATIC_TEMPLATE_INSTANTIATION
			add_to_XCC=0;
#endif
                        break;

		case 'O':       /* invoke optimizer */
			Oflag += Ocount;
                        break;

		case 'p':	/* standard profiler */
			pflag++;
			qpflag=1;
			crt = MCRT1;
			if ( qarg != 0) {
				error('w',
					gettxt(":3","using -p, ignoring the -q%c option\n"),qarg);
				qarg= 0; /* can only have one type 
						of profiling on */
			}
			break;

		case 'P':       /* run only preprocessor part of
                                   compiler, output to file.i */
                        Pflag++;
                        addopt(Xcp,"-P");
                        cflag++;
                        break;

		case 'q':	/* xprof, lprof or standard profiler */
			if (strcmp(optarg, "f") == 0) {
				addopt(Xc0, "-2C");
				addopt(Xld, "-lfprof");
			}
			else if (strcmp(optarg,"p") == 0) {
				pflag++;
				qpflag=2;
				crt = MCRT1;
				if ( qarg != 0) {
                                	error('w',
					gettxt(":4","using -qp, ignoring the -q%c option\n"),
						qarg);
                                	qarg= 0; /* can only have one 
						    type of profiling on */
                        		}

			} else if (strcmp(optarg,"l")==0 || strcmp(optarg,"x")==0) {
				qarg = optarg[0];
				if (pflag != 0) {
					if (qpflag == 1)
					   error('w',
					   gettxt(":5","using -q%c, ignoring the -p option\n"),
						qarg);
					else
					   error('w',
					   gettxt(":6","using -q%c, ignoring the -qp option\n")
						,qarg);

					pflag= 0;
				}
			} else {
				error('e', gettxt(":7","No such profiler %sprof\n"),optarg);
				exit(1);
			}
			break;

		case 'S':	/* produce assembly code only */
			Sflag++;
			cflag++;
			break;
		case 'u':       /* ld: enter sym arg as undef in sym tab */
                        addopt(Xld,"-u");
                        addopt(Xld,optarg);
                        break;

		case 'U':
                        t = stralloc(strlen(optarg)+2);
                        (void) sprintf(t,"-%c%s",c,optarg);
                        addopt(Xcp,t);
                        break;

                case 'V':       /* version flag */
			if ( !Vflag ) {
				if(CCflag) {
                        		addopt(Xfe,"-v");
#if AUTOMATIC_TEMPLATE_INSTANTIATION
                        		addopt(Xpt,"-V");
#endif
				}
                        	addopt(Xc0,"-V");
                        	addopt(Xas,"-V");
                        	addopt(Xld,"-V");
				if (CCflag)
                        		error('i', gettxt(":8","%s %s\n"), CPLUS_DRIVER_PKG, CPLUS_DRIVER_REL);
				else
                        		error('i', gettxt(":8","%s %s\n"), CPL_PKG, CPL_REL);
			}
			Vflag++;
                        break;

		case 'W':
			if (optarg[1] != ',' || optarg[2] == '\0') {
				error('e', gettxt(":9","Invalid subargument: -W%s\n"), optarg);
				exit(1);
			}
			if ((i = getXpass(optarg, "-W")) == -1) {
				error('e', gettxt(":9","Invalid subargument: -W%s\n"), optarg);
				exit(1);
			}
			/*
			 * Added capability to -W option to pass arguments
			 * which themselves contain a comma.
			 * The comma in the argument must be preceded by a
			 * '\' to distinguish it from commas used as
			 * delimiters in the -W option.
			 */
			t = optarg;
			t+=2;
			done=0;
			while (!done) {
				p=t;
				while (((s = strchr(p,',')) != NULL) &&
								(*(s-1) == '\\')) {
					p=s;
					s--;
					while (*s != '\0') {
						*s = *(s+1);
						s++;
					}
				}
				if (s != NULL)
					*s = '\0';
				else
					done=1;
				nopt =stralloc(strlen(t)+1);
				(void) strcpy(nopt, t);
				addopt(i,nopt);
				t+= strlen(t) + 1;
			}
			break;

		case 'Y':
			t = stralloc(strlen(optarg));
			(void) strcpy(t, optarg);
			if (((chpiece=strtok(t,",")) != t) ||
				((npath=strtok(NULL,",")) == NULL)) {
				error('e', gettxt(":10","Invalid argument: -Y %s\n"),optarg);
				exit(1);
			} else if ((t=strtok(NULL," ")) != NULL) {
				error('e', gettxt(":11","Too many arguments: -Y %s,%s,%s\n"),
					chpiece, npath,t);
				exit(1);
			}
			chg_pathnames(chpiece, npath);
			free(t);
			break;

		case 'A':       /* preprocessor - asserts the predicate 
				   and may associate the pp-tokens with 
				   it as if a #assert */
			if(CCflag) {
				error('w', gettxt(":1525","-A not available in C++ mode, ignored (def)\n"));
				break;
			}
                        t = stralloc(strlen(optarg) + 3);
                        (void) sprintf(t,"-%c%s",optopt,optarg);
                        addopt(Xcp,t);
                        break;

                case 'B':       /* Govern library searching algorithm in ld */
                        if((strcmp(optarg,"dynamic") == 0) ||
			   (strcmp(optarg,"static") == 0)  ||
			   (strcmp(optarg,"symbolic") == 0))
			{
                        	t = stralloc(strlen(optarg) + 3);
                        	(void) sprintf(t,"-%c%s",optopt,optarg);
                        	addopt(Xld,t);
                        } 
			else
                        	error('w', gettxt(":12","illegal option -B%s\n"),optarg);
                        break;


                case 'd':       /* Govern linking: -dy dynamic binding;
                                   -dn static binding
                                */
                        switch (optarg[0]) {
                                case 'y':
					link_mode = dynamic_link;
                                        addopt(Xld,"-dy");
                                        break;
                                case 'n':
					link_mode = static_link;
                                        addopt(Xld,"-dn");
                                        break;
                                default:
                                        error('e', gettxt(":13","illegal option -d%c\n")
						,optarg[0]);
					exit(1);
                        }
			if(optarg[1]) {
				error('e', gettxt(":14","illegal option -d%s\n"), optarg);
				exit(1);
			}

                        break;

		case 'G':       /* used with the -dy option to direct linker
                                   to produce a shared object. The start up
                                   routine (crt1.o) shall not be called */
                        Gflag++;
                        addopt(Xld, "-G");
                        break;

		case 'h':       /* ld: Use name as the output filename in the
                                   link_dynamic structure. Take last occurrence.
                                */
                        harg = stralloc(strlen(optarg) + 3);
                        (void) sprintf(harg,"-%c%s",optopt,optarg);
                        break;

		case 'Q':       /* add version stamping information */
                        switch (optarg[0]) {
                                case 'y':
                                        Qflag = 1;
                                        break;
                                case 'n':
                                        Qflag = 0;
                                        break;
                                default:
                                        error('e', gettxt(":15","illegal option -Q %c\n"),
						optarg[0]);
                                        exit(1);
                        }
                        break;

                case 'v':       /* tell compiler to run in verbose mode (produce remarks) */
			vflag++;
			/* addopt()'s will be done later, after conflicting option check */
                        break;

		case 'w':       /* tell compiler to suppress warnings */
			wflag++;
			/* addopt()'s will be done later, after conflicting option check */
			break;

                case 'X':       /* conformance options (different for cc and CC) */
			if (CCflag) {
				/* C++ conformance options */
				switch (optarg[0]) {
				case 'd':
				case 'o':
				case 'w':
				case 'e':
                        		if (Xargfe == NULL)
                        			Xargfe = stralloc(3);
					else if (Xargfe[2] != optarg[0])
                               			Xwarnfe = 1;
                        		(void) sprintf(Xargfe,"-X%c",optarg[0]);
                                       	break;
				default:	
                               		error('e',gettxt(":16","illegal option -X%c\n"),
						optarg[0]);
	                                exit(1);
				}  /* end switch */
			} else {
				/* C conformance options */
                        	switch (optarg[0]) {
                       		case 't':
                       		case 'a':
                        	case 'c':
                        		if (Xarg == NULL)
                        			Xarg = stralloc(3);
					else if (Xarg[2] != optarg[0])
                               			Xwarn = 1;
                        		(void) sprintf(Xarg,"-X%c",optarg[0]);
                                	break;
                        	default:
                               		error('e',gettxt(":16","illegal option -X%c\n"),
						optarg[0]);
	                                exit(1);
				}  /* end switch */
                        }
			break;

		case 'z':       /* turn on asserts in the linker */
                        t = stralloc(strlen(optarg) + 3);
                        (void) sprintf(t,"-%c%s",optopt,optarg);
                        addopt(Xld,t);
                        break;

		case 'K':
			t = stralloc(strlen(optarg));
			p = strcpy(t, optarg);

                        while ((s=strtok(p, ",")) != NULL)
                        {
                                if (strcmp(s, "PIC")==0)
                                        Parg = "-KPIC";
                                else if (strcmp(s, "pic")==0)
                                        Parg = "-Kpic";
				else if (strcmp(s, "schar") == 0) {
					addopt(Xfe, "-s");
                                        addopt(Xc0, "-cs");
                                } else if (strcmp(s, "uchar") == 0) {
					addopt(Xfe, "-u");
                                        addopt(Xc0, "-cu");
                                } else if (strcmp(s, "thread") == 0) {
					addopt(Xcp, "-D_REENTRANT");
					if (CCflag)
						addopt(Xc0, "-Kthread");
					Kthread = 1;	
                                } else if (strcmp(s, "dollar") == 0) {
					addopt(Xcp, "-$");
                                } else if ( !Kelse(s) ) {
					/*
					 * Kelse(), machine dependent routine,
					 * to cover more machine-specific '-Kxx'
					 * options.
					 */
                                        error('e',
                                        gettxt(":17","Illegal argument to -K flag, '-K %s'\n")
                                        	,s);
                                    	exit(1);
                                }
                                p = NULL;
                        }
			free(t);
                        break;

		case '#':	/* cc command debug flag */
			debug++;
			break;

		default:
			/*
			 * CCoptelse(), C++ specific options.
			 * optelse(), machine dependent routine, to cover
			 * more machine-specific options.
			 * If no more opt char in optstr, then pass to ld.
			 */
			if (optopt == '?' )
				questmark = 1;
			else if ( CCoptelse(optopt, optarg) ||
						optelse(optopt, optarg) )
				break;
			if ( strchr(optstr,optopt) != NULL ) {
				error('e', gettxt(":18","Option -%c requires an argument\n"),
					optopt);
				exit(1);
			}
			t = stralloc( 3 );
			(void) sprintf(t,"-%c",optopt);
			addopt(Xld,t);
			break;

		case EOF:	/* file argument */
#if AUTOMATIC_TEMPLATE_INSTANTIATION
			add_to_XCC=0;
#endif
			if ((t = argv[optind++]) == NULL) /* no file name */
				break;
			++nxf;
			s = getsuf(t);
			if ( strcmp(s,"c")==0 || (CCflag && valid_Cplusplus_suffix(s)) ||
			     strcmp(s,"i")==0 || strcmp(s,"s")==0 || Eflag) {
			     /* The Eflag is there because cc/CC -E will preprocess any
				file, regardless of suffix.  This is to give users the
				ability to use the C/C++ preprocessor as a stand-alone tool
				on non-C/C++ files.	*/
				addopt(CF,t);
				t = setsuf(t, "o");
			}
			else if ( !CCflag && inlineflag && strcmp(s, "il")==0 ) {
				/* nobody seems sure of what .il files are for (C inline 
				   expansion code template files?) or whether still needed,
				   so am leaving in for C but not recognizing for C++  */
				ilflag++;
				ilfile = t;
				break;
			}
			if (nodup(list[Xld], t)) {
				addopt(Xld,t);
				if ( strcmp(getsuf(t), "o")==0 )
					nxo++;
				else
					nxn++;
			}
			break;
		} /* end switch */

#if AUTOMATIC_TEMPLATE_INSTANTIATION
		if (CCflag && add_to_XCC) {
			/* add argument to the recursive call list */
			/* need to allocate 2 bytes for -%c, +2 for spaces */
			if(optarg) {
                        	t = stralloc(strlen(optarg)+2+2);
                        	(void) sprintf(t," -%c%s ",optopt,optarg);
			} else {
                        	t = stralloc(2+2);
                        	(void) sprintf(t," -%c ",optopt);
			}
			addopt(XCC, t);
		}
#endif

	} /* end while */

	if ( (nxo == 0) && (nxn == 0) ) {
		if(CCflag)
			error('a', gettxt(":1526","Usage: CC [ -%s ] files ...\n"), optstr);
		else
			error('a', gettxt(":19","Usage: cc [ -%s ] files ...\n"), optstr);
		if (questmark)	/* detail option information */
			option_indep();
		exit(1);
		}

	/*
	 * Two purposes for init_mach_opt():
	 * 1. add machine dependent options which should be always put here.
	 * 2. adjust some variables to force producing different behavior.
	 */
	init_mach_opt();

	if (Vflag)	/* Vflag may be cleaned up in init_mach_opt() */
		addopt(Xc2,"-V");
        if (earg != NULL)
                addopt(Xld, earg);
	if (harg != NULL)
		addopt(Xld, harg);
	if (Parg != NULL)
		addopt(Xc0, Xc0tmp);

	/* if -g and -O are given, disable -O */
	if (gflag && Oflag)
	{
		Oflag = 0;
		
		error('w', 
		gettxt(":1637","debugging and optimization mutually exclusive; -O disabled\n"));
	}

	/* if -q option is given, make sure basicblk exists */
	if (qarg) {
		if(Oflag) {
			Oflag = 0;
			error('w',
			gettxt(":20","%cprof and optimization mutually exclusive; -O disabled\n"),
			qarg);
		}
		passprof= makename(profdir,prefix,N_PROF);
		if ((debug <= 2) && (access(passprof, 0) == -1)) {
			error('e', gettxt(":21","%cprof is not available\n"),qarg);
			exit(1);
		}
		crt = PCRT1;
		dsflag = dlflag = 0;
		gflag++;
		addopt(Xld,"-lprof");
		addopt(Xld,"-lelf");
	}

	if (pflag && !Gflag) /* building a profiled executable */
	{
		addopt(Xld,"-lprof");
	}

	/* if -o is given and linking is not being done, check that there is only one source file */
	if (out_name != NULL && (cflag || Tprelink_objects) && nlist[CF] > 1) {
		error('e', gettxt(":1613","multiple source files would overwrite -o %s\n"), out_name);
		exit(1);
	}

	/* if -o flag was given, the output file shouldn't overwrite an input file */
	if (out_name != NULL)
		if (!(cflag || Tprelink_objects) && !nodup(list[Xld], out_name)) {
			/* when linking, can't overwrite the input to ld */
			error('e', gettxt(":22","-o would overwrite %s\n"), out_name);
			exit(1);
		} else 
			/* when preprocessing, compiling, or assembling, can't overwrite source input */
			for (i = 0; i < nlist[CF]; i++)
				if (!strcmp(list[CF][i], out_name)) {
					error('e', gettxt(":22","-o would overwrite %s\n"), out_name);
					exit(1);
				}

	/* if -o is given and -P or -S is active and effective, see if the 
	   output suffix looks like it's for a later, "wrong" file type and if 
	   it looks like the compilation would "normally" have gone further; if
	   so, issue a warning (because it's likely that -P/-S got stuck into
	   an existing makefile or script command line and that the renaming
	   won't be expected)
	*/
	if (out_name != NULL && (Pflag || Sflag) && nlist[CF] != 0) {
		char* s = getsuf(out_name);
		if ( (strcmp(s, "") == 0 || strcmp(s, "so") == 0 ||
		      strcmp(s, "o") == 0 || (Pflag && strcmp(s, "s") == 0))
		   && (nlist[Xld] > 1 || actual_cflag || (Pflag && Sflag)) )
			error('w', gettxt(":1704", "-o has renamed the output of %s, perhaps unintentionally\n"), Pflag ? "-P" : "-S");
	}

	/* Check for clashing -R options */
	if (CCflag)
	{
		/* currently no such checks, but reserving this capability */

		if (Rcreate) {
			addopt(Xfe,"--create_pch");
			addopt(Xfe,Rcreate);
		} 
		if (Ruse) {
			addopt(Xfe,"--use_pch");
			addopt(Xfe,Ruse);
		}

		/* enable or disable the messages about the use and creation
		   of precompiled headers */
		if (Rflag) {
			if (vflag)
				addopt(Xfe,"--pch_messages");
			else
				addopt(Xfe,"--no_pch_messages");
		}
	}

	/* -T processing.  At this point, all Txxxx variables are 1 if the
	   xxxx option was given on the command line, 0 otherwise.	*/

	/* check for clashing -T options */
	if (Tnone + Tused + Tall + Tlocal > 1) {
		error('w', gettxt(":1586","only one of -T none | used | all | local allowed; using -T none\n"));
		Tnone = 1;
		Tused = Tall = Tlocal = 0;
	}
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if (Tlocal && Tauto) {
		error('w', gettxt(":1589","-T local and -T auto conflict; using -T local and -T no_auto\n"));
		Tauto = 0;
		Tno_auto = 1;
	} else if (Tauto && Tno_auto) {
		error('w', gettxt(":1587","only one of -T auto | no_auto allowed; using -T auto\n"));
		Tauto = 1;
		Tno_auto = 0;
	} else if (Tno_auto && Tprelink_objects) {
		error('w', gettxt(":1612","-T no_auto and -T prelink_objects conflict; using -T no_auto\n"));
		Tprelink_objects = 0;
	}
#endif
#if INSTANTIATION_BY_IMPLICIT_INCLUSION
	if (Timplicit && Tno_implicit) {
		error('w', gettxt(":1588","only one of -T implicit | no_implicit allowed; using -T implicit\n"));
		Timplicit = 1;
		Tno_implicit = 0;
	}
#endif

	/* check for forcing -T options */
	if ((!cflag && !Eflag && !Fflag && !Nflag && !Pflag) && nxf==1 && nlist[CF] == 1 && (valid_Cplusplus_suffix(getsuf(list[CF][0])) || strcmp(getsuf(list[CF][0]),"i")==0) && !lflag && !Tflag)
		/* simple compile and link with single source file and no .o's
		   or libraries and no template options (i.e. we know everything
		   about this compilation from this single file), so can use
		   "instantiation used" option (note when standard library
		   gets templates this may not be possible)	*/
		Tused = 1;
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if ((Tused || Tall) && !Tauto)
		/* these force off defaultness of auto instantiation */
		Tno_auto = 1;
#endif

	/* now set -T defaults */
	if (!Tnone && !Tused && !Tall && !Tlocal)
		Tnone = 1;
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if (!Tauto && !Tno_auto)
		Tauto = 1;
#endif
#if INSTANTIATION_BY_IMPLICIT_INCLUSION
	if (!Timplicit && !Tno_implicit)
		Timplicit = 1;
#endif

	/* now communicate -T options to C++ front end */
	if (Tnone)
		/* even though this is a default to FE, put it out */
		addopt(Xfe, "-tnone");
	else if (Tused)
		addopt(Xfe, "-tused");
	else if (Tall)
		addopt(Xfe, "-tall");
	else if (Tlocal)
		addopt(Xfe, "-tlocal");
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if (Tauto)
		addopt(Xfe,"--auto_instantiation");
	else
		addopt(Xfe,"--no_auto_instantiation");
#endif
#if INSTANTIATION_BY_IMPLICIT_INCLUSION
	if (Timplicit)
		addopt(Xfe,"--implicit_include");
	else
		addopt(Xfe,"--no_implicit_include");
#endif

#if 0
	/* The following commented-out code recognizes how the FE is built
	   and so minimizes the options to have to be explicitly passed to
	   it.  But for early testing and experience, being able to see
	   the explicit options via CC -# is better, thus the #if 0.	*/
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if (Tauto) {
		/* The user wants auto instantiation on, so what happens
		 * here depends on the way the front end
		 * was configured. If it is configured with automatic
		 * instantiation on by default then do nothing here,
		 * otherwise turn it on now.
		 */
#if !DEFAULT_AUTOMATIC_INSTANTIATION_MODE
		addopt(Xfe,"--auto_instantiation");
#endif
	} else {
		/* The user wants auto instantiation off, so if the front
		 * end was configured with automatic instantiation
		 * on then turn it off here, otherwise do nothing. 
		 */
#if DEFAULT_AUTOMATIC_INSTANTIATION_MODE
		addopt(Xfe,"--no_auto_instantiation");
#endif
	}
#endif
#if INSTANTIATION_BY_IMPLICIT_INCLUSION
	if (Timpl) {
		/* The user wants implicit inclusion on, so what happens
		 * here depends on the way the front end
		 * was configured. If it is configured with implicit
		 * inclusion on by default then do nothing here,
		 * otherwise turn it on now.
		 */
#if !DEFAULT_IMPLICIT_TEMPLATE_INCLUSION_MODE
		addopt(AV,"--implicit_include");
#endif
	} else {
		/* The user wants implicit inclusion off, so if the front
		 * end was configured with implicit inclusion 
		 * on then turn it off here, otherwise do nothing. 
		 */
#if DEFAULT_IMPLICIT_TEMPLATE_INCLUSION_MODE
		addopt(AV,"--no_implicit_include");
#endif
	}
#endif
#endif

	/* if -v and -w are both given, issue warning and disable -w */
	if (vflag && wflag) {
		wflag = 0;
		error('w', gettxt(":1552","-v and -w options mutually exclusive; -w disabled\n"));
	}
	/* set the component options on for -v and -w, if specified */
	if (vflag) {
		if (CCflag) {
			addopt(Xfe,"-r");     /* get remarks from EDG FE */
                	addopt(Xc0,"-1I32");  /* get inline warnings from acomp */
#if AUTOMATIC_TEMPLATE_INSTANTIATION
			addopt(Xpt,"-v");     /* get "assignment" messages from prelinker */
#endif
		} else
			addopt(Xc0,"-v");
	}
	if (wflag)
		if (CCflag){
			addopt(Xfe,"-w");
			addopt(Xc0,"-w");
		} else
			addopt(Xc0,"-w");

	if (signal(SIGHUP, SIG_IGN) == SIG_DFL)
		(void) signal(SIGHUP, idexit);
	if (signal(SIGINT, SIG_IGN) == SIG_DFL)
		(void) signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
		(void) signal(SIGTERM, idexit);

	if ( !(Pflag || Eflag || Nflag || Fflag) || fflag ) 
						   /* more than preprocessor or
						    * front end is running, so
						    * temp files are required */
		mktemps();

	if (eflag)
		dexit();

	mk_defpaths();

	/*
	 * To add more machine dependent options which are difficult to
	 * or should not be handled by init_mach_opt().
	 */
	add_mach_opt();

	if (Parg != NULL)
		addopt(Xc2, Parg);
	if (Qflag) {
	    	addopt(Xfe,"-Qy");
		addopt(Xc0,"-Qy");
		addopt(Xas,"-Qy"); 
	    /*	addopt(Xpt,"-Qy"); 	not yet implemented */
		addopt(Xld,"-Qy");
		if (Oflag && Add_Xc2_Qy) 
			addopt(Xc2,"-Qy");
	}

	if ( Xarg != NULL) { /* if more then one -X option and each has a different */
		if ( Xwarn)  /* argument, then warning */
		error('w', gettxt(":23","using %s, ignoring all other -X options\n"),Xarg);
	}	
	else
		Xarg = DFT_X_OPT_C;

	addopt(Xc0,Xarg);

	if (Oflag)
		addopt(Xoptim,Xarg);

	if (CCflag) {
		if (Xargfe == NULL) {
			/* set the default here in user option's terms */
			Xargfe = DFT_X_OPT_CPLUSPLUS;
	 	}
		else {
			if (Xwarnfe)
				error('w',gettxt(":23","using %s, ignoring all other -X options\n"),Xargfe);
		}

		/* now translate either default or user option into EDG's option */
		switch(Xargfe[2]) {
		case 'o':
			addopt(Xfe,"--cfront_3.0");
			/* these are added because EDG doesn't include them in
			   cfront compatibility; when they do, remove these */
			addopt(Xfe,"--no_distinct_template_signatures");
			addopt(Xfe,"--no_exceptions");
			addopt(Xfe,"--no_rtti");
			addopt(Xfe,"--no_array_new_and_delete");
			addopt(Xfe,"--no_explicit");
			addopt(Xfe,"--no_namespaces");
			addopt(Xfe,"--no_extern_inline");
			addopt(Xfe,"--no_wchar_t_keyword");
			addopt(Xfe,"--no_bool");
			addopt(Xfe,"--no_typename");
			addopt(Xfe,"--no_alternative_tokens");
			break;
		case 'w':
			addopt(Xfe,"-a");
			break;
		case 'e':
			addopt(Xfe,"-A");
			break;
		default: /* covers -Xd */
			break;
		}
	}

	/* process each file (name) in list[CF] */

	for (i = 0; i < nlist[CF]; i++) {
		if (nlist[CF] > 1)
			/* if more than one source file, write out name of each as it is compiled */
			error('c', gettxt(":24","%s:\n"), list[CF][i]);

		if ( CCflag && ( (strcmp(list[CF][i],".C") == 0) ||
				 (strcmp(list[CF][i],".c") == 0) ||
                                 (strcmp(list[CF][i]+strlen(list[CF][i])-3,"/.C") == 0) ||
				 (strcmp(list[CF][i]+strlen(list[CF][i])-3,"/.c") == 0) ) ) {
                        /* acomp can handle this, but EDG front end cannot, so reject */
			error('e', gettxt(":1528","File name part of %s is empty\n"),
					list[CF][i]);
                        continue;
                }
		s = getsuf(list[CF][i]);
		sfile = (strcmp(s, "s") == 0);
		if (sfile && !Eflag && !Pflag && !Nflag && !Fflag && !Sflag) {
			as_in = list[CF][i];
			(void) assemble(i);
			continue;
		}
		else if (sfile) {
			if (Sflag)
				error('w', gettxt(":25","Conflicting option -S with %s\n"),
					list[CF][i]);
			continue;
		}
		if ( strcmp(getsuf(list[CF][i]), "i") == 0 ) {
			if ( Eflag || Pflag ) {
				error('w', gettxt(":26","Conflicting option -%c with %s\n"),
					Eflag ? 'E' : 'P', list[CF][i]);
				continue;
			}
		}

		if (CCflag && !frontend(i))
			continue;

		if (!compile(i))
			continue;

		if (Oflag || ilflag)
			if (!optimize(i))
				continue;

#if ASFILT
		if (CCflag && cplusplus_mode == via_glue) {
			if (!asfilt(i))
				continue;
		}
#endif

		if (passprof)
			(void) profile(i);

		if (!Sflag)
			(void) assemble(i);

	} /* end loop */

#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if (CCflag && !eflag && !cflag && Tauto) {
		eflag = ptlink();
		if (eflag) 
			error('e', gettxt(":1529","Pre-linker failed\n"));
	}
#endif

	if (!eflag && !cflag && !Tprelink_objects)
		linkedit();

	if (CCflag && (nlist[CF] == 0) && cflag)
		/* No source files were compiled, so only linker input was specified,
		   but a linker-suppressing option was also specified ... the command
		   therefore had no action.  In C, presume that user knows what they
		   are doing, but in C++, this might well be due to an unrecognized 
		   source file suffix, so give a warning.  (Alternate scenarios which 
		   could generate the warning include doing a make with CFLAGS=-P to
		   preprocess everything, and the final link step does nothing, but
		   the warning will do no harm.)	*/
		error('w', gettxt(":1570", "Only linker input specified, but options suppress linker\n"));

	dexit();
	/*NOTREACHED*/
}


/*===================================================================*/
/*                                                                   */
/*                  C++ FRONT END				     */
/*                                                                   */
/*===================================================================*/

static int
frontend (i)
	int i;
{
	int j;
	int front_ret;
	
	if (fflag) {
		/* Process for Standard Components fs (free store) tool.
		 * This means first preprocess normally (as with the -E
		 * option), then preprocess with the fsipp preprocessor,
		 * then start actual compilation.   
		 */

		/* First run normal C++ preprocessor */
		nlist[AV]= 0;
		addopt(AV,passname(prefix, N_FE));
		addopt(AV,"-E");
		/* bring in all FE arguments, even though all may not be relevant */
		for (j = 0; j < nlist[Xfe]; j++)
			addopt(AV,list[Xfe][j]);
        	addopt(AV,"-I"); 		/* as below */
		addopt(AV,ccincdir);
		addopt(AV,list[CF][i]);
		list[AV][nlist[AV]] = 0;	/* terminate arg list */
		PARGS;
		/* redirect preprocessor output to tmp4 */
		front_ret = callsys( passfe, list[AV], STDOUT_TO_TMP4, NORMAL_STDOUT );
		if (front_ret != 0) {
			cflag++;		/* as below */
			eflag++;
			cunlink(tmp4);
			return 0;		/* failure */
		}

		/* Next run fs tool's fsipp preprocessor, with tmp4 as input, .i as output */
		nlist[AV]= 0;	
		addopt(AV,passname(prefix, N_FS));
		if (Qflag)
			addopt(AV,"-Qy");
		if (Vflag)
			addopt(AV,"-V");
		addopt(AV,"-o");
		addopt(AV, setsuf(list[CF][i], "i") );
		addopt(AV, tmp4);
		list[AV][nlist[AV]] = 0;	/* terminate arg list */
		PARGS;
		front_ret = callsys( passfs, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT );
		if (front_ret != 0) {
			cflag++;
			eflag++;
			cunlink(setsuf(list[CF][i], "i"));
			return 0;		/* failure */
		}
		cunlink(tmp4);

		/* Now ready for normal front end compilation, but using .i as input */

		if (Pflag)
			/* we have the preprocessed .i file, so if that's wanted, we can stop */
			return 0;		/* success, but don't need to run any more phases */
	}
		
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_FE));

	if(Pflag) {
		addopt(AV,"-o");
		addopt(AV, out_name ? out_name : setsuf(list[CF][i], "i") );
	} else if(Fflag && cplusplus_mode == via_glue) {
#if IL_SHOULD_BE_WRITTEN_TO_FILE
		/* must match corresponding define in EDG front end */
		addopt(AV,"-o");
		addopt(AV, setsuf(list[CF][i], "cll") );
#endif
	} else if (!Eflag && !Nflag && cplusplus_mode == via_glue) {
		addopt(AV, "-o");
		addopt(AV, tmp3);
	}
	if(!Eflag && !Pflag && !Nflag && !Fflag) {
		if(fe_out)
			free(fe_out);
		fe_out = (cplusplus_mode == via_glue) ? strcpy(stralloc(strlen(tmp3)), tmp3) : setsuf(list[CF][i], "int.c");
	}
	if(!Eflag && !Pflag && gflag && cplusplus_mode == via_glue)
	{
		addopt(AV,"--generate_symbol_info");
		if (Fflag || Nflag)
			addopt(AV, setsuf(list[CF][i], "dbg"));
		else
			addopt(AV, tmp9);
	}

	for (j = 0; j < nlist[Xfe]; j++)
		addopt(AV,list[Xfe][j]);

	/* Tell it about the standard place for C++ include files (now different from C) */
        addopt(AV,"-I");
	addopt(AV,ccincdir);

	if (fflag)
		/* need to compile the .i file (created above) */
		addopt(AV,setsuf(list[CF][i], "i"));
	else
		/* the normal case, compile the file specified on the command line */
		addopt(AV,list[CF][i]);
	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	front_ret = callsys( passfe, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT );
	if ((Eflag || Pflag || Nflag || Fflag) && front_ret == 0 )
		return 0;		/* success, but don't need to run any subsequent phases */
	else if (front_ret >= 1) {
		cflag++;
		eflag++;
		if (Eflag) {
			/* do nothing */
		} else if (Pflag) {
			cunlink(out_name ? out_name : setsuf(list[CF][i], "i"));
		} else if (Nflag) {
			cunlink(setsuf(list[CF][i], "cil"));
		} else if (Fflag && cplusplus_mode == via_glue) {
#if IL_SHOULD_BE_WRITTEN_TO_FILE
			cunlink(setsuf(list[CF][i], "cll"));
#endif
		} else {
			cunlink(fe_out);
		}
		if (!Eflag && !Pflag && gflag && !Fflag && !Nflag) {
			cunlink(tmp9);
		}
		return 0;		/* failure */
	}

#if AUTOMATIC_TEMPLATE_INSTANTIATION
	process_ii_file(i);
#endif

#ifdef PERF
	STATS("frontend ");
#endif

	return 1;			/* success */
}

#if AUTOMATIC_TEMPLATE_INSTANTIATION

/* some redundant code */
static void
ii_error(int iifd, const char* iifile)
{
	(void) close(iifd);
	cunlink(iifile);
	cunlink(fe_out);
	exit(1);
}

static void
ii_write(int iifd, const char* iifile, const void* buf, size_t nbyte)
{
	if (write(iifd, buf, nbyte) != nbyte) {
		error('e',gettxt(":1534","cannot write file %s\n"),iifile);
		ii_error(iifd, iifile);
	}
}

/* Look for .ii file indicating that templates were involved.
 * If there, rewrite the first part of it with the current
 * compilation information.  (See EDG internal doc and their 
 * "eccp" command script for more details.)
 */
static void
process_ii_file(i)
	int i;
{
	int j;
	int iifd;
	char *iifile;

	iifile = setsuf(list[CF][i], "ii");
	if ((iifd = open(iifile, O_RDONLY)) != -1) {
		/* .ii exists, generate new one using current command line */
		char *mptr, *tmptr, *umptr, *cwd;
		struct stat s;
		int line;

		if (fstat(iifd, &s) == -1) {
			error('e', gettxt(":1530","cannot access file %s\n"),iifile);
			ii_error(iifd, iifile);
		}
		if ((mptr = (char*)malloc((size_t)s.st_size)) == NULL) {
			error('e', gettxt(":35","Out of space\n"));
			ii_error(iifd, iifile);
		}
		if(read(iifd, mptr, (unsigned)s.st_size) != s.st_size) {
			error('e',gettxt(":1532","cannot read file %s\n"),iifile);
			ii_error(iifd, iifile);
		}
		(void) close(iifd);
		if ((iifd = open(iifile, O_WRONLY|O_TRUNC)) == -1) {
			error('e',gettxt(":1533","cannot open file for writing: %s\n"),iifile);
			ii_error(iifd, iifile);
		}

		/* We use the new (as of EDG 2.27) three-line .ii format.
		 * Write out the current command line, current working 
		 * directory, and current source file, replacing the first
		 * three lines (if they exist) in the existing .ii file.
		 * Then write out the balance of the existing .ii file.
		*/

		/* write out current command line */
		for (j = 0; j < nlist[XCC]; j++) 
			ii_write(iifd, iifile, list[XCC][j], strlen(list[XCC][j]));
		ii_write(iifd, iifile, "\n", 1);

		/* write out current working directory */
		if ((cwd = getcwd(NULL, PATH_MAX+1)) == NULL) {
			error('e',gettxt(":1596","cannot access current working directory\n"));
			ii_error(iifd, iifile);
		}
		ii_write(iifd, iifile, cwd, strlen(cwd));
		ii_write(iifd, iifile, "\n", 1);
		free(cwd);

		/* write out current source file */
		ii_write(iifd, iifile, list[CF][i], strlen(list[CF][i]));
		ii_write(iifd, iifile, "\n", 1);

		/* Skip past first three lines from existing .ii file.
		 * Note there may not be three lines, in fact the file
		 * may be empty (as is the case when the compiler first
		 * creates it).
		*/
		tmptr = mptr;	/* tmptr points to the rest of the file */
		j = s.st_size;	/* j is the size of the rest of the file */
		for (line = 1; line <= 3; line++) {
			umptr = memchr(tmptr, '\n', j);
			if (umptr == NULL) {
				/* no line found, finished */
				j = 0;	/* in case chars without nl found */
				break;
			}
			umptr++;	/* point past '\n' */
			j -= (umptr - tmptr);
			tmptr = umptr;
		}

		/* concatenate rest of old .ii file onto new one */
		ii_write(iifd, iifile, tmptr, j);

		(void) close(iifd);
		free(mptr);
	}
}

#endif /* AUTOMATIC_TEMPLATE_INSTANTIATION */

/*
/*===================================================================*/
/*                                                                   */
/*                  END FRONT END				     */
/*                                                                   */
/*===================================================================*/


/*
/*===================================================================*/
/*                                                                   */
/*                  DEMANGLER 					     */
/*                                                                   */
/*===================================================================*/

static void
demangle()
{
	/* demangle C++ names in the captured output of some phase */
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_DE));
	if (Vflag)
		addopt(AV,"-V");
	addopt(AV,"-w");
	list[AV][nlist[AV]] = 0;		/* terminate arg list */

	PARGS;

	(void) callsys(passde, list[AV], STDIN_FROM_TMP4, STDOUT_TO_STDERR);
}

/*
/*===================================================================*/
/*                                                                   */
/*                  END DEMANGLER 				     */
/*                                                                   */
/*===================================================================*/


/*
/*===================================================================*/
/*                                                                   */
/*                  C COMPILER 					     */
/*                                                                   */
/*===================================================================*/

static int
compile (i)
	int i;
{
	int j;
	int acomp_ret;
	
	/* This function is used to invoke the full C compiler (for C,
	   or for C++ the old way), or it is used to invoke just part
	   of the C compiler (just the back end, for C++ the new way).
	   In the latter case, the name of the tool must be changed.
	*/

	if (CCflag && cplusplus_mode == via_glue)
		passc0 = chg_toolname(passc0, /*from*/ N_C0, /*to*/ N_BE);

	nlist[AV]= 0;
	addopt(AV,passname(prefix, 
			   CCflag && cplusplus_mode == via_glue ? N_BE : N_C0));
	addopt(AV,"-i");
	if(CCflag)
		addopt(AV,fe_out);
	else
		addopt(AV,list[CF][i]);
	addopt(AV,"-o");
	if (Eflag || Pflag)
		/* note that "... -E -o filename ..." still goes to standard output */
		addopt(AV, Eflag ? "-" : (out_name ? out_name : setsuf(list[CF][i], "i")) );
	else {
		if (Sflag)
			/* if assembly file requested and this is the
			   last phase, put out .s (possibly renamed); 
			   otherwise, put out temp */
			if ((Oflag && independ_optim)
#if ASFILT
			    || (CCflag && cplusplus_mode == via_glue)
#endif
			    || qarg)
				/* not the last phase */
				as_in = tmp2;
			else
				/* is the last phase */
				as_in = out_name ? out_name : setsuf(list[CF][i], "s");
		else	/* not Sflag */
			as_in = tmp2;
		addopt(AV,c_out = as_in);
	}

	addopt(AV,"-f");
	addopt(AV,list[CF][i]);

	if (CCflag && cplusplus_mode == via_c
	    && ((strcmp(getsuf(list[CF][i]),"i") == 0) || fflag))
		addopt(AV,"-S");   /* needed to tell acomp to still do preprocessing */

	if(!Eflag && !Pflag)
	{
		if (CCflag && cplusplus_mode == via_glue)
		{
			if(gflag)
			{
				addopt(AV,"-g");
				addopt(AV, tmp9);
			}
		}
		else
		{
			if (dsflag)
				addopt(AV,"-ds");
			if (dlflag)
				addopt(AV,"-dl");
		}
	}
	for (j = 0; j < nlist[Xc0]; j++)
		addopt(AV,list[Xc0][j]);

	AVmore(); /* Appended more options to acomp command */

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;


	if (CCflag) {
		struct stat s;

		/* Redirect stderr to tmp4, then run the demangler
		 * on any generated output (e.g., needed for inline-related 
                 * warnings).  
		 */

		acomp_ret = callsys( passc0, list[AV], STDERR_TO_TMP4, NORMAL_STDOUT );

		/* see if tmp4 is non-empty */
		if ((stat(tmp4,&s) == 0) && (s.st_size > 0) && (access(tmp4,R_OK) == 0)) 
			/* there are diagnostics, so demangle them to stderr */
			demangle();

		/* remove the temporary file */
		cunlink(tmp4);

	} else {
		acomp_ret = callsys( passc0, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT );
	}

	if (CCflag) {	/* clean up intermediate file */
		cunlink(fe_out);
		if(!Eflag && !Pflag && gflag && cplusplus_mode == via_glue) {
			cunlink(tmp9);
		}
	}

	if ((Eflag || Pflag) && acomp_ret == 0 )
		return(0);
	else
	if (acomp_ret >= 1) {
		cflag++;
		eflag++;
		if (Pflag){
			cunlink(out_name ? out_name : setsuf(list[CF][i], "i"));
			}
		else {
			cunlink(c_out);
			}
		return(0);
	}

#ifdef PERF
	STATS("compiler ");
#endif
	return(1);
}


#if ASFILT

/*===================================================================*/
/*                                                                   */
/*	Hopefully temporary .s pass to assist C++ EH implementation  */
/*                                                                   */
/*===================================================================*/

static int
asfilt(int i)
{
	int j;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_ASFILT));

#if 0
	if (Qflag)  /* By default, version stamping option is passed */
		addopt(AV,"-Qy"); 
	for (j=0; j < nlist[Xbb]; j++)
		addopt(AV,list[Xbb][j]);
#endif
	addopt(AV,c_out);
	/* if assembly file requested and this is the last phase,
	   put out .s (possibly renamed); otherwise, put out temp	*/
	if (Sflag && !qarg)
		as_in = out_name ? out_name : setsuf(list[CF][i], "s");
	else
		as_in = tmp8;
	addopt(AV,as_in);
#if 0
	addopt(AV,as_in = (Sflag && !qarg) ? setsuf(list[CF][i], "s") : tmp8);
#endif
	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	if (callsys(passasfilt, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT)) {
		if (Sflag) {
			if (move(c_out, as_in)) { /* move failed */
				cunlink(c_out);
				return 0;
			}
		}
		else {
			cunlink(as_in);
			as_in = c_out;
		}
		error('e', gettxt(":9999","Asfilt failed\n"));
	}

#ifdef PERF
	STATS("asfilt");
#endif

	return 1;
}
#endif

/*===================================================================*/
/*                                                                   */
/*                      PROFILER                                     */
/*                                                                   */
/*===================================================================*/

static int
profile(i)
	int i;
{
	int j;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_PROF));
	if (qarg == 'l')
		addopt(AV,"-l");
	else
		addopt(AV,"-x");

	if (Qflag)  /* By default, version stamping option is passed */
		addopt(AV,"-Qy"); 
	for (j=0; j < nlist[Xbb]; j++)
		addopt(AV,list[Xbb][j]);
#if ASFILT
	addopt(AV, (CCflag && cplusplus_mode == via_glue) ? as_in : c_out);
#else
	addopt(AV,c_out);
#endif
	/* if assembly requested, put out .s (possibly renamed); 
	   otherwise, put out temp */
	if (Sflag)
		as_in = out_name ? out_name : setsuf(list[CF][i], "s");
	else
		as_in = tmp7;
	addopt(AV,as_in);
#if 0
	addopt(AV,as_in = Sflag ? setsuf(list[CF][i], "s") : tmp7);
#endif
	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	if (callsys(passprof, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT)) {
		if (Sflag) {
			if (move(c_out, as_in)) { /* move failed */
				cunlink(c_out);
				return(0);
			}
		}
		else {
			cunlink(as_in);
			as_in = c_out;
		}
		error('w', gettxt(":27","Profiler failed, '-q %c' ignored for %s\n"),
			qarg, list[CF][i]);
	}
		

#ifdef PERF
	STATS("profiler");
#endif

	return(1);
}
	
/*===================================================================*/
/*                                                                   */
/*                    ASSEMBLER                                      */
/*                                                                   */
/*===================================================================*/

static int
assemble (i)
	int i;
{
	int j;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_AS));

	addopt(AV,"-o");
	if (cflag && out_name != NULL)
		as_out = out_name;
	else
		as_out = setsuf(list[CF][i], "o");
	addopt(AV,as_out);
	for (j = 0; j < nlist[Xas]; j++)
		addopt(AV,list[Xas][j]);
	addopt(AV,as_in);

	ADDassemble();

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;

	if (callsys(passas, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT)) {
		cflag++;
		eflag++;
		return(0);
	}

#ifdef PERF
	STATS("assembler");
#endif

	return(1);
}


#if AUTOMATIC_TEMPLATE_INSTANTIATION

/*===================================================================*/
/*                                                                   */
/*                  TEMPLATE INSTANTIATOR			     */
/*                                                                   */
/*===================================================================*/

/* The following HUGE chunk of code (up to "END TEMPLATE INSTANTIATOR")
 * is all in support of the prelinker for C++. The reason for all this 
 * code is that the ld command does its own path expansion for libraries.
 * That means that the code from ld had to be ripped out and used in here 
 * so that the prelinker would also know how to find all the libraries given
 * on the command line (and some implied ones as well). Adding this to the 
 * prelinker seemed equally as grody.
 * Be careful of the ld argument processing below. It could easily get out
 * of sync with what ld is really accepting.
 */

struct listnode {			/* A node on a linked list */
	char 	 *data;			/* The data item (lib directory) */
	struct listnode *next;		/* The next element */
};

struct list {				/* A linked list */
	struct listnode *head;		/* The first element */
	struct listnode *tail;		/* The last element */
};

static struct list libdir_list;		/* the path list */
static struct listnode* dirlist_insert;	/* insert place for -L libraries */
					/* see the man page for ld to see how
					 * this all works (LD_LIBRARY_PATH)
					 */

static char *process_ld_path();
static struct listnode* list_append();
static struct listnode* list_prepend();
static struct listnode* list_insert();
static char* find_library();
static void add_libdir();
static void lib_setup();

/* this is the main function for the prelinker. It calls all the other
 * code that we stole from ld.
 */
static int
ptlink ()
{
	int j;
	int dmode = 1;	/* for -dy */
	int Bmode = 0;	/* for -Bdynamic */
	char *t;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_PT));

	/* Must tell prelinker where to find CCS nm and what options to use */
	/* (maybe this belongs in machdep.c, regarding the options?)        */
	t = stralloc(strlen(makename(BINDIR,prefix,"nm")) + 7 + 1);
	(void) sprintf(t, "-c%s -pxr", makename(BINDIR,prefix,"nm"));
	/* addopt will single quote around the whole string, so that the prelinker sees it together */
	addopt(AV,t);

	for (j = 0; j < nlist[Xpt]; j++)	/* ptlink options */
		addopt(AV,list[Xpt][j]);

	/* This is similar to -- but not exactly the same, since C doesn't 
         * need ptlink() -- the linkedit() processing below.
	 */
	if ( Gflag == 0)
		addopt(AV,makename(crtdir,prefix,crt));

	addopt(AV,makename(ccrtdir,prefix,CCRTI));

	if (Gflag == 0) {
		t = stralloc( strlen(values) + strlen(Xarg) + 2);
		(void) sprintf(t,"%s%s.o",values,Xarg);
		addopt(AV,makename(crtdir,prefix,t));
		if (CCflag) {
			t = stralloc( strlen(values) + strlen(Xargfe) + 2);
			(void) sprintf(t,"%s%s.o",values,
				(strcmp(Xargfe,"-Xo") == 0) ? "-Xo" : "-Xd");
			addopt(AV,makename(ccrtdir,prefix,t));
		}
	}

	/* All the files that are passed to ld also need to be passed
	 * to ptlink. Since ld does all of the library searching, we need
	 * to add that capability here.
	 */

	lib_setup();

	/* look through ld arguments for -d[y/n] settings */
	for (j = 0; j < nlist[Xld]; j++) {
		char *tp = list[Xld][j];
		if (*tp++ == '-' && *tp++ == 'd') {
			if (*tp == NULL) 
				tp = list[Xld][++j];
			if (*tp == 'n')
				dmode = 0;
			else if (*tp == 'y')
				dmode = 1;
		}
	}
	if(dmode)
		Bmode = 1;	/* on by default for -dy */

	/* now look through ld arguments again, this time we want to do
	 * things like set library search paths, find libraries, toggle
	 * the static/dynamic lib searching, and accumulate all filenames.
	 */
	for (j = 0; j < nlist[Xld]; j++) {
		char *tp = list[Xld][j];
		if (*tp++ == '-') {
			/* this is an ld argument. In the processing below,
			 * we need to see if there is a subargument, and if
			 * so, is it split across two entries in the Xld list.
			 */
			switch (*tp++) {
			case 'B':
				if (*tp == NULL) /* it is split */
					tp = list[Xld][++j];	
				if (strcmp(tp, "dynamic") == 0){
	                               	Bmode = 1;
				}else if (strcmp(tp,"static") == 0)
	                                Bmode = 0;
				break;
				
			case 'd':
				if (*tp == NULL) /* it is split */
					tp = list[Xld][++j];
				if (*tp == 'n')
					dmode = 0;
				else if (*tp == 'y')
					dmode = 1;
				break;
			case 'l':
				if (*tp == NULL) /* it is split */
					tp = list[Xld][++j];
				addopt(AV, find_library(tp, dmode, Bmode));
				break;
			case 'L':
				if (*tp == NULL) /* it is split */
					tp = list[Xld][++j];
				add_libdir(tp);
				break;
			case 'M':	/* we want to ignore all of these  */
			case 'I':	/* (as well as any others). But    */
			case 'o':	/* these take subarguments so make */
			case 'u':	/* sure we ignore the whole thing  */
			case 'e':	/* and not just the '-x' part.     */
			case 'h':
			case 'z':
			case 'Y':	/* use libpath instead of -YP */
			case 'Q':
			case 'T':
				/* all of these ld options take arguments  */
				/* so skip the next list[Xld] entry if the */
				/* argument is there.			   */
				if (*tp == NULL)  /* it is split */
					++j;
			default:
				/* fall into here and we ignore the argument */
				continue;
			}
		} else {
			/* a filename */
			addopt(AV,list[Xld][j]);
		}
	}

	if (Gflag == 0)
	{
		addopt(AV, find_library("C", dmode, Bmode));
		if (Kthread)
			addopt(AV, find_library("thread", dmode, Bmode));
		addopt(AV, find_library("c", dmode, Bmode));
	}

	addopt(AV,makename(ccrtdir,prefix,CCRTN));

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	if (nlist[AV] > MINLDARGS) /* if file given or flag set by user */
	{
		struct stat s;

		PARGS;

		/* redirect stdout (where the prelinker puts the "assignment"-type
		 * messages) to tmp4, then run the demangler on it if it is non-empty
		 */
		eflag |= callsys(passpt, list[AV], STDOUT_TO_TMP4, NORMAL_STDOUT);

		if ((stat(tmp4,&s) == 0) && (s.st_size > 0) && (access(tmp4,R_OK) == 0)) {
			/* stdout contents are non-empty, so demangle them to stderr */
			demangle();
		}
		/* remove the temporary file */
		cunlink(tmp4);
	}

#ifdef PERF
	STATS("ptlink ");
#endif

	return eflag;
}

/*
 * Allocate a struct listnode.  (This not taken from ld.)
 */
static
struct listnode*
list_alloc(void)
{
	register struct listnode *s;

	if ((s = (struct listnode*)calloc(1, sizeof(struct listnode))) == NULL) {
		error('e', gettxt(":35","Out of space\n"));
		dexit();
	}
	return(s);
}

/*
 * Add an item to the indicated list and return a pointer to the
 * list node created
 */
struct listnode*
list_append(lst, item)
	struct list	*lst;		/* The list */
	char	*item;		/* The item */
{
	if (lst->head == NULL)
		lst->head = lst->tail = list_alloc();
	else {
		lst->tail->next = list_alloc();
		lst->tail = lst->tail->next;
	}
	lst->tail->data = item;
	lst->tail->next = NULL;

	return lst->tail;
}

/*
 * Add an item to the indicated list after the given node
 * and return a pointer to the list node created
 */
struct listnode*
list_insert(after, item)
	struct listnode	*after;		/* The node to insert after */
	char		*item;	/* The item */
{
	struct listnode	*ln;		/* Temp list node ptr */

	ln = list_alloc();
	ln->data = item;
	ln->next = after->next;
	after->next = ln;

	return ln;
}

/*
 * Prepend an item to the indicated list and return a pointer to the
 * list node created
 */
struct listnode*
list_prepend(lst, item)
	struct list	*lst;		/* The list */
	char	*item;	/* The item */
{
	struct listnode	*ln;		/* Temp list node ptr */

	if (lst->head == NULL)
		lst->head = lst->tail = list_alloc();
	else {
		ln = list_alloc();
		ln->next = lst->head;
		lst->head = ln;
	}
	lst->head->data = item;

	return lst->head;
}

/* initialize the library search path linked list. This obeys the rules 
 * as outlined in the man page for ld. Namely:
 *	if LD_LIBRARY_PATH=ldir1:ldir2;ldir3 then
 *	search path = ldir1 -> ldir2 -> -Ldir1 -> -Ldir2... -> ldir3 -> -YPlist
 */

void
lib_setup()
{
	register char *lptr;
	char	*cp;
	
	if ((cp = getenv("LD_LIBRARY_PATH")) != NULL) {
		lptr = stralloc(strlen(cp));
		(void) strcpy(lptr, cp);

		lptr = process_ld_path(lptr);
		if(lptr != NULL) {

			if(*lptr == ';')
				dirlist_insert = libdir_list.tail;
			*lptr = '\0';

			lptr = process_ld_path(++lptr);
			if (lptr != NULL)
				error('w', gettxt(":1070", "LD_LIBRARY_PATH has bad format\n"));
		}
	}
	lptr = process_ld_path(libpath);
	if (lptr != NULL)
		error('e', gettxt(":1071","-YP default library path malformed\n"));
}

/* go through a search path and add certain things to the list */
static char * 
process_ld_path(pathlib)
	char	*pathlib;
{
	char	*cp;
	int	seenflg = 0;

	for(;;) {
		cp = strpbrk(pathlib,";:");
		if(cp == NULL) {
			if(*pathlib == '\0') {
				if(seenflg)
				    (void) list_append(&libdir_list, ".");
			} else
				(void) list_append(&libdir_list, pathlib);
			return(cp);
		}

		if(*cp == ':') {
			if(cp == pathlib)
				(void) list_append(&libdir_list, ".");
			else
				(void) list_append(&libdir_list, pathlib);
			*cp = '\0';
			pathlib = cp + 1;
			++seenflg;
			continue;
		}
		
		/* case ; */

		if(cp != pathlib)
			(void) list_append(&libdir_list, pathlib);
		else {
			if(seenflg)
				(void) list_append(&libdir_list, ".");
		}
		return(cp);
	}
}


/*
 * add_libdir(CONST char* pathlib)
 * adds the indicated path to those to be searched for libraries.
 */
void
add_libdir(pathlib)
	char	*pathlib;	/* the library path to add */
{
	if (dirlist_insert == NULL) {
		(void) list_prepend(&libdir_list, pathlib);
		dirlist_insert = libdir_list.head;
	} else
		dirlist_insert = list_insert(dirlist_insert, pathlib);
}


/*
 * find_library(): takes the abbreviated name of a library file and
 * searches for the library on each of the paths specified in libdir_list;
 * if dmode and Bmode are TRUE then first looks for the .so file, then/else
 * look for the .a file. Returns the full path.
 */
char*
find_library(name, dmode, Bmode)
	char	*name;		/* name to search for */
	int	dmode,		/* 1 for -dy, otherwise 0 */
		Bmode;		/* 1 for -Bdynamic, 0 otherwise */
{
	register struct listnode *tp;
	int	plen;

	/* search path list for file */
	for ( tp = (&libdir_list)->head ; tp != NULL; tp = tp->next ) {
		char *pptr;

		/* shared libs need space for "/lib.so", otherwise "/lib.a" */
		plen = (dmode && Bmode) ? 7 : 6;
	
		pptr = stralloc(strlen(tp->data) + strlen(name) + plen);
		(void) strcpy(pptr, tp->data);
		(void) strcat(pptr, "/lib");
		(void) strcat(pptr, name);
		plen = strlen(pptr);	/* so we can change suffix later */
		if (dmode && Bmode) {
			(void) strcat(pptr, ".so");
			if (access(pptr, R_OK) == 0)
				return pptr;
		}
		(void) strcpy((pptr+plen), ".a");
		if (access(pptr, R_OK) == 0)
			return pptr;
		free(pptr);
	}
	error('e',gettxt(":1069", "library not found: -l%s\n"), name);
	exit(1);
}

/*===================================================================*/
/*                                                                   */
/*                  END TEMPLATE INSTANTIATOR			     */
/*                                                                   */
/*===================================================================*/

#endif  /* AUTOMATIC_TEMPLATE_INSTANTIATION */

/*===================================================================*/
/*                                                                   */
/*                LINKAGE EDITOR                                     */
/*                                                                   */
/*===================================================================*/

static void
linkedit ()
{
	int j;
	char *t;
	
	nlist[AV]= 0;
	addopt(AV,passname(prefix, N_LD));

	if ( Gflag == 0)
		addopt(AV,makename(crtdir,prefix,crt));
	else
	{
		/* All symbols to be hidden must be collected together
		   in one list, since if there is more than one -Bqhide
		   option, ld ignores all but the last */
		const char* long_long_symbols = ",__llasgmul,__llasgremu,__llasgdivu,__llasgrems,__llasgdivs";
		const char* cplusplus_symbols = ",__record_needed_destruction,atexit__3stdFPFv_v";
		char	*hide_list;
		size_t	plen = strlen("-Bqhide=")+1;
		plen += strlen(ABBREV_VERSION_ID);
		plen += strlen(long_long_symbols);
		if (CCflag)
			plen += strlen(cplusplus_symbols);
		hide_list = stralloc(plen);
		(void) strcpy( hide_list, "-Bqhide=" );
		(void) strcat( hide_list, ABBREV_VERSION_ID );
		(void) strcat( hide_list, long_long_symbols );
		if (CCflag)
			strcat( hide_list, cplusplus_symbols );
		addopt(AV,hide_list);
	}

	if (CCflag)
	{
		/* tell ld to comlain about mixing old (pre-EH) and new .o's */
		addopt(AV,"-fcplus_version");
		addopt(AV,makename(ccrtdir,prefix,CCRTI));
	}
	else
		addopt(AV,makename(crtdir,prefix,CRTI));

	if (Gflag == 0) {
		t = stralloc( strlen(values) + strlen(Xarg) + 2);
		(void) sprintf(t,"%s%s.o",values,Xarg);
		addopt(AV,makename(crtdir,prefix,t));
		if (CCflag) {
			t = stralloc( strlen(values) + strlen(Xargfe) + 2);
			(void) sprintf(t,"%s%s.o",values,
				(strcmp(Xargfe,"-Xo") == 0) ? "-Xo" : "-Xd");
			addopt(AV,makename(ccrtdir,prefix,t));
		}
	}

	if (out_name != NULL)
        {
                addopt(AV,"-o");
                addopt(AV,out_name);
        }

	LDmore();	/* get any machine specific options */

	for (j = 0; j < nlist[Xld]; j++) /* app files, opts, and libs */
		addopt(AV,list[Xld][j]);

	if (CCflag && (out_name == NULL || strncmp(out_name,"libC.so", 7) != 0))
		/* when creating a.out, link with libC
		   when creating a .so (that isn't libC.so*), link with libC,
			because need to create a NEEDED binding to libC.so
			in case an a.out is linked with cc not CC	*/
		addopt(AV,"-lC");

	if (Gflag == 0)
		if (Kthread)
			addopt(AV,"-lthread");

	if (Gflag == 0)
		addopt(AV,"-lc");
	addopt(AV,"-lcrt");

	if (CCflag)
		addopt(AV,makename(ccrtdir,prefix,CCRTN));
	else
		addopt(AV,makename(crtdir,prefix,CRTN));

	if (pflag && Gflag == 0 && link_mode == dynamic_link) {
		/* tell ld to use a different interpreter */
		addopt(AV,"-I");
		addopt(AV,makename(crtdir,prefix,"libp/libc.so.1"));
	}

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	if (nlist[AV] > MINLDARGS) /* if file given or flag set by user */
	{
		PARGS;
		if (CCflag) {
			struct stat s;

			/* redirect stderr to tmp4, then run the demangler
			 * on it if it is non-empty
			 */
			eflag |= callsys(passld, list[AV], STDERR_TO_TMP4, NORMAL_STDOUT);

			if ((stat(tmp4,&s) == 0) && (s.st_size > 0) && (access(tmp4,R_OK) == 0)) {
				/* stderr contents are non-empty, so demangle them to stderr */
				demangle();
				/* unlink of tmp4 will be done by dexit() */
			}
		} else {
			eflag |= callsys(passld, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT);
		}
	}

	if ((nlist[CF] == 1) && (nxo == 1) && (eflag == 0) && (debug <= 2) )
		/* delete .o file if single file compiled and loaded */
		cunlink(as_out);

#ifdef PERF
	STATS("link edit");
#endif

}


/* CCYelse - handle additional options to chg_pathnames for C++ */
static int
CCYelse(c, np)
	int c;
	char *np;
{
	switch(c) {
	case 'f':
		passfe = makename( np, prefix, N_FE );
		return 1;
	case 's':
		passfs = makename( np, prefix, N_FS );
		return 1;
	case 'd':
		passde = makename( np, prefix, N_DE );
		return 1;
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	case 't':
		passpt = makename( np, prefix, N_PT );
		return 1;
#endif
#if ASFILT
	case 'z':
		passasfilt = makename(np, prefix, N_ASFILT);
		return 1;
#endif
	}
	return (0);
}

/*
   chg_toolname() creates a new pathname with the changed name of a tool
*/ 

static char*
chg_toolname(char* old_pathname, const char* old_tool, const char* new_tool) 
{
	char*	new_pathname;

	/* old_tool should be the last part of old_pathname; if not, do nothing */
	if (strcmp(old_pathname+strlen(old_pathname)-strlen(old_tool),
		   old_tool) != 0)
		return old_pathname;
	/* allocate new string for pathname */
	new_pathname = stralloc(strlen(old_pathname)-strlen(old_tool)+strlen(new_tool)+1);
	strncpy(new_pathname, old_pathname, strlen(old_pathname)-strlen(old_tool));
	strcat(new_pathname, new_tool);
	return new_pathname;
}

/*
   chg_pathnames() overrides the default pathnames as specified by the -Y option
*/

static void
chg_pathnames(chpiece, npath)
char *chpiece;
char *npath;
{
	char	*t;
	char	*topt;

	for (t = chpiece; *t; t++)
		switch (*t) {
		case 'p':
			if(CCflag)
				passfe = makename( npath, prefix, N_FE );
			else
				passc0 = makename( npath, prefix, N_C0 );
			break;
		case '0':
			passc0 = makename( npath, prefix, N_C0 );
			break;
		case '2':
			if (independ_optim)
				passc2 = makename( npath, prefix, N_OPTIM );
			break;
		case 'b':
			profdir= stralloc(strlen(npath));
			(void) strcpy(profdir,npath);
			break;
		case 'a':
			passas = makename( npath, prefix, N_AS );
			break;
		case 'l':
			passld = makename( npath, prefix, N_LD );
			break;
		case 'S':
			crtdir = stralloc(strlen(npath));
			(void) strcpy(crtdir,npath);
			ccrtdir = stralloc(strlen(npath));
			(void) strcpy(ccrtdir,npath);
			break;
		case 'I':
			if (CCflag) {
				/* For CC this option has two forms:
				   -YI,str changes INCDIR to str and INCDIR/CC to str/CC 
				   -YI,str1:str2 changes INCDIR/CC to str1 if str1 is non-empty
						 and/or INCDIR to str2 if str2 is non-empty */
				char* incdir = NULL;
				char* incdirCC = NULL;
				int colon_count = 0;
				int i;
				for (i = 0; i < strlen(npath); i++)
					if (npath[i] == ':')
						colon_count++;
				switch (colon_count) {
				case 0:
					/* -YI first form */
					incdir = npath;
					incdirCC = stralloc(strlen(npath)+3);
					sprintf(incdirCC,"%s/CC", npath);
					break;
				case 1:
					/* -YI second form */
					if (npath[0] == ':')
						incdir = npath+1;
					else if (npath[strlen(npath)-1] == ':') {
						incdirCC = npath;
						incdirCC[strlen(npath)-1] = '\0';
					} else {
						incdirCC = strtok(npath,":");
						incdir = strtok(NULL,":");
					}
					break;
				default:
					error('e', gettxt(":1618","Only one colon allowed in -YI\n"));
					exit(1);
				}
				/* now change INCDIR */
				if (incdir) {
					topt = stralloc(strlen(incdir)+12);
					sprintf(topt,"USR_INCLUDE=%s",incdir);
					putenv(topt);
				}
				/* now change INCDIR/CC */
				if (incdirCC) 
					ccincdir = incdirCC;
			} else {
				topt = stralloc(strlen(npath)+4);
				(void) sprintf(topt,"-Y%s",npath);
				addopt(Xcp,topt);
			}
			break;
		case 'P':
			if(fplibdir) {
				error('e', gettxt(":32","-YP may not be used with -YF\n"));
				exit(1);
			}
			libpath = stralloc(strlen(npath));
			(void) strcpy(libpath,npath);
			break;

			
		default: /* C++ and machine-specific '-Yx' options or error */
			if ((CCflag && CCYelse((int)*t,npath)) || Yelse((int)*t,npath))
				break;
			error('e', gettxt(":33","Bad option -Y %s,%s\n"),chpiece, npath);
			exit(1);
		} /* end switch */
}

static void
mk_defpaths()
{
	register char 	*nlibpath;
	int x,y;

	/* make defaults */
	if (!crtdir)
		crtdir = LIBDIR;
	if (!ccrtdir)
		ccrtdir = CCLIBDIR;
	if(!libpath)
		libpath = LIBPATH;
	if(libdir || llibdir) {
		/* fix NULL pointer problem */
		if(libdir)
			x = strlen(libdir);
		else
			x = 0;
		if(llibdir)
			y = strlen(llibdir);
		else
			y = 0;
		nlibpath = stralloc(strlen(libpath) + x + y);
		process_lib_path(libpath,nlibpath);
		libpath = nlibpath;
	}

	if (!passfs)
		passfs = makename( CCLIBDIR, prefix, N_FS );
	if (!passfe)
		passfe = makename( CCLIBDIR, prefix, N_FE );
	if (!passde)
		passde = makename( CCBINDIR, prefix, N_DE );
	if (!passc0)
		passc0 = makename( CCLIBDIR, prefix, N_C0 );
	if (!passc2)
		passc2 = makename( CCLIBDIR, prefix, N_OPTIM );
#if ASFILT
	if (!passasfilt)
		passasfilt = makename(CCLIBDIR, prefix, N_ASFILT);
#endif
	if (!passas)
		passas = makename( BINDIR, prefix, N_AS );
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	if (!passpt)
		passpt = makename( CCLIBDIR, prefix, N_PT );
#endif
	if (!passld)
		passld = makename( BINDIR, prefix, N_LD );

	/*
	 * mach_defpath is empty routine by default.
	 * If there is any machine dependent default path should
	 * be made, then could be handled by mach_defpath().
	 */
	mach_defpath();

	if (pflag) {
		int i;
		char * cp;
		char * cp2;

		nlibpath = libpath;
		/* count number of paths */
		for(i=0; ; i++) {
			nlibpath = strchr(nlibpath,':');
			if(nlibpath == 0) {
				i++;
				break;
			}
			nlibpath++;
		}

		/* get enough space for path/libp for every path in libpath +
			enough for the :s */
		nlibpath = stralloc( (int) (2 * strlen(libpath) - 1 + i*sizeof("./libp:")) );

		cp2 = libpath;
		while(cp =  strchr(cp2,':')) {
			if(cp == cp2)
				(void) strcat(nlibpath,"./libp:");
			else {
				(void) strncat(nlibpath,cp2,cp - cp2);
				(void) strcat(nlibpath,"/libp:");
			}
			cp2 = cp + 1;
		}

		if(*cp2 == '\0')
			(void) strcat(nlibpath,"./libp:");
		else {
			(void) strcat(nlibpath,cp2);
			(void) strcat(nlibpath,"/libp:");
		}

		(void) strcat(nlibpath,libpath);
		libpath = nlibpath;

	}
	addopt(Xld,"-Y");
	nlibpath = stralloc(strlen(libpath) + 2);
	(void) sprintf(nlibpath,"P,%s",libpath);
	addopt(Xld,nlibpath);
}


/* return the prefix of "cc"/"CC" */

static char *
getpref( cp )
	char *cp;	/* how cc/CC was called */
{
	static char	*tprefix;
	int		cmdlen,
			preflen;
	char		*prefptr,
			*ccname;

	ccname = CCflag? "CC" : "cc";
	if ((prefptr= strrchr(cp,'/')) == NULL)
		prefptr=cp;
	else
		prefptr++;
	cmdlen= strlen(prefptr);
	if ((strlen(prefptr) >= 3) && prefptr[cmdlen-3] == 'c' 
		&& prefptr[cmdlen-2] == '8' && prefptr[cmdlen-1] == '9') {
		ccname = "c89";
		c89 = 1;
		if (getenv("POSIX2") != NULL) 
			strict = 1;
	
	}
	preflen= cmdlen - strlen(ccname);
	if ( (preflen < 0 )		/* if invoked with a name shorter
					   than ccname */
	    || (strcmp(prefptr + preflen, ccname) != 0)) {
		error('e', gettxt(":34","command name must end in \"%s\"\n"), ccname);
		exit(1);
		/*NOTREACHED*/
	} else {
		tprefix = stralloc(preflen+1);
		(void) strncpy(tprefix,prefptr,preflen);
		return(tprefix);
	}
}

/* initialize all common and machine dependent variables */

static void
initialize()
{
#ifdef PERF
	pstat = 0;
#endif
	tmp2 = tmp3 = tmp4 = tmp5 = tmp6 = tmp7 = tmp9 = NULL;
#if ASFILT
	tmp8 = NULL;
	passasfilt = NULL;
#endif
	passfe = passde = passc0 = passc2 = passprof = passas = passld = NULL;
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	passpt = NULL;
#endif
	passfs = NULL;
	fe_out = c_out = as_in = as_out = NULL;
	crt = CRT1;
	crtdir = NULL;
	ccrtdir = NULL;
	fplibdir = NULL;
	libpath = NULL;
	ilfile = NULL;
	Parg = NULL;
	Tflag = Vflag = Oflag = Sflag = 0;
	cflag = eflag = kflag = pflag = gflag = qpflag = ilflag = 0;
	actual_cflag = 0;
	dsflag = dlflag = 1;
	qarg = 0;
	debug = 0;
	sfile = 0;
	Xcp = CCflag ? Xfe : Xc0;	/* set cpp argument list */

	/* setup optstr:
	 * optstr = combine OPTSTR, CCOPTSTR, and MACHOPTSTR together
	 */
	optstr = stralloc( strlen(OPTSTR) + strlen(MACHOPTSTR) +
			(CCflag ? strlen(CCOPTSTR) : 0) );
	(void) strcpy(optstr, OPTSTR);
	(void) strcat(optstr, MACHOPTSTR);
	if(CCflag)
		(void) strcat(optstr, CCOPTSTR);

	/*
	 * The following variables are initialized to default value which
	 * are used on most machines, however, they might be overwritten
	 * in machine-specific initvars() routines for some special usage.
	 */

	/* defined inline file are not available */
	inlineflag = 0;

	/* indicates optimizer is available */
	independ_optim = 1;

	/* defines the number Oflag would be accumulated */
	Ocount = 1;

	/* Add "-Qy"/"-Qn" to Xc2 (optimizer) ? */
	Add_Xc2_Qy = 1;

	/*
	 * -KPIC and -Kpic are always passed to acomp as "-2k".
	 *
	 * Here "-2k" is copied to Xc0tmp (temporary Xc0, only 4 bytes)
	 * and ready to be appended to Xc0 if Parg is not NULL.
	 *
	 * However, some machines may use different string other "-2k"
	 * (e.g. sparc uses "-2K"), then Xc0tmp could be overwritten
	 * to appended correct string to acomp.
	 */
	(void) strcpy(Xc0tmp, "-2k");

	/* machine-specific variables initialization routine */
	initvars();
}

/* Add the string pointed to by opt to the list given by list[lidx]. */

void
addopt(lidx, opt)
int	lidx;	/* index of list */
char	*opt;  /* new argument to be added to the list */
{
	/* check to see if the list is full */
	if ( nlist[lidx] == limit[lidx] - 1 ) {
		limit[lidx] += argcount;
		if ((list[lidx]= (char **)realloc((char *)list[lidx],
					limit[lidx]*sizeof(char *))) == NULL) {
			error('e', gettxt(":35","Out of space\n"));
			dexit();
		}
	}

	list[lidx][nlist[lidx]++]= opt;
}

/* make absolute path names of called programs */

static char *
makename( path, prefix_cmd, name )
	char *path;
	char *prefix_cmd;
	char *name;
{
	char	*p;

	p = stralloc(strlen(path)+strlen(prefix_cmd)+strlen(name)+1);
	(void) strcpy( p, path );
	(void) strcat( p, "/" );
	(void) strcat( p, prefix_cmd );
	(void) strcat( p, name );

	return( p );
}

/* make the name of the pass */

char *
passname(prefix_cmd, name)
	char *prefix_cmd;
	char *name;
{
	char	*p;

	p = stralloc(strlen( prefix_cmd ) + strlen( name ));
	(void) strcpy( p, prefix_cmd );
	(void) strcat( p, name );
	return( p );
}

/*ARGSUSED0*/
static void
idexit(i)
	int i;
{
        (void) signal(SIGINT, idexit);
        (void) signal(SIGTERM, idexit);
        eflag = i;
        dexit();
}


static void
dexit()
{
	if (!Pflag) {
#if ASFILT
		if (CCflag)
			cunlink(tmp8);
#endif
		if (qarg)
			cunlink(tmp7);
		if (ilflag)
			cunlink(tmp6);
		if (Oflag && independ_optim)
			cunlink(tmp5);
		cunlink(tmp4);
		cunlink(tmp3);
		cunlink(tmp2);
		if (CCflag && !Fflag)
			cunlink(fe_out);
		if ( (!cflag) && (!Tprelink_objects) && (nlist[CF] == 1) && (nxo == 1) && (debug <= 2) )
               		 /* cleanup any .o file if single file compiled and loaded */
			cunlink(as_out);
		if (dsflag && CCflag) {
			cunlink(tmp9);
		}
	}
#ifdef PERF
	if (eflag == 0)
		pwrap();
#endif
	if (strict == 2)
		eflag = strict;
	exit(eflag);
}


/* VARARGS */
void
error(c, fmt, va_alist)
	char c;
	char *fmt;
	va_dcl
{
	va_list ap;
	char	msgbuf[512];

	va_start(ap);

	(void) vsprintf(msgbuf, fmt, ap);
	va_end(ap);

	switch (c) {
	case 'e':	/* Error Messages */
		cflag++;
		eflag++;
		(void) pfmt(stderr,MM_ERROR,":108:%s",msgbuf);
		break;
	case 'w':	/* Warning Messages */
		(void) pfmt(stderr,MM_WARNING,":108:%s",msgbuf);
		break;
	case 'a':	/* Usage Messages */
		(void) pfmt(stderr,MM_ACTION,":108:%s",msgbuf);
		break;
	case 'i':	/* Information Messages */
		(void) pfmt(stderr,MM_INFO,":108:%s",msgbuf);
		break;
	case 'c':	/* Common Messages */
	default:
		(void) pfmt(stderr,MM_NOSTD,":108:%s",msgbuf);
		break;
	}

	return;
}


/*
 * check the argument suffix against a colon-separated list of valid C++ suffixes
 */
static int
valid_Cplusplus_suffix(const char *const s)
{
	char* v = stralloc(strlen(CPLUSPLUS_FILE_SUFFIX_LIST));
	char* t;

	strcpy(v, CPLUSPLUS_FILE_SUFFIX_LIST);

	while ((t = strtok(v, ":")) != NULL) {
		if (strcmp(s, t) == 0)
			return 1;	/* suffix is valid */
		v = NULL;
	}

	return 0;	/* suffix is not valid */
}


static char *
getsuf(as)
	char *as;
{
	register char *s;
	static char *empty = "";

	if ((s = strrchr(as, '/')) == NULL)
		s = as;
	else
		s++;

	if ((s = strrchr(s, '.')) == NULL)
		return(empty);
	else if ( *(++s) == '\0' )
		return(empty);
	else
		return(s);
}

/*
 * setsuf() creates a copy and then changes the suffix of "base" to "suff"
 * and returns a pointer to only the filename component (ie. after the 
 * last '/'). The format of the returned string will be "filename.suffix" 
 * even if "base" is missing the '.' or has no current suffix after the '.'.
 */
char *
setsuf(base, suff)
	char *base, *suff;
{
	register char *s1, *s2, *s3;

	if ((s3 = strrchr(base, '/')) == NULL)
		s3 = base;
	else
		++s3;

	if ((s1 = strrchr(s3, '.')) == NULL) {	/* no . in base */
		s2 = stralloc( strlen(s3) + strlen(suff) + 1 );
		(void) strcpy(s2, s3);
		(void) strcat(s2, ".");
	} else if (*(++s1) == '\0') {		/* nothing after . in base */
		s2 = stralloc( strlen(s3) + strlen(suff) );
		(void) strcpy(s2, s3);
	} else {				/* normal .suff ending */
		s2 = stralloc( (s1-s3) + strlen(suff) );
		(void) strncpy(s2, s3, (s1-s3));
	}
	(void) strcat(s2, suff);

	return(s2);
}


int
callsys(char f[], 
	char *v[], 
	int fd,			/* file descr to redirect, see defines */
	int one_onto_two)	/* boolean: redirect 1>&2? see defines */
{
	register pid_t pid, w;
	char *tf;
	int status;

	(void) fflush(stdout);
	(void) fflush(stderr);

	if (debug >= 2) {	/* user specified at least two #'s */
		error('c', gettxt(":36","%scc: process: %s\n"),  prefix, f);
		if (debug >= 3)	/* 3 or more #'s:  don't exec anything */
			return(0);
	}

#ifdef PERF
	ttime = times(&ptimes);
#endif

	if ((pid = fork()) == 0) {
		if (fd >= 0 && tmp4) {
			/* we want to redirect fd (stdin/out/err) to tmp4 */
			if (close(fd) == -1) {
				error('e', gettxt(":1541","cannot redirect file descriptor %d\n"), fd);
				exit(1);
			}
			if (open(tmp4, O_RDWR|O_CREAT, 0666) != fd) {
				error('e', gettxt(":1541","cannot redirect file descriptor %d\n"), fd);
				exit(1);
			}
			if (one_onto_two == STDOUT_TO_STDERR) {
				/* we want to direct stdout onto stderr, i.e. 1>&2 */
				if (close(1) == -1) {
					error('e', gettxt(":1541","cannot redirect file descriptor %d\n"), STDOUT_FILENO);
					exit(1);
				}
				if (dup(2) == -1) {
					error('e', gettxt(":1541","cannot redirect file descriptor %d\n"), STDOUT_FILENO);
					exit(1);
				}
			}
		}
		(void) execv(f, v);
		error('e', gettxt(":37","Can't exec %s\n"), f);
		exit(1);
	}
	else
		if (pid == -1) {
			error('e', gettxt(":38","Process table full - try again later\n"));
			eflag = 1;
			dexit();
		}
	while ((w = wait(&status)) != pid && w != -1) ;

#ifdef PERF
	ttime = times(&ptimes) - ttime;
#endif

	if (w == -1) {
		error('e', gettxt(":39","Lost %s - No child process!\n"), f);
		eflag = w;
		dexit();
	}
	else {
		if (((w = status & 0xff) != 0) && (w != SIGALRM)) {
			if (w != SIGINT) {
				if (w & WCOREFLG)
					error('e',gettxt(":40","Process %s core dumped with signal %d\n"),f,(w & 0x7f));
				else
					error('e', gettxt(":41","Process %s exited with status %d \n"),
					f, status );
			}
			if (  (tf = strrchr(f,'/'))  == NULL )
				tf=f;
			else
				tf++;
			if ( strcmp(tf,passname(prefix,N_OPTIM)) == 0 ) {
				return(status);
				}				
			else {
				eflag = status;
                        	dexit();	
				}
		}
	}
	return((status >> 8) & 0xff);
}

static int
nodup(l, os)
	char **l, *os;
{
	register char *t;

	if ( strcmp(getsuf(os), "o") )
		return(1);
	while(t = *l++) {
		if (strcmp(t,os) == 0)
			return(0);
	}
	return(1);
}

static int
move(from, to)
	char *from, *to;
{
	if (rename(from, to) == -1) {
		error('w', gettxt(":42","Can't move %s to %s\n"), from, to);
		return(1);
	}
	return(0);
}


char *
stralloc(n)
	int n;
{
	register char *s;

	if ((s = (char *)calloc((unsigned)(n+1),1)) == NULL) {
		error('e', gettxt(":35","Out of space\n"));
		dexit();
	}
	return(s);
}

static void
mktemps()
{
	tmp2 = tempnam(TMPDIR, "ctm2");
	tmp3 = tempnam(TMPDIR, "ctm3");
	tmp4 = tempnam(TMPDIR, "ctm4");
	tmp5 = tempnam(TMPDIR, "ctm5");
	tmp6 = tempnam(TMPDIR, "ctm6");
	tmp7 = tempnam(TMPDIR, "ctm7");
#if ASFILT
	tmp8 = tempnam(TMPDIR, "ctm8");
#endif
	tmp9 = tempnam(TMPDIR, "ctm9");
	if (!(tmp2 && tmp3 && tmp4 && tmp5 && tmp6 && tmp7 && tmp9
#if ASFILT
&& tmp8
#endif
	) || creat(tmp2,(mode_t)0666) < 0)
		error('e', gettxt(":43"," cannot create temporaries: %s\n"),  tmp2);
	cunlink(tmp2);
}

/* CCWelse - handle C++ specific extensions for -W */
static int
CCWelse(c)
	char c;
{
	switch(c) {
	case 'f':
		return(Xfe);
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	case 't':
		return(Xpt);
#endif
	}
	return (-1);
}

static int
getXpass(s, opt)
	char	*s, *opt;
{
	int d;

	switch (*s) {
	case '0':
		return(Xc0);
	case '2':
		return(Xc2);
	case 'b':
		return(Xbb);
	case 'p':
		return(Xcp);
	case 'a':
		return(Xas);
	case 'l':
		return(Xld);
	default:
		/* see if there are any C++ or machine specific cases */
		if ((CCflag && (d=CCWelse(*s)) != -1) || (d=Welse(*s)) != -1) {
			return(d);
		} else {
			error('w', gettxt(":44","Unrecognized pass name: '%s%c\n'"), opt, *s);
			return(-1);
		}
	}
}

#ifdef PERF
pexit()
{
	error('e', gettxt(":45","Too many files for performance stats\n"));
	dexit();
}
#endif

#ifdef PERF
pwrap()
{
	int	i;

	if ((perfile = fopen("cc.perf.info", "r")) == NULL)
		dexit();
	fclose(perfile);
	if ((perfile = fopen("cc.perf.info", "a")) == NULL)
		dexit();
	for (i = ii-1; i > 0; i--) {
		stats[i].perform.proc_user_time -= stats[i-1].perform.proc_user_time;
		stats[i].perform.proc_system_time -= stats[i-1].perform.proc_system_time;
		stats[i].perform.child_user_time -= stats[i-1].perform.child_user_time;
		stats[i].perform.child_system_time -= stats[i-1].perform.child_system_time;
	}
	for (i = 0; i < ii; i++)
		(void) fprintf(perfile, "%s\t%07ld\t%07ld\t%07ld\t%07ld\t%07ld\n",
			stats[i].module,stats[i].ttim,stats[i].perform);
	fclose(perfile);
}
#endif


/* function to handle -YL and -YU substitutions in LIBPATH */

static char *
compat_YL_YU(index)
int index;
{
	/* user supplied -YL,libdir  and this is the pathname that corresponds 
		for compatibility to -YL (as defined in paths.h) */
	if(libdir && index == YLDIR)
		return(libdir);

	/* user supplied -YU,llibdir  and this is the pathname that corresponds 
		for compatibility to -YU (as defined in paths.h) */
	if(llibdir && index == YUDIR)
		return(llibdir);

	return(NULL);
}

static void
process_lib_path(pathlib,npathlib)
char * pathlib;
char * npathlib;
{
	int i;
	char * cp;
	char * cp2;


	for(i=1;; i++) {
		cp = strpbrk(pathlib,":");
		if(cp == NULL) {
			cp2 = compat_YL_YU(i);
			if(cp2 == NULL) {
				(void) strcpy(npathlib,pathlib);
			}
			else {
				(void) strcpy(npathlib,cp2);
			}
			return;
		}

		if(*cp == ':') {
			cp2 = compat_YL_YU(i);
			if(cp2 == NULL) {
				(void) strncpy(npathlib,pathlib,cp - pathlib +1);
				npathlib = npathlib + (cp - pathlib + 1);
			}
			else {
				(void) strcpy(npathlib,cp2);
				npathlib += strlen(cp2);
				*npathlib = ':';
				npathlib++;
			}
			pathlib = cp + 1;
			continue;
		}
		
	}
}

/*
 * Process command line arguments that are unique to C++ (CCOPTSTR)
 */

static int
CCoptelse(int c, char* s) {
	char* p;

	if(!CCflag)
		return 0;	/* only applies to C++ */
	switch(c) {
		case 'f':       /* process for Standard Components fs tool*/
                        fflag++;
                        break;
		case 'F':	/* stop after fe, leave .cll (if created) & .int.c files */
			cflag++;
			Fflag++;
                        break;
 		case 'J':	/* controls C++ compilation: -Jnew (default) goes through glue, -Jold goes through C */
 			if (s == 0) {
 				error('e', gettxt(":99999", "Option -J requires an argument\n"));
 				exit(1);
 			}
 			while ((p = strtok(s, ",")) != 0) {
 				if (strcmp(p, "old") == 0) {
 					cplusplus_mode = via_c;
 				} else if (strcmp(p, "new") == 0) {
 					cplusplus_mode = via_glue;
 				}
 				else {
 					error('e', gettxt(":99999",
 						"Illegal argument to -J flag, '-J %s'\n"), p);
 					exit(1);
 				}
 				s = 0;
 			}
 			break;
#if IL_SHOULD_BE_WRITTEN_TO_FILE
		/* must match corresponding define in EDG front end, if not defined -N is not recognized */
		/* (but leaving Nflag in unconditionally, too messy to preprocess out)                   */
		case 'N':	/* stop after fe, leave .cil file (if created) */
			cflag++;
			Nflag++;
			addopt(Xfe,"-N");
                        break;
#endif
		case 'R':	/* controls for pre-compiled headers */
			if (!s) {
				error('e', gettxt(":1592","Option -R requires an argument\n"));
				exit(1);
			} 
			/* -R takes comma-separated list of suboptions */
			p = s;
			while ((s = strtok(p, ",")) != NULL)
			{
				if (strcmp(s, "auto")==0) {
					addopt(Xfe,"--pch");
					Rflag = 1;
				} else {
					char *sub_arg;
					if ((sub_arg = strchr(s, '=')) != 0 && *(sub_arg+1)) {
						/* addopt()'s will be done later,
				   		after conflicting option check */
						if (strncmp(s, "create", sub_arg - s)==0)
							Rcreate = sub_arg+1;
						else if (strncmp(s, "use", sub_arg - s)==0)
							Ruse = sub_arg+1;
						else if (strncmp(s, "dir", sub_arg - s)==0) {
							addopt(Xfe,"--pch_dir");
							addopt(Xfe,sub_arg+1);
						}
						Rflag = 1;
					} else {
						error('e', gettxt(":1593","Illegal argument to -R flag, '-R %s'\n") ,s);
						exit(1);
					}
				}
				p = NULL;
			}
			break;
		case 'T':	/* template controls */
			if (!s) {
				error('e', gettxt(":1545","Option -T requires an argument\n"));
				exit(1);
			}
			/* -T takes comma-separated list of suboptions */
			p = s;
			while ((s = strtok(p, ",")) != NULL)
			{
				if (strcmp(s, "none")==0)
					Tnone = 1;
				else if (strcmp(s, "used")==0)
					Tused = 1;
				else if (strcmp(s, "all")==0)
					Tall = 1;
				else if (strcmp(s, "local")==0)
					Tlocal = 1;
#if AUTOMATIC_TEMPLATE_INSTANTIATION
				else if (strcmp(s, "auto")==0)
					Tauto = 1;
				else if (strcmp(s, "no_auto")==0)
					Tno_auto = 1;
#endif
#if INSTANTIATION_BY_IMPLICIT_INCLUSION
				else if (strcmp(s, "implicit")==0)
					Timplicit = 1;
				else if (strcmp(s, "no_implicit")==0)
					Tno_implicit = 1;
#endif
				else if (strcmp(s, "prelink_objects")==0)
					Tprelink_objects = 1;
				else {
					error('e', gettxt(":1546","Illegal argument to -T flag, '-T %s'\n") ,s);
					exit(1);
				}
				p = NULL;
			}
			++Tflag;
			break;
		default:
			return 0;
	}
	return 1;
}

/*
 * Print out detail Usage messages.
 * This routine calls machine dependent option_mach()
 * because some options are machine dependent. Those
 * options should be handled by machine dependent routine.
 * It also calls option_CC() for C++ specific options.
 */
static void
option_indep()
{
	if (!CCflag) {
		(void) pfmt(stderr,MM_NOSTD,":50:\t[-A name[(tokens)]]: associates name as a predicate with the\n");
		(void) pfmt(stderr,MM_NOSTD,":51:\t\tspecified tokens as if by a #assert preprocessing directive.\n");
	}
	(void) pfmt(stderr,MM_NOSTD,":52:\t[-B c]: governs library search mode: dynamic, static, etc.\n");
	(void) pfmt(stderr,MM_NOSTD,":1672:\t[-b elf]: accepted and ignored for OSR5 compatibility.\n");
	(void) pfmt(stderr,MM_NOSTD,":53:\t[-C]: cause the preprocessing phase to pass all comments.\n");
	(void) pfmt(stderr,MM_NOSTD,":54:\t[-c]: suppress the link editing phase of the compilation and\n");
	(void) pfmt(stderr,MM_NOSTD,":55:\t\tdo not remove any produced object files.\n");
	(void) pfmt(stderr,MM_NOSTD,":56:\t[-D name[=tokens]]: associates name with the specified tokens\n");
	(void) pfmt(stderr,MM_NOSTD,":57:\t\tas if by a #define preprocessing directive.\n");
	(void) pfmt(stderr,MM_NOSTD,":58:\t[-d c]: c can be either y or n. -dy specifies dynamic linking\n");
	(void) pfmt(stderr,MM_NOSTD,":59:\t\tin the link editor. -dn specifies static linking.\n");
	(void) pfmt(stderr,MM_NOSTD,":60:\t[-E]: only preprocess the named source files and send the results\n");
	(void) pfmt(stderr,MM_NOSTD,":61:\t\tto the standard output.\n");
	(void) pfmt(stderr,MM_NOSTD,":62:\t[-e optarg]: pass to link editor.\n");
	(void) pfmt(stderr,MM_NOSTD,":64:\t[-G]: directs the link editor to produce shared object.\n");
	(void) pfmt(stderr,MM_NOSTD,":65:\t[-g]: cause the compiler to produce additional information\n");
	(void) pfmt(stderr,MM_NOSTD,":66:\t\tneeded for the use of the debugger.\n");
	(void) pfmt(stderr,MM_NOSTD,":67:\t[-H]: print the path name of each file included during the\n");
	(void) pfmt(stderr,MM_NOSTD,":68:\t\tcurrent compilation on the standard error output.\n");
	(void) pfmt(stderr,MM_NOSTD,":69:\t[-h optarg]: pass to link editor.\n");
	(void) pfmt(stderr,MM_NOSTD,":70:\t[-I dir]: alter the search for included files whose names don't\n");
	(void) pfmt(stderr,MM_NOSTD,":71:\t\tbegin with / to look in dir prior to the usual directories.\n");
	(void) pfmt(stderr,MM_NOSTD,":1601:\t[-K PIC]: causes position-independent code to be generated.\n");
	(void) pfmt(stderr,MM_NOSTD,":1602:\t[-K thread]: program will use multi-threading facilities.\n");
	(void) pfmt(stderr,MM_NOSTD,":1603:\t[-K [schar|uchar]]: where plain char types are signed or unsigned.\n");
	(void) pfmt(stderr,MM_NOSTD,":78:\t[-L dir]: add dir to the list of directories searched for\n");
	(void) pfmt(stderr,MM_NOSTD,":79:\t\tlibrary by link editor.\n");
	(void) pfmt(stderr,MM_NOSTD,":80:\t[-l name]: search the library libname.a or libname.so.\n");
	(void) pfmt(stderr,MM_NOSTD,":81:\t[-O]: arrange for compilation phase optimization.\n");
	(void) pfmt(stderr,MM_NOSTD,":1673:\t[-o pathname]: produce an output file pathname instead of the default.\n");
	(void) pfmt(stderr,MM_NOSTD,":84:\t[-P]: only preprocess the named source files and leave the result\n");
	(void) pfmt(stderr,MM_NOSTD,":85:\t\tin corresponding files suffixed .i.\n");
	(void) pfmt(stderr,MM_NOSTD,":86:\t[-p]: arrange for the compiler to produce code that counts\n");
	(void) pfmt(stderr,MM_NOSTD,":87:\t\tthe number of times each routine is called.\n");
	(void) pfmt(stderr,MM_NOSTD,":88:\t[-Q c]: c can be either y or n. -Qy indicates identification\n");
	(void) pfmt(stderr,MM_NOSTD,":89:\t\tinformation about each invoked compilation tool will be\n");
	(void) pfmt(stderr,MM_NOSTD,":90:\t\tadded to the output files. -Qn suppress the information.\n");
	(void) pfmt(stderr,MM_NOSTD,":1600:\t[-q [f|l|p]]: -qf is flow profiling, -ql is line profiling,\n");
	(void) pfmt(stderr,MM_NOSTD,":93:\t\t-qp is synonym for -p.\n");
	(void) pfmt(stderr,MM_NOSTD,":94:\t[-S]: compile but do not assemble or link edit the named source files.\n");
	(void) pfmt(stderr,MM_NOSTD,":95:\t[-U name]: causes any definition of name to be forgotten.\n");
	(void) pfmt(stderr,MM_NOSTD,":96:\t[-u optarg]: pass to link editor.\n");
	(void) pfmt(stderr,MM_NOSTD,":97:\t[-V]: cause each invoked tool to print its version information\n");
	(void) pfmt(stderr,MM_NOSTD,":98:\t\ton the standard error output.\n");
	if (CCflag)
		(void) pfmt(stderr,MM_NOSTD,":1599:\t[-v]: issue compiler remarks and prelinker informational messages.\n");
	else {
		(void) pfmt(stderr,MM_NOSTD,":99:\t[-v]: cause the compiler to perform more and stricter semantic\n");
		(void) pfmt(stderr,MM_NOSTD,":100:\t\tcheck, and to enable lint-like checks on the named C files.\n");
	}
	(void) pfmt(stderr,MM_NOSTD,":101:\t[-W tool,arg1[,arg2 ...]]: hand off the arguments \"arg(x)\"\n");
	(void) pfmt(stderr,MM_NOSTD,":102:\t\teach as a separate argument to tool.\n");
	(void) pfmt(stderr,MM_NOSTD,":1551:\t[-w]: cause the compiler to suppress warning messages.\n");
	(void) pfmt(stderr,MM_NOSTD,":103:\t[-X c]: specify degree of conformance to the ANSI/ISO standard.\n");
	(void) pfmt(stderr,MM_NOSTD,":104:\t[-Y item,dir]: specify a new directory dir for item.\n");
	(void) pfmt(stderr,MM_NOSTD,":105:\t[-z optarg]: pass to link editor.\n");
	(void) pfmt(stderr,MM_NOSTD,":1674:\t[-01234]: accepted and ignored for compatibility with other compilers.\n");
	(void) pfmt(stderr,MM_NOSTD,":106:\t[-#]: turn on debug information.\n");
	(void) pfmt(stderr,MM_NOSTD,":107:\t[-?]: display command options usage.\n");

	/* append some machine dependent options messages */
	option_mach();

	/* append some C++ specific options messages */
	if (CCflag)
		option_CC();

	return;
}

static void 
option_CC()
{
	(void) pfmt(stderr,MM_NOSTD,":1543:\t[-f]: prepare code for Standard Components fs tool.\n");
	(void) pfmt(stderr,MM_NOSTD,":1594:\t[-R [auto|create=filename|use=filename]]:\n\t\tcontrols creation and use of pre-compiled headers.\n");
	(void) pfmt(stderr,MM_NOSTD,":1595:\t[-R dir=directory]: controls placement of pre-compiled headers.\n");
	(void) pfmt(stderr,MM_NOSTD,":1549:\t[-T [none|local|used|all]]: control template instantiations.\n");
#if AUTOMATIC_TEMPLATE_INSTANTIATION
	(void) pfmt(stderr,MM_NOSTD,":1550:\t[-T [auto|no_auto]]: control automatic template instantiations.\n");
#endif
#if INSTANTIATION_BY_IMPLICIT_INCLUSION
	(void) pfmt(stderr,MM_NOSTD,":1590:\t[-T [implicit|no_implicit]]: control template implicit inclusion.\n");
#endif

	return;
} 
