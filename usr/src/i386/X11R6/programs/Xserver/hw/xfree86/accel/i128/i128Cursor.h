/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/i128/i128Cursor.h,v 3.0 1995/12/07 07:24:03 dawes Exp $ */






/* $XConsortium: i128Cursor.h /main/1 1995/12/09 15:31:32 kaleb $ */

extern Bool i128BlockCursor;
extern Bool i128ReloadCursor;

#define BLOCK_CURSOR	i128BlockCursor = TRUE;

#define UNBLOCK_CURSOR	{ \
			   if (i128ReloadCursor) \
			      i128RestoreCursor(i128savepScreen); \
			   i128BlockCursor = FALSE; \
			}
