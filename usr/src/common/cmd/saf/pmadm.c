/*		copyright	"%c%" 	*/


# ident	"@(#)pmadm.c	1.6"
#ident  "$Header$"

/***************************************************************************
 * Command: pmadm
 *
 * Inheritable Privileges: P_SETFLEVEL,P_DACREAD,P_DACWRITE,P_OWNER
 *       Fixed Privileges: None
 *
 * Notes: In order to use this command the user has to be at SYS_PRIVATE
 *	  level.
 ***************************************************************************/

# include <stdio.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <sac.h>
# include <pwd.h>
# include <priv.h>
# include <mac.h>
# include <sys/secsys.h>
#include <locale.h>
#include <pfmt.h>
#include "msg.h"
#include "msgs.h"

# include "extern.h"
# include "misc.h"
# include "structs.h"

# define ADD		0x1	/* -a or other required options seen */
# define REMOVE		0x2	/* -r seen */
# define ENABLE		0x4	/* -e seen */
# define DISABLE	0x8	/* -d seen */
# define PLIST		0x10	/* -l seen */
# define LIST		0x20	/* -L seen */
# define CONFIG		0x40	/* -g seen */
# define CHANGE		0x80	/* -c seen */

# define U_FLAG		0x1	/* -fu seen */
# define X_FLAG		0x2	/* -fx seen */

/*
 * functions
 */

struct	passwd	*getpwnam();
struct	taglist	*find_type();
char	*pflags();
char	*pspec();
void	usage();
void	parseline();
void	add_svc();
void	rem_svc();
void	ed_svc();
void	change();
void	list_svcs();
void	doconf();


/*
 * format of a _pmtab entry - used to hold parsed info
 */

struct	pmtab {
	char	*p_tag;		/* service tag */
	long	p_flags;	/* flags */
	char	*p_id;		/* logname to start service as */
	char	*p_res1;	/* reserved field */
	char	*p_res2;	/* reserved field */
	char	*p_scheme;	/* authentication scheme */
	char	*p_pmspec;	/* port monitor specific info */
};

/*
 * format of a tag list, which is a list of port monitor tags of
 * a designated type
 */

struct	taglist {
	struct	taglist	*t_next;	/* next in list */
	char	t_tag[PMTAGSIZE + 1];	/* PM tag */
	char	t_type[PMTYPESIZE + 1];	/* PM type */
};

/*
 * common error messages
 */


int	Saferrno;	/* internal `errno' for exit */
struct	pmtab	Pmtab;	/* place to hold parsed info for later reference */


/*
 * Procedure:     main
 *
 * Restrictions:
 *               getpwnam: None
 *               check_version: None
 * Notes:  scan args for pmadm and call appropriate handling code 
 */

char    *msg_label = "UX:pmadm";

