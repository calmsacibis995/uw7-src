#ident	"@(#)ksh93:src/lib/libcmd/join.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * join
 */

static const char id[] = "\n@(#)join (AT&T Bell Laboratories) 06/01/92\0\n";

#include <cmdlib.h>

#define C_FILE1		1
#define C_FILE2		2
#define C_COMMON	4
#define C_ALL		(C_FILE1|C_FILE2|C_COMMON)
#define NFIELD		10
#define JOINFIELD	2

struct jfile
{
	Sfio_t*	iop;
	char*		recptr;
	int		reclen;
	int		field;
	int		fieldlen;
	int		nfields;
	int		maxfields;
	int		spaces;
	char**		fieldlist;
};

typedef struct 
{
	unsigned char	state[1<<CHAR_BIT];
	Sfio_t*	outfile;
	int*		outlist;
	int		outmode;
	char*		nullfield;
	int		delim;
	struct jfile	file[2];
} Join_t;

#define S_DELIM	1
#define S_SPACE	2
#define S_NL	3

static void join_free(register Join_t *jp)
{
	if(jp->outlist)
		free(jp->outlist);
	free(jp->file[0].fieldlist);
	free(jp->file[1].fieldlist);
}

static int getolist(Join_t* jp, const char *first, char** arglist)
{
	register const char *cp = first;
	char **argv = arglist;
	register int c;
	int *outptr, *outmax;
	int nfield = NFIELD;
	char *str;
	outptr = jp->outlist = (int*)malloc((NFIELD+1)*sizeof(int*));
	outmax = outptr + NFIELD;
	while(c= *cp++)
	{
		if(c==' ' || c=='\t' || c==',')
			continue;
		str = (char*)--cp;
		if(*cp=='0' && ((c=cp[1])==0 || c==' ' || c=='\t' || c==','))
		{
			str++;
			c = JOINFIELD;
			goto skip;
		}
		if(cp[1]!='.' || (*cp!='1' && *cp!='2') || (c=strtol(cp+2,&str,10)) <=0)
		{
			error(2,gettxt(":299","%s: invalid field list"),first);
			break;
		}
		c--;
		c <<=2;
		if(*cp=='2')
			c |=1;
	skip:
		if(outptr >= outmax)
		{
			jp->outlist = (int*)realloc(jp->outlist,(2*nfield+1)*sizeof(int*));
			outptr = jp->outlist + nfield;
			nfield *= 2;
			outmax = jp->outlist + nfield;
		}
		*outptr++ = c;
		cp = str;
	}
	/* need to accept obsolescent command syntax */
	while(1)
	{
		if(!(cp= *argv) || cp[1]!='.' || (*cp!='1' && *cp!='2'))
		{
			if(*cp=='0' && cp[1]==0)
			{
				c = JOINFIELD;
				goto skip2;
			}
			break;
		}
		str = (char*)cp;
		c = strtol(cp+2, &str,10);
		if(*str || --c<0)
			break;
		argv++;
		c <<= 2;
		if(*cp=='2')
			c |=1;
	skip2:
		if(outptr >= outmax)
		{
			jp->outlist = (int*)realloc(jp->outlist,(2*nfield+1)*sizeof(int*));
			outptr = jp->outlist + nfield;
			nfield *= 2;
			outmax = jp->outlist + nfield;
		}
		*outptr++ = c;
	}
	*outptr = -1;
	return(argv-arglist);
}

/*
 * reads in a record from file <index> and splits into fields
*/
static char *getrec(Join_t* jp, int index)
{
	register unsigned char *state = jp->state;
	register struct jfile *fp = &jp->file[index];
	register char *cp, **ptr = fp->fieldlist;
	register int n=0;
	register char **ptrmax = ptr + fp->maxfields;
	fp->spaces = 0;
	if(!(cp = sfgetr(fp->iop, '\n', 0)))
		return(0);
	fp->recptr = cp;
	fp->reclen = sfslen();
	if(jp->delim=='\n')	/* handle new-line delimiter specially */
	{
		*ptr++ = cp;
		cp += fp->reclen;
	}
	else while(n!=S_NL) /* separate into fields */
	{
		if(ptr >= ptrmax)	
		{
			n = 2*fp->maxfields;
			fp->fieldlist = (char**)realloc(fp->fieldlist,(n+1)*sizeof(char*));
			ptr = fp->fieldlist + fp->maxfields;
			fp->maxfields = n;
			ptrmax = fp->fieldlist+n;
		}
		*ptr++ = cp;
		if(jp->delim<=0 && state[*(unsigned char*)cp]==S_SPACE)
		{
			fp->spaces = 1;
			while(state[*(unsigned char*)cp++]==S_SPACE);
			cp--;
		}
		while((n=state[*(unsigned char*)cp++])==0);
	}
	*ptr = cp;
	fp->nfields = ptr - fp->fieldlist;
	if((n=fp->field) < fp->nfields)
	{
		cp = fp->fieldlist[n];
		/* eliminate leading spaces */
		if(fp->spaces)
		{
			while(state[*(unsigned char*)cp++]==S_SPACE);
			cp--;
		}
		fp->fieldlen = (fp->fieldlist[n+1]-cp)-1;
		return(cp);
	}
	fp->fieldlen = 0;
	return("");
}

