#ident	"@(#)mfpdhwrtns.c	1.1"

/*
 * Hardware related routines.
 */

#ifdef _KERNEL_HEADERS

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <io/mfpd/mfpd.h>
#include <io/mfpd/mfpdhw.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#elif defined(_KERNEL)

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/debug.h>
#include <sys/moddefs.h>
#include <sys/mfpd.h>
#include <sys/mfpdhw.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif /* _KERNEL_HEADERS */


#ifdef STATIC
#undef STATIC
#endif
#define STATIC



unsigned pc87322index, pc87322data; /* Index and data reg. addresses of
				      PC87322VF chip */
unsigned aip82091index, aip82091data; /* Index and data reg. addresses of
				        AIP82091 chip */



extern int	num_mfpd;
extern struct mfpd_cfg mfpdcfg[];



STATIC void mfpd_default_cntl_reg(unsigned long);
STATIC void mfpd_sl82360_enable_config(void);
STATIC void mfpd_sl82360_disable_config(void);
STATIC void mfpd_smcfdc665_enable_config(void);
STATIC void mfpd_smcfdc665_disable_config(void);




/* Forward declarations */

STATIC int mfpd_cent_init(unsigned long);
STATIC int mfpd_cent_enable_intr(unsigned long);
STATIC int mfpd_cent_disable_intr(unsigned long);
STATIC int mfpd_cent_get_fifo_threshold(unsigned long, int *, int *);
STATIC int mfpd_cent_get_status(unsigned long, int, unsigned long *);
STATIC int mfpd_cent_select_mode(unsigned long, int, unsigned long);

STATIC int mfpd_ps_2_init(unsigned long);
STATIC int mfpd_ps_2_enable_intr(unsigned long);
STATIC int mfpd_ps_2_disable_intr(unsigned long);
STATIC int mfpd_ps_2_get_status(unsigned long, int, unsigned long *);
STATIC int mfpd_ps_2_select_mode(unsigned long, int, unsigned long);

STATIC int mfpd_pc87322_init(unsigned long);
STATIC int mfpd_pc87322_get_status(unsigned long, int, unsigned long *);
STATIC int mfpd_pc87322_select_mode(unsigned long, int, unsigned long);

STATIC int mfpd_sl82360_init(unsigned long);
STATIC int mfpd_sl82360_get_status(unsigned long, int, unsigned long *);
STATIC int mfpd_sl82360_select_mode(unsigned long, int, unsigned long);

STATIC int mfpd_aip82091_init(unsigned long);
STATIC int mfpd_aip82091_get_fifo_threshold(unsigned long, int *, int *);
STATIC int mfpd_aip82091_get_status(unsigned long, int, unsigned long *);
STATIC int mfpd_aip82091_select_mode(unsigned long, int, unsigned long);


STATIC int mfpd_smcfdc665_init(unsigned long);
STATIC int mfpd_smcfdc665_get_fifo_threshold(unsigned long, int *, int *);
STATIC int mfpd_smcfdc665_get_status(unsigned long, int, unsigned long *);
STATIC int mfpd_smcfdc665_select_mode(unsigned long, int, unsigned long);


STATIC int mfpd_compaq_init(unsigned long);
STATIC int mfpd_compaq_get_status(unsigned long, int, unsigned long *);
STATIC int mfpd_compaq_select_mode(unsigned long, int, unsigned long);


/* 
 * Initializing mhr array:
 * None of these pointers may be NULL.
 *
 * For each chip set supported, a number is assigned in mfpd.h (0, 1, 2 ...
 * in order). This order should correspond with the order in which
 * the mhr array is initialized.
 *
 */

struct mfpd_hw_routines mhr[] = {

{ 
	(int (*)(unsigned long))mfpd_cent_init, 
	(int (*)(unsigned long))mfpd_cent_enable_intr, 
	(int (*)(unsigned long))mfpd_cent_disable_intr, 
	(int (*)(unsigned long, int *, int *))mfpd_cent_get_fifo_threshold, 
	(int (*)(unsigned long, int, unsigned long *))mfpd_cent_get_status, 
	(int (*)(unsigned long, int, unsigned long))mfpd_cent_select_mode
 }, 
{ 
	(int (*)(unsigned long))mfpd_ps_2_init, 
	(int (*)(unsigned long))mfpd_ps_2_enable_intr, 
	(int (*)(unsigned long))mfpd_ps_2_disable_intr, 
	(int (*)(unsigned long, int *, int *))mfpd_cent_get_fifo_threshold, 
	(int (*)(unsigned long, int, unsigned long *))mfpd_ps_2_get_status, 
	(int (*)(unsigned long, int, unsigned long))mfpd_ps_2_select_mode
 }, 
{ 
	(int (*)(unsigned long))mfpd_pc87322_init, 
	(int (*)(unsigned long))mfpd_cent_enable_intr, 
	(int (*)(unsigned long))mfpd_cent_disable_intr, 
	(int (*)(unsigned long, int *, int *))mfpd_cent_get_fifo_threshold, 
	(int (*)(unsigned long, int, unsigned long *))mfpd_pc87322_get_status, 
	(int (*)(unsigned long, int, unsigned long))mfpd_pc87322_select_mode
 }, 
{ 
	(int (*)(unsigned long))mfpd_sl82360_init, 
	(int (*)(unsigned long))mfpd_cent_enable_intr, 
	(int (*)(unsigned long))mfpd_cent_disable_intr, 
	(int (*)(unsigned long, int *, int *))mfpd_cent_get_fifo_threshold, 
	(int (*)(unsigned long, int, unsigned long *))mfpd_sl82360_get_status, 
	(int (*)(unsigned long, int, unsigned long))mfpd_sl82360_select_mode
 }, 
{ 
	(int (*)(unsigned long))mfpd_aip82091_init, 
	(int (*)(unsigned long))mfpd_cent_enable_intr, 
	(int (*)(unsigned long))mfpd_cent_disable_intr, 
	(int (*)(unsigned long, int *, int *))mfpd_aip82091_get_fifo_threshold, 
	(int (*)(unsigned long, int, unsigned long *))mfpd_aip82091_get_status, 
	(int (*)(unsigned long, int, unsigned long))mfpd_aip82091_select_mode
 }, 
{ 
	(int (*)(unsigned long))mfpd_smcfdc665_init, 
	(int (*)(unsigned long))mfpd_cent_enable_intr, 
	(int (*)(unsigned long))mfpd_cent_disable_intr, 
	(int (*)(unsigned long, int *, int *))mfpd_smcfdc665_get_fifo_threshold, 
	(int (*)(unsigned long, int, unsigned long *))mfpd_smcfdc665_get_status, 
	(int (*)(unsigned long, int, unsigned long))mfpd_smcfdc665_select_mode
 }, 
{ 
	(int (*)(unsigned long))mfpd_compaq_init, 
	(int (*)(unsigned long))mfpd_cent_enable_intr, 
	(int (*)(unsigned long))mfpd_cent_disable_intr, 
	(int (*)(unsigned long, int *, int *))mfpd_cent_get_fifo_threshold, 
	(int (*)(unsigned long, int, unsigned long *))mfpd_compaq_get_status, 
	(int (*)(unsigned long, int, unsigned long))mfpd_compaq_select_mode
 } };



#if (MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20)
/* ddicheck complains if these are not declared */
extern void	outb(int, unsigned char);
extern unsigned char	inb(int);
#endif /* MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20 */


/*
 * Public interface routines for mhr struct, so mhr does not have to be global.
 * Other drivers should use these routines instead of accessing the mhr struct
 * directly.
 */

int (*mfpd_mhr_init(unsigned long index))(unsigned long)
{
	return mhr[index].init;
}


int (*mfpd_mhr_enable_intr(unsigned long index))(unsigned long)
{
	return mhr[index].enable_intr;
}


int (*mfpd_mhr_disable_intr(unsigned long index))(unsigned long)
{
	return mhr[index].disable_intr;
}


