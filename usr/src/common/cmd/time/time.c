/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)time:time.c	1.6.1.4"
#ident "$Header$"
/*
**	Time a command
*/

#include	<stdio.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<time.h>
#include	<sys/time.h>
#include	<sys/times.h>
#include	<unistd.h>
#include	<locale.h>
#include	<pfmt.h>

static void	printt(const char *, clock_t);
static void	printpt(const char *, clock_t);

main(argc, argv)
char **argv;
{
	struct tms buffer;
	register pid_t p;
	int	status;
	clock_t	before, after;
	int	optsw;		/* switch for while of getopt() */
	int	pflag = 0;			/* 1 = -p flag specified */

	(void)setlocale(LC_ALL, "");
        (void)setcat("uxue");
        (void)setlabel("UX:time");
	tzset();

	if(argc<=1)
		exit(0);
	while ((optsw = getopt(argc, argv, "p")) != EOF) {
		switch(optsw) {
		case 'p':
			pflag = 1;
			break;
		default:
			(void)pfmt(stderr, MM_ACTION,
			    ":327:Usage:\ttime [-p] command [argument...]\n");
			exit(1);
			/* NOTREACHED */
		}
	}
	before = times(&buffer);
	
	p = fork();
	if(p == (pid_t)-1) {
		(void)pfmt(stderr, MM_ERROR,
			":322:Cannot fork -- try again.\n");
		exit(2);
	}
	if(p == (pid_t)0) {
		int rc;
		(void) execvp(argv[optind], &argv[optind]);
		if (errno == ENOENT) {
			rc = 127;
		} else {
			rc = 126;
		}
	        (void)pfmt(stderr, MM_ERROR, ":6:%s: %s\n",
			strerror(errno), argv[optind]);
		exit(rc);
	}
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	while(wait(&status) != p);
	if((status & 0377) != '\0')
		(void)pfmt(stderr, MM_ERROR,
			":323:Command terminated abnormally.\n");
	after = times(&buffer);
	(void) fprintf(stderr,"\n");
	if (pflag) {
		printpt(gettxt(":324", "real"), (after-before));
		printpt(gettxt(":325", "user"),
			buffer.tms_utime + buffer.tms_cutime);
		printpt(gettxt(":326", "sys "),
			buffer.tms_stime + buffer.tms_cstime);
	} else {
		printt(gettxt(":324", "real"), (after-before));
		printt(gettxt(":325", "user"),
			buffer.tms_utime + buffer.tms_cutime);
		printt(gettxt(":326", "sys "),
			buffer.tms_stime + buffer.tms_cstime);
	}
	exit(status >> 8);
}

/* number of digits following the radix character */
#define PREC(X) ((((X) > 0) && ((X) < 101)) ? 2 : 3)

static void
printpt(s, a)
const char *s;
clock_t a;
{

	int	clk_tck = sysconf(_SC_CLK_TCK);
	double	seconds = ((double)a) / clk_tck;

	(void) fprintf(stderr,s);
	(void) fprintf(stderr, gettxt(":352", " %9.*f\n"), 
			PREC(clk_tck), seconds);
}

static void	
printt(s, a)
const char *s;
clock_t a;
{
	int	clk_tck = sysconf(_SC_CLK_TCK);
	int	hours; 
	int	minutes;
	double	seconds; 
	char	buf[100];
	clock_t t = a;

	t /= 60 * clk_tck;
	minutes = t % 60;
	t /= 60;
	hours = t % 60;

	seconds = (a - (minutes + hours * 60.0) * 60.0 * clk_tck) / clk_tck;

	if (minutes || hours) {
		if (hours)
			(void)sprintf(buf,gettxt(":353", "%d:%02d:%04.1f"), 
						hours, minutes,seconds);
		else
			(void) sprintf(buf,gettxt(":354", "%d:%04.1f"), 
					minutes,seconds);
	} else
		(void) sprintf(buf,gettxt(":355", "%.1f"), seconds);

	(void)fprintf(stderr,"%s%11s\n",s, buf);

}
