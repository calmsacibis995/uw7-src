#ident	"@(#)cmdtab.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)cmdtab.c	1.12 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "def.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Define all of the command names and bindings.
 */

struct cmd cmdtab[] = {
	"next",		next,		NDMLIST,	0,	MMNDEL,
	"alias",	group,		M|RAWLIST,	0,	1000,
	"print",	type,		MSGLIST,	0,	MMNDEL,
	"type",		type,		MSGLIST,	0,	MMNDEL,
	"Type",		Type,		MSGLIST,	0,	MMNDEL,
	"Print",	Type,		MSGLIST,	0,	MMNDEL,
	"bprint",	btype,		MSGLIST,	0,	MMNDEL,
	"Bprint",	Btype,		MSGLIST,	0,	MMNDEL,
	"visual",	visual,		I|MSGLIST,	0,	MMNORM,
	"bvisual",	bvisual,	I|MSGLIST,	0,	MMNORM,
	"top",		top,		MSGLIST,	0,	MMNDEL,
	"btop",		btop,		MSGLIST,	0,	MMNDEL,
	"touch",	stouch,		W|MSGLIST,	0,	MMNDEL,
	"preserve",	preserve,	I|W|MSGLIST,	0,	MMNDEL,
	"delete",	delete,		W|P|MSGLIST,	0,	MMNDEL,
	"dp",		deltype,	W|MSGLIST,	0,	MMNDEL,
	"dt",		deltype,	W|MSGLIST,	0,	MMNDEL,
	"undelete",	undelete,	P|MSGLIST,	MDELETED,MMNDEL,
	"unset",	unset,		M|RAWLIST,	1,	1000,
	"mail",		sendm,		R|M|I|STRLIST,	0,	0,
	"mailall",	sendmail,	R|M|I|STRLIST,	0,	0,
	"Mail",		Sendm,		R|M|I|STRLIST,	0,	0,
	"Mailrecord",	Sendmail,	R|M|I|STRLIST,	0,	0,
	"mbox",		mboxit,		W|MSGLIST,	0,	0,
	"copy",		copycmd,	M|STRLIST,	0,	0,
	"Copy",		Copy,		M|MSGLIST,	0,	0,
	"chdir",	schdir,		M|STRLIST,	0,	0,
	"cd",		schdir,		M|STRLIST,	0,	0,
	"save",		save,		STRLIST,	0,	0,
	"Save",		Save,		MSGLIST,	0,	0,
	"source",	source,		M|STRLIST,	0,	0,
	"set",		set,		M|RAWLIST,	0,	1000,
	"shell",	shell,		I|STRLIST,	0,	0,
	"!",		shell,		I|STRLIST,	0,	0,
	"version",	pversion,	M|NOLIST,	0,	0,
	"group",	group,		M|RAWLIST,	0,	1000,
	"write",	swrite,		STRLIST,	0,	0,
	"from",		from,		MSGLIST,	0,	MMNORM,
	"followup",	followup,	R|I|MSGLIST,	0,	MMNDEL,
	"followupall",	followupall,	R|I|MSGLIST,	0,	MMNDEL,
	"Followup",	Followup,	R|I|MSGLIST,	0,	MMNDEL,
	"Followupall",	Followupall,	R|I|MSGLIST,	0,	MMNDEL,
	"file",		file,		TX|M|RAWLIST,	0,	1,
	"folder",	file,		TX|M|RAWLIST,	0,	1,
	"folders",	folders,	TX|M|RAWLIST,	0,	1,
	"forward",	forwardmail,	STRLIST,	0,	0,
	"Forward",	Forwardmail,	STRLIST,	0,	0,
	"?",		help,		M|NOLIST,	0,	0,
	"z",		scroll,		M|STRLIST,	0,	0,
	"headers",	headers,	MSGLIST,	0,	MMNDEL,
	"Headers",	Headers,	MSGLIST,	0,	MMNDEL,
	"help",		help,		M|NOLIST,	0,	0,
	"=",		pdot,		NOLIST,		0,	0,
	"Reply",	Respond,	R|I|MSGLIST,	0,	MMNDEL,
	"Respond",	Respond,	R|I|MSGLIST,	0,	MMNDEL,
	"reply",	respond,	R|I|MSGLIST,	0,	MMNDEL,
	"respond",	respond,	R|I|MSGLIST,	0,	MMNDEL,
	"replysender",	replysender,	R|I|MSGLIST,	0,	MMNDEL,
	"replyall",	respondall,	R|I|MSGLIST,	0,	MMNDEL,
	"respondall",	respondall,	R|I|MSGLIST,	0,	MMNDEL,
	"edit",		editor,		I|MSGLIST,	0,	MMNORM,
	"bedit",	beditor,	I|MSGLIST,	0,	MMNORM,
	"echo",		echo,		M|STRLIST,	0,	1000,
	"quit",		edstop,		NOLIST, 	0,	0,
	"list",		pcmdlist,	M|NOLIST,	0,	0,
	"xit",		rexit,		M|NOLIST,	0,	0,
	"exit",		rexit,		M|NOLIST,	0,	0,
	"size",		messize,	MSGLIST,	0,	MMNDEL,
	"hold",		preserve,	I|W|MSGLIST,	0,	MMNDEL,
	"if",		ifcmd,		F|M|RAWLIST,	1,	1,
	"else",		elsecmd,	F|M|RAWLIST,	0,	0,
	"endif",	endifcmd,	F|M|RAWLIST,	0,	0,
	"alternates",	alternates,	M|RAWLIST,	0,	1000,
	"ignore",	igfield,	M|RAWLIST,	0,	1000,
	"discard",	igfield,	M|RAWLIST,	0,	1000,
	"retain",	retainfield,	M|RAWLIST,	0,	1000,
	"unalias",	ungroup,	M|RAWLIST,	0,	1000,
	"unignore",	unigfield,	M|RAWLIST,	0,	1000,
	"undiscard",	unigfield,	M|RAWLIST,	0,	1000,
	"unretain",	unretainfield,	M|RAWLIST,	0,	1000,
	"#",		null,		M|NOLIST,	0,	0,
	"pipe",		dopipe,		STRLIST,	0,	0,
	"|",		dopipe,		STRLIST,	0,	0,
	"Pipe",		doPipe,		STRLIST,	0,	0,
	"newmail",	newmail,	NOLIST,		0,	0,
	"inc",		newmail,	NOLIST,		0,	0,
	"showheaders",	showheaders,	STRLIST,	0,	0,
	"New",		New,		W|MSGLIST,	0,	MMNDEL,
	"unread",	New,		W|MSGLIST,	0,	MMNDEL,
	"Unread",	New,		W|MSGLIST,	0,	MMNDEL,
	0,		0,		0,		0,	0
};
