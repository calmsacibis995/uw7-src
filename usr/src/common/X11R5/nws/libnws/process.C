#ident	"@(#)process.C	1.2"
/******************************************************************************
 *		Process class - put up the list of Processes.
 ******************************************************************************/
#include <iostream.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>

#include "process.h"
#include "proclist.h"
#include "procitem.h"
#include "device.h"

/******************************************************************************
 *  FUNCTION:
 * 	ctor
 *  DESCRIPTION:
 *	Process Constructors/Destructors. Initialize process list and device info.
 *  RETURN:
 *  Nothing
 ******************************************************************************/
Process::Process ()
{
#ifdef DEBUG
cout << "ctor for Process " << endl;
#endif
	_processlist = new ProcList();
	_devl = NULL;
}
		
/******************************************************************************
 *  FUNCTION:
 * 	dtor
 *  DESCRIPTION:
 *  Destructor to delete the process list when destroying the Process Class
 *  RETURN:
 *  Nothing
 ******************************************************************************/
Process::~Process ()
{
#ifdef DEBUG
cout << "dtor for Process " << endl;
#endif
	delete _processlist;
}

/******************************************************************
 *  FUNCTION:
 *	Boolean SetupProcData()
 *  DESCRIPTION:
 *  Get Process Info. Loop thru the proc table and create a linked
 *  list of ProcItem objects that need to be accessed later.
 *  RETURN:
 *  Found or not found.
 ******************************************************************/
Boolean
Process::SetupProcData ()
{
	DIR 			*dirp;
	struct dirent 	*dentp;
	char			pname[100], *procdir;
	int				rc, pdlen, total;
	psinfo_t		eachps;

	/* Get the device data afresh each time we ask for proc data
	 */
	_ndev = 0;
	if (_devl)
		delete [] _devl;
	if ((GetDeviceData()) == False){
			cout << "could not open device data " << endl;
			return False;
	}

	procdir = strdup("/proc");

	total = 0;
	/*
	 * Determine which processes to print info about by searching
	 * the /proc directory and looking at each process.
	 */
	if ((dirp = opendir(procdir)) == NULL) {
			cout << "could not open procdir " << endl;
			return False;
	}

	(void) strcpy(pname, procdir);
	pdlen = strlen(pname);
	pname[pdlen++] = '/';

	/* for each active process --- */
	while (dentp = readdir(dirp)) {
			
		int	procfd;		/* filedescriptor for /proc/nnnnn */

		if (dentp->d_name[0] == '.')		/* skip . and .. */
			continue;
		(void) strcpy(pname + pdlen, dentp->d_name);
		(void) strcat(pname, "/psinfo");
retry:
		if ((procfd = open(pname, O_RDONLY)) == -1) {
			cout << "Cannot open PROC directory" << pname << endl;
			continue;
		}

		/*
	 	 * Get the info structure for the process and close quickly.
	 	 */
		if ((rc = read(procfd, &eachps, sizeof(eachps))) != sizeof(eachps)) {
			int	saverr = errno;

			(void) close(procfd);
			if (rc != -1)
				continue;
			else if (saverr == EACCES)
				continue;
			else if (saverr == EAGAIN)
				goto retry;
			else if (saverr != ENOENT)
				continue;
		}
		
		/* Store each proc info into a structure, reallocate space
		 * for the process structure each time
		 */
		if (!total) 
			_processlist->insert(eachps);
		else 
			_processlist->append(eachps);

		total++;

		(void) close(procfd);
	}
	(void) closedir(dirp);
	free (procdir);

	return True;
}

/******************************************************************
 *  FUNCTION:
 *	char * GetTTY(psinfo_t *)
 *  DESCRIPTION:
 * 	GetTTY returns the user's tty number or ? if none.
 *  RETURN:
 *  tty info.
 ******************************************************************/
char *
Process::GetTTY(psinfo_t *eachps)	/* where the search left off last time */
{
	int			i;

	if (eachps->pr_ttydev != PRNODEV) {
		for (i = 0; i < _ndev; i++) {
			if (_devl[i].dev == eachps->pr_ttydev) 
				return _devl[i].dname;
		}
	}
	return "?";
}

/**************************************************************
 *  FUNCTION:
 *	Boolean GetDeviceData()
 *  DESCRIPTION:
 *  Get the device data here.  The class device is instantiated
 *  and filled and later deleted when new process info is required.
 *  RETURN:
 *  True or False. If device data was found or not.
 **************************************************************/
Boolean
Process::GetDeviceData()
{
	struct stat sbuf1, sbuf2;
	int fd, i;

	fd = open("/etc/ps_data", O_RDONLY);

	if(fd == -1)	{	/* Restore privs before returning error.*/
		cout << "falsie here " << endl;
		return False;
	}

	if (fstat(fd, &sbuf1) < 0
	  || sbuf1.st_size == 0
	  || stat("/etc/passwd", &sbuf2) == -1 
	  || sbuf1.st_mtime <= sbuf2.st_mtime
	  || sbuf1.st_mtime <= sbuf2.st_ctime) {
		(void) close(fd);
		cout << "fstat failed here " << endl;
		return  False;
	}

	/* Read /dev data from /etc/ps_data. */
	if (read(fd, (char *) &_ndev, sizeof(_ndev)) == 0)  {
		(void) close(fd);
		cout << "read failed here " << endl;
		return False;
	}

	_devl =  new Device [_ndev];
	if (_devl == NULL) {
		cout << "devl is null " << endl;
		return False;
	}

	if (read(fd, (char *)_devl, _ndev * sizeof(*_devl)) == 0)  {
		(void) close(fd);
		cout << "read of devl failed" << endl;
		return False;
	}

	/* See if the /dev information is out of date */
	for (i=0; i<_ndev; ++i) {
		if (_devl[i].dev == PRNODEV) {
			char buf[DNSIZE+5];
			strcpy(buf, "/dev/");
			strcat(buf, _devl[i].dname);
			if (stat(buf, &sbuf2) ==  -1 ||
			    sbuf1.st_mtime <= sbuf2.st_mtime ||
			    sbuf1.st_mtime <= sbuf2.st_ctime) {
				(void) close(fd);
				cout << "stat of /dev failed" << endl;
				return False;	/* Out of date */
			}
		}
	}
	(void) close(fd);
	return True;
}

