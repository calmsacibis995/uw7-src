#ident	"@(#)uname:common/cmd/uname/uname.c	1.29.2.14"
#ident	"$Header$"

/***************************************************************************
 * Command: uname
 * Inheritable Privileges: P_SYSOPS,P_DACREAD,P_DACWRITE
 *       Fixed Privileges: None
 ***************************************************************************/

/*
 * This version of the uname command works in four different environments:
 *	+ native UW2
 *	+ UW2 with UDK runtime
 *	+ OSR5 with UDK runtime
 *	+ native SVR5
 * Some of the new functionality will only work in newer environments.
 *
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
#include <errno.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/syscall.h>
#include <string.h>

#define _SCO_INFO 12840
#define __scoinfo(buf, bufsize) syscall(_SCO_INFO, buf, bufsize)

char *prog_name;
enum prog { UNAME, SETUNAME } prog;

/* Parameter overrides if non-NULL */
char *override_release;
char *override_version;
char *override_sysname;
char *override_Xrelease;

/* Mapping between named parameters and sysinfo parameter numbers */
struct param {
	char *p_name;
	enum ptype { SYSINFO, SYSCONF, BUSTYPES } p_type;
	int p_cmd;
} params[] = {
	{ "architecture", SYSINFO, SI_ARCHITECTURE },
	{ "bus_types", BUSTYPES, 0 },
	{ "hostname", SYSINFO, SI_HOSTNAME },
	{ "hw_provider", SYSINFO, SI_HW_PROVIDER },
	{ "hw_serial", SYSINFO, SI_HW_SERIAL },
	{ "kernel_stamp", SYSINFO, SI_KERNEL_STAMP },
	{ "machine", SYSINFO, SI_MACHINE },
	{ "num_cg", SYSCONF, _SC_NCGS_CONF },
	{ "num_cpu", SYSCONF, _SC_NPROCESSORS_CONF },
	{ "os_base", SYSINFO, SI_OS_BASE },
	{ "os_provider", SYSINFO, SI_OS_PROVIDER },
	{ "release", SYSINFO, SI_RELEASE },
	{ "srpc_domain", SYSINFO, SI_SRPC_DOMAIN },
	{ "sysname", SYSINFO, SI_SYSNAME },
	{ "user_limit", SYSINFO, SI_USER_LIMIT },
	{ "version", SYSINFO, SI_VERSION },
	{ 0 }
};

int uname_main(int argc, char **argv);

const char uname_usage[] = ":1463:Usage:\n"
				"\tuname parameter_name\n"
				"\tuname -f\n"
				"\tuname [-acdilmnprsvAX]\n"
				"\tuname [-S system name]\n";

void usage(int getopt_err);
void check_overrides(void);
char *_get_param(int si_param, int fail_ok);
#define get_param(si_param)	_get_param(si_param, 0)
void output_param(int si_param);
void output_sysname(void);
int output_bustypes(void);
int output_selected_param(struct param *pp);
void output_named_param(const char *name);
void output_all(void);

extern int setuname_main(int argc, char **argv);
extern int set_hostname(char *hostname, int permanent);

extern const char setuname_usage[];


int
main(int argc, char **argv)
{
	if ((prog_name = strrchr(argv[0], '/')) != NULL)
		++prog_name;
	else
		prog_name = argv[0];

	setlocale(LC_ALL, "");
	setcat("uxcore.abi");

	if (strcmp(prog_name, "setuname") == 0) {
		prog = SETUNAME;
		setlabel("UX:setuname");
		return setuname_main(argc, argv);
	} else {
		prog = UNAME;
		setlabel("UX:uname");
		return uname_main(argc, argv);
	}
}

void
usage(int getopt_err)
{
	const char *usage_msg;

	if (!getopt_err)
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");

	switch (prog) {
	case UNAME:
		usage_msg = uname_usage;
		break;
	case SETUNAME:
		usage_msg = setuname_usage;
		break;
	}
	pfmt(stderr, MM_ACTION, usage_msg);

	exit(1);
}


/* uname option flags */
static const int opt_a = 1 << 0;
static const int opt_c = 1 << 1;
static const int opt_d = 1 << 2;
static const int opt_f = 1 << 3;
static const int opt_i = 1 << 4;
static const int opt_l = 1 << 5;
static const int opt_m = 1 << 6;
static const int opt_n = 1 << 7;
static const int opt_p = 1 << 8;
static const int opt_r = 1 << 9;
static const int opt_s = 1 << 10;
static const int opt_v = 1 << 11;
static const int opt_A = 1 << 12;
static const int opt_S = 1 << 13;
static const int opt_X = 1 << 14;

static int dflt_opt;

#define DELIMIT(c)	if (!first) putchar(c); else first = 0

