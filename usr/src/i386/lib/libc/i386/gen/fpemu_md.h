#ident	"@(#)libc-i386:gen/fpemu_md.h	1.1"
/* from "@(#)fpemu:i386/fpemu_md.h	1.1"	*/
/* fpemu_md.h */

/* i386-dependent floating point definitions. */

/* Machine is little-endian */
#define	RTOLBYTES

/* Encoding of infinity:  low to high order 4 half-words */
#define	X_INFINITY	0, 0, 0, 0x8000

/* Encoding of "indefinite" result */
#define X_INDEFINITE	-1, X_EXPMAX, IFL_NaN, 0, 0, 0, 0, 0xc000

/* Bias for denormalized number significands */
#define	X_DENORM_BIAS	1

/* Code for NaN processing. */
#define	FP_NAN_PROC \
    fp_i_t * pr = px;			/* Assumed result */	\
								\
    switch(op) {						\
    case FP_NOP_OP:	return( pr );				\
    case FP_CMP_OP:	return( &v_pluszero );			\
    case FP_ISZERO_OP:	return( &v_pluszero );			\
    case FP_XTOFP_OP:						\
	/* -1 accounts for explicit hidden bit in extended */	\
	(void) fp_rshift(pr, X_FRACSIZE-F_FRACSIZE-1);		\
	pr->i_mag[0] = 0;					\
	(void) fp_lshift(pr, X_FRACSIZE-F_FRACSIZE-1);		\
	break;							\
    case FP_XTODP_OP:						\
	/* -1 accounts for explicit hidden bit in extended */	\
	(void) fp_rshift(pr, X_FRACSIZE-D_FRACSIZE-1);		\
	pr->i_mag[0] = 0;					\
	(void) fp_lshift(pr, X_FRACSIZE-D_FRACSIZE-1);		\
	break;							\
    }								\
								\
    if (ISQNaN(px)) {						\
	if (ISQNaN(py) && fp_compmag(px,py) < 0)		\
	    pr = py;						\
    }								\
    else if (ISQNaN(py))					\
	pr = py;						\
    else if (ISSNaN(px)) {					\
	if (ISSNaN(py) && fp_compmag(px,py) < 0)		\
	    pr = py;						\
    }								\
    else if (ISSNaN(py))					\
	pr = py;						\
 								\
    SETQNaN(pr);						\
    if ((pr->i_mag[4] & RB_HIBIT) == 0)				\
	pr = &v_indefinite;					\
    return pr;

/* Use default representation layout. */
