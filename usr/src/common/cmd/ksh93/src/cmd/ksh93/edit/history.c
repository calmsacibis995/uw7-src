#ident	"@(#)ksh93:src/cmd/ksh93/edit/history.c	1.1"
#pragma prototyped
/*
 *   History file manipulation routines
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 */

/*
 * Each command in the history file starts on an even byte is null terminated.
 * The first byte must contain the special character HIST_UNDO and the second
 * byte is the version number.  The sequence HIST_UNDO 0, following a command,
 * nullifies the previous command. A six byte sequence starting with
 * HIST_CMDNO is used to store the command number so that it is not necessary
 * to read the file from beginning to end to get to the last block of
 * commands.  This format of this sequence is different in version 1
 * then in version 0.  Version 1 allows commands to use the full 8 bit
 * character set.  It can understand version 0 format files.
 */


#define HIST_MAX	(sizeof(int)*HIST_BSIZE)
#define HIST_BIG	(0100000-1024)	/* 1K less than maximum short */
#define HIST_LINE	32		/* typical length for history line */
#define HIST_MARKSZ	6
#define HIST_RECENT	600
#define HIST_UNDO	0201		/* invalidate previous command */
#define HIST_CMDNO	0202		/* next 3 bytes give command number */
#define HIST_BSIZE	1024		/* size of history file buffer */
#define HIST_DFLT	128		/* default size of history list */

#define _HIST_PRIVATE \
	off_t	histcnt;	/* offset into history file */\
	off_t	histmarker;	/* offset of last command marker */ \
	int	histflush;	/* set if flushed outside of hflush() */\
	int	histmask;	/* power of two mask for histcnt */ \
	char	histbuff[HIST_BSIZE+1];	/* history file buffer */ \
	off_t	histcmds[2];	/* byte offset for recent commands */

#define hist_ind(hp,c)	((int)((c)&(hp)->histmask))

#include	<ast.h>
#include	<sfio.h>
#include	"FEATURE/time"
#include	<error.h>
#include	<ctype.h>
#include	<ls.h>
#ifdef KSHELL
#   include	"defs.h"
#   include	"variables.h"
#   include	"path.h"
#   include	"builtins.h"
#   include	"io.h"
#   include	"edit.h"
#   include	"sfio.h"
#endif	/* KSHELL */
#include	"history.h"
#include	"national.h"

#ifndef KSHELL
#   define new_of(type,x)	((type*)malloc((unsigned)sizeof(type)+(x)))
#   define NIL(type)		((type)0)
#   define path_relative(x)	(x)
#   ifdef __STDC__
#	define nv_getval(s)	getenv(#s)
#   else
#	define nv_getval(s)	getenv("s")
#   endif /* __STDC__ */
#   define e_unknown	 	"unknown"
#   define e_unknown_id	 	":204";
    char login_sh =		0;
    char hist_fname[] =		"/.history";
#endif	/* KSHELL */

#ifndef O_BINARY
#   define O_BINARY	0
#endif /* O_BINARY */

extern char	*gettxt();
int	_Hist;
static void	hist_marker(char*,long);
static void	hist_trim(int);
static int	hist_nearend(Sfio_t*, off_t);
static int	hist_check(int);
static int	hist_clean(int);
static int	hist_write(Sfio_t*, const void*, int, Sfdisc_t*);
static int	hist_exceptf(Sfio_t*, int, Sfdisc_t*);
static int	histflush;
static int	histinit;
static mode_t	histmode;
static int	histwfail;
static History_t *wasopen;
static History_t *hist_ptr;

#ifdef SHOPT_ACCTFILE
    static int	acctfd;
    static char *logname;
