#ifdef SCOMS
/*
 * sco value add to c-client, included in mail.c
 */

/* Mail deliver message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    message internal date
 *	    envelope sender
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long
mail_deliver(MAILSTREAM *stream, char *mailbox,
		       char *sender, FILE *fd)
{
	DRIVER *d;
	int ret;

	for (d = maildrivers; d; d = d->next) {
		if (d->deliver) {
			/* first one to succeed is our new mailbox format */
			ret = (d->deliver) (NIL,mailbox,sender,fd);
			/* success */
			if (ret)
				return(ret);
		}
	}
	/* failed to deliver */
	return(0);
}

/*
 * std call to link in all dlls'
 * becomes only call in linkage.c
 */
void
mail_link_dlls()
{
	/*extern DRIVER mboxdriver;*/
	extern DRIVER imapdriver;
	extern DRIVER nntpdriver;
	/*extern DRIVER pop3driver;*/
	extern DRIVER scoms1driver;
	extern DRIVER newsdriver;
	extern DRIVER dummydriver;
	extern AUTHENTICATOR auth_log;

	mail_link (&scoms1driver);		/* link in the scoms1 driver */
	mail_link (&imapdriver);		/* link in the imap driver */
	/*mail_link (&pop3driver);		/* link in the pop3 driver */
	/*mail_link (&mboxdriver);		/* link in the mbox driver */
	mail_link (&nntpdriver);		/* link in the nntp driver */
	mail_link (&newsdriver);		/* link in the news driver */
	mail_link (&dummydriver);		/* link in the dummy driver */
	auth_link (&auth_log);		/* link in the log authenticator */
}

/*
 * nolink_open()
 *
 * Open a file, making sure it is a regular file with exactly one hard
 * link.  It returns all the same errno codes as open(S), except that
 * it returns EACCES if it encounters any security problems or suspicious
 * occurences.
 *
 * The algorithm used here is:
 *
 * lstat() the file
 * if it exists
 *     if it's a symlink, or has more than one hardlink, fail
 *     open without the O_CREAT flag; return its errno if it fails
 *     fstat() the fd and compare st_dev and st_ino with the lstat() above.
 *     if they don't match, or if the file has more than one hard link, fail
 * else
 *     open with O_CREAT|O_EXCL, return its errno if it fails
 *     lstat() the file
 *     fstat() the fd and compare st_dev and st_ino with the lstat() above.
 *     if they don't match, or if the file has more than one hard link
 *     then
 * 	   remove the file that was created and the symlink (if any)
 *         fail
 *     fi
 * fi
 * return fd
 *
 * All ``fail'' cases above result in a return of -1 with errno
 * set to EACCES.  Also, if any stat except the first lstat fails,
 * then things are changing underneath us, so EACCES is returned.
 *
 * This function relies on the behavior of the O_CREAT and O_EXCL
 * flags to the open(S) system call.  If the file being opened is
 * a symlink that points at a file that does not exist, the call
 * should fail with EEXIST.  If it doesn't, then we will create
 * a file through the symlink, which could be dangerous (e.g.
 * /usr/lib/cron/cron.deny).  The function can identify this case,
 * and will follow the symlink to find the file it just created
 * and remove it.  There is a timing window there, however, during
 * which the symlink could be removed, and the function would be
 * unable to remove the file it has created.
 *
 * All current filesystems have the behaviour this program needs,
 * so the attempt to remove the file will never be executed.  The
 * test program that exercises this function will fail if it is
 * executed on a filesystem that does not have the correct
 * behaviour.
 */
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/unistd.h>

#ifdef NOLINK_TEST
extern void nolink_open_cb ();
extern void nolink_badfs_cb ();
#endif

/*
 * Open a file, making sure it is a regular file with exactly one hard link.
 */
