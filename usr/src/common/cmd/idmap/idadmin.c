/*		copyright	"%c%" 	*/

#ident	"@(#)idadmin.c	1.3"
#ident  "$Header$"

/*
 * idadmin is a system administrative interface to the name mapping
 * database.  Only priveleged users/administrators may use idadmin.
 *
 * Synopsis
 *
 *	idadmin [ -S scheme [ -l logname ] ]
 *	idadmin -S scheme -a -l logname -r g_name
 *	idadmin -S scheme -d -l logname [ -r g_name ]
 *	idadmin -S scheme -I descr
 *	idadmin -S scheme [ -Duscf ]
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

#define	OPTIONS		"S:l:r:adI:Duscf"

#define	ACT_LIST	0
#define	ACT_ADD		1
#define	ACT_DELETE	2
#define	ACT_INSTALL	3
#define	ACT_UNINSTALL	4
#define	ACT_USER	5
#define	ACT_SECURE	6
#define	ACT_CHECK	7
#define	ACT_FIX		8


static	FILE *logfile;

extern	int	setlabel();
extern	int	fsync();
extern	int	gettimeofday();
extern	int	getopt();
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
	struct timeval t;
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
	} else {
		(void) gettimeofday(&t, NULL);
		fprintf(logfile, "%s", ctime(&t.tv_sec));
		while (argc--)
			fprintf(logfile, "%s ", *argv++);
		fprintf(logfile, "\nUID = %d GID = %d SUCC = ",
			(int) getuid(), (int) getgid());
	}
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
list_scheme(scheme)
char	*scheme;
{
	char	filename[MAXFILE];	/* map file name */
	FILE	*fp;			/* map file stream pointer */
	char	descr[MAXLINE];		/* remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */

	if (strcmp(scheme, ATTRMAP) == 0)
		return;

	/* open idata file */
	sprintf(filename, "%s/%s/%s", MAPDIR, scheme, IDATA);
	if ((fp = fopen(filename, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
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
	DIR	*dirp;
	struct dirent	*dp;
	char	filename[MAXFILE];
	struct stat	stat_buf;
	u_short	mode;

	if ((dirp = opendir(MAPDIR)) == NULL)
		return;

	/* skip over . and .. */
	(void) readdir(dirp);
	(void) readdir(dirp);
	while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ATTRMAP) == 0)
			continue;
		sprintf(filename, "%s/%s/%s", MAPDIR, dp->d_name, UIDATA);
		if (stat(filename, &stat_buf) == 0) {
			mode = stat_buf.st_mode & (~S_IFMT);
			printf("%-16.16s ", dp->d_name);
			if (mode != 0)
				pfmt(stdout, MM_NOSTD, ":112:USER mode\n");
			else
				pfmt(stdout, MM_NOSTD, ":113:SECURE mode\n");
		}
	}
	(void) closedir(dirp);
}


static void
list_logname(scheme, logname)
char	*scheme;
char	*logname;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	descr[MAXLINE];		/* remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	local[MAXLINE];		/* local name */
	FILE	*fp;			/* map file stream pointer */

	if ((*logname != '%') &&
	    (getpwnam(logname) == (struct passwd *) NULL)) {
		(void) pfmt(stdout, MM_WARNING, ":76:%s: No such user\n",
			    logname);
	}

	/* open idata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, IDATA);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		exit(1);
	}

	/* get record descriptor */
	(void) fgets(descr, MAXLINE, fp);

	/* print appropriate records */
	while (fscanf(fp, "%s %s\n", remote, local) == 2) {
		if (strcmp(local, logname) == 0)
			printf("%s %s\n", remote, local);
	}

	(void) fclose(fp);
}


