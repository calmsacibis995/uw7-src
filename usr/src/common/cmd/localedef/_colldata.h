#ident	"@(#)localedef:_colldata.h	1.1"

typedef struct
{
	long	coll_offst;	/* offset to xnd table */
	long	sub_cnt;	/* length of subnd table */
	long	sub_offst;	/* offset to subnd table */
	long	str_offst;	/* offset to strings for subnd table */
	long	flags;		/* nonzero if reg.exp. used */
} hd;

typedef struct
{
	unsigned char	ch;	/* character or number of followers */
	unsigned char	pwt;	/* primary weight */
	unsigned char	swt;	/* secondary weight */
	unsigned char	ns;	/* index of follower state list */
} xnd;

typedef struct
{
	char	*exp;	/* expression to be replaced */
	long	explen; /* length of expression */
	char	*repl;	/* replacement string */
} subnd;

/*----------------------------------*/

#include "_wcharm.h"
#include <limits.h>
#include "_stdlock.h"

/*
* Structure of a collation file:
*  1. CollHead (maintbl is 0 if CHF_ENCODED)
*   if !CHF_ENCODED then
*    2. CollElem[bytes] (256 for 8 bit bytes)
*    3. if CHF_INDEXED then
*	 CollElem[wides] (nmain-256 for 8 bit bytes)
*	else
*	 CollMult[wides]
*    4. CollMult[*] (none if multtbl is 0)
*    5. wuchar_t[*] (none if repltbl is 0)
*    6. CollSubn[*] (none if subntbl is 0)
*    7. strings (first is pathname for .so if CHF_DYNAMIC)
*
* The actual location of parts 2 through 7 is not important.
*
* The main table is in encoded value order.
*
* All indeces/offsets must be nonzero to be effective; zero is reserved
* to indicate no-such-entry.  This implies either that an unused initial
* entry is placed in each of (4) through (7), or that the "start offset"
* given by the header is artificially pushed back by an entry size.
*
* Note that if CHF_ENCODED is not set, then nweight must be positive.
*
* If an element can begin a multiple character element, it contains a
* nonzero multbeg which is the initial index into (4) for its list;
* the list is terminated by a CollMult with a ch of zero.
*
* If there are elements with the same primary weight (weight[1]), then
* for each such element, it must have a CollMult list.  The CollMult
* that terminates the list (ch==0) either notes that the basic weights
* (weight[0]) for the equivalence class members are interspersed (by
* a SUBN_SPECIAL mark), or holds the lowest and highest basic weights
* (respectively in weight[0] and weight[1]) when their basic weights
* are not interrupted by other collating elements.
*
* WGHT_IGNORE is used to denote ignored collating elements for a
* particular collation ordering pass.  All main table entries other
* than for '\0' will have a non-WGHT_IGNORE weight[0].  However, it is
* possible for a CollMult entries from (4) to have a WGHT_IGNORE
* weight[0]:  If, for example, "xyz" is a multiple character collating
* element, but "xy" is not, then the CollMult for "y" will have a
* WGHT_IGNORE weight[0].  Also, WGHT_IGNORE is used to terminate each
* list of replacement weights.
*
* Within (3), it is possible to describe a sequence of unremarkable
* collating elements with a single CollMult entry.  If the SUBN_SPECIAL
* bit is set, the rest of subnbeg represents the number of collating
* elements covered by this entry.  The weight[0] values are determined
* by adding the difference between the encoded value and the entry's ch
* value to the entry's weight[0].  This value is then substituted for
* any weight[n], n>0 that has only the WGHT_SPECIAL bit set.  _collelem()
* hides any match to such an entry by filling in a "spare" CollElem.
*
* If there are substitution strings, then for each character that begins
* a string, it has a nonzero subnbeg which is similarly the initial
* index into (6).  The indeces in (6) refer to offsets within (7).
*/

#define TOPBIT(t)	(((t)1) << (sizeof(t) * CHAR_BIT - 1))

