#ifndef _IO_TSM_TSMDEF_H	/* wrapper symbol for kernel use */
#define _IO_TSM_TSMDEF_H	/* subject to change without notice */

#ident	"@(#)tsmdef.h	2.1"
#ident	"$Header$"

/*
 *	tsmdef.h, definitions exported by the TSMs for use by other
 *	modules. definitions local to the TSM should be put in the
 *	header file local to the TSM's directory.
 */

#define TSM_T_ETHER		0
#define TSM_T_TOKEN		1
#define TSM_T_FDDI		2
#define TSM_T_RXNET		3
#define TSM_T_PCN2		4
#define TSM_T_ISDN		5
#define TSM_T_ATM		6
#define TSM_T_MAX		7	/* number of different TSM types */

/*
 * these are used for indexing into a specific Frame-Type
 */
#define E8022_INTERNALID	0
#define E8023_INTERNALID	1
#define EII_INTERNALID		2
#define ESNAP_INTERNALID	3

#define TOKEN_INTERNALID	0
#define TOKENSNAP_INTERNALID	1

#define FDDI_8022_INTERNALID	0
#define FDDI_SNAP_INTERNALID	1

/*
 * these are the NOVELL assigned Frame IDs
 */
#define	E8023_ID		5
#define	EII_ID			2
#define	E8022_ID		3
#define	ESNAP_ID		10

#define	TOKEN_ID		4
#define	TOKENSNAP_ID		11

#define FDDI_8022_ID		20
#define FDDI_SNAP_ID		23

#define ETHERNETSTATSCOUNT	8
#define NUMBER_OF_PROMISCUOUS_COUNTERS  32
#define MAX_MULTICAST           32

#define LONGCOUNTER		0x00
#define LARGECOUNTER		0x01

/*
 * ECB_DataLength bit assignmets for pipeline and ecb aware adapters.
 */
#define HSM_HAS_BEEN_IN_GETRCB  0x80000000              /* bit 31 */
#define PSTK_WANTS_FRAME	0x40000000              /* bit 30 */
#define HSM_TOLD_TO_SKIP	0x007f0000              /* bits23 through 16 */
#define HSM_TOLD_TO_COPY	0x0000ffff              /* bits15 through 0 */

#endif	/* _IO_TSM_TSMDEF_H */
