#ident	"@(#)valtools:ckrange.c	1.2.6.2"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <values.h>
#include "usage.h"
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

extern int	optind, ckquit, ckindent, ckwidth;
extern char	*optarg;

extern long	atol(), strtol();
extern void	exit(), ckrange_err(), ckrange_hlp();
extern int	getopt(), ckrange(), ckrange_val();
extern void	*malloc();

#define BADPID	(-2)
#define MAXDIG	15	/* max number of digits in number */

char	*prog,
	*deflt,
	*prompt,
	*error,
	*help;
int	kpid = BADPID;
int	signo; 
short	base = 10;

extern	char	*str_upper;
extern	char	*str_lower;

char	vusage[] = "bul";
char	husage[] = "bulWh";
char	eusage[] = "bulWe";

#define USAGE	gettxt(":1","[-l lower] [-u upper] [-b base]")		

void
usage()
{
	switch(*prog) {
	  default:
		(void) pfmt(stderr, MM_ACTION,				
			 ":2:Usage: %s [options] %s\n",prog, USAGE);
		(void) pfmt(stderr, MM_NOSTD, OPTMESG);			
		(void) pfmt(stderr, MM_NOSTD, STDOPTS);			
		break;

	  case 'v':
		(void) pfmt(stderr, MM_ACTION,				
			 ":3:Usage: %s %s input\n", prog, USAGE);
		break;

	  case 'h':
		(void) pfmt(stderr, MM_ACTION,				
			 ":2:Usage: %s [options] %s\n",prog, USAGE);
		(void) pfmt(stderr, MM_NOSTD, OPTMESG);			
		(void) pfmt(stderr, MM_NOSTD,
			":4:\t-W width\n\t-h help\n");			
		break;

	  case 'e':
		(void) pfmt(stderr, MM_ACTION,				
	 		 ":2:Usage: %s [options] %s\n",prog, USAGE);
		(void) pfmt(stderr, MM_NOSTD, OPTMESG);			
		(void) pfmt(stderr, MM_NOSTD,
			":7:\t-W width\n\t-e error\n");			
		break;
	}
	exit(1);
}

main(argc, argv)
int argc;
char *argv[];
{
	int	c, n;
	long	lvalue, uvalue, intval;
	char	*ptr = 0;
	(void)setlocale(LC_ALL,"");					
	(void)setcat("uxvaltools");					
	(void)setlabel("UX:ckrange");					


	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	while((c=getopt(argc, argv, "l:u:b:d:p:e:h:k:s:QW:?")) != EOF) {
		/* check for invalid option */
		if((*prog == 'v') && !strchr(vusage, c))
			usage();
		if((*prog == 'e') && !strchr(eusage, c))
			usage();
		if((*prog == 'h') && !strchr(husage, c))
			usage();

		switch(c) {
		  case 'Q':
			ckquit = 0;
			break;

		  case 'W':
			ckwidth = atol(optarg);
			if(ckwidth < 0) {
				progerr(":8:negative display width specified"); 
				exit(1);
			}
			break;

		  case 'b':
			base = atol(optarg);
			if((base < 2) || (base > 36)) {
				progerr(":9:base must be between 2 and 36"); 
				exit(1);
			}
			break;

		  case 'u':
			str_upper = optarg;
			break;

		  case 'l':
			str_lower = optarg;
			break;

		  case 'd':
			deflt = optarg;
			break;

		  case 'p':
			prompt = optarg;
			break;

		  case 'e':
			error = optarg;
			break;

		  case 'h':
			help = optarg;
			break;

		  case 'k':
			kpid = atol(optarg);
			break;
			
		  case 's':
			signo = atol(optarg);
			break;

		  default:
			usage();
		}
	}

	if(signo) {
		if(kpid == BADPID)
			usage();
	} else
		signo = SIGTERM;

	if (str_upper) {
		uvalue = strtol(str_upper, &ptr, base);
		if (ptr == str_upper) {
			progerr(":10:invalid upper value specification"); 
			exit(1);
		}
	} else {
		str_upper = (char *) malloc(MAXDIG+1);
		uvalue = LONG_MAX;
		sprintf(str_upper, "%ld", uvalue);
		}
	if (str_lower) {
		lvalue =  strtol(str_lower, &ptr, base);
		if(ptr == str_lower) {
			progerr(":11:invalid lower value specification"); 
			exit(1);
		}
	} else {
		str_lower = (char *) malloc(MAXDIG+1);
		lvalue = LONG_MIN;
		sprintf(str_lower, "%ld", lvalue);
		}

	if (uvalue < lvalue) {
		progerr(":12:upper value is less than lower value"); 
		exit(1);
	}

	if(*prog == 'v') {
		if(argc != (optind+1))
			usage();
		exit(ckrange_val(lvalue, uvalue, base, argv[optind]));
	}

	if(optind != argc)
		usage();

	if(*prog == 'e') {
		ckindent = 0;
		ckrange_err(lvalue, uvalue, base, error);
		exit(0);
	} else if(*prog == 'h') {
		ckindent = 0;
		ckrange_hlp(lvalue, uvalue, base, help);
		exit(0);
	}
	
	n = ckrange(&intval, lvalue, uvalue, base, deflt, error, help, prompt);
	if(n == 3) {
		if(kpid > -2)
			(void) kill(kpid, signo);
		(void) puts("q");
	} else if(n == 0)
		(void) printf("%d", intval);
	exit(n);
}
