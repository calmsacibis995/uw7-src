#ident	"@(#)acomp:i386/mddefs.h	55.2.1.8"
/* i386/mddefs.h */


/* Machine-dependent ANSI C definitions. */

/* These definitions will eventually be in CG's macdefs.h */
#define C_CHSIGN		/* plain chars default to signed */
#define	C_SIGNED_RS		/* right shifts of ints are signed */

/* Produce debugging register number corresponding to internal
** register number.
*/
#define	DB_OUTREGNO(i)	(outreg[i])
#define DB_FRAMEPTR	(DB_OUTREGNO(FRAMEPTR))
#define DB_ARGPTR	(DB_OUTREGNO(ARGPTR))

/* Only want for-loop code tests at bottom. */
#define	FOR_LOOP_CODE	LL_BOT
#define WH_LOOP_CODE	LL_BOT

/* Enable #pragma pack; maximum value is 4 */
#define	PACK_PRAGMA	4

/* Enable section mapping */
#define SECTION_MAP_PRAGMA

#ifdef FIXED_FRAME

#define	FRAMEPTR	(fixed_frame() ? REG_ESP : REG_EBP)
#define	ARGPTR	(fixed_frame() ? REG_ESP : REG_EBP)

#else

#define	FRAMEPTR	REG_EBP
#define	ARGPTR		REG_EBP

#endif

/* Enable #pragma weak.  The two strings are for the 1 and 2
** identifier forms of the pragma.
*/
#define	WEAK_PRAGMA "\t.weak\t%1\n", "\t.weak\t%1\n\t.set\t%1,%2\n"

#define FAT_ACOMP
#define RA_DEFAULT RA_GLOBAL
#define GENERATE_LOOP_INFO
extern chars_signed;
