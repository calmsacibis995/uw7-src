#ident	"@(#)ksh93:src/lib/libcmd/paste.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * paste [-s] [-d delim] [file] ...
 *
 * paste lines from files together
 */

static const char id[] = "\n@(#)paste (AT&T Bell Laboratories) 04/01/92\0\n";

#include <cmdlib.h>

/*
 * paste the lines of the <nstreams> defined in <streams> and put results
 * to <out>
 */

static int paste(int nstream,Sfio_t* streams[],Sfio_t *out, register const char *delim,int dlen)
{
	register const char *cp;
	register int d, n, more=1;
	register Sfio_t *fp;
	do
	{
		d = (dlen>0?0:-1);
		for(n=more-1,more=0; n < nstream;)
		{
			if(fp=streams[n])
			{
				if(cp = sfgetr(fp,'\n',0))
				{
					if(n==0)
						more = 1;
					else if(!more) /* first stream with output */
					{
						if(dlen==1)
							sfnputc(out, *delim, n);
						else if(dlen>0)
						{
							for(d=n; d>dlen; d-=dlen)
								sfwrite(out,delim,dlen);
							if(d)
								sfwrite(out,delim,d);
						}
						more = n+1;
					}
					if(sfwrite(out,cp,sfslen()-((n+1)<nstream)) < 0)
						return(-1);
				}
				else
					streams[n] = 0;
			}
			if(++n<nstream && more && d>=0)
			{
				register int c;
				if(d >= dlen)
					d = 0;
				if(c=delim[d++])
					sfputc(out,c);
			}
		}
	}
	while(more);
	return(0);
}

/*
 * Handles paste -s, for file <in> to file <out> using delimiters <delim>
 */
static int spaste(Sfio_t *in,register Sfio_t* out,register const char *delim,int dlen)
{
	register const char *cp;
	register int d=0;
	if(cp = sfgetr(in,'\n',0))
	{
		if(sfwrite(out,cp,sfslen()-1) < 0)
			return(-1);
	}
	while(cp=sfgetr(in, '\n',0)) 
	{
		if(dlen)
		{
			register int c;
			if(d >= dlen)
				d = 0;
			if(c=delim[d++])
				sfputc(out,c);
		}
		if(sfwrite(out,cp,sfslen()-1) < 0)
			return(-1);
	}
	sfputc(out,'\n');
	return(0);
}

int
b_paste(int argc,register char *argv[])
{
	register int		n, sflag=0;
	register Sfio_t	*fp, **streams;
	register char 		*cp, *delim = "\t"; 	/* default delimiter */
	int			dlen;

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "d:[delim]s [file...]")) switch (n)
	{
	    case 'd':
		delim = opt_info.arg;
		break;
	    case 's':
		sflag++;
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
		error(ERROR_usage(2),optusage(NiL));
	dlen = stresc(delim);
	if(cp = *argv)
	{
		n = argc - opt_info.index;
		argv++;
	}
	else
		n = 1;
	if(!sflag)
	{
		streams = (Sfio_t**)stakalloc(n*sizeof(Sfio_t*));
		n = 0;
	}
	do
	{
		if(!cp || streq(cp,"-"))
			fp = sfstdin;
		else if(!(fp = sfopen(NiL,cp,"r")))
		{
			error(ERROR_system(0),gettxt(":27","%s: cannot open"),cp);
			error_info.errors = 1;
		}
		if(fp && sflag)
		{
			if(spaste(fp,sfstdout,delim,dlen) < 0)
			{
				error(ERROR_system(0),gettxt(":305","write failed"));
				error_info.errors = 1;
			}
			if(fp!=sfstdin)
				sfclose(fp);
		}
		else
			streams[n++] = fp;
	}
	while(cp= *argv++);
	if(!sflag)
	{
		if(error_info.errors==0 && paste(n,streams,sfstdout,delim,dlen) < 0)
		{
			error(ERROR_system(0),gettxt(":305","write failed"));
			error_info.errors = 1;
		}
		while(--n>=0)
		{
			if((fp=streams[n]) && fp!=sfstdin)
				sfclose(fp);
		}
	}
	return(error_info.errors);
}
