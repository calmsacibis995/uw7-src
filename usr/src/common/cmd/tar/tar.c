/*	copyright	"%c%"	*/

#ident	"@(#)tar:tar.c	1.77"
#ident  "$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/vnode.h>
#include <dirent.h>
#include <sys/errno.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <utime.h>
#include <wait.h>
#include <sum.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <deflt.h>
#include <limits.h>
#ifndef LPNMAX
#define LPNMAX PATH_MAX
#define LFNMAX NAME_MAX
#endif /* ! LPNMAX */
#include <locale.h>
#include <pfmt.h>
#include <tar.h>
#include <regex.h>	/* for "yes"/"no" */
#include <nl_types.h>	/* for nl_langinfo */
#include <langinfo.h>	/* for nl_langinfo, YESSTR/NOSTR/YESEXPR */

#ifndef MINSIZE
#define MINSIZE 250
#endif
#define	DEF_FILE "/etc/default/tar"

char	badopen[] = ":402:Cannot open %s";
char	missargI[] = ":403:Missing argument for -I flag\n";
char	onlyone[] = ":404:%c and %c are mutually exclusive\n";
char	badopentwo[] = ":405:Cannot open %s: %s";
char	badopentwon[] = ":92:Cannot open %s: %s\n";
char	badchdir[] = ":406:Cannot change directory to %s";
char	onetapeblock[] = ":409:1 tape block\n";
char	tapeblocks[] = ":410:%lu tape blocks\n";
char	badchdirback[] = ":411:Cannot change back to %s";
char	badpasswdinfo[] = ":458:Cannot get passwd information for %s\n";
char	badgrpinfo[] = ":459:Cannot get group information for %s\n";
char	linkedto[] = ":414:%s: linked to %s\n";
char	linkedtoolong[] = ":415:%s: linked name too long\n";
char	alinkto[] = ":416:a %s link to %s\n";
char	singletoobig[] = ":417:Single file cannot fit on volume\n";
char	badlink[] = ":419:%s: Cannot link: %s\n";
char	xlinkto[] = ":420:%s linked to %s\n";
char	badmknod[] = ":421:%s: mknod() failed";
char	seekequals[] = ":407:seek = %luK\t";
char	luKn[] = ":408:%luK\n";
char	onebyte[] = ":422:x %s, 1 byte, ";
char	manybytes[] = ":423:x %s, %lu bytes, ";
char	badspecial[] = ":424:Cannot create special %s: %s\n";
char	badmkdir[] = ":425:Cannot create directory %s";
char	supabspath[] = ":439:Suppressing absolute pathnames\n";
char	sizechanged[] = ":466:%s: File changed size\n";
char	nomem[] = "847:Out of memory\n";
char	Usage1[] = ":994:Usage:\n\ttar {-c|-r|-u}[vwfbLk{F|X}hienA[0-9999]] [device] [block] [volsize] [incfile|excfile] [files ...]\n\ttar -t[vfLXien[0-9999]] [device] [excfile] [files ...]\n\ttar -x[lmovwfLXpienA[0-9999]] [device] [excfile] [files ...]\n";


extern	void	sumout(),
		sumupd(),
		sumpro(),
		sumepi();

extern 	int	errno;

static	void	add_file_to_table(),
		backtape(),
		build_table(),
		closevol(),
		copy(),
		done(),
		dorep(),
		dotable(),
		doxtract(),
		fatal(),
		flushtape(),
		getdir(),
		getempty(),
		initarg(),
		longt(),
		newvol(),
		passtape(),
		putempty(),
		putfile(),
		readtape(),
		seekdisk(),
		splitfile(),
		tomodes(),
		usage(),
		vperror(),
		writetape(),
		xblocks(),
		xsfile();

static	int	bcheck(),
		checkdir(),
		checksum(),
		checksum_bug(),
		checkupdate(),
		checkw(),
		cmp(),
		defset(),
		endtape(),
		getname(),
		is_in_table(),
		notsame(),
		prefix(),
		response();

struct resp
{
	char	*pat;
	int	ret;
};

static	daddr_t	bsrch();
static	char	*nextarg(),
		*temp_str();
static	long	kcheck();
char	*defread();
static	unsigned int hash();
static	void onintr(), onhup(), onquit();


/* -DDEBUG	ONLY for debugging */
#ifdef	DEBUG
#undef	DEBUG
#define	DEBUG(a,b,c)	(void) fprintf(stderr,"DEBUG - "),(void) fprintf(stderr, a, b, c)
#else
#define	DEBUG(a,b,c)
#endif

#define	TBLOCK	512	/* tape block size--should be universal */

#define NBLOCK	20
#define NAMSIZ	100
#define PRESIZ	155
#define MAXNAM	PATH_MAX
#define	MAXEXT	9	/* reasonable max # extents for a file */
#define	EXTMIN	50	/* min blks left on floppy to split a file */
#define DEVNOLN	4	/* length of device number */

#define EQUAL	0	/* SP-1: for `strcmp' return */
#define	TBLOCKS(bytes)	(((bytes) + TBLOCK - 1)/TBLOCK)	/* useful roundup */
#define	K(tblocks)	((tblocks+1)/2)	/* tblocks to Kbytes for printing */

#define	MAXLEV	18
#define	LEV0	1

#define TRUE	1
#define FALSE	0

/* Was statically allocated tbuf[NBLOCK] */
static
union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];	/* size of this extent if file split */
		char mtime[12];
		char chksum[8];
		char typeflag;
		char linkname[NAMSIZ];
		char magic[6];
		char version[2];
		char uname[32];
		char gname[32];
		char devmajor[8];
		char devminor[8];
		char prefix[155];
		char extno;		/* extent #, null if not split */
		char extotal;		/* total extents */
		char efsize[10];	/* size of entire file */
	} dbuf;
} dblock, *tbuf;

/* Enhanced Application Compatibility Support */
union eac_hblock {
	char eac_dummy[TBLOCK];
	struct eac_header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];	/* size of this extent if file split */
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
		char extno[4];		/* extent #, null if not split */
		char extotal[4];	/* total extents */
		char efsize[12];	/* size of entire file */
#ifdef CHKSUM
		char datsum[8];
#endif
		char cpressed;
	} eac_dbuf;
} eac_dblock, *eac_tbuf;

static int	Cflag;
/* End Enhanced Application Compatibility Support */

static
struct gen_hdr {
	ulong	g_mode,         /* Mode of file */
		g_uid,          /* Uid of file */
		g_gid;          /* Gid of file */
	long	g_filesz;       /* Length of file */
	ulong	g_mtime,        /* Modification time */
		g_cksum,        /* Checksum of file */
		g_devmajor,          /* File system of file */
		g_devminor;         /* Major/minor numbers of special files */
} Gen;

static
struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int     count;
	char	pathname[MAXNAM];
	struct	linkbuf *nextp;
} *ihead;

/* see comments before build_table() */
#define TABLE_SIZE 512
struct	file_list	{
	char	*name;			/* Name of file to {in,ex}clude */
	struct	file_list	*next;	/* Linked list */
};
static struct file_list *exclude_tbl[TABLE_SIZE], *include_tbl[TABLE_SIZE];

static	struct stat stbuf;

static	int	Aflag, Errflg = 0, Fflag, Iflag, Sflag, Xflag,
		bflag, cflag, checkflag = 0, chksum, defaults_used = FALSE,
		eflag, first = TRUE, freemem = 1, hflag, iflag, rflag, kflag,
		linkerrok, mflag, mt, nblock = NBLOCK, oflag, pflag, recno, 
		sflag, term, tflag, totfiles = 0, uflag, vflag, wflag, xflag;

static	int	rblock = 0;		/* # of blocks really read. Can differ
										 * from nblocks at the end of archiv. */

static	int 	update = 1;		/* for `open' call */
static	dev_t	mt_dev;			/* device containing output file */
static	ino_t	mt_ino;			/* inode number of output file */


static	daddr_t	low, high;

static	FILE	*tfile, *vfile = stdout, *Sumfp;
static	char	tname[] = "/tmp/tarXXXXXX";
static	char	archive[7 + DEVNOLN + 1] = "archive";
static	char	*Xfile;
static	char	*namep = (char *) NULL;
static	char	fullname[MAXNAM];
static	char	*usefile;
static	char	*Filefile;
static	char	*Sumfile;
static	struct suminfo	Si;	

static	int	mulvol;		/* multi-volume option selected */
static	long	blocklim;	/* number of blocks to accept per volume */
static	long	tapepos;	/* current block number to be written */
long	atol();			/* to get blocklim */
static	int	NotTape;	/* true if tape is a disk */
static	int	dumping;	/* true if writing a tape or other archive */
static	int	extno;		/* number of extent:  starts at 1 */
static	int	extotal;	/* total extents in this file */
static	long	efsize;		/* size of entire file */
static	ushort	Ftype = S_IFMT;

void
main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp, *dp;
	char usefilef[MAXPATHLEN];	/* full pathname for usefile */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel("UX:tar");

	if (argc < 2)
		usage(1);

	if ((tbuf = (union hblock *) calloc(sizeof(union hblock) * NBLOCK, sizeof(char))) == (union hblock *) NULL) {
		(void) pfmt(stderr, MM_ERROR, 
			":426:Cannot allocate physio buffer: %s\n",
			strerror(errno));
		exit(1);
	}

/* Enhanced Application Compatibility Support */
	if ((eac_tbuf = (union eac_hblock *) calloc(sizeof(union eac_hblock) * NBLOCK, sizeof(char))) == (union eac_hblock *) NULL) {
		(void) pfmt(stderr, MM_ERROR, 
			":426:Cannot allocate physio buffer: %s\n",
			strerror(errno));
		exit(1);
	}
/* End Enhanced Application Compatibility Support */

	tfile = NULL;
	argv[argc] = 0;
	argv++;
	/*
	 * Set up default values.
	 *
	 * tar uses the following criteria, listed in descending order
	 * of precedence, to determine which device to use:
	 *
	 *      Is -f device specified on the command line?
	 *
	 *      Is the TAPE environment variable set?
	 *
	 *      Is the num modifier used on the command line?  (tar
	 *      looks up the specified device in /etc/default/tar.)
	 *
	 *      If none of the above are true, tar uses the default
	 *      device specified by the entry ``archive='' in
	 *      /etc/default/tar.
	 *
	 * Search the option string looking for the first digit or an 'f'.
	 * If you find a digit, use the 'archive#' entry in DEF_FILE.
	 * If 'f' is given, bypass looking in DEF_FILE altogether. 
	 * If no digit or 'f' is given, still look in DEF_FILE but use 'archive'.
	 */
	if ((usefile = getenv("TAPE")) == (char *)NULL) {
		for (cp = *argv; *cp; ++cp)
			if (isdigit(*cp) || *cp == 'f')
				break;
		if (*cp != 'f') {
			for (dp = &archive[7]; isdigit(*cp); ) {
				*dp++ = *cp++;
				if (dp > archive + 7 + DEVNOLN) {
					pfmt(stderr, MM_ERROR,
					     ":995:Maximum archive number is %1.*s\n",
					     DEVNOLN,
					     "999999999999999999999999999999");
					usage(0);
				}
			}
			*dp = '\0';
			if (!(defaults_used = defset(archive))) {
				usefile = NULL;
				nblock = 1;
				blocklim = 0;
				NotTape = 0;
			}
		}
	}

	for (cp = *argv++; *cp; cp++)
		switch(*cp) {
		case 'f':
			usefile = *argv++;
			if (usefile && strcmp(usefile, "-") == 0)
				nblock = 1;
			break;
		case 'F':
			Filefile = *argv++;
			Fflag++;
			if (strcmp(Filefile, "-") == 0)
				nblock = 1;
			break;
		case 'c':
			cflag++;
			rflag++;
			update = 1;
			break;
		case 'u':
			uflag++;     /* moved code after signals caught */
			rflag++;
			update = 2;
			break;
		case 'r':
			rflag++;
			update = 2;
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'x':
			xflag++;
			break;
		case 'X':
			if (*argv == 0) {
				(void) pfmt(stderr, MM_ERROR, 
					":427:Exclude file must be specified with 'X' option\n");
				usage(0);
			}
			Xflag = 1;
			Xfile = *argv++;
			if (strcmp(Xfile, "-") == 0)
				nblock = 1;
			build_table(exclude_tbl, Xfile);
			break;
		case 't':
			tflag++;
			break;
		case 'm':
			mflag++;
			break;
		case 'p':
			pflag++;
			break;
		case 'S':
			Sflag++;
			/*FALLTHRU*/
		case 's':
			if (*argv == 0) {
				pfmt(stderr, MM_ERROR,":887:sum file must be specified with -s or -S options\n");
				usage(0);
			}
			sflag++;
			Sumfile = *argv++;
			break;
		case '-':
			nblock = 1;
			break;
		case '0':	/* numeric entries used only for defaults */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		case 'b':
			if (*argv == 0) {
				pfmt(stderr, MM_ERROR, ":888:blocking factor must be specified with -b option\n");
				usage(0);
			}
			bflag++;
			nblock = bcheck(*argv++);
			break;
		case 'k':
			if (*argv == 0) {
				pfmt(stderr, MM_ERROR, ":889:size value must be specified with -k option\n");
				usage(0);
			}
			kflag++;
			blocklim = kcheck(*argv++);
			break;
		case 'n':	/* not a magtape (instead of 'k') */
			NotTape++;	/* assume non-magtape */
			break;
		case 'l':
			linkerrok++;
			break;
		case 'e':
			eflag++;
			break;
		case 'o':
			oflag++;
			break;
		case 'A':
			Aflag++;
			break;
		case 'L':
		case 'h':
			hflag++;
			break;
		case 'i':
			iflag++;
			break;
		case 'q':
			break;
		/* Enhanced Application Compatibility Support */
		case 'C':
			Cflag++;
			break;
		/* End Enhanced Application Compatibility Support */

		default:
			(void) pfmt(stderr, MM_ERROR, 
				"uxlibc:1:Illegal option -- %c\n", *cp);
			usage(0);
		}

	/* Enhanced Application Compatibility Support */
	if (Cflag && !xflag) {
		(void)pfmt(stderr, MM_ERROR,
			":752:compress (C option) is currently supported only for the xtraction (x).\n");
		usage(0);
	}
	/* End Enhanced Application Compatibility Support */

	if (Xflag && Fflag) {
		(void) pfmt(stderr, MM_ERROR, onlyone, 'X', 'F');
		usage(0);
	}
	if (!rflag && !xflag && !tflag)
		usage(1);
	if ((rflag && xflag) || (xflag && tflag) || (rflag && tflag)) {
		(void) pfmt(stderr, MM_ERROR, 
			":428:t, x, r and u are mutually exclusive\n");
		usage(0);
	}
	if (cflag && !*argv && Filefile == NULL)
		fatal(":429:Missing filenames");
	if (rflag && !cflag && !NotTape && nblock != 1)
		fatal(":430:Blocked tapes cannot be updated");
	if (usefile == NULL)
		fatal(":431:Device argument required");

	/* prepend the current directory if usefile is
	 * a relative pathname. In the case of multiple
	 * volume archives, usefile must be reopened,
	 * but the current directory is not necessarily
	 * the original one.
	 */
	if (*usefile != '/' && strcmp(usefile, "-") != EQUAL &&
					getcwd(usefilef, sizeof usefilef)){
		int len = strlen(usefilef);
		if (len + strlen(usefile) + 1 < sizeof usefilef){
			usefilef[len++] = '/';
			strcpy(usefilef + len, usefile);
			usefile = usefilef;
		}
	}
	if (sflag) {
	    if (Sflag && !mulvol)
		fatal(":434:'S' option requires 'k' option.");
	    if ( !(cflag || xflag || tflag) || ( !cflag && (Filefile != NULL || *argv != NULL)) )
		(void) pfmt(stderr, MM_WARNING, 
			":435:'s' option results are predictable only with 'c' option or 'x' or 't' option and 0 'file' arguments\n");
	    if (strcmp(Sumfile, "-") == 0)
		Sumfp = stdout;
	    else if ((Sumfp = fopen(Sumfile, "w")) == NULL)
		fatal(badopentwo, Sumfile, strerror(errno));
	    sumpro(&Si);
	}

	if (rflag) {
		if (cflag && tfile != NULL)
			usage(1);
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			(void) signal(SIGINT, onintr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			(void) signal(SIGHUP, onhup);
		if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
			(void) signal(SIGQUIT, onquit);
/*              if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
 *                      signal(SIGTERM, onterm);
 */
		if (uflag) {
			(void) mktemp(tname);
			if ((tfile = fopen(tname, "w")) == NULL)
				fatal(":436:Cannot create %s: %s",
				      tname, strerror(errno));
			(void) fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
		}
		if (strcmp(usefile, "-") == 0) {
			if (cflag == 0)
				fatal(":437:Can only create standard output archives.");
			vfile = stderr;
			mt = dup(1);
			nblock = 1;
			++bflag;
		}
		else if ((mt = open(usefile, update)) < 0) {
			if (cflag == 0 || (mt =  creat(usefile, 0666)) < 0)
openerr:
				fatal(badopentwo, usefile, strerror(errno));
		}
		/* Get inode and device number of output file */
		(void) fstat(mt,&stbuf);
		mt_ino = stbuf.st_ino;
		mt_dev = stbuf.st_dev;
		if (Aflag && vflag)
			(void) pfmt(vfile, MM_INFO, supabspath);
		dorep(argv);
	}
	else if (xflag) {
		/* for each argument, check to see if there is a "-I file" pair.
	 	* if so, move the 3rd argument into "-I"'s place, build_table()
	 	* using "file"'s name and increment argc one (the second increment
	 	* appears in the for loop) which removes the two args "-I" and "file"
	 	* from the argument vector.
	 	*/
		for (argc = 0; argv[argc]; argc++) {
			if (!strcmp(argv[argc], "-I")) {
				if (!argv[argc+1]) {
					(void) pfmt(stderr, MM_ERROR, missargI);
					done(2);
				} else {
					Iflag = 1;
					if (Fflag) {
						(void) pfmt(stderr, MM_ERROR, 
							onlyone, 'I', 'F');
						usage(0);
					}

					argv[argc] = argv[argc+2];
					build_table(include_tbl, argv[++argc]);
				}
			}
		}
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			nblock = 1;
			++bflag;
		}
		else if ((mt = open(usefile, 0)) < 0)
			goto openerr;
		if (Aflag && vflag)
			(void) pfmt(vfile, MM_INFO, supabspath);
		doxtract(argv);
	}
	else if (tflag) {
		/* for each argument, check to see if there is a "-I file" pair.
	 	* if so, move the 3rd argument into "-I"'s place, build_table()
	 	* using "file"'s name and increment argc one (the second increment
	 	* appears in the for loop) which removes the two args "-I" and "file"
	 	* from the argument vector.
	 	*/
		for (argc = 0; argv[argc]; argc++)
			if (!strcmp(argv[argc], "-I"))
				if (!argv[argc+1]) {
					(void) pfmt(stderr, MM_ERROR, missargI);
					done(2);
				} else {
					Iflag = 1;
					if (Fflag) {
						(void) pfmt(stderr, MM_ERROR, 
							onlyone, 'I', 'F');
						usage(0);
					}
					argv[argc] = argv[argc+2];
					build_table(include_tbl, argv[++argc]);
				}
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			nblock = 1;
			++bflag;
		}
		else if ((mt = open(usefile, 0)) < 0)
			goto openerr;
		dotable(argv);
	}
	else
		usage(1);

	if (sflag) {
		sumepi(&Si);
		sumout(Sumfp, &Si);
		(void) fprintf(Sumfp, "\n");
	}

	done(Errflg);
}

