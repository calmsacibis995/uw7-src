#ident	"@(#)mfpd.c	1.1"

/*
 * The Multifunction parallel port driver.
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

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

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

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif /* _KERNEL_HEADERS */


#ifdef STATIC
#undef STATIC
#endif
#define STATIC


extern int	num_mfpd;		/* Number of parallel ports */
extern struct mfpd_cfg mfpdcfg[];	/* Port configuration structure */
extern struct mfpd_hw_routines mhr[];	/* Table of hardware routines */


int	mfpdstart(void);

#define DRVNAME      "mfpd - multifunction parallel port driver "

STATIC int mfpd_load(void), mfpd_unload(void);

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
int	mfpd_verify(rm_key_t);
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */


#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
MOD_ACDRV_WRAPPER(mfpd, mfpd_load, mfpd_unload, NULL, mfpd_verify, DRVNAME);
#else
MOD_DRV_WRAPPER(mfpd, mfpd_load, mfpd_unload, NULL, DRVNAME);
#endif  /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

#if (MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20)
/* lint will complain if these declarations are absent */
extern void	mod_drvattach(), mod_drvdetach();
#endif /* MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20 */


/*
 * mfpd_load(void)
 * 
 * Calling/Exit State :
 *
 * Description :
 */

STATIC int 
mfpd_load(void)
{
	int	ret;

	if ((ret = mfpdstart()) != 0)
		return ret;
#if (MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20)
	mod_drvattach(&mfpd_attach_info);
#endif /* MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20 */
	return 0;
}


/*
 * mfpd_unload(void)
 * 
 * Calling/Exit State :
 *
 * Description :
 */

STATIC int
mfpd_unload(void)
{
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	int	dev;

	for ( dev = 0; dev < num_mfpd; dev ++) {
		if (mfpdcfg[dev].mfpd_type != MFPD_PORT_ABSENT) {
			cm_intr_detach(mfpdcfg[dev].intr_cookie);
		}
	}
#else
	mod_drvdetach(&mfpd_attach_info);
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */
	return 0;
}





#if (MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20)
/* ddicheck complains if these are not declared  */
extern void	outb(int, unsigned char);
extern unsigned char	inb(int);
#endif /* MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20 */


#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
extern toid_t dtimeout(void (*)(), void *, long, pl_t, processorid_t);
extern void	untimeout(toid_t);
#else
extern int	timeout(void (*)(), caddr_t, long);
extern void	untimeout(int);
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

extern int	portalloc(int, int);
extern void	portfree(int, int);

#if (MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20)
extern int	copyin(caddr_t, caddr_t, size_t);
#endif /* MFPD_PDI_VERSION < MFPD_PDI_UNIXWARE20 */


void	mfpdintr(unsigned int);
STATIC void mfpdtimer(caddr_t);
STATIC int mfpd_portalloc(unsigned, unsigned);
STATIC void mfpd_portfree(unsigned, unsigned);
STATIC void mfpd_dummy_status_read(int);
STATIC void mfpd_determine_port_type(unsigned long);
STATIC ulong mfpd_get_mca_porttype(unsigned long);
STATIC void mfpd_print_port_type(int);

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
STATIC int mfpd_get_parameters_cm(int, unsigned int);
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */


#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
STATIC toid_t mfpd_toid; 	/* mfpd_timer related variables */

#else
STATIC int mfpd_toid;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

STATIC int mfpd_timer_count = 0;
STATIC int mfpd_dummy;                 	/* Dummy argument passed to the timer */
int	mfpddevflag = D_NEW ;


/*
 * In the comments below,
 * 'mfpd_driver' -> refers to a driver using the mfpd services.
 * 'non_mfpd_driver' -> a driver not using mfpd ( but using the parallel port).
 *
 */

/*
 * mfpd_request_port()
 * 	To acquire control of a particular parallel port.
 *
 * Calling/Exit State :
 * 	Returns ACCESS-REFUSED or ACCESS_GRANTED or -1
 *
 * Description :
 * 	If the port is free, access is granted to the requesting driver
 * 	if portalloc() succeeds. Otherwise, it will be informed of the
 * 	availability of the port (when portalloc() eventually succeeds)
 *   	if the driver's callback routine is not NULL. If the port is not 
 * 	free, access will be refused - driver will be called back when
 * 	the port becomes free.
 *	If there is an error, then -1 is returned.
 *
 */

