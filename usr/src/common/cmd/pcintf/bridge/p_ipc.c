#ident	"@(#)pcintf:bridge/p_ipc.c	1.1.1.4"
#include	"sccs.h"
SCCSID(@(#)p_ipc.c	6.7	LCC);	/* Modified: 4/1/92 16:13:30 */

/****************************************************************************

	Copyright (c) 1985 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

****************************************************************************/

#include "sysconfig.h"

#if !defined(NOIPC)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#include <malloc.h>
#include <memory.h>

#include <lmf.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "dossvr.h"
#include "flip.h"
#include "log.h"

#define SUCCESS 	0
#define FAIL 		(-1)
#define MSG		0		/* indicates msg type in table */
#define SEM		1		/* indicates semaphore type in tbl */
#define shortMSB	0x8000		/* most significant bit for short */
#define TBL_INCREMENT	16		/* table size is incremented by 16 */
#define FREE_ID		-1		/* msg & sem table's initial value */

long tmpLong;
short tmp;

LOCAL int	xlat_Out_ID	PROTO((int, ushort));
LOCAL int	xlat_In_ID	PROTO((short, ushort));
LOCAL int	find_id		PROTO((int, ushort));
LOCAL void	free_id		PROTO((short));

extern int	msgget		PROTO((key_t, int));
extern int	msgctl		PROTO((int, int, .../* struct msqid_ds * */));
extern int	msgsnd		PROTO((int, const void *, size_t, int));
extern int	msgrcv		PROTO((int, void *, size_t, long, int));
extern int	semget		PROTO((key_t, int, int));
extern int	semctl		PROTO((int, int, int, .../* union semun * */));
extern int	semop		PROTO((int, struct sembuf *, size_t));

extern int	flipBytes;	/* byte ordering flag */

/*	< Local Variables >	*/
struct table {
    ushort type;	/* MSG indicates msg, SEM indicates semaphore */
    int	id;		/* msg ID or semaphore ID */
};
static struct table *id_tbl = NULL;	/* pointer to array of msg/sem IDs */
static int max_tbl = 0;			/* maximum entries in msg/sem table */

/*
 * Extended Protocol Header for Locus Computing Corporation PC-Interface
 * Bridge. The information for doing IPC messages is contained in the
 * hdr.text field of the incoming packet. Overlay the following
 * structures to get at the information.
 */

struct pci_msgbuf {
	long	mtype;		/* message type */
	char	mtext[1];	/* message text */
};

struct pci_ipc_perm {
	ushort	uid;		/* owner's user id */
	ushort	gid;		/* owner's group id */
	ushort	cuid;		/* creator's user id */
	ushort	cgid;		/* creator's group id */
	ushort	mode;		/* access modes */
	ushort	seq;		/* slut usage sequence number */
	long	key;		/* key  (key_t) */
};

struct pci_msqid_ds {
	struct pci_ipc_perm	msg_perm;	/* operation perm struct */
	long			msg_stime;	/* last msgsnd time */
	long			msg_rtime;	/* last msgrcv time */
	long			msg_ctime;	/* last change time */
	ushort			msg_cbytes;	/* current # bytes on q */
	ushort			msg_qnum;	/* # of messages on q */
	ushort			msg_qbytes;	/* max # of bytes on q */
	ushort			msg_lspid;	/* pid of last msgsnd */
	ushort			msg_lrpid;	/* pid of last msgrcv */
};

struct pci_semid_ds {
	struct pci_ipc_perm	sem_perm;	/* operation perm struct */
	long			sem_otime;	/* last semop time */
	long			sem_ctime;	/* last change time */
	ushort			sem_nsems;	/* # of semaphores in set */
};


struct msgget_type {
	key_t			key;
	long			flag;
};

struct msgsnd_type {
        long			id;
	long			size;
	long			flag;
	struct pci_msgbuf	message;
};

struct msgrcv_type {
	long	 		id;
	long			size;
	long			flag;
	long			type;
	struct pci_msgbuf	message;
};

struct msgctl_type {
	long			id;
	long			command;
	struct pci_msqid_ds	buffer;
}; 

struct semget_type {
	key_t			key;
	long			number;
	long			flag;
};

struct semop_type {
	long			id;
	long			number;
	struct pci_sembuf	(*sops)[];
};

struct semctl_type {
	long			id;
	long			number;
	long			command;
	union {
	    long		val;
	    struct pci_semid_ds	dsbuf;
	    ushort		dsarr[1];
	} dsret;
};

/*
 * Commands used in the incoming packet to identify the control command.
 * These values match the values in the SVR3 <sys/ipc.h> header
 * file but differ from the SVR4 values (and may differ for other OSs as
 * well). 
 */
#define	P_IPC_RMID	0	/* remove identifier */
#define	P_IPC_SET	1	/* set options */
#define	P_IPC_STAT	2	/* get options */

/*
 * get a message queue id 
 */

void 
p_msgget( data , out )
char *data;
struct output *out;
{
	int rc;
	struct msgget_type *buf;

	buf = (struct msgget_type *)data;
	if (flipBytes) {
	    lsflipm(buf->key,tmpLong);
	    lsflipm(buf->flag,tmpLong);
	}
	log("msgget parameters: key %ld  flag %x\n", buf->key,buf->flag);
	do
	    rc = msgget( (key_t)buf->key, (int)buf->flag ); 
	while (rc == -1 && errno == EINTR);
	if (rc != -1) {
	    out->hdr.res = SUCCESS;
	    out->hdr.fdsc = (unsigned short)xlat_Out_ID(rc, MSG);
	   					 /* return msg queue id */
	    /* If xlat_Out_ID() returned a -1, then there is not any more
	     * space in the translation table. Remove the msg created and
	     * set the proper error message.
	     */
	    if (out->hdr.fdsc == -1) {
		msgctl(rc, IPC_RMID, (int)buf->flag);
		out->hdr.res = ENOSPC;
	    }
	} else {
	    out->hdr.res = (unsigned char)errno;
	    out->hdr.fdsc = -1;
	}
	/* has UNIX run out of msg queue IDs? */
	if (rc < 0 && ((int)buf->flag & IPC_CREAT) && errno == ENOSPC)
	    serious(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_IPC1",
		"Cannot create message queue ID, errno = %1\n"),
		"%d", errno));
	log("MSGGET Returned %d\n",rc);
}

