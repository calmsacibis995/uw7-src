#ident	"@(#)kern-pdi:io/hba/adss/him_code/aic6x60.h	1.2"

/* Bit masks for SCSI Sequence Control (SCSISEQ) */

#define TEMODEO 0x80
#define ENSELO 0x40
#define ENSELI 0x20
#define ENRESELI 0x10
#define ENAUTOATNO 0x08
#define ENAUTOATNI 0x04
#define ENAUTOATNP 0x02
#define SCSIRSTO 0x01

/* Bit masks for SCSI Transfer Control 0 (SXFRCTL0) */

#define SCSIEN 0x80
#define DMAEN 0x40
#define CHEN 0x20
#define CLRSTCNT 0x10
#define SPIOEN 0x08
#define CLRCH 0x02

/* Bit masks for SCSI Transfer Control 1 (SXFRCTL1) */

#define BITBUCKET 0x80
#define SWRAPEN 0x40
#define ENSPCHK 0x20
#define STIMESEL 0x18
#define STIMESEL256 0x00
#define STIMESEL128 0x08
#define STIMESEL64 0x10
#define STIMESEL32 0x18
#define ENSTIMER 0x04
#define BYTEALIGN 0x02

/* Bit masks for SCSI Signals (SCSISIGI/SCSISIGO) */

#define CD 0x80
#define IO 0x40
#define MSG 0x20
#define ATN 0x10
#define SEL 0x08
#define BSY 0x04
#define REQ 0x02
#define ACK 0x01

/* SCSI bus phase definitions from SCSISIGI/SCSISIGO */

#define BUS_FREE_PHASE 0xFF		/* Special token */
#define SCSI_PHASE_MASK 0xE0		/* Clear out irrelevant signals */
#define DATA_OUT_PHASE 0
#define DATA_IN_PHASE IO
#define NON_DATA_PHASES (CD | MSG)	/* Mask for all following phases */
#define COMMAND_PHASE CD
#define MESSAGE_OUT_PHASE (CD | MSG)
#define STATUS_PHASE (CD | IO)
#define MESSAGE_IN_PHASE (CD | IO | MSG)


/* Bit masks for SCSI Rate Control (SCSIRATE) */

#define SXFR 0x70
#define SOFS 0x0F

/* Bit masks for SCSI ID (SCSIID) */

#define TID 0x70
#define OID 0x07

/* Bit masks for Clear SCSI Interrupts 0 (CLRSINT0) */

#define SETSDONE 0x80
#define CLRSELDO 0x40
#define CLRSELDI 0x20
#define CLRSELINGO 0x10
#define CLRSWRAP 0x08
#define CLRSDONE 0x04
#define CLRSPIORDY 0x02
#define CLRDMADONE 0x01

/* Bit masks for SCSI Status 0 (SSTAT0) */

#define TARGET 0x80
#define SELDO 0x40
#define SELDI 0x20
#define SELINGO 0x10
#define SWRAP 0x08
#define SDONE 0x04
#define SPIORDY 0x02
#define DMADONE 0x01

/* Bit masks for Clear SCSI Interrupts 1 (CLRSINT1) */

#define CLRSELTIMO 0x80
#define CLRATNO 0x40
#define CLRSCSIRSTI 0x20
#define CLRBUSFREE 0x08
#define CLRSCSIPERR 0x04
#define CLRPHASECHG 0x02
#define CLRREQINIT 0x01

/* Bit masks for SCSI Status 1 (SSTAT1) */

#define SELTO 0x80
#define ATNTARG 0x40
#define SCSIRSTI 0x20
#define PHASEMIS 0x10
#define BUSFREE 0x08
#define SCSIPERR 0x04
#define PHASECHG 0x02
#define REQINIT 0x01

/* Bit masks for SCSI Status 2 (SSTAT2) */

#define SOFFSET 0x20
#define SEMPTY 0x10
#define SFULL 0x08
#define SFCNT 0x0F

/* Bit masks for SCSI Status 3 (SSTAT3) */

#define SCSICNT 0xF0
#define OFFCNT 0x0F

/* Bit masks for SCSI Test Control (SCSITEST) */

#define SCTESTU 0x08
#define SCTESTD 0x04
#define STCTEST 0x01

/* Bit masks for SCSI Status 4 (SSTAT4) */

#define SYNCERR 0x04
#define FWERR 0x02
#define FRERR 0x01

/* Bit masks for Clear SCSI Errors (CLRSERR) */

#define CLRSYNCERR 0x04
#define CLRFWERR 0x02
#define CLRFRERR 0x01

/* Bit masks for SCSI Interrupt Mode 0 (SIMODE0) */

#define ENSELDO 0x40
#define ENSELDI 0x20
#define ENSELINGO 0x10
#define ENSWRAP 0x08
#define ENSDONE 0x04
#define ENSPIORDY 0x02
#define ENDMADONE 0x01

/* Bit masks for SCSI Interrupt Mode 1 (SIMODE1) */

#define ENSELTIMO 0x80
#define ENATNTARG 0x40
#define ENSCSIRST 0x20
#define ENPHASEMIS 0x10
#define ENBUSFREE 0x08
#define ENSCSIPERR 0x04
#define ENPHASECHG 0x02
#define ENREQINIT 0x01

/* Bit masks for DMA Control 0 (DMACNTRL0) */

#define ENDMA 0x80
#define FIFO_8 0x40
#define FIFO_16 0x00
#define FIFO_DMA 0x20
#define FIFO_PIO 0x00
#define DWORDPIO 0x10
#define FIFO_READ 0x00
#define FIFO_WRITE 0x08
#define INTEN 0x04
#define RSTFIFO 0x02
#define SWINT 0x01

/* Bit masks for DMA Control 1 (DMACNTRL1) */

#define PWRDWN 0x80
#define ENSTK32 0x40
#define STK 0x1F

/* Bit masks for DMA Status (DMASTAT) */

#define ATDONE 0x80
#define WORDRDY 0x40
#define INTSTAT 0x20
#define DFIFOFULL 0x10
#define DFIFOEMP 0x08
#define DFIFOHF 0x04
#define DWORDRDY 0x02

/* Bit masks for Burst Control (BRSTCNTRL) */

#define BON 0xF0
#define BOFF 0x0F

/* Bit masks for Port A (PORTA) for AHA-1510 and AHA-152X only */

#define LED_OFF 0x00
#define LED_ON 0x01

/* Bit masks for Test Register (TEST) */

#define BOFFTMR 0x40
#define BONTMR 0x20
#define STCNTH 0x10
#define STCNTM 0x08
#define STCNTL 0x04
#define SCSIBLK 0x02
#define DMABLK 0x01
