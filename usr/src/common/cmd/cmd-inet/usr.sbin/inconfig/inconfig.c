#ident "@(#)inconfig.c	1.2"
#ident "$Header$"

/*
 * This program is a simple configuration utility that can tune the kernel
 * without requiring a reboot.  Only kernel values that don't control
 * storage allocation can be modified with this program, otherwise the
 * forces of evil will be unleashed.
 *
 * Note that using this can override the configuration compiled into the
 * kernel, so be careful about what you set where.  In general, if you're
 * using inconfig, don't ever change kernel tunables in space.c's, unless
 * you are sure to update the inconfig variables file.
 *
 * We parse a file of two fields/line:
 *
 * symbol_name	current-value
 *
 * The configuration driver, /dev/inet/cfg, will be used to modify the
 * current value.
 *
 * As a convenience, a single symbol/value pair may be given on the command
 * line.  If the value is within the legal range, it will be updated.
 * If no value is given, the current information is retrieved and displayed.
 *
 * Note that we know that ints and longs are the same size, and we cannot
 * change any variable that is not an int.
 */
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/stream.h>
#include <stropts.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in_cfg.h>
#include "pathnames.h"

/*
 * Every line we read from the config file is stored in one of these
 * structures.  Blank/and comment lines are stored in their entirety, with
 * sn_value set to COMMENT_LINE.  Otherwise, the line is parsed and split
 * into the appropriate fields.  Sn_kaddr will be filled in by donlist.
 */

#define COMMENT_LINE	0xffffffff

typedef struct sym_node {
	struct	sym_node *sn_next;	/* next */
	char	*sn_name;		/* name */
	u_long	sn_value;		/* what they want */
} symnode_t;

char lbuf[BUFSIZ];	/* the line buffer */
int symcnt = 0;		/* number of symbols we read */
int linecnt = 0;	/* # of lines in the file, used for error msgs */
int vflag = 0;		/* verbose */
int nflag = 0;		/* don't update config file */
int dflag = 0;		/* debugging (no changes are committed) */
int sflag = 0;		/* display current kernel configuration */
int cfd = -1;		/* config driver file-descriptor */
int havesym = 0;	/* a symbol was specified on the command line */
int notyet = 0;		/* not changing running kernel yet */
int justprint = 0;	/* just print out information about the symbol */
char *symname;		/* a symbol specified on the command line (if any) */
u_long symval;		/* the value for the command line symbol */
int noncomment = 0;	/* says whether we found any non-comment lines */

/* function prototypes */
symnode_t *parseline __P((char *, symnode_t *));
void writefile __P((char *, symnode_t *));
void error __P((char *));
void warning __P((char *));
void dosym __P((symnode_t *));
int setsym __P((char *, u_long));
void usage __P((void));

