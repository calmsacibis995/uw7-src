#ifndef	NOIDENT
#ident	"@(#)oldattlib:Xargs.h	1.1"
#endif
/*
 Xargs.h (C header file)
	Acc: 575327073 Fri Mar 25 16:04:33 1988
	Mod: 575321561 Fri Mar 25 14:32:41 1988
	Sta: 575321561 Fri Mar 25 14:32:41 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 12/07/87
************************************************************************/

#ifndef _XARGS_H
#define _XARGS_H

typedef struct
{
	char *		xopt;	/* in .Xdefaults	*/
	char *		copt;	/* command line option	*/
	char *		popt;	/* default value	*/
	char *		vptr;	/* address of data	*/
	int		(*f)();	/* processing function	*/
	char *		arg;	/* passed to f		*/
}
	Option;

extern char *		ExtractDisplay ();
extern char *		ExtractGeometry ();
extern int		ExtractOptions ();

extern int		OptString ();
extern int		OptBoolean ();
extern int		OptInverse ();
extern int		OptInt ();
extern int		OptLong ();
extern int		OptFloat ();
extern int		OptDouble ();
extern int		OptColor ();
extern int		OptFont ();
extern int		OptFILE ();
extern int		OptEnum ();

#endif
