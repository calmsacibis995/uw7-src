#ident	"@(#)process.h	1.2"

/*----------------------------------------------------------------------------
 *					Process class - Object that creates/controls processes
 *----------------------------------------------------------------------------*/
#ifndef PROCESS_H
#define PROCESS_H

#include <sys/procfs.h>
#include <Xm/Xm.h>

class PList;
class ProcList;
class Device;
class Application;

enum 	{ PID, USER, NONE };

class Process {

public:							// Constructors/Destructors
								Process ();
								~Process ();

private:						// Private Data
	int							_ndev;
	Device						*_devl;

protected:
	ProcList					*_processlist;

private:						// Private Methods
	Boolean							GetDeviceData();

public:							// Public Process Methods
	Boolean						SetupProcData ();
	void						SetCommandName(char *);

	Boolean						SignalProcess(int, int);
	Boolean						SignalProcess(pid_t, int);

	psinfo_t					*GetProcStructInfo(int);
	psinfo_t					*GetProcStructInfo(pid_t pid);

	lwpsinfo_t					**GetLWPStructInfo(int);
	lwpsinfo_t					**GetLWPStructInfo(pid_t pid);

	char						*GetUserName(uid_t);
	char						*GetTTY(psinfo_t *);
	ProcList					*GetProcessList() const { return _processlist;}
};

#endif	// PROCESS_H

