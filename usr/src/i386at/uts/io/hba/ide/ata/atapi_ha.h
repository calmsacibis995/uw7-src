#ifndef _IO_HBA_IDE_ATA_ATAPI_HA_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IDE_ATA_ATAPI_HA_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/ata/atapi_ha.h	1.1.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Check for ATAPI signature.
 */
#define atp_isAtapi(cfgp)	((inb((cfgp)->cfg_ioaddr1+ATP_LSB) == ATP_LSIG) && \
 							 (inb((cfgp)->cfg_ioaddr1+ATP_MSB) == ATP_MSIG))

#define ata_intToAtasel(drv)	((drv) == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1)

extern int atapi_drvinit (int , struct ide_cfg_entry *);
extern struct ata_parm_entry *atapi_protocol (struct ide_cfg_entry *);
extern void atp_timeout(struct ide_cfg_entry *);

extern int atapi_delay;
extern int atapi_timeout;

extern boolean_t	ideinit_time;	    /* Init time (poll) flag	*/
#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_IDE_ATA_ATAPI_HA_H */