/*
 * Send a message
 */

void
p_msgsnd( data , out )
char *data;
struct output *out;
{
    register int rc;
    struct msgsnd_type *buf;
    register int i;

	buf = (struct msgsnd_type *) data;
	if (flipBytes) {
	    lsflipm(buf->id,tmpLong);
	    lsflipm(buf->flag,tmpLong);
	    lsflipm(buf->size,tmpLong);
	    lsflipm(buf->message.mtype,tmpLong);
	}
	log("msgsnd paramters:\n");
	log("buf->id %d  buf->size %d  buf->flag %d\n",
	    buf->id ,buf->size ,buf->flag);
	log("          message: %s \n",buf->message.mtext); 

	rc = msgsnd(xlat_In_ID((short)buf->id, MSG),
					(struct msgbuf *)&buf->message,
					buf->size, (int)buf->flag);
	out->hdr.res = (unsigned char)((rc >= 0) ? SUCCESS : errno);
	out->hdr.fdsc = (unsigned short)rc; 
}

static char *msgsnd_buf = NULL;
static char *msgsnd_bp = NULL;
static int   msgsnd_len = 0;

void
p_msgsnd_new(data, data_len, out)
char *data;
int data_len;
struct output *out;
{
	struct msgsnd_type *buf;

	buf = (struct msgsnd_type *) data;
	msgsnd_len = buf->size;
	if (flipBytes)
	    lsflipm(msgsnd_len, tmpLong);
	msgsnd_len += sizeof (struct msgsnd_type);

	log("msgsnd_new: data_len = %d, size = %d\n", data_len, msgsnd_len);
	if (msgsnd_buf != NULL) {
		free(msgsnd_buf);
		msgsnd_buf = NULL;
	}
	if (msgsnd_len < data_len) {
		log("msgsnd_new: Too much data\n");
		out->hdr.res = E2BIG;
		return;
	}
	if ((msgsnd_buf = (char *)malloc(msgsnd_len)) == NULL) {
		log("msgsnd_new: Couldn't allocate buffer\n");
		out->hdr.res = EINVAL;
		return;
	}
	memcpy(msgsnd_buf, data, data_len);
	msgsnd_bp = &msgsnd_buf[data_len];
	out->hdr.res = SUCCESS;
}