static	void
usage(complain)
int complain;
{
	if (complain)
		(void) pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
	(void) pfmt(stderr, MM_ACTION, Usage1);
	done(1);
}

/***    dorep - do "replacements"
 *
 *      Dorep is responsible for creating ('c'),  appending ('r')
 *      and updating ('u');
 */

static	void
dorep(argv)
char	*argv[];
{
	register char *cp, *cp2, *p;
	register long remainder;
	char wdir[MAXPATHLEN], tempdir[MAXPATHLEN], *parent = (char *) NULL;
	char file[MAXPATHLEN], origdir[MAXPATHLEN];
	FILE *fp = (FILE *)NULL;
	FILE *ff = (FILE *)NULL;


	if (!cflag) {
		getdir();                       /* read header for next file */
		while (!endtape()) {	     /* changed from a do while */
			passtape();             /* skip the file data */
			if (term)
				done(Errflg);   /* received signal to stop */
			getdir();
		}
		backtape();			/* was called by endtape */
		if (tfile != NULL) {
			char buf[200];

			(void) sprintf(buf, "sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX;mv %sX %s",
				tname, tname, tname, tname, tname, tname);
			(void) fflush(tfile);
			(void) system(buf);
			(void) freopen(tname, "r", tfile);
			(void) fstat(fileno(tfile), &stbuf);
			high = stbuf.st_size;
		}
	}

	dumping = 1;
	if (getcwd(wdir, MAXPATHLEN) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":640:Cannot determine current directory.\n");
		exit(1);
	}
	if (mulvol) {	/* SP-1 */
		/*
		 * If blocklim (specified with -k) is not 
		 * a multiple of nblock, round blocklim down 
		 * to the nearest multiple of nblock.
		 */
		if ((remainder = blocklim%nblock) != 0)
			blocklim = blocklim - remainder;

		blocklim -= 2;			/* for trailer records */

		if (vflag)
			(void) pfmt(vfile, MM_INFO, 
				":445:Volume ends at %luK, blocking factor = %dK\n",
				K(blocklim - 1), K(nblock));
	}
	if (Fflag) {
		if (Filefile != NULL) {
			if ((ff = fopen(Filefile, "r")) == NULL)
				vperror(0, badopen, Filefile);
		} else {
			(void) pfmt(stderr, MM_ERROR, 
				":446:F requires a file name.\n");
			usage(0);
		}
	}

	(void) strcpy(origdir, wdir);
	while ((*argv || fp || ff) && !term) {
		cp2 = 0;
		if (fp || !strcmp(*argv, "-I")) {
			if (Fflag) {
				(void) pfmt(stderr, MM_ERROR, onlyone, 'I', 'F');
				usage(0);
			}
			if (fp == NULL) {
				if (*++argv == NULL) {
					(void) pfmt(stderr, MM_ERROR, missargI);
					done(1);
				} else if ((fp = fopen(*argv++, "r")) == NULL)
					vperror(0, badopen, argv[-1]);
				continue;
			} else if ((fgets(file, MAXPATHLEN-1, fp)) == NULL) {
				(void) fclose(fp);
				fp = NULL;
				continue;
			} else {
				cp = cp2 = file;
				if (p = strchr(cp2, '\n'))
					*p = 0;
			}
		} else if (!strcmp(*argv, "-C") && argv[1]) {
			if (Fflag) {
				(void) pfmt(stderr, MM_ERROR, onlyone, 'F', 'C');
				usage(0);
			}
			if (chdir(*++argv) < 0)
				vperror(0, badchdir, *argv);
			else
				(void) getcwd(wdir, (sizeof(wdir)));
			argv++;
			continue;
		} else if (Fflag && (ff != NULL)) {
			if ((fgets(file, MAXPATHLEN-1, ff)) == NULL) {
				(void) fclose(ff);
				ff = NULL;
				continue;
			} else {
				cp = cp2 = file;
				if (p = strchr(cp2, '\n'))
					*p = 0;
			}
		} else 
			cp = cp2 = strcpy(file, *argv++);

		parent = wdir;
		for (; *cp; cp++)
			if (*cp == '/') {
			/* do not step to next slash if this is the last character */
				if (cp[1] != '\0')
					cp2 = cp;
				else
					*cp = '\0';
			}
		if (cp2 != file) {
			*cp2 = '\0';
			if (chdir(file) < 0) {
				vperror(0, badchdir, file);
				continue;
			}
			parent = getcwd(tempdir, (sizeof(tempdir)));
			*cp2 = '/';
			cp2++;
		}
		putfile(file, cp2, parent, LEV0);
		if (chdir(origdir) < 0)
			vperror(0, badchdirback, wdir);
	}
	putempty(2L);
	flushtape();
	closevol();	/* SP-1 */
	if (linkerrok == 1)
		for (; ihead != NULL; ihead = ihead->nextp) {
			if (ihead->count == 0)
				continue;
			(void) pfmt(stderr, MM_ERROR,
				":450:Missing links to %s\n",
				ihead->pathname);
			if (eflag)
				done(1);
		}
}



/***    endtape - check for tape at end
 *
 *      endtape checks the entry in dblock.dbuf to see if its the
 *      special EOT entry.  Endtape is usually called after getdir().
 *
 *	endtape used to call backtape; it no longer does, he who
 *	wants it backed up must call backtape himself
 *      RETURNS:        0 if not EOT, tape position unaffected
 *                      1 if     EOT, tape position unaffected
 */
static	int
endtape()
{
	/* Enhanced Application Compatibility Support */
	if (Cflag) {
		if (eac_dblock.eac_dbuf.name[0] == '\0') {	/* null header = EOT */
			return(1);
		}
		else
			return(0);
	/* End Enhanced Application Compatibility Support */
	} else {
		if (dblock.dbuf.name[0] == '\0') {	/* null header = EOT */
			return(1);
		}
		else
			return(0);
	}
}

/***    getdir - get directory entry from tar tape
 *
 *      getdir reads the next tarblock off the tape and cracks
 *      it as a directory.  The checksum must match properly.
 *
 *      If tfile is non-null getdir writes the file name and mod date
 *      to tfile.
 */

static	void
getdir()
{
	register struct stat *sp;
	int	sunflag = 0;

	/* Enhanced Application Compatibility Support */
	if (Cflag) {	
		readtape( (char *) &eac_dblock);

#ifdef CHKSUM
		if (gotsum = (eac_dblock.eac_dbuf.datsum[0] != '\0'))
			sscanf(eac_dblock.eac_dbuf.datsum, "%o", &datsum);
#endif

		if (eac_dblock.eac_dbuf.name[0] == '\0')
			return;
		totfiles++;
		sp = &stbuf;
		(void) sscanf(eac_dblock.eac_dbuf.mode, "%8lo", &Gen.g_mode);
		(void) sscanf(eac_dblock.eac_dbuf.uid, "%8lo", &Gen.g_uid);
		(void) sscanf(eac_dblock.eac_dbuf.gid, "%8lo", &Gen.g_gid);
		(void) sscanf(eac_dblock.eac_dbuf.size, "%12lo", &Gen.g_filesz);
		(void) sscanf(eac_dblock.eac_dbuf.mtime, "%12lo", &Gen.g_mtime);
		(void) sscanf(eac_dblock.eac_dbuf.chksum, "%8lo", &Gen.g_cksum);
	
		sp->st_mode = Gen.g_mode;
		sp->st_uid = Gen.g_uid;
		sp->st_gid = Gen.g_gid;
		sp->st_size = Gen.g_filesz;
		sp->st_mtime = Gen.g_mtime;
	
		if (eac_dblock.eac_dbuf.extno != '\0') {	/* split file? */
			(void) sscanf(eac_dblock.eac_dbuf.extno, "%o", &extno);
			(void) sscanf(eac_dblock.eac_dbuf.extotal, "%o", &extotal);
			(void) sscanf(eac_dblock.eac_dbuf.efsize, "%10lo", &efsize);
		} else
				extno = 0;	/* tell others file not split */
		chksum = Gen.g_cksum;
		if (chksum != checksum() && chksum != checksum_bug()) {
			(void) pfmt(stderr, MM_ERROR, 
				":451:Directory checksum error\n");
			done(2);
		}
		if (tfile != NULL)
			(void) fprintf(tfile, "%s %s\n", eac_dblock.eac_dbuf.name, eac_dblock.eac_dbuf.mtime);
	/* End Enhanced Application Compatibility Support */
	} else {
top:
		readtape( (char *) &dblock);
		if (dblock.dbuf.name[0] == '\0')
			return;
		totfiles++;
		sp = &stbuf;
		(void) sscanf(dblock.dbuf.mode, "%8lo", &Gen.g_mode);
		(void) sscanf(dblock.dbuf.uid, "%8lo", &Gen.g_uid);
		(void) sscanf(dblock.dbuf.gid, "%8lo", &Gen.g_gid);
		(void) sscanf(dblock.dbuf.size, "%12lo", &Gen.g_filesz);
		(void) sscanf(dblock.dbuf.mtime, "%12lo", &Gen.g_mtime);
		(void) sscanf(dblock.dbuf.chksum, "%8lo", &Gen.g_cksum);

		/* Check for SUN Style tar files */
		if (dblock.dbuf.name[strlen(dblock.dbuf.name)-1] == '/')
			sunflag = 1;

		sp->st_mode = Gen.g_mode;
		sp->st_uid = Gen.g_uid;
		sp->st_gid = Gen.g_gid;
		sp->st_size = Gen.g_filesz;
		sp->st_mtime = Gen.g_mtime;

		switch(dblock.dbuf.typeflag){
		case REGTYPE:
		case AREGTYPE:
		case LNKTYPE:	/* No actual type info available for links */
				/* Likely to be a regular file anyway */
			sp->st_mode |= S_IFREG;
			break;
		case SYMTYPE:
			sp->st_mode |= S_IFLNK;
			break;
		case CHRTYPE:
			sp->st_mode |= S_IFCHR;
			break;
		case BLKTYPE:
			sp->st_mode |= S_IFBLK;
			break;
		case DIRTYPE:
			sp->st_mode |= S_IFDIR;
			break;
		case FIFOTYPE:
			sp->st_mode |= S_IFIFO;
			break;
		case NAMTYPE:
			sp->st_mode |= S_IFNAM;
			break;
		}
	
		if (!strncmp(dblock.dbuf.magic, "ustar", 5)) {
			(void) sscanf(dblock.dbuf.devmajor, "%8lo", &Gen.g_devmajor);
			(void) sscanf(dblock.dbuf.devminor, "%8lo", &Gen.g_devminor);
			if (dblock.dbuf.extno != '\0') {	/* split file? */
				extno = dblock.dbuf.extno;
				extotal = dblock.dbuf.extotal;
				(void) sscanf(dblock.dbuf.efsize, "%10lo", &efsize);
			} else
				extno = 0;	/* tell others file not split */
		} else {
			/* We are reading in an old xenix tar archive that may	 *
			 * have files split across floppies. If this is the case *
			 * then the information we need is in the 20 bytes	 *
			 * following the linkname field in the header.  The	 *
			 * extension number is in the first four bytes of the	 *
			 * magic field.  The total number of extensions is in	 *
			 * the last two bytes of the magic field and the first	 *
			 * two bytes of the version field.  The size of each 	 *
			 * extension is in the first twelve bytes of the uname	 *
			 * field.						 *
			 *							 *
		 	 * Old field        where          size   		 *
		 	 * extno	 dbuf.magic[0]    4 bytes 	 	 *
			 * extotal       dbuf.magic[4]    4 bytes 		 *
			 * efsize        dbuf.uname[0]   12 bytes 		 *
			 */

			if (dblock.dbuf.magic[0] != '\0') {	/* file is split */
				(void)sscanf(&dblock.dbuf.magic[0], "%04o", &extno);
				(void)sscanf(&dblock.dbuf.magic[4], "%04o", &extotal);
				(void)sscanf(&dblock.dbuf.uname[0], "%012lo", &efsize);
			} else 
				extno = 0;
		}
		
		chksum = Gen.g_cksum;
		if (chksum != checksum() && chksum != checksum_bug()) {
			(void) pfmt(stderr, MM_ERROR, 
				":451:Directory checksum error\n");
			if (iflag)
				goto top;
			done(2);
		}

		if (sunflag) 
			dblock.dbuf.typeflag = DIRTYPE;

		Gen.g_mode &= MODEMASK;
		if (tfile != NULL)
			(void) fprintf(tfile, "%s %s\n", dblock.dbuf.name, dblock.dbuf.mtime);
	}  /* if (Cflag) */

	if (extno != 0) {
		/*
		 * Check if extotal is out of range or if extno > extotal.
		 * If either is bad, we ignore it anyway, so set everything
		 * to 0 as if there were no extents. This is here to weed
		 * out bogus extent info that may come in an old and "foreign"
		 * tar archive.  If we have ridiculous extent info, just go on
		 * and try to extract the file, since the extents are obviously 
		 * meaningless but there's a good chance the file is still ok.
		 */
		if ((extotal < 1) || (extotal > MAXEXT) || (extno > extotal)) {
			extno = 0;
			extotal = 0;
		}
	}

}



