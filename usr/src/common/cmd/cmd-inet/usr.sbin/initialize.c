#ident	"@(#)initialize.c	1.5"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Inet administration consolidation, replaces Unixware 1.1
 * /etc/inet/rc.inet and /etc/confnet.d/inet/config.boot.sh
 * shell script functionality.
 *
 * /etc/inet/rc.inet had various binaries, such as /usr/sbin/in.pppd,
 * run when their configuration files were present.
 *
 * /etc/confnet.d/inet/config.boot.sh linked interfaces (such as
 * Ethernet(TM), ppp, slip, and localhost) under /dev/ip using slink
 * and configured the interfaces with ifconfig.
 *
 * This binary will replace the rc.inet functionality by reading the
 * /etc/inet/config file and conditionally executing the listed
 * binaries, and reading /etc/confnet.d/inet/interface to run slink
 * and ifconfig.  In addition it will add/modify these configuration
 * files entries.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <search.h>
#include <errno.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <locale.h>
#include <pfmt.h>
#include "route_util.h"
extern char *gettxt();
extern char *strerror();

/* produce a sorted list of lines to process them
 * in the correct order.
 * return 0 on success.
 */
int
sort_file(head, compr)
ConfigFile_t	*head;
int		(*compr)();
{
	ConfigLine_t	*cur_line,	/* current element of each */
			*sorted;	/* current list of sorted lines */
	int		i;

	/* allocate array for sorted lines */
	sorted	= head->cf_sortedLines
		= (ConfigLine_t *) calloc(head->cf_numLines+1, sizeof(ConfigLine_t));
	if (!sorted)
		return 1;

	/* fill the array with pointers to the line structures */
	for (cur_line = head->cf_lines, i=0;
	     ((cur_line) && (i<head->cf_numLines));
	     cur_line = cur_line->cl_next, i++) {
		*(i+sorted) = *cur_line;
	}

/*SORT*/
	/* sort the lines by the compare function */
	head->cf_compareFcn = compr;
	qsort((char *)(head->cf_sortedLines), head->cf_numLines, sizeof(ConfigLine_t), compr);

	return 0;
}


/* shrink_string(s) removes leading and trailing white space from s.
 * If the given pointer is null, it returns a pointer to a static string
 * that contains only a string terminator.
 */

#define	SHRINK_STRING(a)	a = shrink_string(a);
/* XXX library version of this should not encourage string editing as
 * this function does, pure form should encourage dups firs.
 */
char *
shrink_string(char * s)
{
	static char	null_string = '\0';
	char	*e;
	int	len;

	/* process null strings */
	if (!s)
		return &null_string;
	if (!*s)
		return s;

	/* skip past spaces and tabs within s */
	while (*s && ((' ' == *s) || ('\t' == *s)))
		s++;

	/* replace white space at end of s with null strings */
	len = strlen(s) - 1;
	e = s + len;
	/* since len can be -1, can't just use 'len' as condition */
	while ((len > 0) && ((' ' == *e) || ('\t' == *e))) {
		*e = 0;
		len--;
		e--;
	}

	/* process null strings */
	if (!s) {
		return (&null_string);
	}
	return (s);
}

/* Vers_1_proc_interface will configure one interface from the
 * configuration file /etc/confnet.d/inet/interface
 * file format is	prefix:unit#:addr:device:ifconfig opts:slink opts:
 *
 * return codes:
 *	0	no warnings or errors in file, zero return from executables
 *	1	warning in fileentry validation
 *	2	error in fileentry validation
 *	3	non-zero return from executable
 *	4	unable to get system name
 *	5	unable to malloc for system name
 */