main(argc, argv)
int argc;
char *argv[];
{
	int c;			/* option letter */
	int ret;		/* return code from check_version */
	int flag = 0;		/* flag to record requested operations */
	int errflg = 0;		/* error indicator */
	int badcnt = 0;		/* count of bad args to -f */
	int version = -1;	/* argument to -v */
	int sawaflag = 0;	/* true if actually saw -a */
	int conflag = 0;	/* true if output should be in condensed form */
	long flags = 0;		/* arguments to -f */
	char *pmtag = NULL;	/* argument to -p */
	char *type = NULL;	/* argument to -t */
	char *script = NULL;	/* argument to -z */
	char *comment = NULL;	/* argument to -y */
	char *id = NULL;	/* argument to -i */
	char *svctag = NULL;	/* argument to -s */
	char *pmspec = NULL;	/* argument to -m */
	char *scheme = NULL;	/* argument to -S */
	char badargs[SIZE];	/* place to hold bad args to -f */
	char buf[SIZE];		/* scratch buffer */
	char *p;	        /* scratch pointer */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxsaf");
	(void)setlabel(msg_label);

	if (argc == 1)
		usage(argv[0]);
	while ((c = getopt(argc, argv, "acdef:gi:Llm:p:rS:s:t:v:y:z:")) != -1) {
		switch (c) {
		case 'a':
			flag |= ADD;
			sawaflag = 1;
			break;
		case 'c':
			flag |= CHANGE;
			break;
		case 'd':
			flag |= DISABLE;
			break;
		case 'e':
			flag |= ENABLE;
			break;
		case 'f':
			flag |= ADD;
			while (*optarg) {
				switch (*optarg++) {
				case 'u':
					flags |= U_FLAG;
					break;
				case 'x':
					flags |= X_FLAG;
					break;
				default:
					badargs[badcnt++] = *(optarg - 1);
					break;
				}
			}
			/* null terminate just in case anything is there */
			badargs[badcnt] = '\0';
			break;
		case 'g':
			flag |= CONFIG;
			break;
		case 'i':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit(1);
			}
			id = optarg;
			if (strcmp(id, "") && !getpwnam(id)) {
				Saferrno = E_BADARGS;
				pfmt (stderr,MM_ERROR,MSG79);
				quit();
			}
			break;
		case 'L':
			flag |= LIST;
			break;
		case 'l':
			flag |= PLIST;
			break;
		case 'm':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit(1);
			}
			if (*optarg == '\0') {
				/* this will generate a usage message below */
				errflg++;
				break;
			}
			flag |= ADD;
			pmspec = optarg;
			break;
		case 'p':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit(1);
			}
			pmtag = optarg;
			if (strlen(pmtag) > PMTAGSIZE) {
				pmtag[PMTAGSIZE] = '\0';
				pfmt (stderr, MM_ERROR, MSG57, pmtag);
				quit();
			}
			for (p = pmtag; *p; p++) {
				if (!isalnum(*p)) {
					Saferrno = E_BADARGS;
					pfmt (stderr,MM_ERROR,MSG80);
					quit();
				}
			}
			break;
		case 'r':
			flag |= REMOVE;
			break;
		case 'S':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				quit();
			}
			scheme = optarg;
			break;
		case 's':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				quit();
			}
			svctag = optarg;
			if (strlen(svctag) > SVCTAGSIZE) {
				svctag[SVCTAGSIZE] = '\0';
				pfmt (stderr, MM_ERROR, MSG58, svctag);
				quit();
			}
			if (*svctag == '_') {
				Saferrno = E_BADARGS;
				pfmt (stderr,MM_ERROR,MSG81);
				quit();
			}
			for (p = svctag; *p; p++) {
				if (!isalnum(*p) && (*p != '_')) {
					Saferrno = E_BADARGS;
					pfmt (stderr,MM_ERROR,MSG82);
					quit();
				}
			}
			break;
		case 't':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				quit();
			}
			type = optarg;
			if (strlen(type) > PMTYPESIZE) {
				type[PMTYPESIZE] = '\0';
				pfmt (stderr, MM_ERROR, MSG59, type);
				quit();
			}
			for (p = type; *p; p++) {
				if (!isalnum(*p)) {
					Saferrno = E_BADARGS;
					pfmt (stderr,MM_ERROR,MSG83);
					quit();
				}
			}
			break;
		case 'v':
			flag |= ADD;
			version = (int) strtol (optarg, &p, 10);
			if (p == optarg) {
			        Saferrno = E_BADARGS;
					pfmt (stderr,MM_ERROR,MSG84);
					quit();
			} else if (version < 0) {
				Saferrno = E_BADARGS;
				pfmt (stderr,MM_ERROR,MSG85);
				quit();
			}
			break;
		case 'y':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				quit();
			}
			flag |= ADD;
			comment = optarg;
			break;
		case 'z':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				quit();
			}
			script = optarg;
			break;
		case '?':
			errflg++;
		}
	}
	if (errflg || (optind < argc))
		usage(argv[0]);

	if (badcnt) {
		Saferrno = E_BADARGS;
		/* bad flags were given to -f */
		pfmt (stderr,MM_ERROR,MSG86,badargs);
		quit();
	}

/*
 * don't do anything if _sactab isn't the version we understand
 */

	ret = check_version(VERSION, SACTAB);
	if (ret == 1) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG87);
		quit();
	}
	else if (ret == 2) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG88,SACTAB);
		quit();
	}
	else if (ret == 3) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG89,SACTAB);
		quit();
	}


	switch (flag) {
	case ADD:
		if (!sawaflag || (pmtag && type) || (!pmtag && !type) || !svctag || !pmspec || (version < 0))
			usage(argv[0]);
		add_svc(pmtag, type, svctag, id, scheme, pmspec, flags, version, comment, script);
		break;
	case REMOVE:
		if (!pmtag || !svctag || type || script || id || scheme)
			usage(argv[0]);
		rem_svc(pmtag, svctag);
		break;
	case CHANGE:
		if (!pmtag || !svctag || (!id && !scheme) || type || script)
			usage(argv[0]);
		change(pmtag, svctag, id, scheme);
		break;
	case ENABLE:
		if (!pmtag || !svctag || type || script || id || scheme)
			usage(argv[0]);
		ed_svc(pmtag, svctag, ENABLE);
		break;
	case DISABLE:
		if (!pmtag || !svctag || type || script || id || scheme)
			usage(argv[0]);
		ed_svc(pmtag, svctag, DISABLE);
		break;
	case LIST:
		conflag = 1;
		/* fall through */
	case PLIST:
		if ((pmtag && type) || script || id || scheme)
			usage(argv[0]);
		list_svcs(pmtag, type, svctag, conflag);
		break;
	case CONFIG:
		if ((pmtag && type) || (!pmtag && !type) || !svctag || (type && !script) || id || scheme)
			usage(argv[0]);
		doconf(script, pmtag, type, svctag);
		break;
	default:
		/* we only get here if more than one flag bit was set */
		usage(argv[0]);
		/* NOTREACHED */
	}
	quit();
	/* NOTREACHED */
}


