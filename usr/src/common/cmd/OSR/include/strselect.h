#ident	"@(#)OSRcmds:include/strselect.h	1.1"
/* Copyright 1994-1995 The Santa Cruz Operation, Inc. All Rights Reserved. */


#if defined(_NO_PROTOTYPE)	/* Old, crufty environment */
#include <oldstyle/strselect.h>
#elif defined(_SCO_ODS_30) /* Old, Tbird compatible environment */
#include <ods_30_compat/strselect.h>
#else 	/* Normal, default environment */
/*
 *   Portions Copyright (C) 1983-1995 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

#ifndef	_STRSELECT_H
#define	_STRSELECT_H

#pragma comment(exestr, "xpg4plus @(#) strselect.h 20.1 94/12/04 ")

/*	defines for the STRSELECT construct
	for selecting among character strings
	Typical use:

		char	string[100];

		gets( string );
		STRSELECT( string )
		WHEN2( "y", "yes" )	/* only accept "y" or "yes" *
			yescode();
		WHENN( "n" )		/* anything beginning "n" is no *
			nocode();
		DEFAULT
			tryagain();
		ENDSEL
	Note:
		No {}s are used.
		No : after WHEN().
		WHENs do not fall through.
		WHENs are evaluated in order.
		DEFAULT should be last, if present.
*/

#define	STRSELECT(a)	{  char *STRSeLeCT; STRSeLeCT = a; if (0) {

#define	WHEN(a)	} else if( !strcmp(STRSeLeCT, a) ) {

#define	WHEN2(a1,a2)	} else if( !strcmp(STRSeLeCT, a1) \
			||  !strcmp(STRSeLeCT, a2) ) {

#define	WHEN3(a1,a2,a3)	} else \
		if( !strcmp(STRSeLeCT, a1)  ||  !strcmp(STRSeLeCT, a2)  ||\
		!strcmp(STRSeLeCT, a3) ) {

#define	WHENN(a)	} else \
		if( !strncmp(STRSeLeCT, a, sizeof(a)-1) ) {

#define	WHENN2(a1,a2)	} else \
		if( !strncmp(STRSeLeCT, a1, sizeof(a1)-1)  ||\
		!strncmp(STRSeLeCT, a2, sizeof(a2)-1) ) {

#define	WHENN3(a1,a2,a3)	} else \
		if( !strncmp(STRSeLeCT, a1, sizeof(a1)-1)  ||\
		!strncmp(STRSeLeCT, a2, sizeof(a2)-1)  ||\
		!strncmp(STRSeLeCT, a3, sizeof(a3)-1) ) {

#define	DEFAULT	} else {

#define	ENDSEL	}  }

#endif	/* _STRSELECT_H  */
#endif
