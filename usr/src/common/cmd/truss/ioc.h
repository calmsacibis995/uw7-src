/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/ioc.h	1.2.6.2"
#ident  "$Header$"

/*
 * Some of the ioctl codes we want to interpret are defined in header
 * files that may not be visible in truss's compilation environment,
 * so we duplicate them here.  (Sorry.)
 */

/*
 * 3BNET
 */
#define NIOC		('3'<<8)

#define NISETA          (NIOC|1)
#define NIGETA          (NIOC|2)
#define SUPBUF		(NIOC|3)
#define RDBUF		(NIOC|4)
#define NIERRNO		(NIOC|5)
#define STATGET		(NIOC|6)
#define NISTATUS	(NIOC|7)
#define NIPUMP		(NIOC|8)
#define NIRESET		(NIOC|9)
#define NISELGRP	(NIOC|10)
#define NISELECT	(NIOC|11)

/*
 * EMD
 */
#define EICODE		('E'<<8)
#define EI_RESET	(EICODE|1)
#define EI_LOAD		(EICODE|2)
#define EI_FCF		(EICODE|3)
#define EI_SYSGEN	(EICODE|4)
#define EI_SETID	(EICODE|5)
#define EI_TURNON	(EICODE|6)
#define EI_ALLOC	(EICODE|7)
#define EI_TERM		(EICODE|8)
#define EI_TURNOFF	(EICODE|9)
#define EI_SETA		(EICODE|10)
#define EI_GETA		(EICODE|11)
