#ident	"@(#)kern-pdi:io/hba/efp2/efp2.h	1.3"
/*	Copyright (C) Ing. C. Olivetti & C. S.p.a., 1991, 1992.*/
/*	All Rights Reserved	*/

#define HBA_PREFIX	efp2	/* Driver Function prefix for hba.h */

#ifdef  _KERNEL_HEADERS

#include <io/target/scsi.h>             /* REQUIRED */
#include <util/types.h>                 /* REQUIRED */
#include <io/target/sdi/sdi_hier.h>     /* REQUIRED */
#include <io/hba/hba.h>     /* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>                   /* REQUIRED */
#include <sys/types.h>                  /* REQUIRED */
#include <sys/sdi_hier.h>               /* REQUIRED */
#include <sys/hba.h>     /* REQUIRED */

#endif  /* _KERNEL_HEADERS */

#include <sys/ksynch.h>                  /* REQUIRED */
/*
** I get the definition inside ksynch.h 
*/
#define EFP2_HIER	HBA_HIER_BASE

#define NO_INT_PNDG     1
#define INT_NO_JOB      2
#define INT_JOB_CMPLT   3

#define PASS            1
#define FAIL            0
#define SUCCESS         0

#define EFP2_FALSE           0
#define EFP2_TRUE            (~EFP2_FALSE)


#define SPL_0            0
#define SPL_1            1
#define SPL_2            2
#define SPL_3            3
#define SPL_4            4
#define SPL_5            5
#define SPL_PP           5
#define SPL_6            6
#define SPL_NI           6
#define SPL_VM           6              /* 4.0 specific */
#define SPL_IMP          7              /* 4.0 specific */
#define SPL_TTY          7
#define SPL_STR         SPL_6
#define SPL_PIR         SPL_4

#define ONE_TICK        1                       /* Usually .01 seconds HZ=100 *
/
#define ONE_SECOND      HZ                      /* HZ = 100 on AT&T systems   *
/
#define ONE_MINUTE      (60 * ONE_SECOND)       /* Timeout delay time - 60sec *
/
#define ONE_HOUR        (60 * ONE_MINUTE)
#define ONE_DAY         (24 * ONE_HOUR)
#define ONE_WEEK        (7 * ONE_DAY)           /* Used as the max timeout    *
/

/* Time out values in milliseconds (SDI standard) */
#define SDI_ONE_SECOND  1000
#define SDI_TEN_SECONDS (10 * SDI_ONE_SECOND)
#define SDI_ONE_MINUTE  (60 * SDI_ONE_SECOND)
#define SDI_ONE_HOUR    (60 * SDI_ONE_MINUTE)
#define SDI_ONE_DAY     (24 * SDI_ONE_HOUR)
#define SDI_ONE_WEEK    (7 * SDI_ONE_DAY)       /* Used as the max timeout    *
/

/* used for timeout when polling for interrupts */
#define ONE_THOU_U_SEC          1000            /* 1 millisecond */

#define ONE_MILL_U_SEC          1000000         /* 1 second */
#define THREE_MILL_U_SEC        3000000         /* 3 seconds */
#define FIVE_MILL_U_SEC         5000000         /* 5 seconds */
#define TEN_MILL_U_SEC          10000000        /* 10 seconds */




/*
 * ==============================================================
 *	EFP2 Header file
 * ==============================================================
 */

#pragma	pack(2)

/*
 *                      COMMAND QUEUE STRUCTURE
 *
 *                    +-------------------------+
 *                    | rfu  | PUT | rfu | GET  | +0  <--- byte offset
 *                    +-------------------------+
 *                    !                         ! +4
 *                    !        ENTRY #0         !
 *                    !                         !
 *                    +-------------------------+
 *                    !                         ! +36
 *                    !        ENTRY #1         !
 *                    !                         !
 *                    +-------------------------+
 *                    !                         !
 *                    !           .             !
 *                    !           .             !
 *                    !           .             !
 *                    !                         !
 *                    +-------------------------+
 *                    !                         ! +(4 + 32*n)
 *                    !        ENTRY #n         !
 *                    !                         !
 *                    +-------------------------+
 *
 */

typedef
	struct {
		unchar		cq_get;
		unchar		cq_rfu1;
		unchar		cq_put;
		unchar		cq_rfu2;
	} command_queue_header_t;

typedef
	struct {
		long		cqe_1, cqe_2, cqe_3, cqe_4,
				cqe_5, cqe_6, cqe_7, cqe_8;
	} command_queue_entry_t;

typedef struct {
                lock_t          *sh_lock;
		ushort		sh_iobase;
		unchar		sh_queue;
		unchar		sh_flag;
		short		sh_maxq;
		short		sh_size;
} command_system_header_t;

#define	QUEUE_FULL		0x01	/* Queue is full */
#define	QUEUE_DUAL_COMMAND	0x10	/* Queue support dual commands */
#define	QUEUE_DOUBLE_ENTRY	0x20	/* Queue support double entry commands*/

/*
 *                      COMMAND QUEUE HEADER STRUCTURE
 *
 *                    +-------------------------+
 *                    |                         | +0  <--- byte offset
 *                    +-                       -+
 *                    |                         |
 *                    +-                       -+
 *                    |                         |
 *                    +-                       -+
 *                    |                         |
 *                    +- sh_stb : lock stat    -+
 *                    |                         |
 *                    +-                       -+
 *                    |                         |
 *                    +-                       -+
 *                    |                         |
 *                    +-                       -+
 *                    |                         |
 *                    +-------------------------+
 *                    | sh_lock : cq lock       | +32
 *                    +-------------------------+
 *                    | flag |  q  |   iobase   | +36
 *                    +-------------------------+
 *                    | sh_size    | sh_maxq    | +40
 *                    +-------------------------+
 *                    | rfu  | PUT | rfu | GET  | +44
 *                    +-------------------------+
 *
 */


typedef
	struct {
		command_system_header_t	cq_sys_header;
		command_queue_header_t	cq_header;
		command_queue_entry_t	cq_entry[1];
	} command_queue_t;


/*        QUEUES DESCRIPTOR
 *
 *        	QUEUES DESCRIPTOR HEADER:
 *
 *                    +-------------------------------+
 *                    ! N_CMD_QUEUE   ! MAINTENANCE   !  +0  byte offset
 *                    +-------------------------------+
 *                    !  RESERVED     ! TYPE_REPLY    !  +4
 *                    +-------------------------------+
 *                    !        REP_QUEUE_ADDR         !  +8
 *                    +-------------------------------+
 *                    ! RESERVED      ! N_ENTRY_REPLY !  +12
 *                    +-------------------------------+
 */

typedef
	struct {
		short	qh_maintenance;
		short	qh_n_cmd;
		short	qh_type_reply;
		short	qh_rfh1;
		paddr_t	qh_rep_queue;
		short	qh_n_reply;
		short	qh_rfh2;
	} queue_header_t;

#define	YES	1
#define	NO	0

/*
 *        qh_maintenance : (word) if (0001 hex) the controller  is  set
 *                         to  the  MAINTENANCE  environment,  if (0000
 *                         hex) it  is  set  to  the  USER  environment
 *                         (NORMAL or MIRRORING).
 */

#define	MAINTENANCE_MODE	YES
#define	USER_MODE		NO

/*
 *        qh_type_reply : (word) if this field is set to 0001 hex  the
 *                        controller  has  to  interrupt the system at
 *                        each reply, if this field is set to 0000 hex
 *                        the  controller  can  avoid to interrupt the
 *                        system at each reply under  the  constraints
 *                        of an algorithm TBD.
 */

