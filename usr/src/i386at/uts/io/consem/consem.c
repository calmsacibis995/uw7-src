#ident	"@(#)consem.c	1.4"
#ident	"$Header$"

/* 
 * X Window Console Emulator
 * pushable streams module
 */
#include <util/param.h>
#include <util/types.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <proc/cred.h>
#include <io/termios.h>
#include <io/jioctl.h>
#include <io/strtty.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <io/kd/kd.h>
#include <io/ws/vt.h>
#include <io/consem/consem.h>
#include <io/ddi.h>

#ifdef DEBUG
STATIC int csem_debug = 0;
#define DEBUG1(a)	 if (csem_debug) printf a
#define DEBUG2(a)	 if (csem_debug > 1) printf a
#define DEBUG4(a)	 if (csem_debug > 3) printf a
STATIC void csemprt(int);
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG4(a)
#endif /* DEBUG */

extern int csem_cnt;
extern int csem_tab_siz;
extern struct csem csem[];
extern struct csem_esc_t csem_tab[];

STATIC int consemopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int consemclose(queue_t *, int, cred_t *);
STATIC int consemin(queue_t *, mblk_t *);
STATIC int consemout(queue_t *, mblk_t *);

STATIC void consemioc(queue_t *, mblk_t *);
STATIC void csem_tone(unsigned short, unsigned short);
STATIC void csem_toneoff(void);

extern nulldev(void);

STATIC struct module_info consemmiinfo = { 
/*       ID       Name   minpac maxpac hiwat lowat */
	0x00ce, "consem",  0,   512, 300,  100 };

STATIC struct module_info consemmoinfo = { 
	0x00ce, "consem",  0,   512, 300,  200 };

STATIC struct qinit consemrinit = {
	consemin, NULL, consemopen, consemclose, NULL, &consemmiinfo, NULL};

STATIC struct qinit consemwinit = {
	consemout, NULL, consemopen, consemclose, NULL, &consemmoinfo, NULL};

struct streamtab cseminfo = { 
	&consemrinit, &consemwinit };

int csemdevflag = 0;

/*
 * STATIC void csem_tout(void)
 *
 * Calling/Exit State:
 */
STATIC void 
csem_tout(void)
{
	struct csem *csp;	/* Pointer to a conseme structure */
	int flag=0;

	DEBUG1 (("consem timeout\n"));
	for ( csp = csem; csp <= &csem[csem_cnt-1]; csp++){
		if ( csp->state&WAITING){
			csp->iocp->ioc_count=0;
			csp->iocp->ioc_error=ETIME;
			csp->c_mblk->b_datap->db_type = M_IOCNAK;
			csp->state &= ~WAITING;
			qreply(csp->c_q, csp->c_mblk);
			flag++;
			break;
		}
	}
#ifdef DEBUG
	if (flag==0)
		DEBUG1 (("consem: NOTHING to timeout\n")); 
#endif /* DEBUG */
	return;
}

/*
 * STATIC int 
 * consemopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	open routine
 *
 * Calling/Exit State:
 */
/*ARGSUSED*/
STATIC int
consemopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	struct csem *csp;	/* Pointer to a conseme structure */

	DEBUG1 (("consemopen called\n"));

	if (sflag != MODOPEN) {
		return (EINVAL);
	}
	if (q->q_ptr != NULL) 	/* already attached */
		return(0);
	
	for ( csp = csem; csp->state&CS_OPEN; csp++)
		if ( csp >= &csem[csem_cnt-1]) {
			DEBUG1(("No consem structures.\n"));
			return( ENODEV);
		}
	csp->state = CS_OPEN;
	q->q_ptr 	= (caddr_t)csp;
	WR(q)->q_ptr 	= (caddr_t)csp;
	return(0);

}

/*
 * STATIC int consemclose(queue_t *q, int cflag, cred_t *crp)
 *	close routine
 *
 * Calling/Exit State:
 */
/*ARGSUSED*/
STATIC int
consemclose(queue_t *q, int cflag, cred_t *crp)
{
	struct csem *csp = (struct csem *)q->q_ptr;

	DEBUG1 (("consemclose\n")); 
	if ( csp->state&WAITING){
		untimeout(csp->to_id);
		freemsg(csp->c_mblk);
	}
	csp->state = 0;
	q->q_ptr = NULL;
	return(0);
}


