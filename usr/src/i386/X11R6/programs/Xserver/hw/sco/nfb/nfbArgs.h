#ifndef _NFB_ARGS_H
#define _NFB_ARGS_H

#include "ddxArgs.h"

typedef struct _nfbCursorHacksRec {
	Bool		staticFG;
	unsigned short	fgRed,fgGreen,fgBlue;
	Bool		staticBG;
	unsigned short	bgRed,bgGreen,bgBlue;
} nfbCursorHacksRec,*nfbCursorHacksPtr;

extern	nfbCursorHacksRec	nfbCursHacks;


extern	ddxArgDesc	nfbArguments[];

#endif /* _NFB_ARGS_H */
