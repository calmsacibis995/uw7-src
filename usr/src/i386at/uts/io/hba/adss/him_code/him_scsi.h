#ident	"@(#)kern-pdi:io/hba/adss/him_code/him_scsi.h	1.1"

/* SCSI and SCSI-2 definitions for use by Adaptec SCB Manager source
   manuscripts and the 6X60 device driver. */

/* SCSI message byte definitions */

#define COMMAND_COMPLETE_MSG		0x00
#define EXTENDED_MSG			0x01
#define  MODIFY_DATA_POINTER_MSG	0x00
#define  SYNCHRONOUS_DATA_TRANSFER_MSG	0x01
#define  WIDE_DATA_TRANSFER_MSG		0x02
#define SAVE_DATA_POINTER_MSG		0x02
#define RESTORE_POINTERS_MSG		0x03
#define DISCONNECT_MSG			0x04
#define INITIATOR_DETECTED_ERROR_MSG	0x05
#define ABORT_MSG			0x06
#define MESSAGE_REJECT_MSG		0x07
#define NOP_MSG				0x08
#define MESSAGE_PARITY_ERROR_MSG	0x09
#define LINKED_COMMAND_COMPLETE_MSG	0x0A
#define BUS_DEVICE_RESET_MSG		0x0C
#define ABORT_WITH_TAG_MSG		0x0D
#define CLEAR_QUEUE_MSG			0x0E
#define INITIATE_RECOVERY_MSG		0x0F
#define RELEASE_RECOVERY_MSG		0x10
#define TERMINATE_IO_PROCESS_MSG	0x11
#define SIMPLE_QUEUE_TAG_MSG		0x20
#define HEAD_OF_QUEUE_TAG_MSG		0x21
#define ORDERED_QUEUE_TAG_MSG		0x22
#define IGNORE_WIDE_RESIDUE_MSG		0x23
#define IDENTIFY_MSG			0x80	/* 0x80 to 0xFF inclusive */
#define  PERMIT_DISCONNECT		0x40

/* Lengths of SCSI extended messages */

#define MODIFY_DATA_POINTER_MSG_LEN 5
#define SYNCHRONOUS_DATA_TRANSFER_MSG_LEN 3
#define WIDE_DATA_TRANSFER_MSG_LEN 2

/* SCSI status definitions */

#define GOOD				0x00
#define CHECK_CONDITION			0x02
#define CONDITION_MET			0x04
#define BUSY				0x08
#define INTERMEDIATE			0x10
#define INTERMEDIATE_CONDITION_MET	0x14
#define RESERVATION_CONFLICT		0x18
#define COMMAND_TERMINATED		0x22
#define QUEUE_FULL			0x28
