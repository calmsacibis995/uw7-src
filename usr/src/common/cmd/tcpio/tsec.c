/*		copyright	"%c%" 	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)tcpio:tsec.c	1.14.5.5"

#include    <sys/types.h>
#include    <sys/param.h>
#include    <sys/stat.h>
#include    <stdio.h>
#include    <string.h>
#include    <ctype.h>
#include    <fcntl.h>
#include    <pwd.h>
#include    <grp.h>
#include    <errno.h>
#include    <sys/mac.h>
#include    <sys/acl.h>
#include    <sys/secsys.h>
#include    <sys/utsname.h>
#include    <priv.h>
#include    <search.h>
#include    <archives.h>
#include    "tcpio.h"
#include    "tsec.h"
#include    "ttoc.h"

extern
int errno;

extern
struct buf_info	Buffr;

extern
struct gen_hdr	*G_p;
extern
struct passwd *Rpw_p;		/* Password entry for -R option */


extern
struct stat	ArchSt;		/* stat(2) information of the archive */

extern
int
    Bufsize,	/* Size of read buffer */
    Device,	/* Device type being accessed (used with libgenIO) */
    Archive,	/* File descriptor of the archive */
    privileged, /* Flag for privileged user */
    macpkg,	/* MAC security package toggle */
    aclpkg,	/* ACL security package toggle */
    Orig_umask, /* Umask of process when it started */
    aclmax;	/* Maximum number of ACL entries allowed on the system */

extern
struct acl *aclbufp;	/* Buffer to hold up to maximum number of ACL entries */

extern
long	Args;			    /* Mask of selected options */

extern
off_t	Pad_val;		    /* number of bytes to pad (if any) */


static
struct sec_hdr	Sec;
struct sec_hdr	*S_p = &Sec;

priv_t pvec[NPRIVS];

struct priv_map *privmaps;

static
struct med_hdr	Med,    /* medium header */
    	    	Vol;    /* next volume med_hdr */

struct med_hdr	*M_p = &Med,
    	    	*V_p = &Vol;

static
valid_info     Vtbl[IDs];	    /* array of validate structs */

int 	Unverify = 0;		    /* validity check(s) to override */



time_t 	Now; 	    	    	    /* time at start of execution */

static
long 	Save_stat,		    /* which ID segments will be saved  */
 	Save_data;		    /* Of the above, the IDs data will  */
				    /* be saved (in addition to state)  */

level_t	Lo_lvl,	    	    	    /* target (FS/archive) range: */
	Hi_lvl,			    /*   lo <-> hi    */
    	Min_lvl,    	    	    /* range (-X option): */
    	Max_lvl,    	    	    /*   min <-> max  */
	Map_lvl;		    /* for -N option  */

char	Host[SYS_NMLN];

static
char	*Dbuf;	    	    	    /* buffer for DB processed data */

static
id_t 	*ID_Tbl;    	    	    /* make-unique table */

static
off_t 	Db_sz = 0;  	    	    /* size of Dbuf see write_ttoc() */
    	    	    	    	    /* and process_db() */
int	nsets = 0;
ulong	objtyp = PS_FILE_OTYPE;
setdef_t *sdefs = (setdef_t *)0;

/* carry-over from cpio: buffer management etc. */
extern
void
	bflush(),
#ifdef __STDC__
	msg(int, ...),
#else
	msg(),
#endif
	rstbuf(),
	*malloc(),
	*calloc(),
	*realloc(),
	free(),
	exit(),
	qsort();

extern
int
	bfill(),
	lvldom(),
	lvlvalid(),
	lseek(),
	lvlproc(),
	g_read(),
	g_write(),
	read(),
	close(),
	lvlin(),
	acl(),
	mkmld();

int     get_privmap(),
        read_privmap();

void    write_privmap();

static
struct id_info Db_log[] = {
    {UID, UI_DB, UI_LOG, NULL},  /* user login information */
    {GID, GI_DB, GI_LOG, NULL},  /* user group information */
    {LID, LI_DB, LI_LOG, NULL},  /* level information */
};


level_t orig_lvl;	/* original level of the process */
static int no_lvlp = 0; /* set to 1 if lvl_proc() fails due to lack of
			   privilege - so don't try it anymore */

struct archive_tree
	*Arch_root, 	/* the root of the archive tree */
	*cwd_ptr;	/* the CWD pointer into the archive tree */

extern
char 	*align(),		/* function to get aligned memory */
	*privname(),		/* function to look up priv names */
	*Fullnam_p,		/* Full pathname */
	*IOfil_p,		/* -I/-O input/output archive string */
        *Tfil_p,                /* -T (TTOCTT) file string */
	**Pat_pp;		/* Pattern strings */




/*
** in_range:	Check if a given level is between lo and hi.
**
**  Returns 1 if in range, 0 if not.
*/

int
in_range(lvl, lo, hi)
level_t lvl, lo, hi;
{
    if (lvldom(&hi, &lvl) < 1) /* too high */
	return (0);
    if (lvldom(&lvl, &lo) < 1) /* too low */
	return (0);
    return (1);	    	    /* in between */
} /* in_range */


/*
 * Procedure:     curr_info
 *
 * Restrictions:
                 getpwuid: None
                 getgrgid: None

** notes:	Return the current name (string) associated with an ID.
**
**  Returns the pointer to the string,
**  NULL if error.  For LID - no "new" name is possible.
**
**    replacing the get*id() calls with a mechanism similar to
**  	the one used on the TTOC side will be a BIG performance win.
*/

static
char *
curr_info(vsp, type)
VS vsp;
enum TS type;
{
    struct passwd *utmp;
    struct group *gtmp;

    switch ((int)type) {
    case UI:
	if (utmp = getpwuid(vsp->vs_value))
	    return(utmp->pw_name);
	else
	    return(NULL);

    case GI:
	if (gtmp = getgrgid(vsp->vs_value))
	    return(gtmp->gr_name);
	else
	    return(NULL);

    case LI:
    default:
	msg(EXT, ":921", "Impossible case: %s", "curr_info()");
    }

    return (NULL);  /* to apease 'lint' */
} /* curr_info */


/*
** skip_file:	Skip a file (or part of) on the archive.
**
**  Flush the contents of the internal buffer before
**  lseek()-ing the medium.
**
**  NOTE - no gracefull treatment of EOM/BOM conditions.
*/

void
skip_file(cnt, mag)
off_t cnt;
int mag;	/* 1 if skipping a file and should end up at the next mag # */
		/* 0 if not (e.g., skipping a DB in the meduim header) */
{
    register off_t skip;

    skip = cnt;
    if (skip == 0)   /* no work */
	goto newln;

    /* If all the data is in the buffer, */
    /* just adjust the pointers, no need to seek */
    if (skip < Buffr.b_cnt) {
	Buffr.b_out_p += skip;
	Buffr.b_cnt -= skip;
	goto newln;
    }

    /* more data than the buffer holds, reset pointers, adjust 'skip' */
    Buffr.b_in_p = Buffr.b_out_p = Buffr.b_base_p;
    skip -= Buffr.b_cnt;
    Buffr.b_cnt = 0L;

    if ((ArchSt.st_mode & S_IFMT) == S_IFREG) {
	    /* don't seek if you don't have to */
	    if (skip > 0 && lseek(Archive, skip, SEEK_CUR) == ERROR)
		msg(EXTN, ":61", "lseek() failed");
	    FILL(CPIOBSZ);  /* fill to get past possible new-line (see below) */
    } else {
	    while (skip >= Bufsize) {
		/*
		 * Loop around doing reads BUT not processing the data as
		 * lseek() does not work for lots of devices, and the
		 * seek will take us off a block boundary.
		 */
		int rv;
		if ((rv = g_read(Device, Archive, Buffr.b_in_p, Bufsize)) < 0)
		    msg(EXT, ":124", "Unexpected end-of-archive encountered.");
		else
		    skip = skip - (off_t)rv;
	    }
	    bfill();
	    /*
	     * Increment ptr and decrement count for the part of the file that
	     * we are attempting to skip that is in the buffer.
	     */
	    Buffr.b_out_p += skip;
	    Buffr.b_cnt -= skip;
    }

newln:
    /* Get past new-line and position at the beginning
     * of the next magic number.  
     */
    if (mag)
	while (strncmp(Buffr.b_out_p, CMS_SEC, CMS_LEN)) {
	    Buffr.b_out_p++;
	    Buffr.b_cnt--;
        } /* while */

} /* skip_file */


/*
** read_shdr:	Read a security header.
**
**  Called from gethdr() (-i option).
**
**  In case there is any error, the number of bytes to skip
**  is returned.  Else, the number of bytes read is returned.
**  To distinguish between the two cases, the first number is
**  negeted.
**
**  NOTE -  if detected a bad header, there is no reason to
**  try resyncronizing - read_hdr left us in the right spot.
**  Try the next best thing - skipping this file.
**  There is no way to tell how much garbage is left in the IO buffer,
**  so hope that the next gethdr() call will find the magic number...
*/

off_t
read_shdr(sp, gp)
struct sec_hdr *sp;
struct gen_hdr *gp;
{
    int err = 0;
    register struct buf_info *buf = &Buffr;
    register char *bip;
    register struct acl *ap;
    register int cnt, prv_cnt;
    register off_t tot;
    priv_t *pp;

    FILL(SECSZ);
    bip = buf->b_out_p;
    /* Read the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
     if (sscanf(bip, "%8lx%8lx%4hx%8lx", &sp->sh_flags, &sp->sh_level, 
		&sp->sh_acl_num, &sp->sh_fpriv_num) != SEC_CNT) {
	err++;
	msg(ERR, ":1008", "Corrupt security header for \"%s\", skipping", gp->g_nam_p);
    }

	
    /* sanity check: level can't be invalid */
    if (!(Unverify & NO_LID) && sp->sh_level == LVL_INVALID) {
	if (!err) {
	    err++;
	    msg(ERR, ":935", "File \"%s\" has no LID, skipping", gp->g_nam_p);
	}
    }

    buf->b_out_p += SECSZ;
    buf->b_cnt -= SECSZ;
    tot = SECSZ;

    if (err)
 	return (-(gp->g_filesz + (sp->sh_acl_num * ACLSZ)
 		  + (sp->sh_fpriv_num * PRVSZ)));

