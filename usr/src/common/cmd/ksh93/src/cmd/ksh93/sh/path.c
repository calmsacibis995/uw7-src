#ident	"@(#)ksh93:src/cmd/ksh93/sh/path.c	1.1"
#pragma prototyped
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#include	<fcin.h>
#include	<ls.h>
#include	"variables.h"
#include	"path.h"
#include	"io.h"
#include	"jobs.h"
#include	"history.h"
#include	"test.h"
#include	"FEATURE/externs"

#define RW_ALL	(S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH)

static const char	usrbin[] = "/usr/bin";
static char		*prune(char*, const char*);
static char		*execs(const char*, const char*, char**);
static int		canexecute(char*,int);
static char		*path_to_stak(const char*);
static int		path_inpath(const char*,const char*);
static void		funload(int,const char*);
static void		exscript(char*, char*[]);
#ifdef SHOPT_VPIX
    static int		suffix_offset;
    extern char		*suffix_list[];
#endif /* SHOPT_VPIX */

static Namval_t		*tracknod;
static char		**xecenv;
static int		pruned;
static int		exec_err;

/*
 * make sure PWD is set up correctly
 * Return the present working directory
 * Invokes getcwd() if flag==0 and if necessary
 * Sets the PWD variable to this value
 */
char *path_pwd(int flag)
{
	register char *cp;
	register char *dfault = (char*)e_dot;
	register int count = 0;
	if(sh.pwd)
		return((char*)sh.pwd);
	while(1) 
	{
		/* try from lowest to highest */
		switch(count++)
		{
			case 0:
				cp = nv_getval(PWDNOD);
				break;
			case 1:
				cp = nv_getval(HOME);
				break;
			case 2:
				cp = "/";
				break;
			case 3:
				cp = (char*)e_crondir;
				if(flag) /* skip next case when non-zero flag */
					++count;
				break;
			case 4:
			{
				if(cp=getcwd(NIL(char*),0))
				{  
					nv_offattr(PWDNOD,NV_NOFREE);
					nv_unset(PWDNOD);
					PWDNOD->nvalue.cp = cp;
					goto skip;
				}
				break;
			}
			case 5:
				return(dfault);
		}
		if(cp && *cp=='/' && test_inode(cp,e_dot))
			break;
	}
	if(count>1)
	{
		nv_offattr(PWDNOD,NV_NOFREE);
		nv_putval(PWDNOD,cp,NV_RDONLY);
	}
skip:
	nv_onattr(PWDNOD,NV_NOFREE|NV_EXPORT);
	sh.pwd = (char*)(PWDNOD->nvalue.cp);
	return(cp);
}

/*
 * given <s> return a colon separated list of directories to search on the stack
 * This routine adds names to the tracked alias list, if possible, and returns
 * a reduced path string for tracked aliases
 */

char *path_get(const char *name)
/*@
 	assume name!=NULL;
	return path satisfying path!=NULL;
@*/
{
	register char *path=0;
	register char *sp = sh.lastpath;
	static int bin_is_usrbin = -1;
	if(strchr(name,'/'))
		return("");
	if(!sh_isstate(SH_DEFPATH))
		path = nv_getval(nv_scoped(PATHNOD));
	if(!path)
		path = (char*)e_defpath;
	if(bin_is_usrbin < 0)
		bin_is_usrbin = test_inode(usrbin+4,usrbin);
	if(bin_is_usrbin)
		path = path_to_stak(path); 
	else
		path = stakcopy(path);
	if(sh_isstate(SH_DEFPATH))
		return(path);
	if(sp || *name && ((tracknod=nv_search(name,sh.track_tree,0)) &&
		nv_isattr(tracknod,NV_NOALIAS)==0 &&
		(sp=nv_getval(tracknod))))
	{
		path = prune(path,sp);
		pruned++;
	}
	return(path);
}

