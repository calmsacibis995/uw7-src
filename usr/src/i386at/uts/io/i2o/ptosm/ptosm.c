#ident  "@(#)ptosm.c	1.2"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 *
 */

/*
 * Modification history
 *
 *      L000    25Feb97         keithp@sco.com
 */

#include "ads_doc.h"
#include "bytesize.h"

#include "i2o_osmhd.h"     /* contains all required UnixWare includes */
#include "com/cfgparam.h"
#include "i2osig/i2otypes.h"
#include "i2osig/i2omsg.h"
#include "i2osig/i2outil.h"
#include "i2osig/i2oexec.h"
#include "com/osmcomm.h"
#include "com/common.h"
#include "debug.h"
#include "ptosm.h"
#include "ptosmk.h"

#ifdef STANDALONE

U32 i2opt_debug_flags = (U32)(DEBUG_16 | DEBUG_0);
#define EXTERN

#else

U32 i2opt_debug_flags = (U32)(0);
#define EXTERN	extern

#endif

STATIC  int     i2opt_load(void);
STATIC  int     i2opt_unload(void);
STATIC  int     i2opt_open(queue_t *, dev_t *, int, int, cred_t *);
STATIC  int     i2opt_close(queue_t *, int, cred_t *);
STATIC  int     i2opt_wput(queue_t *, mblk_t *);
STATIC 	void	i2opt_Cleanup(void);
STATIC	U8 	i2oPTnotify(I2OHCTEntry_t*);
STATIC	void	i2oPtCallBack(U8, PI2O_MESSAGE_FRAME );
STATIC	void	i2oPtDoIoctl(queue_t *, mblk_t *);
STATIC	void	i2oPtDoIocData(queue_t *qp, mblk_t *mp);
STATIC	void	i2oPtDataCopy(mblk_t *, char *, unsigned int );
STATIC	void	i2oPtBufferFree(mblk_t *);
PUBLIC	int	i2opt_init (void);
STATIC void	i2oOSMSendNop (U8, I2O_MESSAGE_FRAME *);

STATIC	U8	i2oPtNumIops;

pt_iop_t *pt_iops;

int i2optdevflag = 0;

extern	int	SGLBufMaxLength;
extern	int	OUTBOUND_FRAME_SIZE;

#define DRVNAME "i2opt"

/*
 * STREAMS required structures.
 */

struct module_info i2opt_minfo = {

	81,				/* Module ID number */
	"i2opt",			/* Module name, max FMNAMESZ chars */
	0,				/* Minimum packet size */
	INFPSZ,				/* Max packet size */
	1024,				/* High water mark */
	128				/* Low water mark */

};

struct qinit i2opt_write = {

	i2opt_wput,			/* Put procedure */
	NULL,				/* Service procedure */
	i2opt_open,			/* Open procedure */
	i2opt_close,			/* Close procedure */
	NULL,				/* Reserved (qadmin) */
	&i2opt_minfo,			/* Module info structure pointer */
	(struct module_stat *)NULL	/* Module statistics structure */

};

struct qinit i2opt_read = {

	NULL,				/* Put procedure */
	NULL,				/* Service procedure */
	i2opt_open,			/* Open procedure */
	i2opt_close,			/* Close procedure */
	NULL,				/* Reserved (qadmin) */
	&i2opt_minfo,			/* Module info structure pointer */
	(struct module_stat *)NULL	/* Module statistics structure */

};

struct streamtab i2optinfo = {

	&i2opt_read,			/* Read queue init pointer */
	&i2opt_write,			/* Write queue init pointer */
	(struct qinit *)NULL,		/* Lower read queue (mux only) */
	(struct qinit *)NULL		/* Lower write queue (mux only) */

};

MOD_DRV_WRAPPER(i2opt, i2opt_load, i2opt_unload, NULL, DRVNAME);


STATIC	int	i2opt_dynamic = 0;

STATIC	void	i2oPtDummyFree();

/* This structure is given to esballoc to "free" buffers attached to
 * a message header. (Actually it frees nothing as i2oPtBufferFree takes
 * care of everything.
 */

STATIC	struct	free_rtn i2oPtFreeEsballoc = {
	i2oPtDummyFree,
	NULL
};


/****************************************************************************
  Function prototypes for calls into the message layer.
****************************************************************************/
EXTERN S8
OSMMsgIopInit
 (
   U32 IOPaddr,                 /* Address of IOP to initialize */
   U8* IOPnum
 );

EXTERN S8
OSMMsgRegister
 (
   U8 (*osmnotify)(I2OHCTEntry_t*), /* Notify routine address */
   U8* buffer,                      /* Buffer to put HCT into */
   U32 size                         /* Size of buffer */
 );

EXTERN I2O_MESSAGE_FRAME*
OSMMsgAlloc
 (
   U8 iop                       /* IOP from which a buffer is needed */
 );

EXTERN S8
OSMMsgSend
 (
   U8                 iop,      /* IOP to send to */
   I2O_MESSAGE_FRAME* msg       /* message payload */
 );

EXTERN S8
OSMMsgFreeRepBuf
 (
   U8                 iop,      /* IOP # */
   I2O_MESSAGE_FRAME* reply     /* message buffer address */
 );

EXTERN I2O_MESSAGE_FRAME*
OSMMsgGetInboundBuf
 (
   U8		      iop
 );

/******************************************************************************/


/*****************************************************************************
NAME: i2oOSM_load
SUMMARY: OS calls this function when we are loaded dynamically

DESCRIPTION:
calls i2oOSMinit and i2oOSMstart

RETURNS:
  - 0 if successful
  - ENODEV if unsuccessful
*****************************************************************************/
int
i2opt_load (void)
{
#ifdef I2O_DEBUG
  if(i2opt_debug_flags)
    cmn_err(CE_NOTE, "i2opt_load: build 5/19/97\n");
#endif
  I2ODBG0("i2opt_load: starting");

#ifdef I2O_DEBUG
  if(I2O_PT_DEBUG_ENTER)
    ENTER_DEBUGGER();
#endif

  i2opt_dynamic = 1; /* we are being dynamically loaded */

  /*
   * call init and start
   */
  if(i2opt_init() || i2opt_start())
    {
      i2opt_Cleanup(); /* clean up any allocations */
      return (ENODEV);
    }

  return SUCCESS;
}

/*****************************************************************************
NAME: i2opt_Cleanup
SUMMARY: clean up if driver load fails

DESCRIPTION:
Frees up memory and deallocates locks if the driver load fails

RETURNS: Nothing
*****************************************************************************/
STATIC void
i2opt_Cleanup(void)
{

  I2ODBG2("i2opt_Cleanup: starting.");

}

