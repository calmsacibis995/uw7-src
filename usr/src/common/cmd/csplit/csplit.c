#ident	"@(#)csplit:csplit.c	1.13.2.1"
/*
*	csplit - Context or line file splitter
*	Compile: cc -O -s -o csplit csplit.c
*/

#define _POSIX_SOURCE	/* so NAME_MAX can be trusted */

#include <stdio.h>
#include <errno.h>
#include <regex.h>
#include <signal.h>
#include <stdlib.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>


#define LAST	  0L
#define ERR	 -1
#define FALSE	  0
#define TRUE	  1
#define EXPMODE	  2
#define LINMODE	  3

#define LOG2TO10(x)	((x) * 30103L / 100000L) /* floor(x*log(2)/log(10)) */
#define MAXDIGS		LOG2TO10(sizeof(unsigned int) * CHAR_BIT)

	/* Globals */

char *linbuf;			/* Input line buffer */
size_t linsiz;			/* Length of line buffer */
regex_t regbuf;			/* Most recent regular expression */
char *file = NULL;		/* File name buffer */
int ndig = 2;			/* Number of digits in file suffix */
unsigned int maxfls = 100;	/* Max. no. of files that are allowed */
char *targ;			/* Arg ptr for error messages */
char *sptr;
FILE *infile, *outfile;		/* I/O file streams */
int silent, keep, create;	/* Flags: -s(ilent), -k(eep), (create) */
int errflg;
extern int optind;
extern char *optarg;
long offset;			/* Regular expression offset value */
long curline;			/* Current line in input file */

char *getline();

static	const	char	outofrangee[] = ":6:%s - Out of range\n";
static	const	char	outofmemory[] = ":87:Out of memory: %s\n";
static	const	char	toolongpref[] = ":7:Prefix %s too long\n";


main(argc,argv)
int argc;
char **argv;
{
	void sig();
	long findline();
	FILE *getfile();
	int ch, mode;
	char *tail;		/* pointer to basename of file prefix */
	char *head;		/* dirname of file prefix */
	int baselen;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:csplit");
	while((ch=getopt(argc,argv,"f:kn:s")) != EOF) {
		switch(ch) {
			case 'f':
				file = optarg;
				break;
			case 'k':
				keep++;
				break;
			case 'n':
				if(optarg[0] < '0' || '9' < optarg[0]) {
					pfmt(stderr, MM_ERROR,
						":91:Bad number: %s\n", optarg);
					goto usage;
				} else if((ndig = atoi(optarg)) <= MAXDIGS) {
					maxfls = 10;
					for(ch = 1; ch < ndig; ch++)
						maxfls *= 10;
				} else {
					maxfls = INT_MAX - 2;
				}
				break;
			case 's':
				silent++;
				break;
			case '?':
				errflg++;
		}
	}

	argv = &argv[optind];
	argc -= optind;
	if(argc <= 1 || errflg) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":2:Incorrect usage\n");
	usage:;
		fatal(MM_ACTION,
			":148:Usage: csplit [-ks] [-f prefix] [-n digits] file args ...\n");
	}

	if(file == 0)	/* no -f option */
		file = "xx";
	if((head = malloc(strlen(file) + 2 + ndig + 1)) == 0) /* 2 for "./" */
		fatal(MM_ERROR, outofmemory, strerror(errno));
	if((tail = strrchr(file, '/')) != 0) {
#ifndef NAME_MAX
		*tail = '\0';
		strcpy(head, file);
		*tail = '/';
#endif
		tail++;
	} else {
		tail = file;
#ifndef NAME_MAX
		head[0] = '.';
		head[1] = '\0';
#endif
	}
	baselen = strlen(tail) + ndig;
#ifdef NAME_MAX
	if (baselen > NAME_MAX)
		fatal(MM_ERROR, toolongpref, tail);
#else
	if(baselen > _POSIX_NAME_MAX) {
		long n;

		if((n = pathconf(head, _PC_NAME_MAX)) != -1) {
			if(baselen > n)
				fatal(MM_ERROR, toolongpref, tail);
		} else if(errno != 0) {
			fatal(MM_NOGET, "%s: %s\n", head, strerror(errno));
		} else if(creat_check(head, tail) != 0) {
			fatal(MM_ERROR, toolongpref, tail);
		}
	}
