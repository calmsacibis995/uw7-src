#ident	"@(#)optim:i386/imull.c	1.1.1.10"
/* imull.c
**
**	Intel 386 optimizer:  for rewriting imull instruction
**
*/

/* #include "defs.h" -- optim.h takes care of this */
#include "optim.h"
#include "optutil.h"

#define odd(num) ((num) & 1)


/* imull -- imull instruction optimizer
**
** This routine will attempt to rewrite imull instructions of the form:
**		imull	C,op[,%reg]
** where C is an integer constant.  There are 38 algorithms used to
** attempt to factor C such that alternative instructions can be used
** to achieve the multiply in less clock cycles.  These algorithms are
** described in the paper "32-Bit Multiplication on the 386" from Dan Lau
** at Intel.
**
** If there are 3 operands, and the second operand is not a register,
** the second operand will be copied to the destination.  For example,
**		imull $70,-8(%ebp),%edi
** will be optimized as if the sequence was
		movl -8(%ebp),%edi
**		imull $70,%edi,%edi
*/

#define LEA_SIZE (sizeof LEA_set / sizeof LEA_set[0])
#define forallLEA(i) for(i=0; i < LEA_SIZE; ++i)
static int LEA_set[] = { 2, 3, 4, 5, 8, 9 };

#define LEA2_SIZE (sizeof LEA_set2 / sizeof LEA_set2[0])
#define forallLEA2(i) for(i=0; i < LEA2_SIZE; ++i)
static int LEA_set2[] = { 2, 4, 8 };

#define eff_LEA_SIZE (sizeof eff_LEA_set / sizeof eff_LEA_set[0])
#define foralleffLEA(i) for(i=0; i < eff_LEA_SIZE; ++i)
static int eff_LEA_set[] = { 3, 5, 9 };

#define SHIFT_MAX 15	/* checks powers of 2 up to 65536 */
#define forallPWRof2(i) for (i=1, pwrof2=2; i <= SHIFT_MAX; ++i, pwrof2 <<= 1)

static char *src, *dest, *tmp;
static int i, j, k, l, pwrof2, multiplier;
NODE *prepend();
static NODE *mkleal(), *mkmovl(), *prepend_move();
static NODE *pmov0, *ppush, *pmov, *pnew, *instr;
static void mkshll(), mkpushl(), mkaddl(), mksubl(), exchange_instructions();

typedef struct {
	short num;
	char  indx[2];
	} TABLE2;

typedef struct {
	short num;
	char  indx[3];
	} TABLE3;

typedef struct {
	int num;
	char  indx[4];
	} TABLE4;


static boolean
isinthere2(table, numelems)
TABLE2 *table;
int numelems;
{
	register int low, high, mid;
	TABLE2 *t;

	low = 0;
	high = numelems - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		t = &table[mid];
		if (multiplier < t->num)
			high = mid - 1;
		else if (multiplier > t->num)
			low = mid + 1;
		else {
			i = t->indx[0];
			j = t->indx[1];
			return true;
		}
	}

	return false;
}


static boolean
isinthere3(table, numelems)
TABLE3 *table;
int numelems;
{
	register int low, high, mid;
	TABLE3 *t;

	low = 0;
	high = numelems - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		t = &table[mid];
		if (multiplier < t->num)
			high = mid - 1;
		else if (multiplier > t->num)
			low = mid + 1;
		else {
			i = t->indx[0];
			j = t->indx[1];
			k = t->indx[2];
			return true;
		}
	}

	return false;
}


static boolean
isinthere4(table, numelems)
TABLE4 *table;
int numelems;
{
	register int low, high, mid;
	TABLE4 *t;

	low = 0;
	high = numelems - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		t = &table[mid];
		if (multiplier < t->num)
			high = mid - 1;
		else if (multiplier > t->num)
			low = mid + 1;
		else {
			i = t->indx[0];
			j = t->indx[1];
			k = t->indx[2];
			l = t->indx[3];
			return true;
		}
	}

	return false;
}


static isalg1()
{
	forallLEA(i)
		if (multiplier == LEA_set[i])
			return true;
	return false;
}
	

static TABLE2 alg3set[] = { /* num = LEA_set[indx[0]] * LEA_set[indx[1]] */
	 {  6, { 0, 1 }},
	 { 10, { 0, 3 }},
	 { 12, { 1, 2 }},
	 { 15, { 1, 3 }},
	 { 16, { 0, 4 }},
	 { 18, { 0, 5 }},
	 { 20, { 2, 3 }},
	 { 24, { 1, 4 }},
	 { 25, { 3, 3 }},
	 { 27, { 1, 5 }},
	 { 32, { 2, 4 }},
	 { 36, { 2, 5 }},
	 { 40, { 3, 4 }},
	 { 45, { 3, 5 }},
	 { 64, { 4, 4 }},
	 { 72, { 4, 5 }},
	 { 81, { 5, 5 }},
};
#define isalg3() isinthere2(alg3set, sizeof alg3set / sizeof alg3set[0])


