#ident "@(#)if.c	29.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include "include.h"
#include <errno.h>

#ifdef _REENTRANT
#include <synch.h>
extern mutex_t mutex;
#endif

extern int errno;

static struct dlpidev *dlpi_head = (struct dlpidev *)0;

static struct dlpidev *
FindDlpiDev(char *dlpi_name)
{
	struct dlpidev *dp;

	for (dp = dlpi_head; dp; dp=dp->next) {
		if (strcmp(dlpi_name, dp->dlpi_name) == 0)
			return (dp);
	}
	return((struct dlpidev *)0);
}

static struct dlpidev *
FindDlpiFromMdi(char *mdi_name)
{
	struct dlpidev *dp;

	for (dp = dlpi_head; dp; dp=dp->next) {
		if (strcmp(mdi_name, dp->mdi->mdi_name) == 0)
			return (dp);
	}
	return((struct dlpidev *)0);
}

static struct dlpidev *
FindDlpiDevFd(int fd)
{
	struct dlpidev *dp;

	for (dp = dlpi_head; dp; dp=dp->next) {
		if (dp->dlpi_fd == fd)
			return (dp);
	}
	return((struct dlpidev *)0);
}

static struct mdidev *
FindMdiDev(struct dlpidev *dp, char *mdi_name)
{
	struct mdidev *mp, *mpstart;

	mp = mpstart = dp->mdi;
	do {
		if (strcmp(mdi_name, mp->mdi_name) == 0)
			return (mp);
		mp=mp->next;
	} while (mp != mpstart);
	return((struct mdidev *)0);
}

void
DumpIfStructs()
{
	struct dlpidev *dp;

	for (dp=dlpi_head; dp; dp=dp->next) {
		struct mdidev *mp, *mpstart;

		Log(" %s(%s) fd=%d, %s", dp->dlpi_name, dp->dlpi_path, dp->dlpi_fd, dp->mdi_active ? "active" : "");
		
		mp = mpstart = dp->mdi;
		do {
			Log("   %s(%s) backup(%s)", mp->mdi_name, mp->mdi_path, mp->mdi_backup_name);
			mp=mp->next;
		} while (mp != mpstart);
	}
}

/*******************************************************************************
 * Functions manipulating to get and set the multicast address table
 * so that it can be preserved across a hardware restart
 ******************************************************************************/

int
ReadHWFailInd(struct dlpidev *dp)
{
	int fd = dp->dlpi_fd;
	static char buf[1024];
	mac_hwfail_ind_t *hwf;
	struct strbuf ctl;
	int flags;

#ifdef DEBUG
	Log("ReadHWFail for dlpi %s\n", dp->dlpi_name);
#endif

	ctl.buf = buf;
	ctl.maxlen = sizeof(buf);
	ctl.len = 0;

	flags=RS_HIPRI;
	if (getmsg(fd, &ctl, (unchar *)0, &flags) < 0) {
		SystemError(12, "dlpid: MAC_HWFAIL_IND is corrupt\n");
		return(0);
	}
	hwf = (mac_hwfail_ind_t *)buf;
	return(1);
}

void
dlpiHandler(int fd, int pollflags)
{
	struct dlpidev *dp;

#ifdef DEBUG
	Log("dlpiHandler (fd=%d, flags=0x%x)", fd, pollflags);
#endif

	if (!(dp = FindDlpiDevFd(fd))) {
		Error(19, "dlpid: dlpiHandler: Unable to find fd=%d in DLPI List\n",fd);
		goto error;
	}

	if (!ReadHWFailInd(dp)) {
		Error(20, "dlpid: dlpiHandler: Unable to Read MAC_HW_FAIL_IND from DLPI interface (%s)\n", dp->dlpi_name);
		goto error;
	}
	if (!RestartIf(dp, 1)) {
		Log("dlpiHandler: Unable to Restart DLPI Interface (%s)", dp->dlpi_name);
		dp->dlpi_reopening = 1;
		startRestartHandler();
		goto error;
	}
	return;

error:
	RemoveInput(fd);
}

/*******************************************************************************
 * Functions manipulating the DLPI/MDI dev structures which take pointers
 * as their arguments.
 ******************************************************************************/

