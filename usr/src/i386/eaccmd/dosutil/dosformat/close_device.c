/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/close_device.c	1.1.1.2"
#ident  "$Header$"

/* #define		DEBUG		1	/* */

/*
		close_device(handle)

	Use this to close the device when finished.

	Return 0 on success, -1 on failure.
*/

#include	"MS-DOS.h"

#include	<stdio.h>

close_device(handle)
int	handle;
{
	int	index;

#ifdef DEBUG
	(void) fprintf(stderr, "close_device(): DEBUG - Closing down Handle: %d\n", handle);
#endif

	if ((index = lookup_device(handle)) == -1) {
#ifdef DEBUG
		(void) fprintf(stderr, "close_device(): Error - Handle %d not found in device table\n", handle);
#endif
		return(-1);
	}

	if (close(handle) == -1) {
#ifdef DEBUG
		(void) fprintf(stderr, "close_device(): Failed to close device\n");
		perror("	Reason");
#endif
		return(-1);
	}

	TABLE.handle = 0;

	(void) free(TABLE.our_fat);

#ifdef DEBUG
	(void) fprintf(stderr, "close_device(): DEBUG - Handle: %d closed\n", handle);
#endif

	return(0);
}
