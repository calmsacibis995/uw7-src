#ident	"@(#)defs.h	1.4"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


/* defs.h
 *
 * Compiler switches and miscellaneous definitions.
 */

#if	!defined(__STDC__) && !defined(const)
#define	const
#endif	/* !defined(__STDC__) && !defined(const) */

#if	!defined(__STDC__) && !defined(volatile)
#define	volatile
#endif	/* !defined(__STDC__) && !defined(volatile) */

#if	!(defined(__GNUC__) && defined(__STDC__))
#define	inline
#endif	/* __GNUC__ */

/* Common types */

typedef	U_INT8	u_int8;
typedef	S_INT8	s_int8;
typedef	U_INT16	u_int16;
typedef	S_INT16	s_int16;
typedef	U_INT32	u_int32;
typedef	S_INT32	s_int32;
#ifdef	U_INT64
typedef	U_INT64	u_int64;
#endif	/* u_int64 */
#ifdef	S_INT64
typedef	S_INT64	s_int64;
#endif	/* S_INT64 */

#ifndef	_PSAP_
/* Define byte if ISODE has not already */
typedef	u_char byte;
#endif	/* _PSAP_ */
typedef u_int16 as_t;		/* 16 bits unsigned */
typedef s_int32 pref_t;		/* 32 bits signed */
typedef u_int32 flag_t;		/* 32 bits unsigned */
typedef u_int16 proto_t;	/* 16 bits unsigned */
typedef u_int16 mtu_t;		/* 16 bits unsigned */
typedef u_int32 metric_t;	/* 32 bits unsigned */
typedef	u_int32 tag_t;		/* 32 bits unsigned */
#ifdef	notdef
typedef int asmatch_t;		/* Temporary for now */
#endif

typedef	struct _if_link if_link;
typedef struct _if_info if_info;
typedef	struct _if_addr if_addr;
typedef struct _if_addr_entry if_addr_entry;
typedef struct _rt_head rt_head;
typedef struct _rt_entry rt_entry;
typedef struct _rt_aggr_head rt_aggr_head;
typedef struct _rt_list rt_list;
typedef struct _rtq_entry rtq_entry;
typedef struct _task task;
typedef struct _task_timer task_timer;
typedef struct _task_job task_job;
typedef struct _adv_entry adv_entry;
typedef struct task_block *block_t;
typedef struct _trace trace;
typedef struct _trace_file trace_file;

/* Gated uses it's own version of *printf */
#define	fprintf	gd_fprintf
#define	sprintf	gd_sprintf
#define	vsprintf	gd_vsprintf

extern const char *gated_version;
extern const char *build_date;

/* general definitions for GATED user process */

#ifndef	TRUE
#define TRUE	 1
#define FALSE	 0
#endif	/* TRUE */

#ifndef NULL
#define NULL	 0
#endif

#define MAXHOSTNAMELENGTH 64		/*used in init_egpngh & rt_dumb_init*/

#undef  MAXPACKETSIZE

#ifndef	MIN
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#endif	/* MIN */
#ifndef	MAX
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#endif	/* MAX */
#ifndef	ABS
#define	ABS(a)		((a) < 0 ? -(a) : (a))
#endif	/* ABS */

#ifdef	__STDC__
#define	BIT(b)	b ## ul
#define	STRINGIFY(s)	#s
#else	/* __STDC__ */
#define	BIT(b)	b
#define	STRINGIFY(s)	"s"
#endif	/* __STDC__ */
#define	BIT_SET(f, b)	((f) |= b)
#define	BIT_RESET(f, b)	((f) &= ~(b))
#define	BIT_FLIP(f, b)	((f) ^= (b))
#define	BIT_TEST(f, b)	((f) & (b))
#define	BIT_MATCH(f, b)	(((f) & (b)) == (b))
#define	BIT_COMPARE(f, b1, b2)	(((f) & (b1)) == b2)
#define	BIT_MASK_MATCH(f, g, b)	(!(((f) ^ (g)) & (b)))

#ifndef	offsetof
#ifdef	__HIGHC__
#define	offsetof(type, member) _offsetof(type, member)
#else	/* __HIGHC__ */
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif	/* __HIGHC__ */
#endif	/* offsetof */

#define	ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))

/* Error message defines */

#ifndef	errno
extern int errno;
#endif	/* errno */

/*
 *	Definitions of descriptions of bits
 */

typedef struct {
    u_int t_bits;
    const char *t_name;
} bits;


/* Our version of assert */
#define assert(ex) \
{ \
    if (!(ex)) { \
	task_assert(__FILE__, __LINE__, STRINGIFY(ex)); \
    } \
}


/*
 *	Routines defined in grand.c
 */
PROTOTYPE(grand_seed,
	  extern void,
	  (u_int32));
PROTOTYPE(grand,
	  extern u_int32,
	  (u_int32));
PROTOTYPE(grand_log2,
	  extern u_int32,
	  (int));
/*
 *	Routines defined in checksum.c
 */
PROTOTYPE(inet_cksumv,
	  extern u_int16,
	  (struct iovec *v,
	   int,
	   size_t));
PROTOTYPE(inet_cksum,
	  extern u_int16,
	  (void_t,
	   size_t));
#ifdef	FLETCHER_CHECKSUM
PROTOTYPE(iso_cksum,
	  extern u_int32,
	  (void_t,
	   size_t,
	   byte *));
#endif	/* FLETCHER_CHECKSUM */
#ifdef	MD5_CHECKSUM
PROTOTYPE(md5_cksum,
	  extern void,
	  (void_t,
	   size_t,
	   size_t,
	   void_t,
	   u_int32 *));
PROTOTYPE(md5_cksum_partial,
	  extern size_t,
	  (void_t,
	   void_t,
	   int,
	   u_int32 *));
#endif	/* MD5_CHEKCSUM */

/**/


/* Our versions of INSQUE/REMQUE */

/* The structure */
typedef struct _qelement {
    struct _qelement *q_forw;
    struct _qelement *q_back;
} *qelement;


#ifndef	INSQUE
/* Define INSQUE/REMQUE if not already defined (as maybe assembler code) */
#define INSQUE(elem, pred) { \
				 register qelement Xe = (qelement) (elem); \
				 register qelement Xp = (qelement) (pred); \
				 Xp->q_forw = (Xe->q_forw = (Xe->q_back = Xp)->q_forw)->q_back = Xe; \
			      }

#define	REMQUE(elem)	{ \
    			     register qelement Xe = (qelement) elem; \
			     (Xe->q_back->q_forw = Xe->q_forw)->q_back = Xe->q_back; \
			 }
#endif	/* INSQUE */
