#ident	"@(#)truss:i386/cmd/truss/name.c	1.1.1.3"
#ident	"$Header$"

#include <stdio.h>
#include <sys/sysi86.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "proto.h"
#include "machdep.h"

CONST char *
si86name(code)
register int code;
{
	register CONST char * str = NULL;

	switch (code) {
	case SI86FPHW:		str = "SI86FPHW";	break;
	case SETNAME:		str = "SETNAME";	break;
#ifdef SI86KSTR
	case SI86KSTR:		str = "SI86KSTR";	break;
#endif
	case SI86MEM:		str = "SI86MEM";	break;
	case SI86TODEMON:	str = "SI86TODEMON";	break;
	case SI86CCDEMON:	str = "SI86CCDEMON";	break;
/* 71 through 74 reserved for VPIX */
#ifdef SI86V86
	case SI86V86: 		str = "SI86V86";	break;
#endif
	case SI86DSCR:		str = "SI86DSCR";	break;
/* NFA entry point */
#ifdef SI86NFA
	case SI86NFA:		str = "SI86NFA";	break;
#endif
#ifdef SI86VM86
	case SI86VM86:		str = "SI86VM86";	break;
#endif
#ifdef SI86VMENABLE
	case SI86VMENABLE:	str = "SI86VMENABLE";	break;
#endif
#ifdef SI86LIMUSER
	case SI86LIMUSER:	str = "SI86LIMUSER";	break;
#endif
/* Merged Product defines */
	case SI86SHFIL:		str = "SI86SHFIL";	break;
	case SI86PCHRGN:	str = "SI86PCHRGN";	break;
	case SI86BADVISE:	str = "SI86BADVISE";	break;
	case SI86SHRGN:		str = "SI86SHRGN";	break;
	case SI86CHIDT:		str = "SI86CHIDT";	break;
	case SI86EMULRDA: 	str = "SI86EMULRDA";	break;
	}

	return str;
}
