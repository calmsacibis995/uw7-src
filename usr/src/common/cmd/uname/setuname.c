#ident	"@(#)uname:common/cmd/uname/setuname.c	1.1.1.1"
#ident	"$Header$"

/*
 * To make this work on native UW2, we need to use the name "_abi_sysinfo"
 * instead of "sysinfo", since that's the only name that was included in the
 * old shared libraries.
 */
#define sysinfo(cmd, buf, len) _abi_sysinfo(cmd, buf, len)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/types.h>
#include <sys/stat.h>

/* setuname options */
static int t_flag;
static char *n_arg;

const char setuname_usage[] = ":1464:Usage: setuname [-t] [-n node]\n";

int set_hostname(char *hostname, int permanent);

extern void usage(int getopt_err);
extern char *_get_param(int si_param, int fail_ok);
#define get_param(si_param)	_get_param(si_param, 0)


int
setuname_main(int argc, char **argv)
{
	int optlet;

#ifndef NOTYET
	/* Make only the text in standard messages appear */
	putenv("MSGVERB=text");
#endif

	while ((optlet = getopt(argc, argv, "tn:s:")) != EOF)
		switch (optlet) {
		case 't':
			t_flag = 1;
			break;
		case 'n':
			if (n_arg) {
				/* Only one -n allowed. */
				usage(0);
			}
			n_arg = optarg;
			break;
		case 's':
			pfmt(stderr, MM_ERROR,
			     ":1459:-s option not supported;"
			     " can't change operating system name.\n");
			return 1;
		case '?':
			usage(1);
		}

	/* -n must be specified. */
	if (n_arg == NULL)
		usage(0);

	return set_hostname(n_arg, !t_flag);
}

#define	DOMAIN_FILE "/etc/resolv.conf"
#define TMP_DOMAIN_FILE "/etc/resolv.conf.new"
#define NODE_FILE "/etc/nodename"
#define TMP_NODE_FILE "/etc/nodename.new"

/*
 * This code is intended to get the default domain name
 * from the indicated file in the spirit of code in
 * common/cmd/cmd-inet/usr.sbin/in.named/ns_init.c, since
 * we would rather not go to the network for it.
 *
 * Based on _dname_read code from libsocket.
 * Modified to be able to set the domain name as well.
 */

static void
dname_conf(const char *config_file, const char *out_file,
	   char *dname, size_t size_dname)
{
	FILE	*fp, *ofp;
	char	*cp;
	uint_t	dotchars;
	char	buf[BUFSIZ];
	int	found = 0;

	fp = fopen(config_file, "r");
	if (out_file) {
		if ((ofp = fopen(out_file, "w")) == NULL) {
			pfmt(stderr, MM_ERROR,
				":148:Cannot create %s: %s\n",
				out_file, strerror(errno));
			exit(1);
		}
	} else
		dname[0] = '\0';

	/*
	 * Read the config file, looking for a line beginning with "domain".
	 */
	while (fp && fgets(buf, BUFSIZ, fp) != 0) 
	{
		if (!found &&
		    strncmp(buf, "domain", sizeof("domain") - 1) == 0) 
		{
			cp = buf + sizeof("domain") - 1;
			while (*cp == ' ' || *cp == '\t') {
				found = 1;
				cp++;
			}
			if (found) {
				if (!out_file) {
					/* buf always null-terminated */
					memccpy(dname, cp, 0, size_dname);
					if ((cp = strchr(dname, '\n')) != 0)
						*cp = '\0';
					break;
				}
				if (dname[0] == '\0')
					continue;
				strcat(strcpy(cp, dname), "\n");
			}
		}
		if (out_file)
			fputs(buf, ofp);
	}
	if (fp)
		fclose(fp);

	if (out_file) {
		if (!found && dname[0] != '\0')
			fprintf(ofp, "domain %s\n", dname);
		if (ferror(ofp)) {
			pfmt(stderr, MM_ERROR,
				":333:Write error in %s: %s\n",
				out_file, strerror(errno));
			unlink(out_file);
			exit(1);
		}
		fclose(ofp);
	} else {
		/* Since this is an administrator edited file, there
		 * may be a trailing "." for the root domain.
		 * Delete trailing '.'.
		 */
		dotchars = strlen(dname);
		while (dotchars && dname[--dotchars] == '.') 
			dname[dotchars] = 0;
	}
}