/*
 * convert each /usr/bin to /bin and eliminate multiple /bin from <path>
 * The resulting path is placed on the stack
 */
static char *path_to_stak(register const char *path)
{
	register const char *cp=path, *base;
	register int n, nbin=0;
	stakseek(0);
	while(*cp)
	{
		while(*cp==':')
			cp++;
		base = cp;
		while((n= *cp) && n!=':')
			cp++;
		if((n=cp-base)==8 && strncmp(usrbin,base,8)==0)
			;
		else if(n!=4 || strncmp(usrbin+4,base,4))
			continue;
		if((n=base-path) > 0)
			stakwrite(path,n);
		path = cp;
		if(nbin++ == 0)
			stakwrite(usrbin+4,4);
		else
			path++;
		
	}
	if((n=cp-path) > 0)
		stakwrite(path,n);
	return(stakfreeze(1));
}

int	path_open(const char *name,char *path)
/*@
	assume name!=NULL;
 @*/
{
	register int fd;
	struct stat statb;
	if(strchr(name,'/'))
	{
		if(sh_isoption(SH_RESTRICTED))
			error(ERROR_exit(1),gettxt(e_restricted_id,e_restricted),name);
	}
	else
	{
		if(!path)
			path = (char*)e_defpath;
		path = stakcopy(path);
	}
	do
	{
		path=path_join(path,name);
		if((fd = sh_open(path_relative(stakptr(PATH_OFFSET)),O_RDONLY,0)) >= 0)
		{
			if(fstat(fd,&statb)<0 || S_ISDIR(statb.st_mode))
			{
				errno = EISDIR;
				sh_close(fd);
				fd = -1;
			}
		}
	}
	while( fd<0 && path);
	if((fd = sh_iomovefd(fd)) > 0)
	{
		fcntl(fd,F_SETFD,FD_CLOEXEC);
		sh.fdstatus[fd] |= IOCLEX;
	}
	return(fd);
}

/*
 * This routine returns 1 if first directory in <path> is also in <fpath>
 * If <path> begins with :, or first directory is ., $PWD must be in <fpath>
 * Otherwise, 0 is returned
 */

static int path_inpath(const char *path, const char *fpath)
{
	register const char *dp = fpath;
	register const char *sp = path;
	register int c, match=1;
	if(!dp || !sp || *sp==0)
		return(0);
	if(*sp==':' || (*sp=='.' && ((c=sp[1])==0 || c==':')))
		sp = path = path_pwd(1);
	while(1)
	{
		if((c= *dp++)==0 || c == ':')
		{
			if(match==1 && sp>path && (*sp==0 || *sp==':'))
				return(1);
			if(c==0)
				return(0);
			match = 1;
			sp = path;
		}
		else if(match)
		{
			if(*sp++ != c)
				match = 0;
		}
	}
	/* NOTREACHED */
}

/*
 *  set tracked alias node <np> to value <sp>
 */

void path_alias(register Namval_t *np,register char *sp)
/*@
	assume np!=NULL;
@*/
{
	if(!sp)
		nv_unset(np);
	else
	{
		register const char *vp = np->nvalue.cp;
		register int n = 1;
		register int nofree = nv_isattr(np,NV_NOFREE);
		nv_offattr(np,NV_NOPRINT);
		if(!vp || (n=strcmp(sp,vp))!=0)
		{
			int subshell = sh.subshell;
			sh.subshell = 0;
			nv_putval(np,sp,0);
			sh.subshell = subshell;
		}
		nv_setattr(np,NV_TAGGED|NV_EXPORT);
		if(nofree && n==0)
			nv_onattr(np,NV_NOFREE);
	}
}


/*
 * given a pathname return the base name
 */

char	*path_basename(register const char *name)
/*@
	assume name!=NULL;
	return x satisfying x>=name && *x!='/';
 @*/
{
	register const char *start = name;
	while (*name)
		if ((*name++ == '/') && *name)	/* don't trim trailing / */
			start = name;
	return ((char*)start);
}

