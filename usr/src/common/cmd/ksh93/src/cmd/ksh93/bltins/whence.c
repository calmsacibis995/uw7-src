#ident	"@(#)ksh93:src/cmd/ksh93/bltins/whence.c	1.1.1.1"
#pragma prototyped
/*
 * command [-pvV] name [arg...]
 * whence [-afvp] name...
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
#include	"shtable.h"
#include	"name.h"
#include	"path.h"
#include	"shlex.h"
#include	"builtins.h"

extern char	*gettxt();

#define P_FLAG	1
#define V_FLAG	2
#define A_FLAG	4
#define F_FLAG	010
#define X_FLAG	020

static int whence(char**, int);

/*
 * command is called with argc==0 when checking for -V or -v option
 * In this case return 0 when -v or -V or unknown option, otherwise
 *   the shift count to the command is returned
 */
int	b_command(register int argc,char *argv[], void *extra)
{
	register int n, flags=0;
	NOT_USED(extra);
	opt_index = opt_char = 0;
	while((n = optget(argv,(const char *)gettxt(sh_optcommand_id,sh_optcommand)))) switch(n)
	{
	    case 'p':
		sh_onstate(SH_DEFPATH);
		break;
	    case 'v':
		flags |= X_FLAG;
		break;
	    case 'V':
		flags |= V_FLAG;
		break;
	    case ':':
		if(argc==0)
			return(0);
		error(2, opt_arg);
		break;
	    case '?':
		if(argc==0)
			return(0);
		error(ERROR_usage(2), opt_arg);
		break;
	}
	if(argc==0)
		return(flags?0:opt_index);
	argv += opt_index;
	if(error_info.errors || !*argv)
		error(ERROR_usage(2),optusage((char*)0));
	return(whence(argv, flags));
}

/*
 *  for the whence command
 */
int	b_whence(int argc,char *argv[], void *extra)
{
	register int flags=0, n;
	NOT_USED(argc);
	NOT_USED(extra);
	if(*argv[0]=='t')
		flags = V_FLAG;
	while((n = optget(argv,(const char *)gettxt(sh_optwhence_id,sh_optwhence)))) switch(n)
	{
	    case 'a':
		flags |= A_FLAG;
		/* FALL THRU */
	    case 'v':
		flags |= V_FLAG;
		break;
	    case 'f':
		flags |= F_FLAG;
		break;
	    case 'p':
		flags |= P_FLAG;
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	if(error_info.errors || !*argv)
		error(ERROR_usage(2),optusage((char*)0));
	return(whence(argv, flags));
}

static int whence(char **argv, register int flags)
{
	register const char *name;
	register Namval_t *np;
	register const char *cp;
	register aflag,r=0;
	register const char *msg;
	register const char *msg_id, *cp_id;
	int notrack = 1;
	while(name= *argv++)
	{
		aflag = ((flags&A_FLAG)!=0);
		cp = 0;
		np = 0;
		if(flags&P_FLAG)
			goto search;
		/* reserved words first */
		if(sh_lookup(name,shtab_reserved))
		{
			sfprintf(sfstdout,(flags&V_FLAG)?ERROR_translate(gettxt(is_reserved_id,is_reserved),1):"%s\n", name);
			if(!aflag)
				continue;
			aflag++;
		}
		/* non-tracked aliases */
		if((np=nv_search(name,sh.alias_tree,0))
			&& !nv_isnull(np) && !(notrack=nv_isattr(np,NV_TAGGED))
			&& (cp=nv_getval(np))) 
		{
			if(flags&V_FLAG)
			{
				if(nv_isattr(np,NV_EXPORT)) {
					msg = is_xalias;
					msg_id = is_xalias_id;
				} else {
					msg = is_alias;
					msg_id = is_alias_id;
				}
				sfprintf(sfstdout,ERROR_translate(gettxt(msg_id,msg),1),name, sh_fmtq(cp));
			}
			else
				sfputr(sfstdout,sh_fmtq(cp),'\n');
			if(!aflag)
				continue;
			cp = 0;
			aflag++;
		}
		/* built-ins and functions next */
		if((np=nv_search(name,sh.fun_tree,0)) && nv_isattr(np,NV_FUNCTION|NV_BLTIN))
		{
			if(is_abuiltin(np) && (cp=np->nvenv))
				goto search;
			if((flags&F_FLAG) && nv_isattr(np,NV_FUNCTION))
				if(!(np=nv_search(name,sh.bltin_tree,0)) || nv_isnull(np))
					goto search;
			cp = "%s\n";
			cp_id = ":0";
			if(flags&V_FLAG)
			{
				if(nv_isnull(np))
				{
					if(!nv_isattr(np,NV_FUNCTION))
						goto search;
					cp = is_ufunction;
					cp_id = is_ufunction_id;
				}
				else if(is_abuiltin(np)) {
					cp = is_builtin;
					cp_id = is_builtin_id;
				} else if(nv_isattr(np,NV_EXPORT)) {
					cp = is_xfunction;
					cp_id = is_xfunction_id;
				} else {
					cp = is_function;
					cp_id = is_function_id;
				}
			}
			sfprintf(sfstdout,ERROR_translate(gettxt(cp_id,cp),1),name);
			if(!aflag)
				continue;
			cp = 0;
			aflag++;
		}
	search:
		if(sh_isstate(SH_DEFPATH))
		{
			cp=0;
			notrack=1;
		}
		if(path_search(name,cp,2))
			cp = name;
		else
			cp = sh.lastpath;
		sh.lastpath = 0;
		if(cp)
		{
			if(flags&V_FLAG)
			{
				if(*cp!= '/')
				{
					if(!np || nv_isnull(np))
						sfprintf(sfstdout, ERROR_translate(gettxt(is_ufunction_id,is_ufunction),1), name);
					continue;
				}
/*
				sfputr(sfstdout,sh_fmtq(name),' ');
*/
				/* built-in version of program */
				if(np && is_abuiltin(np) && np->nvenv && strcmp(np->nvenv,cp)==0) {
					msg = is_builtver;
					msg_id = is_builtver_id;
				}
				/* tracked aliases next */
				else if(!notrack && *name == '/') {
					msg = "%s is %s";
					msg_id = ":109";
				} else {
					msg = is_talias;
					msg_id = is_talias_id;
				}
				sfprintf(sfstdout, ERROR_translate(gettxt(msg_id,msg),1), sh_fmtq(name), sh_fmtq(cp));
			}
			else
				sfputr(sfstdout,sh_fmtq(cp),'\n');
		}
		else if(aflag<=1) 
		{
			r |= 1;
			if(flags&V_FLAG) {
				sfprintf(sfstderr,ERROR_translate(gettxt(e_found_id,e_found),1),sh_fmtq(name));
				sfprintf(sfstderr,"\n");
			}
		}
	}
	return(r);
}