/*
 * Procedure: usage - print out a usage message
 *
 * Args:	cmdname - the name command was invoked with
 */

void
usage(cmdname)
char *cmdname;
{
	pfmt (stderr, MM_ERROR, MSG69, cmdname);
	pfmt (stderr, MM_ERROR, MSG70);
	pfmt (stderr, MM_ACTION, MSG71, cmdname);
	pfmt (stderr, MM_ACTION, MSG72, cmdname);
	pfmt (stderr, MM_ACTION, MSG73, cmdname);
	pfmt (stderr, MM_ACTION, MSG74, cmdname);
	pfmt (stderr, MM_ACTION, MSG75, cmdname);
	pfmt (stderr, MM_ACTION, MSG76, cmdname);
	pfmt (stderr, MM_ACTION, MSG77, cmdname);
	pfmt (stderr, MM_ACTION, MSG78, cmdname);
	Saferrno = E_BADARGS;
	quit();
}


/*
 * Procedure: add_svc - add a service entry
 *
 * Args:	tag - port monitor's tag (may be null)
 *		type - port monitor's type (may be null)
 *		svctag - service's tag
 *		id - identity under which service should run
 *		scheme - authentication scheme
 *		pmspec - uninterpreted port monitor-specific info
 *		flags - service flags
 *		version - version number of port monitor's _pmtab
 *		comment - comment describing service
 *		script - service's configuration script
 *
 * Restrictions:
 *               check_version: none
 *               fopen: none
 *               lvlin: none
 *               lvlfile(2): none
 *               chown(2): none
 */

void
add_svc(tag, type, svctag, id, scheme, pmspec, flags, version, comment, script)
char *tag;
char *type;
char *svctag;
char *id;
char *scheme;
char *pmspec;
long flags;
int version;
char *comment;
char *script;
{
	FILE *fp;			/* scratch file pointer */
	struct taglist tl;		/* 'list' for degenerate case (1 PM) */
	register struct taglist *tp;	/* working pointer */
	int ret;			/* scratch return code */
	char buf[SIZE];			/* scratch buffer */
	char fname[SIZE];		/* scratch buffer for building names */
	int added;			/* count number added */
	level_t level_lid;		/* MAC level identifier */
	short set_level = 1;            /* flag to set level */

	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG90);
		quit();
	}
	if (tag && !find_pm(fp, tag)) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG91,tag);
		quit();
	}
	if (type && !(tp = find_type(fp, type))) {
		pfmt (stderr,MM_ERROR,MSG91,type);
		quit();
	}
	(void) fclose(fp);

	if (tag) {

/*
 * treat the case of 1 PM as a degenerate case of a list of PMs from a
 * type specification.  Build the 'list' here.
 */

		tp = &tl;
		tp->t_next = NULL;
		(void) strcpy(tp->t_tag, tag);
	}

	added = 0;
	while (tp) {
		(void) sprintf(fname, "%s/%s/_pmtab", HOME, tp->t_tag);
		ret = check_version(version, fname);
		if (ret == 1) {
			Saferrno = E_SAFERR;
			pfmt (stderr,MM_ERROR,MSG92,fname);
			quit();
		}
		else if (ret == 2) {
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG93,fname);
			quit();
		}
		else if (ret == 3) {
			Saferrno = E_SAFERR;
			pfmt (stderr,MM_ERROR,MSG89,fname);
			quit();
		}
		fp = fopen(fname, "r");
		if (fp == NULL) {
			pfmt (stderr,MM_ERROR,MSG93,fname);
			quit();
		}
		if (find_svc(fp, tp->t_tag, svctag)) {
			if (tag) {
				/* special case of tag only */
				Saferrno = E_DUP;
				pfmt (stderr,MM_ERROR,MSG94,svctag, tag);
				quit();
			}
			else {
				pfmt (stdout, MM_WARNING, MSG60, svctag, tp->t_tag);
				tp = tp->t_next;
				(void) fclose(fp);
				continue;
			}
		}
		(void) fclose(fp);

