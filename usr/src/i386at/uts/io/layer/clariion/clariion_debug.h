#ident	"@(#)kern-pdi:io/layer/clariion/clariion_debug.h	1.1.1.1"

/*  Copyright (c) 1996 Data General Corp. All Rights Reserved. */


#ifndef _IO_LAYER_CLARIION_CLARIION_DEBUG_H /* wrapper symbol for kernel use */
#define _IO_LAYER_CLARIION_CLARIION_DEBUG_H /*subject to change without notice */

#ident  "$Header$"

/*
 * This module sets up some macro functions which can be used while
 * debugging the program, and then left in the code, but turned off by
 * just not defining "CLARIION_DEBUG".  This way your production version of
 * the program will not be filled with bunches of debugging junks.
 *
 * Compile the subsystem with -DCLARIION_DEBUG=n where n is	minimum debug
 * level you want to include. If n is not specified, level=100 is the
 * default. There are six types of debug macros that take one string
 * and 0 to 5 parameters.
 *
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef CLARIION_DEBUG
#if CLARIION_DEBUG == 1        /* if default level	*/
#undef CLARIION_DEBUG
#define CLARIION_DEBUG    100	/* use level 100	*/
#endif  /* CLARIION_DEBUG == 1 */

#define CLARIION_DEBUG0(val,str) \
	if( CLARIION_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__); \
	}

#define CLARIION_DEBUG1(val,str,a1) \
	if( CLARIION_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1); \
	}

#define CLARIION_DEBUG2(val,str,a1,a2) \
	if( CLARIION_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2); \
	}

#define CLARIION_DEBUG3(val,str,a1,a2,a3) \
	if( CLARIION_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2,a3); \
	}

#define CLARIION_DEBUG4(val,str,a1,a2,a3,a4) \
	if( CLARIION_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2,a3,a4); \
	}

#define CLARIION_DEBUG5(val,str,a1,a2,a3,a4,a5) \
	if( CLARIION_DEBUG <= val ) { \
		cmn_err(CE_CONT, "%s(%d): " str "\n", \
		__FILE__,__LINE__,a1,a2,a3,a4,a5); \
	}

#else  /* if CLARIION_DEBUG */

#define CLARIION_DEBUG0(val,s)
#define CLARIION_DEBUG1(val,s,a1)
#define CLARIION_DEBUG2(val,s,a1,a2)
#define CLARIION_DEBUG3(val,s,a1,a2,a3)
#define CLARIION_DEBUG4(val,s,a1,a2,a3,a4)
#define CLARIION_DEBUG5(val,s,a1,a2,a3,a4,a5)

#endif /* ifdef CLARIION_DEBUG */

#if defined(__cplusplus)
    }
#endif

#endif /* _IO_LAYER_CLARIION_CLARIION_DEBUG_H */