#define	INTR_EACH_REPLY		YES
#define	INTR_NOT_EACH_REPLY	NO

/*
 *        QUEUES DESCRIPTOR BODY RECORD:
 *
 *         31             23            15           7           0 
 *        +-------------------------------------------------------+
 *        !     LUN      !     ID      !  CHANNEL   ! SCSI_LEVEL  ! +0
 *        +-------------------------------------------------------+
 *        !    RESERVED  !    NO_ARS   ! NOTFULL_INT! N_ENTRY_CMD ! +4
 *        +-------------------------------------------------------+
 *        !                      CMD_QUEUE_ADDR                   ! +8
 *        +-------------------------------------------------------+
 *        !                         RESERVED                      ! +12
 *        +-------------------------------------------------------+
 */

typedef
	struct {
		unchar	qe_scsi_level;
		unchar	qe_channel;
		unchar	qe_id;
		unchar	qe_lun;
		unchar	qe_n_cmd;
		unchar	qe_notfull_intr;
		unchar	qe_no_ars;
		unchar	qe_rfu1;
		paddr_t	qe_cmd_queue;
		long	qe_rfu2;
	} queue_entry_t;

typedef
	struct {
		queue_header_t	qd_header;
		queue_entry_t	qd_entry[1];
	} queue_descr_t;

/*
 *        qd_scsi_level : (byte) SCSI protocol level:
 *
 *                        01 hex : the subsystem (=  SW  driver  +  FW
 *                            controller  +  device)  supports  SCSI-1
 *                            protocol.
 *
 *                        02 hex : the subsystem (=  SW  driver  +  FW
 *                            controller  +  device)  supports  SCSI-2
 *                            protocol.
 *                        This record  field  describing  the  mailbox
 *                        command queue must be filled by 0xFF because
 *                        it is not applicable.
 */

#define	SCSI_1_PROT	   1
#define	SCSI_2_PROT	   2
#define	SCSI_NO_PROT	0xFF

#define	MBOX_PROT	SCSI_NO_PROT

/*
 *        qd_channel :    (byte) SCSI channel (01  hex  =  first  SCSI
 *                        channel, 02 hex = second SCSI channel) which
 *                        the device  is  connected  to.  This  record
 *                        field  describing  the mailbox command queue
 *                        must be filled by 0xFF  because  it  is  not
 *                        applicable.
 */

#define	SCSI_CHAN_1		1
#define	SCSI_CHAN_2		2
#define	SCSI_NO_CHAN		0xFF

#define	MAX_CHANS		2
#define	MAX_DEVS_PER_CHAN	MAX_TCS * MAX_LUS
#define	MAX_DEVS		MAX_DEVS_PER_CHAN * MAX_CHANS

#define	MBOX_CHAN		SCSI_NO_CHAN

#define	HA_CTRLS		4	/* Maximum number of SCSI channels */
#define	SCSI_NO_CTRL		0xFF

/*
 *        qd_id :         (byte) SCSI ID of the device.   This  record
 *                        field  describing  the mailbox command queue
 *                        must be filled by 0xFF  because  it  is  not
 *                        applicable.
 */

#define	SCSI_NO_ID	0xFF

#define	MBOX_ID		SCSI_NO_ID

/*
 *        qd_lun :        (byte) SCSI LUN of the device.  This  record
 *                        field  describing  the mailbox command queue
 *                        must be filled by 0xFF  because  it  is  not
 *                        applicable.
 */

#define	SCSI_NO_LUN	0xFF

#define	MBOX_LUN	SCSI_NO_LUN

/*
 *        qd_notfull_int: (byte) if this byte is set  to  0x01  (0x00)
 *                        the   controller  must  (must  not)  send  a
 *                        full/not_full command queue  interrupt  when
 *                        there  is  a  full to not_full transition in
 *                        the queue.
 *
 *			  use YES to enable, NO to disable
 */

/*
 *        NO_ARS       :  (byte) if this byte is set to 0x01 then  the
 *                        Automatic  Request  Sense (ARS) is disabled;
 *                        if there is a CHECK CONDITION SCSI status on
 *                        this device the FW controller will abort all
 *                        the commands up to when  the  Request  Sense
 *                        command will have been sent.
 *                        Notice that the SW driver  can  disable  the
 *                        ARS  on the sequential devices but it cannot
 *                        disable  the  ARS  on  the   direct   access
 *                        devices.
 *                        If it is  set  to  0x00  (necessary  in  the
 *                        MIRRORING  environment)  then  the Automatic
 *                        Request Sense is enabled.
 *
 *			  use YES to disable, NO to enable ARS
 */

/*
 *        Both  the  SW  driver  and  the  FW  controller  use   these
 *        conventions:
 *        the body entry #0 refers as the mailbox queue (queue  number
 *        = 0);
 *        the body entry #1 refers as the  first  device/queue  (queue
 *        number = 1);
 *        the body entry #2 refers as the second  device/queue  (queue
 *        number = 2);
 *        etc.
 */

#define	MBOX_QUEUE	0
#define	CMD_QUEUE	1	/* use device + CMD_QUEUE */

/*
 *        COMMON HEADER
 *
 *                +-------------------------------+
 *                !           USER_ID             !  +0   byte offset
 *                +-------------------------------+
 *                !CMD_TYPE!  MOD ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 */

typedef
	struct {
		long	ch_userid;
		unchar	ch_sort;
		unchar	ch_prior;
		unchar	ch_mod;
		unchar	ch_cmd_type;
	} command_header_t;

/*
 *        ch_mod :     (byte) into the normal/maintenance  environment
 *                     the  value  of this field must be 00 hex whilst
 *                     into the mirroring environment may be:
 *
 *                     00 hex  : command directed to the paired disks;
 *
 *                     01 hex  : command directed to the source disk;
 *
 *                     02  hex  : command  directed  to  the  mirrored
 *                           disk;
 */

#define	NORMAL_MOD	0

#define	TO_PAIRED	0
#define	TO_SOURCE	1
#define	TO_MIRRORED	2

/*
 *        ch_prior :   (byte) Command priority. The range  is  between
 *                     00  hex  (highest  priority) and FF hex (lowest
 *                     priority).
 */

#define	HIGHEST_PRIOR	0x00
#define	LOWEST_PRIOR	0xFF

/*
 *        ch_sort :    (byte) This field shows whether the command can
 *                     be sorted or not:
 *
 *                     00 hex  :    no sortable command (in this  case
 *                                  the  controller cannot change this
 *                                  command execution order);
 *
 *                     01 hex  :    sortable command (in this case the
 *                                  controller can change this command
 *                                  execution order);
 */
 
#define	NOT_SORTABLE	NO
#define	SORTABLE	YES

/*
 *        "NORMAL"  COMMAND:
 *
 *                +-------------------------------+
 *                !           USER_ID             !  +0   byte offset
 *                +-------------------------------+
 *                !CMD_TYPE!  MOD ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 *                !        RESERVED       ! CDB_L !  +8
 *                +-------------------------------+
 *                !           LENGTH              !  +12
 *                +-------------------------------+
 *                !                               !  +16
 *                !                               !
 *                !          CDB_SCSI             !
 *                !                               !
 *                !                               !
 *                +-------------------------------+
 *                !           ADDRESS             !  +28
 *                +-------------------------------+
 */

#define	CDB_LEN	12

typedef
	struct {
		command_header_t	nc_header;
		unchar			nc_cdb_l;
		unchar			nc_rfu1[3];
		long			nc_length;
		char			nc_cdb_scsi[CDB_LEN];
		paddr_t			nc_address;
	} normal_command_t;

