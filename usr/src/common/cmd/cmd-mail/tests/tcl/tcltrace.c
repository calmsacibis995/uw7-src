#ident "@(#)tcltrace.c	11.1"
/*
 * primitive tcl branch coverage analyser.
 *
 * three functions are provided:
 * o - instrument a tcl program or source file.
 * o - merge multiple trace output files into a main list.
 * o - annotate the source file, prepending all lines not executed with '>'.
 *
 * our instrumentation algorithm is where the primitivenes is.
 * rather than a general purpose lex/yacc thing, we just look for
 * lines that end in '{' that are not case or switch statements.
 * after that line we append a call to our you-got-here routine.
 *
 * the you-got-here routine is named after the file instrumented so
 * that individual modules can be instrumented as needed for focussed
 * analysis.  The naming convention is _branch_module, where module is
 * the file name up to the first '.' as dot is not a valid tcl name.
 *
 * This means that an entire program or individual modules may be instrumented
 * at the tester's convenience.
 *
 * trace output is logged to files named trace.module.pid.
 *
 * a side note, this is not really full branch coverage, it is just
 * most branch coverage, every close brace (except case and switch)
 * should also have an instrument after it for full branch coverage.
 *
 * if this thing was rewritten to parse TCL properly then it could do this.
 * alternatively, we could put in magic cookie comments in the code
 * to provoke instrumentation of branches this program doesn't catch.
 */

#include <stdio.h>

#define debug printf

#define MOD_LEN 128

typedef struct item {
	struct item *i_fwd;
	char *i_name;
} item_t;

void instrument(char *);
void merge(char **);
void annotate(char *, char *);
void usage();

/*
 * parse args and call the right routine
 */
main(argc, argv)
char **argv;
{
	char *option;
	char *srcfile;
	char *tracefile;

	argv++;
	argc--;
	if (argc < 2)
		usage();
	option = *argv++;
	argc--;

	if (strcmp(option, "-i") == 0) {
		srcfile = *argv++;
		argc--;
		if (argc != 0)
			usage();
		instrument(srcfile);
	}
	else if (strcmp(option, "-m") == 0) {
		merge(argv);
	}
	else if (strcmp(option, "-a") == 0) {
		if (argc != 2)
			usage();
		tracefile = *argv++;
		srcfile = *argv++;
		annotate(tracefile, srcfile);
	}
	else {
		fprintf(stderr, "tcltrace: Unknown option: %s\n", option);
		usage();
	}
	exit(0);
}

void
usage()
{
fprintf(stderr, "usage:  tcltrace -i srcfile\n");
fprintf(stderr, "        tcltrace -m tracefile...\n");
fprintf(stderr, "        tcltrace -a tracefile srcfile\n");
fprintf(stderr, "\n");
fprintf(stderr, "-i - Instrument a tcl source file, results to stdout.\n");
fprintf(stderr, "        therafter each invocation of the file will produce\n");
fprintf(stderr, "        a file trace.pid in the current directory.\n");
fprintf(stderr, "-m - Merge multiple trace files to stdout.\n");
fprintf(stderr, "        this resulting output can be used as input to -m again.\n");
fprintf(stderr, "-a - Annotate a source file based on a trace file, prepending each\n");
fprintf(stderr, "        unexecuted line with a '>' character.\n");
exit(1);
}

void
instrument(char *file)
{
	char line[1024];
	char module[MOD_LEN];
	FILE *fd;
	int count;
	register char *cp;
	char *cp1;
	char *first;
	char *last;

	cp = (char *)strrchr(file, '/');
	if (cp == 0)
		cp = file;
	else
		cp++;
	strcpy(module, cp);
	cp = (char *)strchr(module, '.');
	if (cp)
		*cp = 0;
	if ((fd = fopen(file, "r")) == 0) {
		fprintf(stderr, "tcltrace: Unable to open file: %s\n", file);
		exit(1);
	}
	/* get first line */
	if (fgets(line, 1024, fd) == 0) {
		fprintf(stderr, "tcltrace: empty source file: %s\n", file);
		exit(1);
	}
	printf("%s", line);

	/* now do our stuff */

printf("set _trace_%s_fd [open trace.%s.[pid] w]\n", module, module);
printf("proc _branch_%s { arg } {\n", module);
printf("	global _trace_%s_fd\n", module);
printf("	puts $_trace_%s_fd $arg\n", module);
printf("	flush $_trace_%s_fd\n", module);
printf("}\n");

	/*
	 * rest of file, look for lines ending in '{' without case or switch
	 * count total number of branches found and report to stderr
	 */
	count = 0;
	while (fgets(line, 1024, fd) != 0) {
		printf("%s", line);

		/* first non-white space char on line */
		cp = line;
		while (*cp && ((*cp == ' ') || (*cp == '\t')))
			cp++;
		first = cp;

		/* last non-white space char on line */
		last = first;
		for (cp = first; *cp; cp++) {
			if ((*cp == ' ') || (*cp == '\t') || (*cp == '\n'))
				continue;
			last = cp;
		}


		if (*first == '#')
			continue;
		if (*last != '{')
			continue;
		if (memcmp(first, "case ", 5) == 0)
			continue;
		if (memcmp(first, "case\t", 5) == 0)
			continue;
		if (memcmp(first, "switch ", 7) == 0)
			continue;
		if (memcmp(first, "switch\t", 7) == 0)
			continue;
		/* have a line to instrument */
		printf("_branch_%s %s%05d\n", module, module, count++);
	}
	fclose(fd);
	fprintf(stderr, "%d branches instrumented\n", count);
}

