#ident	"@(#)OSRcmds:sh/hash.h	1.1"
/*
 *	@(#) hash.h 1.4 88/11/11 
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

/* #ident	"@(#)sh:hash.h	1.5" */
#pragma comment(exestr, "@(#) hash.h 1.4 88/11/11 ")
/*
 *	UNIX shell
 */

#define		HASHZAP		0x03FF
#define		CDMARK		0x8000

#define		NOTFOUND		0x0000
#define		BUILTIN			0x0100
#define		FUNCTION		0x0200
#define		COMMAND			0x0400
#define		REL_COMMAND		0x0800
#define		PATH_COMMAND	0x1000
#define		DOT_COMMAND		0x8800		/* CDMARK | REL_COMMAND */

#ifdef VPIX
#define         DOSFIELD        0x6000
#define         DOSDOTCOM       0x2000
#define         DOSDOTEXE       0x4000
#define         DOSDOTBAT       0x6000
#endif

#define		hashtype(x)	(x & 0x1F00)
#define		hashdata(x)	(x & 0x00FF)


typedef struct entry
{
	unsigned char	*key;
	short	data;
	unsigned char	hits;
	unsigned char 	cost;
	struct entry	*next;
} ENTRY;

extern ENTRY	*hfind();
extern ENTRY	*henter();
extern int		hcreate();
