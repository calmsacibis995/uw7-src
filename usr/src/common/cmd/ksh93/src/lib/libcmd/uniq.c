#ident	"@(#)ksh93:src/lib/libcmd/uniq.c	1.1"
#pragma prototyped
/*
 * uniq
 *
 * Written by David Korn
 */

static const char id[] = "\n@(#)uniq (AT&T Bell Laboratories) 04/01/93\0\n";

#include <cmdlib.h>

#define C_FLAG	1
#define D_FLAG	2
#define U_FLAG	4
#define CWIDTH	4
#define MAXCNT	9999

/*
 * return a pointer to a side buffer
 */
static char *sidebuff(int size)
{
	static int maxbuff;
	static char *buff;
	register char *cp=0;
	if(size)
	{
		if(size <= maxbuff)
			return(buff);
		buff = newof(buff, char, size, 0);
	}
	else
	{
		free(buff);
		buff = 0;
	}
	maxbuff = size;
	return(buff);
}

static int uniq(Sfio_t *fdin, Sfio_t *fdout, int fields, int chars, int mode)
{
	register int n, outsize=0;
	register char *cp, *bufp, *outp;
	char *orecp, *sbufp=0, *outbuff;
	int reclen,oreclen= -1,count=0, cwidth=0;
	if(mode&C_FLAG)
		cwidth = CWIDTH+1;
	while(1)
	{
		if(cp = bufp = sfgetr(fdin,'\n',0))
		{
			if(n=fields)
			{
				while(*cp!='\n') /* skip over fields */
				{
					while(*cp==' ' || *cp=='\t')
						cp++;
					if(n-- <=0)
						break;
					while(*cp!=' ' && *cp!='\t' && *cp!='\n')
						cp++;
				}
			}
			if(chars)
				cp += chars;
			n = sfslen();
			if((reclen = n - (cp-bufp)) <=0)
			{
				reclen = 1;
				cp = bufp + sfslen()-1;
			}
		}
		else
			reclen=0;
		if(reclen==oreclen && memcmp(cp,orecp,reclen)==0)
		{
			count++;
			continue;
		}
		/* no match */
		if(outsize>0)
		{
			if(((mode&D_FLAG)&&count==0) || ((mode&U_FLAG)&&count))
			{
				if(outp!=sbufp)
					sfwrite(fdout,outp,0);
			}
			else
			{
				if(cwidth)
				{
					outp[CWIDTH] = ' ';
					if(count<MAXCNT)
					{
						sfsprintf(outp,cwidth,"%*d",CWIDTH,count+1);
						outp[CWIDTH] = ' ';
					}
					else
					{
						outsize -= (CWIDTH+1);
						if(outp!=sbufp)
						{
							if(!(sbufp=sidebuff(outsize)))
								return(1);
							memcpy(sbufp,outp+CWIDTH+1,outsize);
							sfwrite(fdout,outp,0);
							outp = sbufp;
						}
						else
							outp += CWIDTH+1;
						sfprintf(fdout,"%4d ",count+1);
					}
				}
				if(sfwrite(fdout,outp,outsize) < 0)
					return(1);
			}
		}
		if(reclen==0)
			break;
		count = 0;
		/* save current record */
		if (!(outbuff = sfreserve(fdout, 0, 0)) || (outsize = sfslen()) < 0)
			return(1);
		outp = outbuff;
		if(outsize < n+cwidth)
		{
			/* no room in outp, clear lock and use side buffer */
			sfwrite(fdout,outp,0);
			if(!(sbufp = outp=sidebuff(outsize=n+cwidth)))
				return(1);
		}
		else
			outsize = n+cwidth;
		memcpy(outp+cwidth,bufp,n);
		oreclen = reclen;
		orecp = outp+cwidth + (cp-bufp);
	}
	sidebuff(0);
	return(0);
}

int
b_uniq(int argc, char** argv)
{
	register int n, mode=0;
	register char *cp;
	int fields=0, chars=0;
	Sfio_t *fpin, *fpout;

	NoP(id[0]);
	NoP(argc);
	cmdinit(argv);
	while (n = optget(argv, "cduf#[fields]s#[chars] [infile [outfile]]")) switch (n)
	{
	    case 'c':
		mode |= C_FLAG;
		break;
	    case 'd':
		mode |= D_FLAG;
		break;
	    case 'u':
		mode |= U_FLAG;
		break;
	    case 'f':
		if(*opt_info.option=='-')
			fields = opt_info.num;
		else
			chars = opt_info.num;
		break;
	    case 's':
		chars = opt_info.num;
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
	if((cp = *argv) && (argv++,!streq(cp,"-")))
	{
		if(!(fpin = sfopen(NiL,cp,"r")))
			error(ERROR_system(1),gettxt(":27","%s: cannot open"),cp);
	}
	else
		fpin = sfstdin;
	if(cp = *argv)
	{
		argv++;
		if(!(fpout = sfopen(NiL,cp,"w")))
			error(ERROR_system(1),gettxt(":20","%s: cannot create"),cp);
	}
	else
		fpout = sfstdout;
	if(*argv)
	{
		error(2, gettxt(":314","too many arguments"));
		error(ERROR_usage(2),optusage(NiL));
	}
	error_info.errors = uniq(fpin,fpout,fields,chars,mode);
	if(fpin!=sfstdin)
		sfclose(fpin);
	if(fpout!=sfstdout)
		sfclose(fpout);
	return(error_info.errors);
}
