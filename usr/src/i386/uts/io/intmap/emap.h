#ifndef _IO_INTMAP_EMAP_H	/* wrapper symbol for kernel use */
#define _IO_INTMAP_EMAP_H	/* subject to change without notice */

#ident	"@(#)emap.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/* Channel mapping ioctl's */
/*	LDIOC  ('D'<< 8) */	/* defined in termio.h */

							/* BEGIN SCO_INTL */
/* Channel Mapping ioctl command definitions */
#define LDSMAP (LDIOC|10)
#define LDGMAP (LDIOC|11)
#define LDNMAP (LDIOC|12)

/* Enhanced Application Compatibility Support */
/* The LDEMAP and LDDMAP IOCTLS conflict with 
*   SCO events LDEV_SETTYPE, LDEV_GETEV put in i386/Version4
*   The reference in ldterm.c and tty1.c for LDEMAP and LDDMAP is being
*   removed for 4.1dt
*/
#define LDEMAP (LDIOC|13)
#define LDDMAP (LDIOC|14)
/* End Enhanced Application Compatibility Support */

/* Emapping state (t_mstate) */
#define	ES_NULL		0	/* Mapping not enabled */
#define	ES_START	1	/* Base mapping state */
#define	ES_DEAD		2	/* Dead key received */
#define	ES_COMP1	3	/* Compose key received */
#define	ES_COMP2	4	/* Compose and 1st following keys received */

/* Enhanced Application Compatibility Support */
#define ES_DEC          5       /* found second digit after compose       */
#define ES_OFF          6       /* existing mapping disabled            */
/* End Enhanced Application Compatibility Support */

#define	EMBSHIFT	10	/* log2 BSIZE (E_TABSZ 1K) */
#define	EMBMASK		01777	/* E_TABSZ - 1 */

#define	NEMBUFS	10		/* Max number of buffers for mapping table */

typedef	struct emtab	*emp_t;
typedef	struct emind	*emip_t;
typedef	struct emout	*emop_t;
typedef	unsigned char	*emcp_t;

extern	emcp_t	emmapout();

/* Emap control structure */
struct emap {
	emp_t	e_tp[NEMBUFS];		/* table of ptrs to mapping tables */
	struct	buf *e_bp;		/* buf hdr for mapping tables */
	short	e_count;		/* use count */
	short	e_ndind;		/* number of dead indexes */
	short	e_ncind;		/* number of compose indexes */
	short	e_nsind;		/* number of string indexes */
};

extern struct emap emap[];		/* allocated in space.h */


/* Emapping tables structures */

struct emind {
	unsigned char	e_key;
	unsigned char	e_ind;
};

struct emout {
	unsigned char	e_key;
	unsigned char	e_out;
};

struct emtab {
	unsigned char	e_imap[256];	/* 8-bit  input map */
	unsigned char	e_omap[256];	/* 8-bit output map */
	unsigned char	e_comp;		/* compose key */
	unsigned char	e_beep;		/* beep on error flag */
/* Enhanced Application Compatibility Support */
#define	e_toggle	e_beep
/* End Enhanced Application Compatibility Support */
	short		e_cind;		/* offset of compose indexes */
	short		e_dctab;	/* offset of dead/compose table */
	short		e_sind;		/* offset of string indexes */
	short		e_stab;		/* offset of string table */
	struct emind	e_dind[1];	/* start of dead key indexes */
};

struct emp_tty {
	char	t_mstate;	/* emapping state */
	unsigned char	t_mchar;/*saved emapping char */
	char	t_merr;		/* emapping error flag */
	char	t_xstate;	/* extended state */
	struct xmap	*t_xmp;  /*ptr to extended struct */
	unsigned char	t_schar;/* save timeout char instead of using lflag */
	char	t_yyy[3];	/* reserved */
};

#define	E_TABSZ		1024		/* size of an emtab */

#define	ESTRUCTOFF(structure, field)	(int) &(((struct structure *)0)->field)
#define	E_DIND		(ESTRUCTOFF(emtab, e_dind[0]))
#define	E_ESC		'\0'		/* key maps to dead/compose/string */

#define bigetl(bp,cp)	(*(long *)((paddr(bp))+cp))

							/* END SCO_INTL */
#if defined(__cplusplus)
	}
#endif

#endif /* _IO_INTMAP_EMAP_H */