int
uname_main(int argc, char **argv)
{
	int opts = 0;
	int optlet;
	int first = 1;
	char *nodename;

	/* "uname -s" is the default (unless overridden) */
	dflt_opt = opt_s;

	/* Check for compatibility overrides. */
	check_overrides();

	while ((optlet = getopt(argc, argv, "acdfilmnprsvAS:X")) != EOF)
		switch (optlet) {
		case 'a':
			opts |= opt_a;
			break;
		case 'c':
			opts |= opt_c;
			break;
		case 'd':
			opts |= opt_d;
			break;
		case 'f':
			opts |= opt_f;
			break;
		case 'i':
			opts |= opt_i;
			break;
		case 'l':
			opts |= opt_l;
			break;
		case 'm':
			opts |= opt_m;
			break;
		case 'n':
			opts |= opt_n;
			break;
		case 'p':
			opts |= opt_p;
			break;
		case 'r':
			opts |= opt_r;
			break;
		case 's':
			opts |= opt_s;
			break;
		case 'v':
			opts |= opt_v;
			break;
		case 'A':
			opts |= opt_A;
			break;
		case 'S':
			if (opts & opt_S) {
				/* Only one -S allowed. */
				usage(0);
			}
			opts |= opt_S;
			nodename = optarg;
			break;
		case 'X':
			opts |= opt_X;
			break;
		case '?':
			usage(1);
		}

	/* Check for named parameter case. */
	if (argc - optind == 1) {
		if (opts) {
			/* No options allowed with parameters. */
			usage(0);
		}
		output_named_param(argv[optind++]);
		putchar('\n');
		return 0;
	}

	if (optind != argc)
		usage(0);

	/* Check for -f, to output all named parameters. */
	if (opts & opt_f) {
		/* -f must be used by itself. */
		if (opts & ~opt_f)
			usage(0);

		output_all();
		return 0;
	}

	/* Check for -S, to set host name. */
	if (opts & opt_S) {
		/* -S must be used by itself. */
		if (opts & ~opt_S)
			usage(0);

		return set_hostname(nodename, 1);
	}

	/* If no options set, use default. */
	if (opts == 0)
		opts = dflt_opt;

	if (opts & (opt_s | opt_a)) {
		/* sysname has several special cases; handle it separately. */
		output_sysname();
		first = 0;
	}

	if (opts & (opt_n | opt_a)) {
		DELIMIT(' ');
		output_param(__O_SI_HOSTNAME);
	}

	if (opts & (opt_r | opt_a)) {
		DELIMIT(' ');
		if (override_release)
			fputs(override_release, stdout);
		else
			output_param(SI_RELEASE);
	}

	if (opts & (opt_v | opt_a)) {
		DELIMIT(' ');
		if (override_version)
			fputs(override_version, stdout);
		else if (strcmp(get_param(__O_SI_SYSNAME), "SCO_SV") == 0)
			putchar('2');
		else
			output_param(SI_VERSION);
	}

	if (opts & (opt_m | opt_a)) {
		DELIMIT(' ');
		output_param(__O_SI_MACHINE);
	}

	if (opts & (opt_p | opt_a)) {
		DELIMIT(' ');
		output_param(__O_SI_ARCHITECTURE);
	}

	if (opts & opt_a) {
		char *s;

		/*
		 * The following parameters are new, so they may fail on an old
		 * system, but don't fail the -a option, since it's not new.
		 */
		if ((s = _get_param(SI_OS_PROVIDER, 1)) != NULL)
			printf(" %s", s);	/* (we know we're not first) */
		if ((s = _get_param(SI_OS_BASE, 1)) != NULL)
			printf(" %s", s);	/* (we know we're not first) */
	}

	if (opts & opt_c) {
		DELIMIT(' ');
		output_param(SI_HW_PROVIDER);
	}

	if (opts & opt_i) {
		DELIMIT(' ');
		output_param(SI_HW_SERIAL);
	}

	if (opts & (opt_A | opt_l)) {
		DELIMIT(' ');
		output_param(SI_USER_LIMIT);
	}

	if (opts & opt_d) {
		DELIMIT(' ');
		output_param(SI_SRPC_DOMAIN);
	}

	if (opts & opt_X) {
		struct scoutsname scouts;
		char *s;

		if (__scoinfo(&scouts, sizeof scouts) == -1) {
			perror("uname -X");
		   	return 1;
		}
		fputs("\nSystem = ", stdout);
		output_sysname();
		fputs("\nNode = ", stdout);
		output_param(__O_SI_HOSTNAME);
		printf("\nRelease = %s\n",
		       override_Xrelease ? override_Xrelease : scouts.release);
		fputs("KernelID = ", stdout);
		if ((s = _get_param(SI_KERNEL_STAMP, 1)) == NULL)
			s = scouts.kernelid;
		fputs(s, stdout);
		fputs("\nMachine = ", stdout);
		if ((s = _get_param(SI_MACHINE, 1)) == NULL)
			s = scouts.machine;
		if (strncmp(s, "Pent ", 5) == 0)
			printf("Pentium %s", s + 5);
		else
			fputs(s, stdout);
		printf("\nBusType = %s\n", scouts.bustype);
		fputs("Serial = ", stdout);
		output_param(SI_HW_SERIAL);
		fputs("\nUsers = ", stdout);
		if ((s = _get_param(SI_USER_LIMIT, 1)) != NULL) {
			if (strcmp(s, "unlimited") == 0)
				s = "unlim";
			fputs(s, stdout);
		}
		printf("\nOEM# = %d\n", scouts.sysoem);
		printf("Origin# = %d\n", scouts.sysorigin);
		printf("NumCPU = %ld\n", sysconf(_SC_NPROCESSORS_CONF));
	}

	putchar('\n');

	return 0;
}