/***    passtape - skip over a file on the tape
 *
 *      passtape skips over the next data file on the tape.
 *      The tape directory entry must be in dblock.dbuf.  This
 *      routine just eats the number of blocks computed from the
 *      directory size entry; the tape must be (logically) positioned
 *      right after thee directory info.
 */
static	void
passtape()
{
	long blocks;
	char buf[TBLOCK];

	/* Enhanced Application Compatibility Support */
	if (Cflag) {
		if (eac_dblock.eac_dbuf.linkflag == LNKTYPE)
			return;
		blocks = TBLOCKS(stbuf.st_size);

		/* if operating on disk, seek instead of reading */
		if (NotTape && !sflag
#ifdef CHKSUM
		&& !zflag) {
			cursum = 0;
			didsum = 0;
#else
		) {
#endif
			seekdisk(blocks);
		}
		else {	
			while (blocks-- > 0) {
				readtape(buf);
			}
		}
	/* End Enhanced Application Compatibility Support */
	} else {
		/*
		 * Types link(1), sym-link(2), char special(3), blk special(4),
		 *  directory(5), and FIFO(6) do not have data blocks associated
		 *  with them so just skip reading the data block.
		 */
		if (dblock.dbuf.typeflag == LNKTYPE || dblock.dbuf.typeflag == SYMTYPE ||
			dblock.dbuf.typeflag == CHRTYPE || dblock.dbuf.typeflag == BLKTYPE ||
			dblock.dbuf.typeflag == DIRTYPE || dblock.dbuf.typeflag == FIFOTYPE ||
			dblock.dbuf.typeflag == NAMTYPE)
			return;
		blocks = TBLOCKS(stbuf.st_size);

		/* if operating on disk, seek instead of reading */
		if (NotTape && !sflag)
			seekdisk(blocks);
		else
			while (blocks-- > 0)
				readtape(buf);
	} /* if (Cflag) */
}

static
void
verb1(fname, blks)
char *fname;
long blks;
{
		if (vflag) {
			if (NotTape)
				(void) pfmt(vfile, MM_INFO, ":407:seek = %luK\t",
					K(tapepos));
			(void) fprintf(vfile, "a %s ", fname);
			if (NotTape)
				(void) pfmt(vfile, MM_NOSTD, luKn, K(blks));
			else if (blks == 1)
				(void) pfmt(vfile, MM_NOSTD, onetapeblock);
			else
				(void) pfmt(vfile, MM_NOSTD, tapeblocks, blks);
		}
		return;
}

static struct linkbuf *getmem();

