#ident	"@(#)ksh93:src/lib/libcmd/dirname.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * dirname path [suffix]
 *
 * print the dirname of a pathname
 */

static const char id[] = "\n@(#)dirname (AT&T Bell Laboratories) 07/17/92\0\n";

#include <cmdlib.h>

static void dirname(register Sfio_t *outfile, register const char *pathname)
{
	register const char  *last;
	/* go to end of path */
	for(last=pathname; *last; last++);
	/* back over trailing '/' */
	while(last>pathname && *--last=='/') 1;
	/* back over non-slash chars */
	for(;last>pathname && *last!='/';last--);
	if(last==pathname)
	{
		/* all '/' or "" */
		if(*pathname!='/')
			last = pathname = ".";
		/* preserve // */
		else if(pathname[1]=='/')
			last++;
	}
	else
	{
		/* back over trailing '/' */
		for(;*last=='/' && last > pathname; last--);
		/* preserve // */
		if(last==pathname && *pathname=='/' && pathname[1]=='/')
			last++;
	}
	sfwrite(outfile,pathname,last+1-pathname);
	sfputc(outfile,'\n');
}

int
b_dirname(int argc,register char *argv[])
{
	register int n;

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, gettxt(":391", " pathname"))) switch (n)
	{
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if(error_info.errors || argc != 1)
		error(ERROR_usage(2),optusage(NiL));
	dirname(sfstdout,argv[0]);
	return(0);
}
