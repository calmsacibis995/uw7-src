#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/sharetab.h	1.2"
#endif

/* this file is also known as :
#ident	"@(#)nfs.cmds:nfs/share/sharetab.h	1.2.4.1"
#ident "$Header$"
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

int		getshare();
int		putshare();
int		remshare();
char *		getshareopt();
