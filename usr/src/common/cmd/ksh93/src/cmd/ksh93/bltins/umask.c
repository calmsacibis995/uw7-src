#ident	"@(#)ksh93:src/cmd/ksh93/bltins/umask.c	1.1"
#pragma prototyped
/*
 * umask [-S] [mask]
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
#include	<ctype.h>	
#include	<ls.h>	
#include	<shell.h>	
#include	"builtins.h"

int	b_umask(int argc,char *argv[], void *extra)
{
	register char *mask;
	register int flag = 0, sflag = 0;
	NOT_USED(extra);
	while((argc = optget(argv,(const char *)gettxt(sh_optumask_id,sh_optumask)))) switch(argc)
	{
		case 'S':
			sflag++;
			break;
		case ':':
			error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(2), opt_arg);
			break;
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	argv += opt_index;
	if(mask = *argv)
	{
		register int c;	
		if(isdigit(*mask))
		{
			while(c = *mask++)
			{
				if (c>='0' && c<='7')	
					flag = (flag<<3) + (c-'0');	
				else
					error(ERROR_exit(1),gettxt(e_number_id,e_number),*argv);
			}
		}
		else
		{
			char *cp = mask;
			flag = umask(0);
			c = strperm(cp,&cp,~flag);
			if(*cp)
			{
				umask(flag);
				error(ERROR_exit(1),gettxt(e_format_id,e_format),mask);
			}
			flag = (~c&0777);
		}
		umask(flag);	
	}	
	else
	{
		umask(flag=umask(0));
		if(sflag)
			sfprintf(sfstdout,"%s\n",fmtperm_pos(~flag&0777));
		else
			sfprintf(sfstdout,"%0#4o\n",flag);
	}
	return(0);
}