int	
mfpd_request_port(struct mfpd_rqst *prqst)
{
	unsigned long	prt_rqst;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	prt_rqst = prqst->port;
	if (prt_rqst >= num_mfpd)
		return - 1;
	if (mfpdcfg[prt_rqst].mfpd_type == MFPD_PORT_ABSENT)
		return - 1;

	oldpri = MFPD_SPL();
	prqst->next = NULL; /* make the next pointer in request block NULL */
	if (mfpdcfg[prt_rqst].excl_flag == PORT_FREE) {
		/* Both q_head and q_tail will be NULL */
		mfpdcfg[prt_rqst].q_head = prqst;
		mfpdcfg[prt_rqst].q_tail = prqst;
		mfpdcfg[prt_rqst].excl_flag = PORT_ACQUIRED;

		if (mfpd_portalloc(mfpdcfg[prt_rqst].base, 
		    mfpdcfg[prt_rqst].end)) {
			/* 
			 * portalloc() succeeded. No 'non_mfpd_driver' is using
			 * the parallel port.
			 */
			splx(oldpri);
			return ACCESS_GRANTED;
		} else {
			/*
			 * A 'non_mfpd_driver' is using the parallel port.
			 * So wait until the port is released and callback
			 * the 'mfpd_driver' then.
			 */
			if ((prqst->drv_cb) == NULL) {
				/* 
				 * Callback routine is NULL. So return.
				 */
				mfpdcfg[prt_rqst].excl_flag = PORT_FREE;
				mfpdcfg[prt_rqst].q_head = NULL;
				mfpdcfg[prt_rqst].q_tail = NULL;
				splx(oldpri);
				return ACCESS_REFUSED;
			}
			mfpdcfg[prt_rqst].access_delay = 1;
			/* 
			 * Set a timer to periodically check whether the port
			 * is released by the 'non_mfpd_driver' or not.
			 */
			if (mfpd_timer_count == 0) {
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
				do {
					mfpd_toid = dtimeout(mfpdtimer, 
					    (void *)(&mfpd_dummy), 
					    drv_usectohz(5000000),
					    plhi, MFPD_PROCESSOR);
					if (mfpd_toid == 0) {
						/*
						 * Timeout table overflow.
						 */
#ifdef MFPD_MESSAGES
						cmn_err(CE_WARN,
						    "!mfpd: Unable to schedule timeout. Retrying ...");
#endif /* MFDP_MESSAGES */
						/* busy wait for 2 ms */
						drv_usecwait(2000);
					} else {
						break;
					}
					/* CONSTCOND */
				} while (1);
#else
				mfpd_toid = timeout(mfpdtimer, 
				    (caddr_t)(&mfpd_dummy), 
				    drv_usectohz(5000000));
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */
			}
			mfpd_timer_count++;
			splx(oldpri);
			return ACCESS_REFUSED;
		}
	} else {
		/*
		 * Port not free. 
		 * So add the request block to the end of the Q.
	      	 */
		if ((prqst->drv_cb) != NULL) {
			(mfpdcfg[prt_rqst].q_tail)->next = prqst;
			mfpdcfg[prt_rqst].q_tail = prqst;
		}
		splx(oldpri);
		return ACCESS_REFUSED;
	}
}


/*
 * mfpd_relinquish_port()
 * 	To relinquish control of a parallel port held by the driver.
 *
 * Calling/Exit State :
 * 	Returns 0/1. 
 * 	If the returned value is 0, this implies that the request block
 *	of the driver is removed from the Q. The driver is expected to 
 * 	reclaim that. If an error occurs (request block not on the
 *	queue etc), 1 is returned.
 *
 * Description :
 *	If the request block is present in the port Q, then it will be
 * 	removed from the Q.If the caller had access to the port, then 
 *	the access will be granted to the waiting driver,if any. If the 
 *	caller did not have access to the port, then the routine simply
 *	returns.
 */

int	
mfpd_relinquish_port(struct mfpd_rqst *prqst)
{
	struct mfpd_rqst *temp_ptr, *backptr;
	unsigned long	port_no;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	if (prqst == NULL)
		return 1;
	port_no = prqst->port;
	if (port_no >= num_mfpd)
		return 1;

	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return 1;

	oldpri = MFPD_SPL();
	if (mfpdcfg[port_no].excl_flag == PORT_FREE) {
		splx(oldpri);
		return 1;
	}
	if (mfpdcfg[port_no].q_head == NULL) { /* Empty Q */
		splx(oldpri);
		return 1;
	}

	/* Search for the request block in the Q */
	backptr = NULL;
	temp_ptr = mfpdcfg[port_no].q_head;
	do {
		if (temp_ptr == prqst)
			break;
		backptr = temp_ptr;
		temp_ptr = temp_ptr->next;
	} while (temp_ptr != NULL);

	if (temp_ptr == NULL) { /* Q traversed fully */
		splx(oldpri);
		return 1;
	}


	if (backptr == NULL) {
		/* 
		 * prqst is at the head of the Q. 
		 */
		mfpdcfg[port_no].excl_flag = PORT_FREE;

		/* Remove the request block from the Q */
		temp_ptr = mfpdcfg[port_no].q_head;
		if (temp_ptr == mfpdcfg[port_no].q_tail) {
			mfpdcfg[port_no].q_head = NULL;
			mfpdcfg[port_no].q_tail = NULL;
		} else {
			mfpdcfg[port_no].q_head = temp_ptr->next;
		}

		temp_ptr = mfpdcfg[port_no].q_head;
		if ((mfpdcfg[port_no].access_delay) && (temp_ptr != NULL)) {
			/*
			 * a 'non_mfpd_driver' is using the port. But the Q
			 * is not empty. So the driver at the head of the Q
			 * has to wait until the port is relinquished by the
			 * non_mfpd_driver.
			 */
			mfpdcfg[port_no].excl_flag = PORT_ACQUIRED;
		} else if ((mfpdcfg[port_no].access_delay) && 
		    (temp_ptr == NULL)) {
			/*
			 * a non_mfpd_driver is using the port and the Q is 
 			 * empty. So untimeout the mfpd_timer.
			 */
			mfpdcfg[port_no].access_delay = 0;
			mfpd_timer_count--;
			if (mfpd_timer_count == 0)
				untimeout(mfpd_toid);
		} else if ((!(mfpdcfg[port_no].access_delay)) && 
		    (temp_ptr != NULL)) {
			/* 
			 * no non_mfpd_driver is using the port and Q is not
			 * empty. So grant access to the driver at the head 
			 * of the Q by calling its callback routine.
			 */
			mfpdcfg[port_no].excl_flag = PORT_ACQUIRED;
			(void)(*(temp_ptr->drv_cb))(temp_ptr);
		} else {
			/* 
			 * no non_mfpd_driver is using the port. Q is empty.
			 * So execute 'portfree' on the port.
			 */
			mfpd_portfree(mfpdcfg[port_no].base, 
			    mfpdcfg[port_no].end);
		}
	} else {
		/*
		 * prqst is not at the head of the Q.
		 */
		if (temp_ptr == mfpdcfg[port_no].q_tail) /* Adjust q_tail */
			mfpdcfg[port_no].q_tail = backptr;
		backptr->next = temp_ptr->next;
	}
	splx(oldpri);
	return 0;
}


