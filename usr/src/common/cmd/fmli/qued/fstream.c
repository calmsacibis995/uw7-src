/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/fstream.c	1.5.3.3"

#include	<stdio.h>
#include	<ctype.h>
#include        <curses.h>
#include	"wish.h"
#include	"terror.h"
#include	"token.h"
#include	"winp.h"
#include	"fmacs.h"
#include	"vtdefs.h"
#include	"ctl.h"	
#include	"attrs.h"

#define MAXSUBS		5

/* ODSH functions */
extern token singleline();
extern token multiline();
extern token editsingle();
extern token editmulti();

/* dmd TCB */
static token (*Substream[][MAXSUBS])() = {
	{
		editsingle,
		singleline,
		NULL
	},
	{
		editsingle,
		editmulti,
		multiline,
		NULL
	},
};

token
field_stream(tok)
token tok;
{
	token stream();

	return(stream(tok, Substream[Currtype]));
}
