#ident	"@(#)ksh93:src/cmd/ksh93/bltins/read.c	1.2"
#pragma prototyped
/*
 * read [-Aprs] [-d delim] [-u filenum] [-t timeout] [name...]
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
#include	<error.h>
#include	<ctype.h>
#include	"defs.h"
#include	"variables.h"
#include	"lexstates.h"
#include	"io.h"
#include	"name.h"
#include	"builtins.h"
#include	"history.h"
#include	"terminal.h"
#include	"national.h"

extern char   *gettxt();

#define	R_FLAG	1	/* raw mode */
#define	S_FLAG	2	/* save in history file */
#define	A_FLAG	4	/* read into array */
#define N_FLAG	8	/* fixed size read */
#define D_FLAG	6	/* must be number of bits for all flags */

int	b_read(int argc,char *argv[], void *extra)
{
	register char *name;
	register int r, flags=0, fd=0;
	long timeout = 1000*sh.st.tmout;
	int save_prompt;
	NOT_USED(argc);
	NOT_USED(extra);
	while((r = optget(argv,(const char *)gettxt(sh_optread_id,sh_optread)))) switch(r)
	{
	    case 'A':
		flags |= A_FLAG;
		break;
	    case 't':
		timeout = 1000*opt_num+1;
		break;
	    case 'd':
		if(opt_arg && *opt_arg!='\n')
		{
			flags &= ~((1<<D_FLAG)-1);
			flags |= ((*opt_arg)<< D_FLAG);
		}
		break;
	    case 'p':
		if((fd = sh.cpipe[0])<=0)
			error(ERROR_exit(1),gettxt(e_query_id,e_query));
		break;
	   case 'n':
		r = (int)opt_num;
		if(r> 0xfff)
			error(1,e_overlimit,"n");
		flags &= ~((1<<D_FLAG)-1);
		flags |= N_FLAG|(r<< D_FLAG);
		break;
	    case 'r':
		flags |= R_FLAG;
		break;
	    case 's':
		/* save in history file */
		flags |= S_FLAG;
		break;
	    case 'u':
		fd = (int)opt_num;
		if(sh_inuse(fd))
			fd = -1;
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	if(!((r=sh.fdstatus[fd])&IOREAD)  || !(r&(IOSEEK|IONOSEEK)))
		r = sh_iocheckfd(fd);
	if(fd<0 || !(r&IOREAD))
		error(ERROR_system(1),gettxt(e_file2_id,e_file2));
	/* look for prompt */
	if((name = *argv) && (name=strchr(name,'?')) && (r&IOTTY))
	{
		r = strlen(++name)+1;
		if(sh.prompt=(char*)sfreserve(sfstderr,r,1))
		{
			memcpy(sh.prompt,name,r);
			sfwrite(sfstderr,sh.prompt,r);
		}
	}
	sh.timeout = 0;
	save_prompt = sh.nextprompt;
	sh.nextprompt = 0;
	r=sh_readline(argv,fd,flags,timeout);
	sh.nextprompt = save_prompt;
	if(r==0 && (r=(sfeof(sh.sftable[fd])||sferror(sh.sftable[fd]))))
	{
		if(fd == sh.cpipe[0])
		{
			sh_pclose(sh.cpipe);
			return(1);
		}
	}
	sfclrerr(sh.sftable[fd]);
	return(r);
}

/*
 * here for read timeout
 */
static void timedout(void *handle)
{
	sfclrlock((Sfio_t*)handle);
	sh_exit(1);
}

/*
 * This is the code to read a line and to split it into tokens
 *  <names> is an array of variable names
 *  <fd> is the file descriptor
 *  <flags> is union of -A, -r, -s, and contains delimiter if not '\n'
 *  <timeout> is number of milli-seconds until timeout
 */

int sh_readline(char **names, int fd, int flags,long timeout)
{
	register int		c;
	register unsigned char	*cp;
	register Namval_t	*np;
	register char		*name, *val;
	register Sfio_t	*iop;
	char			*ifs;
	unsigned char		*cpmax;
	char			was_escape = 0;
	char			use_stak = 0;
	char			was_write = 0;
	int			rel;
	long			array_index = 0;
	void			*timeslot=0;
	int			delim = '\n';
	int			jmpval=0;
	int			size = 0;
	int			readonly_error = 0;
	struct	checkpt		buff;
	if(!(iop=sh.sftable[fd]) && !(iop=sh_iostream(fd)))
		return(1);
	if(flags>>D_FLAG)	/* delimiter not new-line or fixed size read */
	{
		if(flags&N_FLAG)
			size = ((unsigned)flags)>>D_FLAG;
		else
			delim = ((unsigned)flags)>>D_FLAG;
		if(sh.fdstatus[fd]&IOTTY)
			tty_raw(fd,1);
	}
	if(!(flags&N_FLAG))
	{
		/* set up state table based on IFS */
		ifs = nv_getval(np=nv_scoped(IFSNOD));
		if((flags&R_FLAG) && sh.ifstable['\\']==S_ESC)
			sh.ifstable['\\'] = 0;
		else if(!(flags&R_FLAG) && sh.ifstable['\\']==0)
			sh.ifstable['\\'] = S_ESC;
		sh.ifstable[delim] = S_NL;
		if(delim!='\n')
		{
			sh.ifstable['\n'] = 0;
			nv_putval(np, ifs, NV_RDONLY);
		}
		sh.ifstable[0] = S_EOF;
	}
	if(names && (name = *names))
	{
		if(val= strchr(name,'?'))
			*val = 0;
		np = nv_open(name,sh.var_tree,NV_NOASSIGN|NV_VARNAME);
		if(flags&A_FLAG)
		{
			flags &= ~A_FLAG;
			array_index = 1;
			nv_unset(np);
			nv_putsub(np,NIL(char*),0L);
		}
		else
			name = *++names;
		if(val)
			*val = '?';
	}
	else
	{
		name = 0;
		if(hashscope(sh.var_tree))
                	np = nv_search((char*)REPLYNOD,sh.var_tree,HASH_BUCKET);
		else
			np = REPLYNOD;
	}
	sfclrerr(iop);
	was_write = sfset(iop,SF_WRITE,0);
	if(timeout || was_write || ((flags>>D_FLAG) && (sh.fdstatus[fd]&IOTTY)))
	{
		sh_pushcontext(&buff,1);
		jmpval = sigsetjmp(buff.buff,0);
		if(jmpval)
			goto done;
		if (timeout)
                	timeslot = (void*)timeradd(timeout,0,timedout,(void*)iop);
	}
	if(flags&N_FLAG)
	{
		/* reserved buffer */
		if(!(cp = (unsigned char*)malloc(size+1)))
			sh_exit(1);
		if(nv_size(np)<size-1 || nv_isattr(np,NV_INTEGER|NV_RJUST|NV_LJUST))
		{
			nv_putval(np,(char*)cp,NV_NOFREE);
			nv_offattr(np,NV_NOFREE);
			nv_setsize(np,size-1);
		}
		while(size>0)
		{
			*cp = 0;
			c = sfread(iop,(void*)cp,size);
			if(c<=0)
				break;
			cp += c;
			size -= c;
		}
		*cp = 0;
		if(timeslot)
			timerdel(timeslot);
		goto done;
	}
        else if(!(cp = (unsigned char*)sfgetr(iop,delim,0)))
		cp = (unsigned char*)sfgetr(iop,delim,-1);
	if(timeslot)
		timerdel(timeslot);
	if(cp)
	{
		cpmax = cp + sfslen();
		if(*(cpmax-1) != delim)
			*(cpmax-1) = delim;
		if(flags&S_FLAG)
			sfwrite(sh.hist_ptr->histfp,(char*)cp,sfslen());
		c = sh.ifstable[*cp++];
#ifndef SHOPT_MULTIBYTE
		if(!name && (flags&R_FLAG)) /* special case single argument */
		{
			/* skip over leading blanks */
			while(c==S_SPACE)
				c = sh.ifstable[*cp++];
			/* strip trailing delimiters */
			if(cpmax[-1] == '\n')
				cpmax--;
			*cpmax =0;
			if(cpmax>cp)
			{
				while((c=sh.ifstable[*--cpmax])==S_DELIM || c==S_SPACE);
				cpmax[1] = 0;
			}
			if(nv_isattr(np, NV_RDONLY))
			{
				error(ERROR_warn(0), e_readonly, nv_name(np));
				readonly_error = 1;
			}
			else
				nv_putval(np,(char*)cp-1,0);
			goto done;
		}
#endif /* !SHOPT_MULTIBYTE */
	}
	else
		c = S_NL;
	sh.nextprompt = 2;
	rel= staktell();
	/* val==0 at the start of a field */
	val = 0;
	while(1)
	{
		switch(c)
		{
#ifdef SHOPT_MULTIBYTE
		   case S_MBYTE:
			if (val == 0)
				val = (char*)(cp-1);
			if(sh_strchr(ifs,(char*)cp-1)>=0)
			{
				c = mblen((char*)cp-1,MB_CUR_MAX);
				if (name) 
					cp[-1] = 0;
				cp += (c-1);
				c = S_DELIM;
			}
			else
				c = 0;
			continue;
#endif /*SHOPT_MULTIBYTE */
		    case S_ESC:
			/* process escape character */
			if((c = sh.ifstable[*cp++]) == S_NL)
				was_escape = 1;
			else
				c = 0;
			if(val)
			{
				stakputs(val);
				use_stak = 1;
				was_escape = 1;
				*val = 0;
			}
			continue;

		    case S_EOF:
			/* check for end of buffer */
			if(val && *val)
			{
				stakputs(val);
				use_stak = 1;
			}
			val = 0;
			if(cp>=cpmax)
			{
				c = S_NL;
				break;
			}
			/* eliminate null bytes */
			c = sh.ifstable[*cp++];
			if(!name && val && (c==S_SPACE||c==S_DELIM||c==S_MBYTE))
				c = 0;
			continue;
		    case S_NL:
			if(was_escape)
			{
				was_escape = 0;
				if((cp = (unsigned char*)sfgetr(iop,delim,0)) ||
				    (cp = (unsigned char*)sfgetr(iop,delim,-1)))
				{
					if(flags&S_FLAG)
						sfwrite(sh.hist_ptr->histfp,(char*)cp,sfslen());
					cpmax = cp + sfslen();
					c = sh.ifstable[*cp++];
					val=0;
					if(!name && (c==S_SPACE || c==S_DELIM || c==S_MBYTE))
						c = 0;
					continue;
				}
			}
			c = S_NL;
			break;

		    case S_SPACE:
			/* skip over blanks */
			while(c==S_SPACE)
				c = sh.ifstable[*cp++];
			if(!val)
				continue;
#ifdef SHOPT_MULTIBYTE
			if(c==S_MBYTE)
			{
				if(sh_strchr(ifs,(char*)cp-1)>=0)
				{
					c = mblen((char*)cp-1,MB_CUR_MAX);
					cp += (c-1);
					c = S_DELIM;
				}
				else
					c = 0;
			}
#endif /* SHOPT_MULTIBYTE */
			if(c!=S_DELIM)
				break;
			/* FALL THRU */
		    case S_DELIM:
			if(name)
			{
				/* skip over trailing blanks */
				while((c=sh.ifstable[*cp++])==S_SPACE);
				break;
			}

		    case 0:
			if(val==0 || was_escape)
			{
				val = (char*)(cp-1);
				was_escape = 0;
			}
			/* skip over word characters */
			while(1)
			{
				while((c=sh.ifstable[*cp++])==0);
				if(name || c==S_NL || c==S_ESC || c==S_EOF || c==S_MBYTE)
					break;
			}
			if(c!=S_MBYTE)
				cp[-1] = 0;
			continue;
		}
		/* assign value and advance to next variable */
		if(!val)
			val = "";
		if(use_stak)
		{
			stakputs(val);
			stakputc(0);
			val = stakptr(rel);
		}
		if(!name && *val)
		{
			/* strip off trailing delimiters */
			register char	*cp = val + strlen(val);
			register int n;
			while((n=sh.ifstable[*--cp])==S_DELIM || n==S_SPACE);
			cp[1] = 0;
		}
		if(nv_isattr(np, NV_RDONLY))
		{
			error(ERROR_warn(0), e_readonly, nv_name(np));
			readonly_error = 1;
		}
		else
			nv_putval(np,val,0);
		val = 0;
		if(use_stak)
		{
			stakseek(rel);
			use_stak = 0;
		}
		if(array_index)
		{
			nv_putsub(np, NIL(char*), array_index++);
			if(c!=S_NL)
				continue;
			name = *++names;
		}
		while(1)
		{
			if(sh_isoption(SH_ALLEXPORT)&&!strchr(nv_name(np),'.'))
				nv_onattr(np,NV_EXPORT);
			if(name)
			{
				nv_close(np);
				np = nv_open(name,sh.var_tree,NV_NOASSIGN|NV_VARNAME);
				name = *++names;
			}
			else
				np = 0;
			if(c!=S_NL)
				break;
			if(!np)
				goto done;
			if(nv_isattr(np, NV_RDONLY))
			{
				error(ERROR_warn(0), e_readonly, nv_name(np));
				readonly_error = 1;
			}
			else
				nv_putval(np, "", 0);
		}
		val = 0;
	}
done:
	if(was_write)
		sfset(iop,SF_WRITE,1);
	nv_close(np);
	if(timeout)
		sh_popcontext(&buff);
	if((flags>>D_FLAG) && (sh.fdstatus[fd]&IOTTY))
		tty_cooked(fd);
	if(flags&S_FLAG)
		hist_flush(sh.hist_ptr);
	if (readonly_error && !jmpval)
		jmpval = 1;
	return(jmpval);
}

