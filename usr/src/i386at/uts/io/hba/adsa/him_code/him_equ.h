#ident	"@(#)kern-pdi:io/hba/adsa/him_code/him_equ.h	1.2"

/* $Header$ */
/****************************************************************************

 Equates and definitions to be used exclusively by the Sparrow Driver modules

****************************************************************************/

#define  FIFOSIZE       64       /* size of Arrow dma fifo [bytes]         */
#define  NARROW_OFFSET  15       /* maximum 8-bit sync transfer offset     */
#define  WIDE_OFFSET    8        /* maximum 16-bit sync transfer offset    */
#define  WIDE_WIDTH     1        /* maximum tarnsfer width = 16 bits       */
#define  QDEPTH         4        /* Number of commands Arrow can queue up  */

                                 /* scsi messages -                        */
#define  MSG00          0x00     /*   - command complete                   */
#define  MSG01          0x01     /*   - extended message                   */
#define  MSGSYNC        0x01     /*       - synchronous data transfer msg  */
#define  MSGWIDE        0x03     /*       - wide data transfer msg         */
#define  MSG02          0x02     /*   - save data pointers                 */
#define  MSG03          0x03     /*   - restore data pointers              */
#define  MSG04          0x04     /*   - disconnect                         */
#define  MSG05          0x05     /*   - initiator detected error           */
#define  MSG06          0x06     /*   - abort                              */
#define  MSG07          0x07     /*   - message reject                     */
#define  MSG08          0x08     /*   - nop                                */
#define  MSG09          0x09     /*   - message parity error               */
#define  MSG0A          0x0a     /*   - linked command complete            */
#define  MSG0B          0x0b     /*   - linked command complete            */
#define  MSG0C          0x0c     /*   - bus device reset                   */
#define  MSG0D          0x0d     /*   - abort tag                          */
#define  MSGTAG         0x20     /*   - tag queuing                        */
#define  MSGID          0x80     /* identify message, no disconnection     */
#define  MSGID_D        0xc0     /* identify message, disconnection        */

/****************************************************************************

 ARROW SCSI REGISTERS

****************************************************************************/

#define  SCSISEQ        0x00     /* scsi sequence control     (read/write) */

   #define  TEMODEO     0x80     /* target enable mode out                 */
   #define  ENSELO      0x40     /* enable selection out                   */
   #define  ENSELI      0x20     /* enable selection in                    */
   #define  ENRSELI     0x10     /* enable reselection in                  */
   #define  ENAUTOATNO  0x08     /* enable auto attention out              */
   #define  ENAUTOATNI  0x04     /* enable auto attention in               */
   #define  ENAUTOATNP  0x02     /* enable auto attention parity           */
   #define  SCSIRSTO    0x01     /* scsi reset out                         */


#define  SXFRCTL0       0x01     /* scsi transfer control 0   (read/write) */

   #define  CLRSTCNT    0x10     /* clear Scsi Transfer Counter            */
   #define  SPIOEN      0x08     /* enable auto scsi pio                   */
   #define  CLRCHN      0x02     /* clear Channel n                        */


#define  SXFRCTL1       0x02     /* scsi transfer control 1   (read/write) */
                                 
   #define  BITBUCKET   0x80     /* enable bit bucket mode                 */
   #define  SWRAPEN     0x40     /* enable wrap-around                     */
   #define  ENSPCHK     0x20     /* enable scsi parity checking            */
   #define  STIMESEL1   0x10     /* select selection timeout:  00 - 256 ms,*/
   #define  STIMESEL0   0x08     /* 01 - 128 ms, 10 - 64 ms, 11 - 32 ms    */
   #define  ENSTIMER    0x04     /* enable selection timer                 */
   #define  ACTNEG      0x02     /* SCSI active negation on                */
   #define  ACTNEGON    0x02     /* enable active negation                 */