/*
 * mfpd_get_port_type()
 * 	Returns the port type.
 *
 * Calling/Exit State :  
 *
 */

unsigned long	
mfpd_get_port_type(unsigned long port_no)
{
	if (port_no >= num_mfpd)
		return MFPD_PORT_ABSENT;
	return mfpdcfg[port_no].mfpd_type;
}


/*
 * mfpd_get_baseaddr()
 * 	Returns the base address of the port.
 *
 * Calling/Exit State :
 * 	If the return value is 0, then it is not a valid address.
 *
 */

unsigned	
mfpd_get_baseaddr(unsigned long port_no)
{
	if (port_no >= num_mfpd)
		return 0;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return 0;
	return mfpdcfg[port_no].base;
}


/*
 * mfpd_get_capability()
 * 	Returns the capabilities of the port.
 *
 * Calling/Exit State :
 *
 */

unsigned long	
mfpd_get_capability(unsigned long port_no)
{
	if (port_no >= num_mfpd)
		return MFPD_MODE_UNDEFINED;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return MFPD_MODE_UNDEFINED;
	return mfpdcfg[port_no].capability;
}


/*
 * mfpd_get_port_count()
 * 	Returns the number of parallel ports. 
 *
 * Calling/Exit State :
 *
 */

int	
mfpd_get_port_count(void)
{
	return num_mfpd;
}


/*
 * mfpdstart()
 * 	Initialization routine.
 *
 * Calling/Exit State :
 *
 * Description :
 * 	Initializes the various parallel port chip sets.
 */

int	
mfpdstart(void)
{
	int	dev;
	register ushort testval;
	unsigned	temp;
	int	ret_val;
	int	count_of_boards_inited = 0;

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)

	unsigned int	bus_p;

	/* Get the number of ports configured from the configuration manager */
	num_mfpd = cm_getnbrd("mfpd");

	/* Determine the bus type */
	if (drv_gethardware(IOBUS_TYPE, &bus_p) < 0) {
		/*
		 *+ Bus type can not be determined.
		 */
#ifdef MFPD_MESSAGES
		cmn_err(CE_NOTE, "!mfpd : can't decide bus type");
#endif /* MFDP_MESSAGES */
		return EINVAL;
	}

#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	for (dev = 0; dev < num_mfpd; dev++) {

		/* 
		 * Initialize port configuration structure 'mfpdcfg[dev]'.
		 */
		mfpdcfg[dev].excl_flag = PORT_FREE;
		mfpdcfg[dev].access_delay = 0;
		mfpdcfg[dev].q_head = NULL;
		mfpdcfg[dev].q_tail = NULL;

		mfpd_determine_port_type(dev);
		/*
		 *+ Announce the port type.
		 */
		mfpd_print_port_type(dev);

		mfpd_set_capability(dev);

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)

		/*
 		 * If autoconfigurable, then get the interrupt vector, base
		 * address etc from the Configuration Manager.
		 */

		if (mfpd_get_parameters_cm(dev, bus_p)) {
			/*
			 *+ Error obtaining info. from CM.
		  	 */
#ifdef MFPD_MESSAGES
			cmn_err(CE_NOTE, 
			    "!mfpd: Error obtaining information from Configuration Manager for port %d", 
			    dev);
#endif /* MFDP_MESSAGES */
			mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
			continue;
		}

