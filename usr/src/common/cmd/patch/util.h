/*	copyright	"%c%"	*/
/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)patch_p2:util.h	1.2"

/*
 * OSF/1 1.2
 */
/* @(#)$RCSfile$ $Revision$ (OSF) $Date$ */

/* Header: util.h,v 2.0 86/09/17 15:40:06 lwall Exp
 *
 * Log:	util.h,v
 * Revision 2.0  86/09/17  15:40:06  lwall
 * Baseline for netwide release.
 * 
 */

#define	_PATH_TTY	"/dev/tty"
/* and for those machine that can't handle a variable argument list */

#ifdef CANVARARG

#define say1 say
#define say2 say
#define say3 say
#define say4 say
#define ask1 ask
#define ask2 ask
#define ask3 ask
#define ask4 ask
#define fatal1 fatal
#define fatal2 fatal
#define fatal3 fatal
#define fatal4 fatal

#else /* hope they allow multi-line macro actual arguments */

#ifdef lint

#define say1(a) say(a, 0, 0, 0)
#define say2(a,b) say(a, (b)==(b), 0, 0)
#define say3(a,b,c) say(a, (b)==(b), (c)==(c), 0)
#define say4(a,b,c,d) say(a, (b)==(b), (c)==(c), (d)==(d))
#define ask1(a) ask(a, 0, 0, 0)
#define ask2(a,b) ask(a, (b)==(b), 0, 0)
#define ask3(a,b,c) ask(a, (b)==(b), (c)==(c), 0)
#define ask4(a,b,c,d) ask(a, (b)==(b), (c)==(c), (d)==(d))
#define fatal1(a) fatal(a, 0, 0, 0)
#define fatal2(a,b) fatal(a, (b)==(b), 0, 0)
#define fatal3(a,b,c) fatal(a, (b)==(b), (c)==(c), 0)
#define fatal4(a,b,c,d) fatal(a, (b)==(b), (c)==(c), (d)==(d))

#else /* lint */
    /* if this doesn't work, try defining CANVARARG above */
#define say1(a) say(a, Nullch, Nullch, Nullch)
#define say2(a,b) say(a, b, Nullch, Nullch)
#define say3(a,b,c) say(a, b, c, Nullch)
#define say4 say
#define ask1(a) ask(a, Nullch, Nullch, Nullch)
#define ask2(a,b) ask(a, b, Nullch, Nullch)
#define ask3(a,b,c) ask(a, b, c, Nullch)
#define ask4 ask
#define fatal1(a) fatal(a, Nullch, Nullch, Nullch)
#define fatal2(a,b) fatal(a, b, Nullch, Nullch)
#define fatal3(a,b,c) fatal(a, b, c, Nullch)
#define fatal4 fatal

#endif /* lint */

/* if neither of the above work, join all multi-line macro calls. */
#endif

extern char serrbuf[BUFSIZ];		/* buffer for stderr */

char *fetchname();
int move_file();
void copy_file();
void say();
void fatal();
void ask();
char *savestr();
void set_signals();
void ignore_signals();
void makedirs();
void append_file(char *, char *, int *);
char *mapinput(char *);
void cleanmap(void);
