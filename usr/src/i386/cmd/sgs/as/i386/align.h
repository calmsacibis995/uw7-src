#ident	"@(#)nas:i386/align.h	1.2"
/*
* i386/align.h - i386 default alignments & sizes
*
* Only included from:
*	"common/dirs.c"
*	"common/sect.c"
*	"common/syms.c"
*/

#define BSS_COMM_ALIGN	4	/* default alignment for bss and common */
#define EXEC_SECT_ALIGN	1	/* min alignment for executable sections */
#define ALLO_SECT_ALIGN	1	/* min alignment for allocatable sections */

#define FLOAT_SIZE	4	/* .float object size */
#define FLOAT_ALIGN	1	/* .float alignment */
#define DOUBLE_SIZE	8	/* .double ... */
#define DOUBLE_ALIGN	1
#define EXT_SIZE	10	/* .ext ... */
#define EXT_ALIGN	1

#define ALIGN_IS_POW2		/* all alignments must be power of 2 */

#define validalign(a)	((a) != 0 && ((a) & ((a) - 1)) == 0)
