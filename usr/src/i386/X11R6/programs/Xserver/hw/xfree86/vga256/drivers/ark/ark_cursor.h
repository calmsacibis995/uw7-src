/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/ark/ark_cursor.h,v 3.0 1995/03/11 14:17:00 dawes Exp $ */





/* $XConsortium: ark_cursor.h /main/3 1995/11/13 07:23:38 kaleb $ */

/* Variables defined in ark_cursor.c. */

extern int arkCursorHotX;
extern int arkCursorHotY;
extern int arkCursorWidth;
extern int arkCursorHeight;

/* Functions defined in ark_cursor.c. */

extern void ArkCursorInit();
extern void ArkRestoreCursor();
extern void ArkWarpCursor();
extern void ArkQueryBestSize();