/*
 * STATIC int consemin(queue_t *q, mblk_t *mp)
 * 	put procedure for input from driver end of stream (read queue)
 *
 * Calling/Exit State:
 */
STATIC int
consemin(queue_t *q, mblk_t *mp)
{

	struct copyreq *send_buf_p;
	struct csem *csp;
	mblk_t *tmp;
	struct iocblk *iocp;
	int outsiz;
	char pass_fail;

	DEBUG4 (("consemin:\n"));
	csp = (struct csem *)q->q_ptr;

	if (mp->b_datap->db_type != M_DATA) {
		DEBUG1(("consemin: Non DATA default\n"));
		putnext(q, mp);
		return(0);
	}
	if ( csp->state&WAITING){
		untimeout(csp->to_id);
		csp->state &= ~WAITING;
		if ((mp->b_rptr[0] != 033)||(mp->b_rptr[1] != '@')||(mp->b_rptr[2] != '3')) {
			DEBUG4(("Not xterm response %x %x %x %x \n",
				mp->b_rptr[0], mp->b_rptr[1], mp->b_rptr[2],
				mp->b_rptr[3]));
			goto ce_nxt;
		}
		if ( csem_tab[csp->indx].type == CSEM_R){
			DEBUG1(("consemin: Sending rval response.\n"));
			csp->iocp->ioc_count=0;
			if( (char)*(mp->b_rptr+4)){	/* SC non-zero */
				csp->iocp->ioc_rval = -1;
				csp->c_mblk->b_datap->db_type = M_IOCNAK;
				csp->iocp->ioc_error=EINVAL;
			}
			else{
				bcopy ((caddr_t)(mp->b_rptr+5),
				       (caddr_t)(&csp->iocp->ioc_rval),4);
				csp->c_mblk->b_datap->db_type = M_IOCACK;
				csp->iocp->ioc_error=0;
			}
			freemsg(mp);
			putnext(q, csp->c_mblk);
			return(0);
		}
		/* SCEM_O  */
		else if ( csem_tab[csp->indx].type == CSEM_O){
			if( (char)*(mp->b_rptr+4)){	/* SC non-zero */
				csp->c_mblk->b_datap->db_type = M_IOCNAK;
/* joeh */			csp->iocp->ioc_error=EINVAL;
				csp->iocp->ioc_rval = -1;
				freemsg(mp);
				qreply( csp->c_q, csp->c_mblk);
				return(0);
			}
			DEBUG1 ((" consemin: Queing COPYOUT response.\n"));
			outsiz=csem_tab[csp->indx].b_out;
			DEBUG1(("index= %d, outsiz= %d, ioc_count= %d\n", 
				csp->indx, outsiz, csp->iocp->ioc_count));
			if ( csp->iocp->ioc_count == TRANSPARENT) {
				if ((tmp = allocb(outsiz, BPRI_MED)) == NULL)  {
					csp->c_mblk->b_datap->db_type = M_IOCNAK;
					csp->iocp->ioc_error = EAGAIN;
					DEBUG1(("EAGAIN case 1\n"));
					csp->iocp->ioc_rval = -1;
					freemsg(mp);
					qreply( csp->c_q, csp->c_mblk);
					return(0);
				}
				/* LINTED pointer alignment */
				send_buf_p=(struct copyreq *)csp->c_mblk->b_rptr;
				/* LINTED pointer alignment */
				send_buf_p->cq_addr = (caddr_t)(*(long *)(csp->c_mblk->b_cont->b_rptr));
				freemsg( csp->c_mblk->b_cont);
				csp->c_mblk->b_cont = tmp;
				bcopy((caddr_t)(mp->b_rptr+5),
				      (caddr_t)(tmp->b_rptr), outsiz);
				tmp->b_wptr += outsiz;
				send_buf_p->cq_private = NULL;
				send_buf_p->cq_flag = 0;
				send_buf_p->cq_size=outsiz;
				DEBUG1(("COPYOUT outsize= %d\n", outsiz));
				csp->c_mblk->b_datap->db_type = M_COPYOUT;
				freemsg(mp);
				qreply(csp->c_q, csp->c_mblk);
			}
			else { /* This can't happen */
				csp->c_mblk->b_datap->db_type = M_IOCNAK;
				csp->iocp->ioc_count = 0;
				csp->iocp->ioc_error = EAGAIN;
				DEBUG1(("EAGAIN case 2\n"));
				csp->iocp->ioc_rval = -1;
				freemsg(mp);
				qreply(csp->c_q, csp->c_mblk);
			}
			return(0);
		}
		/* CSEM_B */
		else if ( csem_tab[csp->indx].type == CSEM_B){
			pass_fail=(char)*(mp->b_rptr+4);
			outsiz=csem_tab[csp->indx].b_out;
			DEBUG1(("index= %d, outsiz= %d\n", csp->indx, outsiz));
			if ((tmp = allocb(outsiz, BPRI_MED)) == NULL)  {
				DEBUG1(("CSEM_B: 1st buffer alloc failed.\n"));
				putnext(q, mp);
				return(0);
			}
			if( pass_fail ){	/* SC non-zero */
				DEBUG1(("CSEM_B: return code failed\n"));
				/* LINTED pointer alignment */
				iocp = (struct iocblk *)tmp->b_rptr;
/* #ifdef SVR4: this is needed only on SVR4 systems */
				if ((iocp->ioc_cr = (cred_t *) allocb(sizeof(cred_t), BPRI_MED)) == NULL) {
					DEBUG1(("CSEM_B: 2nd buffer alloc failed.\n"));
					putnext(q, mp);
					return(0);
				}
/* #endif /* SVR4 */
				iocp->ioc_cmd= csp->ioc_cmd;
				iocp->ioc_uid= csp->ioctl_uid;
				iocp->ioc_gid= csp->ioctl_gid;
				iocp->ioc_id= csp->ioc_id;
				iocp->ioc_rval = -1;
				iocp->ioc_error=EINVAL;
				tmp->b_datap->db_type = M_IOCNAK;
				freemsg(mp);
				putnext(q, tmp);
				return(0);
			}
			DEBUG1(("consemin:CSEM_B: Queing COPYOUT response.\n"));
			/* LINTED pointer alignment */
			send_buf_p=(struct copyreq *)tmp->b_rptr;
			/* LINTED pointer alignment */
			send_buf_p->cq_addr = (caddr_t)(*(long *)(tmp->b_cont->b_rptr));
			bcopy((caddr_t)(mp->b_rptr+5),
			      (caddr_t)(tmp->b_cont->b_rptr), outsiz);
			tmp->b_cont->b_wptr += outsiz;
			send_buf_p->cq_cmd= csp->ioc_cmd;
/* #ifdef SVR4: this is needed only on SVR4 systems */
			if ((send_buf_p->cq_cr = (cred_t *) allocb(sizeof(cred_t), BPRI_MED)) == NULL) {
				DEBUG1(("CSEM_B: 3rd buffer alloc failed.\n"));
				putnext(q, mp);
				return(0);
			}
/* #endif /* SVR4 */
			send_buf_p->cq_uid= csp->ioctl_uid;
			send_buf_p->cq_gid= csp->ioctl_gid;
			send_buf_p->cq_id= csp->ioc_id;
			send_buf_p->cq_private = NULL;
			send_buf_p->cq_flag = 0;
			send_buf_p->cq_size=outsiz;
			DEBUG1(("COPYOUT outsize= %d\n", outsiz));
			tmp->b_datap->db_type = M_COPYOUT;
			csp->state |= CO_REQ;
			freemsg(mp);
			freemsg(csp->c_mblk);
			putnext(q, tmp);
			return(0);
		}
		else if ((csem_tab[csp->indx].type == CSEM_I) ||
			  (csem_tab[csp->indx].type == CSEM_N)) {
			/* joeh is buffer big enough here? */
			DEBUG1 (("consemin: ACKING set or null ioctl.\n"));
			pass_fail=(char)*(mp->b_rptr+4);
			/* LINTED pointer alignment */
			iocp = (struct iocblk *)mp->b_rptr;
/* #ifdef SVR4: this is needed only on SVR4 systems */
			if ((iocp->ioc_cr = (cred_t *) allocb(sizeof(cred_t), 
				BPRI_MED)) == NULL) {
				iocp->ioc_rval = -1;
				mp->b_datap->db_type = M_IOCNAK;
	  			iocp->ioc_error=EINVAL;
			}
/* #endif /* SVR4 */
			iocp->ioc_cmd= csp->ioc_cmd;
			iocp->ioc_uid= csp->ioctl_uid;
			iocp->ioc_gid= csp->ioctl_gid;
			iocp->ioc_id= csp->ioc_id;
			if( pass_fail ){	/* SC non-zero */
				iocp->ioc_rval = -1;
				mp->b_datap->db_type = M_IOCNAK;
/* joeh */			iocp->ioc_error=EINVAL;
			}
			else{
				iocp->ioc_rval = 0;
				mp->b_datap->db_type = M_IOCACK;
				iocp->ioc_error=0;
			}
			iocp->ioc_count=0;
			freemsg(csp->c_mblk);
			putnext(q, mp);
			return(0);
		}
#ifdef DEBUG
		else
			DEBUG1(("consemin: ERROR.\n"));
#endif /* DEBUG */
	}

ce_nxt:	putnext(q, mp);
	return(0);
}