/*
 * load functions from file <fno>
 */
static void funload(int fno, const char *name)
{
	char buff[IOBSIZE+1];
	int savestates = sh_isstate(~0);
	sh_onstate(SH_NOLOG|SH_NOALIAS);
	sh.readscript = (char*)name;
	sh_eval(sfnew(NIL(Sfio_t*),buff,IOBSIZE,fno,SF_READ),0);
	sh.readscript = 0;
	sh_setstate(savestates);
}

/*
 * do a path search and track alias if requested
 * if flag is 0, or if name not found, then try autoloading function
 * if flag==2, returns 1 if name found on FPATH
 * returns 1, if function was autoloaded.
 * If endpath!=NULL, Path search ends when path matches endpath.
 */

int	path_search(register const char *name,const char *endpath, int flag)
/*@
	assume name!=NULL;
	assume flag==0 || flag==1 || flag==2;
@*/
{
	register Namval_t *np;
	register int fno;
	if(flag)
	{
		/* if not found on pruned path, rehash and try again */
		while(!(sh.lastpath=path_absolute(name,endpath)) && pruned)
			nv_onattr(tracknod,NV_NOALIAS);
		if(!sh.lastpath && (np=nv_search(name,sh.fun_tree,HASH_NOSCOPE))&&np->nvalue.ip)
			return(1);
		np = 0;
	}
	if(flag==0 || !sh.lastpath)
	{
		register char *path=0;
		if(strmatch(name,e_alphanum))
			path = nv_getval(nv_scoped(FPATHNOD));
		if(path && (fno=path_open(name,path))>=0)
		{
			if(flag==2)
			{
				sh_close(fno);
				return(1);
			}
			funload(fno,name);
			if((np=nv_search(name,sh.fun_tree,HASH_NOSCOPE))&&np->nvalue.ip)
				return(1);
		}
		return(0);
	}
	else if(!sh_isstate(SH_DEFPATH))
	{
		if((np=tracknod) || ((endpath||sh_isoption(SH_TRACKALL)) &&
			(np=nv_search(name,sh.track_tree,NV_ADD)))
		  )
			path_alias(np,sh.lastpath);
	}
	return(0);
}


/*
 * do a path search and find the full pathname of file name
 * end search of path matches endpath without checking execute permission
 */

char	*path_absolute(register const char *name, const char *endpath)
/*@
	assume name!=NULL;
	return x satisfying x && *x=='/';
@*/
{
	register int	f;
	register char *path;
	register const char *fpath=0;
	register int isfun;
#ifdef SHOPT_VPIX
	char **suffix = 0;
	char *top;
#endif /* SHOPT_VPIX */
	pruned = 0;
	path = path_get(name);
	if(*path && strmatch(name,e_alphanum))
		fpath = nv_getval(nv_scoped(FPATHNOD));
	do
	{
		sh_sigcheck();
		isfun = (fpath && path_inpath(path,fpath));
#ifdef SHOPT_VPIX
		if(!suffix)
		{
			if(!isfun && path_inpath(path,nv_getval(nv_scoped(DOSPATHNOD))))
				suffix = suffix_list;
			path=path_join(path,name);
			if(suffix)
				top = stakptr(suffix_offset);
		}
		if(suffix)
		{

			strcpy(top,*suffix);
			if(**suffix==0)
				suffix = 0;
			else
				suffix++;
		}
#else
		path=path_join(path,name);
#endif /* SHOPT_VPIX */
		if(endpath && strcmp(endpath,stakptr(PATH_OFFSET))==0)
			return((char*)endpath);
		f = canexecute(stakptr(PATH_OFFSET),isfun);
		if(isfun && f>=0)
		{
			funload(f,stakptr(PATH_OFFSET));
			f = -1;
			path = 0;
		}
	}
#ifdef SHOPT_VPIX
	while(f<0 && (path||suffix));
#else
	while(f<0 && path);
#endif /* SHOPT_VPIX */
	if(f<0)
		return(0);
	/* check for relative pathname */
	if(*stakptr(PATH_OFFSET)!='/')
		path_join(path_pwd(1),(char*)stakfreeze(1)+PATH_OFFSET);
	return((char*)stakfreeze(1)+PATH_OFFSET);
}

