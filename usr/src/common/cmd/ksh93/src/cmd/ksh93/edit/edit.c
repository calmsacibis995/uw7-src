#ident	"@(#)ksh93:src/cmd/ksh93/edit/edit.c	1.2"
#pragma prototyped
/*
 *  edit.c - common routines for vi and emacs one line editors in shell
 *
 *   David Korn				P.D. Sullivan
 *   AT&T Bell Laboratories		AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 *   Coded April 1983.
 */

#include	<ast.h>
#include	<errno.h>
#include	<ctype.h>
#include	"FEATURE/options"
#include	"path.h"
#include	"FEATURE/time"
#ifdef _hdr_utime
#   include	<utime.h>
#   include	<ls.h>
#endif

#ifdef KSHELL
#   include	"defs.h"
#   include	"variables.h"
#else
    extern char ed_errbuf[];
    char e_version[] = "\n@(#)Editlib version 12/28/93c\0\n";
#endif	/* KSHELL */
#include	"io.h"
#include	"terminal.h"
#include	"history.h"
#include	"edit.h"

#define TO_PRINT	('A'-cntl('A'))
#define MINWINDOW	15	/* minimum width window */
#define DFLTWINDOW	80	/* default window width */
#define RAWMODE		1
#define ALTMODE		2
#define ECHOMODE	3
#define	SYSERR	-1

#ifdef SHOPT_OLDTERMIO
#   undef tcgetattr
#   undef tcsetattr
#endif /* SHOPT_OLDTERMIO */

#ifdef RT
#   define VENIX 1
#endif	/* RT */

#define lookahead	editb.e_index
#define env		editb.e_env
#define previous	editb.e_lbuf
#define fildes		editb.e_fd


#ifdef _hdr_sgtty
#   ifdef TIOCGETP
	static int l_mask;
	static struct tchars l_ttychars;
	static struct ltchars l_chars;
	static  char  l_changed;	/* set if mode bits changed */
#	define L_CHARS	4
#	define T_CHARS	2
#	define L_MASK	1
#   endif /* TIOCGETP */
#endif /* _hdr_sgtty */

#ifdef KSHELL
     static int keytrap(char*, int, int, int);
#else
     struct edit editb;
#endif	/* KSHELL */


#ifndef _POSIX_DISABLE
#   define _POSIX_DISABLE	0
#endif

static struct termios savetty;
static int savefd;

#ifdef future
    static int compare(const char*, const char*, int);
#endif  /* future */
#if SHOPT_VSH || SHOPT_ESH
    static struct termios ttyparm;	/* initial tty parameters */
    static struct termios nttyparm;	/* raw tty parameters */
    static char bellchr[] = "\7";	/* bell char */
#endif /* SHOPT_VSH || SHOPT_ESH */


/*
 * This routine returns true if fd refers to a terminal
 * This should be equivalent to isatty
 */
int tty_check(int fd)
{
	struct termios tty;
	savefd = -1;
	return(tty_get(fd,&tty)==0);
}

/*
 * Get the current terminal attributes
 * This routine remembers the attributes and just returns them if it
 *   is called again without an intervening tty_set()
 */

int tty_get(register int fd, register struct termios *tty)
{
	if(fd == savefd)
		*tty = savetty;
	else
	{
		while(tcgetattr(fd,tty) == SYSERR)
		{
			if(errno !=EINTR)
				return(SYSERR);
			errno = 0;
		}
		/* save terminal settings if in cannonical state */
		if(editb.e_raw==0)
		{
			savetty = *tty;
			savefd = fd;
		}
	}
	return(0);
}

/*
 * Set the terminal attributes
 * If fd<0, then current attributes are invalidated
 */

int tty_set(int fd, int action, struct termios *tty)
{
	if(fd >=0)
	{
#ifdef future
		if(savefd>=0 && compare(&savetty,tty,sizeof(struct termios)))
			return(0);
#endif
		while(tcsetattr(fd, action, tty) == SYSERR)
		{
			if(errno !=EINTR)
				return(SYSERR);
			errno = 0;
		}
		savetty = *tty;
	}
	savefd = fd;
	return(0);
}

#if SHOPT_ESH || SHOPT_VSH
/*{	TTY_COOKED( fd )
 *
 *	This routine will set the tty in cooked mode.
 *	It is also called by error.done().
 *
}*/

