#ident	"@(#)ksh93:src/cmd/ksh93/bltins/cd_pwd.c	1.2"
#pragma prototyped
/*
 * cd [-LP]  [dirname]
 * cd [-LP]  [old] [new]
 * pwd [-LP]
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
#include	<stak.h>
#include	<error.h>
#include	"variables.h"
#include	"path.h"
#include	"name.h"
#include	"builtins.h"
#include	<ls.h>

int	b_cd(int argc, char *argv[], void *extra)
{
	register char *dir, *cdpath="";
	register const char *dp;
	int saverrno=0;
	int rval,flag=0;
	char *oldpwd;
	Namval_t *opwdnod, *pwdnod;
	NOT_USED(extra);
	if(sh_isoption(SH_RESTRICTED))
		error(ERROR_exit(1),gettxt(":102","restricted"));
	while((rval = optget(argv,(const char *)gettxt(sh_optcd_id, sh_optcd)))) switch(rval)
	{
		case 'L':
			flag = 0;
			break;
		case 'P':
			flag = 1;
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
	dir =  argv[0];
	if(error_info.errors>0 || argc >2)
		error(ERROR_usage(2),optusage((char*)0));
	oldpwd = (char*)sh.pwd;
	opwdnod = (sh.subshell?sh_assignok(OLDPWDNOD,1):OLDPWDNOD); 
	pwdnod = (sh.subshell?sh_assignok(PWDNOD,1):PWDNOD); 
	if(argc==2)
		dir = sh_substitute(oldpwd,dir,argv[1]);
	else if(!dir || *dir==0)
		dir = nv_getval(HOME);
	else if(*dir == '-' && dir[1]==0)
		dir = nv_getval(opwdnod);
	if(!dir || *dir==0)
		if (argc==2)
			error(ERROR_exit(1),gettxt(":103","bad substitution"));
		else
			error(ERROR_exit(1),gettxt(e_direct_id,e_direct));
	if(*dir != '/')
	{
		cdpath = nv_getval(nv_scoped(CDPNOD));
		if(!oldpwd)
			oldpwd = path_pwd(1);
	}
	if(!cdpath)
		cdpath = "";
	if(*dir=='.')
	{
		/* test for pathname . ./ .. or ../ */
		if(*(dp=dir+1) == '.')
			dp++;
		if(*dp==0 || *dp=='/')
			cdpath = "";
	}
	rval = -1;
	do
	{
		dp = cdpath;
		cdpath=path_join(cdpath,dir);
		if(*stakptr(PATH_OFFSET)!='/')
		{
			char *last=(char*)stakfreeze(1);
			stakseek(PATH_OFFSET);
			stakputs(oldpwd);
			stakputc('/');
			stakputs(last+PATH_OFFSET);
			stakputc(0);
		}
		if(!flag)
		{
			register char *cp;
			stakseek(PATH_MAX+PATH_OFFSET);
#ifdef SHOPT_FS_3D
			if(!(cp = pathcanon(stakptr(PATH_OFFSET),PATH_DOTDOT)))
				continue;
			/* eliminate trailing '/' */
			while(*--cp == '/' && cp>stakptr(PATH_OFFSET))
				*cp = 0;
#else
			if(*(cp=stakptr(PATH_OFFSET))=='/')
				if(!pathcanon(cp,PATH_DOTDOT))
					continue;
#endif /* SHOPT_FS_3D */
		}
		if((rval=chdir(path_relative(stakptr(PATH_OFFSET)))) >= 0)
			goto success;
		if(errno!=ENOENT && saverrno==0)
			saverrno=errno;
	}
	while(cdpath);
	if(rval<0 && *dir=='/' && *(path_relative(stakptr(PATH_OFFSET)))!='/')
		rval = chdir(dir);
	/* use absolute chdir() if relative chdir() fails */
	if(rval<0)
	{
		if(saverrno)
			errno = saverrno;
		error(ERROR_system(1),gettxt(":104","%s:"),dir);
	}
success:
	if(dir == nv_getval(opwdnod) || argc==2)
		dp = dir;	/* print out directory for cd - */
	if(flag)
	{
		dir = stakptr(PATH_OFFSET);
		if (!(dir=pathcanon(dir,PATH_PHYSICAL)))
		{
			dir = stakptr(PATH_OFFSET);
			error(ERROR_system(1),gettxt(":104","%s:"),dir);
		}
		stakseek(dir-stakptr(0));
	}
	dir = (char*)stakfreeze(1)+PATH_OFFSET;
	if(*dp && *dp!= ':' && strchr(dir,'/'))
		sfputr(sfstdout,dir,'\n');
	if(*dir != '/')
		return(0);
	nv_putval(opwdnod,oldpwd,NV_RDONLY);
	if(oldpwd)
		free(oldpwd);
	flag = strlen(dir);
	/* delete trailing '/' */
	while(--flag>0 && dir[flag]=='/')
		dir[flag] = 0;
	nv_putval(pwdnod,dir,NV_RDONLY);
	nv_onattr(pwdnod,NV_NOFREE|NV_EXPORT);
	sh.pwd = pwdnod->nvalue.cp;
	return(0);
}

int	b_pwd(int argc, char *argv[], void *extra)
{
	register int n, flag = 0;
	register char *cp;
	NOT_USED(extra);
	NOT_USED(argc);
	while((n = optget(argv,sh_optpwd))) switch(n)
	{
		case 'L':
			flag = 0;
			break;
		case 'P':
			flag = 1;
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
	if(*(cp = path_pwd(0)) != '/')
		error(ERROR_system(1), gettxt(e_pwd_id, e_pwd));
	if(flag)
	{
#ifdef SHOPT_FS_3D
		if((flag = mount(e_dot,NIL(char*),FS3D_GET|FS3D_VIEW,0))>=0)
		{
			cp = (char*)stakseek(++flag+PATH_MAX);
			mount(e_dot,cp,FS3D_GET|FS3D_VIEW|FS3D_SIZE(flag),0);
		}
		else
#endif /* SHOPT_FS_3D */
			cp = strcpy(stakseek(strlen(cp)+PATH_MAX),cp);
		pathcanon(cp,PATH_PHYSICAL);
	}
	sfputr(sfstdout,cp,'\n');
	return(0);
}

