#ident	"@(#)OSRcmds:ksh/shlib/cannon.c	1.1"
#pragma comment(exestr, "@(#) cannon.c 25.5 93/08/04 ")
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
 *  pathcanon - Generate canonical pathname from given pathname.
 *  This routine works with both relative and absolute paths.
 *  Relative paths can contain any number of leading ../ .
 *  Each pathname is checked for access() before each .. is applied and
 *     NULL is returned if not accessible
 *  A pointer to the end of the pathname is returned when successful
 *  The operator ... is also expanded by this routine when LIB_3D is defined
 *  In this case length of final path may be larger than orignal length
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 3C-526B
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 */

/*
 * Modification History
 *
 *	L000	scol!markhe	4 Nov 92
 *	- added //machine_name/path support, allowing logical cd's to work
 *	  along side
 *	  (carried over from previous version)
 *	L001	scol!markhe	25 Nov 92
 *	- "io.h" needs the definition of 'MSG', so include "defs.h" (which
 *	  also happens to include "io.h")
 *	L002	scol!gregw	26 Mar 93
 *	- Increased size of cwdname[] to match that of Bourne shell. This makes
 *	  ksh behave in the same way as sh with very long path names, > 1024.
 *	L003	scol!ianw	04 Aug 93
 *	- netsysent() (which calls the xnet_sys() system call) when called
 *	  with a command of ISREMOTE returns 1 if remote and zero if not.
 *	  Corrected the check of the return value from netsysent(), there
 *	  is only a remote machine name to copy if 1 is returned.
 */

#include	"defs.h"					/* L001 */

/*								   L000 begin
 * the following defns from XNET file nsysnet.h but
 * that's not currently available to the build so:
 */
#define	ISREMOTE	0x0086		/* is current dir remote */
#define	REMCD		0x0093		/* get remote current directory */
#define	MACH_MAX	19		/* 16 chars, 2 delimiters, a null */
#define	MAXPATH	(MACH_MAX + PATH_MAX)	/* max pathname with machine name */

#define	netsysent(command, argp)	netsyscall(command, argp, 0, 0)
					/* Network system call */
	

struct isnet
{
	int	n_delsiz;
	char	n_del1;
	char	n_del2;
	char n_netnm[16];
};

char machine[MACH_MAX];
char cwdname[MAXPATH+(MAXPATH/2)];	/* L002 */
int isremote = 0;
static struct isnet net;		/*  pwd is remote */
static void netname(char *);
static void del_mchnm(char *);
static char *skipdelim(char *s);
static int netsyscall(int, struct isnet *, int, int);		/* L000 end */


char	*pathcanon(path, flag)					/* L000 */
char *path;
int  flag;							/* L000 */
{
	register char *dp=path;
	register int c = '/';
	register char *sp;
	register char *begin=dp;
#ifdef LIB_3D
	extern char *pathnext();
#endif /* LIB_3D */
#ifdef PDU
	/* Take out special case for /../ as */
	/* Portable Distributed Unix allows it */
	if ((*dp == '/') && (*++dp == '.') &&
	    (*++dp == '.') && (*++dp == '/') &&
	    (*++dp != 0))
		begin = dp = path + 3;
	else
		dp = path;
#endif /* PDU */
	/* if (flag) - Irrelevant for UW */		/* L000 begin */
		/* take out machine name and put into machine[] */
		/* netname(dp); - Irrelevant for UW */
	/* else - Not required for UW */
		/*
		 * parse over any machine name (the machine name
		 * starts with a double slash and continues until
		 * the next slash)
		 */
		if (*dp == '/' && *(dp+1) == '/') {
			dp += 2;
			while (*dp != '/')
				dp++;
			if (!*dp)
				return(dp);
			begin = dp;
		}						/* L000 end */

	if(*dp != '/')
		dp--;
	sp = dp;
	while(1)
	{
		sp++;
		if(c=='/')
		{
#ifdef apollo
			if(*sp == '.')
#else
			if(*sp == '/')
				/* eliminate redundant / */
				continue;
			else if(*sp == '.')
#endif /* apollo */
			{
				c = *++sp;
				if(c == '/')
					continue;
				if(c==0)
					break;
				if(c== '.')
				{
					if((c= *++sp) && c!='/')
					{
#ifdef LIB_3D
						if(c=='.')
						{
							char *savedp;
							int savec;
							if((c= *++sp) && c!='/')
								goto dotdotdot;
							/* handle ... */
							savec = *dp;
							*dp = 0;
							savedp = dp;
							dp = pathnext(path,sp);
							if(dp)
							{
								*dp = savec;
								sp = dp;
								if(c==0)
									break;
								continue;
							}
							dp = savedp;
							*dp = savec;
						dotdotdot:
							*++dp = '.';
						}
#endif /* LIB_3D */
					dotdot:
						*++dp = '.';
					}
					else /* .. */
					{
						if(dp>begin)
						{
							dp[0] = '/';
							dp[1] = '.';
							dp[2] = 0;
							if(access(path,0) < 0)
								return((char*)0);
							while(*--dp!='/')
								if(dp<begin)
									break;
						}
						else if(dp < begin)
						{
							begin += 3;
							goto dotdot;
						}
						if(c==0)
							break;
						continue;
					}
				}
				*++dp = '.';
			}
		}
		if((c= *sp)==0)
			break;
		*++dp = c;
	}
#ifdef LIB_3D
	*++dp= 0;
#else
	/* remove all trailing '/' */
	if(*dp!='/' || dp<=path)
		dp++;
	*dp= 0;
#endif /* LIB_3D */
	return(dp);
}