void
p_msgsnd_ext(data, data_len, out)
char *data;
int data_len;
struct output *out;
{
	log("msgsnd_ext: data_len = %d\n", data_len);
	if (msgsnd_len < (msgsnd_bp - msgsnd_buf) + data_len) {
		log("msgsnd_ext: Too much data\n");
		out->hdr.res = E2BIG;
		return;
	}
	memcpy(msgsnd_bp, data, data_len);
	msgsnd_bp += data_len;
	out->hdr.res = SUCCESS;
}

void
p_msgsnd_end(data, data_len, out)
char *data;
int data_len;
struct output *out;
{
	log("msgsnd_end: data_len = %d\n", data_len);
	if (msgsnd_len < (msgsnd_bp - msgsnd_buf) + data_len) {
		log("msgsnd_ext: Too much data\n");
		out->hdr.res = E2BIG;
		return;
	}
	memcpy(msgsnd_bp, data, data_len);
	msgsnd_bp += data_len;
	p_msgsnd(msgsnd_buf, out);
	free(msgsnd_buf);
	msgsnd_buf = NULL;
}


/*
 * Receive a message
 */

void
p_msgrcv( data , out)
char *data;
struct output *out;
{

	int rc;
	register struct msgrcv_type *buf;
	register int i;
	struct pci_msgbuf *mbp;
	char *op;

	buf = (struct msgrcv_type *)data;
	if (flipBytes) {
	    lsflipm(buf->id,tmpLong);
	    lsflipm(buf->size,tmpLong);  
	    lsflipm(buf->type,tmpLong);
	    lsflipm(buf->flag,tmpLong);
	}
	log("msgrcv parameters: id %d  type %ld  size %d  flag %d\n",
	     buf->id,buf->type,buf->size, buf->flag);

	if (buf->size > MAX_OUTPUT) {
		log("msgrcv: Too large a packet\n");
		out->hdr.res = E2BIG;
		return;
	}
	op = &out->text[-4];
	mbp = (struct pci_msgbuf *)op;
	rc = msgrcv(xlat_In_ID((short)buf->id, MSG), (struct msgbuf *)mbp, 
				buf->size, (long)buf->type, (int)buf->flag);
	if ( rc != -1) 
	{
             log("(type %ld) : %s\n",buf->message.mtype,buf->message.mtext);
             out->hdr.t_cnt = (unsigned short)rc;
	     out->hdr.f_size = mbp->mtype;
             out->hdr.res =  SUCCESS; 
         }
	 else out->hdr.res = (unsigned char)errno;    
	 out->hdr.fdsc = (unsigned short)rc; 
	 log("MSGRCV Returned %d\n",out->hdr.fdsc);
}

/*
 * issue a message queue control operation
 */

void 
p_msgctl( data , out )
char *data;
struct output *out;
{
    int i, rc, ctlcmd;
    register struct msgctl_type *buf;
    register char *cptr;
    struct msqid_ds msgstat;