/*
 *        ch_cmd_type : (byte) command code:
 *
 *                     10 hex :     general data transfer from a  SCSI
 *                                  device  to  the  system memory.  A
 *                                  set of these commands are: "read",
 *                                  "mode   sense",  "read  capacity",
 *                                  etc.
 *
 *                     11 hex :     general  data  transfer  from  the
 *                                  system memory to a SCSI device.  A
 *                                  set   of   these   commands   are:
 *                                  "write", "mode select", etc.
 *
 *                     12 hex :     no data transfer.  A set of  these
 *                                  commands  are:  "rezero",  "seek",
 *                                  etc.
 */

#define	SCSI_TO_SYSTEM	  0x10
#define	SYSTEM_TO_SCSI	  0x11
#define	NO_DATA_TRANSFER  0x12

/*
 *        nc_cdb_l :   (byte) Length of the CDB_SCSI (see below).  For
 *                     example,  it  is  set to 0x06 for group #0 SCSI
 *                     commands, 0x0A for group #1, 0x0C for group #2.
 */

#define	GROUP_0_LEN	 6
#define	GROUP_1_LEN	10
#define	GROUP_2_LEN	12

/*
 *        SHORT SCATTER-GATHER COMMAND:
 *
 *                +-------------------------------+
 *                !           USER_ID             !  +0   byte offset
 *                +-------------------------------+
 *                !CMD_TYPE!  MOD ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 *                !     LOGICAL_BLOCK_ADDRESS     !  +8
 *                +-------------------------------+
 *                !      L1       !SIZE_BLK! RFU  !  +12
 *                +-------------------------------+
 *                !      L3       !      L2       !  +16
 *                +-------------------------------+
 *                !              A1               !  +20
 *                +-------------------------------+
 *                !              A2               !  +24
 *                +-------------------------------+
 *                !              A3               !  +28
 *                +-------------------------------+
 */

typedef
	struct {
		command_header_t	sg_header;
		long			sg_log_blk;
		unchar			sg_size_blk;
		unchar			sg_rfu1;
		unsigned short		sg_l1;
		unsigned short		sg_l2;
		unsigned short		sg_l3;
		paddr_t			sg_a1;
		paddr_t			sg_a2;
		paddr_t			sg_a3;
	} sg_command_t;

/*
 *        ch_cmd_type : (byte) command code:
 *
 *                     20 hex  :           short read  scatter  gather
 *                                         command;
 *
 *                     21 hex  :           short write scatter  gather
 *                                         command;
 */

#define SHORT_SG_READ	0x20
#define SHORT_SG_WRITE	0x21

/*
 *        sg_size_blk : (byte) Logical block size (see  SCSI  standard)
 *                     of  the disk in 256 bytes length, such as 
 *		       0x01->256 bytes, 0x02->512 bytes, etc.
 */

#define SCSI_BLK_SIZ	256
#define STD_BLK_SIZ	2	/* 512 bytes */

/*        sg_l1,l2,l3 : (words)  lengths  of  the  buffers  that  are
 *                     interested  by  the scatter gather command; the
 *                     sum L1 + L2 + L3 must be <= 64Kbytes.  If  some
 *                     lengths  are  unused  (unsignificant),  fill by
 *                     zeros (00 hex).
 */

#define SHORT_SG_MAX_LEN	65536	/* 64 Kb */
#define NO_LEN			0

/*
 *        sg_a1,a2,a3 : (longs) Physical ddresses of the buffers  which
 *                     are used in the scatter gather command.  If any
 *                     addresses are  not  significant  they  must  be
 *                     filled by FF hex.
 */

#define NO_ADDR			0xFFFFFFFF

/*
 *        LONG SCATTER-GATHER COMMAND:
 *
 *                +-------------------------------+
 *                !           USER_ID             !  +0    byte offset
 *                +-------------------------------+
 *                !CMD_TYPE!  MOD ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 *                !     LOGICAL_BLOCK_ADDRESS     !  +8
 *                +-------------------------------+
 *                !      L1       !SIZE_BLK! RFU  !  +12
 *                +-------------------------------+        MAIN_ENTRY
 *                !      L3       !      L2       !  +16
 *                +-------------------------------+
 *                !              A1               !  +20
 *                +-------------------------------+
 *                !              A2               !  +24
 *                +-------------------------------+
 *                !              A3               !  +28
 *                +-------------------------------+
 *                                        ^
 *                                        |
 *                                        |
 *                +-------------------------------+
 *                !      L4       !     LINK      !  +32    byte offset
 *                +-------------------------------+
 *                !      L6       !      L5       !  +36
 *                +-------------------------------+
 *                !      L8       !      L7       !  +40
 *                +-------------------------------+
 *                !              A4               !  +44
 *                +-------------------------------+        LINKED_ENTRY
 *                !              A5               !  +48
 *                +-------------------------------+
 *                !              A6               !  +52
 *                +-------------------------------+
 *                !              A7               !  +56
 *                +-------------------------------+
 *                !              A8               !  +60
 *                +-------------------------------+
 */

typedef
	struct {
		short			le_link;
		unsigned short		le_l4;
		unsigned short		le_l5;
		unsigned short		le_l6;
		unsigned short		le_l7;
		unsigned short		le_l8;
		paddr_t			le_a4;
		paddr_t			le_a5;
		paddr_t			le_a6;
		paddr_t			le_a7;
		paddr_t			le_a8;
	} linked_entry_t;

/*
 *        The LONG READ SCATTER GATHER  command  has  CMD_TYPE=30 hex.
 *        The LONG WRITE SCATTER GATHER command has CMD_TYPE=31 hex.
 */

#define LONG_SG_READ	0x30
#define LONG_SG_WRITE	0x31

/*
 *        le_link :    (word) Must be FFFF hex.  It  shows  a  logical
 *                     link to the previous command entry.
 */

#define CMD_LINK	0xFFFF

/*
 *        EXTENDED SCATTER-GATHER COMMAND:
 *
 *                +-------------------------------+
 *                !           USER_ID             !  +0
 *                +-------------------------------+
 *                !CMD_TYPE!  MOD ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 *                !       RESERVED        ! CDB_L !  +8
 *                +-------------------------------+
 *                !    RESERVED   !      LB       !  +12
 *                +-------------------------------+
 *                !                               !  +16
 *                !                               !
 *                !            CDB_SCSI           !
 *                !                               !
 *                !                               !
 *                +-------------------------------+
 *                !              AB               !  +28
 *                +-------------------------------+
 *
 */

typedef
	struct {
		command_header_t	esg_header;
		unchar			esg_cdb_l;
		unchar			esg_rfu1[3];
		unsigned short		esg_lb;
		short			esg_rfu2;
		char			esg_cdb_scsi[CDB_LEN];
		paddr_t			esg_ab;
	} extended_sg_t;

/*
 *        ch_cmd_type : (byte) command code:
 *
 *                     40  hex  :          extended   read   scatter
 *                                         gather;
 *
 *                     41  hex  :          extended   write   scatter
 *                                         gather;
 */

#define EXT_SG_READ	0x40
#define EXT_SG_WRITE	0x41