static TABLE2 alg4set[] = { /* num = LEA_set[indx[0]] + LEA_set2[indx[1]] */
	 {  7, { 1, 1 }},
	 { 11, { 1, 2 }},
	 { 13, { 3, 2 }},
	 { 17, { 5, 2 }}
	};
#define isalg4() isinthere2(alg4set, sizeof alg4set / sizeof alg4set[0])


static boolean
isalg5()
{
	forallPWRof2(i)
		foralleffLEA(j)
			if (multiplier == pwrof2 * eff_LEA_set[j])
				return true;
	return false;
}


static TABLE3 alg6set[] = {
	/* num = LEA_set[indx[0]] * LEA_set[indx[1]] * LEA_set[indx[2]] */
	{   30, {    0,    1,    3 }},
	{   48, {    0,    1,    4 }},
	{   50, {    0,    3,    3 }},
	{   54, {    0,    1,    5 }},
	{   60, {    1,    2,    3 }},
	{   75, {    1,    3,    3 }},
	{   80, {    0,    3,    4 }},
	{   90, {    0,    3,    5 }},
	{   96, {    1,    2,    4 }},
	{  100, {    2,    3,    3 }},
	{  108, {    1,    2,    5 }},
	{  120, {    1,    3,    4 }},
	{  125, {    3,    3,    3 }},
	{  128, {    0,    4,    4 }},
	{  135, {    1,    3,    5 }},
	{  144, {    0,    4,    5 }},
	{  160, {    2,    3,    4 }},
	{  162, {    0,    5,    5 }},
	{  180, {    2,    3,    5 }},
	{  192, {    1,    4,    4 }},
	{  200, {    3,    3,    4 }},
	{  216, {    1,    4,    5 }},
	{  225, {    3,    3,    5 }},
	{  243, {    1,    5,    5 }},
	{  256, {    2,    4,    4 }},
	{  288, {    2,    4,    5 }},
	{  320, {    3,    4,    4 }},
	{  324, {    2,    5,    5 }},
	{  360, {    3,    4,    5 }},
	{  405, {    3,    5,    5 }},
	{  512, {    4,    4,    4 }},
	{  576, {    4,    4,    5 }},
	{  648, {    4,    5,    5 }},
	{  729, {    5,    5,    5 }}
};
#define isalg6() isinthere3(alg6set, sizeof alg6set / sizeof alg6set[0])

#define isalg7() isalg3()

static boolean
isalg8()
{
	int ndx;

	/* rewrite to do binary search */
	for (ndx=0; ndx < sizeof alg6set / sizeof alg6set[0]; ++ndx)
		if (multiplier == alg6set[ndx].num + 1) {
			i = alg6set[ndx].indx[0];
			j = alg6set[ndx].indx[1];
			k = alg6set[ndx].indx[2];
			if (odd(i) && odd(j) && odd(k))
				continue;
			if (odd(k)) {
				if (odd(i)) {	/* swap j and k */
					ndx = j;
					j = k;
				} else {	/* swap i and k */
					ndx = i;
					i = k;
				}
				k = ndx;
			}
			return true;
		}

	return false;
}


static TABLE3 alg9set[] = {
	/* num = (LEA_set[indx[0]] + LEA_set2[indx[1]]) * LEA_set[indx[2]] */
	{  14, {    1,    1,    0 }},
	{  22, {    1,    2,    0 }},
	{  34, {    5,    2,    0 }},
	{  35, {    1,    1,    3 }},
	{  39, {    3,    2,    1 }},
	{  44, {    1,    2,    2 }},
	{  52, {    3,    2,    2 }},
	{  56, {    1,    1,    4 }},
	{  63, {    1,    1,    5 }},
	{  68, {    5,    2,    2 }},
	{  85, {    5,    2,    3 }},
	{  88, {    1,    2,    4 }},
	{  99, {    1,    2,    5 }},
	{ 104, {    3,    2,    4 }},
	{ 117, {    3,    2,    5 }},
	{ 136, {    5,    2,    4 }},
	{ 153, {    5,    2,    5 }},
	};
#define isalg9() isinthere3(alg9set, sizeof alg9set / sizeof alg9set[0])


static TABLE3 alg10set[] = {
	/* num = LEA_set[indx[0]] * LEA_set2[indx[2]] + LEA_set[indx[1]] */
	{  23, {    3,    1,    1 }},
	{  29, {    1,    3,    2 }},
	{  38, {    5,    0,    1 }},
	{  42, {    3,    0,    2 }},
	{  43, {    3,    1,    2 }},
	{  66, {    4,    0,    2 }},
	{  67, {    4,    1,    2 }},
	{  69, {    4,    3,    2 }},
	{  74, {    5,    0,    2 }},
	{  76, {    5,    2,    2 }},
	{  77, {    5,    3,    2 }},
	};
#define isalg10() isinthere3(alg10set, sizeof alg10set / sizeof alg10set[0])