main(argc, argv)
	int             argc;
	char          **argv;
{
	FILE	*fp;
	int		c;
	char *tunef = _PATH_CONFIGFILE;
	symnode_t head;
	extern char *optarg;
	extern int optind;
	symnode_t *cursym;

	head.sn_next = 0;

	while ((c = getopt(argc, argv, "dvnf:s")) != EOF) {
		switch (c) {
		case 'v':
			vflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'f':
			tunef = optarg;
			break;
		case 'n':
			nflag++;
			break;
		case 's':
			sflag++;
			justprint++;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc && argc == 2) {
		/* we must just have a symbol to update */
		havesym++;
		symname = argv[0];
		symval = strtol(argv[1], (char **)0, 0);
	} else if (argc == 1) {
		/* we must just have a symbol to update */
		havesym++;
		symname = argv[0];
		justprint = 1;
	} else if (argc) {
		usage();
	}

	cfd = open(_PATH_INCONFIG, O_RDWR);
	if (cfd < 0) {
		if (sflag) {
			/* can't report current configuration, so bail */
			fprintf(stderr, "inconfig: ");
			perror(_PATH_INCONFIG);
			exit(1);
		}
		/* system must be installing, just build file */
		notyet = 1;
	}

	fp = fopen(tunef, "r");

	if (fp == (FILE *)0) {
		error(tunef);
	}

	cursym = &head;

	/* read the file */

	while (fgets(lbuf, sizeof(lbuf), fp) != (char *)0) {
		cursym = parseline(lbuf, cursym);
	}

	if (noncomment == 0) {
		/* nothing to tune */
		exit(0);
	}

	if (sflag) {
		printf("inconfig:\n");
	}

	dosym(head.sn_next);

	(void) fclose(fp);

	if (havesym && (dflag == 0) && (nflag == 0) && !justprint) {
		writefile(tunef, head.sn_next);
	}

	exit(0);
	/* NOTREACHED */
}

/*
 * Crude line parsing using strpbrk.  Splits lines into fields and
 * allocates symbol entries to hold lines.  Each symbol is attached to
 * the end of the symbol list.
 *
 * Increments linecnt.
 * Increments symcnt for every symbol allocated.
 */
symnode_t *
parseline(line, current)
	char	*line;
	symnode_t	*current;
{
	symnode_t *sn;
	char *p;
	char *p2;
	char *p3 = line;

	linecnt++;

	if (lbuf[0] == '#' || lbuf[0] == '\n') {
		sn = (symnode_t *) malloc(sizeof(*sn));
		if (sn == 0) {
			error("malloc symnode");
		}
		sn->sn_value = COMMENT_LINE;
		sn->sn_name = strdup(line);
		current->sn_next = sn;
		sn->sn_next = 0;
		return sn;
	}

	noncomment = 1;

	sn = (symnode_t *) malloc(sizeof(*sn));
	if (sn == 0) {
		errno = ENOMEM;
		error("malloc symnode");
	}
	p = strpbrk(line, " \t");
	if (p == 0) {
		(void) fprintf(stderr,
			"inconfig: line %d is missing value field\n", linecnt);
		return current;
	}
	*p++ = '\0';

	sn->sn_name = strdup(p3);
	while (*p == ' ' || *p == '\t')
		p++;
	p2 = p;

	p = strpbrk(p2, "# \t\n");
	
	if (p == 0) {
		(void) fprintf(stderr,
			"inconfig: extra information on line %d will be ignored\n", 
			linecnt);
		return current;
	}
	*p++ = '\0';
	sn->sn_value = strtol(p2, (char **)0, 0);

	current->sn_next = sn;
	sn->sn_next = 0;
	symcnt++;

	return sn;
}

/*
 * Process every symbol in the list, unless just one was specified on the 
 * command line, in which case skip every one else.
 */
void
dosym(symbol)
	symnode_t *symbol;
{
	symnode_t *sn = symbol;
	u_long val;
	int cantfind = 1;

	while (sn) {
		/* skip this; it's a comment line */
		if (sn->sn_value == COMMENT_LINE) {
			sn = sn->sn_next;
			continue;
		}
		/* just one we got on the command line */
		if (havesym) {
			if (strcmp(symname, sn->sn_name) != 0) {
				sn = sn->sn_next;
				continue;
			}
			val = symval;
			cantfind = 0;
		} else {
			val = sn->sn_value;
		}
				
		if (vflag) {
			(void) printf("%s %ld\n", sn->sn_name, sn->sn_value);
		}

		/*
		 * skip updating kernel if it doesn't exist
		 */
		if (notyet == 0) {
			if (setsym(sn->sn_name, val) == -1) {
				/* failed, leave current value alone */
				sn = sn->sn_next;
				continue;
			}
		}
		if (havesym) {
			sn->sn_value = val;
		}
		sn = sn->sn_next;
	}
	if (havesym && cantfind) {
		fprintf(stderr,"inconfig: symbol %s is not in %s\n",
			symname, _PATH_CONFIGFILE);
	}
}

/*
 * Use config driver to update symbol unless justprint == 1, in which
 * case we just retrieve it
 */
int
setsym(name, val)
	char	*name;
	u_long	val;
{
	long r;
	struct strioctl si;
	in_config_t ict;

	si.ic_cmd = justprint ? INCFG_RETRIEVE : INCFG_MODIFY;
	si.ic_len = sizeof(ict);
	si.ic_dp = (char *)&ict;
	si.ic_timout = 0;

	ict.ic_current = val;
	strcpy(ict.ic_name, name);

	if (dflag && !justprint) {
		(void) printf("inconfig: would have set %s to %d\n", name, val);
		return 0;
	}

	r = ioctl(cfd, I_STR, (char *)&si);
	if (r == -1) {
		switch (errno) {
		case ENOENT:
			if (vflag || justprint) {
				(void) fprintf(stderr,"inconfig: symbol %s: ",
					name);
				(void)fprintf(stderr,"not found in the kernel\n");
			}
			return 0;
		case EDOM:
			(void) fprintf(stderr,"inconfig: symbol %s: ", name);
			fprintf(stderr,"value too small\n");
			return -1;
		case ERANGE:
			(void) fprintf(stderr,"inconfig: symbol %s: ", name);
			fprintf(stderr,"value too large\n");
			return -1;
		case EACCES:
			(void) fprintf(stderr,"inconfig: symbol %s: ", name);
			fprintf(stderr, "cannot be modified: %s\n",
				strerror(errno));
			return -1;
		default:
			(void) fprintf(stderr,"inconfig: symbol %s: ", name);
			fprintf(stderr,"unexpected error: %s\n", 
				strerror(errno));
			return -1;
		}
	}

	if (justprint) {
		printf("%ssymbol %s has current value %d, minimum %ld, maximum %ld\n", 
			sflag ? "\t" : "inconfig: ", name, ict.ic_current,
			ict.ic_minimum, ict.ic_maximum);
	}

	if (vflag) {
		(void) printf("inconfig: set %s to %d\n", name, val);
	}
	return 0;
}

void
error(s)
	char           *s;
{
	(void) fprintf(stderr, "inconfig: ");
	perror(s);
	exit(1);
}

void
warning(s)
	char           *s;
{
	(void) fprintf(stderr, "inconfig warning: ");
	perror(s);
}

void
usage()
{
	(void) fprintf(stderr, 
	       "usage: inconfig [-dnv] [-f file] [-s] [symbol [value]]\n");
	exit(1);
}

void
writefile(tunef, symbol)
	char	*tunef;
	symnode_t *symbol;
{
	long r;
	symnode_t *sn;
	char *otunef;
	FILE *fp;
	int fl;

	otunef = malloc(strlen(tunef) + strlen(".bak") + 1);

	if (otunef == 0) {
		error("malloc failed - configuration file unchanged");
	}

	strcpy(otunef, tunef);
	strcat(otunef, ".bak");

	sighold(SIGHUP);
	sighold(SIGTERM);
	sighold(SIGINT);

	r = rename(tunef, otunef);

	if (r < 0) {
		error("rename - configuration file unchanged");
	}

	fp = fopen(tunef, "w");
	if (fp == NULL) {
		fprintf(stderr,"inconfig: open of %s failed (%s) - restoring to original state\n", tunef, strerror(errno));
		goto restor;
	}

	fl = fcntl(fileno(fp), F_GETFL, 0);
	if (r < 0) {
		fprintf(stderr,"inconfig: flag retrieval for %s failed (%s) - restoring to original state\n", tunef, strerror(errno));
		goto restor;
	}
	fl |= O_SYNC;
	r = fcntl(fileno(fp), F_SETFL, fl);
	if (r < 0) {
		fprintf(stderr,"inconfig: flag update for %s failed (%s) - restoring to original state\n", tunef, strerror(errno));
		goto restor;
	}

	sn = symbol;
	while (sn) {

		if (sn->sn_value == COMMENT_LINE) {
			/* comments have a new-line that fgets left in */
			r = fprintf(fp, "%s", sn->sn_name);
		} else {
			r = fprintf(fp, "%-32s%ld\n", sn->sn_name, 
				sn->sn_value);
		}
		if (r < 0) {
			fprintf(stderr,"inconfig: write of %s failed (%s) - restoring to original state\n", tunef, strerror(errno));
			goto restor;
		}
		sn = sn->sn_next;
	}

	r = fclose(fp);

	if (r < 0) {
		fprintf(stderr,
			"inconfig: close of %s failed (%s) - restoring to original state\n",
			tunef, strerror(errno));
restor:
		r = rename(otunef, tunef);
		if (r < 0) {
			fprintf(stderr, 
				"inconfig: restore of configuration failed - configuration left in %s\n",
				otunef);
			exit(2);
		}
		exit(1);
	}

	r = unlink(otunef);
	if (r < 0) {
		fprintf(stderr,"inconfig: warning - couldn't remove %s\n",
		otunef);
		exit(1);
	}
}
