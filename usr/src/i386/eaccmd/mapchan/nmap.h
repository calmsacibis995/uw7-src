/*		copyright	"%c%" 	*/

#ident	"@(#)nmap.h	1.2"
#ident  "$Header$"
/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1988.
 *	Copyright (C) Microsoft Corporation, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/* Channel mapping ioctl's */

#define	NMIOC  ('n'<< 8)	/* So not to conflict with LDIOCs */
#define NMSMAP (NMIOC|1)	/* set no-map */
#define NMGMAP (NMIOC|2)	/* get no-map */
#define NMNMAP (NMIOC|3)	/* clear no-map */

#ifdef M_I386
typedef	struct nmtab	*nmp_t;
typedef	struct nmseq	*nmsp_t;
typedef	unsigned char	*nmcp_t;
#else
typedef	struct nmtab	far *nmp_t;
typedef	struct nmseq	far *nmsp_t;
typedef	unsigned char	far *nmcp_t;
#endif

/* Nmap control structure */

struct nmap {
	unsigned char	n_count;	/* Usage count of this map */
	nmp_t		n_p;		/* Ptr. to sequence table */
	struct buf	*n_bp;		/* Ptr. to table buffer header */
};


/* Nmap sequence table structs */

struct nmtab {
	unsigned char	n_iseqs;	/* Number of input sequences */
	unsigned char	n_aseqs;	/* Number of input + output sequences */
	short		n_seqidx[1];	/* Array of sequence offsets */
};

struct nmseq {
	unsigned char	n_nmcnt;	/* Sequence trailer length */
	char		n_nmseq[1];	/* Sequence lead-in(null terminated) */
};

extern struct nmap nmap[];		/* Allocated in space.h */