/*
 *        EXTENDED SCATTER GATHER TABLE:
 *
 *                +-------------------------------+
 *                !              ADD1             !   +0   byte offset
 *                +-------------------------------+
 *                !              LEN1             !   +4
 *                +-------------------------------+
 *                !              ...              !
 *                !              ...              !
 *                !              ...              !
 *                +-------------------------------+
 *                !              ADDn             !   +((8 * n) - 8)
 *                +-------------------------------+
 *                !              LENn             !   +((8 * n) - 4)
 *                +-------------------------------+
 *
 *        each pair of longs specifies the physical  address  and  the
 *        length of the buffer where the data are trasferred to/from.
 *
 *        The sum LEN1 + LEN2 + ... + LENn must be less than 32Mbytes.
 */

typedef
	struct {
		paddr_t	esge_addr;
		long	esge_len;
	} ext_sg_entry_t;

typedef
	ext_sg_entry_t	ext_sg_table[1];

/*
 *        DUAL SCATTER GATHER READ:
 *
 *                +-------------------------------+
 *                !           USER_ID             !  +0   byte offset
 *                +-------------------------------+
 *                !CMD_TYPE!  MOD ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 *                !     LOGICAL_BLOCK_ADDRESS     !  +8
 *                +-------------------------------+
 *                !      L1       !SIZE_BLK! RFU  !  +12
 *                +-------------------------------+        MAIN_ENTRY
 *                !      L3       !      L2       !  +16
 *                +-------------------------------+
 *                !              A1               !  +20
 *                +-------------------------------+
 *                !              A2               !  +24
 *                +-------------------------------+
 *                !              A3               !  +28
 *                +-------------------------------+
 *                                        ^
 *                                        |
 *                                        |
 *                +-------------------------------+
 *                !      L4       !     LINK      !  +32   byte offset
 *                +-------------------------------+
 *                !      L6       !      L5       !  +36
 *                +-------------------------------+
 *                !      L8       !      L7       !  +40
 *                +-------------------------------+
 *                !              A4               !  +44
 *                +-------------------------------+        LINKED_ENTRY
 *                !              A5               !  +48
 *                +-------------------------------+
 *                !              A6               !  +52
 *                +-------------------------------+
 *                !              A7               !  +56
 *                +-------------------------------+
 *                !              A8               !  +60
 *                +-------------------------------+
 *
 *        The   SHORT   DUAL   READ   SCATTER   GATHER   command   has
 *        CMD_TYPE=50 hex.
 */

#define	SHORT_DUAL_SG_READ	0x50

/*
 *        DUAL EXTENDED SCATTER GATHER READ :
 *
 *                +-------------------------------+
 *                !           USER_ID             !  +0    byte offset
 *                +-------------------------------+
 *                !CMD_TYPE!  MOD ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 *                !      RESERVED         ! CDB_L !  +8
 *                +-------------------------------+
 *                !       LS      !      LB       !  +12
 *                +-------------------------------+
 *                !                               !  +16
 *                !                               !
 *                !           CDB_SCSI            !
 *                !                               !
 *                !                               !
 *                +-------------------------------+
 *                !              AB               !  +28
 *                +-------------------------------+
 */

typedef
	struct {
		command_header_t	desg_header;
		unchar			desg_cdb_l;
		unchar			desg_rfu1[3];
		unsigned short		desg_lb;
		unsigned short		desg_ls;
		char			desg_cdb_scsi[CDB_LEN];
		paddr_t			desg_ab;
	} dual_ext_sg_t;

/*
 *        ch_cmd_type  : (byte) command code: its value is 60 hex.
 */

#define	EXT_DUAL_SG_READ	0x60

/*
 *        REPLY QUEUE :
 *
 *         NORMAL/MAINTENANCE MODE                 MIRRORING MODE
 *
 *   +---------------------------------+ +---------------------------------+
 *   !             USER_ID             ! !             USER_ID             !+0
 *   +---------------------------------+ +---------------------------------+
 *   !             SCSI_LEN            ! !             SCSI_LEN            !+4
 *   +---------------------------------+ +---------------------------------+
 *   !                                 ! ! QUALIF1| ADDIT1 ! SENSE1 !VALID1!+8
 *   !                                 ! +---------------------------------+
 *   !                                 ! !        LOGICAL_BLOCK_NUM1       !+12
 *   !          SCSI_SENSE_DATA        ! +---------------------------------+
 *   !                                 ! ! QUALIF2| ADDIT2 ! SENSE2 !VALID2!+16
 *   !                                 ! +---------------------------------+
 *   !                                 ! !        LOGICAL_BLOCK_NUM2       !+20
 *   +----------------+                + +---------------------------------+
 *   !    RESERVED    !                ! ! D_OFF  | OFF_ATTR | RESERVED    !+24
 *   +---------------------------------+ +---------------------------------+
 *   ! FLAG ! CMD_QUE ! EX_STAT !STATUS! !FLAG ! CMD_QUE ! EX_STAT !STATUS !+28
 *   +---------------------------------+ +---------------------------------+
 */

#define	SCSI_SENSE_LEN	18		/* scsi.h: SENSE_SZ */

typedef
	struct {
		long	re_user_id;
		long	re_scsi_len;
		union	re_u {
			struct {
				char	nre_scsi_sense[SCSI_SENSE_LEN];
				short	nre_rfu;
			}	nre_u;

			struct {
				struct {
					unchar	mre_valid;
					unchar	mre_sense;
					unchar	mre_addit;
					unchar	mre_qualif;
					long	mre_log_blk;
				}	mre_data[2];	
                                short   mre_rfu;
                                unchar  mre_off_attr;
                                unchar  mre_d_off;
			} 	mre_u;
#define	MRE	re_u.mre_u.mre_data
#define MRE_ATTR re_u.mre_u.mre_off_attr
		}	re_u;
		unchar	re_status;
		unchar	re_ex_stat;
		unchar	re_cmd_que;
		unchar	re_flag;	/* Flag MORMAL/MIRRORED */
	} reply_queue_entry_t;

typedef
	struct {
		short	sh_full;
		short	sh_size;
		long	sh_get;
		long	sh_maxr;
		long	sh_iobase;
	} reply_system_header_t;

typedef
	struct {
		reply_system_header_t	rq_sys_header;
		reply_queue_entry_t	rq_entry[1];
	} reply_queue_t;

/*
 *        mrq_valid : (byte) this field has the values  which
 *                     are described below:
 *
 *                     00 hex:   SENSE,ADDIT,QUALIF,LOGICAL_BLOCK_NUM
 *                               are not significant;
 *
 *                     70 hex:   SENSE,ADDIT,QUALIF are significant;
 *
 *                     F0 hex:   SENSE,ADDIT,QUALIF,LOGICAL_BLOCK_NUM 
 *				 are significant.
 *
 *                     3x hex:   particular errors (see  SCSI  specs),
 *                               that is:
 *
 *                               31 hex CONDITION  MET/GOOD: see  SCSI
 *                                    protocol;
 *
 *                               32 hex BUSY: the target is BUSY.  The
 *                                    controller  returns  this  state
 *                                    when it has  received  256  BUSY
 *                                    status from the target.
 *
 *                               34  hex  INTERMEDIATE/GOOD: see  SCSI
 *                                    protocol;
 *
 *                               35     hex     INTERMEDIATE/CONDITION
 *                                    MET/GOOD: see SCSI protocol;
 *
 *                               36 hex RESERVATION CONFLICT: see SCSI
 *                                    protocol;  the  normal  recovery
 *                                    action is to issue  the  command
 *                                    again at a later time;
 *
 *                               otherwise: TBD.
 *                               In  all  these  cases  SENSE,  ADDIT,
 *                               QUALIF,   LOGICAL_BLOCK_NUM  are  not
 *                               significant.
 */