static boolean
isalg11()
{
	int jj;

	forallPWRof2(i)
		for (jj=0; jj < sizeof alg3set / sizeof alg3set[0]; ++jj)
			if (multiplier == pwrof2 * alg3set[jj].num) {
				j = alg3set[jj].indx[0];
				k = alg3set[jj].indx[1];
				return true;
			}

	return false;
}


static boolean
isalg12()
{
	forallPWRof2(i)
		if (multiplier == pwrof2 - 1)
			return true;
	return false;
}


static boolean
isalg13()
{
	forallLEA(i)
		forallPWRof2(j)
			if (multiplier == LEA_set[i] + pwrof2)
				return true;
	return false;
}


static boolean
isalg14()
{
	forallLEA(i)
		forallPWRof2(j)
			if (multiplier == pwrof2 - LEA_set[i])
				return true;
	return false;
}


static boolean
isalg15()
{
	forallLEA(i)
		forallPWRof2(j)
			if (multiplier == pwrof2 * LEA_set[i] + 1)
				return true;
	return false;
}


static boolean
isalg16()
{
	forallLEA(i)
		forallPWRof2(j)
			forallLEA2(k)
			if (multiplier == pwrof2 + LEA_set[i] * LEA_set2[k])
				return true;
	return false;
}


static TABLE4 alg17set[] = {
	/* num = LEA_set[indx[0]] * LEA_set[indx[1]] * LEA_set[indx[2]] *
		 LEA_set[indx[3]] */

	{  150, {    0,    1,    3,    3 }},
	{  250, {    0,    3,    3,    3 }},
	{  270, {    0,    1,    3,    5 }},
	{  300, {    1,    2,    3,    3 }},
	{  375, {    1,    3,    3,    3 }},
	{  450, {    0,    3,    3,    5 }},
	{  486, {    0,    1,    5,    5 }},
	{  500, {    2,    3,    3,    3 }},
	{  540, {    1,    2,    3,    5 }},
	{  600, {    1,    3,    3,    4 }},
	{  625, {    3,    3,    3,    3 }},
	{  675, {    1,    3,    3,    5 }},
	{  810, {    0,    3,    5,    5 }},
	{  900, {    2,    3,    3,    5 }},
	{  972, {    1,    2,    5,    5 }},
	{ 1000, {    3,    3,    3,    4 }},
	{ 1024, {    0,    4,    4,    4 }},
	{ 1080, {    1,    3,    4,    5 }},
	{ 1125, {    3,    3,    3,    5 }},
	{ 1152, {    0,    4,    4,    5 }},
	{ 1215, {    1,    3,    5,    5 }},
	{ 1280, {    2,    3,    4,    4 }},
	{ 1296, {    0,    4,    5,    5 }},
	{ 1440, {    2,    3,    4,    5 }},
	{ 1458, {    0,    5,    5,    5 }},
	{ 1536, {    1,    4,    4,    4 }},
	{ 1600, {    3,    3,    4,    4 }},
	{ 1620, {    2,    3,    5,    5 }},
	{ 1728, {    1,    4,    4,    5 }},
	{ 1800, {    3,    3,    4,    5 }},
	{ 1944, {    1,    4,    5,    5 }},
	{ 2025, {    3,    3,    5,    5 }},
	{ 2048, {    2,    4,    4,    4 }},
	{ 2187, {    1,    5,    5,    5 }},
	{ 2304, {    2,    4,    4,    5 }},
	{ 2560, {    3,    4,    4,    4 }},
	{ 2592, {    2,    4,    5,    5 }},
	{ 2880, {    3,    4,    4,    5 }},
	{ 2916, {    2,    5,    5,    5 }},
	{ 3240, {    3,    4,    5,    5 }},
	{ 3645, {    3,    5,    5,    5 }},
	{ 4096, {    4,    4,    4,    4 }},
	{ 4608, {    4,    4,    4,    5 }},
	{ 5184, {    4,    4,    5,    5 }},
	{ 5832, {    4,    5,    5,    5 }},
	{ 6561, {    5,    5,    5,    5 }},
};
#define isalg17() isinthere4(alg17set, sizeof alg17set / sizeof alg17set[0])


static boolean
isalg18()
{
	/* num = LEA_set[i] * LEA_set[j] - 1 */
	/* the only number left that matches this formula is 71 */
	if (multiplier == 71) {
		i = 4; 
		j = 5; 
		return true;
	}
	return false;
}


static TABLE3 alg19set[] = {
	/* num = LEA_set[indx[0]] + LEA_set[indx[1]] * LEA_set[indx[2]] */

	{ 47, { 0, 3, 5 }},
	{ 53, { 4, 3, 5 }},
	{ 83, { 0, 5, 5 }},
	{ 86, { 3, 5, 5 }},
	{ 89, { 4, 5, 5 }},
};
#define isalg19() isinthere3(alg19set, sizeof alg19set / sizeof alg19set[0])


static boolean
isalg20()
{
	/* num = (LEA_set[i] + LEA_set[j]) * LEA_set[k] */
	/* the only number left that matches this formula is 112 */
	if (multiplier == 112) {
		i = 3; 
		j = 5; 
		k = 4;
		return true;
	}
	return false;
}


