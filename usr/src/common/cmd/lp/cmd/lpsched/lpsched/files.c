/*		copyright	"%c%" 	*/


#ident	"@(#)files.c	1.5"
#ident  "$Header$"


/*******************************************************************************
 *
 * FILENAME:    files.c
 *
 * DESCRIPTION: File handling for the lpsched process
 *
 * SCCS:	files.c 1.5  9/23/97 at 16:08:10
 *
 * CHANGE HISTORY:
 *
 * 18-08-97  Paul Cunningham        cs94-32601
 *           Change function rmfiles() so that it removes the notification file
 *           from the lp/tmp/<systenName> directory when it has been finished
 *           with.
 * 23-09-97  Paul Cunningham        ul95-11743
 *           Change function rmfiles() to correct the set up the pathname for
 *           the data file in the lp/tmp/.net/<sysName> directory, thus allowing
 *           these tmp files to be deleted by the lpsched.
 *           Note: correcting this makes code to remove these files in the 
 *           bsdChild lpNet redundent, ie. in bsdChild/printreq.c function
 *           s_print_request it removes these files using unlink(). Also there
 *           must be code in the s5Child to do it. But for now I have not 
 *           changed that code because the extra unlinks do no harm.
 *
 *******************************************************************************
 */


#include <sys/types.h>
#include <sys/stat.h>

#include "lpsched.h"

static char time_buf[50];

#define WHO_AM_I      I_AM_LPSCHED
#include "oam.h"

/* check if file is a FIFO */

static int
#ifdef __STDC__
ispipe(
       char *	file
)
#else
ispipe(file)
       char * file;
#endif
{
    struct stat buf;
    int ret;

    while (ret = stat(file, &buf) == -1 && errno == EINTR)
	;

    if (buf.st_mode & S_IFIFO && ret == 0)
	return 1;
    else
	return 0;
}

/*
 * Procedure:     chfiles
 *
 * Restrictions:
 *               Chmod: None
 *               Chown: None
 *
 * Notes - CHANGE OWNERSHIP OF FILES
*/

#ifdef	__STDC__
int	chfiles (char ** list, uid_t uid, gid_t gid)
#else
int	chfiles (list, uid, gid)
char	**list;
uid_t	uid;
gid_t	gid;
#endif
{
	char		*file;

	DEFINE_FNNAME (chfiles)

while(file = *list++)
{
	if ((STRNEQU(Lp_Temp, file, strlen(Lp_Temp)) ||
		STRNEQU(Lp_Tmp, file, strlen(Lp_Tmp))) && !ispipe(file))
	{
		/*
		 * Once this routine (chfiles) is called for a request,
		 * any temporary files are ``ours'', i.e. they are on our
		 * machine. A user running on an RFS-connected remote machine
		 * can't necessarily know our machine name, so can't put
		 * the files where they belong (Lp_Tmp/machine). But now we
		 * can. Of course, this is all done with mirrors, as Lp_Temp
		 * and Lp_Tmp/local-machine are symbolicly linked. So we just
		 * change the name. This saves on wear and tear later.
		 */
		if (STRNEQU(Lp_Temp, file, strlen(Lp_Temp)))
		{
			char *newfile = makepath(Lp_Tmp, Local_System,
				file + strlen(Lp_Temp) + 1, NULL);

			Free(file);
			list[-1] = file = newfile;
		}
		
		(void) Chmod(file, 0600);
		(void) Chown(file, uid, gid);

	}
}
	return	1;
}

/*
 * Procedure:     chfiles2
 *
 * Restrictions:
 *               Chmod: None
 *               Chown: None
 *
 * Notes - CHANGE OWNERSHIP OF FILES
*/

