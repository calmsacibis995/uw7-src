#ident	"@(#)kern-pdi:io/hba/adsa/him_code/vulture.h	1.1"

/* $Header$ */
/****************************************************************************
*
* 	Equates and definitions to be used by Vulture (AHA-2840VL)
*
*	VULTURE CONTROL REGISTERS  (Pilot Release 06/15/93)
* 
****************************************************************************/
#define	VL_ID			0x56			/* character 'V'								*/
#define	VL_IDA		0x64			/* alternate id  								*/

#define	VESA_SCAN	2				/* scan type for VL board   				*/
#define	EISA_SCAN	1				/* scan type for EISA board				*/
#define	BIEN			0x01			/* bios enable bit 							*/
#define	M_INTR_CH	0x0f			/* mask of int channel						*/
#define	M_INTR_TR	0x80			/* mask of int trigger						*/
#define	M_HC_TH		0Xc0			/* mask of threshold							*/
#define	M_HC_RL		0x3f			/* mask of bus release time 				*/
#define	M_SC_ID		0x07			/* mask of scsi id 							*/
#define	M_SC_IDW		0x0f			/* mask of scsi id of wide 				*/

/****************************************************************************
*  The following hardware registers are mainly for reading NMC9346 EEPROM 
*  contents.  Ref National Semiconductor NMC9346 data sheet for algorithm.
****************************************************************************/
#define  CNTRL0_WR      0xC0     /* EEPROM program register					*/

   #define  EEPROM_DI   0x01     /* EEPROM write data							*/
   #define  EEPROM_SK   0x02     /* EEPROM clock								*/
   #define  EEPROM_CS   0x04     /* EEPROM chip select						*/
	
#define  CNTRL1_WR      0xC1     /* Control register (write)				*/

   #define  TERM_ON     0x01     /* Termination control 1=ON 0=OFF		*/
   #define  SRAM_WREN   0x04     /* Enable shadow ram for write			*/
   #define  SRAM_EN     0x08     /* Enable shadow ram							*/
   #define  PAGE_MODE   0x10     /* Page mode for BIOS						*/
   #define  IRQ_SEL0    0x20     /* Interrupt request select				*/
   #define  IRQ_SEL1    0x40     /* Interrupt request select				*/
   #define  IRQ_SEL2    0x80     /* Interrupt request select				*/

#define  CNTRL0_RD      0xC0     /* Clear timing flag TF						*/

#define  CNTRL1_RD      0xC1     /* Control register read					*/

   #define  EEPROM_DO   0x01     /* EEPROM data out bit		       		*/
   #define  IO_SEL0     0x02     /* I/O port select							*/
   #define  IO_SEL1     0x04     /* I/O port select							*/
   #define  IO_SEL2     0x08     /* I/O port select							*/
   #define  IO_SEL3     0x10     /* I/O port select							*/
   #define  BIOS_SEL0   0x20     /* BIOS address select						*/
   #define  BIOS_SEL1   0x40     /* BIOS address select						*/
   #define  EEPROM_TF   0x80     /* EEPROM timing flag						*/

/************************************************* 
* EEPROM address definition.
* Address is organized as 64x16 (1024 bits).
* Only first 32x16 is used. 		       
*************************************************/
#define EE_Target0	0x00
#define EE_Target1	0x01
#define EE_Target2	0x02
#define EE_Target3	0x03
#define EE_Target4	0x04
#define EE_Target5	0x05
#define EE_Target6	0x06
#define EE_Target7	0x07
#define EE_Target8	0x08
#define EE_Target9	0x09
#define EE_Target10	0x0a
#define EE_Target11	0x0b
#define EE_Target12	0x0c
#define EE_Target13	0x0d
#define EE_Target14	0x0e
#define EE_Target15	0x0f

#define EE_BiosBits	0x10
#define EE_CtrlBits	0x11
#define EE_IrqId	0x12				/* low  order byte of 0x12 				*/
#define EE_Bus_Release	0x12		/* high order byte of 0x12 				*/
#define EE_Checksum	0x1f

/************************************************* 
*  prototype definition
*************************************************/

void e2prom (him_config *config_ptr);
UWORD read_eeprom (UBYTE eeprom_addr, UWORD base_addr);
void wait2usec (UWORD base_addr);

