#ident "@(#)ntp_types.h	1.2"

/*
 *  ntp_types.h - defines how int32 and u_int32 are treated.  For 64 bit systems
 *  like the DEC Alpha, they has to be defined as int and u_int.  for 32 bit
 *  systems, define them as long and u_long
 */
#include "ntp_machine.h"

#ifndef _NTP_TYPES_
#define _NTP_TYPES_

/*
 * This is another naming conflict.
 * On NetBSD for MAC the macro "mac" is defined as 1
 * this is fun for a as a paket structure contains an
 * optional "mac" member - severe confusion results 8-)
 * As we hopefully do not have to rely on that macro we
 * just undefine that.
 */
#ifdef mac
#undef mac
#endif

/*
 * Set up for prototyping
 */
#ifndef P
#if defined(__STDC__) || defined(USE_PROTOTYPES)
#define	P(x)	x
#else /* __STDC__ USE_PROTOTYPES */
#define P(x)	()
#if	!defined(const)
#define	const
#endif /* const */
#endif /* __STDC__ USE_PROTOTYPES */
#endif /* P */

/*
 * VMS DECC (v4.1), {u_char,u_short,u_long} are only in SOCKET.H,
 *			and u_int isn't defined anywhere
 */
#if defined(VMS)
#include <socket.h>
typedef unsigned int u_int;
/*
 * Note: VMS DECC has  long == int  (even on __alpha),
 *	 so the distinction below doesn't matter
 */
#endif /* VMS */

/*
 * DEC Alpha systems need int32 and u_int32 defined as int and u_int
 */
#ifdef __alpha
#ifndef int32
#define int32 int
#endif /* int32 */
#ifndef u_int32
#define u_int32 u_int
#endif /* u_int32 */
/*
 *  All other systems fall into this part
 */
#else /* __alpha */
#ifndef int32
#define int32 long
#endif /* int32 */
#ifndef u_int32
#define u_int32 u_long
#endif /* u_int32 */
#endif /* __ alplha */
    
#endif /* _NTP_TYPES_ */

