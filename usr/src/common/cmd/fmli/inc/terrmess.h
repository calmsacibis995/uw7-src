/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:inc/terrmess.h	1.2.3.4"

/*
 * NOTE: these error messages depend upon the order of error numbers in
 * errno.  When that changes, so must this array and the list of defines
 * in terror.h
 */
static char	*Errlist[] = {
	nil,
	"Permissions are wrong",
	"File does not exist",
	nil,
	nil,
	"Hardware error",
	nil,
	"Arguments are too long",
	"File has been corrupted",
	"Software error",
	nil,
	"Can't create another process",
	"Out of memory",
	"Permissions are wrong",
	nil,
	nil,
	nil,
	"File already exists",
	nil,
	nil,
	"Improper name",
	"It is a directory",
	nil,
	"Too many files in use on system",
	"Too many files in use by program",
	nil,
	nil,
	nil,
	"System out of disk space",
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
};

static char	*Errlistid[] = {
	nil,
	":17",
	":18",
	nil,
	nil,
	":19",
	nil,
	":20",
	":21",
	":22",
	nil,
	":23",
	":24",
	":17",
	nil,
	nil,
	nil,
	":25",
	nil,
	nil,
	":26",
	":27",
	nil,
	":28",
	":29",
	nil,
	nil,
	nil,
	":30",
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
	nil,
};
/*
 * NOTE: this array depends on the numbering scheme in terror.h
 * If you add an element to this array, add it at the end and change
 * terror.h to define the new value. Also, don't forget to change
 * TS_NERRS and add a line to Use_errno.
 */
static char	*What[TS_NERRS] = {
	nil,
	"Can't open file",
	"Invalid arguments",
	"Data has been corrupted",
	"Some necessary information is missing",
	"Software failure error",
	"Can't execute the program",
	"Can't create or remove file",
	"Input is not valid",
	"Frame not updated: definition file missing or not readable",
	"Can't open frame: definition file missing or not readable",
	"Relationship of values in 2 or more fields is not valid", /* abs s13 */
	"Can't open frame: Permission denied",/* ES: only in maintanance mode */
	"Can't open frame: Frame definition file missing",
	"Frame not updated: Permission denied",
	"Frame not updated: Frame definition file missing"
};

static char	*Whatid[TS_NERRS] = {
        nil,
        ":31",
        ":32",
        ":33",
        ":34",
        ":35",
        ":36",
        ":37",
        ":38",
        ":39",
        ":40",
        ":41",
        ":360",
        ":362",
        ":363",
        ":364"
};
/*
 * This array indicates whether or not errno may be considered
 * valid when this type of error occurs
 */
static bool	Use_errno[TS_NERRS] = {
	FALSE,
	TRUE,
	FALSE,
	FALSE,
	FALSE,
	TRUE,
	TRUE,
	TRUE,
	FALSE,
	FALSE,
	FALSE,			/* abs s16 */
	FALSE,
	FALSE,
	FALSE,
	FALSE,
	FALSE
};
