#ident	"@(#)ksh93:src/lib/libcmd/fold.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * fold
 */

static const char id[] = "\n@(#)fold (AT&T Bell Laboratories) 04/01/92\0\n";

#include <cmdlib.h>

#define WIDTH	80
#define TABSIZE	8

#define T_EOF	1
#define T_NL	2
#define T_BS	3
#define T_TAB	4
#define T_SP	5
#define T_RET	6

static char cols[1<<CHAR_BIT];

static void fold(Sfio_t *in, Sfio_t *out, register int width)
{
	register char *cp, *first;
	register int n, col=0;
	register char *last_space=0;
	cols[0] = 0;
	while(cp  = sfgetr(in,'\n',0))
	{
		/* special case -b since no column adjustment is needed */ 
		if(cols['\b']==0 && (n=sfslen())<=width)
		{
			sfwrite(out,cp,n);
			continue;
		}
		first = cp;
		col = 0;
		last_space = 0;
		while(1)
		{
			while((n=cols[*(unsigned char*)cp++])==0);
			while((cp-first) > (width-col))
			{
				if(last_space)
					col = last_space - first;
				else
					col = width-col;
				sfwrite(out,first,col);
				first += col;
				col = 0;
				last_space = 0;
				if(cp>first+1 || (n!=T_NL && n!=T_BS))
					sfputc(out,'\n');
			}
			if(n==T_NL)
				break;
			switch(n)
			{
			    case T_RET:
				col = 0;
				break;
			    case T_BS:
				if((cp+(--col)-first)>0) 
					col--;
				break;
			    case T_TAB:
				n = (TABSIZE-1) - (cp+col-1-first)&(TABSIZE-1);
				col +=n;
				if((cp-first) > (width-col))
				{
					sfwrite(out,first,(--cp)-first);
					sfputc(out,'\n');
					first = cp;
					col =  TABSIZE-1;
					last_space = 0;
					continue;
				}
				if(cols[' '])
					last_space = cp;
				break;
			    case T_SP:
				last_space = cp;
				break;
			}
		}
		sfwrite(out,first,cp-first);
	}
	if(cp = sfgetr(in,'\n',-1))
		sfwrite(out,cp,sfslen());
}

int
b_fold(int argc, char *argv[])
{
	register int n, width=WIDTH;
	register Sfio_t *fp;
	register char *cp;

	NoP(id[0]);
	cmdinit(argv);
	cols['\t'] = T_TAB;
	cols['\b'] = T_BS;
	cols['\n'] = T_NL;
	cols['\r'] = T_RET;
	while (n = optget(argv, "bsw#[width] [file...]")) switch (n)
	{
	    case 'b':
		cols['\r'] = cols['\b'] = 0;
		cols['\t'] = cols[' '];
		break;
	    case 's':
		cols[' '] = T_SP;
		if(cols['\t']==0)
			cols['\t'] = T_SP;
		break;
	    case 'w':
		if((width =  opt_info.num) <= 0)
			error(2, gettxt(":296","%d: width must be positive"), opt_info.num);
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
		fold(fp,sfstdout,width);
		if(fp!=sfstdin)
			sfclose(fp);
	}
	while(cp= *argv++);
	return(error_info.errors);
}
