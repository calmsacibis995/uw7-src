#ident	"@(#)valtools:cktime.c	1.2.6.2"
/* This file is not fully internationalized. */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "usage.h"
#include <locale.h>							
#include <ctype.h>							
#include <pfmt.h>							

extern int	optind, ckquit, ckindent, ckwidth;
extern char	*optarg;

extern long	atol();
extern void	exit();

#define BADPID	(-2)

char	*prog,
	*deflt,
	*prompt,
	*error,
	*help;
int	kpid = BADPID;
int	signo;
char	*fmt;

char	vusage[] = "f";
char	husage[] = "fWh";
char	eusage[] = "fWe";

#define USAGE "[-f format]"
#define MYFMT "%s:ERROR:invalid format\n\
valid format descriptors are:\n\
\t%%H  #hour (00-23)\n\
\t%%I  #hour (00-12)\n\
\t%%M  #minute (00-59)\n\
\t%%p  #AM, PM, am or pm\n\
\t%%r  #time as %%I:%%M:%%S %%p\n\
\t%%R  #time as %%H:%%M (default)\n\
\t%%S  #seconds (00-59)\n\
\t%%T  #time as %%H:%%M:%%S\n\
"

void
usage()
{
	switch(*prog) {
	  default:
		(void) fprintf(stderr, "usage: %s [options] %s\n", 
			prog, USAGE);
		(void) pfmt(stderr, MM_NOSTD, OPTMESG);			
		(void) pfmt(stderr, MM_NOSTD, STDOPTS);			
		break;

	  case 'v':
		(void) fprintf(stderr, "usage: %s %s input\n", prog, USAGE);
		break;

	  case 'h':
		(void) fprintf(stderr, "usage: %s [options] %s\n", 
			prog, USAGE);
		(void) pfmt(stderr, MM_NOSTD, OPTMESG);			
		(void) fputs("\t-W width\n\t-h help\n", stderr);
		break;

	  case 'e':
		(void) fprintf(stderr, "usage: %s [options] %s\n", 
			prog, USAGE);
		(void) pfmt(stderr, MM_NOSTD, OPTMESG);			
		(void) fputs("\t-W width\n\t-e error\n", stderr);
		break;
	}
	exit(1);
}

main(argc, argv)
int argc;
char *argv[];
{
	int c, n;
	char tod[64];

	(void)setlocale(LC_ALL,"");					
	(void)setcat("uxvaltools");					
	(void)setlabel("UX:cktime");					

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	while((c=getopt(argc, argv, "f:d:p:e:h:k:s:QW:?")) != EOF) {
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

		  case 'f':
			fmt = optarg;
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

	if(*prog == 'v') {
		if(argc != (optind+1))
			usage();
		n = cktime_val(fmt, argv[optind]);
		if(n == 4)
			(void) fprintf(stderr, MYFMT, prog);
		exit(n);
	}

	if(optind != argc)
		usage();

	if(*prog == 'e') {
		ckindent = 0;
		if(cktime_err(fmt, error)) {
			(void) fprintf(stderr, MYFMT, prog);
			exit(4);
		} else
			exit(0);
	} else if(*prog == 'h') {
		ckindent = 0;
		if(cktime_hlp(fmt, help)) {
			(void) fprintf(stderr, MYFMT, prog);
			exit(4);
		} else
			exit(0);
	}

	n = cktime(tod, fmt, deflt, error, help, prompt);
	if(n == 3) {
		if(kpid > -2)
			(void) kill(kpid, signo);
		(void) puts("q");
	} else if(n == 0)
		(void) fputs(tod, stdout);
	if(n == 4)
		(void) fprintf(stderr, MYFMT, prog);
	exit(n);
}
