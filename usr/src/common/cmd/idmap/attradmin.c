/*		copyright	"%c%" 	*/

#ident	"@(#)attradmin.c	1.3"
#ident  "$Header$"

/*
 * attradmin is a system administrative interface to the attribute mapping
 * database.  Only priveleged users/administrators may use attradmin.
 *
 * Synopsis
 *
 *	attradmin [ -A attrname [ -l localval ] ]
 *	attradmin -A attrname -a -l localval -r remoteval
 *	attradmin -A attrname -d -l localval [ -r remoteval ]
 *	attradmin -A attrname -I descr
 *	attradmin -A attrname [ -Dcf ]
 *
 */


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <pfmt.h>
#include <locale.h>
#include <mac.h>
#include "idmap.h"

#define	OPTIONS		"A:l:r:adI:Dcf"

#define	ACT_LIST	0
#define	ACT_ADD		1
#define	ACT_DELETE	2
#define	ACT_INSTALL	3
#define	ACT_UNINSTALL	4
#define	ACT_CHECK	7
#define	ACT_FIX		8


static	FILE *logfile;

extern	int	setlabel();
extern	int	fsync();
extern	int	gettimeofday();
extern	void	*malloc();
extern	void	free();
extern	void	qsort();
extern	struct passwd	*getpwnam();
extern	struct group	*getgrnam();
extern	mode_t	umask();
extern	int	lvlin();
extern	int	lvlfile();

extern	int	breakname();
extern	int	namecmp();


static void
log_cmd(argc, argv)
int	argc;
char	*argv[];
{
	struct timeval	t;
	struct passwd	*pwd;
	struct group	*grp;

	if (access(LOGFILE, F_OK) != 0) {
		(void) close(creat(LOGFILE, LOGFILE_MODE));
		pwd = getpwnam(IDMAP_LOGIN);
		grp = getgrnam(IDMAP_GROUP);
		(void) chown(LOGFILE, pwd->pw_uid, grp->gr_gid);
	}

	if ((logfile = fopen(LOGFILE, "a")) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			    ":58:%s: Cannot open logfile\n", LOGFILE);
		exit(1);
	}

	(void) gettimeofday(&t, NULL);
	fprintf(logfile, "%s", ctime(&t.tv_sec));
	while (argc--)
		fprintf(logfile, "%s ", *argv++);
	fprintf(logfile, "\nUID = %d GID = %d SUCC = ",
		(int) getuid(), (int) getgid());
}


static void
log_succ()
{
	fprintf(logfile, "+\n\n");
	(void) fclose(logfile);
}


static void
log_fail()
{
	fprintf(logfile, "-\n\n");
	(void) fclose(logfile);
}


static void
list_attr(attr)
char	*attr;
{
	char	filename[MAXFILE];	/* map file name */
	FILE	*fp;			/* map file stream pointer */
	char	descr[MAXLINE];		/* remote attribute descriptor */
	char	remote[MAXLINE];	/* remote attribute value */
	char	local[MAXLINE];		/* local attribute value */

	/* open attribute file */
	sprintf(filename, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);
	if ((fp = fopen(filename, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":59:%s: No such attribute\n",
			    attr);
		exit(1);
	}

	/* print the record descriptor (skip the !) */
	(void) fgets(descr, MAXLINE, fp);
	printf("%s", descr+1);

	/* print all records */
	while (fscanf(fp, "%s %s\n", remote, local) == 2)
		printf("%s %s\n", remote, local);

	(void) fclose(fp);
}


static void
list_all()
{
	char	dirname[MAXFILE];
	DIR	*dirp;
	struct dirent	*dp;
	char	filename[MAXFILE];

	sprintf(dirname, "%s/%s", MAPDIR, ATTRMAP);
	if ((dirp = opendir(dirname)) == NULL)
		return;

	/* skip over . and .. */
	(void) readdir(dirp);
	(void) readdir(dirp);
	while ((dp = readdir(dirp)) != NULL) {
		sprintf(filename, "%s/%s/%s", MAPDIR, ATTRMAP, dp->d_name);
		if (access(filename, F_OK) == 0) {
			/* cover up the trailing .map */
			dp->d_name[strlen(dp->d_name) - strlen(DOTMAP)] = '\0';
			printf("%s\n", dp->d_name);
		}
	}
	(void) closedir(dirp);
}


static void
list_attr_value(attr, value)
char	*attr;
char	*value;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	descr[MAXLINE];		/* remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	FILE	*fp;			/* map file stream pointer */

	/* open attribute map file */
	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":59:%s: No such attribute\n",
			    attr);
		exit(1);
	}

	/* get record descriptor */
	(void) fgets(descr, MAXLINE, fp);

	/* print appropriate records */
	while (fscanf(fp, "%s %s\n", remote, local) == 2) {
		if (strcmp(local, value) == 0)
			printf("%s %s\n", remote, local);
	}

	(void) fclose(fp);
}


