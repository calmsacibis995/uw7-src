#ident	"@(#)amigo:common/bitvector.c	1.15"
#ifndef TESTING
#include "amigo.h"
#else
#include "bitvector.h"
#include <stdio.h>
#endif

/* allocate a vector */
#ifdef TESTING
Bit_vector
bv_alloc(n)
int n;
#else
Bit_vector
bv_alloc(n, arena)
int n;
Arena arena;
#endif
{
	Bit_vector bv;
	int s;
	int i;

	s = BV_NUM_WORDS(n);
#ifdef TESTING
	bv = (Bit_vector)malloc(s * sizeof(Word));
#else
	bv = (Bit_vector)Arena_alloc(arena, s, Word);
#endif
	BV_SIZE(bv) = n;
	BV_CARD(bv) = 0;

	return bv;
}

/* set all bits to 0 or 1 */
void
bv_init(b, bv)
Boolean b;
Bit_vector bv;
{
	int fill;
	int n;
	int u;

	assert(bv && (b == false || b == true));

	if (!BV_SIZE(bv))
		return;

	fill = (b == true ? ~0 : 0);
	n = BV_NUM_WORDS(BV_SIZE(bv)) - BV_LO_WORD;
	memset((void*)(&bv[BV_LO_WORD]), fill, n * BYTESPERWORD);
	if (fill) {
		u = (BITSPERWORD - BV_SIZE(bv) % BITSPERWORD) % BITSPERWORD;
		bv[n + BV_LO_WORD - 1] &= (~(unsigned)0 >> u);
	}
	BV_CARD(bv) = (b == true ? BV_SIZE(bv) : 0);
}

/* print bit vector */
void
bv_print(bv)
Bit_vector bv;
{
	assert(bv);

	fputc('(',stderr);
	BV_FOR(bv,bit)
		fprintf(stderr," %d",bit);
	END_BV_FOR
	fputs(" )\n",stderr);	
	fflush(stderr);
}

/* format bit vector to file */
char*
bv_sprint(bv)
Bit_vector bv;
{
	static char str[200] = "(";
	char* p = str + 1;

	assert(bv);
	BV_FOR(bv,bit)
		p += sprintf(p," %d",bit);
	END_BV_FOR
	sprintf(p," )");
	return str;
}

/* and two bit vectors */
void
bv_and_eq(bv1, bv2)
Bit_vector bv1;
Bit_vector bv2;
{
	int i;
	int n;

	assert(bv1 && bv2);
	assert(BV_SIZE(bv1) == BV_SIZE(bv2));

	n = BV_NUM_WORDS(BV_SIZE(bv1));
	for (i = BV_LO_WORD; i < n; i++)
		bv1[i] &= bv2[i];

	BV_CARD(bv1) = BADCARD;
}

/* or two bit vectors */
void
bv_or_eq(bv1, bv2)
Bit_vector bv1;
Bit_vector bv2;
{
	int i;
	int n;

	assert(bv1 && bv2);
	assert(BV_SIZE(bv1) == BV_SIZE(bv2));

	n = BV_NUM_WORDS(BV_SIZE(bv1));
	for (i = BV_LO_WORD; i < n; i++)
		bv1[i] |= bv2[i];

	BV_CARD(bv1) = BADCARD;
}

/* minus two bit vectors */
void
bv_minus_eq(bv1, bv2)
Bit_vector bv1;
Bit_vector bv2;
{
	int i;
	int n;

	assert(bv1 && bv2);
	assert(BV_SIZE(bv1) == BV_SIZE(bv2));

	n = BV_NUM_WORDS(BV_SIZE(bv1));
	for (i = BV_LO_WORD; i < n; i++)
		bv1[i] &= ~bv2[i];

	BV_CARD(bv1) = BADCARD;
}

/* assign one vector to another */
void
bv_assign(bv1, bv2)
Bit_vector bv1;
Bit_vector bv2;
{
	assert(bv1 && bv2);
	assert(BV_SIZE(bv1) == BV_SIZE(bv2));

	memcpy((void*)(&bv1[BV_LO_WORD]), (void*)(&bv2[BV_LO_WORD]),
	    BYTESPERWORD * (BV_NUM_WORDS(BV_SIZE(bv1)) - BV_LO_WORD));

	BV_CARD(bv1) = BV_CARD(bv2);
}

/* check for equality */
Boolean
bv_equal(bv1, bv2)
Bit_vector bv1;
Bit_vector bv2;
{
	assert(bv1 && bv2);

	if (BV_SIZE(bv1) != BV_SIZE(bv2))
		return false;
	if (!BV_SIZE(bv1))
		return true;

	return memcmp((void*)(&bv1[BV_LO_WORD]), (void*)(&bv2[BV_LO_WORD]),
	    BYTESPERWORD * (BV_NUM_WORDS(BV_SIZE(bv1)) - BV_LO_WORD)) == 0 ?
		true : false;
}

