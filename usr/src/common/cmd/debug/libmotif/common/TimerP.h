#ifndef _TIMERP_H
#define _TIMERP_H
#ident	"@(#)debugger:libmotif/common/TimerP.h	1.1"

// toolkit specific members of Timer class,
// included by ../../gui.d/common/Timer.h

#define	TIMER_TOOLKIT_SPECIFICS 		\
private:					\
	XtIntervalId	timer_id; 		\
public:				  		\
	void		clear_timer_id() { timer_id = (XtIntervalId)0; }	\
	XtIntervalId	get_timer_id() { return timer_id; }

#endif	// _TIMERP_H