#define  SCSISIG        0x03     /* actual scsi bus signals   (write/read) */

   #define  CDI         0x80     /* c/d                                    */
   #define  IOI         0x40     /* i/o                                    */
   #define  MSGI        0x20     /* msg                                    */
   #define  ATNI        0x10     /* atn                                    */
   #define  SELI        0x08     /* sel                                    */
   #define  BSYI        0x04     /* bsy                                    */
   #define  REQI        0x02     /* req                                    */
   #define  ACKI        0x01     /* ack                                    */

   #define  CDO         0x80     /* expected c/d  (initiator mode)         */
   #define  IOO         0x40     /* expected i/o  (initiator mode)         */
   #define  MSGO        0x20     /* expected msg  (initiator mode)         */
   #define  ATNO        0x10     /* set atn                                */
   #define  SELO        0x08     /* set sel                                */
   #define  BSYO        0x04     /* set busy                               */
   #define  REQO        0x02     /* not functional in initiator mode       */
   #define  ACKO        0x01     /* set ack                                */

   #define  BUSPHASE    CDO+IOO+MSGO   /* scsi bus phase -                 */
   #define  DOPHASE     0x00           /*  data out                        */
   #define  DIPHASE     IOO            /*  data in                         */
   #define  CMDPHASE    CDO            /*  command                         */
   #define  MIPHASE     CDO+IOO+MSGO   /*  message in                      */
   #define  MOPHASE     CDO+MSGO       /*  message out                     */
   #define  STPHASE     CDO+IOO        /*  status                          */


#define  SCSIRATE       0x04     /* scsi rate control      (write)         */

   #define  WIDEXFER    0x80     /* ch 0 wide scsi bus                     */
   #define  SXFR2       0x40     /* synchronous scsi transfer rate         */
   #define  SXFR1       0x20     /*                                        */
   #define  SXFR0       0x10     /*                                        */
   #define  SXFR        SXFR2+SXFR1+SXFR0       /*                         */
   #define  SOFS3       0x08     /* synchronous scsi offset                */
   #define  SOFS2       0x04     /*                                        */
   #define  SOFS1       0x02     /*                                        */
   #define  SOFS0       0x01     /*                                        */
   #define  SOFS        SOFS3+SOFS2+SOFS1+SOFS0 /*                         */

   #define  SYNCRATE_5MB   0x80  /* 5 MB/sec transfer rate */
   #define  SYNCRATE_10MB  0x00  /* 10 MB/sec transfer rate */

#define  SCSIID         0x05     /* scsi id       (read/write)                  */

   #define  TID3        0x80     /* other scsi device id                   */
   #define  TID2        0x40     /*                                        */
   #define  TID1        0x20     /*                                        */
   #define  TID0        0x10     /*                                        */
   #define  OID3        0x08     /* my id                                  */
   #define  OID2        0x04     /*                                        */
   #define  OID1        0x02     /*                                        */
   #define  OID0        0x01     /*                                        */
   #define  TID         TID3+TID2+TID1+TID0  /* scsi device id mask        */
   #define  OID         OID3+OID2+OID1+OID0  /* our scsi device id mask    */

#define  SCSIDATL       0x06     /* scsi latched data, lo     (read/write) */
#define  SCSIDATH       0x07     /* scsi latched data, hi     (read/write) */

#define  STCNT0         0x08     /* scsi transfer count, lsb  (read/write) */
#define  STCNT1         0x09     /*                    , mid  (read/write) */
#define  STCNT2         0x0a     /*                    , msb  (read/write) */

#define  CLRSINT0       0x0b     /* clear scsi interrupts 0   (write)      */

   #define  CLRSELDO    0x40     /* clear seldo interrupt & status         */
   #define  CLRSELDI    0x20     /* clear seldi interrupt & status         */
   #define  CLRSELINGO  0x10     /* clear selingo interrupt & status       */
   #define  CLRSWRAP    0x08     /* clear swrap interrupt & status         */
   #define  CLRSDONE    0x04     /* clear sdone interrupt & status         */
   #define  CLRSPIORDY  0x02     /* clear spiordy interrupt & status       */
   #define  CLRDMADONE  0x01     /* clear dmadone interrupt & status       */


#define  SSTAT0         0x0b     /* scsi status 0       (read)             */

   #define  TARGET      0x80     /* mode = target                          */
   #define  SELDO       0x40     /* selection out completed                */
   #define  SELDI       0x20     /* have been reselected                   */
   #define  SELINGO     0x10     /* arbitration won, selection started     */
   #define  SWRAP       0x08     /* transfer counter has wrapped around    */
   #define  SDONE       0x04     /* not used in mode = initiator           */
   #define  SPIORDY     0x02     /* auto pio enabled & ready to xfer data  */
   #define  DMADONE     0x01     /* transfer completely done               */