static void
add(attr, g_value, value)
char	*attr;		/* attribute name */
char	*g_value;	/* global/remote value */
char	*value;		/* local value */
{
	char	mapfile[MAXFILE];	/* map file name */
	char	tmpfile[MAXFILE];	/* temp map file name */
	char	descr[MAXLINE];		/* remote name descriptor */
	char	descr2[MAXLINE];	/* copy of remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	remote2[MAXLINE];	/* copy of remote name */
	char	local[MAXLINE];		/* local name */
	char	g_value2[MAXLINE];	/* copy of g_value */
	FILE	*fp, *fp2;		/* map file stream pointers */
	int	passed = 0;		/* insert location flag */
	int	eof = 0;		/* end of file flag */
	int	sr;			/* scanf return code */
	FIELD	r_fields[MAXFIELDS];	/* remote name field info */
	FIELD	g_fields[MAXFIELDS];	/* global name field info */
	FIELD	d_fields[MAXFIELDS];	/* descriptor field info */
	struct passwd	*pwd;
	struct group	*grp;
	int	i;			/* general counter */
	level_t	lvlno;			/* file level number */
	int	tmpfd;			/* temporary fd */

	/* open attribute map file */
	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":59:%s: No such attribute\n",
			    attr);
		log_fail();
		exit(1);
	}

	/* create temp file */
	(void) strcpy(tmpfile, mapfile);
	(void) strcat(tmpfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, ATTR_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(ATTR_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((fp2 = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	/* get record descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		(void) fclose(fp2);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	/* copy it */
	(void) fputs(descr, fp2);

	/* find place to insert */
	(void) strcpy(g_value2, g_value);
	if (breakname(g_value2, descr, g_fields) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":63:mandatory field(s) missing\n");
		(void) fclose(fp);
		(void) fclose(fp2);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}


	/* check that a %x field is present in global name */
	if (value[0] == '%')
		if ((value[1] != 'i') &&
		    (strlen(g_fields[value[1] - '0'].value) == 0)) {
			(void) pfmt(stderr, MM_ERROR,
				    ":64:Return field missing\n");
			(void) fclose(fp);
			(void) fclose(fp2);
			(void) unlink(tmpfile);
			log_fail();
			exit(1);
		}

	(void) strcpy(descr2, descr+1);
	(void) breakname(descr2, descr, d_fields);

	while (!passed && !eof) {
		sr = fscanf(fp, "%s %s\n", remote, local);

		/* check for duplicates */
		if (strcmp(g_value, remote) == 0) {
			(void) fclose(fp);
			(void) fclose(fp2);
			(void) unlink(tmpfile);
			if (strcmp(value, local) == 0) {

			/* exact duplicate (local and remote names ) */
				(void) pfmt(stdout, MM_WARNING,
					    ":65:entry already in file\n");
				log_succ();
				exit(0);

			} else {

			/* local name was not the same */
				(void) pfmt(stderr, MM_ERROR,
					    ":66:remote name already in file\n");
				log_fail();
				exit(1);
			}
		}

		(void) strcpy(remote2, remote);
		switch (sr) {
		case EOF:
			eof++;
			break;
		case 2:
			(void) breakname(remote2, descr, r_fields);
			if (namecmp(r_fields, g_fields) > 0)
				passed++;
			else
				fprintf(fp2, "%s %s\n", remote, local);
			break;
		default: {
				(void) pfmt(stderr, MM_ERROR,
					    ":67:%s: format error in file\n",
					    mapfile);
				(void) fclose(fp);
				(void) fclose(fp2);
				(void) unlink(tmpfile);
				log_fail();
				exit(1);
			}
		}
	}

	/* write new record */
	fprintf(fp2, "%s %s\n", g_value, value);

	/* copy after insert */
	if (!eof) {
		fprintf(fp2, "%s %s\n", remote, local);
		while (fscanf(fp, "%s %s\n", remote, local) != EOF)
			fprintf(fp2, "%s %s\n", remote, local);
	}

	(void) fclose(fp);
	(void) fflush(fp2);
	(void) fsync((int) fileno(fp2));
	(void) fclose(fp2);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
delete(attr, g_value, value)
char	*attr;
char	*g_value;
char	*value;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	tmpfile[MAXFILE];	/* temp file name */
	char	descr[MAXLINE];		/* remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	FILE	*fp, *fp2;		/* map file stream pointers */
	struct passwd	*pwd;
	struct group	*grp;
	level_t	lvlno;			/* file level number */
	int	tmpfd;			/* temporary fd */
	int	delete_happened = 0;	/* has a delete happened? */

	/* open data file */
	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":59:%s: No such attribute\n",
			    attr);
		log_fail();
		exit(1);
	}

	/* create temp file */
	(void) strcpy(tmpfile, mapfile);
	(void) strcat(tmpfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, ATTR_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(ATTR_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((fp2 = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	/* get record descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		(void) fclose(fp2);
		log_fail();
		exit(1);
	}

	/* copy it */
	(void) fputs(descr, fp2);

	/* copy file while deleting entries */
	while (fscanf(fp, "%s %s\n", remote, local) == 2) {
		if ((strcmp(local, value) != 0) ||
		    ((g_value != NULL) && (strcmp(remote, g_value) != 0)))
			fprintf(fp2, "%s %s\n", remote, local);
		else {
			printf("%s %s\n", remote, local);
			delete_happened++;
		}
	}

	if (!delete_happened) {
		(void) pfmt(stdout, MM_WARNING, ":69:Entry not found\n");
	}

	(void) fclose(fp);
	(void) fflush(fp2);
	(void) fsync((int) fileno(fp2));
	(void) fclose(fp2);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
install(attr, descr)
char	*attr;
char	*descr;
{
	char	mapfile[MAXFILE];
	FILE	*mf;
	struct passwd	*pwd;
	struct group	*grp;
	int	descrlen;
	int	descrerr = 0;
	int	i;
	int	fieldused[MAXFIELDS];
	level_t	lvlno;			/* file level number */
	int	tmpfd;			/* temporary fd */

	/* check descriptor */
	descrlen = strlen(descr);
	if ((descrlen % 3) != 2)
		descrerr++;
	for (i = 0; i < MAXFIELDS; i++)
		fieldused[i] = 0;
	for (i = 0; i < descrlen; i += 3) {
		if ((descr[i] != 'M') && (descr[i] != 'O'))
			descrerr++;
		if ((descr[i+1] < '0') || (descr[i+1] > '9'))
			descrerr++;
		if (!descrerr)
			if (fieldused[descr[i+1] - '0'])
				descrerr++;
			else
				fieldused[descr[i+1] - '0']++;
	}
	if (descrerr) {
		(void) pfmt(stderr, MM_ERROR, ":70:Bad descriptor\n");
		log_fail();
		exit(1);
	}

	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);

	/* check if exists */
	if (access(mapfile, F_OK) == 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":71:%s: Attribute already exists\n", attr);
		log_fail();
		exit(1);
	}

	/* create attribute map file */
	if ((tmpfd = creat(mapfile, ATTR_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    mapfile);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(ATTR_LEVEL, &lvlno);
	(void) lvlfile(mapfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((mf = fopen(mapfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    mapfile);
		log_fail();
		exit(1);
	}
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(mapfile, pwd->pw_uid, grp->gr_gid);

	/* write descriptor */
	fprintf(mf, "!%s\n", descr);
	(void) fclose(mf);

	log_succ();
}


static void
uninstall(attr)
char	*attr;
{
	char mapfile[MAXFILE];

	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);

	if (access(mapfile, F_OK) != 0) {
		(void) pfmt(stderr, MM_ERROR, ":59:%s: No such attribute\n",
			    attr);
		log_fail();
		exit(1);
	}

	if (unlink(mapfile) != 0) {
		(void) pfmt(stderr, MM_ERROR, ":72:%s: Cannot remove file\n",
			    mapfile);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
check(attr)
char	*attr;
{
	char mapfile[MAXFILE];	/* map file name */
	char descr[MAXLINE];	/* field descriptor */
	char mapline[MAXLINE];	/* a line from the map file */
	char prevmapline[MAXLINE];/* previous line */
	FILE *fp;		/* map file stream pointer */
	int numerrors = 0;	/* number of errors found */
	int linenum = 1;	/* line number */
	int sr;			/* scanf return code */
	char remote[MAXLINE];	/* remote name */
	char local[MAXLINE];	/* local name */
	int cr;			/* check return code */

	/* open attribute map file */
	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":59:%s: No such attribute\n",
			    attr);
		exit(1);
	}

	/* get descriptor and check it */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		exit(1);
	}

	descr[strlen(descr)-1] = '\0';

	if ((*descr != '!') || (check_descr(&descr[1]) < 0)) {
		pfmt(stdout, MM_NOSTD, ":86:Bad descriptor in attribute map file\n");
		(void) fclose(fp);
		exit(1);
	}

	descr[strlen(descr)] = '\n';

	/* read and check the file */
	while (fgets(mapline, MAXLINE, fp) != NULL) {

		linenum++;

		/*
		 * check syntax errors, missing fields,
		 * duplicate entries, and entries out of order
		 */

		if ((cr = check_entry(descr,
				      (linenum == 2)? NULL:prevmapline,
				      mapline)) != 0) {

			pfmt(stdout, MM_NOSTD, ":87:Error on line number %d: ", linenum);

			switch(cr) {
			case IE_SYNTAX:
				pfmt(stdout, MM_NOSTD, ":88:Syntax error\n");
				break;
			case IE_MANDATORY:
				pfmt(stdout, MM_NOSTD, ":89:Mandatory field missing\n");
				break;
			case IE_DUPLICATE:
				pfmt(stdout, MM_NOSTD, ":90:Duplicate entry\n");
				break;
			case IE_ORDER:
				pfmt(stdout, MM_NOSTD, ":91:Line out of order\n");
				break;
			case IE_NOFIELD:
				pfmt(stdout, MM_NOSTD, ":92:Bad transparent mapping\n");
				break;
			}
			numerrors++;
		}

		(void) strncpy(prevmapline, mapline, MAXLINE);
	}
	if (numerrors) {
		if (numerrors == 1)
			pfmt(stdout, MM_NOSTD, ":93:1 error found in attribute map\n");
		else
			pfmt(stdout, MM_NOSTD, ":94:%d errors found in attribute map\n", numerrors);
	}
	else
		pfmt(stdout, MM_NOSTD, ":95:No errors in attribute map\n");
	(void) fclose(fp);
}


typedef struct {
	char line[MAXLINE];
	char rem[MAXLINE];
	FIELD flds[MAXFIELDS];
} MAPTABLE;


static int
mapcompar(name1, name2)
MAPTABLE **name1, **name2;
{
	MAPTABLE *n1 = *name1, *n2 = *name2;

	return (namecmp(n1->flds, n2->flds));
}


static void
fix(attr)
char	*attr;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	tmpfile[MAXFILE];	/* tmp file name */
	char	descr[MAXLINE];		/* field descriptor */
	char	mapline[MAXLINE];	/* a line from the map file */
	char	prevmapline[MAXLINE];	/* previous line */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	char	response[MAXLINE];	/* a line from the user */
	char	*def;			/* default response */
	char	n1[MAXLINE],		/* name pieces */
		n2[MAXLINE],
		n3[MAXLINE];		/* extra piece */
	FILE	*fp;			/* map file stream pointer */
	FILE	*tmpfp;			/* tmp map file stream pointer */
	int	numerrors = 0;		/* number of errors found */
	int	numfixes = 0;		/* number of fixes made */
	int	linenum = 1;		/* line number */
	int	sr;			/* scanf return code */
	MAPTABLE *maptable;		/* mapping file table */
	MAPTABLE **mapptrs;		/* pointers into mapping table */
	int	entry;			/* entry number */
	int	entries = 0;		/* number of entries in mapping file */
	int	i;
	struct passwd	*pwd;
	struct group	*grp;
	level_t	lvlno;			/* file level number */
	int	cr;			/* check() return code */
	int	tmpfd;			/* temporary fd */

	/* open attribute map file */
	sprintf(mapfile, "%s/%s/%s%s", MAPDIR, ATTRMAP, attr, DOTMAP);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":59:%s: No such attribute\n",
			    attr);
		log_fail();
		exit(1);
	}

	/* create temp file */
	sprintf(tmpfile, "%s%s", mapfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, ATTR_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(ATTR_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((tmpfp = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	/* get descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		(void) fclose(tmpfp);
		(void) unlink(tmpfile);
		exit(1);
	}

	descr[strlen(descr)-1] = '\0';

	while ((*descr != '!') || (check_descr(&descr[1]) < 0)) {
		pfmt(stdout, MM_NOSTD, ":96:Bad descriptor in attribute map file:\n%s\n", descr);
		pfmt(stdout, MM_NOSTD, ":97:Type c to change, a to abort (default c) ");
		(void) fgets(response, sizeof(response), stdin);
		response[strlen(response)-1] = '\0';
		if ((*response == '\0') || (*response == 'c')) {
			pfmt(stdout, MM_NOSTD, ":98:New descriptor (include leading !): ");
			(void) fgets(descr, sizeof(descr), stdin);
			descr[strlen(descr)-1] = '\0';
		} else if (*response == 'a') {
			pfmt(stdout, MM_NOSTD, ":99:Aborting fix...\n");
			(void) fclose(fp);
			(void) fclose(tmpfp);
			(void) unlink(tmpfile);
			exit(1);
		}
	}

	descr[strlen(descr)] = '\n';

	/* copy descriptor */
	(void) fputs(descr, tmpfp);

	/* read and check the file */
	while (fgets(mapline, MAXLINE, fp) != NULL) {

		linenum++;

		if ((cr = check_entry(descr,
				      (linenum == 2)? NULL:prevmapline,
				      mapline)) != 0) {

			pfmt(stdout, MM_NOSTD, ":87:Error on line number %d: ", linenum);

			switch(cr) {
			case IE_SYNTAX:
				pfmt(stdout, MM_NOSTD, ":100:Syntax error:\n");
				break;
			case IE_MANDATORY:
				pfmt(stdout, MM_NOSTD, ":101:Mandatory field missing:\n");
				break;
			case IE_DUPLICATE:
				pfmt(stdout, MM_NOSTD, ":102:Duplicate entry:\n");
				break;
			case IE_ORDER:
				pfmt(stdout, MM_NOSTD, ":103:Line out of order:\n");
				break;
			case IE_NOFIELD:
				pfmt(stdout, MM_NOSTD, ":104:Bad transparent mapping:\n");
				break;
			}
			pfmt(stdout, MM_NOGET|MM_NOSTD, "%s", mapline);
			numerrors++;

			pfmt(stdout, MM_NOSTD, ":105:Type c to change, s to skip, d to delete (default c) ");
			(void) fgets(response,sizeof(response),stdin);
			response[strlen(response)-1] = '\0';
			if ((*response == '\0') || (*response == 'c')) {
				pfmt(stdout, MM_NOSTD, ":106:New remote value: ");
				(void) fgets(remote,sizeof(remote),stdin);
				remote[strlen(remote)-1] = '\0';
				pfmt(stdout, MM_NOSTD, ":107:New local value: ");
				(void) fgets(local,sizeof(local),stdin);
				local[strlen(local)-1] = '\0';
				fprintf(tmpfp, "%s %s\n", remote, local);
				entries++;
				pfmt(stdout, MM_NOSTD, ":108:new entry inserted\n");
				numfixes++;
			} else if (*response == 'd') {
				pfmt(stdout, MM_NOSTD, ":109:entry deleted\n");
				numfixes++;
			} else {
				(void) fputs(mapline, tmpfp);
				entries++;
				pfmt(stdout, MM_NOSTD, ":110:entry skipped\n");
			}
		} else {
			(void) fputs(mapline, tmpfp);
			entries++;
		}
		(void) strncpy(prevmapline, mapline, MAXLINE);
	}
	if (numerrors) {
		if (numerrors == 1)
			pfmt(stdout, MM_NOSTD, ":93:1 error found in attribute map\n", numerrors);
		else
			pfmt(stdout, MM_NOSTD, ":94:%d errors found in attribute map\n", numerrors);
		pfmt(stdout, MM_NOSTD, ":111:%d of them fixed\n", numfixes);
	} else
		pfmt(stdout, MM_NOSTD, ":95:No errors in attribute map\n");
	(void) fclose(fp);
	(void) fflush(tmpfp);
	(void) fsync((int) fileno(tmpfp));
	(void) fclose(tmpfp);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	/* check and fix the order of entries in the file */

	/* open attribute file */
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    mapfile);
		log_fail();
		exit(1);
	}

	/* get record descriptor */
	if (fgets(descr, MAXLINE, fp) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":62:%s: Cannot read from file\n",
			    mapfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}

	/* allocate required space for the mapping file */
	maptable = (MAPTABLE *) malloc(sizeof(MAPTABLE) * entries);
	mapptrs = (MAPTABLE **) malloc(sizeof(MAPTABLE *) * entries);

	/* read map file into table */
	entry = 0;
	while(fgets(maptable[entry].line, MAXLINE, fp) != NULL) {
		(void) sscanf(maptable[entry].line, "%s %s", remote, local);
		(void) strcpy(maptable[entry].rem, remote);
		(void) breakname(maptable[entry].rem, descr, maptable[entry].flds);
		mapptrs[entry] = &maptable[entry];
		entry++;
	}
	(void) fclose(fp);

	/* sort the map table in memory */
	qsort((void *) mapptrs, (unsigned int) entry, sizeof(char *),
	      mapcompar);

	/* write the sorted map back out */
	/* create temp file */
	if ((tmpfd = creat(tmpfile, ATTR_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(ATTR_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

	/* open temp file */
	if ((tmpfp = fopen(tmpfile, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    tmpfile);
		log_fail();
		exit(1);
	}
	(void) chown(tmpfile, pwd->pw_uid, grp->gr_gid);

	(void) fputs(descr, tmpfp);
	for (i = 0; i < entry; i++)
		(void) fputs(mapptrs[i]->line, tmpfp);

	(void) fflush(tmpfp);
	(void) fsync((int) fileno(tmpfp));
	(void) fclose(tmpfp);

	free(maptable);
	free(mapptrs);

	/* replace the old file with the temp file */
	if (rename(tmpfile, mapfile) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":68:%s: Cannot replace file\n",
			    mapfile);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
usage(prog, file)
char	*prog;
FILE	*file;
{
	pfmt(file, MM_ERROR, ":73:Syntax\n");
	pfmt(file, MM_ACTION, ":74:%s: Usage:\n\t%s [ -A attrname [ -l localval ] ]\n\t%s -A attrname -a -l localval -r remoteval\n\t%s -A attrname -d -l localval [ -r remoteval ]\n\t%s -A attrname -I descr\n\t%s -A attrname [ -Dcf ]\n", prog, prog, prog, prog, prog, prog);
}


main(argc, argv)
int	argc;
char	*argv[];
{
	extern int getopt();
	int c;
	extern char *optarg;
	int opterror = 0;
	int action = ACT_LIST;
	char *attr, *value, *g_value, *descr;

	(void) umask((mode_t) 0000);

/* set up error message handling */

	(void) setlocale(LC_MESSAGES, "");
	(void) setlabel("UX:attradmin");
	(void) setcat("uxnsu");

/* parse command line */

	attr = NULL;
	value = NULL;
	g_value = NULL;
	descr = NULL;

	while ((c = getopt(argc, argv, OPTIONS)) != EOF)
		switch(c) {
		case 'A':
			attr = optarg;
			break;
		case 'l':
			value = optarg;
			break;
		case 'r':
			g_value = optarg;
			break;
		case 'a':
			if (action)
				opterror++;
			else {
				action = ACT_ADD;
			}
			break;
		case 'd':
			if (action)
				opterror++;
			else {
				action = ACT_DELETE;
			}
			break;
		case 'I':
			if (action)
				opterror++;
			else {
				action = ACT_INSTALL;
				descr = optarg;
			}
			break;
		case 'D':
			if (action)
				opterror++;
			else {
				action = ACT_UNINSTALL;
			}
			break;
		case 'c':
			if (action)
				opterror++;
			else {
				action = ACT_CHECK;
			}
			break;
		case 'f':
			if (action)
				opterror++;
			else {
				action = ACT_FIX;
			}
			break;
		case '?':
			opterror++;
			break;
		}

	if (opterror) {
		usage(argv[0], stderr);
		exit(1);
	}

/* log command line (initial) */
	if ((action != ACT_LIST) && (action != ACT_CHECK))
		log_cmd(argc, argv);

	switch(action) {
	case ACT_LIST:
		if (g_value != NULL)
			opterror++;
		else {
			if (attr != NULL) {
				if (value != NULL)
					list_attr_value(attr, value);
				else
					list_attr(attr);
			} else {
				list_all();
			}
		}
		break;
	case ACT_ADD:
		if ((attr == NULL) || (value == NULL) || (g_value == NULL))
			opterror++;
		else {
			add(attr, g_value, value);
		}
		break;
	case ACT_DELETE:
		if ((attr == NULL) || (value == NULL))
			opterror++;
		else {
			delete(attr, g_value, value);
		}
		break;
	case ACT_INSTALL:
		if ((attr == NULL) || (descr == NULL) ||
		    (value != NULL) || (g_value != NULL))
			opterror++;
		else {
			install(attr, descr);
		}
		break;
	case ACT_UNINSTALL:
		if ((attr == NULL) || (value != NULL) || (g_value != NULL))
			opterror++;
		else {
			uninstall(attr);
		}
		break;
	case ACT_CHECK:
		if ((attr == NULL) || (value != NULL) || (g_value != NULL))
			opterror++;
		else {
			check(attr);
		}
		break;
	case ACT_FIX:
		if ((attr == NULL) || (value != NULL) || (g_value != NULL))
			opterror++;
		else {
			fix(attr);
		}
		break;
	}

	if (opterror) {
		usage(argv[0], stderr);
		log_fail();
		exit(1);
	}
	return(0);
}
