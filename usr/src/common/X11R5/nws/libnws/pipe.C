#ident	"@(#)pipe.C	1.2"
/*******************************************************************************
 * Pipe:: 	Generic object containing pipe related functions to be invoked
 * 			for executing commands directly from an application, and wait for
 * 			input without blocking the X event loop.
 ******************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <signal.h>

#include "pipe.h"
#include "process.h"

/*****************************************************************************
 *  FUNCTION:
 *	ctor
 *  DESCRIPTION:
 *  The Pipe ctor. Initialize all the variables. 
 *  RETURN:
 *  nothing
 *****************************************************************************/
Pipe::Pipe ()
{
#ifdef DEBUG
cout << "ctor for Pipe" << endl;
#endif
	_error_id = _read_id = 0;
	_PipeErrorFlag = False;
	_retbuf = 0;
	_ExitValue  = 0;
	_timer_id = 0;
	_statusRetCode = 0;
}

/*****************************************************************************
 *  FUNCTION:
 *	dtor
 *  DESCRIPTION:
 *  Dtor for Pipe. Close the pipe and remove ids if any.
 *  RETURN:
 *  nothing
 *****************************************************************************/
Pipe::~Pipe()
{
#ifdef DEBUG
cout << "dtor for Pipe" << endl;
#endif
	if (_retbuf) {
		delete [] _retbuf;
		_retbuf = 0;
	}
	ClosePipe();
}

/*******************************************************************************
 *  FUNCTION:
 *	void OpenPipe (char *, XtAppContext, void (*)(void *), void *)
 *  DESCRIPTION:
 *  Load the pipe command and open input for accessing return info from 
 *  executed command. Pass in a function ptr and a clientdata to be invoked 
 *  upon completion of the command.
 *  RETURN:
 *	A pointer to just the string portion of the message file entry.
 ******************************************************************************/
Boolean 
Pipe::OpenPipe (char *command, XtAppContext appContext, void (*funcptr)(void *),
				void *clientdata)
{
	Boolean			retflag = True;

	/* Initialize the user method to be called upon completion of command
	 * and the clientdata to be passed to it (usually the class pointer)
	 */
	_MethodPtr = funcptr;
	_clientdata = clientdata;
	_appContext = appContext;

	/* Open 2 pipes for input and output channels using the pipe system call
	 */
	if (pipe (_pipefd1) == -1)
		return False;
	if (pipe (_pipefd2) == -1)
		return False;

	/* Fork the child process. In the child redirect stdin, stdout and
	 * stderr to other ends of the pipes setup by the parent. The 
	 * communication between parent and child is established here.
	 */
	if ((_childpid = fork ()) == 0) {	// child process
		close (_pipefd1[0]);
		close (1);
		dup (_pipefd1[1]);
		close (2);		// 2 new lines
		dup (_pipefd1[1]);
		close (_pipefd2[1]);
		close (0);
		dup (_pipefd2[0]);

		/* Exec the command by passing it to the shell command
		 */
		execl (SHELL_COMMAND, SHELL_NAME, SHELL_PARAM, command, (char *) 0);
		_PipeErrorFlag = True;
		_ExitValue = 1;
		retflag = False;		// Exec failed.
	}
	else {				// parent process
		/* register a function to get the stdout of the pipe. Also register
		 * a function for PipeError if any. 
      	 */
		_error_id = XtAppAddInput (appContext, _pipefd1[0], (XtPointer)
                                   XtInputExceptMask, Pipe::PipeError, 
                                   (XtPointer) this);


		_read_id = XtAppAddInput (appContext, _pipefd1[0], (XtPointer)
                                  XtInputReadMask, Pipe::GetFileInput, 
                                  (XtPointer) this);

		_timer_id = XtAppAddTimeOut (appContext, 0, (XtTimerCallbackProc)
                                     CheckProcessStatus, this);
	}
	return retflag;
}

/********************************************************************* 
 *  FUNCTION:
 *	void CheckProcessStatus(XtPointer , int *, XtInputId *)
 *  DESCRIPTION:
 *  Check the process status by waiting without a block on the parent.
 *  Keeps getting called periodically till status is returned. 
 *  RETURN:
 *  nothing.
 *********************************************************************/
void
Pipe::CheckProcessStatus(XtPointer clientdata, XtIntervalId id)
{
	Pipe	*obj = (Pipe *) clientdata;
	int 	status;

	/* Wait in the parent for the child to examine the status.  This
	 * wait with the WNOHANG is a non-blocking wait, which means that 
	 * parent process is not suspended while the child is in progress.
	 */
	obj->_statusRetCode = waitpid (-1, &status, WNOHANG);

	/* Shift bits to return the status of the exit function  from the 
	 * command executed. The return status of exit is stored here.
	 */
	obj->_ExitValue = status >> 8;

	/* If the status was not available keep waiting else remove timer id
	 */
	if (obj->_statusRetCode == 0)
		obj->_timer_id = XtAppAddTimeOut (obj->_appContext, 0,
                                          (XtTimerCallbackProc)  
										 &Pipe::CheckProcessStatus, obj);
	else { /* Returns pid of the child here */
		XtRemoveTimeOut (obj->_timer_id);
		obj->_timer_id = 0;
	}
}