#define  CLRSINT1       0x0c     /* clear scsi interrupts 1   (write)      */

   #define  CLRSELTIMO  0x80     /* clear selto interrupt & status         */
   #define  CLRATNO     0x40     /* clear atno control signal              */
   #define  CLRSCSIRSTI 0x20     /* clear scsirsti interrupt & status      */
   #define  CLRBUSFREE  0x08     /* clear busfree interrupt & status       */
   #define  CLRSCSIPERR 0x04     /* clear scsiperr interrupt & status      */
   #define  CLRPHASECHG 0x02     /* clear phasechg interrupt & status      */
   #define  CLRREQINIT  0x01     /* clear reqinit interrupt & status       */


#define  SSTAT1         0x0c     /* scsi status 1       (read)             */

   #define  SELTO       0x80     /* selection timeout                      */
   #define  ATNTARG     0x40     /* mode = target:  initiator set atn      */
   #define  SCSIRSTI    0x20     /* other device asserted scsi reset       */
   #define  PHASEMIS    0x10     /* actual scsi phase <> expected          */
   #define  BUSFREE     0x08     /* bus free occurred                      */
   #define  SCSIPERR    0x04     /* scsi parity error                      */
   #define  PHASECHG    0x02     /* scsi phase change                      */
   #define  REQINIT     0x01     /* latched req                            */


#define  SSTAT2         0x0d     /* scsi status 2       (read)             */

   #define  SFCNT4      0x10     /* scsi fifo count                        */
   #define  SFCNT3      0x08     /*                                        */
   #define  SFCNT2      0x04     /*                                        */
   #define  SFCNT1      0x02     /*                                        */
   #define  SFCNT0      0x01     /*                                        */


#define  SSTAT3         0x0e     /* scsi status 3       (read)             */

   #define  SCSICNT3    0x80     /*                                        */
   #define  SCSICNT2    0x40     /*                                        */
   #define  SCSICNT1    0x20     /*                                        */
   #define  SCSICNT0    0x10     /*                                        */
   #define  OFFCNT3     0x08     /* current scsi offset count              */
   #define  OFFCNT2     0x04     /*                                        */
   #define  OFFCNT1     0x02     /*                                        */
   #define  OFFCNT0     0x01     /*                                        */


#define  SCSITEST       0x0f     /* scsi test control   (read/write)       */

   #define  CNTRTEST    0x02     /*                                        */
   #define  CMODE       0x01     /*                                        */


#define  SIMODE0        0x10     /* scsi interrupt mask 0     (read/write) */

   #define  ENSELDO     0x40     /* enable seldo status to assert int      */
   #define  ENSELDI     0x20     /* enable seldi status to assert int      */
   #define  ENSELINGO   0x10     /* enable selingo status to assert int    */
   #define  ENSWRAP     0x08     /* enable swrap status to assert int      */
   #define  ENSDONE     0x04     /* enable sdone status to assert int      */
   #define  ENSPIORDY   0x02     /* enable spiordy status to assert int    */
   #define  ENDMADONE   0x01     /* enable dmadone status to assert int    */


#define  SIMODE1        0x11     /* scsi interrupt mask 1     (read/write) */

   #define  ENSELTIMO   0x80     /* enable selto status to assert int      */
   #define  ENATNTARG   0x40     /* enable atntarg status to assert int    */
   #define  ENSCSIRST   0x20     /* enable scsirst status to assert int    */
   #define  ENPHASEMIS  0x10     /* enable phasemis status to assert int   */
   #define  ENBUSFREE   0x08     /* enable busfree status to assert int    */
   #define  ENSCSIPERR  0x04     /* enable scsiperr status to assert int   */
   #define  ENPHASECHG  0x02     /* enable phasechg status to assert int   */
   #define  ENREQINIT   0x01     /* enable reqinit status to assert int    */


#define  SCSIBUSL       0x12     /* scsi data bus, lo  direct (read)       */

