/*		copyright	"%c%" 	*/

#ident	"@(#)fmli:inc/inc.types.h	1.1.3.3"

#ifndef PRE_CI5_COMPILE		/* then assume EFT types are defined
				 * in system header files.
				 */
#include <sys/types.h>
#else
#include "eft.types.h"		/* EFT defines for pre SVR4.0 systems */
#endif
