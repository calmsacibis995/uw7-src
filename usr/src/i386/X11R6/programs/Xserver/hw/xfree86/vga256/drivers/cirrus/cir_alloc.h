/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_alloc.h,v 3.4 1995/01/28 16:11:30 dawes Exp $ */

/*
 * Definitions for video memory allocator in cir_alloc.c.
 */

/* $XConsortium: cir_alloc.h /main/5 1995/11/13 08:20:35 kaleb $ */

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

#if NeedFunctionPrototypes

int CirrusInitializeAllocator(int base);
int CirrusAllocate(int size);
void CirrusFree(int vidaddr);
void CirrusUploadPattern(unsigned char *pattern, int w, int h, int vidaddr,
	int srcpitch);
int CirrusCursorAllocate ( cirrusCurRecPtr cirrusCur );

#else

int CirrusInitializeAllocator();
int CirrusAllocate();
int CirrusCursorAllocate ();
void CirrusFree();
void CirrusUploadPattern();

#endif

_XFUNCPROTOEND