/*
 * put in the config script, if specified
*/

		if (script) {
			(void) sprintf(fname, "%s/%s", tp->t_tag, svctag);
			if (do_config(script, fname)) {
				/* do_config put out any messages */
				tp = tp->t_next;
				continue;
			}

			(void) sprintf(buf, "%s/%s", HOME, fname);

            /* set svctag file to SYS_PRIVATE */
		    if (lvlin("SYS_PRIVATE", &level_lid) < 0) {
				if (mac_check()) {
					Saferrno = E_SYSERR;
					pfmt (stderr,MM_ERROR,MSG95);
					quit();
				} else set_level = 0;
			}
			if (set_level) {
				if ( lvlfile(buf, MAC_SET, &level_lid) != 0 ) {
					Saferrno = E_NOPRIV;
					pfmt (stderr,MM_ERROR,MSG96,HOME,fname);
					quit();
				}
			}

			if (chown(buf, (uid_t) SAF_OWNER, (gid_t) SAF_GROUP) != 0) {
				Saferrno = E_SYSERR;
				pfmt (stderr,MM_ERROR,MSG97,HOME,fname);
				quit();
			}
		}

/*
 * add the line
 */

		(void) sprintf(fname, "%s/%s/_pmtab", HOME, tp->t_tag);
		fp = fopen(fname, "a");
		if (fp == NULL) {
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG93,fname);
			quit();
		}
		(void) fprintf(fp, "%s:%s:%s:reserved:reserved:%s:%s#%s\n",
			svctag, (flags ? pflags(flags, FALSE) : ""), 
			(id ? id : ""),
			(scheme ? scheme : ""), pmspec,
			(comment ? comment : ""));
		(void) fclose(fp);
		added++;

/*
 * tell the SAC to to tell PM to read _pmtab
 */

		(void) tell_sac(tp->t_tag);
		tp = tp->t_next;
	}
	if (added == 0) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG68);
		quit();
	}
	return;
}


/*
 * Procedure: rem_svc - remove a service
 *
 * Args 	pmtag - tag of port monitor responsible for the service
 *		svctag - tag of the service to be removed
 *
 * Restrictions:
 *               fopen: none
 *               unlink(2): none
 */

void
rem_svc(pmtag, svctag)
char *pmtag;
char *svctag;
{
	FILE *fp;		/* scratch file pointer */
	FILE *tfp;		/* file pointer for temp file */
	int line;		/* line number entry is on */
	char *tname;		/* temp file name */
	char buf[SIZE];		/* scratch buffer */
	char fname[SIZE];	/* path to correct _pmtab */
	char fullname[SIZE];	/* full path to correct _pmtab */
	
	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG90);
	}
	if (!find_pm(fp, pmtag)) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG91,pmtag);
		quit();
	}
	(void) fclose(fp);

	(void) sprintf(fname, "%s/_pmtab", pmtag);
	(void) sprintf(fullname, "%s/%s", HOME, fname);
	fp = fopen(fullname, "r");
	if (fp == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG93,fullname);
		quit();
	}
	if ((line = find_svc(fp, pmtag, svctag)) == 0) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG98,svctag,pmtag);
		quit();
	}
	tname = make_tempname(fname);
	tfp = open_temp(tname);
	if (line != 1) {
		if (copy_file(fp, tfp, 1, line - 1)) {
			(void) unlink(tname);
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG99);
			quit();
		}
	}
	if (copy_file(fp, tfp, line + 1, -1)) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG99);
		quit();
	}
	(void) fclose(fp);
	if (fclose(tfp) == EOF) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG100);
		quit();
	}
	/* note - replace only returns if successful */
	replace(fullname, tname);

/*
 * tell the SAC to to tell PM to read _pmtab
 */

	if (tell_sac(pmtag)) {

/*
 * if we got rid of the service, try to remove the config script too.
 * Don't check return status since it may not have existed anyhow.
 */

		(void) sprintf(buf, "%s/%s/%s", HOME, pmtag, svctag);
		(void) unlink(buf);
		return;
	}
}



/*
 * Procedure: ed_svc - enable or disable a particular service
 * Restrictions:
 *               fopen: none
 *               unlink(2): none
 *
 * Args 	pmtag - tag of port monitor responsible for the service
 *		svctag - tag of service to be enabled or disabled
 *		flag - operation to perform (ENABLE or DISABLE)
 *
 */

void
ed_svc(pmtag, svctag, flag)
char *pmtag;
char *svctag;
int flag;
{
	FILE *fp;		/* scratch file pointer */
	FILE *tfp;		/* file pointer for temp file */
	int line;		/* line number entry is on */
	register char *from;	/* working pointer */
	register char *to;	/* working pointer */
	char *tname;		/* temp file name */
	char *p;		/* scratch pointer */
	char buf[SIZE];		/* scratch buffer */
	char tbuf[SIZE];	/* scratch buffer */
	char fname[SIZE];	/* path to correct _pmtab */
	char fullname[SIZE];	/* full path to correct _pmtab */
	
	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG90);
		quit();
	}
	if (!find_pm(fp, pmtag)) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG91,pmtag);
		quit();
	}
	(void) fclose(fp);

	(void) sprintf(fname, "%s/_pmtab", pmtag);
	(void) sprintf(fullname, "%s/%s", HOME, fname);
	fp = fopen(fullname, "r");
	if (fp == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG93,fullname);
		quit();
	}
	if ((line = find_svc(fp, pmtag, svctag)) == 0) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG98,svctag,pmtag);
		quit();
	}
	tname = make_tempname(fname);
	tfp = open_temp(tname);
	if (line != 1) {
		if (copy_file(fp, tfp, 1, line - 1)) {
			(void) unlink(tname);
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG99);
			quit();
		}
	}

