#ident	"@(#)ksh93:src/lib/libcmd/comm.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * comm
 */

static const char id[] = "\n@(#)comm (AT&T Bell Laboratories) 04/01/92\0\n";

#include <cmdlib.h>

#define C_FILE1		1
#define C_FILE2		2
#define C_COMMON	4
#define C_ALL		(C_FILE1|C_FILE2|C_COMMON)

static int comm(Sfio_t *in1, Sfio_t *in2, register Sfio_t *out,register int mode)
{
	register char *cp1, *cp2;
	register int n1, n2, n, comp;
	if(cp1 = sfgetr(in1,'\n',0))
		n1 = sfslen();
	if(cp2 = sfgetr(in2,'\n',0))
		n2 = sfslen();
	while(cp1 && cp2)
	{
		n=(n1<n2?n1:n2);
		if((comp=memcmp(cp1,cp2,n-1))==0 && (comp=n1-n2)==0)
		{
			if(mode&C_COMMON)
			{
				if(mode!=C_COMMON)
				{
					sfputc(out,'\t');
					if(mode==C_ALL)
						sfputc(out,'\t');
				}
				if(sfwrite(out,cp1,n) < 0)
					return(-1);
			}
			if(cp1 = sfgetr(in1,'\n',0))
				n1 = sfslen();
			if(cp2 = sfgetr(in2,'\n',0))
				n2 = sfslen();
		}
		else if(comp > 0)
		{
			if(mode&C_FILE2)
			{
				if(mode&C_FILE1)
					sfputc(out,'\t');
				if(sfwrite(out,cp2,n2) < 0)
					return(-1);
			}
			if(cp2 = sfgetr(in2,'\n',0))
				n2 = sfslen();
		}
		else
		{
			if((mode&C_FILE1) && sfwrite(out,cp1,n1) < 0)
				return(-1);
			if(cp1 = sfgetr(in1,'\n',0))
				n1 = sfslen();
		}
	}
	n = 0;
	if(cp2)
	{
		cp1 = cp2;
		in1 = in2;
		n1 = n2;
		if(mode&C_FILE1)
			n = 1;
		mode &= C_FILE2;
	}
	else
		mode &= C_FILE1;
	if(!mode || !cp1)
	{
		if(cp1 && in1==sfstdin)
			sfseek(in1,0L,SEEK_END);
		return(0);
	}
	/* process the remaining stream */
	while(1)
	{
		if(n)
			sfputc(out,'\t');
		if(sfwrite(out,cp1,n1) < 0)
			return(-1);
		if(!(cp1 = sfgetr(in1,'\n',0)))
			return(0);
		n1 = sfslen();
	}
	/* NOT REACHED */
}

int
b_comm(int argc, char *argv[])
{
	register int n;
	register int mode = C_FILE1|C_FILE2|C_COMMON;
	register char *cp;
	Sfio_t *f1, *f2;

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "123 file1 file2")) switch (n)
	{
 	    case '1':
		mode &= ~C_FILE1;
		break;
	    case '2':
		mode &= ~C_FILE2;
		break;
	    case '3':
		mode &= ~C_COMMON;
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
	if(error_info.errors || argc!=2)
		error(ERROR_usage(2),optusage(NiL));
	cp = *argv++;
	if(streq(cp,"-"))
		f1 = sfstdin;
	else if(!(f1 = sfopen(NiL, cp,"r")))
		error(ERROR_system(1),gettxt(":27","%s: cannot open"),cp);
	cp = *argv;
	if(streq(cp,"-"))
		f2 = sfstdin;
	else if(!(f2 = sfopen(NiL, cp,"r")))
		error(ERROR_system(1),gettxt(":27","%s: cannot open"),cp);
	if(mode)
	{
		if(comm(f1,f2,sfstdout,mode) < 0)
			error(ERROR_system(1),gettxt(":290"," write error"));
	}
	else if(f1==sfstdin || f2==sfstdin)
		sfseek(sfstdin,0L,SEEK_END);
	if(f1!=sfstdin)
		sfclose(f1);
	if(f2!=sfstdin)
		sfclose(f2);
	return(error_info.errors);
}
