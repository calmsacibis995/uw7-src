/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)cmdtab.c	1.2"
#ident	"$Header$"

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#)cmdtab.c	5.9 (Berkeley) 3/21/89
 */


#include <stdio.h>
#include <arpa/ftp.h>
#include <pfmt.h>
#include "ftp_var.h"

/*
 * User FTP -- Command Tables.
 */

int	setascii(), setbell(), setbinary(), setdebug(), setform();
int	setglob(), sethash(), setmode(), setpeer(), setport();
int	setprompt(), setstruct();
int	settenex(), settrace(), settype(), setverbose();
int	disconnect(), restart(), reget(), syst();
int	cd(), lcd(), delete(), mdelete(), user();
int	ls(), mls(), get(), mget(), help(), append(), put(), mput();
int	quit(), renamefile(), status();
int	quote(), rmthelp(), shell(), site();
int	pwd(), makedir(), removedir(), setcr();
int	account(), doproxy(), reset(), setcase(), setntrans(), setnmap();
int	setsunique(), setrunique(), cdup(), macdef(), domacro();
int	sizecmd(), modtime(), newer(), rmtstatus();
int	do_chmod(), do_umask(), idle();
#ifndef NO_PASSIVE_MODE
int	setpassive();
#endif

struct cmd cmdtab[] = {

	{ "!",		NULL,	0,	0,	0,	shell },
	{ "$",		NULL,	1,	0,	0,	domacro },
	{ "account",	NULL,	0,	1,	1,	account},
	{ "append",	NULL,	1,	1,	1,	put },
	{ "ascii",	NULL,	0,	1,	1,	setascii },
	{ "bell",	NULL,	0,	0,	0,	setbell },
	{ "binary",	NULL,	0,	1,	1,	setbinary },
	{ "bye",	NULL,	0,	0,	0,	quit },
	{ "case",	NULL,	0,	0,	1,	setcase },
	{ "cd",		NULL,	0,	1,	1,	cd },
	{ "cdup",	NULL,	0,	1,	1,	cdup },
	{ "chmod",	NULL,	0,	1,	1,	do_chmod },
	{ "close",	NULL,	0,	1,	1,	disconnect },
	{ "cr",		NULL,	0,	0,	0,	setcr },
	{ "delete",	NULL,	0,	1,	1,	delete },
	{ "debug",	NULL,	0,	0,	0,	setdebug },
	{ "dir",	NULL,	1,	1,	1,	ls },
	{ "disconnect",	NULL,	0,	1,	1,	disconnect },
	{ "form",	NULL,	0,	1,	1,	setform },
	{ "get",	NULL,	1,	1,	1,	get },
	{ "glob",	NULL,	0,	0,	0,	setglob },
	{ "hash",	NULL,	0,	0,	0,	sethash },
	{ "help",	NULL,	0,	0,	1,	help },
	{ "idle",	NULL,	0,	1,	1,	idle },
	{ "image",	NULL,	0,	1,	1,	setbinary },
	{ "lcd",	NULL,	0,	0,	0,	lcd },
	{ "ls",		NULL,	1,	1,	1,	ls },
	{ "macdef",	NULL,	0,	0,	0,	macdef },
	{ "mdelete",	NULL,	1,	1,	1,	mdelete },
	{ "mdir",	NULL,	1,	1,	1,	mls },
	{ "mget",	NULL,	1,	1,	1,	mget },
	{ "mkdir",	NULL,	0,	1,	1,	makedir },
	{ "mls",	NULL,	1,	1,	1,	mls },
	{ "mode",	NULL,	0,	1,	1,	setmode },
	{ "modtime",	NULL,	0,	1,	1,	modtime },
	{ "mput",	NULL,	1,	1,	1,	mput },
	{ "newer",	NULL,	1,	1,	1,	newer },
	{ "nmap",	NULL,	0,	0,	1,	setnmap },
	{ "nlist",	NULL,	1,	1,	1,	ls },
	{ "ntrans",	NULL,	0,	0,	1,	setntrans },
	{ "open",	NULL,	0,	0,	1,	setpeer },
#ifndef NO_PASSIVE_MODE
	{ "passive",	NULL,	0,	0,	0,	setpassive },
#endif
	{ "prompt",	NULL,	0,	0,	0,	setprompt },
	{ "proxy",	NULL,	0,	0,	1,	doproxy },
	{ "sendport",	NULL,	0,	0,	0,	setport },
	{ "put",	NULL,	1,	1,	1,	put },
	{ "pwd",	NULL,	0,	1,	1,	pwd },
	{ "quit",	NULL,	0,	0,	0,	quit },
	{ "quote",	NULL,	1,	1,	1,	quote },
	{ "recv",	NULL,	1,	1,	1,	get },
	{ "reget",	NULL,	1,	1,	1,	reget },
	{ "rstatus",	NULL,	0,	1,	1,	rmtstatus },
	{ "rhelp",	NULL,	0,	1,	1,	rmthelp },
	{ "rename",	NULL,	0,	1,	1,	renamefile },
	{ "reset",	NULL,	0,	1,	1,	reset },
	{ "restart",	NULL,	1,	1,	1,	restart },
	{ "rmdir",	NULL,	0,	1,	1,	removedir },
	{ "runique",	NULL,	0,	0,	1,	setrunique },
	{ "send",	NULL,	1,	1,	1,	put },
	{ "site",	NULL,	0,	1,	1,	site },
	{ "size",	NULL,	1,	1,	1,	sizecmd },
	{ "status",	NULL,	0,	0,	1,	status },
	{ "struct",	NULL,	0,	1,	1,	setstruct },
	{ "system",	NULL,	0,	1,	1,	syst },
	{ "sunique",	NULL,	0,	0,	1,	setsunique },
	{ "tenex",	NULL,	0,	1,	1,	settenex },
	{ "trace",	NULL,	0,	0,	0,	settrace },
	{ "type",	NULL,	0,	1,	1,	settype },
	{ "user",	NULL,	0,	1,	1,	user },
	{ "umask",	NULL,	0,	1,	1,	do_umask },
	{ "verbose",	NULL,	0,	0,	0,	setverbose },
	{ "?",		NULL,	0,	0,	1,	help },
	{ 0 },
};

