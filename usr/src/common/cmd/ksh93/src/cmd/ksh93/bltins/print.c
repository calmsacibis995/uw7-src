#ident	"@(#)ksh93:src/cmd/ksh93/bltins/print.c	1.2"
#pragma prototyped
/*
 * echo [arg...]
 * print [-nrps] [-f format] [-u filenum] [arg...]
 * printf  format [arg...]
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   research!dgk
 *
 */

#include	"defs.h"
#include	<error.h>
#include	<stak.h>
#include	"io.h"
#include	"name.h"
#include	"history.h"
#include	"builtins.h"
#include	"streval.h"

union types_t
{
	unsigned char	c;
	int		i;
	long		l;
	double		d;
	float		f;
	char		*s;
	int		*ip;
	char		**p;
};

static int		getarg(int, union types_t *);
static int		extend(char*, int, int, char**);
static char		*genformat(char*);
static int		fmtvecho(const char*);
#define fmtre(x)	(x)

static char		bsd_univ,cescape, raw, echon;
static char		**nextarg;
static const char	*options;
static const char	preformat[] = "%&%@";
static char 		*nullarg;
static int		argsize;

/*
 * Need to handle write failures to avoid locking output pool
 */
static int outexceptf(Sfio_t* iop, int mode, Sfdisc_t* dp)
{
	if(mode==SF_DPOP || mode==SF_CLOSE)
		free((void*)dp);
	else if(mode==SF_WRITE)
	{
		int save = errno;
		sfpurge(iop);
		sfpool(iop,NIL(Sfio_t*),SF_WRITE);
		errno = save;
		error(ERROR_system(1),e_badwrite,sffileno(iop));
	}
	return(0);
}

#ifndef	SHOPT_ECHOPRINT
   int    b_echo(int argc, char *argv[],void *extra)
   {
	options = sh_optecho+5;
	raw = echon = 0;
	NOT_USED(argc);
	NOT_USED(extra);
	/* This mess is because /bin/echo on BSD is different */
	if(!sh.universe)
	{
		register char *universe;
		if(universe=astconf("_AST_UNIVERSE",0,0))
			bsd_univ = (strcmp(universe,"ucb")==0);
		sh.universe = 1;
	}
	if(!bsd_univ)
		return(b_print(0,argv,0));
	options = sh_optecho;
	raw = 1;
	if(argv[1] && strcmp(argv[1],"-n")==0)
		echon = 1;
	return(b_print(0,argv+echon,0));
   }
#endif /* SHOPT_ECHOPRINT */

int    b_printf(int argc, char *argv[],void *extra)
{
	NOT_USED(argc);
	NOT_USED(extra);
	options = sh_optprintf;
	return(b_print(-1,argv,0));
}

/*
 * argc==0 when called from echo
 * argc==-1 when called from printf
 */

int    b_print(int argc, char *argv[], void *extra)
{
	register Sfio_t *outfile;
	register int n, fd = 1;
	const char *msg = e_file+4;
	char *format = 0;
	int sflag = 0, nflag, rflag;
	NOT_USED(extra);
	if(argc>0)
	{
		options = sh_optprint;
		nflag = rflag = 0;
		format = 0;
	}
	else if(argc==0)
	{
		nflag = echon;
		rflag = raw;
		argv++;
		goto skip;
	}
	while((n = optget(argv,options))) switch(n)
	{
		case 'n':
			nflag++;
			break;
		case 'p':
			fd = sh.coutpipe;
			msg = e_query;
			break;
		case 'f':
			format = opt_arg;
			break;
		case 's':
			/* print to history file */
			if(!sh_histinit())
				error(ERROR_system(1),e_history);
			fd = sffileno(sh.hist_ptr->histfp);
			sh_onstate(SH_HISTORY);
			sflag++;
			break;
		case 'e':
			rflag = 0;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'u':
			fd = (int)strtol(opt_arg,&opt_arg,10);
			if(*opt_arg)
				fd = -1;
			else if(fd<0 || fd >= sh.lim.open_max)
				fd = -1;
			else if(sh_inuse(fd) || (sh.hist_ptr && fd==sffileno(sh.hist_ptr->histfp)))
				fd = -1;
			break;
		case ':':
			/* The followin is for backward compatibility */
			if(strcmp(opt_option,"-R")==0)
			{
				rflag = 1;
				if(error_info.errors==0)
				{
					argv += opt_index+1;
					/* special case test for -Rn */
					if(strchr(argv[-1],'n'))
						nflag++;
					if(*argv && strcmp(*argv,"-n")==0)
					{

						nflag++;
						argv++;
					}
					goto skip2;
				}
			}
			else
				error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(2), opt_arg);
			break;
	}
	argv += opt_index;
	if(error_info.errors || (argc<0 && !(format = *argv++)))
		error(ERROR_usage(2),optusage((char*)0));
skip:
	if(format)
		format = genformat(format);
	/* handle special case of '-' operand for print */
	if(argc>0 && *argv && strcmp(*argv,"-")==0 && strcmp(argv[-1],"--"))
		argv++;
