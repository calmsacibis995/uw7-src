/*		copyright	"%c%" 	*/

#ident	"@(#)cpmv.c	1.2"
#ident "$Header$"

#include "uucp.h"
#include <sys/stat.h>

/*
 * copy f1 to f2 locally
 *	f1	-> source file name
 *	f2	-> destination file name
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 */

static int
xcp(f1, f2)
char *f1, *f2;
{
	register int	fd1, fd2;
	register int	nr, nw;
	char buf[BUFSIZ];
	char *temp_p, temp[MAXFULLNAME];

	if ((fd1 = open(f1, O_RDONLY)) == -1)
		return(FAIL);

	if (DIRECTORY(f2)) {
		(void) strcat(f2, "/");
		(void) strcat(f2, BASENAME(f1, '/'));
	}
	DEBUG(4, "file name is %s\n", f2);

	(void) strcpy(temp, f2);
	if ((temp_p = strrchr(temp, '/')) == NULL )
	    temp_p = temp;
	else
	    temp_p++;
	(void) strcpy(temp_p, ".TM.XXXXXX");
	/* if mktemp fails, it nulls out the string */
	if ( *mktemp(temp_p) == '\0' )
	    temp_p = f2;
	else {
	    temp_p = temp;
	    DEBUG(4, "temp name is %s\n", temp_p);
	}

	if ((fd2 = open(temp_p, O_CREAT | O_WRONLY, PUB_FILEMODE)) == -1) {
		/* open of temp may fail if called from uidxcp() */
		/* in this case, try f2 since it is pre-created */
		temp_p = f2;
		if ((fd2 = open(temp_p, O_CREAT | O_WRONLY, PUB_FILEMODE)) == -1) {
			DEBUG(5, "open of file returned errno %d\n", errno);
			(void) close(fd1);
			return(FAIL);
		}
		DEBUG(4, "using file name directly.%s\n", "");
	}
	(void) chmod(temp_p, PUB_FILEMODE);

	/*	copy, looking for read or write failures */
	while ( (nr = read( fd1, buf, sizeof buf )) > 0  &&
		(nw = write( fd2, buf, nr )) == nr )
		;

	close(fd1);
	close(fd2);

	if( nr != 0  ||  nw == -1 )
		return(FAIL);

	if ( temp_p != f2 ) {
	    if ( rename(temp_p, f2) != 0 ) {
		DEBUG(5, "rename failed: errno %d\n", errno);
		(void) unlink(temp_p);
		return(FAIL);
	    }
	}
	return(0);
}

/*
 * copy f1 to fd2
 *	f1	-> source file name
 *	fd2	-> destination file descriptor
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 */

static int
fdxcp(f1, fd2)
char	*f1;
int	 fd2;
{
	register int	fd1;
	register int	nr, nw;
	char buf[BUFSIZ];

	if ((fd1 = open(f1, O_RDONLY)) == -1)
		return(FAIL);

	/*	copy, looking for read or write failures */
	while ( (nr = read( fd1, buf, sizeof buf )) > 0  &&
		(nw = write( fd2, buf, nr )) == nr )
		;

	close(fd1);
	close(fd2);

	if( nr != 0  ||  nw == -1 )
		return(FAIL);

	return(0);
}

/*
 * move f1 to f2 locally
 * returns:
 *	0	-> ok
 *	FAIL	-> failed
 */

int
xmv(f1, f2)
register char *f1, *f2;
{
	register int do_unlink, ret;
	struct stat sbuf;

	if ( stat(f2, &sbuf) == 0 )
		do_unlink = ((sbuf.st_mode & S_IFMT) == S_IFREG);
	else
		do_unlink = 1;

	if ( do_unlink )
		(void) unlink(f2);   /* i'm convinced this is the right thing to do */
	if ( (ret = link(f1, f2)) < 0) {
	    /* copy file */
	    ret = xcp(f1, f2);
	}

	if (ret == 0)
	    (void) unlink(f1);
	return(ret);
}


/* toCorrupt - move file to CORRUPTDIR
 * return - none
 */

void
toCorrupt(file)
char *file;
{
	char corrupt[MAXFULLNAME];

	(void) mkdirs(CORRUPTDIR, DIRMASK);
	(void) sprintf(corrupt, "%s/%s", CORRUPTDIR, BASENAME(file, '/'));
	(void) link(file, corrupt);
	ASSERT(unlink(file) == 0, Ct_UNLINK, file, errno);
	return;
}

/*
 * append f1 to f2
 *	f1	-> source FILE pointer
 *	f2	-> destination FILE pointer
 * return:
 *	SUCCESS	-> ok
 *	FAIL	-> failed
 */
int
xfappend(fp1, fp2)
register FILE	*fp1, *fp2;
{
	register int nc;
	char	buf[BUFSIZ];

	while ((nc = fread(buf, sizeof (char), BUFSIZ, fp1)) > 0)
		(void) fwrite(buf, sizeof (char), nc, fp2);

	return(ferror(fp1) || ferror(fp2) ? FAIL : SUCCESS);
}


/*
 * copy f1 to f2 locally under uid of uid argument
 *	f1	-> source file name
 *	f2	-> destination file name
 *	Uid, Euid, Gid, and Egid are global
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 * NOTES:
 *  This code used to not work correctly when realuid was root 
 *  on System 5, but now we only change the effective uid.
 */

int
uidxcp(f1, f2)
char *f1, *f2;
{
	int status;
	int fd2;
	char full[MAXFULLNAME];

	(void) strcpy(full, f2);
	if (DIRECTORY(f2)) {
	    (void) strcat(full, "/");
	    (void) strcat(full, BASENAME(f1, '/'));
	}

	/* create full owned by uucp */
	if ((fd2 = creat(full, DFILEMODE)) < 0)
		return(FAIL);
	(void) chmod(full, DFILEMODE);

	/* do file copy as real uid */
	(void) seteuid(Uid);
	(void) setegid(Gid);
	status = fdxcp(f1, fd2);
	(void) seteuid(Euid);
	(void) setegid(Egid);
	return(status);

}

/*
 * put file in public place
 * if successful, filename is modified
 * returns:
 *	0	-> success
 *	FAIL	-> failure
 */
int
putinpub(file, tmp, user)
char *file, *user, *tmp;
{
	int status;
	char fullname[MAXFULLNAME];

	(void) sprintf(fullname, "%s/%s/", Pubdir, user);
	if (mkdirs(fullname, PUBMASK) != 0) {
		/* cannot make directories */
		return(FAIL);
	}
	(void) strcat(fullname, BASENAME(file, '/'));
	status = xmv(tmp, fullname);
	if (status == 0) {
		(void) strcpy(file, fullname);
		(void) chmod(fullname, PUB_FILEMODE);
	}
	return(status);
}
