#ident	"@(#)kern-i386at:io/hba/ide/ide.cf/Space.c	1.8.3.1"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"
#include <sys/ide.h>
#include <sys/ide_ha.h>
#include <sys/ata_ha.h>
#include <sys/mc_esdi.h>

int		ide_gtol[SDI_MAX_HBAS];	/* global-to-local hba# mapping */
int		ide_ltog[SDI_MAX_HBAS];	/* local-to-global hba# mapping */

struct	ver_no    ide_sdi_ver;

#ifndef	IDE_CNTLS
#define IDE_CNTLS 1
#endif

#ifdef	IDE_CPUBIND

#define	IDE_VER	HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	ide_extinfo = {
	0, IDE_CPUBIND
};

#else

#define	IDE_VER	HBA_UW21_IDATA

#endif	/* IDE_CPUBIND */

struct	hba_idata_v5	_ideidata[]	= {
	{ IDE_VER, "(ide,1) Generic ESDI/IDE/ATAPI", 7, 0, -1, 0, -1, 0 }
#ifdef	IDE_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&ide_extinfo }
#endif
};

int ide_cntls = IDE_CNTLS;

/* ata_waitsecs is the maximum number of seconds ata_wait will wait for  */
/* the desired status prior to returning an error. A value of 10 should be */
/* sufficient for any controller/drive, but some slower drives might need  */
/* longer in a worst case scenario during a reset of the controller/drive. */

int	ide_waitsecs = 10;


/** Number of seconds idehalt() will **/
/** "delay" the system shutdown. This **/
/** allows disk drives and controllers **/
/** time to flush any onboard caches   **/

int	ide_halt_delay = 2;

int	atapi_delay=2000;
int	atapi_timeout=1000;

struct  ide_cfg_entry ide_table[] = {

	{
	'I','D','E','1',0x00,
	"(ata,1) ISA (MFM,RLL,ESDI,ATA)",	/* Controller Name */
	(CCAP_PIO | CCAP_RETRY | CCAP_ERRCOR | CCAP_NOSEEK),  /* capabilities */
	0x0,	/* Primary memory address */
	0x0,	/* Secondary memory address */
	0x1f0,	/* Primary I/O address */
	0x3f0,	/* Secondary I/O address */
	0,	/* Primary DMA Channel */
	255,	/* Max # of sector transfer count */
	0,	/* Up to 2 ATA drives */
	0,	/* Up to 2 ATAPI drives */
	10,	/* 100us drive switch delay */
	512,	/* Default sector size */
	ata_find_ha,	/* find board function */
	ata_drvinit,	/* init drive function */
	ata_getinfo,	/* getinfo function */
	ata_cmd,	/* command function */
	ata_send_cmd,	/* atapi command function */
	NULL,	/* open function */
	NULL,	/* close function */
	ata_int,	/* Interrupt function */
	ata_cplerr,	/* Completion function */
	ata_int_enable,	/* Interrupt enable function */
	ata_halt,	/* halt function */
	},
	{
	'I','D','E','2',0x00,
	"(ata,2) ISA (MFM,RLL,ESDI,ATA)",	/* Controller Name */
	(CCAP_PIO | CCAP_RETRY | CCAP_ERRCOR | CCAP_NOSEEK),  /* capabilities */
	0x0,	/* Primary memory address */
	0x0,	/* Secondary memory address */
	0x170,	/* Primary I/O address */
	0x370,	/* Secondary I/O address */
	0,	/* Primary DMA Channel */
	255,	/* Max # of sector transfer count */
	0,	/* Up to 2 ATA drives */
	0,	/* Up to 2 ATAPI drives */
	10,	/* 100us drive switch delay */
	512,	/* Default sector size */
	ata_find_ha,	/* find board function */
	ata_drvinit,	/* init drive function */
	ata_getinfo,	/* getinfo function */
	ata_cmd,	/* command function */
	ata_send_cmd,	/* atapi command function */
	NULL,	/* open function */
	NULL,	/* close function */
	ata_int,	/* Interrupt function */
	ata_cplerr,	/* Completion function */
	ata_int_enable,	/* Interrupt enable function */
	ata_halt,	/* halt function */
	},
	{
	'M','C','E','1',0x00,
	"(mcesdi,1) ISA (MFM,RLL,ESDI,ATA)",	/* Controller Name */
	(CCAP_PIO | CCAP_RETRY | CCAP_ERRCOR | CCAP_NOSEEK),  /* capabilities */
	0x0,	/* Primary memory address */
	0x0,	/* Secondary memory address */
	0x1f0,	/* Primary I/O address */
	0x3f0,	/* Secondary I/O address */
	0,	/* Primary DMA Channel */
	255,	/* Max # of sector transfer count */
	0,	/* Up to 2 ATA drives */
	0,	/* Up to 2 ATAPI drives */
	10,	/* 100us drive switch delay */
	512,	/* Default sector size */
	mcesdi_find_ha,	/* find board function */
	mcesdi_drvinit,	/* init drive function */
	mcesdi_getinfo,	/* getinfo function */
	mcesdi_cmd,	/* command function */
	mcesdi_send_cmd,/* atapi command function */
	NULL,	/* open function */
	NULL,	/* close function */
	mcesdi_int,	/* Interrupt function */
	mcesdi_cplerr,	/* Completion function */
	mcesdi_int_enable,	/* Interrupt enable function */
	mcesdi_halt,	/* halt function */
	},
};

int ide_table_entries = sizeof(ide_table)/sizeof(struct ide_cfg_entry);
