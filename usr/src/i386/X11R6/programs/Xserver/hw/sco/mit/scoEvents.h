/*
 *	@(#) scoEvents.h 11.1 97/10/22 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Feb 04 21:36:34 PST 1991	kylec@sco.com
 *		- create
 *
 */

# ifndef _SCOEVENTS_H_
# define _SCOEVENTS_H_

extern void 	ProcessInputEvents();
extern Bool	XqueEnable();
extern Bool 	XqueDisable();
extern Bool	XqueReset();

# endif /* _SCOEVENTS_H_ */
