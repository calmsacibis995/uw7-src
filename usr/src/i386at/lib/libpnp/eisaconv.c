/*
 *	@(#)eisaconv.c	7.1	10/22/97	12:21:20
 * File eisaconv.c
 * Routines for conversion of vendor ID.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include "libpnp.h"

static u_long PnP_xVal(u_char c);
static u_long PnP_aVal(u_char c);


const char *
PnP_idStr(u_long vendor)
{
    static char		idBuf[] = "pnp1234";
    static const char	xdigit[] = "0123456789abcdef";
    const u_char	*id;
#   define ID_CVT	('a' - 1)	/* Must be lower case for getbsword() */


    id = (u_char *)&vendor;
    idBuf[0] = ID_CVT | ((u_int)(id[0] & 0x7cU) >> 2);
    idBuf[1] = ID_CVT | ((u_int)(id[0] & 0x03U) << 3) |
			((u_int)(id[1] & 0xe0U) >> 5);
    idBuf[2] = ID_CVT |  (id[1] & 0x1f);

    idBuf[3] = xdigit[(id[2] >> 4) & 0x0f];
    idBuf[4] = xdigit[id[2] & 0x0f];

    idBuf[5] = xdigit[(id[3] >> 4) & 0x0f];
    idBuf[6] = xdigit[id[3] & 0x0f];

    return idBuf;
}

/*
 * "ctl109d"
 *
 * 0x9d108c0e
 * 9999 dddd 1111 0000 tttl llll 0ccc cctt
 *
 * 0x0e 0x8c 0x10 0x9d
 * 0ccc cctt   tttl llll   1111 0000   9999 dddd
 */

u_long
PnP_idVal(const char *idStr)
{
    u_long	value;

    if (!idStr)
	return ~0UL;

    return  (PnP_xVal(idStr[6]) << 24) |
	    (PnP_xVal(idStr[5]) << 28) |
	    (PnP_xVal(idStr[4]) << 16) |
	    (PnP_xVal(idStr[3]) << 20) |

	    (PnP_aVal(idStr[2]) << 8) |
	    (PnP_aVal(idStr[1]) >> 3) |
	    ((PnP_aVal(idStr[1]) & 0x07U) << 13) |
	    (PnP_aVal(idStr[0]) << 2);
}


static u_long
PnP_xVal(u_char c)
{
#ifdef NO_CTYPE
    if ((c >= '0') && (c <= '9'))
	return (u_long)c - (u_long)'0';

    c |= 0x20U;		/* c = tolower(c) */
#else
    if (isdigit(c))
	return (u_long)c - (u_long)'0';

    c = tolower(c);
#endif

    if ((c >= 'a') && (c <= 'f'))
	return (u_long)c - (u_long)('a' - 10);

    return 0;
}

static u_long
PnP_aVal(u_char c)
{
#ifdef NO_CTYPE
    c |= 0x20U;

    if ((c >= 'a') && (c <= 'z'))
	return (u_long)c - (u_long)ID_CVT;
#else
    if (isalpha(c))
	return (u_long)tolower(c) - (u_long)ID_CVT;
#endif

    return 0;
}

