#ifndef _IO_HBA_IDE_ATA_ATAPI_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IDE_ATA_ATAPI_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/ata/atapi.h	1.3.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * port offsets from base address in dcbp->dcb_ioaddr1, above.
 */
#define	ATP_DATA	0x00	/* data register */
#define	ATP_ERROR	0x01	/* error register/write precomp */
#define	ATP_FEAT	0x01	/* features */
#define ATP_INTR    0x02    /* interrupt reason */
#define	ATP_RES		0x03	/* reseved */
#define	ATP_LSB		0x04	/* byte Count least significant */
#define	ATP_MSB		0x05	/* byte count most significant */
#define ATP_DRVSEL  0x06    /* drive select */
#define	ATP_STATUS	0x07	/* status register */
#define	ATP_CMD		0x07	/* command register */

/*
 * Definition of ATP_INTR register.
 */
#define ATP_COD		0x01
#define ATP_IO		0x02

/*
 * Atapi signature presented in byte count.
 */
#define ATP_LSIG	0x14
#define ATP_MSIG	0xEB

/*
 * ATA commands used for ATAPI command codes.
 */
#define ATP_SOFTRST	0x08
#define ATP_PKTCMD	0xA0
#define ATP_IDENT	0xA1

/*
 * ATAPI packet size.
 */
#define ATP_PKTSZ	12

/*
 * Maximum programable PIO transfer is 64KB due to the 16 bit register.
 */
#define ATP_MAXPIO      32768

/*
 * In some ATAPI drives interrupts are missed, therefore timeouts are
 * used instead.
 */
#define ATP_MAXTIMEOUT	2

/*
 * ATAPI protocol states.
 */
enum atp_state { ATP_INVALID,
		 ATP_INITIAL,
		 ATP_SNDPKT,
		 ATP_GETDATA,
		 ATP_GETSTATUS };

/*
 * Size of the ATP_IDENT data
 */
struct atp_ident {
	int			atp_pktsz:2;
	int			atp_res0:3;
	int			atp_drq:2;
	int			atp_remove:1;
	int			atp_devtype:5;
	int			atp_res1:1;
	int			atp_protoc:2;
	ushort_t	fill1[9];
	uchar_t		atp_serial[20];
	ushort_t	atp_buftyp;
	ushort_t	atp_bufsz;
	ushort_t	fill2;
	uchar_t		atp_fw[8];
	uchar_t		atp_model[40];
	ushort_t	fill3[2];
	ushort_t	atp_capabil;
	ushort_t	fill4;
	ushort_t	atp_piocycle;
	ushort_t	atp_dmacycle;
	ushort_t	atp_valid;
	ushort_t	fill5[8];
	ushort_t	atp_other[7];
	ushort_t	fill6[187];
};

#define ATP_IDENTSZ 256	/* Size of the previous structure in words */

/*
 * Possible values for atp_drq.
 */
#define	ATP_MCDRQ	0x00	/* 3 ms delay before receiving Packet Command */
#define	ATP_INTDRQ	0x01	/* Packet is accepted when interrupt is asserted */
#define	ATP_ACCDRQ	0x02	/* Packet is accepted within 50 us */


#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_IDE_ATA_ATAPI_H */