#endif
	file = strcpy(head, file);

	/*
	* Because seeks are used for repositioning,
	* copy stdin to a temporary file so that seeks
	* are valid.  Unfortunately, there doesn't appear
	* to be a safe and cheap test to see whether stdin
	* is seekable already.
	*/
	if(argv[0][0] == '-' && argv[0][1] == '\0') {
		char buf[BUFSIZ];
		int n;

		infile = tmpfile();
		while((n = fread(buf, (size_t)1, sizeof(buf), stdin)) != 0) {
			if(fwrite(buf, (size_t)1, (size_t)n, infile) != n) {
				pfmt(stderr, MM_ERROR,
					":10:Cannot write temporary file: %s\n",
					strerror(errno));
				exit(1);
			}
		}
		rewind(infile);
	} else if((infile = fopen(*argv,"r")) == NULL) {
		fatal(MM_ERROR,":3:Cannot open %s: %s\n",
			*argv, strerror (errno));
	}
	++argv;
	curline = 1L;
	signal(SIGINT,sig);
	if((linbuf = malloc(linsiz = 1024)) == 0)
		fatal(MM_ERROR, outofmemory, strerror(errno));

	/*
	*	The following for loop handles the different argument types.
	*	A switch is performed on the first character of the argument
	*	and each case calls the appropriate argument handling routine.
	*/

	for(; *argv; ++argv) {
		targ = *argv;
		switch(**argv) {
		case '/':
			mode = EXPMODE;
			create = TRUE;
			re_arg(*argv);
			break;
		case '%':
			mode = EXPMODE;
			create = FALSE;
			re_arg(*argv);
			break;
		case '{':
			num_arg(*argv,mode);
			mode = FALSE;
			break;
		default:
			mode = LINMODE;
			create = TRUE;
			line_arg(*argv);
			break;
		}
	}
	create = TRUE;
	to_line(LAST, 1);

	exit (0);
}

/*
*	Atol takes an ascii argument (str) and converts it to a long (plc)
*	It returns ERR if an illegal character.  The reason that atolong
*	does not return an answer (long) is that any value for the long
*	is legal, and this version of atolong detects error strings.
*/

atolong(str,plc)
register char *str;
long *plc;
{
	register int f;
	*plc = 0;
	f = 0;
	for(;;str++) {
		switch(*str) {
		case ' ':
		case '\t':
			continue;
		case '-':
			f++;
		case '+':
			str++;
		}
		break;
	}
	for(; *str != NULL; str++)
		if(*str >= '0' && *str <= '9')
			*plc = *plc * 10 + *str - '0';
		else
			return(ERR);
	if(f)
		*plc = -(*plc);
	return(TRUE);	/* not error */
}

/*
*	Closefile prints the byte count of the file created, (via fseek
*	and ftell), if the create flag is on and the silent flag is not on.
*	If the create flag is on closefile then closes the file (fclose).
*/

closefile()
{
	long ftell();

	if(!silent && create) {
		fseek(outfile,0L,2);
		fprintf(stdout,"%ld\n",ftell(outfile));
	}
	if(create)
		fclose(outfile);
}

/*
*	Fatal handles error messages and cleanup.
*	Because "arg" can be the global file, and the cleanup processing
*	uses the global file, the error message is printed first.  If the
*	"keep" flag is not set, fatal unlinks all created files.  If the
*	"keep" flag is set, fatal closes the current file (if there is one).
*	Fatal exits with a value of 1.
*/

fatal(howbad, string,arg1, arg2)
int	howbad;
char *string, *arg1, *arg2;
{
	register char *fls;
	register unsigned int num;

	pfmt(stderr, howbad, string, arg1, arg2);
	if(!keep) {
		if(outfile) {
			fclose(outfile);
			fls = strchr(file, '\0') - ndig;
			for(num = atoi(fls);; num--) {
				sprintf(fls, "%.*u", ndig, num);
				unlink(file);
				if(num == 0)
					break;
			}
		}
	} else
		if(outfile)
			closefile();
	exit(1);
}

static void
reprob(err)
{
	char msg[128];

	regerror(err, &regbuf, msg, sizeof(msg));
	fatal(MM_ERROR, ":147:RE error in %s: %s\n", targ, msg);
}

/*
*	Findline returns the line number referenced by the current argument.
*	Its arguments are a pointer to the compiled regular expression (expr),
*	and an offset (oset).  The variable lncnt is used to count the number
*	of lines searched.  First the current stream location is saved via
*	ftell(), and getline is called so that R.E. searching starts at the
*	line after the previously referenced line.  The while loop checks
*	that there are more lines (error if none), bumps the line count, and
*	checks for the R.E. on each line.  If the R.E. matches on one of the
*	lines the old stream location is restored, and the line number
*	referenced by the R.E. and the offset is returned.
*/