static	void
putfile(longname, shortname, parent, lev)
char *longname;
char *shortname;
char *parent;
int lev;
{
	int infile;
	long blocks;
	char buf[TBLOCK];
	char filetmp[NAMSIZ];
	register char *cp;
	char *name;
	struct dirent *dp;
	struct passwd *dpasswd;
	struct group *dgroup;
	DIR *dirp;
	int i = 0;
	long l;
	int split = 0;
	char newparent[MAXNAM+64];
	char *tmpbuf = NULL;
	char dirbuf[MAXNAM+1];
	char goodbuf[MAXNAM], junkbuf[MAXNAM];
	char abuf[PRESIZ];
	char *prefix = &abuf[0];
	char *lastcomp;
	char type;
	mode_t mode;

	for(i=0; i <MAXNAM; i++) {
		goodbuf[i] = '\0';
		junkbuf[i] = '\0';
	}
	for(i=0; i < NAMSIZ; i++)
		filetmp[i] = '\0';
	for(i=0; i < PRESIZ; i++)
		abuf[i] = '\0';
	if (lev >= MAXLEV) {
		/*
		 * Notice that we have already recursed, so we have already
		 * allocated our frame, so things would in fact work for this
		 * level.  We put the check here rather than before each
		 * recursive call because it is cleaner and less error prone.
		 */
		(void) pfmt(stderr, MM_ERROR, 
			":452:Directory nesting too deep, %s not dumped\n", 
			longname);
		return;
	}
	if (!hflag)
		i = lstat(shortname, &stbuf);
	else
		i = stat(shortname, &stbuf);

	if (i < 0) {
		(void) pfmt(stderr, MM_ERROR, ":21:Cannot access %s: %s\n", 
			longname, strerror(errno));
		return;
	}

	if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
		/*
		 * For compatibility with bsd/sunos tars that ignore the
		 * typeflag and decide a file is a directory when it's name
		 * end's in '/'.  
		 */
		sprintf(dirbuf, "%s/", longname);
		longname = dirbuf;
	}

	/*
	 * Check if the input file is the same as the tar file we
	 * are creating
	 */
	if((mt_ino == stbuf.st_ino) && (mt_dev == stbuf.st_dev)) {
		(void) pfmt(stderr, MM_ERROR, 
			":453:%s same as archive file\n", longname);
		Errflg = 1;
		return;
	}

	/*
	 * Silently ignore /proc files.
	 */

	if (strcmp(stbuf.st_fstype, "proc") == 0)
		return;

	if (tfile != NULL && checkupdate(longname) == 0) {
		return;
	}
	if (checkw('r', longname) == 0) {
		return;
	}

	if (Xflag) {
		if (is_in_table(exclude_tbl, longname)) {
			if (vflag) {
				(void) pfmt(vfile, MM_NOSTD, 
					":454:a %s excluded\n", longname);
			}
			return;
		}
	}
	/* If the length of the fullname is greater than PATH_MAX,
	   print out a message and return.
	*/

	if ((split = strlen(longname)) > MAXNAM) {
		(void) pfmt(stderr, MM_ERROR, 
			":455:%s: File name too long\n", longname);
		if (eflag)
			done(1);
		return;
	} else if (split > NAMSIZ) {
		/* The length of the fullname is greater than 100, so
		   we must split the filename from the path
		*/
		(void) strcpy(&goodbuf[0], longname);
		tmpbuf = goodbuf;
		lastcomp = strrchr(tmpbuf, '\0') - 1;
		if (*lastcomp == '/')
			lastcomp--;
		while (lastcomp > tmpbuf && *lastcomp != '/')
			lastcomp--;
		if (*lastcomp == '/')
			lastcomp++;
		i = split - (lastcomp - tmpbuf);
		/* If the filename is greater than 100 we cannot
		   archive the file
		*/
		if (i > NAMSIZ) {
			(void) pfmt(stderr, MM_ERROR, 
				":456:%s: Filename is greater than %d\n", 
				lastcomp, NAMSIZ);
			if (eflag)
				done(1);
			return;
		}
		(void) strcpy(&junkbuf[0], lastcomp);
		/* If the prefix is greater than 155 we cannot archive the
		   file.
		*/
		if ((split - i) > PRESIZ) {
			(void) pfmt(stderr, MM_ERROR, 
				":457:%s: prefix is greater than %d\n", 
				longname, PRESIZ);
			if (eflag)
				done(1);
			return;
		}
		(void) strncpy(&abuf[0], &goodbuf[0], split - i);
		name = junkbuf;
	} else {
		name = longname;
	}
	if (Aflag)
		if ((prefix != NULL) && (*prefix != '\0'))
			while (*prefix == '/')
				++prefix;
		else
			while(*name == '/')
				++name;

	tomodes(&stbuf);
	(void) strncpy(dblock.dbuf.name, name, (size_t)NAMSIZ);
	(void) sprintf(dblock.dbuf.linkname, "%s", "\0");
	(void) sprintf(dblock.dbuf.magic, "%s", "ustar");
	(void) sprintf(dblock.dbuf.version, "%2s", "00");
	dpasswd = getpwuid(stbuf.st_uid);
	if (dpasswd == (struct passwd *) NULL) {
		(void) pfmt(stderr, MM_WARNING, badpasswdinfo, longname);
		dblock.dbuf.uname[0]='\0';
	} else
		(void) sprintf(dblock.dbuf.uname, "%s",  dpasswd->pw_name);
	dgroup = getgrgid(stbuf.st_gid);
	if (dgroup == (struct group *) NULL) {
		(void) pfmt(stderr, MM_WARNING, badgrpinfo, longname);
		dblock.dbuf.gname[0]='\0';
	} else
		(void) sprintf(dblock.dbuf.gname, "%s", dgroup->gr_name);
	if ((stbuf.st_mode & S_IFMT) == S_IFBLK || 
	    (stbuf.st_mode & S_IFMT) == S_IFCHR || 
	    (stbuf.st_mode & S_IFMT) == S_IFNAM) {
		(void) sprintf(dblock.dbuf.devmajor, "%07o", major(stbuf.st_rdev));
		(void) sprintf(dblock.dbuf.devminor, "%07o", minor(stbuf.st_rdev));
	} else {
		(void) sprintf(dblock.dbuf.devmajor, "%07o", major(stbuf.st_dev));
		(void) sprintf(dblock.dbuf.devminor, "%07o", minor(stbuf.st_dev));
	}
	(void) strncpy(dblock.dbuf.prefix, prefix, (size_t)PRESIZ);

	
	switch (mode = stbuf.st_mode & S_IFMT) {
	case S_IFDIR:
		stbuf.st_size = 0;
		blocks = 0;
		(void) sprintf(dblock.dbuf.size, "%011lo", stbuf.st_size);
		cp = strrchr(longname, '\0');
		dblock.dbuf.typeflag = DIRTYPE;
		(void) sprintf(dblock.dbuf.chksum, "%07o", checksum());
		writetape((char *)&dblock);
		verb1(longname, blocks);
		if (*shortname != '/')
			(void) sprintf(newparent, "%s/%s", parent, shortname);
		else
			(void) sprintf(newparent, "%s", shortname);
		if (chdir(shortname) < 0) {
			vperror(0, badchdir, newparent);
			(void) close(infile);
			return;
		}
		if ((dirp = opendir(".")) == NULL) {
			vperror(0, ":460:Cannot open directory %s", longname);
			if (chdir(parent) < 0)
				vperror(0, badchdirback, parent);
			(void) close(infile);
			return;
		}
		while ((dp = readdir(dirp)) != NULL && !term) {
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			(void) strcpy(cp, dp->d_name);
			errno = 0;  /* for the sake of telldir */
			l = telldir(dirp);
			if (errno == ENOSYS) {
				/* break to avoid looping infinitely in /dev/fd 
					("lseek" is not implemented in the kernel for the fdfs
					filesystem, and therefore errno=ENOSYS) */
				(void) closedir(dirp);
				(void) pfmt(stderr,MM_ERROR,":757:Cannot process directory %s: no system call for this file system\n");
				break;
			}
			(void) closedir(dirp);
			putfile(longname, cp, newparent, lev + 1);
			dirp = opendir(".");
			errno = 0;
			seekdir(dirp, l);
			if (errno) {
				(void) closedir(dirp);
				pfmt(stderr, MM_ERROR,":886:seekdir of %s failed:  errno %d\n", newparent, errno);
			}
		}
		(void) closedir(dirp);
		if (chdir(parent) < 0)
			vperror(0, badchdirback, parent);
			(void) close(infile);
		return;

	case S_IFLNK:
		if (stbuf.st_size + 1 >= NAMSIZ) {
			(void) pfmt(stderr, MM_ERROR, 
				":461:%s: Symbolic link too long\n", 
				longname);
			if (eflag)
				done(1);
			return;
		}
		/*
		 * Sym-links need header size of zero since you
		 * don't store any data for this type.
		 */
		stbuf.st_size = 0;
		(void) sprintf(dblock.dbuf.size, "%011lo", stbuf.st_size);
		i = readlink(shortname, filetmp, NAMSIZ - 1);
		if (i < 0) {
			vperror(0, ":462:Cannot read symbolic link %s", longname);
			return;
		}
		else 
			filetmp[i] = '\0';   /* since readlink does not supply a '\0' */
		(void) sprintf(dblock.dbuf.linkname, "%s", filetmp);
		dblock.dbuf.typeflag = SYMTYPE;
		if (vflag)
			(void) pfmt(vfile, MM_NOSTD, 
				":463:a %s symbolic link to %s\n", longname, 
				filetmp);
		(void) sprintf(dblock.dbuf.chksum, "%07o", checksum());
		dblock.dbuf.typeflag = SYMTYPE;
		break;

	case S_IFREG:
		if ((infile = open(shortname, 0)) < 0) {
			vperror(0, badopen, longname);
			return;
		}

		blocks = (stbuf.st_size + (TBLOCK-1)) / TBLOCK;
		if (stbuf.st_nlink > 1) {
			struct linkbuf *lp;
			int found = 0;

			for (lp = ihead; lp != NULL; lp = lp->nextp)
				if (lp->inum == stbuf.st_ino &&
				    lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			if (found) {
				if(strlen(lp->pathname) > (size_t)NAMSIZ) {
					(void) pfmt(stderr, MM_ERROR, linkedto,
						longname, lp->pathname);
					(void) pfmt(stderr, MM_ERROR,
						linkedtoolong, lp->pathname);
					if (eflag)
						done(1);
					(void) close(infile);
					return;
				}
				stbuf.st_size = 0;
				(void) sprintf(dblock.dbuf.size, "%011lo", stbuf.st_size);
				(void) strncpy(dblock.dbuf.linkname, lp->pathname, (size_t)NAMSIZ);
				dblock.dbuf.typeflag = LNKTYPE;
				(void) sprintf(dblock.dbuf.chksum, "%07o", checksum());
				dblock.dbuf.typeflag = LNKTYPE;
				if (mulvol && tapepos + 1 >= blocklim)
					newvol();
				writetape((char *) &dblock);
				if (vflag) {
					if (NotTape)
						(void) pfmt(vfile, MM_INFO, 
							seekequals, K(tapepos));
					(void) pfmt(vfile, MM_NOSTD, alinkto,
						longname, lp->pathname);
				}
				lp->count--;
				(void) close(infile);
				return;
			} else {
				lp = getmem(sizeof(*lp));
				if (lp != NULL) {
					lp->nextp = ihead;
					ihead = lp;
					lp->inum = stbuf.st_ino;
					lp->devnum = stbuf.st_dev;
					lp->count = stbuf.st_nlink - 1;
					(void) strcpy(lp->pathname, longname);
				}
			}
		}
		/* correctly handle end of volume */
		while (mulvol && tapepos + blocks + 1 > blocklim) { /* file won't fit */
			if (eflag) {
				if (blocks <= blocklim) {
					newvol();
					break;
				}
				(void) pfmt(stderr, MM_ERROR, singletoobig);
				done(3);
			}
			/* split only if floppy has some room and file is large */
	    		if (blocklim - tapepos >= EXTMIN && blocks + 1 >= blocklim/10) {
				splitfile(longname, infile);
				(void) close(infile);
				return;
			}
			newvol();	/* not worth it--just get new volume */
		}

		DEBUG("putfile: %s wants %lu blocks\n", longname, blocks);
		verb1(longname, blocks);
		dblock.dbuf.typeflag = REGTYPE;
		(void) sprintf(dblock.dbuf.chksum, "%07o", checksum());
		dblock.dbuf.typeflag = REGTYPE;
		writetape((char *)&dblock);
		while ((i = read(infile, buf, TBLOCK)) > 0 && blocks > 0) {
			if (term) {
				(void) pfmt(stderr, MM_ERROR, 
					":464:Interrupted in the middle of a file\n");
				done(Errflg);
			}

			writetape(buf);
			blocks--;
		}
		(void) close(infile);
		if (i < 0)
			vperror(0, ":465:Read error on %s", longname);
		else if (blocks != 0 || i != 0) {
			(void) pfmt(stderr, MM_ERROR, sizechanged, longname);
			if (eflag)
				done(1);
		}
		putempty(blocks);
		return;

	case S_IFIFO:
	case S_IFCHR:
	case S_IFBLK:
	case S_IFNAM:
		/* Set generic type variable to use for 
		 * setting typeflag later.  Other than assigning
		 * different typeflag values, we do exactly the 
		 * same thing for these file types.
		 */
		switch(mode) {
			case S_IFIFO: 
				type = FIFOTYPE; 
				break;
			case S_IFCHR: 
				type = CHRTYPE; 
				break;
			case S_IFBLK: 
				type = BLKTYPE; 
				break;
			case S_IFNAM: 
				type = NAMTYPE; 
				break;
		}

		blocks = (stbuf.st_size + (TBLOCK-1)) / TBLOCK;
		stbuf.st_size = 0;
		(void) sprintf(dblock.dbuf.size, "%011lo", stbuf.st_size);
		if (stbuf.st_nlink > 1) {
			struct linkbuf *lp;
			int found = 0;

			for (lp = ihead; lp != NULL; lp = lp->nextp)
				if (lp->inum == stbuf.st_ino &&
				    lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			if (found) {
				if(strlen(lp->pathname) > (size_t)(NAMSIZ -1)) {
					(void) pfmt(stderr, MM_ERROR, linkedto, 
						longname, lp->pathname);
					(void) pfmt(stderr, MM_ERROR, 
						linkedtoolong, lp->pathname);
					if (eflag)
						done(1);
					return;
				}
				(void) sprintf(dblock.dbuf.linkname, "%s", lp->pathname);
				dblock.dbuf.typeflag = type;
				(void) sprintf(dblock.dbuf.chksum, "%07o", checksum());
				dblock.dbuf.typeflag = type;
				
				if (mulvol && tapepos + 1 >= blocklim)
					newvol();
				writetape( (char *) &dblock);
				if (vflag) {
					if (NotTape)
						(void) pfmt(vfile, MM_INFO, 
							seekequals, K(tapepos));
					(void) pfmt(vfile, MM_NOSTD, alinkto, 
						longname, lp->pathname);
				}
				lp->count--;
				return;
			} else {
				lp = getmem(sizeof(*lp));
				if (lp != NULL) {
					lp->nextp = ihead;
					ihead = lp;
					lp->inum = stbuf.st_ino;
					lp->devnum = stbuf.st_dev;
					lp->count = stbuf.st_nlink - 1;
					(void) strcpy(lp->pathname, longname);
				}
			}
		}

		while (mulvol && tapepos + blocks + 1 > blocklim) { 
			if (eflag) {
				if (blocks <= blocklim) {
					newvol();
					break;
				}
				(void) pfmt(stderr, MM_ERROR, singletoobig);
				done(3);
			}
			newvol();
		}

		DEBUG("putfile: %s wants %lu blocks\n", longname, blocks);
		verb1(longname, blocks);
		dblock.dbuf.typeflag = type;
		(void) sprintf(dblock.dbuf.chksum, "%07o", checksum());
		dblock.dbuf.typeflag = type;
		break;

	default:
		(void) pfmt(stderr, MM_ERROR,
			":467:%s is not a file. Not dumped\n", longname);
		if (eflag)
			done(1);
		break;

	} /* end switch(mode = stbuf.st_mode & S_IFMT) */

	writetape((char *)&dblock);
	return;
}

/***	splitfile	dump a large file across volumes
 *
 *	splitfile(longname, fd);
 *		char *longname;		full name of file
 *		int ifd;		input file descriptor
 *
 *	NOTE:  only called by putfile() to dump a large file.
 */
static	void
splitfile(longname, ifd)
char *longname;
int ifd;
{
	long blocks, bytes, s;
	char buf[TBLOCK];
	register int i = 0, extents = 0;

	blocks = TBLOCKS(stbuf.st_size);	/* blocks file needs */

	/* # extents =
	 *	size of file after using up rest of this floppy
	 *		blocks - (blocklim - tapepos) + 1	(for header)
	 *	plus roundup value before divide by blocklim-1
	 *		+ (blocklim - 1) - 1
	 *	all divided by blocklim-1 (one block for each header).
	 * this gives
	 *	(blocks - blocklim + tapepos + 1 + blocklim - 2)/(blocklim-1)
	 * which reduces to the expression used.
	 * one is added to account for this first extent.
	 */
	extents = (blocks + tapepos - 1L)/(blocklim - 1L) + 1;

	if (extents < 2 || extents > MAXEXT) {	/* let's be reasonable */
		(void) pfmt(stderr, MM_ERROR, 
			":468:%s needs unusual number of volumes to split\n",
			longname);
		(void) pfmt(stderr, MM_ERROR, ":469:%s not dumped\n",
			longname);
		return;
	}
	dblock.dbuf.extotal = extents;
	bytes = stbuf.st_size;
	(void) sprintf(dblock.dbuf.efsize, "%lo", bytes);

	(void) pfmt(stderr, MM_INFO, 
		":766:Large file %s needs %d volumes.\n", longname, extents);
	(void) pfmt(stderr, MM_INFO, 
		":471:Current device seek position = %luK\n", K(tapepos));

	s = (blocklim - tapepos - 1) * TBLOCK;
	for (i = 1; i <= extents; i++) {
		if (i > 1) {
			newvol();
			if (i == extents)
				s = bytes;	/* last ext. gets true bytes */
			else
				s = (blocklim - 1)*TBLOCK; /* whole volume */
		}
		bytes -= s;
		blocks = TBLOCKS(s);

		(void) sprintf(dblock.dbuf.size, "%lo", s);
		dblock.dbuf.extno = i;
		(void) sprintf(dblock.dbuf.chksum, "%07o", checksum());
		writetape( (char *) &dblock);

		if (vflag)
			(void) pfmt(vfile, MM_NOSTD, 
				":769:+++ a %s %luK [volume #%d of %d]\n", 
				longname, K(blocks), i, extents);
		while (blocks > 0 && read(ifd, buf, TBLOCK) > 0) {
			blocks--;
			writetape(buf);
		}
		if (blocks != 0) {
			(void) pfmt(stderr, MM_ERROR, sizechanged, longname);
			(void) pfmt(stderr, MM_ERROR, 
				":474:Aborting split file %s\n", longname);
			(void) close(ifd);
			return;
		}
	}
	(void) close(ifd);
	if (vflag)
		if (extents == 1)
			(void) pfmt(vfile, MM_NOSTD, 
				":767:a %s %luK (in 1 volume)\n", longname, K(
				TBLOCKS(stbuf.st_size)));
		else
			(void) pfmt(vfile, MM_NOSTD, 
				":768:a %s %luK (in %d volumes)\n", longname, K(
				TBLOCKS(stbuf.st_size)), extents);
}

static	void
doxtract(argv)
char	*argv[];
{
	struct	stat	xtractbuf;	/* stat on file after extracting */
	long blocks, bytes;
	char *curarg;
	int ofile;
	int xcnt = 0;			/* count # files extracted */
	int fcnt = 0;			/* count # files in argv list */
	int dir = 0;
	ushort Uid;
	char *linkp;			/* for removing absolute paths */
	extern errno;
	char dirname[MAXNAM];
	struct passwd *dpasswd;
	struct group *dgroup;
	int once = 1;
	int symflag;
	int xret = 0;
	char type;
	mode_t mode;
	dev_t dev;

	dumping = 0;	/* for newvol(), et al:  we are not writing */

	/*
	 * Count the number of files that are to be extracted 
	 */
	Uid = getuid();
	initarg(argv, Filefile);
	while (nextarg() != NULL)
		++fcnt;

	/* Enhanced Application Compatibility Support */
	if (Cflag) {
		for (;;) {
			symflag = 0;
			linkp = (char *) NULL;
			dir = 0;
			xret = getname();
			if (xret == 2)
				continue;
			else if (xret == 1)
				goto eac_gotit;

			initarg(argv, Filefile);
			if (endtape())
				break;
			if (Aflag)
				while (*namep == '/')  /* step past leading slashes */
					namep++;
			if ((curarg = nextarg()) == NULL)
				goto eac_gotit;

			for ( ; curarg != NULL; curarg = nextarg())
				if (prefix(curarg, namep))
					goto eac_gotit;
			passtape();
			continue;

eac_gotit:
			if (checkw('x', namep) == 0) {
				passtape();
				continue;
			}

			(void) strcpy(&dirname[0], namep);
			if (checkdir(&dirname[0])) {
				dir = 1;
				if (vflag) {
					(void) pfmt(vfile, MM_NOSTD, 
						":477:x %s, 0 bytes, ", &dirname[0]);
					if (NotTape)
						(void) pfmt(vfile, MM_NOSTD, 
							":478:0K\n");
					else
						(void) pfmt(vfile, MM_NOSTD, 
							":479:0 tape blocks\n");
				}
				goto eac_filedone;
			}

			if (eac_dblock.eac_dbuf.cpressed == '1' && !Xflag) Cflag=1;
			if (eac_dblock.eac_dbuf.linkflag == LNKTYPE) {
				linkp = eac_dblock.eac_dbuf.linkname;
				if (Aflag && *linkp == '/')
					linkp++;

				if (Cflag) {
					docompr(linkp, 0, 0, namep);
					xcnt++;
					continue;
				}

				if (rmdir(namep) < 0) {
					if (errno == ENOTDIR)
						(void) unlink(namep);
				}
				if (link(linkp, namep) < 0) {
					(void) pfmt(stderr, MM_ERROR, badlink, 
						namep, strerror(errno));
					continue;
				}
				if (vflag)
					(void) pfmt(vfile, MM_NOSTD, xlinkto,
						namep, linkp);
				xcnt++;		/* increment # files extracted */
				continue;
			}

			if (Cflag) 
				(void) unlink(namep);
			if ((ofile = creat(namep, stbuf.st_mode & MODEMASK)) < 0) {
				(void) pfmt(stderr, MM_ERROR, ":174:Cannot create %s: %s\n",
					namep, strerror(errno));
				passtape();
				continue;
			}
#ifdef CHKSUM
			zflag++;
#endif
			if (extno != 0)	{	/* file is in pieces */
				xsfile(ofile);	/* extract it */
				extno = 0;
				goto eac_filedone;
			}
			extno = 0;	/* let everyone know file is not split */
			blocks = TBLOCKS(bytes = stbuf.st_size);
			if (vflag) {
				if (bytes == 1l)
					(void) pfmt(vfile, MM_NOSTD, onebyte, namep);
				else
					(void) pfmt(vfile, MM_NOSTD, manybytes, namep,
						bytes);
				if (NotTape)
					(void) pfmt(vfile, MM_NOSTD, luKn, K(blocks));
				else
					if (blocks == 1)
						(void) pfmt(vfile, MM_NOSTD, onetapeblock);
					else
						(void) pfmt(vfile, MM_NOSTD, 
							tapeblocks, blocks);
			}

			xblocks(bytes, ofile);
eac_filedone:
#ifdef CHKSUM
			zflag--;
#endif
			if (mflag == 0 ) {
				struct utimbuf timep;

				timep.actime = time((time_t *) 0);
				timep.modtime = stbuf.st_mtime;
				if (utime(namep, &timep) < 0)
					vperror(0, ":483:Cannot set time on %s", namep);
			}
			/* moved this code from above */
			if (pflag) {
				(void) chmod(namep, stbuf.st_mode & MODEMASK);
				(void) chown(namep, stbuf.st_uid, stbuf.st_gid);
			}

			if (fstat(ofile, &xtractbuf) == -1)
				(void) pfmt(stderr, MM_ERROR, 
					":488:Cannot access extracted file %s: %s\n",
					ofile, strerror(errno));
			(void) close(ofile);
			xcnt++;
			if (Cflag)
				docompr(namep, xtractbuf.st_mode, bytes, "");

		}
		if (Cflag)
			docompr("SENDaNULL", 0, 0, "");
		if (sflag) {
			getempty(1L);	/* don't forget extra EOT */
		}

		/*
	 	* Check if the number of files extracted is different from the
	 	* number of files listed on the command line 
	 	*/
		if (fcnt > xcnt ) {
			if ((fcnt-xcnt) == 1)
				(void) pfmt(stderr, MM_INFO, 
					":490:1 file not extracted\n", fcnt-xcnt);
			else
				(void) pfmt(stderr, MM_INFO, 
					":491:%d files not extracted\n", fcnt-xcnt);
			Errflg = 1;
		}
	/* End Enhanced Application Compatibility Support */
	} else {
		for (;;) {
			symflag = 0;
			linkp = (char *) NULL;
			dir = 0;
			xret = getname();
			if (xret == 2)
				continue;
			else if (xret == 1)
				goto gotit;

			initarg(argv, Filefile);
			if (endtape())
				break;
			if (Aflag)
				while (*namep == '/')  /* step past leading slashes */
					namep++;
			if ((curarg = nextarg()) == NULL)
				goto gotit;

			for ( ; curarg != NULL; curarg = nextarg())
				if (prefix(curarg, namep))
					goto gotit;
			passtape();
			continue;

gotit:
			if (checkw('x', namep) == 0) {
				passtape();
				continue;
			}

			if (once) {
				once = 0;
				if (geteuid() == (uid_t) 0) {
					pflag = 1;
					if (!strncmp(dblock.dbuf.magic, "ustar", 5))
						checkflag = 1;
					else
						checkflag = 2;
				}

			}

			type = dblock.dbuf.typeflag;

			(void) strcpy(&dirname[0], namep);
			if (checkdir(&dirname[0]) || (type == DIRTYPE)) {
				dir = 1;
				if (vflag) {
					(void) pfmt(vfile, MM_NOSTD, 
						":477:x %s, 0 bytes, ", &dirname[0]);
					if (NotTape)
						(void) pfmt(vfile, MM_NOSTD, 
							":478:0K\n");
					else
						(void) pfmt(vfile, MM_NOSTD, 
							":479:0 tape blocks\n");
				}
			}

			/* Must be privileged for char or blk special. */
			if ((Uid != 0) && (type == CHRTYPE || type == BLKTYPE)) {
				(void) pfmt(stderr, MM_ERROR, badspecial, namep,
					strerror(EACCES));
				continue;
			}

			if (type == BLKTYPE || type == CHRTYPE || type == FIFOTYPE || type == NAMTYPE) {
				if(rmdir(namep) < 0) {
					if (errno == ENOTDIR)
						(void) unlink(namep);
				}
				linkp = temp_str(dblock.dbuf.linkname);
				if (*linkp != NULL) {
					if (Aflag && *linkp == '/')
						linkp++;
					if (link(linkp, namep) < 0) {
						(void) pfmt(stderr, MM_ERROR, badlink, 
							namep, strerror(errno));
						continue;
					}
					if (vflag)
						(void) pfmt(vfile, MM_NOSTD, xlinkto,
								namep, linkp);
					xcnt++;		/* increment # files extracted */
					continue;
				}

				/* Set arg values for mknod() call. */
				switch(type) {
					case BLKTYPE:
						mode = (mode_t)(Gen.g_mode|S_IFBLK);
						dev = (dev_t)makedev(Gen.g_devmajor, Gen.g_devminor);
						break;
					case CHRTYPE:
						mode = (mode_t)(Gen.g_mode|S_IFCHR);
						dev = (dev_t)makedev(Gen.g_devmajor, Gen.g_devminor);
						break;
					case FIFOTYPE:
						mode = (mode_t)(Gen.g_mode|S_IFIFO);
						/* dev ignored for fifo */
						dev = (dev_t)0;
						break;
					case NAMTYPE:
						mode = (mode_t)(Gen.g_mode|S_IFNAM);
						dev = (dev_t)Gen.g_devminor;
						break;
				}

				if (mknod(namep, mode, dev) < 0) {
					vperror(0, badmknod, namep);
					continue;
				}
				blocks = TBLOCKS(bytes = stbuf.st_size);
				if (vflag) {
					if (bytes == 1l)
						(void) pfmt(vfile, MM_NOSTD, onebyte,
							namep);
					else
						(void) pfmt(vfile, MM_NOSTD, 
							manybytes, namep, bytes);
					if (NotTape)
						(void) pfmt(vfile, MM_NOSTD, luKn,
							K(blocks));
					else
						if (blocks == 1)
							(void) pfmt(vfile, MM_NOSTD, 
								onetapeblock);
						else
							(void) pfmt(vfile, MM_NOSTD, 
								tapeblocks, blocks);
				}
				goto filedone;
			}

			if (type == SYMTYPE) {
				linkp = temp_str(dblock.dbuf.linkname);
				if (Aflag && *linkp == '/')
					linkp++;
				if (rmdir(namep) < 0) {
					if (errno == ENOTDIR)
						(void) unlink(namep);
				}
				if (symlink(linkp, namep)<0) {
					vperror(0, ":480:%s: symbolic link failed", namep);
					continue;
				}
				if (vflag)
					(void) pfmt(vfile, MM_NOSTD, 
						":481:x %s symbolic link to %s\n",
						dblock.dbuf.name, linkp);
				symflag = 1;
				goto filedone;
			}
			if (type == LNKTYPE) {
				linkp = temp_str(dblock.dbuf.linkname);
				if (Aflag && *linkp == '/')
					linkp++;
				if (rmdir(namep) < 0) {
					if (errno == ENOTDIR)
						(void) unlink(namep);
				}
				if (link(linkp, namep) < 0) {
					(void) pfmt(stderr, MM_ERROR, badlink, namep,
						strerror(errno));
					continue;
				}
				if (vflag)
					(void) pfmt(vfile, MM_NOSTD, xlinkto, namep,
						linkp);
				xcnt++;		/* increment # files extracted */
				continue;
			}

			if (type == REGTYPE || type == AREGTYPE) {
				if (rmdir(namep) < 0) {
					/* test writeability to prevent non-writeable files
						from being unlinked: */
					if (errno == ENOTDIR && stat(namep, &xtractbuf) == 0 &&
							(xtractbuf.st_mode & (S_IWUSR | S_IWGRP)) )
						(void) unlink(namep);
				}
				linkp = temp_str(dblock.dbuf.linkname);
				if (*linkp != NULL) {
					if (Aflag && *linkp == '/')
						linkp++;
					if (link(linkp, namep) < 0) {
						(void) pfmt(stderr, MM_ERROR, badlink, 
							namep, strerror(errno));
						continue;
					}
					if (vflag)
						(void) pfmt(vfile, MM_NOSTD, xlinkto,
							namep, linkp);
					xcnt++;		/* increment # files extracted */
					continue;
				}

				if ((ofile = creat(namep, stbuf.st_mode & MODEMASK)) < 0) {
					(void) pfmt(stderr, MM_ERROR, ":174:Cannot create %s: %s\n",
						namep, strerror(errno));
					passtape();
					continue;
				}
	
				if (extno != 0)	{	/* file is in pieces */
					xsfile(ofile);	/* extract it */
					extno = 0;
					goto filedone;
				}
				extno = 0;	/* let everyone know file is not split */
				blocks = TBLOCKS(bytes = stbuf.st_size);
				if (vflag) {
					if (bytes == 1l)
						(void) pfmt(vfile, MM_NOSTD, onebyte, namep);
					else
						(void) pfmt(vfile, MM_NOSTD, manybytes, namep,
							bytes);
					if (NotTape)
						(void) pfmt(vfile, MM_NOSTD, luKn, K(blocks));
					else
						if (blocks == 1)
							(void) pfmt(vfile, MM_NOSTD, onetapeblock);
						else
							(void) pfmt(vfile, MM_NOSTD, 
								tapeblocks, blocks);
				}
	
				xblocks(bytes, ofile);

			}  /* if (type == REGTYPE || type == AREGTYPE) */
filedone:
			if (mflag == 0 && !symflag) {
				struct utimbuf timep;

				timep.actime = time((time_t *) 0);
				timep.modtime = stbuf.st_mtime;
				if (utime(namep, &timep) < 0)
					vperror(0, ":483:Cannot set time on %s", namep);
			}
			/* moved this code from above */
			if (pflag && !symflag) 
				(void) chmod(namep, stbuf.st_mode & MODEMASK);
			if (!oflag)
				if (checkflag == 1) {
					if ((dpasswd = getpwnam(&dblock.dbuf.uname[0])) == (struct passwd *)NULL) {
						(void) pfmt(stderr, MM_WARNING,
							badpasswdinfo, &dblock.dbuf.uname[0]);
						(void) pfmt(stderr, MM_WARNING, 
							":485:%s: owner not changed\n", namep);
					} else
					if ((dgroup = getgrnam(&dblock.dbuf.gname[0])) == (struct group *)NULL) {
						(void) pfmt(stderr, MM_WARNING, 
							badgrpinfo, &dblock.dbuf.gname[0]);
						(void) pfmt(stderr, MM_WARNING, 
							":487:%s: group not changed\n", namep);
					}
					if (symflag)
						(void) lchown(namep,
							(dpasswd ?
							 dpasswd->pw_uid : -1),
							(dgroup ?
							 dgroup->gr_gid:-1));
					else
						(void) chown(namep, 
							(dpasswd ?
							 dpasswd->pw_uid : -1),
							(dgroup ?
							 dgroup->gr_gid:-1));
				} else if (checkflag == 2)
					if (symflag)
						(void) lchown(namep, stbuf.st_uid, stbuf.st_gid);
					else
						(void) chown(namep, stbuf.st_uid, stbuf.st_gid);

			if (!dir && (type == REGTYPE || type == AREGTYPE || type == LNKTYPE))
				(void) close(ofile);
	
			xcnt++;			/* increment # files extracted */
		}

		if (sflag) {
			getempty(1L);	/* don't forget extra EOT */
		}

		/*
		 * Check if the number of files extracted is different from the
		 * number of files listed on the command line 
		 */
		if (fcnt > xcnt ) {
			if ((fcnt-xcnt) == 1)
				(void) pfmt(stderr, MM_INFO, 
					":490:1 file not extracted\n", fcnt-xcnt);
			else
				(void) pfmt(stderr, MM_INFO, 
					":491:%d files not extracted\n", fcnt-xcnt);
			Errflg = 1;
		}
	} /* if (Cflag)  */
}

/***	xblocks		extract file/extent from tape to output file	
 *
 *	xblocks(bytes, ofile);
 *		long bytes;	size of extent or file to be extracted
 *
 *	called by doxtract() and xsfile()
 */
static	void
xblocks(bytes, ofile)
long bytes;
int ofile;
{
	long blocks;
	char buf[TBLOCK];

	blocks = TBLOCKS(bytes);
	while (blocks-- > 0) {
		readtape(buf);
		if (bytes > TBLOCK) {
			if (write(ofile, buf, TBLOCK) < 0) {
exwrterr:
				/* Enhanced Application Compatibility Support */
				if (Cflag)
					namep = eac_dblock.eac_dbuf.name;
				/* End Enhanced Application Compatibility Support */
				else
					namep = dblock.dbuf.name;

				(void) pfmt(stderr, MM_ERROR, 
					":492:%s: HELP - extract write error: %s\n",
					namep, strerror(errno));
				done(2);
			}
		} else
			if (write(ofile, buf, (int) bytes) < 0)
				goto exwrterr;
		bytes -= TBLOCK;
	}
}



/***	xsfile	extract split file
 *
 *	xsfile(ofd);	ofd = output file descriptor
 *
 *	file extracted and put in ofd via xblocks()
 *
 *	NOTE:  only called by doxtract() to extract one large file
 */

static	union	hblock	savedblock;	/* to ensure same file across volumes */

static	void
xsfile(ofd)
int ofd;
{
	register i, c;
	char name[NAMSIZ];	/* holds name for diagnostics */
	int extents, totalext;
	long bytes, totalbytes;

	(void) strncpy(name, dblock.dbuf.name, NAMSIZ); /* so we don't lose it */
	totalbytes = 0L;	/* in case we read in half the file */
	totalext = 0;		/* these keep count */

	(void) pfmt(stderr, MM_INFO, ":493:%s split across %d volumes\n", 
		name, extotal);

	/* Make sure we do extractions in order. */
	if (extno != 1) {  /* starting in middle of file? */
		(void) pfmt(vfile, MM_INFO, 
			":759:First volume read is not #1\n");
		(void) pfmt(vfile, MM_ACTION,
			":966:OK to read file beginning with volume #%d",
			extno);
		if (response(vfile, (char *)0, (struct resp *)0) != 'y') {
canit:
			passtape();
			(void) close(ofd);
			return;
		}
	}
	extents = extotal;
	for (i = extno; ; ) {
		bytes = stbuf.st_size;
		if (vflag)
			(void) pfmt(vfile, MM_NOSTD, 
				":761:+++ x %s [volume #%d], %lu bytes, %luK\n",
				name, extno, bytes, K(TBLOCKS(bytes)));
		xblocks(bytes, ofd);

		totalbytes += bytes;
		totalext++;
		if (++i > extents)
			break;

		/* get next volume and verify it's the right one */
		copy((char *)&savedblock, (char *)&dblock);
tryagain:
		newvol();
		getdir();
		if (endtape()) {	/* seemingly empty volume */
			(void) pfmt(stderr, MM_WARNING, 
				":497:First record is null\n");
asknicely:
			(void) pfmt(stderr, MM_WARNING, 
				":762:Need volume #%d of %s\n", i, name);
			goto tryagain;
		}
		if (notsame()) {
			(void) pfmt(stderr, MM_WARNING, 
				":499:First file on that volume is not the same file\n");
			goto asknicely;
		}
		if (i != extno) {
			static struct resp list[3];

			(void) pfmt(stderr, MM_WARNING, 
				":763:Volume #%d received out of order\nshould be #%d\n",
				extno, i);
			if (list[0].pat == 0) {
				list[0].pat = gettxt(":967", "^(a|A)");
				list[0].ret = 'a';
				list[1].pat = gettxt(":968", "^(i|I)");
				list[1].ret = 'i';
			}
			(void) pfmt(stderr, MM_ACTION,
				":969:Ignore error, Abort this file, or load New volume");
			c = response(stderr, ":970: (i/a/n)? ", list);
			if (c == 'a')
				goto canit;
			if (c != 'i')		/* default to new volume */
				goto asknicely;
			i = extno;		/* okay, start from there */
		}
	}
	bytes = stbuf.st_size;
	if (vflag) {
		if (totalext == 1)
			(void) pfmt (vfile, MM_NOSTD,
				":764:x %s (in 1 volume), ", name);
		else
			(void) pfmt (vfile, MM_NOSTD,
				":765:x %s (in %d volumes), ", name, totalext);
		if (totalbytes == 1l)
			(void) pfmt (vfile, MM_NOSTD, ":504:1 byte, %luK\n",
				K(TBLOCKS(totalbytes)));
		else
			(void) pfmt (vfile, MM_NOSTD, ":505:%lu bytes, %luK\n",
				totalbytes, K(TBLOCKS(totalbytes)));
	}
}



/***	notsame()	check if extract file extent is invalid
 *
 *	returns true if anything differs between savedblock and dblock
 *	except extno (extent number), checksum, or size (extent size).
 *	Determines if this header belongs to the same file as the one we're
 *	extracting.
 *
 *	NOTE:	though rather bulky, it is only called once per file
 *		extension, and it can withstand changes in the definition
 *		of the header structure.
 *
 *	WARNING:	this routine is local to xsfile() above
 */
static	int
notsame()
{
	return(
	    strncmp(savedblock.dbuf.name, dblock.dbuf.name, NAMSIZ)
	    || strcmp(savedblock.dbuf.mode, dblock.dbuf.mode)
	    || strcmp(savedblock.dbuf.uid, dblock.dbuf.uid)
	    || strcmp(savedblock.dbuf.gid, dblock.dbuf.gid)
	    || strcmp(savedblock.dbuf.mtime, dblock.dbuf.mtime)
	    || savedblock.dbuf.typeflag != dblock.dbuf.typeflag
	    || strncmp(savedblock.dbuf.linkname, dblock.dbuf.linkname, NAMSIZ)
	    || savedblock.dbuf.extotal != dblock.dbuf.extotal
	    || strcmp(savedblock.dbuf.efsize, dblock.dbuf.efsize)
	);
}
static
int
getname()
{
	static	char	strbuf[NAMSIZ+1];

	char *preptr;
	int k = 0, j;

	for (j = 0; j < MAXNAM; j++)
		fullname[j] = '\0';

	getdir();

	if (Cflag)
		namep = eac_dblock.eac_dbuf.name;
	else {
		preptr = dblock.dbuf.prefix;
		if (*preptr != (char) NULL) {
			k = strlen(&dblock.dbuf.prefix[0]);
			if (k < PRESIZ) {
				(void) strcpy(&fullname[0], dblock.dbuf.prefix);
				j = 0;
				if (fullname[k-1] != '/' && dblock.dbuf.name[0] != '/')
					fullname[k++] = '/';
				while ((j < NAMSIZ) && (dblock.dbuf.name[j] != (char) NULL)) {
					fullname[k] = dblock.dbuf.name[j];
					k++; j++;
				} 

			} else {  /* k >= PRESIZ */
				k = 0;
				while ((k < PRESIZ) && (dblock.dbuf.prefix[k] != (char) NULL)) {
					fullname[k] = dblock.dbuf.prefix[k];
					k++;
				}
				if (fullname[k-1] != '/' && dblock.dbuf.name[0] != '/')
					fullname[k++] = '/';
				j = 0;
				while ((j < NAMSIZ) && (dblock.dbuf.name[j] != (char) NULL)) {
					fullname[k] = dblock.dbuf.name[j];
					k++; j++;
				} 
			}
			namep = &fullname[0];

		} else  /* *preptr == NULL */
			namep = strcpy(strbuf, temp_str(dblock.dbuf.name));

	}

	if (Xflag) {
		if (is_in_table(exclude_tbl, namep)) {
			if (vflag)
				(void) pfmt(stderr, MM_NOSTD, ":418:%s excluded\n", namep);
			passtape();
			return(2);
		}
	}
	if (Iflag) {
		if (is_in_table(include_tbl, namep))
			return(1);
	}
	return(0);
}


static	void
dotable(argv)
char	*argv[];
{
	char *curarg;
	int tcnt;			/* count # files tabled*/
	int fcnt;			/* count # files in argv list */
	int xret = 0;

	dumping = 0;

	/* if not on magtape, maximize seek speed */
	if (NotTape && !bflag)
		nblock = 1;

	/*
	 * Count the number of files that are to be tabled
	 */
	fcnt = tcnt = 0;
	initarg(argv, Filefile);
	while (nextarg() != NULL)
		++fcnt;

	for (;;) {
		xret = getname();
		if (xret == 2)
			continue;
		else if (xret == 1)
			goto tableit;

		initarg(argv, Filefile);
		if (endtape())
			break;
		if ((curarg = nextarg()) == NULL)
			goto tableit;
		for ( ; curarg != NULL; curarg = nextarg())
			if (prefix(curarg, namep))
				goto tableit;
		passtape();
		continue;
tableit:
		++tcnt;
		if (vflag)
			longt(&stbuf);
		(void) fprintf(vfile, "%s", namep);
		if (extno != 0) {
			if (vflag)
				(void) pfmt(vfile, MM_NOSTD, 
					":770:\n [volume #%d of %d] %lu bytes total",
					extno, extotal, efsize);
			else
				(void) pfmt(vfile, MM_NOSTD, 
					":771: [volume #%d of %d]", extno, 
					extotal);
		}
		if (dblock.dbuf.typeflag == LNKTYPE)
			(void) pfmt(vfile, MM_NOSTD, ":508: linked to %s", 
				dblock.dbuf.linkname);
		if (dblock.dbuf.typeflag == SYMTYPE)
			(void) pfmt(vfile, MM_NOSTD, 
				":509: symbolic link to %s", dblock.dbuf.
				linkname);
		(void) fprintf(vfile, "\n");
		passtape();
	}
	if (sflag) {
		getempty(1L);	/* don't forget extra EOT */
	}
	/*
	 * Check if the number of files tabled is different from the
	 * number of files listed on the command line
	 */
	if (fcnt > tcnt ) {
		if ((fcnt-tcnt) == 1)
			(void) pfmt(stderr, MM_WARNING, 
				":510:1 file not found\n");
		else
			(void) pfmt(stderr, MM_WARNING, 
				":511:%d files not found\n", fcnt-tcnt);
		Errflg = 1;
	}
}

static	void
putempty(n)
register long n;		/* new argument 'n' */
{
	char buf[TBLOCK];
	register char *cp;

	for (cp = buf; cp < &buf[TBLOCK]; )
		*cp++ = '\0';
	while (n-- > 0)
		writetape(buf);
	return;
}

static	void
getempty(n)
register long n;
{
	char buf[TBLOCK];
	register char *cp;

	if (!sflag)
		return;
	for (cp = buf; cp < &buf[TBLOCK]; )
		*cp++ = '\0';
	while (n-- > 0)
		sumupd(&Si, buf, TBLOCK);
	return;
}

static	void
verbose(st)
struct stat *st;
{
	register int i, j, temp;
	mode_t mode;
	char modestr[11];

	for (i = 0; i < 10; i++)
		modestr[i] = '-';
	modestr[i] = '\0';

	mode = st->st_mode;
	for (i = 0; i < 3; i++) {
		temp = (mode >> (6 - (i * 3)));
		j = (i * 3) + 1;
		if (S_IROTH & temp)
			modestr[j] = 'r';
		if (S_IWOTH & temp)
			modestr[j + 1] = 'w';
		if (S_IXOTH & temp)
			modestr[j + 2] = 'x';
	}
	temp = st->st_mode & Ftype;
	switch (temp) {
		case (S_IFIFO):
			modestr[0] = 'p';
			break;
		case (S_IFCHR):
			modestr[0] = 'c';
			break;
		case (S_IFDIR):
			modestr[0] = 'd';
			break;
		case (S_IFBLK):
			modestr[0] = 'b';
			break;
		case (S_IFNAM):
			if (Gen.g_devminor == S_INSEM)  /* Xenix semaphore */
				modestr[0] = 's';
			else if (Gen.g_devminor == S_INSHD)  /* Xenix shared data */
				modestr[0] = 'm';
			else
				(void) pfmt(stderr, MM_ERROR, 
					":512:Impossible file type\n");
			break;
		case (S_IFREG): /* was initialized to '-' */
			break;
		case (S_IFLNK):
			modestr[0] = 'l';
			break;
		default:
			(void) pfmt(stderr, MM_ERROR, 
				":512:Impossible file type\n");
	}
	if ((S_ISUID & Gen.g_mode) == S_ISUID)
		modestr[3] = 's';
	if ((S_ISVTX & Gen.g_mode) == S_ISVTX)
		modestr[9] = 't';
	if ((S_ISGID & Gen.g_mode) == S_ISGID && modestr[6] == 'x')
		modestr[6] = 's';
	else if ((S_ENFMT & Gen.g_mode) == S_ENFMT && modestr[6] != 'x')
		modestr[6] = 'l';
	(void)fprintf(vfile, "%s", modestr);
}

static	void
longt(st)
register struct stat *st;
{
	char time_buf[50];

	verbose(st);
	(void) fprintf(vfile, "%3d/%-3d", st->st_uid, st->st_gid);
	if (dblock.dbuf.typeflag == SYMTYPE)	
		st->st_size = strlen(temp_str(dblock.dbuf.linkname));
	(void) fprintf(vfile, "%7lu", st->st_size);
	cftime(time_buf, gettxt(":348", "%b %e %H:%M %Y"), &st->st_mtime);
	(void) fprintf(vfile, " %s ", time_buf);
}

/*
 * Make all directories needed by `name'.  If `name' is itself
 * a directory on the tar tape (indicated by a trailing '/'),
 * return 1; else 0.
 */
static	int
checkdir(name)
	register char *name;
{
	register char *cp;

	/* Enhanced Application Compatibility Support */
	if (Cflag) {
		if (*(cp = name) == '/')
			cp++;
		for(; *cp; cp ++) {
			if (*cp =='/') {
				*cp = '\0';
				if (access(name, F_OK) < 0) {
					if (mkdir(name, 0777) < 0) {
						vperror(0, badmkdir, name);
						return(0);
					} else if (pflag)
							chown(name, stbuf.st_uid, stbuf.st_gid);
				}
				*cp = '/';

			}
		}
		return(0);
	}
	/* End Enhanced Application Compatibility Support */

	if (dblock.dbuf.typeflag != DIRTYPE) {
		/*
	 	* Quick check for existence of directory.
	 	*/
		if ((cp = strrchr(name, '/')) == 0)
			return (0);
		*cp = '\0';

	} else if ((cp = strrchr(name, '/')) == 0) {
		if (access(name, 0) < 0) {
			if(mkdir(name, 0777) < 0) {
				vperror(0, badmkdir, name);
				return(0);
			}
			return(1);
		}
		return(0);
	}

	if (access(name, 0) == 0) {	/* already exists */
		*cp = '/';
		return (cp[1] == '\0');	/* return (lastchar == '/') */
	}


	/*
	 * No luck, try to make all directories in path.
	 */
	cp = name;
	if (*cp == '/')
		cp++;
	for ( ; *cp; cp++) {
		if (*cp != '/')
			continue;
		*cp = '\0';
		if (access(name, 0) < 0) {
			if (mkdir(name, 0777) < 0) {
				vperror(0, badmkdir, name);
				*cp = '/';
				return (0);
			}
		}
		*cp = '/';
	}

	if (access(name, 0) < 0) {
		if (mkdir(name, 0777) < 0) {
			vperror(0, badmkdir, name);
			return (0);
		}
		return(0);
	}

	return (cp[-1]=='/');
}

static
void
onintr()
{
	(void) signal(SIGINT, SIG_IGN);
	term++;
}

static
void
onquit()
{
	(void) signal(SIGQUIT, SIG_IGN);
	term++;
}

static
void
onhup()
{
	(void) signal(SIGHUP, SIG_IGN);
	term++;
}

/*	uncomment if you need it
static
void
onterm()
{
	signal(SIGTERM, SIG_IGN);
	term++;
}
*/

static	void
tomodes(sp)
register struct stat *sp;
{
	register char *cp;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	(void) sprintf(dblock.dbuf.mode, "%07o", sp->st_mode & MODEMASK);
	(void) sprintf(dblock.dbuf.uid, "%07o", sp->st_uid);
	(void) sprintf(dblock.dbuf.gid, "%07o", sp->st_gid);
	(void) sprintf(dblock.dbuf.size, "%011lo", sp->st_size);
	(void) sprintf(dblock.dbuf.mtime, "%011lo", sp->st_mtime);
}

static	int
checksum()
{
	register i;
	register unsigned char *cp;

	/* Enhanced Application Compatibility Support */
	if (Cflag) {
		for (cp = (unsigned char *)eac_dblock.eac_dbuf.chksum;
		     cp < (unsigned char *)&eac_dblock.eac_dbuf.chksum[
			sizeof(eac_dblock.eac_dbuf.chksum)];
		     cp++)
			*cp = ' ';
		i = 0;
		for (cp = (unsigned char *)eac_dblock.eac_dummy; 
		     cp < (unsigned char *)&eac_dblock.eac_dummy[TBLOCK];
		     cp++)
			i += *cp;
		return(i);
	/* End Enhanced Application Compatibility Support */
	} else {
		for (cp = (unsigned char *)dblock.dbuf.chksum;
		     cp < (unsigned char *)&dblock.dbuf.chksum[
			sizeof(dblock.dbuf.chksum)];
		     cp++)
			*cp = ' ';
		i = 0;
		for (cp = (unsigned char *)dblock.dummy;
		     cp < (unsigned char *)&dblock.dummy[TBLOCK];
		     cp++)
			i += *cp;
		return(i);
	}
}

static	int
checksum_bug()
{
	register i;
	register char *cp;

	/* Enhanced Application Compatibility Support */
	if (Cflag) {
		for (cp = eac_dblock.eac_dbuf.chksum; cp < &eac_dblock.eac_dbuf.chksum[sizeof(eac_dblock.eac_dbuf.chksum)]; cp++)
			*cp = ' ';
		i = 0;
		for (cp = eac_dblock.eac_dummy; cp < &eac_dblock.eac_dummy[TBLOCK]; cp++)
			i += *cp;
		return(i);
	/* End Enhanced Application Compatibility Support */
	} else {
		for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
			*cp = ' ';
		i = 0;
		for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
			i += *cp;
		return(i);
	}
}

static	int
checkw(c, name)
char c;
char *name;
{
	if (wflag) {
		(void) fprintf(vfile, "%c ", c);
		if (vflag)
			longt(&stbuf);
		(void) pfmt(vfile, MM_NOSTD, ":63:%s: ", name);
		if (response(vfile, (char *)0, (struct resp *)0) == 'y'){
			return(1);
		}
		return(0);
	}
	return(1);
}

static	int
response(fp, str, list)
FILE *fp;
char *str;
struct resp *list;
{
	static char *ystr, *nstr;
	static struct resp yorn[2];
	char ans[128];
	regex_t re;
	int ch;

	if (str != 0)
		pfmt(fp, MM_NOSTD, str);
	else /* use "yes"/"no" */
	{
		if (ystr == 0) /* first time */
		{
			ystr = nl_langinfo(YESSTR);
			nstr = nl_langinfo(NOSTR);
		}
		pfmt(fp, MM_NOSTD, ":971: (%s/%s)? ", ystr, nstr);
	}

	errno=0;
	if (fgets(ans, 128, stdin) == 0) {
		if (errno != 0)
			vperror(1, ":465:Read error on %s", "stdin");
		else {
			/*
			 * If fgets() returned null, but errno is not
			 * set, this just means EOF was encountered and
			 * no characters were read (e.g. user did <ctl-d>
			 * at yes/no prompt).  We'll exit gracefully here.
			 */
			printf("\n");
			exit(0);
		}
	}

	if (list == 0) /* use default patterns */
	{
		if (yorn[0].pat == 0) /* first time */
		{
			yorn[0].pat = nl_langinfo(YESEXPR);
			yorn[0].ret = 'y';
		}
		list = yorn;
	}
	while (list->pat != 0)
	{
		if ((ch = regcomp(&re, list->pat, REG_EXTENDED | REG_NOSUB))
			!= 0)
		{
		badre:;
			regerror(ch, &re, ans, 128);
			pfmt(stderr, MM_ERROR,
				"uxcore.abi:1234:RE failure: %s\n", ans);
			exit(2);
		}
		ch = regexec(&re, ans, (size_t)0, (regmatch_t *)0, 0);
		regfree(&re);

		if (ch == 0)  /* match */
			break;
		else if (ch == REG_NOMATCH) {  /* no match yet */
			/*
			 * There may possibly be other valid
			 * responses to match against the user's
			 * answer.  (See the xsfile() routine.)
			 */
			list++;
			continue;
		}
		else  /* bad regular expression */
			goto badre;
	}
	return list->ret;
}

static daddr_t	lookup();

static	int
checkupdate(arg)
char	*arg;
{
	char name[MAXNAM];	/* was 100; parameterized */
	long	mtime;
	daddr_t seekp;

	rewind(tfile);
	/*for (;;) {*/
		if ((seekp = lookup(arg)) < 0)
			return(1);
		(void) fseek(tfile, seekp, 0);
		(void) fscanf(tfile, "%s %lo", name, &mtime);
		return(stbuf.st_mtime > mtime);
/* NOTREACHED */
	/*}*/
/* NOTREACHED */
}

/***	newvol	get new floppy (or tape) volume
 *
 *	newvol();		resets tapepos and first to TRUE, prompts for
 *				for new volume, and waits.
 *	if dumping, end-of-file is written onto the tape.
 */

static	void
newvol()
{
	register int c;

	if (dumping) {
		DEBUG("newvol called with 'dumping' set\n", 0, 0);
		closevol();
		sync();
		tapepos = 0;
	} else
		first = TRUE;
	(void) close(mt);
	if (sflag) {
		sumepi(&Si);
		sumout(Sumfp, &Si);
		(void) fprintf(Sumfp, "\n");

		sumpro(&Si);
	}
	(void) pfmt(stderr, MM_INFO, ":513:\007Needs new volume:\n");
	(void) pfmt(stderr, MM_ACTION, 
		":514:Please insert new volume, then press RETURN.");
	(void) fseek(stdin, 0L, 2);	/* scan over read-ahead */
	while ((c = getchar()) != '\n' && ! term)
		if (c == EOF)
			done(0);
	if (term)
		done(0);

	mt = strcmp(usefile, "-") == EQUAL  ?  dup(1) : open(usefile, dumping ? update : 0);
	if (mt == -1) {
		if (dumping)
			pfmt(stderr, MM_ERROR,
				":515:Cannot reopen output file %s: %s\n",
				usefile, strerror(errno));
		else
			pfmt(stderr, MM_ERROR,
				":516:Cannot reopen input file %s: %s\n",
				usefile, strerror(errno));

		done(2);
	}
}

/*
 * Write a trailer portion to close out the current output volume.
 */

static	void
closevol()
{
	putempty(2L);	/* 2 EOT marks */
	if (mulvol && Sflag) {
		/*
		 * blocklim does not count the 2 EOT marks;
		 * tapepos  does     count the 2 EOT marks;
		 * therefore we need the +2 below.
		 */
		putempty(blocklim + 2L - tapepos);
	}
	flushtape();
}

static	void
done(n)
{
	(void) unlink(tname);
	exit(n);
}

static	int
prefix(s1, s2)
register char *s1, *s2;
{
	while (*s1)
		if (*s1++ != *s2++)
			return(0);
	if (*s2)
		return(*s2 == '/');
	return(1);
}

#define	N	200
static	int	njab;
static	daddr_t
lookup(s)
char *s;
{
	register i;
	daddr_t a;

	for(i=0; s[i]; i++)
		if(s[i] == ' ')
			break;
	a = bsrch(s, i, low, high);
	return(a);
}

static	daddr_t
bsrch(s, n, l, h)
daddr_t l, h;
char *s;
{
	register i, j;
	char b[N];
	daddr_t m, m1;

	njab = 0;

loop:
	if(l >= h)
		return(-1L);
	m = l + (h-l)/2 - N/2;
	if(m < l)
		m = l;
	(void) fseek(tfile, m, 0);
	(void) fread(b, 1, N, tfile);
	njab++;
	for(i=0; i<N; i++) {
		if(b[i] == '\n')
			break;
		m++;
	}
	if(m >= h)
		return(-1L);
	m1 = m;
	j = i;
	for(i++; i<N; i++) {
		m1++;
		if(b[i] == '\n')
			break;
	}
	i = cmp(b+j, s, n);
	if(i < 0) {
		h = m;
		goto loop;
	}
	if(i > 0) {
		l = m1;
		goto loop;
	}
	return(m);
}

static	int
cmp(b, s, n)
  register char *b, *s;
{
	register i;

	if(b[0] != '\n')
		exit(2);
	for(i=0; i<n; i++) {
		if(b[i+1] > s[i])
			return(-1);
		if(b[i+1] < s[i])
			return(1);
	}
	return(b[i+1] == ' '? 0 : -1);
}

/***	seekdisk	seek to next file on archive
 *
 *	called by passtape() only
 *
 *	WARNING: expects "nblock" to be set, that is, readtape() to have
 *		already been called.  Since passtape() is only called
 *		after a file header block has been read (why else would
 *		we skip to next file?), this is currently safe.
 */
static	void
seekdisk(blocks)
long blocks;
{
	long seekval;

	tapepos += blocks;
	DEBUG("seekdisk(%lu) called\n", blocks, 0);
	if (recno + blocks <= rblock) {
		recno += blocks;
		return;
	}
	if (recno > rblock)
		recno = rblock;
	seekval = blocks - (rblock - recno);
	recno = rblock;	/* so readtape() reads next time through */
	if (lseek(mt, (long) TBLOCK * seekval, 1) == -1L) {
		(void) pfmt(stderr, MM_ERROR,
			":520:Device seek error: %s\n", strerror(errno));
		done(3);
	}
}

static	void
readtape(buffer)
char *buffer;
{
	register int i, j;

	++tapepos;
	if (recno >= rblock || first) {
		if (first) {
			/*
			 * set the number of blocks to
			 * read initially.
			 * very confusing!
			 */
			if (bflag)
				j = nblock;
			else if (!NotTape)
				j = NBLOCK;
			else if (defaults_used)
				j = nblock;
			else
				j = NBLOCK;
		} else
			j = nblock;
		/* Enhanced Application Compatibility Support */
		if (Cflag) {
			if ((i = iter_read(mt, eac_tbuf, TBLOCK*j)) < 0) {
				(void) pfmt(stderr, MM_ERROR, 
					":522:Tape read error: %s\n", strerror(errno));
				done(3);
			}
		/* End Enhanced Application Compatibility Support */
		} else {
			if ((i = iter_read(mt, tbuf, TBLOCK*j)) < 0) {
				(void) pfmt(stderr, MM_ERROR, 
					":522:Tape read error: %s\n", strerror(errno));
				done(3);
			}
		}
		if (i == 0) {
			(void) pfmt(stderr, MM_ERROR, ":665:Unexpected EOF\n");
			done(3);
		}
		if ((i % TBLOCK) != 0) {
			(void) pfmt(stderr, MM_ERROR, 
				":523:Tape blocksize error\n");
			done(3);
		}
		i /= TBLOCK;
		if (first) {
			if (rflag && i != 1 && !NotTape) {
				(void) pfmt(stderr, MM_ERROR, 
					":524:Cannot update blocked tapes\n");
				done(4);
			}
			if (vflag && i != nblock && i != 1) {
				if (NotTape)
					(void) pfmt(stderr, MM_INFO, 
						":525:buffer size = %dK\n", 
						K(i));
				else
					(void) pfmt(stderr, MM_INFO, 
						":526:blocksize = %d\n", i);
			}
			nblock = i;
		}
		if (!first && i == 0) {
		    /* 
		     * tar expects more data, but no data was found -> error.
		     * ignore EOF the first time: tbuf is filled with zero's
		     * and they are recognized as end-of-archiv, so tar doesn't
		     * print an error, when reading an empty archiv.
		     */
			(void) pfmt(stderr, MM_ERROR, ":665:Unexpected EOF\n");
			done(3);
		}
		recno = 0;
		rblock = i;	/* # of blocks read */
	}

	first = FALSE;
	/* Enhanced Application Compatibility Support */
	if (Cflag)
		copy(buffer, &eac_tbuf[recno++]);
	/* End Enhanced Application Compatibility Support */
	else
		copy(buffer, (char *)&tbuf[recno++]);
	if (sflag)
		sumupd(&Si, buffer, TBLOCK);
	return;
}

static	void
writetape(buffer)
char *buffer;
{
	tapepos++;	/* output block count */

	first = FALSE;	/* removed setting of nblock if file arg given */
	if (recno >= nblock)
		flushtape();
	copy((char *)&tbuf[recno++], buffer);
	if (recno >= nblock)
		flushtape();
}



/***    backtape - reposition tape after reading soft "EOF" record
 *
 *      Backtape tries to reposition the tape back over the EOF
 *      record.  This is for the -u and -r options so that the
 *      tape can be extended.  This code is not well designed, but
 *      I'm confident that the only callers who care about the
 *      backspace-over-EOF feature are those involved in -u and -r.
 *      Thus, we don't handle it correctly when there is
 *      a block factor, but the -u and -r options refuse to work
 *      on block tapes, anyway.
 *
 *      Note that except for -u and -r, backtape is called as a
 *      (apparently) unwanted side effect of endtape().  Thus,
 *      we don't bitch when the seeks fail on raw devices because
 *      when not using -u and -r tar can be used on raw devices.
 */

static	void
backtape()
{
	DEBUG("backtape() called, recno=%d nblock=%d\n", recno, nblock);

	/*
	 * The first seek positions the tape to before the eof;
	 * this currently fails on raw devices.
	 * Thus, we ignore the return value from lseek().
	 * The second seek does the same.
	 */
	(void) lseek(mt, (long) -(TBLOCK*rblock), 1);	/* back one large tape block */
	recno--;				/* reposition over soft eof */
	tapepos--;				/* back up tape position */
	if (read(mt, tbuf, TBLOCK*rblock) <= 0) {
		(void) pfmt(stderr, MM_ERROR, 
			":527:Tape read error after seek: %s\n", strerror(errno));
		done(4);
	}
	(void) lseek(mt, (long) -(TBLOCK*rblock), 1);	/* back large block again */
}


/***    flushtape  write buffered block(s) onto tape
 *
 *      recno points to next free block in tbuf.  If nonzero, a write is done.
 *
 *      NOTE:   this is called by writetape() to do the actual writing
 */
static	void
flushtape()
{

	DEBUG("flushtape() called, recno=%d\n", recno, 0);
	if (recno > 0) {	/* anything buffered? */
		if (NotTape) {
			if (recno > nblock)
				recno = nblock;
		}
		DEBUG("writing out %d blocks of %d bytes\n",
			(NotTape ? recno : nblock),
			(NotTape ? recno : nblock) * TBLOCK);
	
		if (write(mt, tbuf, (NotTape ? recno : nblock) * TBLOCK) < 0) {
			(void) pfmt(stderr, MM_ERROR, 
				":528:Tape write error: %s\n", strerror(errno));
			done(2);
		}
		if (sflag)
			sumupd(&Si, tbuf, (NotTape ? recno : nblock) * TBLOCK);
		recno = 0;
	}
}

static	void
copy(to, from)
register char *to, *from;
{
	register i;

	i = TBLOCK;
	do {
		*to++ = *from++;
	} while (--i);
}

/***	initarg -- initialize things for nextarg.
 *
 *	argv		filename list, a la argv.
 *	filefile	name of file containing filenames.  Unless doing
 *		a create, seeks must be allowable (e.g. no named pipes).
 *
 *	- if filefile is non-NULL, it will be used first, and argv will
 *	be used when the data in filefile are exhausted.
 *	- otherwise argv will be used.
 */
static char **Cmdargv = NULL;
static FILE *FILEFile = NULL;
static long seekFile = -1;
static char *ptrtoFile, *begofFile, *endofFile; 

static	void
initarg(argv, filefile)
char *argv[];
char *filefile;
{
	struct stat statbuf;
	register char *p;
	int nbytes;

	Cmdargv = argv;
	if (filefile == NULL)
		return;		/* no -F file */
	if (FILEFile != NULL) {
		/*
		 * need to REinitialize
		 */
		if (seekFile != -1)
			(void) fseek(FILEFile, seekFile, 0);
		ptrtoFile = begofFile;
		return;
	}
	/*
	 * first time initialization 
	 */
	if ((FILEFile = fopen(filefile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, badopentwon, filefile,
			strerror(errno));
		done(1);
	}
	(void) fstat(fileno(FILEFile), &statbuf);
	if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
		(void) pfmt(stderr, MM_ERROR, 
			":529:%s is not a regular file\n", filefile);
		(void) fclose(FILEFile);
		done(1);
	}
	ptrtoFile = begofFile = endofFile;
	seekFile = 0;
	if (!xflag)
		return;		/* the file will be read only once anyway */
	nbytes = statbuf.st_size;
	while ((begofFile = malloc(nbytes)) == NULL)
		nbytes -= 20;
	if (nbytes < 50) {
		free(begofFile);
		begofFile = endofFile;
		return;		/* no room so just do plain reads */
	}
	if (fread(begofFile, 1, nbytes, FILEFile) != nbytes) {
		(void) pfmt(stderr, MM_ERROR, ":205:Cannot read %s: %s\n", 
			filefile, strerror(errno));
		done(1);
	}
	ptrtoFile = begofFile;
	endofFile = begofFile + nbytes;
	for (p = begofFile; p < endofFile; ++p)
		if (*p == '\n')
			*p = '\0';
	if (nbytes != statbuf.st_size)
		seekFile = nbytes + 1;
	else
		(void) fclose(FILEFile);
}

/***	nextarg -- get next argument of arglist.
 *
 *	The argument is taken from wherever is appropriate.
 *
 *	If the 'F file' option has been specified, the argument will be
 *	taken from the file, unless EOF has been reached.
 *	Otherwise the argument will be taken from argv.
 *
 *	WARNING:
 *	  Return value may point to static data, whose contents are over-
 *	  written on each call.
 */
static	char  *
nextarg()
{
	static char nameFile[LPNMAX];
	int n;
	char *p;

	if (FILEFile) {
		if (ptrtoFile < endofFile) {
			p = ptrtoFile;
			while (*ptrtoFile)
				++ptrtoFile;
			++ptrtoFile;
			return(p);
		}
		if (fgets(nameFile, LPNMAX, FILEFile) != NULL) {
			n = strlen(nameFile);
			if (n > 0 && nameFile[n-1] == '\n')
				nameFile[n-1] = '\0';
			return(nameFile);
		}
	}
	return(*Cmdargv++);
}

/*
 * kcheck()
 *	- checks the validity of size values for non-tape devices
 *	- if size is zero, mulvol tar is disabled and size is
 *	  assumed to be infinite.
 *	- returns volume size in TBLOCKS
 */
static	long
kcheck(kstr)
char	*kstr;
{
	long kval;

	kval = atol(kstr);
	if (kval == 0L) {	/* no multi-volume; size is infinity.  */
		mulvol = 0;	/* definitely not mulvol, but we must  */
		return(0);	/* took out setting of NotTape */
	}
	if (kval < (long) MINSIZE) {
		(void) pfmt(stderr, MM_ERROR, 
			":530:Sizes below %luK not supported (%lu).\n", (
			long) MINSIZE, kval);
		if (!kflag)
			(void) pfmt(stderr, MM_ERROR, 
				":531:Bad size entry for %s in %s.\n", archive, 
				DEF_FILE);
		done(1);
	}
	mulvol++;
	NotTape++;			/* implies non-tape */
	return(kval * 1024L / TBLOCK);	/* convert to TBLOCKS */
}

/*
 * bcheck()
 *	- checks the validity of blocking factors
 *	- returns blocking factor
 */
static	int
bcheck(bstr)
char	*bstr;
{
	int bval;

	bval = atoi(bstr);
	if (bval > NBLOCK || bval <= 0) {
		(void) pfmt(stderr, MM_ERROR, 
			":532:Invalid blocksize. (Max %d)\n", NBLOCK);
		if (!bflag)
			(void) pfmt(stderr, MM_ERROR, 
				":533:Bad blocksize entry for '%s' in %s.\n", 
				archive, DEF_FILE);
		done(1);
	}
	return(bval);
}

/*
 * defset()
 *	- reads DEF_FILE for the set of default values specified.
 *	- initializes 'usefile', 'nblock', and 'blocklim', and 'NotTape'.
 *	- 'usefile' points to static data, so will be overwritten
 *	  if this routine is called a second time.
 *	- the pattern specified by 'arch' must be followed by four
 *	  blank-separated fields (1) device (2) blocking,
 *    	                         (3) size(K), and (4) tape
 *	  for example: archive0=/dev/fd 1 400 n
 *	- the define's below are used in defcntl() to ignore case.
 */
static	int
defset(arch)
char	*arch;
{
	FILE *def_fp;
	extern int defcntl();
	char *bp;

	if ((def_fp = defopen(DEF_FILE)) == NULL)
		return(FALSE);
	if (defcntl(DC_SETFLAGS, (DC_STD & ~(DC_CASE))) == -1) {
		(void) pfmt(stderr, MM_ERROR, 
			":534:Error setting parameters for %s.\n", DEF_FILE);
		return(FALSE);			/* & following ones too */
	}
	if ((bp = defread(def_fp, arch)) == NULL) {
		(void) pfmt(stderr, MM_ERROR, 
			":535:Missing or invalid '%s' entry in %s.\n", arch,
			DEF_FILE);
		return(FALSE);
	}
	if ((usefile = strtok(bp, " \t")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, 
			":536:'%s' entry in %s is empty!\n", arch, DEF_FILE);
		return(FALSE);
	}
	if ((bp = strtok(NULL, " \t")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, 
			":537:Block component missing in '%s' entry in %s.\n",
			arch, DEF_FILE);
		return(FALSE);
	}
	nblock = bcheck(bp);
	if ((bp = strtok(NULL, " \t")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, 
			":538:Size component missing in '%s' entry in %s.\n",
			arch, DEF_FILE);
		return(FALSE);
	}
	blocklim = kcheck(bp);
	if ((bp = strtok(NULL, " \t")) != NULL)
		NotTape = (*bp == 'n' || *bp == 'N');
	else 
		NotTape = (blocklim > 0);
	(void)defclose(def_fp);
	DEBUG("defset: archive='%s'; usefile='%s'\n", arch, usefile);
	DEBUG("defset: nblock='%d'; blocklim='%ld'\n",
	      nblock, blocklim);
	DEBUG("defset: not tape = %d\n", NotTape, 0);
	return(TRUE);
}

/*
 * Following code handles excluded and included files.
 * A hash table of file names to be {in,ex}cluded is built.
 * For excluded files, before writing or extracting a file
 * check to see if it is in the exluce_tbl.
 * For included files, the wantit() procedure will check to
 * see if the named file is in the include_tbl.
 */
static	void
build_table(table, file)
struct file_list *table[];
char	*file;
{
	FILE	*fp;
	char	buf[MAXNAM];

	if ((fp = fopen(file, "r")) == (FILE *)NULL)
		vperror(1, badopen, file);
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		size_t n = strlen(buf);
		if (n != 0) {
			buf[n - 1] = '\0';
			add_file_to_table(table, buf);
		}
	}
	(void) fclose(fp);
}