int (*mfpd_mhr_get_fifo_threshold(unsigned long index))(unsigned long, int *, int*)
{
	return mhr[index].get_fifo_threshold;
}


int (*mfpd_mhr_get_status(unsigned long index))(unsigned long, int, unsigned long *)
{
	return mhr[index].get_status;
}


int (*mfpd_mhr_select_mode(unsigned long index))(unsigned long, int, unsigned long)
{
	return mhr[index].select_mode;
}


/* 
 * mfpd_default_cntl_reg()
 * 	Writes a default value to the control register.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */
STATIC void
mfpd_default_cntl_reg(unsigned long port_no)
{
	unsigned char	cntl_reg;

	/*
	 * PS/2 (Type 1) requires bits 6 and 7 to be zeros (reserved bits ).
	 * EPP mode requires bits 0, 1 and 3 to be zeros. 
	 * Fast mode of SL82360 requires bits 0, 1, 3, 6 and 7 to be zeros.
         * The following assignment is fine for all modes and for all
         * chip sets supported.
   	 */

	cntl_reg = 0;
	cntl_reg |= STD_INTR_ON;
	cntl_reg |= STD_RESET;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
	return;
}



/*
 * mfpd_get_options_avail()
 * 	Returns the data xfer options available for the port.
 *
 * Calling/Exit State :
 *
 * Description :
 *	The data xfer options available depends on the mode in which 
 * 	the port is in. So this routine should be called after setting
 *	the mode appropriately.
 *	direction is either MFPD_FORWARD_CHANNEL or MFPD_REVERSE_CHANNEL.
 *
 */

unsigned long	
mfpd_get_options_avail(unsigned long port_no, int direction)
{
	unsigned long	option;

	if (port_no >= num_mfpd)
		return MFPD_NO_OPTION;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return MFPD_NO_OPTION;

	option = 0;
	switch (mfpdcfg[port_no].mfpd_type) {

	case MFPD_SIMPLE_PP:
		if (direction == MFPD_FORWARD_CHANNEL) {
			option |= MFPD_BYTE_FORWARD;
		} else {
			option |= MFPD_NIBBLE_REVERSE;
		}
		break;

	case MFPD_PS_2:     /* FALL THROUGH */

	case MFPD_SL82360:
		if (direction == MFPD_FORWARD_CHANNEL) {
			option |= MFPD_BYTE_FORWARD;
		} else {
			option |= MFPD_BYTE_REVERSE;
		}
		break;

	case MFPD_PC87322:
		if (direction == MFPD_FORWARD_CHANNEL) {
			option |= MFPD_BYTE_FORWARD;
		} else {
			option |= MFPD_BYTE_REVERSE;
		}
		break;

	case MFPD_AIP82091: /* FALL THROUGH */
	case MFPD_SMCFDC665:
		if ((mfpdcfg[port_no].cur_mode & MFPD_ECP_MODE) || 
		    (mfpdcfg[port_no].cur_mode & MFPD_ECP_CENTRONICS)) {
			if (direction == MFPD_FORWARD_CHANNEL) {
				option |= MFPD_BYTE_FORWARD;
				option |= MFPD_DMA_FORWARD;
			} else {
				if (mfpdcfg[port_no].cur_mode & 
				    MFPD_ECP_CENTRONICS) {
					option = MFPD_NO_OPTION;
				} else {
					option |= MFPD_BYTE_REVERSE;
					option |= MFPD_DMA_REVERSE;
				}
			}
		} else {
			if (direction == MFPD_FORWARD_CHANNEL) {
				option |= MFPD_BYTE_FORWARD;
			} else {
				if (mfpdcfg[port_no].cur_mode & 
				    MFPD_CENTRONICS) {
					option = MFPD_NIBBLE_REVERSE;
				} else {
					option |= MFPD_BYTE_REVERSE;
				}
			}
		}
		break;

	case MFPD_COMPAQ:
		if (direction == MFPD_FORWARD_CHANNEL) {
			option |= MFPD_BYTE_FORWARD;
			option |= MFPD_DMA_FORWARD;
		} else {
			option |= MFPD_BYTE_REVERSE;
		}
		break;

	default :
		option = MFPD_NO_OPTION;
		break;

	}

	return option;
}


/*
 * mfpd_set_capability()
 * 	Sets the capability field in the mfpd_cfg structure.
 *
 * Calling/Exit State:
 *
 * Description:
 * 	The port type (mfpd_type field) should be set before this is
 *	called.
 */

void
mfpd_set_capability(unsigned long port_no)
{

	if (port_no >= num_mfpd)
		return;

/*
 * The various capabilities supported by the different chip sets:
 * 
 * #define MFPD_CENTRONICS         0x01     Centronics mode : Output only 
 * #define MFPD_BIDIRECTIONAL      0x02     Supports both input and output 
 * #define MFPD_EPP_MODE           0x04     supports EPP
 * #define MFPD_ECP_MODE           0x08     supports ECP 
 * #define MFPD_ECP_CENTRONICS	   0x10     Centronics while in ECP mode 
 * #define MFPD_HW_COMP            0x20     supports hardware compression 
 * #define MFPD_HW_DECOMP          0x40     supports hardware decompression 
 * #define MFPD_PROPRIETARY        0x80     proprietary 
 * 
 * The values below are obtained by ORing the capabilities supported by a chip
 * set. For example, a port of type MFPD_PC87322 has bidirectional data tranfer
 * capability, supports EPP mode (version 1.7) and a proprietary mode (EPP 1.9).
 */

	switch (mfpdcfg[port_no].mfpd_type) {

	case MFPD_SIMPLE_PP:
		mfpdcfg[port_no].capability = 0x01;
		break;
	case MFPD_PS_2 :
		mfpdcfg[port_no].capability = 0x02;
		break;
	case MFPD_PC87322:
		mfpdcfg[port_no].capability = 0x86;
		break;
	case MFPD_SL82360:
		mfpdcfg[port_no].capability = 0x82;
		break;
	case MFPD_AIP82091:
		mfpdcfg[port_no].capability = 0x1F;
		break;
	case MFPD_SMCFDC665:
		mfpdcfg[port_no].capability = 0x5F;
		break;
	case MFPD_COMPAQ:
		mfpdcfg[port_no].capability = 0x02;
		break;
	default :
		break;

	}
	return;
}


/* 
 * Routines for two most commonly used types of parallel ports follow.
 * These types are :
 * 1) MFPD_SIMPLE_PP ( Unidirectional. Also known as AT_mode)
 * 2) MFPD_PS_2      ( Bidirectional)
 * 
 * Note 1: 
 * Reserved bits in the registers of PS/2 parallel port pose a peculiar
 * problem. They must be written as 0's but they read as 1's !
 * So we use a mask to reset the reserved bits whenever a register is 
 * written.
 * This step does not seem necessary for parallel ports on SuperI/O 
 * chips.
 */


/*
 * Simple parallel port (centronix) routines.
 */

/* 
 * mfpd_cent_init()
 * 	Initializes the parallel port. Puts it in the output mode.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int 
mfpd_cent_init(unsigned long port_no)
{
	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	/* Only one mode possible : MFPD_CENTRONICS */
	mfpdcfg[port_no].cur_mode = MFPD_CENTRONICS;
	mfpd_default_cntl_reg(port_no);
	/* Nothing need to be done to put it in the output mode */

	return 0;
}


/* 
 * mfpd_cent_enable_intr()
 * 	Enables the interrupt.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int 
mfpd_cent_enable_intr(unsigned long port_no)
{
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= PS_2_CNTL_MASK;
	cntl_reg |= STD_INTR_ON;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);

	return 0;
}


/* 
 * mfpd_cent_disable_intr()
 * 	Disables the interrupt.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int 
mfpd_cent_disable_intr(unsigned long port_no)
{
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= PS_2_CNTL_MASK;
	cntl_reg &= ~STD_INTR_ON;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);

	return 0;
}


/* 
 * mfpd_cent_get_fifo_threshold()
 * 	Returns the fifo threshold in both directions.
 *
 * Calling/Exit State :
 * 	ffd and frd point to the FIFO thresholds in forward and reverse 
 *	directions.
 *
 * Description :
 *
 */

