#ifndef _TIMER_H
#define _TIMER_H
#ident	"@(#)debugger:gui.d/common/Timer.h	1.2"

#include "UI.h"
#include "TimerP.h"

// A Timer lets the user set an alarm with a callback to be
// called when the timer expires.
// Assumptions:
//	1) multiple concurrent timers are set as in:
//		t1 = new Timer(cs1);
//		t1->set(ms1, cb1);
//		...
//		t2 = new Timer(cs2);
//		t2->set(ms2, cb2);
//		...
//	   and NOT:
//		t = new Timer(cs);
//		t->set(ms1, cb1);
//		t->set(ms2, cb2);
//		...
//	2) timer callback does not call unset(), unset() is only called
//	   to turn off an impending alarm.

class Command_sender;

// Framework callbacks

class Timer
{
	TIMER_TOOLKIT_SPECIFICS

private:
	Callback_ptr	callback;
	Command_sender	*creator;

public:
			Timer(Command_sender *cs);
			~Timer() { unset(); }
	Command_sender	*get_creator()	{ return creator; }
	Callback_ptr	get_callback()	{ return callback; }
	void		set(int, Callback_ptr);	// set alarm in milliseconds
	void		unset();		// turn off alarm before it sounds
	void		clear()		{ callback = 0; clear_timer_id(); }
};
#endif	// _TIMER_H
