#ident	"@(#)table.h	1.1"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "sys/ascii.h"
#include "sys/termios.h"
#include "sys/stream.h"
#include "sys/strtty.h"
#include "sys/stropts.h"
#include "sys/proc.h"
#include "sys/xque.h"

/*
 * This table is used to translate keyboard scan codes to ASCII character
 * sequences for the AT386 keyboard/display driver.  It is the default table,
 * and may be changed with system calls.
 */
keyinfo_t	nop_key = {
	{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP },0xff, L_O 
};

struct {
	int	escape_code;
	int	sco_ext_key;
	int	usl_ext_key;
} usl_sco_extkeys[] = {
	0x0,	0x8c,	  -1,	  /* Extended Num '/' */
	0x1a,	0x54,	  -1,	  /* [ { or Extended Sys Request/PrtScrn */
	0x1c,	0x8d,	  0x74,	  /* enter key */
	0x1d,	0x80,	  0x73,	  /* right control key */
	0x2a,	-1,	  0x00,	  /* map to no key stroke */
	0x35,	0x8c,	  0x75,	  /* keypad '/'	key */
	0x36,	-1,	  0x00,	  /* map to no key stroke */
	0x37,	-1,	  0x54,	  /* print screen key */
	0x38,	0x81,	  0x72,	  /* right alt key */
	0x46,	-1,	  0x77,	  /* pause/break key */
	0x47,	0x84,	  0x7f,	  /* home key */
	0x48,	0x8a,	  0x78,	  /* up	arrow key */
	0x49,	0x86,	  0x6f,	  /* page up key */
	0x4b,	0x89,	  0x6b,	  /* left arrow	key */
	0x4d,	0x88,	  0x7d,	  /* right arrow key */
	0x4f,	0x85,	  0x7a,	  /* end key */
	0x50,	0x8b,	  0x55,	  /* down arrow	key */
	0x51,	0x87,	  0x7e,	  /* page down key */
	0x52,	0x82,	  0x7b,	  /* insert key	*/
	0x53,	0x83,	  0x79	  /* delete key	*/
	-1,	-1,	-1
};

/* the following keys are K_NOP for a sco map */
short sco_nop_ind[] = 	{ 85,86,107,110,111,114,115,
			  116,117,119,120,121,122,123,125,126,127,-1
			};
/* the following keys are K_NOP for a usl map */
short usl_nop_ind[] = 	{ 96,97,98,99,100,101,102,103,104,105,106,-1 };