#define  SCSIBUSH       0x13     /* scsi data bus, hi  direct (read)       */

#define  SHADDR0        0x14     /* scsi/host address      (read)          */
#define  SHADDR1        0x15     /*                                        */
#define  SHADDR2        0x16     /* host address incremented by scsi ack   */
#define  SHADDR3        0x17     /*                                        */

#define  SELID          0x19     /* selection/reselection id  (read)       */

   #define  SELID3      0x80     /* binary id of other device              */
   #define  SELID2      0x40     /*                                        */
   #define  SELID1      0x20     /*                                        */
   #define  SELID0      0x10     /*                                        */
   #define  ONEBIT      0x08     /* non-arbitrating selection detection    */


#define  SBLKCTL        0x1f     /* scsi block control     (read/write)    */

   #define  AUTOFLUSHDIS   0x20  /* disable automatic flush                */
   #define  SELBUS1        0x08  /* select scsi channel 1                  */
   #define  SELWIDE        0x02  /* scsi wide hardware configure           */


/****************************************************************************

 ARROW SCRATCH RAM AREA

****************************************************************************/

#define  EISA_SCRATCH1  0x20     /* offset from base to EISA host registers*/
#define  ISA_SCRATCH1   0x0400   /* offset from base to ISA host registers */

#define  EISA_SCRATCH2  0x40     /* offset from base to EISA host registers*/
#define  ISA_SCRATCH2   0x0800   /* offset from base to ISA host registers */

#define  BIOS_CONFIG    0x0d     /* bios configuration options             */
#define  IGNORE_IN_SCAN 0x0e     /* 2 byte mask for bios scan              */
#define  INT13_DRIVES   0x10     /* table of int13 drives (up to 8)        */
#define  START_UNIT     0x18     /* 2 byte mask for start unit command     */
#define  SCSI_CONFIG    0x1a     /* scsi configuration info (1 byte/bus)   */
#define  INTR_LEVEL     0x1c     /* storage for interrupt level            */
#define  HOST_CONFIG    0x1d     /* threshold and bus release information  */
#define  DMA_CHANNEL    0x1e     /* storage for DMA channel                */
#define  BIOS_CNTRL     0x1f     /* copy of SBLKCTL after reset            */

/****************************************************************************

 ARROW SEQUENCER REGISTERS

****************************************************************************/

#define  EISA_SEQ       0x60     /* offset from base to EISA seq registers */
#define  ISA_SEQ        0x0c00   /* offset from base to ISA seq registers  */

#define  SEQCTL         0x00     /* sequencer control      (read/write)    */

   #define  PERRORDIS   0x80     /* enable sequencer parity errors         */
   #define  PAUSEDIS    0x40     /* disable pause by driver                */
   #define  FAILDIS     0x20     /* disable illegal opcode/address int     */
   #define  FASTMODE    0x10     /* sequencer clock select                 */
   #define  BRKINTEN    0x08     /* break point interrupt enable           */
   #define  STEP        0x04     /* single step sequencer program          */
   #define  SEQRESET    0x02     /* clear sequencer program counter        */
   #define  LOADRAM     0x01     /* enable sequencer ram loading           */


#define  SEQRAM         0x01     /* sequencer ram data     (read/write) */

#define  SEQADDR0       0x02     /* sequencer address 0    (read/write) */

#define  SEQADDR1       0x03     /* sequencer address 1    (read/write) */

#define  ACCUM          0x04     /* accumulator         (read/write) */

#define  SINDEX         0x05     /* source index register     (read/write) */

#define  DINDEX         0x06     /* destination index register   (read/write) */

#define  BRKADDR0       0x07     /* break address, lo      (read/write) */

#define  BRKADDR1       0x08     /* break address, hi      (read/write) */

   #define  BRKDIS      0x80     /* disable breakpoint                     */
   #define  BRKADDR08   0x01     /* breakpoint addr, bit 8                 */


#define  ALLONES        0x09     /* all ones, source reg = 0ffh    (read)  */

#define  ALLZEROS       0x0a     /* all zeros, source reg = 00h    (read)  */

#define  NONE           0x0a     /* no destination, No reg altered (write) */