void tty_cooked(register int fd)
{

	if(editb.e_raw==0)
		return;
	if(fd < 0)
		fd = savefd;
#ifdef L_MASK
	/* restore flags */
	if(l_changed&L_MASK)
		ioctl(fd,TIOCLSET,&l_mask);
	if(l_changed&T_CHARS)
		/* restore alternate break character */
		ioctl(fd,TIOCSETC,&l_ttychars);
	if(l_changed&L_CHARS)
		/* restore alternate break character */
		ioctl(fd,TIOCSLTC,&l_chars);
	l_changed = 0;
#endif	/* L_MASK */
	/*** don't do tty_set unless ttyparm has valid data ***/
	if(tty_set(fd, TCSANOW, &ttyparm) == SYSERR)
		return;
	editb.e_raw = 0;
	return;
}

/*{	TTY_RAW( fd )
 *
 *	This routine will set the tty in raw mode.
 *
}*/

tty_raw(register int fd, int echo)
{
#ifdef L_MASK
	struct ltchars lchars;
#endif	/* L_MASK */
	if(editb.e_raw==RAWMODE)
		return(echo?-1:0);
	else if(editb.e_raw==ECHOMODE)
		return(echo?0:-1);
#ifndef SHOPT_RAWONLY
	if(editb.e_raw != ALTMODE)
#endif /* SHOPT_RAWONLY */
	{
		if(tty_get(fd,&ttyparm) == SYSERR)
			return(-1);
	}
#if  L_MASK || VENIX
	if(!(ttyparm.sg_flags&ECHO) || (ttyparm.sg_flags&LCASE))
		return(-1);
	nttyparm = ttyparm;
	if(!echo)
		nttyparm.sg_flags &= ~(ECHO | TBDELAY);
#   ifdef CBREAK
	nttyparm.sg_flags |= CBREAK;
#   else
	nttyparm.sg_flags |= RAW;
#   endif /* CBREAK */
	editb.e_erase = ttyparm.sg_erase;
	editb.e_kill = ttyparm.sg_kill;
	editb.e_eof = cntl('D');
	editb.e_werase = cntl('W');
	editb.e_lnext = cntl('V');
	if( tty_set(fd, TCSADRAIN, &nttyparm) == SYSERR )
		return(-1);
	editb.e_ttyspeed = (ttyparm.sg_ospeed>=B1200?FAST:SLOW);
#   ifdef TIOCGLTC
	/* try to remove effect of ^V  and ^Y and ^O */
	if(ioctl(fd,TIOCGLTC,&l_chars) != SYSERR)
	{
		lchars = l_chars;
		lchars.t_lnextc = -1;
		lchars.t_flushc = -1;
		lchars.t_dsuspc = -1;	/* no delayed stop process signal */
		if(ioctl(fd,TIOCSLTC,&lchars) != SYSERR)
			l_changed |= L_CHARS;
	}
#   endif	/* TIOCGLTC */
#else
	if (!(ttyparm.c_lflag & ECHO ))
		return(-1);
#   ifdef FLUSHO
	ttyparm.c_lflag &= ~FLUSHO;
#   endif /* FLUSHO */
	nttyparm = ttyparm;
#  ifndef u370
	nttyparm.c_iflag &= ~(IGNPAR|PARMRK|INLCR|IGNCR|ICRNL);
	nttyparm.c_iflag |= BRKINT;
#   else
	nttyparm.c_iflag &= 
			~(IGNBRK|PARMRK|INLCR|IGNCR|ICRNL|INPCK);
	nttyparm.c_iflag |= (BRKINT|IGNPAR);
#   endif	/* u370 */
	if(echo)
		nttyparm.c_lflag &= ~ICANON;
	else
		nttyparm.c_lflag &= ~(ICANON|ECHO|ECHOK);
	nttyparm.c_cc[VTIME] = 0;
	nttyparm.c_cc[VMIN] = 1;
#   ifdef VDISCARD
	nttyparm.c_cc[VDISCARD] = _POSIX_DISABLE;
#   endif /* VDISCARD */
#   ifdef VDSUSP
	nttyparm.c_cc[VDSUSP] = _POSIX_DISABLE;
#   endif /* VDSUSP */
#   ifdef VWERASE
	if(ttyparm.c_cc[VWERASE] == _POSIX_DISABLE)
		editb.e_werase = cntl('W');
	else
		editb.e_werase = nttyparm.c_cc[VWERASE];
	nttyparm.c_cc[VWERASE] = _POSIX_DISABLE;
#   else
	    editb.e_werase = cntl('W');
#   endif /* VWERASE */
#   ifdef VLNEXT
	if(ttyparm.c_cc[VLNEXT] == _POSIX_DISABLE )
		editb.e_lnext = cntl('V');
	else
		editb.e_lnext = nttyparm.c_cc[VLNEXT];
	nttyparm.c_cc[VLNEXT] = _POSIX_DISABLE;
#   else
	editb.e_lnext = cntl('V');
#   endif /* VLNEXT */
	editb.e_eof = ttyparm.c_cc[VEOF];
	editb.e_erase = ttyparm.c_cc[VERASE];
	editb.e_kill = ttyparm.c_cc[VKILL];
	if( tty_set(fd, TCSADRAIN, &nttyparm) == SYSERR )
		return(-1);
	editb.e_ttyspeed = (cfgetospeed(&ttyparm)>=B1200?FAST:SLOW);
#endif
	editb.e_raw = (echo?ECHOMODE:RAWMODE);
	return(0);
}

