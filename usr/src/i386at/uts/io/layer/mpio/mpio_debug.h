#ifndef _IO_LAYER_MPIO_MPIO_DEBUG_H    /* wrapper symbol for kernel use */
#define _IO_LAYER_MPIO_MPIO_DEBUG_H    /* subject to change without notice */

#ident	"@(#)kern-pdi:io/layer/mpio/mpio_debug.h	1.1.3.1"

/*
 * This module sets up some macro functions which can be used while
 * debugging the program, and then left in the code, but turned off by
 * just not defining "MPIO_DEBUG".  This way your production version of
 * the program will not be filled with bunches of debugging junks.
 *
 * Compile the subsystem with -DMPIO_DEBUG=n where n is    minimum debug
 * level you want to include. If n is not specified, level=100 is the
 * default. There are six types of debug macros that take one string
 * and 0 to 5 parameters.
 *
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef MPIO_DEBUG
#if MPIO_DEBUG == 1        /* if default level	*/
#undef MPIO_DEBUG
#define MPIO_DEBUG    100	/* use level 100	*/
#endif  /* MPIO_DEBUG == 1 */

#define MPIO_DEBUG0(val,str) \
	if( MPIO_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__); \
	}

#define MPIO_DEBUG1(val,str,a1) \
	if( MPIO_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1); \
	}

#define MPIO_DEBUG2(val,str,a1,a2) \
	if( MPIO_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2); \
	}

#define MPIO_DEBUG3(val,str,a1,a2,a3) \
	if( MPIO_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2,a3); \
	}

#define MPIO_DEBUG4(val,str,a1,a2,a3,a4) \
	if( MPIO_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2,a3,a4); \
	}

#define MPIO_DEBUG5(val,str,a1,a2,a3,a4,a5) \
	if( MPIO_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2,a3,a4,a5); \
	}

#else  /* if MPIO_DEBUG */

#define MPIO_DEBUG0(val,s)
#define MPIO_DEBUG1(val,s,a1)
#define MPIO_DEBUG2(val,s,a1,a2)
#define MPIO_DEBUG3(val,s,a1,a2,a3)
#define MPIO_DEBUG4(val,s,a1,a2,a3,a4)
#define MPIO_DEBUG5(val,s,a1,a2,a3,a4,a5)

#endif /* ifdef DEBUG */

#if defined(__cplusplus)
    }
#endif

#endif /* _IO_LAYER_MPIO_MPIO_DEBUG_H */
