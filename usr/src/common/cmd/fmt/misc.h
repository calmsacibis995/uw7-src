/*	copyright	"%c%"	*/

#ident	"@(#)fmt:misc.h	1.2.1.2"

#include <stdio.h>
#include <widec.h>
#include <ctype.h>
#include <wctype.h>
#include <pfmt.h>
#include <sys/euc.h>

#if !defined(LINE_MAX)
#define LINE_MAX	4096
#endif

#define TAB	L'\t'
#define SPACE	L' '
#define BACKSPC	L'\b'

extern eucwidth_t wi;

#define	eucw1	wi._eucw1
#define eucw2	wi._eucw2
#define eucw3	wi._eucw3
#define scrw1	wi._scrw1
#define scrw2	wi._scrw2
#define scrw3	wi._scrw3
#define mb	wi._multibyte 

#define IS_MB		(mb)

wchar_t _ctmp_;

#define iswlnbr(c)	((c) == L'\n')
#define iswblank(c)	((_ctmp_=(c)) == L' ' || _ctmp_ == L'\t')
#define iswendpunct(c)	((_ctmp_=(c)) == L'.' || _ctmp_ == L'?' || \
		_ctmp_ == L'!')
/* iswmb is really 'is supplementary character in a multibyte locale' */
#define iswmb(c)	(IS_MB && ((c) & EUCMASK))

#define scrwidth(c) (!iswprint(c) ? 0 : wcwidth(c))

/*
* The #defines below are an attempt to get some speed for eight bit locales.
* They make use of knowledge of how to build a wide character from
* an eight-bit character.
*/

#if defined(getwchar)
#undef getwchar
#endif

#define getwchar() ( IS_MB ? fgetwc(stdin) : (wchar_t)getc(stdin) )

#if defined(putwchar)
#undef putwchar
#endif

#define putwchar(c) ( IS_MB ? fputwc((c),stdout) : putc((char)(c),stdout) )

/*
* struct dont is used by dontlist.
* valid -- whether this string is still a potential match.
* str   -- the string to compare with.
* type  -- the type of results the string causes when matched.
*/

struct dont {
    char valid;
    wchar_t *str;
    unsigned char type;
};

/* Legal bits in the structure element, type, which is a set. */

#define JOIN	1
#define SPLIT	2

typedef unsigned char uchar;

void output(wchar_t outline[], uchar charw[], int *count, int *lastwd, int *nnl);
void mkindent(char indentstr[], int inwidth);
int addwidth(uchar charw[], int count);
