#ifndef _IO_HBA_MCESDI_MCESDI_H	/* wrapper symbol for kernel use */
#define _IO_HBA_MCESDI_MCESDI_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/mcesdi/mc_esdi.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * PS2/80 ESDI Hard disk controller definitions.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1986 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifdef __STDC__
extern int mcesdi_find_ha(struct ide_cfg_entry *, HBA_IDATA_STRUCT *);
extern int mcesdi_drvinit(int , struct ide_cfg_entry *);
extern int mcesdi_getinfo(struct hbagetinfo *);
extern int mcesdi_cmd(int , int , struct ide_cfg_entry *);
extern void mcesdi_send_cmd(struct ide_cfg_entry *, struct control_area *);
extern struct ata_parm_entry * mcesdi_int(struct ide_cfg_entry *);
extern void mcesdi_cplerr(struct ide_cfg_entry *, struct control_area *);
extern int mcesdi_bdinit(struct ide_cfg_entry *);
extern int mcesdi_drvinit(int, struct ide_cfg_entry *);
extern struct ata_parm_entry *mcesdi_int(struct ide_cfg_entry *);
extern void mcesdi_halt(struct ide_cfg_entry *);
extern void mcesdi_int_enable(struct ide_cfg_entry *);
#else /* __STDC__ */
extern int mcesdi_drvinit();
extern int mcesdi_bdinit(), mcesdi_cmd();
extern struct ata_parm_entry *mcedis_int();
#endif /* __STDC__ */


/* I/O register addresses */

#define ESDI_SIR	0x0	/* Status Interface Register */
#define ESDI_CIR	0x0	/* Command Interface Register */
#define ESDI_BSR	0x2	/* Basic Status Register */
#define ESDI_BCR	0x2	/* Basic Control Register */
#define ESDI_ISR	0x3	/* Interrupt Status Register */
#define ESDI_ATTN	0x3	/* Attention Register */
#define ESDI_DIR	0x4	/* Data Interface Register */

/* Status Interface Register */

#define WORD_COUNT	0xff00	/* Word Count */
#define WORD_SHIFT	8	/* Word Count Shift */
#define DEVICE_SELECT	0x00e0	/* Device Select Mask */
#define TEST_ERROR	0x001f	/* Test Error Code */

/* Basic Status Register */

#define DMAENAB		0x80	/* DMA Enable: 0-DMA disabled, 1-DMA enabled */
#define INTPND		0x40	/* Interrupt Pending */
#define CIP		0x20	/* Command In Progress */
#define BUSY		0x10	/* Busy */
#define STOUT		0x08	/* Status Out Register Is Full */
#define CMDIN		0x04	/* Command In Register Is Full */
#define XFER		0x02	/* Transfer Request */
#define IRPT		0x01	/* Interrupt */

/* Interrupt Status Register */

#define DEVSEL		0xe0	/* Device Select */
#define ADAPTER_ID	0xe0	/* Adapter Device Select ID */
#define FAULT		0x10	/* Adapter Error */
#define INTID		0x0f	/* Interrupt ID */
#define ID_OK		0x01	/* Success */
#define ID_ECC		0x03	/* Success w/ ECC */
#define ID_RETRY	0x05	/* Success w/ Retries */
#define ID_FORMAT	0x06	/* Format Partially Complete */
#define ID_ECC_RETRY	0x07	/* Success w/ ECC and Retries */
#define ID_WARNING	0x08	/* Warning */
#define ID_ABORT	0x09	/* Abort */
#define ID_RESET	0x0a	/* Reset */
#define ID_XFER		0x0b	/* Data Transfer Ready */
#define ID_FAILURE	0x0c	/* Failure */
#define ID_DMA_ERROR	0x0d	/* DMA Error */
#define ID_CB_ERROR	0x0e	/* Command Block Error */
#define ID_ATTN_ERROR	0x0f	/* Attention Error */


/* Basic Control Register */
#define RESET		0x80	/* Reset */
#define DMAEN		0x02	/* DMA Enable */
#define INTEN		0x01	/* Interrupt Enable */

/* Attention Register */

#define DEV_SEL		0xe0	/* Device Select */
#define ATTN_REQ	0x0f	/* Attention Request Code */
#define AR_COMMAND	0x01	/* Command Request */
#define AR_EOI		0x02	/* End Of Interrupt Request */
#define AR_ABORT	0x03	/* Abort Request */
#define AR_RESET	0x04	/* Reset Adapter */