/*
 * Add a file name to the the specified table, if the file name has any
 * trailing '/'s then delete them before inserting into the table
 */
static	void
add_file_to_table(table, str)
struct file_list *table[];
char	*str;
{
	char	name[MAXNAM];
	unsigned int h;
	struct	file_list	*exp;

	(void) strcpy(name, str);
	while (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = NULL;
	}

	h = hash(name);
	exp = (struct file_list *) malloc(sizeof(struct file_list));
	if (exp == 0)
		vperror(1, nomem, h);
	else {
		if ((exp->name = strdup(name)) == 0)
			vperror(1, nomem, h);
		else {
			exp->next = table[h];
			table[h] = exp;
		}
	}
}

/*
 * See if a file name or any of the file's parent directories is in the
 * specified table, if the file name has any trailing '/'s then delete
 * them before searching the table
 */
static	int
is_in_table(table, str)
struct file_list *table[];
char	*str;
{
	char	name[MAXNAM];
	unsigned int	h;
	struct	file_list	*exp;
	char	*ptr;

	(void) strcpy(name, str);
	while (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = NULL;
	}

	/*
	 * check for the file name in the passed list
	 */
	h = hash(name);
	exp = table[h];
	while (exp != NULL) {
		if (strcmp(name, exp->name) == 0) {
			return(1);
		}
		exp = exp->next;
	}

	/*
	 * check for any parent directories in the file list
	 */
	while (ptr = strchr(name, '/')) {
		*ptr = NULL;
		h = hash(name);
		exp = table[h];
		while (exp != NULL) {
			if (strcmp(name, exp->name) == 0) {
				return(1);
			}
			exp = exp->next;
		}
	}

	return(0);
}

