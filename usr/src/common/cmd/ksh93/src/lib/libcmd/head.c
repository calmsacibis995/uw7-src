#ident	"@(#)ksh93:src/lib/libcmd/head.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * head [-c number] [-n number] [-s skip] [file ...]
 *
 * print the head of one or more files
 */

static const char id[] = "\n@(#)head (AT&T Bell Laboratories) 04/01/92\0\n";

#include <cmdlib.h>

int
b_head(int argc, register  char *argv[])
{
	static char header_fmt[] = "\n==> %s <==\n";
	register Sfio_t	*fp;
	register char		*cp;
	register long		number = 10;
	register off_t		skip = 0;
	register int		n;
	register int		delim = '\n';
	char			*format = header_fmt+1;

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, gettxt(":392", "n#[lines]c#[chars]s#[skip] [file ...]"))) switch (n)
	{
	case 'c':
		delim = -1;
		/* FALL THRU */
	case 'n':
		if((number = opt_info.num) <= 0)
			error(2, gettxt(":297","%c: %d: option requires positive number"), n, number);
		break;
	case 's':
		skip = opt_info.num;
		break;
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if(error_info.errors)
		error(ERROR_usage(2),optusage(NiL));
	if(cp = *argv)
		argv++;
	do
	{
		if(!cp || streq(cp,"-"))
			fp = sfstdin;
		else if(!(fp = sfopen(NiL,cp,"r")))
		{
			error(ERROR_system(0),gettxt(":27","%s: cannot open"),cp);
			error_info.errors = 1;
			continue;
		}
		if(argc>1)
			sfprintf(sfstdout,format,cp);
		format = header_fmt;
		if(skip>0)
			sfmove(fp,NiL,skip,delim);
		sfmove(fp, sfstdout,number,delim);
		if(fp!=sfstdin)
			sfclose(fp);
	}
	while(cp= *argv++);
	return(error_info.errors);
}