int
StartMdiIf(struct dlpidev *dp)
{
	struct strioctl si;

	Log("StartMdiIf (DLPI %s, MDI %s)", dp->dlpi_name, dp->mdi->mdi_name);
	if (dp->mdi_active)
	{
		Error(21, "dlpid: DLPI Interface (%s) already started\n", dp->dlpi_name);
		goto error2;
	}

	Log("DLPI driver %s, fd=%d", dp->dlpi_path, dp->dlpi_fd);

	/* DLPI->MDI driver link hasn't been built yet,
	 * so do it now */

	/* Open the MDI driver */
	if ((dp->mdi_fd = open (dp->mdi->mdi_path, O_RDWR)) == -1)
	{
		SystemError(23, "dlpid: Unable to open network adapter driver (%s)\n",
					    dp->mdi->mdi_path);
		
		goto error1;
	}

	Log("Opened MDI driver (%s), fd=%d", dp->mdi->mdi_name, dp->mdi_fd);

	/* in the netinstall case, we want to be able to stop dlpid and restart it
	 * with a different interface.  rather than use complicated signal handlers
	 * with sigprocmask we take the easy way out and use I_LINK instead so
	 * we will I_UNLINK automatically in strclose when we exit upon receiving
	 * a signal we don't catch (like SIGTERM).  We don't have to worry about
	 * modifying the I_PUNLINK code since we won't ever get there in netisl case
	 * as FIFO doesn't exist
	 */
	if (ioctl(dp->dlpi_fd, netisl ? I_LINK : I_PLINK, dp->mdi_fd) == -1)
	{
		SystemError(25, "dlpid: Unable to link DLPI module (%s) onto network adapter driver (%s)\n",
							dp->dlpi_path, dp->mdi->mdi_path);
		goto error;
	}

	dp->mdi_active = 1;
	return(1);

error:
	close(dp->mdi_fd);
	dp->mdi_fd = -1;

error1:
	close(dp->dlpi_fd);
	dp->dlpi_fd = -1;

error2:
	return(0);						/* ERROR */
}

int
StartIf(struct dlpidev *dp)
{
	struct strioctl si;
	int regstatus;

	Log("StartIf (DLPI %s, MDI %s)", dp->dlpi_name, dp->mdi->mdi_name);
	if (dp->mdi_active)
	{
		Error(21, "dlpid: DLPI Interface (%s) already started\n", dp->dlpi_name);
		goto error2;
	}

	/* Open the DLPI module */
	if ((dp->dlpi_fd = open (dp->dlpi_path, O_RDWR)) == -1)
	{
		SystemError(22, "dlpid: Unable to open DLPI module (%s)\n", dp->dlpi_path);
		goto error2;
	}
	Log("Opened DLPI driver %s, fd=%d", dp->dlpi_path, dp->dlpi_fd);

	/* Register with the DLPI module */
	si.ic_cmd = DLPID_REGISTER;
	si.ic_timout = 0;
	si.ic_dp = NULL;
	si.ic_len = 0;

	/* ksl support removed; see SCCS */
	if ((regstatus=ioctl(dp->dlpi_fd, I_STR, &si)) < 0)
	{
		/* dlpi module indicates that someone else has already registered.
		 * errno is EBUSY.
		 */
		Log("this or another dlpid already registered, continuing");
	}
	else
	{
		if (regstatus == 0)
		{
			/* DLPI->MDI driver link hasn't been built yet,
			 * so do it now */

			/* Open the MDI driver */
			if ((dp->mdi_fd = open (dp->mdi->mdi_path, O_RDWR)) == -1)
			{
				SystemError(23, "dlpid: Unable to open network adapter driver (%s)\n",
							    dp->mdi->mdi_path);
				
				goto error1;
			}

			Log("Opened MDI driver (%s), fd=%d", dp->mdi->mdi_name, dp->mdi_fd);

			/* in the netinstall case, we want to be able to stop dlpid and 
			 * restart it with a different interface.  rather than use complicated
			 * signal handlers with sigprocmask we take the easy way out and use
			 * I_LINK instead so we will I_UNLINK automatically in strclose when 
			 * we exit upon receiving a signal we don't catch (like SIGTERM).  We 
			 * don't have to worry about modifying the I_PUNLINK code since we 
			 * won't ever get there in netisl case as FIFO doesn't exist
			 */
			if (ioctl(dp->dlpi_fd, netisl ? I_LINK : I_PLINK, dp->mdi_fd) == -1)
			{
				SystemError(25, "dlpid: Unable to link DLPI module (%s) onto network adapter driver (%s)\n",
									dp->dlpi_path, dp->mdi->mdi_path);
				goto error;
			}

			/* ksl support (issuing DLPID_REGISTER yet again) removed; see SCCS */
		}
		else
		{
			SystemError(27, "dlpid: DLPI module busy, possible already in use\n");
			goto error1;
		}
	}

	Log("StartIF dlpi %s adding dlpiHandler(0x%x) for dlpi_fd %d", dp->dlpi_name, dlpiHandler, dp->dlpi_fd);
	AddInput(dp->dlpi_fd, dlpiHandler);
	dp->mdi_active = 1;
	return(1);

error:
	close(dp->mdi_fd);
	dp->mdi_fd = -1;

error1:
	close(dp->dlpi_fd);
	dp->dlpi_fd = -1;

error2:
	return(0);						/* ERROR */
}