int
Vers_1_proc_interface(iface, valid_iface, stack_up, line, logfilehandle, logfilename)
char	*iface;			/* optional interface */
int	*valid_iface;		/* interface valid */
int	stack_up;		/* wheather to configure the stack up or down */
ConfigLine_t *line;
FILE	*logfilehandle;		/* FILE handle for verbose log */
char	*logfilename;		/* name of log file */
{
	struct utsname	uts;	/* null address gets `uname -n` */
	struct stat	provider_device_stat;
	char		*cmdline, *cmdfmt;
	int		cmdlen, cmdret;
	InterfaceToken_t *params;
	char
	*prefix,	/* identifier for driver's netstat stats */
	*unit,		/* device ifstats index [per prefix array] */
	*address,	/* address used by ifconfig to initialize
			 * this transport provider.
			 * may be the internet name or number.
			 * if a name, must be in /etc/inet/hosts since 
			 * DNS/NIS may only be used after init.
			 * Null is expanded to `/usr/bin/uname -n`
			 */
	*device,	/* full path name to the /dev device.
			 * Obtained via /etc/confnet.d/netdrivers
			 * by running generic /etc/confnet.d/configure
			 * with -i option
			 */
	*ifconfig_opts,	/* ifconfig options such as:
			 * netmask 0xffffff00 -trailers 
			 * May be null for providers like lo0.
			 */
	*slink_opts;	/* slink options such as:
			 * add_interface
			 * or
			 * add_loop
			 * or
			 * slink_func ip device prefixunit
			 * or null, defaults to add_interface
			 */
	/* The slink_opts value is used by slink to initialize the device
	 * into the TCP/IP protocol stack.
	 * slink_opts selects the strcf function from /etc/inet/strcf and
	 * any initial arguments.
	 * add_interface is the default slink_opts value; it is used with
	 * Ethernet-style devices.
	 * Additional arguments will be appended to slink_opt to make the final
	 * form of the slink operation:
	 * 	slink_opts ip device prefixunit#
	 * Where ip will be an open file descriptor to /dev/ip and device prefix
	 * and unit are defined in the current interface entry.
	 * For a standard Ethernet board, slink_opts may be null; the defaults
	 * will take care of all arguments.
	 */

	/* ignore comment type stuff */
	if (DataLine != line->cl_kind) {
		return 0;
	}

	params		= line->cl_tokens;
	prefix		= params->it_prefix;
	unit		= params->it_unit;

	if (iface) {			/* only change specified interface */
		size_t slen = strlen(prefix);

		if (strlen(iface) < slen)
			return 0;

		if (memcmp(prefix, iface, slen) != 0)
			return 0;

		for (; slen; slen--)
			iface++;

		if (*iface == '_')	/* skip optional ODI underscore */
			iface++;

		if (strcmp(iface, unit) != 0)
		    return 0;

		(*valid_iface)++;
	}

	address		= params->it_addr;
	device		= params->it_device;
	ifconfig_opts	= params->it_ifconfigOptions;
	slink_opts	= params->it_slinkOptions;

	/* mark utsname as unused */
	uts.nodename[0] = 0;

	/* validate prefix does not contain white space */
	SHRINK_STRING(prefix);
	if (!*prefix || strpbrk(prefix, " \t")) {
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":119", "prefix is null or has white space.\n ignored:"),
			line->cl_origLine);
		return 2;
	}

	/* validate unit, numerical only and contains no spaces */
	SHRINK_STRING(unit);
	if (!*unit || strpbrk(unit, " \t") ||
	    (strlen(unit) != strspn(unit, "0123456789"))) {
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":122", "unit is null or contains non-numeric or white space.\n ignored:"),
			line->cl_origLine);
		return 2;
	}

	/* NOTE: ifconfig will do its own validation on the address entry */

	/* verify device entry is a character special device */
	SHRINK_STRING(device);
	if (!*device || stat(device, &provider_device_stat) ||
	    ((provider_device_stat.st_mode & S_IFMT) != S_IFCHR)) {
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":116", "The device entry is not a character device.\n ignored:"),
			line->cl_origLine);
		return 2;
	}

	/* If the slink_opts string is null, use "add_interface"
	 */
	SHRINK_STRING(slink_opts);
	if (!*slink_opts) {
		slink_opts = "add_interface";
	}

	/* there is no further verification processing of ifconfig_opts or
	 * slink_opts, the ifconfig and slink commands will verify them.
	 */

	/* If the address is null, use `uname -n` for the address.
	 */
	SHRINK_STRING(address);
	if (!*address) {
		if ((0 > uname(&uts)) ||
		    (!uts.nodename[0])) {
			fprintf(logfilehandle, "%s %s\n",
				gettxt(":121", "uname() failed, or null nodename.\n ignored:"),
				line->cl_origLine);
			return 4;
		}
		address = malloc(strlen(&uts.nodename[0])+1);
		if (!address) {
			fprintf(logfilehandle, "%s %s\n",
				gettxt(":120", "unable to get malloc() for nodename.\n ignored:"),
				line->cl_origLine);
			return 5;
		}
		strcpy(address, &uts.nodename[0]);
	}

	/* assemble the command line, write it to the log file, run cmd,
	 * and report error code if it is not zero return.
	 *
	 * intuitively we may have wanted to run the slink first,
	 * check the return, and run slink -u to unlink the result
	 * in the case of an error.  Since there is one slink done
	 * per provider, and it should only succeed when the link
	 * is complete, then we need not slink -u since it realy never
	 * completed.
	 *
	 * use slink ... && ifconfig ... to conditionally run ifconfig
	 * when slink returns zero.
	 */
