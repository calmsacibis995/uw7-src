#ident "@(#)mexit.c	1.2"

#ifdef SYS_WINNT
#include <stdio.h>
#include <windows.h>

HANDLE hServDoneEvent = NULL;

void
service_exit(status)
int status;
{
	extern int debug;

	if (debug) /* did not become a service, simply exit */
		ExitThread((DWORD)status);
	else {
		/* service mode, need to have the service_main routine
	 	 * register with the service control manager that the 
		 * service has stopped running, before exiting
		 */
		 if ((status > 0) && (hServDoneEvent != NULL))
			SetEvent(hServDoneEvent);
		 ExitThread((DWORD)status);
	}
}
#endif /* SYS_WINNT */
