#ident	"@(#)ksh93:src/lib/libcmd/tail.c	1.1"
#pragma prototyped
/*
 * tail [-fr] [-c number] [-n number] [file ...]
 *
 * print the head of one or more files
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 3C-526B
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   ulysses!dgk
 *
 */

static const char id[] = "\n@(#)tail (AT&T Bell Laboratories) 05/03/94\0\n";

#include <cmdlib.h>
#include <ctype.h>

#define F_FLAG		1
#define R_FLAG		2
#define N_FLAG		4
#define LINE_AVE	128

/*
 * If file is seekable, position file to tail location and return offset
 * Otherwise, return -1
 */
static off_t tailpos(register Sfio_t *fp, register long nitems, int delim)
{
	register int nleft,n;
	register off_t offset, first, last;
	if((first=sfseek(fp,(off_t)0,SEEK_CUR))<0)
		return((off_t)-1);
	last = sfsize(fp);
	if(delim < 0)
	{
		if((offset=last-nitems) < first)
			return(first);
		return(offset);
	}
	nleft = nitems;
	if((offset=last-nitems*LINE_AVE) < first)
		offset = first;
	while(offset >= first)
	{
		sfseek(fp, offset ,SEEK_SET);
		n = sfmove(fp, NiL, SF_UNBOUND, delim);
		if(n > nitems)
		{
			sfseek(fp, offset ,SEEK_SET);
			sfmove(fp, NiL, n-nitems, delim); 
			return(sftell(fp));
		}
		if(n == 0)
			offset -=  SF_BUFSIZE;
		else
		{
			nleft = nitems - n;
			n = 1 + (last-offset)/(nitems-nleft);
			offset -= (nleft+1)*n;
		}
	}
	return(first);
}

/*
 * This code handles tail from a pipe without any size limits
 */
static void pipetail(Sfio_t *infile, Sfio_t *outfile, int nitems, int delim)
{
	register Sfio_t *out;
	register int n=(2*SF_BUFSIZE), nleft=nitems, fno=0;
	off_t offset[2];
	Sfio_t *tmp[2];
	if(delim<0 && nitems < n)
		n = nitems;
	out = tmp[0] = sftmp(n);
	tmp[1] = sftmp(n);
	offset[0] = offset[1] = 0;
	while((n=sfmove(infile,out,nleft,delim)) >0)
	{
		offset[fno] = sftell(out);
		if((nleft-=n) <=0)
		{
			out = tmp[fno= !fno];
			sfseek(out, (off_t)0, SEEK_SET);
			nleft = nitems;
		}
	}
	if(nleft==nitems)
	{
		offset[fno]=0;
		fno= !fno;
	}
	sfseek(tmp[0], (off_t)0, SEEK_SET);
	/* see whether both files are needed */
	if(offset[fno])
	{
		sfseek(tmp[1], (off_t)0, SEEK_SET);
		if((n=nitems-nleft)>0) 
			sfmove(tmp[!fno], NiL, n, delim); 
		if((n=offset[!fno]-sftell(tmp[!fno])) > 0)
			sfmove(tmp[!fno], outfile, n, -1); 
	}
	else
		fno = !fno;
	sfmove(tmp[fno], outfile, offset[fno], -1); 
	sfclose(tmp[0]);
	sfclose(tmp[1]);
}

int
b_tail(int argc, char** argv)
{
	register Sfio_t *fp;
	register int n, delim='\n', flags=0;
	char *cp;
	off_t offset;
	long number = 10;

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "+fr[n:[lines]c:[chars]] [file]")) switch (n)
	{
	    case 'r':
		flags |= R_FLAG;
		break;
	    case 'f':
		flags |= F_FLAG;
		break;
	    case 'c':
		delim = -1;
		if(*opt_info.arg=='f' && opt_info.arg[1]==0)
		{
			flags |= F_FLAG;
			break;
		}
		/* Fall Thru */
	    case 'n':
		flags |= N_FLAG;
		cp = opt_info.arg;
		number = strtol(cp, &cp, 10);
		if(n=='c' && *cp=='f')
		{
			cp++;
			flags |= F_FLAG;
		}
		if(*cp)
		{
			error(2, gettxt(":309","%c requires numeric argument"),n);
			break;
		}
		if(*opt_info.arg=='+' || *opt_info.arg=='-')
			number = -number;
		if(opt_info.option[0]=='+')
			number = -number;
		break;
	    case ':':
		/* handle old style arguments */
		cp = argv[opt_info.index];
		number = strtol(cp, &cp, 10);
		if(cp!=argv[opt_info.index])
			flags |= N_FLAG;
		while(n = *cp++)
		{
			switch(n)
			{
			    case 'r':
				flags |= R_FLAG;
				continue;
			    case 'f':
				flags |= F_FLAG;
				continue;
			    case 'b':
				number *= 512;
			    case 'c':
				delim = -1;
				continue;
			    case 'l':
				delim = '\n';
				continue;
			    default:
				error(2, opt_info.arg);
				break;
			}
			break;
		}
		if(n==0)
		{
			opt_info.offset = (cp-1) - argv[opt_info.index];
			if(number==0 && !(flags&N_FLAG))
				number = (opt_info.option[0]=='-'?10:-10);
			number = -number;
		}
		break;
	    case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if(flags&R_FLAG)
	{
		if(delim<0)
			error(2,gettxt(":310","r option requires line mode"));
		else if(!(flags&N_FLAG))
			number=0;
		flags &= ~F_FLAG;
	}
	if(error_info.errors || argc>1)
		error(ERROR_usage(2),optusage(NiL));
	if(*argv)
	{
		if(streq(*argv,"-"))
			fp = sfstdin;
		else if(!(fp = sfopen(NiL,*argv,"r")))
			error(ERROR_system(3),gettxt(":27","%s: cannot open"),*argv);
	}
	else
		fp = sfstdin;
	if(number<=0)
	{
		if(number = -number)
			sfmove(fp,NiL, number, delim);
		if(flags&R_FLAG)
			rev_line(fp,sfstdout,sfseek(fp,(off_t)0,SEEK_CUR));
		else
			sfmove(fp,sfstdout,SF_UNBOUND, -1);
	}
	else
	{
		if((offset=tailpos(fp,number,delim))>=0)
		{
			if(flags&R_FLAG)
				rev_line(fp,sfstdout,offset);
			else
			{
				sfseek(fp,offset,SEEK_SET);
				sfmove(fp,sfstdout,SF_UNBOUND, -1);
			}
		}
		else
		{
			Sfio_t *out = sfstdout;
			if(flags&R_FLAG)
				out = sftmp(4*SF_BUFSIZE);
			pipetail(fp,out,number,delim);
			if(flags&R_FLAG)
			{
				sfseek(out,(off_t)0,SEEK_SET);
				rev_line(out,sfstdout,(off_t)0);
				sfclose(out);
			}
			flags = 0;
		}
	}
	if(flags&F_FLAG)
	{
		register char *bufp;
		while(1)
		{
			sfsync(sfstdout);
			sleep(1);
			if ((bufp = sfreserve(fp,0,0))&&(n=sfslen())>0)
			{
				sfwrite(sfstdout,bufp,n);
				sfread(fp,bufp,n);
			}
		}
	}
	if(fp!=sfstdin)
		sfclose(fp);
	return(0);
}
