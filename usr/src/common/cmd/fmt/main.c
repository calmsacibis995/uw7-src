/*	copyright	"%c%"	*/


#ident	"@(#)fmt:main.c	1.2.1.1"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <limits.h>
#include <pfmt.h>
#include <errno.h>
#include <sys/euc.h>
#include <getwidth.h>

#define USAGE	":130:Usage: fmt [-c] [-s] [-w width | -width] [inputfile...]\n"
#define ERRNUM	":3:Cannot open %s: %s\n"
#define NOWIDTH	":131:width not specified following -w option\n"
#define NONNUM	":132:Non-numeric character found in width specification\n"
#define BADNUM	":133:Number out of range: %s\n"

#if !defined(LINE_MAX)
#define LINE_MAX	4096
#endif

eucwidth_t wi;

static FILE *fd;
static int split=0, crown=0, mwidth=72;
static char cmdbuf[LINE_MAX];

int main(int argc, char *argv[]);
int gettargs(int argc, char *argv[]);
void fmtfile(int split, int crown, int mwidth);

int
main(int argc, char *argv[]) {

    int around=0;
    int retval=0;
    int optind;

    (void)setlocale(LC_ALL,"");
    (void)setcat("uxdfm");
    (void)setlabel("UX:fmt");
    (void)getwidth(&wi);

    optind = gettargs(argc, argv);

    argc -= optind;
    argv += optind;

    /*
    * format each file, printing a newline between them.
    */
    do {
	if (argc > 0) {
	    (void)close(0);
	    if (freopen(argv[0], "r", stdin) == NULL) {
		pfmt(stderr, MM_WARNING, ERRNUM,
		    argv[0], strerror(errno));
		retval++;
		argc--;
		argv++;
		continue;
	    }
	    argc--;
	    argv++;
	}
	if (around) {
	    (void)putchar('\n');
	}
	around++;
	fmtfile(split, crown, mwidth);
	(void)fflush(stdout);
    } while (argc > 0);
    exit(retval);
}

/*
* gettargs -- set the variables from the options on the command line
*
* Inputs:	argc	pointer to the number of arguments.
*		argv	pointer to an array of pointers (aaarrrgh!)
*
* Outputs:	mwidth	set to the last -w<number> or -<number>
*		split	set if -s found, otherwise clear
*		crown	set if -c found, otherwise clear
*		return	the value argc and argv are changed by
*/

int
gettargs(int argc, char *argv[]) {

    char c, *remains, *space=cmdbuf, *tmp;
    int i,j;

    /*
     * Pre-process the argument list, converting any '-<number>'
     * strings to '-w<number>'.
     */
    for (i=1; i < argc; i++) {
	if (*argv[i] == '-') {
	    /*
	     * if the argument is '-digit...', then create a new
	     * string that starts with '-w' rather than '-'.
	     */
	    if (isdigit(c=*(argv[i]+1))) {
		tmp = space;
		*space++ = '-';
		*space++ = 'w';
		for (j=1; *space++ = *(argv[i]+j); j++);
		argv[i] = tmp;
	    /*
	     * if the argument is '--...' then finish processing here.
	     */
	    } else if (c == '-') {
		i = argc;
	    /*
	     * if the argument is '-...w ...' (note the space between the
	     * '-...w' and the '...') then skip the next argument (the '...').
	     */
	    } else {
		for (j=1; *(argv[i]+j) != '\0'; j++);
		if (*(argv[i]+j-1) == 'w') {
			i++;
		}
	    }
	/*
	 * if the argument doesn't start with a '-', finish processing.
	 */
	} else {
	    i = argc;
	}
    }

    while ((c = getopt(argc, argv, "csw:")) != EOF) {
	switch (c) {
	case 'c' :
	    crown = 1;
	    break;
	case 's' :
	    split = 1;
	    break;
	case 'w' :
	    mwidth = strtol(optarg, &remains, 10);
	    if (*remains != '\0') {
		pfmt(stderr, MM_ERROR, NONNUM);
		pfmt(stderr, MM_ACTION, USAGE);
		exit(1);
	    } else if (mwidth < 0 || mwidth > LINE_MAX) {
		pfmt(stderr, MM_ERROR, BADNUM, optarg);
		pfmt(stderr, MM_ACTION, USAGE);
		exit(1);
	    }
	    break;
	default :
	    pfmt(stderr, MM_ACTION, USAGE);
	    exit(1);
	    break;
	}
    }
    return(optind);
}
