#ident	"@(#)kern-pdi:io/hba/efp2/ior0005.c	1.1"

#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <io/mkdev.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <svc/bootinfo.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/dynstructs.h>
#include <io/hba/efp2/efp2.h>
#include <util/mod/moddefs.h>

#else	/* _KENREL_HEADERS */

#include "sys/errno.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/cmn_err.h"
#include "sys/buf.h"
#include "sys/systm.h"
#include "sys/mkdev.h"
#include "sys/conf.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/kmem.h"
#include "sys/bootinfo.h"
#include "sys/debug.h"
#include "sys/scsi.h"
#include "sys/sdi_edt.h"
#include "sys/sdi.h"
#include "sys/dynstructs.h"
#include "efp2.h"
#include "sys/moddefs.h"

#endif /* KERNEL_HEADERS */


/*
Filename           : now := IOROUT5.c, org.:= IOR0004.C

Authors                : BADA Daniele, DI SCIULLO Tiziano, RUFFONI Ivano.
Previous revision date : September 20th, 1990
Last revision date     : August 26th, 1991

Brief description  : EFP2/EISA interface I/O routines 

-------------------------------------------------------------------------------
History            : 
-------------------------------------------------------------------------------
*/

#include "sys/param.h"
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif

typedef unsigned long   ULONG;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;


extern void outb(ULONG,BYTE);
extern BYTE inb(ULONG);


ULONG gain_semaphore0( ULONG );
ULONG efp_set ( ULONG , ULONG );
ULONG efp_start ( ULONG , ULONG );
ULONG efp_set_diag ( ULONG );
ULONG efp_cmd ( ULONG , ULONG );
ULONG efp_warning ( ULONG );
ULONG efp_reply_no_full ( ULONG );
ULONG efp_read_msg( ULONG );
void wait_before_retry ( ULONG );
void delaycpu( void );

/*
	AT Status Port for
	refresh toggle bit test
*/
#define  PIO   0x61



/*
   BMIC EISA register addresses 
*/

#define  ID_BYTE0     0x0C80
#define  ID_BYTE1     0x0C81
#define  ID_BYTE2     0x0C82
#define  ID_BYTE3     0x0C83

#define  GLOBAL_CONF  0x0C88
#define  EN_INT_FROM_BMIC   0x0C89
#define  SEMAPHORE0   0x0C8A
#define  SEMAPHORE1   0x0C8B
#define  L_DBELL_EN   0x0C8C
#define  L_DBELL_IN   0x0C8D
#define  E_DBELL_EN   0x0C8E
#define  E_DBELL_IN   0x0C8F

#define  TYPE_SERVICE 0x0C90	/* start of mailbox register commands */
#define  TYPE_MSG     0x0C98	/* start of mailbox register responses */

#define  IN_PAR1    0x0C91
#define  IN_PAR2    0x0C92
#define  IN_PAR3    0x0C93
#define  IN_PAR4    0x0C94

/*
   Operative CODE which must be written into the TYPE_SERVICE register.
*/

#define   SET_EFP2      0x01
#define   START_EFP2    0x02
#define   SET_DIAG      0x04
#define   REPLY_N_FULL  0x08
#define   WARNING       0x10
#define   COMMAND       0x80

#define   OK            0x00000000
#define   NOTOK         0xFFFFFFFF

#define   NUM_ATTEMPTS  100  /* number of retries testing the semaphore #0 */
#define   RING_D_BELL1  0x02

/*

-------------------------------------------------------------------------------

Function name: gain_semaphore


Input        : start_io : controller EISA address

Output       :  OK , NOTOK

Descr        :  It waits until the semaphore #0 will be free.

--------------------------------------------------------------------------------
*/
ULONG gain_semaphore0( start_io )

unsigned long start_io;

{
	register int     i;

	outb( (ULONG)(start_io + SEMAPHORE0), 0x01);
		/* testing flag and test bits */
	if( ((inb((ULONG)start_io + SEMAPHORE0)) & 0x03) == 0x01 )
		return(OK);    /* resource locked */

	for(i = NUM_ATTEMPTS; i>0 ; i--) {
		outb( (ULONG)(start_io + SEMAPHORE0), 0x01);
						/* testing flag and test bits */
			if( ((inb((ULONG)start_io + SEMAPHORE0)) & 0x03) == 0x01 )
				return(OK);    /* resource locked */

		wait_before_retry( (ULONG)1 );            /* delay before reattempting */
	}

	return(NOTOK);  /* semaphore0 is dead */
}