int	NCMDS = (sizeof (cmdtab) / sizeof (cmdtab[0])) - 1;

char	*strerror();
char	*getcpytxt();
char	*gettxt();
char	*malloc();

void init_cmdtab()

{

	int i;

	i = 0;
	cmdtab[i++].c_help = getcpytxt(":222",
		"escape to the shell");
	cmdtab[i++].c_help = getcpytxt(":184",
		"execute macro");
	cmdtab[i++].c_help = getcpytxt(":169",
		"send account command to remote server");
	cmdtab[i++].c_help = getcpytxt(":170",
		"append to a file");
	cmdtab[i++].c_help = getcpytxt(":171",
		"set ascii transfer type");
	cmdtab[i++].c_help = getcpytxt(":172",
		"beep when command completed");
	cmdtab[i++].c_help = getcpytxt(":173",
		"set binary transfer type");
	cmdtab[i++].c_help = getcpytxt(":209",
		"terminate ftp session and exit");
	cmdtab[i++].c_help = getcpytxt(":174",
		"toggle mget upper/lower case id mapping");
	cmdtab[i++].c_help = getcpytxt(":175",
		"change remote working directory");
	cmdtab[i++].c_help = getcpytxt(":176",
		"change remote working directory to parent directory");
	cmdtab[i++].c_help = getcpytxt(":177",
		"change file permissions of remote file");
	cmdtab[i++].c_help = getcpytxt(":183",
		"terminate ftp session");
	cmdtab[i++].c_help = getcpytxt(":179",
		"toggle carriage return stripping on ascii gets");
	cmdtab[i++].c_help = getcpytxt(":180",
		"delete remote file");
	cmdtab[i++].c_help = getcpytxt(":181",
		"toggle/set debugging mode");
	cmdtab[i++].c_help = getcpytxt(":182",
		"list contents of remote directory");
	cmdtab[i++].c_help = getcpytxt(":183",
		"terminate ftp session");
	cmdtab[i++].c_help = getcpytxt(":185",
		"set file transfer format");
	cmdtab[i++].c_help = getcpytxt(":211",
		"receive file");
	cmdtab[i++].c_help = getcpytxt(":186",
		"toggle metacharacter expansion of local file names");
	cmdtab[i++].c_help = getcpytxt(":187",
		"toggle printing `#' for each buffer transferred");
	cmdtab[i++].c_help = getcpytxt(":188",
		"print local help information");
	cmdtab[i++].c_help = getcpytxt(":189",
		"get (set) idle timer on remote side");
	cmdtab[i++].c_help = getcpytxt(":173",
		"set binary transfer type");
	cmdtab[i++].c_help = getcpytxt(":190",
		"change local working directory");
	cmdtab[i++].c_help = getcpytxt(":191",
		"list contents of remote directory");
	cmdtab[i++].c_help = getcpytxt(":192",
		"define a macro");
	cmdtab[i++].c_help = getcpytxt(":193",
		"delete multiple files");
	cmdtab[i++].c_help = getcpytxt(":194",
		"list contents of multiple remote directories");
	cmdtab[i++].c_help = getcpytxt(":195",
		"get multiple files");
	cmdtab[i++].c_help = getcpytxt(":196",
		"make directory on the remote machine");
	cmdtab[i++].c_help = getcpytxt(":197",
		"list contents of multiple remote directories");
	cmdtab[i++].c_help = getcpytxt(":199",
		"set file transfer mode");
	cmdtab[i++].c_help = getcpytxt(":198",
		"show last modification time of remote file");
	cmdtab[i++].c_help = getcpytxt(":200",
		"send multiple files");
	cmdtab[i++].c_help = getcpytxt(":201",
		"get file if remote file is newer than local file ");
	cmdtab[i++].c_help = getcpytxt(":203",
        	"set templates for default file name mapping");
	cmdtab[i++].c_help = getcpytxt(":202",
        	"nlist contents of remote directory");
	cmdtab[i++].c_help = getcpytxt(":204",
        	"set translation table for default file name mapping");
	cmdtab[i++].c_help = getcpytxt(":178",
        	"connect to remote ftp");
#ifndef NO_PASSIVE_MODE
	cmdtab[i++].c_help = getcpytxt(":399",
        	"toggle passive transfer mode");
#endif
	cmdtab[i++].c_help = getcpytxt(":206",
        	"force interactive prompting on multiple commands");
	cmdtab[i++].c_help = getcpytxt(":207",
        	"issue command on alternate connection");
	cmdtab[i++].c_help = getcpytxt(":205",
        	"toggle use of PORT cmd for each data connection");
	cmdtab[i++].c_help = getcpytxt(":220",
        	"send one file");
	cmdtab[i++].c_help = getcpytxt(":208",
        	"print working directory on remote machine");
	cmdtab[i++].c_help = getcpytxt(":209",
        	"terminate ftp session and exit");
	cmdtab[i++].c_help = getcpytxt(":210",
        	"send arbitrary ftp command");
	cmdtab[i++].c_help = getcpytxt(":211",
        	"receive file");
	cmdtab[i++].c_help = getcpytxt(":212",
        	"get file restarting at end of local file");
	cmdtab[i++].c_help = getcpytxt(":217",
        	"show status of remote machine");
	cmdtab[i++].c_help = getcpytxt(":213",
        	"get help from remote server");
	cmdtab[i++].c_help = getcpytxt(":214",
		"rename file");
	cmdtab[i++].c_help = getcpytxt(":219",
        	"clear queued command replies");
	cmdtab[i++].c_help = getcpytxt(":215",
        	"restart file transfer at bytecount");
	cmdtab[i++].c_help = getcpytxt(":216",
        	"remove directory on the remote machine");
	cmdtab[i++].c_help = getcpytxt(":218",
        	"toggle store unique for local files");
	cmdtab[i++].c_help = getcpytxt(":220",
        	"send one file");
	cmdtab[i++].c_help = getcpytxt(":221",
        	"send site specific command to remote server\n\t\tTry \"rhelp site\" or \"site help\" for more information");
	cmdtab[i++].c_help = getcpytxt(":223",
        	"show size of remote file");
	cmdtab[i++].c_help = getcpytxt(":224",
        	"show current status");
	cmdtab[i++].c_help = getcpytxt(":225",
        	"set file transfer structure");
	cmdtab[i++].c_help = getcpytxt(":227",
        	"show remote system type");
	cmdtab[i++].c_help = getcpytxt(":226",
        	"toggle store unique on remote machine");
	cmdtab[i++].c_help = getcpytxt(":228",
        	"set tenex file transfer type");
	cmdtab[i++].c_help = getcpytxt(":229",
        	"toggle packet tracing");
	cmdtab[i++].c_help = getcpytxt(":230",
        	"set file transfer type");
	cmdtab[i++].c_help = getcpytxt(":232",
        	"send new user information");
	cmdtab[i++].c_help = getcpytxt(":231",
		"get (set) umask on remote side");
	cmdtab[i++].c_help = getcpytxt(":233",
        	"toggle verbose mode");
	cmdtab[i++].c_help = getcpytxt(":188",
		"print local help information");

}

char *getcpytxt(s1, s2)
char *s1, *s2;

{

	char *ptr, *ptr2;

	ptr = gettxt(s1, s2);
	if (ptr == NULL) {
		pfmt(stderr, MM_ERROR, ":163:gettxt failed");
		exit(1);
	}
	if ((ptr2 = malloc(strlen(ptr)+1)) == NULL) {
		pfmt(stderr, MM_ERROR, ":234: getcpytxt: %s\n",
			strerror(errno));
		exit(1);
	}
	strcpy(ptr2, ptr);

	return ptr2;

}