	buf = (struct msgctl_type *) data;
	if (flipBytes) {
	    lsflipm(buf->id,tmpLong);
	    lsflipm(buf->command,tmpLong);
	    if (buf->command == P_IPC_SET)
	    {
		dosflipm(buf->buffer.msg_perm.uid,tmp); 
		dosflipm(buf->buffer.msg_perm.gid,tmp); 
		dosflipm(buf->buffer.msg_perm.cuid,tmp); 
		dosflipm(buf->buffer.msg_perm.cgid,tmp); 
		dosflipm(buf->buffer.msg_perm.mode,tmp); 
		dosflipm(buf->buffer.msg_perm.seq,tmp); 
		lsflipm(buf->buffer.msg_perm.key,tmpLong);
		dosflipm(buf->buffer.msg_cbytes,tmp); 
		dosflipm(buf->buffer.msg_qnum,tmp); 
		dosflipm(buf->buffer.msg_qbytes,tmp); 
		dosflipm(buf->buffer.msg_lspid,tmp); 
		dosflipm(buf->buffer.msg_lrpid,tmp); 
		lsflipm(buf->buffer.msg_stime,tmpLong);
		lsflipm(buf->buffer.msg_rtime,tmpLong);
		lsflipm(buf->buffer.msg_ctime,tmpLong);
	    }
	}
	msgstat.msg_perm.uid = buf->buffer.msg_perm.uid; 
	msgstat.msg_perm.gid = buf->buffer.msg_perm.gid; 
	msgstat.msg_perm.cuid = buf->buffer.msg_perm.cuid; 
    	msgstat.msg_perm.cgid = buf->buffer.msg_perm.cgid; 
	msgstat.msg_perm.mode = buf->buffer.msg_perm.mode; 
	msgstat.msg_perm.seq = buf->buffer.msg_perm.seq; 
	msgstat.msg_perm.key = buf->buffer.msg_perm.key;
	msgstat.msg_cbytes = buf->buffer.msg_cbytes; 
	msgstat.msg_qnum = buf->buffer.msg_qnum; 
	msgstat.msg_qbytes = buf->buffer.msg_qbytes; 
	msgstat.msg_lspid = buf->buffer.msg_lspid; 
	msgstat.msg_lrpid = buf->buffer.msg_lrpid; 
	msgstat.msg_stime = buf->buffer.msg_stime;
	msgstat.msg_rtime = buf->buffer.msg_rtime;
	msgstat.msg_ctime = buf->buffer.msg_ctime;
	log("msgctl parameters: id %d  command %d\n", buf->id,buf->command);
	/*
	 * Message control command values in the incoming packet correspond to
	 * the SVR3 values.  For SVR4, the values have changed so they have to
	 * be converted here.  (This may be true for other OSs as well).
	 */
	switch (buf->command) {
	    case P_IPC_STAT:
		ctlcmd = IPC_STAT;
		break;
	    case P_IPC_SET:
		ctlcmd = IPC_SET;
		break;
	    case P_IPC_RMID:
		ctlcmd = IPC_RMID;
		break;
	    default:
		ctlcmd = buf->command;
		break;
	}
	do
	    rc = msgctl( xlat_In_ID((short)buf->id, MSG), ctlcmd, &msgstat);
	while (rc == -1 && errno == EINTR);
	if (buf->command == P_IPC_RMID)	/* clear from translation table */
	    free_id((short)buf->id);
	log("MSGCTL Returned %d\n",rc);
	if ( rc != -1)
	{
	    if (buf->command == P_IPC_STAT)
	    {
		buf->buffer.msg_perm.uid = msgstat.msg_perm.uid;
		buf->buffer.msg_perm.gid = msgstat.msg_perm.gid;
		buf->buffer.msg_perm.cuid = msgstat.msg_perm.cuid;
		buf->buffer.msg_perm.cgid = msgstat.msg_perm.cgid;
		buf->buffer.msg_perm.mode = msgstat.msg_perm.mode;
		buf->buffer.msg_perm.seq = msgstat.msg_perm.seq;
		buf->buffer.msg_perm.key = msgstat.msg_perm.key;
		buf->buffer.msg_cbytes = msgstat.msg_cbytes;
		buf->buffer.msg_qnum = msgstat.msg_qnum;
		buf->buffer.msg_qbytes = msgstat.msg_qbytes;
		buf->buffer.msg_lspid = msgstat.msg_lspid;
		buf->buffer.msg_lrpid = msgstat.msg_lrpid;
		buf->buffer.msg_stime = msgstat.msg_stime;
		buf->buffer.msg_rtime = msgstat.msg_rtime;
		buf->buffer.msg_ctime = msgstat.msg_ctime;

	        log("(msgctl returned) uid %u  gid %u  cuid %u\n",
		     buf->buffer.msg_perm.uid, buf->buffer.msg_perm.gid,
		     buf->buffer.msg_perm.cuid);
	        log("    cgid %u  mode %u  seq %u  key %ld\n",
	             buf->buffer.msg_perm.cgid, buf->buffer.msg_perm.mode,
	             buf->buffer.msg_perm.seq, buf->buffer.msg_perm.key);
	        log("    msg_cbytes %u msg_qnum %u\n",
		     buf->buffer.msg_cbytes, buf->buffer.msg_qnum);
	        log("    msg_qbytes %u msg_lspid %u msg_lrpid %u msg_stime %ld\n",
	             buf->buffer.msg_qbytes,buf->buffer.msg_lspid,
		     buf->buffer.msg_lrpid, buf->buffer.msg_stime);
 	        log("    msg_rtime %ld  msg_ctime %ld\n", 
	             buf->buffer.msg_rtime, buf->buffer.msg_ctime);
	        out->hdr.t_cnt = sizeof(struct pci_msqid_ds);
		if (flipBytes) {
	            dosflipm(buf->buffer.msg_perm.uid,tmp); 
	            dosflipm(buf->buffer.msg_perm.gid,tmp); 
	            dosflipm(buf->buffer.msg_perm.cuid,tmp); 
	            dosflipm(buf->buffer.msg_perm.cgid,tmp); 
	            dosflipm(buf->buffer.msg_perm.mode,tmp); 
	            dosflipm(buf->buffer.msg_perm.seq,tmp); 
	            lsflipm(buf->buffer.msg_perm.key,tmpLong);
	            dosflipm(buf->buffer.msg_cbytes,tmp); 
	            dosflipm(buf->buffer.msg_qnum,tmp); 
	            dosflipm(buf->buffer.msg_qbytes,tmp); 
	            dosflipm(buf->buffer.msg_lspid,tmp); 
	            dosflipm(buf->buffer.msg_lrpid,tmp); 
	            lsflipm(buf->buffer.msg_stime,tmpLong);
	            lsflipm(buf->buffer.msg_rtime,tmpLong);
	            lsflipm(buf->buffer.msg_ctime,tmpLong);
		}
	        cptr = (char *)&buf->buffer;
	        for (i = 0; i < sizeof(struct pci_msqid_ds); i++)
		    out->text[i] = *cptr++;
	    }
	    else		/* Command NOT P_IPC_STAT */
		out->hdr.t_cnt = 0;
	    out->hdr.res = SUCCESS;
	}
	else
	    out->hdr.res = (unsigned char)errno;
	out->hdr.fdsc = (unsigned short)rc; 
}