/*
 * Note: find_svc above has already read and parsed this entry, thus
 * we know it to be well-formed, so just change the flags as appropriate
 */

	if (fgets(buf, SIZE, fp) == NULL) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG99);
		quit();
	}
	from = buf;
	to = tbuf;

/*
 * copy initial portion of entry
 */

	p = strchr(from, DELIMC);
	for ( ; from <= p; )
		*to++ = *from++;

/*
 * isolate and fix the flags
 */

	p = strchr(from, DELIMC);
	for ( ; from < p; ) {
		if (*from == 'x') {
			from++;
			continue;
		}
		*to++ = *from++;
	}

/*
 * above we removed x flag, if this was a disable operation, stick it in
 * and also copy the field delimiter
 */

	if (flag == DISABLE)
		*to++ = 'x';
	*to++ = *from++;

/*
 * copy the rest of the line
 */

	for ( ; *from; )
		*to++ = *from++;
	*to = '\0';

	(void) fprintf(tfp, "%s", tbuf);

	if (copy_file(fp, tfp, line + 1, -1)) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG99);
		quit();
	}
	(void) fclose(fp);
	if (fclose(tfp) == EOF) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG100);
		quit();
	}
	/* note - replace only returns if successful */
	replace(fullname, tname);


/*
 * tell the SAC to to tell PM to read _pmtab
 */

	(void) tell_sac(pmtag);
}


/*
 * Procedure: change - change authentication information
 *
 * Args:	pmtag - port monitor's tag
 *		svctag - service's tag
 *		id - identity under which service should run (may be null)
 *		scheme - authentication scheme (may be null)
 *
 * Restrictions:
 *               fopen: none
 *               unlink(2): none
 */

void
change(pmtag, svctag, id, scheme)
char *pmtag;
char *svctag;
char *id;
char *scheme;
{
	FILE *fp;			/* scratch file pointer */
	FILE *tfp;			/* file pointer for temp file */
	int line;			/* line number entry is on */
	char *tname;			/* temp file name */
	register struct pmtab *p;	/* pointer to parsed info */
	char buf[SIZE];			/* scratch buffer */
	char fname[SIZE];		/* path to correct _pmtab */
	char fullname[SIZE];		/* full path to correct _pmtab */
	
	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG90);
		quit();
	}
	if (!find_pm(fp, pmtag)) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG91,pmtag);
		quit();
	}
	(void) fclose(fp);

	(void) sprintf(fname, "%s/_pmtab", pmtag);
	(void) sprintf(fullname, "%s/%s", HOME, fname);
	fp = fopen(fullname, "r");
	if (fp == NULL) {
		pfmt (stderr,MM_ERROR,MSG93,fullname);
		quit();
	}
	if ((line = find_svc(fp, pmtag, svctag)) == 0) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG98,svctag,pmtag);
		quit();
	}
	tname = make_tempname(fname);
	tfp = open_temp(tname);
	if (line != 1) {
		if (copy_file(fp, tfp, 1, line - 1)) {
			(void) unlink(tname);
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG99);
			quit();
		}
	}

/*
 * Note: find_svc above has already read and parsed this entry, thus
 * we know it to be well-formed and in Pmtab, so just make changes as
 * appropriate
 */

/*
 * flush the line
 */

	if (fgets(buf, SIZE, fp) == NULL) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG99);
		quit();
	}

	p = &Pmtab;
	(void) fprintf(tfp, "%s:%s:%s:reserved:reserved:%s:%s#%s\n",
			p->p_tag, (p->p_flags ? pflags(p->p_flags, FALSE) : ""),
			(id ? id : p->p_id), (scheme ? scheme : p->p_scheme),
			p->p_pmspec, Comment);

	if (copy_file(fp, tfp, line + 1, -1)) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG99);
		quit();
	}
	(void) fclose(fp);
	if (fclose(tfp) == EOF) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG100);
		quit();
	}
	/* note - replace only returns if successful */
	replace(fullname, tname);

/*
 * tell the SAC to tell PM to read _pmtab
 */

	(void) tell_sac(pmtag);
}


