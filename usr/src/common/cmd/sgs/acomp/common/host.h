#ident "@(#)acomp:common/host.h	55.2"
/* host.h */

/* Declarations for the host system. */

/* These are good for a host with 32-bit ints. */

typedef	unsigned char I7;	/* has at least 7 bits */
typedef int I16;		/* has at least 16 bits */
typedef unsigned short U16;	/* has at least 16 bits, unsigned */
typedef int I32;		/* has at least 32 bits */
typedef unsigned long U32;	/* has at least 32 bits, unsigned */
typedef unsigned int SIZE;	/* type (presumed unsigned) that can
				** hold the size (number of elements),
				** and, consequently the index number
				** for any host array
				*/
/* typedef long BITOFF;		/* holds bit offsets for the target machine */

/* Assume the host supports IEEE format floating point and
** ieee.h trapping if not a VAX or Amdahl.
*/
#if !defined(IEEE_HOST) && !(defined(vax) || defined(uts))
#define	IEEE_HOST
#endif