#ifdef	__STDC__
int	chfiles2 (char ** list, uid_t uid, gid_t gid)
#else
int	chfiles2 (list, uid, gid)
char	**list;
uid_t	uid;
gid_t	gid;
#endif
{
	char		*file;

	DEFINE_FNNAME (chfiles)

while(file = *list++)
{
	if ((STRNEQU(Lp_Temp, file, strlen(Lp_Temp)) ||
		STRNEQU(Lp_Tmp, file, strlen(Lp_Tmp))) && !ispipe(file))
	{
		/*
		 * Once this routine (chfiles) is called for a request,
		 * any temporary files are ``ours'', i.e. they are on our
		 * machine. A user running on an RFS-connected remote machine
		 * can't necessarily know our machine name, so can't put
		 * the files where they belong (Lp_Tmp/machine). But now we
		 * can. Of course, this is all done with mirrors, as Lp_Temp
		 * and Lp_Tmp/local-machine are symbolicly linked. So we just
		 * change the name. This saves on wear and tear later.
		 */
		if (STRNEQU(Lp_Temp, file, strlen(Lp_Temp)))
		{
			char *newfile = makepath(Lp_Tmp, Local_System,
				file + strlen(Lp_Temp) + 1, NULL);

			Free(file);
			list[-1] = file = newfile;
		}
		
		(void) Chmod(file, 0600);
		(void) Chown(file, uid, gid);

	}
}
	return	1;
}
/*
 * Procedure:     statfiles
 *
 * Restrictions:
 *               Stat: None
 *               lvlproc(2): None
*/

#ifdef	__STDC__
off_t	statfiles (char **list, level_t lid)
#else
off_t	statfiles (list, lid)
char	**list;
level_t	lid;
#endif
{
	size_t		total = 0;
	struct stat	stbuf;
	char		*file;

	DEFINE_FNNAME (chfiles)

while(file = *list++)
{
	if (STRNEQU(Lp_Temp, file, strlen(Lp_Temp)) ||
		STRNEQU(Lp_Tmp, file, strlen(Lp_Tmp)))
	{
		if (Stat(file, &stbuf) < 0)
		{
			return	-1;
		}
	}
	else	/*  A user file  */
	{
		/*
		**  We must change our proc level to get at
		**  MLD files.
		*/
		(void)	lvlproc (MAC_SET, &lid);

		if (Stat(file, &stbuf) < 0)
		{
			(void)	lvlproc (MAC_SET, &Lp_Lid);
			return	-1;
		}
		(void)	lvlproc (MAC_SET, &Lp_Lid);
	}
	switch (stbuf.st_mode & S_IFMT) {
	case 0:
	case S_IFREG:
		break;

	case S_IFIFO:
		stbuf.st_size = 1;
		break;

	case S_IFDIR:
	case S_IFCHR:
	case S_IFBLK:
	default:
		return	-1;
	}

		total += stbuf.st_size;
	}
	return	total;
}

/*
 * Procedure:     rmfiles
 *
 * Restrictions:
 *               Unlink: None
 *               open_lpfile: None
 *               Open: None
 *               cftime: None
 *               fprintf: None
 *               Read: None
 *               fwrite: None
 *               fflush: None
 * Notes - DELETE/LOG FILES FOR DEFUNCT REQUEST
 */

#ifdef	__STDC__
void
rmfiles (RSTATUS * rp, int log_it)	/* funcdef */
#else
void
rmfiles (rp, log_it)

