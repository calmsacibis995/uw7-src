#ident	"@(#)pcnfsd_misc.c	1.3"
#ident	"$Header$"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
**=====================================================================
** Copyright (c) 1986-1993 by Sun Microsystems, Inc.
**	@(#)pcnfsd_misc.c	1.13	08 Feb 1993
**=====================================================================
*/

/*******************************************************************************
 *
 * FILENAME:    pcnfsd_misc.c
 *
 * SCCS:	pcnfsd_misc.c 1.3  11/11/97 at 16:23:47
 *
 * CHANGE HISTORY:
 *
 * 11-11-97  Paul Cunningham        MR us96-10602
 *           Changed to apply the esculation 132384 fix for ptf3025, see MR
 *           resolution for a description of the fix.
 *           Also changed pcnfsd_print.c
 *
 *******************************************************************************
 */

/*
**=====================================================================
** Any and all changes made herein to the original code obtained from
** Sun Microsystems may not be supported by Sun Microsystems, Inc.
**=====================================================================
*/

#include "common.h"
#include "pcnfsd.h"
#include <stdio.h>
#include <pwd.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <ia.h>
#include <unistd.h>
#include <pfmt.h>

#ifdef SHADOW_SUPPORT
#include <shadow.h>
#endif

#ifdef WTMP
int wtmp_enabled = 1;
#endif

#ifdef USE_GETUSERSHELL
extern char *getusershell();
#endif

/*
**---------------------------------------------------------------------
** Other #define's 
**---------------------------------------------------------------------
*/

#define	zchar		0x5b
#define NOBODY	(uid_t)(-2)

extern char	sp_name[1024]; /* in pcnfsd_print.c */

#define	NUMUIDS	64
int		uidrsize = 0;
int		uidrlo[NUMUIDS];
int		uidrhi[NUMUIDS];
/*
**=====================================================================
**                      C O D E   S E C T I O N                       *
**=====================================================================
*/
/*
**---------------------------------------------------------------------
**                          Support procedures 
**---------------------------------------------------------------------
*/

/*
** grab
**
** bulletproof malloc
*/
void *grab(n)
int n;
{
void *p;
char  tmp[256];

	p = (void *)malloc(n);
	if(p == NULL) {
		sprintf(tmp, gettxt(":114", "%s: %s failed"), "grab", "malloc");
		msg_out(tmp);
		exit(1);
	}
	return(p);
}

char *
my_strdup(s)
char *s;
{
char *r;
	r = (char *)grab(strlen(s)+1);
	strcpy(r, s);
	return(r);
}

/*
** This is the latest word on the security check. The following
** routine "suspicious()" returns non-zero if the character string
** passed to it contains any shell metacharacters.
** Callers will typically code
**
**	if(suspicious(some_parameter)) reject();
**
** Non-Unix-based systems: examine this carefully.
**/

int suspicious (s)
char *s;
{
  static char ok_chars[] =
      "1234567890@%-_=+:,.abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  if (strspn(s,ok_chars) != strlen(s))
	return 1;
  else
	return 0;
}

void
scramble(s1, s2)
char           *s1;
char           *s2;
{
	while (*s1) 
	      {
	      *s2++ = (*s1 ^ zchar) & 0x7f;
	      s1++;
	      }
	*s2 = 0;
}


/*
** build_uid_ranges(s)
*/
void
build_uid_ranges(s)
char *s;
{
	char *r1;
	char *r2;
	int v1;
	int v2;

	r1 = strtok(s, " ,\t");
	while(r1 && uidrsize < NUMUIDS) {
		if(r2 = strchr(r1, '-')) {
			*r2++ = '\0';
		}
		v1 = atoi(r1);
		if(v1 == 0 && *r1 != '0')	/* syntax error */
			continue;
		if(r2) {
			v2 = atoi(r2);
			if(v2 == 0 && *r2 != 0)	/* syntax */
				continue;
			if(v2 < v1)		/* botch */
				continue;
		} else
			v2 = v1;
		uidrlo[uidrsize] = v1;
		uidrhi[uidrsize] = v2;
		uidrsize++;
		r1 = strtok(NULL, " ,\t");
	}
}
/*
** val_uid(u)
**
** Return non-zero if the uid is legal - within range
** and zero otherwise.
**
** If uid_range is NULL, check for 101-60002. Otherwise
** check against ranges.
*/
int
val_uid(u)
int u;
{
	int i;

	if(uidrsize == 0)
		return(u >= 101 && u <= 60002);

	for (i = 0; i < uidrsize; i++)
		if (u >= uidrlo[i] && u <= uidrhi[i])
			return(1);
	return(0);
}


struct passwd  *
get_password(usrnam, msgpp)
char           *usrnam;
char	      **msgpp;
{
struct passwd  *p;
static struct passwd localp;
char           *pswd;
char           *ia_pwdp;
char           *ushell;
int		ok = 0;


#ifdef SHADOW_SUPPORT
struct spwd    *sp;
long		today;
long		expday;
uinfo_t	uinfo;
#endif

#ifdef SHADOW_SUPPORT
/* 
 * ia_openinfo is the valid way to retrieve shadow information for
 * particular login id.
 */
	if ((p = getpwnam(usrnam)) == (struct passwd *)NULL ||
	 (ia_openinfo(usrnam, &uinfo) || (uinfo == NULL))) 
		return ((struct passwd *)NULL);

/*
 * New - check password expiry situation
 *
 *	compute today (as days since Jan.1 1970)
 */
	today = (long)time((time_t *)0)/86400;
/*
 *	If sp_expire is greater than zero and is less than today, the password
 *	has expired.
 */
	if(uinfo->ia_expire > 0 && today > uinfo->ia_expire) {
		*msgpp = gettxt(":162", "This account may no longer be used.");
		ia_closeinfo(uinfo);
		return ((struct passwd *)NULL);
	}
/*
 *	If sp_max is greater zero, the password will expire on
 *	sp_lstchg+sp_max. If today is greater than this, it already did.
 */
	if(uinfo->ia_max > 0) {
		expday = uinfo->ia_max+uinfo->ia_lstchg;
		if(today > expday) {
			*msgpp = gettxt(":163", "The password for this account has expired.");
			ia_closeinfo(uinfo);
			return ((struct passwd *)NULL);
		}
/*
 *	Next, subtract the warning period, sp_warn, from
 *	expday. This is the first day on which warnings should
 *	be issued. If today >= expday, set msgp, but proceed.
 */
		if(uinfo->ia_warn >= 0) {
			expday -= uinfo->ia_warn;
			if(today > expday) {
				*msgpp = gettxt(":164", "The password for this account will shortly expire. Please change it.");
			}
		}
	}
	ia_get_logpwd((uinfo_t)uinfo, &ia_pwdp);
	pswd = ia_pwdp;
	ia_closeinfo(uinfo);

#else
	p = getpwnam(usrnam);
	if (p == (struct passwd *)NULL)
		return ((struct passwd *)NULL);
	pswd = p->pw_passwd;
#endif

	localp = *p;
	localp.pw_passwd = pswd;
/*
 * Fix SDR#pcn1857: If the shell field is null, this entry is legal
 * (assuming that the default shell, "/bin/sh" is always legal).
 */
	if(localp.pw_shell == NULL || strlen(localp.pw_shell) == 0)
		return(&localp);

#ifdef USE_GETUSERSHELL

	setusershell();
	while(ushell = getusershell()){
		if(!strcmp(ushell, localp.pw_shell)) {
			ok = 1;
			break;
		}
	}
	endusershell();
	if(!ok)
		return ((struct passwd *)NULL);
#else
/*
 * the best we can do is to ensure that the shell ends in "sh"
 */
	ushell = localp.pw_shell;
	if((int)strlen(ushell) < 2)
		return ((struct passwd *)NULL);
	ushell += strlen(ushell) - 2;
	if(strcmp(ushell, "sh"))
		return ((struct passwd *)NULL);

#endif
	if(!val_uid(localp.pw_uid)) {
		*msgpp = gettxt(":165", "No access via pcnfsd to this account.");
		return((struct passwd *)NULL);
	}
	return (&localp);
}

/*
**---------------------------------------------------------------------
**                      Print support procedures 
**---------------------------------------------------------------------
*/

char           *
mapfont(f, i, b)
	char            f;
	char            i;
	char            b;
{
	static char     fontname[64];

	fontname[0] = 0;	/* clear it out */

	switch (f) {
	case 'c':
		(void)strcpy(fontname, "Courier");
		break;
	case 'h':
		(void)strcpy(fontname, "Helvetica");
		break;
	case 't':
		(void)strcpy(fontname, "Times");
		break;
	default:
		(void)strcpy(fontname, "Times-Roman");
		goto finis ;
	}
	if (i != 'o' && b != 'b') {	/* no bold or oblique */
		if (f == 't')	/* special case Times */
			(void)strcat(fontname, "-Roman");
		goto finis;
	}
	(void)strcat(fontname, "-");
	if (b == 'b')
		(void)strcat(fontname, "Bold");
	if (i == 'o')		/* o-blique */
		(void)strcat(fontname, f == 't' ? "Italic" : "Oblique");

finis:	return (&fontname[0]);
}

/*
 * run_ps630 performs the Diablo 630 emulation filtering process. ps630
 * was broken in certain Sun releases: it would not accept point size or
 * font changes. If your version is fixed, undefine the symbol
 * PS630_IS_BROKEN and rebuild pc-nfsd.
 */
/* #define PS630_IS_BROKEN 1 */

void
run_ps630(f, opts)
	char           *f;
	char           *opts;
{
	char            temp_file[256];
	char            commbuf[512];
	int             i;

	(void)strcpy(temp_file, f);
	(void)strcat(temp_file, "X");	/* intermediate file name */

#ifndef PS630_IS_BROKEN
	(void)sprintf(commbuf, "/usr/bin/ps630 -s '%c%c' -p '%s' -f '",
		opts[2], opts[3], temp_file);
	(void)strcat(commbuf, mapfont(opts[4], opts[5], opts[6]));
	(void)strcat(commbuf, "' -F '");
	(void)strcat(commbuf, mapfont(opts[7], opts[8], opts[9]));
	(void)strcat(commbuf, "'  '");
	(void)strcat(commbuf, f);
	(void)strcat(commbuf, "'");
#else	/* PS630_IS_BROKEN */
	/*
	 * The pitch and font features of ps630 appear to be broken at
	 * this time.
	 */
	(void)sprintf(commbuf, "/usr/bin/ps630 -p '%s' '%s'", temp_file, f);
#endif	/* PS630_IS_BROKEN */


	if (i = system(commbuf)) {
		/*
		 * Under (un)certain conditions, ps630 may return -1 even
		 * if it worked. Hence the commenting out of this error
		 * report.
		 */
		 /* (void)fprintf(stderr, "\n\nrun_ps630 rc = %d\n", i) */ ;
		/* exit(1); */
	}
	if (rename(temp_file, f)) {
		pfmt(stderr, MM_ERROR, ":148:%s: %s failed from %s to %s\n",
		     "run_ps630", "rename", temp_file, f);
		/* exit(1);  This would exit all of pcnfsd */
	}
	return;
}

/*
**---------------------------------------------------------------------
**                      WTMP update support 
**---------------------------------------------------------------------
*/


#ifdef WTMP

#include <utmpx.h>
#include <sac.h>

void
wlogin(name)
        char *name;
{

extern char *getcallername();

	struct utmpx ut;

	if(!wtmp_enabled)
		return;
	memset((char *)&ut, 0, sizeof (ut));
	ut.ut_type = USER_PROCESS;
	ut.ut_id[0] = 'n';
	ut.ut_id[1] = 'f';
	ut.ut_id[2] = 's';
	ut.ut_id[3] = SC_WILDC;
        (void) strncpy(ut.ut_name,name,sizeof ut.ut_name);
        (void) time (&ut.ut_tv.tv_sec);
        (void) strncpy(ut.ut_host, getcallername(), sizeof ut.ut_host);
	ut.ut_syslen = strlen(ut.ut_host) + 1; /* hdr file says include null */
        (void) strcpy(ut.ut_line, "PC-NFS login");
	(void)updwtmpx(WTMPX_FILE, &ut);
}

#endif WTMP

/*
**---------------------------------------------------------------------
**                      Run-process-as-user procedures 
**---------------------------------------------------------------------
*/


#define	READER_FD	0
#define	WRITER_FD	1

static int      child_pid;

static char     cached_user[64] = "";
static uid_t    cached_uid;
static gid_t    cached_gid;

static	struct sigaction old_action;
static	struct sigaction new_action;
static	struct itimerval timer;

int interrupted = 0;
static	FILE *pipe_handle;

static	void myhandler()
{
 interrupted = 1;
 fclose(pipe_handle);
 kill(child_pid, SIGKILL);
 msg_out(gettxt(":166", "rpc.pcnfsd: su_popen timeout - killed child process"));
}

void start_watchdog(n)
int n;
{
	/*
	 * Setup SIGALRM handler, force interrupt of ongoing syscall
	 */

	new_action.sa_handler = myhandler;
	sigemptyset(&(new_action.sa_mask));
	new_action.sa_flags = 0;
#ifdef SA_INTERRUPT
	new_action.sa_flags |= SA_INTERRUPT;
#endif
	sigaction(SIGALRM, &new_action, &old_action);

	/*
	 * Set interval timer for n seconds
	 */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = n;
	timer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	interrupted = 0;

}

void stop_watchdog()
{
	/*
	 * Cancel timer
	 */

	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);

	/*
 	 * restore old signal handling
	 */
	sigaction(SIGALRM, &old_action, NULL);
}