/**************************************************************
 *  FUNCTION:
 *	psinfo_t * GetProcStructInfo(pid_t , int signo)
 *  DESCRIPTION:
 *  Return the psinfostruct for the process by pid. 
 *  RETURN:
 * 	psinfo structure or NULL if not found
 **************************************************************/
psinfo_t *
Process::GetProcStructInfo(pid_t pid)
{
	ProcItem		*tmp;

	tmp = _processlist->FindProcItem (pid);
	return (tmp == NULL ? NULL : tmp->GetPSInfoStruct());
}

/**************************************************************
 *  FUNCTION:
 *	psinfo_t * GetProcStructInfo(pid_t , int signo)
 *  DESCRIPTION:
 *  Return the psinfostruct for the process by position. 
 *  RETURN:
 * 	psinfo structure or NULL if not found
 **************************************************************/
psinfo_t * 
Process::GetProcStructInfo(int position)
{
	ProcItem		*tmp;

	tmp = _processlist->FindProcItem (position);
	return (tmp == NULL ? NULL : tmp->GetPSInfoStruct());
}

/**************************************************************
 *  FUNCTION:
 *	lwpsinfo_t ** GetLWPStructInfo(pid_t , int signo)
 *  DESCRIPTION:
 *  Return the lwpsinfostruct for the process by pid. 
 *  RETURN:
 * 	lwpsinfo structure or NULL if not found
 **************************************************************/
lwpsinfo_t ** 
Process::GetLWPStructInfo(int position)
{
	ProcItem		*tmp;

	tmp = _processlist->FindProcItem (position);
	return (tmp == NULL ? NULL : tmp->GetLWPSInfoStruct());
}

/**************************************************************
 *  FUNCTION:
 *	lwpsinfo_t ** GetLWPStructInfo(pid_t , int signo)
 *  DESCRIPTION:
 *  Return the lwpsinfostruct for the process by pid. 
 *  RETURN:
 * 	lwpsinfo structure or NULL if not found
 **************************************************************/
lwpsinfo_t ** 
Process::GetLWPStructInfo(pid_t pid)
{
	ProcItem		*tmp;

	tmp = _processlist->FindProcItem (pid);
	return (tmp == NULL ? NULL : tmp->GetLWPSInfoStruct());
}

/******************************************************************************
 *  FUNCTION:
 *	Boolean SignalProcess(pid_t , int signo)
 *  DESCRIPTION:
 *  Send a signal to the process by passing in the process.
 *  RETURN:
 * 	True or False
 ******************************************************************************/
Boolean 
Process::SignalProcess(pid_t pid, int signo)
{
	ProcItem		*tmp;
	psinfo_t		*psinfostruct;

	tmp = _processlist->FindProcItem (pid);
	if (tmp == NULL)
		return False;
	psinfostruct = tmp->GetPSInfoStruct();
	if ((kill (psinfostruct->pr_pid, signo)) == -1)
		return False;
	else
		return True;
}

/******************************************************************************
 *  FUNCTION:
 *	Boolean SignalProcess(int position, int signo)
 *  DESCRIPTION:
 *  Send a signal to the process by passing in the position in the list.
 *  RETURN:
 * 	True or False
 ******************************************************************************/
Boolean 
Process::SignalProcess(int position, int signo)
{
	ProcItem		*tmp;
	psinfo_t		*psinfostruct;

	tmp = _processlist->FindProcItem (position);
	if (tmp == NULL)
		return False;
	psinfostruct = tmp->GetPSInfoStruct();
	if ((kill (psinfostruct->pr_pid, signo)) == -1)
		return False;
	else
		return True;
}

/******************************************************************
 *  FUNCTION:
 *	char *SetCommandName (char *)
 *  DESCRIPTION:
 *  Get the Command name from the argument.  Ignore the parameters.  
 *  RETURN:
 * 	nothing
 ******************************************************************/
void
Process::SetCommandName (char	*commandname)
{
	while (*commandname++) {
		if (*commandname == ' ') {
				*commandname = '\0';
				break;
		}
	}
}

/******************************************************************
 *  FUNCTION:
 *	char *GetUserName (uid_t)
 *  DESCRIPTION:
 *  Get the User name from the User id.  
 *  RETURN:
 *	A pointer to the username
 ******************************************************************/
char *
Process::GetUserName (uid_t	uid)
{
	struct passwd		*pwd;
	char				*username;

	setpwent ();
	while (pwd = getpwent()) {
		if (pwd->pw_uid == uid) {
			username = strdup (pwd->pw_name);
			break;
		}
   } 
	endpwent ();
	return username;
}
