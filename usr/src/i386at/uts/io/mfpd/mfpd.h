#ident	"@(#)mfpd.h	1.1"

/*
 * The header file for multifunction parallel port.
 */

#ifndef _MFPD_H
#define _MFPD_H

/*
 * The 4 symbols defined below give the different values for comparing with
 * MFPD_PDI_VERSION. These are taken from hba.h ( and the prefix MFPD is
 * attached to them).
 */

#define MFPD_PDI_UNIXWARE11	1
#define MFPD_PDI_SVR42_DTMT	2
#define MFPD_PDI_SVR42MP	3
#define MFPD_PDI_UNIXWARE20	4


/*
 * Get PDI_VERSION from sdi.h
 */
#ifdef _KERNEL_HEADERS
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#elif defined(_KERNEL)
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#endif /* _KERNEL_HEADERS */

#define MFPD_PDI_VERSION        PDI_VERSION


/*  Types of parallel port chip sets supported   */

#define MFPD_PORT_ABSENT     1000	/* Port not present */
#define MFPD_SIMPLE_PP       0		/* Simple parallel port */
#define MFPD_PS_2            1		/* PS/2 parallel port */
#define MFPD_PC87322	     2		/* PC87322VF SuperI/O chip */
#define MFPD_SL82360	     3		/* 82360SL SuperI/O chip */
#define MFPD_AIP82091	     4		/* AIP82091 SuperI/O chip */
#define MFPD_SMCFDC665	     5		/* SMC FDC37C665 SuperI/O chip */
#define MFPD_COMPAQ	     6		/* Compaq parallel port */


/* Capabilities of the chip sets */

#define MFPD_CAPABILITY      1

#define MFPD_CENTRONICS         0x01    /* Centronics mode : Output only */
#define MFPD_BIDIRECTIONAL      0x02    /* Supports both input and output */
#define MFPD_EPP_MODE           0x04    /* supports EPP */
#define MFPD_ECP_MODE           0x08    /* supports ECP */
#define MFPD_ECP_CENTRONICS	0x10    /* Centronics while in ECP mode */
#define MFPD_HW_COMP            0x20    /* supports hardware compression */
#define MFPD_HW_DECOMP          0x40    /* supports hardware decompression */
#define MFPD_PROPRIETARY        0x80    /* proprietary */


#define MFPD_MODE_UNDEFINED 	0x00	/* Current mode not known */



/* The various modes of data xfer supported by the different chip sets  */


#define MFPD_DIRECTION       2

#define MFPD_FORWARD_CHANNEL    1      /* Output */
#define MFPD_REVERSE_CHANNEL    2      /* Input */



#define MFPD_BYTE_FORWARD	0x01    /* supports byte mode in forward
					   direction. All parallel ports
					   do this */
#define MFPD_DMA_FORWARD        0x02    /* supports DMA in forward direction */

#define MFPD_NIBBLE_REVERSE     0x04    /* supports nibble mode in reverse  
                                           direction  */
#define MFPD_BYTE_REVERSE       0x08    /* supports byte mode in reverse  
                                           direction  */
#define MFPD_DMA_REVERSE        0x10    /* supports DMA in reverse direction  */


#define MFPD_NO_OPTION          0x00    /* Return this for indicating error */



/* Hardware routines of each chip set */

struct mfpd_hw_routines {
	int	(*init)(unsigned long);           /* Initialize */
	int	(*enable_intr)(unsigned long);    /* Enable interrupts */
	int	(*disable_intr)(unsigned long);   /* Disable interrupts */
	int	(*get_fifo_threshold)(unsigned long, int *, int *);
	/* Get the fifo threshold */
	int	(*get_status)(unsigned long, int, unsigned long *);
	/* Get the status about the port */
	int	(*select_mode)(unsigned long, int, unsigned long);
	/* Select the specified mode */

};



#define MFPD_MAX_DEVICES  8       /* Maximum number of devices that may be 
                                    connected to any parallel port */

/* Device information structure */

struct mfpd_dev_spec {
	unsigned long	device_num;        /* device number */
	unsigned long	device_type;       /* type of the device */
};


/* The request block structure */