/*
 * STATIC int consemout(queue_t *q, mblk_t *mp)
 * 	output queue put procedure: 
 *
 * Calling/Exit State:
 */
STATIC int
consemout(queue_t *q, mblk_t *mp)
{
	struct csem *csp;
	struct iocblk *iocp;
	mblk_t *tmp;
	int	insiz, tone;
	struct copyresp *resp;
	int 	cmd;

	DEBUG4(("consemout:\n"));
	csp = (struct csem *)q->q_ptr;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	cmd  = iocp->ioc_cmd;

	switch (mp->b_datap->db_type) {

	case M_IOCTL:
		DEBUG1(("consemout: M_IOCTL\n"));

#ifdef DEBUG
		csemprt (cmd);
#endif /* DEBUG */

#ifdef USL_OLD_MODESWITCH
       		if ((cmd & 0xffffff00) == USL_OLD_MODESWITCH)
            	     cmd = (cmd & ~IOCTYPE) | MODESWITCH;
#endif
		switch (cmd) {
/*		case PIO_KEYMAP:	Disabled	*/
		case PIO_SCRNMAP:
			if (iocp->ioc_uid){
				iocp->ioc_count=0;
				iocp->ioc_error=EACCES;
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				return(0);
			}	/* else fall through */
			/* FALLTHRU */
		case KDDISPTYPE:
		case KDGKBENT:
		case KDSKBENT:
		case KDGKBMODE:
		case KDSKBMODE:
		case GIO_ATTR:
		case GIO_COLOR:
/*		case GIO_KEYMAP:	Disabled	*/
		case GIO_STRMAP:
		case PIO_STRMAP:
		case GIO_SCRNMAP:
		case GETFKEY:
		case SETFKEY:
		case SW_B40x25:
		case SW_C40x25:
		case SW_B80x25:
		case SW_C80x25:
		case SW_EGAMONO80x25:
		case SW_ENHB40x25:
		case SW_ENHC40x25:
		case SW_ENHB80x25:
		case SW_ENHC80x25:
		case SW_ENHB80x43:
		case SW_ENHC80x43:
		case SW_MCAMODE:
		case O_SW_B40x25:
		case O_SW_C40x25:
		case O_SW_B80x25:
		case O_SW_C80x25:
		case O_SW_EGAMONO80x25:
		case O_SW_ENHB40x25:
		case O_SW_ENHC40x25:
		case O_SW_ENHB80x25:
		case O_SW_ENHC80x25:
		case O_SW_ENHB80x43:
		case O_SW_ENHC80x43:
		case O_SW_MCAMODE:
		case CONS_CURRENT:
		case CONS_GET:
		case TIOCVTNAME:
			consemioc(q, mp);
			return(0);

		case KDGETMODE:
			DEBUG1(("KDGETMODE case\n"));
			iocp->ioc_count=0;
			iocp->ioc_error=0;
			iocp->ioc_rval=KD_TEXT;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			return(0);

		case KDSETMODE:
		case KDSBORDER:
		case CGA_GET:
		case EGA_GET:
		case PGA_GET:
		case MCA_GET:
		case TIOCKBOF:
		case KBENABLED:
			iocp->ioc_count=0;
			iocp->ioc_error=EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return(0);
	
		case KDMKTONE:
			DEBUG1(("KDMKTONE case\n"));
			/* LINTED pointer alignment */
			tone = *(int *)mp->b_cont->b_rptr;
			csem_tone((tone & 0xffff), ((tone >> 16) & 0xffff));
			iocp->ioc_count=0;
			iocp->ioc_error=0;
			iocp->ioc_rval=0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			return(0);
	
		default:
			DEBUG1(("consem: default ioctl case\n"));
			putnext(q, mp);
			return(0);
		}
	case M_IOCDATA:
		/* LINTED pointer alignment */
		resp = (struct copyresp *)mp->b_rptr;
		if ( resp->cp_rval)  {
			freemsg( mp);		/* Just return on failure */
			return(0);
		}
		switch ( resp->cp_cmd) {
		case KDDISPTYPE: /* CSEM_O or CSEM_R Pocsessing */
		case KDGKBMODE:
		case GIO_KEYMAP:
		case GIO_STRMAP:
		case GIO_SCRNMAP:
		case TIOCVTNAME:
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			iocp->ioc_rval = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply( q, mp);
			return(0);

		case GETFKEY: 		/* CSEM_B Processing */
		case KDGKBENT:
			if ( csp->state&CO_REQ){
				iocp->ioc_error = 0;
				iocp->ioc_count = 0;
				iocp->ioc_rval = 0;
				mp->b_datap->db_type = M_IOCACK;
				csp->state &= ~CO_REQ;
				qreply( q, mp);
				return(0);
			}
			/* else: fall through and do COPYIN */
			/* FALLTHRU */
		case KDSKBENT:		/* CSEM_I (COPYIN) Pocsessing */
		case KDSKBMODE: 
		case PIO_KEYMAP:
		case PIO_STRMAP:
		case PIO_SCRNMAP:
		case SETFKEY:
			insiz=mp->b_cont->b_wptr - mp->b_cont->b_rptr;
#ifdef DEBUG
			DEBUG1(("COPYIN insize= %d\n", insiz));
			if (insiz != csem_tab[csp->indx].b_in)
				DEBUG1(("copyin siz err, sizin= %d, tab=%d, ioctl= %s\n", insiz, csem_tab[csp->indx].b_in, csem_tab[csp->indx]. name));
#endif /* DEBUG */
			if ((tmp=allocb(insiz+4, BPRI_HI)) == NULL ){
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				DEBUG1(("EAGAIN case 3\n"));
				qreply(q, mp);
				return(0);
			}
			tmp->b_rptr[0] = 033;	/* Escape */
			tmp->b_rptr[1] = '@';
			tmp->b_rptr[2] = '2';
			tmp->b_rptr[3] = csem_tab[csp->indx].esc_at;
			bcopy((caddr_t)(mp->b_cont->b_rptr),
			      (caddr_t)(tmp->b_rptr+4), insiz);
			tmp->b_wptr= tmp->b_rptr + 4 +insiz;
			tmp->b_datap->db_type = M_DATA;
			csp->c_q = q;
			csp->c_mblk=mp;
			csp->ioc_cmd = resp->cp_cmd;
			csp->ioctl_uid = resp->cp_uid;
			csp->ioctl_gid = resp->cp_gid;
			csp->ioc_id = resp->cp_id;
			csp->state |= WAITING;
			csp->to_id=timeout(csem_tout , 0, 20*HZ);
			putnext(q, tmp);
			return(0);

		default:
			DEBUG1(("default consem: IOC_DATA case\n"));
			putnext(q, mp);
			return(0);
		}
	}

	putnext(q, mp);
	return(0);
}