/* XXX there has to be a better way */

	if (stack_up) {
#ifdef DEBUG
	cmdfmt = "echo debug /usr/sbin/slink -vc /etc/inet/strcf\\\n\
 %s %s %s %s%s >>%s 2>&1 &&\n\
\
\
echo debug \
\
\
/usr/sbin/ifconfig %s%s %s %s up >>%s 2>&1\n";
#else
	cmdfmt = "/usr/sbin/slink -vc /etc/inet/strcf\\\n\
 %s %s %s %s%s >>%s 2>&1 &&\n\
/usr/sbin/ifconfig %s%s %s %s up >>%s 2>&1\n";
#endif
	} else {
#ifdef DEBUG
	cmdfmt = "echo debug /usr/sbin/slink -uvc /etc/inet/strcf\\\n\
 %s %s %s %s%s >>%s 2>&1\n";
#else
	cmdfmt = "/usr/sbin/slink -uvc /etc/inet/strcf\\\n\
 %s %s %s %s%s >>%s 2>&1\n";
#endif
	}
	cmdlen=	strlen(cmdfmt) + strlen(slink_opts)
		+ strlen("reserved_for_ip") + strlen(device)
		+ strlen(prefix) + strlen(unit) + strlen(logfilename)
		+ strlen(prefix) + strlen(unit) + strlen(address)
		+ strlen(ifconfig_opts) + strlen(logfilename);

	cmdline = malloc(cmdlen);
	if (!cmdline) {
		if (uts.nodename[0]) {
			free(address);
		}
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":113", "unable to get malloc() for commnd line.\n ignored:"),
				line->cl_origLine);
			return 5;
	}

	sprintf(cmdline, cmdfmt, slink_opts, "reserved_for_ip",
		   device, prefix, unit, logfilename, prefix, unit,
		   address, ifconfig_opts, logfilename);
	fprintf(logfilehandle, "\n%s %s\n", gettxt(":123", "Running command: "), cmdline);
	
/* XXX address is not part of the string if everything sprintf'd,
 *so we can free address if used */
	/* free malloc'd address if nodename is used */
	if (uts.nodename[0]) {
		free(address);
	}
	line->cl_cmdStarted = 1;
	cmdret = system(cmdline);
	free(cmdline);
#ifdef DEBUG
	cmdlen = WIFEXITED(cmdret);
#endif
	if (!WIFEXITED(cmdret) || (WIFEXITED(cmdret) && WEXITSTATUS(cmdret))) {
		if (WIFEXITED(cmdret)) {
			line->cl_cmdReturned = WEXITSTATUS(cmdret);
			fprintf(logfilehandle,
				gettxt(":111", "Command returned non-zero (%d)\n"), cmdret);
		} else {
			line->cl_cmdReturned = WTERMSIG(cmdret);
			fprintf(logfilehandle,
				gettxt(":112", "Command teminated by signal (%d)\n"), cmdret);
		}
		return 3;
	}
	return 0;
}

/* vers_1_proc_config_cmds will run a binary as described by the
 * configuration file /etc/inet/config
 *
 * return codes:
 *	0	no warnings or errors in file,
 *		zero return from executable,
 *		turned off with N or missing required config file
 *	1	warning in fileentry validation
 *	2	error in fileentry validation
 *	3	non-zero return from executable
 *	4	malloc failed
 */

