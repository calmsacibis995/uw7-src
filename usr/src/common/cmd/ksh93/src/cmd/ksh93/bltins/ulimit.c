#ident	"@(#)ksh93:src/cmd/ksh93/bltins/ulimit.c	1.2"
#pragma prototyped
/*
 * ulimit [-HSacdfmnstv] [limit]
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   research!dgk
 *
 */

#include	<ast.h>
#include	<sfio.h>
#include	<error.h>
#include	<shell.h>
#include	"builtins.h"
#include	"ulimit.h"

#ifdef _no_ulimit
	int	b_ulimit(int argc,char *argv[], void *extra)
	{
		NOT_USED(argc);
		NOT_USED(argv);
		NOT_USED(extra);
		error(ERROR_exit(2),gettxt(e_nosupport_id,e_nosupport));
		return(0);
	}
#else

#define HARD	1
#define SOFT	2

int	b_ulimit(int argc,char *argv[], void *extra)
{
	register char *limit;
	register int flag = 0, mode=0, n;
#ifdef _lib_getrlimit
	struct rlimit rlp;
#endif /* _lib_getrlimit */
	const Shtable_t *tp;
	int label, unit, noargs;
	long i;
	NOT_USED(extra);
	while((n = optget(argv,(const char *)gettxt(sh_optulimit_id,sh_optulimit)))) switch(n)
	{
		case 'H':
			mode |= HARD;
			continue;
		case 'S':
			mode |= SOFT;
			continue;
		case 'f':
			flag |= (1<<1);
			break;
		case 'a':
#ifdef _lib_ulimit
			flag = (1<<1);
			break;
#else
			flag = (0x2f
#   ifdef RLIMIT_RSS
			|(1<<4)
#   endif /* RLIMIT_RSS */
#   ifdef RLIMIT_NOFILE
			|(1<<6)
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
			|(1<<7)
#   endif /* RLIMIT_VMEM */
				);
			break;
		case 't':
			flag |= 1;
			break;
#   ifdef RLIMIT_RSS
		case 'm':
			flag |= (1<<4);
			break;
#   endif /* RLIMIT_RSS */
		case 'd':
			flag |= (1<<2);
			break;
		case 's':
			flag |= (1<<3);
			break;
		case 'c':
			flag |= (1<<5);
			break;
#   ifdef RLIMIT_NOFILE
		case 'n':
			flag |= (1<<6);
			break;
#   endif /* RLIMIT_NOFILE */
#   ifdef RLIMIT_VMEM
		case 'v':
			flag |= (1<<7);
			break;
#   endif /* RLIMIT_VMEM */
#endif /* _lib_ulimit */
		case ':':
			error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(2), opt_arg);
			break;
	}
	limit = argv[opt_index];
	/* default to -f */
	if(noargs=(flag==0))
		flag |= (1<<1);
	/* only one option at a time for setting */
	label = (flag&(flag-1));
	if(error_info.errors || (limit && label) || argc>opt_index+1)
		error(ERROR_usage(2),optusage((char*)0));
	tp = shtab_limits;
	if(mode==0)
		mode = (HARD|SOFT);
	for(; flag; tp++,flag>>=1)
	{
		if(!(flag&1))
			continue;
		n = tp->sh_number>>11;
		unit = tp->sh_number&0x7ff;
		if(limit)
		{
			if(sh.subshell)
				sh_subfork();
			if(strcmp(limit,e_unlimited)==0)
				i = INFINITY;
			else
			{
				char *last;
				if((i=sh_strnum(limit,&last,1)) < 0 || *last)
					error(ERROR_system(1),gettxt(e_number_id,e_number),limit);
				i *= unit;
			}
#ifdef _lib_getrlimit
			if(getrlimit(n,&rlp) <0)
				error(ERROR_system(1),gettxt(e_number_id,e_number),limit);
			if(mode&HARD)
				rlp.rlim_max = i;
			if(mode&SOFT)
				rlp.rlim_cur = i;
			if(setrlimit(n,&rlp) <0)
				error(ERROR_system(1),gettxt(e_overlimit_id,e_overlimit),limit);
#else
			if((i=vlimit(n,i)) < 0)
				error(ERROR_system(1),gettxt(e_number_id,e_number),limit);
#endif /* _lib_getrlimit */
		}
		else
		{
#ifdef  _lib_getrlimit
			if(getrlimit(n,&rlp) <0)
				error(ERROR_system(1),gettxt(e_number_id,e_number),limit);
			if(mode&HARD)
				i = rlp.rlim_max;
			if(mode&SOFT)
				i = rlp.rlim_cur;
#else
#   ifdef _lib_ulimit
			n--;
#   endif /* _lib_ulimit */
			i = -1;
			if((i=vlimit(n,i)) < 0)
				error(ERROR_system(1),gettxt(e_number_id,e_number),limit);
#endif /* _lib_getrlimit */
			if(label)
				sfputr(sfstdout,tp->sh_name,' ');
			if(i!=INFINITY || noargs)
			{
				if(!noargs)
					i += (unit-1);
				sfprintf(sfstdout,"%d\n",i/unit);
			}
			else
				sfputr(sfstdout,e_unlimited,'\n');
		}
	}
	return(0);
}
#endif /* _no_ulimit */
