#ident	"@(#)mfpdhw.h	1.1"


#ifndef _MFPDHW_H
#define _MFPDHW_H

/* 
 * Parallel Port declarations (Standard).
 * If a particular chip set has definitions different from the ones 
 * below, only then declare them.
 */


#define STD_STATUS    1 		/* Status reg. offset from the base*/
#define STD_CONTROL   2			/* Control reg. offset from the base */

/* Defines for extracting the necessary bits from the status register */

#define STD_UNBUSY   0x80	        /* If bit 7 is set -> port is unbusy */
#define STD_NOPAPER  0x20		/* If bit 5 is set -> paper exhausted */

/* Defines for setting the necessary bits in the control register */

#define STD_INPUT    0x20		/* to put the port in the i/p mode */
#define STD_INTR_ON  0x10		/* to enable the interrupts */
#define STD_SEL      0x08		/* to select the printer */
#define STD_RESET    0x04		/* 
					 * reseting bit 2 initializes the 
					 * device connected to the port 
					 */
#define STD_AFD      0x02		/* to cause the printer to line_feed */
#define STD_STROBE   0x01		/* to strobe data into the printer */


/* Standard parallel port addresses */
#define PORT1	0x378
#define PORT2	0x278
#define PORT3	0x3BC



/*
 * PS/2 declaration:
 *
 * Reserved bits in the registers of PS/2 parallel port pose a peculiar
 * problem. They must be written as 0's but they read as 1's !
 * So we use a mask to reset the reserved bits whenever a register is 
 * written.
 */

#define PS_2_CNTL_MASK		0x3F



/*
 * PC87322VF (SuperI/O III) declarations
 */


/* Possible index & data register addresses and their default values */
#define PC87322_INDEX1		0x398
#define PC87322_DATA1		0x399

#define PC87322_INDEX2		0x26E
#define PC87322_DATA2		0x26F

#define PC87322_DEFAULT_INDEX	0x88
#define PC87322_DEFAULT_DATA	0x00


/* Configuration registers : Offsets are as below. */
#define PC87322_FER		0	/* Function Enable Register */
#define PC87322_FAR		1	/* Function Address Register */
#define PC87322_PTR		2	/* Power & Test Register */
#define PC87322_FCR		3	/* Function Control Register */
#define PC87322_PCR		4	/* Printer Control Register */


/* Defines to extract/set bits in the configuration registers */


/* In FER */
#define PC87322_PP_ENABLE	0x01	/* to enable parallel port access */



/* In FAR */
#define PC87322_PPORT		0x03	/* 
					 * to extract the bits defining the 
					 * parallel port address.
					 */

#define PC87322_LPT2		0x00	/* Base address will be 0x378 */
#define PC87322_LPT1		0x01 	/* Base address will be 0x3BC */
#define PC87322_LPT3		0x02	/* Base address will be 0x278 */


/* In PTR */
#define PC87322_EXTENDED	0x80	/* 
					 * to put the parallel port in extended
					 * mode.
					 */
#define PC87322_IRQ_LPT2	0x08    /* to choose the IRQ level for LPT2 */



/* In FCR */
#define PC87322_PPM_ON		0x04	/* 
					 * to enable the parallel port 
					 * multiplexor.
					 */
#define PC87322_ZWS_ENABLE	0x20	/* to enable zero wait state */
#define PC87322_NOT_ZWS_PIN	0x40	/* 
				  	 * if bit 6 is 0, then pin 3 configured
					 * as zero wait state output.
					 */
#define PC87322_MFM_PIN		0x80	/* 
					 * If bit 7 is 0, then pin 53 is 
					 * configured as IOCHRDY pin.
					 */


/* In PCR */
#define PC87322_EPP		0x01	/* EPP enable bit */
#define PC87322_1_9		0x02	/* to select EPP version 1.9 */
#define PC87322_LEVEL_INT	0x10 	/* 
					 * to have level interrupts generated
					 * as against the pulse interrupts.
					 */


/* EPP mode supports additional registers. Thier offsets are as follows : */

#define PC87322_EPPADDR		3	/* EPP address register offset */
#define PC87322_EPPDP0		4	/* EPP data registers' offsets */
#define PC87322_EPPDP1		5
#define PC87322_EPPDP2		6
#define PC87322_EPPDP3		7


/* 
 * The direction bit in the control register should be changed only during
 * EPP idle phase. The loop reading the status register is retried 
 * MFPD_PC87322_RETRY times.
 */

#define MFPD_PC87322_RETRY	5



/*
 * 82360SL declarations 
 */

/* Some of the register addresses */
#define SL82360_CPUPWRMODE	0x22	/* CPUPWRMODE register */
#define SL82360_CFGSTAT		0x23	/* Configuration Status Register */
#define SL82360_INDEX		0x24	/* Configuration index register */
#define SL82360_DATA		0x25	/* Configuration data register */
#define SL82360_SFSINDEX	0xAE	/* Special feature index register */
#define SL82360_SFSDATA		0xAF	/* Special feature data register */
#define SL82360_SFSEN_REG	0xFB	/* SFS enable register */
#define SL82360_PPCONFIG	0x102	/* Parallel port configuration reg. */
/* 
 * SL82360 configuration space is enabled by performing 4 sequential reads to
 * the following addresses:
 */
