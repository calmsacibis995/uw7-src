#ident	"@(#)kern-i386at:io/hba/adsl/adsl.cf/Space.c	1.1.3.1"
#ident	"$Header: UnixWare 2.1 Adaptec 7800 family PCI SCSI driver V2.10S3/d2.01.2 with chimscsi-B1@000193_TO_VL0_SCBDMAv000, instrchim-ks06, uerrchim-0007,  09-30-97 VHu"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"
#include <sys/adsl.h>

int	adsl_gtol[SDI_MAX_HBAS];	/* global-to-local hba# mapping */
int	adsl_ltog[SDI_MAX_HBAS];	/* local-to-global hba# mapping */

struct	ver_no    adsl_sdi_ver;

#ifdef  ADSL_CPUBIND

#define ADSL_VER    HBA_UW21_IDATA | HBA_IDATA_EXT

struct  hba_ext_info    adsl_extinfo = {
    0, ADSL_CPUBIND
};

#else

#define ADSL_VER    HBA_UW21_IDATA

#endif  /* ADSL_CPUBIND */

struct	hba_idata_v5	_adslidata[]	= {
	{ ADSL_VER, "(adsl,1) Adaptec PCI SCSI",
	  ADSL_SCSI_ID, 0, -1, 10, 0, 0, 0 }
#ifdef ADSL_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&adsl_extinfo }
#endif
};

#ifndef ADSL_CNTLS
#define ADSL_CNTLS 1
#endif

int 	adsl_cntls = ADSL_CNTLS;


int	adsl_sg_merge = 1;	
		/* If this is set to 0, no local scatter/gather
	   	 * merge will be done in the driver however, there
	   	 * will be scatter/gather done by the kernel.
 	   	 * Default = 1  
		 */

char 	adsl_cmdque_enable = 1; 
		/* If this is set to 0, command tagged queue will
		 * be not implemented. 
		 * Default = 1 
		 */

char 	adsl_num_quecmds = 8;  
		/* Number of tagged queuing commands per target.
		 * Defauat = 8, Max. is 32, min. is 2 
		 */

char 	adsl_instr_enable = 1; 
		/* If this is set to 0, the instrumentation will
		 * be not implemented(trun off). 
		 * Default = 1 
		 */

char 	adsl_instr_ioperf = 0; 
		/* If this is set to 1, the instrumentation 
		 * io-performance will be implemented if 
		 * adsl_instr_enable is on. 
		 * Default = 0 
		 */

char 	adsl_instr_errlog = 32; 
		/* Number of error logs. Max = 32, min = 0. 
		 * Default = 32 
		 */

char 	adsl_mem_map = 0; 
		/* For all I/O used to access adapter register registers. 
		 * If Hardware offers choice,
		 * A value of 0 indicates io_mapped access.
		 * A value of 1 indicates memory_mapped access.
		 * Default = 0 
		 */

/*****************************************************************************
 *  NOTE:  The following variables control many functions of the adapter, but *
 * 	the variables will not be used unless the adsl_ha_reinit variable is set. 
 *	A message from the driver will report whether the following variables *
 *	will be used or not						      *
 *****************************************************************************/

