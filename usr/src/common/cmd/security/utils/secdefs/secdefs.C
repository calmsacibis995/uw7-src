#ident  "@(#)secdefs.C	1.3"
#ident  "$Header$"

/******************************************************************************
 *	secdefs.C
 *-----------------------------------------------------------------------------
 * Comments:
 *
 * Examines the packages (aka levels) of defaults stored in the 
 * /tcb/lib/relax directories, and compares them with the
 * defaults currently in use then displays a count of, or
 * the actual differences found.
 * 
 * See relax(ADM) for details of the contents of the relax
 * directories.  This command does not attempt to match the
 * kernel configurable parameters, which are usually set in
 * the script 
 *
 * Usage: secdefs [-v] [ -a | level names ]
 *
 * The default behaviour is to display the level which best
 * matches the defaults with a count of the differences.
 *
 * The -a flag causes all levels to be displayed, or if
 * a list of level names is given, then only those levels
 * are displayed.
 *
 * The -v flag changes the output format from a count of 
 * differences to a short report of the actual differences.
 *-----------------------------------------------------------------------------
 *      @(#)secdefs.C	7.3	97/08/30
 *
 *      Copyright (C) The Santa Cruz Operation, 1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation, and should be treated as Confidential.
 *-----------------------------------------------------------------------------
 * Revision History:
 *
 *	Mon Dec 19, 17:00:50 PST 1996	louisi
 *		Created file.
 *
 *	Mon Dec 30 17:02:14 PST 1996	louisi
 *		Added check for EACESS of files.  If this fails we can
 *              assume the user is not authorized to run the utility.
 *              We no longer require them to be root since they could
 *              run tfadmin.
 *
 *	Tue Sep  9 11:39:17 BST 1997	andrewma
 *		Default dir now /etc/security/seclevel. Was /usr/lib/scoadmin/security.
 *		Changed includes to avoid dependance on scoadmin part of tree and to 
 *		use xenv instead.
 *============================================================================*/


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <basicIncl.h>
#include "secdefs.msgd.h"
#include "secdefs.msg.h"

#define	EXIT_OK		0		/* no errors */
#define	EXIT_ERROR	1		/* an error occurred */
#define	EXIT_USAGE	2		/* usage message */
#define	ARBSTRSIZ	512		/* arbitrary string size */

extern "C" {
extern FILE *defopen(char * );
extern char *defread(FILE *ptr, char *defname);
extern int defclose(FILE * );
}



struct etc_default {
	char *file;
	char *field;
	char *value;
	char operation;
	struct etc_default *next_default;
};

struct package {
	int count;
	char *name;
	struct etc_default *etc_set;
	struct package *next_package;
};

struct display {
	int file;
	int field;
};

static void	usage(void);
static void     compare_package(struct package *);
static void     display_package(struct package *, int);
static void 	check_exists(char *);
static void 	check_read(char *);
static int 	check_name(char *);
static int 	package_comp(struct package **, struct package **);
static char	*make_path(char	*, char	*);
static char 	*make_path(char *, char *);
static char 	*newstring(char *);
static char     *read_def_file(char *, char *);
static char     *get_etc_def(char *, char *);

static struct display     *find_longest_strings(struct etc_default *);
static struct package 	  *all_packages(void);
static struct package 	  *read_package(char *);
static struct package 	  *read_package(char *);
static struct package     *sort_list(struct package *);
static struct etc_default *new_etc_default(void);
static struct etc_default *read_etc_values(char *);
static struct etc_default *store_on_list(
	struct etc_default *, char, char *, char *, char *
);
static int compare_etc_defaults(
	char *, struct etc_default *, struct display *
);
static void display_values(
	char *, struct display *, char *, char *, char *,char *
);

static char	*default_dir = "/etc/security/seclevel/";
static char	*etc_def = "/etc_def";
static char	*shell_script = "/script";
static char	*etc_default_dir = "/etc/default/";
static char	*command_name = "secdefs";

char * on;
char * off;				/* L003 Stop */