#define SAQL_FROM_RECOVERY	0x00
#define	SAQ_FROM_DEV		0x70
#define	SAQL_FROM_DEV		0xF0
#define	COND_MET_GOOD		0x31
#define	TARGET_BUSY		0x32
#define	INTERMED_GOOD		0x34
#define	INTERMED_COND_MET_GOOD	0x35
#define	RESERV_CONFLICT		0x36

/*
 *      mre_d_off : (byte) off-line device
 *
 *                      This fiels is set only when EX_STAT is set by
 *                      0x01 and indicates the off-line physical disk
 *                      that is :
 *
 *
 *                        7    6    5    4    3    2    1    0
 *                      +----+----+----+----+----+----+----+----+
 *                      |     LUN      |      ID      | CHANNEL |
 *                      +----+----+----+----+----+----+----+----+
 *
 *                      where :
 *
 *                      CHANNEL = 01 means "first physical SCSI channel"
 *                                10 means "second physical SCSI channel"
 *                                11 means "third physical SCSI channel"
 *                                00 means "fourth physical SCSI channel"
 *
 *                      ID/LUN = physical SCSI ID/LUN of the disk.
*/
#define OFF_LINE_CHANNEL ( (x) & 0x3 )
#define OFF_LINE_ID      ( ((x) >> 2) & 0x7 )
#define OFF_LINE_LUN     ( ((x) >> 5) & 0x7 )


/*
 *      mre_off_attr : (byte) off-line device attribute
 *
 *                      When the mirroring enviroment is unbroken this
 *                      this field is set by 0x00
 *                      otherwise it shows the device which was put
 *                      "off-line", precisely :
 *
 *                      0x00:   mirroring unbroken
 *                      0x01:   source disk out of mirror
 *                      0x02:   mirrored disk out of mirror
*/
#define SOURCE_OUT      0x01
#define MIRROR_OUT      0x02

/*
 *        rq_status : (byte) command global result. Its value can be:
 *
 *                     [00]      Successful command end.
 *
 *                     [01]      Warning or Fatal Error: it was  found
 *                               at   least  one  error  on  the  disk
 *                               (NORMAL/MAINTENANCE  environment)  or
 *                               on the disks (MIRRORING environment).
 *                               See  the  SCSI_SENSE_DATA  (when   in
 *                               NORMAL/MAINTENANCE   environment)  or
 *                               VALID1-2,     SENSE1-2,     ADDIT1-2,
 *                               QUALIF1-2    (when    in    MIRRORING
 *                               environment).     Note:    if     the
 *                               controller  is  set  to the MIRRORING
 *                               environment and exits  from  it  (see
 *                               EXTENDED  STATUS  field)  refering to
 *                               that disks pair, this field  reflects
 *                               the  command  result of the "on-line"
 *                               disk. If it is set to "zero" it means
 *                               that  no  error  has  occured  on the
 *                               "on-line" disk. If it is set to "one"
 *                               it means that an error has occured on
 *                               the "on-line"disk.
 *
 *                     1x hex:   Generic error  during  the  SCSI  bus
 *                               handling, precisely:
 *
 *                               10 hex: error was recovered using  an
 *                                    autonomous recovery procedure of
 *                                    the controller;
 *
 *                               11 hex: selection timeout expired;
 *
 *                               12 hex: data overrun/underrun;
 *
 *                               13  hex: unexpected  BUS  FREE  phase
 *                                    detected;
 *
 *                               14 hex: SCSI phase sequence failure;
 *
 *                               18 hex: an error  was  not  recovered
 *                                    using   an  autonomous  recovery
 *                                    procedure of the controller;
 *
 *
 *                     FF hex    Generic error  during  data  transfer
 *                               to/from  the system memory or unknown
 *                               command.
 *
 */

#define	CORRECT_END	0
#define	DISK_ERROR	0x01
#define	RECOVER_OK	0x10
#define	SEL_TIMEOUT	0x11
#define OVER_UNDER_RUN  0x12	/* overrun error */
#define	BUS_FREE	0x13
#define	PHASE_SEQ	0x14
#define	RECOVER_NOK	0x18
#define	BUS_ERROR	0xE0	/* .. 0xEF */
#define	TRANSFER_ERROR	-1

/*
 *        rq_ex_stat:  (byte) into the NORMAL/MAINTENANCE  environment
 *                     this field is set as follows:
 *
 *                     00 hex NOTHING TO SIGNAL
 *
 *                     30 hex CHECK CONDITION    : there was  a  CHECK
 *                          CONDITION SCSI status on a device on which
 *                          the Automatic  Request  Sense  command  is
 *                          disabled   or,   during   the   autonomous
 *                          recovery procedure, the controller,  after
 *                          having  received a CHECK_CONDITION status,
 *                          has sent a REQUEST SENSE command.  If  the
 *                          Automatic   Request   Sense   command   is
 *                          disabled then the FW controller will abort
 *                          (that   is  EX_STAT  =  3B  hex)  all  the
 *                          commands  up  to  when  a  Request   Sense
 *                          command will have been sent.
 *
 *                     31 hex CONDITION MET/GOOD : see SCSI protocol;
 *
 *                     32 hex BUSY: the target is BUSY. The controller
 *                          returns  this  state  when it has received
 *                          256 BUSY status from the target.
 *
 *                     34 hex INTERMEDIATE/GOOD: see SCSI protocol;
 *
 *                     35  hex  INTERMEDIATE/CONDITION   MET/GOOD: see
 *                          SCSI protocol;
 *
 *                     36 hex RESERVATION CONFLICT: see SCSI protocol;
 *                          the normal recovery action is to issue the
 *                          command again at a later time;
 *
 *                     3B hex ABORT COMMAND: the  command  is  aborted
 *                          because  the  FW  controller found a CHECK
 *                          CONDITION SCSI status and up to now the SW
 *                          driver   has  not  sent  a  Request  Sense
 *                          command.
 *                     Into the MIRRORING environment  this  field  is
 *                     zero  if  the  environment  is  still MIRRORING
 *                     otherwise this field is set to  one  (01  hex).
 *                     The  controller  can  exit  from  the MIRRORING
 *                     environment after an unrecovered error during a
 *                     write   command   or  after  recognizing  a  no
 *                     consistent disks pair  (the  controller  checks
 *                     the   consistence  by  reading  autonomously  a
 *                     reserved disk block).  If the controller  exits
 *                     from  the  MIRRORING  environment  on a pair of
 *                     disks  it  will  report  continuously  hardware
 *                     error on the disk which was put "off-line".
 */

#define	STILL_MIRRORING		0x00
#define	NOT_YET_MIRRORING	0x01

#define	NOTHING_TO_SIGNAL	0x00
#define	CHECK_CONDITION		0x30
#define	COND_MET_GOOD		0x31
#define	TARGET_BUSY		0x32
#define	INTERMED_GOOD		0x34
#define	INTERM_COND_MET_GOOD	0x35
#define	RESERV_CONFLICT		0x36
#define	ABORT_COMMAND		0x3B

/*
 *        rq_flag :    (byte) in NORMAL/MAINTENANCE MODE if it is  set
 *                     to  01  hex the response is valid, if it is set
 *                     to 00 hex the response is invalid; in MIRRORING
 *                     mode  if  it  is  set to 03 hex the response is
 *                     valid, if it is set to 00 hex the  response  is
 *                     invalid;
 */

#define	VALID_RESPONSE	0x01
#define	MIRROR_RESPONSE	0x02

