#ident	"@(#)ksh93:src/lib/libcmd/mkfifo.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * mkfifo
 */

static const char id[] = "\n@(#)mkfifo (AT&T Bell Laboratories) 04/01/92\0\n";

#include <cmdlib.h>
#include <ls.h>

#define RWALL	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

int
b_mkfifo(int argc, char *argv[])
{
	register char *arg;
	register mode_t mode=RWALL, mask=0;
	register int n, mflag=0;

	NoP(argc);
	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "m:[mode] dir ...")) switch (n)
	{
	  case 'm':
		mflag=1;
		mode = strperm(arg=opt_info.arg,&opt_info.arg,mode);
		if(*opt_info.arg)
			error(ERROR_exit(0),gettxt(":285","%s: invalid mode"),arg);
		break;
	  case ':':
		error(2, opt_info.arg);
		break;
	  case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	if(error_info.errors || !*argv)
		error(ERROR_usage(2),optusage(NiL));
	while(arg = *argv++)
	{
		if(mkfifo(arg,mode) < 0)
			error(ERROR_system(0),gettxt(":104","%s:"),arg);
	}
	if(mask)
		umask(mask);
	return(error_info.errors!=0);
}