/* set size */
void
bv_set_size(n, bv)
int n;
Bit_vector bv;
{
	assert(bv && n >= 0);

	if (BV_SIZE(bv) != n)
		BV_CARD(bv) = BADCARD;
	BV_SIZE(bv) = n;
}

/* cardinality */
int
bv_set_card(bv)
Bit_vector bv;
{
	assert(bv);

	if (BV_CARD(bv) == BADCARD)
		BV_CARD(bv) = bv_slow_card(bv);
	return BV_CARD(bv);
}

/* slow cardinality computation */
int
bv_slow_card(bv)
Bit_vector bv;
{
	int card;
	int i, j;
	Word w;
	int mask;

	assert(bv);

	i = 1;
	j = BV_LO_WORD;
	mask = (1 << LOGBITSPERWORD) - 1;
	card = 0;
	while (i <= BV_SIZE(bv)) {
		if ((i & mask) == 1) {
			w = bv[j++];
			if (!w) {
				i += BITSPERWORD;
				continue;
			}
		}
		card += (w & LO_MASK);
		w >>= 1;
		i++;
	}

	return card;
}

#ifdef TESTING

#define N1 40
#define N1_MAX 1025
#define N1_SIZE 33
int list1[N1] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
	17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
	33,34,63,64,65,1023,1024,1025};
#define N2 8
#define N2_MAX 1024
#define N2_SIZE 32
int list2[N2] = {1, 31, 32, 33, 63, 64, 65, 1024};