#define SL82360_CONFENABLE1	0xFC23
#define SL82360_CONFENABLE2	0xF023
#define SL82360_CONFENABLE3	0xC023
#define SL82360_CONFENABLE4	0x0023



#define SL82360_CFGR2		0x61	/* index of configuration reg. 2 */
#define SL82360_IDXLCK		0xFA	/* index of configuration lock reg. */
#define SL82360_FPPCNTL_INDEX	0x02	/* 
					 * index of fast parallel port control
					 * regsister
					 */



/* defines to extract/set necessary bits in the above registers */

#define SL82360_UNLOCKSTAT	0x01	/* 
					 * to extract the unlock status bit
					 * from the CPUPWRMODE register.
					 */
#define SL82360_CPUCNFGLCK	0x01	/* to lock the CPUPWRMODE register */
#define SL82360_CFGSPCLCKSTAT	0x80	/* 
					 * to extract the configuration space
					 * open/closed bit.
					 */
#define SL82360_CFGSPCLCK	0x01	/* to lock the configuration space */



/* In parallel port configuration register */
#define SL82360_BIDI		0x80	/* to enable bidirectional mode */
#define SL82360_PPORT		0x60	/* 
					 * to extract the bits defining the
					 * parallel port address.
					 */
/* defines to set the base address */
#define SL82360_BA378		0x00	/* Base address is 378 */
#define SL82360_BA278		0x20
#define SL82360_BA3BC		0x40



/* In the configuration register 2 */
#define SL82360_PS2_ENABLE	0x01	/* Enable PS/2 compatible registers */
#define SL82360_SFS_ENABLE	0x08	/* To enable the SFS registers */



/* In the fast parallel port control register */
#define SL82360_FPP_ENABLE 	0x80	/* to enable the FPP mode */
#define SL82360_FPP_BIDI	0x40	/* bidirectional mode for FPP */
#define SL82360_FPP_PPORT	0x30	/* 
					 * to extract the bits defining the
					 * port address.
					 */
#define SL82360_FPP378		0x10	/* to set the port addr. to 378 */
#define SL82360_FPP278		0x20	/* to set the port addr. to 278 */


/* used in the dummy read to SFS enable register */
#define SL82360_SFSEN_DUMMY	0x00 	


/* Fast mode supports additional registers. Their offsets : */
#define SL82360_FASTADDR	3	/* Address register */
#define SL82360_FASTDP0		4	/* Data registers */
#define SL82360_FASTDP1		5
#define SL82360_FASTDP2		6
#define SL82360_FASTDP3		7



/*
 * AIP82091 declarations
 */

/* Index and data register addresses */
#define AIP82091_INDEX		0x22
#define AIP82091_DATA		0x23


/* Parallel port configuration register and bit definitions */
#define AIP82091_PCFG_INDEX	0x20	/* Offset from the base */


#define AIP82091_PPHMOD_SEL	0x60	/* 
					 * Extract bits defining the parallel
					 * port hardware mode.
					 */
/* Different hardware modes */
#define AIP82091_ISA_SEL	0x00
#define AIP82091_PS2_SEL	0x20
#define AIP82091_EPP_SEL	0x40

#define AIP82091_IRQ7_SEL	0x08	/* to select IRQ 7 as against 5 */


#define AIP82091_PPORT		0x06	/*
					 * to extract the bits defining the 
					 * parallel port base address.
					 */
#define AIP82091_BA378		0x00	/* to select base address = 0x378 */
#define AIP82091_BA278		0x02
#define AIP82091_BA3BC		0x04


#define AIP82091_PP_ENABLE	0x01	/* to enable the parallel port */
#define AIP82091_FIFO_SEL	0x80	/* to select the fifo threshold */

/* Fifo threshold in forward direction is either 1 or 8 */
#define AIP82091_FIFO_FWD1	0x01
#define AIP82091_FIFO_FWD2	0x08
/* Fifo threshold in reverse direction is either 15 or 8 */
#define AIP82091_FIFO_REV1	0x0F
#define AIP82091_FIFO_REV2	0x08



/* EPP mode has additional registers. Their offsets are as follows: */
#define AIP82091_EPPADDR	3	/* address register */
#define AIP82091_EPPDP0		4	/* data registers */
#define AIP82091_EPPDP1		5
#define AIP82091_EPPDP2		6
#define AIP82091_EPPDP3		7


/* ECP mode is controlled by different registers. Their offsets: */
#define AIP82091_ECPAFIFO	0x00	/* ECP addr./RLE FIFO register */
#define AIP82091_SDFIFO		0x400	/* Standard PP data fifo */
#define AIP82091_ECPDFIFO	0x400	/* ECP data fifo */
#define AIP82091_ECP_ECR 	0x402	/* extended control register */

