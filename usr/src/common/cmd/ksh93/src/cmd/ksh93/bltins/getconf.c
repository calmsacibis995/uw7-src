#ident	"@(#)ksh93:src/cmd/ksh93/bltins/getconf.c	1.1"
#pragma prototyped
/*
 * getconf.c
 * Written by David Korn
 * Tue November 5, 1991
 */

#include	<shell.h>
#include	<ast.h>
#include	<stak.h>
#include	<error.h>
#include	"shtable.h"
#include	"builtins.h"
#include	"FEATURE/externs"

#define next_config(p)	((Shtable_t*)((char*)(p) + sizeof(*shtab_config)))
#define MIN_LEN	20

int	b_getconf(register int argc, char *argv[])
{
	register int m,n;
	register long val;
	register const char *name, *path="";
	int offset = staktell();
	const Shtable_t *tp = shtab_config;
	error_info.id = argv[0];
	while (n = optget(argv, (const char *)gettxt(sh_optgetconf_id,sh_optgetconf))) switch (n)
	{
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	argc -= opt_index;
	if(argc >2)
		error(ERROR_usage(2),optusage((char*)0));
	while(1)
	{
		if(argc)
		{
			name = *argv;
			n = sh_lookup(name,shtab_config);
		}
		else
		{
			name = tp->sh_name;
			n = tp->sh_number;
			if(*name==0)
				break;
		}
		m = n>>2;
		errno = 0;
		val = -1;
		switch(n&03)
		{
		    case 0:
			if(m==0)
				errno = EINVAL;
			break;
		    case 1:
#ifdef _lib_confstr
			stakseek(offset+MIN_LEN);
			if((n=confstr(m,stakptr(offset),MIN_LEN)) > MIN_LEN)
			{
				stakseek(offset+n);
				confstr(m,stakptr(offset),n);
			}
#else
			if(strcmp(name,"PATH"))
				errno = EINVAL;
			else
				stakputs(e_defpath);
#endif /* _lib_confstr */
			val = 0;
			break;
	 	    case 2:
			val = sysconf(m);
			break;
		    case 3:
			if(argc==0)
				path = "/";
			else if(argc!=2)
				error(ERROR_exit(1),e_needspath,name);
			else
				path=argv[1];
			val = pathconf(path,m);
			break;
		}
		if(errno)
			error(ERROR_system(0),e_badconf,name,path);
		path= "";
		if(argc==0)
			sfputr(sfstdout,name,'=');
		if(val==-1)
			sfputr(sfstdout,"undefined",'\n');
		else if((n&03)==1)
			sfputr(sfstdout,stakptr(offset),'\n');
		else
			sfprintf(sfstdout,"%ld\n",val);
		if(argc)
			break;
		tp= next_config(tp);
		val = -1;
	}
	stakseek(offset);
	return(error_info.errors);
}

