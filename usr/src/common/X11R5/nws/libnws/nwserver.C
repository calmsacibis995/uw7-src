#ident	"@(#)nwserver.C	1.2"
////////////////////////////////////////////////////////////////////////////////
// 	NWServer = NetWare Server. NetWare Server related methods. e.g check status
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/shm.h>

#include <Xm/Xm.h>

/* libnws files */
#include <nwserver.h>

/* NetWare related files */
extern "C" {
#include <nwconfig.h>
#include <nwtypes.h>
#include "nwparam.h"
#include <shm_proto.h>
#include "server_proto.h"
};

/* Has to be globally declared. NWU implementation dependency. */
SHM_HEADER		*ShmHeaderPtr;		/* Shared Memory Pointer */
#define			NWSHUT					"/sbin/tfadmin nwshut -b -g"
#define			NWSERVER				"/sbin/tfadmin nwserver"

/******************************************************************************
 * Constructor for the NWServer class.  
 *****************************************************************************/
NWServer::NWServer()
{
#ifdef DEBUG
cout << "ctor for NWServer" << endl;
#endif
	/* Initialize local variables
	 */
	_StatusFlag = False;
	_ReturnCode = _AttachFlag = 0;
	_serverName = _serveraction = NULL; 

	/* Attach to shared memory segment here */
	AttachShm();
}

/******************************************************************************
 * Destructor for the NWServer class.
 *****************************************************************************/
NWServer::~NWServer()
{
#ifdef DEBUG
cout << "dtor for NWServer" << endl;
#endif
	CleanupStorage();
}

/******************************************************************************
 * Attach to shared meemory
 *****************************************************************************/
void
NWServer::AttachShm()
{
	int			shmKey = 0, shmAccess = 0;

	/* Get the NWCM parameters for shm access and shm key.
	 */
	NWCMGetParam("shm_access", NWCP_INTEGER, (void *) &shmAccess);
	NWCMGetParam("shm_key", NWCP_INTEGER, (void *) &shmKey);

	/* See if I could attach or not and set the flag appropriately
	 * NWServerStatus api needs the attach flag to be passed in.
	 */
	_AttachFlag = AttachToSharedMemory (shmKey, shmAccess, (char **) 
                                     (&ShmHeaderPtr)) != SUCCESS ? False : True;
	
	/* If the attach failed for some reason, remove the /var/netware/nwshut.pid
	 * file.  I am not testing the return value for unlink becoz if the file
 	 * does not exist, then it does not matter. If it exists it removes it.
	 */
	if (_AttachFlag == False) {
		ShmHeaderPtr = NULL;
		unlink (PIDFILE);
	}
}

/******************************************************************************
 * Detach from shared memory that was previously attached to.
 *****************************************************************************/
void
NWServer::DetachShm()
{
	shmdt (ShmHeaderPtr);
}

/******************************************************************************
 * Cleanup the storage space and free strings.
 *****************************************************************************/
void
NWServer::CleanupStorage()
{
	/* Cleanup space and delete strings
	 */
	if (_serverName)
		delete [] _serverName;
	if (_serveraction)
		delete [] _serveraction;
}

/******************************************************************************
 * Check the NetWare server status. Return success/failure. 
 * Attach to shared memory, after getting the shared memory parameters. 
 * Check the return code and return string depending on the status code.
 *****************************************************************************/
void
NWServer::CheckNWServerStatus()
{
	/* If space had been allocated earlier, for servername and errormessage
	 * free it here, first.
	 */
	CleanupStorage();
	_serverName = new char [50];

	/* Status flag is for our code.  Need it to see if server is up or
	 * down. Get the string matching the return code and return it.
	 * Return status if server is running and shared memory is attached as
 	 * follows:
 	 *	 1 Server is DOWN,
 	 *	 2 Server is Coming UP,
 	 *   3 Server is UP BUT needs DSINSTALL,
 	 *   4 Server needs DSREPAIR,
 	 *   5 Server is UP and running.
 	 *   6 Core dumped
	 *   7 Server is going down
 	 */
	_ReturnCode = NWServerStatus(_serverName, NULL, _AttachFlag);
#ifdef DEBUG
	cout << "The return code is " << _ReturnCode << endl;
#endif
	_StatusFlag = (_ReturnCode == 1) ? False : True;

	/* Setup the serveraction to be initiated based on the return code from the 
	 * netware server status. If it is down then nwserver else nwshut.
	 */ 
	_serveraction = new char[_ReturnCode == ISDOWN ? (strlen (NWSERVER) +1) :
						(strlen (NWSHUT) + 1)]; 
	strcpy (_serveraction, _ReturnCode == ISDOWN ? NWSERVER : NWSHUT);
}

/******************************************************************************
 * Abort the /usr/sbin/nwshut process by sending a kill signal to it.
 *****************************************************************************/
int
NWServer::KillShutdownProcess()
{
	FILE		*fp;
	int			retcode = ABORTPASSED;
	char		msg[64];
	
	/* Open the file where the pid for nwshut is stored
	 */
	if ((fp = fopen(PIDFILE, "r")) == NULL) 
		retcode = OPENFAILED;		
	else {
		/* Get the pid string from the file
		 */
		if ((fgets (msg, sizeof(msg), fp)) == NULL) 		
			retcode = READFAILED;		
		else {
			/* Convert it to a long field and send signal to the pid.
		 	 * If the kill fails then send error else send success info.
			 */
			pid_t pid = atoi (msg);
			if (kill (pid, SIGINT) != 0)
				retcode = ABORTFAILED;		
			else 
				/* If it succeeded remove the file, just in case it did not
				 * get removed by nwshut itself. Abort succeeded.
				 */
				unlink (PIDFILE);
		} /* Send the signal */
	} /* Read the file */
	return retcode;
}

/******************************************************************************
 * Check to see if the server is coming down, by accessing the file where the
 * pid of nwshut process is stored. If the file exists, then the server is
 * coming down else it is not. Return the appropriate boolean value.
 *****************************************************************************/
Boolean
NWServer::IsServerComingDown()
{
	return (_ReturnCode == ISGOINGDOWN ? True : False);
/*
	struct 	stat 	buf;

	assert (PIDFILE != 0);
	if (stat (PIDFILE, &buf) == 0) {
		if (buf.st_size > 0)
			return True;
	}

	return False;
*/
}

/******************************************************************************
 * Detach first, then attach to shared memory.
 *****************************************************************************/
void
NWServer::DetachAttachShm()
{
	/* If it  is already attached detach it here */
	if (IsAttachedToShm()) {
		DetachShm();
		ShmHeaderPtr = NULL;
#ifdef DEBUG
		cout << "detached" << endl;
#endif
		_AttachFlag = False;
	}

	/* Attach to shared memory */
	AttachShm();
}