int
RemoveIf(struct dlpidev *dp, struct mdidev *mp)
{
	int last_mdi = (mp->next == mp);

	/* obtain mutex so that if a signal occurs it won't traverse a
	 * potentially bogus linked list
	 */
	if (MUTEX_TRYLOCK(&mutex) == EBUSY) {
		/* shouldn't happen, as we're not called from signal handler */
		Error(48, "dlpid: RemoveIf: cannot obtain mutex\n");
		return(0);
	}

	Log("RemoveIf STARTS (dp(%s)=0x%x, mp(%s)=0x%x, last_mdi=%d)", dp->dlpi_name, dp, mp->mdi_name, mp, last_mdi);
	if (dp->mdi == mp)
	{
		/*
		 * Taking out current MDI driver
		 */
		Log("RemoveIf: mp is at head of list");
		if ( dp->mdi_active ) {
			MUTEX_UNLOCK(&mutex);
			Error(28, "dlpid: RemoveIf: Removing active MDI driver (%s) from DLPI module (%s)\n", mp->mdi_name, dp->dlpi_name);
			return(0);				/* ERROR */
		}
		if ( !last_mdi ) {
			dp->mdi = mp->next;
		} else {
			if ( dp == dlpi_head ) {
				dlpi_head = dp->next;
			} else {
				struct dlpidev *dp1;

				for (dp1=dlpi_head; dp1 && dp1->next; dp1=dp1->next) {
					if (dp1->next == dp) {
						dp1->next = dp->next;
						break;
					}
				}
			}
			free(dp);
		}
	}
	if ( !last_mdi ) {	/* Not last one in list */
		mp->next->prev = mp->prev;
		mp->prev->next = mp->next;
	}
	MUTEX_UNLOCK(&mutex);
	free(mp);
	Log("RemoveIf ENDS");
	return(1);
}

int
StopMdiIf(struct dlpidev *dp)
{
	struct	strioctl	si;
	uint	dlpid_dereg_flag = 1;

	Log("StopMdiIf (DLPI %s, MDI %s)", dp->dlpi_name, dp->mdi->mdi_name);
	if (!dp->mdi_active)
	{
		Error(29, "dlpid: DLPI Interface (%s) not started\n", dp->dlpi_name);
		return(0);
	}

	if (ioctl(dp->dlpi_fd, I_PUNLINK, -1 /*MUXID_ALL*/) == -1)
	{
		SystemError(32, "dlpid: Error unlinking DLPI module (%s) from network adapter driver (%s)\n",
													dp->dlpi_path, dp->mdi->mdi_path);
		return(0);					/* ERROR */
	}

	Log("StopMdiIf mdi_fd %d)", dp->mdi_fd);
	if (dp->mdi_fd != -1)
	{
		close(dp->mdi_fd);
		dp->mdi_fd = -1;
	}

	dp->mdi_active = 0;
	return(1);
}