/*
 *        MAILBOX COMMAND ENTRY STRUCTURE
 *
 *                          COMMAND ENTRY
 *
 *                +-------------------------------+
 *                !             USER_ID           !  +0   byte offset
 *                +-------------------------------+
 *                !CMD_TYPE! RFU  ! PRIOR ! SORT  !  +4
 *                +-------------------------------+
 *                !            LENGTH             !  +8
 *                +-------------------------------+
 *                !                               !  +12
 *                +--                           --+
 *                !                               !
 *                +--          USER_DATA        --+
 *                !                               !
 *                +--                           --+
 *                !                               !
 *                +-------------------------------+
 *                !             ADDRESS           !  +28
 *                +-------------------------------+
 */

typedef
	struct {
		command_header_t	mc_header;
		long			mc_length;
		long			mc_user_data[4];
		paddr_t			mc_address;
	} mbox_command_t;

/*
 *        MAILBOX REPLY ENTRY STRUCTURE
 *
 *             The mailbox reply structure is defined as below:
 *
 *                             REPLY ENTRY
 *
 *                +-------------------------------+
 *                !             USER_ID           !  +0   byte offset
 *                +-------------------------------+
 *                !             LENGTH            !  +4
 *                +-------------------------------+
 *                !             RESERVED          !  +8
 *                +-------------------------------+
 *                !                               !  +12
 *                +--                           --+
 *                !                               !
 *                +--         APPL_FIELD        --+
 *                !                               !
 *                +--                           --+
 *                !                               !
 *                +-------------------------------+
 *                ! FLAG  !CMD_QUE !    STATUS    !+28
 *                +-------------------------------+
 */

typedef
	struct {
		long	mr_user_id;
		long	mr_length;
		long	mr_rfu;
		long	mr_appl_field[4];
		short	mr_status;
		unchar	mr_cmd_que;
		unchar	mr_flag;
	} mbox_reply_t;

/*
 *        mr_flag : (word) if it is set to 01 hex  the  response  is
 *                  valid, otherwise the response is invalid.
 */

#define VALID_RESPONSE	0x01

/*
 *        SPECIFIC_MAILBOX_COMMAND
 */

/*
 *	get_information	command
 *
 *		No user data
 */

#define	MBX_GET_INFO	0x00

/*
 *		Response :
 *
 *                    31           23            15            7        0
 *                   +------------------------------------------------------+
 *                   | SCSI_LEVEL |             FIRMWARE_RELEASE            | #0
 *                   +------------------------------------------------------+
 *                   |  RESERVED  | MAX_CMD_ENT |  DUAL_LONG  | ENVIRONMENT | #1
 *                   +------------------------------------------------------+
 *                   |    ID3     |      ID2    |    ID1      |    ID0      | #1
 *                   +------------------------------------------------------+
 *                   |                      RESERVED                        | #3
 *                   +------------------------------------------------------+
 */

typedef
	struct {
		char	gi_fw_rel[3];
		unchar	gi_scsi_level;
		unchar	gi_environment;
		unchar	gi_dual_long;
		unchar	gi_max_cmd_ent;
		unchar	gi_rfu1;
		unchar	gi_id[HA_CTRLS];
		long	gi_reserved;
	} get_information_t;

#define MIRRSINGLE0 0x1
#define MIRRSINGLE1 0x2
#define MIRRDUAL 0x33

/*
 *                   ENVIRONMENT  : (byte)   defines   the   mirroring
 *                         environment, that is:
 *
 *                         single  bus   mirroring   environment  : if
 *                              bit(i)==(1)  and  bit  (i+4)==(0) than
 *                              there  is  a  single   bus   mirroring
 *                              environment on the SCSI channel #i;
 *                              for example: b00000010--->SCSI channel
 *                              #1    in    single    bus    mirroring
 *                              environment;
 *
 *
 *                         dual   bus   mirroring    environment  : if
 *                              bit(i,j)==(1,1)         and        bit
 *                              (i+4,j+4)==(1,1) than there is a  dual
 *                              bus  mirroring environment on the SCSI
 *                              channels #i,j;
 *                              for     example:     b00110011--->SCSI
 *                              channels  #0,1  in  dual bus mirroring
 *                              environment;
 */

/*
 *                   gi_dual_long : (byte) defines particular
 *                         constraints, that is:
 *
 *                         0x00  : the controller is  able  to  handle
 *                              dual  commands and commands defined by
 *                              two command entries, that is:
 *                              - long scatter gather
 *                              - dual  scatter   gather   read
 *                              - dual extended scatter  gather  read
 *
 *                         0x01  : the  controller  is  not  able   to
 *                              handle dual commands, that is:
 *                              - dual  scatter   gather   read
 *                              - dual extended scatter  gather  read
 *
 *                         0x02  : the  controller  is  not  able   to
 *                              handle  the  commands  defined  by two
 *                              command entries, that is:
 *                              - long scatter gather
 *                              - dual  scatter   gather   read
 *
 *                         0x03  : the  controller  is  not  able   to
 *                              handle   dual  commands  and  commands
 *                              defined by two command  entries,  that
 *                              is:
 *                              - long scatter gather
 *                              - dual  scatter   gather   read
 *                              - dual extended scatter  gather  read
 */

#define	ALL_EXTENDED		0x00
#define	NO_DUAL_COMMAND		0x01
#define	NO_DOUBLE_ENTRIES	0x02
#define NO_EXTENDED		0x03

#define	MASTER_ID		0x07
/*
 *	download command
 *
 *	No user data
 */

#define	MBX_DOWNLOAD 0x01

/*
 *	upload command
 *
 *	No user data
 */

#define	MBX_UPLOAD	0x02
 
/*
 *	force_execution command
 */

#define	MBX_FORCE_EXEC	0x03

/*
 *              31                                                  0
 *             +-----------------------------------------------------+
 *             ! local memory offset of the function entry point     ! #0
 *             +-----------------------------------------------------+
 *             !    physical   address of the input/output structure ! #1
 *             +-----------------------------------------------------+
 *             ! length of the input/output parameters structure     ! #2
 *             +-----------------------------------------------------+
 *             !                    RESERVED                         ! #3
 *             +-----------------------------------------------------+
 */

typedef
	struct {
		long	fe_entry_point;
		paddr_t	fe_io_parm;
		long	fe_io_parm_len;
		long	fe_rfu;
	} force_execution_t;

/*
 *	get_configuration command
 *
 *	No user data
 */

#define	MBX_GET_CONFIG	0x04

/*	
 *	CONFIGURATION TABLE AREA
 *
 *                   +-------------------------------------------+
 *                   !   ENV   ! SCSI_LEV ! DEV_QUAL! DEV_TYPE   ! +0
 *                   +-------------------------------------------+
 *                   !   RFU   !   LUN    !    ID   !  CHANNEL   ! +4
 *                   +-------------------------------------------+
 *                   !   ENV   ! SCSI_LEV ! DEV_QUAL! DEV_TYPE   ! +8
 *                   +-------------------------------------------+
 *                   !   RFU   !   LUN    !    ID   !  CHANNEL   ! +12
 *                   +-------------------------------------------+
 *                   !                                           !
 *                                  . . . . . . .
 *
 *                   !                                           !
 *                   +-------------------------------------------+
 *                   !   ENV   ! SCSI_LEV ! DEV_QUAL! DEV_TYPE   !
 *                   +-------------------------------------------+
 *                   !   RFU   !   LUN    !    ID   !  CHANNEL   !
 *                   +-------------------------------------------+
 */

typedef
	struct {
		unchar	ce_dev_type;
		unchar	ce_dev_qual;
		unchar	ce_scsi_lev;
		unchar	ce_env;
		unchar	ce_channel;
		unchar	ce_id;
		unchar	ce_lun;
		unchar	ce_rfu2;
	} configuration_entry_t;