#define CHF_ENCODED	0x1	/* collation by encoded values only */
#define CHF_INDEXED	0x2	/* main table indexed by encoded values */
#define CHF_MULTICH	0x4	/* a multiple char. coll. elem. exists */
#define CHF_DYNAMIC	0x8	/* shared object has collation functions */

#define CWF_BACKWARD	0x1	/* reversed ordering for this weight */
#define CWF_POSITION	0x2	/* weight takes position into account */

#define CLVERS		1	/* most recent version */

#define WGHT_IGNORE	0	/* ignore this collating element */
#define WGHT_SPECIAL	TOPBIT(wuchar_t)
#define SUBN_SPECIAL	TOPBIT(unsigned short)

typedef struct
{
	unsigned long	maintbl;	/* start of main table */
	unsigned long	multtbl;	/* start of multi-char table */
	unsigned long	repltbl;	/* start of replacement weights */
	unsigned long	subntbl;	/* start of substitutions */
	unsigned long	strstbl;	/* start of sub. strings */
	unsigned long	nmain;		/* # entries in main table */
	unsigned short	flags;		/* CHF_* bits */
	unsigned short	version;	/* handle future changes */
	unsigned char	elemsize;	/* # bytes/element (w/padding) */
	unsigned char	nweight;	/* # weights/element */
	unsigned char	order[COLL_WEIGHTS_MAX]; /* CWF_* bits/weight */
} CollHead;

typedef struct
{
	unsigned short	multbeg;	/* start of multi-chars */
	unsigned short	subnbeg;	/* start of substitutions */
	wuchar_t	weight[COLL_WEIGHTS_MAX];
} CollElem;

typedef struct
{
	wchar_t		ch;	/* "this" character (of sequence) */
	CollElem	elem;	/* its full information */
} CollMult;

typedef struct
{
	unsigned short	strbeg;		/* start of match string */
	unsigned short	length;		/* length of match string */
	unsigned short	repbeg;		/* start of replacement */
} CollSubn;

#ifdef __STDC__
#define PTRFCN(nm, par)	(*nm)par
#else
#define PTRFCN(nm, par)	(*nm)()
#endif

struct lc_collate
{
	const unsigned char	*strstbl;
	const wuchar_t		*repltbl;
	const CollElem		*maintbl;
	const CollMult		*multtbl;
	const CollSubn		*subntbl;
#ifdef DSHLIB
	void	*handle;
	void	PTRFCN(done, (struct lc_collate *));
	int	PTRFCN(strc, (struct lc_collate *,
				const char *, const char *));
	int	PTRFCN(wcsc, (struct lc_collate *,
				const wchar_t *, const wchar_t *));
	size_t	PTRFCN(strx, (struct lc_collate *,
				char *, const char *, size_t));
	size_t	PTRFCN(wcsx, (struct lc_collate *,
				wchar_t *, const wchar_t *, size_t));
#endif
	const char		*mapobj;
	size_t			mapsize;
	unsigned long		nmain;
	short			nuse;
	unsigned short		flags;
	unsigned char		elemsize;
	unsigned char		nweight;
	unsigned char		order[COLL_WEIGHTS_MAX];
};

#undef PTRFCN

#define ELEM_BADCHAR	((CollElem *)0)
#define ELEM_ENCODED	((CollElem *)-1)

#ifdef __STDC__
int	_old_collate(struct lc_collate *);
int	_strqcoll(struct lc_collate *, const char *, const char *);
int	_wcsqcoll(struct lc_collate *, const wchar_t *, const wchar_t *);
struct lc_collate *_lc_collate(struct lc_collate *);
const CollElem	*_collelem(struct lc_collate *, CollElem *, wchar_t);
const CollElem	*_collmult(struct lc_collate *, const CollElem *, wchar_t);
const CollElem	*_collmbs(struct lc_collate *, CollElem *, const unsigned char **);
const CollElem	*_collwcs(struct lc_collate *, CollElem *, const wchar_t **);
#else
int _old_collate(), _strqcoll(), _wcsqcoll();
struct lc_collate *_lc_collate();
const CollElem *_collelem(), *_collmult(), *_collmbs(), *_collwcs();
#endif
