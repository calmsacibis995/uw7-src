/*		copyright	"%c%" 	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)cpio:common/cmd/cpio/cpio.h	1.3.14.4"

/* Option Character keys (OC#), where '#' is the option character specified. */

#define OCa	0x1
#define OCb	0x2
#define OCc	0x4
#define OCd	0x8
#define OCf	0x10
#define OCi	0x20
#define OCk	0x40
#define OCl	0x80
#define OCm	0x100
#define OCo	0x200
#define OCp	0x400
#define OCr	0x800
#define OCs	0x1000
#define OCt	0x2000
#define OCu	0x4000
#define OCA	0x8000
#define OCB	0x10000
#define OCC	0x20000
#define OCE	0x40000
#define OCG	0x80000
#define OCH	0x100000
#define OCI	0x200000
#define OCL	0x400000
#define OCM	0x800000
#define OCO	0x1000000
#define OCR	0x2000000
#define OCS	0x4000000
#define OC6	0x8000000
#define OCK	0x10000000
#define OCT	0x20000000

 /*
  * Extent Information Options
  */
 #define       OCe_IGNORE      0
 #define       OCe_WARN        1
 #define       OCe_FORCE       2
 
 #define VX_CPIOMAGIC  0xa5afcf5
 #define VX_CPIONEED   33

/* Invalid option masks for each action option (-i, -o or -p). */

#define INV_MSK4i	( OCa | OCo | OCp | OCA | OCK | OCL | OCO )

#define INV_MSK4o	( OCi | OCk | OCm | OCp | OCr | OCt | OCE \
			| OCI | OCR | OCT | OC6 )

#define INV_MSK4p	( OCf | OCi | OCk | OCo | OCr | OCt | OCA \
			| OCE | OCG | OCH | OCI | OCK | OCO | OCT \
			| OC6 )

/* Header types */

#define NONE	0	/* No header value verified */
#define	BIN	1	/* Binary */
#define	CHR	2	/* ASCII character (-c) */
#define	ASC	3	/* ASCII with expanded maj/min numbers */
#define	CRC	4	/* CRC with expanded maj/min numbers */
#define	TARTYP	5	/* Tar or USTAR */
#define	SEC	6	/* Secure system */

/* Differentiate between TAR and USTAR */

#define TAR	7	/* Regular tar */
#define USTAR	8	/* IEEE data interchange standard */

/* Flag values for lasthdr. */
#define BAD	0
#define GOOD	1

/* the pathname lengths for the USTAR header */

#define MAXNAM	256	/* The maximum pathname length */
#define NAMSIZ	100	/* The maximum length of the name field */
#define PRESIZ	155	/* The maximum length of the prefix */

/* HDRSZ: header size minus filename field length */

#define HDRSZ (Hdr.h_name - (char *)&Hdr)

/*
 * IDENT: Determine if two stat() structures represent identical files.
 * Assumes that if the device and inode are the same the files are
 * identical (prevents the archive file from appearing in the archive).
 */

#define IDENT(a, b) ((a.st_ino == b.st_ino && a.st_dev == b.st_dev) ? 1 : 0)

#ifdef _REENTRANT
#define MUTEX_LOCK(lock)	(void)mutex_lock(lock)
#define MUTEX_UNLOCK(lock)	(void)mutex_unlock(lock)
#define I_AM_INIT_THR()		(Init_thr_id == -1 || thr_self() == Init_thr_id)
#else /* _REENTRANT */
#define MUTEX_LOCK(lock)
#define MUTEX_UNLOCK(lock)
#define I_AM_INIT_THR()		1
#endif /* _REENTRANT */

/*
 * WAIT_FOR_SPACE: Determine if enough space remains in the buffer to hold cnt
 * bytes, if not, don't return until there is enough space.  For the
 * multithreaded version, this means waiting on the Buffr.b_put condition until
 * bflush_thread has flushed enough of the buffer to the archive.  For the
 * non-multithreaded version, this means calling bflush to flush the whole
 * buffer.  If we're past the end of the buffer (in the scratch area there),
 * wrap around to the beginning: copy scratch area to the beginning and set
 * pointer there.
 */

#ifdef _REENTRANT
#define WAIT_FOR_SPACE(cnt) \
{ \
	MUTEX_LOCK(&Buffr.b_mutex); \
	if (Bufcnt_update) { \
		/* \
		 * first take care of any accrued updates resulting from calls \
		 * to INCR_BUFCNT().  If the result is a buffer with >= Bufsize \
		 * bytes, signal the Buffr.b_take condition to let bflush_thread() \
		 * know that data is available to write. \
		 */ \
		Buffr.b_in_p += Bufcnt_update; \
		if ((Buffr.b_cnt += (long)Bufcnt_update) >= Bufsize && Buffr.b_waitfortake) \
			(void)cond_signal(&Buffr.b_take); \
		Bufcnt_update = 0; \
	} \
	while (Buffr.b_size - Buffr.b_cnt < (cnt)) { \
		Buffr.b_waitforput = 1; \
		(void)cond_wait(&Buffr.b_put, &Buffr.b_mutex); \
	} \
	Buffr.b_waitforput = 0; \
	wrap(cnt); \
	MUTEX_UNLOCK(&Buffr.b_mutex); \
}

