#ident	"@(#)ksh93:src/lib/libcmd/rev.c	1.1"
#pragma prototyped
/*
 * rev [-l] [file ...]
 *
 * reverse the characters or lines of one or more files
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 3C-526B
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   ulysses!dgk
 *
 */

static const char id[] = "\n@(#)rev (AT&T Bell Laboratories) 11/11/92\0\n";

#include	<cmdlib.h>

/*
 * reverse the characters within a line
 */
static int rev_char(Sfio_t *in, Sfio_t *out)
{
	register int c;
	register char *ep, *bp, *cp;
	register int n;
	while(cp = bp = sfgetr(in,'\n',0))
	{
		ep = bp + (n=sfslen()) -1;
		while(ep > bp)
		{
			c = *--ep;
			*ep = *bp;
			*bp++ = c;
		}
		if(sfwrite(out,cp,n)<0)
			return(-1);
	}
	return(0);
}

int
b_rev(int argc, register char** argv)
{
	register Sfio_t *fp;
	register char *cp;
	register int n, line=0;
	NOT_USED(argc);

	NoP(id[0]);
	NoP(argc);
	cmdinit(argv);
	while (n = optget(argv, "l [file ...]")) switch (n)
	{
	    case 'l':
		line=1;
		break;
	    case ':':
		error(2, opt_info.arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	n=0;
	if(cp = *argv)
		argv++;
	do
	{
		if(!cp || streq(cp,"-"))
			fp = sfstdin;
		else if(!(fp = sfopen((Sfio_t*)0,cp,"r")))
		{
			error(ERROR_system(0),gettxt(":27","%s: cannot open"),cp);
			n=1;
			continue;
		}
		if(line)
			line = rev_line(fp,sfstdout,sftell(fp));
		else
			line = rev_char(fp,sfstdout);
		if(fp!=sfstdin)
			sfclose(fp);
		if(line < 0)
			error(ERROR_system(1),gettxt(":305","write failed"));
	}
	while(cp= *argv++);
	return(n);
}