#define  FLAGS          0x0b     /* flags            (read)                */

   #define  ZERO        0x02     /* sequencer 'zero' flag                  */
   #define  CARRY       0x01     /* sequencer 'carry' flag                 */


#define  SINDIR         0x0c     /* source index reg, indirect      (read) */

#define  DINDIR         0x0d     /* destination index reg, indirect (read) */

#define  FUNCTION1      0x0e     /* funct: bits 6-4 -> 1-of-8 (read/write) */

#define  STACK          0x0f     /* stack, for subroutine returns   (read) */


/****************************************************************************

 ARROW HOST REGISTERS

****************************************************************************/

#define  EISA_HOST      0x80     /* offset from base to EISA host registers*/
#define  ISA_HOST       0x1000   /* offset from base to ISA host registers */

#define  BID0           0x00     /* eisa id       (read/write)             */
   #define  HA_ID_HI    0x04

#define  BID1           0x01     /* eisa id       (read/write)             */
   #define  HA_ID_LO    0x90
   #define  HA_ID       0x9004

#define  BID2           0x02     /* eisa id       (read/write)             */
   #define  HA_PROD_HI  0x77

#define  BID3           0x03     /* eisa id       (read/write)             */
   #define  HA_PROD_LO  0x70
   #define  HA_PRODUCT  0x0077
   #define  HA_PROD_MSK 0x00FF

#define  BCTL           0x04     /* board control       (read/write)       */

   #define  ENABLE      0x01     /* enable board                           */


#define  BUSTIME        0x05     /* bus on/off time     (read/write)       */

   #define  BOFF3       0x80     /* bus on time  [1 us]                    */
   #define  BOFF2       0x40     /*                                        */
   #define  BOFF1       0x20     /*                                        */
   #define  BOFF0       0x10     /*                                        */
   #define  BON3        0x08     /*                                        */
   #define  BON2        0x04     /*                                        */
   #define  BON1        0x02     /*                                        */
   #define  BON0        0x01     /*                                        */
   #define  BOFF        BOFF3+BOFF2+BOFF1+BOFF0 /*                         */
   #define  BON         BON3+BON2+BON1+BON0     /*                         */

   #define  BMECC_MODE  BON0     /* Intel BMECC compatibility mode         */

#define  BUSSPD         0x06     /* bus speed        (read/write)          */

   #define  DFTHRSH1    0x80     /* data fifo threshold control            */
   #define  DFTHRSH0    0x40     /*                                        */
   #define  STBOFF2     0x20     /*                                        */
   #define  STBOFF1     0x10     /*                                        */
   #define  STBOFF0     0x08     /*                                        */
   #define  STBON2      0x04     /*                                        */
   #define  STBON1      0x02     /*                                        */
   #define  STBON0      0x01     /*                                        */
   #define  STBOFF      STBOFF2+STBOFF1+STBOFF0 /*                         */
   #define  STBON       STBON2+STBON1+STBON0    /*                         */


#define  HCNTRL         0x07     /* host control     (read/write)          */

   #define  POWRDN      0x40     /* power down                             */
   #define  SWINT       0x10     /* force interrupt                        */
   #define  IRQMS       0x08     /* 0 = high true edge, 1 = low true level */
   #define  PAUSE       0x04     /* pause sequencer            (write)     */
   #define  PAUSEACK    0x04     /* sequencer is paused        (read)      */
   #define  INTEN       0x02     /* enable hardware interrupt              */
   #define  CHIPRESET   0x01     /* device hard reset                      */


#define  HADDR0         0x08     /* host address 0         (read/write)    */
#define  HADDR1         0x09     /* host address 1         (read/write)    */
#define  HADDR2         0x0a     /* host address 2         (read/write)    */
#define  HADDR3         0x0b     /* host address 3         (read/write)    */

#define  HCNT0          0x0c     /* host count 0        (read/write)       */
#define  HCNT1          0x0d     /* host count 1        (read/write)       */
#define  HCNT2          0x0e     /* host count 2        (read/write)       */

#define  SCBPTR         0x10     /* scb pointer         (read/write)       */

   #define  SCBVAL2     0x04     /* label of element in scb array          */
   #define  SCBVAL1     0x02     /*                                        */
   #define  SCBVAL0     0x01     /*                                        */


