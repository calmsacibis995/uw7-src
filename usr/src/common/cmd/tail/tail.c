/*		copyright	"%c%" 	*/

#ident	"@(#)tail:tail.c	1.16.4.4"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/* tail command 
**
**	tail where [file]
**	where is [+|-]n[type]
**	- means n lines before end
**	+ means nth line from beginning
**	type 'b' means tail n blocks, not lines
**	type 'c' means tail n characters
**	Type 'r' means in lines in reverse order from end
**	 (for -r, default is entire buffer )
**	option 'f' means loop endlessly trying to read more
**		characters after the end of file, on the  assumption
**		that the file is growing
*/

#include	<stdio.h>
#include	<ctype.h>
#include	<sys/signal.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<string.h>
#include	<limits.h>
#include	<stdlib.h>

#ifndef	LINE_MAX
#define	LINE_MAX 2048
#endif

#define	LBIN	(LINE_MAX * 10 + 1)
#define BSHIFT	9	/* log2(512)=9, 512 is standard buffer size */

static boolean_t	oldsynopsis(char *);

struct	stat	statb;
char	bin[LBIN];
extern	int	errno;
int	follow;		/* f option flag; 0 not following, 1 following*/
int	piped;		/*
			** Indicates whether or not stdin is from a pipe;
			** 0 no, non-zero yes.
			*/

static char	cannotopen[] =
	":92:Cannot open %s: %s\n";

static char	usage[]	=
	":831:Usage:\ttail [-f] [-c number | -n number] [file]\n"
	"\t\t\ttail +/-[[number][lbc][f]] [file]\n"
	"\t\t\ttail +/-[[number][l][r|f]] [file]\n";

static char c_opt[256];		/* used to add default chars to -c on the fly */
static char defstr[] = "10";	/* default for -c */

static void c_alone(char **argv, int argc);

main(argc,argv)
char **argv;
{
	register i,j,k;
	long	n;	/*
			** Relative position in file in units (lines,
			** characters, blocks) to start output.
			*/
	long	di;
	int	fromend;/*
			** Indicates relative position in file where tail
			** starts its output; 0 indicates from the beginning,
			** 1 indicates from the end.
			*/
	int	partial;
	int	bylines;/*
			** Indicates whether or not output is by lines;
			** 0 indicates not by lines, 1 indicates by lines,
			** -1 is a precondition state.
			*/
	int	bkwds;	/*
			** r option flag indicates the order of the output;
			** 0 indicates from relative beginning of file to end,
			** 1 indicates reverse order, relative end of file to
			** relative beginning.
			*/
	int	lastnl;
	char	*p;
	char	*arg = "";

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:tail");

	lseek(0,(long)0,1);
	piped = errno==ESPIPE;
	if (argv[1])
		arg = argv[1];

	c_alone(argv + 1, argc - 1);

	if (oldsynopsis(arg) == B_TRUE) {
		fromend = *arg=='-';
		arg++;

		if (isdigit(*arg)) {
			n = strtol(arg, &p, 10);
			arg = p;
		}
		else
			n = -1;
		if (argc > 2 && (strcmp(argv[2], "--") == 0))	{
			argv++;
			argc--;
		}
		if(argc>2) {
			close(0);
			piped = 0;
			if(open(argv[2],0)!=0) {
				pfmt(stderr, MM_ERROR, cannotopen,
					argv[2], strerror(errno));
				exit(2);
			}
		}
		bylines = -1;
		bkwds = 0;
		while(*arg)
		switch(*arg++) {
		case 'b':
			if(n == -1) n = 10;
			n <<= BSHIFT;
			if(bylines!=-1 || bkwds==1) goto errcom;
			bylines=0;
			break;
		case 'c':
			if(bylines!=-1 || bkwds==1) goto errcom;
			bylines=0;
			break;
		case 'f':
			if(bkwds) goto errcom;
			follow = 1;
			break;
		case 'r':
			if(follow) goto errcom;
			if (n==-1) n = LBIN;
			if (bylines==0) goto errcom;
			bkwds = 1;
			/*
			** r option is always from relative end of file
			** regardless of sign.
			*/
			fromend = 1;
			break;
		case 'l':
			if(bylines!=-1 && bylines==1) goto errcom;
			bylines = 1;
			break;
		default:
			goto errcom;
		}
	} else {
		bylines = -1;
		bkwds = 0;
		n = -1;
		fromend = 1;
		follow = 0;

		while ((i = getopt(argc, argv, "c:fn:")) != EOF) {
			switch (i) {
			case 'c':
			case 'n':
				switch (i) {
				case 'c':
					if (bylines != -1) {
						goto errcom;
					}
					bylines = 0;
					break;
				case 'n':
					if (bylines != -1) {
						goto errcom;
					}
					bylines = 1;
					break;
				default:
					goto prtusage;
				}

				n = strtol(optarg, &p, 10);
				if (*p != '\0') {
					goto errcom;
				}

				if (isdigit(*optarg)) {
					fromend = 1;
				} else if (*optarg == '-') {
					n = -n;
					fromend = 1;
				} else if (*optarg == '+') {
					fromend = 0;
				} else {
					goto errcom;
				}
				break;
			case 'f':
				follow = 1;
				break;
			default:
				goto prtusage;
			}
		}

		if(argc > optind) {
			(void)close(0);
			piped = 0;
			if (open(argv[optind] ,0) != 0) {
				pfmt(stderr, MM_ERROR, cannotopen,
					argv[optind], strerror(errno));
				exit(2);
			}
		}
	}

	if (n == -1) n = 10;
	if(!fromend&&n>0)
		n--;
	if(bylines==-1) bylines = 1;
	if(fromend)
		goto keep;

			/*seek from beginning */

	if(bylines) {
		j = 0;
		while(n-->0) {
			do {
				if(j--<=0) {
					p = bin;
					j = read(0,p,512);
					if(j--<=0)
						fexit();
				}
			} while(*p++ != '\n');
		}
		write(1,p,j);
	} else  if(n>0) {
		if(!piped)
			fstat(0,&statb);
		if (piped || S_ISCHR(statb.st_mode)
			  || S_ISFIFO(statb.st_mode))
			while(n>0) {
				i = n>512?512:n;
				i = read(0,bin,i);
				if(i<=0)
					fexit();
				n -= i;
			}
		else
			lseek(0,n,0);
	}
copy:
	while((i=read(0,bin,512))>0)
		write(1,bin,i);
	fexit();

			/*seek from end*/

keep:
	if(n < 0)
		fexit();
	if(!piped) {
		fstat(0,&statb);
		if (!S_ISFIFO(statb.st_mode)) {
			di = !bylines&&n<LBIN?n:LBIN-1;
			if(statb.st_size > di)
				lseek(0,-di,2);
			if (!bylines) 
				goto copy;
		}
	}
	partial = 1;
	for(;;) {
		i = 0;
		do {
			j = read(0,&bin[i],LBIN-i);
			if(j<=0)
				goto brka;
			i += j;
		} while(i<LBIN);
		partial = 0;
	}
brka:
	if(!bylines) {
		k =
		    n<=i ? i-n:
		    partial ? 0:
		    n>=LBIN ? i+1:
		    i-n+LBIN;
		k--;
	} else {
		if(bkwds && bin[i==0?LBIN-1:i-1]!='\n'){	/* force trailing newline */
			bin[i]='\n';
			if(++i>=LBIN) {i = 0; partial = 0;}
		}
		k = i;
		j = 0;
		do {
			lastnl = k;
			do {
				if(--k<0) {
					if(partial) {
						if(bkwds) 
						    (void)write(1,bin,lastnl+1);
						goto brkb;
					}
					k = LBIN -1;
				}
			} while(bin[k]!='\n'&&k!=i);
			if(bkwds && j>0){
				if(k<lastnl) (void)write(1,&bin[k+1],lastnl-k);
				else {
					(void)write(1,&bin[k+1],LBIN-k-1);
					(void)write(1,bin,lastnl+1);
				}
			}
		} while(j++<n&&k!=i);
brkb:
		if (bkwds) exit(0);
		if(k==i) do {
			if(++k>=LBIN)
				k = 0;
		} while(bin[k]!='\n'&&k!=i);
	}
	if(k<i)
		write(1,&bin[k+1],i-k-1);
	else {
		write(1,&bin[k+1],LBIN-k-1);
		write(1,bin,i);
	}
	fexit();
errcom:
	pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
prtusage:
	pfmt(stderr, MM_ACTION, usage);
	exit(2);
}
fexit()
{	register int n;
	if (!follow || piped) exit(0);
	for (;;)
	{	sleep(1);
		while ((n = read (0, bin, 512)) > 0)
			write (1, bin, n);
	}
}