int
main(
	int	argc,
	char	**argv
){
	int vflag = 0, aflag = 0;
	struct package *current, *head = (struct package *)0;
	char c;
	errStatus_cl errStk;

	/* parse command line arguments */
	while ((c = getopt(argc, argv, "va")) != EOF) {
		switch (c) {

		case ('v'):	/* verbose */
			vflag++;
			break;
		case ('a'):	/* all packages */
			aflag++;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	/* check the relax defaults directory exists */
	check_read(default_dir);

	if (optind < argc) {
		if (aflag) {
			usage();
			/* NOTREACHED */
		}

		/* display specified packages */
		for (; optind < argc; optind++) {
			current = read_package(argv[optind]);
			compare_package(current);
			display_package(current, vflag);
		}
		exit(EXIT_OK);
	}
	else {
		head = all_packages();
		if (head == (struct package *)0) {

			errStk.Push(&SCO_SECDEFS_ERR_NO_ENT, default_dir);
			errStk.Output(stderr);
			exit(EXIT_OK);
			/* NOTREACHED */
		}
	}
	
	/* 
	 * Compare each of the packages read against the real 
	 * defaults and record the number of differences.
	 */
	for (current = head;
	     current != (struct package *)0; 
	     current = current->next_package)
	{
		compare_package(current);
	}

	head = sort_list(head);
	
	/* display the results */
	for (current = head;
	     current != (struct package *)0; 
	     current = current->next_package)
	{
		if (aflag) {
			display_package(current, vflag);
		}
		else if (current->count == head->count) {
			display_package(current, vflag);
		}
	}
	exit(EXIT_OK);
}

/*
 * Display the usage message on stderr and exit.
 */
static void
usage(
	void /* no args */
){
	errStatus_cl errStk;

	errStk.Push(&SCO_SECDEFS_ERR_USAGE, command_name);
	errStk.Output(stderr);
	exit(EXIT_USAGE);
	/* NOTREACHED */
}

/* 
 * Sorts a linked list of struct package's in ascending order of count.
 */
static struct package *
sort_list(
	struct package *head
){
	struct package **table, *current;
	errStatus_cl errStk;

	int i, count = 0;

	for (current = head;
	     current != (struct package *)0; 
	     current = current->next_package)
	{
		count++;
	}
	table = new struct package *[count * sizeof(struct package *)];
	if (table == (struct package **)0) {			/* L002 */
		errStk.Push(&SCO_SECDEFS_ERR_MEM);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	count = 0;
	for (current = head;
	     current != (struct package *)0; 
	     current = current->next_package)
	{
		table[count++] = current;
	}
	qsort(table, count, sizeof(struct package *), (int (*)(const void *, const void *)) package_comp);
	
	for(i = 0; i < count -1; i++) {
		table[i]->next_package = table[i+1];
	}
	table[i]->next_package = (struct package *)0;
	current = table[0];
	delete table;
	return(current);
}

/* 
 * Comparison routine used for qsort (called in sort_list).
 */
static int package_comp(
	struct package **first,
	struct package **second
){
	if ((*first)->count > (*second)->count) {
		return(1);
	}
	else if ((*first)->count < (*second)->count) {
		return(-1);
	}
	else {
		return(0);
	}
}
		
/* 
 * Read all packages in /tcb/lib/relax and store them in a linked list.
 */
static struct package *
all_packages(
	void /* NO ARGS */	
){
	DIR *dir_list;
	struct dirent *dir_ent;
	struct package *head, *tail, *current;
	errStatus_cl errStk;

	head = (struct package *)0;

	dir_list = opendir(default_dir);
	if (dir_list == (DIR *)0) {

		errStk.PushUnixErr(errno);
		errStk.Push(&SCO_SECDEFS_ERR_OPEND, default_dir);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}

	while ((dir_ent = readdir(dir_list)) != (struct dirent *)0) {
		if  (strcmp(dir_ent->d_name, ".") == 0 ||
		     strcmp(dir_ent->d_name, "..") == 0) 
		{
			continue;
		}
		current = read_package(dir_ent->d_name);
		if (head == (struct package *)0) {
			head = current;
		} 
		else {
			tail->next_package = current;
		}
		tail = current;
	}
	(void)closedir(dir_list);
	return(head);
}

/*
 * Read a package of defaults. 
 */
static struct package *
read_package(
	char *package_name
){
	char *base_dir, *script_path, *etc_path;
	struct package *package;
	errStatus_cl errStk;

	/* check base directory of the package is readable */
	base_dir = make_path(default_dir, package_name);
	check_read(base_dir);

	/* make pathnames and check they exist and are readable */
	script_path = make_path(base_dir, shell_script);
	etc_path = make_path(base_dir, etc_def);
	check_exists(script_path);
	check_read(etc_path);

	/* allocate space for new package */
	package = new struct package [sizeof(struct package)];
	if (package == (struct package *)0) {
		errStk.Push(&SCO_SECDEFS_ERR_MEM2);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	package->next_package = (struct package *)0;

	package->count = 0;
	package->name = newstring(package_name);

	/* read /etc defaults */
	package->etc_set = read_etc_values(etc_path);

	delete script_path;
	delete etc_path;

	return(package);
}
	
/*
 * Read the list of /etc/default style value from <path>.
 */
static struct etc_default *
read_etc_values(
	char *path
){
	char *buf, *pos, operation, *file, *field, *value, c;
	int length, lineno;
	FILE *fp;
	struct etc_default *head = (struct etc_default *)0;
	errStatus_cl errStk;

	if ((fp = fopen(path, "r")) == (FILE *)0) {
		errStk.PushUnixErr(errno);
		errStk.Push(&SCO_SECDEFS_ERR_OPENF, path);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}

	buf = new char[BUFSIZ];
	if (buf == (char *) NULL) {
		errStk.Push(&SCO_SECDEFS_ERR_MEM2);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	lineno = 0;
	while ((pos = fgets(buf, BUFSIZ, fp)) != (char *)0) {
		lineno++;
		length = strlen(pos) - 1;
		if (pos[length] != '\n') {
			/* skip the remainder of the line */
			while ((c = getc(fp)) != EOF && c != '\n') {
				;
			}
			pos[BUFSIZ - 2] = '\0';
			errStk.Push(&SCO_SECDEFS_ERR_TRUNK, path, lineno);
			errStk.Output(stderr);
		}
		else {
			pos[length] = '\0';
		}

		/* check for comment line */
		if (*pos == '#') {
			continue;
		}

		/* check the first character of the line for + or - */
		operation = *pos++;
		if (operation != '+' && operation != '-') {
			errStk.Push(&SCO_SECDEFS_ERR_MISS, path, lineno);
			errStk.Output(stderr);
			continue;
		}

		/* the next characters are a filename terminated by a ":" */
		file = pos;
		while (*pos && (*pos != ':')) {
			pos++;
		}
		if (*pos == '\0') {
			errStk.Push(&SCO_SECDEFS_ERR_MISS2, path, lineno);
			errStk.Output(stderr);
			continue;
		}
		*pos++ = '\0';

		if (check_name(file) < 0) {
			errStk.Push(&SCO_SECDEFS_ERR_LINE, path,lineno,file);
			errStk.Output(stderr);
			continue;
		}

		/* the fieldname is next terminated by a "=" */
		field = pos;
		while (*pos && *pos != '=') {
			pos++;
		}
		if (*pos == '\0') {
			errStk.Push(&SCO_SECDEFS_ERR_MISS3, path, lineno);
			errStk.Output(stderr);
			continue;
		}

		/* field value is the last string */
		*pos++ = '\0';
		value = pos;

		/* store the values in the list */
		head = store_on_list(head, operation, file, field, value);
	}
	delete buf;
	if (ferror(fp)) {
		(void)fclose(fp);
		errStk.PushUnixErr(errno);
		errStk.Push(&SCO_SECDEFS_ERR_READF, path);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	(void)fclose(fp);
	return(head);
}
		
/*
 * Add a new struct etc_default to the end of the linked list
 * pointed to by head.
 */
static struct etc_default *
store_on_list(
	struct etc_default *head,
	char operation,
	char *file,
	char *field,
	char *value
){
	struct etc_default *entry;
	int found = 0;

	/* the head of the list is a special case */
	if (head == (struct etc_default *)0) {
		head = new_etc_default();
		head->file = newstring(file);
		head->field = newstring(field);
		head->value = newstring(value);
		head->operation = operation;
		return(head);
	}

	for (entry = head;;entry = entry->next_default) {
		if (entry->file != (char *)0 && 
		    strcmp(entry->file,file) == 0 &&
		    entry->field != (char *)0 && 
		    strcmp(entry->field, field) == 0)
		{
			found = 1;
			break;
		}
		if (entry->next_default == (struct etc_default *)0) {
			break;
		}
	}
	if (found) {
		delete entry->value;
		entry->value = newstring(value);
		entry->operation = operation;
	}
	else {
		entry->next_default = new_etc_default();
		entry = entry->next_default;
		entry->file = newstring(file);
		entry->field = newstring(field);
		entry->value = newstring(value);
		entry->operation = operation;
	}
	return(head);
}

/*
 * Allocate space for a new struct etc_default and initialize values.
 */
static struct etc_default *
new_etc_default(
	void /* NOARGS */
){
	struct etc_default *newdef;
	errStatus_cl errStk;

	newdef = new struct etc_default[sizeof(struct etc_default)];
	if (newdef == (struct etc_default *)0) {
		errStk.Push(&SCO_SECDEFS_ERR_MEM2);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	newdef->file = (char *)0;
	newdef->field = (char *)0;
	newdef->value = (char *)0;
	newdef->next_default = (struct etc_default *)0;
	return(newdef);
}

/* 
 * Read <field> from /etc/default/<file>.  Cache values to reduce
 * repeated opens and closes of /etc/default files.
 */
static char *
get_etc_def(
	char *file,
	char *field
){
	static struct etc_default *tail, *head = (struct etc_default *)0;
	struct etc_default *current;

	for (current = head; 
	     current != (struct etc_default *)0; 
	     current = current->next_default) 
	{
		if (strcmp(current->file, file) == 0 &&
		    strcmp(current->field, field) == 0)
		{
			return(current->value);
			
		}
	}
	if (head == (struct etc_default *)0) {
		head = new_etc_default();
		tail = head;
	}
	else {
		tail->next_default = new_etc_default();
		tail = tail->next_default;
	}
	tail->file = file;
	tail->field = field;
	tail->value = newstring(read_def_file(file, field));
	return(tail->value);
}

/* 
 * Helper routine for get_etc_def.  Uses defopen to read
 * <field> from /etc/default/<file>.
 */
static char *
read_def_file(
	char *file,
	char *field
) {
	char *filename, *gotvalue;
	struct stat st;
	errStatus_cl errStk;
	FILE *defp;

	filename = make_path(etc_default_dir, file);
	if (stat(filename, &st) == -1) {
		return((char *)0);
		/* NOTREACHED */
	}

	defp=defopen(file);
	if (defp == NULL) {
		errStk.PushUnixErr(errno);
		errStk.Push(&SCO_SECDEFS_ERR_OPEN, command_name);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	gotvalue = defread(defp, field);
	defclose(defp);
	delete filename;
	return(gotvalue);
}
			
/*
 * Compare package <pkg> against the real defaults and 
 * print the difference.
 */
static void
display_package(
	struct package *pkg,
	int verbose
){
	struct display *longest;
	errStatus_cl errStk;
	intlMgr_cl intlMsg;

	if (!verbose) {
		printf("%s", 
			intlMsg.LocalizeMsg(&SCO_SECDEFS_MSG_DIFF, 
						pkg->name, 
						pkg->count)
		);
		return;
	}
	if (pkg->count == 0) {
		printf("%s", 
			intlMsg.LocalizeMsg(&SCO_SECDEFS_MSG_MATCH, 
						pkg->name, 
						pkg->count)
		);
		return;
	}

	longest = find_longest_strings(pkg->etc_set);
	
	printf("%s", 
	    intlMsg.LocalizeMsg(&SCO_SECDEFS_MSG_LEVEL, pkg->name, pkg->count)
	);

	/* display the etc defaults */
	(void)compare_etc_defaults(pkg->name, pkg->etc_set, longest);

	(void)printf("\n");

	return;
}

/*
 * Find the longest field and value string in a package.
 * Returns a pointer to static struct display.
 */
static struct display *
find_longest_strings(
	struct etc_default *head
){
	struct etc_default *current;
	static struct display longest;
	int l;

	longest.file = strlen(etc_def);
	longest.field = 16;

	for (current = head;
	     current != (struct etc_default *)0;
	     current = current->next_default)
	{
		l = strlen(current->file);
		longest.file = l > longest.file ? l : longest.file;
		l = strlen(current->field);
		longest.field = l > longest.field ? l : longest.field;
	}
	return(&longest);
}

/*
 * Compare package <pkg> against the real defaults.
 */
static void
compare_package(
	struct package *pkg
){

	/*compare the etc defaults */
	pkg->count += compare_etc_defaults(pkg->name, pkg->etc_set, 
                      (struct display *)0);

	return;
}

/*
 * Compare candidate etc defaults against real defaults.
 */
static int
compare_etc_defaults(
	char *name,
	struct etc_default *head,
	struct display *display
){
	int count = 0;
	struct etc_default *current;
	char *real_def;
	intlMgr_cl intlMsg;

	for (current = head; 
	     current != (struct etc_default *)0;
	     current = current->next_default)
	{
		real_def = get_etc_def(current->file, current->field);
		if (real_def == (char *)0 && current->operation != '-') {
			count++;
			display_values(
				name,
				display, 
				current->file, 
				current->field, 
				current->value, 
				(char *)intlMsg.LocalizeMsg(&SCO_SECDEFS_MSG_NOTSET));
		}
		else if (real_def != (char *)0 && current->operation != '+') {
			count++;
			display_values(
				name,
				display, 
				current->file, 
				current->field, 
				(char *)intlMsg.LocalizeMsg(&SCO_SECDEFS_MSG_NOTSET),
				real_def);
		}
		else if (real_def != (char *)0 && 
			 current->operation == '+' &&
			 strcmp(real_def, current->value) != 0) 
		{
			count++;
			display_values(
				name,
				display, 
				current->file, 
				current->field, 
				current->value,
				real_def);
		}
	}
	return(count);
}

/*
 * Prints a value from the package of defaults <dv> and the 
 * corresponding current default value <cf>.  Tries to format the
 * fields in the most readable style.
 */
#define LINELENGTH	80
static void
display_values(
	char *name,
	struct display *longest,
	char *file,
	char *field,
	char *cv,
	char *dv
){
	int i;

	if (longest == (struct display *)0) {
		return;
	}
	
	(void)printf("%s ", file);
	for (i = longest->file - strlen(file); i != 0; i--) {
		(void)putchar(' ');
	}
	(void)printf("%s ", field);
	for (i = longest->field - strlen(field); i != 0; i--) {
		(void)putchar(' ');
	}
	(void)printf("%s", dv);

	/* 
	 * Figure out if the package value should be printed on a
	 * separate line.
	 */
	if (longest->file + 	/* length of the file name */
	    longest->field + 	/* length of the field name */
	    5 + 		/* spaces, brackets and eqauls */
	    strlen(name) + 	/* length of package name */
	    strlen(cv) + 	/* length of default value */
	    strlen(dv) 		/* length of package value */
	    >= LINELENGTH) 
	{
		putchar('\n');
		i = longest->file + longest->field - strlen(name);
		while (--i) {
			putchar(' ');
		}
	} 
	printf(" (%s=%s)\n", name, cv);
}

/*
 * Return -1 if the name supplied is an invalid filename otherwise return 0.
 */
static int
check_name(
	char	*name
){
	/* check a non empty string was supplied */
	if (strlen(name) < 1) {
		return(-1);
	}
	/* check the current directory "." is not specified */
	if (strcmp(name, ".") == 0) {
		return(-1);
	}
	/* ensure name contains no ".." or / characters */
	if (strchr(name, '/') != (char *)0) {
		return(-1);
	}
	while (*name && *(name + 1)) {
		if ((*name == '.') && (*(name + 1) == '.')) {
			return(-1);
		}
		name++;
	}
	return(0);
}

/*
 * Check if <path> exists.
 */
static void
check_exists(
	char *path
){
	struct stat st;
	errStatus_cl errStk;

	if (stat(path, &st) == -1) {
	    if (errno == EACCES) {
		errStk.Push(&SCO_SECDEFS_ERR_SU);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
	    } else {
		errStk.PushUnixErr(errno);
		errStk.Push(&SCO_SECDEFS_ERR_FIND, path);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
	    }
	    /* NOTREACHED */
	}
}

/*
 * Check <path> exists and is readable.
 */
static void
check_read(
	char *path
){
	errStatus_cl errStk;

	check_exists(path);
	if (access(path, R_OK) < 0) {
		errStk.PushUnixErr(errno);
		errStk.Push(&SCO_SECDEFS_ERR_READ2, path);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
}


/*
 * Return a pointer to a new string created by appending 
 * <file> to <directory>.
 */
static char     *
make_path(
	char    *directory,
	char    *file
){
	char    *path;
	errStatus_cl errStk;

	path = new char[strlen(directory) + strlen(file) + 1];
	if (path == (char *)0) {
		errStk.Push(&SCO_SECDEFS_ERR_MEM2);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	(void)strcpy(path, directory);
	(void)strcat(path, file);
	return(path);
}

/* 
 * Strdup with memory check.
 */
static char *
newstring(
	char *string
){
	char *dup;
	errStatus_cl errStk;

	if (string == (char *)0) {
		return((char *)0);
	}

	dup = (char *)strdup(string);
	if (dup == (char *)0) {
		errStk.Push(&SCO_SECDEFS_ERR_MEM2);
		errStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	return(dup);
}