static TABLE3 alg21set[] = {
	/* num = (LEA_set[indx[0]] * LEA_set[indx[1]] + 1) * LEA_set[indx[2]] */

	{  57, { 0, 5, 1 }},
	{  78, { 3, 3, 1 }},
	{  92, { 3, 5, 0 }},
	{  95, { 0, 5, 3 }},
	{ 105, { 2, 3, 3 }},
	{ 111, { 2, 5, 1 }},
	{ 112, { 1, 5, 2 }},
	{ 165, { 2, 4, 3 }},
	{ 171, { 0, 5, 5 }},
	{ 184, { 3, 5, 2 }},
	{ 185, { 2, 5, 3 }},
	{ 189, { 2, 3, 5 }},
	{ 195, { 4, 4, 1 }},
	{ 205, { 3, 4, 3 }},
	{ 208, { 3, 3, 4 }},
	{ 219, { 4, 5, 1 }},
	{ 224, { 1, 5, 4 }},
	{ 230, { 3, 5, 3 }},
	{ 234, { 3, 3, 5 }},
	{ 246, { 5, 5, 1 }},
	{ 297, { 2, 4, 5 }},
	{ 333, { 2, 5, 5 }},
	{ 365, { 4, 5, 3 }},
	{ 368, { 3, 5, 4 }},
	{ 369, { 3, 4, 5 }},
	{ 410, { 5, 5, 3 }},
	{ 414, { 3, 5, 5 }},
	{ 585, { 4, 4, 5 }},
	{ 656, { 5, 5, 4 }},
	{ 657, { 4, 5, 5 }},
	{ 738, { 5, 5, 5 }},
};
#define isalg21() isinthere3(alg21set, sizeof alg21set / sizeof alg21set[0])


static TABLE4 alg22set[] = {
	/* num = (LEA_set[indx[0]] * LEA_set[indx[1]] * LEA_set2[indx[3]] +
		  LEA_set[indx[2]] */

	{  58, { 1, 5, 2, 0 }},
	{  93, { 3, 5, 1, 0 }},
	{  94, { 3, 5, 2, 0 }},
	{  98, { 1, 2, 0, 2 }},
	{ 102, { 3, 3, 0, 1 }},
	{ 103, { 3, 3, 1, 1 }},
	{ 110, { 1, 5, 0, 1 }},
	{ 113, { 1, 5, 3, 1 }},
	{ 116, { 1, 5, 4, 1 }},
	{ 122, { 1, 3, 0, 2 }},
	{ 147, { 0, 5, 1, 2 }},
	{ 149, { 0, 5, 3, 2 }},
	{ 166, { 5, 5, 2, 0 }},
	{ 167, { 5, 5, 3, 0 }},
	{ 169, { 2, 3, 5, 2 }},
	{ 170, { 5, 5, 4, 0 }},
	{ 182, { 3, 5, 0, 1 }},
	{ 183, { 3, 5, 1, 1 }},
	{ 188, { 3, 5, 4, 1 }},
	{ 194, { 1, 4, 0, 2 }},
	{ 196, { 1, 4, 2, 2 }},
	{ 197, { 1, 4, 3, 2 }},
	{ 202, { 3, 3, 0, 2 }},
	{ 203, { 3, 3, 1, 2 }},
	{ 204, { 3, 3, 2, 2 }},
	{ 209, { 3, 3, 5, 2 }},
	{ 218, { 1, 5, 0, 2 }},
	{ 220, { 1, 5, 2, 2 }},
	{ 221, { 1, 5, 3, 2 }},
	{ 290, { 2, 5, 0, 2 }},
	{ 291, { 2, 5, 1, 2 }},
	{ 293, { 2, 5, 3, 2 }},
	{ 322, { 3, 4, 0, 2 }},
	{ 323, { 3, 4, 1, 2 }},
	{ 326, { 5, 5, 0, 1 }},
	{ 327, { 5, 5, 1, 1 }},
	{ 329, { 3, 4, 5, 2 }},
	{ 332, { 5, 5, 4, 1 }},
	{ 362, { 3, 5, 0, 2 }},
	{ 363, { 3, 5, 1, 2 }},
	{ 364, { 3, 5, 2, 2 }},
	{ 578, { 4, 5, 0, 2 }},
	{ 579, { 4, 5, 1, 2 }},
	{ 580, { 4, 5, 2, 2 }},
	{ 581, { 4, 5, 3, 2 }},
	{ 585, { 4, 5, 5, 2 }},
	{ 650, { 5, 5, 0, 2 }},
	{ 651, { 5, 5, 1, 2 }},
	{ 652, { 5, 5, 2, 2 }},
	{ 653, { 5, 5, 3, 2 }},
	{ 657, { 5, 5, 5, 2 }},
};
#define isalg22() isinthere4(alg22set, sizeof alg22set / sizeof alg22set[0])


