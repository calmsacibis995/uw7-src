#ident	"@(#)OSRcmds:csh/doprnt.c	1.1"
/*
 *	@(#) doprnt.c 1.4 88/11/11 
 *
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/* #ident	"@(#)csh:doprnt.c	1.2" */
#pragma comment(exestr, "@(#) doprnt.c 1.4 88/11/11 ")

/*
 *	@(#) doprnt.c 1.1 88/03/29 csh:doprnt.c
 */

/*
 *      doprnt - Portable Version of doprnt.s
 *
 * Modification History
 *	arw	10 Feb 1982	M000
 *		- Fixed a bug in which formats with > 128 characters
 *		  between format specifiers caused array bound problems
 *		  in the static work area.
 *	jjd	25 May 1982	M001
 *		- Made code portable for 16- and 32-bit compilers.
 *	Hans	4 June 1982	M002
 *		- made crack use unsigned.
 */

# include <stdio.h>
static		crack();
static		crack2();
static	int	gnum();
static		getuw();
static		getsw();
static		getud();
static		getsd();

/* string build area.
 *      numbers are cracked into stwork, in which case strfwa == &stwork[0]
 *      strings and sections of formats are printed from where they lay,
 *		in which case strfwa = fwa of string
 */
static  char    stwork[128];
static  char    *strfwa;
static  char    *wp;            /* string pointer */

static  long    work;                      /* number to crack */

static  int     width;
static  int     rjust;
static  int     ndfnd;
static  int     ndigit;
static  int     zfill;
static  int     *args;
static  struct  _iobuf  *file;

_doprnt (formp, ar, fi)
	register char *formp;
	int *ar;
	struct _iobuf *fi;
{
	register int i;
	register int pad;

	args = ar;
	file = fi;
loop:
	/* begin M000 ...		print format string sections */
	strfwa = formp;
	while ((*formp) && (*formp != '%'))
		formp++;
	if (strfwa != formp)
		_strout(strfwa, formp-strfwa, 0, file, '\0');
	/* ... end M000 */

	if (*formp == 0)
		return;

	/* have detected a % character. process format */
	rjust = 0;
	ndigit = 0;
	strfwa = wp = &stwork[0];
	zfill = ' ';
	formp++;                        /* skip % */
	if (*formp == '-') {
		formp++;
		rjust++;
	}
	if (*formp == '0')
		zfill = *formp++;
	width = gnum(formp);            /* get first number */
	formp += ndfnd;
	ndfnd = 0;
	if (*formp == '.') {
		formp++;
		ndigit = gnum(formp);   /* get digit count */
		formp += ndfnd;
	}
	switch (*formp++) {

	default:			/* unrecognized or '%' */
		*wp++ = formp[-1];
		break;

	case 'o':                       /* octal */
		getuw();
		crack (8);
		break;

	case 'O':                       /* long octal */
		getud();
		crack (8);
		break;

	case 'x':                       /* hex */
		getuw();
		crack (16);
		break;

	case 'X':                       /* long hex */
		getud();
		crack (16);
		break;

	case 'd':                       /* decimal */
		getsw();
		crack (10);
		break;

	case 'D':                       /* long decimal */
		getsd();
		crack (10);
		break;

	case 'u':                       /* unsigned */
		getuw();
		crack (10);
		break;

	case 'U':                       /* long unsigned */
		getud();
		crack (10);
		break;

	case 'g':                       /* general */
		pgen (&args, &wp, ndigit, ndfnd);
		break;

	case 'c':                       /* character */
		zfill = ' ';
		if ( (*wp++ = (char)(*args++ & 0377)) == 0)
			wp--;
		break;

	case 's':                       /* string */
		zfill = ' ';
		strfwa = (char *)(*args++);     /* get string pointer */
		if (strfwa == (char *)0)
			strfwa = "(null)";      /* print (null) if null */
		wp = strfwa;
		while  (*wp) {
			*wp++;
			if (--ndigit == 0)      /* count string */
				break;
		}
		break;

	case 'l':
	case 'L':
		switch (*formp++) {

		case 'o':                       /* octal */
			getud();
			crack (8);
			break;

		case 'x':                       /* hex */
			getud();
			crack (16);
			break;

		case 'd':                       /* dec */
			getsd();
			crack (10);
			break;

		case 'u':                       /* unsigned */
			getud();
			crack (10);
			break;

		default:
			formp--;
			getuw();
			crack(10);
		}
		break;

	case 'f':                               /* floating */
		pfloat (&args, &wp, ndigit, ndfnd);
		break;

	case 'e':                               /* scientific */
		pscien (&args, &wp, ndfnd? ndigit+1:7, ndfnd);
		break;

	case 'r':                               /* remote */
		args = (int *)*args;
		formp = (char *)*args++;
		goto loop;
	}

	/* Have decoded the item to print into a string
	 *
	 *      (wp) = pointer to last char+1
	 *      (strfwa) = pointer to first char
	 */

	i = wp - strfwa;                  /* compute length */
	pad = (i < width)? width-i:0;
	if (rjust == 0)
		pad = -pad;
	_strout(strfwa, i, pad, file, zfill);
	goto loop;
}

/*      crack (base) - crack the number in the specified base
 *
 */
static
crack (base)
{

	if ((work != 0) && (ndigit))
		*wp++ = '0';            /* force a 00 */
	crack2 (base);                  /* decode it */
}

static
crack2 (base)
    register unsigned base;	/* M002 */
{
	register unsigned c;	/* M002 */

	c = work % base;
	work = work / base;
	if (work)
		crack2 (base);
	if (c < 10)
		*wp++ = (char)(c + '0');
	else
		*wp++ = (char)(c - 10 + 'a');
}



/*      getuw - get unsigned word into work
 *      getsw - get   signed word into work
 *      getud - get unsigned double into work
 *      getsd - get   signed double into work
 */

static
getuw()
{

	work = (unsigned)*args++;
}

static
getsw()
{

	work = *args++;
	if (work < 0) {
		*wp++ = '-';
		work = -work;
	}
}

static
getud()
{

	work = *((long *)args)++;		/*M001*/
}

static
getsd()
{

	work = *((long *)args)++;		/*M001*/
	if (work < 0) {
		*wp++ = '-';
		work = -work;
	}
}

/*
 *      gnum - crack number from 'formp' string
 *
 *      cracks characters from *formp.  Returns the number of characters
 *      read in ndfnd.
 */

static  int
gnum(formp)
    register char *formp;
{
	register int accum = 0;

	ndfnd = 0;              /* no digit found */
loop:
	if (*formp == '*') {
		ndfnd++;
		formp++;
		return (*args++);        /* use value from arg list */
	}
	if ((*formp < '0') || (*formp > '9'))
		return (accum);
	accum = accum*10 + *formp++ - '0';
	ndfnd++;
	goto loop;
}
