/*		copyright	"%c%" 	*/

#ident	"@(#)tr:tr.h	1.1"

#include <stdio.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <stdarg.h>
#include <wctype.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#define NCHARS	10	/* Number of wide characters in a chrs struct. */
#define CH_OCTAL 1	/* Character specified using octal sequence */
#define MAXWC	INT_MAX	/* Maximum value a wide char can take.  */
/* #define DEBUG */	/* Define to compile in debugging options */

/* 
 * The various modes that tr can run in
 */
extern int mode;
#define TR_MAP		1	/* Two strings, no flags */
#define TR_SQUEEZE	2	/* 1 or 2 strings, -s specified */
#define TR_DELETE	4	/* 1 or 2 strings, -d specified */
#define TR_COMPLEMENT	8	/* first string complemented */


/*
 * A linked list of 'chs' structures holds the compiled strings passed as 
 * arguments to tr.
 */ 
typedef struct _chs chs;
struct _chs { 
	unsigned long nch;	/* 
				 * Number of characters represented by
				 * this structure.
				 */
	union {
		wint_t _chr;	/* Base for range/repeat */
		wint_t *_chrs;	/* Array of wide characters */
	} _ch_un;
#define chr _ch_un._chr
#define chrs _ch_un._chrs
	unsigned int flags;
#define CHS_CHARS	0	/* Struct represents chars in chrs[]      */
#define	CHS_RANGE	1	/* Struct represents chr-chr+nch          */
#define CHS_REPEAT	2	/* Struct represents nch chr's            */
#define CHS_UPPER	4	/* Placeholder for possible toupper class */
#define CHS_LOWER	8	/* Placeholder for possible tolower class */
#define CHS_ARRAY	16	/* chrs is an already allocated array -   */
				/* changed to CHS_CHARS in addtostring()  */
	chs *next;
	unsigned long i;	/* Index into struct used when reading list */
};

/* Unadvertised libc routines  */
extern size_t _wcl_class(wchar_t *, size_t , wctype_t );
extern size_t _wcl_equiv(wchar_t *, size_t , wchar_t );
extern size_t _wcl_range(wchar_t *, size_t , wchar_t , wchar_t );
