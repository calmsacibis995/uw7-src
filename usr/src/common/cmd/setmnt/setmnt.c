#ident	"@(#)setmnt:setmnt.c	4.13.1.14"

/***************************************************************************
 * Command: setmnt
 * Inheritable Privileges: P_MACWRITE,P_DACWRITE,P_MACREAD,P_DACREAD,
 *			   P_SETFLEVEL,P_COMPAT,P_OWNER
 *       Fixed Privileges: None
 * Notes: /sbin/setmnt is generally used only to set the /etc/mnttab
 *	  file when the system is first boot up.
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<errno.h>
#include	<sys/mnttab.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/statvfs.h>
#include	<stdlib.h>
#include	<sys/param.h>
#include	<mac.h>

#define	LINESIZ	BUFSIZ
#define	OPTSIZ	64
#define	MNTTAB_OWN	0	/* root       */
#define	MNTTAB_GRP	3	/* sys        */
#define	MNTTAB_MODE	0444	/* -r--r--r-- */

extern char	*fgets();
extern char	*strtok();
extern char	*strrchr();
extern time_t	time();

static char	*opts(ulong);
static void	error(char *);
static void	parseline(char *);

static char	*myname;
static char	line[LINESIZ];
static char	sepstr[] = " \t\n";
static char	mnttab[] = MNTTAB;
static char	parent_dir[] = "/etc";
static char	mnttab_tmp[] = "/etc/mnttab_tmp";
char		resolved[MAXPATHLEN];
static time_t	date;
static FILE	*fp;

/*
 * Procedure:     main
 *
 * Restrictions:
                 fprintf: None
                 fopen: None
                 fgets: None
                 statvfs(2): None
                 fclose: None
                 unlink(2): None
                 rename(2): None
                 lvlfile(2): None
                 chmod(2): None
                 chown(2): None
 *
 * Notes: /sbin/setmnt creates a temporary file /etc/mnttab_tmp and
 *	  takes from stdin the input of the mnttab entry.  It writes to
 *	  this temp file after stat'ing the mount-points.  Then it
 *	  replace the original /etc/mnttab file (if there is one) with
 *	  the temp file.  It sets the default attributes of the file.
 */
void	
main(argc, argv)
	int	argc;
	char	**argv;
{
	int	c, argx;
	char	*lp = line;
	level_t level_lid = 0;	/* MAC level identifier */
	
	myname = strrchr(argv[0], '/');
	if (myname)
		myname++;
	else
		myname = argv[0];

	if ( (fp = fopen(mnttab_tmp, "w")) == NULL )
		error("cannot create temp mnttab file");
	
	time(&date);


	/* write immediately, so errors will be noticed */
	setbuf(fp, NULL);

	/*
	 * To save a process, this command has been updated to take
	 * pairs of "fs mtpt" args.  It will still read args from
	 * stdin.  The new behavior is undocumented.
	 */

	for (argx = 1; argx < argc; argx++)
		parseline(argv[argx]);

	while ((lp = fgets(line, LINESIZ, stdin)) != NULL)
		parseline(lp);

	fclose(fp);

	(void) unlink(mnttab);
	if ( rename(mnttab_tmp, mnttab) != 0 ) 
		error("cannot rename temp file");

	/* 
	 * set correct attributes: level = SYS_PUBLIC
	 * owner = root, group = sys, mode = 0444
	 */

	/* find lid of SYS_PUBLIC by finding lid of mnttab's parent dir. */
	if ( lvlfile(parent_dir, MAC_GET, &level_lid) < 0) {
		error("cannot get level identifier");
	}

	/* If level_lid is then, set level of mnttab */
	if ( level_lid ) {
		if ( lvlfile(mnttab, MAC_SET, &level_lid) != 0) {
				error("cannot set level");
		}
	}
	
	if ( chmod(mnttab, (mode_t) MNTTAB_MODE) != 0 || 
	     chown(mnttab, (uid_t) MNTTAB_OWN, (gid_t) MNTTAB_GRP) != 0 )
		error("cannot set attributes");
	
	exit(0);
}

static char *
opts(ulong flag)
{
	static char	mntopts[OPTSIZ];

	sprintf(mntopts, "%s,%s",
		(flag & ST_RDONLY) ? "ro" : "rw",
		(flag & ST_NOSUID) ? "nosuid" : "suid");
	return	mntopts;
}

static void 
error(char *string)
{
	fprintf(stderr, "%s: %s\n", myname, string);
	exit(1);
}

static void
parseline(char *lp)
{
	struct mnttab	mm;
	struct statvfs	sbuf;

	if ((mm.mnt_special = strtok(lp, sepstr)) != NULL &&
	    (mm.mnt_mountp = strtok(NULL, sepstr)) != NULL &&
	     statvfs(mm.mnt_mountp, &sbuf) == 0) {
		if ( realpath(mm.mnt_mountp,resolved) )
			mm.mnt_mountp=resolved;
		if (fprintf(fp, "%s\t%s\t%s\t%s\t%d\n",
			mm.mnt_special,
			mm.mnt_mountp,
			sbuf.f_basetype[0] ? sbuf.f_basetype : "-",
			opts(sbuf.f_flag),
			date) < 0) {
		    unlink(mnttab_tmp);
		    error("cannot update mnttab");
		}
	}
}
