/*
 *	@(#)getidval.c	7.1	10/22/97	12:28:58
 */

#include <stdio.h>
#include <sys/types.h>

#define STATIC

#define ID_CVT	('a' - 1)


u_long PnP_idVal(const char *idStr);
STATIC u_long PnP_xVal(u_char c);
STATIC u_long PnP_aVal(u_char c);

main(int argc, char **argv)
{
if(argc != 2)
	{
	fprintf(stderr, "usage: %s id\n", argv[0]);
	exit(1);
	}

fprintf(stdout, "%ld\n", PnP_idVal(argv[1]));
exit(0);
}



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

STATIC u_long
PnP_xVal(u_char c)
{
    if ((c >= '0') && (c <= '9'))
	return (u_long)c - (u_long)'0';

    c |= 0x20U;		/* c = tolower(c) */

    if ((c >= 'a') && (c <= 'f'))
	return (u_long)c - (u_long)('a' - 10);

    return 0;
}

STATIC u_long
PnP_aVal(u_char c)
{
    c |= 0x20U;

    if ((c >= 'a') && (c <= 'z'))
	return (u_long)c - (u_long)ID_CVT;

    return 0;
}
