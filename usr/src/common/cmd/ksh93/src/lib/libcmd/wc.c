#ident	"@(#)ksh93:src/lib/libcmd/wc.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * wc [-c] [-m] [-w] [-l] [file] ...
 *
 * count the number of bytes, words, and lines in a file
 */

static const char id[] = "\n@(#)wc (AT&T Bell Laboratories) 08/11/94\0\n";

#include <cmdlib.h>
#include <wc.h>
#include <ls.h>

#define ERRORMAX	125

static void printout(register Wc_t *wp, register char *name,register int mode)
{
	if(mode&WC_LINES)
		sfprintf(sfstdout," %7lu",wp->lines);
	if(mode&WC_WORDS)
		sfprintf(sfstdout," %7lu",wp->words);
	if(mode&WC_CHARS)
		sfprintf(sfstdout," %7lu",wp->chars);
	sfprintf(sfstdout," %s\n",name?name:"");
}

int
b_wc(int argc,register char **argv)
{
	register char	*cp;
	register int	mode=0, n;
	register Wc_t	*wp;
	Sfio_t		*fp;
	long		tlines=0, twords=0, tchars=0;
	struct stat	statb;

	NoP(id[0]);
	NoP(argc);
	cmdinit(argv);
	while (n = optget(argv, gettxt(":394", "lw[cm] [file...]"))) switch (n)
	{
	case 'w':
		mode |= WC_WORDS;
		break;
	case 'c':
	case 'm':
		if(mode&WC_CHARS)
		{
			if((n=='m') ^ ((mode&WC_MBYTE)!=0))
				error(2, gettxt(":315","c and m are mutually exclusive"));
		}
		mode |= WC_CHARS;
		if(n=='m')
			mode |= WC_MBYTE;
		break;
	case 'l':
		mode |= WC_LINES;
		break;
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	if (error_info.errors) error(ERROR_usage(2),optusage(NiL));
	if(!(mode&(WC_WORDS|WC_CHARS|WC_LINES)))
		mode |= (WC_WORDS|WC_CHARS|WC_LINES);
	if(!(wp = wc_init(NiL)))
		error(3,gettxt(":316","internal error"));
	if(!(mode&WC_WORDS))
	{
		memzero(wp->space, (1<<CHAR_BIT));
		wp->space['\n'] = -1;
	}
	if(cp = *argv)
		argv++;
	do
	{
		if(!cp || streq(cp,"-"))
			fp = sfstdin;
		else if(!(fp = sfopen(NiL,cp,"r")))
		{
			error(ERROR_system(0),gettxt(":27","%s: cannot open"),cp);
			continue;
		}
		if(cp)
			n++;
		if(!(mode&(WC_WORDS|WC_LINES)) && fstat(sffileno(fp),&statb)>=0
			 && S_ISREG(statb.st_mode))
		{
			wp->chars = statb.st_size - lseek(sffileno(fp),0L,1);
			lseek(sffileno(fp),0L,2);
		}
		else
			wc_count(wp, fp);
		if(fp!=sfstdin)
			sfclose(fp);
		tchars += wp->chars;
		twords += wp->words;
		tlines += wp->lines;
		printout(wp,cp,mode);
	}
	while(cp= *argv++);
	if(n>1)
	{
		wp->lines = tlines;
		wp->chars = tchars;
		wp->words = twords;
		printout(wp,"total",mode);
	}
	return(error_info.errors<ERRORMAX?error_info.errors:ERRORMAX);
}