/*
 * returns 0 if path can execute
 * sets exec_err if file is found but can't be executable
 */
#undef S_IXALL
#ifdef S_IXUSR
#   define S_IXALL	(S_IXUSR|S_IXGRP|S_IXOTH)
#else
#   ifdef S_IEXEC
#	define S_IXALL	(S_IEXEC|(S_IEXEC>>3)|(S_IEXEC>>6))
#   else
#	define S_IXALL	0111
#   endif /*S_EXEC */
#endif /* S_IXUSR */

static int canexecute(register char *path, int isfun)
/*@
	assume path!=NULL;
@*/
{
	struct stat statb;
	register int fd=0;
	path = path_relative(path);
	if(isfun)
	{
		if((fd=open(path,O_RDONLY,0))<0 || fstat(fd,&statb)<0)
			goto err;
	}
	else if(stat(path,&statb) < 0)
	{
#ifdef _WIN32
		/* check for .exe suffix */
		if(errno==ENOENT)
		{
			stakputs(".exe");
			path = stakptr(PATH_OFFSET);
			if(stat(path,&statb) < 0)
				goto err;
		}
		else
#endif /* _WIN32 */
		goto err;
	}
	errno = EPERM;
	if(S_ISDIR(statb.st_mode))
		errno = EISDIR;
	else if((statb.st_mode&S_IXALL)==S_IXALL || sh_access(path,X_OK)>=0)
		return(fd);
	if(isfun && fd>=0)
		sh_close(fd);
err:
	if(errno!=ENOENT)
		exec_err = errno;
	return(-1);
}

/*
 * Return path relative to present working directory
 */

char *path_relative(register const char* file)
/*@
	assume file!=NULL;
	return x satisfying x!=NULL;
@*/
{
	register const char *pwd;
	register const char *fp = file;
	/* can't relpath when sh.pwd not set */
	if(!(pwd=sh.pwd))
		return((char*)fp);
	while(*pwd==*fp)
	{
		if(*pwd++==0)
			return((char*)e_dot);
		fp++;
	}
	if(*pwd==0 && *fp == '/')
	{
		while(*++fp=='/');
		if(*fp)
			return((char*)fp);
		return((char*)e_dot);
	}
	return((char*)file);
}

char *path_join(register char *path,const char *name)
/*@
	assume path!=NULL;
	assume name!=NULL;
@*/
{
	/* leaves result on top of stack */
	register char *scanp = path;
	register int c;
	stakseek(PATH_OFFSET);
	if(*scanp=='.')
	{
		if((c= *++scanp)==0 || c==':')
			path = scanp;
		else if(c=='/')
			path = ++scanp;
		else
			scanp--;
	}
	while(*scanp && *scanp!=':')
		stakputc(*scanp++);
	if(scanp!=path)
	{
		stakputc('/');
		/* position past ":" unless a trailing colon after pathname */
		if(*scanp && *++scanp==0)
			scanp--;
	}
	else
		while(*scanp == ':')
			scanp++;
	path=(*scanp ? scanp : 0);
	stakputs(name);
#ifdef SHOPT_VPIX
	/* make sure that there is room for suffixes */
	suffix_offset = staktell();
	stakputs(*suffix_list);
	*stakptr(suffix_offset) = 0;
#endif /*SHOPT_VPIX */
	return((char*)path);
}


