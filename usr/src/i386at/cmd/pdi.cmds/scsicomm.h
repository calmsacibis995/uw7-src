#ifndef _IO_TARGET_SCSICOMM_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SCSICOMM_H	/* subject to change without notice */

#ident	"@(#)pdi.cmds:scsicomm.h	1.3.3.1"
#ident	"$Header$"

#define	TRUE		1
#define	FALSE		0
#define	NORMEXIT	0
#define	ERREXIT		-1
#define	REMOTE(x)	((x) & 2)

#define	MNTTAB		"/etc/mnttab"
#define	MAXFIELD	64
#define	MAXMAJNUMS	7
#define	MAXMINOR	255
#define	MFIELDS		6	/* number of fields in a mdevice file */
#define	NDISKPARTS	2	/* # of disk partitions per mirror partition */
#define NO_MAJOR	-1
#define TEMPNODE	"/dev/scsiXXXXXX"

extern struct drv_majors *GetDriverMajors(char *, int *);
extern void		error();
extern void		warning();
extern int		get_chardevice(char *, char *);
extern int		get_blockdevice(char *, char *);
extern short		scl_swap16(unsigned long);
extern long		scl_swap24(unsigned long);
extern long		scl_swap32(unsigned long);

#define LUN(x)		(x & 0x03)

#define TC(x)		((x >> 2) & 0x07)

#endif /* _IO_TARGET_SCSICOMM_H */