/* defines to extract/set bits in the extended control register */
#define AIP82091_ECP_SEL	0x60	/* to select ECP mode */
#define AIP82091_ECP_CENT_SEL	0x40	/* to select ISA compatible FIFO mode */


#define AIP82091_NO_ERROR_INTR	0x10	/* to disable error interrupts */
#define AIP82091_DMA_ENABLE	0x08	/* to enable DMA */
#define AIP82091_NO_SRVC_INTR	0x04	/* to disable service interrupts */



/*
 * SMCFDC665 declarations.
 */

/* index and data register addresses */
#define SMCFDC665_INDEX		0x3F0
#define SMCFDC665_DATA 		0x3F1


/* values to be written to index reg. to enter and exit config. mode */
#define SMCFDC665_INITVAL	0x55
#define SMCFDC665_EXITVAL	0xAA


/* Offsets of configuration registers */
#define SMCFDC665_CR1		0x01	/* Configuration register 1 */
#define SMCFDC665_CR4		0x04	/* Configuration register 4 */
#define SMCFDC665_CRA		0x0A	/* Configuration register 10 (decimal)*/


/* Bit definitions for CR1 */
#define SMCFDC665_PPORT		0x03	/* to extract addr. select bits */
#define SMCFDC665_BA3BC		0x01	/* to select base addr. = 0x3BC */
#define SMCFDC665_BA378		0x02
#define SMCFDC665_BA278		0x03


#define SMCFDC665_PRN_MODE	0x08	/* 
					 * to select printer mode (centronics)
					 * as against the extended modes  
					 */


/* Bit definitions for CR4 */
#define SMCFDC665_MODE_SEL	0x03	/* to extract mode select bits */
#define SMCFDC665_BIDI_SEL	0x00	/* to select bidirectional mode */
#define SMCFDC665_EPP_SEL	0x01	/* to select EPP mode */
#define SMCFDC665_ECP_SEL	0x02	/* to select ECP mode */


/* Bit definitions for CRA */
#define SMCFDC665_FIFO_THRESH	0x0F	/* 
					 * to extract the lower nibble defining
					 * FIFO threshold
					 */
#define SMCFDC665_THRESHVAL8	0x07	/*
					 * value to be written to CRA to make
					 * FIFO threshold 8 in both directions
					 */

/* EPP mode supports additional registers. Their offsets : */
#define SMCFDC665_EPPADDR	3	/* Address register */
#define SMCFDC665_EPPDP0	4	/* Data registers */
#define SMCFDC665_EPPDP1	5
#define SMCFDC665_EPPDP2	6
#define SMCFDC665_EPPDP3	7



/* ECP mode uses additional registers. Their offsets : */
#define SMCFDC665_ECPAFIFO	0x00	/* ECP fifo : address/RLE */
#define SMCFDC665_SDFIFO	0x400	/* Standard PP data fifo */
#define SMCFDC665_DFIFO		0x400	/* ECP data fifo */
#define SMCFDC665_ECP_ECR 	0x402	/* extended control register */


/* bit definition for extended control register */
#define SMCFDC665_ECP_MODE_SEL	0xD0	/* to extract ECP mode select bits */
#define SMCFDC665_ECP_SEL_ECR	0x60	/* to select ECP parallel port mode */
#define SMCFDC665_ECP_CENT_SEL	0x40	/* to select PP data fifo mode */
#define SMCFDC665_ECP_BIDI_SEL	0x20	/* to select PS/2 PP mode */
#define SMCFDC665_NO_ERR_INTR	0x10	/* to disable error interrupts */
#define SMCFDC665_DMA_ENABLE	0x08	/* to enable DMA */
#define SMCFDC665_NO_SRVC_INTR	0x04	/* to disable service interrupts */
#define SMCFDC665_IS_FIFO_FULL	0x02	/* to check whether the fifo is full */
#define SMCFDC665_IS_FIFO_EMP	0x01	/* to check whether the fifo is empty */



/*
 * Compaq declarations
 */

#define MFPD_COMPAQ_DMAPORT	0x0C7B 	/*
					 * Bit 0 of this port controls DMA
					 * transfers.
					 */
#define MFPD_COMPAQ_IOPORT	0x0065	/*
					 * Bit 7 of this port controls the
					 * parallel port direction.
					 */


#define MFPD_COMPAQ_DMA_ENABLE	0x01	/* Enable DMA transfers */

#define MFPD_COMPAQ_OUTPUT	0x80	/* Enable output mode */


/* Public function declarations */

extern int	mfpd_determine_pc87322_address(void);
extern int	mfpd_determine_aip82091_address(void);
extern unsigned long	mfpd_get_options_avail(unsigned long port_no, int direction);
extern void	mfpd_set_capability(unsigned long port_no);

#endif  /* _MFPDHW_H  */
