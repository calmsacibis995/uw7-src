#ident	"@(#)OSRcmds:ksh/sh/echo.c	1.1"
#pragma comment(exestr, "@(#) echo.c 25.2 92/12/11 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1992.
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
 * This is the code for the echo and print command
 */

#ifdef KSHELL
#   include	"defs.h"
#endif	/* KSHELL */

#ifdef __STDC__
#   define ALERT	'\a'
#else
#   define ALERT	07
#endif /* __STDC__ */

/*
 * echo the argument list
 * if raw is non-zero then \ is not a special character.
 * returns 0 for \c otherwise 1.
 */

int echo_list(raw,com)
int raw;
char *com[];
{
	register int outc;
	register char *cp;
	while(cp= *com++)
	{
		if(!raw) for(; *cp; cp++)
		{
			outc = *cp;
			if(outc == '\\')
			{
				switch(*++cp)
				{
					case 'a':
						outc = ALERT;
						break;
					case 'b':
						outc = '\b';
						break;
					case 'c':
						return(0);
					case 'f':
						outc = '\f';
						break;
					case 'n':
						outc = '\n';
						break;
					case 'r':
						outc = '\r';
						break;
					case 'v':
						outc = '\v';
						break;
					case 't':
						outc = '\t';
						break;
					case '\\':
						outc = '\\';
						break;
					case '0':
					{
						register char *cpmax;
						outc = 0;
						cpmax = cp + 4;
						while(++cp<cpmax && *cp>='0' && 
							*cp<='7')
						{
							outc <<= 3;
							outc |= (*cp-'0');
						}
						cp--;
						break;
					}
					default:
					cp--;
				}
			}
			p_char(outc);
		}
#ifdef POSIX
		else if(raw>1)
			p_qstr(cp,0);
#endif /* POSIX */
		else
			p_str(cp,0);
		if(*com)
			p_char(' ');
#ifdef KSHELL
		if(sh.trapnote&SIGSET)
			sh_exit(SIGFAIL);
#endif	/* KSHELL */
	}
	return(1);
}

