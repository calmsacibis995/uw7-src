/*		copyright	"%c%" 	*/
#ident	"@(#)message.c	15.1"

#include <stdio.h>
#include <termio.h>
#include <signal.h>
#include <setjmp.h>
#include <locale.h>
#include <unistd.h>

struct termio newtty, oldtty;

jmp_buf env;
int wait = 0;

#define MT_CONFIRM 1
#define MT_DISPLAY 8
#define MT_ERROR 0
#define MT_HELP 32
#define MT_INFO	16
#define MT_POPUP 2
#define MT_QUIT 64

#define  Max_lines	15
#define  Max_cols	60

extern void	exit();

void sig_hand();
void alarm_hand();

	/*
	 *  main  -  Start execution of process
	 */

extern int optind;
extern char *optarg;

int	option = 0;
char   *title=NULL;
int	title_ct=0;

main (argc, argv)

	int	argc;
	char	*argv [];

	{
	char	errtext [80*24];
	int	c;
	int	key;

	if(argc <= 1)
	   exit(-1);
	setlocale(LC_ALL, "");
	setcat("uxpkgtools");

	while((c = getopt(argc, argv, "cdt:uif:w:")) != EOF)
		switch(c) {
			case 'c':
				option |=1;
				break;
			case 'u':
				break;
			case 'i':
				option |=2;
				break;
			case 'f':
				option |=4;
				break;
			case 'd':
				option |= 8;
				break;
			case 't':
				option |= 16;
				title = optarg;
				title_ct = optind;
				break;
			case 'w':
				wait = atoi(optarg);
				break;
			case '?':
				puts (gettxt(":817","Usage: message -c|d|i|t <border title>|u <dead option>|f <dead option> |w <second> \"your message\""));
				exit (1);	
		}

	format(argc,argv,optind,errtext);
	key = MT_ERROR;
	if (option&16) key=MT_INFO;
	if (option&8) key=MT_DISPLAY;
	if (option&2) key=MT_POPUP;
	if (option&1) key=MT_CONFIRM;

	_ioctl (0, TCGETA, &oldtty);
	newtty = oldtty;
	newtty.c_lflag &= ~(ICANON|ECHO);
	newtty.c_iflag &= ~ICRNL;
	newtty.c_cc[VMIN] = 1;
	newtty.c_cc[VTIME] = 1;
	signal (SIGINT, sig_hand);
	signal (SIGQUIT, sig_hand);
	signal (SIGHUP, sig_hand);
	if (key != MT_DISPLAY)
		_ioctl (0, TCSETAW, &newtty);

	key = messagestr(key,NULL,NULL,errtext);

	if (!(option&8))
		_ioctl (0, TCSETAW, &oldtty);
	
	exit(key=='\n'?0:1);
	}

void
sig_hand()
{
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGHUP, SIG_IGN);
	_ioctl (0, TCSETAW, &oldtty);
	exit (1);
}

format(argc, argv,s,p)
int argc,s;
char **argv;
char *p;
{
	register char	*cp;
	register int	i, wd;
	int	j;

	if(--argc == 0) {
		*p++ = '\n';
		*p = '\0';
		return;
	}

	if (title_ct > 0) {
	   cp=title;
	   for ( ; *cp && (cp != argv[optind]) ; cp++) {
		*p++ = *cp;
	   }
	   *p++ = '\n';
	}

	for(i = s; i <= argc; i++) {
		for(cp = argv[i]; *cp; cp++) {
			if(*cp == '\\')
			switch(*++cp) {
				case 'b':
					*p++ = '\b';
					continue;

				case 'n':
					*p++ = '\n';
					continue;

				case 'r':
					*p++ = '\r';
					continue;

				case 't':
					*p++ = '\t';
					continue;

				case '\\':
					*p++ = '\\';
					continue;

				case '0':
					j = wd = 0;
					while ((*++cp >= '0' && *cp <= '7') && j++ < 3) {
						wd <<= 3;
						wd |= (*cp - '0');
					}
					*p++ = wd;
					--cp;
					continue;

				default:
					cp--;
			}
			*p++ = *cp;
		}
		*p++ = (i == argc? '\n': ' ');
	}
}

