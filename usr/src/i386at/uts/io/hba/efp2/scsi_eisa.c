#ident	"@(#)kern-pdi:io/hba/efp2/scsi_eisa.c	1.2"

#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <io/mkdev.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <svc/bootinfo.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/dynstructs.h>
#include <io/hba/efp2/efp2.h>
#include <util/mod/moddefs.h>

#else
#include "sys/errno.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/cmn_err.h"
#include "sys/buf.h"
#include "sys/systm.h"
#include "sys/mkdev.h"
#include "sys/conf.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/kmem.h"
#include "sys/bootinfo.h"
#include "sys/debug.h"
#include "sys/scsi.h"
#include "sys/sdi_edt.h"
#include "sys/sdi.h"
#include "sys/dynstructs.h"
#include "efp2.h"
#include "sys/moddefs.h"
#include "sys/ddi.h"
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif
#endif

#define	READ_CONF	0x41
#define	READ_FW_REV	0x42

#define	AT_CHAN		0x00	/* Only	EFP2 */
#define	AT_CONF		0x01
#define	IRQ_CONF	0x02
#define	BIOS_COMM	0x03	/* ESC-1, ESC-2 */
#define	ENVIR		0x03	/* EFP2 */

typedef
	struct	{
		uchar_t	ach_unused : 6,
			ach_drive0 : 1,	/* SCSI chan. of AT comp. drive #0 */
			ach_drive1 : 1;	/* SCSI chan. of AT comp. drive #1 */
	} at_chan_t;

typedef
	struct	{
		uchar_t	ac_enable : 1,	/* Enable AT mode */
			ac_drive0 : 3,	/* SCSI ID of AT comp. drive #0 */
			ac_drive1 : 3,	/* SCSI ID of AT comp. drive #1 */
			ac_unused : 1;
	} at_conf_t;

#define	NO_DRIVE	0x7

typedef
	struct	{
		uchar_t	irq_lev    : 2,	/* Alternate IRQ */
			irq_unused : 6;

	} irq_lev_t;

#define	IRQ11		0x00
#define	IRQ10		0x01
#define	IRQ05		0x02
#define	IRQ15		0x03

typedef
	struct	{
		uchar_t	e_id0   : 1,	/* ID on chan. #1 */
			e_id1   : 1,	/* ID on chan. #2 */
			e_envir : 1,	/* 1=single mirr, 0=dual mirr */
			e_disks : 4,
			e_mode  : 1;	/* 1=diag, 0=user */
	} environment_t;

/****************************************************************************/

#define	SEMINC		0xC8A
#define	SEMOUT		0xC8B
#define	BELLINC		0xC8D
#define	BELLOUT		0xC8F
#define	CMD_ADDR	0xC90
#define	REPLY_ADDR	0xC98

typedef
	struct {
		uchar_t	c_id;
		uchar_t	c_cmd;
		uchar_t	c_parm;
	} esccmd_t;

typedef
	struct {	
		uchar_t		r_id;
		uchar_t		r_unused;
		ushort_t	r_status;
		uchar_t		r_val;
	} reply_t;

/****************************************************************************/

/*
 *	Read configuration register using ESC-like protocol
 *
 *	Input:
 *		base - EISA base address	( 0xz000 )
 *		reg  - register to read 	( 0..3 )
 */
int
esc_getreg(base, reg)
int	base;
int	reg;
{
	int i;
	esccmd_t	cmd;
	reply_t	reply;
	caddr_t	cp;

	cmd.c_id = reg|0x80;	/* Just to set a value */
	cmd.c_cmd = READ_CONF;
	cmd.c_parm = reg;
		
	/* wait for previous command to be accepted */
	for (i = 100; i >= 0; i--) {
		outb(base+SEMINC, 0x01);
		if ((inb(base+SEMINC) & 0x03) == 0x01)
			break;

		if (i == 0) {
			cmn_err(CE_WARN, "SCSI: Semaphore is stuck (%0x)\n",base+SEMINC);
			return -1;
		}
		drv_usecwait(10);  /* wait 10 usec */
	}

	cp = (caddr_t) &cmd;

	/* write incoming mailbox */
	for (i = 0; i < sizeof(esccmd_t); i++) {
		outb((base+CMD_ADDR+i), *cp++);
	}

	/* send interrupt request to the adapter */
	outb(base+BELLINC, 0x80);
	
	/* wait for an answer */
	for (i=100; i >= 0; i--) {
		if (inb(base+BELLOUT) & 0x80)
			break;
		
		if (i == 0) {
			cmn_err(CE_WARN,"SCSI: No answer (%x)\n",base+BELLOUT);
			return -1;
		}
		drv_usecwait(ONE_THOU_U_SEC);	/* wait 1 milli-seconds */
	}

	outb(base+BELLOUT, 0x80);  /* acknowledge */

	/* read the outgoing mailbox */
	cp = (caddr_t) &reply;

	for (i = 0; i < sizeof(reply_t); i++)
		*cp++ = inb(base+REPLY_ADDR+i);

	/* release outgoing semaphore */
	outb(base+SEMOUT, 0x00);

	if (reply.r_status == 0)
		return reply.r_val;
	else
		return -1;
}

/****************************************************************************/

/*
 *	Check if the specified controller could be bootable
 *	( AT compatible enabled and valid SCSI ID for drive #0 )
 *
 *	Input:
 *		base  - EISA base address	( 0xz000 )
 *		ctype - controller type		( EFP2, ESC-1, ESC-2 )
 *
 *	Output:
 *		*p_id   - SCSI ID of drive #0
 *		*p_chan - SCSI chan. of drive #0
 *
 *	Return:
 *		 0	- Specified controller is not bootable
 *			  *p_id, *p_chan are meaningless
 *
 *		 1	- Specified controller is bootable
 *			  *p_id, *p_chan are meaningfull
 *
 *		-1	- Some errors encountered
 */

scsi_get_boot(base, ctype, p_id, p_chan)
int base, ctype, *p_id, *p_chan;
{
	union {
		uchar_t		byte;
		at_conf_t	conf;
		at_chan_t 	chan;
	} cf;
	int r;

	*p_id = *p_chan = -1;

	if ((r=esc_getreg(base, AT_CONF)) == -1)
		return -1;

	cf.byte = r & 0xFF;

	if (!cf.conf.ac_enable || cf.conf.ac_drive0 == NO_DRIVE)
		return 0;

	/* Bootable controller */
		
	*p_id = cf.conf.ac_drive0;

	if (ctype != OLI_EFP2) {
		*p_chan = 0;
		return 1;
	}

	if ((r=esc_getreg(base, AT_CHAN)) == -1)
		return -1;

	cf.byte = r & 0xFF;

	*p_chan = cf.chan.ach_drive0;

	return 1;
}