long findline(oset)
long oset;
{
	static int benhere;
	long lncnt = 0, saveloc, ftell();
	int err;

	saveloc = ftell(infile);
	if(curline != 1L || benhere)		/* If first line, first time, */
		getline(FALSE);			/* then don't skip */
	else
		lncnt--;
	benhere = 1;
	while(getline(FALSE) != NULL) {
		lncnt++;
		if((sptr=strrchr(linbuf,'\n')) != NULL)
			*sptr = '\0';
		if((err = regexec(&regbuf, linbuf, (size_t)0,
			(regmatch_t *)0, 0)) == 0)
		{
			fseek(infile,saveloc,0);
			return(curline+lncnt+oset);
		} else if (err != REG_NOMATCH)
			reprob(err);
	}
	fseek(infile,saveloc,0);
	return(curline+lncnt+oset+2);
}

/*
*	Flush uses fputs to put lines on the output file stream (outfile)
*	Since fputs does its own buffering, flush doesn't need to.
*	Flush does nothing if the create flag is not set.
*/

flush()
{
	if(create)
		fputs(linbuf,outfile);
}

/*
*	Getfile does nothing if the create flag is not set.  If the
*	create flag is set, getfile positions the file pointer (fptr) at
*	the end of the file name prefix on the first call (fptr=0).
*	Next the file counter (ctr) is tested for MAXFLS, fatal if too
*	many file creations are attempted.  Then the file counter is
*	stored in the file name and incremented.  If the subsequent
*	fopen fails, the file name is copied to tfile for the error
*	message, the previous file name is restored for cleanup, and
*	fatal is called.  If the fopen succecedes, the stream (opfil)
*	is returned.
*/

FILE *getfile()
{
	static char *fptr;
	static unsigned int ctr;
	FILE *opfil;
	char *tfile;

	if(create) {
		if(fptr == 0)
			for(fptr = file; *fptr != NULL; fptr++);
		if(ctr >= maxfls)
			fatal(MM_ERROR,
				":146: %u file limit reached at arg %s\n",
				maxfls, targ);
		sprintf(fptr, "%.*u", ndig, ctr++);
		if((opfil = fopen(file,"w")) == NULL) {
			if ((tfile = strdup(file)) == NULL)
				pfmt(stderr,MM_NOGET,"%s: %s\n",file,strerror(errno));
			sprintf(fptr, "%.*u", ndig, ctr - 2);
			fatal(MM_ERROR,":12:Cannot create %s: %s\n",tfile, strerror (errno));
		}
		return(opfil);
	}
	return(NULL);
}

/*
*	Getline gets a line via fgets from the input stream "infile".
*	If getline is called with a non-zero value, the current line
*	is bumped, otherwise it is not (for R.E. searching).
*/

char *getline(bumpcur)
int bumpcur;
{
	if(bumpcur)
		curline++;
	linbuf[linsiz - 1] = '\n';
	if(fgets(linbuf, linsiz, infile) == 0)
		return 0;
	while(linbuf[linsiz - 1] != '\n') {
		char *p;

		if((linbuf = realloc(linbuf, linsiz + 1024)) == 0)
			fatal(MM_ERROR, outofmemory, strerror(errno));
		p = &linbuf[linsiz];
		linsiz += 1024;
		linbuf[linsiz - 1] = '\n';
		if(fgets(p, (size_t)1024, infile) == 0)
			break;
	}
	return linbuf;
}

/*
*	Line_arg handles line number arguments.
*	line_arg takes as its argument a pointer to a character string
*	(assumed to be a line number).  If that character string can be
*	converted to a number (long), to_line is called with that number,
*	otherwise error.
*/

line_arg(line)
char *line;
{
	long to;

	if(atolong(line,&to) == ERR)
		fatal(MM_ERROR,":13:%s: Bad line number\n",line);
	to_line(to, 1);
}

/*
*	Num_arg handles repeat arguments.
*	Num_arg copies the numeric argument to "rep" (error if number is
*	larger than 11 characters or } is left off).  Num_arg then converts
*	the number and checks for validity.  Next num_arg checks the mode
*	of the previous argument, and applys the argument the correct number
*	of times. If the mode is not set properly its an error.
*/

num_arg(arg,md)
register char *arg;
int md;
{
	long repeat, toline;
	char rep[12];
	register char *ptr;
	int step = 1;

	ptr = rep;
	for(++arg; *arg != '}'; arg++) {
		if(ptr == &rep[11])
			fatal(MM_ERROR,":14:%s: Repeat count too large\n",targ);
		if(*arg == NULL)
			fatal(MM_ERROR,":15:%s: Missing '}'\n",targ);
		*ptr++ = *arg;
	}
	*ptr = NULL;
	if(rep[0] == '*' && rep[1] == '\0') {
		step = 0;
		repeat = 1;
	} else if((atolong(rep,&repeat) == ERR) || repeat < 0L)
		fatal(MM_ERROR,":16:Illegal repeat count: %s\n",targ);
	if(md == LINMODE) {
		toline = offset = curline;
		for(;repeat > 0L; repeat -= step) {
			toline += offset;
			to_line(toline, step);
		}
	} else	if(md == EXPMODE)
			for(;repeat > 0L; repeat -= step)
				to_line(findline(offset), step);
		else
			fatal(MM_ERROR,":17:No operation for %s\n",targ);
}

