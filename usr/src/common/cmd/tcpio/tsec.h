/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)tcpio:tsec.h	1.7.4.2"

#ifndef _SEC_H
#define _SEC_H

/* this should be in ttoc.h, but is used in tcpio.c... */
#define	NO_OPS	5	    /* number of check overrides */

/*
**    object flags (used in sec_hdr)
*/

#define O_MLD	 0x00000001  /* MultiLevel Directory */
#define O_VLDIN	 0x00000002  /* This LID was valid-inactive on -o */
#define O_UINVLD 0x00000004  /* This UID was invalid on -o */
#define O_GINVLD 0x00000010  /* This GID was invalid on -o */
#define O_ACL	 0x00000020  /* The UID/GID being validated is for an ACL */


/*
**  per-file additional header
**  (comes right after the generic header)
*/

struct sec_hdr {		/* file description */
    ulong	sh_flags;	/* defined above */
    level_t	sh_level;	/* MAC info */
    short	sh_acl_num;     /* number of ACL entries */
    struct acl	*sh_acl_tbl;	/* see <enhsec.h> */
    int         sh_fpriv_num;   /* number of file privileges */
    priv_t      *sh_fpriv_tbl;  /* privilege descriptors */
};


/*
**  size in bytes of the some structures when written out to
**  the medium.  The formula is:
**  	  8 * #(int|long)
**  	+ 4 * #shorts
**  	+ total of all other constant length fields
**  Also, wherever there is a '+ 1', it is to leave room for a null
**  character at the end of the field.
**  Note that the var-length fields are not included.
**  (The mh_magic field of the med_hdr struct is written as 6 bytes.)
*/

#define	SECSZ	(3*8 + 1*4 + 1)	/* sec_hdr */
#define	ACLSZ	(2*8 + 1*4 + 1)  	/* acl */
#define MEDCONST	(1*6 + 4*8 + 1*4 )
#define	MEDSZ	(MEDCONST + SYS_NMLN)
#define PRVSZ   (8 + 1)         /* privilege descriptor */
#define PMAPSZ  (2*8)           /* priv_map structure */
/*
** xxx_CNT represents the number of sscanf items that will be matched
** if the sscanf to read a header is successful.  If sscanf returns a number
** that is not equal to this, an error occured (which indicates that this
** is not a valid header of the type assumed.
*/

#define SEC_CNT 4       /* security headers */
#define ACL_COUNT 3	/* acl struct */
#define	MED_CNT	6   	/* medium header */
#define PRV_CNT 1       /* privilege descriptor */
#define PMAP_CNT 2      /* priv_map struct (without names) */
/*
** med_hdr is the basic medium header.
** A med_hdr structure is written to the beginning of
** each volume of the archive.
*/

struct med_hdr {
    long	mh_magic;	/* TI/E magic number */
    time_t 	mh_date;	/* archive creation date */
    level_t	mh_lo_lvl;	/* medium level range: */
    level_t	mh_hi_lvl;	/* low <-> high */
    short	mh_volume;	/* volume number */
    int         mh_numsets;     /* number of privilege sets stored */
    char	mh_host[SYS_NMLN];	/* name of local host */
};

/*
 * MH_UNBND is stored in mh_lo_lvl and mh_hi_lvl 
 * to indicate that the archive range is unbounded,
 * when -X is not used.
 */
#define MH_UNBND        -1


/*
** archive_tree is the structure of the tree which stores the files
** which have been backed up on the archive (-o).
*/

#define NOT_COPIED	0
#define COPY_SUCCEED	1
#define COPY_FAIL	2

struct headers {	/* So we'll only need one pointer... */
    struct sec_hdr sec;
    struct gen_hdr gen;
};

struct archive_tree {
    char *name;				/* name of the FS object */
    char copied;			/* set to 1 if copied in or out */
    struct archive_tree *parent,	/* parent pointer */
			*child,		/* child pointer */
			*sibling;	/* sibling pointer */
    /* The following is only for -i, using patterns. */
    struct headers *hdrs;
};

/* 
** priv_map structure - used to store privilege descriptor-to name
**    information on backup.  On restore, stored and current mapping
**    is compared and a warning issued if differences are found.
**    When file privileges are displayed, the stored mapping is
**    used.
**
** There is one priv_map structure for each privilege set applicable to
** files.
*/
typedef struct priv_map {
  priv_t  pm_mask;  /* combined with offset to get priv. descriptor */
  uint    pm_count; /* number of privileges in set */
  char    pm_name[PRVNAMSIZ]; /* name of set */
  char    pm_privs[NPRIVS][PRVNAMSIZ]; /* array of names of privs */
} privmap_t;
  
/*
**  misc memory allocation definitions.
**
**
*/

#define	STR(old)    	(char *)malloc((size_t)strlen(old) +1)
#define	REC(type)	(struct type *)malloc((size_t)sizeof(struct type))
#define	TBL(type, num) (struct type *)calloc((size_t)(num), (size_t)sizeof(struct type))

#define	ERROR	-1   	    /* erroneous return-from-routine value */
#define	DONE	0   	    /* successful return-from-routine value */
#define	MAX_NUM_SZ	11  /* max number of digits of a base 10 number */

#endif	/* _SEC_H */
