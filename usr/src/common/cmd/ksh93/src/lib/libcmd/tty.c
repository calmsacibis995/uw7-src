#ident	"@(#)ksh93:src/lib/libcmd/tty.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * tty
 */

static const char id[] = "\n@(#)tty (AT&T Bell Laboratories) 04/01/92\0\n";

#include <cmdlib.h>

int
b_tty(int argc, char *argv[])
{
	register int n,sflag=0;
	register char *tty;

	NoP(argc);
	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "s ")) switch (n)
	{
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
	if(error_info.errors)
		error(ERROR_usage(2),optusage(NiL));
	if(!(tty=ttyname(0)))
	{
		tty = "not a tty";
		error_info.errors++;
	}
	if(!sflag)
		sfputr(sfstdout,tty,'\n');
	return(error_info.errors);
}