/*
 * STATIC int consemioc(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 */
STATIC void
consemioc(queue_t *q, mblk_t *mp)
{
	struct csem *csp;
	struct iocblk *iocp;
	mblk_t *tmp;
	int i;

	csp = (struct csem *)q->q_ptr;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;

	/* Message must be of type M_IOCTL for this routine to be called.  */
	DEBUG1(("consemioc:iocp->ioc_cmd = %x\n", iocp->ioc_cmd));

	for (i=0; i < csem_tab_siz; i++){
		if (csem_tab[i].ioctl == iocp->ioc_cmd){
			DEBUG1(("consemioc: esc match, i=%d\n", i));
			if (csem_tab[i].type & CSEM_I){
				if ( iocp->ioc_count == TRANSPARENT) {
					struct copyreq *get_buf_p;
					/* LINTED pointer alignment */
					get_buf_p=(struct copyreq *)mp->b_rptr;
					get_buf_p->cq_private = NULL;
					get_buf_p->cq_flag = 0;
					get_buf_p->cq_size = csem_tab[i].b_in;
					/* LINTED pointer alignment */
					get_buf_p->cq_addr = (caddr_t) (*(long*)(mp->b_cont->b_rptr));
					freeb(mp->b_cont);
					mp->b_cont=NULL;
					DEBUG1(("consemioc: Queing COPYIN.\n"));
					mp->b_datap->db_type = M_COPYIN;
					csp->indx=i;
					csp->iocp=iocp;
					qreply( q, mp);
					return;
				}
				else{
					goto ce_err;
				}
			}
			else{	/* Not CSEM_I */
				if ((tmp=allocb(sizeof(struct csem_esc_t), BPRI_HI)) == NULL) {
					mp->b_datap->db_type = M_IOCNAK;
					iocp->ioc_error = EAGAIN;
					DEBUG1(("EAGAIN case 4\n"));
					qreply(q, mp);
					return;
				}
				tmp->b_rptr[0] = 033;	/* Escape octal code */
				tmp->b_rptr[1] = '@';
				tmp->b_rptr[2] = '2';
				tmp->b_rptr[3] = csem_tab[i].esc_at;
				tmp->b_wptr= tmp->b_rptr + 4;
				tmp->b_datap->db_type = M_DATA; /*change IOCTL to DATA*/
				csp->c_q = q;
				csp->c_mblk=mp;
				csp->iocp=iocp;
				csp->indx=i;
				csp->ioc_cmd = iocp->ioc_cmd;
				csp->ioctl_uid = iocp->ioc_uid;
				csp->ioctl_gid = iocp->ioc_gid;
				csp->ioc_id = iocp->ioc_id;
				csp->state |= WAITING;
				csp->to_id=timeout(csem_tout , 0, 20*HZ);
				putnext(q, tmp);
				return;
			}
		}
	}
	/* If we got here we could find ioc_cmd in table of esc codes */
	DEBUG1(("consemioc: esc match errror\n"));
ce_err:	iocp->ioc_count=0;
	iocp->ioc_error=EINVAL;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(q, mp);
	return;
}