int
StopIf(struct dlpidev *dp, int junk)
{
	struct	strioctl	si;
	uint	dlpid_dereg_flag = 1;

	Log("StopIf (DLPI %s, MDI %s)", dp->dlpi_name, dp->mdi->mdi_name);
	if (!dp->mdi_active)
	{
		Error(29, "dlpid: DLPI Interface (%s) not started\n", dp->dlpi_name);
		return(0);
	}

	/* De-Register with the DLPI module */
	si.ic_cmd = DLPID_REGISTER;
	si.ic_timout = 0;
	si.ic_dp = (char *)&dlpid_dereg_flag;
	si.ic_len = sizeof(uint);

	if (ioctl(dp->dlpi_fd, I_STR, &si) == -1)
		SystemError(31, "dlpid: Error De-registering dlpid from DLPI Interface (%s)\n", dp->dlpi_name);

	if (ioctl(dp->dlpi_fd, I_PUNLINK, -1 /*MUXID_ALL*/) == -1)
	{
		SystemError(32, "dlpid: Error unlinking DLPI module (%s) from network adapter driver (%s)\n",
											dp->dlpi_path, dp->mdi->mdi_path);
		return(0);					/* ERROR */
	}

	Log("StopIf mdi_fd %d)", dp->mdi_fd);
	if (dp->mdi_fd != -1)
	{
		close(dp->mdi_fd);
		dp->mdi_fd = -1;
	}

	close(dp->dlpi_fd);
	RemoveInput(dp->dlpi_fd);
	dp->dlpi_fd = -1;

	dp->mdi_active = 0;
	return(1);
}

int
FailoverIf(struct dlpidev *dp)
{
	int was_active = dp->mdi_active;

	Log("FailoverIf (DLPI %s, MDI %s, active %d", dp->dlpi_name, dp->mdi->mdi_name, was_active);
	if ( was_active ) {
		if ( !StopMdiIf(dp) ) {
			return(0);				/* ERROR */
		}
		if (*dp->mdi->mdi_backup_name != NULL) {
			/* first HWFAIL on primary - start backup */
			Log("FailoverIf HWFAIL from restart backup (%s)\n", dp->mdi->mdi_backup_name);
			AddInterface(dp->dlpi_name, dp->mdi->mdi_backup_name);
			DumpIfStructs();
		}
	}
	Log("FailoverIf dp->mdi (%s) 0x%x, dp->mdi->next (%s) 0x%x", dp->mdi->mdi_name, dp->mdi, dp->mdi->next->mdi_name, dp->mdi->next);
	dp->mdi = dp->mdi->next;
	return ( StartMdiIf(dp) );
}

int
RestartIf(struct dlpidev *dp, int from_dlpi)
{
	int was_active = dp->mdi_active;

	Log("RestartIf (DLPI %s, MDI %s, from_dlpi %d) active %d", dp->dlpi_name, dp->mdi->mdi_name, from_dlpi, was_active);
	if ( was_active ) {
		if ( !StopIf(dp, 0) ) {
			return(0);				/* ERROR */
		}
		if (from_dlpi && *dp->mdi->mdi_backup_name != NULL) {
			/* first HWFAIL on primary - start backup */
			Log("RestartIf HWFAIL from restart backup (%s)\n", dp->mdi->mdi_backup_name);
			AddInterface(dp->dlpi_name, dp->mdi->mdi_backup_name);
			DumpIfStructs();
		}
	}
	Log("RestartIf dp->mdi (%s) 0x%x, dp->mdi->next (%s) 0x%x", dp->mdi->mdi_name, dp->mdi, dp->mdi->next->mdi_name, dp->mdi->next);
	dp->mdi = dp->mdi->next;
	return ( StartIf(dp) );
}

/*******************************************************************************
 * Functions manipulating the DLPI/MDI dev structures which take character
 * string names as their arguments.
 ******************************************************************************/