#else /* _REENTRANT */
#define WAIT_FOR_SPACE(cnt) \
{ \
	if ((Buffr.b_size - Buffr.b_cnt) < cnt) \
		bflush(); \
	wrap(cnt); \
}
#endif /* _REENTRANT */

/*
 * INCR_BUFCNT:  After adding data to buffer, this macro is invoked to update
 * the Buffr counters appropriately.
 *
 * For the multi-threaded version, to save on the number of mutex acquisitions,
 * the update is not actually made here.  We just save the amount in the global
 * variable Bufcnt_update.  The actual changes to the counts will be made in
 * WAIT_FOR_SPACE, when we already have the mutex anyway.
 *
 * For the single threaded version, where there is no mutex acquisition
 * involved, we just update the counts directly.
 */

#ifdef _REENTRANT
#define INCR_BUFCNT(cnt) \
{ \
	Bufcnt_update += (cnt); \
}
#else /* _REENTRANT */
#define INCR_BUFCNT(cnt) \
{ \
	Buffr.b_in_p += (cnt); \
	Buffr.b_cnt += (long)(cnt); \
}
#endif /* _REENTRANT */

/*
 * WAIT_FOR_DATA: Determine if there are enough bytes in the buffer to meet
 * current needs, if not, don't return until there is enough data.  For the
 * multi-threaded version, this means waiting on Buffr.b_take until
 * bfill_thread() gets enough data from the archive (unless the -D option was
 * used, in which case we read on block at a time via
 * bfill()).  For the non-multithreaded version, this means calling
 * bfill to fill the whole buffer.
 */

#ifdef _REENTRANT
#define WAIT_FOR_DATA(cnt)  \
{ \
	if (!Dflag) { \
		MUTEX_LOCK(&Buffr.b_mutex); \
		if (Bufcnt_update) { \
			/* \
			 * first take care of any accrued updates resulting \
			 * from calls to DECR_BUFCNT().  If this results in >= \
			 * Bufsize bytes of free space in the buffer, signal \
			 * the Buffr.b_put condition to let bfill_thread() know that \
			 * there's room to read data into the buffer. \
			 */ \
			Buffr.b_out_p += Bufcnt_update; \
			if (Buffr.b_size - (Buffr.b_cnt -= (long)Bufcnt_update) >= Bufsize && \
			    Buffr.b_waitforput) \
				(void)cond_signal(&Buffr.b_put); \
			Bufcnt_update = 0; \
		} \
	} \
	while (Buffr.b_cnt < (cnt)) { \
		if (Dflag) { \
			bfill(); \
		} else { \
			if (Eomflag) { \
				Eomflag++; \
				(void)cond_signal(&Buffr.b_endmedia); \
			} \
			Buffr.b_waitfortake = cnt; \
			(void)cond_wait(&Buffr.b_take, &Buffr.b_mutex); \
			Buffr.b_waitfortake = 0; \
		} \
	} \
	wrap(cnt); \
	if (!Dflag) MUTEX_UNLOCK(&Buffr.b_mutex); \
}
#else /* _REENTRANT */
#define WAIT_FOR_DATA(cnt) \
{ \
	while (Buffr.b_cnt < (cnt)) \
		bfill(); \
	wrap(cnt); \
}
#endif /* _REENTRANT */


/*
 * DECR_BUFCNT:  After taking data from buffer, this macro is invoked to update
 * the Buffr counters appropriately.  To save on the number of mutex
 * acquisitions, the update is not actually made here.  We just save the amount
 * in the global variable Bufcnt_update.  The actual changes to the counts will
 * be made in WAIT_FOR_DATA, when we already have the mutex anyway.
 *
 * Unless the -D option in used, in which case we are not multi-threaded.
 * Then we make the update immediately.
 */

#ifdef _REENTRANT
#define DECR_BUFCNT(cnt) \
{ \
	if (Dflag) { \
		Buffr.b_out_p += (cnt); \
		Buffr.b_cnt -= (long)(cnt); \
	} else { \
		Bufcnt_update += (cnt); \
	} \
}
#else /* _REENTRANT */
#define DECR_BUFCNT(cnt) \
{ \
	Buffr.b_out_p += (cnt); \
	Buffr.b_cnt -= (long)(cnt); \
}
#endif /* _REENTRANT */

/* Values for File_mode flag, used in xattr handling */
#define CREAT_ONLY	0x1	/* Only create the file; don't data_in() or rstfiles() (yet) */
#define CREAT_SUCCESS	0x2	/* Tells xattr_in() that the file was successfully created */
#define DATA_PROC	0x4	/* Tells xattr_in() to do data_in(P_PROC) after xattrs are applied */
#define DATA_SKIP	0x8	/* Tells xattr_in() to do data_in(P_SKIP) after xattrs are applied */
#define LNKS_OUT	0x10	/* Tells write_xattr_and_hdr()/xattr_out() to separate xattr file and header */

