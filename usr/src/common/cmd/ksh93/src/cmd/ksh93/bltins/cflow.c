#ident	"@(#)ksh93:src/cmd/ksh93/bltins/cflow.c	1.1"
#pragma prototyped
/*
 * break [n]
 * continue [n]
 * return [n]
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
#include	<ast.h>
#include	<error.h>
#include	<ctype.h>
#include	"shnodes.h"
#include	"builtins.h"

/*
 * return and exit
 */
int	b_ret_exit(register int n, register char *argv[], void *extra)
{
	struct checkpt *pp = (struct checkpt*)sh.jmplist;
	register char *arg;
	NOT_USED(extra);
	while((n = optget(argv,sh_optcflow))) switch(n)
	{
	    case ':':
		if(!strmatch(argv[opt_index],"[+-]+([0-9])"))
			error(2, opt_arg);
		goto done;
	    case '?':
		error(ERROR_usage(0), opt_arg);
		return(2);
	}
done:
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	pp->mode = (**argv=='e'?SH_JMPEXIT:SH_JMPFUN);
	argv += opt_index;
	n = (((arg= *argv)?atoi(arg):sh.oldexit)&SH_EXITMASK);
	/* return outside of function, dotscript and profile is exit */
	if(sh.fn_depth==0 && sh.dot_depth==0 && !sh_isstate(SH_PROFILE))
		pp->mode = SH_JMPEXIT;
	sh_exit(n);
	return(1);
}


/*
 * break and continue
 */
int	b_brk_cont(register int n, register char *argv[], void *extra)
{
	char *arg;
	register int cont= **argv=='c';
	NOT_USED(extra);
	while((n = optget(argv,sh_optcflow))) switch(n)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(0), opt_arg);
		return(2);
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	argv += opt_index;
	n=1;
	if(arg= *argv)
	{
		n = strtol(arg,&arg,10);
		if(n<=0 || *arg)
			error(ERROR_exit(1),gettxt(e_nolabels_id,e_nolabels),*argv);
	}
	if(sh.st.loopcnt)
	{
		sh.st.execbrk = sh.st.breakcnt = n;
		if(sh.st.breakcnt > sh.st.loopcnt)
			sh.st.breakcnt = sh.st.loopcnt;
		if(cont)
			sh.st.breakcnt = -sh.st.breakcnt;
	}
	return(0);
}

