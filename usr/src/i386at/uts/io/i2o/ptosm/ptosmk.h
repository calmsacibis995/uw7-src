#ident  "@(#)ptosmk.h	1.2"
#ident	"$Header$"
#define I2O_PT_HCT_SIZE 4096
#define I2O_PT_DMASIZE 32
#define I2OTRANS_MODNAME "i2otrans"
#define I2O_PT_VERSION "00.00.01"

/* Define states. We need to keep track of what we've asked for from
 * the STREAM head, and what we still need to send back etc.
 */ 

#define I2O_PT_STATE_INIT	0
#define I2O_PT_GET_HEADER	1
#define I2O_PT_GET_MSG		2
#define I2O_PT_GET_DB		3
#define I2O_PT_GET_DB0		4
#define I2O_PT_GET_DB1		5
#define I2O_PT_GET_DB2		6
#define I2O_PT_GET_DB3		7
#define I2O_PT_SEND_REPLY	8
#define I2O_PT_SEND_DB		9
#define I2O_PT_SEND_DB0		10
#define I2O_PT_SEND_DB1		11
#define I2O_PT_SEND_DB2		12
#define I2O_PT_ALL_DONE		13
#define I2O_PT_Q_GONE		14

extern int I2O_PT_DEBUG_ENTER;

/* While the structure of the I2O subsystem is unresolved we need to
 * acertain how many IOPs there are for ourselves. Use this structure.
 */

typedef struct {
unsigned int IOPNum;
unsigned int Active;
unsigned long BaseAddr[2];
} pt_iop_t;


typedef struct {

I2OptUserMsg_t	UserMsg;
unsigned char	State;
unsigned char	NumSGL;
unsigned char	pad[2];
queue_t		*WriteQueue;
mblk_t		*OurMsgBlk;
void 		*KernelAddress[MAX_PT_SGL_BUFFERS];
} I2OptMsg_t;

typedef struct {

I2O_MESSAGE_FRAME	    Header;
I2O_TRANSACTION_CONTEXT     TransactionContext;

} I2O_PT_MSG, *PI2O_PT_MSG;