RSTATUS *	rp;
int		log_it;
#endif
{
	DEFINE_FNNAME (rmfiles)

	char	**file	= rp->request->file_list;
	char	*path;
	char	*p;
	char	*basename;
	char	num[STRSIZE(MOST_FILES) + 1];
	static FILE	*logfp	= 0;
	int		reqfd;
	int		count	= 0;
	int		isremote	= False;


	if (rp->req_file) 
	{
		/*
		 * The secure request file is removed first to prevent
		 * reloading should the system crash while in rmfiles().
		 */
		path = makepath(Lp_Requests, rp->req_file, (char *)0);
		(void) Unlink(path);
		Free(path);

		/* If destination is a remote, remove jobfiles.
		 */
		if (rp->printer && (rp->printer->status & PS_REMOTE)) 
		{
		    isremote=True;

		    if (rp->secure && rp->secure->req_id) 
		    {
			basename = makestr(getreqno(rp->secure->req_id),
					   "-0",(char *)0);
			path = makepath(Lp_NetTmp,"requests",rp->secure->system,
					basename,(char *)0);
			(void) Unlink(path);
			Free(path);
			path = makepath(Lp_NetTmp,"tmp",rp->secure->system,
					basename,(char *)0);
			(void) Unlink(path);
			Free(path);
			Free(basename);
		    }
		}
		else
		{
		  /* delete the notification file
		   */
		  basename = makestr
			       (
			       getreqno( rp->secure->req_id), (char*)0
			       );
		  path = makepath
			    (
			    Lp_Spooldir, "tmp", rp->secure->system,
			    basename, (char*)0
			    );
		  (void)Unlink( path);
		  Free( path);
		  Free( basename);
		}

		/* Copy the request file to the log file, if asked,
		 * or simply remove it.
		 */
		path = makepath(Lp_Tmp, rp->req_file, (char *)0);

		if (log_it && rp->secure && rp->secure->req_id) {
		if (!logfp)
			logfp = open_lpfile(Lp_ReqLog, "a", MODE_NOREAD);
		if (logfp && (reqfd = Open(path, O_RDONLY, 0)) != -1) {
			register int	n;
			char		buf[BUFSIZ];

			(void) cftime(time_buf, NULL, &(rp->secure->date));
			(void) fprintf (
			logfp,
			"= %s, uid %d, gid %d, lid %d, size %ld, %s\n",
			rp->secure->req_id,
			(int) rp->secure->uid,
			(int) rp->secure->gid,
			(int) rp->secure->lid,
			rp->secure->size,
			time_buf
			);
			if (rp->slow)
			(void) fprintf (logfp, "x %s\n", rp->slow);
			if (rp->fast)
			(void) fprintf (logfp, "y %s\n", rp->fast);
			if (rp->printer && rp->printer->printer->name)
			(void) fprintf (logfp, "z %s\n", rp->printer->printer->name);
			while ((n = Read(reqfd, buf, BUFSIZ)) > 0)
			(void) fwrite (buf, 1, n, logfp);
			(void) Close (reqfd);
			(void) fflush (logfp);
		}
		}
		(void) Unlink (path);
		Free (path);
	}

	if (file)
	{
	    if ( isremote == True)
	    {
		/* create template of pathnames for tmp/.net jobfiles
		 */
		basename = makestr(getreqno(rp->secure->req_id),
			"-",MOST_FILES_S,(char *)0);
		path = makepath(Lp_NetTmp,"tmp",rp->secure->system,
			basename,(char *)0);

		/* get pointer of where to position the 'n' part
		 * of the XXX-n files pathname
		 */
		p = path + (strlen( path) - strlen( MOST_FILES_S));
	    }

	    while( *file)
	    {
		/* The  remove copies of user files.
		 */
		if (STRNEQU(Lp_Temp, *file, strlen(Lp_Temp)) ||
		    STRNEQU(Lp_Tmp, *file, strlen(Lp_Tmp)))
		{
			(void) Unlink(*file);
		}

		count++;
		file++;

		if ( isremote == True)
		{
			/* create pathname of next jobfile to remove
			 */
			(void) sprintf(p, "%d", count);
			(void) Unlink (path);
		}
	    }

	    if ( isremote == True)
	    {
		Free (basename);
		Free (path);
	    }
	}

	if (rp->secure && rp->secure->req_id) {
	p = getreqno(rp->secure->req_id);

	/*
	 * The filtered files. We can't rely on just the RS_FILTERED
	 * flag, since the request may have been cancelled while
	 * filtering. On the other hand, just checking "rp->slow"
	 * doesn't mean that the files exist, because the request
	 * may have been canceled BEFORE filtering started. Oh well.
	 */
	if (rp->slow)
		while(count > 0)
		{
		(void) sprintf(num, "%d", count--);
		path = makestr(Lp_Temp, "/F", p, "-", num, (char *)0);
		(void) Unlink(path);
		Free(path);
		}

	/*
	 * The notify/error file.
	 */
	path = makepath(Lp_Temp, p, (char *)0);
	(void) Unlink(path);
	Free(path);
	}
}

/**
 ** _alloc_req_id() - ALLOCATE NEXT REQUEST ID
 **/

#define	SEQF_DEF_START	1
#define	SEQF_DEF_END	BIGGEST_REQID
#define	SEQF_DEF_INCR	1
#define	SEQF		".SEQF"

/*
 * Procedure:     _alloc_req_id
 *
 * Restrictions:
 *               fopen: None
 *               rewind: None
 *               fscanf: None
 *               fprintf: None
 *               fflush: None
*/