int
vers_1_proc_config_cmds(line, logfilehandle, logfilename, all_lines, count)
ConfigLine_t *line, *all_lines;
FILE	*logfilehandle;		/* FILE handle for verbose log */
char	*logfilename;		/* name of log file */
uint_t	count;			/* number of entries in all_lines */
{
	ConfigToken_t *params;
	char	*cmdline, *cmdfmt;
	int	cmdlen, cmdret;
	char	*executable,	/* name of the executable */
		*activate,	/* Y/y to run, N/n to not run */
		*config_file,	/* manditory configuration file or
				 * null.  If non-null, file must
				 * exist to run cmd */
		*overide,	/* If this cmd has run, do not run
				 * the cmd listed */
		*options_list;	/* options for the executable */

	/* ignore comment type stuff */
	if (DataLine != line->cl_kind) {
		return 0;
	}

	params		= line->cl_tokens;
	executable	= params->ct_fullCmdPath;
	activate	= params->ct_isRunning;
	config_file	= params->ct_configFile;
	overide		= params->ct_overridingCmdPath;
	options_list	= params->ct_cmdOptions;

	/* if activate N or n, ignore */
	SHRINK_STRING(activate);
	if (('N' == *activate) || ('n' == *activate)) {
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":117", "Turned off, ignored:"), line->cl_origLine);
		return 0;
	}

	/* If there is a manditory configuration file listed, and
	 * it is not readble, or is not a full path name,
	 * then ignore this entry.
	 * If a manditory configuration file is not listed, continue.
	 */
	SHRINK_STRING(config_file);
	if (*config_file) {
		if (('/' != *config_file) || access(config_file, R_OK)) {
			fprintf(logfilehandle, "%s %s\n",
				gettxt(":114", "Listed configuration file is not a found full path name.\n ignored:"),
				line->cl_origLine);
			return 0;
		}
	}

	/* If the overide cmd has run, do not run the cmd listed.
	 * Otherwise, continue.
	 */
	SHRINK_STRING(overide);
	if (*overide) {
	  for (; (count>0); count--, all_lines++) {
	    if ((DataLine == line->cl_kind) &&
		!strcmp(((ConfigToken_t *)all_lines->cl_tokens)->ct_fullCmdPath, overide)) {
		/* found the matching entry */

		/* was not run */
		if (!all_lines->cl_cmdStarted) {
			fprintf(logfilehandle, "%s %s\n",
				gettxt(":125", "overiding cmd was not run, continuing to execute cmd"),
				line->cl_origLine);
			break;
		}

		/* ran with fatal error */
		if (all_lines->cl_cmdReturned) {
			fprintf(logfilehandle, "%s %s\n",
				gettxt("126", "overiding cmd had fatal error, continuing to execute cmd"),
				line->cl_origLine);
			break;
		}

		/* ran fine. overide and do not run current line */
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":127", "overiding cmd ran successfully, no need to run this cmd"),
			line->cl_origLine);
		return 0;
	    }
	  }
	}

	/* if activate is not Y or y, ignore */
	if (('Y' != *activate) && ('y' != *activate)) {
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":115", "No Y or y, ignored:"), line->cl_origLine);
		return 1;
	}

	/* if manditory executable is not available, or a full
	 * path name, ignore
	 */
	SHRINK_STRING(executable);
	if (('/' != *executable) || access(executable, EX_OK | F_OK)) {
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":118", "cmd is not found or a full path name.\n ignored:"),
			line->cl_origLine);
		return 2;
	}

	/* assemble the command line, write it to the log file, run cmd,
	 * and report error code if it is not zero return.
	 */
#ifdef DEBUG
	cmdfmt = "echo debug %s %s >>/dev/console 2>&1";
#else
	cmdfmt = "%s %s >>/dev/console 2>&1";