#ifdef DEBUG

struct csemv{
	int cmd;
	char *str;
} csema[]={
	TCGETA,"TCGETA",
	TCSETA,"TCSETA",
	TCSETAW,"TCSETAW",
	TCSETAF,"TCSETAF",
	TCGETS,"TCGETS",
	TCSETS,"TCSETS",
	TCSETSW,"TCSETSW",
	TCSETSF,"TCSETSF",
	TCSBRK,"TCSBRK",
	TCXONC,"TCXONC",
	TCFLSH,"TCFLSH",
	TIOCSETP,"TIOCSETP",
	IOCTYPE,"IOCTYPE",
	JWINSIZE,"JWINSIZE",
	TIOCSWINSZ,"TIOCSWINSZ",
	TIOCGWINSZ,"TIOCGWINSZ",
	KDDISPTYPE,"KDDISPTYPE",
	KDGKBENT,"KDGKBENT",
	KDSKBENT,"KDSKBENT",
	KDGKBMODE,"KDGKBMODE",
	KDSKBMODE,"KDSKBMODE",
	KDSBORDER,"KDSBORDER",
	KDGETMODE,"KDGETMODE",
	KDSETMODE,"KDSETMODE",
	KDMKTONE,"KDMKTONE",
	GIO_ATTR,"GIO_ATTR",
	GIO_COLOR,"GIO_COLOR",
	GIO_KEYMAP, "GIO_KEYMAP",
	PIO_KEYMAP, "PIO_KEYMAP",
	GIO_STRMAP, "GIO_STRMAP",
	PIO_STRMAP, "PIO_STRMAP",
	GIO_SCRNMAP, "GIO_SCRNMAP",
	PIO_SCRNMAP, "PIO_SCRNMAP",
	GETFKEY, "GETFKEY",
	SETFKEY, "SETFKEY",
	SW_B40x25,"SW_B40x25",
	SW_C40x25, "SW_C40x25",
	SW_B80x25, "SW_B80x25",
	SW_C80x25, "SW_C80x25",
	SW_EGAMONO80x25, "SW_EGAMONO80x25",
	SW_ENHB40x25, "SW_ENHB40x25",
	SW_ENHC40x25, "SW_ENHC40x25",
	SW_ENHB80x25, "SW_ENHB80x25",
	SW_ENHC80x25, "SW_ENHC80x25",
	SW_ENHB80x43, "SW_ENHB80x43",
	SW_ENHC80x43, "SW_ENHC80x43",
	SW_MCAMODE, "SW_MCAMODE",
	CONS_CURRENT,"CONS_CURRENT",
	CONS_GET,"CONS_GET",
	CGA_GET,"CGA_GET",
	EGA_GET,"EGA_GET",
	PGA_GET,"PGA_GET",
	MCA_GET,"MCA_GET",
	KBENABLED,"KBENABLED",
	TIOCKBON,"TIOCKBON",
	TIOCKBOF,"TIOCKBOF",
	TIOCVTNAME,"TIOCVTNAME"
};