static void
add(scheme, g_name, logname)
char	*scheme;
char	*g_name;
char	*logname;
{
	char	mapfile[MAXFILE];	/* map file name */
	char	tmpfile[MAXFILE];	/* temp map file name */
	char	descr[MAXLINE];		/* remote name descriptor */
	char	descr2[MAXLINE];	/* copy of remote name descriptor */
	char	remote[MAXLINE];	/* remote name */
	char	remote2[MAXLINE];	/* copy of remote name */
	char	local[MAXLINE];		/* local name */
	char	g_name2[MAXLINE];	/* copy of gname */
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

	if ((*logname != '%') &&
	    (getpwnam(logname) == (struct passwd *) NULL)) {
		(void) pfmt(stdout, MM_WARNING, ":76:%s: No such user\n",
			    logname);
	}

	/* open idata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, IDATA);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	/* create temp file */
	(void) strcpy(tmpfile, mapfile);
	(void) strcat(tmpfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, IDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(IDATA_LEVEL, &lvlno);
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

	/* check that all fields are there */

	(void) strcpy(g_name2, g_name);
	if (breakname(g_name2, descr, g_fields) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			   ":63:mandatory field(s) missing\n");
		(void) fclose(fp);
		(void) fclose(fp2);
		(void) unlink(tmpfile);
		log_fail();
		exit(1);
	}

	/* check that a %x field is present in global name */
	if (logname[0] == '%')
		if ((logname[1] != 'i') &&
		    (strlen(g_fields[logname[1] - '0'].value) == 0)) {
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
		if (strcmp(g_name, remote) == 0) {
			(void) fclose(fp);
			(void) fclose(fp2);
			(void) unlink(tmpfile);
			if (strcmp(logname, local) == 0) {

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
	fprintf(fp2, "%s %s\n", g_name, logname);

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
delete(scheme, g_name, logname)
char	*scheme;
char	*g_name;
char	*logname;
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
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, IDATA);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	/* create temp file */
	(void) strcpy(tmpfile, mapfile);
	(void) strcat(tmpfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, IDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(IDATA_LEVEL, &lvlno);
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
		if ((strcmp(local, logname) != 0) ||
		    ((g_name != NULL) && (strcmp(remote, g_name) != 0)))
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
install(scheme, descr)
char	*scheme;
char	*descr;
{
	char	dirname[MAXFILE];
	char	idata[MAXFILE];
	char	uidata[MAXFILE];
	FILE	*ifp, *uifp;
	struct passwd	*pwd;
	struct group	*grp;
	level_t	lvlno;			/* file level number */
	int	tmpfd;			/* temporary fd */

	/* check descriptor */
	if (check_descr(descr) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":70:Bad descriptor\n");
		log_fail();
		exit(1);
	}

	/* check if name == attrmap */
	if (strcmp(scheme, ATTRMAP) == 0) {
		(void) pfmt(stderr, MM_ERROR, ":77:%s: Bad scheme name\n",
			    scheme);
		log_fail();
		exit(1);
	}

	/* check if exists */
	sprintf(dirname, "%s/%s", MAPDIR, scheme);
	if (access(dirname, F_OK) == 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":78:%s: Scheme already exists\n", scheme);
		log_fail();
		exit(1);
	}

	/* create dir */
	if (mkdir(dirname, DIR_MODE) != 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":79:%s: Cannot make directory\n", dirname);
		log_fail();
		exit(1);
	}
	(void) lvlin(DIR_LEVEL, &lvlno);
	(void) lvlfile(dirname, MAC_SET, &lvlno);
	pwd = getpwnam(IDMAP_LOGIN);
	grp = getgrnam(IDMAP_GROUP);
	(void) chown(dirname, pwd->pw_uid, grp->gr_gid);
	
	/* create idata */
	sprintf(idata, "%s/%s", dirname, IDATA);
	if ((tmpfd = creat(idata, IDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    idata);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(IDATA_LEVEL, &lvlno);
	(void) lvlfile(idata, MAC_SET, &lvlno);

	/* open idata */
	if ((ifp = fopen(idata, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":61:%s: Cannot open file\n",
			    idata);
		log_fail();
		exit(1);
	}

	(void) chown(idata, pwd->pw_uid, grp->gr_gid);

	/* write descriptor */
	fprintf(ifp, "!%s\n", descr);
	(void) fclose(ifp);

	/* create uidata */
	sprintf(uidata, "%s/%s", dirname, UIDATA);
	if ((tmpfd = creat(uidata, UIDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    idata);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(UIDATA_LEVEL, &lvlno);
	(void) lvlfile(uidata, MAC_SET, &lvlno);

	if ((uifp = fopen(uidata, "w+")) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			    ":61:%s: Cannot open file\n", uidata);
		log_fail();
		exit(1);
	}

	/* set secure mode */
	(void) chmod(uidata, SECURE_MODE);
	(void) chown(uidata, pwd->pw_uid, grp->gr_gid);

	/* write descriptor */
	fprintf(uifp, "!%s\n", descr);
	(void) fclose(uifp);
	log_succ();
}


static void
uninstall(scheme)
char	*scheme;
{
	char dirname[MAXFILE];
	char idata[MAXFILE];
	char uidata[MAXFILE];

	if (strcmp(scheme, ATTRMAP) == 0) {
		(void) pfmt(stderr, MM_ERROR, ":77:%s: Bad scheme name\n",
			    scheme);
		log_fail();
		exit(1);
	}
	sprintf(dirname, "%s/%s", MAPDIR, scheme);

	if (access(dirname, F_OK) != 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":75:%s: No such scheme\n", scheme);
		log_fail();
		exit(1);
	}
	sprintf(idata, "%s/%s", dirname, IDATA);
	if (unlink(idata) != 0) {
		(void) pfmt(stdout, MM_WARNING,
			    ":72:%s: Cannot remove file\n", idata);
	}

	sprintf(uidata, "%s/%s", dirname, UIDATA);
	if (unlink(uidata) != 0) {
		(void) pfmt(stdout, MM_WARNING,
			    ":72:%s: Cannot remove file\n", uidata);
	}

	if (rmdir(dirname) != 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":80:%s: Cannot remove directory\n", dirname);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
user(scheme)
char	*scheme;
{
	char uidata[MAXFILE];

	sprintf(uidata, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if (chmod(uidata, UIDATA_MODE) != 0) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
secure(scheme)
char	*scheme;
{
	char uidata[MAXFILE];

	sprintf(uidata, "%s/%s/%s", MAPDIR, scheme, UIDATA);
	if (chmod(uidata, SECURE_MODE) != 0) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	log_succ();
}


static void
check(scheme)
char	*scheme;
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

	/* open idata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, IDATA);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
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
		pfmt(stdout, MM_NOSTD, ":114:Bad descriptor in system map file\n");
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

		cr = check_entry(descr, (linenum == 2)? NULL:prevmapline, mapline);
		if (cr == 0)
			cr = check_user(mapline, 1);

		if (cr != 0) {

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
			case IE_NOUSER:
				pfmt(stdout, MM_NOSTD, ":115:Unknown mapped user\n");
				break;
			}
			numerrors++;
		}

		(void) strncpy(prevmapline, mapline, MAXLINE);
	}
	if (numerrors) {
		if (numerrors == 1)
			pfmt(stdout, MM_NOSTD, ":116:1 error found in system map\n", numerrors);
		else
			pfmt(stdout, MM_NOSTD, ":117:%d errors found in system map\n", numerrors);
	}
	else
		pfmt(stdout, MM_NOSTD, ":118:No errors in system map\n");
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
fix(scheme)
char	*scheme;
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

	/* open idata file */
	sprintf(mapfile, "%s/%s/%s", MAPDIR, scheme, IDATA);
	if ((fp = fopen(mapfile, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":75:%s: No such scheme\n",
			    scheme);
		log_fail();
		exit(1);
	}

	/* create temp file */
	sprintf(tmpfile, "%s%s", mapfile, DOTTMP);
	if ((tmpfd = creat(tmpfile, IDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		(void) fclose(fp);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(IDATA_LEVEL, &lvlno);
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
		pfmt(stdout, MM_NOSTD, ":119:Bad descriptor in system map file:\n%s\n", descr);
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

		cr = check_entry(descr, (linenum == 2)? NULL:prevmapline, mapline);
		if (cr == 0)
			cr = check_user(mapline, 1);

		if (cr != 0) {

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
			case IE_NOUSER:
				pfmt(stdout, MM_NOSTD, ":120:Unknown mapped user:\n");
				break;
			}
			pfmt(stdout, MM_NOSTD|MM_NOGET, "%s", mapline);
			numerrors++;

			pfmt(stdout, MM_NOSTD, ":105:Type c to change, s to skip, d to delete (default c) ");
			(void) fgets(response,sizeof(response),stdin);
			response[strlen(response)-1] = '\0';
			if ((*response == '\0') || (*response == 'c')) {
				pfmt(stdout, MM_NOSTD, ":121:New remote name: ");
				(void) fgets(remote,sizeof(remote),stdin);
				remote[strlen(remote)-1] = '\0';
				pfmt(stdout, MM_NOSTD, ":122:New local name: ");
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
			pfmt(stdout, MM_NOSTD, ":116:1 error found in system map\n", numerrors);
		else
			pfmt(stdout, MM_NOSTD, ":117:%d errors found in system map\n", numerrors);
		pfmt(stdout, MM_NOSTD, ":111:%d of them fixed\n", numfixes);
	} else
		pfmt(stdout, MM_NOSTD, ":118:No errors in system map\n");
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

	/* open idata file */
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
	if ((tmpfd = creat(tmpfile, IDATA_MODE)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":60:%s: Cannot create file\n",
			    tmpfile);
		log_fail();
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin(IDATA_LEVEL, &lvlno);
	(void) lvlfile(tmpfile, MAC_SET, &lvlno);

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
	pfmt(file, MM_ACTION, ":81:%s: Usage:\n\t%s [ -S scheme [ -l logname ] ]\n\t%s -S scheme -a -l logname -r g_name\n\t%s -S scheme -d -l logname [ -r g_name ]\n\t%s -S scheme -I descr\n\t%s -S scheme [ -Duscf ]\n", prog, prog, prog, prog, prog, prog);
}


main(argc, argv)
int	argc;
char	*argv[];
{
	int c;
	extern char *optarg;
	int opterror = 0;
	int action = ACT_LIST;
	char *scheme, *logname, *g_name, *descr;

	(void) umask(0000);

/* set up error message handling */

	(void) setlocale(LC_MESSAGES, "");
	(void) setlabel("UX:idadmin");
	(void) setcat("uxnsu");

/* parse command line */

	scheme = NULL;
	logname = NULL;
	g_name = NULL;
	descr = NULL;

	while ((c = getopt(argc, argv, OPTIONS)) != EOF)
		switch(c) {
		case 'S':
			scheme = optarg;
			break;
		case 'l':
			logname = optarg;
			break;
		case 'r':
			g_name = optarg;
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
		case 'u':
			if (action)
				opterror++;
			else {
				action = ACT_USER;
			}
			break;
		case 's':
			if (action)
				opterror++;
			else {
				action = ACT_SECURE;
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
		if (g_name != NULL)
			opterror++;
		else {
			if (scheme != NULL) {
				if (logname != NULL)
					list_logname(scheme, logname);
				else
					list_scheme(scheme);
			} else {
				list_all();
			}
		}
		break;
	case ACT_ADD:
		if ((scheme == NULL) || (logname == NULL) || (g_name == NULL))
			opterror++;
		else {
			add(scheme, g_name, logname);
		}
		break;
	case ACT_DELETE:
		if ((scheme == NULL) || (logname == NULL))
			opterror++;
		else {
			delete(scheme, g_name, logname);
		}
		break;
	case ACT_INSTALL:
		if ((scheme == NULL) || (descr == NULL) ||
		    (g_name != NULL) || (logname != NULL))
			opterror++;
		else {
			install(scheme, descr);
		}
		break;
	case ACT_UNINSTALL:
		if ((scheme == NULL) || (g_name != NULL) || (logname != NULL))
			opterror++;
		else {
			uninstall(scheme);
		}
		break;
	case ACT_USER:
		if ((scheme == NULL) || (g_name != NULL) || (logname != NULL))
			opterror++;
		else {
			user(scheme);
		}
		break;
	case ACT_SECURE:
		if ((scheme == NULL) || (g_name != NULL) || (logname != NULL))
			opterror++;
		else {
			secure(scheme);
		}
		break;
	case ACT_CHECK:
		if ((scheme == NULL) || (g_name != NULL) || (logname != NULL))
			opterror++;
		else {
			check(scheme);
		}
		break;
	case ACT_FIX:
		if ((scheme == NULL) || (g_name != NULL) || (logname != NULL))
			opterror++;
		else {
			fix(scheme);
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
