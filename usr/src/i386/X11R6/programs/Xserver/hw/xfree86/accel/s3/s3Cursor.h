/* $XFree86: $ */





/* $XConsortium: s3Cursor.h /main/2 1995/11/12 19:05:50 kaleb $ */

extern Bool s3BlockCursor;
extern Bool s3ReloadCursor;

#define BLOCK_CURSOR	s3BlockCursor = TRUE;

#define UNBLOCK_CURSOR	{ \
			   if (s3ReloadCursor) \
			      s3RestoreCursor(s3savepScreen); \
			   s3BlockCursor = FALSE; \
			}