/*****************************************************************************
NAME: i2opt_unload
SUMMARY: OS calls this function when it wants to unload us

DESCRIPTION:
returns EBUSY, since IOP drivers can never be unloaded

RETURNS: EBUSY
*****************************************************************************/
PUBLIC int
i2opt_unload (void)
{
  return EBUSY;
}

#ifndef STANDALONE
/*****************************************************************************
NAME: i2opt_init
SUMMARY: initialization routine called by OS or by i2opt_load

DESCRIPTION:
set up our idata array using autoconfiguration

RETURNS:
  - 0 if successful
  - -1 if unsuccessful
*****************************************************************************/
PUBLIC int
i2opt_init (void)
{
  U32    i;            /* loop counter */
  U8     IOPNum;       /* controller number assigned in OSMMsgIopInit */
  U32    sleepflag;
  unsigned int	pt_iop_cnt = 0;
  struct cm_args          cm_args;
  I2ODBG0("i2opt_init: starting");

  /*
   * identify ourselves
   */
  cmn_err(CE_CONT, "I2O Pass Through OSM " I2O_PT_VERSION "\n");

  sleepflag = i2opt_dynamic ? KM_SLEEP : KM_NOSLEEP;

if((i2oPtNumIops = cm_getnbrd(I2OTRANS_MODNAME)) <= 0)
 {
   I2ODBG0("i2opt_init: Number of boards <= 0 ");

   return(-1);
 }

if(i2oPtNumIops > MAXIOPS)
 {
   I2ODBG0("i2opt_init: Found more IOPs than supported by driver");

   i2oPtNumIops = MAXIOPS;
 }

if((pt_iops = (pt_iop_t *)kmem_zalloc(i2oPtNumIops * sizeof(pt_iop_t), KM_NOSLEEP)) == 0)
 {
   I2ODBG0("i2opt_init: Failed to alloc mem for hwinfo");

   return(-1);
 }

for(i = 0; i < i2oPtNumIops; i++)
 {

   /*
    * Get the key into the resource manager.
    */
   cm_args.cm_key = cm_getbrdkey( I2OTRANS_MODNAME, i);
   cm_args.cm_n = 0;

   /* get the base memory address */
   cm_args.cm_param = CM_MEMADDR;
   cm_args.cm_val = &(pt_iops[i].BaseAddr[0]);
   cm_args.cm_vallen = 8;

   if(cm_getval(&cm_args ) != 0)
    {
      I2ODBG0("i2opt_init: Failed to get MemAddr for board");

      pt_iops[i].Active = 0;
      continue;
    }

    pt_iops[i].Active = 1;

 }
  /* Initialize all of the IOPs */
  for(i = 0; i < i2oPtNumIops; i++)
    {
      if(pt_iops[i].Active == 0)
	continue;

      if(OSMMsgIopInit(pt_iops[i].BaseAddr[0], &IOPNum) == SUCCESS)
        {
	  pt_iops[i].IOPNum = (int)IOPNum;
	  pt_iop_cnt++;
	}
	else
	{
	  pt_iops[i].Active = 0;
	}
     }


  if(pt_iop_cnt >= 1)
    return SUCCESS;
  else
    return FAILURE;
}

/*****************************************************************************
NAME: i2opt_start
SUMMARY: called by OS or by i2oOSM_load

DESCRIPTION:
register ourselves with SDI and with the I2O message layer

RETURNS:
  - 0 if successful
  - -1 if unsuccessful
*****************************************************************************/
PUBLIC int
i2opt_start (void)
{
  S32             cntl_num;
  U8              count = 0;
  U32             c;
  U32             sleepflag;
  I2OHCTEntry_t * i2opt_HctPtr;

  I2ODBG0("i2oOSMstart: starting");

  sleepflag = i2opt_dynamic ? KM_SLEEP : KM_NOSLEEP;

  /*
   * Allocate memory for the HCT. We don't need to DMA with this memory so
   * kmem_alloc should work fine.
   */
  if((i2opt_HctPtr = (I2OHCTEntry_t *)kmem_zalloc(I2O_PT_HCT_SIZE, sleepflag))
                                                                        == NULL)
    {
      cmn_err(CE_WARN, "i2opt: Unable to allocate memory for the HCT\n");
      return FAILURE;
    }

  /*
   * Register with the I2O message layer and get a copy of the HCT.
   */
  if(OSMMsgRegister(i2oPTnotify, (U8 *)i2opt_HctPtr, I2O_PT_HCT_SIZE)
                                                                     != SUCCESS)
    {
      cmn_err(CE_WARN, "i2opt_start: Failed to register with message layer\n");
      kmem_free((void *)i2opt_HctPtr, I2O_PT_HCT_SIZE);
      return FAILURE;
    }

  kmem_free((void *)i2opt_HctPtr, I2O_PT_HCT_SIZE);

  return SUCCESS;
}

#endif /* !STANDALONE */

/*****************************************************************************
NAME: i2opt_halt
SUMMARY: called by OS at shutdown time

DESCRIPTION:
This is an optional routine.
Should make sure that no interrupts are pending, and inform the devices that
no more interrupts should be generated.
Right now does nothing.

RETURNS: nothing
*****************************************************************************/
PUBLIC void
i2opt_halt (void)
{
  I2ODBG0("i2opt_halt: starting");
}

/*****************************************************************************
NAME: i2opt_kmem_zalloc_physreq
SUMMARY: Allocate zeroed out aligned contiguous physical memory.

DESCRIPTION:
function to allocate physically aligned kernel memory.

RETURNS:
  - NULL if unsuccessful
  - a pointer to the allocated memory if successful
*****************************************************************************/
PRIVATE U8 *
i2opt_kmem_zalloc_physreq
  (
    size_t      size,         /* size of needed memory */
    int         flags,        /* sleep flag */
    paddr_t     phys_align,   /* alignment requirements */
    paddr_t     phys_boundary /* boundary requirements */
  )
{
  U8    *mem = NULL;
  physreq_t     *preq;

  /* allocate a physical alignment requirements structure */
  preq = physreq_alloc(flags);
  if(preq == NULL)
    return NULL;

  /* fill out the physreq structure based on the type of memory needed */
  preq->phys_align = phys_align;
  preq->phys_boundary = phys_boundary;
  preq->phys_dmasize = I2O_PT_DMASIZE;
  preq->phys_flags |= PREQ_PHYSCONTIG; /* make memory physically contiguous */

  /* prep the physreq structure for use */
  if(!physreq_prep(preq, flags))
    {
      physreq_free(preq);
      return NULL;
    }

  mem = (U8 *)kmem_zalloc_physreq(size, preq, flags);
  physreq_free(preq);

  return mem;
}

STATIC	U8 
i2oPTnotify(I2OHCTEntry_t* dummy)
{
}