char	adsl_ha_reinit[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	
		/*
		 * Setting this variable to a 1 will force
		 * the following fields changes to the adapter profile.
		 * Default = 0   
		 * The array of first byte is for Host Adapter 0 (A), 
		 * the second byte is for Host Adapter 1, and so on.
		 * NOTE: If more than one channel reside in the same 
		 * controller, it should assume per channel as per
		 * controller. But it should remain the same value for
		 * the shared controller.
		 */

char	adsl_target_mode = 0;
		/* Used to switch an adapter target mode on (1) or off (0).
		 * Default = 0   
		 */

char	adsl_initiator_mode = 1;
		/* Used to switch an adapter initiator mode on (1) or
		 * off (0). Can be used in conjunction with 
		 * adsl_target_mode to support a bi-directional mode.
 	   	 * Default = 1   
		 */

/*
 * This variable controls the data FIFO in the AIC-78xx.  
 * Percentage available for data xfer to start.
 *
 * Here are the meanings and effect this variable can have:
 * 100% - Host transfers much faster than SCSI transfers.
 *	The AIC-78xx will start transferring data from the FIFO to host
 *	memory as soon as it detects the FIFO is full for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as the FIFO is empty.
 * 75%  - Host transfers faster than SCSI transfers
 *	The AIC-78xx will start transferring data from the FIFO to host
 *	memory as soon as it detects the FIFO is 75% full for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as the FIFO is 75% empty.
 * 50%  - Host and SCSI transfer rates are nearly equal
 *	The AIC-78xx will start transferring data from the FIFO to host
 *	memory as soon as it detects the FIFO is 50% full for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as the FIFO is 50% empty.
 * 0%   - Host transfer rates are slower than SCSI
 *	The AIC-78xx will start transferring data from the FIFO to host
 *	memory as soon as data is available for READ commands.
 *	For WRITE commands, the data will be transferred into the FIFO as soon
 *	as there is room in the FIFO.
 */
unsigned char	adsl_threshold = 100;		
		/* Default = 100 */

unsigned char	adsl_adapterID[] = {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
		/* The variable setting is corespond to 
		 * adsl_ha_reinit array.
		 * Default = 7 
		 * NOTE: If more than one channel reside in the same 
		 * controller, it should assume per channel as per
		 * controller. But it should remain the same value for
		 * the shared controller.
		 */

unsigned char	adsl_adapter_nluns[] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16};
		/* The variable setting is corespond to 
		 * adsl_ha_reinit array.
		 * Default = 16, min = 1. 
		 * If you want disable the multiple luns, set it to 1.
		 * NOTE: If more than one channel reside in the same 
		 * controller, it should assume per channel as per
		 * controller. But it should remain the same value for
		 * the shared controller.
		 */

/*****************************************************************************
 *  NOTE:  The following variables control many functions of the target, but *
 * 	the variables will not be used unless the adsl_tg_reinit variable is set. 
 *	A message from the driver will report whether the following variables *
 *	will be used or not						      *
 *****************************************************************************/
char	adsl_tg_reinit = 0;	
	   /* Setting this variable to a 1 will force
	    * the following fields changes to the target profile
 	    * Default = 0   
		*/

unsigned short	adsl_scsi_speed = 200;			/* Default = 200 */
		/*
 		 * This array controls the rate that will be 
		 * negotiated for if synchronous
 		 * negotiation is enabled for the given target.  
		 * Value is in tenths of Mtransfers/second. 
 		 * Example: Values of 36, 40, 44, 50, 57, 67, 80, 100, 
		 *  which stands for
 		 *  3.6, 4.0, 4.4, 5.0, 5.7, 6.7, 8.0, 10.0 
		 *  mega-bytes per second 
 		 *  for the SCSI bus (non-Ultra).
		 * Values of 134, 160, 200, which stands for
		 *  13.4, 16, 20, mega-bytes per second 
 		 *  for the SCSI bus (Ultra).
		 * Values of 268, 320, 400, which stands for
		 *  26.8, 32, 40 mega-bytes per second 
 		 *  for the SCSI bus (UltraWide).
		 * A value of 0 indicates asynchronous transfer.
 		 * The default = 200 (20MB/sec) 
 		 */

unsigned char	adsl_scsi_width = 8;			
		/* The width, which attempts to negotiate with the target.
		 * Default = 8	
		 */
	
unsigned char	adsl_discnt_allowed = 1;			
		/* If 1, targets are allowed to disconnect.
		 * Default = 1		
		 */



/*
 * This variable sets the number of seconds to  suspend
 * the logical unit queues when a third party SCSI bus
 * reset occurs.
 */

int     adsl_3pty_reset_wait = 8;

/*
 * This variable sets the size of the READ/WRITE BUFFER used for target mode.
 */

#ifdef UW21
int	adsl_rwbufsize = ADSL_RWBUFSIZE;
#else
int	adsl_rwbufsize = 0x2000;
#endif

