#ifndef Interface_h
#define Interface_h

#ident	"@(#)debugger:inc/common/Interface.h	1.15"

// Debugger interface structure.

// Enables different interfaces (e.g., screen and line mode)
// to work together.

#include <stdio.h>

#include "Iaddr.h"
#include "Msgtypes.h"
#include "Severity.h"
#include "print.h"

class	ProcObj;

enum ui_type {
	ui_no_type = 0,
	ui_cli,
	ui_gui,
};

extern	ui_type	get_ui_type();

extern	void	pushoutfile(FILE *);
extern	void	popout();
extern	void	set_interface(const char *interface, char **options,
			const char *gpath);
extern	void	stop_interface();

extern	int	PrintaxGenNL;
extern	int	PrintaxSpeakCount;
extern	int	processing_query;

// Debugging flags and debugging interface - debugging code disappears
// unless compiled with -DDEBUG=1
// example - DPRINT(DBG_PARSER, ("token = %s\n", t->print()))

#if DEBUG

extern int debugflag;
extern void dprintm(char * ...);

#define DBG_PROC	0x1
#define	DBG_FRAME	0x2
#define	DBG_PARSER	0x4
#define DBG_SEG		0x8
#define DBG_EXPR	0x10
#define DBG_FOLLOW	0x20
#define DBG_THREAD	0x40
#define DBG_CTL		0x80
#define DBG_STR		0x100
#define DBG_EVENT	0x200
#define DBG_DWARF	0x400
#define	DPRINT(D,M)	if (debugflag & D) { dprintm M; }
#else
#define	DPRINT(D,M)
#endif

#endif	/* Interface_h */