static TABLE4 alg23set[] = {
	/* num = (LEA_set[indx[0]] * LEA_set2[indx[2]] + LEA_set[indx[1]]) *
		  LEA_set[indx[3]] */

	{  87, { 1, 3, 2, 1 }},
	{ 114, { 5, 0, 1, 1 }},
	{ 115, { 3, 1, 1, 3 }},
	{ 154, { 5, 3, 2, 0 }},
	{ 156, { 5, 1, 1, 2 }},
	{ 172, { 3, 1, 2, 2 }},
	{ 175, { 2, 1, 2, 3 }},
	{ 176, { 3, 0, 1, 4 }},
	{ 190, { 5, 0, 1, 3 }},
	{ 198, { 3, 0, 1, 5 }},
	{ 207, { 3, 1, 1, 5 }},
	{ 210, { 3, 0, 2, 3 }},
	{ 215, { 3, 1, 2, 3 }},
	{ 222, { 5, 0, 2, 1 }},
	{ 228, { 5, 2, 2, 1 }},
	{ 231, { 5, 3, 2, 1 }},
	{ 232, { 1, 3, 2, 4 }},
	{ 245, { 3, 5, 2, 3 }},
	{ 304, { 5, 0, 1, 4 }},
	{ 306, { 2, 0, 2, 5 }},
	{ 308, { 5, 3, 2, 2 }},
	{ 312, { 5, 1, 1, 4 }},
	{ 315, { 2, 1, 2, 5 }},
	{ 330, { 4, 0, 2, 3 }},
	{ 335, { 4, 1, 2, 3 }},
	{ 336, { 3, 0, 2, 4 }},
	{ 340, { 4, 2, 2, 3 }},
	{ 342, { 5, 0, 1, 5 }},
	{ 344, { 3, 1, 2, 4 }},
	{ 345, { 4, 3, 2, 3 }},
	{ 351, { 5, 1, 1, 5 }},
	{ 352, { 3, 2, 2, 4 }},
	{ 370, { 5, 0, 2, 3 }},
	{ 378, { 3, 0, 2, 5 }},
	{ 380, { 5, 2, 2, 3 }},
	{ 387, { 3, 1, 2, 5 }},
	{ 392, { 3, 5, 2, 4 }},
	{ 396, { 3, 2, 2, 5 }},
	{ 441, { 3, 5, 2, 5 }},
	{ 592, { 5, 0, 2, 4 }},
	{ 594, { 4, 0, 2, 5 }},
	{ 603, { 4, 1, 2, 5 }},
	{ 608, { 5, 2, 2, 4 }},
	{ 612, { 4, 2, 2, 5 }},
	{ 616, { 5, 3, 2, 4 }},
	{ 621, { 4, 3, 2, 5 }},
	{ 666, { 5, 0, 2, 5 }},
	{ 684, { 5, 2, 2, 5 }},
	{ 693, { 5, 3, 2, 5 }},
};
#define isalg23() isinthere4(alg23set, sizeof alg23set / sizeof alg23set[0])


static boolean
isalg24()
{
	forallLEA(i)
		forallPWRof2(j)
			if (multiplier == pwrof2 + LEA_set[i] - 1)
				return true;
	return false;
}


static boolean
isalg25()
{
	forallLEA(i)
		forallPWRof2(j)
			if (multiplier == pwrof2 - LEA_set[i] - 1)
				return true;
	return false;
}


static boolean
isalg26()
{
	forallLEA(i)
		forallPWRof2(j)
			if (multiplier == pwrof2 - LEA_set[i] + 1)
				return true;
	return false;
}


static boolean
isalg27()
{
	forallPWRof2(i)
		forallLEA(j)
			if (multiplier == (pwrof2 + 1) * LEA_set[j])
				return true;
	return false;
}


static boolean
isalg28()
{
	forallPWRof2(i)
		forallLEA(j)
			if (multiplier == (pwrof2 - 1) * LEA_set[j])
				return true;
	return false;
}


static boolean
isalg29()
{
	forallLEA(i)
		forallLEA(j)
			forallPWRof2(k)
				if (multiplier ==
				    pwrof2 - LEA_set[j] * LEA_set[i])
					return true;
	return false;
}


static boolean
isalg30()
{
	forallLEA(i)
		forallLEA2(j)
			forallPWRof2(k)
				if (multiplier ==
				    pwrof2 - (LEA_set2[j] + LEA_set[i]))
					return true;
	return false;
}


static boolean
isalg31()
{
	forallLEA(i)
		forallPWRof2(j)
			forallLEA(k)
				if (multiplier ==
				    LEA_set[k] * pwrof2 + LEA_set[i])
					return true;
	return false;
}


static boolean
isalg32()
{
	forallLEA(i)
		forallPWRof2(j)
			forallLEA(k)
				if (multiplier ==
				    LEA_set[k] * pwrof2 - LEA_set[i])
					return true;
	return false;
}