/*
 * STATIC void csemprt(int cmd)
 *	print out the command for debugging purpose.
 *
 * Calling/Exit State:
 */
STATIC void
csemprt(int cmd)
{
	int i, csem_tsiz;

	csem_tsiz = sizeof(csema)/sizeof(csema[0]);
	if (csem_debug > 1){
		cmn_err(CE_CONT, "consem: ioctl= %x, ", cmd);
		for (i=0; i<csem_tsiz; i++)
			if (csema[i].cmd == cmd)
				cmn_err(CE_CONT, "%s", csema[i].str);
		cmn_err(CE_CONT, "\n");
	}
}

#endif /* DEBUG */

int csem_toneon;

/*
 * STATIC void csem_tone(unsigned short freq, unsigned short length) 
 * 	Sound generator.  This plays a tone at frequency freq
 * 	for length milliseconds.
 *
 * Calling/Exit State:
 */
STATIC void
csem_tone(unsigned short freq, unsigned short length)
{
	unsigned char	status;
	int		linhz;
	int		oldpri;

	if (freq == 0)
		return;
	linhz = (int) (((long) length * HZ) / 1000L);
	if (linhz == 0)
		return;
	oldpri = spltty();
	while (csem_toneon)
		sleep((caddr_t) &csem_toneon, TTOPRI);
	splx(oldpri);
	csem_toneon = 1; 
	/*
 	* set up timer mode and load initial value
 	*/
	outb(TIMERCR, T_CTLWORD);
	outb(TIMER2, freq & 0xff);
	outb(TIMER2, (freq>>8) & 0xff);
	/* 
 	* turn tone generator on
 	*/
	status = inb(TONE_CTL);
	status |= TONE_ON;
	outb(TONE_CTL, status);
	timeout(csem_toneoff, 0, linhz);
}


/*
 * STATIC void csem_toneoff(void)
 * 	Turn the sound generation off.
 *
 * Calling/Exit State:
 */
STATIC void 
csem_toneoff(void)
{
	unsigned char status;

	status = inb(TONE_CTL);
	status &= ~TONE_ON;
	outb(TONE_CTL, status);
	csem_toneon = 0;
	wakeup((caddr_t) &csem_toneon); 
}