/*
 * This routine is needed to get around the getopt-incompatible behavior
 * with the -c option. If -c has no numeric argument, it defaults to 10.
 * Since getopt cannot handle optional option arguments, understandably,
 * we have to insert the '10', (actually a default string), to help
 * poor old getopt.
 */

static void
c_alone(char **argv, int argc)
{
	int done = 0;
	long n;
	char *opt, *new_opt, *def, *p;

	while (argc) {				/* go through each argument */
		if (argv[0][0] != '-') {	/* if this isn't an option */
			break;
		} else if (strcmp(*argv, "--") == 0) {
			break;
		}

		new_opt = c_opt;
		opt = *argv;
		while (*opt) {
			*new_opt++ = *opt;
			if (*opt++ == 'c') {
				/*
				 * if the next set of characters is not
				 * a number, or doesn't exist, then
				 * insert the default value. Else pass thru.
				 */
				if (*opt == '\0') {
					if (argc > 1) {
						n = strtol(argv[1], &p, 10);
						if (*p == '\0') {
							argv++;
							argc--;
							break;
						}
					}
				} else {
					n = strtol(opt, &p, 10);
					if (*p == '\0') {
						break;
					}
				}
				for (def = defstr; *def; def++) {
					*new_opt++ = *def;
				}
				/*
				 * copy the newly built argument into
				 * the argv array in the appropriate place.
				 */
				*argv = c_opt;
			}
		}
		argv++;
		argc--;
	}
}


static boolean_t
oldsynopsis(arg)
	register char	*arg;
{
	if (*arg == '+') {
		return (B_TRUE);
	} else if (*arg == '-') {
		arg++;
		if (*arg == 'n') {
			return (B_FALSE);
		} else if (isdigit(*arg) || (*arg == '\0')
					 || (*arg == 'l')
					 || (*arg == 'b')
					 || (*arg == 'r')) {
			return (B_TRUE);
		} else if (*arg == 'c') {
			arg++;
			if ((*arg == 'f') || (*arg == 'l')
					  || (*arg == 'b')
					  || (*arg == 'r')) {
				return (B_TRUE);
			} else {
				return (B_FALSE);
			}
		} else if (*arg == 'f') {
			arg++;
			if ((*arg == 'l') || (*arg == 'b')
					  || (*arg == 'r')) {
				return (B_TRUE);
			} else {
				return (B_FALSE);
			}
		} else {
			return (B_FALSE);
		}
	} else {
		return (B_FALSE);
	}
}