static boolean
isalg33()
{
	forallLEA(i)
		forallPWRof2(j)
			forallLEA(k)
				if (multiplier ==
				    LEA_set[k] * (pwrof2 - LEA_set[i]))
					return true;
	return false;
}


static boolean
isalg34()
{
	forallLEA(i)
		forallPWRof2(j)
			forallLEA(k)
				if (multiplier ==
				    (LEA_set[i] * pwrof2 + 1) * LEA_set[k])
					return true;
	return false;
}


static boolean
isalg35()
{
	forallLEA(i)
		forallLEA(j)
			forallPWRof2(k)
				if (multiplier ==
				    pwrof2 + LEA_set[j] * LEA_set[i])
					return true;
	return false;
}


static boolean
isalg36()
{
	forallLEA(i)
		forallPWRof2(j)
			forallLEA(k)
				if (multiplier ==
				    LEA_set[k] * (pwrof2 + LEA_set[i]))
					return true;
	return false;
}


static boolean
isalg37()
{
	forallLEA(i)
		forallLEA(j)
			forallPWRof2(k)
				forallLEA2(l)
					if (multiplier ==
					    LEA_set[i] * LEA_set[j] *
					    LEA_set2[l] + pwrof2)
					return true;
	return false;
}


static boolean
isalg38()
{
	forallLEA(i)
		forallPWRof2(j)
			forallLEA2(k)
				forallLEA(l)
					if (multiplier ==
					    (LEA_set2[k] * LEA_set[i] +
					     pwrof2) * LEA_set[l])
					return true;
	return false;
}


static void
gettmpreg()
{
	static char *tmpregs[] = {"%edx", "%ecx", "%eax",  "%esi" ,"%edi" ,"%ebx",
	                          "%ebi" , "%edx", "%ecx","%eax", NULL };
	char **tmpreg, *found;

	for (tmpreg = tmpregs; *tmpreg != NULL; ++tmpreg) {
		tmp = *tmpreg;
		if (strcmp(dest, tmp) && strcmp(src, tmp))
			if (instr->nlive & scanreg(tmp, false))
				found = tmp;
			else
				return;
		
	}

	tmp = found;	/* forced to do a push */
	mkpushl();
}