long
#ifdef	__STDC__
_alloc_req_id (
	void
)
#else
_alloc_req_id ()
#endif
{
	DEFINE_FNNAME (_alloc_req_id)

	static short		started	= 0;

	static FILE		*fp;

	static long		start;
	static long		end;
	static long		incr;
	static long		curr;
	long			atol();

	static char		fmt[
				STRSIZE(BIGGEST_REQID_S)/* start   */
				  + 1			/* :	   */
				  + STRSIZE(BIGGEST_REQID_S)/* end	 */
				  + 1			/* :	   */
				  + STRSIZE(BIGGEST_REQID_S)/* incr	*/
				  + 1			/* :	   */
				  + 4			/* %ld\n   */
				  + 1			/* (nul)   */
				];

	char *			file;

	long			ret;


	if (!started) {
		file = makepath(Lp_Temp, SEQF, (char *)0);
		if (
			!(fp = fopen(file, "r+"))
			 && !(fp = fopen(file, "w"))
		)
			lpfail (ERROR, E_SCH_CANTOP, file, PERROR);

		rewind (fp);
		if (fscanf(fp, "%ld:%ld:%ld:%ld\n", &start, &end, &incr, &curr) != 4) {
			start = SEQF_DEF_START;
			end = SEQF_DEF_END;
			curr = start;
			incr = SEQF_DEF_INCR;
		}

		if (start < 0)
			start = SEQF_DEF_START;
		if (end > (atol(BIGGEST_REQID_S)))
			end = SEQF_DEF_END;
		if (curr < start || curr > end)
			curr = start;

		(void) sprintf (fmt, "%ld:%ld:%ld:%%ld\n", start, end, incr);
		started = 1;
	}

	ret = curr;

	if ((curr += incr) >= end)
		curr = start;

	rewind (fp);
	(void) fprintf (fp, fmt, curr);
	(void) fflush (fp);

	return (ret);
}



/*
 * Procedure:     _alloc_files
 *
 * Restrictions:
 *               Access: None
 *               Open: None
 *               Chlvl: None
 *               Chown: None
 *
 * Notes - ALLOCATE FILES FOR A REQUEST
 */

char *
#ifdef	__STDC__
_alloc_files (
	int			num,
	char *			prefix,
	uid_t			uid,
	gid_t			gid,
	level_t			lid
)
#else
_alloc_files(num, prefix, uid, gid, lid)
	int			num;
	char			*prefix;
	uid_t			uid;
	gid_t			gid;
	level_t			lid;
#endif
{
	DEFINE_FNNAME (_alloc_files)

	static char		base[
				1			/* F	   */
				  + STRSIZE(BIGGEST_REQID_S)/* req-id  */
				  + 1			/* -	   */
				  + STRSIZE(MOST_FILES_S)	/* file-no */
				  + 1			/* (nul)   */
				];

	char *			file;
	char *			cp;

	int			fd;
	int			plus;


	if (num > MOST_FILES)
		return (0);

	if (!prefix) {
		(void) sprintf (base, "%d-%d", (int) _alloc_req_id(), MOST_FILES);
		plus = 0;
	} else {
		if ((int)strlen(prefix) > 6)
			return (0);
		(void) sprintf (base, "F%s-%d", prefix, MOST_FILES);
		plus = 1;
	}

	file = makepath(Lp_Temp, base, (char *)0);
		
	cp = strrchr(file, '-') + 1;
	/*
	 * Check queue slot and
	 * prevent overwriting of print
	 * job already queued
	*/
	(void) sprintf (cp, "%d", plus);
	if ( !prefix && Access(file, 0) == 0 ) {
		Free (file);
		errno = EEXIST;
		return (0);
	}
	while (num--) {
		(void) sprintf (cp, "%d", num + plus);
		if ((fd = Open(file, O_CREAT|O_TRUNC, 0600)) == -1) {
			Free (file);
			return (0);
		} else {
			(void)	Close (fd);
			(void)	Chlvl (file, lid);
			(void)	Chown (file, uid, gid);
		}
	}

	Free (file);

	if ((cp = strrchr(base, '-')))
		*cp = 0;

	return (base);
}