STATIC int 
mfpd_cent_get_fifo_threshold(unsigned long port_no, int *ffd, int *frd)
{
	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;
	*ffd = 0;
	*frd = 0;
	return 0;
}



/* 
 * mfpd_cent_get_status()
 * 	Get some port specific information.
 *
 * Calling/Exit State :
 * 	If flag is MFPD_DIRECTION, then *param will be set to the 
 * 	current parallel port direction (output/input).
 *
 */

STATIC int 
mfpd_cent_get_status(unsigned long port_no, int flag, unsigned long *param)
{
	int	retval = 0;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		*param = MFPD_FORWARD_CHANNEL;
		break;

	default :
		retval = -1;
		break;
	}
	return retval;
}


/* 
 * mfpd_cent_select_mode()
 * 	Does some chip configuration.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	If the flag is MFPD_DIRECTION, then the chip set is put in the
 * 	output/input mode depending on param.
 *	If the flag is MFPD_CAPABILITY, then the chip set is put in the
 *	mode specified by param.
 *
 */

STATIC int 
mfpd_cent_select_mode(unsigned long port_no, int flag, unsigned long param)
{
	int	retval = 0;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		if (param == MFPD_FORWARD_CHANNEL) {
			retval = 0;
		} else {
			retval = -1;
		}
		break;

	case MFPD_CAPABILITY :
		if (param & MFPD_CENTRONICS) {
			mfpdcfg[port_no].cur_mode = MFPD_CENTRONICS;
			mfpd_default_cntl_reg(port_no);
			retval = 0;
		} else {
			retval = -1;
		}
		break;

	default :
		retval = -1;
		break;
	}
	return retval;
}






/*
 * PS/2 routines  
 */



/* 
 * mfpd_ps_2_init()
 * 	Initializes the parallel port. Puts it in output mode.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int 
mfpd_ps_2_init(unsigned long port_no)
{
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	/* Only one mode possible : MFPD_BIDIRECTIONAL */
	mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
	mfpd_default_cntl_reg(port_no);

	/* Put it in the output mode */
	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= PS_2_CNTL_MASK;
	cntl_reg &= ~STD_INPUT;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
	return 0;
}




/* 
 * mfpd_ps_2_enable_intr()
 * 	Enables the interrupt.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int 
mfpd_ps_2_enable_intr(unsigned long port_no)
{
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= PS_2_CNTL_MASK;
	cntl_reg |= STD_INTR_ON;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);

	return 0;
}


/* 
 * mfpd_ps_2_disable_intr()
 * 	Disables the interrupt.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int 
mfpd_ps_2_disable_intr(unsigned long port_no)
{
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= PS_2_CNTL_MASK;
	cntl_reg &= ~STD_INTR_ON;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);

	return 0;
}



/* 
 * mfpd_ps_2_get_status()
 * 	Get some port specific information.
 *
 * Calling/Exit State :
 * 	If flag is MFPD_DIRECTION, then *param will be set to the 
 * 	current parallel port direction (output/input).
 *
 */


STATIC int 
mfpd_ps_2_get_status(unsigned long port_no, int flag, unsigned long *param)
{
	int	retval = 0;
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		cntl_reg &= STD_INPUT;
		if (cntl_reg) {
			*param = MFPD_REVERSE_CHANNEL;
		} else {
			*param = MFPD_FORWARD_CHANNEL;
		}
		retval = 0;
		break;

	default :
		retval = -1;
		break;

	}
	return retval;
}


/* 
 * mfpd_ps_2_select_mode()
 * 	Does some chip configuration.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	If the flag is MFPD_DIRECTION, then the chip set is put in the
 * 	output/input mode depending on param.
 *	If the flag is MFPD_CAPABILITY, then the chip set is put in the
 *	mode specified by param.
 *
 */

STATIC int 
mfpd_ps_2_select_mode(unsigned long port_no, int flag, unsigned long param)
{
	int	retval = 0;
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		cntl_reg &= PS_2_CNTL_MASK;
		if (param == MFPD_FORWARD_CHANNEL) {
			cntl_reg &= ~STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
		} else {
			cntl_reg |= STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
		}
		retval = 0;
		break;

	case MFPD_CAPABILITY :
		if (param & MFPD_BIDIRECTIONAL) {
			mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
			mfpd_default_cntl_reg(port_no);
			retval = 0;
		} else {
			retval = -1;
		}
		break;

	default :
		retval = -1;
		break;
	}
	return retval;
}


/*
 * NATIONAL Semiconductor PC87322VF (SuperI/O III) routines.
 *
 * Bits that are of interest to the parallel port in the configuration
 * registers of the chip :
 *
 * Function Enable Register(FER):
 *	Bit 0: Parallel port enable.
 *
 * Function Address Register(FAR):
 *	Bits 0 & 1: Determine the parallel port address.
 *
 * Power and Test Register(PTR):
 * 	Bit 3: Controls IRQ level for LPT2.
 *	Bit 7: Compatible/Extended mode bit.
 *
 * Function Control Register(FCR):
 * 	Bit 2: Controls Parallel Port Multiplexor.
 *	Bits 5, 6 & 7: Relevant when in EPP mode.
 *
 * Printer Control Register(PCR):
 * 	Bits 0, 1 & 4: Relevant in EPP mode.
 *
 */


/*
 * mfpd_determine_pc87322_address()
 * 	This determines the index and the data register addresses for
 *	pc87322 and sets two global variables. It is ASSUMED that the
 * 	chip has not been read (after reset) before.
 *
 * Calling/Exit State :
 * 	Sets two global variables : pc87322index and pc87322data.
 *
 * Description :
 *
 */
 
int
mfpd_determine_pc87322_address(void)
{
	unsigned char	ch1, ch2;

	/* 
	 * There are two possible addresses for index and data registers.
	 * Only at the correct address, the values read will match the
	 * default values.
	 */

	ch1 = inb(PC87322_INDEX1);
	ch2 = inb(PC87322_DATA1);
	if ((ch1 == PC87322_DEFAULT_INDEX) && (ch2 == PC87322_DEFAULT_DATA)) {
		pc87322index = PC87322_INDEX1;
		pc87322data = PC87322_DATA1;
		return 0;
	}
	ch1 = inb(PC87322_INDEX2);
	ch2 = inb(PC87322_DATA2);
	if ((ch1 == PC87322_DEFAULT_INDEX) && (ch2 == PC87322_DEFAULT_DATA)) {
		pc87322index = PC87322_INDEX2;
		pc87322data = PC87322_DATA2;
		return 0;
	}
	/*
 	 *+  Unable to identify the index and data register addresses.
	 */
#ifdef MFPD_MESSAGES
	cmn_err(CE_WARN, "!mfpd: PC87322VF Programming Error");
#endif /* MFDP_MESSAGES */
	return - 1;
}



/* 
 * mfpd_pc87322_init()
 * 	Initializes the parallel port.
 *
 * Calling/Exit State :
 *
 * Description :
 *	Sets the appropriate bits of the various configuration registers
 * 	of the chip.
 */