void
imull()
{
	char sameregs;
	register NODE *pf;
	int done_opt = false;
	int shift_count =  0;
#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("imull");

#ifdef FLIRT
	++numexec;
#endif

	for (ALLN(pf)) { 		/* for all instruction nodes... */
	if (shift_count) {
		instr = pf;
		mkshll(shift_count,dest);
		shift_count = 0;
	}
	if (pf->op == IMULL			/* multiply instruction */
	&&  isnumlit(pf->op1)			/* with a constant */
	&&  pf->op1[1] != '-'			/* that is positive */
/*	&&  (!instr->op3 || isreg(instr->op2))	/* and uses only registers */
	   ) {
		COND_SKIP(continue,"%d ",second_idx,0,0);
		done_opt = true;
		FLiRT(O_IMULL,numexec);
		instr = pf;
		multiplier = (int)strtoul(instr->op1+1,(char **)NULL,0);
		pnew = pmov = ppush = pmov0 = NULL;

		if (instr->op3) {
			dest = instr->op3;
			src = instr->op2;
			if (!isreg(instr->op2)) {
				pmov0 = prepend_move();
				instr->op2 = dest;
			}
			sameregs = !strcmp(src, dest);
		} else {
			dest = src = instr->op2;
			sameregs = true;
		}

retry:
		/* try to get a temporary register */
		if (strcmp(src, tmp = dest))
			/* EMPTY */
			;
		else
			gettmpreg();
		/* algorithm 1 */
		if (isalg1()) {

			mkleal(src, src, LEA_set[i], true, NULL);
			if (ppush != NULL) DELNODE(ppush);
			continue;
		}
			
		/* algorithm 2: already done by front end */

		/* algorithm 3 */
		if (isalg3()) {

			pnew = mkleal(src, src, LEA_set[i], true, dest);
			mkleal(dest, dest, LEA_set[j], true, NULL);

			if (ppush != NULL) DELNODE(ppush), ppush = NULL;
			exchange_instructions();
			continue;
		}
		
		/* algorithm 4 */
		if (isalg4()) {

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, src, LEA_set2[j], false, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 5 */
		if (isalg5()) {

			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, eff_LEA_set[j] - 1, false, src);
			mkshll(i, NULL);

			if (ppush != NULL) DELNODE(ppush), ppush = NULL;
			exchange_instructions();
			continue;
		}

		/* algorithm 6 */
		if (isalg6()) {

			pnew = mkleal(src, src, LEA_set[i], true, dest);
			mkleal(dest, dest, LEA_set[j], true, dest);
			mkleal(dest, dest, LEA_set[k], true, NULL);

			if (ppush != NULL) DELNODE(ppush), ppush = NULL;
			exchange_instructions();
			continue;
		}
		
		/* algorithm 7 */
		--multiplier;
		if (isalg7()) {

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, tmp, LEA_set[j], true, tmp);
			mkaddl(tmp == dest ? src : tmp, NULL);

			exchange_instructions();
			continue;
		}
		++multiplier;
		
		/* algorithm 8 */
		if (isalg8()) {

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, tmp, LEA_set[j], true, tmp);
			mkleal(src, tmp, LEA_set[k], false, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 9 */
		if (isalg9()) {

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, src, LEA_set2[j], false, dest);
			mkleal(dest, dest, LEA_set[k], true, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 10 */
		if (isalg10()) {

			if (tmp == dest) gettmpreg();
			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(src, src, LEA_set[j], true, dest);
			mkleal(dest, tmp, LEA_set2[k], false, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 11 */
		if (isalg11()) {

			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[j], true, src);
			mkshll(i, src);
			mkleal(src, src, LEA_set[k], true, NULL);

			if (ppush != NULL) DELNODE(ppush), ppush = NULL;
			exchange_instructions();
			continue;
		}
		
		/* algorithm 12 */
		if (isalg12()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkmovl(src, tmp);
			mkshll(i, src);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 13 */
		if (isalg13()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mkaddl(tmp, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 14 */
		if (isalg14()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 15 */
		if (isalg15()) {

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, tmp);
			mkaddl(tmp == dest ? src : tmp, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 16 */
		if (isalg16()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mkleal(src, tmp, LEA_set2[k], false, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 17 */
		if (isalg17()) {

			pnew = mkleal(src,  src,  LEA_set[i], true, dest);
			mkleal(dest, dest, LEA_set[j], true, dest);
			mkleal(dest, dest, LEA_set[k], true, dest);
			mkleal(dest, dest, LEA_set[l], true, NULL);

			if (ppush != NULL) DELNODE(ppush), ppush = NULL;
			exchange_instructions();
			continue;
		}
		
		/* algorithm 18 */
		if (isalg18()) {

			if (tmp == dest) gettmpreg();
			pnew = mkmovl(src, tmp);
			mkleal(src, src, LEA_set[i], true, dest);
			mkleal(dest, dest, LEA_set[j], true, dest);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 19 */
		if (isalg19()) {

			if (tmp == dest) gettmpreg();
			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(src, src, LEA_set[j], true, dest);
			mkleal(dest, dest, LEA_set[k], true, dest);
			mkaddl(tmp, NULL);

			exchange_instructions();
			continue;
		}
		
		/* algorithm 20 */
		if (isalg20()) {

			if (tmp == dest) gettmpreg();
			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(src, src, LEA_set[j], true, dest);
			mkaddl(tmp, dest);
			mkleal(dest, dest, LEA_set[k], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 21 */
		if (isalg21()) {

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, tmp, LEA_set[j], true, tmp);
			mkaddl(tmp == dest ? src : tmp, dest);
			mkleal(dest, dest, LEA_set[k], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 22 */
		if (isalg22()) {

			if (tmp == dest) gettmpreg();
			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, tmp, LEA_set[j], true, tmp);
			mkleal(src, src, LEA_set[k], true, dest);
			mkleal(dest, tmp, LEA_set2[l], false, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 23 */
		if (isalg23()) {

			if (tmp == dest) gettmpreg();
			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(src, src, LEA_set[j], true, dest);
			mkleal(dest, tmp, LEA_set2[k], false, dest);
			mkleal(dest, dest, LEA_set[l], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 24 */
		if (isalg24()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mksubl(src, tmp);
			mkshll(j, src);
			mkaddl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 25 */
		if (isalg25()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkaddl(src, tmp);
			mkshll(j, src);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 26 */
		if (isalg26()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mksubl(src, tmp);
			mkshll(j, src);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 27 */
		if (isalg27()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkmovl(src, tmp);
			mkshll(i, src);
			mkaddl(tmp, src);
			mkleal(src, src, LEA_set[j], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 28 */
		if (isalg28()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkmovl(src, tmp);
			mkshll(i, src);
			mksubl(tmp, src);
			mkleal(src, src, LEA_set[j], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 29 */
		if (isalg29()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, tmp, LEA_set[j], true, tmp);
			mkshll(k, src);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 30 */
		if (isalg30()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, src, LEA_set2[j], false, tmp);
			mkshll(k, src);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 31 */
		if (isalg31()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mkleal(src, src, LEA_set[k], true, src);
			mkaddl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 32 */
		if (isalg32()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mkleal(src, src, LEA_set[k], true, src);
			mksubl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 33 */
		if (isalg33()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mksubl(tmp, src);
			mkleal(src, src, LEA_set[k], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 34 */
		if (isalg34()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, tmp);
			mkaddl(tmp, src);
			mkleal(src, src, LEA_set[k], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 35 */
		if (isalg35()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, tmp, LEA_set[j], true, tmp);
			mkshll(k, src);
			mkaddl(tmp, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 36 */
		if (isalg36()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mkaddl(tmp, src);
			mkleal(src, src, LEA_set[k], true, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 37 */
		if (isalg37()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkleal(tmp, tmp, LEA_set[j], true, tmp);
			mkshll(k, src);
			mkleal(src, tmp, LEA_set2[l], false, NULL);

			exchange_instructions();
			continue;
		}

		/* algorithm 38 */
		if (isalg38()) {

			if (tmp == dest) gettmpreg();
			if (!sameregs)
				pmov = prepend_move();

			pnew = mkleal(src, src, LEA_set[i], true, tmp);
			mkshll(j, src);
			mkleal(src, tmp, LEA_set2[k], false, src);
			mkleal(src, src, LEA_set[l], true, NULL);

			exchange_instructions();
			continue;
		}

		/* no replacement found */
		if (ppush != NULL) DELNODE(ppush);
		shift_count = 0;
		while (! (multiplier & 1)) {
				multiplier >>= 1;
				shift_count++;
		}
		if (shift_count)
			goto retry;
	} /* if instruction == imull */
	} /* for ALLN(pf) */
	if 	(done_opt) sets_and_uses();
}


#define SHLLSIZE (1+1+10+1)		/* for "$[-]2147483647\0" */

static char LEA_str[] = "(%s,%s,%d)";
static char SHL_str[] = "$%d";

static NODE *
mkleal(base, index, scale, basechk, livereg)
char *base, *index;
int scale;	/* scale factor */
boolean basechk;
char *livereg;
{
	NODE *new;

	if (basechk)
		if (odd(scale))
			--scale;
		else
			base = "";

	if (livereg == NULL) {
		instr->op2 = dest;
		instr->op3 = NULL;
		new = instr;
	} else
		new = prepend(instr, livereg);

	if (*base == '\0' && !strcmp(index, new->op2)) {
		if (scale == 2) {
			chgop(new, ADDL, "addl");
			new->op1 = new->op2;
		}
		else {
			int num;

			chgop(new, SHLL, "shll");
			new->op1 = getspace(SHLLSIZE);
			num = scale == 4 ? 2 : 3;
			sprintf(new->op1, SHL_str, num);
		}
	}
	else {
		chgop(new, LEAL, "leal");
		new->op1 = getspace(LEALSIZE);
		sprintf(new->op1, LEA_str, base, index, scale);
	}
	return new;
}


static void
mkshll(num, livereg)
int num;
char *livereg;
{
	NODE *new;

	if (livereg == NULL) {
		instr->op2 = dest;
		instr->op3 = NULL;
		new = instr;
	} else
		new = prepend(instr, livereg);

	chgop(new, SHLL, "shll");
	new->op1 = getspace(SHLLSIZE);
	sprintf(new->op1, SHL_str, num);
}


static void
mkaddl(addop1, addop2)
char *addop1, *addop2;
{
	NODE *new;

	if (addop2 == NULL) {
		instr->op2 = dest;
		instr->op3 = NULL;
		new = instr;
	} else
		new = prepend(instr, addop2);

	chgop(new, ADDL, "addl");
	new->op1 = addop1;
}


static void
mksubl(subop1, subop2)
char *subop1, *subop2;
{
	NODE *new;

	if (subop2 == NULL) {
		instr->op2 = dest;
		instr->op3 = NULL;
		new = instr;
	} else
		new = prepend(instr, subop2);

	chgop(new, SUBL, "subl");
	new->op1 = subop1;
}


static void
mkpushl()
{
	ppush = Saveop(0,"",0,GHOST);/* create isolated node, init. */
	INSNODE(ppush, instr);

	chgop(ppush, PUSHL, "pushl");
	ppush->op1 = tmp;
	ppush->nlive = instr->nlive;
	ppush->ndead = instr->ndead;
}


static NODE *
mkmovl(movop1, movop2)
char *movop1, *movop2;
{
	NODE *new;

	new = prepend(instr, movop2);
	chgop(new, MOVL, "movl");
	new->op1 = movop1;
	return new;
}


NODE *					/* return pointer to the new node */
prepend(pn, livereg)
NODE * pn;				/* node to add pointer before */
char * livereg;				/* register in node to be set live */
{
	NODE * new = Saveop(0,"",0,GHOST);/* create isolated node, init. */

	INSNODE(new, pn);		/* append new node before current */

	new->op2 = livereg;
	new->nlive = pn->nlive;
	new->ndead = pn->ndead;
	makelive(livereg, new);

	return new;			/* return pointer to the new one */
}


static NODE *
prepend_move()
{
	NODE *new = prepend(instr, dest);

	chgop(new, MOVL, "movl");
	new->op1 = src;
	src = dest;
	return new;
}


static void
exchange_instructions()
{
	lexchin(instr, pmov0 != NULL ? pmov0 : ppush != NULL ? ppush : pmov != NULL ? pmov : pnew);
	if (ppush != NULL) {
		NODE *ppop;

		ppop = Saveop(0,"",0,GHOST);/* create isolated node, init. */
		APPNODE(ppop, instr);

		chgop(ppop, POPL, "popl");
		ppop->op1 = tmp;
		ppush->nlive = instr->nlive;
		ppush->ndead = instr->ndead;
	}
}
