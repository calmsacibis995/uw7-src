#ident	"@(#)pat:oscompat.c	1.2"

#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <utime.h>

#define FEATURE_LIMIT		12	/* from OSr5 sys/osfeatures.h */
#define SI86GETFEATURES		114	/* from OSr5 sys/sysi86.h */
#define ACPU_LOCK		4	/* from OSr5 sys/ci/ciioctl.h */
#define P_PID			0	/* from UW   sys/procset.h */

#define SYS_fsync		58	/* from UW   sys/syscall.h */
#define SYS_fxstat		125	/* from UW   sys/syscall.h */
#define SYS_processor_bind	179	/* from UW   sys/syscall.h */
#define SYS_getksym		202	/* from UW   sys/syscall.h */

#define UW_STAT_VER		2	/* from UW   sys/stat.h */
#define O5_STAT_VER		51	/* from OSr5 sys/stat.h */

#define UWOPSYS			1
#define O5OPSYS			2
static int opsys;			/* initially 0 for unknown */

/*
 * _init_features_vector() from OSr5 lib/libc/port/gen/features.c:
 * because the OSr5 build links a reference to it,
 * but the UW libc.so.1 does not resolve it.
 */
char _features_vector[FEATURE_LIMIT + 3];
void
_init_features_vector(void)
{
	sysi86(SI86GETFEATURES, _features_vector, sizeof(_features_vector));
}

/*
 * getopt() from UW lib/libc/port/gen/getopt.c:
 * because there seems to be an inconsistency in #pragma weaking,
 * such that the UW build links optind etc. into the binary itself,
 * but the OSr5 libc.so.1 getopt() resolves those references locally.
 * Also remove opterr->err()->pfmt() code, since the UW build then links
 * a reference to _pfmt_label, but the OSr5 libc.so.1 does not resolve it.
 */
int
getopt(int argc, char *const *argv, const char *opts)
{
	extern int _sp;
	char *p, *s;
	int ch;

	if (optind >= argc || (p = argv[optind]) == 0)
		return -1;
	if (_sp == 1) /* start of a new list member */
	{
		if (p[0] != '-' || p[1] == '\0') /* not an option or just "-" */
			return -1;
		if (p[1] == '-' && p[2] == '\0') /* was "--" */
		{
			optind++;
			return -1;
		}
	}
	p += _sp;
	optopt = ch = *(unsigned char *)p; /* current (presumed) option */
	p++;
	if (ch == ':' || (s = strchr(opts, ch)) == 0) /* isn't an option */
	{
		ch = '?';
	}
	else if (*++s == ':') /* option needs an argument */
	{
		_sp = 1;
		if (*p != '\0') /* argument is rest of current member */
			optarg = p;
		else if (++optind < argc) /* next member exists */
			optarg = argv[optind];
		else
		{
			optarg = 0;
			if (*opts == ':')
				return ':';
			return '?';
		}
		optind++;
		return ch;
	}
	optarg = 0;
	if (*p == '\0')
	{
		optind++;
		_sp = 0;
	}
	_sp++;
	return ch;
}

/*
 * getksym() via syscall() because the OSr5 libc.so.1 does not resolve it.
 */
int
getksym(char *symname, unsigned long *valuep, unsigned long *infop)
{
	int rv;

	if (opsys != O5OPSYS) {
		if (!opsys)
			(void)signal(SIGSYS, SIG_IGN);
		rv = syscall(SYS_getksym, symname, valuep, infop, 0);
		if (!opsys)
			opsys = (rv < 0 && errno == ENOSYS)? O5OPSYS: UWOPSYS;
		return rv;
	}
	errno = ENOSYS;
	return -1;
}

/*
 * processor_bind_me() via syscall() because the OSr5 libc.so.1 does not
 * resolve processor_bind(), and we need this subset of its functionality
 * even when running on OSr5.
 */
int
processor_bind_me(int engine)
{
	char bindpath[18];
	int bfd, pid, rv;

	if (opsys != O5OPSYS) {
		if (!opsys)
			(void)signal(SIGSYS, SIG_IGN);
		rv = syscall(SYS_processor_bind, P_PID, getpid(), engine, 0);
		if (!opsys)
			opsys = (rv < 0 && errno == ENOSYS)? O5OPSYS: UWOPSYS;
	}
	if (opsys == O5OPSYS) {
		pid = errno = 0;
		sprintf(bindpath, "/dev/at%u", engine + 1);
		if ((rv = open(bindpath, O_RDWR, 0)) >= 0) {
			rv = ioctl(bfd = rv, ACPU_LOCK, &pid);
			close(bfd);
		}
		else if (engine == 0
		&& (errno == ENOENT || errno == ENODEV)) /* UniProcessor */
			rv = errno = 0;
	}
	return rv;
}

/*
 * save_mtime() and restore_mtime() to preserve last modification time
 * (so that a package can be removed after patches have been removed),
 * independent of struct stat variations and interface redefinitions;
 * wanted to use SYS_fstat, but on UW that's liable to get EOVERFLOW.
 */
static struct utimbuf utimbuf;

void
save_mtime(int fd, char *path)
{
	long statbuf[34];
	int rv;

	if (opsys != O5OPSYS) {
		rv = syscall(SYS_fxstat, UW_STAT_VER, fd, &statbuf, 0);
		if (!opsys)
			opsys = (rv < 0 && errno == EINVAL)? O5OPSYS: UWOPSYS;
		if (rv >= 0) {
			utimbuf.actime  = statbuf[14];
			utimbuf.modtime = statbuf[16];
		}
	}
	if (opsys == O5OPSYS) {
		rv = syscall(SYS_fxstat, O5_STAT_VER, fd, &statbuf, 0);
		if (rv >= 0) {
			utimbuf.actime  = statbuf[12];
			utimbuf.modtime = statbuf[13];
		}
	}
}

void
restore_mtime(int fd, char *path)
{
	if (utimbuf.modtime) {
		(void) syscall(SYS_fsync, fd, 0, 0, 0);	/* might help NFS */
		(void) utime(path, &utimbuf);
	}
}
