/*		copyright	"%c%" 	*/

#ident	"@(#)pfmt.c	1.2"
/*
 * pfmt and lfmt
 *
 * Usage:
 * pfmt [-l label] [-s severity] [-g catalog:msgid] format [args]
 * lfmt [-c] [-f flags][-l label] [-s severity] [-g catalog:msgid] format [args]
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

#define NEWSEV	5	/* User defined severity */

#ifdef LFMT
#define FMT	lfmt
#define USAGE	":381:Usage:\n\tlfmt [-c] [-f flags] [-l label] [-s sev] [-g cat:msgid] format [args]\n"
#define LABEL	"UX:lfmt"
#define OPTIONS	"cf:l:s:g:"
char *logopts[] = {
#define SOFT	0
	"soft",
#define HARD	1
	"hard",
#define FIRM	2
	"firm",
#define UTIL	3
	"util",
#define APPL	4
	"appl",
#define OPSYS	5
	"opsys",
	NULL
};
#else
#define FMT	pfmt
#define USAGE	":382:Usage:\n\tpfmt [-l label] [-s severity] [-g cat:msgid] format [args]\n"
#define LABEL	"UX:pfmt"
#define OPTIONS	"l:s:g:"
#endif

extern char *optarg;
extern int optind;

struct sevs {
	int s_int;
	const char *s_string;
} case_dep_sev[] = {
	MM_WARNING,	"warn",
	MM_WARNING,	"WARNING",
	MM_ACTION,	"action",
	MM_ACTION,	"TO FIX"
}, case_indep_sev[] = {
	MM_HALT,	"halt",
	MM_ERROR,	"error",
	MM_INFO,	"info",
	MM_NOSTD,	"nostd"
};

#define NCASE_DEP_SEV	sizeof case_dep_sev / sizeof case_dep_sev[0]
#define NCASE_INDEP_SEV	sizeof case_indep_sev / sizeof case_indep_sev[0]

usage(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
	pfmt(stderr, MM_ACTION, USAGE);
	exit(3);
}

main(argc, argv)
int argc;
char *argv[];
{
	int opt, ret;
	register int i;
	char *loc_label = NULL;
	char *loc_id = NULL;
	char *format;
	char *fmt;
	long severity = -1L;
#ifdef LFMT
	int do_console = 0;
	long log_opt = 0;
	char *subopt, *logval;
#endif

	if ((setlocale(LC_ALL, "")) == (char *) NULL) {
		(void)setlocale(LC_CTYPE, "");
		(void)setlocale(LC_MESSAGES, "");
	}
	(void)setcat("uxcore.abi");
	(void)setlabel(LABEL);

	while ((opt = getopt(argc, argv, OPTIONS)) != EOF){
		switch(opt){
#ifdef LFMT
		case 'c':
			do_console = 1;
			break;
		case 'f':
			subopt = optarg;
			while (*subopt != 0){
				switch(getsubopt(&subopt, logopts, &logval)){
				case SOFT:
					log_opt |= MM_SOFT;
					break;
				case HARD:
					log_opt |= MM_HARD;
					break;
				case FIRM:
					log_opt |= MM_FIRM;
					break;
				case UTIL:
					log_opt |= MM_UTIL;
					break;
				case APPL:
					log_opt |= MM_APPL;
					break;
				case OPSYS:
					log_opt |= MM_OPSYS;
					break;
				default:
					pfmt(stderr, MM_ERROR, ":383:Invalid flag\n");
					pfmt(stderr, MM_ACTION,
						":384:Valid flags are: soft, hard, firm, util, appl and opsys\n");
					exit(3);
				}
			}
			break;
#endif
		case 'l':
			if (loc_label){
				(void)pfmt(stderr, MM_ERROR,
					":385:Can only specify one label\n");
				exit(3);
			}
			loc_label = optarg;
			break;
		case 's':
			if (severity != -1){
				(void)pfmt(stderr, MM_ERROR,
					":387:Can only specify one severity\n");
				exit(3);
			}
				
			for (i = 0 ; i < NCASE_DEP_SEV ; i++){
				if (strcmp(optarg, case_dep_sev[i].s_string) == 0){
					severity = case_dep_sev[i].s_int;
					break;
				}
			}
			if (severity == -1 && (int)strlen(optarg) <= 7){
				char buf[8];
				for (i = 0 ; optarg[i] ; i++)
					buf[i] = tolower(optarg[i]);
				buf[i] = '\0';
				for (i = 0 ; i < NCASE_INDEP_SEV ; i++){
					if (strcmp(buf, case_indep_sev[i].s_string) == 0){
						severity = case_indep_sev[i].s_int;
						break;
					}
				}
			}
			if (severity == -1){
				if (addsev(NEWSEV, optarg) == -1){
					(void)pfmt(stderr, MM_ERROR, ":388:Cannot add user-defined severity\n");
					exit(1);
				}
				severity = NEWSEV;
			}
			break;
		case 'g':
			if (loc_id){
				(void)pfmt(stderr, MM_ERROR,
					":389:Can only specify one message identifier\n");
				exit(3);
			}
			loc_id = optarg;
			break;
		default:
			usage(0);
		}
	}

	if (optind >= argc)
		usage(1);

	if (severity == -1)
		severity = MM_ERROR;

#ifdef LFMT
	severity |= log_opt;
	if (do_console)
		severity |= MM_CONSOLE;
#endif

        if ((fmt = (char *)malloc(strlen(argv[optind]) + 1)) == NULL){
                pfmt(stderr, MM_ERROR, ":312:Out of memory: %s\n", strerror(errno));
                exit(1);
        }
	strccpy(fmt, argv[optind]);

	if (loc_id){
		(void)setcat("uxcore.abi");	/* Do not accept default catalogue */
		format = gettxt(loc_id, fmt);
		(void)setcat("uxcore.abi");
	}
	else
		format = fmt;

	setlabel(loc_label);

	if ((ret = FMT(stderr, severity | MM_NOGET, format, 
			argv[optind + 1] ? argv[optind + 1] : "",
			argv[optind + 2] ? argv[optind + 2] : "",
			argv[optind + 3] ? argv[optind + 3] : "",
			argv[optind + 4] ? argv[optind + 4] : "",
			argv[optind + 5] ? argv[optind + 5] : "",
			argv[optind + 6] ? argv[optind + 6] : "",
			argv[optind + 7] ? argv[optind + 7] : "",
			argv[optind + 8] ? argv[optind + 8] : "",
			argv[optind + 9] ? argv[optind + 9] : "",
			argv[optind + 10] ? argv[optind + 10] : "",
			argv[optind + 11] ? argv[optind + 11] : "",
			argv[optind + 12] ? argv[optind + 12] : "",
			argv[optind + 13] ? argv[optind + 13] : "",
			argv[optind + 14] ? argv[optind + 14] : "",
			argv[optind + 15] ? argv[optind + 15] : "",
			argv[optind + 16] ? argv[optind + 16] : "",
			argv[optind + 17] ? argv[optind + 17] : "",
			argv[optind + 18] ? argv[optind + 18] : "",
			argv[optind + 19] ? argv[optind + 19] : "")) < 0){
		exit(-ret);
	}
	exit(0);
}