typedef
	configuration_entry_t	configuration_table_t[MAX_DEVS];

/*
 *        ce_dev_type :  (byte)  device  type   (see   SCSI   standard
 *                       protocol):
 *
 *                          - 00  hex      =  Direct  Access   Devices
 *                            (such as magnetic disk);
 *
 *                          - 01 hex      = Sequential Access  devices
 *                            (such as magnetic tape);
 *
 *                          - 02 hex      = Printer Devices;
 *
 *                          - 03 hex      = Processor Devices;
 *
 *                          - 04 hex      = Write once, read  multiple
 *                            devices (such as some optical disks);
 *
 *                          - 05 hex      = Read  only  Direct  Access
 *                            devices (such as some optical disks);
 *
 *                          - 06 - 7E hex = Reserved;
 *
 *                          - 80 - FF hex = Vendor Unique.
 */

#define	DIRECT_ACCESS	0x00
#define	SEQ_ACCESS	0x01
#define	PRINTER_DEV	0x02
#define	PROCESSOR_DEV	0x03
#define	WORM_DEV	0x04
#define	RO_DIRECT	0x05

/*
 *        ce_env :       (byte) this field defines the environment  of
 *                       the disk, that is:
 *
 *                            7    6    5    4        3    2    1    0     
 *			    +----+----+----+----+   +----+----+----+----+
 *			    | 0  | 0  | DM | SM |   | 0  | 0  | MI | SO |
 *			    +----+----+----+----+   +----+----+----+----+
 *				  environment              parameters
 *
 *				    DM = dual mirroring
 *				    SM = single mirroring
 *				    MI = mirrored disk
 *				    SO = source disk
 *
 *			 The meaning of this fiels must be  considered
 *			 as follows:
 *	
 *			 NORMAL ENVIRONMENT :
 *			      DM = 0, SM = 0;
 *			      MI = 0, SO = 0;
 *	
 *			 SINGLE MIRRORING ENVIRONMENT :
 *			      DM = 0, SM = 1;
 *	
 *			      SO = 1 the source disk exists;
 *				   0 the source disk does not exist;
 *			      MI = 1 the mirrored disk exists;
 *				   0 the mirrored disk does not exist;
 *	
 *			 DUAL MIRRORING ENVIRONMENT :
 *			      DM = 1, SM = 0;
 *			      SO = 1 the source disk exists;
 *				   0 the source disk does not exist;
 *			      MI = 1 the mirrored disk exists;
 *				   0 the mirrored disk does not exist;
 */
	
#define	DUAL_MIRROR	0x20
#define	SINGLE_MIRROR	0x10
#define	MIRROR_DISK	0x02
#define	SOURCE_DISK	0x01

/*
 *        mr_status : (word) command global result. Its value can be:
 *
 *                      - [0000] hex successful commands end;
 *
 *                      - [0001] hex the length of the buffer  is  too
 *                        short to contain the whole configuration;
 *
 *                      - [FFFF] hex error on SYSTEM-BUS;
 *
 *                      - [EExx]  hex  error  during  the   SCSI   bus
 *                        handling.
 */

#define	BUFFER_TOO_SHORT	0x0001
#define	SCSI_BUS_ERROR		0xEE00

/*
 *        reset_scsi_bus command
 */

#define	MBX_RESET_SCSI_BUS	0x05

/*
 *        user data: (4 longs) the first long contains  the  SCSI  bus
 *                   channel  to  reset.  The other longs are reserved
 *                   (fill by zeros).
 */

typedef
	struct {
		long	rs_channel;
		long	rfu1;
		long	rfu2;
		long	rfu3;
	} reset_scsi_t;

/*
 *
 *        mr_status : (word) command global result. Its value can be:
 *
 *                      - [0000] hex successful command end.
 *
 *                      - [0001] hex It was impossible  to  reset  the
 *                        given SCSI bus.
 */

#define	CANNOT_RESET	0x0001

/*
 *	set_copy command
 */

#define	MBX_SET_COPY	0x06

/*
 *	set_verify command
 */

#define	MBX_SET_VERIFY	0x07

/*
 *        user_data : (4 longs) the first long  contains  the  master
 *                   copy disk identifier (SCSI-CHANNEL,ID,LUN), while
 *                   the second long contains the copy disk identifier
 *                   (SCSI-CHANNEL,ID,LUN).    The  structure  of  the
 *                   identifiers are as below:
 *
 *                    31        23       15       7         0   <-- bit position
 *                   +---------------------------------------+
 *                   !  RFU    ! LUN    !   ID   !  CHANNEL  !  #0
 *                   +---------------------------------------+
 *                   !  RFU    ! LUN    !   ID   !  CHANNEL  !  #1
 *                   +---------------------------------------+
 *                   !              RESERVED                 !  #2
 *                   +---------------------------------------+
 *                   !              RESERVED                 !  #3
 *                   +---------------------------------------+
 */

typedef
	struct {
		struct {
			unchar	s_channel;
			unchar	s_id;
			unchar	s_lun;
			unchar	s_rfu;
		}	s_disk[2];
		long	s_rfu1;
		long	s_rfu2;
	} set_t;


/*
 *        mr_status   : (word) command global result. Its value can be:
 *
 *                      - [0000] hex successful command end.
 *
 *                      - [0001] hex error during the set_copy command
 *                        (see APPL_FIELD).
 *
 *                      - [0009] hex permission denied: the controller
 *                        is    not   running   in   the   MAINTENANCE
 *                        environment.
 *
 *                      - [EExx] hex generic error during the SCSI bus
 *                        handling.
 */

#define	SET_COPY_ERROR		0x0001
#define	PERMISSION_DENIED	0x0009
#define	SCSI_BUS_ERROR		0xEE00

/*
 *        mr_appl_field : (4 longs) This field describes the errors, if any.
 *                   These longs are structured as follows:
 *
 *                         31     24-23    16-15     8-7      0     bit position
 *                        +------------------------------------+
 *                        ! QUALIF1 ! ADDIT1 ! SENSE1 ! VALID1 !  #0
 *                        +------------------------------------+
 *                        !        LOGICAL_BLOCK_NUM1          !  #1
 *                        +------------------------------------+
 *                        ! QUALIF2 ! ADDIT2 ! SENSE2 ! VALID2 !  #2
 *                        +------------------------------------+
 *                        !        LOGICAL_BLOCK_NUM2          !  #3
 *                        +------------------------------------+
 */

typedef
	struct {
		unchar	se_valid;
		unchar	se_sense;
		unchar	se_addit;
		unchar	se_qualif;
		long	se_blk_num;
	} set_error[2];

/*
 *	download_firmware_and_program command
 */

#define	MBX_DOWNLOAD_FW	0x08

/*
 *        mr_status : (word) command global result. Its value can be:
 *
 *                      - [0000] hex successful command end (EEPROM
 *                        was programmed).
 *
 *                      - [0001] hex cannot programme the EEPROM.
 *
 *                      - [FFFF] hex error on SYSTEM-BUS.
 */

#define	CANT_PROGRAM	0x0001