/*
 * VERBOSE: If x is non-zero, call verbose().
 */

#define VERBOSE(x, name) if (!(x) || ((File_mode & CREAT_ONLY) && !(Args & OCt))) ; else verbose(name)

/*
 * FORMAT: Date time formats
 * b - abbreviated month name
 * e - day of month (1 - 31)
 * H - hour (00 - 23)
 * M - minute (00 - 59)
 * Y - year as ccyy
 */

#define FORMAT	"%b %e %H:%M %Y"
#define FORMATID ":31"

#define INPUT	0	/* -i mode (used for chgreel() */
#define OUTPUT	1	/* -o mode (used for chgreel() */
#define APATH	1024	/* maximum ASC or CRC header path length */
#define CPATH	256	/* maximum -c and binary path length */
#define BUFSZ	512	/* default buffer size for archive I/O */
#define CPIOBSZ	4096	/* buffer size for file system I/O */
#define LNK_INC	500	/* link allocation increment */
#define MX_BUFS	10	/* max. number of buffers to allocate */

#define F_SKIP	0	/* an object did not match the patterns */
#define F_LINK	1	/* linked file */
#define F_EXTR	2	/* extract non-linked object that matched patterns */

#define MX_SEEKS	10	/* max. number of lseek attempts after error */
#define SEEK_ABS	0	/* lseek absolute */
#define SEEK_REL	1	/* lseek relative */

/*
 * xxx_CNT represents the number of sscanf items that will be matched
 * if the sscanf to read a header is successful.  If sscanf returns a number
 * that is not equal to this, an error occured (which indicates that this
 * is not a valid header of the type assumed.
 */

#define ASC_CNT	14	/* ASC and CRC headers */
#define CHR_CNT	11	/* CHR header */

/* Exit codes */

#define EXIT_OK		0	/* No problem */
#define EXIT_USAGE	1	/* Usage error */
#define EXIT_FILE	2	/* non-fatal File error */
#define	EXIT_FATAL	3	/* Fatal error */

/* These defines determine the severity of the message sent to the user. */

#define ER	1	/* Error message - no exit */
#define EXT	2	/* Error message - fatal - exit with EXIT_FATAL */
#define ERN	3	/* Error message with errno - no exit */
#define EXTN	4	/* Error message with errno - fatal - exit with EXIT_FATAL */
#define POST	5	/* Information message, not an error */
#define EPOST	6	/* Information message to stderr */
#define WARN	7	/* Warning to stderr */
#define WARNN	8	/* Warning to stderr with errno */
#define USAGE	9	/* Error message with usage message and exit */

#define SIXTH	060000	/* UNIX 6th edition files */

#define P_SKIP	0	/* File should be skipped */
#define P_PROC	1	/* File should be processed */

#define U_KEEP	0	/* Keep the existing version of a file (-u) */
#define U_OVER	1	/* Overwrite the existing version of a file (-u) */

/*
 * _20K: Allocate the maximum of (20K or (MX_BUFS * Bufsize)) bytes 
 * for the main I/O buffer.  Therefore if a user specifies a small buffer
 * size, they still get decent performance due to the buffering strategy.
 */

#define _20K	20480	

#define HALFWD	1	/* Pad headers/data to halfword boundaries */
#define FULLWD	3	/* Pad headers/data to word boundaries */
#define FULLBK	511	/* Pad headers/data to 512 byte boundaries */

/* search_access(path) */
#define SRCH_PRVS	pm_work(P_MACREAD), pm_work(P_DACREAD)
/* mac_access(f, WRITE) */
#define MACWR_PRVS	pm_work(P_MACWRITE)
#define ENDPRV		(priv_t) 0


/* ACLGET and ACLGETCNT */
#define ACLGET_PRVS	SRCH_PRVS
/* ACLSET */
#define ACLSET_PRVS	SRCH_PRVS, MACWR_PRVS, pm_work(P_OWNER)
#define CHMOD_PRVS	SRCH_PRVS, MACWR_PRVS, pm_work(P_OWNER)
#define STAT_PRVS	SRCH_PRVS
#define LVLF_PRVS	SRCH_PRVS
/* lvlproc() - MAC_SET */
#define LVLP_PRVS	pm_work(P_SETPLEVEL)
#define TSTMLD_PRVS	SRCH_PRVS
#define MKDIR_PRVS	SRCH_PRVS, MACWR_PRVS, pm_work(P_DACWRITE)
#define MKMLD_PRVS	pm_work(P_MULTIDIR), MKDIR_PRVS

/* The cpioinfo structure exists only to get "old style" stat info from
 * cpiostat.c to "new style" stat struct in cpio.c.  This intermediate
 * structure is needed because you can't have both new and old style stat
 * structures in the same file.
 */
struct cpioinfo {
	o_dev_t	st_dev;
	o_ino_t	st_ino;
	o_mode_t	st_mode;
	o_nlink_t	st_nlink;
	o_uid_t	st_uid;
	o_gid_t	st_gid;
	o_dev_t	st_rdev;
	off_t	st_size;
	time_t	st_modtime;
	time_t	st_actime;
};