    /* now read the ACLs of this file */
    cnt = sp->sh_acl_num;
    if (cnt > aclmax) {
	msg(ERR, ":925", "Too many ACLs for file \"%s\", skipping", gp->g_nam_p);
 	/* skip file plus all ACLS and fileprivs */
 	return (-(gp->g_filesz + (sp->sh_acl_num * ACLSZ)
 		  + (sp->sh_fpriv_num * PRVSZ)));
    }
    sp->sh_acl_tbl = aclbufp;
    for (ap = sp->sh_acl_tbl; cnt > 0; ap++, cnt--) {
	FILL(ACLSZ); /* NOTE: reading the whole chain? (if fits in buffer) */
	    	     /* will be faster */

	/* Read the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
	if (sscanf(buf->b_out_p, "%8lx%8lx%4hx",
		   &ap->a_type, &ap->a_id, &ap->a_perm) != ACL_COUNT) {
	    msg(ERR, ":925", "Too many ACLs for file \"%s\", skipping", gp->g_nam_p);
	    return (-(off_t)(gp->g_filesz + (cnt * ACLSZ)
 			     + (sp->sh_fpriv_num * PRVSZ)));
	}
	buf->b_out_p += ACLSZ;
	buf->b_cnt -= ACLSZ;
	tot += ACLSZ;
    }
     /* now read the file privileges (if any) of this file */
     prv_cnt = sp->sh_fpriv_num;
     /* if filepriv() returned error during backup, display warning restore */
     if (prv_cnt == ERROR)
       msg(WARN, ":909", "Cannot retrieve file privileges for \"%s\"",gp->g_nam_p);
     /* if too many privileges, something is wrong - skip file and privs. */
     if (prv_cnt > NPRIVS) {
 	msg(ERR, ":937", "Too many privileges for file \"%s\", skipping", gp->g_nam_p);
 	/* skip file plus all fileprivs */
 	return (-(gp->g_filesz + (sp->sh_fpriv_num * PRVSZ)));
     }
      for (pp = &pvec[0];prv_cnt > 0; pp++, prv_cnt--) {
 	FILL(PRVSZ);
 	/* Read the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
  	if (sscanf(buf->b_out_p, "%8lx", pp) != PRV_CNT) {
 	  msg(ERR, ":938", "Misformed privileges for \"%s\", skipping.", gp->g_nam_p);
 	  return (-(off_t)(gp->g_filesz + (prv_cnt * PRVSZ))); 
 	}
 	buf->b_out_p += PRVSZ;
 	buf->b_cnt -= PRVSZ;
 	tot += PRVSZ;
 	}

    return (tot);
} /* read_shdr */


/*
** write_shdr:	Write the security header to the medium.
**
**  Called from write_hdr() (-o option).
**
*/

int
write_shdr(sp)
struct sec_hdr *sp;
{
    register struct buf_info *buf = &Buffr;
    register int acl_num, priv_num;
    register struct acl *ap;
    register priv_t *pp;

    FLUSH(SECSZ);

    /* Write the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
     (void)sprintf(buf->b_in_p, "%.8lx%.8lx%.4x%.8lx", sp->sh_flags, 
 		  sp->sh_level, sp->sh_acl_num, sp->sh_fpriv_num);
    buf->b_in_p += SECSZ;
    buf->b_cnt += SECSZ;
    for (ap = sp->sh_acl_tbl, acl_num = sp->sh_acl_num;
	 acl_num > 0; ap++, acl_num--) {
	FLUSH(ACLSZ);

	/* Write the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
	sprintf(buf->b_in_p, "%.8lx%.8lx%.4x",
		ap->a_type, ap->a_id, ap->a_perm);
	buf->b_in_p += ACLSZ;
	buf->b_cnt += ACLSZ;
    }
     /* Now write file privileges  */
     for (pp = &pvec[0], priv_num = sp->sh_fpriv_num;
  	 priv_num > 0; pp++, priv_num--) {
       FLUSH(PRVSZ);
       
       /* Write the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
       sprintf(buf->b_in_p, "%.8lx", *pp);
       buf->b_in_p += PRVSZ;
       buf->b_cnt += PRVSZ;
     }
     return (SECSZ + (sp->sh_acl_num * ACLSZ) + (sp->sh_fpriv_num * PRVSZ));

} /* write_shdr */


/*
** fill_mhdr:	Fill a medium header with the relevant info.
**
**  Called from out_sec_setup() (-o option) to prepare a header to write to 
**  the meduim.
**  Note: for a privileged user, the archive range
**  reflects the range of the device it is created on.
**  For other users, there is no "range" and both level
**  are set to the level of the running process.
*/

static
void
fill_mhdr(mp)
struct med_hdr *mp;
{
    mp->mh_magic = CMN_SEC;
    mp->mh_date = Now;	    /* time stamp (when tcpio started) */

    if (Args & OCX) {	/* -X option used to specify archive range */
	/* level range is the device range */
	mp->mh_lo_lvl = Lo_lvl = Min_lvl;
	mp->mh_hi_lvl = Hi_lvl = Max_lvl;
    } else {	/* -X not used, default action: unbounded range
	         * so the range values are set to MH_UNBND.
		 */
	mp->mh_lo_lvl = Lo_lvl = MH_UNBND;
	mp->mh_hi_lvl = Hi_lvl = MH_UNBND;
    }

    mp->mh_volume = 1;	/* this is the first volume */
    memcpy(mp->mh_host, Host, SYS_NMLN);
} /*fill_mhdr */


/*
 * Procedure:     read_mhdr
 *
 * Restrictions:
                 g_read: None
*/


/*
** notes:	Read a medium header from the archive.
**
**  Called in restore mode (-i option) from in_sec_setup() and also from 
**  chgreel() for multi-volume media.
**
**  When called from in_sec_setup() with the fill argument set, the medium
**  header may be buffered as any other data.
**
**  When called from chgreel(), however (fill argument not set), we don't want
**  to put the medium header in the main buffer with the rest of the data, but
**  rather, process it immediately.  Therefore, we read directly from the new
**  volume here.  But, since we must read Bufsize bytes at a time (otherwise,
**  unpredictable results may occur with raw devices), we allocate a local
**  buffer whose size is the smallest multiple of Bufsize greater than the
**  medium header size, and fill in this buffer by reading Bufsize bytes at a
**  time into it.  After validating the medium header, if there is any "real"
**  data in the local buffer after the medium header, we transfer it to the
**  main buffer.
*/

int
read_mhdr(mp, fill)
struct med_hdr *mp;
int fill;  /* 1 if should fill normally through the I/O buffer, 
	      0 if being called from chgreel()
	      so only want exact medium header */
{
    register struct buf_info *buf = &Buffr;
    char holdch;
    static char *mhdr_buf = NULL;
    register char *bip;
    register off_t pad;
    int cnt, ret, leftovers;
    static int bufcnt, medsz;
    char nowstr[BUFSIZ], archtimestr[BUFSIZ];

    /* calculate length of pad for medium header */
    pad = (Pad_val + 1 - (MEDSZ & Pad_val)) & Pad_val;

    if (fill) {
        FILL(MEDSZ);

        bip = buf->b_out_p;
        if (buf->b_end_p != (bip + MEDSZ)) {
	    holdch = *(bip + MEDSZ);
	    *(bip + MEDSZ) = '\0';
        }
    } else {
	if (mhdr_buf == NULL) {
	    medsz = MEDSZ + pad;
	    bufcnt = medsz / Bufsize + ((medsz % Bufsize) ? 1 : 0);
	    if ((mhdr_buf = align(Bufsize * bufcnt)) == (char *) -1)
	        msg(EXT, ":643", "Out of memory: %s", "read_mhdr()");
        }
	bip = mhdr_buf;

	for (cnt = 0; cnt < medsz; cnt += ret) {
	    if ((ret = g_read(Device, Archive, mhdr_buf + cnt, Bufsize)) <= 0) {
		msg(ERRN, ":939", "Read failure--check medium and try again");
		return(ERROR);
	    }
        }
    }


    /* Read the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
    if (sscanf(bip, "%6lx%8lx%8lx%8lx%4hx%8lx",
	       &mp->mh_magic,
	       &mp->mh_date,
	       &mp->mh_lo_lvl,
	       &mp->mh_hi_lvl,
	       &mp->mh_volume,
	       &mp->mh_numsets) != MED_CNT) {
	msg(ERR, ":940", "Incomplete/misformed medium header");
	return (ERROR);
    }

    /*   sanity checks:	    	    	    */
    /* magic number = secure magic  	    */
    /* date now is AFTER archive creation   */
    /* levels are VALID	    	    	    */
    /* volume number is 1 or more   	    */

    /* An archive dated in the future could reasonably happen if the archive
     * was created on one system and read on another system, where the two
     * systems' clocks were off.
     *
     * Also, the "bad magic" error is possible if an attempt is made to read a
     * non-tcpio-archive file.
     *
     * So for thees, we print more specific error messages.
     *
     * The other errors should really never happen, so we won't clutter
     * up the message file with a specific message for each one.
     */

    if (mp->mh_magic != CMN_SEC) {
	msg(ERR, ":110", "Bad magic number/header.");
	return(ERROR);
    }
    if (fill && mp->mh_date > Now) {
	(void)cftime(nowstr, "%c", &Now);
	(void)cftime(archtimestr, "%c", &mp->mh_date);
	msg(WARN, ":1004", "Time of archive creation, %s, is later than current time, %s", archtimestr, nowstr);
	/* Note:  There used to be a "return (ERROR)" here, which would cause
	 * tcpio to exit.  But this caused too much trouble when moving
	 * archives between machines whose clocks were out of sync, so it was
	 * changed to just a warning.
	 */
    }
    if ((macpkg && (mp->mh_lo_lvl == LVL_INVALID ||
	mp->mh_hi_lvl == LVL_INVALID)) ||
	mp->mh_volume < 1) {
	msg(ERR, ":941", "Bad value in medium header");
	return (ERROR);
    }
    if (fill && mp->mh_volume != 1) {
	msg(ERR, ":952", "Wrong volume, expected %d, got %d", 1, mp->mh_volume);
	return (ERROR);
    }
    bip += MEDCONST;

    /* 
     * if -P specified, just print level range and exit.
     */
    if (Args & OCP) {
	if (mp->mh_lo_lvl == MH_UNBND)
	    msg(EPOST, ":942", "Archive level range unbounded.");
	else
	    msg(EPOST, ":943", "Archive level range is (%d, %d), inclusive.", 
	        mp->mh_lo_lvl, mp->mh_hi_lvl);
	exit(0);
    }

    memcpy(mp->mh_host, bip, SYS_NMLN);

    if (fill) {
        if (buf->b_end_p != (buf->b_out_p + MEDSZ)) {
	    *(buf->b_out_p + MEDSZ) = holdch;
        }

        buf->b_out_p += MEDSZ;
        buf->b_cnt -= MEDSZ;

        buf->b_out_p += pad;
        buf->b_cnt -= pad;
    } else {
	leftovers = cnt - medsz;
	if (comp_mhdr(mp, M_p) == ERROR)
	    /* error message printed in comp_mhdr */
	    return(ERROR);


	bip += SYS_NMLN + pad;
	memcpy(buf->b_in_p, bip, leftovers);
	buf->b_in_p += leftovers;
	buf->b_cnt += leftovers;
    }

    if (macpkg) {
	    /* set the global archive-level-range */
	    Lo_lvl = mp->mh_lo_lvl;
	    Hi_lvl = mp->mh_hi_lvl;
    }

    return (DONE);
} /* read_mhdr */


/*
 * Procedure:     write_mhdr
 *
 * Restrictions:
                 g_write: None
*/


/*
** write_mhdr:	Write a medium header to the archive.
**
**  Called in backup mode (-o option) from out_sec_setup() and also from
**  chgreel() for multi-volume media.
**
**  When called from out_sec_setup() with the flush argument set, the medium
**  header may be buffered as any other data.
**
**  When called from chgreel(), however (flush argument not set), we must write
**  the medium header to the new volume *before* any of the buffered data
**  (since the medium header must be at the beginning of the volume).  But,
**  since we must write Bufsize bytes at a time (otherwise, unpredictable
**  results may occur with raw devices), we allocate a local buffer whose size
**  is the smallest multiple of Bufsize greater than the medium header size,
**  put the medium header in that buffer, and, if there's any space left over,
**  take some data from the main tcpio buffer and put it in the local buffer.
**  We then write the local buffer Bufsize bytes at a time.
*/

int
write_mhdr(mp, flush)
struct med_hdr *mp;
int flush;
{
    register struct buf_info *buf = &Buffr;
    static char *mhdr_buf = NULL;
    register char *bop;
    register off_t pad;
    int cnt, ret;
    static int medsz, bufcnt, leftovers;

    /* calculate length of pad for medium header */
    pad = (Pad_val + 1 - (MEDSZ & Pad_val)) & Pad_val;

    if (flush) {
        FLUSH(MEDSZ);
        bop = buf->b_in_p;
    } else {
	if (mhdr_buf == NULL) {
	    medsz = MEDSZ + pad;
	    bufcnt = medsz / Bufsize + ((medsz % Bufsize) ? 1 : 0);
	    if ((mhdr_buf = align(Bufsize * bufcnt)) == (char *) -1)
	        msg(EXT, ":643", "Out of memory: %s", "write_mhdr()");
	    leftovers = (Bufsize * bufcnt) - medsz;
        }
	bop = mhdr_buf;
    }

    /* Write the data using the MEDIUM STRUCTURE (ASCII) FORMAT */
    (void)sprintf(bop, "%.6lx%.8lx%.8lx%.8lx%.4x%.8lx",
		  mp->mh_magic,
		  mp->mh_date,
		  mp->mh_lo_lvl,
		  mp->mh_hi_lvl,
		  mp->mh_volume,
		  mp->mh_numsets);

    bop += MEDCONST;
    memcpy(bop, mp->mh_host, SYS_NMLN);

    bop += SYS_NMLN;

    if (flush) {
        buf->b_in_p += MEDSZ;
        buf->b_cnt += MEDSZ;

	FLUSH(pad);
	(void)memset(buf->b_in_p, 0, pad);
	buf->b_in_p += pad;
	buf->b_cnt += pad;
    } else {
	(void)memset(bop, 0, pad);
	bop += pad;
	memcpy(bop, buf->b_out_p, leftovers);

	for (cnt = 0; cnt < medsz; cnt += ret) {
	    if ((ret = g_write(Device, Archive, mhdr_buf + cnt, Bufsize)) <= 0) {
		msg(ERRN, ":944", "Write failure--check medium and try again");
		return(ERROR);
	    }
        }
	buf->b_out_p += (cnt - medsz);
	buf->b_cnt -= (cnt - medsz);
    }
    return(DONE);
} /* write_mhdr */





/*
** MAKE_SPC:	macro for the on-demand allocation of space for the
**  ID-mapping info.
**  Note that Db_sz is increased by 50% of its current size
**  each time more space is needed.
*/

#define	MAKE_SPC()	    if (Db_sz < tot + len) {                 \
		    	    	Db_sz += Db_sz/2;                    \
    	    			Dbuf = (char *)realloc(Dbuf, Db_sz); \
		    	    	dp = Dbuf + tot;                     \
	    	    	    }

/*
 * Procedure:     count_lid
 *
 * Restrictions:
                 open(2): None
                 read(2): None
*/

/*
** notes:	Count the number of valid (active/inactive) LIDs.
**
**  Called from fill_vhdr() when filling the validate structure
**  so the TTOC can be written to the archive on output (-o option).
**
**  This number is the upper limit on the possible different
**  LIDs any file in the archive might have.
**  This information is used to allocate the correct table size
**  when restoring.
*/

static
int
count_lid(db)
char *db;
{

#define LVL_STRUCT_SIZE         sizeof(struct mac_level)

    register int fd;
    register id_t nact;
    register off_t ndel;
    struct mac_level buf;
    struct mac_level *lvlp = &buf;   /* buffer for reading levels */
    int cnt;

#define	LID_OFF	(4*CAT_SIZE + 2)    /* offest of active/inactive indicator */

    nact = ndel = 0L;

    if ((fd = open(db, O_RDONLY)) == ERROR) {
	msg(EXT, ":945", "Cannot open DB \"%s\".", db);
    }

    /* Seek past the zeroth entry, reserved */
    if (lseek(fd, LVL_STRUCT_SIZE, SEEK_SET) == ERROR)
	msg(EXT, ":946", "Cannot seek DB \"%s\".", db);

    for (cnt = read(fd, lvlp, LVL_STRUCT_SIZE);
         cnt == LVL_STRUCT_SIZE;
         cnt = read(fd, lvlp, LVL_STRUCT_SIZE)) {

	switch (lvlp->lvl_valid) {

	case LVL_INVALID:

	    break;

	case LVL_ACTIVE:

	    nact++;
	    break;

	case LVL_INACTIVE:

	    ndel++;
	    break;

	default:
	    msg(EXT, ":921", "Impossible case: %s", "count_lid()");
	}
    }
    (void) close(fd);

    return (nact + ndel);	/* conservative "estimate" */
} /* count_lid */


/*
 * Procedure:     process_db
 *
 * Restrictions:
                 setpwent: None
                 getpwent: None
                 endpwent: None
                 setgrent: None
                 getgrent: None
                 endgrent: None
*/


/*
** notes:	Collect ID mapping info into temporary memory buffer,
**  discarding non-relevant info and normalizing the format.
**
**  Called from write_ttoc() (-o option) so the appropriate DBs (UID and GID)
**  can be written to the archive.
*/

static
void
process_db(vp, type)
VP vp;
enum TS type;
{
    register off_t len;
    register off_t tot;
    register id_t nelm;
    register char *dp;
    struct id_info *idp;

    tot = nelm = len = 0L;
    dp = Dbuf;	    	    /* global buffer */
    idp = ID_REC(type);
    switch((int)type) {

    case UI: {
	    struct passwd *pw;

	    setpwent();
	    while ((pw = getpwent()) != (struct passwd *)NULL) {
		/* estimate the space needed:   */
		/*   11 - max char size of int  */
		/* + 1  (for ':')  	    	*/
		/* + 1  (for '\n') 	    	*/
		/* + length of the name string  */
		len = 13 + (off_t)strlen(pw->pw_name);
		MAKE_SPC();
		sprintf(dp, "%d:%s\n\0", pw->pw_uid, pw->pw_name);
		len = (off_t)strlen(dp);	/* adjust to "real" space consumed */
		dp += len;
		tot += len;
		nelm++;
	    }
	    endpwent();
	}
	vp->v_med_size = tot;
	vp->v_nelm = nelm;
	break;

    case GI: {
	    struct group *gr;

	    setgrent();
	    while ((gr = getgrent()) != (struct group *)NULL) {
		len = 13 + (off_t)strlen(gr->gr_name);
		MAKE_SPC();
		sprintf(dp, "%d:%s\n\0", gr->gr_gid, gr->gr_name);
		len = (off_t)strlen(dp);
		dp += len;
		tot += len;
		nelm++;
	    }
	    endgrent();
	}
	vp->v_med_size = tot;
	vp->v_nelm = nelm;
	break;

    case LI:	/* Don't write LID DB to the archive */
	break;

    default:
	vp->v_med_size = vp->v_nelm = -1L;
	msg(EXT, ":921", "Impossible case: %s", "process_db()");
	break;
    }

} /* process_db */


/*
** neql:    Compare two IDs.
**
**  Used in (l)search/find calls.
*/

static
int
neql(x, y)
id_t *x, *y;
{
    if (*x == *y)
	return (0);
    return (1);
} /* neql */


/*
** gt:    Compare two IDs.
**
**  Used in (b)search and (q)sort calls;
*/


int
gt(x, y)
vs *x, *y;
{
    if (x->vs_value < y->vs_value)
	return (ERROR);
    if (x->vs_value == y->vs_value)
	return (0);
    return (1);
} /* gt */


/*
** read_db:	Read a database (part of the TTOC) off the medium into memory.
**
**  Called from read_ttoc() (-i option) for UI and GI (db not saved for LI).
*/

static
VS
read_db(vp)
VP vp;
{
    register struct buf_info *buf = &Buffr;
    off_t red;
    id_t val, *v = &val;
    char string[LVL_SZ];
    register char *bp = string;
    register VS vsp, Vp;
    register id_t nelm, cnt;
    id_t prev, n, *nelp = &n;
    register off_t tot, to_read;


    nelm = vp->v_nelm;
    if ((Vp = TBL(val_str, nelm)) == NULL) {
	msg(EXT, ":643", "Out of memory: %s", "read_db()");
    }
    if (ID_Tbl == NULL) {   /* called the first time */
	/* allocate the max size it might be (if no ID is repeated) */
	if ((ID_Tbl = (id_t *)calloc(nelm, sizeof(id_t))) == NULL)
	    msg(EXT, ":643", "Out of memory: %s", "read_db()");
    } else {
	/* adjust the size of the array.  NOTE: may shrink or grow */
	if ((ID_Tbl = (id_t *)realloc(ID_Tbl, nelm * sizeof(id_t))) == NULL)
	    msg(EXT, ":643", "Out of memory: %s", "read_db()");
    }

    /* read each entry, filling the input buffer whan needed */
    for (vsp = Vp, cnt = tot = n = 0; cnt < nelm; cnt++) {
	if (buf->b_cnt < VS_MAX_SZ) {	/* will it all fit? */
	    to_read = vp->v_med_size - tot;
	    if (to_read > CPIOBSZ) {
		FILL(CPIOBSZ - VS_MAX_SZ); /* do not overrun what's left */
	    } else {
		FILL(to_read);  /* this is the end... */
	    }
	}
	if (sscanf(buf->b_out_p, "%d:%s\n%n", v, bp, &red) != VS_CNT) {
	    msg(EXT, ":947", "Misformed DB file.");
	}

	buf->b_out_p += red;
	buf->b_cnt -= red;
	tot += red;

	/* keep only one mapping pair in memory */
	prev = n;   /* how many elements we had? */
	/* next call will update 'n' */
	(void)lsearch((char *)v, (char *)ID_Tbl, (size_t *)nelp, sizeof(id_t), (int (*)()) neql);
	if (prev == n)	/* item was not added - so it is already there */
	    continue;

	/* complete new item information */
	vsp->vs_value = val;
	vsp->vs_current = val;   /* Initially, the current value is the same
				    as the orig.  This is changed on remap */
	if ((vsp->vs_name = (char *)malloc((size_t)strlen(bp) +1)) == (char *)NULL)
	    msg(EXT, ":643", "Out of memory: %s", "read_db()");
	(void)strcpy(vsp->vs_name, bp);
	vsp++;	/* point to next empty slot */
    }

    vp->v_nelm = n; 	/* number of actual elements in table */

    /* must sort to prepare for bsearch action */
    qsort(Vp, n, sizeof(vs), gt);

    return (Vp);
} /* read_db */


/*
** write_db:	Write a db file to the medium from memory.
**
**  Called from write_ttoc() (-o option) for UI and GI (db not saved for LI).
**
*/

static
void
write_db(data, size)
char *data;
off_t size;
{
    register struct buf_info *buf;
    register char *dp;
    register off_t cnt;

    /* write the information to the global buffer variable (Buffr) */
    for (dp = data, buf = &Buffr, cnt = 0;
	 size > 0;
	 dp += cnt, buf->b_in_p += cnt,	buf->b_cnt += cnt , size -= cnt) {
	cnt = (size > CPIOBSZ) ? CPIOBSZ : size;
	FLUSH(cnt);
	memcpy(buf->b_in_p, dp, cnt);
    }
} /* write_db */

/*
 * Procedure:     check_db
 *
 * Restrictions:
                 stat(2): None
*/

/*
** notes:	Initial check of the DB/LOG state.
**
**  If any "override" option was used, we can
**  eliminate some of the work later.
**
**  Called from read_ttoc() (-i option).
**
**  SIDE EFFECTS:
**  	The 'v_flags' field of the VP struct passed as a parameter
**  	is set to reflect the state of the tested DB/LOG:
**	    V_OK     - no change, no need to validate any IDs.
**	    V_CHANGE - there was some change, validate each ID
**  	    	       when it is encounterd.
*/

static
void
check_db(vp)
VP vp;
{
    register struct id_info *id;
    struct stat dbuf;

    /*
     * First check if any override options were used.
     * This indicates that no further checking is needed.
     */
    switch ((int)vp->v_ts) {

    case UI:
	if (Unverify & NO_UID) {   	/* -n option */
	    vp->v_flags |= V_OK;
	}
	if (Args & OCR) {   	/* -R option */
	    vp->v_flags |= V_REMAP | V_OK;
	    vp->v_remap = Rpw_p->pw_uid;
	}
	break;

    case GI:
	if (Unverify & NO_GID) {   	/* -n option */
	    vp->v_flags |= V_OK;
	}
	if (Args & OCR) {   	/* -R option */
	    vp->v_flags |= V_REMAP | V_OK;
	    vp->v_remap = Rpw_p->pw_gid;
	}
	break;

    case LI:
	if (Unverify & NO_LID) {   	/* -n option */
	    vp->v_flags |= V_OK;
	}
	if (Args & OCN) {   	/* -N option */
	    vp->v_flags |= V_REMAP;
	    vp->v_remap = Map_lvl;
	    if (!(Args & OCX))
		vp->v_flags |= V_OK;
	}
	break;
    default:
	msg(EXT, ":921", "Impossible case: %s", "check_db()");
    }

    if (!privileged)
	vp->v_flags |= V_OK;

    if (vp->v_flags & V_OK)		/* no need to REALLY check */
	return;

    if (!(vp->v_flags & V_DB)) {	/* no status info for database*/
	goto do_log;
    }

   /*
    * Database was saved -- check its status.  
    * If the database is unmodified, no checking is necessary.
    */
    id = ID_REC(vp->v_ts);

    if (stat(id->it_db, &dbuf) == ERROR) {
	msg(EPOST, ":948", "the DB for Type Specifier \"%s\" cannot be accessed.", id->it_ts);
	goto do_log;
    }

    if (vp->v_db_mdate == dbuf.st_mtime &&
	vp->v_db_cdate == dbuf.st_ctime &&
	vp->v_db_size == dbuf.st_size) {
	vp->v_flags |= V_OK;
	return;
    }

 /*
  * See if log (history) file stat info was saved (for LIDs only).
  * If it was check its status.  If the log file is
  * unmodified, no checking is necessary (the database has not changed).
  */
 do_log:
    if (!(vp->v_flags & V_LOG)) {	/* can't verify - assume change */
	vp->v_flags |= V_CHANGE;
	return;
    }

    if (stat(id->it_log, &dbuf) == ERROR) { /* can't access - no verification */
	vp->v_flags |= V_CHANGE;
	return;
    }
    if (vp->v_log_mdate == dbuf.st_mtime &&
	vp->v_log_cdate == dbuf.st_ctime &&
	vp->v_log_size == dbuf.st_size) {
	vp->v_flags |= V_OK;
	return;
    }

    vp->v_flags |= V_CHANGE;
} /* check_db */


/*
 * Procedure:     fill_vhdr
 *
 * Restrictions:
                 stat(2): None
*/


/*
** notes:	Fill a validate structure with the appropriate information.
**
**  Called from write_ttoc() (-o option).
*/

static
void
fill_vhdr(vp, type)
VP vp;
enum TS type;
{
    register struct id_info *dlp;
    struct stat stb;

    /* reset fields value */
    vp->v_flags = 0L;
    vp->v_ts = type;
    vp->v_med_size = 0;

    dlp = ID_REC(type);		/* Get db/logfile struct assoc w/ type */
    if (*dlp->it_db == NULL) {
	vp->v_db_mdate = vp->v_db_cdate = vp->v_db_size = ERROR;
	goto log;
    }

    if (stat(dlp->it_db, &stb) == ERROR) {
	msg(EPOST, ":34", "Cannot access \"%s\"", dlp->it_db);
	vp->v_db_mdate = vp->v_db_cdate = vp->v_db_size = ERROR;
    } else {
	vp->v_flags |= V_DB;		/* Transfer info from stat of db */
	vp->v_db_mdate = stb.st_mtime;  /* into the validate structure.  */
	vp->v_db_cdate = stb.st_ctime;
	vp->v_db_size = stb.st_size;
    }

 log:
    if (*dlp->it_log == NULL) {
	vp->v_log_mdate = vp->v_log_cdate = vp->v_log_size = ERROR;
	return;
    }

    if (stat(dlp->it_log, &stb) == ERROR) {
	if (vp->v_db_size == ERROR) {
	    /* A non administrative user will not be able to stat the level
	     * history file, since it is at SYS_PRIVATE.  No sense in worrying
	     * the user by complaining about.  Therefore, we don't print an
	     * error message unless we were also unable to stat the DB.
	     */
	    msg(EPOST, ":34", "Cannot access \"%s\"", dlp->it_log);
	    msg(EXT, ":949", "Must be able to access the DB or the LOG file.");
	} 
	vp->v_log_mdate = vp->v_log_cdate = vp->v_log_size = ERROR;
    } else {
	vp->v_flags |= V_LOG;		/* Transfer info from stat of db */
	vp->v_log_mdate = stb.st_mtime; /* into the validate structure.  */
	vp->v_log_cdate = stb.st_ctime;
	vp->v_log_size = stb.st_size;
    }

    if (type == LI)
	vp->v_nelm = count_lid(dlp->it_db);
    else
	vp->v_nelm = 0;
} /* fill_vhdr */


/*
** ts_indx:	Find the TS of the DB and LOG files via the
**  		the Db_log[] table.
**
**  Called by read_ttoc() and read_ttoctt() (-i option).
**  Returns the indx of str's entry.
*/

static
enum TS
ts_indx(str)
char *str;
{
    register int indx;

    for (indx = 0; indx < IDs; indx++) {
	if (!strcmp(str, TS_NAME(indx)))
	    return ((enum TS)indx);
    }
    return (TS_ERR);
} /* ts_indx */


/*
** read_ttoc:	Read the TTOC from the medium into memory.
**
**  Called by in_sec_setup() (-i option).
**  Scans the medium reading each validate struct and calling check_db()
**  before deciding if this DB/LOG should be read or skipped.
**
**  The tables are kept sorted in memory - to increase the search speed.
**  However, the LID table is different.  Both because there is no
**  name/is mapping saved and because the hash-table search library
**  function will be used to search/insert items in it.
**  We can only have ONE hash-table per process, or we would have
**  used hsearch() for all IDs.
*/

static
void
read_ttoc()
{
    char holdch;
    register struct buf_info *buf = &Buffr;
    register char *bip;
    register VP vp;
    register int indx = 0;
    char tsname[TS_SZ];
    off_t pad;

    tsname[TS_SZ-1] = '\0';
    do {			/* Do for all db's (UID, GID, LID) */

	if (indx >= IDs)
	    msg(EXT, ":950", "Corrupt tcpio header.");
	vp = &Vtbl[indx++];
	vp->v_nelm = 0L;
	vp->v_map = (VS)NULL;

	FILL(VALSZ);
	bip = buf->b_out_p;

        if (buf->b_end_p != (bip + VALSZ)) {
	    holdch = *(bip + VALSZ);
	    *(bip + VALSZ) = '\0';
	}

	/*
	 * Read in the validate structure for the database.
	 */
	if (sscanf(bip, "%8lx%2s%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx",
			      &vp->v_flags,
			      tsname,
			      &vp->v_db_mdate,
			      &vp->v_db_cdate,
			      &vp->v_db_size,
			      &vp->v_log_mdate,
			      &vp->v_log_cdate,
			      &vp->v_log_size,
			      &vp->v_med_size,
			      &vp->v_nelm) != VAL_CNT) {
	    msg(EXT, ":950", "Corrupt tcpio header.");
	}

    	/* sanity checks */

    	/* sanity checks */
	if (vp->v_db_mdate > Now ||
	    vp->v_db_mdate < -1 ||
	    vp->v_db_cdate > Now ||
	    vp->v_db_cdate < -1 ||
	    vp->v_db_size < -1 ||
	    vp->v_log_mdate < -1 ||
	    vp->v_log_mdate > Now ||
	    vp->v_log_cdate > Now ||
	    vp->v_log_cdate < -1 ||
	    vp->v_log_size < -1 ||
	    vp->v_med_size < -1 ||
	    vp->v_nelm < -1) {
	    msg(EXT, ":950", "Corrupt tcpio header.");
	}

	if ((vp->v_ts = ts_indx(tsname)) == TS_ERR)
	    msg(EXT, ":950", "Corrupt tcpio header.");

    	if (buf->b_end_p != (bip + VALSZ)) {
	    *(bip + VALSZ) = holdch;
	}

	buf->b_out_p += VALSZ;
	buf->b_cnt -= VALSZ;

	check_db(vp);	/* The value returned in vp->v_flags         */
			/* tells whether we need to read this db in. */

	switch(vp->v_flags & V_TESTED) {

	case V_OK:
	    if (vp->v_flags & V_DATA) {
		/* we can skip the data, unless the -T option */
		/* is used, then we might need the mapping later! */
		if (Args & OCT)
		    vp->v_map = read_db(vp);
		else
		    skip_file(vp->v_med_size, 0);
	    }
	    break;

	case V_CHANGE:
	    if (vp->v_flags & V_DATA) {
		vp->v_map = read_db(vp);
	    } else {
		if (vp->v_ts != LI) {
		    msg(EPOST, ":951", "DB changed, but no data saved");
		}
	    }
	    break;

	default:
	    msg(EXT, ":950", "Corrupt tcpio header.");
	}

	if (macpkg && (vp->v_ts == LI)) {/* Prepare hash table for the LID db */
	    if (hcreate(vp->v_nelm) == 0) {
		msg(EXT, ":643", "Out of memory: %s", "read_ttoc()");
	    }
	    vp->v_flags |= V_DATA;
	    break;
	}

	pad = (Pad_val + 1 - ((VALSZ + vp->v_med_size) & Pad_val)) & Pad_val;
	Buffr.b_out_p += pad;
	Buffr.b_cnt -= pad;
    } while (!(vp->v_flags & V_LAST));

} /* read_ttoc */


/*
** write_ttoc:	Write all the validate structs, then write the data.
**
**  Called by out_sec_setup() (-o option).
*/

static
void
write_ttoc()
{
    register struct buf_info *buf = &Buffr;
    register char *bop;
    register VP vp;
    register int indx;
    register long sset, dset;
    off_t pad;

    if ((vp = REC(validate)) == (VP)NULL) {
	msg(EXT, ":643", "Out of memory: %s", "write_ttoc()");
    }

    for (sset = Save_stat, dset = Save_data, indx = 0;
	 0 != sset; sset >>= 1, dset >>= 1, indx++) {
	if (!(sset & 0x1))
	    continue; /* can check if !dset, but that is paranoid... */

	fill_vhdr(vp, (enum TS)indx);   /* Fill the validate structure. */
	if  (dset & 0x1) {		/* dset (Save_data) specifies */
	    vp->v_flags |= V_DATA;	/* which db's are to be saved.*/
	    if (Db_sz == 0) {
		Db_sz = vp->v_db_size/3;
		Dbuf = (char *)malloc(Db_sz);
	    }
	    process_db(vp, (enum TS)indx); /* Write db in Dbuf */
	}

	if (!(sset >>1))			/* last ID type saved */
	    vp->v_flags |= V_LAST;

	FLUSH(VALSZ);
	bop = buf->b_in_p;
	(void)sprintf(bop, "%.8lx%*s%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx",
		      vp->v_flags,
		      TS_SZ -1,
		      TS_NAME(vp->v_ts),	/* Write the validate    */
		      vp->v_db_mdate,		/* struct to the medium. */
		      vp->v_db_cdate,
		      vp->v_db_size,
		      vp->v_log_mdate,
		      vp->v_log_cdate,
		      vp->v_log_size,
		      vp->v_med_size,
		      vp->v_nelm);


	buf->b_in_p += VALSZ;
	buf->b_cnt += VALSZ;
	write_db(Dbuf, vp->v_med_size);	   /* Write the db to the medium. */

	pad = (Pad_val + 1 - ((VALSZ + vp->v_med_size) & Pad_val)) & Pad_val;
	if (pad != 0) {
	    FLUSH(pad);
	    (void)memset(buf->b_in_p, 0, pad);
	    buf->b_in_p += pad;
	    buf->b_cnt += pad;
	}
    }

} /* write_ttoc */


/*
** comp_mhdr:	Compare two med_hdr's and decide if they belong
**  	    	to the same archive.
**
**  Called from chgreel() (-i option) when reading from the archive 
**  and the end of medium is reached.
*/

int
comp_mhdr(hp_got, hp_expect)
struct med_hdr *hp_got, *hp_expect;
{
    if (strcmp(hp_got->mh_host, hp_expect->mh_host)) {
        msg(ERR, ":954", "Medium header of volume %d: wrong medium, host should be %s, is %s", hp_got->mh_volume, hp_expect->mh_host, hp_got->mh_host);

	return (ERROR);
    }
    if (hp_got->mh_volume != hp_expect->mh_volume) { /* vol# already adjusted */
	msg(ERR, ":952", "Wrong volume, expected %d, got %d",
	    hp_expect->mh_volume, hp_got->mh_volume);
	return (ERROR);
    }
    if (hp_got->mh_magic != hp_expect->mh_magic) {
        msg(ERR, ":953", "Medium header of volume %d does not match previous medium header.", hp_got->mh_volume);
	return (ERROR);
    }
    if (hp_got->mh_date != hp_expect->mh_date) {
        msg(ERR, ":953", "Medium header of volume %d does not match previous medium header.", hp_got->mh_volume);
	return (ERROR);
    }
    if (hp_got->mh_lo_lvl != hp_expect->mh_lo_lvl) {
        msg(ERR, ":953", "Medium header of volume %d does not match previous medium header.", hp_got->mh_volume);
	return (ERROR);
    }
    if (hp_got->mh_hi_lvl != hp_expect->mh_hi_lvl) {
        msg(ERR, ":953", "Medium header of volume %d does not match previous medium header.", hp_got->mh_volume);
	return (ERROR);
    }
    
    return (DONE);
} /* comp_mhdr */


/*
** loc_ttoc:	Find the base of the ttoc of the given ID type.
**
**  Called from read_ttoctt() and in_ttoctt() (-i option) to
**  scan the global Vtbl[] until a match is found.
*/


VP
loc_ttoc(type)
enum TS type;
{
    register int i;

    for (i = 0; i < IDs; i++) {
	if (type == Vtbl[i].v_ts)
	    return (&Vtbl[i]);
    }
    return (NULL);
} /* loc_ttoc */


/*
** ssearch: String-match search.
**
**  Look through the TTOC segment and try to locate the record (val_str
**  structure) of ID with "name".  Returns the pointer to the record 
**  (if found), NULL in any other case.
**
**  Called from read_ttoctt() (-i option).
*/

static
VS
ssearch(name, tbl)
char * name;
VP tbl;
{
    register int indx;
    register VS vsp;

    if (tbl->v_map == NULL)  /* no data */
	return (NULL);

    for (indx = 0, vsp = tbl->v_map; indx < tbl->v_nelm; indx++, vsp++) {
	if (!strcmp(name, vsp->vs_name))
	    return (vsp);
    }
    return (NULL);
} /* ssearch */

/*
 * Procedure:     read_ttoctt
 *
 * Restrictions:
                 setpwent: None
                 setgrent: None
                 lvlin: None
                 getpwnam: None
                 getgrnam: None
                 getpwuid: None
                 getgrgid: None
                 lvlvalid: None
*/


/*
** read_ttoctt:	Read the TTOCTT, fix the TTOC as you go.
**
**  Called when the -T option is used with -i.  The TTOCTT resides
**  in a file (file pointer given as a argument).
**  Must validate each of the entries to make sure the
**  given IDs make sense.
*/

static
void
read_ttoctt(f)
FILE *f;
{
    enum TS type;
    register VS ovs;
    VP tent;
    char buf[CPIOBSZ];
    char *id_spec, *from_id, *to_id;	
    register id_t indx;
    id_t id;
    register int insert = 0;
    struct passwd *pw;
    struct group *gr;

    setpwent();
    setgrent();

    for (indx = 1L; ; indx++, insert = 0) {
	pw = NULL;
	gr = NULL;
	if (fgets(buf, CPIOBSZ, f) == NULL)	/* get a line from */ 
	    if (feof(f)) {			/* the TTOCTT file */
		break;
	    } else
		msg(EXT, ":955", "Read error in translation table at line #%d.",  indx);

	if ((id_spec = strtok(buf, " \t\n")) == NULL) {
		msg(WARN, ":956", "Invalid translation table entry on line #%d, skipping to next entry.", indx);
		continue;
	}

	if ((from_id = strtok(NULL, " \t\n")) == NULL) {
		msg(WARN, ":956", "Invalid translation table entry on line #%d, skipping to next entry.", indx);
		continue;
	}

	if ((to_id = strtok(NULL, " \t\n")) == NULL) {
		msg(WARN, ":956", "Invalid translation table entry on line #%d, skipping to next entry.", indx);
		continue;
	}

	if ((strtok(NULL, " \t\n")) != NULL) {
		/* too many fields */
		msg(WARN, ":956", "Invalid translation table entry on line #%d, skipping to next entry.", indx);
		continue;
	}


	if (((type = ts_indx(id_spec)) == TS_ERR) ||
	    ((type == LI) && (macpkg == 0))) {   /* get the TS indx */
	    msg(WARN, ":1009", "TTOCTT at line #%d: %s: no such ID type, skipping to next entry.", indx, id_spec);
	    continue;
	}

	tent = loc_ttoc(type);	/* Get the validate structure (TTOC entry) */
				/* for type. */


	if (isalpha(*from_id)) { /* first ID is alphabetical */

/* for type LI, search hash table; for other types, *
 * search TTOC table.                               *
 */
	    if (type == LI) {
		ENTRY item, *ep;      /* Search for LID in hash table. */

		if (lvlin(from_id, (level_t *)(&id)) == ERROR) {          
		    msg(WARN, ":958", "TTOCTT at line #%d: ID %s of type %s does not exist on archive, skipping to next entry.", 
			indx, from_id, TS_NAME(type));
		    continue;
		}						       
								       
		item.key = (char *)malloc((size_t)MAX_NUM_SZ);
		sprintf(item.key, "%d", id);
		if ((ep = hsearch(item, FIND)) == NULL) {	       
		    free(item.key);
		    insert = 1;
		} else {					       
		    free(item.key);

		    msg(WARN, ":961", "TTOCTT at line #%d: ID %s of type %s mapped twice, skipping to next entry.", indx, from_id, TS_NAME(type));
		    continue;					       
		}						       

	    } else { /* UI or GI */   
	        ovs = ssearch(from_id, tent);  /* Search TTOC entry for the ID */
    
	        if (ovs == NULL) { /* ID not found for name */
		    msg(WARN, ":958", "TTOCTT at line #%d: ID %s of type %s does not exist on archive, skipping to next entry.", indx, from_id, TS_NAME(type));
		    if ((ovs = REC(val_str)) == NULL) {
		        msg(EXT, ":643", "Out of memory: %s", "read_ttoctt()");
		    }
		    if ((ovs->vs_name = strdup(from_id)) == NULL)
		        msg(EXT, ":643", "Out of memory: %s", "read_ttoctt()");
		    ovs->vs_value = ERROR;
		    insert = 1;
	        } else {					
		    if (ovs->vs_state & VS_MAPPED) {		
			msg(WARN, ":961", "TTOCTT at line #%d: ID %s of type %s mapped twice, skipping to next entry.", indx, from_id, TS_NAME(type));
			continue;				
		    } else {					
		    	ovs->vs_state |= VS_MAPPED;		
		    }						
		}						

	    } /* else (UI or GI) */
	} else {  /* "from" identifier not alpha; should be numeric*/
	    if (!isdigit(*from_id)) {  /* first ID is a number */
		msg(WARN, ":962", "TTOCTT at line #%d: %s: field must be alphanumeric, skipping to next entry.",
		    indx, from_id);
		continue;
	    }

	    id = atoi(from_id);/* convert string to int */

	    if (type == LI) {	/* Search for LID in hash table. */
		ENTRY item, *ep;
		item.key = (char *)malloc((size_t)MAX_NUM_SZ);
		sprintf(item.key, "%d", id);
		if ((ep = hsearch(item, FIND)) == NULL) {
		    free(item.key);
		    insert = 1;
		} else {
		    free(item.key);

		    msg(WARN, ":961", "TTOCTT at line #%d: ID %s of type %s mapped twice, skipping to next entry.", indx, from_id, TS_NAME(type));
		    continue;					           
		}

	    } else {     /* Search for UID or GID in appropriate table. */
		vs item;

		item.vs_value = id;
		ovs = (VS)bsearch(&item, tent->v_map, tent->v_nelm, sizeof(vs), (int (*)()) gt);

		if (ovs == NULL) {
		    msg(WARN, ":958", "TTOCTT at line #%d: ID %s of type %s does not exist on archive, skipping to next entry.", indx, from_id, TS_NAME(type));
		    insert = 1;
	        } else {					
		    if (ovs->vs_state & VS_MAPPED) {		
			msg(WARN, ":961", "TTOCTT at line #%d: ID %s of type %s mapped twice, skipping to next entry.", indx, from_id, TS_NAME(type));
			continue;				
		    } else {					
		    	ovs->vs_state |= VS_MAPPED;		
		    }						
		}						

	    } /* else (UI or GI) */
	} /* else (not alpha) */

	if (insert) {
	    if ((ovs = REC(val_str)) == NULL) {
	        msg(EXT, ":643", "Out of memory: %s", "read_ttoctt()");
	    }
	    ovs->vs_value = id;
	    ovs->vs_name = NULL;
	    ovs->vs_state = VS_MAPPED;				
	}

	if (isalpha(*to_id)) {  /* second ID is alphabetical */
	    switch ((int)type) {

	    case UI:
		/* Verify user name is valid*/
		if ((pw = getpwnam(to_id)) == (struct passwd *)NULL) {
		    msg(WARN, ":964", "TTOCTT at line #%d: ID %s of type %s does not exist on system, skipping to next entry.", indx, to_id, TS_NAME(type));
		    continue;
		}

		break;

	    case GI:
		/* Verify group name is valid. */
		if ((gr = getgrnam(to_id)) == (struct group *)NULL) {
		    msg(WARN, ":964", "TTOCTT at line #%d: ID %s of type %s does not exist on system, skipping to next entry.", indx, to_id, TS_NAME(type));
		    continue;
		}

		break;

	    case LI:
		/* Verify level name is valid. */
		if (lvlin(to_id, (level_t *)(&id)) == ERROR) {
		    msg(WARN, ":964", "TTOCTT at line #%d: ID %s of type %s does not exist on system, skipping to next entry.", indx, to_id, TS_NAME(type));
		    continue;
		}

		break;

	    default:
		msg(EXT, ":921", "Impossible case: %s", "read_ttoctt()");
	    }
	} else {  /* second ID is a number */
	    if (!isdigit(*to_id)) {
		msg(WARN, ":962", "TTOCTT at line #%d: %s: field must be alphanumeric, skipping to next entry.",
		    indx, to_id);
		continue;
	    }

	    /* Verify ID is not too long. */
	    if (strlen(to_id) > (size_t) MAX_NUM_SZ) { 
		msg(WARN, ":966", "TTOCTT at line #%d: %s: field is too long, skipping to next entry.",
		    indx, to_id);
		continue;
	    }
	    id = atoi(to_id);

	}

	switch ((int)type) {

	case UI:
	    if (pw == NULL) {	  /* Was not alphabetical, must validate UID */
		pw = getpwuid(id);
	    }
	    if (pw == NULL) {
		msg(WARN, ":964", "TTOCTT at line #%d: ID %s of type %s does not exist on system, skipping to next entry.", indx, to_id, TS_NAME(type));
		continue;
	    }
	    id = pw->pw_uid; /* if UI was alpha, id has not been set yet. */
	    break;

	case GI:
	    if (gr == NULL) {      /* Was not alphabetical, must validate GID */
		gr = getgrgid(id);
	    }
	    if (gr == NULL) {
		msg(WARN, ":964", "TTOCTT at line #%d: ID %s of type %s does not exist on system, skipping to next entry.", indx, to_id, TS_NAME(type));
		continue;
	    }
	    id = gr->gr_gid; /* if GI was alpha, id was not yet set.*/
	    break;

	case LI:	/* Must validate level being mapped to. */
	    switch (lvlvalid(&id)) {
	    case -1:	    	    /* invalid level */
		msg(WARN, ":964", "TTOCTT at line #%d: ID %s of type %s does not exist on system, skipping to next entry.", indx, to_id, TS_NAME(type));
		continue;
	    case 0: 	    	    /* valid level */

	    default:	    	    /* valid-inactive level */
		break;		/* handle v-i level as valid in TTOCTT */

	    }
	    break;

	default:
	    msg(EXT, ":921", "Impossible case: %s", "read_ttoctt()");
	} /* switch  on type */

	/* if the DB is marked V_OK, we have to change it to V_CHANGE */
	/* otherwise we will never "visit" the node with the new value */
	/* We also have to mark all "untested" IDs as VS_OK, so they will */
	/* not be checked in vain */
	if (tent->v_flags & V_OK) {
	    register VS p;
	    register int i;
	    tent->v_flags |= V_CHANGE;
	    tent->v_flags &= ~V_OK;
	    if (type != LI)
		for (p = tent->v_map, i = tent->v_nelm; i; p++, i--)
		    if (!(p->vs_state & VS_TESTED))
			p->vs_state |= VS_OK;
	}

	/* adujst the current ID data */
	ovs->vs_state |= VS_OK;
	ovs->vs_current = id;

	/* if this ID is "new", we have to insert it in the proper table */
	if (insert) {
	    tent->v_nelm++;
	    if (type == LI){
		ENTRY item;
		item.key = (char *)malloc((size_t)MAX_NUM_SZ);
		sprintf(item.key, "%d", ovs->vs_value);
		item.data = (char *)ovs;
		if (hsearch(item, ENTER) == NULL) {
		    msg(EXT, ":968", "Internal hash table overflow.");
		}
	    } else {
		if ((tent->v_map = (VS)realloc(tent->v_map, (tent->v_nelm * VS_SZ + strlen(ovs->vs_name) + 1))) == NULL) {
		    msg(EXT, ":643", "Out of memory: %s", "read_ttoctt()");
		}
		(tent->v_map + tent->v_nelm - 1)->vs_value = ovs->vs_value;
		(tent->v_map + tent->v_nelm - 1)->vs_current = ovs->vs_current;
		(tent->v_map + tent->v_nelm - 1)->vs_state = ovs->vs_state;
		(tent->v_map + tent->v_nelm - 1)->vs_name = ovs->vs_name;
		qsort((char *)tent->v_map, tent->v_nelm, sizeof(vs), gt);
	    }
	} /* insert */
    } /* for loop on TTOCTT */

    /* endpwent() and endgrent() are not called because we will use */
    /* these files again later. */
} /* read_ttoctt */


/*
 * Procedure:     check_id
 *
 * Restrictions:
                 setpwent: None
                 setgrent: None
                 getpwuid: None
                 getgrgid: None
                 lvlvalid: None
*/
/*
** notes:	Check a given ID.
**
**  First, check if there is a need for a check.
**  Second, locate the corresponding record in the TTOC.
**  Then, check if we already "visited" this ID.
**  If we did, return the stored ID state.
**  If we didn't, test this ID and set the state accordingly.
**  Now we can return that state info.
**
**  Returns VS_OK if the id is valid (or rempped) and VS_BAD
**  if the id is not valid and there is no remapping specified.
**
**  NOTE: if the return value is NOT VS_BAD, the argument "alt"
**  	will be assigned the "real" value of the ID.
**
**  Called by valid_id() (-i option).
*/

static
int
check_id(id, type, alt, vi_flag)
id_t id;
enum TS type;
id_t *alt;
ulong vi_flag;	/* sp->sh_flags value for handline v-i LIDs/invalid IDs */
		/* It is set to O_ACL if the ID being validated is for an ACL */
{
    VP vp = NULL;	/* pointer to the relevant TS validate structure */
    VS vsp = NULL;	/* pointer to this IDs val_str sructure */
    char *curr_name;	/* pointer to the current "name" of an ID */

    *alt = id;	/* "default" value */
    /* get the validate struct for this ID type */
    vp = &Vtbl[type];
    if (vp->v_flags == 0) {
	msg(ERR, ":969", "No information available on \"%s\"", TS_NAME(type));
	return (VS_BAD);
    }

    /* don't have to validate if nothing changed */
    /* NOTE: we don't even try to locate the IDs vs struct */
    if (vp->v_flags & V_OK) {
	if (vp->v_flags & V_REMAP)
	    *alt = vp->v_remap;
	return (VS_OK);
    }

    if (!(vp->v_flags & V_DATA))   /* Database must have been saved for     */
	return (VS_BAD);	   /* UIDs/GIDs at this point.  For LIDs    */
				   /* V_DATA is set, but the DB isn't saved.*/

    /* locate the record in the TTOC */
    if (type == LI) {
	ENTRY item, *ep;
	item.key = (char *)malloc((size_t)MAX_NUM_SZ);
	sprintf(item.key, "%d", id);
	if ((ep = hsearch(item, FIND)) == NULL) {
	    if ((vsp = REC(val_str)) == NULL) { 
		msg(EXT, ":643", "Out of memory: %s", "check_id()");
	    }
	    vsp->vs_value = id;		/* new LID, add to table */
	    vsp->vs_name = NULL;
	    vsp->vs_state = NULL;
	    item.data = (char *)vsp;
	    if (hsearch(item, ENTER) == NULL) {
		msg(ERR, ":968", "Internal hash table overflow.");
		return (VS_BAD);
	    }
	} else {
	    free(item.key);
	    vsp = (VS)ep->data;
	}
    } else {  /* not LI */
	vs item;

	item.vs_value = id;
	vsp = (VS)bsearch(&item, vp->v_map, vp->v_nelm, sizeof(vs), (int (*)()) gt);

	/* 
	 * if invalid when archived, restore 
	 */
	if (((type == UI) && (vi_flag & O_UINVLD)) ||
	   ((type == GI) && (vi_flag & O_GINVLD))) {

            setpwent();
            setgrent();

	    /* if still invalid */
            if (((type == UI) && (getpwuid(id) == NULL)) ||
               ((type == GI) && (getgrgid(id) == NULL))) {

	        if (vsp == NULL)
    	            *alt = id;
	        else
    	            *alt = vsp->vs_current; 	/* set to current value */
    	        return (VS_OK);
	    }
	}

	/* 
	 * If the ID not stored on the archive, for an ACL, 
	 * this means it was invalid on input.  In this case,
	 * it is used without a warning.
	 */
	if ((vsp == NULL) && (vi_flag & O_ACL)) {

    	    *alt = id; 	/* set to current value */
    	    return (VS_OK);
	}
	    
	/* no record for this ID (maybe an orphan ID) */
	if (vsp == NULL) {
	    msg(ERR, ":970", "No information is available on ID %ld of type \"%s\"",
		id, TS_NAME(type));
	    return (VS_BAD);
	}
    }

    /* if this ID was tested before, we need no further action */
    switch (vsp->vs_state & VS_TESTED) {

    case VS_OK:  	    /* ID was tested and can be used */
	*alt = vsp->vs_current;
	return (VS_OK);

    case VS_CHANGE:	    /* unresolved change */
	if (Unverify & NO_ACT && type == LI) { /* maybe ERROR! */
	    *alt = vsp->vs_current;
	    return (VS_OK);
	}
	/* fall through */

    case VS_BAD: 	    /* ID is really bad */
	return (VS_BAD);

    case NONE:  	    /* not tested */
	break;

    default:
	msg(ERR, ":921", "Impossible case: %s", "check_id()");
	return (VS_BAD);
    }

    /* now it MUST be !VS_TESTED, so we compare to the current ID info */
    if (type == LI) {
	switch (lvlvalid(&id)) {

	case -1:
	    vsp->vs_state |= VS_BAD;
	    if (!(Args & OCx))
		msg(EPOST, ":971", "ID %d of type LI is invalid.", vsp->vs_value);
	    return (VS_BAD);
    
	case 0:
	    vsp->vs_state |= VS_OK;
	    break;

	default:
	    if (Unverify & NO_ACT)
		vsp->vs_state |= VS_OK;
	    else if (vi_flag & O_VLDIN)  /* if v-i when archived, restore */
		vsp->vs_state |= VS_OK;
	    else {
		vsp->vs_state |= VS_CHANGE;
		if (!(Args & OCx))
		    msg(EPOST, ":972", "ID %d of type LI is inactive.", vsp->vs_value);
		return (VS_BAD);		/* handle like invalid  */
	    }
	}
    } else {	/* not LID */
	if ((curr_name = curr_info(vsp, type)) == (char *)NULL) {
	    /* no such ID */
	    vsp->vs_state |= VS_BAD;

	    return (VS_BAD);
	}
    
	if (strcmp(vsp->vs_name, curr_name)) {
	    vsp->vs_state |= VS_CHANGE;
	    vsp->vs_value = ERROR;
	    return (VS_BAD);
	}
	vsp->vs_state |= VS_OK;
    }

    if (vp->v_flags & V_REMAP)
	vsp->vs_current = vp->v_remap;
    else
	vsp->vs_current = vsp->vs_value;

    *alt = vsp->vs_current; 	/* set to current value */
    return (VS_OK);
} /* check_id */


/*
** valid_id:	Validate a given ID.
**
**  the ID may be set to a new value, 
**  and the return value is set
**  depending on the value of check_id() and
**  the type of action requested.
**
**  Called through the CHECK() macro by validate (-i option).
*/

static
int
valid_id(idp, type, action, vi_flag)
id_t *idp;
enum TS type;
int action;
ulong vi_flag;	/* sp->sh_flags value for handline v-i LIDs/invalid IDs */
		/* It is set to O_ACL if the ID being validated is for an ACL */
{
    id_t alt, *a = &alt;

    if (check_id(*idp, type, a, vi_flag) != VS_OK)
	if (action == USE) {
	    if (!(Args & OCx))
		msg(EPOST, ":973", "using invalid ID <%d> anyway.", *idp);
	    return (VS_OK);
	} else {
	    return (VS_BAD);
	}

    if (*idp != alt)
	switch (action) {

	case USE:
	case MOD:
	    *idp = alt;
	    return (VS_OK);

	default:
	    msg(EXT, ":921", "Impossible case: %s", "valid_id()");
	}
    else
	return (VS_OK);

    return (VS_BAD); /* should never reach this point */
} /* valid_id */


/*
** CHECK:    remember-bad-validate code segment.
**
**  The condition is a validation test on an ID.  Type is a TS.
*/

#define	CHECK(id, typ, act, vi) 	    	    	    	    	    	\
    	    	    	if (valid_id((id_t *)&(id), typ, act, vi) == VS_BAD) {\
		    	    	msg(ERR, ":1010", "Invalid ID <%ld>, type <%s> for file \"%s\", skipping.",	\
    	    	    	    	    id, TS_NAME(typ), gp->g_nam_p);    	\
    	    	    	    err = 1;			    	    	\
			}

/*
** validate:    Validate all the IDs associated with a specific file.
**
**  Each of the IDs referenced in the gen & sec headers
**  is validated in turn.  If a non-valid ID is encountered
**  the process is stopped and an error indicator is returned.
**
**  SIDE EFFECTS: The given ID might be assigned a new value.
**
**  NOTE: One way to speed things up maybe to keep a "cache" of
**  the IDs referenced by the last file read.  Assuming most
**  files that belong to the same user/group/... are read is a
**  sequence, this will be a performance win.
**
**  Called by ckname() when reading a file in (via file_in(); -i option).
*/

int
validate(gp, sp)
struct gen_hdr *gp;
struct sec_hdr *sp;
{
    register short indx;
    register struct acl *ap;
    register int err = 0;	     /* Possibly set in CHECK() */

    CHECK(gp->g_uid, UI, MOD, sp->sh_flags);    /* Check UID, GID, and LID for the file */
    CHECK(gp->g_gid, GI, MOD, sp->sh_flags);

    if (macpkg) {
	CHECK(sp->sh_level, LI, MOD, sp->sh_flags);
    }

    /* Check all UIDs and GIDs on the ACL associated with the file.
     * Called with USE so the files aren't skipped because of a bad
     * ID in an ACL.
     */
    for (indx = sp->sh_acl_num, ap = sp->sh_acl_tbl; indx; indx--, ap++) {
	switch (ap->a_type) {

	case USER:
	case DEF_USER:
	    CHECK(ap->a_id, UI, USE, O_ACL);
	    break;

	case GROUP:
	case DEF_GROUP:
	    CHECK(ap->a_id, GI, USE, O_ACL);
	    break;

	case USER_OBJ:
	case GROUP_OBJ:
	case CLASS_OBJ:
	case OTHER_OBJ:
	case DEF_USER_OBJ:
	case DEF_GROUP_OBJ:
	case DEF_CLASS_OBJ:
	case DEF_OTHER_OBJ:
	    break;
	default:
	    msg(ERR, ":921", "Impossible case: %s", "validate()");
	}
    }

    return (err? ERROR : DONE);
} /* validate */


/*
 * Procedure:     hiv
 *
 * Restrictions:
                 lvlvalid: None
*/


/*
** notes:	Hand ID validation.
**
**  Manual validation of some of the IDs that are not
**  referenced by files, but are used elsewhere.
**  This includes the LID range levels for the medium.
**
**  Called by in_sec_setup() (-i option).
*/

static
void
hiv(mh)
struct med_hdr *mh;
{
    /* NOTE: if the stored archive level range is INVALID,  */
    /* we can't compare it to the device range.  The only   */
    /* way to "fix" this is to define this level(s).	    */
    if (mh->mh_lo_lvl != MH_UNBND)
        switch (lvlvalid(&(mh->mh_lo_lvl))) {

        case -1:
	    msg(EXT, ":975", "Archive low level (%d) is invalid.", mh->mh_lo_lvl);

        case 0:
	    break;

        default:
	    if (!(Args & OCx) && !(Unverify & NO_ACT))
	        msg(EPOST, ":976", "Archive low level (%d) is inactive.", mh->mh_lo_lvl);
	    break;
        }

    if (mh->mh_hi_lvl != MH_UNBND)
        switch (lvlvalid(&(mh->mh_hi_lvl))) {

        case -1:
	    msg(EXT, ":977", "Archive high level (%d) is invalid.", mh->mh_hi_lvl);
    
        case 0:
	    break;
    
        default:
	    if (!(Args & OCx) && !(Unverify & NO_ACT))
	        msg(EPOST, ":978", "Archive high level (%d) is inactive.", mh->mh_hi_lvl);
	    break;
        }
} /* hiv */


/*
** in_sec_setup:    Set up security related variables/structures.
**  	    	    on INPUT.
**
**  Called by main().
*/

void
in_sec_setup(f)
FILE *f;
{
    int rv;
    void init_arch_tree();

    /* Currently, tcpio saves the UID/GID databases and stat information
     * for UID/GID/LID.  Mark flags accordingly.
     */
    if (macpkg) {
	Save_stat = S_UID | S_GID | S_LID;
    } else {
	Save_stat = S_UID | S_GID;
    }
    Save_data = S_UID | S_GID;

    if (read_mhdr(M_p, 1) == ERROR)
	msg(EXT, ":979", "Bad medium header");
    if (! (Unverify & NO_SYS) && strcmp(M_p->mh_host, Host))
	msg(EXT, ":980", "System name %s does not match name %s in header", Host, M_p->mh_host);
    read_ttoc();
    if (f != NULL ) {	/* safer than (Args & OCT) */
	read_ttoctt(f);
    }
    hiv(M_p);
    rv = read_privmap();
    if (rv == ERROR) {
      msg(WARN, ":981",
	  "Privilege header not found.  Privilege information unavailable.");
      M_p->mh_numsets = 0;
    }
    else
      if(rv > 0)
      msg(WARN, ":982",
	  "Set of definable privileges differ between archive and system.  \nUsing archive privilege set.");
    init_arch_tree();
} /* in_sec_setup */


/*
 * add_node:  Add a node to the archive tree
 *
 *  Called from init_arch_tree() and get_component().
 */
static
void
add_node(new, parent, name)
struct archive_tree **new, *parent;
char *name;
{
    if ((*new = REC(archive_tree)) == (struct archive_tree *)NULL)
	        msg(EXT, ":643", "Out of memory: %s", "add_node()");
    (*new)->parent = parent;
    (*new)->sibling = NULL;
    (*new)->child = NULL;
    (*new)->hdrs = NULL;
    (*new)->copied = NOT_COPIED;
    if (((*new)->name = strdup(name)) == NULL)
	    msg(EXT, ":643", "Out of memory: %s", "add_node()");

} /* add_node */


/*
 * init_arch_tree:  Initialize the archive tree
 *
 * Called by out_sec_setup() (-o) or in_sec_setup() (-i) to set
 * up a tree of archive_tree structures which echos the current working
 * directory (CWD). For -o, a node will be added to this tree in the
 * appropriate place each time another FS object is added to the archive.  For
 * -i, not the whole archive will be placed in the tree, but only certain
 * "interesting" directories.
 *
 *  ASSERT: Fullnam_p (return from getcwd()) is a non-redundant pathname.  
 *  I.e., it contains no '.'s or '..'s, no more than one '/' in a row, and
 *  no unnecessary tailing '/'.
 */
static
void
init_arch_tree()
{
    register struct archive_tree *tp;
    char *cp, *name, save;

    if ((tp = REC(archive_tree)) == (struct archive_tree *)NULL)
	msg(EXT, ":643", "Out of memory: %s", "init_arch_tree()");

    /* all trees start with '/' */
    if ((tp->name = strdup("/")) == NULL)
	    msg(EXT, ":643", "Out of memory: %s", "init_arch_tree()");
    tp->copied = NOT_COPIED;
    tp->parent = tp;
    tp->sibling = NULL;
    tp->hdrs = NULL;

    Arch_root = tp;

    cp = Fullnam_p;

    if ((name = ++cp) == '\0') {
	    cwd_ptr == Arch_root;
	    return;
    }

    do {				 /* Add each component of the CWD to */
	if (*cp == '/' || *cp == '\0') { /* the archive tree.		     */
	    save = *cp;
	    *cp = '\0';
	    add_node(&tp->child, tp, name);
	    tp = tp->child;
	    *cp = save;
	    name = cp + 1;
	}
    } while (*cp++);

    cwd_ptr = tp;	/* set CWD ptr in archive tree */

} /* init_arch_tree() */


/*
** out_sec_setup:   Setup security related variables/structures
**  	    	    on OUTPUT.
**
**  Called by main().
*/

void
out_sec_setup()
{
    /* Currently, tcpio saves the UID/GID databases and stat information
     * for UID/GID/LID.  Mark flags accordingly.
     */
    if (macpkg) {
	Save_stat = S_UID | S_GID | S_LID;
    } else {
	Save_stat = S_UID | S_GID;
    }
    Save_data = S_UID | S_GID;
    if ((M_p->mh_numsets = get_privmap()) <= 0)
      msg(WARN, ":983", "Cannot access system privilege information.  No privilege information available.");
    fill_mhdr(M_p);
    (void)write_mhdr(M_p, 1);
    write_ttoc();
    write_privmap();
    init_arch_tree();
} /* out_sec_setup */


/*
 * Procedure:     set_sec
 *
 * Restrictions:
                 acl(2): None
                 chmod(2): None
*/

/*
** notes: Set a file's security-related (ACL) parameters.
**
**  Called from rstfiles() (-i option).
*/

void
set_sec(gp, sp)
struct gen_hdr *gp;
struct sec_hdr *sp;
{
	mode_t modebits;
	static int no_acl = 0;	/* Set to 1 if acl() fails with EPERM  */
				/* which means lack of privilege.      */

	if (aclpkg && !no_acl && sp->sh_acl_num > 0) {
		if (acl(gp->g_nam_p, ACL_SET, sp->sh_acl_num, sp->sh_acl_tbl) == ERROR) {
			if (!no_acl && errno == EPERM)
				no_acl = 1;	/* Don't try acl() anymore */
			msg(ERRN, ":984", "Cannot set ACL of \"%s\"", gp->g_nam_p);
		} else if (privileged ? !(gp->g_mode & S_ISVTX) : !(gp->g_mode & Orig_umask )) {
			/*
			 * If the acl() call above succeeded, the mode bits are
			 * taken care of automatically, so we can skip the
			 * chmod call below, except:
			 *  - If the user is not privileged, we want the umask
			 *    bits of the mode to be cleared.  If any of the
			 *    umask bits are set in the mode, we call chmod to
			 *    clear them.
			 *  - If the user is privileged, and the "sticky" bit
			 *    is on in the mode, we need to call chmod to set
			 *    it on the file.  (Note, the creat() call ignores
			 *    sticky bit.)
			 */
			return;
		}
	}

	/* if we skipped the acl() system call above, either because DAC isn't
	 * running currently, or because DAC wasn't running when the archive
	 * was created (sp->sh_acl_num == 0), or if the acl() call failed (it
	 * really shouldn't), must set permissions via chmod().  Or, even if
	 * permissions were successfully set via acl(); for an unprivileged
	 * user, must clear the umask bits via chmod(), and for a privileged
	 * user, may need to set "sticky" bit.
	 */

	/* if not privileged, clear umask bits */
	modebits = gp->g_mode & (privileged ? ~0 : ~Orig_umask);
	(void)chmod(gp->g_nam_p, modebits);
} /* set_sec */


/*
 * Procedure:     adjust_lvl
 *
 * Restrictions:
                 lvlproc(2): None

** notes:	Adjust the level of the process.
**
**  When privileged, must call this function in order to get the correct
**  level on the file being restored.
**
**  Ordinarily, this function will be called before
**  creating a new file.  The level of the process will be set, which will
**  cause the file to be created at the correct level.  The reason this is done
**  rather when a lvlfile() is that setting the processes level will cause
**  deflection to the correct effective directories if there are any MLDs in
**  file's path.
**  
**  However, in the case where MAC is not running, (for example, if mUNIX is
**  running) it is not possible to set a process level.  But that's ok, because
**  lvlfile() is still possible, and we don't have to worry about MLDs, because
**  MLDs look just like regular directories when MAC isn't running.  In this
**  case, this function will not be called, but rather, the fuction
**  adjust_lvl_file() will be called after the file is
**  created, and the files level will be adjusted via lvlfile().
**
**  Note: it is assumed that this function is only called when tcpio
**  has the proper privs.
**
**  Called from openout() and creat_spec() (-i option).
**
**  Returns 0 if the level was changed.
**  Returns ERROR if the level was not changed.
*/

int
adjust_lvl(sp)
struct sec_hdr *sp;
{
    int rv = 0;		/* assume success unless set otherwise */

    if (!no_lvlp && (orig_lvl != sp->sh_level)) {
    	if (lvlproc(MAC_SET, &sp->sh_level) == ERROR) {
	    if (errno == EINVAL)
		msg(WARN, ":1011", "Level %d of file %s invalid, restoring at level of process.", sp->sh_level, G_p->g_nam_p);
	    else {
		/* should never happen.  This function is never called unless
		 * priv-ed and mac is running, so EPERM and ENOPKG are not
		 * possible.
		 */
		if (!no_lvlp && errno == EPERM)
		    no_lvlp=1;  /* Don't try lvlproc(MAC_SET) anymore */
		msg(ERRN, ":985", "Cannot set level of process");
	    }
	    rv = ERROR;
        }
    } else 							 
        rv = ERROR;						
    return(rv);							
} /* adjust_lvl */


/*
 * Procedure:     chg_lvl_back
 *
 * Restrictions:
                 lvlproc(2): None

** Notes:  Change back to the calling process's level
**
**  Called from openout() and creat_spec() (-i option) to change back to 
**  the original level of the calling process.  
**  This is done in case tcpio should happen 
**  to core dump.  This insures that the core will not be written at an 
**  inappropriate level.
**
**  It is assumed that adjust_lvl() has been called prior to calling
**  chg_lvl_back() and it changed the level of the calling process.
*/

void
chg_lvl_back()
{
    if (!no_lvlp && lvlproc(MAC_SET, &orig_lvl) == ERROR)
        msg(ERRN, ":776", "Cannot restore level of process");
} /* chg_lvl_back() */


/*
 * Procedure:     adjust_lvl_file
 *
 * Restrictions:
                 lvlfile(2): None

** notes:	Adjust the level of the file.
**
**  See comment preceding adjust_lvl().
**
**  Note: it is assumed that this function is only called when tcpio
**  has the proper privs.
**
**  Called from openout() and creat_spec().
**
*/

void
adjust_lvl_file(gp, sp)
struct gen_hdr *gp;
struct sec_hdr *sp;
{
    if (!no_lvlp) {
    	if (lvlfile(gp->g_nam_p, MAC_SET, &sp->sh_level) == ERROR) {
	    if (!no_lvlp && errno == EPERM)
	        no_lvlp=1;  /* Don't try lvlproc(MAC_SET) anymore */
            msg(ERRN, ":1002", "Cannot set level of file \"%s\".", gp->g_nam_p);
	}
    }
} /* adjust_lvl_file */


/*
 * Procedure:     sec_dir
 *
 * Restrictions:
                 mkmld(2): None
                 mkdir(2): None
*/


/*
** notes: Make directories in the secure environment.
**
**  NOTE: You don't need to be in MLD_REAL to make
**  an MLD.
**
**  Called by creat_spec() (-i option) when bringing a directory 
**  in from archive.
*/

int
sec_dir(path, mode, sp)
char *path;
mode_t mode;
struct sec_hdr *sp;
{
    int rv;
    static int no_mld = 0;	/* Set to 1 if mkmld() fails due   
				   to lack of privilege.    	   */
    int lvl_changed = 0;	/* Set to 1 if the lvl is changed. */

    if (privileged && macpkg && adjust_lvl(sp) == 0)					
	lvl_changed = 1;					

    if (!no_mld && sp->sh_flags & O_MLD) {

	if (rv = mkmld(path, mode) == ERROR) {
	    if (!no_mld && errno == EPERM)  /* P_MULTIDIR not in wkg set */
	    	no_mld=1;  /* Don't try mkmld() anymore */
	    msg(ERR, ":986", "Cannot create \"%s\" as an MLD, making regular directory.",path);	
	    rv = mkdir(path, mode);				
	}
    } else {
	rv = mkdir(path, mode);
    }

    if (lvl_changed)						
	chg_lvl_back();						

    return (rv);
} /* sec_dir */


/*
 *  get_component: break a path name up into individual components.
 *
 *  When this function is called with pathname != NULL, it will
 *  extract the first component of pathname.  After this, successive
 *  calls with pathname == NULL will extract successive components of
 *  the original pathname.  The function returns 1 whenever another
 *  component was found, 0 when the pathname runs out.
 *
 *  The component_pp argument is set to point to the archive_tree entry
 *  that corresponds with the extracted pathname.  New archive_tree
 *  entries are created by this function (through calls to add_node)
 *  as needed, except that new entries are *not* created for the last
 *  component of the path when the is_dir flag is 0.  In this case,
 *  component_pp is set to NULL.
 *
 *  Besides returning a pointer to a node in the archive tree (if
 *  appropriate), this routine will put a NULL characther in pathname
 *  in the appropriate place, such that the resulting sub-pathname
 *  corresponds with the pointer returned.  When this routine returns
 *  0, pathname will have been restored to what it was originally.
 *
 *  Note: A component '.' occuring anywhere but the beginning of the path
 *  is skipped, since it is redundant.  A component of '..' is redundant in
 *  some cases, but not all.  This routine does not try to figure it out, but
 *  always returns '..' as a separate component.
 *
 *  ASSERT: '/' and cwd exist on the tree (due to init_arch_tree()).
 *
 */
int
get_component(pathname, component_pp, is_dir)
char *pathname;
struct archive_tree **component_pp;
int is_dir;
{
    struct archive_tree *parent, *prev_ptr;
    char *name;
    static struct archive_tree *tp;
    static char *cp, *savep, save;

    if (pathname != NULL) {
	    cp = pathname;
	    if (*cp == '/') {
		    savep = cp + 1;
		    save = *savep;
		    *savep = '\0';
		    *component_pp = tp = Arch_root;
		    return 1;
	    } else
		    tp = cwd_ptr;
    } else
	    *savep = save;

    /* skip over '/'s and './'s to get to the begining of the next
     * component or the end of the path
     */
    while (*cp == '/') {
	    ++cp;
	    while (*cp == '.' && (cp[1] == '/' || cp[1] == '\0'))
		    cp += 2;
    }

    /* if we're at the end of the path, return NULL (no more
     * components)
     */
    if (*cp == '\0') {
	    *component_pp = NULL;
	    return 0;
    }

    name = cp;

    /* get to the end of the component and process it. */
    while (*cp != '/' && *cp != '\0')
	    cp++;
    save = *cp;
    savep = cp;
    *cp = '\0';

    if (!strcmp(name,"..")) {   /* "..":  go to the parent */
	    if ((tp = tp->parent) == 0) /* can't happen */
		    msg(EXT, ":921", "Impossible case: %s", "get_component()");
    } else if (!strcmp(name,".")) {
	    /* do nothing.  can only happen when "." is first
	     * component of path, since other "."s are skipped
	     * above [while (*cp == '.' ...]
	     */
	    ;
    } else if (tp->child == NULL) {
	    /*
	     * Add component to tree, unless this is the last
	     * component of the path and it's not a dir.
	     */
	    if (is_dir || save != '\0') {
		    add_node(&tp->child, tp, name);
		    tp = tp->child;
	    } else
		    tp = NULL;
    } else {
	    parent = tp;

	    /* search childern */
	    for (tp = tp->child; tp; prev_ptr = tp, tp = tp->sibling) {
		    if (!strcmp(tp->name,name))
			    break;
	    }

	    /* if not found, add new child to tree, unless is the last */
	    /* component of the path and it's not a dir. */
	    if (tp == NULL && (is_dir || save != '\0')) {
		    add_node(&prev_ptr->sibling, parent, name);
		    tp = prev_ptr->sibling;
	    }
    }

    *component_pp = tp;
    return 1;
} /* get_component */


/*
 * Procedure:     get_privmap
 *
 * Restrictions:
                 secsys(2): None
*/
 
 /*
  *  Notes:
  *   gets system privilege information, and for those privilege 
  *   sets which are applicable to files,  files priv_map structures
  *   to be stored on the archive.
  *  
  *  Called from : out_sec_setup
  *
  *  Return values:
  *     on success: number of privilege sets defined for files
  *     on failure: ERROR
  *
  */
int
get_privmap()
{
  int i,j;
  int cnt=0; 
  char *namebuf;   
  setdef_t *sdefs; /* buffer for secsys info */
  struct priv_map *ppmap;
  
/* allocate memory and get system info */
  if ((nsets = secsys(ES_PRVSETCNT, 0)) < 0) 
    return(ERROR);
  if ((sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t))) == NULL) 
    return(ERROR);
  (void)secsys(ES_PRVSETS, (char *)sdefs);

/* allocate memory and fill priv_map structures */
  if ((privmaps = (struct priv_map*)malloc(nsets * sizeof(struct priv_map))) 
      == NULL) 
    msg(ERR, ":643","Out of memory: %s", "get_privmap()");
  ppmap = privmaps;
  for (i = 0; i < nsets; ++i) {
    if (sdefs[i].sd_objtype == PS_FILE_OTYPE) {
      cnt++;
      ppmap->pm_mask=sdefs[i].sd_mask;
      if (memcpy(ppmap->pm_name,sdefs[i].sd_name,PRVNAMSIZ) == NULL)
	return(ERROR);
      ppmap->pm_count=sdefs[i].sd_setcnt;
      for (j = 0; j < ppmap->pm_count; ++j) {
	if (privname(ppmap->pm_privs[j],j) == NULL)
	  return(ERROR);
      }
      ppmap++;
    }
  }
  return(cnt);
}

/*
 * Procedure:     read_privmap
 *
 * Restrictions:
                 secsys(2): None
*/


 /*
  *  notes:
  *   reads system privilege information, 
  *       gets current system privilege info,
  *       and compares
  *  
  *  Called from : in_sec_setup
  *
  *  Return values:
  *     sets match: DONE
  *     sets don't match: 1
  *     on failure to read archived info: ERROR
  *
  */
int
read_privmap()
{
  int i,j,k,found;
  register struct buf_info *buf = &Buffr;
  setdef_t *sdefs; /* buffer for secsys info */
  struct priv_map *ppmap;
  char pnamebuff[PRVNAMSIZ];

/* get archive info */
  if (M_p->mh_numsets < 1)
    return(ERROR);
  if((ppmap = privmaps = 
      (struct priv_map*)malloc(M_p->mh_numsets * sizeof(struct priv_map))) 
     == NULL) {
    msg(EXTN, ":643","Out of memory: %s", "read_privmap()");
  }
  for (i = 0;i < M_p->mh_numsets; i++) /* for each set */
    {
      FILL(PMAPSZ + PRVNAMSIZ);
      if(sscanf(buf->b_out_p, "%8lx%8ux", 
		&ppmap->pm_mask,
		&ppmap->pm_count) != PMAP_CNT) 
	return(ERROR);
      buf->b_cnt -= PMAPSZ;
      buf->b_out_p += PMAPSZ;
      if (memcpy(ppmap->pm_name,buf->b_out_p,PRVNAMSIZ) == NULL)
	return(ERROR);
      buf->b_cnt -= PRVNAMSIZ;
      buf->b_out_p += PRVNAMSIZ;
      FILL(ppmap->pm_count * PRVNAMSIZ);
      for (j=0; j < ppmap->pm_count; j++) {   /* for each priv in set */
	if (memcpy(ppmap->pm_privs[j],buf->b_out_p,PRVNAMSIZ) == NULL)
	  return(ERROR);
	buf->b_cnt -= PRVNAMSIZ;
	buf->b_out_p += PRVNAMSIZ;
      } /* for each priv */
      ppmap++;
    } /* for each set */

  /* get system info */  
  if ((nsets = secsys(ES_PRVSETCNT, 0)) < 0) 
    {
      msg(WARN, ":983", "Cannot access system privilege information.  No privilege information available.");
      return(ERROR);
    }
  if(nsets < M_p->mh_numsets) return(1);
  if ((sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t))) == NULL)
    msg(EXTN, ":643","Out of memory: %s", "read_privmap()");
  (void) secsys(ES_PRVSETS, (char *)sdefs);
  for (i = 0, ppmap=privmaps; i < M_p->mh_numsets; ++i, ppmap++) 
    {                                 /* for each set on archive */
     for (j = 0, found = 0; j < nsets && found == 0; ++j)
       {                       /* look for matching set (by mask) on system */
	 if (ppmap->pm_mask == sdefs[j].sd_mask)
	   {                   /* if matching set is found */
	   found = 1;
	   if (ppmap->pm_count > sdefs[j].sd_setcnt
	       || strcmp(ppmap->pm_name,sdefs[j].sd_name))
	     return(1);     /* error if set name differs */
	   for (k = 0; k < ppmap->pm_count; ++k) 
	     {    /* for each priv on archive set, compare sys/arch names */
	     if (strcmp(ppmap->pm_privs[k],privname(pnamebuff,k)))
		 return(1);
	   }
	 } 
       } /* for each set on system */
      if (found == 0) return(1);
   } /* for each set on archive */
return(DONE);
}