#ifndef SHOPT_RAWONLY

/*
 *
 *	Get tty parameters and make ESC and '\r' wakeup characters.
 *
 */

#   ifdef TIOCGETC
tty_alt(register int fd)
{
	int mask;
	struct tchars ttychars;
	switch(editb.e_raw)
	{
	    case ECHOMODE:
		return(-1);
	    case ALTMODE:
		return(0);
	    case RAWMODE:
		tty_cooked(fd);
	}
	l_changed = 0;
	if( editb.e_ttyspeed == 0)
	{
		if((tty_get(fd,&ttyparm) != SYSERR))
			editb.e_ttyspeed = (ttyparm.sg_ospeed>=B1200?FAST:SLOW);
		editb.e_raw = ALTMODE;
	}
	if(ioctl(fd,TIOCGETC,&l_ttychars) == SYSERR)
		return(-1);
	if(ioctl(fd,TIOCLGET,&l_mask)==SYSERR)
		return(-1);
	ttychars = l_ttychars;
	mask =  LCRTBS|LCRTERA|LCTLECH|LPENDIN|LCRTKIL;
	if((l_mask|mask) != l_mask)
		l_changed = L_MASK;
	if(ioctl(fd,TIOCLBIS,&mask)==SYSERR)
		return(-1);
	if(ttychars.t_brkc!=ESC)
	{
		ttychars.t_brkc = ESC;
		l_changed |= T_CHARS;
		if(ioctl(fd,TIOCSETC,&ttychars) == SYSERR)
			return(-1);
	}
	return(0);
}
#   else
#	ifndef PENDIN
#	    define PENDIN	0
#	endif /* PENDIN */
#	ifndef IEXTEN
#	    define IEXTEN	0
#	endif /* IEXTEN */

tty_alt(register int fd)
{
	switch(editb.e_raw)
	{
	    case ECHOMODE:
		return(-1);
	    case ALTMODE:
		return(0);
	    case RAWMODE:
		tty_cooked(fd);
	}
	if((tty_get(fd, &ttyparm)==SYSERR) || (!(ttyparm.c_lflag&ECHO)))
		return(-1);
#	ifdef FLUSHO
	    ttyparm.c_lflag &= ~FLUSHO;
#	endif /* FLUSHO */
	nttyparm = ttyparm;
	editb.e_eof = ttyparm.c_cc[VEOF];
#	ifdef ECHOCTL
	    /* escape character echos as ^[ */
	    nttyparm.c_lflag |= (ECHOE|ECHOK|ECHOCTL|PENDIN|IEXTEN);
	    nttyparm.c_cc[VEOL] = ESC;
#	else
	    /* switch VEOL2 and EOF, since EOF isn't echo'd by driver */
	    nttyparm.c_lflag |= (ECHOE|ECHOK);
	    nttyparm.c_cc[VEOF] = ESC;	/* make ESC the eof char */
#	    ifdef VEOL2
		nttyparm.c_iflag &= ~(IGNCR|ICRNL);
		nttyparm.c_iflag |= INLCR;
		nttyparm.c_cc[VEOL] = '\r';	/* make CR an eol char */
		nttyparm.c_cc[VEOL2] = editb.e_eof; /* make EOF an eol char */
#	    else
		nttyparm.c_cc[VEOL] = editb.e_eof; /* make EOF an eol char */
#	    endif /* VEOL2 */
#	endif /* ECHOCTL */
#	ifdef VWERASE
	    if(ttyparm.c_cc[VWERASE] == _POSIX_DISABLE)
		    nttyparm.c_cc[VWERASE] = cntl('W');
	    editb.e_werase = nttyparm.c_cc[VWERASE];
#	else
	    editb.e_werase = cntl('W');
#	endif /* VWERASE */
#	ifdef VLNEXT
	    if(ttyparm.c_cc[VLNEXT] == _POSIX_DISABLE )
		    nttyparm.c_cc[VLNEXT] = cntl('V');
	    editb.e_lnext = nttyparm.c_cc[VLNEXT];
#	else
	    editb.e_lnext = cntl('V');
#	endif /* VLNEXT */
	editb.e_erase = ttyparm.c_cc[VERASE];
	editb.e_kill = ttyparm.c_cc[VKILL];
	if( tty_set(fd, TCSADRAIN, &nttyparm) == SYSERR )
		return(-1);
	editb.e_ttyspeed = (cfgetospeed(&ttyparm)>=B1200?FAST:SLOW);
	editb.e_raw = ALTMODE;
	return(0);
}