/*
 * Procedure: doconf - take a config script and have it put where it
 *		       belongs or output an existing one
 *
 * Restrictions:
 *               fopen: none
 * Args 	script - name of file containing script (if NULL, means
 *			 output existing one instead)
 *		tag - tag of port monitor that is responsible for the
 *		      designated service (may be null)
 *		type - type of port monitor that is responsible for the
 *		       designated service (may be null)
 *		svctag - tag of service whose config script we're operating on
 */

void
doconf(script, tag, type, svctag)
char *script;
char *tag;
char *type;
char *svctag;
{
	FILE *fp;			/* scratch file pointer */
	int added;			/* count of config scripts added */
	struct taglist tl;		/* 'list' for degenerate case (1 PM) */
	register struct taglist *tp;	/* working pointer */
	char buf[SIZE];			/* scratch buffer */
	char fname[SIZE];		/* scratch buffer for names */

	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG90);
		quit();
	}
	if (tag && !find_pm(fp, tag)) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG91,tag);
		quit();
	}
	if (type && !(tp = find_type(fp, type))) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG91,type);
		quit();
	}
	(void) fclose(fp);

	if (tag) {

/*
 * treat the case of 1 PM as a degenerate case of a list of PMs from a
 * type specification.  Build the 'list' here.
 */

		tp = &tl;
		tp->t_next = NULL;
		(void) strcpy(tp->t_tag, tag);
	}

	added = 0;
	while (tp) {
		(void) sprintf(fname, "%s/%s/_pmtab", HOME, tp->t_tag);
		fp = fopen(fname, "r");
		if (fp == NULL) {
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG88,fname);
			quit();
		}
		if (!find_svc(fp, tp->t_tag, svctag)) {
			if (tag) {
				/* special case of tag only */
				Saferrno = E_NOEXIST;
				pfmt (stderr,MM_ERROR,MSG98,svctag,tag);
				quit();
			}
			else {
				pfmt (stdout, MM_WARNING, MSG61, svctag, tp->t_tag);
				Saferrno = E_NOEXIST;
				tp = tp->t_next;
				(void) fclose(fp);
				continue;
			}
		}
		(void) fclose(fp);

		(void) sprintf(fname, "%s/%s", tp->t_tag, svctag);

/*
 * do_config does all the real work (keep track if any errors occurred)
 */

		if (do_config(script, fname) == 0)
			added++;
		tp = tp->t_next;
	}
	if (added == 0) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG101);
		quit();
	}
	return;
}


/*
 * Procedure: tell_sac - use sacadm to tell the sac to tell a port
 *			 monitor to read its _pmtab.
 *
 * Notes: Return TRUE on success, FALSE on failure.
 *
 * Args:	tag - tag of port monitor to be notified
 *
 * Restrictions:
 *               fopen: none
 *               execl: none
 */


tell_sac(tag)
char *tag;
{
	pid_t pid;	/* returned pid from fork */
	int status;	/* return status from sacadm child */

	if ((pid = fork()) < 0) {
		pfmt (stdout, MM_WARNING, MSG62, tag);
		pfmt (stdout, MM_WARNING, MSG63, tag);
		Saferrno = E_NOCONTACT;
		return(FALSE);
	}
	else if (pid) {
		/* parent */
		(void) wait(&status);
		if (status) {
			if (((status >> 8) & 0xff) == E_PMNOTRUN) {
				pfmt (stdout, MM_WARNING, MSG64, tag);
			}
			else {
				pfmt (stdout, MM_WARNING, MSG65, tag);
				pfmt (stdout, MM_WARNING, MSG66, tag);
				Saferrno = E_NOCONTACT;
			}
			return(FALSE);
		}
		else {
			return(TRUE);
		}
	}
	else {
		/* set IFS for security */
		(void) putenv("IFS=\" \"");
		/* muffle sacadm warning messages */
		(void) fclose(stderr);
		(void) fopen("/dev/null", "w");
		(void) execl("/usr/sbin/sacadm", "sacadm", "-x", "-p", tag, 0);

/*
 * if we got here, it didn't work, exit status will clue in parent to
 * put out the warning
 */

		exit(1);
	}
	/* NOTREACHED */
}


/*
 * Procedure: list_svcs - list information about services
 * Restrictions:
 *               fopen: none
 *
 * Args 	pmtag - tag of port monitor responsible for the service
 *			(may be null)
 *		type - type of port monitor responsible for the service
 *		       (may be null)
 *		svctag - tag of service to be listed (may be null)
 *		oflag - true if output should be easily parseable
 */