struct dlpidev *
AddInterface(char *dlpi_name, char *mdi_name)
{
	struct mdidev *mp;
	struct dlpidev *dp;
	struct strioctl si;

	/* obtain mutex so that if a signal occurs it won't traverse a
	 * potentially bogus linked list
	 */
	if (MUTEX_TRYLOCK(&mutex) == EBUSY) {
		/* shouldn't happen, as we're not called from signal handler */
		Error(47, "dlpid: AddInterface: cannot obtain mutex\n");
		return((struct dlpidev *)NULL);
	}

	Log("AddInterface STARTS(DLPI=%s,MDI=%s)",dlpi_name,mdi_name);

	if (!(mp = (struct mdidev *)malloc(sizeof(struct mdidev))))
	{
		MUTEX_UNLOCK(&mutex);
		Error(33, "dlpid: AddInterface cannot allocate memory\n");
		return((struct dlpidev *)NULL);
	}
	memset((void *)mp, 0, sizeof(struct mdidev));

	if (dp = FindDlpiDev(dlpi_name))
	{
		Log("Found DLPI(%s), Adding MDI(%x) to DLPI(%x)", dlpi_name,mp,dp);

		if (FindMdiDev(dp, mdi_name)) {
			MUTEX_UNLOCK(&mutex);
			free(mp);
			Log("dlpid: MDI (%s) already added under DLPI (%s)\n",
				mdi_name, dlpi_name);
			return((struct dlpidev *)NULL);
		}
		/* Add mp to the end of the list of mdidev's */
		mp->next = dp->mdi;
		mp->prev = dp->mdi->prev;
		dp->mdi->prev->next = mp;
		dp->mdi->prev = mp;
	}
	else
	{
		dp = (struct dlpidev *)malloc(sizeof(struct dlpidev));
		Log("New DLPI(%s), Adding MDI(%x) to DLPI(%x)", dlpi_name,mp,dp);
		if (!dp)
		{
			MUTEX_UNLOCK(&mutex);
			free(mp);
			Error(35, "dlpid: AddInterface cannot allocate memory\n");
			return((struct dlpidev *)NULL);
		}

		memset((void *)dp, 0, sizeof(struct dlpidev));
		strcpy(dp->dlpi_name, dlpi_name);
		sprintf(dp->dlpi_path, "/dev/%s", dlpi_name);
		dp->dlpi_fd = -1;
		dp->mdi_fd = -1;
		dp->mdi = mp;
		dp->mdi_active = 0;
		mp->next = mp->prev = mp;

		dp->next = dlpi_head;
		dlpi_head = dp;
	}

	MUTEX_UNLOCK(&mutex);
	strcpy(mp->mdi_name, mdi_name);
	sprintf(mp->mdi_path, "/dev/mdi/%s", mdi_name);
	Log("AddInterface Returns(0x%x)",dp);
	return(dp);
}

int
RemoveInterface(char *dlpi_name, char *mdi_name)
{
	struct dlpidev *dp;
	struct mdidev *mp;

	if (!(dp = FindDlpiDev(dlpi_name))) {
		Error(36, "dlpid: RemoveInterface: Unable to find DLPI Interface (%s) in internal table\n", dlpi_name);
		return (0);					/* ERROR */
	}
	if (!(mp = FindMdiDev(dp, mdi_name))) {
		Error(37, "dlpid: RemoveInterface: Unable to find MDI Driver (%s) under DLPI Interface (%s)\n", mdi_name, dlpi_name);
		return (0);					/* ERROR */
	}
	if (dp->mdi_active && dp->mdi == mp) {
		Error(38, "dlpid: RemoveInterface: MDI Driver (%s) under DLPI Interface (%s) currently active\n", mdi_name, dlpi_name);
		Error(39, "dlpid: RemoveInterface: Stop the interface and then remove the MDI driver (%s)\n", mdi_name);
		return (0);					/* ERROR */
	}
	return (RemoveIf(dp,mp));
}

int
StartInterface(char *dlpi_name)
{
	struct dlpidev *dp;

	if (!(dp = FindDlpiDev(dlpi_name))) {
		Error(40, "dlpid: StartInterface: Unable to find DLPI Interface (%s) in internal table\n", dlpi_name);
		return (0);					/* ERROR */
	}
	if ( dp->mdi_active ) {
		Log("dlpid: DLPI (%s) already started\n", dp->dlpi_name);
		return(0);
	}
	return (StartIf(dp));
}

int
StopInterface(char *dlpi_name)
{
	struct dlpidev *dp;

	if (!(dp = FindDlpiDev(dlpi_name))) {
		Log(42, "dlpid: StopInterface: Unable to find DLPI Interface (%s) in internal table\n", dlpi_name);
		return (0);					/* ERROR */
	}
	if ( !dp->mdi_active ) {
		Log(43, "dlpid: DLPI Interface (%s) has not been started\n", dp->dlpi_name);
		return(0);
	}
	return (StopIf(dp, 0));
}

int
FailoverInterface(char *dlpi_name)
{
	struct dlpidev *dp;

	if (!(dp = FindDlpiDev(dlpi_name))) {
		Error(46, "dlpid: FailoverInterface: Unable to find DLPI Interface (%s) in internal table\n", dlpi_name);
		return (0);					/* ERROR */
	}
	return (FailoverIf(dp));
}