#   endif /* TIOCGETC */
#endif	/* SHOPT_RAWONLY */

/*
 *	ED_WINDOW()
 *
 *	return the window size
 */
int ed_window(void)
{
	int	rows,cols;
	register char *cp = nv_getval(COLUMNS);
	if(cp)
		cols = atoi(cp)-1;
	else
	{
		astwinsize(2,&rows,&cols);
		if(--cols <0)
			cols = DFLTWINDOW-1;
	}
	if(cols < MINWINDOW)
		cols = MINWINDOW;
	else if(cols > MAXWINDOW)
		cols = MAXWINDOW;
	return(cols);
}

/*	E_FLUSH()
 *
 *	Flush the output buffer.
 *
 */

void ed_flush(void)
{
	register int n = editb.e_outptr-editb.e_outbase;
	register int fd = ERRIO;
	if(n<=0)
		return;
	write(fd,editb.e_outbase,(unsigned)n);
	editb.e_outptr = editb.e_outbase;
}

/*
 * send the bell character ^G to the terminal
 */

void ed_ringbell(void)
{
	write(ERRIO,bellchr,1);
}

/*
 * send a carriage return line feed to the terminal
 */

void ed_crlf(void)
{
#ifdef cray
	ed_putchar('\r');
#endif /* cray */
#ifdef u370
	ed_putchar('\r');
#endif	/* u370 */
#ifdef VENIX
	ed_putchar('\r');
#endif /* VENIX */
	ed_putchar('\n');
	ed_flush();
}
 
/*	ED_SETUP( max_prompt_size )
 *
 *	This routine sets up the prompt string
 *	The following is an unadvertised feature.
 *	  Escape sequences in the prompt can be excluded from the calculated
 *	  prompt length.  This is accomplished as follows:
 *	  - if the prompt string starts with "%\r, or contains \r%\r", where %
 *	    represents any char, then % is taken to be the quote character.
 *	  - strings enclosed by this quote character, and the quote character,
 *	    are not counted as part of the prompt length.
 */

void	ed_setup(int fd)
{
	register char *pp;
	register char *last;
	char *ppmax;
	int myquote = 0;
	register int qlen = 1;
	char inquote = 0;
	editb.e_fd = fd;
#ifdef KSHELL
	if(!(last = sh.prompt))
		last = "";
	sh.prompt = 0;
#else
	last = editb.e_prbuff;
#endif /* KSHELL */
	if(sh.hist_ptr)
	{
		register History_t *hp = sh.hist_ptr;
		editb.e_hismax = hist_max(hp);
		editb.e_hismin = hist_min(hp);
	}
	else
	{
		editb.e_hismax = editb.e_hismin = editb.e_hloff = 0;
	}
	editb.e_hline = editb.e_hismax;
	editb.e_wsize = ed_window()-2;
	editb.e_crlf = 1;
	pp = editb.e_prompt;
	ppmax = pp+PRSIZE-1;
	*pp++ = '\r';
	{
		register int c;
		while(c= *last++) switch(c)
		{
			case '\r':
				if(pp == (editb.e_prompt+2)) /* quote char */
					myquote = *(pp-1);
				/*FALLTHROUGH*/

			case '\n':
				/* start again */
				editb.e_crlf = 1;
				qlen = 1;
				inquote = 0;
				pp = editb.e_prompt+1;
				break;

			case '\t':
				/* expand tabs */
				while((pp-editb.e_prompt)%TABSIZE)
				{
					if(pp >= ppmax)
						break;
					*pp++ = ' ';
				}
				break;

			case '\a':
				/* cut out bells */
				break;

			default:
				if(c==myquote)
				{
					qlen += inquote;
					inquote ^= 1;
				}
				if(pp < ppmax)
				{
					qlen += inquote;
					*pp++ = c;
					if(!inquote && !isprint(c))
						editb.e_crlf = 0;
				}
		}
	}
	editb.e_plen = pp - editb.e_prompt - qlen;
	*pp = 0;
	if((editb.e_wsize -= editb.e_plen) < 7)
	{
		register int shift = 7-editb.e_wsize;
		editb.e_wsize = 7;
		pp = editb.e_prompt+1;
		strcpy(pp,pp+shift);
		editb.e_plen -= shift;
		last[-editb.e_plen-2] = '\r';
	}
	sfsync(sfstderr);
	qlen = sfset(sfstderr,SF_READ,0);
	/* make sure SF_READ not on */
	editb.e_outbase = editb.e_outptr = (char*)sfreserve(sfstderr,SF_UNBOUND,1);
	editb.e_outlast = editb.e_outptr + sfslen();
	if(qlen)
		sfset(sfstderr,SF_READ,1);
	sfwrite(sfstderr,editb.e_outptr,0);
}

