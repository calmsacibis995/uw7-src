#ident	"@(#)kern-i386:util/kdb/scodb/bkp.h	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) 1989-1993 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

#define		BPNL	31
#define		NLPBP	8

#define		_BKP_CLEAR		0
#define		_BKP_SET		1
#define		_BKP_DISABLED		2

#define		BKP_CLEAR(b)		((b)->bp_flags =  _BKP_CLEAR, \
					 (b)->bp_name[0] = '\0'		)
#define		BKP_CLEARD(b)		((b)->bp_flags =  _BKP_CLEAR, \
					 (b)->bp_name[0] = '\0',      \
					 (b)->bp_type = 0		)
#define		BKP_SET(b)		((b)->bp_flags =  _BKP_SET	)
#define		BKP_DISABLE(b)		((b)->bp_flags |= _BKP_DISABLED	)
#define		BKP_ISCLEAR(b)		((b)->bp_flags == _BKP_CLEAR	)
#define		BKP_ISSET(b)		((b)->bp_flags != _BKP_CLEAR	)
#define		BKP_ISDISABLED(b)	((b)->bp_flags &  _BKP_DISABLED	)
#define		BKP_ENABLE(b)		((b)->bp_flags &= ~_BKP_DISABLED)

struct bkp {
	int		 bp_flags;
	long		 bp_seg;
	long		 bp_off;
	unsigned char	 bp_svopcode;
	char		 bp_name[BPNL];
	struct ilin	*bp_cmds;
};

struct dbkp {
	int		 bp_flags;
	long		 bp_seg;
	long		 bp_off;
	char		 bp_type;	/* 0 : not used */
	char		 bp_mode;
	char		 bp_name[BPNL];
	struct ilin	*bp_cmds;
};

#define		FOUND_NOTFOUND	0	/* sounds stupid */
#define		FOUND_USERQ	1
#define		FOUND_BP	2
#define		FOUND_DBP	3

/*
*	The following macros make extensive use of token
*	concatenation given by the ANSI cpp
*
*	DBPTYP: type of data breakpoint
*	DBPSET:	is data breakpoint n set?
*/
#define		DR6(f, n)	(dr6.d6_bits.d6_ ## f ## n)
#define		DR7(f, n)	(dr7.d7_bits.d7_ ## f ## n)
#define		DBPTYP(n)	DR7(rw, n)
#define		DBPSET(n)	(DR6(b, n) && (DR7(l, n) || DR7(g, n)))

/*
*	what DR7 looks like
*/
union dbr7 {
	long	d7_val;
	struct {
		unsigned int	d7_l0:1;	/*  0		*/
		unsigned int	d7_g0:1;	/*  1		*/
		unsigned int	d7_l1:1;	/*  2		*/
		unsigned int	d7_g1:1;	/*  3		*/
		unsigned int	d7_l2:1;	/*  4		*/
		unsigned int	d7_g2:1;	/*  5		*/
		unsigned int	d7_l3:1;	/*  6		*/
		unsigned int	d7_g3:1;	/*  7		*/
		unsigned int	d7_le:1;	/*  8		*/
		unsigned int	d7_ge:1;	/*  9		*/
		unsigned int	d7_u1:3;	/* 10 - 12	*/
		unsigned int	d7_u2:1;	/* 13		*/
		unsigned int	d7_u3:2;	/* 14 - 15	*/
		unsigned int	d7_rw0:2;	/* 16 - 17	*/
		unsigned int	d7_ln0:2;	/* 18 - 19	*/
		unsigned int	d7_rw1:2;	/* 20 - 21	*/
		unsigned int	d7_ln1:2;	/* 22 - 23	*/
		unsigned int	d7_rw2:2;	/* 24 - 25	*/
		unsigned int	d7_ln2:2;	/* 26 - 27	*/
		unsigned int	d7_rw3:2;	/* 28 - 29	*/
		unsigned int	d7_ln3:2;	/* 30 - 31	*/
	}	d7_bits;
};

/*
*	what DR6 looks like
*/
union dbr6 {
	long	d6_val;
	struct {
		unsigned int	d6_b0:1;	/*  0		*/
		unsigned int	d6_b1:1;	/*  1		*/
		unsigned int	d6_b2:1;	/*  2		*/
		unsigned int	d6_b3:1;	/*  3		*/
		unsigned int	d6_u1:9;	/*  4 - 12	*/
		unsigned int	d6_bd:1;	/* 13		*/
		unsigned int	d6_bs:1;	/* 14		*/
		unsigned int	d6_bt:1;	/* 15		*/
		unsigned int	d6_u2:16;	/* 16 - 31	*/
	}	d6_bits;
};

#define BP_CURPROC	(ismpx() ? processor_index : 0)

/*
 * Structure used for setting up value breakpoints.
 */

struct bp_value {
	unsigned vaddr;		/* breakpoint virtual addr */
	unsigned value;		/* value to break on */
	unsigned cond;		/* conditional */
	unsigned length;	/* data length */
};

struct bp_values {			/* per processor value breakpoints */
	struct bp_value dr[4];
};

extern struct bp_values bp_values[];

/*
 * Conditions for value breakpoints ("cond" field above).
 */

#define COND_ALWAYS	0	/* no conditional */
#define COND_NE		1	/* data != value */
#define COND_EQ		2	/* data == value */
#define COND_GE		3	/* data >= value */
#define COND_LE		4	/* data <= value */
#define COND_AND	5	/* (data & value) != 0 */