/*

-------------------------------------------------------------------------------

Function name: efp_set	


Input        : start_io : controller EISA address
             : irq      : IRQ selection
                          irq =  5 --> IRQ05
                          irq = 10 --> IRQ10
                          irq = 11 --> IRQ11
                          irq = 15 --> IRQ15

Output       : OK , NOTOK

Descr        : Set the the EFP2 interface

--------------------------------------------------------------------------------
*/
unsigned long efp_set ( start_io, irq )

unsigned long start_io;
unsigned long irq;

{
				/* enable interrupt from BMIC System doorbell */
	outb( ((ULONG)start_io + EN_INT_FROM_BMIC), 0x01);


				/* EISA System doorbell enable */
				/* enable all interrupts */
	outb( ((ULONG)start_io + E_DBELL_EN), 0xFF);

	if ( gain_semaphore0(start_io) == NOTOK ) 
		return ( NOTOK );


	outb((ULONG)start_io + IN_PAR1, (unsigned char)irq);  /* IRQ level */
					/* putting SET_EFP2 command */
	outb((ULONG)start_io + TYPE_SERVICE, SET_EFP2);

				/* sending interrupt request to adapter */
	outb((ULONG)start_io + L_DBELL_IN  , RING_D_BELL1);


   return ( OK );
}
/*

-------------------------------------------------------------------------------

Function name: efp_start


Input        : start_io : controller EISA address,
               phys_ptr : queue descriptor physical address.

Output       : OK , NOTOK 

Descr        : It sends to the controller the physical address of 
               the queue descriptor.

--------------------------------------------------------------------------------
*/
unsigned long efp_start ( start_io, phys_ptr )

unsigned long start_io;
unsigned long phys_ptr;

{

   
   register int i;
   register unsigned long mb_add;	/* starting IN_PAR mailbox registers */


   mb_add = start_io + IN_PAR1;

   if ( gain_semaphore0(start_io) == NOTOK ) 
   	return ( NOTOK );

					/* setting descriptor table address */
   for(i=0;i<4;i++){
		outb((ULONG)mb_add++, (unsigned char)phys_ptr);
   	phys_ptr >>= 8;
   }

					/* putting START_EFP2 command */
	outb((ULONG)start_io + TYPE_SERVICE, START_EFP2);

				/* sending interrupt request to adapter */
	outb((ULONG)start_io + L_DBELL_IN  , RING_D_BELL1);

   return ( OK );
}

/*

-------------------------------------------------------------------------------

Function name: efp_set_diag


Input        :	start_io : controller_EISA address

Output       :  OK , NOTOK 

Descr        : It starts the controller on-board diagnostic.

--------------------------------------------------------------------------------
*/
unsigned long efp_set_diag ( start_io )

unsigned long start_io;

{

   if ( gain_semaphore0(start_io) == NOTOK ) 
   	return ( NOTOK );


					/* putting SET_DIAG command */
	outb((ULONG)start_io + TYPE_SERVICE, SET_DIAG);

				/* sending interrupt request to adapter */
	outb((ULONG)start_io + L_DBELL_IN  , RING_D_BELL1);

   return ( OK );
}
   
/*

-------------------------------------------------------------------------------

Function name: 	 efp_cmd


Input        :	start_io : controller EISA address; 
                queue : the command queue into which a command has been 
                        compiled.

Output       :  OK , NOTOK

Descr        :  It signals that a new command has been compiled into a command
                queue.

--------------------------------------------------------------------------------
*/
unsigned long efp_cmd ( start_io, queue )

unsigned long start_io;
unsigned long queue   ;

{


   if ( gain_semaphore0(start_io) == NOTOK ) 
   	return ( NOTOK );

					/* putting COMMAND command */
                                        /* in "or" with the device queue */

	outb((ULONG)start_io + TYPE_SERVICE, (COMMAND | (unsigned char)queue));

				/* sending interrupt request to adapter */
	outb((ULONG)start_io + L_DBELL_IN , RING_D_BELL1);


   return ( OK );
}
/*

-------------------------------------------------------------------------------

Function name: 	efp_warning


Input        : start_io : controller EISA address 

Output       : OK , NOTOK 

Descr        : It signals that one of the module of the system has encountered
               a power-on to power-off transition or viceversa.

--------------------------------------------------------------------------------
*/
unsigned long efp_warning ( start_io )

unsigned long start_io;

