#ifndef NOIDENT
#pragma ident	"@(#)sets_plus.c	15.1"
#endif

/*
 * List sets and those packages that are not in any set.
 * (And a "main" to test it.)
 *
 * Usage: see usage()
 */

/*
 * Please note that it is assumed that the spool directory,
 * local or remote, is set up correctly.  If it is not, the
 * results of running this code are unpredictable.  On assumption
 * in particular, is that if a package is listed in a set, then
 * the package exists in the spool directory.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define	LOC_SPOOL_DIR		"/var/spool/pkg"
#define	REM_SPOOL_DIR		"/var/spool/dist"
#define TMPFILNAMBASE	 	"sets+"
#define TMPFILNAMDIR	 	"/tmp"
#define READBUFSZ		1000
#define	TOKEN_SEPAR		" \t\n"
#define	ERR_RET			1	/* generic error return */
#define	OLD_DEAMON		2	/* server has old in.inetinst */


int	rmtmp = 1;	/* if zero, don't rm tmpfile - while debugging */
int	minus_p = 0;	/* pass -p option to pkginfo */
int	minus_i = 0;	/* pass -i option to pkginfo */

char	*usage();
char	*skip_first_field(char *s1, const char *s2);


main(int argc, char **argv)
{
	char	*spool_direct = NULL;
	char	*host = NULL;
	int	c;


	while((c = getopt(argc, argv, "d:s:npi")) != -1) {
		switch (c) {
		case 'd':
			spool_direct = optarg;
			break;

		case 's':
			host = optarg;
			break;

		case 'n':
			/* don't remove tmp file - while debugging */
			rmtmp = 0;
			break;

		case 'p':
			minus_p = 1;
			break;

		case 'i':
			minus_i = 1;
			break;

		default:
			fprintf(stderr, usage());
			exit(ERR_RET);
		}
	}

	if(minus_p && minus_i) {
		fprintf(stderr, usage());
		exit(ERR_RET);
	}

	exit(sets_plus(host, spool_direct));
}

/*
 * List sets and those packages that are not in any set.
 *
 * Arguments:
 *	host -		pointer to the name of the server system;
 *			if NULL, local it's the local system
 *	spool_direct -	directory where the packages are stored
 *			if NULL, then
 *				- if local: /var/spool/pkg
 *				- if remote: /var/spool/dist
 */