STATIC int
mfpd_pc87322_init(unsigned long port_no)
{
	unsigned char	val, cntl_reg;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	/* Enable the parallel port */
	oldpri = splhi();
	outb(pc87322index, PC87322_FER);
	val = inb(pc87322data);
	val |= PC87322_PP_ENABLE;
	outb(pc87322data, val);
	outb(pc87322data, val);
	splx(oldpri);

	/* 
	 * Select the appropriate port address depending on the 
	 * base address value in mfpdcfg structure
	 */

	oldpri = splhi();
	outb(pc87322index, PC87322_FAR);
	val = inb(pc87322data);
	val &= ~PC87322_PPORT;
	switch (mfpdcfg[port_no].base) {

	case PORT1 :
		val |= PC87322_LPT2;
		outb(pc87322data, val);
		outb(pc87322data, val);
		break;
	case PORT3 :
		val |= PC87322_LPT1;
		outb(pc87322data, val);
		outb(pc87322data, val);
		break;
	case PORT2 :
		val |= PC87322_LPT3;
		outb(pc87322data, val);
		outb(pc87322data, val);
		break;
	default :  /* Case will not occur. Checked in mfpdstart */

		break;
	}
	splx(oldpri);

	/* 
	 * Put the chip into the extended mode.
	 * If the base address is 0x378, then select the interrupt vector.
	 */
	oldpri = splhi();
	outb(pc87322index, PC87322_PTR);
	val = inb(pc87322data);
	val |= PC87322_EXTENDED;
	if (mfpdcfg[port_no].vect == 7) {
		val |= PC87322_IRQ_LPT2;
	} else {
		val &= ~PC87322_IRQ_LPT2;
	}
	outb(pc87322data, val);
	outb(pc87322data, val);
	splx(oldpri);

	/*
	 * Disable the Parallel Port Multiplexor. Leave the EPP related 
	 * bits as they are.
	 */

	oldpri = splhi();
	outb(pc87322index, PC87322_FCR);
	val = inb(pc87322data);
	val &= ~PC87322_PPM_ON;
	outb(pc87322data, val);
	outb(pc87322data, val);
	splx(oldpri);

	/*
	 * Disable EPP mode. 
	 * Leave the other bits as they are.
 	 */

	oldpri = splhi();
	outb(pc87322index, PC87322_PCR);
	val = inb(pc87322data);
	val &= ~PC87322_EPP;
	outb(pc87322data, val);
	outb(pc87322data, val);
	splx(oldpri);

	mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
	mfpd_default_cntl_reg(port_no);

	/*  Put the parallel port in output mode */

	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= ~STD_INPUT;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);

	return 0;
}



/* 
 * mfpd_pc87322_get_status()
 * 	Get some port specific information.
 *
 * Calling/Exit State :
 * 	If flag is MFPD_DIRECTION, then *param will be set to the 
 * 	current parallel port direction (output/input). 
 *
 */

STATIC int 
mfpd_pc87322_get_status(unsigned long port_no, int flag, unsigned long *param)
{
	int	retval = 0;
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		cntl_reg &= STD_INPUT;
		if (cntl_reg) {
			*param = MFPD_REVERSE_CHANNEL;
		} else {
			*param = MFPD_FORWARD_CHANNEL;
		}
		retval = 0;
		break;

	default :
		retval = -1;
		break;
	}
	return retval;

}


/* 
 * mfpd_pc87322_select_mode()
 *	Does some chip configuration.
 * 	
 * Calling/Exit State :
 *
 * Description :
 * 	If the flag is MFPD_DIRECTION, then the chip set is put in the 
 *	input/output mode depending on param.
 *	If the flag is MFPD_CAPABILITY, then the chip set is put in the 
 *	appropriate mode depending on param.
 *
 */

STATIC int 
mfpd_pc87322_select_mode(unsigned long port_no, int flag, unsigned long param)
{
	unsigned char	val, cntl_reg;
	int	count;

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);

		/*
		 * For EPP 1.9, direction bit should be changed only during
		 * EPP idle phase.
		 */
		if (mfpdcfg[port_no].cur_mode & MFPD_PROPRIETARY) {


			count = MFPD_PC87322_RETRY;
			while (!((val = inb(mfpdcfg[port_no].base + STD_STATUS))
			    &STD_UNBUSY)) {
				drv_usecwait(100); /* Wait for 100 micro sec. */
				count--;
				if (count <= 0) {
					return - 1;
				}
			}
		}

		if (param == MFPD_FORWARD_CHANNEL) {
			cntl_reg &= ~STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
			return 0;
		} else {
			cntl_reg |= STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
			return 0;
		}
		/* NOTREACHED */
		break;

	case MFPD_CAPABILITY :
		/* There are 3 possible modes for PC87322 */

		if (((param & MFPD_EPP_MODE) != 0) || 
		    ((param & MFPD_PROPRIETARY) != 0)) {
			/* If Port base address is 0x3BC then no EPP */
			oldpri = splhi();
			outb(pc87322index, PC87322_FAR);
			val = inb(pc87322data);
			val &= PC87322_PPORT;
			splx(oldpri);
			if (val == PC87322_LPT1) {
				return - 1;
			}

			if (param & MFPD_EPP_MODE) { /* Ver. 1.7 */
				oldpri = splhi();
				outb(pc87322index, PC87322_PCR);
				val = inb(pc87322data);
				val &= ~PC87322_1_9;
				val |= PC87322_EPP;
				outb(pc87322data, val);
				outb(pc87322data, val);
				splx(oldpri);

				mfpdcfg[port_no].cur_mode = MFPD_EPP_MODE;

			} else { /* Ver. 1.9 */
				oldpri = splhi();
				outb(pc87322index, PC87322_PCR);
				val = inb(pc87322data);
				val |= PC87322_1_9;
				val |= PC87322_EPP;
				outb(pc87322data, val);
				outb(pc87322data, val);
				splx(oldpri);

				mfpdcfg[port_no].cur_mode = MFPD_PROPRIETARY;
			}

			/*
			 * Configure the IOCHRDY pin.
 			 * Put in extended mode so that software has
			 * control of the direction control bit.
			 */

			oldpri = splhi();
			outb(pc87322index, PC87322_FCR);
			val = inb(pc87322data);
			val &= ~PC87322_MFM_PIN;
			outb(pc87322data, val);
			outb(pc87322data, val);


			outb(pc87322index, PC87322_PTR);
			val = inb(pc87322data);
			val |= PC87322_EXTENDED;
			outb(pc87322data, val);
			outb(pc87322data, val);
			splx(oldpri);

			mfpd_default_cntl_reg(port_no);
			return 0;
		}

		if (param & MFPD_BIDIRECTIONAL) {
			oldpri = splhi();
			outb(pc87322index, PC87322_PTR);
			val = inb(pc87322data);
			val |= PC87322_EXTENDED;
			outb(pc87322data, val);
			outb(pc87322data, val);

			/* Disable EPP */

			outb(pc87322index, PC87322_PCR);
			val = inb(pc87322data);
			val &= ~PC87322_EPP;
			outb(pc87322data, val);
			outb(pc87322data, val);
			splx(oldpri);

			mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
			mfpd_default_cntl_reg(port_no);
			return 0;
		} else {
			return - 1;
		}
		/* NOTREACHED */
		break;

	default :
		return - 1;
		/* NOTREACHED */
		break;
	}
}




/*
 * 82360SL routines
 */



/* 
 * mfpd_sl82360_enable_config()
 *	Enables access to the configuration registers.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC void
mfpd_sl82360_enable_config(void)
{
	unsigned char	val;

#ifdef CPU386SL
	/* Lock the CPUPWRMODE reg. if unlocked */
	val = inb(SL82360_CPUPWRMODE);
	val &= SL82360_UNLOCKSTAT;
	if (val) { /* Unlocked. So lock it */
		val = inb(SL82360_CFGSTAT);
		val |= SL82360_CPUCNFGLCK;
		outb(SL82360_CFGSTAT, val);
	}
#endif /* CPU386SL */
	/* Enable the configuration register set if not enabled */
	val = inb(SL82360_CFGSTAT);
	val &= SL82360_CFGSPCLCKSTAT;
	if (val) { /* Not enabled. So enable */
		(void)inb(SL82360_CONFENABLE1);
		(void)inb(SL82360_CONFENABLE2);
		(void)inb(SL82360_CONFENABLE3);
		(void)inb(SL82360_CONFENABLE4);
	}
	return;
}


