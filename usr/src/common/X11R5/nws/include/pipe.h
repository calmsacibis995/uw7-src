#ident	"@(#)pipe.h	1.2"
/*****************************************************************************
 * Pipe.h: Generic pipe object that opens a pipe to instantiate a command and
 * 			waits for completion without blocking the X event loop.
 *****************************************************************************/
#ifndef  PIPE_H 
#define  PIPE_H

#include <Xm/Xm.h>

#define SHELL_COMMAND		"/bin/sh"
#define SHELL_NAME			"sh"
#define SHELL_PARAM			"-c"

class Pipe  {
public:						/* CTOR & DTOR for Pipe */
   	Pipe ();
   	~Pipe ();

private:					/* Private data */

		int					_pipefd1[2], _pipefd2[2];
		int					_ExitValue, _statusRetCode;
		char				*_retbuf;
		pid_t				_childpid;
		Boolean				_PipeErrorFlag;
		XtAppContext		_appContext;
		XtIntervalId		_timer_id;

							/* user method called when command completes */
		void				(*_MethodPtr)(void *clientdata);
							/* Pass calling object this pointer as data */
		void				*_clientdata;	

		XtInputId			_nofunc_id, _error_id, _read_id;

private:					/* Private methods */
		static void			GetFileInput (XtPointer clientData, int *FileId,
                                          XtInputId *id);
		void				HandleInput ();

		static void			PipeError (XtPointer clientData, int *FileId,
                                       XtInputId *id);
		void				ClosePipe ();
		static void			CheckProcessStatus (XtPointer, XtIntervalId);

public:						/* Public methods */
		Boolean				OpenPipe (char *, XtAppContext, void (*funcptr)
                                      (void *),void *clientdata);
		Boolean				GetPipeErrorFlag () const { return _PipeErrorFlag;}
		char				*GetStringOutput() const { return _retbuf; }
		int					GetExitValue () const { return _ExitValue; }
		Boolean				KillProcess(pid_t);
		XtIntervalId		GetTimerId() const { return _timer_id; }
};

#endif /* PIPE_H */