skip2:
	if(fd < 0)
	{
		errno = EBADF;
		n = 0;
	}
	else if(!(n=sh.fdstatus[fd]))
		n = sh_iocheckfd(fd);
	if(!(n&IOWRITE))
	{
		/* don't print error message for stdout for compatibility */
		if(fd==1)
			return(1);
		error(ERROR_system(1),msg);
	}
	if(!(outfile=sh.sftable[fd]))
	{
		Sfdisc_t *dp;
		sh_onstate(SH_NOTRACK);
		n = SF_WRITE|((n&IOREAD)?SF_READ:0);
		sh.sftable[fd] = outfile = sfnew(NIL(Sfio_t*),sh.outbuff,IOBSIZE,fd,n);
		sh_offstate(SH_NOTRACK);
		sfpool(outfile,sh.outpool,SF_WRITE);
		if(dp = new_of(Sfdisc_t,0))
		{
			dp->exceptf = outexceptf;
			dp->seekf = 0;
			dp->writef = 0;
			dp->readf = 0;
			sfdisc(outfile,dp);
		}
	}
	cescape = 0;
	/* turn off share to guarantee atomic writes for printf */
	n = sfset(outfile,SF_SHARE|SF_PUBLIC,0);
	if(format)
	{
		/* printf style print */
		Sfio_t *pool;
		sh_offstate(SH_STOPOK);
		pool=sfpool(sfstderr,NIL(Sfio_t*),SF_WRITE);
		nextarg = argv;
		do
		{
			if(sh.trapnote&SH_SIGSET)
				break;
			sfprintf(outfile,format,extend,getarg);
		}
		while(*nextarg && nextarg!=argv);
		if(nextarg == &nullarg && argsize>0)
			sfwrite(outfile,stakptr(staktell()),argsize);
		sfpool(sfstderr,pool,SF_WRITE);
	}
	else
	{
		/* echo style print */
		if(sh_echolist(outfile,rflag,argv) && !nflag)
			sfputc(outfile,'\n');
	}
	if(sflag)
	{
		hist_flush(sh.hist_ptr);
		sh_offstate(SH_HISTORY);
	}
	else if(n&SF_SHARE)
	{
		sfset(outfile,SF_SHARE|SF_PUBLIC,1);
		sfsync(outfile);
	}
	return(0);
}

/*
 * echo the argument list onto <outfile>
 * if <raw> is non-zero then \ is not a special character.
 * returns 0 for \c otherwise 1.
 */

int sh_echolist(Sfio_t *outfile, int raw, char *argv[])
{
	register char	*cp;
	register int	n;
	while(!cescape && (cp= *argv++))
	{
		if(!raw  && (n=fmtvecho(cp))>=0)
		{
			if(n)
				sfwrite(outfile,stakptr(staktell()),n);
		}
		else
			sfputr(outfile,cp,-1);
		if(*argv)
			sfputc(outfile,' ');
		sh_sigcheck();
	}
	return(!cescape);
}

/*
 * modified version of stresc for generating formats
 */
static char strformat(char *s)
{
        register char*  t;
        register int    c;
        char*           b;
        char*           p;

        b = t = s;
        for (;;)
        {
                switch (c = *s++)
                {
                case '\\':
                        c = chresc(s - 1, &p);
			if(c=='%')
				*t++ = '%';
			else if(c==0)
			{
				*t++ = '%';
				c = 'Z';
			}
                        s = p;
                        break;
                case 0:
                        *t = 0;
                        return(t - b);
                }
                *t++ = c;
        }
}


static char *genformat(char *format)
{
	register char *fp;
	stakseek(0);
	stakputs(preformat);
	stakputs(format);
	fp = (char*)stakfreeze(1);
	strformat(fp+sizeof(preformat)-1);
	return(fp);
}

