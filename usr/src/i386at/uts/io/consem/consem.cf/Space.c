#ident	"@(#)Space.c	1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/vt.h>
#include <sys/termio.h>
#include <sys/consem.h>
#include "config.h"		/* to collect #defines and tunable parameters */

int csem_cnt = CSEM_UNITS;
struct csem csem[CSEM_UNITS];

struct csem_esc_t csem_tab[]={
	{ 'a', "KDDISPTYPE",        KDDISPTYPE,        CSEM_O,    0,  136 },
	{ 'b', "KDGKBENT",          KDGKBENT,          CSEM_B,    4,    4 },
	{ 'c', "KDSKBENT",          KDSKBENT,          CSEM_I,    4,    0 },
	{ 'd', "KDGKBMODE",         KDGKBMODE,         CSEM_O,    0,    4 },
	{ 'e', "KDSKBMODE",         KDSKBMODE,         CSEM_I,    4,    0 },
	{ 'f', "GIO_ATTR",          GIO_ATTR,          CSEM_R,    0,    4 },
	{ 'g', "GIO_COLOR",         GIO_COLOR,         CSEM_R,    0,    4 },
	{ 'h', "GIO_KEYMAP",        GIO_KEYMAP,        CSEM_O,    0, 2572 },
	{ 'i', "PIO_KEYMAP",        PIO_KEYMAP,        CSEM_I, 2572,    0 },
	{ 'j', "GIO_STRMAP",        GIO_STRMAP,        CSEM_O,    0,  512 },
	{ 'k', "PIO_STRMAP",        PIO_STRMAP,        CSEM_I,  512,    0 },
	{ 'l', "GETFKEY",           GETFKEY,           CSEM_B,   34,   34 },
	{ 'm', "SETFKEY",           SETFKEY,           CSEM_I,   34,    0 },
	{ 'n', "SW_B40x25",         SW_B40x25,         CSEM_N,    0,    0 },
	{ 'n', "O_SW_B40x25",       O_SW_B40x25,       CSEM_N,    0,    0 },
	{ 'o', "SW_C40x25",         SW_C40x25,         CSEM_N,    0,    0 },
	{ 'o', "O_SW_C40x25",       O_SW_C40x25,       CSEM_N,    0,    0 },
	{ 'p', "SW_B80x25",         SW_B80x25,         CSEM_N,    0,    0 },
	{ 'p', "O_SW_B80x25",       O_SW_B80x25,       CSEM_N,    0,    0 },
	{ 'q', "SW_C80x25",         SW_C80x25,         CSEM_N,    0,    0 },
	{ 'q', "O_SW_C80x25",       O_SW_C80x25,       CSEM_N,    0,    0 },
	{ 'r', "SW_EGAMONO80x25",   SW_EGAMONO80x25,   CSEM_N,    0,    0 },
	{ 'r', "O_SW_EGAMONO80x25", O_SW_EGAMONO80x25, CSEM_N,    0,    0 },
	{ 's', "SW_ENHB40x25",      SW_ENHB40x25,      CSEM_N,    0,    0 },
	{ 's', "O_SW_ENHB40x25",    O_SW_ENHB40x25,    CSEM_N,    0,    0 },
	{ 't', "SW_ENHC40x25",      SW_ENHC40x25,      CSEM_N,    0,    0 },
	{ 't', "O_SW_ENHC40x25",    O_SW_ENHC40x25,    CSEM_N,    0,    0 },
	{ 'u', "SW_ENHB80x25",      SW_ENHB80x25,      CSEM_N,    0,    0 },
	{ 'u', "O_SW_ENHB80x25",    O_SW_ENHB80x25,    CSEM_N,    0,    0 },
	{ 'v', "SW_ENHC80x25",      SW_ENHC80x25,      CSEM_N,    0,    0 },
	{ 'v', "O_SW_ENHC80x25",    O_SW_ENHC80x25,    CSEM_N,    0,    0 },
	{ 'w', "SW_ENHB80x43",      SW_ENHB80x43,      CSEM_N,    0,    0 },
	{ 'w', "O_SW_ENHB80x43",    O_SW_ENHB80x43,    CSEM_N,    0,    0 },
	{ 'x', "SW_ENHC80x43",      SW_ENHC80x43,      CSEM_N,    0,    0 },
	{ 'x', "O_SW_ENHC80x43",    O_SW_ENHC80x43,    CSEM_N,    0,    0 },
	{ 'y', "SW_MCAMODE",        SW_MCAMODE,        CSEM_N,    0,    0 },
	{ 'y', "O_SW_MCAMODE",      O_SW_MCAMODE,      CSEM_N,    0,    0 },
	{ 'z', "CONS_CURRENT",      CONS_CURRENT,      CSEM_R,    0,    4 },
	{ 'A', "CONS_GET",          CONS_GET,          CSEM_R,    0,    4 },
	{ 'B', "TIOCVTNAME",        TIOCVTNAME,        CSEM_O,    0, VTNAMESZ},
	{ 'C', "GIO_SCRNMAP",       GIO_SCRNMAP,       CSEM_O,    0,  256 },
	{ 'D', "PIO_SCRNMAP",       PIO_SCRNMAP,       CSEM_I,  256,    0 }
};

int csem_tab_siz= sizeof(csem_tab)/sizeof(struct csem_esc_t);
