/*		copyright	"%c%" 	*/


#ident	"@(#)secure.c	1.2"
#ident	"$Header$"

#include "string.h"
#include "sys/param.h"
#include "stdlib.h"

#include "lp.h"
#include "secure.h"

/**
 ** getsecure() - EXTRACT SECURE REQUEST STRUCTURE FROM DISK FILE
 **/

SECURE *
#if	defined(__STDC__)
getsecure (
	char *			file
)
#else
getsecure (file)
	char			*file;
#endif
{
	static SECURE		secbuf;

	char			buf[BUFSIZ],
				*path;

	FILE			*fp;

	int			fld;


	if (*file == '/')
		path = Strdup(file);
	else
		path = makepath(Lp_Requests, file, (char *)0);
	if (!path)
		return (0);

	if (!(fp = open_lpfile(path, "r", MODE_NOREAD))) {
		Free (path);
		return (0);
	}
	Free (path);

	secbuf.req_id	= (char *) 0;
	secbuf.user	= (char *) 0;
	secbuf.system	= (char *) 0;
	secbuf.lid	= (level_t) -1;
	secbuf.status	= SC_STATUS_UNACCEPTED;
	secbuf.rem_reqid = (char *) 0;

	for (fld = 0; fld < SC_MAX && fgets (buf, BUFSIZ, fp); fld++)
	{
		buf [strlen (buf) - 1] = 0;
		switch (fld) {
		case SC_REQID:
			secbuf.req_id = Strdup(buf);
			break;

		case SC_UID:
			secbuf.uid = (uid_t)atol(buf);
			break;

		case SC_USER:
			secbuf.user = Strdup(buf);
			break;

		case SC_GID:
			secbuf.gid = (gid_t)atol(buf);
			break;

		case SC_SIZE:
			secbuf.size = (size_t)atol(buf);
			break;

		case SC_DATE:
			secbuf.date = (time_t)atol(buf);
			break;

		case SC_SYSTEM:
			secbuf.system = Strdup(buf);
			break;

		/*
		**  The lid and status will not be present if it is
		**  a request file that originated on a remote
		**  system that is running pre-SVR4.0ES.
		*/
		case SC_LID:
			secbuf.lid = (level_t) atol (buf);
			break;

		case SC_STATUS:
			secbuf.status = (uint) strtoul (buf, (char **)0, 16);
			break;

		case SC_REM_REQID:
			secbuf.rem_reqid = Strdup(buf);
			break;
		}
	}
	/*
	**  SC_MAX-3 to handle missing lid's and status'.
	**  Both are left alone.
	*/
	if (ferror (fp) && fld < (SC_MAX-3))
	{
		int	save = errno;

		freesecure (&secbuf);
		close_lpfile (fp);
		errno = save;
		return	0;
	}
	close_lpfile (fp);

	/*
	 * Now go through the structure and see if we have
	 * anything strange.
	 */
	if (
	        secbuf.uid > MAXUID+1
	     || !secbuf.user
	     || secbuf.gid > MAXUID+1
	     || secbuf.size == 0
	     || secbuf.date <= 0
	) {
		freesecure (&secbuf);
		errno = EBADF;
		return	0;
	}

	return	&secbuf;
}

/**
 ** putsecure() - WRITE SECURE REQUEST STRUCTURE TO DISK FILE
 **/

int
#if	defined(__STDC__)
putsecure (
	char *			file,
	SECURE *		secbufp
)
#else
putsecure (file, secbufp)
	char			*file;
	SECURE			*secbufp;
#endif
{
	char			*path;

	FILE			*fp;

	int			fld;

	if (*file == '/')
		path = Strdup(file);
	else
		path = makepath(Lp_Requests, file, (char *)0);
	if (!path)
		return (-1);

	if (!(fp = open_lpfile(path, "w", MODE_NOREAD))) {
		Free (path);
		return (-1);
	}
	Free (path);

	if (
		!secbufp->req_id ||
		!secbufp->user
	)
		return (-1);

	for (fld = 0; fld < SC_MAX; fld++)

		switch (fld) {

		case SC_REQID:
			(void)fprintf (fp, "%s\n", secbufp->req_id);
			break;

		case SC_UID:
			(void)fprintf (fp, "%ld\n", secbufp->uid);
			break;

		case SC_USER:
			(void)fprintf (fp, "%s\n", secbufp->user);
			break;

		case SC_GID:
			(void)fprintf (fp, "%ld\n", secbufp->gid);
			break;

		case SC_SIZE:
			(void)fprintf (fp, "%lu\n", secbufp->size);
			break;

		case SC_DATE:
			(void)fprintf (fp, "%ld\n", secbufp->date);
			break;

		case SC_SYSTEM:
			(void)fprintf (fp, "%s\n", secbufp->system);
			break;

		case SC_LID:
			(void)fprintf (fp, "%ld\n", secbufp->lid);
			break;

		case SC_STATUS:
			(void)fprintf (fp, "0x%x\n", secbufp->status);
			break;

		case SC_REM_REQID:
			(void)fprintf (fp, "%s\n", secbufp->rem_reqid);
			break;
		}


	close_lpfile (fp);

	return (0);
}

/*
**  rmsecure ()
**
**	o  'reqfilep' is of the form 'node-name/request-file'
**	   e.g. 'sfcalv/123-0'.
*/
#ifdef	__STDC__
int
rmsecure (char *reqfilep)
#else
int
rmsecure (reqfilep)

char *	reqfilep;
#endif
{
	int	n;
	char *	pathp;

	pathp = makepath (Lp_Requests, reqfilep, (char *) 0);
	if (! pathp)
		return	-1;

	n = Unlink (pathp);
	Free (pathp);

	return	n;
}


/**
 ** freesecure() - FREE A SECURE STRUCTURE
 **/

void
#if	defined(__STDC__)
freesecure (
	SECURE *		secbufp
)
#else
freesecure (secbufp)
	SECURE *		secbufp;
#endif
{
	if (!secbufp)
		return;
	if (secbufp->req_id)
		Free (secbufp->req_id);
	if (secbufp->user)
		Free (secbufp->user);
	if (secbufp->system)
		Free (secbufp->system);
	if (secbufp->rem_reqid)
		Free (secbufp->rem_reqid);
	return;
}