/*
 * Do read, restart on interrupt unless SH_SIGSET or SH_SIGTRAP is set
 * Use sfpkrd() to poll() or select() to wait for input if possible
 * Unfortunately, systems that get interrupted from slow reads update
 * this access time for for the terminal (in violation of POSIX).
 * The fixtime() macro, resets the time to the time at entry in
 * this case.  This is not necessary for systems that can handle
 * sfpkrd() correctly (i,e., those that support poll() or select()
 */
int ed_read(int fd, char *buff, int size)
{
	register int rv= -1;
	register int delim = (editb.e_raw==RAWMODE?'\r':'\n');
	sh_onstate(SH_TTYWAIT);
	errno = EINTR;
	while(rv<0 && errno==EINTR)
	{
		if(sh.trapnote&(SH_SIGSET|SH_SIGTRAP))
			goto done;
		/* an interrupt that should be ignored */
		errno = 0;
		if(sh.waitevent && (rv=(*sh.waitevent)(fd,0L))>=0)
			rv = sfpkrd(fd,buff,size,delim,-1L,1);
	}
	if(rv > 0)
		rv = read(fd,buff,rv);
	else
	{
#ifdef _hdr_utime
#		define fixtime()	if(isdevtty)utime(tty,&utimes)
		static ino_t tty_ino;
		static dev_t tty_dev;
		static char *tty;
		int	isdevtty=0;
		struct stat statb;
		struct utimbuf utimes;
	 	if(errno==0 && !tty)
		{
			if((tty=ttyname(fd)) && stat(tty,&statb)>=0)
			{
				tty_ino = statb.st_ino;
				tty_dev = statb.st_dev;
			}
		}
		if(tty_ino && fstat(fd,&statb)>=0 && statb.st_ino==tty_ino && statb.st_dev==tty_dev)
		{
			utimes.actime = statb.st_atime;
			utimes.modtime = statb.st_mtime;
			isdevtty=1;
		}
#else
#		define fixtime()
#endif /* _hdr_utime */
		while(1)
		{
			rv = read(fd,buff,size);
			if(rv>=0 || errno!=EINTR)
				break;
			if(sh.trapnote&(SH_SIGSET|SH_SIGTRAP))
				goto done;
			/* an interrupt that should be ignored */
			fixtime();
		}
	}
done:
	sh_offstate(SH_TTYWAIT);
	return(rv);
}


/*
 * put <string> of length <nbyte> onto lookahead stack
 * if <type> is non-zero,  the negation of the character is put
 *    onto the stack so that it can be checked for KEYTRAP
 * putstack() returns 1 except when in the middle of a multi-byte char
 */