/*
 * get a semaphore id
 */

void 
p_semget( data , out )
char *data;
struct output *out;
{
    register int rc;
    register struct semget_type *buf;

	buf = (struct semget_type *)data;
	if (flipBytes) {
	    lsflipm(buf->key,tmpLong);
	    lsflipm(buf->number,tmpLong);
	    lsflipm(buf->flag,tmpLong);
	}
	log("semget parameters: key %ld  number %d  flag %x\n", buf->key,
	    buf->number, buf->flag);
	do
	    rc = semget( (key_t)buf->key, (int)buf->number, (int)buf->flag ); 
	while (rc == -1 && errno == EINTR);
	if (rc != -1) {
	    out->hdr.res = SUCCESS;
	    out->hdr.fdsc = (unsigned short)xlat_Out_ID(rc, SEM);
	   					 /* return semaphore id */
	    /* If xlat_Out_ID() returned -1, then there is not any more
	     * space in the translation table. Remove the msg created and 
	     * set the proper error message.
	     */
	    if (out->hdr.fdsc == -1) {
		semctl(rc, (int)buf->number, IPC_RMID, (int)buf->flag);
		out->hdr.res = ENOSPC;
	    }
	} else {
	    out->hdr.res = (unsigned char)errno;
	    out->hdr.fdsc = -1;
	}
	/* has UNIX run out of semaphores? */
	if (rc < 0 && ((int)buf->flag & IPC_CREAT) && errno == ENOSPC)
	    serious(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("P_IPC2",
		"Cannot create semaphore, errno = %1\n"),
		"%d", errno));
	log("SEMGET Returned %d\n",rc);
}

/*
 * perform semaphore operation
 */

void 
p_semop( data , out )
char *data;
struct output *out;
{
    int rc;
    register i;
    register struct semop_type *buf;
    struct sembuf *sbp;