void	path_exec(register const char *arg0,register char *argv[],struct argnod *local)
/*@
	assume arg0!=NULL argv!=NULL && *argv!=NULL;
@*/
{
	register const char *path = "";
	nv_setlist(local,NV_EXPORT|NV_IDENT|NV_ASSIGN);
	xecenv=sh_envgen();
	if(strchr(arg0,'/'))
	{
		/* name containing / not allowed for restricted shell */
		if(sh_isoption(SH_RESTRICTED))
			error(ERROR_exit(1),gettxt(e_restricted_id,e_restricted),arg0);
	}
	else
		path=path_get(arg0);
	/* leave room for inserting _= pathname in environment */
	xecenv--;
	exec_err = ENOENT;
	sfsync(NIL(Sfio_t*));
	timerdel(NIL(void*));
	while(path=execs(path,arg0,argv));
	/* force an exit */
	((struct checkpt*)sh.jmplist)->mode = SH_JMPEXIT;
	if((errno = exec_err)==ENOENT)
		error(ERROR_exit(ERROR_NOENT),gettxt(e_found_id,e_found),arg0);
	else
		error(ERROR_system(ERROR_NOEXEC),gettxt(e_exec_id,e_exec),arg0);
}

/*
 * This routine constructs a short path consisting of all
 * Relative directories up to the directory of fullname <name>
 */
static char *prune(register char *path,const char *fullname)
/*@
	assume path!=NULL;
	return x satisfying x!=NULL && strlen(x)<=strlen(in path);
@*/
{
	register char *p = path;
	register char *s;
	int n = 1; 
	const char *base;
	char *inpath = path;
	if(!fullname  || *fullname != '/' || *path==0)
		return(path);
	base = path_basename(fullname);
	do
	{
		/* a null path means current directory */
		if(*path == ':')
		{
			*p++ = ':';
			path++;
			continue;
		}
		s = path;
		path=path_join(path,base);
		if(*s != '/' || (n=strcmp(stakptr(PATH_OFFSET),fullname))==0)
		{
			/* position p past end of path */
			while(*s && *s!=':')
				*p++ = *s++;
			if(n==0 || !path)
			{
				*p = 0;
				return(inpath);
			}
			*p++ = ':';
		}
	}
	while(path);
	/* if there is no match just return path */
	path = nv_getval(nv_scoped(PATHNOD));
	if(!path)
		path = (char*)e_defpath;
	strcpy(inpath,path);
	return(inpath);
}