/*
 * print field <n> from file <index>
 */
static int outfield(Join_t* jp, int index, register int n, int last)
{
	register struct jfile *fp = &jp->file[index];
	register char *cp, *cpmax;
	register int size;
	register Sfio_t *iop = jp->outfile;
	{
		if(n < fp->nfields)
		{
			cp = fp->fieldlist[n];
			cpmax = fp->fieldlist[n+1];
		}
		else
			cp = 0;
		if(last)
			n = '\n';
		else if((n=jp->delim)<=0)
		{
			if(fp->spaces)
			{
				/*eliminate leading spaces */
				while(jp->state[*(unsigned char*)cp++]==S_SPACE);
				cp--;
			}
			n = ' ';
		}
		if(cp)
			size = cpmax-cp;
		else
			size = 0;
		if(size==0)
		{
			if(!jp->nullfield)
				sfputc(iop,n);
			else if(sfputr(iop,jp->nullfield,n) < 0)
				return(-1);
		}
		else
		{
			cp[size-1] = n;
			if(sfwrite(iop,cp,size) < 0)
				return(-1);
		}
	}
	return(0);
}

static int joinout(register Join_t *jp, int mode)
{
	register struct jfile *fp;
	register int i,j,n;
	int	last,*out;
	if(out= jp->outlist)
	{
		while((n = *out++) >= 0)
		{
			if(n==JOINFIELD)
			{
				i = (mode<0?0:1);
				j = jp->file[i].field;
			}
			else
			{
				i = n&1;
				if((mode<0 && i>0) || mode>0 && i==0)
					j = jp->file[i].nfields;
				else
					j = n>>2;
			}
			if(outfield(jp, i, j, *out<0) < 0)
				return(-1);
		}
		return(0);
	}
	for(i=0; i<2; i++)
	{
		if(mode>0 && i==0)
			continue;
		fp = &jp->file[i];
		n = fp->field;
		if(mode||i==0)
		{
			/* output join field first */
			last = ((n+1)>=fp->nfields &&  mode!=0);
			if(outfield(jp,i,n,last) < 0)
				return(-1);
			if(last)
				return(0);
			for(j=0; j<n; j++)
				if(outfield(jp,i,j,0) < 0)
					return(-1);
			j= n+1;
		}
		else
			j = 0;
		for(;j<fp->nfields-1; j++)
		{
			if(j!=n && outfield(jp,i,j,0) < 0)
				return(-1);
		}
		last = mode || i==1;
		if(outfield(jp,i,j,last) < 0)
			return(-1);
		if(mode<0 && i==0)
			break;
	}
	return(0);
}

static Join_t *join_init(void)
{
	register Join_t *jp;
	static Join_t Join;
	jp = &Join;
	memzero(jp->state, sizeof(jp->state));
	jp->state[' '] = jp->state['\t'] = S_SPACE;
	jp->delim = -1;
	jp->nullfield = 0;
	jp->file[0].fieldlist = (char**)malloc((NFIELD+1)*sizeof(char*));
	jp->file[0].maxfields = NFIELD;
	jp->file[1].fieldlist = (char**)malloc((NFIELD+1)*sizeof(char*));
	jp->file[1].maxfields = NFIELD;
	jp->outmode = C_COMMON;
	return(jp);
}