struct mfpd_rqst {
	unsigned long	port;      /* the port number requested */
	void	(*intr_cb)(struct mfpd_rqst *);
	/* interrupt handler for the upper layer */
	void	(*drv_cb)(struct mfpd_rqst *);
	/* driver call back routine to inform the
           upper layer when the port becomes free */
	struct mfpd_rqst *next;  /* to chain the requests in the request Q */
};


/* Port configuration structure */

struct mfpd_cfg {

	unsigned long	mfpd_type;    /* type of the port */
	unsigned	base;         /* base register address (DATA REGISTER)*/
	unsigned	end;          /* last register address in the io range*/
	int	dma_channel;          /* dma_channel used by the port */
	unsigned long	ip_level;     /* intr. priority level used by the port*/
	unsigned int	vect;         /* interrupt vector */
	unsigned long	capability;   /* the capability of the port */
	unsigned long	cur_mode;     /* mode in use */
	struct mfpd_hw_routines *pmhr;   /* ptr to one of the entries in mhr */
	int	num_devices;	      /* # devices connected to the port */
	struct mfpd_dev_spec mfpd_device[MFPD_MAX_DEVICES]; /* array of devices
                                         connected to the port */
	unsigned long	excl_flag;    /* flag indicating excl. access to port */
	int	access_delay;         /* indicates whether the access to the 						 port has been delayed or not */

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
	void	*intr_cookie;		 /* cookie for interrupts */
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */

	struct mfpd_rqst *q_head;        /* head pointer of the request queue */
	struct mfpd_rqst *q_tail;        /* tail pointer of the request queue */
};


/* Flags indicating port status */

#define PORT_FREE         0		/* indicates Parallel port is free */
#define PORT_ACQUIRED     1		/* indicates that port is acquired */


/*
 * Define these as non-negative numbers. mfpd_request_port() returns these
 * to indicate whether access to the port is granted or not.
 */
#define ACCESS_GRANTED    0
#define ACCESS_REFUSED    1


/*
 * mfpd uses ipl of 5 (System file) hence spldisk() is being used. 
 * spldisk() however is not defined in Unixware1.1 . So spl5() is used. 
 */

#if (MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20)
#define MFPD_SPL()   spldisk()
#else
#define MFPD_SPL()   spl5()
#endif /* MFPD_PDI_VERSION >= MFPD_PDI_UNIXWARE20 */



/* Various types of devices that can be connected to the parallel port */

#define MFPD_DEVICE_ADDR  3

#define MFPD_PRINTER 	  1	/* printer device */
#define MFPD_TRAKKER      2	/* trakker device */

/*
 * mfpd is single threaded. The following #define gives the processor to 
 * which mfpd is bound. The drivers which use mfpd services must also
 * be bound to the same proccessor.
 * Relevant only in Unixware 2.0 or above.
 */

#define MFPD_PROCESSOR	0



/* MCA bus defines */

#define SYS_ENAB	0x94		/* System enable register */
#define ADAP_ENAB	0x96		/* adapter enable register */
#define POS_2		0x102		/* POS_2 register */


/* Public function declarations */

extern int	mfpd_request_port(struct mfpd_rqst *prqst);
extern int	mfpd_relinquish_port(struct mfpd_rqst *prqst);
extern unsigned long	mfpd_get_port_type(unsigned long port_no);
extern unsigned	mfpd_get_baseaddr(unsigned long port_no);
extern unsigned long	mfpd_get_capability(unsigned long port_no);
extern int	mfpd_get_port_count(void);
extern int	mfpd_query(unsigned long port_no, int enquiry_type, unsigned long device_type, unsigned long *result);

extern int	(*mfpd_mhr_init(unsigned long))(unsigned long);
extern int	(*mfpd_mhr_enable_intr(unsigned long))(unsigned long);
extern int	(*mfpd_mhr_disable_intr(unsigned long))(unsigned long);
extern int	(*mfpd_mhr_get_fifo_threshold(unsigned long))(unsigned long, int *, int*);
extern int	(*mfpd_mhr_get_status(unsigned long))(unsigned long, int, unsigned long *);
extern int	(*mfpd_mhr_select_mode(unsigned long))(unsigned long, int, unsigned long);


#endif   /* _MFPD_H  */

