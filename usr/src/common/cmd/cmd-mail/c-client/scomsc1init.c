#ident	"@(#)scomsc1init.c	11.2"


/*
 * SCO Message Store Version 1 cache initialization routines.
 * basically parse /etc/default/mail and company
 *
 * this file is designed to be linked into other apps without the rest
 * of c-client (for non-c-client applications).
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/param.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <pwd.h>
#include <errno.h>
#include "mail.h"
#include "osdep.h"
#include "scomsc1.h"

#define debug printf

/*
 * parse /etc/default/mail and $HOME/.maildef
 * use getpwent to find home dir, not $HOME environment variable
 * must provide good default values if user put in invalid values
 * the restart argument is used by the test suite to simulate
 * a new user
 */
void
msc1_init(int restart)
{
	FILE *fd;
	char path[MAXPATHLEN*2];		/* buf for creating paths */
	struct passwd *pw;
	static last_user = -1;
	int new_user;

	if (restart)
		last_user = -1;
	/* real and effective uid should be same or we have a security hole */
	new_user = geteuid();
	if (new_user == last_user)	/* did this already */
		return;
	last_user = new_user;
	pw = getpwuid(new_user);
	/* default values */
	conf_mmdf = 1;
	if (conf_inbox)
		free(conf_inbox);
	if (conf_inbox_dir)
		free(conf_inbox_dir);
	if (conf_inbox_name)
		free(conf_inbox_name);
	conf_inbox = 0;			/* complete inbox built later */
	conf_inbox_dir = strdup(MAILSPOOL); /* default directory for inbox */
	conf_inbox_name = 0;		/* default name for inbox */
	conf_fsync = 0;			/* not do fsync */
	conf_extended_checks = 0;	/* don't use extended checks */
	conf_expunge_threshold = 100;	/* always expunge */
	conf_long_cache = 0;		/* not read mbox into core */
	conf_file_locking = 0;		/* file locking (backwards compat) */
	conf_lock_timeout = 10;		/* 10 seconds is default */
	conf_umask = 077;		/* default is to mask everything */

	/* first parse /etc/default/mail */
	msc1_init_parse("/etc/default/mail");
#ifdef OpenServer
	msc1_parse_mmdftailor();
#endif
	/* then parse users path dir/.maildef */
	strcpy(path, pw->pw_dir);
	strcat(path, "/.maildef");
	msc1_init_parse(path);
	msc1_build_home_dir(path, pw);
	/* turn mask into logical-and value */
	conf_umask = (~conf_umask)&0777;
}

/*
 * parse an init file, override any previous values
 */