#endif
	cmdlen=	strlen(cmdfmt) + strlen(executable) + strlen(options_list);

	cmdline = malloc(cmdlen);
	if (!cmdline) {
		fprintf(logfilehandle, "%s %s\n",
			gettxt(":113", "unable to get malloc() for commnd line.\n ignored:"),
				line->cl_origLine);
			return 5;
	}

	sprintf(cmdline, cmdfmt, executable, options_list);
	fprintf(logfilehandle, "\n%s %s\n", gettxt(":123", "Running command: "), cmdline);
	line->cl_cmdStarted = 1;
	cmdret = system(cmdline);
	free(cmdline);
	if (!WIFEXITED(cmdret) || (WIFEXITED(cmdret) && WEXITSTATUS(cmdret))) {
		if (WIFEXITED(cmdret)) {
			line->cl_cmdReturned = WEXITSTATUS(cmdret);
			fprintf(logfilehandle,
				gettxt(":111", "Command returned non-zero (%d)\n"), cmdret);
			return 3;
		} else {
			line->cl_cmdReturned = WTERMSIG(cmdret);
			fprintf(logfilehandle,
				gettxt(":112", "Command teminated by signal (%d)\n"), cmdret);
			return 3;
		}
	}
	return 0;
}



/* compare two interface file lines.
 *
 * when first argument is less than, equal to, or greater than the second arg,
 * return -1, 0, +1, respectively.
 *
 * comments are located at the top of the file, so let them be first/least.
 * localhost is first, as that is arbitrarily fast.
 * ethernet cards should be next since they are our fastest device types
 * in this release.
 * ppp is encouraged more than slip.
 *
 * unknown types are arbitrarily weighed faster than ether since we
 * know fddi is out there.
 *
 * order is:   comments, localhost, unk, ether, ppp, slip
 * values are: 0,        1,         3,   5,     7,   9
 */
#define LOCAL_VAL	1
#define UNKNOWN_VAL	3
#define ETHERNET_VAL	5
#define PPP_VAL		7
#define SLIP_VAL	9

value_provider(line)
ConfigLine_t *line;
{
	char	*address,	/* address used by ifconfig to initialize */
		*device,	/* full path name to the /dev device. */
		*ifconfig_opts,	/* ifconfig options such as: -trailers */
		*slink_opts;	/* slink options such as: add_interface */
	InterfaceToken_t *params;

	params		= line->cl_tokens;
	address		= params->it_addr;
	device		= params->it_device;
	ifconfig_opts	= params->it_ifconfigOptions;
	slink_opts	= params->it_slinkOptions;

	/* we skip leading spaces and tabs with strspn offsets,
	 * and ignore trailing ones by using strncmp */

	if (!strncmp("/dev/loop", device+strspn(device, " \t"), 9))
		return LOCAL_VAL;

	if (!strncmp("/dev/ppp", device+strspn(device, " \t"), 8))
		return PPP_VAL;

	if (!*slink_opts ||
	    !strncmp("add_interface_SNAP",
		slink_opts+strspn(slink_opts, " \t"), 18) ||
	    !strncmp("add_interface",
		slink_opts+strspn(slink_opts, " \t"), 13))
		return ETHERNET_VAL;

/* XXX is slip in interface? */
	return UNKNOWN_VAL;
}

int
compare_providers(x, y)
ConfigLine_t *x, *y;
{
	ConfigLine_t *a = x, *b =y;

	if (!x || !y) {
		return 0;
	}
	/* compares are optimized for DataLine equal to zero */
	if (a->cl_kind) {
		/* line a is a comment, blank or version */
		if (b->cl_kind) {
			/* line a is a comment, blank or version */
			/* line b is a comment, blank or version */
			return 0;
		} else {
			/* line a is a comment, blank or version */
			/* line b is data */
			return -1;
		}
	} else {
		/* line a is data */
		if (b->cl_kind) {
			/* line a is data */
			/* line b is a comment, blank or version */
			return 1;
		} else {
			int delta = value_provider(a) - value_provider(b);
			/* line a is data */
			/* line b is data */
			if (delta)
				return(delta);
			return (a->cl_lineNumber - b->cl_lineNumber);
		}
	}
}

int
compare_config(x, y)
ConfigLine_t *x, *y;
{
	ConfigLine_t *a = x, *b =y;
	/* compares are optimized for DataLine equal to zero */
	if (a->cl_kind) {
		/* line a is a comment, blank or version */
		if (b->cl_kind) {
			/* line a is a comment, blank or version */
			/* line b is a comment, blank or version */
			return 0;
		} else {
			/* line a is a comment, blank or version */
			/* line b is data */
			return -1;
		}
	} else {
		/* line a is data */
		if (b->cl_kind) {
			/* line a is data */
			/* line b is a comment, blank or version */
			return 1;
		} else {
			/* line a is data */
			/* line b is data */
			return strcmp(
			  ((ConfigToken_t *)a->cl_tokens)->ct_level,
			  ((ConfigToken_t *)b->cl_tokens)->ct_level);
		}
	}
}


