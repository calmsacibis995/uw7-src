#ident	"@(#)OSRcmds:ksh/sh/print.c	1.1"
#pragma comment(exestr, "@(#) print.c 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

/*
 * Modification History
 *
 *	L000	scol!markhe (from scol!hughd)	4 Nov 92
 *	- removed local definition of TIC_SEC, use Hz from gethz() instead
 *
 *	L001	scol!markhe	23 Nov 92
 *	- quote characters that are *not* alphabetics of the portable
 *	  character set or digits '0' to '9',
 *	- "sh_config" is included from "defs.h", so don't need it here
 *	- make numbering on the output from p_list() optional.  kill -l is
 *	  not allowed to display the numbered list for POSIX2/XPG4.
 */

#include	"defs.h"
#include	"builtins.h"

/* This module references the following external */
extern void	nam_rjust();

/* printing and io conversion */

/*
 *  flush the output queue and reset the output stream
 */

void	p_setout(fd)
register int fd;
{
	register struct fileblk *fp;
	register int count;
	if(!(fp=io_ftable[fd]))
		fp = io_ftable[fd] = &io_stdout;
	else if(fp->flag&IOREAD)
	{
		if(count=fp->last-fp->ptr)
			lseek(fd,-((off_t)count),SEEK_CUR);
		fp->fseek = 0;
		fp->ptr = fp->base;
	}
	fp->last = fp->base + IOBSIZE;
	fp->flag &= ~(IOREAD|IOERR|IOEOF);
	fp->flag |= IOWRT;
	if(output==fd)
		return;
	if(fp = io_ftable[output])
		if(io_ftable[fd]==fp || (fp->flag&IOSLOW))
			p_flush();
	output = fd;
}

/*
 * flush the output if necessary and null terminate the buffer
 */

void p_flush()
{
	register struct fileblk *fp = io_ftable[output];
	register unsigned count;
	if(fp)
	{
		if(count=fp->ptr-fp->base)
		{
			if(write(output,fp->base,count) < 0)
				fp->flag |= IOERR;
			if(sh.heretrace)
				write(ERRIO,fp->base,count);
			fp->ptr = fp->base;
		}
		/* leave buffer as a null terminated string */
		*fp->ptr = 0;
	}
}

/*
 * print a given character
 */

void	p_char(c)
register int c;
{
	register struct fileblk *fp = io_ftable[output];
	if(fp->ptr >= fp->last)
		p_flush();
	*fp->ptr++ = c;
}

/*
 * print a string optionally followed by a character
 * The buffer is always terminated with a zero byte.
 */

void	p_str(string,c)
register const char *string;
int c;
{
	register struct fileblk *fp = io_ftable[output];
	register int cc;

	while(1)
	{
		if((cc= *string)==0)
			cc = c,c = 0;
		else
			string++;
		if(fp->ptr >= fp->last)
			p_flush();
		*fp->ptr = cc;
		if(cc==0)
			break;
		fp->ptr++;
	}
}

/*
 * print a given character a given number of times
 */

void	p_nchr(c,n)
register int c,n;
{
	register struct fileblk *fp = io_ftable[output];
	while(n-- > 0)
	{
		if(fp->ptr >= fp->last)
			p_flush();
		*fp->ptr++ = c;
	}
}
/*
 * print a message preceded by the command name
 */

void p_prp(s1)
const char *s1;
{
	register unsigned char *cp;
	register int c;
	if(cp=(unsigned char *)st.cmdadr)
	{
		if(*cp=='-')
			cp++;
		c = ((st.cmdline>1)?0:':');
		p_str((char*)cp,c);
		if(c==0)
			p_sub(st.cmdline,':');
		p_char(SP);
	}
	if(cp = (unsigned char*)s1)
	{
		for(;c= *cp;cp++)
		{
			if(!isprint(c))
			{
				p_char('^');
				c ^= TO_PRINT;
			}
			p_char(c);
		}
	}
}

/*
 * print a time and a separator 
 */

void	p_time(t,c)
#ifndef pdp11
    register
