#ifndef _DYN_INFO_H
#define _DYN_INFO_H

#ident	"@(#)debugger:inc/i386/Dyn_info.h	1.1"

#include "Iaddr.h"

// machine specific, per-object dynamic information;
// maintained by the Seglist and Symnode classes.

struct Dyn_info {
	Iaddr	pltaddr;	// address of procedure linkage table
	long	pltsize;	// size of procedure linkage table
	Iaddr	gotaddr;	// address of global offset table
};

#endif