/* 
 * mfpd_sl82360_disable_config()
 * 	Disables access to the configuration registers.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC void
mfpd_sl82360_disable_config(void)
{
	unsigned char	val;

#ifdef CPU386SL
	/* If the CPUPWRMODE reg. is unlocked, then return */
	val = inb(SL82360_CPUPWRMODE);
	val &= SL82360_UNLOCKSTAT;
	if (val) { /* Unlocked. So return */
		return;
	}
#endif /* CPU386SL */

	val = inb(SL82360_CFGSTAT);
	val &= SL82360_CFGSPCLCKSTAT;
	if (val) { /* Not enabled. So return */
		return;
	}
	/* Config. reg set is enabled, so disable it */

	outb(SL82360_INDEX, SL82360_IDXLCK);
	val = inb(SL82360_DATA);
	val |= SL82360_CFGSPCLCK;
	outb(SL82360_DATA, val);
	return;
}


/* 
 * mfpd_sl82360_enable_bidi_mode()
 * 	Enables the bidirectional mode.
 *
 * Calling/Exit State :
 *
 * Description :
 *	Bidirectional mode is enabled by programming the PPCONFIG
 * 	register.
 *
 */

STATIC int
mfpd_sl82360_enable_bidi_mode(unsigned long port_no)
{
	unsigned char	temp;
	unsigned char	val;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */


	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	oldpri = splhi();
	mfpd_sl82360_enable_config();

	/* Enable the PS/2 registers */
	outb(SL82360_INDEX, SL82360_CFGR2);
	val = inb(SL82360_DATA);
	val |= SL82360_PS2_ENABLE;
	outb(SL82360_DATA, val);

	/* Choose the base address */
	switch (mfpdcfg[port_no].base) {

	case PORT1 :
		temp = SL82360_BA378;
		break;
	case PORT2 :
		temp = SL82360_BA278;
		break;
	case PORT3 :
		temp = SL82360_BA3BC;
		break;
	default : /* Case will not occur. Checked in mfpdstart */
		break;
	}

	/* Set address select bits and bidirection enable bit in PPCONFIG */
	val = inb(SL82360_PPCONFIG);
	val &= ~SL82360_PPORT;
	val |= temp;
	val |= SL82360_BIDI;
	outb(SL82360_PPCONFIG, val);

	mfpd_sl82360_disable_config();
	splx(oldpri);

	return 0;
}


/* 
 * mfpd_sl82360_disable_bidi_mode()
 *	Disables the bidirectional mode .
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int
mfpd_sl82360_disable_bidi_mode(unsigned long port_no)
{
	unsigned char	val;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	oldpri = splhi();
	mfpd_sl82360_enable_config();

	/* Disable the PS/2 registers */
	outb(SL82360_INDEX, SL82360_CFGR2);
	val = inb(SL82360_DATA);
	val &= ~SL82360_PS2_ENABLE;
	outb(SL82360_DATA, val);

	mfpd_sl82360_disable_config();
	splx(oldpri);

	return 0;

}


/* 
 * mfpd_sl82360_enable_fast_mode()
 *	Enables fast mode. This mode is similar to EPP.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int
mfpd_sl82360_enable_fast_mode(unsigned long port_no)
{
	unsigned char	temp;
	unsigned char	val;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	/* Adrress 0x3BC is not possible in this mode */
	/* Choose the base address */
	switch (mfpdcfg[port_no].base) {

	case PORT3 :
		return - 1;
		/* NOTREACHED */
		break;
	case PORT1 :
		temp = SL82360_FPP378;
		break;
	case PORT2 :
		temp = SL82360_FPP278;
		break;
	default : /* Case does not occur */
		break;
	}

	oldpri = splhi();
	mfpd_sl82360_enable_config();

	/* Enable special features if not enabled */
	outb(SL82360_INDEX, SL82360_CFGR2);
	val = inb(SL82360_DATA);
	if (!(val & SL82360_SFS_ENABLE)) {
		val |= SL82360_SFS_ENABLE;
		outb(SL82360_DATA, val);
		/* Dummy write to SFS_ENABLE reg. */
		outb(SL82360_SFSEN_REG, SL82360_SFSEN_DUMMY);
	}

	/* Enable fast mode, set address select bits */
	outb(SL82360_SFSINDEX, SL82360_FPPCNTL_INDEX);
	val = inb(SL82360_SFSDATA);
	val &= ~SL82360_FPP_PPORT;
	val |= temp;
	val |= SL82360_FPP_ENABLE;
	val |= SL82360_FPP_BIDI;
	outb(SL82360_SFSDATA, val);

	mfpd_sl82360_disable_config();
	splx(oldpri);

	return 0;

}


/* 
 * mfpd_sl82360_disable_fast_mode()
 *	 Disables fast mode.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int
mfpd_sl82360_disable_fast_mode(unsigned long port_no)
{
	unsigned char	val;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	oldpri = splhi();
	mfpd_sl82360_enable_config();

	outb(SL82360_INDEX, SL82360_CFGR2);
	val = inb(SL82360_DATA);
	val &= SL82360_SFS_ENABLE;
	if (!val) { /* Special features not enabled. Neither is fast mode */
		mfpd_sl82360_disable_config();
		splx(oldpri);
		return 0;
	}
	/* Disable fast mode. Don't disable special features */
	outb(SL82360_SFSINDEX, SL82360_FPPCNTL_INDEX);
	val = inb(SL82360_SFSDATA);
	val &= ~SL82360_FPP_ENABLE;
	outb(SL82360_SFSDATA, val);
	mfpd_sl82360_disable_config();
	splx(oldpri);
	return 0;
}




/* 
 * mfpd_sl82360_init()
 * 	Initializes the parallel port. Puts it in output mode.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int
mfpd_sl82360_init(unsigned long port_no)
{
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	(void)mfpd_sl82360_disable_fast_mode(port_no);
	(void)mfpd_sl82360_enable_bidi_mode(port_no);
	mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
	mfpd_default_cntl_reg(port_no);

	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= ~STD_INPUT;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);

	return 0;

}



/* 
 * mfpd_sl82360_get_status()
 * 	Get some port specific information.
 *
 * Calling/Exit State :
 * 	If flag is MFPD_DIRECTION, then *param will be set to the 
 * 	current parallel port direction (output/input).
 *
 */

STATIC int 
mfpd_sl82360_get_status(unsigned long port_no, int flag, unsigned long *param)
{
	int	retval = 0;
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		cntl_reg &= STD_INPUT;
		if (cntl_reg) {
			*param = MFPD_REVERSE_CHANNEL;
		} else {
			*param = MFPD_FORWARD_CHANNEL;
		}
		retval = 0;
		break;

	default :
		retval = -1;
		break;
	}
	return retval;

}


/* 
 * mfpd_sl82360_select_mode()
 * 	Does some chip configuration.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	If the flag is MFPD_DIRECTION, then the chip set is put in the 
 *	input/output mode depending on param.
 *	If the flag is MFPD_CAPABILITY, then the chip set is put in the 
 *	appropriate mode depending on param.
 */

STATIC int 
mfpd_sl82360_select_mode(unsigned long port_no, int flag, unsigned long param)
{
	int	ret_val;
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		if (param == MFPD_FORWARD_CHANNEL) {
			cntl_reg &= ~STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
		} else {
			cntl_reg |= STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
		}
		return 0;
		/* NOTREACHED */
		break;

	case MFPD_CAPABILITY :

		if (param & MFPD_BIDIRECTIONAL) { /* Bidirectional mode */
			(void)mfpd_sl82360_disable_fast_mode(port_no);
			(void)mfpd_sl82360_enable_bidi_mode(port_no);
			mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
			mfpd_default_cntl_reg(port_no);
			return 0;

		} else if (param & MFPD_PROPRIETARY) { /* Fast mode */

			(void)mfpd_sl82360_disable_bidi_mode(port_no);
			ret_val = mfpd_sl82360_enable_fast_mode(port_no);
			if (ret_val == -1) {
				mfpdcfg[port_no].cur_mode = MFPD_MODE_UNDEFINED;
				return - 1;
			}
			mfpdcfg[port_no].cur_mode = MFPD_PROPRIETARY;
			mfpd_default_cntl_reg(port_no);
			return 0;

		} else {
			return - 1;
		}
		/* NOTREACHED */
		break;

	default :
		return - 1;
		/* NOTREACHED */
		break;
	}
}





