/*		copyright	"%c%"	*/
#ident	"@(#)status.c	1.2"
/*
 * (c) Copyright 1991 Hewlett-Packard Company.  All Rights Reserved.
 *
 * This source is for demonstration purposes only.
 *
 * Since this source has not been debugged for all situations, it will not be
 * supported by Hewlett-Packard in any manner.
 *
 * This material is provided "as is".  Hewlett-Packard makes no warranty of
 * any kind with regard to this material.  Hewlett-Packard shall not be liable
 * for errors contained herein for incidental or consequential damages in 
 * connection with the furnishing, performance, or use of this material.
 * Hewlett-Packard assumes no responsibility for the use or reliability of 
 * this software.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define HPNPADMIN    "/usr/bin/hpnpadmin"

#ifdef sun
char *Status = NULL;
int  SavedStatus = 0;
char OldStatus[BUFSIZ];
#endif

/*
 * NAME
 *	StatusFile - save the name of the status file
 *
 * SYNOPSIS
 *	void StatusFile(s)
 *      char *s;
 *
 * DESCRIPTION
 *      Check that the file can be read and written to.  If so, save
 *      the name of the status file.
 *
 * RETURNS
 *	"StatusFile" returns nothing.
 */
void
StatusFile(s)
char *s;
{
#ifdef sun
	if (access(s, R_OK|W_OK) == -1)
		return;
	Status = s;
#endif
}


/*
 * NAME
 *	NewStatus - optionally send message to logging file
 *
 * SYNOPSIS
 *      void NewStatus(status, saveold)
 *      char *status;
 *      int saveold;
 *
 * DESCRIPTION
 *      Update the status file.  Possibly save the status that was
 *      already there.
 *
 * RETURNS
 *	"NewStatus" returns nothing.
 */
void
NewStatus(status, saveold)
char *status;
int saveold;
{
#ifdef sun
	int fd, n;
	char buff[BUFSIZ], *s;

	LogMessage(1, "NewStatus: entered");

	if(Status == NULL)
	    return;

	if((fd = open(Status, O_RDWR|O_CREAT, 0664)) < 0)
	    return;

	if(flock(fd, LOCK_EX) < 0){
	    (void) close(fd);
	    return;
	}

	if(saveold && !SavedStatus){
	    if((n = read(fd, OldStatus, BUFSIZ)) >= 0){
		SavedStatus = 1;
		OldStatus[n] = '\0';
		LogMessage(1, "NewStatus: old status saved");
	    }
	    if(lseek(fd, 0, SEEK_SET) < 0){
		(void) flock(fd, LOCK_UN);
		(void) close(fd);
		return;
	    }
	}

	if(ftruncate(fd, 0) < 0){
	    (void) flock(fd, LOCK_UN);
	    (void) close(fd);
	    return;
	}

	(void) write(fd, status, strlen(status));
	(void) flock(fd, LOCK_UN);
	(void) close(fd);

	/*
	 * Replace newlines with blanks for log file.
	 */
	(void) strcpy(buff, status);
	s = strchr(buff, '\n');
	if(s != NULL)
	    *s = ' ';
	s = strchr(buff, '\n');
	if(s != NULL)
	    *s = ' ';
	LogMessage(1, "NewStatus: new status: %s", buff);

#endif /* sun */
}

/*
 * NAME
 *	RestoreStatus - restore the saved status
 *
 * SYNOPSIS
 *	void RestoreStatus()
 *
 * DESCRIPTION
 *      Restore the previously saved status.
 *
 * RETURNS
 *	"RestoreStatus" returns nothing.
 */
void
RestoreStatus()
{
#ifdef sun
	LogMessage(1, "RestoreStatus: entered");
	if(!SavedStatus)
		return;
	SavedStatus = 0;
	NewStatus(OldStatus, 0);
#endif /* sun */
}

/*
 * NAME
 *	ConnStatus - create a status message before trying to connect
 *
 * SYNOPSIS
 *      void ConnStatus(netperiph, saveold, tryhpnpadmin)
 *      char *netperiph;
 *      int saveold;
 *      int tryhpnpadmin;
 *
 * DESCRIPTION
 *      Create a status message before trying to connect.  Use hpnpadmin
 *      to get the status message.  If hpnpadmin fails for some reason,
 *      create a message that is simply "connecting to <peripheral>".
 *
 * RETURNS
 *	"ConnStatus" returns nothing.
 */
void
ConnStatus(netperiph, saveold, tryhpnpadmin)
char *netperiph;
int saveold;
int tryhpnpadmin;
{
#ifdef sun
	char buff[BUFSIZ], *s, *t;
	char cmd[160];
	FILE *hpnpadmin;
	int n, cc;
	static int failure = 0;

	if(Status == NULL)
	    return;

	LogMessage(1, "ConnStatus: entered, host %s saveold %d tryhpnpadmin %d",
					netperiph, saveold, tryhpnpadmin);

	sprintf(buff, "connecting to %s\n", netperiph);

	sprintf(cmd, "%s -s %s 2>/dev/null", HPNPADMIN, netperiph);
	if((failure == 0) && (tryhpnpadmin != 0)){
	    if((hpnpadmin = popen(cmd, "r")) == NULL){
		failure = 1;
		LogMessage(1, "ConnStatus: popen failed on: %s", cmd);
	    } else {
		t = s = &buff[strlen(buff)];
		strcat(buff, netperiph);
		s = strchr(s, '.');
		if(s != NULL)
		    *s = '\0';
		strcat(buff, ": ");
		cc = strlen(buff);
		while((n = fread(&buff[cc], 1, sizeof(buff) - cc -1, hpnpadmin))
									> 0)
		    cc += n;

		buff[cc] = '\0';
		failure = pclose(hpnpadmin);

		/*
		 * If the host is does not exist or the host is not
		 * up, or the community name does not match, then
		 * hpnpadmin exits with a non-zero status.   Back out
		 * the peripheral name from the status string.
		 */
		if(failure != 0){
		    *t = '\0';
		    LogMessage(1, 
			"ConnStatus: hpnpadmin: non-zero return status");
		}
	    }
	}

	NewStatus(buff, saveold);
#endif /* sun */
}

