/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:inc/sizes.h	1.2.4.3"

#ifndef CURSES_H
    extern int COLS;
#endif
#define RESERVED_LINES  (3)	/* banner + message + command lines     */
#define FIXED_TITLE	(6)	/* max overhead for frame no. + border  */
#define FIXED_COLS	(4)	/* overhead for frame border + margin   */
#define MAX_TITLE	(COLS - FIXED_TITLE) /* longest frame title     */
#define MESS_COLS	(COLS - 1) /* longest message line              */
#define FILE_NAME_SIZ	(256)	/* length +1 of longest file name       */
#define PATHSIZ		(1024)	/* length +1 of longest UNIX path name  */
#define MAX_WIDTH	(256)	/* the widest screen possibly supported *
				 * used for allocating string buffers   *
				 * that are then limited by the real    *
				 * screen width or other constraints.   */
#define TRUNCATE_STR	("...")	/* str to indicate desc. was truncated  */
#define LEN_TRUNC_STR	(3)	/* length of above string, TRUNCATE_STR */