/* Programmable Option Select (POS) Registers 0 and 1 */

#define ESDI_ID		0xddff	/* adapter ID */

/* Programmable Option Select (POS) Register 2 */

#define ARB_MODE	0x40	/* Fairness: 0=No Fairness, 1=Fairness */
#define ARB_LEVEL	0x3c	/* Arbitration Level */
#define ALT_ADDR	0x02	/* Alternate Address: 0=3510-17, 1=3518-1f */
#define CARD_ENABLE	0x01	/* Card Enable: 0=Disable, 1=Enable */

/* Programmable Option Select (POS) Register 3 */

#define MCADES		0x40	/* DMA Enable Select: 0=Reset DMA Enable on TC
						      1=Don't Reset DMA Enable */
#define BURST_LENGTH	0x30	/* Burst Length: 00=Burst Disbale
						 01=8 words
						 10=16 words
						 11=24 words */
#define ROM_DISABLE	0x08	/* ROM Disable: 0=Enable, 1=Disable */
#define ROM_SEGMENT	0x07	/* ROM Segment */

/* Command Block */

struct cb {
	unsigned short	command;
#define TYPE		0xc000	/* Type: 00=2 words, 01=4 words */
#define TYPE_SHIFT	14	/* Type Shift */
#define TYPE_2		0x0000	/* 2 words */
#define TYPE_4		0x4000	/* 4 words */
#define RESERVED2	0x0200	/* Reserved 2 */
#define RESERVED6	0x0600	/* Reserved 6 */
#define DEVICE		0x00e0	/* Device Mask */
#define DEVICE_SHIFT	5	/* Device Shift */
#define COMMAND		0x000f	/* Command Mask */
#define READ_DATA	0x01	/* Read Data */
#define WRITE_DATA	0x02	/* Write Data */
#define READ_VERIFY	0x03	/* Read Verify */
#define WRITE_VERIFY	0x04	/* Write Verify */
#define SEEK		0x05	/* Seek */
#define PARK_HEADS	0x06	/* Park Heads */
#define GET_CMD_STATUS	0x07	/* Get Command Completion Status */
#define GET_DEV_STATUS	0x08	/* Get Device Status */
#define GET_DEV_CFG	0x09	/* Get Device Configuration */
#define GET_POS_INFO	0x0a	/* Get POS Information */
#define TRANSLATE_RBA	0x0b	/* Translate RBA */
#define WRITE_BUFFER	0x10	/* Write Adapter Buffer */
#define READ_BUFFER	0x11	/* Read Adapter Buffer */
#define RUN_DIAG	0x12	/* Run Diagnostic Test */
#define GET_DIAG_SB	0x14	/* Get Diagnostic Status Block */
#define GET_MFG_HDR	0x15	/* Get MFG Header */
#define FORMAT_UNIT	0x16	/* Format Unit */
#define FORMAT_PREPARE	0x17	/* Format Prepare */
	unsigned short	blocks;
	unsigned long	RBA;
};

#define ESDI_SB_SIZE	8	/* Maximum size of ESDI Status Block accepted */


#define DMA_WR5		0x05	/* Write Operation on DMA Channel 5 */
#define DMA_RD5		0x09	/* Read Operation on DMA Channel 5 */

#define	HDTIMOUT	500000	/* how many 10usecs in a 1/4 sec.*/

#define NUMDRV  	2	/* maximum number of drives */
#define SECTOR_SIZE	512
#define MAX_CYLINDERS	1024
#define cylin		av_back

/* Values of hd_state */
#define HD_OPEN		0x01	/* drive is open */
#define HD_OPENING	0x02	/* drive is being opened */
#define HD_XFER		0x04	/* data transfer required */
#define HD_DO_FMT	0x08	/* track is being formatted */
#define HD_VTOC_OK	0x10	/* VTOC (pdinfo, vtoc, alts table) OK */
#define HD_BADBLK	0x40	/* bad block is being remapped */

#define DISKLIGHT_REG	0x92	/* The light that tells us there is */
				/*  disk activity 	            */

#define SUCCESS	1
#define FAILURE 0

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_MCESDI_MCESDI_H */