int
set_hostname(char *hostname, int permanent)
{
	char nodename[SYS_NMLN];
	char domain[SYS_NMLN];
	char fullname[SYS_NMLN];
	sigset_t allsigs, prevmask;
	FILE *fp;
	char *p;

	/*
	 * The size of the node name must be less than SYS_NMLN.
	 */
	if (strlen(hostname) > SYS_NMLN - 1) {
too_long:
		pfmt(stderr, MM_ERROR,
			":730:Name must be <= %d letters\n", SYS_NMLN-1);
		return 1;
	}

	/*
	 * Check for valid characters.
	 */
	for (p = hostname; *p; p++) {
		if (!isalnum(*p) && !strchr("-_.", *p)) {
			pfmt(stderr, MM_ERROR,
				":1460:Illegal character in host name\n");
			return 1;
		}
	}
	if (*hostname == '\0' || *hostname == '.') {
		pfmt(stderr, MM_ERROR, ":1461:Node name must be non-blank\n");
		return 1;
	}

	/*
	 * Caller may set just the node, or the full host name including domain.
	 */
	strcpy(nodename, hostname);
	if ((p = strchr(nodename, '.')) != NULL) {
		*p++ = '\0';
		strcpy(domain, p);
		strcpy(fullname, hostname);
	} else {
		/*
		 * Try to get domain from DOMAIN_FILE. If that doesn't work,
		 * try the kernel.
		 */
		dname_conf(DOMAIN_FILE, NULL, domain, sizeof domain);
		if (domain[0] == '\0' &&
		    sysinfo(SI_HOSTNAME, fullname, sizeof fullname) != -1 &&
		    (p = strchr(fullname, '.')) != NULL) {
			strcpy(domain, p + 1);
		}
		if (domain[0] != '\0') {
			if (snprintf(fullname, sizeof fullname, "%s.%s",
				     nodename, domain) < 0)
				goto too_long;
		} else
			strcpy(fullname, nodename);
	}

	/*
	 * Change the hostname in the running kernel.
	 */
	if (sysinfo(SI_SET_HOSTNAME, fullname, 0) == -1) {
		/*
		 * Try just the node name; older systems don't support
		 * setting the fully-qualified hostname.
		 */
		if (sysinfo(__O_SI_SET_HOSTNAME, nodename, 0) == -1) {
			pfmt(stderr, MM_ERROR,
			     ":1462:Can't set hostname: %s\n",
			     strerror(errno));
			return 1;
		}
	}

	if (!permanent)
		return 0;

	/*
	 * The node name (first part of full host name) is stored in
	 * NODE_FILE for use when booting, to allow permanent settings
	 * to persist across reboots. The domain name is stored in DOMAIN_FILE.
	 * Create the new versions of these files first, and then move them
	 * both in place together, to reduce windows of inconsistency if we
	 * die partway through.
	 */

	umask(~(S_IRWXU|S_IRGRP|S_IROTH) & S_IAMB);

	dname_conf(DOMAIN_FILE, TMP_DOMAIN_FILE, domain, sizeof domain);

	if ((fp = fopen(TMP_NODE_FILE, "w")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":148:Cannot create %s: %s\n",
			TMP_NODE_FILE, strerror(errno));
		unlink(TMP_DOMAIN_FILE);
		return 1;
	}

	if (fprintf(fp, "%s\n", nodename) < 0) {
		pfmt(stderr, MM_ERROR,
			":333:Write error in %s: %s\n",
			TMP_NODE_FILE, strerror(errno));
		unlink(TMP_DOMAIN_FILE);
		unlink(TMP_NODE_FILE);
		return 1;
	}
	fclose(fp);

	/* Give us a reasonable chance to complete without interruptions */
	sigfillset(&allsigs);
	sigprocmask(SIG_BLOCK, &allsigs, &prevmask);

	rename(TMP_NODE_FILE, NODE_FILE);
	rename(TMP_DOMAIN_FILE, DOMAIN_FILE);

	sigprocmask(SIG_SETMASK, &prevmask, NULL);

	/*
	 * If there's a kernel tunable called NODE and it has an old value,
	 * re-tune it and make sure the kernel gets rebuilt. (This is needed
	 * only for the older OpenServer systems.)
	 */
	if (strcmp(get_param(__O_SI_SYSNAME), "SCO_SV") != 0)
		return 0;

	fp = popen("echo y|ROOT=/. /etc/conf/bin/idtune NODE"
		   " \\\"`cat /etc/nodename`\\\" 2>/dev/null", "r");
	if (fp != NULL) {
		int c = getc(fp);

		if (pclose(fp) == 0 && c != EOF) {
			/*
			 * If we get here, the parameter, NODE, exists, and
			 * we successfully changed it to a different value
			 * than it had before. This works because idtune
			 * prompts for confirmation iff the value doesn't
			 * match the current value, and doesn't output
			 * anything to stdout otherwise.
			 */
			(void)system("ROOT=/. /etc/conf/bin/idbuild");
		}
	}

	return 0;
}
