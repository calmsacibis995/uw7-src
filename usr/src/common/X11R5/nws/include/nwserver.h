#ident	"@(#)nwserver.h	1.2"
/*****************************************************************************
 *			NWServer class - header file definition.
 *****************************************************************************/
#ifndef NWSERVER_H
#define NWSERVER_H

enum { ISDOWN = 1, ISCOMINGUP, DSNEEDSINSTALL, DSNEEDSREPAIR, ISUP, COREDUMPED, 
	   ISGOINGDOWN};

enum { ABORTPASSED, OPENFAILED, READFAILED, ABORTFAILED};

#define PIDFILE "/var/netware/nwshut.pid"
class NWServer {

public:							/* Constructors/Destructors */
	NWServer (); 
	~NWServer ();

private:						/* Private Data */
	int			_ReturnCode, _AttachFlag;
	Boolean		_StatusFlag;
	char		*_serverName;
	char		*_serveraction;

protected:

private:						/* Private Methods */
	void		CleanupStorage();

public:							/* Public Interface Methods */
	void		CheckNWServerStatus();
	int			KillShutdownProcess (); 
	void		AttachShm();
	void		DetachShm();
	void		DetachAttachShm();

	Boolean		IsServerUp () const { return _StatusFlag; }
	Boolean		IsAttachedToShm () const { return _AttachFlag; }
	Boolean		IsServerComingDown();

	int			GetReturnCode() const { return _ReturnCode; }
	char		*GetServerName() const { return _serverName; }
	char		*GetServerAction() const { return _serveraction; }
};

#endif	/* NWSERVER_H */
