#ident	"@(#)kern-i386at:io/tp/tpath.cf/Space.c	1.2"
#ident	"$Header$"

#include <config.h>
#include <sys/types.h>
#include <sys/termios.h>

int tp_listallocsz = 32;	/* List expansion chunk size*/
major_t tp_imaj = TP_CMAJOR_0;	/* TP's internal major number*/

/*
* Major and minor device number of default system console device.
*/
major_t tp_consoledevmaj = CMUX_CMAJOR_0;
minor_t tp_consoledevmin = 0;


/*
* Time interval (expressed in milliseconds) to delay in switching SAK
* definition from saktypeNONE to saktypeDATA when the TP data channel
* is disconnected from the TP device.
* A value of zero, 0, indicates no delay.
*/
clock_t tp_saktypeDATA_switchdelay = 4000;

/********************************************************************************
* The following variables are representations of termios structures.
*
*		-tpXXXXMaskTermios
*
* -Values set in the fields correspond to specific termios flags definitions.
* -TP uses these masking variables to prevent certain termio flags from being
*  changed.  If the specified set of termio flags were allowed to be changed,
*  SAK recognition and processing could be circumvented.
* -Each mask is an array of two termios structures.
*	-the first element is the mask used on the data channel.
*	-the second element is the mask used on the ctrl channel.
*  The difference between the ctrl and data mask is that the ctrl mask may be
*  less restrictive (always a subset of the data mask) than the data mask.
*  This is to allow certain ioctls (i.e. setting BAUD rate) to be set by the
*  ctrl channel that could NOT put the tp device in a state such that SAK
*  recognition could be defeated.
*  NOTE: the reason a ctrl mask exists is that setting certain termios ioctls
*  even by the ctrl channel could put the tp device in a state such that SAK
*  recognition could be defeated.
* -NOTE: c_cc mask values are ignored since their bit postions are not flag
*  values.  The c_cc mask values are just 0x00 filled.  The driver does
*  comparison checks on them directly.
*
*		-tpXXXXValidTermios
*
* -For every bit position set in the ctrl mask of the tpXXXXMaskTermios
*  variable, the value of what that flag should be (so that it can not defeat
*  SAK recognition), is indicated in tpXXXXValidTermios.
*/

struct termios tp_charmasktermios[2] = {	/* if sak is CHARACTER type */
					/* Char SAK termios mask */

	/* -data channel mask- */
{	(0000100 + 0000200),	/* _iflag: INLCR + IGNCR */
	(0000000),		/* _oflag */
	(0000017 + 0000060),	/* _cflag: CBAUD + CSIZE */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00 },

	/* -ctrl channel mask- */
{	(0000100+0000200),	/* _iflag: INLCR + IGNCR */
	(0000000),		/* _oflag */
	(0000000),		/* _cflag */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00 },
};

struct termios tp_charvalidtermios = {	/* Char SAK valid termios */
	(0000100 + 0000200),	/* _iflag: INLCR + IGNCR */
	(0000000),		/* _oflag */
	(0000000),		/* _cflag */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00
};

struct termios tp_dropmasktermios[2] = { /* if sak is SPECIAL type, LINEDROP */
					/* Linedrop SAK termios mask */
	/* -data channel mask- */
{	(0000000),		/* _iflag */
	(0000000),		/* _oflag */
	(0004000),		/* _cflag: CLOCAL */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00 },

	/* -ctrl channel mask- */
{	(0000000),		/* _iflag */
	(0000000),		/* _oflag */
	(0004000),		/* _cflag: CLOCAL */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00 },
};

struct termios tp_dropvalidtermios = {	/* Linedrop SAK valid termios */
	(0000000),		/* _iflag */
	(0000000),		/* _oflag */
	(0004000),		/* _cflag: CLOCAL */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00
};

struct termios tp_breakmasktermios[2] = { /* if sak is SPECIAL type, BREAK */
					 /* Break SAK termios mask */
	/* -data channel mask- */
{	(0000001 + 0000002),	/* _iflag: IGNBRK + BRKINT */
	(0000000),		/* _oflag */
	(0000000),		/* _cflag */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00 },

	/* -data channel mask- */
{	(0000001 + 0000002),	/* _iflag: IGNBRK + BRKINT */
	(0000000),		/* _oflag */
	(0000000),		/* _cflag */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00 },
};

struct termios tp_breakvalidtermios = {	/* Break SAK valid termios */
	(0000000 + 0000000),	/* _iflag: IGNBRK + BRKINT */
	(0000000),		/* _oflag */
	(0000000),		/* _cflag */
	(0000000),		/* _lflag */
	0x00,0x00,0x00,0x00,	/* c_cc */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00
};
