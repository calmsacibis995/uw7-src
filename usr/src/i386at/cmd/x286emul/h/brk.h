/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)x286emul:h/brk.h	1.1"

/* commands for brkctl */
#define	BR_ARGSEG	0001	/* specified segment */
#define	BR_NEWSEG	0002	/* new segment */
#define	BR_IMPSEG	0003	/* implied segment - last data segment */
#define BR_FREESEG	0004	/* free the specified segment */
#define BR_HUGE		0100	/* do the specified command in huge context */