static int putstack(char string[], register int nbyte, int type) 
{
	register int c;
#ifdef SHOPT_MULTIBYTE
	register int max,last,size;
	static int curchar, cursize=0;
	last = max = nbyte;
	nbyte = 0;
	while (nbyte < max)
	{
		if (cursize == 0) {
			size = mbtowc(NULL, &(string[nbyte]), CHARSIZE);
			if (size < 0)
				size = 1;
			cursize = size;
			if (cursize == 0)
				goto zero;
			while(cursize > 0) 
			{
				c = string[nbyte++] & STRIP;
				/* if(size == 1 && type) */
				if(type)
					c = -c;
				if (nbyte > max) {
					return(0);
				}
				cursize--;
				previous[lookahead+ (--last)] = c;
			}
		} else {
		zero:
			c = string[nbyte++];
#   ifndef CBREAK
			if( c == '\0' )
			{
				/*** user break key ***/
				lookahead = 0;
#	ifdef KSHELL
				sh_fault(SIGINT);
				siglongjmp(env, UINTR);
#	endif	/* KSHELL */
			}
			previous[lookahead+ (--last)] = c;
		}
#   endif /* CBREAK */
	}
	/* shift lookahead buffer if necessary */
	if(last)
	{
		for(nbyte=last;nbyte < max;nbyte++)
			previous[lookahead+nbyte-last] = previous[lookahead+nbyte];
	}
	lookahead += max-last;
#else
	while (nbyte > 0)
	{
		c = string[--nbyte] & STRIP;
		previous[lookahead++] = (type?-c:c);
#   ifndef CBREAK
		if( c == '\0' )
		{
			/*** user break key ***/
			lookahead = 0;
#	ifdef KSHELL
			sh_fault(SIGINT);
			siglongjmp(env, UINTR);
#	endif	/* KSHELL */
		}
#   endif /* CBREAK */
	}
#endif /* SHOPT_MULTIBYTE */
	return(1);
}

/*
 * routine to perform read from terminal for vi and emacs mode
 * <mode> can be one of the following:
 *   -2		vi insert mode - key binding is in effect
 *   -1		vi control mode - key binding is in effect
 *   0		normal command mode - key binding is in effect
 *   1		edit keys not mapped
 *   2		Next key is literal
 */
int ed_getchar(int mode)
{
	register int n, c;
	char readin[LOOKAHEAD+1];
	if(!lookahead)
	{
		ed_flush();
		editb.e_inmacro = 0;
		/* The while is necessary for reads of partial multbyte chars */
		while((n=ed_read(fildes,readin, LOOKAHEAD)) > 0 && putstack(readin,n,1)==0);
	}
	if(lookahead)
	{
		/* check for possible key mapping */
		if((c = previous[--lookahead]) < 0)
		{
			if(mode<=0 && sh.st.trap[SH_KEYTRAP])
			{
				n=1;
				if((readin[0]= -c) == ESC)
				{
					while(1)
					{
						if(!lookahead)
						{
							if((c=sfpkrd(fildes,readin+n,LOOKAHEAD-n,'\r',(mode?400L:-1L),0))>0)
								putstack(readin+n,c,1);
						}
						if(!lookahead)
							break;
						if((c=previous[--lookahead])>=0)
						{
							lookahead++;
							break;
						}
						readin[n++] = -c;
						if(n>2 || c!= -'[' )
							break;
					}
				}
				if(n=keytrap(readin,n,LOOKAHEAD-n,mode))
				{
					putstack(readin,n,0);
					c = previous[--lookahead];
				}
				else
					c = ed_getchar(mode);
			}
			else
				c = -c;
		}
		/*** map '\r' to '\n' ***/
		if(c == '\r' && mode!=2)
			c = '\n';
	}
	else
		siglongjmp(env,(n==0?UEOF:UINTR));
	return(c);
}

void ed_ungetchar(register int c)
{
	if (lookahead < LOOKAHEAD)
		previous[lookahead++] = c;
	return;
}

/*
 * put a character into the output buffer
 */

void	ed_putchar(register int c)
{
	char buf[CHARSIZE];
	int size, i;
	register char *dp = editb.e_outptr;
#ifdef SHOPT_MULTIBYTE
	/* check for place holder */
	if(c == MARKER)
		return;
	if((size = wctomb(buf, (wchar_t)c)) > 1) {
		for (i = 0; i < (size-1); i++) {
			*dp++ = buf[i];
		}
	} else {
		if (size < 0) {
			buf[0] = (wchar_t)c;
			size = 1;
		}
	}
#endif	/* SHOPT_MULTIBYTE */
	if (c == '_' && size == 1)
	{
		*dp++ = ' ';
		*dp++ = '\b';
	}
	*dp++ = buf[size-1];
	*dp = '\0';
	if(dp >= editb.e_outlast)
		ed_flush();
	else
		editb.e_outptr = dp;
}

/*
 * copy virtual to physical and return the index for cursor in physical buffer
 */