	buf = (struct semop_type *)data;
	sbp = (struct sembuf *)&data[8];

	if (flipBytes) {
	    lsflipm(buf->id,tmpLong);
	    lsflipm(buf->number,tmpLong);
	    for (i = 0; i < buf->number; i++) {
		dosflipm(sbp->sem_num, tmp);
		dosflipm(sbp->sem_op, tmp);
		dosflipm(sbp->sem_flg, tmp);
		sbp++;
	    }
	    sbp = (struct sembuf *)&data[8];
	}
	log("semop parameters: id %d  number %d\n", buf->id, buf->number);
	for (i = 0; i < buf->number; i++) {
	    log("  sbp->sem_num   %d\n",sbp->sem_num);
	    log("  sbp->sem_op    %d\n",sbp->sem_op);
	    log("  sbp->sem_flag  %d\n",sbp->sem_flg);
	    sbp++;
	}
	sbp = (struct sembuf *)&data[8];
	do
	    rc = semop(xlat_In_ID((short)buf->id, SEM), sbp,
	   					 (unsigned)buf->number); 
	while (rc == -1 && errno == EINTR);
	out->hdr.res = (unsigned char)((rc >= 0) ? SUCCESS : errno);
	out->hdr.fdsc = (unsigned short)rc;  
	log("SEMOP Returned %d\n",rc);
}

/*
 * Issue a semaphore control operation
 */

void
p_semctl( data, out)
char *data;
struct output *out;
{
    int i, rc, ctlcmd;
    register struct semctl_type *buf;
    struct semid_ds semstat;
    struct pci_semid_ds *ostat;
    register char *cptr;