#define  INTSTAT        0x11     /* interrupt status    (write)            */

   #define  INTCODE3    0x80     /* seqint interrupt code                  */
   #define  INTCODE2    0x40     /*                                        */
   #define  INTCODE1    0x20     /*                                        */
   #define  INTCODE0    0x10     /*                                        */
   #define  BRKINT      0x08     /* program count = breakpoint             */
   #define  SCSIINT     0x04     /* scsi event interrupt                   */
   #define  CMDCMPLT    0x02     /* scb command complete w/ no error       */
   #define  SEQINT      0x01     /* sequencer paused itself                */
   #define  INTCODE     INTCODE3+INTCODE2+INTCODE1+INTCODE0 /* intr code   */
   #define  ANYINT      BRKINT+SCSIINT+CMDCMPLT+SEQINT   /* any interrupt  */
   #define  ANYPAUSE    BRKINT+SCSIINT+SEQINT   /* any intr that pauses    */

/* ;  Bits 7-4 are written to '0' or '1' by sequencer.
   ;  Bits 3-0 can only be written to '1' by sequencer.  Previous '1's are
   ;  preserved by the write. */


#define  CLRINT         0x12     /* clear interrupt status    (write)      */

   #define  CLRBRKINT   0x08     /* clear breakpoint interrupt  (brkint)   */
   #define  CLRSCSINT   0x04     /* clear scsi interrupt  (scsiint)        */
   #define  CLRCMDINT   0x02     /* clear command complete interrupt       */
   #define  CLRSEQINT   0x01     /* clear sequencer interrupt  (seqint)    */
   #define  LED_OLVTT   0x00     /* LED bits to leave set for Olivetti */

#define  ERROR          0x12     /* hard error       (read)                */

   #define  PARERR      0x08     /* status = sequencer ram parity error    */
   #define  ILLOPCODE   0x04     /* status = illegal command line          */
   #define  ILLSADDR    0x02     /* status = illegal sequencer address     */
   #define  ILLHADDR    0x01     /* status = illegal host address          */


#define  DFCNTRL        0x13     /* data fifo control register (read/write)*/

   #define  WIDEODD     0x40     /* prevent flush of odd last byte         */
   #define  SCSIEN      0x20     /* enable xfer: scsi <-> sfifo    (write) */
   #define  SCSIENACK   0x20     /* SCSIEN clear acknowledge       (read)  */
   #define  SDMAEN      0x10     /* enable xfer: sfifo <-> dfifo   (write) */
   #define  SDMAENACK   0x10     /* SDMAEN clear acknowledge       (read)  */
   #define  HDMAEN      0x08     /* enable xfer: dfifo <-> host    (write) */
   #define  HDMAENACK   0x08     /* HDMAEN clear acknowledge       (read)  */
   #define  DIRECTION   0x04     /* transfer direction = write             */
   #define  FIFOFLUSH   0x02     /* flush data fifo to host                */
   #define  FIFORESET   0x01     /* reset data fifo                        */


#define  DFSTATUS       0x14     /* data fifo status    (read)             */

   #define  MREQPEND    0x10     /* master request pending                 */
   #define  HDONE       0x08     /* host transfer done:                    */
                                 /*  hcnt=0 & bus handshake done           */
   #define  DFTHRSH     0x04     /* threshold reached                      */
   #define  FIFOFULL    0x02     /* data fifo full                         */
   #define  FIFOEMP     0x01     /* data fifo empty                        */


#define  DFWADDR0       0x15     /* data fifo write address   (read/write) */
#define  DFWADDR1       0x16     /* reserved         (read/write)          */

#define  DFRADDR0       0x17     /* data fifo read address    (read/write) */
#define  DFRADDR1       0x18     /* reserved         (read/write)          */

#define  DFDAT          0x19     /* data fifo data register   (read/write) */

#define  SCBCNT         0x1a     /* SCB count register     (read/write)    */

   #define  SCBAUTO     0x80     /*                                        */


#define  QINFIFO        0x1b     /* queue in fifo       (read/write)       */