/********************************************************************* 
 *  FUNCTION:
 *	void GetFileInput(XtPointer , int *, XtInputId *)
 *  DESCRIPTION:
 *  Use the clientdata (which should be the this pointer) to invoke member.
 *  RETURN:
 *  nothing.
 *********************************************************************/
void 
Pipe::GetFileInput (XtPointer client_data, int *fid, XtInputId * id)
{
	Pipe				*obj = (Pipe *) client_data;	

	obj->HandleInput();
}  /* end of GetFileInput() */

/********************************************************************* 
 *  FUNCTION:
 *	void HandleInput()
 *  DESCRIPTION:
 *  Test to see if command has completed. Check number of bytes returned.
 *  If it has, invoke the passed in friend function of calling object and
 *  pass the clientdata (which should be the this pointer).
 *  RETURN:
 *  nothing.
 *********************************************************************/
void
Pipe::HandleInput ()
{
	int 		status,  nbytes;

#ifdef DEBUG
	cout << "getting input " << endl;
#endif DEBUG
	_retbuf = new char [BUFSIZ];

	/* Read the bytes returned from the XtAppAddInput call
	 */
	nbytes = read(_pipefd1[0], _retbuf, BUFSIZ);
#ifdef DEBUG
	cout << "retbuf " << _retbuf << endl;
#endif DEBUG

	ClosePipe ();

	/* If the I/O had failed , close the pipe and set the error flag
	 */
	if (nbytes == -1)  
		_PipeErrorFlag = True;

	/* If the status has not been completely returned, because the read 
	 * handler pre-empted the timer routine the status should be checked
	 * here till it is done, so that the calling function can access the
	 * correct value. Remove the timer if it is still there.
	 */
	if (_statusRetCode == 0) {
		while ((_statusRetCode = waitpid (-1, &status, WNOHANG)) == 0);
		_ExitValue = status >> 8;
		if (_timer_id) {
			XtRemoveTimeOut (_timer_id);
			_timer_id = 0;
		}
	}

	/* Nothing else to be read, Command completed. Call user method here.
   	 */
	_MethodPtr(_clientdata);

}  /* end of HandleInput() */

/********************************************************************* 
 *  FUNCTION:
 *	void PipeError(XtPointer , int *, XtInputId *)
 *  DESCRIPTION:
 * 	Invoked when there is an error in the pipe command. 
 *  RETURN:
 *  nothing.
 *********************************************************************/
void
Pipe::PipeError (XtPointer client_data, int  *fid, XtInputId * id)
{
	Pipe		*obj = (Pipe *) client_data;

	/* Close the pipes 
	 */
	obj->ClosePipe ();

	/* Set up the error flag to be true
	 */
	obj->_PipeErrorFlag = True;

	/* Call the member method that is passed in to perform error or success
	 * routine
	 */
	obj->_MethodPtr(obj->_clientdata);

}   /* end of PipeError() */

/*********************************************************************
 *  FUNCTION:
 *	void ClosePipe()
 *  DESCRIPTION:
 *  Close the pipe and remove  id's if any.
 *  RETURN:
 *  nothing.
 *********************************************************************/
void
Pipe::ClosePipe ()
{
    /*  End of file. Remove the read and error ids.
	 */
	if (_read_id) {
    	XtRemoveInput(_read_id);
		_read_id = 0;
	}
	if (_error_id) {
    	XtRemoveInput(_error_id);
		_error_id = 0;
	}

	/* Close the input/output pipes between both processes.
	 */
	if (_pipefd1[0])
		close (_pipefd1[1]);
	if (_pipefd2[0])
		close (_pipefd2[0]);
}

/*********************************************************************
 *  FUNCTION:
 *	Boolean KillProcess()
 *  DESCRIPTION:
 *  Kill the child that was forked off and return.
 *  RETURN:
 *  True or False depending on condition.
 *********************************************************************/
Boolean
Pipe::KillProcess(pid_t mypid)
{
	Process 		proc;
	Boolean			retflag = False;
	int 			i = 1;
	psinfo_t 		*ps;
	

	/* Look up the proc table and retrieve all process information. Set it up.
	 */
	if ((proc.SetupProcData()) != False) {

		/* Loop thru the proc table and search for any process whose parent
		 * pid matches this process id. If found, then this is our child.
		 * Send a kill signal to it. Recursively go thru all our children
	 	 * and kill them all. Any one of them left behind can get inherited
		 * by init (1) if the parent is killed.
	 	 */
		while ((ps = proc.GetProcStructInfo(i++)) != NULL) {

			if (ps->pr_ppid ==  mypid) {
#ifdef DEBUG
				cout << "parent pid " << ps->pr_ppid << endl; 
				cout << "mypid is = " << mypid << endl;
#endif
				if (!KillProcess(ps->pr_pid)) { 
					retflag = False;
				}
#ifdef DEBUG
				cout << "child pid " << ps->pr_pid << endl; 
#endif
				if ((proc.SignalProcess(ps->pr_pid, SIGKILL)) == False) {
					retflag =  False;
				}
				else {
#ifdef DEBUG
					cout << "killed the pid " << ps->pr_pid << endl; 
#endif
					retflag = True;
				}
			}
		}
	}

	return retflag;
}