/*
 *	type_service
 *
 *
 *
 *		 31         23         15         7          0
 *		+---------------------+----------+------------+
 *		|         0x0000      | msg_type | int_source |
 *		+---------------------+----------+------------+
 *		int_source: status of pending interrupt events
 *		            if bit #i is set then the "i"th
 *		            interrupt events is arrived
 *		msg_type  : message associated with type #1 
 *                            interrupt, if any.
 *
 *
 *	         rep.code      description          comment
 *		----------------------------------------------------------------
 *		0x00000000     no events
 *		0x00000001     event #0             EFP2 reply
 *		0x0000xx02     event #1,            EFP2 msg = xx hex
 *		0x00000080     event #7             ESC1 reply
 *		0x0000xx03     events #0 and #1     EFP2 reply and msg = xx hex
 *		0x00000081     events #0 and #7     EFP2 reply and ESC1 reply
 *		----------------------------------------------------------------
 */

#define	EFP2_REPLY	0x01
#define	EFP2_MSG	0x02
#define	ESC1_REPLY	0x80

#pragma	pack()

/*
 *	msg_type
 */

#define	INIT_DIAG_OK	 	0x00
#define	REPLY_QUEUE_FULL 	0x01
#define	ERROR_MESSAGE		0x40
#define	COMMAND_QUEUE_NOT_FULL	0x80

#define	GET_MSG(x)	(((x)>>8) & 0xFF)
#define	GET_QUEUE(m)	((m) & 0x7F)

#define	GET_ERROR(x)		((x) & 0x0F)
#define	GET_ERROR_PARM(x)	(((x)>>4) & 0x03)

/*
 *
 */

#define	MBOX_ENTRIES		8
#define	MAX_QUEUES		MAX_DEVS
#define	RQ_ENTRIES		256

#ifdef	EFP_KLUDGE			/* Only 1 cmd per time */
#define	FEW_ENTRIES		3
#define	MANY_ENTRIES		3
#else
#define	FEW_ENTRIES		8
#define	MANY_ENTRIES		64
#endif

#define	EFP_RETRIES		5

/*
 *
 */

/*
 * DMA vector structure
 */
struct dma_vect {
        unsigned long   d_addr;      /* Physical src or dst          */
        unsigned long   d_cnt;       /* Size of data transfer        */
};

/*
 * DMA list structure
 */

/* Marco: 11-05-93
** SG_SEGMENT era limitato a 16 
**/
#define SG_SEGMENTS     64              /* Hard limit in the controller */
#define NBPP 4096
#define pgbnd(a)        (NBPP - ((NBPP - 1) & (int)(a)))

struct dma_list {
        unsigned int    d_size; /* List size (in bytes)         */
        struct dma_list *d_next;        /* Points to next free list     */
        struct dma_vect d_list[SG_SEGMENTS];    /* DMA scatter/gather list */
};

typedef struct dma_list dma_t;


/*
 * SCSI Request Block structure
 */
struct efp2_srb {
        struct xsb      *sbp;           /* Target drv definition of SB  */
	struct efp2_srb	*s_next;
	struct efp2_srb *s_priv;
	struct efp2_srb *s_prev;
        dma_t           *s_dmap;        /* DMA scatter/gather list      */
        paddr_t         s_addr;         /* Physical data pointer        */
	char		s_c;
	char		s_t:4;
	char		s_l:4;
	char 		s_b;

};

typedef struct efp2_srb sblk_t;

#define MAX_CMDSZ	12

typedef struct {
	short			ha_state;
	short 			ha_start_addr;
	short 			ha_scsi_channel;
	short			ha_iov;
	char			ha_name[12];
	char			ha_nameid;
	char			ha_id;
	char			ha_boot;
	int			ha_lhba;	/* was char */
	int			ha_lenlist;
	sblk_t			*ha_startlist;
	sblk_t			*ha_endlist;
	get_information_t	ha_info;
	long			ha_flag;
	unchar			ha_ctrl[HA_CTRLS];
	queue_descr_t		*ha_qd;
	reply_queue_t		*ha_rq;
	command_queue_t		*ha_mbox;
	configuration_table_t	*ha_conf;		/* <====== */
	long			ha_l;
	long			ha_msg;
	mbox_reply_t		ha_mbox_reply;		/* <====== ONLY 1 */
        lock_t                  *ha_lock;                /* SMP */
	lock_t			*ha_listlock;
	command_queue_t		*ha_cmd[MAX_TCS][MAX_LUS];
	char 			ha_sc_cmd[12]; /* SCSI cmd for pass-thru */
	bcb_t			*ha_bcbp;    /* Device control breakup pointer*/

} EFP2_DATA;

#define NDMA            200              /* Number of DMA lists          */

#define C_OPERATIONAL	0x01
#define OLI_EFP2	0x00
#define OLI_ESC2	0x01
#define SLOTS   15
#define EISA_IOBASE     0x1000

#define SYSID           0x0C80  /* system identifier offset                */
#define OLI1            0x3D    /* first byte manufacturer code            */
#define OLI2            0x89    /* second byte manufacturer code           */
#define SCSIBOARD       0x10    /* first byte board identifier             */
#define BID_ESC1        0x21    /* second byte board identifier - ESC-1    */
#define BID_ESC2mb      0x22    /*  "      "                    - ESC-2 mb */
#define BID_ESC2pi      0x23    /*  "      "                    - ESC-2 pi */
#define BID_ESC2fd      0x24    /*  "      "    - ESC-2 pi con floppy disk */
#define BID_EFP2        0x51    /*  "      "                    - EFP-2    */
#define BID_ARW         0xff    /*  Logical name for Arrow chip            */


#define	HA_F_MIRRORING		0xFF000000
#define	HA_F_DUAL_COMMAND	0x00000100
#define HA_F_DOUBLE_ENTRY	0x00000200
#define	HA_F_INIT		0x00000001
#define	HA_F_POSTINIT		0x00000002
#define	HA_F_START		0x00000004

typedef	struct {
	unsigned int	base;
} EFP2_HA_ADDR;

#define EFP2_BASE(c)	Efpboard[c].base_io_addr

typedef struct {
	unchar	c;
	unchar	t;
	unchar	l;
	unchar	rfu;
} EFP2_SCSI_RB;

typedef struct {
	command_queue_t	*cq;
} EFP2_LU;

/*	
**	Macros to help code, maintain, etc.
*/

#define SUBDEV(ch,t,l)	((ch << 6) | (t << 3) | l)
/* channel */
#define	CMD_Q(c, b, t, l)	((Efpboard[c].board_datap[b])->ha_cmd[t][l]) 
#define sp_prio(x)	((x)->sbp->sb.sb_type)
#define	NORMAL_PRIO	SCB_TYPE

/* Because of the use of bit fields and data padding the addresses and
 * sizes that are passed to the host adapter driver has been
 * adjusted. The following macros provide the address of
 * the structure for use by in scm structures. For address the macro
 * should be passed the address of the SCSI cdb ( see scsi.h - SCM_AD(x) )
 */

#define SCM_ADJ(x) 	((char *) x - 2)

extern int	efp2_ha_init();
extern int	efp2_ha_post_init();
extern void	efp2_ha_start();
extern int	efp2_ha_send();
extern int	efp2_ha_poll();
extern int	efp2_ha_intr();
extern int	efp2_ha_ioctl();
extern void	efp2_ha_io_setup();
extern int	efp2_ha_inquiry();

/*
 *	LMS error
 */

#define	E_TRANSFER	1
#define	E_RECOVER_OK	2
#define	E_RECOVER_NOK	3
#define	E_MIRROR	4

#define	EM_STILLMIRROR	0
#define	EM_NOMIRROR	1


#define	SOURCE_ERR	1
#define	MIRROR_ERR	2
#define	BOTH_ERR	(SOURCE_ERR|MIRROR_ERR)

typedef struct {
	long	addr;
  	long    ch;
} save_data;	