int
RestartInterface(char *dlpi_name)
{
	struct dlpidev *dp;

	if (!(dp = FindDlpiDev(dlpi_name))) {
		Error(44, "dlpid: RestartInterface: Unable to find DLPI Interface (%s) in internal table\n", dlpi_name);
		return (0);					/* ERROR */
	}
	return (RestartIf(dp, 0));
}

int
AddBackupInterface(char *dlpi_name, char *mdi_name)
{
	struct dlpidev *dp;
	struct mdidev *mp;

#ifdef DEBUG
	Log("AddBackupInterface: dlpi_name <%s> mdi_name <%s>", dlpi_name, mdi_name);
#endif

	/* dp is backup netX */
	if ((dp = FindDlpiFromMdi(mdi_name))) {
		Error(34, "dlpid: MDI driver (%s) already exists under DLPI (%s)\n", mdi_name, dlpi_name);
		return (0);					/* ERROR */
	}

	/* dp is default netX */
	if (!(dp = FindDlpiDev(dlpi_name))) {
		Error(45, "AddBackupInterface: Unable to find DLPI Interface (%s) in internal table\n", dlpi_name);
		return (0);					/* ERROR */
	}
	strcpy(dp->mdi->mdi_backup_name, mdi_name);
}

static int
DoAllInterfaces( int (*func)(struct dlpidev *, int))
{
	struct dlpidev *dp;
	int rc=1;

	for (dp=dlpi_head; dp; dp=dp->next) {
		if (!func(dp, 0)) {
			rc=0;
		}
	}
	return(rc);
}

int
StopAllInterfaces()
{
	return (DoAllInterfaces(StopIf));
}

int
RestartAllInterfaces()
{
	return (DoAllInterfaces(RestartIf));
}

/*******************************************************************************
 * Functions handling the restart of interfaces which failed to open following
 * a hardware restart
 ******************************************************************************/

static int restartHandlerRunning = 0;

void
startRestartHandler()
{
	if (!restartHandlerRunning) {
		Log("Restart Handler starting, will fire in %d seconds", RESTART_INTERVAL);
		restartHandlerRunning = 1;
		sigaction(SIGALRM, &alrmhandler, NULL); 
		alarm(RESTART_INTERVAL);
	}
}

void
stopRestartHandler()
{
	if (restartHandlerRunning) {
		Log("Restart Handler stopped");
		restartHandlerRunning = 0;
	}
	sigaction(SIGALRM, &alrmnohandler, NULL); 
}

/* 
 * there were 3 big problems with restartHandler:
 * a) calling signal and not checking EINTR instead of sigaction w/ SA_RESTART
 *    solved by calling sigaction.
 * b) calling stdio routines from signal handler - solved using SVR4
 *    reentrant libc instead of global variable denoting "in signal handler".
 * c) walking a linked list which could be in the process of being changed 
 *    when signal sent.  Solved using mutex.  This is somewhat expensive today 
 *    as we mmap in all of libthread to call its few funtions, but we expect
 *    to make dlpid multithreaded soon.  A global variable could suffice in
 *    the interim.
 */
void
restartHandler(int sig)
{
	struct dlpidev *dp;
	int handler_needed = 0;

	Log("RestartHandler fired");

	/* signal may have occurred in a place where we were modifying the
	 * linked list.  Go check and see
	 */
	if (MUTEX_TRYLOCK(&mutex) == EBUSY) {
		/* nothing we can really do here except try again later on */
		Log("can't acquire lock! - re-arming restartHandler");
		sigaction(SIGALRM, &alrmhandler, NULL); 
		alarm(RESTART_INTERVAL);
		return;
	}

	/* safe to traverse list at this point since nobody in process of
	 * modifying it
	 */
	for (dp = dlpi_head; dp; dp=dp->next) {
		if (dp->dlpi_reopening) {
			if (RestartIf(dp, 0)) {
				dp->dlpi_reopening=0;
			} else {
				handler_needed = 1;
			}
		}
	}

	/* done with critical section, release mutex */
	MUTEX_UNLOCK(&mutex);

	if (handler_needed) {
		Log("Re-arming restartHandler");
		sigaction(SIGALRM, &alrmhandler, NULL); 
		alarm(RESTART_INTERVAL);
	} else {
		stopRestartHandler();
	}
}

void
ignoreHandler(int sig)
{
	/* don't do anything */
}