/* If the remote part of this stuff, which didn;t exist in the UnixWare
 * version of ksh, is not needed, then all the below should really be
 * removed
 */

/*								   L000 begin
 *  remote() sets up the structure 'net', which contains
 *  machine name, and network delimiter information.  The
 *  return value of remote indicates whether the current
 *  directory is remote (1) or local (0).
 *
 *  We save machine names that were typed by the user
 *  during a cd by calling netname(). The stack will only
 *  contain the pathname. It will be copied into sh.pwd.
 *  cwdname[] will contain the full
 *  path i.e. //machine/pathname. This makes it easier to
 *  recognize machine names later when a pwd is done.
 *  If our current directory is really remote, we actually
 *  print the contents of cwdname, else we print out sh.pwd.
 *
 */
remote()
{
	register int i;
	register char *ptr;
	extern int errno;

	machine[0] = '\0';
	if ((isremote = netsysent(ISREMOTE, &net)) == 1)	/* L003 */
	{
		i = net.n_delsiz;
		if (i == 1 ) {
			ptr = &net.n_del2;
			*ptr = net.n_del1;
		}
		else if (i == 2)
			ptr = &net.n_del1;
		i += sizeof(net.n_netnm);
		strncpy(machine, ptr, i );
		machine[i] = '\0';
		return(isremote );
	}
	return(0);	/* assume local if ISREMOTE failed */
}

static void
netname(char *p)
{
	remote();
	del_mchnm(p);
}

static void
del_mchnm(register char *s)
{
	register char *ptr;
	int len;
	char *skipdelim();

	/* if path is //machine, append a / at the end */
	if ((ptr = skipdelim(s)) == s) /* go beyond extraneous delimiters */
		return;
	ptr -= net.n_delsiz;    /* only consider last two delimiters */
	len = strlen(machine);
	if (strncmp(machine, ptr, len) == 0) {
		ptr += len;
		sh_copy(ptr, s);
	}
	if (*s == '\0' ) {
		*s = '/';
		*(s+1) = '\0';
	}
}

static char *
skipdelim(register char *s)
{
	register char c;
	char *save = s;

	if (net.n_delsiz == 0 )
		remote();	/* fill in net delim info */
	while ((c = *s) && (c == net.n_del1 || c == net.n_del2))
		++s;
	if (c && s - save >= net.n_delsiz)
		return(s);
	else
		return(save);
}

static int
netsyscall(int a, struct isnet *b, int c, int d)
{
	return syscall(0x1528, a, b, c, d);
}

/* machine[]	- machine name (local/remote) */
/* sh.pwd	- absolute path of pwd */
/* cwdname[]	- machine_name + sh.pwd */
cwd(char *path)
{
	/* append cannonical path to machine name and store in cwdname[]. */
	strcpy(cwdname, machine);
	strcat(cwdname, path);
}								/* L000 end */