int
main(argc, argv)
int	argc;
char	*argv[];
{
	ConfigFile_t	conf, interface;
	int		l, c, retcode=0;
	ConfigLine_t	*line;
	FILE *logfile;
	extern char *optarg;
	extern int optind;
	int	up_flag = 0, down_flag = 0, usage = 0;
	int	restart_flag = 0;
	char	*iface = NULL;

	(void)setlocale(LC_ALL,"");
	(void)setcat("inet.pkg");
	(void)setlabel("UX:initialize");
	logfile = fopen(INET_LOG_FILE, "a");
/* XXX what if this failed?*/
	setbuf(stderr, NULL);
	setbuf(stdout, NULL);
	setbuf(logfile, NULL);

	while ((c = getopt(argc, argv, "Uu:Dd:")) != -1)
	  switch (c) {
		case 'u':		/* specified interface UP */
		    if (strcmp("all", optarg) != 0)
			iface = optarg;
		    up_flag++;
		    if (down_flag)
			usage++;
		    break;
		case 'U':		/* all interfaces UP at boot time */
		    restart_flag++;	/* also rattle through inet/config */
		    up_flag++;
		    if (down_flag)
			usage++;
		    break;
		case 'd':		/* specified interface DOWN */
		    iface = optarg;
					/*FALLTHRU*/
		case 'D':		/* all interfaces DOWN at shutdown */
		    down_flag++;
		    if (up_flag)
			usage++;
		    break;
		case '?':
		    usage++;
	  }

	if (optind != argc || !up_flag && !down_flag)
		usage++;

	if (usage) {
		pfmt(stderr, MM_ACTION, ":109:Usage:\n\tinitialize -U | -D\n\tinitialize -u all|<interface> | -d <interface>\n");
		exit(1);
	}

	/* these cases require access to the interface file */
	if (up_flag || down_flag) {
		interface.cf_kind = InterfaceFile;
		interface.cf_minFields = IF_FIELDS;
		errno=0;
		if (readConfigFile(&interface)) {
			pfmt(stderr, MM_ERROR, ":110:Fatal error reading %s: %s\n",
				interface.cf_fileName, strerror(errno));
			exit(2);
		}
		if (sort_file(&interface, compare_providers)) {
			pfmt(stderr, MM_ERROR, ":111:Fatal error processing %s: %s\n",
				interface.cf_fileName, strerror(errno));
			exit(3);
		}
	}

	/* bring interfaces up or down */
	if (up_flag || down_flag) {
		int valid_iface = 0;
		for (line=interface.cf_sortedLines, l=0;
		     (l < interface.cf_numLines);
		     line++, l++) {
			retcode != Vers_1_proc_interface(iface, &valid_iface,
					up_flag, line, logfile,
					INET_LOG_FILE);
		}
		if (iface && !valid_iface) {
			pfmt(stderr, MM_ERROR, ":129:No such interface: %s\n",
				iface);
			exit(2);
		}
	}

	/* these cases require access to the config file */
	if (up_flag && restart_flag) {
		conf.cf_kind		= ConfigFile;
		conf.cf_minFields	= CF_FIELDS;
		errno=0;
		if (readConfigFile(&conf)) {
			pfmt(stderr, MM_ERROR, ":110:Fatal error reading %s: %s\n",
				conf.cf_fileName, strerror(errno));
			exit(2);
		}
		if (sort_file(&conf, compare_config)) {
			pfmt(stderr, MM_ERROR, ":111:Fatal error processing %s: %s\n",
				conf.cf_fileName, strerror(errno));
			exit(3);
		}
	}

	/* initialization commands */
	if (up_flag && restart_flag) {
		for (line=conf.cf_sortedLines, l=0;
		     (l < conf.cf_numLines);
		     line++, l++) {
			retcode |= vers_1_proc_config_cmds(line,
						logfile, INET_LOG_FILE,
						conf.cf_sortedLines,
						conf.cf_numLines);
		}
	}
	exit(retcode);
}