int
messagestr(mtype, file, title, format)

int	mtype;		/* Type of message */
char	*file;		/* Name of help file */			/* NULL */
char	*title;		/* Title of initial help screen */	/* NULL */
char	*format;
{
	char	lines [Max_lines] [Max_cols+1];
	short	lin;
	register	col;		/* Current column number */
	register char	*dptr;		/* Destination pointer */
	char	*sv_ptr;
	char	*sv_dptr;		/* Destination pointer at last break */

	short	w_id;			/* ID of message window */
	short   i;
	char	chr;

	char	looping;

/* First wrap the message to fit */

	lin = col = 0;
	sv_ptr = NULL;
	dptr = lines [lin];
	while (*format)
		{
		*dptr++ = *format;
		if (*format == '\n')
			{
			*--dptr = 0;
			format++;
			if (++lin >= Max_lines)
				break;		/* Lines buffer full */
			col = 0;
			sv_ptr = NULL;
			dptr = lines [lin];
			continue;
			}
		if (*format++ == ' ')
			{			/* Remember line breaks */
			sv_ptr = format;
			sv_dptr = dptr - 1;
			}
		if (++col >= Max_cols)
			{
			if (sv_ptr != NULL)
				{		/* Backup to last break */
				format = sv_ptr;
				dptr = sv_dptr;
				}
			*dptr = 0;
			if (++lin >= Max_lines)
				break;		/* Lines buffer full */
			col = 0;
			sv_ptr = NULL;
			dptr = lines [lin];
			}
		}
	*dptr = 0;
	if (++lin > Max_lines)
		lin = Max_lines;

	/*
	 *	Now display the message
	 */

	puts ("\n");
	switch (mtype)
		{
		case MT_HELP:
			puts (gettxt(":818", "Help\n"));
			for (i = 0; i < lin; i++)
				{
				puts ( lines [i]);
				}
			i = lin + 2;
			puts (gettxt(":819", "Strike ENTER when ready."));
			break;

		case MT_ERROR:
			puts (gettxt(":820", "System Message\n"));
			for (i = 0; i < lin; i++)
				{
				puts ( lines [i]);
				}
			i = lin + 2;
			puts (gettxt(":819","Strike ENTER when ready."));
			break;

		case MT_QUIT:
			puts (gettxt(":821", "Quit\n"));
			for (i = 0; i < lin; i++)
				{
				puts ( lines [i]);
				}
			i = lin + 2;
			puts (gettxt(":822","Strike ENTER when ready"));
			puts (gettxt(":823","or ESC to stop."));
			break;

		case MT_DISPLAY:
		case MT_POPUP:
			for (i = 0; i < lin; i++)
				{
				puts ( lines [i]);
				}
			i = lin + 2;
			break;

		case MT_CONFIRM:
			puts (gettxt(":824","Confirm\n"));
			for (i = 0; i < lin; i++)
				{
				puts ( lines [i]);
				}
			i = lin + 2;
			puts (gettxt(":822", "Strike ENTER when ready"));
			puts (gettxt(":823", "or ESC to stop."));
			break;

		case MT_INFO:
			puts (lines [0]);
			puts ("");
			for (i = 1; i < lin; i++)
				{
				puts (lines [i]);
				}
			i = lin + 1;
			puts (gettxt(":819","Strike ENTER when ready."));
			break;
		}
	/*
	 *	Now wait for user input
	 */

	fflush (stdout);
	looping = 1;
	if (mtype == MT_DISPLAY) {
		looping=0;
		chr='\n';
	}
	signal(SIGALRM, alarm_hand);
	if (setjmp(env) == 0)
		while (looping) {
			alarm(wait);
			if (read (0, &chr, 1) < 0)
				continue;
			alarm(0);
			switch (chr)
				{
				case 012:
				case 015:
					chr = '\n';
				case 033:
					looping = 0;
					break;
				default:
					if (mtype == MT_POPUP)
						looping = 0;
					else
						fputs ("\007",stdout);
					break;
				}
		}
	else
		chr = '\n';

	return (chr);

}

void
alarm_hand()
{
	longjmp(env, 1);
}
