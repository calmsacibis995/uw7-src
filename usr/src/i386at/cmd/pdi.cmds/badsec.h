
#ifndef _IO_TARGET_BADSEC_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_BADSEC_H	/* subject to change without notice */

#ident	"@(#)pdi.cmds:badsec.h	1.2"
#ident	"$Header$"

#define	BADSECFILE	"/etc/scsi/badsec"

#define	MAXBLENT	4
struct	badsec_lst {
	int	bl_cnt;
	struct	badsec_lst *bl_nxt;
	int	bl_sec[MAXBLENT];
};

#define BADSLSZ		sizeof(struct badsec_lst)

#endif /* _IO_TARGET_BADSEC_H */