void
msc1_init_parse(char *fname)
{
	FILE *fp;
	char buf[MAXPATHLEN];
	register char *cp;

	fp = fopen(fname, "r");
	if (fp == 0)
		return;
	while (fgets(buf, MAXPATHLEN, fp)) {
		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n')
			continue;
		cp = buf + strlen(buf) - 1;
		*cp = 0;
		cp = strchr(buf, '=');
		if (cp == 0)
			continue;
		cp++;
		while ((*cp == ' ') || (*cp == '\t'))
			cp++;
		/* have token */
		if (msc1_strhcmp("MS1_FOLDER_FORMAT", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			if (msc1_strccmp(cp, "Sendmail") == 0)
				conf_mmdf = 0;
			if (msc1_strccmp(cp, "MMDF") == 0)
				conf_mmdf = 1;
		}
		else if (msc1_strhcmp("MS1_INBOX_NAME", buf, '=') == 0) {
			if (conf_inbox_name)
				free(conf_inbox_name);
			conf_inbox_name = strdup(cp);
			msc1_strip_quotes(conf_inbox_name);
		}
		else if (msc1_strhcmp("MS1_INBOX_DIR", buf, '=') == 0) {
			if (conf_inbox_dir)
				free(conf_inbox_dir);
			conf_inbox_dir = strdup(cp);
			msc1_strip_quotes(conf_inbox_dir);
		}
		else if (msc1_strhcmp("MS1_FSYNC", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			conf_fsync = msc1_tf(cp);
		}
		else if (msc1_strhcmp("MS1_EXTENDED_CHECKS", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			conf_extended_checks = msc1_tf(cp);
		}
		else if (msc1_strhcmp("MS1_EXPUNGE_THRESHOLD", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			conf_expunge_threshold = atoi(cp);
			if (conf_expunge_threshold < 0)
				conf_expunge_threshold = 0;
			if (conf_expunge_threshold > 100)
				conf_expunge_threshold = 100;
		}
		else if (msc1_strhcmp("MS1_FOLDERS_INCORE", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			conf_long_cache = msc1_tf(cp);
		}
		else if (msc1_strhcmp("MS1_FILE_LOCKING", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			conf_file_locking = msc1_tf(cp);
		}
		else if (msc1_strhcmp("MS1_LOCK_TIMEOUT", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			conf_lock_timeout = atoi(cp);
			if (conf_lock_timeout < 1)
				conf_lock_timeout = 1;
		}
		else if (msc1_strhcmp("MS1_UMASK", buf, '=') == 0) {
			msc1_strip_quotes(cp);
			conf_umask = strtol(cp, 0, 8);
		}
	}
	fclose(fp);
}

/*
 * check for true or false strings
 */
int
msc1_tf(char *cp)
{
	if (msc1_strccmp("TRUE", cp) == 0)
		return(1);
	else if (msc1_strccmp("FALSE", cp) == 0)
		return(0);
	return(0);
}

/*
 * build home inbox from configuration info
 */
void
msc1_build_home_dir(char *buf, struct passwd *pw)
{
	if (conf_inbox_dir && *conf_inbox_dir)
		strcpy(buf, conf_inbox_dir);
	else
		strcpy(buf, pw->pw_dir);
	strcat(buf, "/");
	if (conf_inbox_name && *conf_inbox_name)
		strcat(buf, conf_inbox_name);
	else
		strcat(buf, pw->pw_name);

	if (conf_inbox)
		free(conf_inbox);
	conf_inbox = strdup(buf);
}

#ifdef OpenServer
/*
 * routine to parse mmdftailor
 * we care about MDLVRDIR, MMBXNAME and MMBXPREF
 */
void
msc1_parse_mmdftailor()
{
	FILE *fp;
	char buf[MAXPATHLEN];
	register char *cp;

	fp = fopen("/usr/mmdf/mmdftailor", "r");
	if (fp == 0)
		return;
	while (fgets(buf, MAXPATHLEN, fp)) {
		if (buf[0] == '#')
			continue;
		if (buf[0] == ';')
			continue;
		if (buf[0] == '\n')
			continue;
		cp = buf + strlen(buf) - 1;
		*cp = 0;
		cp = strchr(buf, ' ');
		if (cp == 0)
			cp = strchr(buf, '\t');
		if (cp == 0)
			continue;
		while ((*cp == ' ') || (*cp == '\t'))
			cp++;
		/* have token */
		if (msc1_strxccmp("MDLVRDIR", buf, 0) == 0) {
			if (conf_inbox_dir)
				free(conf_inbox_dir);
			conf_inbox_dir = strdup(cp);
			msc1_strip_quotes(conf_inbox_dir);
		}
		else if (msc1_strxccmp("MMBXNAME", buf, 0) == 0) {
			if (conf_inbox_name)
				free(conf_inbox_name);
			conf_inbox_name = strdup(cp);
			msc1_strip_quotes(conf_inbox_name);
		}
		else if (msc1_strxccmp("MMBXPREF", buf, 0) == 0) {
			msc1_strip_quotes(cp);
			/* we only allow them to turn it off */
			if (*cp == 0)
				conf_mmdf = 0;
		}
	}
	fclose(fp);
}
#endif

void
msc1_strip_quotes(register char *cp)
{
	int l;

	if (*cp != '"')
		return;
	strcpy(cp, cp+1);
	l = strlen(cp);
	if (l == 0)
		return;
	cp += l-1;
	if (*cp == '"')
		*cp = 0;
}

/*
 * special routines for caseless string comparison and header matching
 * returns 0 on match, 1 on no match
 *
 * strxccmp - where STRING<delim character> is what we are trying to match
 *	     strxcmp expects exactly STRING<d> with no white space before <d>
 * strhccmp - like strxccmp except white space can come before <d>
 * strhcmp -  like strhcmp except case is sensitive.
 * strccmp  - basic caseless string compare
 * strnccmp - basic caseless string compare with length param
 *
 * for strxccmp if delim is zero then any white space can follow the token
 * for a match
 */
int
msc1_strxccmp(char *s1, char *s2, int delim)
{
	unsigned char *cp;
	int ret;
	int len;

	len = strlen(s1);
	ret = msc1_strnccmp(s1, s2, len);
	if (ret)
		return(1);
	cp = (unsigned char *)s2 + len;
	if (delim) {
		if (*cp != delim)
			return(1);
	}
	else {
		if ((*cp != ' ') && (*cp != '\t'))
			return(1);
	}
	return(0);
}

int
msc1_strhccmp(char *s1, char *s2, int delim)
{
	unsigned char *cp;
	int ret;
	int len;

	len = strlen(s1);
	ret = msc1_strnccmp(s1, s2, len);
	if (ret)
		return(1);
	cp = (unsigned char *)s2 + len;
	if ((*cp != delim) && (*cp != ' ') && (*cp != '\t'))
		return(1);
	return(0);
}

int
msc1_strhcmp(char *s1, char *s2, int delim)
{
	unsigned char *cp;
	int ret;
	int len;

	len = strlen(s1);
	ret = strncmp(s1, s2, len);
	if (ret)
		return(1);
	cp = (unsigned char *)s2 + len;
	if ((*cp != delim) && (*cp != ' ') && (*cp != '\t'))
		return(1);
	return(0);
}

int
msc1_strccmp(char *s1, char *s2)
{
	register unsigned char *u1;
	register unsigned char *u2;

	u1 = (unsigned char *)s1;
	u2 = (unsigned char *)s2;
	while (*u1) {
		if (*u2 == 0)
			return(1);
		if ((isupper(*u1) ? *u1 : _toupper(*u1)) !=
			(isupper(*u2) ? *u2 : _toupper(*u2)))
			return(1);
		u1++;
		u2++;
	}
	if (*u2)
		return(1);
	return(0);
}

int
msc1_strnccmp(char *s1, char *s2, int len)
{
	register unsigned char *u1;
	register unsigned char *u2;

	u1 = (unsigned char *)s1;
	u2 = (unsigned char *)s2;
	while (len-- > 0) {
		if (*u2 == 0)
			return(1);
		if (*u1 == 0)
			return(1);
		if ((isupper(*u1) ? *u1 : _toupper(*u1)) !=
			(isupper(*u2) ? *u2 : _toupper(*u2)))
			return(1);
		u1++;
		u2++;
	}
	return(0);
}