#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */


		/* Initialize the parallel port depending on its type. */

		switch (mfpdcfg[dev].mfpd_type) {

		case MFPD_SIMPLE_PP :   /* FALL THROUGH */
		case MFPD_PS_2 :  /* FALL THROUGH */
		case MFPD_COMPAQ :
			/* Probe for the board */
			outb(mfpdcfg[dev].base, 0x55);
			testval = ((short)inb(mfpdcfg[dev].base) & 0xFF);
			if (testval != 0x55) {
				/*
				 *+ Board not present.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: No parallel port at address 0x%x", 
				    mfpdcfg[dev].base);
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}

			/*
			 * NOTYET: The device-reset sequence below is
			 * temporary. A proper implementation should
			 * probably add a .reset function ptr to the
			 * struct mfpd_hw_routines for each chipset.
			 * Note also that this reset is currently only
			 * done for the "SIMPLE" types of chipsets
			 * under this switch's case.
			 * 
			 * Also, look into whether device reset should be
			 * done before or after controller initialization.
			 * 
			 * Also, as currently implemented, it appears that
			 * the init routine is a combination of initialization
			 * plus set the device to "output only" mode. Look into
			 * possible need to seperate this functionality.
			 * 
			 */
			outb(mfpdcfg[dev].base + STD_CONTROL, STD_SEL);
			drv_usecwait(750);
			outb(mfpdcfg[dev].base + STD_CONTROL, STD_SEL | STD_RESET);


			/* Controller is present so initialize it */
			if ( (mfpdcfg[dev].pmhr)->init == NULL ) {
				/*
				 *+ Error. Missing init routine.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, "!mfpd: Init routine not found");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			} else {
				/*
				 *+ Announce that the board/controller is
				 *+ present.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Parallel port found at address 0x%x with IRQ %u", 
				    mfpdcfg[dev].base, mfpdcfg[dev].vect);
#endif /* MFDP_MESSAGES */
				(void)(*((mfpdcfg[dev].pmhr)->init))(
				    (unsigned long)dev);
				count_of_boards_inited++;
			}
			break;

		case MFPD_PC87322 :
			/* Determine the index and data register addresses */
			ret_val = mfpd_determine_pc87322_address();
			if (ret_val == -1) {
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			/*
			 * This chip allows only three addresses for PP :
			 * 0x378, 0x278, 0x3BC.
			 */
			temp = mfpdcfg[dev].base;
			if ((temp != 0x378) && (temp != 0x278)
			     && (temp != 0x3BC)) {
				/*
				 *+ Illegal Parallel port address. Chip
				 *+ can not support this.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Invalid parallel port address. Port not configured");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			/* 
			 * This chip allows only two interrupt vectors for 
			 * PP : 5 and 7.
			 */
			temp = mfpdcfg[dev].vect;
			if ((temp != 5) && (temp != 7)) {
				/*
				 *+ Illegal interrupt vector. Chip can not
				 *+ support this.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Invalid parallel port interrupt vecotr. Port not configured");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}

			/*
			 * If the base address is 0x3BC, then IRQ 7 has to
			 * be used. If the base address is 0x278, then
			 * IRQ 5 has to be used. If the base address is
			 * 0x378, then the vector used is programmable 
			 * (either 5 or 7).
			 */
			if ((mfpdcfg[dev].base == 0x3BC) && 
			    (mfpdcfg[dev].vect != 7)) {
				/*
				 *+ Error
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE,
				    "!mfpd: Base address and interrupt vector mismatch. Port not configured");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			if ((mfpdcfg[dev].base == 0x278) && 
			    (mfpdcfg[dev].vect != 5)) {
				/*
				 *+ Error
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE,
				    "!mfpd: Base address and interrupt vector mismatch. Port not configured");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}

			/* Initialize the chip set */
			if ( (mfpdcfg[dev].pmhr)->init == NULL ) {
				/*
				 *+ Error. Missing init routine.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, "!mfpd: Init routine not found");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			} else {
				/*
				 *+ Announce that the board/controller is
				 *+ present.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Parallel port found at address 0x%x with IRQ %u", 
				    mfpdcfg[dev].base, mfpdcfg[dev].vect);
#endif /* MFDP_MESSAGES */
				(void)(*((mfpdcfg[dev].pmhr)->init))(
				    (unsigned long)dev);
				count_of_boards_inited++;
			}
			break;

		case MFPD_SL82360:

			/*
			 * This chip allows only three addresses for PP :
			 * 0x378, 0x278, 0x3BC.
			 */
			temp = mfpdcfg[dev].base;
			if ((temp != 0x378) && (temp != 0x278) && 
			    (temp != 0x3BC)) {
				/*
				 *+ Illegal Parallel port address. Chip
				 *+ can not support this.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Invalid parallel port address. Port not configured ");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			/* Initialize the chip set */
			if ( (mfpdcfg[dev].pmhr)->init == NULL ) {
				/*
				 *+ Error. Missing init routine.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, "!mfpd: Init routine not found");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			} else {
				/*
				 *+ Announce that the board/controller is
				 *+ present.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Parallel port found at address 0x%x with IRQ %u", 
				    mfpdcfg[dev].base, mfpdcfg[dev].vect);
#endif /* MFDP_MESSAGES */
				(void)(*((mfpdcfg[dev].pmhr)->init))(
				    (unsigned long)dev);
				count_of_boards_inited++;
			}
			break;

		case MFPD_AIP82091 :
			/* Determine the index and data register addresses */
			ret_val = mfpd_determine_aip82091_address();
			if (ret_val == -1) {
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			/*
			 * This chip allows only three addresses for PP :
			 * 0x378, 0x278, 0x3BC.
			 */
			temp = mfpdcfg[dev].base;
			if ((temp != 0x378) && (temp != 0x278) && 
			    (temp != 0x3BC)) {
				/*
				 *+ Illegal Parallel port address. Chip
				 *+ can not support this.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Invalid parallel port address. Port not configured ");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			/* 
			 * This chip allows only two interrupt vectors for 
			 * PP : 5 and 7.
			 */
			temp = mfpdcfg[dev].vect;
			if ((temp != 5) && (temp != 7)) {
				/*
				 *+ Illegal interrupt vector. Chip can not
				 *+ support this.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Invalid parallel port interrupt vecotr. Port not configured");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			/* Initialize the chip set */
			if ( (mfpdcfg[dev].pmhr)->init == NULL ) {
				/*
				 *+ Error. Missing init routine.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, "!mfpd: Init routine not found");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			} else {
				/*
				 *+ Announce that the board/controller is
				 *+ present.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Parallel port found at address 0x%x with IRQ %u", 
				    mfpdcfg[dev].base, mfpdcfg[dev].vect);
#endif /* MFDP_MESSAGES */
				(void)(*((mfpdcfg[dev].pmhr)->init))(
				    (unsigned long)dev);
				count_of_boards_inited++;
			}
			break;

		case MFPD_SMCFDC665:

			/*
			 * This chip allows only three addresses for PP :
			 * 0x378, 0x278, 0x3BC.
			 */
			temp = mfpdcfg[dev].base;
			if ((temp != 0x378) && (temp != 0x278) && 
			    (temp != 0x3BC)) {
				/*
				 *+ Illegal Parallel port address. Chip
				 *+ can not support this.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE,
				    "!mfpd: Invalid parallel port address. Port not configured ");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			}
			/* Initialize the chip set */
			if ( (mfpdcfg[dev].pmhr)->init == NULL ) {
				/*
				 *+ Error. Missing init routine.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, "!mfpd: Init routine not found");
#endif /* MFDP_MESSAGES */
				mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
				break;
			} else {
				/*
				 *+ Announce that the board/controller is
				 *+ present.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_NOTE, 
				    "!mfpd: Parallel port found at address 0x%x with IRQ %u", 
				    mfpdcfg[dev].base, mfpdcfg[dev].vect);
#endif /* MFDP_MESSAGES */
				(void)(*((mfpdcfg[dev].pmhr)->init))(
				    (unsigned long)dev);
				count_of_boards_inited++;
			}
			break;

		default :
#ifdef MFPD_MESSAGES
			cmn_err(CE_NOTE, "!mfpd: Port not present at address 0x%x",
			    mfpdcfg[dev].base);
#endif /* MFDP_MESSAGES */
			mfpdcfg[dev].mfpd_type = MFPD_PORT_ABSENT;
			break;
		}
	}

	/* If no boards are initialised, then return error */
	if (count_of_boards_inited == 0) {
#ifdef MFPD_MESSAGES
		cmn_err(CE_NOTE, "!mfpd: No board initialised. So not loaded");
#endif /* MFPD_MESSAGES */
		return ENODEV;
	}

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	for ( dev = 0; dev < num_mfpd; dev++) {
		if (mfpdcfg[dev].mfpd_type != MFPD_PORT_ABSENT) {
			(void)cm_intr_attach(cm_getbrdkey("mfpd", dev), mfpdintr,
			    &mfpddevflag, &(mfpdcfg[dev].intr_cookie));
		}
	}
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */
	return 0;
}


/*
 * mfpdintr()
 * 	Interrupt handler for the driver.
 *
 * Calling/Exit State : 
 *
 * Description : 
 * 	Identifies the port which caused the interrupt and invokes
 * 	the interrupt handler of the owner of the port.
 *      
 *	The routine mfpd_dummy_status_read() reads the status register of
 *	the parallel port and thus clears the interrupt latch (interrupt
 *	pending bit). If this is not done, then there can be a panic on
 *	certain systems.
 */


void	
mfpdintr(unsigned int vec)
{
	int	dev;
	struct mfpd_rqst *temp_ptr;

	for (dev = 0; dev < num_mfpd; dev++) {
		if (mfpdcfg[dev].vect == vec)
			break;
	}
	if (dev >= num_mfpd)
		return;
	if (mfpdcfg[dev].mfpd_type == MFPD_PORT_ABSENT)
		return;

	if (mfpdcfg[dev].excl_flag == PORT_FREE) {
		/* 
		 * No 'mfpd_driver' has requested access to the port.
		 */
		if (mfpd_portalloc(mfpdcfg[dev].base, mfpdcfg[dev].end)) {
			/* 
			 * portalloc() succeeded. This means that neither
			 * are the 'non_mfpd_drivers' using the port.
			 * So the interrupt was probably caused by some
			 * change in the status of the device connected 
			 * to the parallel port or is a spurious one. 
			 */
			/*
			 * Nothing special needs to be done to handle this 
			 * interrupt.
			 */
			mfpd_dummy_status_read(dev);
			mfpd_portfree(mfpdcfg[dev].base, mfpdcfg[dev].end);
		}
		/*
		 * If portalloc fails, then a 'non_mfpd_driver' must be
		 * using the port. So the interrupt will be handled by 
		 * that driver.
		 */
		return;
	}
	/*
	 * If some 'mfpd_driver' had requested access to the port but 
	 * access was delayed ( because some 'non_mfpd_driver' was using
	 * the port), then check to see whether that 'non_mfpd_driver'
	 * is still using the port. If not (portalloc() succeeds), 
	 * grant access to the waiting 'mfpd_driver'.
	 */
	if (mfpdcfg[dev].access_delay) {
		if (mfpd_portalloc(mfpdcfg[dev].base, mfpdcfg[dev].end)) {
			/* Dummy status read */
			mfpd_dummy_status_read(dev);
			mfpdcfg[dev].access_delay = 0;
			mfpd_timer_count--;
			if (mfpd_timer_count == 0)
				untimeout(mfpd_toid);
			temp_ptr = mfpdcfg[dev].q_head;
			(void)(*(temp_ptr->drv_cb))(temp_ptr);
		}
		return;
	}

	/* 
	 * At this point, a 'mfpd_driver' is using the parallel port.
	 * So call its interrupt handler.
 	 */
	mfpd_dummy_status_read(dev);
	if ((mfpdcfg[dev].q_head)->intr_cb != NULL) {
		(void)(*((mfpdcfg[dev].q_head)->intr_cb))(mfpdcfg[dev].q_head);
	}
	return;

}


/*
 * mfpd_dummy_status_read()
 * 	Reads the status register. This clears the interrupt latch.
 *
 * Calling/Exit State :
 * 
 * Description :
 *
 */

STATIC void
mfpd_dummy_status_read(int dev)
{

	switch (mfpdcfg[dev].mfpd_type) {

	case MFPD_SIMPLE_PP :
	case MFPD_PS_2 :
	case MFPD_PC87322:
	case MFPD_SL82360 :
	case MFPD_COMPAQ :
		(void)inb(mfpdcfg[dev].base + STD_STATUS);
		break;

	case MFPD_AIP82091 :
	case MFPD_SMCFDC665 :
		/*
		 * When in ECP mode or ECP-Centronics mode, the interrupt
		 * pending bit is in the Extended Control Register and
		 * not in the Standard Status Port.
		 */
		if ((mfpdcfg[dev].cur_mode & MFPD_ECP_MODE) || 
		    (mfpdcfg[dev].cur_mode & MFPD_ECP_CENTRONICS)) {
			(void)inb(mfpdcfg[dev].base + AIP82091_ECP_ECR);
		} else {
			(void)inb(mfpdcfg[dev].base + STD_STATUS);
		}

		break;

	case MFPD_PORT_ABSENT :
		return;

	default :
		return;
	}
	return;
}


/*
 * mfpdtimer()
 * 	If the ports to which access have been delayed are freed 
 *	(through portfree()), then the access is granted to the driver
 * 	waiting at the head of the Q.
 * 
 * Calling/Exit State :
 *
 * Description:
 *
 */

/* ARGSUSED */
STATIC void
mfpdtimer(caddr_t arg)
{
	int	i;
	struct mfpd_rqst *temp_ptr;

	for (i = 0; i < num_mfpd; i++) {
		if (mfpdcfg[i].access_delay) {
			/* 
			 * Access to this port was delayed because a 
			 * non_mfpd_driver was using the port.
			 */
			if (mfpd_portalloc(mfpdcfg[i].base, mfpdcfg[i].end)) {
				/* 
				 * That non_mfpd_driver has now released the
				 * port.
				 */
				mfpdcfg[i].access_delay = 0;
				mfpd_timer_count--;
				temp_ptr = mfpdcfg[i].q_head;
				(void)(*(temp_ptr->drv_cb))(temp_ptr);
			}
		}
	}
	/* Set the timer if necessary */
	if (mfpd_timer_count != 0) {
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
		do {
			mfpd_toid = dtimeout(mfpdtimer, (void *)(&mfpd_dummy),
			    drv_usectohz(5000000), plhi, MFPD_PROCESSOR);
			if (mfpd_toid == 0) {
				/*
				 * Timeout table overflow.
				 */
#ifdef MFPD_MESSAGES
				cmn_err(CE_WARN,
				    "!mfpd: Unable to schedule timeout. Retrying ...");
#endif /* MFDP_MESSAGES */
				/* busy wait for 2 ms */
				drv_usecwait(2000);
			} else {
				break;
			}
			/* CONSTCOND */
		} while (1);
#else
		mfpd_toid = timeout(mfpdtimer, (caddr_t)(&mfpd_dummy),
		    drv_usectohz(5000000));
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */
	}
}


/*
 * mfpd_portalloc()
 * 	Calls portalloc() if MERGE386 is defined.
 *
 * Calling/Exit State :
 *
 */

STATIC int 
mfpd_portalloc(unsigned a, unsigned b)
{
#ifndef MERGE386
	return 1;
#else
	return portalloc(a, b);
#endif /* ~MERGE386 */

}


/*
 * mfpd_portfree()
 * 	Calls portfree() if MERGE386 is defined.
 *
 * Calling/Exit State :
 *
 */

STATIC void 
mfpd_portfree(unsigned a, unsigned b)
{
#ifndef MERGE386
	return;
#else
	portfree(a, b);
#endif /* ~MERGE386 */
}


/*
 * mfpd_query()
 * 	Returns various information about mfpd driver and the devices 
 * 	attached to the parallel port.
 *
 *	At present, the only supported enquiry_type is MFPD_DEVICE_ADDR.
 *
 * Calling/Exit State :
 *	If the enquiry_type is MFPD_DEVICE_ADDR, then the routine returns 
 * 	the addresses of the devices of type specified by device_type, 
 *	connected to parallel port port_no, encoded as follows : If the
 *	i'th bit of result is set, then there is a device at address i.
 *
 */

int
mfpd_query(unsigned long port_no, int enquiry_type, 
unsigned long device_type, unsigned long *result)
{
	int	i;

	if (port_no >= num_mfpd)
		return ENXIO;
	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return ENXIO;

	switch (enquiry_type) {

	case MFPD_DEVICE_ADDR :
		*result = 0;
		if (mfpdcfg[port_no].num_devices <= 0)
			return 0;
		/*
		 * 'mfpd_device' is an array of structures, one structure 
		 * for each device connected to the parallel port. 
		 * This structure has two fields:
		 * 1) 'device_type' giving the type of the device.
		 * 2) 'device_num' giving the address of the device ( in case
		 *     multiple devices are connected to the port).
		 *
		 * Check the type of each device specified in 'mfpd_device'
		 * array. If the type of a device matches with the passed 
		 * parameter 'device_type', then obtain 'device_num' field 
		 * for that device and set 'device_num'th bit in result.
		 */
		for (i = 0; i < mfpdcfg[port_no].num_devices; i++) {
			if ((mfpdcfg[port_no].mfpd_device)[i].device_type == 
			    device_type) {
				(*result) |= (1 << ((mfpdcfg[port_no].mfpd_device)[i].device_num));
			}
		}
		return 0;

	default :
		return - 1;
	}
}



/*
 * mfpd_determine_port_type()
 *	Find out the type of the port.
 * 
 * Calling/Exit State :
 *
 * Description:
 * 	This determines the type of the parallel port and sets
 * 	mfpd_type and the pmhr pointer in the mfpd_cfg structure.
 *
 */

STATIC void
mfpd_determine_port_type(unsigned long port_num)
{
	unsigned int	bus_p;

	/*
	 * If the machine is a MCA one, then the port type is corrected
    	 * depending on bit 7 in the POS2 register. If the port type as 
	 * specified in Space.c is a SuperI/O chip, then the port type
	 * is not corrected. 
	 *
         * Correct the pmhr pointr.
	 */

	if (port_num >= num_mfpd)
		return;

	if (drv_gethardware(IOBUS_TYPE, &bus_p) < 0) {
		mfpdcfg[port_num].pmhr = &mhr[mfpdcfg[port_num].mfpd_type];
		return;
	}

	if (!(bus_p & BUS_MCA)) {
		mfpdcfg[port_num].pmhr = &mhr[mfpdcfg[port_num].mfpd_type];
		return;
	}

	if ((mfpdcfg[port_num].mfpd_type == MFPD_SIMPLE_PP) || 
	    (mfpdcfg[port_num].mfpd_type == MFPD_PS_2)) {
		mfpdcfg[port_num].mfpd_type = mfpd_get_mca_porttype(port_num);
	}

	mfpdcfg[port_num].pmhr = &mhr[mfpdcfg[port_num].mfpd_type];

	return;
}


/*
 * mfpd_get_mca_porttype()
 *	Decodes bit 7 of POS2 register and returns the port type.
 *
 * Calling/Exit State :
 *
 * Description :
 */

/* ARGSUSED */
STATIC ulong
mfpd_get_mca_porttype(unsigned long port_num)
{

	/*
	 * The POS register read by this routine actually depends on the 
	 * port_num parameter ( the slot where that board is present).
	 * At present, just read the POS register of the Mother board.
	 */

	unsigned char	curval;
#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	pl_t oldpri;
#else
	int	oldpri;
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	oldpri = splhi();
	/*
	 * Put the system board in setup mode and disable adapter setup. 
	 */
	outb(ADAP_ENAB, 0);
	outb(SYS_ENAB, 0x7F);
	/* Read the POS_2 register */
	curval = inb(POS_2);
	/* We are interested in the 7th bit of the POS_2 register */
	curval &= 0x80;
	/* Enable the system board functions. Come out of setup. */
	outb(SYS_ENAB, 0xFF);
	splx(oldpri);
	/* 
	 * If the 7th bit of POS_2 register is reset, then the parallel port
	 * is in bidirectional mode. If set, it is in centronics mode.  
	 */
	if (curval) {
		return MFPD_SIMPLE_PP;
	} else {
		return MFPD_PS_2;
	}
}


/*
 * mfpd_print_port_type()
 * 	Prints the port type.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

STATIC void
mfpd_print_port_type(int port_num)
{
	unsigned long	p_type;
	char	*port_types[] = {
		"Simple Parallel Port", "PS_2 Parallel Port", 
		"PC87322 SuperI/O Chip Parallel Port", 
		"SL82360 Chip Parallel Port", 
		"AIP82091 SuperI/O Chip Parallel Port", 
		"SMCFDC665 SuperI/O Chip Parallel Port", 
		"Compaq Parallel Port" 	};

	p_type = mfpdcfg[port_num].mfpd_type;

	if (p_type == MFPD_PORT_ABSENT) {
#ifdef MFPD_MESSAGES
		cmn_err(CE_NOTE, 
		    "!mfpd: Type of port %d : PORT_ABSENT", port_num);
#endif /* MFDP_MESSAGES */
	} else if (((int)p_type >= MFPD_SIMPLE_PP) && 
	    ((int)p_type <= MFPD_COMPAQ)) {
#ifdef MFPD_MESSAGES
		cmn_err(CE_NOTE, "!mfpd: Type of port %d : %s", port_num,
		    port_types[p_type]);
#endif /* MFDP_MESSAGES */
	} else {
#ifdef MFPD_MESSAGES
		cmn_err(CE_NOTE, "!mfpd: Type of port %d : Error", port_num);
#endif /* MFDP_MESSAGES */
	}

	return;
}


#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)

/* 
 * mfpd_get_parameters_cm()
 * 	Gets the interrupt vector, io address etc from the Configuration Manager
 *      and initializes the fields in mfpd_cfg structure.
 *
 * Calling/Exit State :
 *
 * Description :
 */

/* ARGSUSED */
STATIC int
mfpd_get_parameters_cm(int dev, unsigned int bus_p)
{
	cm_args_t cm_args;
	struct cm_addr_rng ioaddr;
	int	error = 0;
	int	x;

	cm_args.cm_key = cm_getbrdkey("mfpd", dev);
	cm_args.cm_n = 0;

	/* Get the IRQ from the Configuration Manager */
	cm_args.cm_param = CM_IRQ;
	cm_args.cm_val = &(mfpdcfg[dev].vect);
	cm_args.cm_vallen = sizeof(mfpdcfg[dev].vect);
	if (cm_getval(&cm_args)) {
		error = 1;
		mfpdcfg[dev].vect = 0;
	} else {
		if (mfpdcfg[dev].vect == 0)
			error = 1;
	}

	/* Get the start and end addresses */
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &ioaddr;
	cm_args.cm_vallen = sizeof(struct cm_addr_rng );
	if (cm_getval(&cm_args)) {
		error = 1;
		mfpdcfg[dev].base = 0;
		mfpdcfg[dev].end = 0;
	} else {
		mfpdcfg[dev].base = ioaddr.startaddr;
		mfpdcfg[dev].end = ioaddr.endaddr;
	}

	/* Get the interrupt priority level */
	cm_args.cm_param = CM_IPL;
	cm_args.cm_val = &(mfpdcfg[dev].ip_level);
	cm_args.cm_vallen = sizeof(mfpdcfg[dev].ip_level);
	if (cm_getval(&cm_args)) {
		error = 1;
		mfpdcfg[dev].ip_level = 0;
	}


	/* Get the DMA channel used by the port */
	cm_args.cm_param = CM_DMAC;
	cm_args.cm_val = &(mfpdcfg[dev].dma_channel);
	cm_args.cm_vallen = sizeof(mfpdcfg[dev].dma_channel);
	/* parallel ports *may* use DMA */
	x = cm_getval(&cm_args);
	if (x) {
		if (x == ENOENT) {
			mfpdcfg[dev].dma_channel = -1;
		} else {
			error = 1;
			mfpdcfg[dev].dma_channel = -1;
		}
	}

	return error;
}


/* 
 * mfpd_verify()
 * 	Checks for the presence of boards/controllers.
 *
 * Calling/Exit State :
 *
 * Description :
 *	This routine is invoked from the DCU.
 */

int	
mfpd_verify(rm_key_t key)
{
	cm_args_t cm_args;
	struct cm_addr_rng ioaddr;
	unsigned int	vect;
	unsigned	base;
	unsigned char	testval;


	cm_args.cm_key = key;
	cm_args.cm_n = 0;

	/* Get the IRQ */
	cm_args.cm_param = CM_IRQ;
	cm_args.cm_val = &(vect);
	cm_args.cm_vallen = sizeof(vect);
	if (cm_getval(&cm_args)) {
		return EINVAL;
	}

	/* Get the base address */
	cm_args.cm_param = CM_IOADDR;
	cm_args.cm_val = &ioaddr;
	cm_args.cm_vallen = sizeof(struct cm_addr_rng );
	if (cm_getval(&cm_args)) {
		return EINVAL;
	} else {
		base = ioaddr.startaddr;
	}

	/*
	 * The code below does not consider the case of superI/O chips. 
	 * The resource manager does not provide information on the port type
	 * yet and we have no method for determining the port type from the 
	 * key passed to this routine.
 	 */

	/* Probe for the board */
	outb(base, 0x55);
	testval = ((short)inb(base) & 0xFF);
	if (testval != 0x55) {
		return ENODEV;
	}
	return 0;

}


#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */



/*   For testing purposes Only     */


#ifdef MFPD_TEST

/*
 * If this is defined, then add the following three entry points
 * to the entry point line in the Master file.
 */

#define MFPD_ACQ_PORT		0
#define MFPD_RELIN_PORT 	1
#define MFPD_ALLOC_PORT		2
#define MFPD_FREE_PORT		3


/* One structure for each parallel port */

#define MFPD_MAX_NUMBER_PP	4
static struct mfpd_rqst mf[MFPD_MAX_NUMBER_PP];
static int	port_allocflag[MFPD_MAX_NUMBER_PP] = { 
	0, 0, 0, 0 };


/* 
 * mfpdioctl()
 * 	If cmd is MFPD_ACQ_PORT, then an attempt to acquire the port is made.
 * 	If cmd is MFPD_RELIN_PORT, then the port is relinquished.
 *	If cmd is MFPD_ALLOC_PORT, portalloc() is executed on the specified 
 *                                  port.
 *	If cmd is MFPD_FREE_PORT, portfree() is executed on the specified port.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

/* ARGSUSED */
int
mfpdioctl(dev_t dev, int cmd, caddr_t arg, int mode, 
cred_t *cred_p, int *rval_p)
{
	int	ret;
	unsigned int	port_no;


	if (copyin(arg, (caddr_t)(&port_no), sizeof(unsigned int)) != 0) {
		return - 1;
	}

	if (port_no >= num_mfpd)
		return - 1;

	if (mfpdcfg[port_no].mfpd_type == MFPD_PORT_ABSENT)
		return - 1;


	switch (cmd) {

	case MFPD_ACQ_PORT :
		mf[port_no].port = port_no;
		mf[port_no].intr_cb = NULL;
		mf[port_no].drv_cb = NULL;
		ret = mfpd_request_port(&mf[port_no]);
#ifdef MFPD_TEST_DEBUG
		if (ret == ACCESS_GRANTED) {
			cmn_err(CE_NOTE, "mfpd: granted");
		} else {
			cmn_err(CE_NOTE, "mfpd: refused");
		}
#endif /* MFPD_TEST_DEBUG */
		break;

	case MFPD_RELIN_PORT :
		if (!mfpd_relinquish_port(&mf[port_no])) {
#ifdef MFPD_TEST_DEBUG
			cmn_err(CE_NOTE, "mfpd: released");
#endif /* MFPD_TEST_DEBUG */
		} else {
#ifdef MFPD_TEST_DEBUG		
			cmn_err(CE_NOTE, "mfpd: Sorry");
#endif /* MFPD_TEST_DEBUG */
		}
		break;

	case MFPD_ALLOC_PORT :
		if (mfpd_portalloc(mfpdcfg[port_no].base, 
		    mfpdcfg[port_no].end)) {
			port_allocflag[port_no] = 1;
#ifdef MFPD_TEST_DEBUG		
			cmn_err(CE_NOTE, "mfpd: PORT_ALLOCATED");
#endif /* MFPD_TEST_DEBUG */
		} else {
#ifdef MFPD_TEST_DEBUG		
			cmn_err(CE_NOTE, "mfpd: PORT NOT ALLOCATED");
#endif /* MFPD_TEST_DEBUG */
		}
		break;

	case MFPD_FREE_PORT :
		if (port_allocflag[port_no]) {
			mfpd_portfree(mfpdcfg[port_no].base, 
			    mfpdcfg[port_no].end);
			port_allocflag[port_no] = 0;
#ifdef MFPD_TEST_DEBUG		
			cmn_err(CE_NOTE, "mfpd: Port_freed");
#endif /* MFPD_TEST_DEBUG */
		} else {
#ifdef MFPD_TEST_DEBUG		
			cmn_err(CE_NOTE, "mfpd: Sorry");
#endif /* MFPD_TEST_DEBUG */
		}
		break;

	default :
		return - 1;
	}
	return 0;
}


/*
 * mfpdopen()
 * 	Dummy open.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

/* ARGSUSED */
int	
mfpdopen(dev_t *dev, int flag, int sflag, struct cred *cred_p)
{
	return 0;
}


/*
 * mfpdclose()
 * 	Dummy close.
 *
 * Calling/Exit State :
 *
 * Description :
 *
 */

/* ARGSUSED */
int	
mfpdclose(dev_t dev, int flags, int sflags, struct cred *cred_p)
{
	return 0;
}


#endif /* MFPD_TEST */