void
list_svcs(pmtag, type, svctag, oflag)
char *pmtag;
char *type;
char *svctag;
{
	FILE *fp;				/* scratch file pointer */
	register struct taglist *tp;		/* pointer to PM list */
	int nprint = 0;				/* count # of svcs printed */
	register struct pmtab *pp = &Pmtab;	/* pointer to parsed info */
	register char *p;			/* working pointer */
	char buf[SIZE];				/* scratch buffer */
	char fname[SIZE];			/* scratch buffer for building names */

	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG90);
		quit();
	}
	if (pmtag && !find_pm(fp, pmtag)) {
		Saferrno = E_NOEXIST;
		pfmt (stderr,MM_ERROR,MSG91,pmtag);
		quit();
	}
	rewind(fp);
	if (type) {
		tp = find_type(fp, type);
		if (tp == NULL) {
			Saferrno = E_NOEXIST;
			pfmt (stderr,MM_ERROR,MSG91,type);
			quit();
		}
	}
	else
		tp = find_type(fp, NULL);
	(void) fclose(fp);

	while (tp) {
		if (pmtag && strcmp(tp->t_tag, pmtag)) {
			/* not interested in this port monitor */
			tp = tp->t_next;
			continue;
		}
		(void) sprintf(fname, "%s/%s/_pmtab", HOME, tp->t_tag);
		fp = fopen(fname, "r");
		if (fp == NULL) {
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG93,fname);
			quit();
		}
		while (fgets(buf, SIZE, fp)) {
			p = trim(buf);
			if (*p == '\0')
				continue;
			parseline(p, pp, tp->t_tag);
			if (!svctag || !strcmp(pp->p_tag, svctag)) {
				if (oflag) {
					(void) printf("%s:%s:%s:%s:%s:%s:%s:%s:%s#%s\n",
						tp->t_tag, tp->t_type, pp->p_tag,
						pflags(pp->p_flags, FALSE),
						pp->p_id, pp->p_res1, pp->p_res2,
						pp->p_scheme, pp->p_pmspec, Comment);
				}
				else {
					if (nprint == 0) {
						(void) printf("PMTAG          PMTYPE         SVCTAG         FLGS ID       SCHEME   <PMSPECIFIC>\n");
					}
					(void) printf("%-14s %-14s %-14s %-4s %-8s %-6s %s #%s\n", tp->t_tag, tp->t_type, pp->p_tag,
						pflags(pp->p_flags, TRUE), (*pp->p_id ? pp->p_id : "-       "), (*pp->p_scheme ? pp->p_scheme : "-     "), pspec(pp->p_pmspec), Comment);
				}
				nprint++;
			}
		}
		if (!feof(fp)) {
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG102,fname);
			quit();
		}
		else {
			(void) fclose(fp);
			tp = tp->t_next;
		}
	}
	/* if we didn't find any valid ones, indicate an error */
	if (nprint == 0) {

		if (svctag)
			pfmt (stderr, MM_ERROR, MSG67, svctag);
		else
			pfmt (stderr, MM_ERROR, MSG68);
		Saferrno = E_NOEXIST;
	}
	return;
}


/*
 * Procedure: find_svc - find an entry in _pmtab for a particular service tag
 *
 * Args:	fp - file pointer for _pmtab
 *		tag - port monitor tag (for error reporting)
 *		svctag - tag of service we're looking for
 */

find_svc(fp, tag, svctag)
FILE *fp;
char *tag;
char *svctag;
{
	register char *p;	/* working pointer */
	int line = 0;		/* line number we found entry on */
	static char buf[SIZE];	/* scratch buffer */

	while (fgets(buf, SIZE, fp)) {
		line++;
		p = trim(buf);
		if (*p == '\0')
			continue;
		parseline(p, &Pmtab, tag);
		if (!(strcmp(Pmtab.p_tag, svctag)))
			return(line);
	}
	if (!feof(fp)) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG103, HOME, tag);
		quit();
	}
	else
		return(0);
	/* NOTREACHED */
}


/*
 * Procedure: parseline - parse a line from _pmtab.  This routine will
 *			  return if the parse wa successful, otherwise
 *			  it will output an error and exit.
 *
 * Args:	p - pointer to the data read from the file (note - this is
 *		    a static data region, so we can point into it)
 *		pp - pointer to a structure in which the separated fields
 *		     are placed
 *		tag - port monitor tag (for error reporting)
 *
 * Notes: A line in the file has the following format: 
 *	  tag:flags:identity:reserved:reserved:scheme:PM_spec_info # comment
 */


void
parseline(p, pp, tag)
register char *p;
register struct pmtab *pp;
char *tag;
{
	char buf[SIZE];	/* scratch buffer */

/*
 * get the service tag
 */

	p = nexttok(p, DELIM, FALSE);
	if (p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG104, HOME, tag);
		quit();
	}
	if (strlen(p) > PMTAGSIZE) {
		p[PMTAGSIZE] = '\0';
		pfmt (stderr, MM_ERROR, MSG57, p);
	}
	pp->p_tag = p;