static int getarg(int format,union types_t *value)
{
	register char *argp = *nextarg;
	char *lastchar = "";
	register int neg = 0;
	double d, longmin= LONG_MIN, longmax=LONG_MAX;
	if(!argp || format=='Z')
	{
		switch(format)
		{
			case 'c':
				value->c = 0;
				break;
			case 's':
			case 'q':
			case 'P':
			case 'R':
			case 'Z':
			case 'b':
				value->s = "";
				break;
			case 'f':
				value->f = 0.;
				break;
			case 'F':
				value->d = 0.;
				break;
			case 'n':
			{
				static int intvar;
				value->ip = &intvar;
				break;
			}
			default:
				value->l = 0;
		}
		return(0);
	}
	switch(format)
	{
		case 'p':
			value->p = (char**)strtol(argp,&lastchar,10);
			break;
		case 'n':
		{
			Namval_t *np;
			np = nv_open(argp,sh.var_tree,NV_VARNAME|NV_NOASSIGN|NV_ARRAY);
			nv_unset(np);
			nv_onattr(np,NV_INTEGER);
			np->nvalue.lp = new_of(long,0);
			nv_setsize(np,10);
			if(sizeof(int)==sizeof(long))
				value->ip = (int*)np->nvalue.lp;
			else
			{
				struct temp { int hi; int low; } *sp = (struct temp*)(np->nvalue.lp);
				sp->hi = 0;
				sp->low = 1;
				if(*np->nvalue.lp==1)
					value->ip = &sp->low;
				else
					value->ip = &sp->hi;
			}
			nv_close(np);
			break;
		}
		case 'q':
		case 'b':
		case 's':
		case 'P':
		case 'R':
			value->s = argp;
			break;
		case 'c':
			value->c = *argp;
			break;
		case 'u':
		case 'U':
			longmax = (unsigned long)ULONG_MAX;
		case 'd':
		case 'D':
			switch(*argp)
			{
				case '\'': case '"':
					value->l = argp[1];
					break;
				default:
					d = sh_strnum(argp,&lastchar,0);
					if(d<longmin)
					{
						error(ERROR_warn(0),e_overflow,argp);
						d = longmin;
					}
					else if(d>longmax)
					{
						error(ERROR_warn(0),e_overflow,argp);
						d = longmax;
					}
					value->l = (long)d;
					if(lastchar == *nextarg)
					{
						value->l = *argp;
						lastchar = "";
					}
			}
			if(neg)
				value->l = -value->l;
			if(sizeof(int)!=sizeof(long) && format=='d')
				value->i = (int)value->l;
			break;
		case 'f':
		case 'F':
			value->d = sh_strnum(*nextarg,&lastchar,0);
			if(sizeof(float)!=sizeof(double) && format=='f')
				value->f = (float)value->d;
			break;
		default:
			value->l = 0;
			error(ERROR_exit(1),e_formspec,format);
	}
	if(*lastchar)
		error(ERROR_warn(0),e_argtype,format);
	nextarg++;
	return(0);
}

/*
 * This routine adds new % escape sequences to printf
 */
static int extend(char *invalue,int format,int precis,char **outval)
{
	register int n;
	NOT_USED(precis);
	switch(format)
	{
		case 'Z':
			*outval = invalue;
			return(1);
		case 'b':
			if((n=fmtvecho(invalue))>=0)
			{
				if(nextarg == &nullarg)
				{
					*outval = 0;
					argsize = n;
					return(-1);
				}
				*outval = stakptr(staktell());
				return(n);
			}
			*outval = invalue;
			return(strlen(*outval));
		case 'q':
			*outval = sh_fmtq(invalue);
			return(strlen(*outval));
		case 'P':
			*outval = fmtmatch(invalue);
			if(*outval==0)
				error(ERROR_exit(1),e_badregexp,invalue);
			return(strlen(*outval));
		case 'R':
			*outval = fmtre(invalue);
			if(*outval==0)
				error(ERROR_exit(1),e_badregexp,invalue);
			return(strlen(*outval));
		default:
			*outval = 0;
			return(-1);
	}
}

/*
 * construct System V echo string out of <cp>
 * If there are not escape sequences, returns -1
 * Otherwise, puts null terminated result on stack, but doesn't freeze it
 * returns lenght of output.
 */

static int fmtvecho(const char *string)
{
	register const char *cp = string, *cpmax;
	register int c;
	register int offset = staktell();
#ifdef SHOPT_MULTIBYTE
	int chlen;
	if (MB_CUR_MAX > 1)
	{
		while(1)
		{
			if ((chlen = mblen(cp, MB_CUR_MAX)) > 1)
				/* Skip over multibyte characters */
				cp += chlen;
			else if((c= *cp++)==0 || c == '\\')
				break;
		}
	}
	else
#endif /* SHOPT_MULTIBYTE */
	while((c= *cp++) && (c!='\\'));
	if(c==0)
		return(-1);
	c = --cp - string;
	if(c>0)
		stakwrite((void*)string,c);
	for(; c= *cp; cp++)
	{
#ifdef SHOPT_MULTIBYTE
		if ((MB_CUR_MAX > 1) && ((chlen = mblen(cp, MB_CUR_MAX)) > 1))
		{
			stakwrite(cp,chlen);
			cp +=  (chlen-1);
			continue;
		}
#endif /* SHOPT_MULTIBYTE */
		if( c=='\\') switch(*++cp)
		{
			case 'E':
				c = '\033';
				break;
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'c':
				cescape++;
				nextarg = &nullarg;
				goto done;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 'v':
				c = '\v';
				break;
			case 't':
				c = '\t';
				break;
			case '\\':
				c = '\\';
				break;
			case '0':
				c = 0;
				cpmax = cp + 4;
				while(++cp<cpmax && *cp>='0' && *cp<='7')
				{
					c <<= 3;
					c |= (*cp-'0');
				}
			default:
				cp--;
		}
		stakputc(c);
	}
done:
	c = staktell()-offset;
	stakseek(offset);
		return(c);
}

