#ident	"@(#)dis:common/data32.h	1.1"

/* macros to get unaligned data in proper byte order
 * code assumes 2-byte shorts and 4-byte longs
 * and 2's complement arithmetic
 *
 * P should be an unsigned char * that points to the
 * source data
 */

#define MGET_LONG(P)	\
		((long)(((unsigned long)(P)[0] << 24) | \
		((unsigned long)(P)[1] << 16) | \
		((unsigned long)(P)[2] << 8) | \
		(unsigned long)(P)[3]))


#define LGET_LONG(P)	\
		((long)(((unsigned long)(P)[3] << 24) | \
		((unsigned long)(P)[2] << 16) | \
		((unsigned long)(P)[1] << 8) | \
		(unsigned long)(P)[0]))

#define MGET_SHORT(P)	\
		((short)(((unsigned short)(P)[0] << 8) | \
		(unsigned short)(P)[1]))

#define LGET_SHORT(P)	\
		((short)(((unsigned short)(P)[1] << 8) | \
		(unsigned short)(P)[0]))