static char *execs(const char *ap,const char *arg0,register char **argv)
/*@
	assume ap!=NULL;
	assume argv!=NULL && *argv!=NULL;
@*/
{
	register char *path, *prefix;
	sh_sigcheck();
	prefix=path_join((char*)ap,arg0);
	xecenv[0] =  stakptr(0);
	*stakptr(0) = '_';
	*stakptr(1) = '=';
	path=stakptr(PATH_OFFSET);
	sfsync(sfstderr);
#ifdef SHOPT_VPIX
	if(path_inpath(ap,nv_getval(nv_scoped(DOSPATHNOD))))
	{
		char **suffix;
		char *savet = argv[0];
		argv[0] = path;
		argv[-2] = (char*)e_vpix+1;
		argv[-1] = "-c";
		suffix = suffix_list;
		while(**suffix)
		{
			char *vp;
			strcpy(stakptr(suffix_offset),*suffix++);
			if(canexecute(path,0)>=0)
			{
				stakfreeze(1);
				if(path=nv_getval(nv_scoped(VPIXNOD)))
					stakputs(path);
				else
					stakputs(e_vpixdir);
				stakputs(e_vpix);
				execve(stakptr(0), &argv[-2] ,xecenv);
				switch(errno)
				{
				    case ENOENT:
					error(ERROR_system(ERROR_NOENT),gettxt(e_found_id,e_found),vp);
				    default:
					error(ERROR_system(ERROR_NOEXEC),gettxt(e_exec_id,e_exec),vp);
				}
			}
		}
		argv[0] = savet;
		*stakptr(suffix_offset) = 0;
	}
#endif /* SHOPT_VPIX */
	sh_sigcheck();
	path = path_relative(path);
#ifdef SHELLMAGIC
	if(*path!='/' && path!=stakptr(PATH_OFFSET))
	{
		/*
		 * The following code because execv(foo,) and execv(./foo,)
		 * may not yield the same resulst
		 */
		char *sp = (char*)malloc(strlen(path)+3);
		sp[0] = '.';
		sp[1] = '/';
		strcpy(sp+2,path);
		path = sp;
	}
#endif /* SHELLMAGIC */
	execve(path, &argv[0] ,xecenv);
#ifdef SHELLMAGIC
	if(*path=='.' && path!=stakptr(PATH_OFFSET))
	{
		free(path);
		path = path_relative(stakptr(PATH_OFFSET));
	}
#endif /* SHELLMAGIC */
	switch(errno)
	{
#ifdef apollo
	    /* 
  	     * On apollo's execve will fail with eacces when
	     * file has execute but not read permissions. So,
	     * for now we will pretend that EACCES and ENOEXEC
 	     * mean the same thing.
 	     */
	    case EACCES:
#endif /* apollo */
	    case ENOEXEC:
		exscript(path,argv);
#ifndef apollo
	    case EACCES:
	    {
		struct stat statb;
		if(stat(path,&statb)>=0 && S_ISDIR(statb.st_mode))
			errno = EISDIR;
	    }
		/* FALL THROUGH */
#endif /* !apollo */
#ifdef ENAMETOOLONG
	    case ENAMETOOLONG:
#endif /* ENAMETOOLONG */
	    case EPERM:
		exec_err = errno;
	    case ENOTDIR:
	    case ENOENT:
	    case EINTR:
#ifdef EMLINK
	    case EMLINK:
#endif /* EMLINK */
		return(prefix);
	    default:
		error(ERROR_system(ERROR_NOEXEC),gettxt(e_exec_id,e_exec),path);
	}
}

/*
 * File is executable but not machine code.
 * Assume file is a Shell script and execute it.
 */


static void exscript(register char *path,register char *argv[])
/*@
	assume path!=NULL;
	assume argv!=NULL && *argv!=NULL;
@*/
{
	register Sfio_t *sp;
	sh.comdiv=0;
	sh.bckpid = 0;
	sh.st.ioset=0;
	/* clean up any cooperating processes */
	if(sh.cpipe[0]>0)
		sh_pclose(sh.cpipe);
	if(sh.cpid)
		sh_close(*sh.outpipe);
	sh.cpid = 0;
	if(sp=fcfile())
		while(sfstack(sp,SF_POPSTACK));
	job_clear();
	if(sh.infd>0)
		sh_close(sh.infd);
	sh_setstate(SH_FORKED);
	sfsync(sfstderr);
#ifdef SHOPT_SUID_EXEC
	/* check if file cannot open for read or script is setuid/setgid  */
	{
		static char name[] = "/tmp/euidXXXXXXXXXX";
		register int n;
		register uid_t euserid;
		char *savet;
		struct stat statb;
		if((n=sh_open(path,O_RDONLY,0)) >= 0)
		{
			/* move <n> if n=0,1,2 */
			n = sh_iomovefd(n);
			if(fstat(n,&statb)>=0 && !(statb.st_mode&(S_ISUID|S_ISGID)))
				goto openok;
			sh_close(n);
		}
		if((euserid=geteuid()) != sh.userid)
		{
			strncpy(name+9,fmtbase((long)getpid(),10,0),sizeof(name)-10);
			/* create a suid open file with owner equal effective uid */
			if((n=creat(name,S_ISUID|S_IXUSR)) < 0)
				goto fail;
			unlink(name);
			/* make sure that file has right owner */
			if(fstat(n,&statb)<0 || statb.st_uid != euserid)
				goto fail;
			if(n!=10)
			{
				sh_close(10);
				fcntl(n, F_DUPFD, 10);
				sh_close(n);
				n=10;
			}
		}
		savet = *--argv;
		*argv = path;
		execve(e_suidexec,argv,xecenv);
	fail:
		/*
		 *  The following code is just for compatibility
		 */
		if((n=open(path,O_RDONLY,0)) < 0)
			error(ERROR_system(1),gettxt(e_open_id,e_open),path);
		*argv++ = savet;
	openok:
		sh.infd = n;
	}
#else
	sh.infd = sh_chkopen(path);
#endif /* SHOPT_SUID_EXEC */
#ifdef SHOPT_ACCT
	sh_accbegin(path) ;  /* reset accounting */
#endif	/* SHOPT_ACCT */
	sh_reinit(argv);
	sh.lastarg = strdup(path);
	/* save name of calling command */
	sh.readscript = error_info.id;
	if(sh.heredocs)
	{
		sfclose(sh.heredocs);
		sh.heredocs = 0;
	}
	/* close history file if name has changed */
	if(sh.hist_ptr && (path=nv_getval(HISTFILE)) && strcmp(path,sh.hist_ptr->histname))
	{
		hist_close(sh.hist_ptr);
		(HISTCUR)->nvalue.lp = 0;
	}
	sh_offstate(SH_FORKED);
	siglongjmp(*sh.jmplist,SH_JMPSCRIPT);
}