{

   if ( gain_semaphore0(start_io) == NOTOK ) 
   	return ( NOTOK );

					/* putting REPLY_N_FULL command */
	outb((ULONG)start_io + TYPE_SERVICE, WARNING);

				/* sending interrupt request to adapter */
	outb((ULONG)start_io + L_DBELL_IN , RING_D_BELL1);

   return ( OK );

}
/*

-------------------------------------------------------------------------------

Function name: 	efp_reply_no_full


Input        : start_io : controller EISA address 

Output       : OK , NOTOK 

Descr        : It signals that the reply queue, previously signaled as full,
               is not quite full now.

--------------------------------------------------------------------------------
*/
unsigned long efp_reply_no_full ( start_io )

unsigned long start_io;

{

   if ( gain_semaphore0(start_io) == NOTOK ) 
   	return ( NOTOK );

					/* putting REPLY_N_FULL command */
	outb((ULONG)start_io + TYPE_SERVICE, REPLY_N_FULL);

				/* sending interrupt request to adapter */
	outb((ULONG)start_io + L_DBELL_IN , RING_D_BELL1);

   return ( OK );

}
/*

-------------------------------------------------------------------------------

Function name: 	efp_read_msg


Input        : start_io : controller EISA address

Output       : rep.code   : is defined as follows:

		 31         23         15         7          0
		+---------------------+----------+------------+
		|         0x0000      | msg_type | int_source |
		+---------------------+----------+------------+
		int_source: status of pending interrupt events
		            if bit #i is set then the "i"th
		            interrupt events is arrived
		msg_type  : message associated with type #1 
                            interrupt, if any.


	         rep.code      description          comment
		----------------------------------------------------------------
		0x00000000     no events
		0x00000001     event #0             EFP2 reply
		0x0000xx02     event #1,            EFP2 msg = xx hex
		0x00000080     event #7             ESC1 reply
		0x0000xx03     events #0 and #1     EFP2 reply and msg = xx hex
		0x00000081     events #0 and #7     EFP2 reply and ESC1 reply
		----------------------------------------------------------------

Descr        : this routine resets only the events #0 and #1 but it
               does not change the status of the event #7, if present.

Notice:        At the end of this routine all the doorbell interrupts are
               enabled.

--------------------------------------------------------------------------------
*/

unsigned long efp_read_msg(start_io)

unsigned long start_io;
{

register unsigned char source;
register union {
	unsigned long code;
	unsigned char ch[4];
}rep;

rep.code = 0L;
outb((ULONG)start_io + E_DBELL_EN , 0x00);/* disable all the doorbell interrupts */

#ifndef MIPSEB
source = (rep.ch[0] = inb((ULONG)start_io + E_DBELL_IN)) & 0x03 ;
#else
source = (rep.ch[3] = inb((ULONG)start_io + E_DBELL_IN)) & 0x03 ;
#endif  MIPSEB

if(source & 0x01){
					/* switch off the event #0 */
	outb(((ULONG)start_io + E_DBELL_IN) , 0x01);
	source &= 0xFE;		
}
if (source){
					/* switch off the event #1 */
	outb(((ULONG)start_io + E_DBELL_IN) , 0x02);
#ifndef MIPSEB
	rep.ch[1] = inb((ULONG)start_io + TYPE_MSG);
#else
	rep.ch[2] = inb((ULONG)start_io + TYPE_MSG);
#endif  MIPSEB
					/* release the semaphore */
	outb(((ULONG)start_io + SEMAPHORE1) , 0x00);
}
outb((ULONG)start_io + E_DBELL_EN , 0xFF);/* enable all the doorbell interrupts */

return(rep.code);

}


/*

-------------------------------------------------------------------------------

Function name:    wait_before_retry


Input        : unsigned int delayTime

Output       : none

Descr        : It waits for a transition from 0 to 1 ( or viceversa )
					of the Standard EISA bit, allowing a delay of at least
					15 * delayTime microseconds.

--------------------------------------------------------------------------------
*/
void wait_before_retry ( delayTime )
ULONG delayTime;
{

	register BYTE value_hi;
	register BYTE value_lo;

	value_lo = (BYTE)inb( (ULONG)PIO );
	value_hi = value_lo;
	value_hi &= 0x10;

	do {
		do {
			delaycpu();
			value_lo = (BYTE)inb( (ULONG)PIO );
			value_lo &= 0x10;
		} while ( value_lo == value_hi );
		value_hi = value_lo;
	} while( delayTime-- );

}


/*

-------------------------------------------------------------------------------

Function name:    delaycpu


Input        : none

Output       : none

Descr        : It waits for about 5 microseconds on a 80386 - 33 MHz system

--------------------------------------------------------------------------------
*/

void delaycpu(  )
{

	drv_usecwait(5);
}