#endif /* pdp11 */
clock_t t;
int c;
{
	register int  min, sec, frac;
	register int hr;

	if (Hz <= 0 && (Hz = gethz()) <= 0)			/* L000 begin */
		Hz = HZ;		/* defined in sh_config.h */
	frac = t % Hz;
	frac = (frac*100)/Hz;
	t /= Hz;						/* L000 end */
	sec=t%60; t /= 60;
	min=t%60;
	if(hr=t/60)
	{
		p_num(hr,'h');
	}
	p_num(min,'m');
	p_num(sec,'.');
	if(frac<10)
		p_char('0');
	p_num(frac,'s');
	p_char(c);
}

/*
 * print a number optionally followed by a character
 */

void	p_num(n,c)
int 	n;
int c;
{
	p_str(sh_itos(n),c);
}


/* 
 * print a list of arguments in columns
 */
#define NROW	15	/* number of rows in output before going to multi-columns */
#define LBLSIZ	3	/* size of label field and interfield spacing */

void	p_list(argn,com,flag)					/* L001 */
char *com[];
{
	register int i,j;
	register char **arg;
	char a1[12];
	int nrow;
	int ncol = 1;
	int ndigits = 1;
	int fldsize;
#if ESH || VSH
	int wsize = ed_window();
#else
	int wsize = 80;
#endif
	char *cp = nam_fstrval(LINES);
	nrow = (cp?1+2*(atoi(cp)/3):NROW);
	if (flag)						/* L001 */
		for(i=argn;i >= 10;i /= 10)			/* L001 */
			ndigits++;				/* L001 */
	if(argn < nrow)
	{
		nrow = argn;
		goto skip;
	}
	i = 0;
	for(arg=com; *arg;arg++)
	{
		i = max(i,strlen(*arg));
	}
	if (flag)						/* L001 */
		i += (ndigits+LBLSIZ);				/* L001 */
	else							/* L001 */
		i++;						/* L001 */
	if(i < wsize)
		ncol = wsize/i;
	if(argn > nrow*ncol)
	{
		nrow = 1 + (argn-1)/ncol;
	}
	else
	{
		ncol = 1 + (argn-1)/nrow;
		nrow = 1 + (argn-1)/ncol;
	}
skip:
	fldsize = (wsize/ncol)-(ndigits+LBLSIZ);
	for(i=0;i<nrow;i++)
	{
		j = i;
		while(1)
		{
			arg = com+j;
			if (flag) {				/* L001 begin */
				strcpy(a1,sh_itos(j+1));
				nam_rjust(a1,ndigits,' ');
				p_str(a1,')');
				p_char(SP);
				p_str(*arg,0);
				j += nrow;
				if(j >= argn)
					break;
				p_nchr(SP,fldsize-strlen(*arg));
			} else {
#ifdef	POSIX
				p_qstr(*arg, 0);
#else
				p_str(*arg, 0);
#endif	/* POSIX */
				j += nrow;
				if (j >= argn)
					break;
				p_char(' ');
			}					/* L001 end */
		}
		newline();
	}
}

/*
 * Print a number enclosed in [] followed by a character
 */

void	p_sub(n,c)
register int n;
register int c;
{
	p_char('[');
	p_num(n,']');
	if(c)
		p_char(c);
}

#ifdef POSIX
/*
 * print <str> qouting chars so that it can be read by the shell
 * terminate with the character <cc>
 */
void	p_qstr(str,cc)
char *str;
{
	register char *cp = str;
	register int c = *cp;
	register int state = (c==0);
	do
	{
		if(ispalpha(c))					/* L001 */
		{
			while((c = *++cp),ispalnum(c));		/* L001 */
			if(c=='=')
			{
				*cp = 0;
				p_str(str,c);
				*cp++ = c;
				str = cp;
				c = *cp;
			}
		}
		if(c=='~')
			state++;
		while((c = *cp++) && (c!= '\''))
			state |= _ctype1[c];
		if(c || state)
		{
			/* needs single quotes */
			p_char('\'');
			if(c)
			{
				/* string contains single quote */
				c = *cp;
				*cp = 0;
				state = '\\';
			}
			else
				state = '\'';
			p_str(str,state);
			if(c)
				*cp = c;
			str = (cp-1);
		}
	}
	while(c);
	p_str(str,cc);
}
#endif /* POSIX */