/*
 * Compute a hash from a string.
 */
static	unsigned int
hash(str)
char	*str;
{
	char	*cp;
	unsigned int	h;

	h = 0;
	for (cp = str; *cp; cp++) {
		h += *cp;
	}
	return(h % TABLE_SIZE);
}

static struct linkbuf *
getmem(size)
{
	struct linkbuf *p = malloc((unsigned) size);

	if (p == NULL && freemem) {
		(void) pfmt(stderr, MM_ERROR, 
			":539:Out of memory, link and directory modtime info lost: %s\n",
			strerror(errno));
		freemem = 0;
		if (eflag)
			done(1);
	}
	return (p);
}

/* vperror() --variable argument perror.
 * Takes 3 args: exit_status, formats, arg.  If exit status is 0, then
 * the eflag (exit on error) is checked -- if it is non-zero, tar exits
 * with the value of whatever "errno" is set to.  If exit_status is not
 * zero, then it exits with that error status. If eflag and exit_status
 * are both zero, the routine returns to where it was called.
 */
/*VARARGS*/
static	void
vperror(exit_status, fmt, arg)
register char *fmt;
{
	int errnum;

	errnum = errno;
	(void) pfmt(stderr, MM_ERROR, fmt, arg);
	(void) pfmt(stderr, MM_NOSTD, ":540:: %s\n", strerror(errnum));
	if (exit_status)
		done(exit_status);
	else if (eflag)
		done(errnum);
}