/*
 * get the flags
 */

	p = nexttok(NULL, DELIM, FALSE);
	if (p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG104, HOME, tag);
		quit();
	}
	pp->p_flags = 0;
	while (*p) {
		switch (*p++) {
		case 'u':
			pp->p_flags |= U_FLAG;
			break;
		case 'x':
			pp->p_flags |= X_FLAG;
			break;
		default:
			Saferrno = E_SAFERR;
			pfmt (stderr,MM_ERROR,MSG105,*(p - 1));
			quit();
		}
	}

/*
 * get the identity
 */

	p = nexttok(NULL, DELIM, FALSE);
	if (p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG104, HOME, tag);
		quit();
	}
	pp->p_id = p;

/*
 * get the first reserved field
 */

	p = nexttok(NULL, DELIM, FALSE);
	if (p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG104, HOME, tag);
		quit();
	}
	pp->p_res1 = p;

/*
 * get the second reserved field
 */

	p = nexttok(NULL, DELIM, FALSE);
	if (p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG104, HOME, tag);
		quit();
	}
	pp->p_res2 = p;

/*
 * get the authentication scheme
 */

	p = nexttok(NULL, DELIM, FALSE);
	if (p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG104, HOME, tag);
		quit();
	}
	pp->p_scheme = p;

/*
 * the rest is the port monitor specific info
 */

	p = nexttok(NULL, DELIM, TRUE);
	if (p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG104, HOME, tag);
		quit();
	}
	pp->p_pmspec = p;
	return;
}


/*
 * Procedure: pspec - format port monitor specific information
 *
 * Args:	spec - port monitor specific info, separated by
 *		       field separater character (may be escaped by \)
 */

char *
pspec(spec)
char *spec;
{
	static char buf[SIZE];		/* returned string */
	register char *from;		/* working pointer */
	register char *to;		/* working pointer */
	int newflag;			/* flag indicating new field */

	to = buf;
	from = spec;
	newflag = 1;
	while (*from) {
		switch (*from) {
		case ':':
			if (newflag) {
				*to++ = '-';
			}
			*to++ = ' ';
			from++;
			newflag = 1;
			break;
		case '\\':
			if (*(from + 1) == ':') {
				*to++ = ':';
				/* skip over \: */
				from += 2;
			}
			else
				*to++ = *from++;
			newflag = 0;
			break;
		default:
			newflag = 0;
			*to++ = *from++;
		}
	}
	*to = '\0';
	return(buf);
}


/*
 * Procedure: pflags - put service flags into intelligible form for output
 *
 * Args:	flags - binary representation of flags
 *		dflag - true if a "-" should be returned if no flags
 */

char *
pflags(flags, dflag)
long flags;
int dflag;
{
	register int i;			/* scratch counter */
	static char buf[SIZE];		/* formatted flags */

	if (flags == 0) {
		if (dflag)
			return("-");
		else
			return("");
	}
	i = 0;
	if (flags & U_FLAG) {
		buf[i++] = 'u';
		flags &= ~U_FLAG;
	}
	if (flags & X_FLAG) {
		buf[i++] = 'x';
		flags &= ~X_FLAG;
	}
	if (flags) {
		Saferrno = E_SAFERR;
		pfmt (stderr,MM_ERROR,MSG106);
		quit();
	}
	buf[i] = '\0';
	return(buf);
}


/*
 * Procedure: find_type - find entries in _sactab for a particular port
 *			  monitor type 
 *
 * Args:	fp - file pointer for _sactab
 *		type - type of port monitor we're looking for (if type is
 *		       null, it means find all PMs)
 */

struct taglist *
find_type(fp, type)
FILE *fp;
char *type;
{
	register char *p;			/* working pointer */
	struct sactab stab;			/* place to hold parsed info */
	register struct sactab *sp = &stab;	/* and a pointer to it */
	char buf[SIZE];				/* scratch buffer */
	struct taglist *thead;			/* linked list of tags */
	register struct taglist *temp;		/* scratch pointer */

	thead = NULL;
	while (fgets(buf, SIZE, fp)) {
		p = trim(buf);
		if (*p == '\0')
			continue;
		parse(p, sp);
		if ((type == NULL) || !(strcmp(sp->sc_type, type))) {
			temp = (struct taglist *) malloc(sizeof(struct taglist));
			if (temp == NULL) {
				Saferrno = E_SYSERR;
				pfmt (stderr,MM_ERROR,MSG107);
				quit();
			}
			temp->t_next = thead;
			(void) strcpy(temp->t_tag, sp->sc_tag);
			(void) strcpy(temp->t_type, sp->sc_type);
			thead = temp;
		}
	}
	if (!feof(fp)) {
		Saferrno = E_SYSERR;
		pfmt (stderr,MM_ERROR,MSG108);
	}
	else
		return(thead ? thead : NULL);
	/* NOTREACHED */
}