/*
 * AIP82091 routines
 */

/* 
 * mfpd_determine_aip82091_address()
 *	Determines the data and index register addresses.
 *
 * Calling/Exit State :
 * 	Sets two global variables : aip82091index and aip82091data.
 *
 * Description :
 *
 */

int
mfpd_determine_aip82091_address(void)
{
	aip82091index = AIP82091_INDEX;
	aip82091data = AIP82091_DATA;
	return 0;
}


/* 
 * mfpd_aip82091_init()
 *	Intializes the parallel port and puts it in the output mode.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	The parallel port configuration registers are programmed
 *	appropriately.
 *
 */

STATIC int 
mfpd_aip82091_init(unsigned long port_no)
{
	unsigned char	cntl_reg;
	unsigned char	val;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	outb(aip82091index, AIP82091_PCFG_INDEX);
	val = inb(aip82091data);

	/* choose bidirectional mode */
	mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;

	val &= ~AIP82091_PPHMOD_SEL;
	val |= AIP82091_PS2_SEL;

	/* select the appropriate interrupt vector */
	if (mfpdcfg[port_no].vect == 7) {
		val |= AIP82091_IRQ7_SEL;
	} else {
		val &= ~AIP82091_IRQ7_SEL;
	}

	/* Set the address select bits */
	val &= ~AIP82091_PPORT;
	switch (mfpdcfg[port_no].base) {

	case PORT1 :
		val |= AIP82091_BA378;
		break;
	case PORT2 :
		val |= AIP82091_BA278;
		break;
	case PORT3 :
		val |= AIP82091_BA3BC;
		break;
	default : /* Case does not occur */
		break;
	}

	/* Enable the parallel port */
	val |= AIP82091_PP_ENABLE;

	outb(aip82091data, val);

	/* 
	 * Don't write anything to the Extended Control register (ECP mode).
	 * Its default value is fine.
	 */

	mfpd_default_cntl_reg(port_no);

	/* Put the chip in output mode */
	cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
	cntl_reg &= ~STD_INPUT;
	outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);

	return 0;
}



/* 
 * mfpd_aip82091_get_fifo_threshold()
 *	Returns the fifo threshold in both forward and reverse directions.
 *
 * Calling/Exit State :
 * 	ffd and frd point to the FIFO thresholds in forward and reverse 
 *	directions.
 *
 * Description :
 *	If the 7th bit in the parallel port configuration register is set,
 *	it means that the fifo threshold is 1 in forward direction and 15
 *	in reverse direction. Otherwise it is 8 in both directions.
 *	These values are #defined in mfpdhw.h
 */

STATIC int 
mfpd_aip82091_get_fifo_threshold(unsigned long port_no, int *ffd, int *frd)
{
	unsigned char	val;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	if ((mfpdcfg[port_no].cur_mode & MFPD_ECP_MODE) || 
	    (mfpdcfg[port_no].cur_mode & MFPD_ECP_CENTRONICS)) {

		outb(aip82091index, AIP82091_PCFG_INDEX);
		val = inb(aip82091data);
		if (val & AIP82091_FIFO_SEL) {
			*ffd = AIP82091_FIFO_FWD1;
			*frd = AIP82091_FIFO_REV1;
		} else {
			*ffd = AIP82091_FIFO_FWD2;
			*frd = AIP82091_FIFO_REV2;
		}

	} else {
		*ffd = 0;
		*frd = 0;
	}
	return 0;
}


/* 
 * mfpd_aip82091_get_status()
 * 	Get some port specific information.
 *
 * Calling/Exit State :
 * 	If flag is MFPD_DIRECTION, then *param will be set to the 
 * 	current parallel port direction (output/input).
 *
 */

STATIC int 
mfpd_aip82091_get_status(unsigned long port_no, int flag, unsigned long *param)
{
	int	retval = 0;
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		if ((mfpdcfg[port_no].cur_mode & MFPD_CENTRONICS) || 
		    (mfpdcfg[port_no].cur_mode & MFPD_ECP_CENTRONICS)) {
			*param = MFPD_FORWARD_CHANNEL;
			return retval;
		}
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		cntl_reg &= STD_INPUT;
		if (cntl_reg) {
			*param = MFPD_REVERSE_CHANNEL;
		} else {
			*param = MFPD_FORWARD_CHANNEL;
		}
		retval = 0;
		break;

	default :
		retval = -1;
		break;
	}
	return retval;
}


/* 
 * mfpd_aip82091_select_mode()
 *	Does some chip configuration.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	If the flag is MFPD_DIRECTION, then the chip set is put in the 
 *	input/output mode depending on param.
 *	If the flag is MFPD_CAPABILITY, then the chip set is put in the 
 *	appropriate mode depending on param.
 *
 */

STATIC int 
mfpd_aip82091_select_mode(unsigned long port_no, int flag, unsigned long param)
{
	unsigned char	val, cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		if ((mfpdcfg[port_no].cur_mode & MFPD_CENTRONICS) || 
		    (mfpdcfg[port_no].cur_mode & MFPD_ECP_CENTRONICS)) {
			if (param == MFPD_FORWARD_CHANNEL) {
				return 0;
			} else {
				return - 1;
			}
		}
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		if (param == MFPD_FORWARD_CHANNEL) {
			cntl_reg &= ~STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
		} else {
			cntl_reg |= STD_INPUT;
			outb(mfpdcfg[port_no].base + STD_CONTROL, cntl_reg);
		}
		return 0;
		/* NOTREACHED */
		break;

	case MFPD_CAPABILITY :
		if (param & MFPD_BIDIRECTIONAL) { /* Bidirectional mode */
			/* 
			 * Set the hardware mode select bits appropriately.
			 */
			outb(aip82091index, AIP82091_PCFG_INDEX);
			val = inb(aip82091data);
			val &= ~AIP82091_PPHMOD_SEL;
			val |= AIP82091_PS2_SEL;
			outb(aip82091data, val);
			mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
			mfpd_default_cntl_reg(port_no);
			return 0;

		} else if (param & MFPD_CENTRONICS) { /* Centronics mode */
			/* 
						 * Set the hardware mode select bits appropriately.
						 */
			outb(aip82091index, AIP82091_PCFG_INDEX);
			val = inb(aip82091data);
			val &= ~AIP82091_PPHMOD_SEL;
			val |= AIP82091_ISA_SEL;
			outb(aip82091data, val);
			mfpdcfg[port_no].cur_mode = MFPD_CENTRONICS;
			mfpd_default_cntl_reg(port_no);
			return 0;
		} else if (param & MFPD_EPP_MODE) {
			/* 
			 * Set the hardware mode select bits appropriately.
			 * If the base address is 0x3BC, then  no EPP.
			 */
			outb(aip82091index, AIP82091_PCFG_INDEX);
			val = inb(aip82091data);
			if ((val & AIP82091_PPORT) == AIP82091_BA3BC) {
				/* Base addr. is 0x3BC */
				return - 1;
			}
			val &= ~AIP82091_PPHMOD_SEL;
			val |= AIP82091_EPP_SEL;
			outb(aip82091data, val);
			mfpdcfg[port_no].cur_mode = MFPD_EPP_MODE;
			mfpd_default_cntl_reg(port_no);
			return 0;

		}
		if ((param & MFPD_ECP_CENTRONICS) || (param & MFPD_ECP_MODE)) {

			/* Make fifo size 8 in both directions */
			outb(aip82091index, AIP82091_PCFG_INDEX);
			val = inb(aip82091data);
			val &= ~AIP82091_FIFO_SEL;
			outb(aip82091data, val);

			/* Program the Extended control register */
			if (param & MFPD_ECP_MODE) {
				mfpdcfg[port_no].cur_mode = MFPD_ECP_MODE;
				val = AIP82091_ECP_SEL;
			} else {
				mfpdcfg[port_no].cur_mode = MFPD_ECP_CENTRONICS;
				val = AIP82091_ECP_CENT_SEL;
			}
			/*
			 * Error interrupts disabled, DMA disabled and service
			 * interrupts enabled.
			 */

			val |= AIP82091_NO_ERROR_INTR;
			val &= ~AIP82091_DMA_ENABLE;
			val &= ~AIP82091_NO_SRVC_INTR;

			outb(mfpdcfg[port_no].base + AIP82091_ECP_ECR, val);

			mfpd_default_cntl_reg(port_no);
			return 0;

		} else {
			return - 1;
		}
		/* NOTREACHED */
		break;

	default :
		return - 1;
		/* NOTREACHED */
		break;
	}
}