	buf = (struct semctl_type *) data;
	if (flipBytes) {
	    lsflipm(buf->id,tmpLong);
	    lsflipm(buf->number,tmpLong);
	    lsflipm(buf->command,tmpLong);
	    switch (buf->command) {
		case SETVAL:
		    lsflipm(buf->dsret.val,tmpLong);
		    break;
		case SETALL:
		    for (i = 0; i < buf->number; i++)
			dosflipm(buf->dsret.dsarr[i], tmp);
		    break;
		case P_IPC_SET:
		    dosflipm(buf->dsret.dsbuf.sem_perm.uid,tmp); 
		    dosflipm(buf->dsret.dsbuf.sem_perm.gid,tmp); 
		    dosflipm(buf->dsret.dsbuf.sem_perm.cuid,tmp); 
		    dosflipm(buf->dsret.dsbuf.sem_perm.cgid,tmp); 
		    dosflipm(buf->dsret.dsbuf.sem_perm.mode,tmp); 
		    dosflipm(buf->dsret.dsbuf.sem_perm.seq,tmp); 
		    lsflipm(buf->dsret.dsbuf.sem_perm.key,tmpLong);
		    break;
	    }
	}
	cptr = (char *)&buf->dsret.dsbuf;
	if (buf->command == P_IPC_SET) {
		semstat.sem_perm.uid = buf->dsret.dsbuf.sem_perm.uid; 
		semstat.sem_perm.gid = buf->dsret.dsbuf.sem_perm.gid; 
		semstat.sem_perm.cuid = buf->dsret.dsbuf.sem_perm.cuid; 
		semstat.sem_perm.cgid = buf->dsret.dsbuf.sem_perm.cgid; 
		semstat.sem_perm.mode = buf->dsret.dsbuf.sem_perm.mode; 
		semstat.sem_perm.seq = buf->dsret.dsbuf.sem_perm.seq; 
		semstat.sem_perm.key = buf->dsret.dsbuf.sem_perm.key;
		cptr = (char *)&semstat;
	} else if (buf->command == P_IPC_STAT)
		cptr = (char *)&semstat;
	else if (buf->command == SETVAL)
		cptr = (char *)buf->dsret.val;
	log("semctl parameters:  id %d  number %d  command %d\n",
	    buf->id, buf->number, buf->command);
	/*
	 * Semaphore control command values in the incoming packet correspond to
	 * the SVR3 values.  For SVR4, the values have changed so they have to
	 * be converted here.  (This may be true for other OSs as well).
	 */
	switch (buf->command) {
	    case P_IPC_STAT:
		ctlcmd = IPC_STAT;
		break;
	    case P_IPC_SET:
		ctlcmd = IPC_SET;
		break;
	    case P_IPC_RMID:
		ctlcmd = IPC_RMID;
		break;
	    default:
		ctlcmd = buf->command;
		break;
	}
	do
	    rc = semctl(xlat_In_ID((short)buf->id, SEM),
				(int)buf->number, ctlcmd, cptr);
	while (rc == -1 && errno == EINTR);
	if (buf->command == P_IPC_RMID)
	    free_id((short)buf->id);
	if (rc != -1) {
	    cptr = (char *)&buf->dsret.dsbuf;
	    if (buf->command == P_IPC_STAT) {
		ostat = (struct pci_semid_ds *)out->text;
		ostat->sem_perm.uid = semstat.sem_perm.uid;
		ostat->sem_perm.gid = semstat.sem_perm.gid;
		ostat->sem_perm.cuid = semstat.sem_perm.cuid;
		ostat->sem_perm.cgid = semstat.sem_perm.cgid;
		ostat->sem_perm.mode = semstat.sem_perm.mode;
		ostat->sem_perm.seq = semstat.sem_perm.seq;
		ostat->sem_perm.key = semstat.sem_perm.key;
		ostat->sem_nsems = semstat.sem_nsems;
		ostat->sem_otime = semstat.sem_otime;
		ostat->sem_ctime = semstat.sem_ctime;
		out->hdr.t_cnt = sizeof(struct pci_semid_ds);
		if (flipBytes) {
		    dosflipm(ostat->sem_perm.uid,tmp); 
		    dosflipm(ostat->sem_perm.gid,tmp); 
		    dosflipm(ostat->sem_perm.cuid,tmp); 
		    dosflipm(ostat->sem_perm.cgid,tmp); 
		    dosflipm(ostat->sem_perm.mode,tmp); 
		    dosflipm(ostat->sem_perm.seq,tmp); 
		    lsflipm(ostat->sem_perm.key,tmpLong);
		    dosflipm(ostat->sem_nsems,tmp);
		    lsflipm(ostat->sem_otime,tmpLong);
		    lsflipm(ostat->sem_ctime,tmpLong);
		}
	    } else if (buf->command == GETALL) {
		for (i = 0; i < buf->number * sizeof(ushort); i++)
		    out->text[i] = *cptr++;
		out->hdr.t_cnt = (ushort)(buf->number * sizeof (ushort));
	    } else	/* Doesn't return anything */
		out->hdr.t_cnt = 0;
	    out->hdr.res = SUCCESS;
	}
	else
	    out->hdr.res = (unsigned char)errno;
        out->hdr.fdsc = (unsigned short)rc;
}


/*
 *	xlat_Out_ID(id, type): 
 *		Translate the id to a uniqe short number to be returned to
 *		DOS.  The algorithm is that if the value of id is greater
 *		than 0x7f, then the id will be stored in the msgIDtbl and
 *		the index into the msgIDTable with MSB turned on will be 
 *		returned.
 *
 *	Input:
 *		id	- the actual msg/sem ID.
 *		type	- the sem/msg type (SEM/MSG)
 *
 *	Output:
 *		id, if id is less than 0x8000.
 *		-1, if no more space in the table.
 *		index into the ID table with the MSB turned on, otherwise.
 */
#if defined(__STDC__)
LOCAL int
xlat_Out_ID(int id, ushort type)
#else
LOCAL int
xlat_Out_ID(id, type)
int	id;		/* msg/sem ID */
ushort	type;		/* MSG/SEM */
#endif
{
    int index;

    /* If ID fits into a short, simply return it */
    if ((id & (~0x7fff)) == 0)
	return((unsigned short) id);

    /* 
     * ID is longer than short.  Look for the ID in the table.
     * If ID is in the table, return the index with the MSB
     * bit set.
     */
    if ((index = find_id(id, type)) >= 0) 
	return(index | shortMSB);

    /* 
     * ID not in table, insert it in the table 
     * find_id() will dynamically grow the table 
     * if necessary.
     */
    index = find_id(FREE_ID, type);
    if (index < 0)	/* no more space, we tried */
	return(-1);

    id_tbl[index].id = id;
    id_tbl[index].type = type;
    
    log("xlat_Out_ID: Inserting ID = %d, aindex = %d \n", 
					id, index|shortMSB);
    return (index | shortMSB);
}