FILE           *
su_popen(user, cmd, maxtime)
	char           *user;
	char           *cmd;
	int		maxtime;
{
	int             p[2];
	int             parent_fd, child_fd, pid;
	struct passwd *pw;
	char	tmp[256];

	if (strcmp(cached_user, user)) {
		pw = getpwnam(user);
		if (!pw)
			pw = getpwnam("nobody");
		if (pw) {
			cached_uid = pw->pw_uid;
			cached_gid = pw->pw_gid;
			strcpy(cached_user, user);
		} else {
			cached_uid = NOBODY;
			cached_gid = (gid_t) NOBODY;
			cached_user[0] = '\0';
		}
	}
	if(cached_uid != NOBODY && !val_uid((int)cached_uid)) {
		sprintf(tmp, gettxt(":167", "rpc.pcnfsd: rejecting request from user %s (uid %d)"),
			user, (int)cached_uid);
		msg_out(tmp);
		return(NULL);
	};
	if (pipe(p) < 0) {
		msg_out(gettxt(":168", "rpc.pcnfsd: cannot create pipe in su_popen"));
		return (NULL);
	}
	parent_fd = p[READER_FD];
	child_fd = p[WRITER_FD];
	if ((pid = fork()) == 0) {
		int             i;

		for (i = 0; i < 10; i++)
			if (i != child_fd)
				(void) close(i);
		if (child_fd != 1) {
			(void) dup2(child_fd, 1);
			(void) close(child_fd);
		}
		dup2(1, 2);	/* let's get stderr as well */

		(void) setgid(cached_gid);
		(void) setuid(cached_uid);

		(void) execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
		_exit(255);
	}
	if (pid == -1) {
		sprintf(tmp, gettxt(":114", "%s: %s failed"),
			"rpc.pcnfsd", "fork");
		msg_out(tmp);
		close(parent_fd);
		close(child_fd);
		return (NULL);
	}
	child_pid = pid;
	close(child_fd);
	start_watchdog(maxtime);
	pipe_handle = fdopen(parent_fd, "r");
	return (pipe_handle);
}