/*
 * have fwd linked list of structures, always in sorted order
 * compare strings as we add them for matches.
 */
void
merge(char **filelist)
{
	char line[1024];
	char *file;
	FILE *fd;
	item_t *base;
	register item_t *ip;
	item_t *ip1;
	int spot;
	int match;

	/* init to no list */
	base = 0;

	while (*filelist) {
		file = *filelist++;
		if ((fd = fopen(file, "r")) == 0) {
			fprintf(stderr, "tcltrace: Unable to open file: %s\n",
				file);
			exit(1);
		}
		while (fgets(line, 1024, fd) != 0) {
			line[strlen(line)-1] = 0;
			/* find line in list or new position */
			ip1 = 0;
			spot = 0;
			match = 0;
			for (ip = base; ip; ip = ip->i_fwd) {
				switch (strcmp(ip->i_name, line)) {
				case 0:
					/* match */
					match = 1;
					break;
				case -1:
					/* go on */
					break;
				case 1:
					/* found spot in ip1 */
					spot = 1;
					break;
				}
				if (spot || match)
					break;
				ip1 = ip;
			}
			/* nothing to do */
			if (match)
				continue;
			/* always create new entry if nomatch */
			ip = (item_t *)malloc(sizeof(item_t));
			ip->i_name = (char *)malloc(strlen(line)+1);
			strcpy(ip->i_name, line);
			if (ip1) {
				ip->i_fwd = ip1->i_fwd;
				ip1->i_fwd = ip;
			}
			else {
				ip->i_fwd = base;
				base = ip;
			}
		}
		fclose(fd);
	}
	for (ip = base; ip; ip = ip->i_fwd) {
		printf("%s\n", ip->i_name);
	}
}

void
annotate(char *tracefile, char *srcfile)
{
	FILE *fd;
	char line[1024];
	item_t *base;
	register item_t *ip;
	item_t *ip1;
	char *cp;
	int found;

	/* read in trace file */
	if ((fd = fopen(tracefile, "r")) == 0) {
		fprintf(stderr, "tcltrace: Unable to open file: %s\n",
			tracefile);
		exit(1);
	}
	base = 0;
	ip1 = 0;
	while (fgets(line, 1024, fd) != 0) {
		line[strlen(line)-1] = 0;

		ip = (item_t *)malloc(sizeof(item_t));
		ip->i_name = (char *)malloc(strlen(line)+1);
		strcpy(ip->i_name, line);
		ip->i_fwd = 0;

		if (base == 0)
			base = ip;
		else
			ip1->i_fwd = ip;
		ip1 = ip;
	}
	fclose(fd);

	/* process srcfile */
	if ((fd = fopen(srcfile, "r")) == 0) {
		fprintf(stderr, "tcltrace: Unable to open file: %s\n",
			srcfile);
		exit(1);
	}
	while (fgets(line, 1024, fd) != 0) {
		line[strlen(line)-1] = 0;
		if (memcmp(line, "_branch_", 8) == 0) {
			cp = (char *)strchr(line, ' ');
			if (cp == 0)
				goto cont;
			cp++;
			if (*cp == 0)
				goto cont;
			/* find cp in list */
			found = 0;
			for (ip = base; ip; ip = ip->i_fwd) {
				if (strcmp(ip->i_name, cp) == 0) {
					found = 1;
					break;
				}
			}
			if (found == 0)
				printf("%c", '>');
		}
cont:
		printf("%s\n", line);
	}
	fclose(fd);
}