/*VARARGS*/
static	void
fatal(s1, s2, s3, s4, s5)
char *s1, *s2, *s3, *s4, *s5;
{
	(void) pfmt(stderr, MM_ERROR, s1, s2, s3, s4, s5);
	putc('\n', stderr);
	done(1);
}

static	char	*
temp_str(cptr)
char	*cptr;
{
	static	char	strbuf[NAMSIZ+1];

	register char	*ip, *op;
	register int	i;

	ip = cptr;
	op = strbuf;
	i  = 0;

	while(*ip && i < NAMSIZ)	{
		*op++ = *ip++;
		i++;
	}
	strbuf[i] = '\0';

	return(strbuf);
}
/* Enhanced Application Compatibility Support */
/* docompr()							*/
/* Determine if file needs to be decompressed and send its path */
/* to the child for decompression.				*/

docompr(fname, ftype, fsize ,lname) 
char *fname;	/* Name of file to be uncompressed. */
int ftype;   	/* xstractbuf.st_mode 		    */
long fsize;	/* Size of entire file. 	    */
char *lname;	/* Name of link to be made. 	    */
{
	static int child_pid;		/* Child process id.  		*/
	int status, w; 			/* Status of child process. 	*/
	int i;
	int e;				
	static int apipe[2];		/* Pipe file descriptors. 	*/
	char pipebuf[BUFSIZ];  /* Buffer used to send filename */
		/* Note: Tar sends in <stdio.h> defined BUFSIZ chunks   */
		/* and compress reads in BUFSIZ chunks. 		*/
	static int firstime=1;

	static struct Clinkbuf {	/* Keep track of linked files. 	*/
		char	filename[NAMSIZ];
		char	linkname[NAMSIZ];
		struct	Clinkbuf *nextp;
	} *Clp;
	static struct Clinkbuf *Clphead=NULL;

	/* FIRSTIME: This section of code creates a pipe and	*/
	/*	     forks a child. 				*/
	if (firstime) {
		/* Create pipe */
		if(pipe(apipe) == -1) {
			e = errno;
			(void)pfmt(stderr, MM_ERROR, ":754:Cannot create pipe: %s\n",
				strerror(errno));
			 exit(e);	
		}
		switch (child_pid = fork()) { 
		case -1:
			/* Error fork failed */
			e = errno; 
			(void)pfmt(stderr, MM_ERROR, ":755:Fork() failed: %s\n",
				strerror(errno));
			exit(e);
		case 0:
			/* child */
			close(apipe[1]);	/* Close write end of pipe. */
			dochild(apipe[0]);	/* Pass read end of pipe.   */
			exit(0);
		}
		/* Parent */
		close(apipe[0]);		/* Close read end of pipe.  */
		firstime=0;
	}	

	/* DONE: This section of code signals our child were finished	*/
	/*       sending filenames across the pipe via a NULL and	*/
	/* 	 processes the list of links we have yet to make.	*/

	if ((strcmp(fname, "SENDaNULL") == 0) && ftype == 0) {
		for(i=0; i<(sizeof(pipebuf)); i++) pipebuf[i]=' ';
		pipebuf[0]='\0';
		write(apipe[1], pipebuf, sizeof(pipebuf));

		/* Wait for child to complete decompression. */
		while ((w = wait(&status)) != child_pid && w != -1);

		/* Close pipe. */
		close(apipe[1]);

		/* Process list of links to be made. */
		for (Clp = Clphead; Clp != NULL; Clp = Clp->nextp) {
			unlink(Clp->linkname);
			if (link(Clp->filename, Clp->linkname) < 0) {
				(void) pfmt(stderr, MM_ERROR, ":756:Cannot link %s to %s: %s\n",
					Clp->filename, Clp->linkname,
					strerror(errno));
				continue;
			}
			if (vflag)
				pfmt(vfile, MM_NOSTD, xlinkto,
					Clp->filename, Clp->linkname);
		}
		return;
	}

	/* LINKED FILE: This section of code adds filenames to our list */
	/*		of links to be made after decompression. This	*/
	/* 		is flagged by a non-zero length lname.		*/
	if (strlen(lname) != 0) {
		Clp = (struct Clinkbuf *) malloc(sizeof(*Clp));
		if (Clp == NULL) 
		(void) pfmt(stderr, MM_ERROR, ":758:Out of memory. Link information lost for %s\n",
				lname);
		else {
			Clp->nextp = Clphead;
			Clphead = Clp;
			strcpy(Clp->filename, fname);
			strcpy(Clp->linkname, lname);
		}
		return;
	}

	/* SEND FILE: This section of code sends appropriate files 	*/
	/*	      through the pipe to compress.			*/

	if ((ftype & S_IFMT) == S_IFREG && fsize != 0) {
		/* Initialize pipe buffer */
		for(i=0; i<(sizeof(pipebuf)); i++) pipebuf[i]=' ';
		strcpy(pipebuf, fname);
		write(apipe[1], pipebuf, sizeof(pipebuf));
	}
	return;
}