#   include <pwd.h>
    
    int  acctinit(void)
    {
	register char *cp, *acctfile;
	Namval_t *np = nv_search("ACCTFILE",sh.var_tree,0);

	if(!np || !(acctfile=nv_getval(np)))
		return(0);
	if(!(cp = getlogin()))
	{
		struct passwd *userinfo = getpwuid(getuid());
		if(userinfo)
			cp = userinfo->pw_name;
		else
			cp = "unknown";
	}
	logname = strdup(cp);

	if((acctfd=sh_open(acctfile,
		O_BINARY|O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR))>=0 &&
	    (unsigned)acctfd < 10)
	{
		int n;
		if((n = fcntl(acctfd, F_DUPFD, 10)) >= 0)
		{
			close(acctfd);
			acctfd = n;
		}
	}
	if(acctfd < 0)
	{
		acctfd = 0;
		return(0);
	}
	if(strmatch(acctfile,e_devfdNN))
	{
		char newfile[16];
		sfsprintf(newfile,sizeof(newfile),"%.8s%d\0",e_devfdNN,acctfd);
		nv_putval(np,newfile,NV_RDONLY);
	}
	else
		fcntl(acctfd,F_SETFD,FD_CLOEXEC);
	return(1);
    }
#endif /* SHOPT_ACCTFILE */

static const unsigned char hist_stamp[2] = { HIST_UNDO, HIST_VERSION };
static const Sfdisc_t hist_disc = { NULL, hist_write, NULL, hist_exceptf, NULL};

static void hist_touch(void *handle)
{
	touch((char*)handle, (time_t)0, (time_t)0, 0);
}

/*
 * open the history file
 * if HISTNAME is not given and userid==0 then no history file.
 * if login_sh and HISTFILE is longer than HIST_MAX bytes then it is
 * cleaned up.
 * hist_open() returns 1, if history file is open
 */
