/*
 * simple pre-processor for TCL to allow for "conditional" compilation
 *
 * only one construct supported:
@if varname
	stuff...
@else
	stuff...
@endif
 * @ must be first char on line.
 * nesting is not supported.
 */

#include <stdio.h>

#define debug printf

void usage();
void do_file(char *, char **, int);

#define MAXDEF 64

char *file;
char *defs[MAXDEF];
int defcnt;
char buf[BUFSIZ];

main(argc, argv)
char **argv;
{
	argc--;
	argv++;
	while (argc && (strcmp(*argv, "-D") == 0)) {
		argc--;
		argv++;
		if (argc == 0)
			break;
		if (defcnt == (MAXDEF-1)) {
			fprintf(stderr, "Definition table exhausted\n");
			exit(1);
		}
		defs[defcnt++] = *argv;
		argc--;
		argv++;
	}
	if (argc != 1)
		usage();
	file = *argv;
	do_file(file, defs, defcnt);
}

void
do_file(char *file, char **defs, int defcnt)
{
	FILE *fd;
	int inside;
	int output;
	int line;
	char *cp;
	int i;

	fd = fopen(file, "r");
	if (fd == 0) {
		fprintf(stderr, "gentcl: unable to open %s\n", file);
		exit(1);
	}
	inside = 0;
	output = 1;
	for (line = 1; fgets(buf, BUFSIZ, fd) != 0; line++) {
		buf[strlen(buf)-1] = 0;
		if (strncmp(buf, "@if", 3) == 0) {
			if (inside) {
				fprintf(stderr, "gentcl: nested if at line %d\n", line);
				exit(1);
			}
			cp = buf + 3;
			while ((*cp == ' ') || (*cp == '\t'))
				cp++;
			if (*cp == 0) {
				fprintf(stderr, "gentcl: syntax err: line %d\n", line);
				exit(1);
			}
			inside = 1;
			output = 0;
			for (i = 0; i < defcnt; i++) {
				if (strcmp(cp, defs[i]) == 0) {
					output = 1;
					break;
				}
			}
			continue;
		}
		if (strncmp(buf, "@else", 5) == 0) {
			if (inside == 0) {
				fprintf(stderr, "gentcl: else without if at line %d\n", line);
				exit(1);
			}
			if (output == 1)
				output = 0;
			else
				output = 1;
			continue;
		}
		if (strncmp(buf, "@endif", 6) == 0) {
			if (inside == 0) {
				fprintf(stderr, "gentcl: endif without if at line %d\n", line);
				exit(1);
			}
			inside = 0;
			output = 1;
			continue;
		}
		if (buf[0] == '@') {
			fprintf(stderr, "Unknown directive: line %d\n", line);
			exit(1);
		}
		if (output)
			printf("%s\n", buf);
	}
	if (inside) {
		fprintf(stderr, "eof without endif\n");
		exit(1);
	}
	fclose(fd);
}

void
usage()
{
	fprintf(stderr, "usage: gentcl [-D varname...] file\n");
	fprintf(stderr, "    processed file goes to stdout\n");
	exit(1);
}