STATIC  int     
i2opt_open(queue_t *qp, dev_t *dev, int flag, int sflag, cred_t *credp)
{

	return 0;


}

STATIC  int     
i2opt_close(queue_t *qp, int flag, cred_t *credp)
{

	return 0;

}

/*****************************************************************************
NAME: i2opt_wput
SUMMARY: This is the driver write out routine. It accepts all messages from
	 upstream and acts on them

DESCRIPTION:

	M_FLUSH - standard flushing code

	M_IOCTL - Normally a user request to perform action
		  Two types at present:
			GetNumber of IOPs - simply initiates a M_COPYOUT
					    with the data

			Message Transaction - Starts a complicated sequence
					    of events that begin with an 
					    M_COPYIN of the users request 
					    structure.
	 
RETURNS: Nothing - a qreply is sent back up stream

*****************************************************************************/

STATIC  int     
i2opt_wput(queue_t *qp, mblk_t *mp)
{

mblk_t		*mp1;
I2OptUserMsg_t	UserMsg;
unsigned char 	*dst;
unsigned int	count;
unsigned int	BufferSize;
PI2O_SGE_SIMPLE_ELEMENT	ssg;
I2OptMsg_t 	*Msg_p;
PI2O_PT_MSG	MsgFrame_p;
U8		*MsgBuf_p;
int		i,j;
int		SGLOffset;
int		stat;

	I2ODBG16("PTOSM wput entered");

	switch (mp->b_datap->db_type)

	{

	case M_FLUSH:

		I2ODBG16("PTOSM wput case M_FLUSH:");

      		/* Standard M_FLUSH code */

		if (*mp->b_rptr & FLUSHW) {
			flushq(qp,FLUSHDATA);
		}
		if (*mp->b_rptr & FLUSHR) {
    			flushq(RD(qp), FLUSHDATA);
    			*mp->b_rptr &= ~FLUSHW;
    			qreply (qp,mp);
		} else
    			freemsg (mp);

		break;

	case M_IOCTL:

		I2ODBG16("PTOSM wput case M_IOCTL:");

		/* Do all the ioctl processing in i2oPtDoIoctl */
	
		i2oPtDoIoctl(qp, mp);	

		break;

	case M_IOCDATA:

		I2ODBG16("PTOSM wput case M_IOCDATA:");

		/* This is the result of a M_COPYIN/OUT */

		i2oPtDoIocData(qp, mp);

		break;

	default:

		I2ODBG16("PTOSM wput case default:");

		/* Don't know (don't really care) what this is
		 * simply free the message and go
		 */

		freemsg(mp);
		break;

	}
}

/*****************************************************************************
NAME: i2oi2oPtDoIoctl
SUMMARY: This is where the IOCTL processing takes place

DESCRIPTION:

  Two types at present:
	GetNumber of IOPs - simply initiates a M_COPYOUT with the data

	Message Transaction - Starts a complicated sequence of events that
			      begin with an M_COPYIN of the users request 
			      structure.
	 
RETURNS: Nothing - a qreply is sent back up stream

*****************************************************************************/

