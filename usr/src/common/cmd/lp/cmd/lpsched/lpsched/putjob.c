/*		copyright	"%c%" 	*/


#ident	"@(#)putjob.c	1.2"
#ident  "$Header$"

#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "lpsched.h"

#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"

/*
 * Procedure:     makelink
 *
 * Restrictions:
									Unlink: none
									Link: none
									Symlink: none
*/
static void
#ifdef	__STDC__
makelink (
	char *			src,
	char *			dst
)
#else
makelink (src, dst)
	char *			src;
	char *			dst;
#endif
{
	DEFINE_FNNAME (makelink)

	(void)Unlink (dst);
	if (Link(src, dst) == -1 && Symlink(src, dst) == -1)
		lpfail (ERROR, E_SCH_SYMLINK, src, dst, PERROR);
	return;
}

/*
 * Procedure:     chk_net_tmp
 *
 * Restrictions:
 *               stat(2): None
 *               mkdir(2): None
 *               Chmod: None
 *               Chown: None
 *               lvlfile(2): None
*/
static void
#ifdef	__STDC__
chk_net_tmp (
	char			*sysname
)
#else
chk_net_tmp (sysname)
	char			*sysname;
#endif
{
	DEFINE_FNNAME (chk_net_tmp)

	char			*path;
	int			exist;
	mode_t			mode = 0770;
	uid_t			uid = Lp_Uid;
	gid_t			gid = Lp_Gid;
	level_t			lid = LPSCHED_SYS_PRIVATE;
	struct stat		stbuf;

	path = makepath(Lp_NetTmp, "tmp", sysname, (char *)0);

	exist = (stat(path, &stbuf) == 0);

	if (exist && (stbuf.st_mode & S_IFDIR) == 0)
		lpfail (ERROR, E_SCH_NOTDIR, path);
	if (!exist)
	{
		(void) Mkdir (path, 0);
		(void) Chmod (path, mode);
		(void) Chown (path, uid, gid);
		(void) lvlfile (path, MAC_SET, &lid);

	}
	Free (path);
	path = makepath(Lp_NetTmp, "requests", sysname, (char *)0);

	exist = (stat(path, &stbuf) == 0);

	if (exist && (stbuf.st_mode & S_IFDIR) == 0)
		lpfail (ERROR, E_SCH_NOTDIR, path);
	if (!exist)
	{
		(void) Mkdir (path, 0);
		(void) Chmod (path, mode);
		(void) Chown (path, uid, gid);
		(void) lvlfile (path, MAC_SET, &lid);

	}
	Free (path);

	return;
}
/*
 * Procedure:     putjobfiles
 *
 * Restrictions:
 *               putrequest: None
 *               lvlfile(2): None
 *               putsecure: None
*/

void 
#ifdef	__STDC__
putjobfiles (
	RSTATUS *		prs
)
#else
putjobfiles (prs)
	RSTATUS *		prs;
