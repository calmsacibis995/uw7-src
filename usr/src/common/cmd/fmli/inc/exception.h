/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:inc/exception.h	1.3.5.1"

/*  include in .c file not in header file  abs 9/13/88
#include	<termio.h>
#define        _SYS_TERMIO_H
*/
extern struct termio	Echo;
extern struct termio	Noecho;
extern int	Echoit;

#define fmli_echo()	(Echoit = TRUE)
#define fmli_noecho()	(Echoit = FALSE)
#define restore_tty()	(Echo.c_cflag ? ioctl(0, TCSETAW, &Echo) : -1)

#define LCKPREFIX	".L"