int  sh_histinit(void)
{
	register int fd;
	register History_t *hp;
	register char *histname;
	char *fname=0;
	int histmask, maxlines, hist_start;
	register char *cp;
	register off_t hsize = 0;

	if(sh.hist_ptr=hist_ptr)
		return(1);
	if(!(histname = nv_getval(HISTFILE)))
	{
		int offset = staktell();
		if(cp=nv_getval(HOME))
			stakputs(cp);
		stakputs(hist_fname);
		stakputc(0);
		stakseek(offset);
		histname = stakptr(offset);
	}
#ifdef future
	if(hp=wasopen)
	{
		/* reuse history file if same name */
		wasopen = 0;
		sh.hist_ptr = hist_ptr = hp;
		if(strcmp(histname,hp->histname)==0)
			return(1);
		else
			hist_free();
	}
#endif
retry:
	cp = path_relative(histname);
	if(!histinit)
		histmode = S_IRUSR|S_IWUSR;
	if((fd=open(cp,O_BINARY|O_APPEND|O_RDWR|O_CREAT,histmode))>=0)
	{
		hsize=lseek(fd,(off_t)0,SEEK_END);
	}
	if((unsigned)fd <=2)
	{
		int n;
		if((n=fcntl(fd,F_DUPFD,10))>=0)
		{
			close(fd);
			fd=n;
		}
	}
	/* make sure that file has history file format */
	if(hsize && hist_check(fd))
	{
		close(fd);
		hsize = 0;
		if(unlink(cp)>=0)
			goto retry;
		fd = -1;
	}
	if(fd < 0)
	{
#ifdef KSHELL
		/* don't allow root a history_file in /tmp */
		if(sh.userid)
#endif	/* KSHELL */
		{
			if(!(fname = pathtemp(NIL(char*),0,0)))
				return(0);
			fd = open(fname,O_BINARY|O_APPEND|O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
		}
	}
	if(fd<0)
		return(0);
	/* set the file to close-on-exec */
	fcntl(fd,F_SETFD,FD_CLOEXEC);
	if(cp=nv_getval(HISTSIZE))
		maxlines = (unsigned)atoi(cp);
	else
		maxlines = HIST_DFLT;
	for(histmask=16;histmask <= maxlines; histmask <<=1 );
	if(!(hp=new_of(History_t,(--histmask)*sizeof(off_t))))
	{
		close(fd);
		return(0);
	}
	sh.hist_ptr = hist_ptr = hp;
	hp->histsize = maxlines;
	hp->histmask = histmask;
	hp->histfp= sfnew(NIL(Sfio_t*),hp->histbuff,HIST_BSIZE,fd,SF_READ|SF_WRITE|SF_APPEND|SF_SHARE);
	memset((char*)hp->histcmds,0,sizeof(off_t)*(hp->histmask+1));
	hp->histind = 1;
	hp->histcmds[1] = 2;
	hp->histcnt = 2;
	hp->histname = strdup(histname);
	hp->histdisc = hist_disc;
	if(hsize==0)
	{
		/* put special characters at front of file */
		sfwrite(hp->histfp,(char*)hist_stamp,2);
		sfsync(hp->histfp);
	}
	/* initialize history list */
	else
	{
		int first,last;
		off_t mark,size = (HIST_MAX/4)+maxlines*HIST_LINE;
		hp->histind = first = hist_nearend(hp->histfp,hsize-size);
		hist_eof(hp);	 /* this sets histind to last command */
		if((hist_start = (last=(int)hp->histind)-maxlines) <=0)
			hist_start = 1;
		mark = hp->histmarker;
		while(first > hist_start)
		{
			size += size;
			first = hist_nearend(hp->histfp,hsize-size);
			hp->histind = first;
		}
		histinit = hist_start;
		hist_eof(hp);
		if(!histinit)
		{
			sfseek(hp->histfp,hp->histcnt=hsize,SEEK_SET);
			hp->histind = last;
			hp->histmarker = mark;
		}
		histinit = 0;
	}
	if(fname)
	{
		unlink(fname);
		free((void*)fname);
	}
	if(hist_clean(fd) && hist_start>1 && hsize > HIST_MAX)
	{
#ifdef DEBUG
		sfprintf(sfstderr,"%d: hist_trim hsize=%d\n",getpid(),hsize);
		sfsync(sfstderr);
#endif /* DEBUG */
		hist_trim((int)hp->histind-maxlines);
	}
	sfdisc(hp->histfp,&hp->histdisc);
#ifdef KSHELL
	(HISTCUR)->nvalue.lp = (&hp->histind);
#endif /* KSHELL */
	timeradd(1000L*(HIST_RECENT-30), 1, hist_touch, (void*)hp->histname);
#ifdef SHOPT_ACCTFILE
	if(sh_isstate(SH_INTERACTIVE))
		acctinit();
#endif /* SHOPT_ACCTFILE */
	return(1);
}

/*
 * close the history file and free the space
 */

void hist_close(register History_t *hp)
{
	sfclose(hp->histfp);
	free((char*)hp);
	hist_ptr = 0;
	sh.hist_ptr = 0;
#ifdef SHOPT_ACCTFILE
	if(acctfd)
	{
		close(acctfd);
		acctfd = 0;
	}
#endif /* SHOPT_ACCTFILE */
}

/*
 * check history file format to see if it begins with special byte
 */
static int hist_check(register int fd)
{
	unsigned char magic[2];
	lseek(fd,(off_t)0,SEEK_SET);
	if((read(fd,(char*)magic,2)!=2) || (magic[0]!=HIST_UNDO))
		return(1);
	return(0);
}

/*
 * clean out history file OK if not modified in HIST_RECENT seconds
 */
static int hist_clean(int fd)
{
	struct stat statb;
	return(fstat(fd,&statb)>=0 && (time((time_t*)0)-statb.st_mtime) >= HIST_RECENT);
}

/*
 * Copy the last <n> commands to a new file and make this the history file
 */

static void hist_trim(int n)
{
	register char *cp;
	register int incmd=1, c=0;
	register History_t *hist_new, *hist_old = hist_ptr;
	char *buff, *endbuff;
	off_t oldp,newp;
	struct stat statb;
	unlink(hist_old->histname);
	hist_ptr = 0;
	if(fstat(sffileno(hist_old->histfp),&statb)>=0)
	{
		histinit = 1;
		histmode =  statb.st_mode;
	}
	if(!sh_histinit())
	{
		/* use the old history file */
		hist_ptr = hist_old;
		return;
	}
	hist_new = hist_ptr;
	hist_ptr = hist_old;
	if(--n < 0)
		n = 0;
	newp = hist_seek(hist_old,++n);
	while(1)
	{
		if(!incmd)
		{
			c = hist_ind(hist_new,++hist_new->histind);
			hist_new->histcmds[c] = hist_new->histcnt;
			if(hist_new->histcnt > hist_new->histmarker+HIST_BSIZE/2)
			{
				char locbuff[HIST_MARKSZ];
				hist_marker(locbuff,hist_new->histind);
				sfwrite(hist_new->histfp,locbuff,HIST_MARKSZ);
				hist_new->histcnt += HIST_MARKSZ;
				hist_new->histmarker = hist_new->histcmds[hist_ind(hist_new,c)] = hist_new->histcnt;
			}
			oldp = newp;
			newp = hist_seek(hist_old,++n);
			if(newp <=oldp)
				break;
		}
		if(!(buff=(char*)sfreserve(hist_old->histfp,SF_UNBOUND,0)))
			break;
		*(endbuff=(cp=buff)+sfslen()) = 0;
		/* copy to null byte */
		incmd = 0;
		while(*cp++);
		if(cp > endbuff)
			incmd = 1;
		else if(*cp==0)
			cp++;
		if(cp > endbuff)
			cp = endbuff;
		c = cp-buff;
		hist_new->histcnt += c;
		sfwrite(hist_new->histfp,buff,c);
	}
	hist_ptr = hist_new;
	hist_cancel(hist_ptr);
	sfclose(hist_old->histfp);
	free((char*)hist_old);
}

/*
 * position history file at size and find next command number 
 */
static int hist_nearend(Sfio_t *iop, register off_t size)
{
        register unsigned char *cp, *endbuff;
        register int n, incmd=1;
        unsigned char *buff, marker[4];
	if(size <= 2L || sfseek(iop,size,SEEK_SET)<0)
		goto begin;
	/* skip to marker command and return the number */
	/* numbering commands occur after a null and begin with HIST_CMDNO */
        while(cp=buff=(unsigned char*)sfreserve(iop,SF_UNBOUND,1))
        {
		n = sfslen();
                *(endbuff=cp+n) = 0;
                while(1)
                {
			/* check for marker */
                        if(!incmd && *cp++==HIST_CMDNO && *cp==0)
                        {
                                n = cp+1 - buff;
                                incmd = -1;
                                break;
                        }
                        incmd = 0;
                        while(*cp++);
                        if(cp>endbuff)
                        {
                                incmd = 1;
                                break;
                        }
                        if(*cp==0 && ++cp>endbuff)
                                break;
                }
                size += n;
		sfread(iop,(char*)buff,n);
		if(incmd < 0)
                {
			if((n=sfread(iop,(char*)marker,4))==4)
			{
				n = (marker[0]<<16)|(marker[1]<<8)|marker[2];
				if(n < size/2)
				{
					hist_ptr->histmarker = hist_ptr->histcnt = size+4;
					return(n);
				}
				n=4;
			}
			if(n >0)
				size += n;
			incmd = 0;
		}
	}
begin:
	sfseek(iop,(off_t)2,SEEK_SET);
	hist_ptr->histmarker = hist_ptr->histcnt = 2L;
	return(1);
}

/*
 * This routine reads the history file from the present position
 * to the end-of-file and puts the information in the in-core
 * history table
 * Note that HIST_CMDNO is only recognized at the beginning of a command
 * and that HIST_UNDO as the first character of a command is skipped
 * unless it is followed by 0.  If followed by 0 then it cancels
 * the previous command.
 */

void hist_eof(register History_t *hp)
{
	register char *cp,*first,*endbuff;
	register int incmd = 0;
	register off_t count = hp->histcnt;
	int n,skip=0;
	char *buff;
	sfseek(hp->histfp,count,SEEK_SET);
        while(cp=buff=(char*)sfreserve(hp->histfp,SF_UNBOUND,0))
	{
		n = sfslen();
		*(endbuff = cp+n) = 0;
		first = cp += skip;
		while(1)
		{
			while(!incmd)
			{
				if(cp>first)
				{
					count += (cp-first);
					n = hist_ind(hp, ++hp->histind);
#ifdef future
					if(count==hp->histcmds[n])
					{
	sfprintf(sfstderr,"count match n=%d\n",n);
						if(histinit)
						{
							histinit = 0;
							return;
						}
					}
					else if(n>=histinit)
#endif
						hp->histcmds[n] = count;
					first = cp;
				}
				switch(*((unsigned char*)(cp++)))
				{
					case HIST_CMDNO:
						if(*cp==0)
						{
							hp->histmarker=count+2;
							cp += (HIST_MARKSZ-1);
							hp->histind--;
#ifdef future
							if(cp <= endbuff)
							{
								unsigned char *marker = (unsigned char*)(cp-4);
								int n = ((marker[0]<<16)
|(marker[1]<<8)|marker[2]);
								if((n<count/2) && n !=  (hp->histind+1))
									error(2,gettxt(":205","index=%d marker=%d"), hp->histind, n);
							}
#endif
						}
						break;
					case HIST_UNDO:
						if(*cp==0)
						{
							cp+=1;
							hp->histind-=2;
						}
						break;
					default:
						cp--;
						incmd = 1;
				}
				if(cp > endbuff)
				{
					cp++;
					goto refill;
				}
			}
			first = cp;
			while(*cp++);
			if(cp > endbuff)
				break;
			incmd = 0;
			while(*cp==0)
			{
				if(++cp > endbuff)
					goto refill;
			}
		}
	refill:
		count += (--cp-first);
		skip = (cp-endbuff);
		if(!incmd && !skip)
			hp->histcmds[hist_ind(hp,++hp->histind)] = count;
	}
	hp->histcnt = count;
}

/*
 * This routine will cause the previous command to be cancelled
 */

void hist_cancel(register History_t *hp)
{
	register int c;
	if(!hp)
		return;
	sfputc(hp->histfp,HIST_UNDO);
	sfputc(hp->histfp,0);
	sfsync(hp->histfp);
	hp->histcnt += 2;
	c = hist_ind(hp,--hp->histind);
	hp->histcmds[c] = hp->histcnt;
}

/*
 * flush the current history command
 */

void hist_flush(register History_t *hp)
{
	register char *buff;
	if(hp)
	{
		if(buff=(char*)sfreserve(hp->histfp,0,1))
		{
			histflush = sfslen()+1;
			sfwrite(hp->histfp,buff,0);
		}
		else
			histflush=0;
		if(sfsync(hp->histfp)<0)
		{
			hist_close(hp);
			if(!sh_histinit())
				sh_offoption(SH_HISTORY);
		}
		histflush = 0;
	}
}

/*
 * This is the write discipline for the history file
 * When called from hist_flush(), trailing newlines are deleted and
 * a zero byte.  Line sequencing is added as required
 */

static int hist_write(Sfio_t *iop,const void *buff,register int insize,Sfdisc_t* handle)
{
	register char *bufptr = ((char*)buff)+insize;
	register History_t *hp = hist_ptr;
	register int c,size = insize;
	register off_t cur;
	NOT_USED(handle);
	if(!histflush)
		return(write(sffileno(iop),(char*)buff,size));
	if((cur = lseek(sffileno(iop),(off_t)0,SEEK_END)) <0)
	{
		error(2,gettxt(":206","hist_flush: EOF seek failed errno=%d"),errno);
		return(-1);
	}
	hp->histcnt = cur;
	/* remove whitespace from end of commands */
	while(--bufptr >= (char*)buff)
	{
		c= *bufptr;
		if(!isspace(c))
		{
			if(c=='\\' && *(bufptr+1)!='\n')
				bufptr++;
			break;
		}
	}
	/* don't count empty lines */
	if(++bufptr <= (char*)buff)
		return(insize);
	*bufptr++ = '\n';
	*bufptr++ = 0;
	size = bufptr - (char*)buff;
#ifdef	SHOPT_ACCTFILE
	if(acctfd)
	{
		int timechars, offset;
		offset = staktell();
		stakputs(buff);
		stakseek(staktell() - 1);
		timechars = sfprintf(staksp, "\t%s\t%x\n",logname,time(NIL(long *)));
		lseek(acctfd, (off_t)0, SEEK_END);
		write(acctfd, stakptr(offset), size - 2 + timechars);
		stakseek(offset);

	}
#endif /* SHOPT_ACCTFILE */
	if(size&01)
	{
		size++;
		*bufptr++ = 0;
	}
	hp->histcnt +=  size;
	c = hist_ind(hp,++hp->histind);
	hp->histcmds[c] = hp->histcnt;
	if(histflush>HIST_MARKSZ && hp->histcnt > hp->histmarker+HIST_BSIZE/2)
	{
		hp->histcnt += HIST_MARKSZ;
		hist_marker(bufptr,hp->histind);
		hp->histmarker = hp->histcmds[hist_ind(hp,c)] = hp->histcnt;
		size += HIST_MARKSZ;
	}
	errno = 0;
	if(write(sffileno(iop),(char*)buff,size)>=0)
	{
		histwfail = 0;
		return(insize);
	}
	return(-1);
}

/*
 * Put history sequence number <n> into buffer <buff>
 * The buffer must be large enough to hold HIST_MARKSZ chars
 */

static void hist_marker(register char *buff,register long cmdno)
{
	*buff++ = HIST_CMDNO;
	*buff++ = 0;
	*buff++ = (cmdno>>16);
	*buff++ = (cmdno>>8);
	*buff++ = cmdno;
	*buff++ = 0;
}

/*
 * return byte offset in history file for command <n>
 */
off_t hist_tell(register History_t *hp, int n)
{
	return(hp->histcmds[hist_ind(hp,n)]);
}

/*
 * seek to the position of command <n>
 */
off_t hist_seek(register History_t *hp, int n)
{
	return(sfseek(hp->histfp,hp->histcmds[hist_ind(hp,n)],SEEK_SET));
}

/*
 * write the command starting at offset <offset> onto file <outfile>.
 * if character <last> appears before newline it is deleted
 * each new-line character is replaced with string <nl>.
 */

void hist_list(register History_t *hp,Sfio_t *outfile, off_t offset,int last, char *nl)
{
	register int oldc=0;
	register int c;
	if(offset<0 || !hp)
	{
		sfputr(outfile,gettxt(e_unknown_id,e_unknown),'\n');
		return;
	}
	sfseek(hp->histfp,offset,SEEK_SET);
	while((c = sfgetc(hp->histfp)) != EOF)
	{
		if(c && oldc=='\n')
			sfputr(outfile,nl,-1);
		else if(last && (c==0 || (c=='\n' && oldc==last)))
			return;
		else if(oldc)
			sfputc(outfile,oldc);
		oldc = c;
		if(c==0)
			return;
	}
	return;
}
		 
/*
 * find index for last line with given string
 * If flag==0 then line must begin with string
 * direction < 1 for backwards search
*/

Histloc_t hist_find(register History_t*hp,char *string,register int index1,int flag,int direction)
{
	register int index2;
	off_t offset;
	int *coffset=0;
	Histloc_t location;
	location.hist_command = -1;
	location.hist_char = 0;
	if(!hp)
		return(location);
	/* leading ^ means beginning of line unless escaped */
	if(flag)
	{
		index2 = *string;
		if(index2=='\\')
			string++;
		else if(index2=='^')
		{
			flag=0;
			string++;
		}
	}
	if(flag)
		coffset = &location.hist_char;
	index2 = (int)hp->histind;
	if(direction<0)
	{
		index2 -= hp->histsize;
		if(index2<1)
			index2 = 1;
		if(index1 <= index2)
			return(location);
	}
	else if(index1 >= index2)
		return(location);
	while(index1!=index2)
	{
		direction>0?++index1:--index1;
		offset = hist_tell(hp,index1);
		if((location.hist_line=hist_match(hp,offset,string,coffset))>=0)
		{
			location.hist_command = index1;
			return(location);
		}
#ifdef KSHELL
		/* allow a search to be aborted */
		if(sh.trapnote&SH_SIGSET)
			break;
#endif /* KSHELL */
	}
	return(location);
}

/*
 * search for <string> in history file starting at location <offset>
 * If coffset==0 then line must begin with string
 * returns the line number of the match if successful, otherwise -1
 */

int hist_match(register History_t *hp,off_t offset,char *string,int *coffset)
{
	register unsigned char *cp;
	register int c;
	register off_t count;
	int line = 0;
	int chrs=0;
#ifdef SHOPT_MULTIBYTE
	int nbytes = 0;
	int hbytes = 0;
#endif /* SHOPT_MULTIBYTE */
	do
	{
		if(offset>=0)
		{
			sfseek(hp->histfp,offset,SEEK_SET);
#ifdef SHOPT_MULTIBYTE
			/* mblen(NIL(char*),MB_CUR_MAX); */
			nbytes = 0;
#endif /* SHOPT_MULTIBYTE */
			count = offset;
		}
		offset = -1;
		for(cp=(unsigned char*)string;*cp;cp++)
		{
			if((c=sfgetc(hp->histfp)) == EOF || c ==0)
				break;
			count++;
#ifdef SHOPT_MULTIBYTE
			/* always position at character boundary */

			/* if(--nbytes < 0) */
			if(--nbytes <= 0)
			{
				nbytes = mblen((char*)cp,MB_CUR_MAX);
				if (nbytes < 1)
					nbytes=1;
			}
			/* if(--hbytes < 0) */
			if(--hbytes <= 0)
			{
				hbytes = sfgetmblen(hp->histfp);
				if (hbytes < 1)
					hbytes=1;
			}

#endif /* SHOPT_MULTIBYTE */
			if(c == '\n')
				line++;
			/* save earliest possible matching character */
			if(coffset && c == *(unsigned char*)string && offset<0)
			{
#ifdef SHOPT_MULTIBYTE
				offset = count + (nbytes - 1);
#else
				offset = count;
#endif /* SHOPT_MULTIBYTE */
				*coffset = chrs;
			}
			if((*cp != c) || (hbytes != nbytes) )
			{
				while (--hbytes > 0)
				{
					/* move to start of next character */
					if((c=sfgetc(hp->histfp)) == EOF || 
								c ==0)
						break;
				}
				nbytes = 0;
				break;
			}
		}
		if(*cp==0) /* match found */
			return(line);
		chrs++;
	}
	while(coffset && c && c != EOF);
	return(-1);
}


#if SHOPT_ESH || SHOPT_VSH
/*
 * copy command <command> from history file to s1
 * at most <size> characters copied
 * if s1==0 the number of lines for the command is returned
 * line=linenumber  for emacs copy and only this line of command will be copied
 * line < 0 for full command copy
 * -1 returned if there is no history file
 */

int hist_copy(char *s1,int size,int command,int line)
{
	register int c;
	register History_t *hp = hist_ptr;
	register int count = 0;
	register char *s1max = s1+size;
	off_t offset;
	if(!hp)
		return(-1);
	offset =  hist_seek(hp,command);
	while ((c = sfgetc(hp->histfp)) && c!=EOF)
	{
		if(c=='\n')
		{
			if(count++ ==line)
				break;
			else if(line >= 0)	
				continue;
		}
		if(s1 && (line<0 || line==count))
		{
			if(s1 >= s1max)
			{
				*--s1 = 0;
				break;
			}
			*s1++ = c;
		}
			
	}
	sfseek(hp->histfp,(off_t)0,SEEK_END);
	if(s1==0)
		return(count);
	if(count && (c= *(s1-1)) == '\n')
		s1--;
	*s1 = '\0';
	return(count);
}

/*
 * return word number <word> from command number <command>
 */

char *hist_word(char *string,int size,int word)
{
	register int c;
	register char *s1 = string;
	register unsigned char *cp = (unsigned char*)s1;
	register int flag = 0;
	if(!hist_ptr)
#ifdef KSHELL
	{
		strncpy(string,sh.lastarg,size);
		return(string);
	}
#else
		return(NIL(char*));
#endif /* KSHELL */
	hist_copy(string,size,(int)hist_ptr->histind-1,-1);
	for(;c = *cp;cp++)
	{
		c = isspace(c);
		if(c && flag)
		{
			*cp = 0;
			if(--word==0)
				break;
			flag = 0;
		}
		else if(c==0 && flag==0)
		{
			s1 = (char*)cp;
			flag++;
		}
	}
	*cp = 0;
	if(s1 != string)
		strcpy(string,s1);
	return(string);
}

#endif	/* SHOPT_ESH */

#ifdef SHOPT_ESH
/*
 * given the current command and line number,
 * and number of lines back or foward,
 * compute the new command and line number.
 */

Histloc_t hist_locate(History_t *hp,register int command,register int line,int lines)
{
	Histloc_t next;
	line += lines;
	if(!hp)
	{
		command = -1;
		goto done;
	}
	if(lines > 0)
	{
		register int count;
		while(command <= hp->histind)
		{
			count = hist_copy(NIL(char*),0, command,-1);
			if(count > line)
				goto done;
			line -= count;
			command++;
		}
	}
	else
	{
		register int least = (int)hp->histind-hp->histsize;
		while(1)
		{
			if(line >=0)
				goto done;
			if(--command < least)
				break;
			line += hist_copy(NIL(char*),0, command,-1);
		}
		command = -1;
	}
	next.hist_command = command;
	return(next);
done:
	next.hist_line = line;
	next.hist_command = command;
	return(next);
}
#endif	/* SHOPT_ESH */


/*
 * Handle history file exceptions
 */
static int hist_exceptf(Sfio_t* fp, int type, Sfdisc_t *handle)
{
	register int newfd,oldfd;
	History_t *hp = (History_t*)handle;
	if(type==SF_WRITE)
	{
		if(errno==ENOSPC || histwfail++ >= 10)
			return(0);
		/* write failure could be NFS problem, try to re-open */
		close(oldfd=sffileno(fp));
		if((newfd=open(hp->histname,O_BINARY|O_APPEND|O_CREAT|O_RDWR,S_IRUSR|S_IWUSR)) >= 0)
		{
			if(fcntl(newfd, F_DUPFD, oldfd) !=oldfd)
				return(-1);
			fcntl(oldfd,F_SETFD,FD_CLOEXEC);
			close(newfd);
			if(lseek(oldfd,(off_t)0,SEEK_END) < hp->histcnt)
			{
				register int index = hp->histind;
				lseek(oldfd,(off_t)2,SEEK_SET);
				hp->histcnt = 2;
				hp->histind = 1;
				hp->histcmds[1] = 2;
				hist_eof(hp);
				hp->histmarker = hp->histcnt;
				hp->histind = index;
			}
			return(1);
		}
		error(2,gettxt(":207","History file write error-%d %s: file unrecoverable"),errno,hp->histname);
		return(-1);
	}
	return(0);
}