/*
*	Re_arg handles regular expression arguments.
*	Re_arg takes a csplit regular expression argument.  It checks for
*	delimiter balance, computes any offset, and compiles the regular
*	expression.  Findline is called with the compiled expression and
*	offset, and returns the corresponding line number, which is used
*	as input to the to_line function.
*/

re_arg(string)
char *string;
{
	register char *ptr;
	register char ch;
	int cur, bkt;

	ch = *string;
	ptr = string;
	bkt = '\0';
	for(;;) {
		if((cur = *++ptr) == '\0') {
	missing:;
			fatal(MM_ERROR,":18:%s: Missing delimiter\n",targ);
		}
		if(cur == ch && bkt == '\0')
			break;
		if(cur == ']') {
			if(bkt == '[')
				bkt = '\0';
			else if(bkt != '\0' && ptr[-1] == bkt)
				bkt = '[';
		} else if(cur == '[' && (bkt == '\0' || bkt == '[')) {
			if(bkt == '\0')
				bkt = '[';
			else
				bkt = ']';	/* temporary */
			if((cur = *++ptr) == '\0')
				goto missing;
			if(bkt == ']') {
				if(cur != ':' && cur != '=' && cur != '.')
					bkt = '[';
				else {
					bkt = cur;
					if((cur = *++ptr) == '\0')
						goto missing;
				}
			}
		}
		if(cur == '\\' && bkt == '\0') {
			if((cur = *++ptr) == '\0')
				goto missing;
		}
	}

	*ptr = '\0';
	if(atolong(++ptr,&offset) == ERR)
		fatal(MM_ERROR,":19:%s: Illegal offset\n",string);

	string++;
	if((bkt = regcomp(&regbuf, string, REG_ANGLES | REG_ESCNL | REG_NOSUB)) != 0)
		reprob(bkt);
	to_line(findline(offset), 1);
}

/*
*	Sig handles breaks.  When a break occurs the signal is reset,
*	and fatal is called to clean up and print the argument which
*	was being processed at the time the interrupt occured.
*/

void
sig(s)
int	s;
{
	signal(SIGINT,sig);
	fatal(MM_ERROR,":20:Interrupt - Program aborted at arg '%s'\n",targ);
}

/*
*	To_line creates split files.
*	To_line gets as its argument the line which the current argument
*	referenced.  To_line calls getfile for a new output stream, which
*	does nothing if create is False.  If to_line's argument is not LAST
*	it checks that the current line is not greater than its argument.
*	While the current line is less than the desired line to_line gets
*	lines and flushes (error if EOF is reached).
*	If to_line's argument is LAST, it checks for more lines, and gets
*	and flushes lines till the end of file.
*	Finally, to_line calls closefile to close the output stream.
*/

to_line(ln, err)
long ln;
{
	outfile = getfile();
	if(ln != LAST) {
		if(curline > ln)
			goto range;
		while(curline < ln) {
			if(getline(TRUE) == NULL)
				goto range;
			flush();
		}
	} else if(getline(TRUE) != NULL) {
		flush();
		while(TRUE) {
			if(getline(TRUE) == NULL)
				break;
			flush();
		}
	} else {
range:;
		if(err)
			fatal(MM_ERROR, outofrangee, targ);
		closefile();
		exit(0);
	}
	closefile();
}
/* 
 * creat_check accounts for some file systems types (e.g. nfs) where a 
 * file name limit cannot be determined via pathconf.  In this case, we try
 * to create the file,if is doesn't exist, and then read the directory to
 * see if the file is there, or a truncated version.  If the file did
 * not exist before entering this function, it is removed.
 */
int
creat_check(name, filename)
char *name;
char *filename;
{
	int found, existed;
	struct dirent *entp;
	DIR *dirp;
	FILE *fp;
	char *p;

	if((dirp = opendir(name)) == 0)
		return -1;
	p = strchr(name, '\0');
	sprintf(p, "/%s%.*u", filename, ndig, 0); /* enough room provided */
	p++;
	if((fp = fopen(name, "r")) != 0)
		existed = 1;
	else if((fp = fopen(name, "a+")) == 0)
		return -1;
	else
		existed = 0;
	found = -1;
	while((entp = readdir(dirp)) != 0) {
		if(strcmp(entp->d_name, p) != 0)
			continue;
		found = 0;
		break;
	}
	closedir(dirp);
	fclose(fp);
	if(existed)
		unlink(name);
	return found;
}