STATIC void
i2oPtDoIoctl(queue_t *qp, mblk_t *mp)
{

struct iocblk	*iocbp;
I2OptMsg_t	*Msg_p;
struct copyreq	*cqp;
mblk_t		*mp1;


	iocbp = (struct iocblk *)mp->b_rptr;
	switch(iocbp->ioc_cmd) {

	case I2O_PT_NUMIOPS:

		I2ODBG16("PTOSM i2oPtDoIoctl case I2O_PT_NUMIOPS:");

		/* Simple request wanting to know how many IOPs we have.
		 * Data is returned as an int into the user arg.
		 */

		/* Check if its TRANSPARENT (reject it if its not) */

		if(iocbp->ioc_count != TRANSPARENT)
		{

			I2ODBG16("PTOSM i2oPtDoIoctl not TRANSPARENT");

			/* Free anything after the first block now */
			if(mp->b_cont)
			{
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
			
			mp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
	
			qreply(qp, mp);
		
			break;

		}

		/* The user data address is in the block mp->b_cont
		 * Convert mp into a M_COPYOUT message and replace the
		 * address in the attached block with the data value
		 */

		cqp = (struct copyreq *)mp->b_rptr;
		cqp->cq_addr = (caddr_t) *(long *)mp->b_cont->b_rptr;
		cqp->cq_size = sizeof(int);
		cqp->cq_flag = 0;
		mp->b_datap->db_type = M_COPYOUT;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		*(long *)mp->b_cont->b_rptr = i2oPtNumIops;
		
		I2ODBG16("PTOSM i2oPtDoIoctl M_COPYOUT for NumIOPs");

		qreply(qp, mp);

		break;

	/* End of case I2O_PT_NUMIOPS */

	case I2O_PT_MSGTFR:

		I2ODBG16("PTOSM i2oPtDoIoctl case I2O_PT_MSGTFR");

		/* Perform an IOP message transaction */

		/* Check if its TRANSPARENT (moan it if its not) */

		if(iocbp->ioc_count != TRANSPARENT)
		{
			/* Free anything after the first block now */
			if(mp->b_cont)
			{
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
			
			mp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
	
			qreply(qp, mp);
		
			break;

		}

		/* Reuse the M_IOCTL Block for M_COPYIN
		 * The STREAM head guarantees that it will be big enough
		 */

		cqp = (struct copyreq *)mp->b_rptr;

		/* The user address is the only data in the attached block
		 * Use it, then free the block
		 */

		cqp->cq_addr = (caddr_t) *(long *)mp->b_cont->b_rptr;
		freemsg(mp->b_cont);
		mp->b_cont = NULL;

	
		/* Need to hold our state information, put it in an mblk_t */

		mp1 = allocb(sizeof(I2OptMsg_t), BPRI_MED);

		if(mp1 == NULL)
		{
			if(mp->b_cont)
			{
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
			mp->b_datap->db_type = M_IOCNAK;
			mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

			iocbp->ioc_error = ENOMEM;
			qreply(qp, mp);
		
			break;
		}

		bzero(mp1->b_rptr, sizeof(I2OptMsg_t));
	
		/* Make a note of how far we've got in the private data
		 * field. We use I2O_PT_GET_HEADER here to indicate that
		 * the next M_IOCDATA message has the header attached.
		 */

		((I2OptMsg_t *)(mp1->b_rptr))->State = I2O_PT_GET_HEADER;
	
		cqp->cq_private = mp1;
		cqp->cq_size = sizeof(I2OptUserMsg_t);
		cqp->cq_flag = 0;
		mp->b_datap->db_type = M_COPYIN;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

		I2ODBG16("PTOSM i2oPtDoIoctl M_COPYIN for header");
		
		qreply(qp, mp);

		break;

	/* End of case I2O_PT_MSGTFR */

	default:
	
		I2ODBG16("PTOSM i2oPtDoIoctl case default:");

		/* Nobody wanted this one (not even us) and we're bottom of
		 * the stack. Reject it with an error. Free anything after
		 * the first block now
		 */

		if(mp->b_cont)
		{
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		
		mp->b_datap->db_type = M_IOCNAK;

		qreply(qp, mp);
	
		break;

	}		

	
}


/*****************************************************************************
NAME: i2oPtDoIocData
SUMMARY: This is where the M_IOCDATA processing takes place

DESCRIPTION:

	The iocblk contains the ioctl type, so we can switch on it to
	determine whether we have state info in the private data field.

	GetNumber of IOPs - simply initiated a M_COPYOUT with the data
			    If it was OK simply send M_IOCACK upstream
			    as we are done

	Message Transaction - The M_IOCDATA will be in response to a M_COPYIN
			      or M_COPYOUT. Which one is determined by the 
			      State field in the attached private data.
			      Determine which phase of the sequence has just
			      comleted and start the next one.
	 
RETURNS: Nothing - a qreply is sent back up stream

*****************************************************************************/

STATIC void
i2oPtDoIocData(queue_t *qp, mblk_t *mp)
{

struct	iocblk		*iocbp;	
struct	copyreq		*cqp;
struct	copyresp	*crp;
I2OptUserMsg_t 		*I2oUserMsg_p;
I2OptMsg_t 		*I2oMsg_p;
I2O_SGE_SIMPLE_ELEMENT	*ssg;
I2O_PT_MSG		*MsgFrame_p;
mblk_t			*mp1, *mp2;
int			count, i;
int			EndSeen, NextState;
int			UserMsgError;
int			SGLOffset;

	crp = (struct copyresp *)mp->b_rptr;
	iocbp = (struct iocblk *)mp->b_rptr;

	switch(crp->cp_cmd)
	{

	case I2O_PT_MSGTFR:

		I2ODBG16("PTOSM i2oPtDoIocData case I2O_PT_MSGTFR:");

		mp1 = crp->cp_private;

		I2oMsg_p = (I2OptMsg_t *)mp1->b_rptr;

		/* Check if the M_COPYIN/OUT failed */
		if(crp->cp_rval)
		{

			I2ODBG16("PTOSM M_COPYIN/OUT failed:");

			/* Check if we have buffers to free */
			
			i2oPtBufferFree(mp1);

			/* No M_IOCNAK - the STREAM head already knows */
	
			freemsg(mp);
			
			break;
		}

		switch(I2oMsg_p->State)
		{

		case I2O_PT_GET_HEADER:

			I2ODBG16("PTOSM case I2O_PT_GET_HEADER:");

			/* The M_COPYIN was to fetch the user's structure */
		
			/* Copy the User data structure  */
			i2oPtDataCopy(mp->b_cont, (char *)mp1->b_wptr,
							sizeof(I2OptUserMsg_t));

			/* Free the buffer now we've used it */

			freemsg(mp->b_cont);
			mp->b_cont = NULL;

			I2oUserMsg_p = (I2OptUserMsg_t *)mp1->b_rptr;

			/* Sanity check of what we've got before we go
			 * any further.
			 * 1) The version matches
			 * 2) IOP num must be < i2oPtNumIops
			 * 3) Message size < INBOUND_MSG_FRAME_SIZE  and < 12
			 * 4) SGL buffers > 0 and < SGLBufMaxLength
			 * 5) SGL buffers in contiguous array slots
			 * 6) Reply buffer is < sizeof(I2O_MSG_FRAME)
			 */

			UserMsgError = SUCCESS;

			if(I2oUserMsg_p->Version != I2O_PT_VERSION_1 ||
			   I2oUserMsg_p->IopNum >= i2oPtNumIops ||
			   I2oUserMsg_p->MessageLength < 12	||
			   I2oUserMsg_p->MessageLength > INBOUND_MSG_FRAME_SIZE)
			{
				UserMsgError = EINVAL;
			}

			for(count = 0, EndSeen = 0; count < MAX_PT_SGL_BUFFERS
					&& UserMsgError == SUCCESS ; count++)
			{
				if((I2oUserMsg_p->Data[count].Flags & 
						I2O_PT_DATA_MASK) == 0)
				{
					EndSeen = 1;
					continue;
				}

				if(I2oUserMsg_p->Data[count].Length == 0 ||
					I2oUserMsg_p->Data[count].Length >
							 SGLBufMaxLength ||
					EndSeen == 1)
				{
					UserMsgError = EINVAL;
					break;
				}

			}				

			if(UserMsgError != SUCCESS ||
			   I2oUserMsg_p->ReplyLength > OUTBOUND_FRAME_SIZE * 4)
			{
				/* Oh dear, bail out now */

				I2ODBG16("PTOSM header invalid ");

				freemsg(mp1);
				mp->b_datap->db_type = M_IOCNAK;
				mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

				iocbp->ioc_error = EINVAL;
				qreply(qp, mp);
			
				break;
			}

			/* All looks OK, so send a M_COPYIN to get the 
			 * users prototype for the I2O Message 
			 */

			mp1->b_wptr += sizeof(I2OptMsg_t);

			I2oMsg_p = (I2OptMsg_t *)mp1->b_rptr;

			/* Send a M_COPYIN up to get the Message block */

			cqp = (struct copyreq *)mp->b_rptr;

			cqp->cq_private = mp1;
			cqp->cq_addr = (caddr_t)I2oMsg_p->UserMsg.Message; 
			cqp->cq_size = I2oMsg_p->UserMsg.MessageLength;
			cqp->cq_flag = 0;
			mp->b_datap->db_type = M_COPYIN;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			I2oMsg_p->State = I2O_PT_GET_MSG;

			I2ODBG16("PTOSM M_COPYIN for Message");
	
			qreply(qp, mp);

			break;

		/* End of case I2O_PT_GET_HEADER */

		case I2O_PT_GET_MSG:

			I2ODBG16("PTOSM case I2O_PT_GET_MSG");

			/* Place the user's message in safe keeping */

			linkb(mp1, unlinkb(mp));

			if(((PI2O_MESSAGE_FRAME)(mp1->b_cont->b_rptr))->
						TargetAddress == I2O_HOST_TID)
			{
				/* Don't support talking to OSM itself (yet) */

				i2oPtBufferFree(mp1);
				mp->b_datap->db_type = M_IOCNAK;
				mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

				iocbp->ioc_error = EINVAL;
				qreply(qp, mp);
		
				break;	
				
			}
			I2oMsg_p->State = I2O_PT_GET_DB;

			/* Fall through into I2O_PT_GET_DB */

		case I2O_PT_GET_DB:
		case I2O_PT_GET_DB0:
		case I2O_PT_GET_DB1:
		case I2O_PT_GET_DB2:

		case_I2O_PT_GET_DB:	/* case_I etc. is correct, jump label */

			switch(I2oMsg_p->State)
			{	
			case I2O_PT_GET_DB:

				I2ODBG16("PTOSM case I2O_PT_GET_DB");
				count = -1;
				NextState = I2O_PT_GET_DB0;
				break;

			case I2O_PT_GET_DB0:

				I2ODBG16("PTOSM case I2O_PT_GET_DB0");
				count = 0;
				NextState = I2O_PT_GET_DB1;
				break;

			case I2O_PT_GET_DB1:

				I2ODBG16("PTOSM case I2O_PT_GET_DB1");
				count = 1;
				NextState = I2O_PT_GET_DB2;
				break;

			case I2O_PT_GET_DB2:

				I2ODBG16("PTOSM case I2O_PT_GET_DB2");
				count = 2;
				NextState = I2O_PT_GET_DB3;
				break;

			}

			/* Check if this is a response to a 
			 * previous M_COPYIN, or a fall through from the
			 * I2O_PT_GET_HEADER case
			 */

			if(I2oMsg_p->State != I2O_PT_GET_DB && 
				(I2oMsg_p->UserMsg.Data[count].Flags &
					I2O_PT_DATA_WRITE) == I2O_PT_DATA_WRITE)
			{
				/* Real data to sort out here */
				I2ODBG16("PTOSM Data to copy");

				i2oPtDataCopy(mp->b_cont,
					I2oMsg_p->KernelAddress[count],
					I2oMsg_p->UserMsg.Data[count].Length);
			}

			/* Now that ones sorted, is there another to do */

			count++;

			I2oMsg_p->NumSGL = count;	

			I2oMsg_p->State = NextState;

			if((I2oMsg_p->UserMsg.Data[count].Flags &
				 		(I2O_PT_DATA_MASK)) == 0)
			{
				/* We've done all the SGLs */

				I2ODBG16("PTOSM goto case_I2O_PT_GET_DB3");

				goto case_I2O_PT_GET_DB3;

			}


			/* We need a buffer for this. If it returns its OK */

			I2oMsg_p->KernelAddress[count] =
				 i2opt_kmem_zalloc_physreq(
					I2oMsg_p->UserMsg.Data[count].Length,
							 KM_SLEEP, 1024, 0);

			/* For write operations we need to get the data
			 * from user space with a M_COPYIN message (so that
			 * it can be copied into the kernel buffer by the 
			 * code at the beginning of this case statement - 
			 */

			if((I2oMsg_p->UserMsg.Data[count].Flags &
				I2O_PT_DATA_WRITE) == I2O_PT_DATA_WRITE)
			{
				/* Its a write operation
				 * Send a M_COPYIN  to get the Message block
				 */

				cqp = (struct copyreq *)mp->b_rptr;

				cqp->cq_private = mp1;
				cqp->cq_addr =
				  (caddr_t)I2oMsg_p->UserMsg.Data[count].Data; 
				cqp->cq_size =
				   I2oMsg_p->UserMsg.Data[count].Length;
				cqp->cq_flag = 0;
				mp->b_datap->db_type = M_COPYIN;
				mp->b_wptr = 
				  mp->b_rptr + sizeof(struct copyreq);

				I2ODBG16("PTOSM M_COPYIN for SGL buffer");

				qreply(qp,mp);

				break;
			}

			if(NextState != I2O_PT_GET_DB3)
			{
				goto case_I2O_PT_GET_DB;
			}
			else
			{
				goto case_I2O_PT_GET_DB3;
			}

			break;	/* Can't actually get here...*/

		/* end of case I2O_PT_GET_DBx */

		case I2O_PT_GET_DB3:

			count = 3;

		case_I2O_PT_GET_DB3:	/* case_I etc. is correct, jump label */

			I2ODBG16("PTOSM case I2O_PT_GET_DB3");

			/* If the final SGL is a write, copy the data for
			 * it which is attached in the b_cont field
			 */
	
			if((I2oMsg_p->UserMsg.Data[3].Flags &
				I2O_PT_DATA_WRITE) == I2O_PT_DATA_WRITE)
			{

				I2ODBG16("PTOSM I2O_PT_GET_DB3 data copy");

				i2oPtDataCopy(mp->b_cont,
					I2oMsg_p->KernelAddress[3],
					I2oMsg_p->UserMsg.Data[3].Length);
			}

			if((I2oMsg_p->UserMsg.Data[3].Flags &
							I2O_PT_DATA_MASK) != 0)
			{
				I2oMsg_p->NumSGL = count;	
			}

			/* We've done with it, free anything still attached.*/

			if(mp->b_cont != NULL)
			{
				freemsg(mp->b_cont);
				mp->b_cont= NULL;
			}

			/* Enough getting data etc. from the user, put it
			 * all together and send it on to the IOP
		 	*/
	
			if((MsgFrame_p = 
		   		(PI2O_PT_MSG)OSMMsgGetInboundBuf(
					I2oMsg_p->UserMsg.IopNum)) == NULL)
			{
				/* No memory (or no IOP mfa) */

				i2oPtBufferFree(mp1);
				mp->b_datap->db_type = M_IOCNAK;
				mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

				iocbp->ioc_error = ENOMEM;
				qreply(qp, mp);
			
				break;
			}

			/* Park the message buffer for later. Its a bad
			 * idea to free it and get another because it contains
			 * our iocd etc.
			 * In fact allocate a message block for the reply
			 * now, this will save time (and be less messy if
			 * it fails) in the interrupt handler.
			 */

			I2oMsg_p->OurMsgBlk = mp;
			I2oMsg_p->WriteQueue = qp;
			
			mp2 = allocb(I2oMsg_p->UserMsg.ReplyLength, BPRI_MED);

			if(mp2 == NULL)
			{
				/* Gimme a break..its run out of memory now..*/

				i2oPtBufferFree(mp1);
	
				mp->b_datap->db_type = M_IOCNAK;
				mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);
				iocbp->ioc_error = ENOMEM;
				qreply(qp, mp);

				break;
			}	

			linkb(mp, mp2);

			I2oMsg_p->OurMsgBlk = mp;

			/* Copy the users prototype message into the message
			 * frame and fix up the special bits.
			 */
			
			i2oPtDataCopy(mp1->b_cont, (char *)MsgFrame_p, 
					I2oMsg_p->UserMsg.MessageLength);

			SGLOffset =
			   (I2oMsg_p->NumSGL)?I2oMsg_p->UserMsg.MessageLength:0;

			MsgFrame_p->Header.MessageSize = (
				I2oMsg_p->UserMsg.MessageLength +
			    sizeof(I2O_SGE_SIMPLE_ELEMENT)*I2oMsg_p->NumSGL)/4;
			MsgFrame_p->Header.VersionOffset = 
				(SGLOffset/4 << 4) | I2O_VERSION_11;
			MsgFrame_p->Header.InitiatorAddress = I2O_HOST_TID;
			MsgFrame_p->Header.MsgFlags = 0;
			MsgFrame_p->Header.InitiatorContext =
							 (U32)i2oPtCallBack;
			MsgFrame_p->TransactionContext = (U32)mp1;

			/* Fill in the SGLs */

			ssg = (PI2O_SGE_SIMPLE_ELEMENT)
					((char *)MsgFrame_p + SGLOffset);

			for(count = 0, i = I2oMsg_p->NumSGL; i > 0 ;
								 count++, i--)
			{
			
				ssg->FlagsCount.Flags =
					I2O_SGL_FLAGS_END_OF_BUFFER |
					I2O_SGL_FLAGS_CONTEXT32_NULL |
					I2O_SGL_FLAGS_SIMPLE_ADDRESS_ELEMENT;
				if(i == 1)
				{
					ssg->FlagsCount.Flags |= 
						I2O_SGL_FLAGS_LAST_ELEMENT;
				}
				if((I2oMsg_p->UserMsg.Data[count].Flags &
					I2O_PT_DATA_WRITE) == I2O_PT_DATA_WRITE)
				{
					ssg->FlagsCount.Flags 
							|= I2O_SGL_FLAGS_DIR;
				}
   				ssg->FlagsCount.Count = 
					I2oMsg_p->UserMsg.Data[count].Length;
				ssg->PhysicalAddress = 
					(U32) OSMMsgVtoP((U32*)
						I2oMsg_p->KernelAddress[count]);
				ssg++;	
			}

			/* Send the message off to the hardware */
 
			I2ODBG16("PTOSM OSMMsgSend");

			if(OSMMsgSend(I2oMsg_p->UserMsg.IopNum,
			 	(I2O_MESSAGE_FRAME*) MsgFrame_p) != SUCCESS)
			{
				i2oPtBufferFree(mp1);
				mp->b_datap->db_type = M_IOCNAK;
				mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

				iocbp->ioc_error = ENOMEM;
				qreply(qp, mp);
			
				break;	
			}

			break;

		/* End of case I2O_PT_GET_DB3: */

		case I2O_PT_SEND_REPLY:

			I2ODBG16("PTOSM case I2O_PT_SEND_REPLY");

			/* The the IOPs reply is now in a buffer in user space
			 * Now any reply SGL buffers need to be copied into
			 * user space.
			 * As the buffers were allocated by us and are due
			 * to be freed on completion we can safely wrap a
			 * mblk_t structure around them and send them up stream
			 * where they will be freed after use. (Note that they
			 * don't actually get freed because we supply a dummy
			 * routine - i2oPtBufferFree takes care of it later
			 */

			if(I2oMsg_p->NumSGL == 0)
			{
				/* There were no SGLs - we're done */

				goto case_I2O_PT_ALL_DONE;
			}

			I2oMsg_p->State = I2O_PT_SEND_DB;

			/* Fall through to next I2O_PT_SEND_DB */

		case I2O_PT_SEND_DB:
		case I2O_PT_SEND_DB0:
		case I2O_PT_SEND_DB1:
		case I2O_PT_SEND_DB2:

		case_I2O_PT_SEND_DB:	/* case_I etc. is correct, jump label */

			switch(I2oMsg_p->State)
			{
			case I2O_PT_SEND_DB:

				I2ODBG16("PTOSM case I2O_PT_SEND_DB");

				NextState = I2O_PT_SEND_DB0;
				count = 0;
				break;

			case I2O_PT_SEND_DB0:

				I2ODBG16("PTOSM case I2O_PT_SEND_DB0");

				NextState = I2O_PT_SEND_DB1;
				count = 1;
				break;

			case I2O_PT_SEND_DB1:

				I2ODBG16("PTOSM case I2O_PT_SEND_DB1");

				NextState = I2O_PT_SEND_DB2;
				count = 2;
				break;

			case I2O_PT_SEND_DB2:

				I2ODBG16("PTOSM case I2O_PT_SEND_DB2");

				NextState = I2O_PT_ALL_DONE;
				count = 3;
				break;
			}

			if((I2oMsg_p->UserMsg.Data[count].Flags &
						 I2O_PT_DATA_MASK) == 0)
			{
				/* All the SGLs are done - all finished */

				goto case_I2O_PT_ALL_DONE;
			}

			I2oMsg_p->State = NextState;

			/* Get a buffer and copy the data to it */

			if((I2oMsg_p->UserMsg.Data[count].Flags &
				 I2O_PT_DATA_READ) == I2O_PT_DATA_READ)
			{
				mp2 = allocb(
					I2oMsg_p->UserMsg.Data[count].Length,
					BPRI_MED);

				if(mp2 == NULL)
				{
					i2oPtBufferFree(mp1);
					mp->b_datap->db_type = M_IOCNAK;
					mp->b_wptr =
					     mp->b_rptr + sizeof(struct iocblk);
	
					iocbp->ioc_error = ENOMEM;
					qreply(qp, mp);
			
					break;	
				}

				bcopy( I2oMsg_p->KernelAddress[count],
					mp2->b_rptr,
					I2oMsg_p->UserMsg.Data[count].Length);

				mp2->b_wptr +=
					 I2oMsg_p->UserMsg.Data[count].Length;

				/* Build a M_COPYOUT message with mp2 linked */

				linkb(mp, mp2);

				mp->b_datap->db_type = M_COPYOUT;

				mp->b_wptr = mp->b_rptr+sizeof(struct copyreq);

				cqp = (struct copyreq *)mp->b_rptr;

				cqp->cq_size = 
					I2oMsg_p->UserMsg.Data[count].Length;
				cqp->cq_addr =
					I2oMsg_p->UserMsg.Data[count].Data;
				cqp->cq_flag = 0;
			
				cqp->cq_private = mp1;

				I2ODBG16("PTOSM M_COPYOUT for SGL buffer");

				qreply(qp, mp);

				break;
				
			}
			else
			{
				/* Must have been a write buffer, the
				 * State variable has already been set to
				 * point to the NextState, so try this case
				 * again
				 */
				if(NextState != I2O_PT_ALL_DONE)	
					goto case_I2O_PT_SEND_DB;

			}


		/* End of case I2O_PT_SEND_DBx - if its got to here fall
		 * through into case I2O_PT_ALL_DONE
		 */										
		case I2O_PT_ALL_DONE:

		case_I2O_PT_ALL_DONE:	/* case_I etc. is correct, jump label */

			I2ODBG16("PTOSM case I2O_PT_ALL_DONE");

			/* All that remains is to free all the buffers that
			 * were created and send a M_IOCACK upstream
			 */

			i2oPtBufferFree(mp1);

			if(mp->b_cont)
			{
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}

			mp->b_datap->db_type = M_IOCACK;
			mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

			iocbp->ioc_error = 0;
			iocbp->ioc_count = 0;
			iocbp->ioc_rval = 0;

			qreply(qp, mp);

			I2ODBG16("PTOSM M_IOCACK");

			break;

		/* End of case I2O_PT_ALL_DONE */


		default:

			/* I don't believe that this can happen - ignore it */

			I2ODBG16("PTOSM default:");
	
			break;

		}	/* End of switch (I2oMsg_p->State) */

		break;

	/* End of case I2O_PT_I2O_PT_MSGTFR */
	
	
	case I2O_PT_NUMIOPS:

		I2ODBG16("PTOSM case I2O_PT_NUMIOPS entered");

		/* Check if the M_COPYOUT succeeded, M_IOCACK if it did */

		if(crp->cp_rval == 0)
		{
			
			if(mp->b_cont)
			{
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}

			mp->b_datap->db_type = M_IOCACK;
			mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

			iocbp->ioc_error = 0;
			iocbp->ioc_count = 0;
			iocbp->ioc_rval = 0;

			qreply(qp, mp);
		}
		else
		{
			/* Simply free the message, no need for M_IOCNAK */

			freemsg(mp);
		}

		break;

	/* End of case I2O_PT_NUMIOPS */
	
	default:

	/* This is really bad - its one of those things "that can't happen"
	 * We should only get M_IOCDATA messages in response to M_COPYIN
	 * or M_COPYOUT that we started. Oh well, just free it up and keep
	 * quiet.
	 * (We can't tear down the state information and data buffers for 
	 * several reasons - like the DMA may be in progress, the state
	 * addresses may not be valid, we may get a valid M_IOCDATA...)
	 */

		freemsg(mp->b_cont);
		mp->b_cont = NULL;
		mp->b_datap->db_type = M_IOCNAK;
		mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);

		iocbp->ioc_error = EINVAL;
		qreply(qp, mp);
			
		break;

		/* End of default: */

	} /* End of switch (crp->cp_cmd) */	

}

/*****************************************************************************
NAME: i2oPtCallBack

SUMMARY: This is where we come in response to a IOP reply

DESCRIPTION:
		The IOP reply contains 2 private fields, the InitiatorContext
		and the TransactionContext. The first on was tha address of
		routine (which is why we're here) and the second is the
		address of the mblk_t structure where al the state information
		is saved.

		Recover the state and also the reply mblk_t that was saved.
		Copy in the reply frame data and send it upstream as a 
		M_COPYOUT aimed at the users replay buffer.

		Note that this runs in interrupt context.
	 
RETURNS: Nothing - a qreply is sent back up stream

*****************************************************************************/

STATIC void
i2oPtCallBack(unsigned char IOPNum, I2O_MESSAGE_FRAME *reply)
{

I2O_SINGLE_REPLY_MESSAGE_FRAME *ReplyFrame;
I2O_MESSAGE_FRAME	*PreservedMsg;
I2OptMsg_t 		*I2oMsg_p;
mblk_t			*mp, *mp1, *mp2;
struct copyreq 		*cqp;


	/*  Note: This routine is running in interrupt context */

	/* Recover our world ... */

	ReplyFrame = (I2O_SINGLE_REPLY_MESSAGE_FRAME *)reply;
      	mp1 = (mblk_t *)ReplyFrame->TransactionContext;
      	I2oMsg_p = (I2OptMsg_t *)mp1->b_rptr;
	mp = I2oMsg_p->OurMsgBlk;
	

	if((reply->MsgFlags & I2O_MESSAGE_FLAGS_FAIL))
	{
     		/* the transport of the message failed
		 * The reply is valid, so the ioctl itself did not fail
		 * Simply M_COPYOUT the reply then M_IOCACK
		 */
		I2oMsg_p->State = I2O_PT_ALL_DONE;


		/* If a preserved message is attached we must free it here */

		PreservedMsg = OSMMsgMfaConvert(IOPNum, 
		    ((I2O_FAILURE_REPLY_MESSAGE_FRAME *)reply)->PreservedMFA);

		if(PreservedMsg != NULL)
		{
			i2oOSMSendNop(IOPNum, PreservedMsg);
		}
	}
	else
	{
		I2oMsg_p->State = I2O_PT_SEND_REPLY;
	}

	/* We carefully stashed a reply buffer on mp->b_cont */

	mp2 = mp->b_cont;
	mp2->b_wptr = mp2->b_datap->db_base;
	mp2->b_rptr = mp2->b_datap->db_base;

	/* Copy data and send the reply buffer back to the IOP for re-use */

	bcopy(reply,  mp2->b_wptr, I2oMsg_p->UserMsg.ReplyLength);
	mp2->b_wptr += I2oMsg_p->UserMsg.ReplyLength;
	mp2->b_datap->db_type = M_DATA;

	OSMMsgFreeRepBuf(IOPNum, reply);

	/* Set up rest of mp as COPYOUT message */	

	mp->b_datap->db_type = M_COPYOUT;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

	cqp = (struct copyreq *)mp->b_rptr;

	cqp->cq_size = I2oMsg_p->UserMsg.ReplyLength;
	cqp->cq_addr = I2oMsg_p->UserMsg.Reply;
	cqp->cq_flag = 0;

	cqp->cq_private = mp1;

	/* Send it upstream, stream head will respond with M_IOCDATA */

	I2ODBG16("PTOSM M_COPYOUT for reply buffer");

	qreply(I2oMsg_p->WriteQueue, mp);
	
	return;
}

/*****************************************************************************
NAME: i2oPtDataCopy

SUMMARY: Copies specified amount of data from a mblk_t linked message list
	 into a buffer.

DESCRIPTION:

RETURNS: Nothing

*****************************************************************************/

STATIC void
i2oPtDataCopy(mblk_t *mp, char *dst, unsigned int count)
{

	while(mp != (mblk_t *)NULL && count > 0)
	{
		if(mp->b_rptr >= mp->b_wptr)
		{
			mp=mp->b_cont;
		}
		else
		{
			*dst++ = *mp->b_rptr++;
			count--;
		}
	}
}

/*****************************************************************************
NAME: i2oPtBufferFree

SUMMARY: Frees all memory attached to the mblk_t that holds the state
	 information.

DESCRIPTION: The mblk_t must hold a I2oMsg_t block. All associated kernel
	     buffers are freed. (Message, SGL buffers etc.)
	 
RETURNS: Nothing

*****************************************************************************/

STATIC void
i2oPtBufferFree(mblk_t *mp)
{
I2OptMsg_t	*I2oMsg_p;
int		count;

	I2oMsg_p = (I2OptMsg_t *)mp->b_rptr;

	for( count = 0; count < MAX_PT_SGL_BUFFERS ; count++)
	{
		if(I2oMsg_p->KernelAddress[count])
		{
			kmem_free(I2oMsg_p->KernelAddress[count],
					I2oMsg_p->UserMsg.Data[count].Length);
		}
	}

	freemsg(mp);
}

/*****************************************************************************
NAME: i2oOSMSendNop
SUMMARY: Send a UTIL_NOP message in the provided message frame

DESCRIPTION:  This function is used to free up a request message buffer that
the OSM has but doesn't need. It sends a NOP to the Iop using the message frame
that was passed as a parameter.

RETURNS: nothing
*****************************************************************************/
STATIC void
i2oOSMSendNop
  (
    U8                   IopNum,        /* Iop to send to */
    I2O_MESSAGE_FRAME   *MsgFrame       /* message frame to use */
  )
{
  MsgFrame->VersionOffset    = I2O_VERSION_11;
  MsgFrame->MsgFlags         = 0;
  MsgFrame->MessageSize      = sizeof(I2O_UTIL_NOP_MESSAGE) / 4;
  MsgFrame->TargetAddress    = 0; /* send it to the executive */
  MsgFrame->InitiatorAddress = I2O_HOST_TID;
  MsgFrame->Function         = I2O_UTIL_NOP;
  MsgFrame->InitiatorContext = 0;

  /*
   * i2oOSMSendNop was probably called because of an error condition so if this
   * send fails there is not much we can do.
   */
  (void)OSMMsgSend(IopNum, MsgFrame);

  return;
}

/*****************************************************************************
NAME: i2oPtDummyFree

SUMMARY: Free routine used by freeb() to free buffers "created" by esballoc()

DESCRIPTION: Doesn't actually do anything, all buffers are freed by
	     i2oPtBufferFree().

RETURNS: Nothing

*****************************************************************************/


STATIC void 
i2oPtDummyFree ()
{
	return;
}




/* STANDALONE is a #define that is used to build a version of the pass through
 * OSM that runs without actually sending anything to the IOPs. It is intended
 * to be used to test the OSMs logic.
 */

#ifdef STANDALONE

/* In standalone mode no IOP transfers actually take place. These test routines
 * loop back the requests, allowing all "shapes" of message to be tested 
 * through the pass through interface.
 */

/* Init has nothing to do except "invent" how many IOPs we have */

PUBLIC int
i2opt_init (void)
{
  	/*
   	 * identify ourselves
   	 */

	i2oPtNumIops = 3;

	return 0;
}

STATIC I2O_MESSAGE_FRAME*
OSMMsgGetInboundBuf(U8 Iop)
{
	return(kmem_alloc(INBOUND_MSG_FRAME_SIZE, KM_SLEEP));
}

/* Start has only to return without error */

PUBLIC int
i2opt_start (void)
{
	return 0;
}

/* OSMMsgAlloc simply kmem_allocs a message frame */

I2O_MESSAGE_FRAME*
OSMMsgAlloc
 (
   U8 iop                       /* IOP from which a buffer is needed */
 )

{
	return(kmem_alloc(INBOUND_MSG_FRAME_SIZE, KM_SLEEP));


}

/* OSMMsgSend is a little more complicated.
 * 1) Creates a reply frame and fills it with ascending numbers
 * 2) Next goes down the SGLs and fills any "read" buffers with ascending
 *    data pattern
 * 3) Calls i2oPtCallBack so that PT OSM thinks I/O has completed.
 * 
 * The use of the ascending pattern allows us to check how many bytes have
 * been transferred into user space from user space. If the user buffers
 * are alloced longer than the declared length then we can also check for
 * data overruns.
 */

S8
OSMMsgSend
 (
   U8                 iop,      /* IOP to send to */
   I2O_MESSAGE_FRAME* msg       /* message payload */
 )
{

unsigned char		*reply;
unsigned int		i, count;
mblk_t			*mp1;
I2OptMsg_t		*I2oMsg_p;
I2O_SGE_SIMPLE_ELEMENT	*ssg;
I2O_PT_MSG		*MsgFrame_p;
unsigned char		*ptr;
int			SGLOffset;


	/* First create a reply frame */

	MsgFrame_p = (I2O_PT_MSG *)msg;

	reply = kmem_alloc(OUTBOUND_FRAME_SIZE, KM_SLEEP);

	if(reply == NULL)
	{
		return ENOMEM;
	}

	/* Fill in reply frame */

	for(i = 0; i < OUTBOUND_FRAME_SIZE; i++)
	{
		*(reply + i) = (unsigned char)(i % 256);
	}


	/* Make sure that the context etc. are copied over */
	
	bcopy(MsgFrame_p, reply, sizeof(I2O_PT_MSG));

	/* Attempt to decode the message enough to discover if there are
	 * SGLs that need to be filled in
	 */

	mp1 = (mblk_t *)(MsgFrame_p->TransactionContext);
	I2oMsg_p = (I2OptMsg_t *)mp1->b_rptr;

	SGLOffset = ((MsgFrame_p->Header.VersionOffset & 0xF0) >> 2); 

	if(SGLOffset != 0)
	{
		ssg = (PI2O_SGE_SIMPLE_ELEMENT)((char *)MsgFrame_p + SGLOffset);

		for(count = 0; count < MAX_PT_SGL_BUFFERS  ; count++)
		{
			if((ssg->FlagsCount.Flags & I2O_SGL_FLAGS_DIR) ==
				I2O_SGL_FLAGS_DIR)
			{
				ptr = I2oMsg_p->KernelAddress[count];
				for(i = 0; i < ssg->FlagsCount.Count; i++)
					*ptr++ = (unsigned char)(i % 256);

			}

			if((ssg->FlagsCount.Flags & I2O_SGL_FLAGS_LAST_ELEMENT)
					== I2O_SGL_FLAGS_LAST_ELEMENT)
				break;

			ssg++;
		}
	}

	i2oPtCallBack(iop, (I2O_MESSAGE_FRAME *)reply);

	kmem_free(msg, INBOUND_MSG_FRAME_SIZE);

	return 0;
}

/* Simply free the buffer */

S8
OSMMsgFreeRepBuf
 (
   U8                 iop,      /* IOP # */
   I2O_MESSAGE_FRAME* reply     /* message buffer address */
 )
{
	kmem_free(reply, OUTBOUND_FRAME_SIZE);
	return 0;
}

#endif 	/* STANDALONE */