void
check_overrides(void)
{
	char *s, *s2;

	/*
	 * If the UNAME_OLD environment variable is set, uname w/o args
	 * gives nodename instead of sysname, to emulate very old behavior.
	 */
	if (getenv("UNAME_OLD"))
		dflt_opt = opt_n;

	/*
	 * If the SCOMPAT environment variable is set, and matches the
	 * following syntax, use it to override certain parameter values.
	 *
	 *	release : version [ : sysname [ : Xrelease ] ]
	 */

	if ((s = getenv("SCOMPAT")) == NULL || *s == '\0')
		return;

	if ((s2 = strchr(s, ':')) == NULL || s2 == s)
		return;
	override_release = s;
	*s2++ = '\0';

	s = s2;
	if ((s2 = strchr(s, ':')) == s)
		return;
	override_version = s;
	if (s2 == NULL)
		return;
	*s2++ = '\0';

	s = s2;
	if ((s2 = strchr(s, ':')) == s)
		return;
	override_sysname = s;
	if (s2 == NULL)
		return;
	*s2++ = '\0';

	s = s2;
	if ((s2 = strchr(s, ':')) == s)
		return;
	override_Xrelease = s;
	if (s2 == NULL)
		return;
	*s2++ = '\0';
}

char *
_get_param(int si_param, int fail_ok)
{
	static char staticbuf[SYS_NMLN];
	static char *bufp = staticbuf;
	static long buflen = sizeof staticbuf;
	long len;

	while ((len = sysinfo(si_param, bufp, buflen)) > buflen) {
		if (bufp != staticbuf)
			free(bufp);
		bufp = malloc(buflen = len);
	}
	if (len == -1) {
		if (!fail_ok) {
			pfmt(stderr, MM_ERROR, ":731:Sysinfo failed: %s\n",
				strerror(errno));
			exit(1);
		}
		return NULL;
	}
	return bufp;
}

void
output_param(int si_param)
{
	fputs(get_param(si_param), stdout);
}

void
output_sysname(void)
{
	char *sysname, *save_sysname;

	if ((sysname = override_sysname) == NULL) {
		sysname = _get_param(SI_SYSNAME, 1);
		if (sysname && strcmp(sysname, "UnixWare") == 0) {
			save_sysname = strdup(sysname);
			if (atoi(get_param(SI_RELEASE)) <= 4)
				sysname = NULL;
			else
				sysname = save_sysname;
		}
		if (sysname == NULL)
			sysname = get_param(__O_SI_SYSNAME);
	}
	fputs(sysname, stdout);
}

int
output_bustypes(void)
{
	char *bustypes;
	struct scoutsname scouts;

	if ((bustypes = _get_param(SI_BUSTYPES, 1)) == NULL) {
		if (__scoinfo(&scouts, sizeof scouts) == -1)
			return -1;
		bustypes = scouts.bustype;
	}
	fputs(bustypes, stdout);
	return 0;
}

int
output_selected_param(struct param *pp)
{
	long val;
	char *s;

	switch (pp->p_type) {
	case SYSINFO:
		if ((s = _get_param(pp->p_cmd, 1)) == NULL)
			return -1;
		fputs(s, stdout);
		break;
	case SYSCONF:
		val = sysconf(pp->p_cmd);
		if (val == -1)
			return -1;
		printf("%ld", val);
		break;
	case BUSTYPES:
		return output_bustypes();
	}

	return 0;
}

void
output_named_param(const char *name)
{
	struct param *pp;

	for (pp = params; pp->p_name != NULL; pp++) {
		if (strcmp(pp->p_name, name) == 0) {
			if (output_selected_param(pp) == -1)
				break;
			return;
		}
	}

	pfmt(stderr, MM_ERROR, ":1457:Parameter not supported: %s\n", name);
	exit(1);
}

void
output_all(void)
{
	struct param *pp;
	char *unknown = NULL;

	for (pp = params; pp->p_name != NULL; pp++) {
		printf("%s=", pp->p_name);
		if (output_selected_param(pp) == -1) {
			if (!unknown)
				unknown = gettxt(":1458", "<unknown>");
			fputs(unknown, stdout);
		}
		putchar('\n');
	}
}
