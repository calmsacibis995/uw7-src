#ident	"@(#)debugger:libmotif/common/Timer.C	1.1"

// Timer service:
// 	only a single alarm per Timer instance.

#include "UI.h"
#include "Timer.h"

static void
timeoutCB(Timer *t, XtIntervalId *id)
{
	if (t->get_timer_id() != *id)
		return;
	Command_sender	*creator = t->get_creator();
	Callback_ptr	func = t->get_callback();
	if (func)
		(creator->*func)(0,0);
	t->clear();
}

Timer::Timer(Command_sender *cs)
{
	creator = cs;
	clear();
}

void
Timer::set(int msecs, Callback_ptr cb)
{
	if (callback)
	{
		// an alarm already pending
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	callback = cb;
	timer_id = XtAppAddTimeOut(base_context, msecs, 
		(XtTimerCallbackProc)timeoutCB, this);
}

void
Timer::unset()
{
	if (!callback)
		return;
	// a timer has been set and has not expired
	XtRemoveTimeOut(timer_id);
	clear();
}