ed_virt_to_phys(genchar *virt,genchar *phys,int cur,int voff,int poff)
{
	register genchar *sp = virt;
	register genchar *dp = phys;
	register int c;
	genchar *curp = sp + cur;
	genchar *dpmax = phys+MAXLINE;
	int r;
#ifdef SHOPT_MULTIBYTE
	int d;
#endif /* SHOPT_MULTIBYTE */
	sp += voff;
	dp += poff;
	for(r=poff;c= *sp;sp++)
	{
		if(curp == sp)
			r = dp - phys;
#ifdef SHOPT_MULTIBYTE
		d = wcwidth(c);
		if(d > 1)
		{
			/* multiple width character put in place holders */
			*dp++ = c;
			while(--d >0)
				*dp++ = MARKER;
			/* in vi mode the cursor is at the last character */
			if(dp>=dpmax)
				break;
			continue;
		}
		else
#endif	/* SHOPT_MULTIBYTE */
		if(d < 0)  /* c is a non-printing character */
		{
			if(c=='\t')
			{
				c = dp-phys;
				if(sh_isoption(SH_VI))
					c += editb.e_plen;
				c = TABSIZE - c%TABSIZE;
				while(--c>0)
					*dp++ = ' ';
				c = ' ';
			}

			else
			{
				*dp++ = '^';
				c ^= TO_PRINT;
			}

			/* in vi mode the cursor is at the last character */
			if(curp == sp && sh_isoption(SH_VI))
				r = dp - phys;
		}
		*dp++ = c;
		if(dp>=dpmax)
			break;
	}
	*dp = 0;
	return(r);
}

#ifdef SHOPT_MULTIBYTE
/*
 * convert external representation <src> to an array of genchars <dest>
 * <src> and <dest> can be the same
 * returns width of dest (not number of chars, which may be less)
 */

int	ed_internal(const char *src, genchar *dest)
{
	char *cp = (char *)src;
	register int c;
	register genchar *dp = dest;
	register int remainder;
	register int size;

	if((char *)dest == cp)
	{
		genchar buffer[MAXLINE];
		c = ed_internal(src,buffer);
		ed_gencpy(dp,buffer);
		return(c);
	}
	remainder = strlen(src);
	while(*cp != 0)
	{
		size = mbtowc(dp, (const char *)cp, remainder);
		dp++;
		if (size <= 0)  /* error */
			break;
		remainder -= size;
		cp += size;
	}
	*dp = 0;
	return(dp-dest);
}

/*
 * convert internal representation <src> into character array <dest>.
 * The <src> and <dest> may be the same.
 * returns number of chars in dest.
 */

int	ed_external(const genchar *src, char *dest)
{
	register genchar wc;
	int c;
	register char *dp = dest;
	register int size;
	char *dpmax = dp+sizeof(genchar)*MAXLINE-2;

	if((char*)src == dp)
	{
		genchar buffer[MAXLINE];
		c = ed_external(src,(char *)buffer);
		wcscpy(dest,buffer);
		return(c);
	}
	while((wc = *src++) && dp<dpmax)
	{
		size = wctomb(dp, wc);
		if (size ==  -1)
		{
			size = 1;
			/* copy the character as is */
			*dp = wc;
		}
		dp+= size;
	}
	*dp = 0;
	return(dp-dest);
}

/*
 * copy <sp> to <dp>
 */

void	ed_gencpy(genchar *dp,const genchar *sp)
{
	while(*dp++ = *sp++);
}

/*
 * copy at most <n> items from <sp> to <dp>
 */

void	ed_genncpy(register genchar *dp,register const genchar *sp, int n)
{
	while(n-->0 && (*dp++ = *sp++));
}

/*
 * find the string length of <str>
 */

int	ed_genlen(register const genchar *str)
{
	register const genchar *sp = str;
	while(*sp++);
	return(sp-str-1);
}
#endif /* SHOPT_MULTIBYTE */
#endif /* SHOPT_ESH || SHOPT_VSH */

#ifdef SHOPT_MULTIBYTE
/*
 * set the multibyte widths
 * format of string is x1[:y1][,x2[:y2][,x3[:y3]]]
 * returns 1 if string in not in this format, 0 otherwise.
 */

