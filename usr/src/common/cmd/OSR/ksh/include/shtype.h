#ident	"@(#)OSRcmds:ksh/include/shtype.h	1.1"
#pragma comment(exestr, "@(#) shtype.h 25.4 94/08/24 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992.			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * Modification History
 *
 *	L000		scol!markhe	18 Nov 92
 *	- use is*() macros from ctype.h so that they are affected by the
 *	  current locale (only when built internationalised).
 *	- add two new macros ispalpha() (is character in portable character
 *	  set) and ispalnum() (is character in portable character set or
 *	  a digit '0' to '9').  Needed as a 'word' (as defined in POSIX2 and
 *	  XPG4) can only consists of these characters).
 *	L001		scol!anthonys	24 Aug 94
 *	- Don't attempt to create macro versions of "toupper()/tolower()"
 *	  if they doesn't exist in <ctype.h>, since they will almost certainly
 *	  be incorrect (as were the ones I just removed).
 */

/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	AT&T Bell Laboratories
 *
 */

#include	"sh_config.h"
#ifdef	INTL							/* L000 begin */
#  define	_USE_MACROS
#  include	<ctype.h>
#endif	/* INTL */						/* L000 end */

/* table 1 */
#define QUOTE	(~0177)
#define T_PSC	01	/* Special pattern characters, insert escape */
#define T_MET	02
#define	T_SPC	04
#define T_DIP	010
#define T_EXP	020	/* characters which require expansion or substitution */
#define T_EOR	040
#define T_QOT	0100
#define T_ESC	0200

/* table 2 */
#define T_ALP	01	/* alpha, but not upper or lower */
#define T_DEF	02
#define T_PAT	04	/* special chars when preceding () */
#define	T_DIG	010	/* digit */
#define T_UPC	020	/* uppercase only */
#define T_SHN	040	/* legal parameter characters */
#define	T_LPC	0100	/* lowercase only */
#define T_SET	0200

/* for single chars */
#define _TAB	(T_SPC)
#define _SPC	(T_SPC)
#define _ALP	(T_ALP)
#define _UPC	(T_UPC)
#define _LPC	(T_LPC)
#define _DIG	(T_DIG)
#define _EOF	(T_EOR)
#define _EOR	(T_EOR)
#define _BAR	(T_DIP|T_PSC)
#define _BRA	(T_MET|T_DIP|T_EXP|T_PSC)
#define _KET	(T_MET|T_PSC)
#define _AMP	(T_DIP|T_PSC)
#define _SEM	(T_DIP)
#define _LT	(T_DIP)
#define _GT	(T_DIP)
#define _LQU	(T_QOT|T_ESC|T_EXP)
#define _QU1	T_EXP
#define _AST1	T_EXP
#define _BSL	(T_ESC|T_PSC)
#define _DQU	(T_QOT)
#define _DOL1	(T_ESC|T_EXP)

#define _CKT	T_DEF
#define _AST	(T_PAT|T_SHN)
#define _EQ	(T_DEF)
#define _MIN	(T_DEF|T_SHN)
#define _PCT	(T_DEF|T_SET)
#define _PCS	(T_SHN|T_PAT)
#define _NUM	(T_DEF|T_SHN|T_SET)
#define _DOL2	(T_SHN|T_PAT)
#define _PLS	(T_DEF|T_SET|T_PAT)
#define _AT	(T_SHN|T_PAT)
#define _QU	(T_DEF|T_SHN|T_PAT)
#define _LPAR	T_SHN
#define _SS2	T_ALP
#define _SS3	T_ALP

/* abbreviations for tests */
#define _IDCH	(T_UPC|T_LPC|T_DIG|T_ALP)
#define _META	(T_SPC|T_DIP|T_MET|T_EOR)

extern const unsigned char	_ctype1[];

/* these args are not call by value !!!! */
#ifndef	isblank						/* L000 */
#define	isblank(c)	(_ctype1[c]&(T_SPC))
#endif	/* isblank */					/* L000 */
#ifndef	isspace						/* L000 */
#define	isspace(c)	(_ctype1[c]&(T_SPC|T_EOR))
#endif	/* isspace */					/* L000 */
#define ismeta(c)	(_ctype1[c]&(_META))
#define isqmeta(c)	(_ctype1[c]&(_META|T_QOT))
#define qotchar(c)	(_ctype1[c]&(T_QOT))
#define eolchar(c)	(_ctype1[c]&(T_EOR))
#define dipchar(c)	(_ctype1[c]&(T_DIP))
#define addescape(c)	(_ctype1[c]&(T_PSC))
#define escchar(c)	(_ctype1[c]&(T_ESC))
#define expchar(c)	(_ctype1[c]&(T_EXP|_META|T_QOT))
#define isexp(c)	(_ctype1[c]&T_EXP)
#define iochar(c)	((c)=='<'||(c)=='>')

extern const unsigned char	_ctype2[];

#ifndef	isprint							/* L000 */
#define	isprint(c)	(((c)&0340) && ((c)!=0177))
#endif	/* isprint */						/* L000 */
#ifndef	isdigit							/* L000 */
#define	isdigit(c)	(_ctype2[c]&(T_DIG))
#endif	/* isdigit */						/* L000 */
#define dolchar(c)	(_ctype2[c]&(_IDCH|T_SHN))
#define patchar(c)	(_ctype2[c]&(T_PAT))
#define defchar(c)	(_ctype2[c]&(T_DEF))
#define setchar(c)	(_ctype2[c]&(T_SET))
#ifndef isalpha							/* L000 */
#define	isalpha(c)	(_ctype2[c]&(T_UPC|T_LPC|T_ALP))
#endif	/* isalpha */						/* L000 */
#define	ispalpha(c)	(_ctype2[c]&(T_UPC|T_LPC|T_ALP))	/* L000 */
#ifndef	isalnum							/* L000 */
#define isalnum(c)	(_ctype2[c]&(_IDCH))
#endif	/* isalnum */						/* L000 */
#define	ispalnum(c)	(_ctype2[c]&(_IDCH))			/* L000 */
#ifndef	isupper							/* L000 */
#define isupper(c)	(_ctype2[c]&(T_UPC))
#endif	/* isupper */						/* L000 */
#ifndef	islower							/* L000 */
#define islower(c)	(_ctype2[c]&(T_LPC))
#endif	/* islower */						/* L000 */
#define astchar(c)	((c)=='*'||(c)=='@')
