#ident	"@(#)ksh93:src/cmd/ksh93/bltins/getopts.c	1.2"
#pragma prototyped
/*
 * getopts  optstring name [arg...]
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
#include	"variables.h"
#include	<error.h>
#include	<nval.h>
#include	"builtins.h"

int	b_getopts(int argc,char *argv[],void *extra)
{
	register char *options=error_info.context->id;
	register Namval_t *np;
	register int flag, mode, r=0;
	static char value[2], key[2];
	NOT_USED(extra);
	while((flag = optget(argv,(const char *)gettxt(sh_optgetopts_id,sh_optgetopts)))) switch(flag)
	{
	    case 'a':
		options = opt_arg;
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	argc -= opt_index;
	if(error_info.errors || argc<2)
		error(ERROR_usage(2),optusage((char*)0));
	error_info.context->flags |= ERROR_SILENT;
	error_info.id = options;
	options = argv[0];
	np = nv_open(argv[1],sh.var_tree,NV_NOASSIGN|NV_VARNAME);
	if(argc>2)
	{
		argv +=1;
		argc -=1;
	}
	else
	{
		argv = sh.st.dolv;
		argc = sh.st.dolc;
	}
	opt_index = sh.st.optindex;
	opt_char = sh.st.optchar;
	if(mode= (*options==':'))
		options++;
	switch(opt_index<=argc?(opt_num= LONG_MIN,flag=optget(argv,options)):0)
	{
	    case '?':
		if(mode==0)
		{
			struct checkpt *pp = (struct checkpt*)sh.jmplist;
			pp->mode = SH_JMPEXIT;
			error(ERROR_usage(0),opt_arg);
			sh_exit(2);
		}
		opt_option[1] = '?';
		/* FALL THRU */
	    case ':':
		key[0] = opt_option[1];
		if(strmatch(opt_arg,"*unknown*"))
			flag = '?';
		if(mode)
			opt_arg = key;
		else
		{
			error(2, opt_arg);
			opt_arg = 0;
			flag = '?';
		}
		*(options = value) = flag;
		sh.st.opterror = 1;
		if (opt_char != 0 && !argv[opt_index][opt_char]) /* jv fix */
		{
			opt_char = 0;
			opt_index++;
		}
		break;
	    case 0:
		if(sh.st.opterror)
		{
			char *com[2];
			com[0] = "-?";
			com[1] = 0;
			flag = opt_index;
			opt_index = 0;
			optget(com,options);
			opt_index = flag;
			if(!mode && strchr(options,' '))
				error(ERROR_usage(0),optusage((char*)0));
		}
		opt_arg = 0;
		options = value;
		*options = '?';
		r=1;
		opt_char = 0;
		break;
	    default:
		options = opt_option + (*opt_option!='+');
	}
	error_info.context->flags &= ~ERROR_SILENT;
	sh.st.optindex = opt_index;
	sh.st.optchar = opt_char;
	nv_putval(np, options, 0);
	nv_close(np);
	np = nv_search((char*)OPTARGNOD,sh.var_tree,NV_ADD|HASH_BUCKET|HASH_NOSCOPE);
	if(opt_num != LONG_MIN)
	{
		double d = opt_num;
		nv_putval(np, (char*)&d, NV_INTEGER|NV_RDONLY);
	}
	else
		nv_putval(np, opt_arg, NV_RDONLY);
	return(r);
}