#endif
{
	DEFINE_FNNAME (putjobfiles)

	char **			listp;
	char **			flist;

	char *			reqno;
	char *			basename;
	char *			src_fdf		= 0;
	char *			src_fdf_no;
	char *			dst_df;
	char *			dst_df_no;
	char *			bogus;
	char *			bogus_no;
	char *			rfile;
	char *			system_name;

	int			count;

	RSTATUS			rs;

	REQUEST			rtmp;

	SECURE			stmp;


	/*
	 * WARNING! DON'T FREE PREVIOUSLY ALLOCATED POINTERS WHEN
	 * REPLACING THEM WITH NEW VALUES IN rs.secure AND rs.request,
	 * AS THE ORIGINAL POINTERS ARE STILL IN USE IN THE ORIGINAL
	 * COPIES OF THESE STRUCTURES.
	 */

	rs = *(prs);
	rtmp = *(prs->request);
	rs.request = &rtmp;
	stmp = *(prs->secure);
	rs.secure = &stmp;

	reqno = getreqno (rs.secure->req_id);
	system_name = Strdup (rs.secure->system);

	/*
	*  If originating system is remote,
	*  we have to change system name in outgoing
	*  request file so that forwarding system
	*  will receive R_SEND_JOB.
	*/
	if (!STREQU(Local_System, rs.secure->system))
		rs.secure->system = Strdup (Local_System);


	/*
	 * Link the user's data files into the network temporary
	 * directory, and construct a new file-list for a copy
	 * of the request-file.
	 * First check that system specific sub-directories are
	 * available and create them if necessary.
	 */

	chk_net_tmp (system_name);


	if (rs.request->outcome & RS_FILTERED)
	{
		basename = makestr ("F", reqno, "-", MOST_FILES_S, (char *)0);
		src_fdf =
		makepath (Lp_Tmp, Local_System, basename, (char *)0);
		src_fdf_no = strchr (src_fdf, 0) - STRSIZE (MOST_FILES_S);
		Free (basename);
	}

	basename = makestr (reqno, "-", MOST_FILES_S, (char *)0);
	dst_df =
	makepath (Lp_NetTmp, "tmp", system_name, basename, (char *)0);
	dst_df_no = strchr (dst_df, 0) - STRSIZE (MOST_FILES_S);
	bogus = makepath (Lp_Tmp, system_name, basename, (char *)0);
	bogus_no = strchr (bogus, 0) - STRSIZE (MOST_FILES_S);
	Free (basename);

	count = 0;
	flist = 0;
	for (listp = rs.request->file_list; *listp; listp++) {
		char *	src_df;

		count++;

		/*
		 * Link the next data file to a name in the
		 * network temporary directory.
		 */
		(void) sprintf (dst_df_no, "%d", count);
		if (rs.request->outcome & RS_FILTERED)
		{
			(void) sprintf (src_fdf_no, "%d", count);
			src_df = src_fdf;
		} else
			src_df = *listp;
		makelink (src_df, dst_df);

		/*
		 * Add this name to the list we'll put in the
		 * request file. Note: The prefix of this name
		 * is bogus; the "lpNet" daemon will replace it
		 * with the real prefix.
		 */
		(void) sprintf (bogus_no, "%d", count);
		(void) appendlist (&flist, bogus);
	}

	if (src_fdf)
		Free (src_fdf);
	Free (dst_df);
	Free (bogus);


	/*
	 * Change the content of the request and secure files,
	 * to reflect how they should be seen on the remote side.
	 */
	if (rs.request->alert)
		rs.request->alert = 0;
	rs.request->actions &= ~(ACT_WRITE|ACT_MAIL);
	rs.request->actions |= ACT_NOTIFY;
	rs.request->file_list = flist;
	rs.request->destination = Strdup (rs.printer->remote_name);
	if (strchr(rs.secure->user, BANG_C))
		rs.secure->user = Strdup (rs.secure->user);
	else
		rs.secure->user =
		makestr(Local_System, BANG_S, rs.secure->user, (char *)0);
	rs.secure->status &= ~SC_STATUS_ACCEPTED;

	if (rs.request->outcome & RS_FILTERED) {
		rs.request->input_type = Strdup (rs.slowparm->type);
		rs.request->modes = Strdup (rs.slowparm->modes);

		if (rs.request->pages && STREQU (rs.request->pages,
						rs.slowparm->pages))
			rs.request->pages = (char *)0;

		if (rs.request->copies == rs.slowparm->copies)
			rs.request->copies = 1;
	}
	/*
	**  Copy the request and secure files to the network temporary
	**  directory.
	**
	**  ES Note:
	**  The secure file will be at SYS_PRIVATE but the request
	**  will need to have its level changed to the user.
	*/

	basename = makestr(reqno, "-0", (char *)0);

	rfile =
	makepath(Lp_NetTmp, "tmp", system_name, basename, (char *)0);
	if (putrequest(rfile, rs.request) == -1)
		lpfail (ERROR, E_SCH_PUTREQ, rfile, PERROR);
	(void)	lvlfile (rfile, MAC_SET, &rs.secure->lid);

	Free (rfile);

	rfile =
		makepath(Lp_NetTmp, "requests", system_name,
		basename, (char *)0);
	if (putsecure (rfile, rs.secure) == -1)
		lpfail (ERROR, E_SCH_PUTSEC, rfile, PERROR);
	Free (rfile);

	Free (basename);
	Free (system_name);

	freelist (rs.request->file_list);
	Free (rs.request->destination);
	Free (rs.secure->user);

	return;

}