/*  						     	          */
/* dochild()						          */
/* Exec uncompress with -P <rpipe> telling it to uncompress       */
/* and overwrite files read from the pipe descriptor. uncompress  */
/* will continue reading names from the pipe until a NULL is      */
/* or the pipe is closed. 				          */

dochild(rpipe)
int rpipe;	/* Read end of pipe */
{
	char param[10];
	int e;						

	sprintf(param, "-P %d", rpipe); 
	execl("/usr/bin/uncompress", "uncompress", param, (char*)0);
	e = errno;
	pfmt(stderr, MM_ERROR, ":148:Cannot execute %s: %s\n",
		"/usr/bin/uncompress", strerror(errno));
	exit(e);			
} 						
/* End Enhanced Application Compatibility Support */


/* iterative read (read may return less bytes than requested
   if the file is associated with a communication line -- "rsh <host> tar ") */
iter_read(fildes, buf, nbyte)
int fildes;
char *buf;
int nbyte;
{
int rc, rest;
char *buf_ptr;

	buf_ptr = buf;
	rest = nbyte;
	while ((rc = read(fildes, buf_ptr, rest)) != rest) {
		if (rc == 0)
			return(nbyte-rest);
		if (rc < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			return(rc);
		}
		rest -= rc;
		buf_ptr += rc;
	} 
	return(nbyte);
}