/*
 * SMC FDC37C665 routines.
 */

/*
 * mfpd_smcfdc665_enable_config()
 *	Enables access to the configuration registers.
 *
 * Calling/Exit State :
 *
 * Description :
 *	Write INITVAL twice to the index register.
 *
 */

STATIC void
mfpd_smcfdc665_enable_config(void)
{
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	oldpri = splhi();
	outb(SMCFDC665_INDEX, SMCFDC665_INITVAL);
	outb(SMCFDC665_INDEX, SMCFDC665_INITVAL);
	splx(oldpri);
	return;
}


/*
 * mfpd_smcfdc665_disable_config()
 *	Disables access to the configuration registers.
 *
 * Calling/Exit State :
 *
 * Description :
 *	Write EXITVAL once to the index register.
 * 
 */

STATIC void
mfpd_smcfdc665_disable_config(void)
{
	outb(SMCFDC665_INDEX, SMCFDC665_EXITVAL);
	return;
}


/*
 * mfpd_smcfdc665_init()
 *	Initializes the parallel port. Puts it in output mode.
 *
 * Calling/Exit State :
 *
 * Description :
 *	Sets the appropriate bits of the various configuration registers
 * 	of the chip.
 */

STATIC int 
mfpd_smcfdc665_init(unsigned long port_no)
{
	unsigned char	val;

	mfpd_smcfdc665_enable_config();

	outb(SMCFDC665_INDEX, SMCFDC665_CR1);
	val = inb(SMCFDC665_DATA);

	/* Select the parallel port address */

	val &= ~SMCFDC665_PPORT;

	switch (mfpdcfg[port_no].base) {

	case PORT3 :
		val |= SMCFDC665_BA3BC;
		break;
	case PORT1 :
		val |= SMCFDC665_BA378;
		break;
	case PORT2 :
		val |= SMCFDC665_BA278;
		break;
	default : /* Can not occur. Checked in mfpdstart */
		break;

	}

	/* Enable extended parallel port modes */

	val &= ~SMCFDC665_PRN_MODE;

	outb(SMCFDC665_DATA, val);

	/* Select the bidirectional mode */

	outb(SMCFDC665_INDEX, SMCFDC665_CR4);
	val = inb(SMCFDC665_DATA);
	val &= ~SMCFDC665_MODE_SEL;
	val |= SMCFDC665_BIDI_SEL;
	outb(SMCFDC665_DATA, val);

	mfpd_smcfdc665_disable_config();

	mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
	mfpd_default_cntl_reg(port_no);

	/* Put the chip in the output mode */

	val = inb(mfpdcfg[port_no].base + STD_CONTROL);
	val &= ~STD_INPUT;
	outb(mfpdcfg[port_no].base + STD_CONTROL, val);

	return 0;
}


/*
 * mfpd_smcfdc665_get_fifo_threshold()
 * 	Returns the fifo threshold in both directions.
 *
 * Calling/Exit State :
 * 	ffd and frd point to the FIFO thresholds in forward and reverse 
 *	directions.
 *
 * Description : 
 *	Threshold is 16 - ( <val> + 1) where <val> is the lower nibble of 
 *	Configuration register 10 (0x0A).
 */

STATIC int 
mfpd_smcfdc665_get_fifo_threshold(unsigned long port_no, int *ffd, int *frd)
{
	unsigned char	val;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	if ((mfpdcfg[port_no].cur_mode & MFPD_ECP_MODE) || 
	    (mfpdcfg[port_no].cur_mode & MFPD_ECP_CENTRONICS)) {
		mfpd_smcfdc665_enable_config();
		outb(SMCFDC665_INDEX, SMCFDC665_CRA);
		val = inb(SMCFDC665_DATA);
		val &= SMCFDC665_FIFO_THRESH;
		*ffd = 16 - (val + 1);
		*frd = 16 - (val + 1);
		mfpd_smcfdc665_disable_config();
	} else {
		*ffd = 0;
		*frd = 0;
	}
	return 0;
}


/* 
 * mfpd_smcfdc665_get_status()
 * 	Get some port specific information.
 *
 * Calling/Exit State :
 * 	If flag is MFPD_DIRECTION, then *param will be set to the 
 * 	current parallel port direction (output/input).
 *
 */

STATIC int 
mfpd_smcfdc665_get_status(unsigned long port_no, int flag, unsigned long *param)
{
	int	retval = 0;
	unsigned char	cntl_reg;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		if ((mfpdcfg[port_no].cur_mode & MFPD_CENTRONICS) || 
		    (mfpdcfg[port_no].cur_mode & MFPD_ECP_CENTRONICS)) {
			*param = MFPD_FORWARD_CHANNEL;
			return retval;
		}
		cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
		cntl_reg &= STD_INPUT;
		if (cntl_reg) {
			*param = MFPD_REVERSE_CHANNEL;
		} else {
			*param = MFPD_FORWARD_CHANNEL;
		}
		retval = 0;
		break;

	default :
		retval = -1;
		break;
	}
	return retval;
}


/* 
 * mfpd_smcfdc665_select_mode()
 *	Does some chip configuration.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	If the flag is MFPD_DIRECTION, then the chip set is put in the 
 *	input/output mode depending on param.
 *	If the flag is MFPD_CAPABILITY, then the chip set is put in the 
 *	appropriate mode depending on param.
 *
 */