#define  QINCNT         0x1c     /* queue in count         (read)          */

   #define  QINCNT2     0x04     /*                                        */
   #define  QINCNT1     0x02     /*                                        */
   #define  QINCNT0     0x01     /*                                        */


#define  QOUTFIFO       0x1d     /* queue out fifo         (read/write)    */

#define  QOUTCNT        0x1e     /* queue out count     (read)             */

#define  TESTCHIP       0x1f     /* test chip        (read/write)          */

   #define  TESTHOST    0x08     /* select Host module for testing         */
   #define  TESTSEQ     0x04     /* select Sequencer module for testing    */
   #define  TESTFIFO    0x02     /* select Fifo module for testing         */
   #define  TESTSCSI    0x01     /* select Scsi module for testing         */


/****************************************************************************

 ARROW HOST REGISTERS

****************************************************************************/

#define  EISA_ARRAY     0xA0     /* offset from base to EISA SCB array     */
#define  ISA_ARRAY      0x1400   /* offset from base to ISA SCB array      */

#define  SCB00          0x00     /* scb array  (read/write)                */
#define  SCB01          0x01
#define  SCB02          0x02
#define  SCB03          0x03
#define  SCB04          0x04
#define  SCB05          0x05
#define  SCB06          0x06
#define  SCB07          0x07
#define  SCB08          0x08
#define  SCB09          0x09
#define  SCB10          0x0a
#define  SCB11          0x0b
#define  SCB12          0x0c
#define  SCB13          0x0d
#define  SCB14          0x0e
#define  SCB15          0x0f
#define  SCB16          0x10
#define  SCB17          0x11
#define  SCB18          0x12
#define  SCB19          0x13
#define  SCB20          0x14
#define  SCB21          0x15
#define  SCB22          0x16
#define  SCB23          0x17
#define  SCB24          0x18
#define  SCB25          0x19
#define  SCB26          0x1a
#define  SCB27          0x1b
#define  SCB28          0x1c
#define  SCB29          0x1d
#define  SCB30          0x1e
#define  SCB31          0x1f

/****************************************************************************

 Driver - Sequencer interface

****************************************************************************/

/* INTCODES  - */

/* Seqint Driver interrupt codes identify action to be taken by the Driver.*/

#define  SYNC_NEGO_NEEDED  0x00     /* initiate synchronous negotiation    */
#define  ATN_TIMEOUT       0x00     /* timeout in atn_tmr routine          */
#define  CDB_XFER_PROBLEM  0x10     /* possible parity error in cdb:  retry*/
#define  HANDLE_MSG_OUT    0x20     /* handle Message Out phase            */
#define  DATA_OVERRUN      0x30     /* data overrun detected               */
#define  UNKNOWN_MSG       0x40     /* handle the message in from target   */
#define  CHECK_CONDX       0x50     /* Check Condition from target         */
#define  PHASE_ERROR       0x60     /* unexpected scsi bus phase           */
#define  EXTENDED_MSG      0x70     /* handle Extended Message from target */
#define  ABORT_TARGET      0x80     /* abort connected target              */
#define  NO_ID_MSG         0x90     /* reselection with no id message      */

/****************************************************************************

 External SCB stuff

****************************************************************************/

#define SCRATCH   EISA_SCRATCH1

#define  WAITING_SCB_CH0   SCRATCH+27     /* 1 byte index         C3B */
#define  ACTIVE_SCB        SCRATCH+28     /* 1 byte index         C3C */
#define  WAITING_SCB_CH1   SCRATCH+29     /* 1 byte index         C3D */
#define  SCB_PTR_ARRAY     SCRATCH+30     /* 4 byte ptr           C3E */
#define  QIN_CNT           SCRATCH+34     /* 1 byte count         C42 */
#define  QIN_PTR_ARRAY     SCRATCH+35     /* 4 byte ptr to array  C43 */
#define  NEXT_SCB_ARRAY    SCRATCH+39     /* 16 byte index array  C47 */
#define  QOUT_PTR_ARRAY    SCRATCH+55     /* 4 byte ptr to array  C57 */
#define  BUSY_PTR_ARRAY    SCRATCH+59     /* 4 byte ptr to array  C5B */