sets_plus(char	*host,
	  char	*spool_direct)
{
	FILE	*fp;		/* to read commands output */
	char	cmd[READBUFSZ];	/* buffer to build the first command */
	char	cmd2[READBUFSZ];/* buffer to build the second command */
	int	netflg;		/* local (pkginfo), or remote (pkglist) */
	int	retval;		/* generic return value */
	char	x[READBUFSZ];	/* dummy buffer */
	char	setname[READBUFSZ];	/* scan in set name */
	char	readbuf[READBUFSZ];	/* generic read buf */
	int	linecnt;	/* counter for lines read */
	int     old_deamon = 0; /* old in.inetinst deamon flag */
	int     some_pkg_error = 0;/* UX:pkginfo error */


	/* work buffers and pointers when eliminating duplicates */
	char	rbuf_1[READBUFSZ];
	char	rbuf_2[READBUFSZ];
	char	sbuf_1[READBUFSZ];
	char	sbuf_2[READBUFSZ];
	char	*p1, *p2;

	/* tmp file (name anf FP) for sorting, etc */
	char	tmpfilnam[100];
	FILE	*tmpfp;


	if(host == NULL) {
		netflg = 0;
	} else {
		netflg = 1;
	}

	if(spool_direct == NULL) {
		if(netflg)
			spool_direct = REM_SPOOL_DIR;
		else
			spool_direct = LOC_SPOOL_DIR;
	}

	/*
	 * Part 1: get the set names; build second command and
	 *         print set lines to stdout
	 */

	/* start building cmd2 that will include the set names */
	if(netflg) {
		sprintf(cmd2, "/usr/bin/pkglist -s %s:%s ", host, spool_direct);
	} else {
		if(minus_p) {
		    sprintf(cmd2, "/usr/bin/pkginfo -p -d %s ", spool_direct);
		} else if (minus_i) {
		    sprintf(cmd2, "/usr/bin/pkginfo -i -d %s ", spool_direct);
		} else {
		    sprintf(cmd2, "/usr/bin/pkginfo    -d %s ", spool_direct);
		}
	}

	/* build command cmd - to first get a list of sets */
	if(netflg) {
		sprintf(cmd, "/usr/bin/pkglist -c set -s %s:%s all", host, spool_direct);
	} else {
		if(minus_p) {
		    sprintf(cmd, "/usr/bin/pkginfo -c set -p -d %s", spool_direct);
		} else if (minus_i) {
		    sprintf(cmd, "/usr/bin/pkginfo -c set -i -d %s", spool_direct);
		} else {
		    sprintf(cmd, "/usr/bin/pkginfo -c set    -d %s", spool_direct);
		}
	}

	if((fp = popen(cmd, "r")) == NULL) {
		perror("popen");
		return(ERR_RET);
	}

	linecnt = 0;
	while(fgets(readbuf, READBUFSZ, fp) != NULL) {
		linecnt++;
		sscanf(readbuf, "%s%s", x, setname);
		/*
		 * check if the first line contains the word "set"
		 * If not, that's an old in.inetinst on the server.
		 */
		if((linecnt == 1) && netflg) {
			if(strcmp(x, "set")) {
				if(!strncmp(x, "UX:pkg", 6)) {
		                        some_pkg_error++;
				} else {
		                        old_deamon++;
				}
		        }
		}
		if(!some_pkg_error) {
			fputs(readbuf, stdout);
			if(!old_deamon) {
				strcat(cmd2, setname);
				strcat(cmd2, " ");
			}
		}
	}
	if(retval = pclose(fp)) {
		fprintf(stderr, "pclose returns %d\n", retval);
		return(ERR_RET);
	}

	if(some_pkg_error) {
		/* pkginfo failed (wrong directory, etc..) */
		return(ERR_RET);
	}
	if(old_deamon) {
		/* the job is done */
		return(OLD_DEAMON);
	}

	/*
	 * Part 2: get a listing of packages in sets
	 */

	/* use a tmp file to store the list of packages */
	sprintf(tmpfilnam, "%s", tempnam(TMPFILNAMDIR, TMPFILNAMBASE));
	if((tmpfp = fopen(tmpfilnam, "w")) == NULL) {
		perror("fopen tmpfile");
		return(ERR_RET);
	}

	if(linecnt) {
		/*
		 * There are sets: call pkginfo with set names;
		 * write to tmp file.
		 */
		if((fp = popen(cmd2, "r")) == NULL) {
			perror("popen2");
			fclose(tmpfp);
			if(rmtmp)
				unlink(tmpfilnam);
			return(ERR_RET);
		}
		while(fgets(readbuf, READBUFSZ, fp) != NULL) {
			fputs(readbuf, tmpfp);
		}
		if(retval = pclose(fp)) {
			fprintf(stderr, "pclose2 returns %d\n", retval);
			fclose(tmpfp);
			if(rmtmp)
				unlink(tmpfilnam);
			return(ERR_RET);
		}
	}

	/*
	 * Part 3: get a listing of all packages
	 * 	(whether in sets or not)
	 */

	if(netflg) {
		sprintf(cmd2, "/usr/bin/pkglist -s %s:%s all", host, spool_direct);
	} else {
		if(minus_p) {
		    sprintf(cmd2, "/usr/bin/pkginfo -p -d %s all", spool_direct);
		} else if (minus_i) {
		    sprintf(cmd2, "/usr/bin/pkginfo -i -d %s all", spool_direct);
		} else {
		    sprintf(cmd2, "/usr/bin/pkginfo    -d %s all", spool_direct);
		}
	}
	if((fp = popen(cmd2, "r")) == NULL) {
		perror("popen3");
		fclose(tmpfp);
		if(rmtmp)
			unlink(tmpfilnam);
		return(ERR_RET);
	}
	while(fgets(readbuf, READBUFSZ, fp) != NULL) {
		fputs(readbuf, tmpfp);
	}
	if(retval = pclose(fp)) {
		fprintf(stderr, "pclose3 returns %d\n", retval);
		fclose(tmpfp);
		if(rmtmp)
			unlink(tmpfilnam);
		return(ERR_RET);
	}

	/* close tmp file */
	fclose(tmpfp);

	/*
	 * Part 4: sort and uniq tmp file; read it in thru this
	 *	code which eliminates lines with the same short
	 *	package name.
	 */
	sprintf(cmd2, "/usr/bin/sort -b -k 2,2 %s | /usr/bin/uniq -u", tmpfilnam);
	/* fprintf(stderr, "CMD %s\n", cmd2); */
	if((fp = popen(cmd2, "r")) == NULL) {
		perror("popen4");
		if(rmtmp)
			unlink(tmpfilnam);
		return(ERR_RET);
	}

	while (fgets(rbuf_1, READBUFSZ, fp) != NULL) {
again:
		if (fgets(rbuf_2, READBUFSZ, fp) == NULL) {
			fputs(rbuf_1, stdout);
			if(rmtmp)
				unlink(tmpfilnam);
			return(0);
		}

		/* compare the pkg name in the two lines */
		memcpy(sbuf_1, rbuf_1, READBUFSZ);
		memcpy(sbuf_2, rbuf_2, READBUFSZ);
		p1 = skip_first_field(sbuf_1, TOKEN_SEPAR);
		p2 = skip_first_field(sbuf_2, TOKEN_SEPAR);
		if(p1 == NULL || p2 == NULL) {
			fprintf(stderr, "error in tmp file %s\n", tmpfilnam);
			if(rmtmp)
				unlink(tmpfilnam);
			return(ERR_RET);
		}
		if (strcmp(p1,p2) == 0)
			/* both lines for the same package */
			continue;
		/* line 1 different from line 2 */
		fputs(rbuf_1, stdout);
		memcpy(rbuf_1, rbuf_2, READBUFSZ);
		goto again;
	}
	if(retval = pclose(fp)) {
		fprintf(stderr, "pclose4 returns %d\n", retval);
		if(rmtmp)
			unlink(tmpfilnam);
		return(ERR_RET);
	}

	/* clean up: rm tmp file */
	if(rmtmp)
		unlink(tmpfilnam);

	return(0);
}

char *
usage()
{
	return("watch your options:\
	sets_plus [-n] [-i|-p] [-s host] [-d spool_directory]\n");
}


char *
skip_first_field(char *s1, const char *s2)
{
	if (strtok(s1, s2) == NULL)
		return (NULL);

	return (strtok(NULL, s2));
}
