/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 *
 */

#ident	"@(#)fmli:inc/terror.h	1.4.3.4"

extern char	nil[];

#define warn(what, name)	_terror(0, what, name, __FILE__, __LINE__, FALSE)
#define error(what, name)	_terror(TERR_LOG, what, name, __FILE__, __LINE__, FALSE)
#define child_error(what, name)	_terror(TERR_LOG, what, name, __FILE__, __LINE__, TRUE)
#define fatal(what, name)	_terror(TERR_LOG | TERR_EXIT, what, name, __FILE__, __LINE__, FALSE)
#define child_fatal(what, name)	_terror(TERR_LOG | TERR_EXIT, what, name, __FILE__, __LINE__, TRUE)

#define TERR_CONT	0
#define TERR_LOG	1
#define TERR_EXIT	2

#define TERRLOG		"/tmp/TERRLOG"

/*
 * These values are indices into the What array in terrmess.c
 * If you want to add a new error, the procedure is as follows:
 *  add the message for it to the end of the What array.
 *  add a define for it to this group of defines.
 *  add one to the value of TS_NERRS in this file.
 */
#define NONE		0
#define NOFORK		0
#define NOMEM		0
#define NOPEN		1
#define BADARGS		2
#define MUNGED		3
#define MISSING		4
#define SWERR		5
#define NOEXEC		6
#define LINK		7
#define VALID		8
#define NOT_UPDATED     9
#define FRAME_NOPEN    10
#define VALIDONDONE    11	/* abs s13 */
#define FRAME_NOPERM   12
#define FRAME_NOENT    13
#define UPDAT_NOPERM   14
#define UPDAT_NOENT    15

#define TS_NERRS       16
