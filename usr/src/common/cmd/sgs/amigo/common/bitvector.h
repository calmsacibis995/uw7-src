#ident	"@(#)amigo:common/bitvector.h	1.14"
/*
This is the amigo bitvector package.  It assumes that a bitvector is
laid out as:

	word 0		size in bits

	word 1		cardinality (number of bits set) or BADCARD

	word 2+		actual bits (bit #1 has mask 0x1)

Excess bits in the last word are always 0.
*/

#include <assert.h>

typedef unsigned int Word;
typedef Word* Bit_vector;
typedef int Bit;
typedef enum {false, true} Boolean;

#define BADCARD 0xffffffff		/* bad cardinality representation */
#define BITSPERWORD 32			/* bits per word */
#define BV_LO_WORD 2			/* first two are size/cardinality */
#define LOGBITSPERWORD 5		/* used for shifting */
#define BYTESPERWORD 4			/* bytes per word */
#define HI_MASK 0x80000000		/* high bit */
#define LO_MASK 0x1			/* low bit */

/*****************************************************************************/

/* iterate over a bit vector */

#define BV_FOR(bv,bit) \
	if (BV_SIZE((bv)) && BV_CARD((bv))) \
	{ \
		Bit bit; \
		int bv_i, bv_j, bv_tmp, bv_mask; \
		Word bv_w; \
		bv_i = 1; \
		bv_j = BV_LO_WORD; \
		bv_mask = (1 << LOGBITSPERWORD) - 1; \
		while (bv_i <= BV_SIZE((bv))) { \
			if ((bv_i & bv_mask) == 1) { \
				bv_w = (bv)[bv_j++]; \
				if (!bv_w) { \
					bv_i += BITSPERWORD; \
					continue; \
				} \
			} \
			bit = bv_i++; \
			bv_tmp = bv_w & LO_MASK; \
			bv_w >>= 1; \
			if (bv_tmp) {

/* iterate backwards */

#define BV_FOR_REVERSE(bv,bit) \
	if (BV_SIZE((bv)) && BV_CARD((bv))) \
	{ \
		Bit bit; \
		int bv_i, bv_j, bv_tmp, bv_mask; \
		Word bv_w; \
		bv_i = ((BV_SIZE((bv)) - 1) / BITSPERWORD + 1) * BITSPERWORD; \
		bv_j = BV_NUM_WORDS(BV_SIZE((bv))) - 1; \
		bv_mask = (1 << LOGBITSPERWORD) - 1; \
		while (bv_i >= 1) { \
			if ((bv_i & bv_mask) == 0) { \
				bv_w = (bv)[bv_j--]; \
				if (!bv_w) { \
					bv_i -= BITSPERWORD; \
					continue; \
				} \
			} \
			bit = bv_i--; \
			bv_tmp = bv_w & HI_MASK; \
			bv_w <<= 1; \
			if (bv_tmp) {

#define END_BV_FOR \
			} \
		} \
	}

#define BV_BREAK break

#define BV_CONTINUE continue

#define BV_SIZE(bv) ((bv)[0])		/* size */
#define BV_CARD(bv) ((bv)[1])		/* cardinality */

#define BV_WORD_OFFSET(bit) ((((bit)-1) >> LOGBITSPERWORD) + BV_LO_WORD)
/* word offset for a given bit */

#define BV_BIT_MASK(bit) (((unsigned)1) << (((bit)-1) % BITSPERWORD))
/* bit mask for setting bits */

#define BV_NUM_WORDS(bit) ((bit) < 1 ? BV_LO_WORD : BV_WORD_OFFSET((bit)) + 1)
/* number of words required */

/*****************************************************************************/

#ifndef PROTO
#ifdef __STDC__
#define PROTO(x,y) x y
#else
#define PROTO(x,y) x()
#endif
#endif

#ifdef TESTING
extern Bit_vector PROTO(bv_alloc, (int));
#else
extern Bit_vector PROTO(bv_alloc, (int,Arena));
#endif

extern void PROTO(bv_init, (Boolean,Bit_vector));

extern void PROTO(bv_print, (Bit_vector));

extern char* PROTO(bv_sprint, (Bit_vector));

extern void PROTO(bv_and_eq, (Bit_vector,Bit_vector));

extern void PROTO(bv_or_eq, (Bit_vector,Bit_vector));

extern void PROTO(bv_minus_eq, (Bit_vector,Bit_vector));

extern void PROTO(bv_assign, (Bit_vector,Bit_vector));

extern Boolean PROTO(bv_equal, (Bit_vector,Bit_vector));

extern void PROTO(bv_set_size, (int,Bit_vector));

extern int PROTO(bv_set_card, (Bit_vector));

extern int PROTO(bv_slow_card, (Bit_vector));

#define bv_belongs(b,bv) \
	(assert((bv) && (b) > 0 && (b) <= BV_SIZE((bv))), \
	((bv)[BV_WORD_OFFSET((b))] & BV_BIT_MASK((b))) ? 1 : 0)

#define bv_set_bit(b,bv) \
	(assert((bv) && (b) > 0 && (b) <= BV_SIZE((bv))), \
	!bv_belongs((b), (bv)) && BV_CARD((bv)) != BADCARD ? BV_CARD((bv))++ : 0, \
	(bv)[BV_WORD_OFFSET((b))] |= BV_BIT_MASK((b)))

#define bv_clear_bit(b,bv) \
	(assert((bv) && (b) > 0 && (b) <= BV_SIZE((bv))), \
	bv_belongs((b), (bv)) && BV_CARD((bv)) != BADCARD ? BV_CARD((bv))-- : 0, \
	(bv)[BV_WORD_OFFSET((b))] &= ~BV_BIT_MASK((b)))
