#ifndef _WCHARM_H
#define _WCHARM_H
#ident	"@(#)localedef:_wcharm.h	1.1"
/*
* wcharm.h - internal wide and multibyte character declarations
*/

#include <wchar.h>
#define __wc /* no side effects in isw* or tow* invocations! */
#include <wctype.h>
#include "_stdlock.h"

				/* _E[1-21] in <wctype.h> */
#define	_E22	0x20000000
#define	_E23	0x40000000

#define XWCTYPE	0x80000000	/* no _E24...special bit */

#define	EUCMASK	0xf0000000
#define P00	0x00000000	/* code set 0 */
#define P11	0x30000000	/* code set 1 */
#define P01	0x10000000	/* code set 2 */
#define P10	0x20000000	/* code set 3 */

#define EUCDOWN	28
#define DOWNMSK	0xf	/* (EUCMASK >> EUCDOWN) without the warning */
#define DOWNP00	(P00 >> EUCDOWN)
#define DOWNP11	(P11 >> EUCDOWN)
#define DOWNP01	(P01 >> EUCDOWN)
#define DOWNP10	(P10 >> EUCDOWN)

#define SS2	0x8e	/* byte that prefixes code set 2 multibyte encoding */
#define SS3	0x8f	/* byte that prefixes code set 3 multibyte encoding */

#ifndef MB_LEN_MAX
#define MB_LEN_MAX	6	/* UTF8 worst case */
#endif

#if !defined(_ctype) && defined(__STDC__)
#define _ctype	__ctype
#endif

extern unsigned char	_ctype[];

#define eucw1	_ctype[514]	/* # bytes for code set 1 multibyte characters */
#define eucw2	_ctype[515]	/* # bytes for code set 2, not including the SS2 */
#define eucw3	_ctype[516]	/* # bytes for code set 3, not including the SS3 */

#define scrw1	_ctype[517]	/* printing width for code set 1 */
#define scrw2	_ctype[518]	/* printing width for code set 2 */
#define scrw3	_ctype[519]	/* printing width for code set 3 */

#define _mbyte	_ctype[520]	/* max(1, eucw1, 1+eucw2, 1+eucw3) */
#define multibyte (_mbyte > 1)	/* true if real multibyte characters present */
#define utf8	(_mbyte == 6)	/* true if using UTF8 mb/wc encoding */

#define MBENC_OLD	0	/* original extended LC_CTYPE */
#define MBENC_NONE	1	/* only when !multibyte; no extended info */
#define MBENC_EUC	2	/* updated LC_CTYPE with EUC mb/wc encoding */
#define MBENC_UTF8	3	/* updated LC_CTYPE with UTF8 mb/wc encoding */

#define CTYPE_VERSION	1	/* current lc_ctype_dir version value */

#define ISONEBYTE(wi)	((wi) <= 0x7f || !multibyte)

struct lc_ctype_dir	/* new style LC_CTYPE description */
{
	unsigned char	version;
	unsigned char	codeset;
	unsigned short	nstrtab;	/* length of strtab array */
	unsigned long	nwctype;	/* length of wctype array */
	unsigned long	ntypetab;	/* length of typetab array */
	unsigned long	wctype;		/* struct t_wctype[] base */
	unsigned long	typetab;	/* struct t_ctype[] base */
	unsigned long	strtab;		/* string table base */
};

struct t_ctype	/* defines an extended LC_CTYPE predicate */
{
	wctype_t	type;	/* type mask; high=value, low=bits */
	unsigned long	name;	/* index into strtab[] */
	unsigned long	npair;	/* length of wchar_t[2] array */
	unsigned long	pair;	/* wchar_t[2] base; class member list */
};

struct t_wctype	/* describes a range of code values */
{
	wchar_t		tmin;	/* min. code value for __iswctype() */
	wchar_t		tmax;	/* max. code value for __iswctype() */
	unsigned long	index;	/* uchar[] base; class type index */
	unsigned long	type;	/* wctype_t[] base; type mask */
	wchar_t		cmin;	/* min. code value for __trwctype() */
	wchar_t		cmax;	/* max. code value for __trwctype() */
	unsigned long	code;	/* wchar_t[] base; matching u/l-case char */
	unsigned long	dispw;	/* uchar[] base; display widths */
};

struct	_wctype	/* old-style extended LC_CTYPE info */
{
	wchar_t		tmin;	/* minimum code for wctype */
	wchar_t		tmax;	/* maximum code for wctype */
	unsigned char	*index;	/* class index */
	wctype_t	*type;	/* class type */
	wchar_t		cmin;	/* minimum code for conversion */
	wchar_t		cmax;	/* maximum code for conversion */
	wchar_t		*code;	/* conversion code */
};

struct lc_ctype /* the ready-to-use form of lc_ctype_dir */
{
	const unsigned char	*base;
	const unsigned char	*strtab;
	const struct t_ctype	*typetab;
	const struct t_wctype	*wctype;
#ifdef _REENTRANT
	StdLock			*lockptr;
#endif
	size_t			nstrtab;
	size_t			ntypetab;
	size_t			nwctype;
	unsigned char		encoding;
	unsigned char		version;
};

#ifdef __STDC__
extern size_t	_iwcstombs(char *, const wchar_t **, size_t);
extern size_t	_mbsize(const unsigned char *, size_t *);
extern size_t	_wssize(const wchar_t *, size_t, size_t);
extern size_t	_xmbstowcs(wchar_t *, const char **, size_t);
extern size_t	_xwcstombs(char *, const wchar_t **, size_t);
struct lc_ctype	*_lc_ctype(void);
const struct t_wctype *_t_wctype(struct lc_ctype *, wint_t);
#else
extern size_t	_iwcstombs(), _mbssize(), _wssize();
extern size_t	_xmbstowcs(), _xwcstombs();
struct lc_ctype	*_lc_ctype();
const struct t_wctype *_t_wctype();
#endif

#endif /*_WCHARM_H*/
