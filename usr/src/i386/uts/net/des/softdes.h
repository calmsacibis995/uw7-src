#ifndef _NET_DES_SOFTDES_H	/* wrapper symbol for kernel use */
#define _NET_DES_SOFTDES_H	/* subject to change without notice */

#ident	"@(#)softdes.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	softdes.h, data types and definition for software DES
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif

/*
 * A chunk is an area of storage used in three different ways
 * - As a 64 bit quantity (in high endian notation)
 * - As a 48 bit quantity (6 low order bits per byte)
 * - As a 32 bit quantity (first 4 bytes)
 */
typedef union {
	struct {
		/*
		 * the order of the following 2 u_longs is
		 * very machine specific.
		 */
		u_long	_long1;
		u_long	_long0;
	} _longs;
#define	long0	_longs._long0
#define	long1	_longs._long1
	struct {
		/*
		 * the order of the following 8 u_chars is
		 * very machine specific.
		 */
		u_char	_byte7;
		u_char	_byte6;
		u_char	_byte5;
		u_char	_byte4;
		u_char	_byte3;
		u_char	_byte2;
		u_char	_byte1;
		u_char	_byte0;
	} _bytes;
#define	byte0	_bytes._byte0
#define	byte1	_bytes._byte1
#define	byte2	_bytes._byte2
#define	byte3	_bytes._byte3
#define	byte4	_bytes._byte4
#define	byte5	_bytes._byte5
#define	byte6	_bytes._byte6
#define	byte7	_bytes._byte7
} chunk_t;

/*
 * Intermediate key storage
 * Created by des_setkey, used by des_encrypt and des_decrypt
 * 16 48 bit values
 */
struct deskeydata {
	chunk_t	keyval[16];
};

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_DES_SOFTDES_H */
