#ident	"@(#)kern-i386at:io/hba/adsc/adsc.cf/Space.c	1.15.2.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"

int     adsc_gtol[SDI_MAX_HBAS];     /* global-to-local hba# mapping */
int     adsc_ltog[SDI_MAX_HBAS];     /* local-to-global hba# mapping */

int	adsc_hiwat = 2;			/* LU queue high water mark	*/

struct	ver_no    adsc_sdi_ver;

#ifndef ADSC_CNTLS
#define ADSC_CNTLS 1
#endif

#ifdef	ADSC_CPUBIND

#define	ADSC_VER	HBA_UW21_IDATA | HBA_IDATA_EXT

struct	hba_ext_info	adsc_extinfo = {
	0, ADSC_CPUBIND
};

#else

#define	ADSC_VER	HBA_UW21_IDATA

#endif	/* ADSC_CPUBIND */

struct	hba_idata_v5	_adscidata[]	= {
	{ ADSC_VER, "(adsc,1) Adaptec SCSI", 7, 0, -1, 0, -1, 0 }
#ifdef	ADSC_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&adsc_extinfo }
#endif
};

int	adsc_cntls	= ADSC_CNTLS;

/*
 * If this is set to 0, no local scatter/gather will be done in the driver
 * however, there will be scatter/gather done by the kernel.
 * Default = 1
 * RAN 03/31/92
 */
int	adsc_sg_enable = 1;

/*
 * This variable controls the amount of time the host adapter will spend on the
 * AT bus.  The value is in micro-seconds and should not be set higher than 8
 * to allow proper floppy operation.  It is also recommended the value not be
 * set below 4.  This variable has no effect on EISA host adapters.
 * Default = 7
 * RAN 03/31/92
 */
int	adsc_buson = 7;

/*
 * This variable controls the amount of time the host adapter will spend off
 * the AT bus after the bus on time has expired.  It should be set in
 * increments of 4, but not higher than 64.  This variable has no effect on
 * EISA host adapters.
 * Default = 4
 * RAN 03/31/92
 */
int	adsc_busoff = 4;

/*
 * This variable controls the host adapter DMA transfer rate.  By default
 * the host adapter can be set via jumpers to speeds 5, 5.7, 6.7, and 8.0
 * MBytes/sec.  The default jumper setting is 5.0MB/sec.  The following
 * settings are allowed via this variable that will override the jumper
 * settings.
 *	0 = 5.0MBytes/sec
 *	1 = 6.7MBytes/sec
 * 	2 = 8.0MBytes/sec
 *	3 = 10.0MBytes/sec
 *	4 = 5.7MBytes/sec (AHA-1540A and later)
 *
 * Optionally, the DMA rate can be adjusted to many different values by
 * setting the high bit (7) and using the following table.
 *		     Read      Write
 *		     Pulse     Pulse
 *		     Width     Width
 *	Bits	7    6 5 4  3  2 1 0	MBytes/sec
 *	------------------------------------------
 *		1    0 0 0  x  0 0 0	10.0
 *		1    0 0 1  x  0 0 1	 6.7
 *		1    0 1 0  x  0 1 0	 5.0
 *		1    0 1 1  x  0 1 1	 4.0
 *		1    1 0 0  x  1 0 0	 3.3
 *		1    1 0 1  x  1 0 1	 2.8
 *		1    1 1 0  x  1 1 0	 2.5
 *		1    1 1 1  x  1 1 1	 2.2
 *	------------------------------------------
 *	Bit 3 (x) sets the strobe pulse width.
 *			    0 = 100ns
 *			    1 = 150ns
 *	------------------------------------------
 *
 * The Read, Write, and Strobe pulse widths can be mixed and matched to get
 * various read/write rates.  For instance, say you wanted a read rate of
 * 6.7MB/sec and a write rate of 5.0MB/sec and a strobe width of 100 ns.
 * You would use 1 0 0 1 0 0 1 0 (remember the high bit must be set).
 * 10010010 = 146 (decimal) or 0x92 (hex).
 * You should know setting this to a rate you have not tested in your system
 * may result in a non-operational kernel.  The best way to determine how fast
 * your system can allow the host adapter DMA rate can be is to use a floppy,
 * set the DMA rate jumper on the adapter, run debug and execute (g=dc00:9)
 * in the BIOS of the adapter (assuming the BIOS is addressed at dc00:9).
 * If this test passes, you have some assurance the system will run at the
 * rate you jumpered the adapter to.  If it fails, do not attempt to alter
 * this value to a higher value.  This variable has no effect on EISA host
 * adapters.
 * Default = 0 (5.0MB/sec)
 * RAN 03/31/92
 */
int	adsc_dma_rate = 0;