static int join(Join_t *jp)
{
	register char *cp1, *cp2;
	register int n1, n2, n, comp;
	off_t offset = -1;
	int ndup=0;
	cp1 = getrec(jp,0);
	n1 = jp->file[0].fieldlen;
	cp2 = getrec(jp,1);
	n2 = jp->file[1].fieldlen;
	while(cp1 && cp2)
	{
		n=(n1<n2?n1:n2);
		if((comp=memcmp(cp1,cp2,n))==0 && (comp=n1-n2)==0)
		{
			if((jp->outmode&C_COMMON) && joinout(jp,comp) < 0)
				return(-1);
			if(!(jp->outmode&C_COMMON))
			{
				if(cp1 = getrec(jp,0))
					n1 = jp->file[0].fieldlen;
			}
			else if(offset < 0)
			{
				if((offset=sfseek(jp->file[1].iop,0L,SEEK_CUR))>=0)
					offset -= jp->file[1].reclen;
			}
			else
				ndup++;
			if(cp2 = getrec(jp,1))
			{
				n2 = jp->file[1].fieldlen;
				continue;
			}
			if(offset <0)
				continue;
			comp = -1;
		}
		if(comp>=0)
		{
			if(ndup>0)
				ndup--;
			else if((jp->outmode&C_FILE2) && joinout(jp,1) < 0)
				return(-1);
			if(cp2=getrec(jp,1))
				n2 = jp->file[1].fieldlen;
		}
		else
		{
			if(offset<0 && (jp->outmode&C_FILE1) && joinout(jp,-1) < 0)
				return(-1);
			if(cp1=getrec(jp,0))
				n1 = jp->file[0].fieldlen;
			if(offset >= 0)
			{
				if(ndup>0 && cp1)
				{
					sfseek(jp->file[1].iop,offset,SEEK_SET);
					if(cp2=getrec(jp,1))
						n2 = jp->file[1].fieldlen;
				}
				offset = -1;
			}
		}
	}
	comp = -1;
	n = 0;
	if(cp2)
	{
		comp = 1;
		cp1 = cp2;
		n1 = n2;
		n = 1;
	}
	if(!(jp->outmode&(1<<n)) || !cp1)
	{
		if(cp1 && jp->file[n].iop==sfstdin)
			sfseek(sfstdin,0L,SEEK_END);
		return(0);
	}
	/* process the remaining stream */
	while(1)
	{
		if(joinout(jp,comp) < 0)
			return(-1);
		if(!(cp1=getrec(jp,n)))
			return(0);
		n1 = jp->file[n].fieldlen;
	}
	/* NOT REACHED */
}

int
b_join(int argc, char *argv[])
{
	register int n;
	register char *cp;
	register Join_t *jp = join_init();

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "[a#[fileno]v#[fileno]]e:[string]o:[list]t:[delim]1#[field]2#[field] file1 file2")) switch (n)
	{
 	    case '1': case '2':
		if(opt_info.num <=0)
			error(2,gettxt(":300","field number must positive"));
		jp->file[n-'1'].field = (int)(opt_info.num-1);
		break;
	    case 'a': case 'v':
		if(opt_info.num!=1 && opt_info.num!=2)
			error(2,gettxt(":301","-a fileno: number must be 1 or 2"));
		jp->outmode |= 1<<(opt_info.num-1);
		if(n=='v')
			jp->outmode &= ~C_COMMON;
		break;
	    case 'e':
		jp->nullfield = opt_info.arg;
		break;
	    case 'o':
		/* need to accept obsolescent command syntax */
		n = getolist(jp, opt_info.arg, opt_info.argv+opt_info.index);
		opt_info.index += n;
		break;
	    case 't':
		jp->state[' '] = jp->state['\t'] = 0;
		n= *(unsigned char*)opt_info.arg;
		jp->state[n] = S_DELIM;
		jp->delim = n;
		break;
	    case ':':
		/* need to accept obsolescent command syntax */
		if((cp=opt_info.argv[opt_info.index]) && *cp=='-' && cp[1]=='j')
		{
			char *str;
			if((n=cp[2]) && n!='1' &&  n!='2') 
				error(2,gettxt(":302","obsolete -j<number> must be 1 or 2"));
			str = cp = opt_info.argv[opt_info.index+1];
			opt_info.index+=2;
			opt_info.offset = 0;
			opt_info.num = strtol(cp,&str,10);
			if(*str || opt_info.num<=0)
				error(2,gettxt(":303","%s: field number must be greater than 0"),cp);
			if(n)
				jp->file[n-'1'].field = (int)(opt_info.num-1);
			else
				jp->file[0].field = jp->file[1].field = (int)(opt_info.num-1);
			break;
		}
		error(2, opt_info.arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if(error_info.errors || argc!=2)
		error(ERROR_usage(2),optusage(NiL));
	cp = *argv++;
	if(streq(cp,"-"))
		jp->file[0].iop = sfstdin;
	else if(!(jp->file[0].iop = sfopen(NiL, cp, "r")))
		error(ERROR_system(1),gettxt(":27","%s: cannot open"),cp);
	cp = *argv;
	if(streq(cp,"-"))
		jp->file[1].iop = sfstdin;
	else if(!(jp->file[1].iop = sfopen(NiL, cp, "r")))
		error(ERROR_system(1),gettxt(":27","%s: cannot open"),cp);
	jp->state['\n'] = S_NL;
	jp->outfile = sfstdout;
	if(!jp->outlist)
		jp->nullfield = 0;
	if(join(jp) < 0)
		error(ERROR_system(1),gettxt(":304"," write error"));
	else if(jp->file[0].iop==sfstdin || jp->file[1].iop==sfstdin)
		sfseek(sfstdin,0L,SEEK_END);
	if(jp->file[0].iop!=sfstdin)
		sfclose(jp->file[0].iop);
	if(jp->file[1].iop!=sfstdin)
		sfclose(jp->file[1].iop);
	join_free(jp);
	return(error_info.errors);
}
