/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 *
 */

#ident	"@(#)fmli:inc/message.h	1.4.4.3"

extern	int Mess_lock;
#define mess_lock()	(Mess_lock++)
#define mess_unlock()	(Mess_lock = 0)
#define MESSIZ	(256)  /* that should be wider than any screen */