int
nolink_open (char *file, int flags, int mode)
{
    struct stat		statbuf;
    struct stat		fdstatbuf;
    int			fd;
    int			save_errno;

    /*
     * Check to see if the file exists.
     */
    if (lstat (file, &statbuf) != -1)
    {
	/*
	 * If they wanted an exclusive create, fail.
	 */
	if ((flags & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL))
	{
	    errno = EEXIST;
	    return (-1);
	}

	/*
	 * If it's a symlink, or has more than one hard link,
	 * fail with ``Permission denied.''
	 */
	if (!S_ISREG (statbuf.st_mode) || statbuf.st_nlink != 1)
	{
	    errno = EACCES;
	    return (-1);
	}

	/*
	 * The file exists, so we don't want to create it.
	 * This will make sure the open fails if the file becomes
	 * a symlink to a non-existent file between the lstat
	 * and the open.
	 */
	flags &= ~O_CREAT;

#ifdef NOLINK_TEST
	/*
	 * Call back to the test program, so it can simulate
	 * the successful use of the timing window.
	 */
	nolink_open_cb ();
#endif

	/*
	 * If the open fails, just return whatever errno
	 * it sets.
	 */
	if ((fd = open (file, flags, mode)) == -1)
	{
	    return (-1);
	}
    }
    else
    {
	/*
	 * If the lstat failed, so would a normal open,
	 * so we just return with the same errno (e.g.
	 * ENOTDIR if something in the path wasn't a
	 * directory).  The only exception is ENOENT;
	 * we should continue if they set O_CREAT, and
	 * return ENOENT otherwise.
	 */
	if (errno != ENOENT || (flags & O_CREAT) != O_CREAT)
	{
	    return (-1);
	}

	/*
	 * Make sure that the open will fail if the file appears
	 * between the lstat and the open.
	 */
	flags |= O_EXCL;

#ifdef NOLINK_TEST
	/*
	 * Call back to the test program, so it can simulate
	 * the successful use of the timing window.
	 */
	nolink_open_cb ();
#endif

	/*
	 * If the open fails, just return whatever errno
	 * it sets.
	 */
	if ((fd = open (file, flags, mode)) == -1)
	{
	    return (-1);
	}

#ifdef NOLINK_TEST
	/*
	 * Call back to the test program, so it can simulate a
	 * filesystem that doesn't have the correct open behaviour.
	 */
	nolink_badfs_cb ();
#endif

	/*
	 * Now lstat the (presumably existing) file again, so
	 * the comparison below will work.  If it fails, something
	 * has happened, so return EACCES to indicate the suspicious
	 * situation.
	 */
	if (lstat (file, &statbuf) == -1)
	{
	    close (fd);
	    errno = EACCES;
	    return (-1);
	}
    }

    /*
     * Now, check to make sure the file didn't get linked
     * in between the lstat and the open.  Again, EACCES is
     * returned on failure.
     */
    if (fstat (fd, &fdstatbuf) == -1)
    {
	close (fd);
	errno = EACCES;
	return (-1);
    }

    if ((statbuf.st_dev != fdstatbuf.st_dev) ||
	(statbuf.st_ino != fdstatbuf.st_ino) ||
	fdstatbuf.st_nlink != 1)
    {
	/*
	 * Caught you, you evil cracker!  Close the file
	 * and return an error.
	 */
	close (fd);

	if (flags & O_CREAT)
	{
	    char		linkfile[MAXPATHLEN];
	    int			linkcount = 0;

	    /*
	     * Oops.  We've just created a file through a symlink.
	     * Try to undo the damage.  To clean up, we follow the
	     * symlink, which might point to another symlink, until
	     * we reach the file we've created.  We then unlink that
	     * file, and the original filename we were trying to open.
	     *
	     * There's a timing window here, in that if they symlink
	     * in between the first lstat and the open, and then
	     * remove that symlink in between the open and this
	     * readlink, there is no way we can remove the file
	     * we've created.
	     *
	     * This code will never be reached if the filesystems
	     * continue to behave the way they currently do.
	     */
	    strncpy (linkfile, file, sizeof (linkfile));
	    linkfile[sizeof(linkfile) - 1] = '\0';

	    while (readlink (linkfile, linkfile, sizeof (linkfile)) != -1
		&& ++linkcount <= MAXSYMLINKS)
	    {
		if (lstat (linkfile, &statbuf) != -1)
		{
		    if ((statbuf.st_dev == fdstatbuf.st_dev) &&
			(statbuf.st_ino == fdstatbuf.st_ino))
		    {
			(void) unlink (linkfile);
			break;
		    }
		}
	    }

	    (void) unlink (file);
	}

	errno = EACCES;
	return (-1);
    }

    /*
     * All's well.
     */
    return (fd);
}
#endif