int
su_pclose(ptr)
	FILE           *ptr;
{
	int             pid, status;

	stop_watchdog();

	fclose(ptr);
	if (child_pid == -1)
		return (-1);
	while ((pid = wait(&status)) != child_pid && pid != -1);
	return (pid == -1 ? -1 : status);
}

/*
** The following routine reads a file "/etc/pcnfsd.conf" if present,
** and uses it to replace certain builtin elements, like the
** name of the print spool directory. The configuration file
** Is the usual kind: Comments begin with '#', blank lines are ignored,
** and valid lines are of the form
**
**	<keyword><whitespace><value>
**
** The following keywords are recognized:
**
**	spooldir dirname
**	printer name alias-for command
**	wtmp yes|no
*/
void
config_from_file()
{
FILE *fd;
char buff[1024];
char *cp;
char *kw;
char *val;
char *arg1;
char *arg2;

	if((fd = fopen("/etc/pcnfsd.conf", "r")) == NULL)
		return;
	while(fgets(buff, 1024, fd)) {
		cp = strchr(buff, '\n');
		*cp = '\0';
		cp = strchr(buff, '#');
		if(cp)
			*cp = '\0';
		kw = strtok(buff, " \t");
		if(kw == NULL)
			continue;
		val = strtok(NULL, " \t");
		if(val == NULL)
			continue;
		if(!mystrcasecmp(kw, "spooldir")) {
			strcpy(sp_name, val);
			continue;
		}
		if(!mystrcasecmp(kw, "uidrange")) {
			build_uid_ranges(val);
			continue;
		}
#ifdef WTMP
		if(!mystrcasecmp(kw, "wtmp")) {
			/* assume default is YES, just look for negatives */
			if(!mystrcasecmp(val, "no") ||
			   !mystrcasecmp(val, "off") ||
			   !mystrcasecmp(val, "disable") ||
			   !strcmp(val, "0"))
				wtmp_enabled = 0;;
			continue;
		}
#endif	
		if(!mystrcasecmp(kw, "printer")) {
			arg1 = strtok(NULL, " \t");
			arg2 = (arg1 == NULL ? NULL : strtok(NULL, ""));
			(void)add_printer_alias(val, arg1, arg2);
			continue;
		}
/*
** Add new cases here
*/
	}
	fclose(fd);
}


/*
** The following are replacements for the SunOS library
** routines strcasecmp and strncasecmp, which SVR4 doesn't
** include.
**
 * The following is introduced - regretfully - to deal with the broken
 * toupper/tolower in libc
 */
static int ToUpper(c)
char c;
{
	return(islower(c) ? toupper(c) : c);
}

int mystrcasecmp(s1, s2)
	char *s1, *s2;
{

	while (ToUpper(*s1) == ToUpper(*s2++))
		if (*s1++ == '\0')
			return(0);
	return(ToUpper(*s1) - ToUpper(*--s2));
}

int mystrncasecmp(s1, s2, n)
	char *s1, *s2;
	int n;
{

	while (--n >= 0 && ToUpper(*s1) == ToUpper(*s2++))
		if (*s1++ == '\0')
			return(0);
	return(n <= 0 ? 0 : ToUpper(*s1) - ToUpper(*--s2));
}


/*
** strembedded - returns true if s1 is embedded (in any case) in s2
*/

int strembedded(s1, s2)
char *s1;
char *s2;
{
	while(*s2) {
		if(!mystrcasecmp(s1, s2))
			return 1;
		s2++;
	}
	return 0;
}