ed_setwidth(const char *string)
{
	register int indx = 0;
	register int state = 0;
	register int c;
	register int n = 0;
	static char widths[6] = {1,1};
	while(1) switch(c = *string++)
	{
		case ':':
			if(state!=1)
				return(1);
			state++;
			/* fall through */

		case 0:
		case ',':
			if(state==0)
				return(1);
			widths[indx++] = n;
			if(state==1)
				widths[indx++] = n;
			if(c==0)
			{
				for(n=1;n<= 3;n++)
				{
					int_charsize[n] = widths[c++];
					int_charsize[n+4] = widths[c++];
				}
				return(0);
			}
			else if(c==',')
				state = 0;
			n = 0;
			break;

		case '0': case '1': case '2': case '3': case '4':
			if(state&1)
				return(1);
			n = c - '0';
			state++;
			break;
			
		default:
			return(1);
	}
	/* NOTREACHED */
}
#endif /* SHOPT_MULTIBYTE */

#ifdef future
/*
 * returns 1 when <n> bytes starting at <a> and <b> are equal
 */
static int compare(register const char *a,register const char *b,register int n)
{
	while(n-->0)
	{
		if(*a++ != *b++)
			return(0);
	}
	return(1);
}
#endif

#ifdef SHOPT_OLDTERMIO

#   include	<sys/termio.h>

#ifndef ECHOCTL
#   define ECHOCTL	0
#endif /* !ECHOCTL */
char echoctl;
static char tcgeta;
static struct termio ott;

/*
 * For backward compatibility only
 * This version will use termios when possible, otherwise termio
 */


tcgetattr(int fd, struct termios *tt)
{
	register int r;
	register int i;
	tcgeta = 0;
	echoctl = (ECHOCTL!=0);
	if((r=ioctl(fd,TCGETS,tt))>=0 ||  errno!=EINVAL)
		return(r);
	if((r=ioctl(fd,TCGETA,&ott)) >= 0)
	{
		tt->c_lflag = ott.c_lflag;
		tt->c_oflag = ott.c_oflag;
		tt->c_iflag = ott.c_iflag;
		tt->c_cflag = ott.c_cflag;
		for(i=0; i<NCC; i++)
			tt->c_cc[i] = ott.c_cc[i];
		tcgeta++;
		echoctl = 0;
	}
	return(r);
}

tcsetattr(int fd,int mode,struct termios *tt)
{
	register int r;
	if(tcgeta)
	{
		register int i;
		ott.c_lflag = tt->c_lflag;
		ott.c_oflag = tt->c_oflag;
		ott.c_iflag = tt->c_iflag;
		ott.c_cflag = tt->c_cflag;
		for(i=0; i<NCC; i++)
			ott.c_cc[i] = tt->c_cc[i];
		if(tt->c_lflag&ECHOCTL)
		{
			ott.c_lflag &= ~(ECHOCTL|IEXTEN);
			ott.c_iflag &= ~(IGNCR|ICRNL);
			ott.c_iflag |= INLCR;
			ott.c_cc[VEOF]= ESC;  /* ESC -> eof char */
			ott.c_cc[VEOL] = '\r'; /* CR -> eol char */
			ott.c_cc[VEOL2] = tt->c_cc[VEOF]; /* EOF -> eol char */
		}
		switch(mode)
		{
			case TCSANOW:
				mode = TCSETA;
				break;
			case TCSADRAIN:
				mode = TCSETAW;
				break;
			case TCSAFLUSH:
				mode = TCSETAF;
		}
		return(ioctl(fd,mode,&ott));
	}
	return(ioctl(fd,mode,tt));
}
#endif /* SHOPT_OLDTERMIO */

#ifdef KSHELL
/*
 * Execute keyboard trap on given buffer <inbuff> of given size <isize>
 * <mode> < 0 for vi insert mode
 */
static int keytrap(char *inbuff,register int insize, int bufsize, int mode)
{
	static char vi_insert[2];
	static long col;
	register char *cp;
	inbuff[insize] = 0;
	col = editb.e_cur;
	if(mode== -2)
	{
		col++;
		*vi_insert = ESC;
	}
	else
		*vi_insert = 0;
	nv_putval(ED_CHRNOD,inbuff,NV_NOFREE);
	nv_putval(ED_COLNOD,(char*)&col,NV_NOFREE|NV_INTEGER);
	nv_putval(ED_TXTNOD,(char*)editb.e_inbuf,NV_NOFREE);
	nv_putval(ED_MODENOD,vi_insert,NV_NOFREE);
	sh_trap(sh.st.trap[SH_KEYTRAP],0);
	if((cp = nv_getval(ED_CHRNOD)) == inbuff)
		nv_unset(ED_CHRNOD);
	else
	{
		strncpy(inbuff,cp,bufsize);
		insize = strlen(inbuff);
	}
	nv_unset(ED_TXTNOD);
	return(insize);
}
#endif /* KSHELL */
