#ifndef _IO_HBA_IDE_ATA_ATA_HA_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IDE_ATA_ATA_HA_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/ata/ata_ha.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * low-level entry point defs (for space.c files)
 */

#ifdef __STDC__

struct ide_cfg_entry;
struct gdev_parm_block;
struct gdev_ctl_block;

extern int ata_find_ha();
extern int ata_bdinit(struct ide_cfg_entry *);
extern int ata_drvinit(int, struct ide_cfg_entry *);
extern int ata_getinfo(struct hbagetinfo *);
extern int ata_cmd(int, int, struct ide_cfg_entry *);
extern void ata_docmd(int, struct ide_cfg_entry *);
extern void ata_reset(struct ide_cfg_entry *, int);
extern void ata_setskew(struct ata_parm_entry *);
extern int ata_senddata(struct ide_cfg_entry *, struct ata_parm_entry *);
extern int ata_recvdata(struct ide_cfg_entry *, struct ata_parm_entry *);
extern void ata_send_cmd(struct ide_cfg_entry *, struct control_area *);
extern int ata_ata_cmd(struct control_area *, int);
extern int ata_atapi_cmd(struct control_area *, int);
extern void ata_cplerr(struct ide_cfg_entry *, struct control_area *);
extern struct ata_parm_entry *ata_int(struct ide_cfg_entry *);
extern void ata_int_enable(struct ide_cfg_entry *);
extern void ata_halt(struct ide_cfg_entry *);
#else /* __STDC__*/
extern int ata_bdinit();
extern int ata_cmd();
extern struct gdev_parm_block *ata_int();
/*
extern int ata_wait(), ata_DPTinit();
*/
extern int ata_drvinit(), ata_DPTinit();
extern ushort ata_err();
extern void  ata_reset(), ata_docmd(), ata_setskew();
extern int ata_senddata(), ata_recvdata();
#endif /* __STDC__*/

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_IDE_ATA_ATA_HA_H */
