#ident	"@(#)dstime:dstime/dst_pgen.c	1.2"

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define	STAND_BOOT	"/stand/boot"

static char delete = 0;
static char rdonly = 0;
static char update = 0;
static char *file = STAND_BOOT;
static char *t_file;
static char buf[BUFSIZ];
static FILE *tmpfp = NULL;
static char *prog;

void trap();
void usage();
char *param_read(FILE *, char *);
char *param_write(FILE *, char *, char *);

main(int argc, char **argv)
{
	int c;
	int errflag = 0;
	char *param;
	char *value;
	register FILE *fp;
	void (*func)();

	prog = strrchr(argv[0], '/');
	if (prog++ == NULL) {
		prog = argv[0];
	}
	(void)sprintf(buf, "UX:%s", prog);
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel(buf);

	while ((c = getopt(argc, argv, "df:ru")) != EOF) {
		switch (c) {
			case 'd':
				delete++;
				if (rdonly || update) {
					errflag++;
				}
				break;
			case 'f':
				file = optarg;
				break;
			case 'r':
				rdonly++;
				if (delete || update) {
					errflag++;
				}
				break;
			case 'u':
				update++;
				if (delete || rdonly) {
					errflag++;
				}
				break;
			case '?':
				usage();
		}
	}

	switch (argc - optind) {
		case 1:
			if (!rdonly && !delete) {
				errflag++;
				break;
			}
			param = argv[optind];
			value = NULL;
			break;
		case 2:
			if (rdonly || delete) {
				errflag++;
				break;
			}
			param = argv[optind];
			value = argv[optind + 1];
			break;
		default:
			errflag++;
	}

	if (errflag) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		usage();
	}

	func = signal(SIGINT, trap);
	if (func != SIG_DFL) {
		(void)signal(SIGINT, func);
	}
	(void)signal(SIGHUP, trap);

	if ((fp = fopen(file, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n",
			file, strerror(errno));
		exit(1);
	}
	value = rdonly ? param_read(fp, param) : param_write(fp, param, value);
	if (value == NULL) {
		pfmt(stderr, MM_INFO, ":471:Undefined label: %s\n", param);
	} else if (!delete) {
		fprintf(stdout, "%s=%s\n", param, value);
	}
	exit(value ? 0 : 1);
}

void
trap(int signo)
{
	if (tmpfp != NULL) {
		(void)fclose(tmpfp);
		(void)unlink(t_file);
	}
	exit(3);
}

void
usage()
{

	pfmt(stderr, MM_ACTION|MM_NOGET,
		"Usage: %s [-f pathname] -d parameter\n", prog);
	pfmt(stderr, MM_NOSTD|MM_NOGET,
		"\t\t\t    %s [-f pathname] -r parameter\n", prog);
	pfmt(stderr, MM_NOSTD|MM_NOGET,
		"\t\t\t    %s [-f pathname] [-u] parameter value\n", prog);
	exit(2);
}

char *
param_read(FILE *fp, char *pat)
{
	register int len;
	register int patlen;

	if (pat) {
		patlen = strlen(pat);
		rewind(fp);
	}
	while (fgets(buf, sizeof(buf), fp)) {
		if (buf[0] == '#') {
			continue;
		}
		len = strlen(buf);
		if (buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
		} else {
			return NULL;
		}
		if (pat == NULL) {
			return buf;
		}
		if ((strncmp(pat, buf, patlen) == 0) && (buf[patlen] == '=')) {
			return &buf[++patlen];
		}
	}
	return NULL;
}

char *
param_write(FILE *fp, char *param, char *value)
{
	int found = 0;
	register int len;
	register char *cwd;
	struct stat stbuf;

	if (stat(file, &stbuf)) {
		pfmt(stderr, MM_ERROR,
			":602:%s failed: %s", "stat()", strerror(errno));
		exit(4);
	}

	cwd = buf;
	*cwd = '\0';
	if (*file != '/') {
		if ((cwd = getcwd(buf, sizeof(buf))) == NULL) {
			pfmt(stderr, MM_ERROR,
				":1185:Cannot determine current directory\n");
			exit(4);
		}
		(void)strcat(cwd, "/");
	}
	(void)strcat(buf, file);
	t_file = strrchr(buf, '/');
	*t_file++ = '\0';
	if (access(cwd, W_OK|EFF_ONLY_OK)) {
		pfmt(stderr, MM_ERROR,
			":5:Cannot access %s: %s\n", cwd, strerror(errno));
		exit(4);
	}
	if ((t_file = tempnam(cwd, t_file)) == NULL) {
		pfmt(stderr, MM_ERROR, ":95:Cannot get temporary file name.\n");
		exit(4);
	}
	if ((tmpfp = fopen(t_file, "w")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":424:Cannot create temp file: %s\n", strerror(errno));
		exit(4);
	}
	rewind(fp);
	len = strlen(param);
	while (fgets(buf, sizeof(buf), fp)) {
		if (!found &&
		    (strncmp(param, buf, len) == 0) && (buf[len] == '=')) {
			found++;
			if (delete) {
				int k;

				value = strdup(&buf[len + 1]);
				k = strlen(value);
				if (value[k - 1] == '\n') {
					value[k - 1] = '\0';
				}
				continue;
			}
			(void)sprintf(&buf[len], "=%s\n", value);
		}
		(void)fputs(buf, tmpfp);
	}
	if (!found) {
		if (update || delete) {
			(void)fclose(tmpfp);
			tmpfp = NULL;
			(void)unlink(t_file);
			return NULL;
		}
		(void)sprintf(buf, "%s=%s\n", param, value);
		(void)fputs(buf, tmpfp);
	}
	(void)fclose(tmpfp);
	tmpfp = NULL;
	if (rename(t_file, file)) {
		pfmt(stderr, MM_ERROR,
			":339:Cannot rename %s to %s: %s\n",
			t_file, file, strerror(errno));
		(void)unlink(t_file);
		exit(4);
	}
	(void)chmod(file, stbuf.st_mode);
	(void)chown(file, stbuf.st_uid, stbuf.st_gid);
	return value;
}
