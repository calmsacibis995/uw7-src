/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)sharetab.h	1.2"
#ident	"$Header$"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *	PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *     Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 *
 *  (c) 1986,1987,1988.1989  Sun Microsystems, Inc
 *  (c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *            All rights reserved.
 *
 */

struct share {
	char *sh_path;
	char *sh_res;
	char *sh_fstype;
	char *sh_opts;
	char *sh_descr;
};

#define SHARETAB  "/etc/dfs/sharetab"

/* generic options */
#define SHOPT_RO	"ro"
#define SHOPT_RW	"rw"

/* options for nfs */
#define SHOPT_ROOT	"root"
#define SHOPT_ANON	"anon"
#define SHOPT_SECURE	"secure"
#define SHOPT_WINDOW	"window"
#define SHOPT_WRASYNC	"asyncwrites"

int		getshare();
int		putshare();
int		remshare();
char *		getshareopt();
