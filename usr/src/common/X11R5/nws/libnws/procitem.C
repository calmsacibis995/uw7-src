#ident	"@(#)procitem.C	1.2"
/*****************************************************************************
 *		ProcItem class - put up the list of ProcItems.
 *****************************************************************************/
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "procitem.h"

/*****************************************************************************
 *  FUNCTION:
 *	ctor
 *  DESCRIPTION:
 *	ProcItem Constructors
 *  Initialize variables, psinfo structure and copy info into it.
 *  RETURN:
 *  nothing
 *****************************************************************************/
ProcItem::ProcItem (psinfo_t&  eachps, ProcItem *item)
{
#ifdef DEBUG
cout << "ctor for ProcItem" << endl;
#endif
	_psinfostruct = new psinfo_t;
	memcpy (_psinfostruct, &eachps, sizeof(psinfo_t));
	_lwpsinfostruct = NULL;
	_next = item;

	GetLWP();
}
		
/*****************************************************************************
 *  FUNCTION:
 *	dtor
 *  DESCRIPTION:
 *	ProcItem Destructors
 *  RETURN:
 *  nothing
 *****************************************************************************/
ProcItem::~ProcItem ()
{
#ifdef DEBUG
cout << "dtor for ProcItem" << endl;
#endif
	delete _psinfostruct;
	delete []_lwpsinfostruct;
	_next = 0;
}

/*****************************************************************************
 *  FUNCTION:
 *	GetLWP()
 *  DESCRIPTION:
 *  Obtain an lwpsinfo structure  - If Lflg is specified
 * 	call all lwps for process else only the representative lwp
 *  RETURN:
 *  nothing
 *****************************************************************************/
void 
ProcItem::GetLWP()
{
	DIR 			*dirp;
	struct dirent 	*dentp;
	char			fnam[100];
	lwpid_t			lwp;
	int				lwpi = 0;
	char 			*procdir = "/proc";

	/*
	 * If process is zombie, call print routine and return.
	 */
	if (_psinfostruct->pr_lwp.pr_lwpid == 0)
			return;

	sprintf(fnam, "%s/%d/lwp", procdir, _psinfostruct->pr_pid);

	dirp = opendir(fnam);

	if (dirp == NULL) {
		if (errno == ENOENT) {
			/*
 			 *  The process has become a zombie since we read psinfo structure,
 			 *  but before we could read the lwp directory.  Kludge the psinfo
 			 *  structure to reflect the fact that the process is now a zombie.
 			 */
			_psinfostruct->pr_nlwp = 0;
			_psinfostruct->pr_lwp.pr_sname = 'z';
			return;
		}

		/*
 	 	 *  Be silent about not being able to open the lwp directory,
 	 	 *  since this may not reflect an error condition
 	 	 */
		return;
	}

	/* Create space for the lwpsinfostruct structure
	 */
	_lwpsinfostruct = new lwpsinfo_t* [_psinfostruct->pr_nlwp];

	/* for each active lwp 
	 */
	while (dentp = readdir(dirp)) {
		if (dentp->d_name[0] == '.')	/* skip . and .. */
			continue;

		lwp = (lwpid_t) atol (dentp->d_name);

		GetEachLWP(lwpi++, lwp);
	}

	closedir(dirp);
}

/*****************************************************************************
 *  FUNCTION:
 *	GetEachLWP()
 *  DESCRIPTION:
 *  Read the lwpsinfo file for the specified lwp
 *  RETURN:
 *  nothing
 *****************************************************************************/
void
ProcItem::GetEachLWP(int lwpi, lwpid_t lwp)
{
	char 		fnam[200];
	int 		fd;
	int 		rc;
	char 		*procdir = "/proc";

	sprintf(fnam,"%s/%d/lwp/%d/lwpsinfo", procdir, _psinfostruct->pr_pid, lwp);

retry:
	if ((fd = open(fnam, O_RDONLY)) == -1)
		return;

	/*
	 * Get the info structure for the lwp and close quickly.
	 */

	rc = read(fd, &_psinfostruct->pr_lwp, sizeof (_psinfostruct->pr_lwp));

	if (rc != sizeof(_psinfostruct->pr_lwp)) {
		int	saverr = errno;

		(void) close(fd);
		if (rc != -1)
				cout << "Unexpected return value %d on %s\n" << endl;
		else if (saverr == EACCES)
			return;
		else if (saverr == EAGAIN)
			goto retry;
		else if (saverr != ENOENT)
			  cout << "lwpsinfo on %s: %s\n" <<  fnam << endl;
		return ;
	}

	/* Create space for each lwp in the lwpsinfostructure
	 */
	_lwpsinfostruct[lwpi] = new lwpsinfo_t;
	memcpy (_lwpsinfostruct[lwpi], &_psinfostruct->pr_lwp, sizeof(lwpsinfo_t));

	(void) close(fd);
}
