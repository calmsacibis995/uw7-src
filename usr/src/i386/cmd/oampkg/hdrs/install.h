#ident	"@(#)install.h	15.1"
#ident  "$Header$"

#define MAILCMD	"/usr/bin/mail"
#define DATSTRM	"datastream"
#define SHELL	"/sbin/sh"
#define PKGINFO	"pkginfo"
#define PKGMAP	"pkgmap"
#define	SETINFO	"setinfo"
#define isdot(x)	((x[0]=='.')&&(!x[1]||(x[1]=='/')))
#define isdotdot(x)	((x[0]=='.')&&(x[1]=='.')&&(!x[2]||(x[2]=='/')))
#define INPBUF	128
#define DUPENV(x,y)	((x=getenv(y)) == NULL ? NULL : strdup(x))
#define PUTPARAM(x,y)	( ((y) == NULL) ? (void ) NULL : putparam(x,y))

struct mergstat {
	char	*setuid;
	char	*setgid;
	char	contchg;
	char	attrchg;
	char	shared;
};

struct admin {
	char	*mail;
	char	*instance;
	char	*partial;
	char	*runlevel;
	char	*idepend;
	char	*rdepend;
	char	*space;
	char	*setuid;
	char	*conflict;
	char	*action;
	char	*basedir;
	char	*list_files;
};

#define ADM(x, y)	((adm.x != NULL) && (y != NULL) && !strcmp(adm.x, y))

/*
 * Value returned by Set Installation Package request script
 * when no set member packages are selected for installation.
 */
#define	NOSET	77

#define MSG_REBOOT_I \
gettxt(":813", "\\n*** IMPORTANT NOTICE ***\\n\\tIf installation of all desired packages is complete,\\n\\tthe machine should be rebooted in order to\\n\\tensure sane operation. Execute the shutdown\\n\\tcommand with the appropriate options to reboot.")

#define MSG_REBOOT_R \
gettxt(":814", "\\n*** IMPORTANT NOTICE ***\\n\\tIf removal of all desired packages is complete,\\n\\tthe machine should be rebooted in order to\\n\\tensure sane operation. Execute the shutdown\\n\\tcommand with the appropriate options to reboot.")