main()
{
	Bit_vector bv1, bv2;
	int i, j;

	bv1 = bv_alloc(N1_MAX);
	bv_init(false, bv1);
	if (bv1[2] || bv1[N1_SIZE])
		return 1;
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N1_MAX)
		return 2;
	for (i = 0; i < N1; i++)
		bv_set_bit(list1[i], bv1);
	for (i = 0; i < N1; i++)
		if (!bv_belongs(list1[i], bv1))
			return 3;
	if (bv_set_card(bv1) != N1 || BV_SIZE(bv1) != N1_MAX)
		return 4;
	for (i = 0; i < N1; i++)
		bv_clear_bit(list1[i], bv1);
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N1_MAX)
		return 5;
	bv_init(true, bv1);
	bv_init(false, bv1);
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N1_MAX)
		return 6;
	if (bv1[2] || bv1[N1_SIZE])
		return 7;
	bv_init(true, bv1);
	if (bv_set_card(bv1) != N1_MAX || BV_SIZE(bv1) != N1_MAX)
		return 8;
	if (bv1[2] != 0xffffffff || bv1[N1_SIZE] != 0xffffffff)
		return 9;

	bv1 = bv_alloc(N2_MAX);
	bv_init(false, bv1);
	if (bv1[2] || bv1[N2_SIZE])
		return 10;
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N2_MAX)
		return 11;
	for (i = 0; i < N2; i++)
		bv_set_bit(list2[i], bv1);
	for (i = 0; i < N2; i++)
		if (!bv_belongs(list2[i], bv1))
			return 12;
	if (bv_set_card(bv1) != N2 || BV_SIZE(bv1) != N2_MAX)
		return 13;
	for (i = 0; i < N2; i++)
		bv_clear_bit(list2[i], bv1);
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N2_MAX)
		return 14;
	bv_init(true, bv1);
	bv_init(false, bv1);
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N2_MAX)
		return 15;
	if (bv1[2] || bv1[N2_SIZE])
		return 16;
	bv_init(true, bv1);
	if (bv_set_card(bv1) != N2_MAX || BV_SIZE(bv1) != N2_MAX)
		return 17;
	if (bv1[2] != 0xffffffff || bv1[N2_SIZE] != 0xffffffff)
		return 18;

	bv1 = bv_alloc(N1_MAX);
	bv_init(false, bv1);
	bv2 = bv_alloc(N1_MAX);
	bv_init(false, bv2);

	if (bv_equal(bv1, bv2) != true)
		return 19;
	bv_init(true, bv2);
	if (bv_equal(bv1, bv2) != false)
		return 20;
	bv_init(true, bv1);
	if (bv_equal(bv1, bv2) != true)
		return 21;

	bv_init(false, bv2);
	bv_set_bit(1, bv2);
	bv_set_bit(N1_MAX, bv2);
	bv_assign(bv1, bv2);
	if (bv_equal(bv1, bv2) != true)
		return 22;
	if (bv_set_card(bv1) != 2 || bv_set_card(bv2) != 2)
		return 23;
	if (BV_SIZE(bv1) != N1_MAX || BV_SIZE(bv2) != N1_MAX)
		return 24;

	bv_init(true, bv1);
	bv_init(false, bv2);
	bv_and_eq(bv1, bv2);
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N1_MAX)
		return 25;
	if (bv1[2] != 0 || bv1[N1_SIZE] != 0)
		return 26;
	bv_init(true, bv1);
	bv_init(true, bv2);
	bv_or_eq(bv1, bv2);
	if (bv_set_card(bv1) != N1_MAX || BV_SIZE(bv1) != N1_MAX)
		return 27;
	if (bv1[2] != 0xffffffff || bv1[N1_SIZE] != 0xffffffff)
		return 28;
	bv_init(true, bv1);
	bv_init(true, bv2);
	bv_minus_eq(bv1, bv2);
	if (bv_set_card(bv1) != 0 || BV_SIZE(bv1) != N1_MAX)
		return 29;
	if (bv1[2] != 0 || bv1[N1_SIZE] != 0)
		return 30;

	bv1 = bv_alloc(N1_MAX);
	bv_init(false, bv1);
	for (i = 0; i < N1; i++)
		bv_set_bit(list1[i], bv1);
	j = 0;
	BV_FOR(bv1, b)
		if (b != list1[j])
			return 31;
		j++;
	END_BV_FOR
	j = N1 - 1;
	BV_FOR_REVERSE(bv1, b)
		if (b != list1[j])
			return 32;
		j--;
	END_BV_FOR

	bv1 = bv_alloc(N2_MAX);
	bv_init(false, bv1);
	for (i = 0; i < N2; i++)
		bv_set_bit(list2[i], bv1);
	j = 0;
	BV_FOR(bv1, b)
		if (b != list2[j])
			return 33;
		j++;
	END_BV_FOR
	j = N2 - 1;
	BV_FOR_REVERSE(bv1, b)
		if (b != list2[j])
			return 34;
		j--;
	END_BV_FOR

	bv1 = bv_alloc(N2_MAX);
	bv_init(false, bv1);
	BV_FOR(bv1, b)
		return 35;
	END_BV_FOR
	bv1 = bv_alloc(N2_MAX);
	bv_init(true, bv1);
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != BV_SIZE(bv1))
		return 36;

	bv1 = bv_alloc(N2_MAX);
	bv_init(false, bv1);
	BV_FOR_REVERSE(bv1, b)
		return 37;
	END_BV_FOR
	bv1 = bv_alloc(N2_MAX);
	bv_init(true, bv1);
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != N2_MAX)
		return 38;

	bv1 = bv_alloc(1);
	bv_init(true, bv1);
	bv_or_eq(bv1, bv1);
	if (bv_set_card(bv1) != 1)
		return 39;
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != 1)
		return 40;
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != 1)
		return 41;

	bv1 = bv_alloc(31);
	bv_init(true, bv1);
	bv_or_eq(bv1, bv1);
	if (bv_set_card(bv1) != 31)
		return 42;
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != 31)
		return 43;
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != 31)
		return 44;

	bv1 = bv_alloc(32);
	bv_init(true, bv1);
	bv_or_eq(bv1, bv1);
	if (bv_set_card(bv1) != 32)
		return 45;
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != 32)
		return 46;
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != 32)
		return 47;

	bv1 = bv_alloc(33);
	bv_init(true, bv1);
	bv_or_eq(bv1, bv1);
	if (bv_set_card(bv1) != 33)
		return 48;
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != 33)
		return 49;
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != 33)
		return 50;

	bv1 = bv_alloc(63);
	bv_init(true, bv1);
	bv_or_eq(bv1, bv1);
	if (bv_set_card(bv1) != 63)
		return 51;
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != 63)
		return 52;
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != 63)
		return 53;

	bv1 = bv_alloc(64);
	bv_init(true, bv1);
	bv_or_eq(bv1, bv1);
	if (bv_set_card(bv1) != 64)
		return 54;
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != 64)
		return 55;
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != 64)
		return 56;

	bv1 = bv_alloc(65);
	bv_init(true, bv1);
	bv_or_eq(bv1, bv1);
	if (bv_set_card(bv1) != 65)
		return 57;
	i = 0;
	BV_FOR(bv1, b)
		i++;
	END_BV_FOR
	if (i != 65)
		return 58;
	i = 0;
	BV_FOR_REVERSE(bv1, b)
		i++;
	END_BV_FOR
	if (i != 65)
		return 59;

	return 0;
}
#endif
