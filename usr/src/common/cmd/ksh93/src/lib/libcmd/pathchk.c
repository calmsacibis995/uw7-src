#ident	"@(#)ksh93:src/lib/libcmd/pathchk.c	1.1"
#pragma prototyped
/*
 * pathchk
 *
 * Written by David Korn
 */

static const char id[] = "\n@(#)pathchk (AT&T Bell Laboratories) 11/04/92\0\n";

#include	<cmdlib.h>
#include	<ls.h>

#define isport(c)	(((c)>='a' && (c)<='z') || ((c)>='A' && (c)<='Z') || ((c)>='0' && (c)<='9') || (strchr("._-",(c))!=0) )

/*
 * call pathconf and handle unlimited sizes
 */ 
static long mypathconf(const char *path, int op)
{
	register long r;
	errno=0;
	if((r=pathconf(path, op))<0 && errno==0)
		return(LONG_MAX);
	return(r);
}

/*
 * returns 1 if <path> passes test
 */
static int pathchk(char* path, int mode)
{
	register char *cp=path, *cpold;
	register int c;
	register long r,name_max,path_max;
	if(mode)
	{
		name_max = _POSIX_NAME_MAX;
		path_max = _POSIX_PATH_MAX;
	}
	else
	{
		static char buff[2];
		name_max = path_max = 0;
		buff[0] = (*cp=='/'? '/': '.');
		if((r=mypathconf(buff, _PC_NAME_MAX)) > _POSIX_NAME_MAX)
			name_max = r;
		if((r=mypathconf(buff, _PC_PATH_MAX)) > _POSIX_PATH_MAX)
			path_max = r;
		if(*cp!='/')
		{
			if((name_max==0||path_max==0) && (cpold=getcwd((char*)0,0)))
			{
				cp = cpold + strlen(cpold);
				while(name_max==0 || path_max==0)
				{
					if(cp>cpold)
						while(--cp>cpold && *cp=='/');
					*++cp = 0;
					if(name_max==0 && (r=mypathconf(cpold, _PC_NAME_MAX)) > _POSIX_NAME_MAX)
						name_max = r;
					if(path_max==0 && (r=mypathconf(cpold, _PC_PATH_MAX)) > _POSIX_PATH_MAX)
						path_max=r;
					if(--cp==cpold)
					{
						free((void*)cpold);
						break;
					}
					while(*cp!='/')
						cp--;
				}
				cp=path;
			}
			while(*cp=='/')
				cp++;
		}
		if(name_max==0)
			name_max=_POSIX_NAME_MAX;
		if(path_max==0)
			path_max=_POSIX_PATH_MAX;
		while(*(cpold=cp))
		{
			while((c= *cp++) && c!='/');
			if((cp-cpold) > name_max)
				goto err;
			errno=0;
			cp[-1] = 0;
			r = mypathconf(path, _PC_NAME_MAX);
			if((cp[-1]=c)==0)
				cp--;
			else while(*cp=='/')
				cp++;
			if(r>=0)
				name_max=(r<_POSIX_NAME_MAX?_POSIX_NAME_MAX:r);
			else if(errno==EINVAL)
				continue;
#ifdef ENAMETOOLONG
			else if(errno==ENAMETOOLONG)
			{
				error(2,gettxt(":306","%s: pathname too long"),path);
				return(0);
			}
#endif /*ENAMETOOLONG*/
			else
				break;
		}
	}
	while(*(cpold=cp))
	{
		while((c= *cp++) && c!='/')
		{
			if(mode && !isport(c))
			{
				error(2,gettxt(":307","%s: %c not in portable character set"),path,c);
				return(0);
			}
		}
		if((cp-cpold) > name_max)
			goto err;
		if(c==0)
			break;
		while(*cp=='/')
			cp++;
	}
	if((cp-path) >= path_max)
	{
		error(2,gettxt(":306","%s: pathname too long"),path);
		return(0);
	}
	return(1);
err:
	error(2,gettxt(":308","%s: component name %.*s too long"),path,cp-cpold-1,cpold);
	return(0);
}

int
b_pathchk(int argc, char** argv)
{
	register int n, mode=0;
	register char *cp;

	NoP(argc);
	cmdinit(argv);
	while (n = optget(argv, "p pathname ...")) switch (n)
	{
  	    case 'p':
		mode = 1;
		break;
	    case ':':
		error(2, opt_info.arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	if(*argv==0 || error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	while(cp = *argv++)
	{
		if(!pathchk(cp,mode))
			error_info.errors=1;
	}
	return(error_info.errors);
}