#ifdef SHOPT_ACCT
#   include <sys/acct.h>
#   include "FEATURE/time"

    static struct acct sabuf;
    static struct tms buffer;
    static clock_t	before;
    static char *SHACCT; /* set to value of SHACCT environment variable */
    static shaccton;	/* non-zero causes accounting record to be written */
    static int compress(time_t);
    /*
     *	initialize accounting, i.e., see if SHACCT variable set
     */
    void sh_accinit(void)
    {
	SHACCT = getenv("SHACCT");
    }
    /*
    * suspend accounting unitl turned on by sh_accbegin()
    */
    void sh_accsusp(void)
    {
	shaccton=0;
#ifdef AEXPAND
	sabuf.ac_flag |= AEXPND;
#endif /* AEXPAND */
    }

    /*
     * begin an accounting record by recording start time
     */
    void sh_accbegin(const char *cmdname)
    {
	if(SHACCT)
	{
		sabuf.ac_btime = time(NIL(time_t *));
		before = times(&buffer);
		sabuf.ac_uid = getuid();
		sabuf.ac_gid = getgid();
		strncpy(sabuf.ac_comm, (char*)path_basename(cmdname),
			sizeof(sabuf.ac_comm));
		shaccton = 1;
	}
    }
    /*
     * terminate an accounting record and append to accounting file
     */
    void	sh_accend(void)
    {
	int	fd;
	clock_t	after;

	if(shaccton)
	{
		after = times(&buffer);
		sabuf.ac_utime = compress(buffer.tms_utime + buffer.tms_cutime);
		sabuf.ac_stime = compress(buffer.tms_stime + buffer.tms_cstime);
		sabuf.ac_etime = compress( (time_t)(after-before));
		fd = open( SHACCT , O_WRONLY | O_APPEND | O_CREAT,RW_ALL);
		write(fd, (const char*)&sabuf, sizeof( sabuf ));
		close( fd);
	}
    }
 
    /*
     * Produce a pseudo-floating point representation
     * with 3 bits base-8 exponent, 13 bits fraction.
     */
    static int compress(register time_t t)
    {
	register int exp = 0, rund = 0;

	while (t >= 8192)
	{
		exp++;
		rund = t&04;
		t >>= 3;
	}
	if (rund)
	{
		t++;
		if (t >= 8192)
		{
			t >>= 3;
			exp++;
		}
	}
	return((exp<<13) + t);
    }
#endif	/* SHOPT_ACCT */