STATIC int 
mfpd_smcfdc665_select_mode(unsigned long port_no, int flag, unsigned long param)
{
	unsigned char	val, cntl_reg, temp;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		if ((mfpdcfg[port_no].cur_mode & MFPD_CENTRONICS) || 
		    (mfpdcfg[port_no].cur_mode & MFPD_ECP_CENTRONICS)) {
			if (param == MFPD_FORWARD_CHANNEL) {
				return 0;
			} else {
				return - 1;
			}
		}
		if (mfpdcfg[port_no].cur_mode & MFPD_ECP_MODE) {

			/* The fifo should be empty before this routine 
							is called */
			val = inb(mfpdcfg[port_no].base + SMCFDC665_ECP_ECR);
			temp = val; /* save */
			val &= ~SMCFDC665_ECP_MODE_SEL;
			val |= SMCFDC665_ECP_BIDI_SEL;
			/* Direction can be changed only in this mode */
			outb(mfpdcfg[port_no].base + SMCFDC665_ECP_ECR, val);
			cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
			if (param == MFPD_FORWARD_CHANNEL) {
				cntl_reg &= ~STD_INPUT;
				outb(mfpdcfg[port_no].base + STD_CONTROL, 
				    cntl_reg);
			} else {
				cntl_reg |= STD_INPUT;
				outb(mfpdcfg[port_no].base + STD_CONTROL, 
				    cntl_reg);
			}
			/* Restore the old mode */
			outb(mfpdcfg[port_no].base + SMCFDC665_ECP_ECR, temp);
			return 0;
		} else {
			cntl_reg = inb(mfpdcfg[port_no].base + STD_CONTROL);
			if (param == MFPD_FORWARD_CHANNEL) {
				cntl_reg &= ~STD_INPUT;
				outb(mfpdcfg[port_no].base + STD_CONTROL, 
				    cntl_reg);
			} else {
				cntl_reg |= STD_INPUT;
				outb(mfpdcfg[port_no].base + STD_CONTROL, 
				    cntl_reg);
			}
			return 0;
		}
		/* NOTREACHED */
		break;

	case MFPD_CAPABILITY :
		if (param & MFPD_CENTRONICS) { /* Printer mode */
			mfpd_smcfdc665_enable_config();
			outb(SMCFDC665_INDEX, SMCFDC665_CR1);
			val = inb(SMCFDC665_DATA);
			val |= SMCFDC665_PRN_MODE;
			outb(SMCFDC665_DATA, val);
			mfpd_smcfdc665_disable_config();

			mfpdcfg[port_no].cur_mode = MFPD_CENTRONICS;
			mfpd_default_cntl_reg(port_no);
			return 0;
		} else if (param & MFPD_BIDIRECTIONAL) {
			/*
			 * Disable printer mode in CR1 i.e. enable extended
			 * mode. Then choose bidirectional mode in CR4.
			 */
			mfpd_smcfdc665_enable_config();

			outb(SMCFDC665_INDEX, SMCFDC665_CR1);
			val = inb(SMCFDC665_DATA);
			val &= ~SMCFDC665_PRN_MODE;
			outb(SMCFDC665_DATA, val);

			outb(SMCFDC665_INDEX, SMCFDC665_CR4);
			val = inb(SMCFDC665_DATA);
			val &= ~SMCFDC665_MODE_SEL;
			val |= SMCFDC665_BIDI_SEL;
			outb(SMCFDC665_DATA, val);

			mfpd_smcfdc665_disable_config();

			mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
			mfpd_default_cntl_reg(port_no);
			return 0;
		} else if (param & MFPD_EPP_MODE) {
			/*
			 * Disable printer mode in CR1 i.e. enable extended
			 * mode. Then choose EPP mode in CR4.
			 */

			mfpd_smcfdc665_enable_config();

			outb(SMCFDC665_INDEX, SMCFDC665_CR1);
			val = inb(SMCFDC665_DATA);

			/* If port is at 0x3BC, then no EPP */
			if ((val & SMCFDC665_PPORT) == SMCFDC665_BA3BC) {
				mfpd_smcfdc665_disable_config();
				return - 1;
			}
			val &= ~SMCFDC665_PRN_MODE;
			outb(SMCFDC665_DATA, val);

			outb(SMCFDC665_INDEX, SMCFDC665_CR4);
			val = inb(SMCFDC665_DATA);
			val &= ~SMCFDC665_MODE_SEL;
			val |= SMCFDC665_EPP_SEL;
			outb(SMCFDC665_DATA, val);

			mfpd_smcfdc665_disable_config();

			mfpdcfg[port_no].cur_mode = MFPD_EPP_MODE;
			mfpd_default_cntl_reg(port_no);
			return 0;
		}
		if ((param & MFPD_ECP_MODE) || (param & MFPD_ECP_CENTRONICS)) {
			/*
			 * Disable printer mode in CR1 i.e. enable extended
			 * mode. Then choose ECP mode in CR4.
			 */

			mfpd_smcfdc665_enable_config();

			outb(SMCFDC665_INDEX, SMCFDC665_CR1);
			val = inb(SMCFDC665_DATA);
			val &= ~SMCFDC665_PRN_MODE;
			outb(SMCFDC665_DATA, val);

			outb(SMCFDC665_INDEX, SMCFDC665_CR4);
			val = inb(SMCFDC665_DATA);
			val &= ~SMCFDC665_MODE_SEL;
			val |= SMCFDC665_ECP_SEL;
			outb(SMCFDC665_DATA, val);

			/* Program the threshold to be 8 in both directions */
			outb(SMCFDC665_INDEX, SMCFDC665_CRA);
			outb(SMCFDC665_DATA, SMCFDC665_THRESHVAL8);

			mfpd_smcfdc665_disable_config();

			/* Program the ECP extended control register */
			if (param & MFPD_ECP_MODE) {
				val = SMCFDC665_ECP_SEL_ECR;
				mfpdcfg[port_no].cur_mode = MFPD_ECP_MODE;
			} else {
				val = SMCFDC665_ECP_CENT_SEL;
				mfpdcfg[port_no].cur_mode = MFPD_ECP_CENTRONICS;
			}

			/* 
			 * Disable error interrupts, disable DMA and
			 * enable service interrupts.
			 */

			val |= SMCFDC665_NO_ERR_INTR;
			val &= ~SMCFDC665_DMA_ENABLE;
			val &= ~SMCFDC665_NO_SRVC_INTR;


			outb(mfpdcfg[port_no].base + SMCFDC665_ECP_ECR, val);

			mfpd_default_cntl_reg(port_no);
			return 0;

		} else {
			return - 1;
		}
		/* NOTREACHED */
		break;

	default :
		return - 1;
		/* NOTREACHED */
		break;
	}
}


/*
 * Compaq routines 
 */


/* 
 * mfpd_compaq_init()
 * 	Initializes the parallel port. Puts it in the output mode.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC int 
mfpd_compaq_init(unsigned long port_no)
{
	unsigned char	val;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	/* Disable DMA */
	val = inb(MFPD_COMPAQ_DMAPORT);
	val &= ~MFPD_COMPAQ_DMA_ENABLE;
	outb(MFPD_COMPAQ_DMAPORT, val);

	/* Only one mode possible : MFPD_BIDIRECTIONAL */
	mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
	mfpd_default_cntl_reg(port_no);

	/* Put in the output mode */
	val = inb(MFPD_COMPAQ_IOPORT);
	val |= MFPD_COMPAQ_OUTPUT;
	outb(MFPD_COMPAQ_IOPORT, val);

	return 0;
}



/* 
 * mfpd_compaq_get_status()
 * 	Get some port specific information.
 *
 * Calling/Exit State :
 * 	If flag is MFPD_DIRECTION, then *param will be set to the 
 * 	current parallel port direction (output/input).
 *
 */

STATIC int 
mfpd_compaq_get_status(unsigned long port_no, int flag, unsigned long *param)
{
	int	retval = 0;
	unsigned char	val;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		val = inb(MFPD_COMPAQ_IOPORT);
		val &= MFPD_COMPAQ_OUTPUT;
		if (val) {
			*param = MFPD_FORWARD_CHANNEL;
		} else {
			*param = MFPD_REVERSE_CHANNEL;
		}
		retval = 0;
		break;

	default :
		retval = -1;
		break;
	}
	return retval;
}


/* 
 * mfpd_compaq_select_mode()
 * 	Does some chip configuration.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	If the flag is MFPD_DIRECTION, then the chip set is put in the
 * 	output/input mode depending on param.
 *	If the flag is MFPD_CAPABILITY, then the chip set is put in the
 *	mode specified by param.
 *
 */

STATIC int 
mfpd_compaq_select_mode(unsigned long port_no, int flag, unsigned long param)
{
	int	retval = 0;
	unsigned char	val;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (flag) {

	case MFPD_DIRECTION :
		val = inb(MFPD_COMPAQ_IOPORT);
		if (param == MFPD_FORWARD_CHANNEL) {
			val |= MFPD_COMPAQ_OUTPUT;
		} else {
			val &= ~MFPD_COMPAQ_OUTPUT;
		}
		outb(MFPD_COMPAQ_IOPORT, val);
		retval = 0;
		break;

	case MFPD_CAPABILITY :
		if (param & MFPD_BIDIRECTIONAL) {
			mfpdcfg[port_no].cur_mode = MFPD_BIDIRECTIONAL;
			mfpd_default_cntl_reg(port_no);
			retval = 0;
		} else {
			retval = -1;
		}
		break;

	default :
		retval = -1;
		break;
	}
	return retval;
}