/* write_privmap */
void
write_privmap()
{
  register struct buf_info *buf=&Buffr;
  struct priv_map *ppmap=privmaps;
  int set,i;

  for(set=0;set < M_p->mh_numsets; set++,ppmap++)
    {
      FLUSH(PMAPSZ + PRVNAMSIZ);
      (void)sprintf(buf->b_in_p,"%.8lx%.8ux",
		    ppmap->pm_mask,
		    ppmap->pm_count);
      buf->b_in_p += PMAPSZ;
      buf->b_cnt  += PMAPSZ;
      memcpy(buf->b_in_p, ppmap->pm_name, PRVNAMSIZ);
      buf->b_in_p += PRVNAMSIZ;
      buf->b_cnt  += PRVNAMSIZ;
      for(i=0;i < ppmap->pm_count; i++)
	{
	  FLUSH(PRVNAMSIZ);
	  memcpy(buf->b_in_p,
		       ppmap->pm_privs[i],
		       PRVNAMSIZ);
	  buf->b_in_p += PRVNAMSIZ;
	  buf->b_cnt  += PRVNAMSIZ;
	}/* for each priv */
    } /* for each set */
}

/* print_privs 
 *
 *  displays privileges for a file, using the descriptor-to-name mapping
 *    stored on the archive.
 *
 *  Called from verbose()
 */