/*
 *	xlat_In_ID: 
 *		Translate the id (short) to an int (int is long on the AIX).
 *
 *	Input:	id (short).
 *		type	SEM/MSG
 *	Output:	id if id is less than 0x8000.
 *		message ID in the msg_tbl corresponding to the input id.
 */
#if defined(__STDC__)
LOCAL int
xlat_In_ID(short id, ushort type)
#else
LOCAL int
xlat_In_ID(id, type)
short	id;		/* msg/sem ID */
ushort	type;		/* MSG/SEM */
#endif
{
    int	index;

    /* If ID dos NOT have the MSB bit on, simply return the id. */
    if ((id & shortMSB) == 0)
	return((int) id);

    /* 
     * Id has the MSB set, must be index into table. Use the index
     * to get the real ID.
     */
    index = id & 0x7fff;		/* Get index */
    log("xlat_In_ID: index = %d\n", index);
    if (index >= max_tbl || type != id_tbl[index].type)
	return(-1);	/* index out of range or type mismatch */

    log("xlat_In_ID: actual ID = %d\n", id_tbl[index].id);
    
    return(id_tbl[index].id);	
}

/*
 *	find_id:
 *		Finds the id in the table. 
 *
 *	Input:	id, the actual msg/sem ID.
 *	Output:	index into the table, if id exists in table.
 *		returns -1 if id does not exist or memory cannot
 *		be allocated, and -2 if there was a memory 
 *		reallocation error when trying to dynamically
 *		grow the table.
 *		Also, dynamically grow translation table
 *		when ther is no more free space.
 */
#if defined(__STDC__)
LOCAL int
find_id(int id, ushort type)
#else
LOCAL int
find_id(id, type)
int	id;		/* ID to be searched for */
ushort	type;		/* MSG/SEM */
#endif
{
    int	index;		/* index into table being returned */
    int i;		/* another index variable */
    int success = 0;	/* did we find the id */
    
    /* if memory has not been allocated yet */
    if (id_tbl == NULL) {
	/* allocate memory for this table */
	id_tbl = (struct table *)malloc(TBL_INCREMENT * sizeof(struct table));
	if (id_tbl == NULL) 	/* couldn't allocate memory */	
	    return(-1);
	
	/* initialize to FREE_ID */
	for (i = 0; i < TBL_INCREMENT; i++)
	    id_tbl[i].id = FREE_ID;
	
	/* update max table entries */
	max_tbl = TBL_INCREMENT;
    }
    
    /* Search the table for the id */
    for (index = 0; index < max_tbl; index++) {
	if (id != id_tbl[index].id)
	    continue;
	if (id == FREE_ID || type == id_tbl[index].type) {
	    success = 1;	/* found it */
	    break;
	}
    }
    
    if (index == max_tbl && id == FREE_ID) {
	/* we've run out of space in our table */
	/* reallocate the translation table */
	id_tbl = (struct table *)realloc(id_tbl, 
			(max_tbl + TBL_INCREMENT) * sizeof(struct table));
	if (id_tbl == NULL) 	/* BAD news, couldn't reallocate */
	    return(-2);
	
	/* initialize to FREE_ID */
	for (i = max_tbl; i < (max_tbl + TBL_INCREMENT); i++)
	    id_tbl[i].id = FREE_ID;
	
	/* set index to proper value */
	index = max_tbl;
	success = 1;	/* found a free slot */
	
	/* update max table entries */
	max_tbl += TBL_INCREMENT;
    }

    /* Return the index */
    if (success)
	return(index);
    else
	return(-1);
}


#if defined(__STDC__)
LOCAL void
free_id(short index)
#else
LOCAL void
free_id(index)
short	index;
#endif
{
    log("Cleaning up id_tbl[%d], %d\n", index & 0x7fff, index);
    if ((index & shortMSB) != 0)
	id_tbl[(int)(index & 0x7fff)].id = FREE_ID;
}


#endif	/* NOIPC */