void
print_privs(numprivs,fprivs,pbuff)
int numprivs; /* number of privs for this file */
priv_t fprivs[];
FILE *pbuff; /* buffer verbose will write out */
{
  int i,j,k,legend,printed,found = 0;
  struct priv_map *ppmap = privmaps;
  for(i=0;i < M_p->mh_numsets;ppmap++, i++) { 
                                           /* for each stored priv set */
    legend=printed=0;
    for(j=0;j < numprivs; j++) {           /* for each file privilege  */
      for(k=0;k < ppmap->pm_count;k++) 
	{   /* for each priv in this set */
	if((ppmap->pm_mask|k) == fprivs[j])  /* priv descriptor = mask|offset*/
	  {
	    found++;
	    if (!legend){
	      legend=1;
	      (void)fputc(':',pbuff);
	      (void)fputs(ppmap->pm_name, pbuff);
	      (void)fputc(' ',pbuff);
	    }
	    if(printed) {
	      printed=0;
	      (void)fputc(',',pbuff);
	    }
	    (void)fputs(ppmap->pm_privs[k],pbuff);
	    printed = 1;
	  }
      }/* for each priv in set */
    } /* for each file priv */
  } /* for each set */
  if (found != numprivs)
    msg(EPOST, ":830","Unrecognized privilege name");  /* This should not happen */
}
