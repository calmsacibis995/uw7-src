/*
 * Connection-less datagram service over DLPI..
 */
#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/errno.h>
#include <sys/dlpi_ether.h>
#include <signal.h>
#include <errno.h>
#include <dl.h> 
#include <tc_net.h>
#include <tc_res.h>

#define DLGADDR		(('D' << 8) | 5)	/* get physical addr */
static char tmpbuf[BUFSIZ];	/* 1K temp buf */

/* All dl interface functions behaves in the following manner.
 * returns 0 on succes and -1 on failure and dl_errno is set
 * to indicate the error. 
 */
int		dl_errno;
int		dl_timeout = DL_GETMSG_TIMEOUT;
extern int	dl_debug;

/*
 * dl_bind - binds a SAP address to a Data Link end point..
 */
int
dl_bind(int fd, dl_bind_req_t *bind_req, dl_bind_ack_t *bind_ack) 
{

	if (dl_bindreq(fd, bind_req) < 0) 
		return -1;

	return dl_bindack(fd, bind_ack);
}	
		
/*
 * dl_unbind - unbinds a SAP address to a Data Link end point..
 */
int 
dl_unbind(int fd)
{
	dl_unbind_req_t unbind_req;
	dl_ok_ack_t ok_ack;
	
	unbind_req.dl_primitive = DL_UNBIND_REQ;

	if (dl_unbindreq(fd, &unbind_req) < 0)
		return -1;

	if (dl_okack(fd, &ok_ack) < 0) {
		DL_DEBUG(1,"dl_unbind: dl_okack failed\n");
		return -1;
	}

	if (ok_ack.dl_correct_primitive != DL_UNBIND_REQ) {
		DL_DEBUG1(1,"dl_unbindack: bad primitive %x\n",
				ok_ack.dl_correct_primitive);
		dl_errno = DL_EPROTO;
		return -1;
	}
	return 0;
}

/*
 * show dl information..
 */
void
dl_showinfo(int fd)
{
	dl_info_ack_t info_ack;

	if (dl_info(fd, &info_ack) < 0) {
		DL_DEBUG(1,"dl_showinfo: dl_info failed");
		return;
	}
	printf("dl_max_sdu: %x\n",info_ack.dl_max_sdu); 
	printf("dl_min_sdu: %x\n",info_ack.dl_min_sdu); 
	printf("dl_addr_length: %x\n", info_ack.dl_addr_length); 
	printf("dl_mac_type: %x\n" , info_ack.dl_mac_type);
	printf("dl_current_state: %x\n", info_ack.dl_current_state);
	printf("dl_service_mode: %x\n", info_ack.dl_service_mode);
	printf("dl_qos_length: %x\n", info_ack.dl_qos_length);
	printf("dl_provider_style: %x\n", info_ack.dl_provider_style);
	printf("dl_growth: %x\n", info_ack.dl_growth); 
}
/*
 * get dl information..
 */
int
dl_info(int fd, dl_info_ack_t *info_ack)
{
	dl_info_req_t info_req;

	info_req.dl_primitive = DL_INFO_REQ;

	if (dl_inforeq(fd, &info_req) < 0) 
		return -1;

	return dl_infoack(fd, info_ack);
}

/*
 * Subsequent bind operation..
 */
int
dl_subsbind(int fd, struct snap_sap snap_sap)
{
	dl_subs_bind_ack_t subs_bind_ack;
	char	reqbuf[sizeof(dl_subs_bind_req_t) + sizeof(struct snap_sap)];
	dl_subs_bind_req_t *subs_bind_reqp;

	subs_bind_reqp = (dl_subs_bind_req_t *)reqbuf;
	subs_bind_reqp->dl_primitive = DL_SUBS_BIND_REQ;
	subs_bind_reqp->dl_subs_sap_offset = sizeof(dl_subs_bind_req_t);
	subs_bind_reqp->dl_subs_sap_length = sizeof(struct snap_sap);
	
	*(struct snap_sap *)(reqbuf + sizeof(dl_subs_bind_req_t)) = snap_sap;


	if (dl_subsbindreq(fd, subs_bind_reqp) < 0) 
		return -1;

	return dl_subsbindack(fd, &subs_bind_ack);
}

/*
 * Try subsequent unbind ...
 */
int
dl_subsunbind(int fd, struct snap_sap snap_sap)
{
	dl_ok_ack_t	ok_ack;
	char	reqbuf[sizeof(dl_subs_unbind_req_t) + sizeof(struct snap_sap)];
	dl_subs_unbind_req_t *subs_unbind_reqp;

	subs_unbind_reqp = (dl_subs_unbind_req_t *)reqbuf;
	subs_unbind_reqp->dl_primitive = DL_SUBS_UNBIND_REQ;
	subs_unbind_reqp->dl_subs_sap_offset = sizeof(dl_subs_unbind_req_t);
	subs_unbind_reqp->dl_subs_sap_length = sizeof(struct snap_sap);
	
	*(struct snap_sap *)(reqbuf + sizeof(dl_subs_bind_req_t)) = snap_sap;

	if (dl_subsunbindreq(fd, subs_unbind_reqp) < 0) {
		DL_DEBUG(1,"dl_subsunbind: dl_subsunbindreq failed\n");
		return -1;
	}

	if (dl_okack(fd, &ok_ack) < 0) {
		DL_DEBUG(1,"dl_subsunbind: dl_okack failed\n");
		return -1;
	}

	if (ok_ack.dl_correct_primitive != DL_SUBS_UNBIND_REQ) {
		DL_DEBUG1(1,"dl_subsunbindack: bad primitive %x\n",
				ok_ack.dl_correct_primitive);
		dl_errno = DL_EPROTO;
		return -1;
	}
	
	return 0;
}

copy(d, s, n)
char	*d;
char 	*s; 
int	n;
{
	int	i;

	for (i = 0; i < n; i++)
		*d++ = *s++;
}	


static int promisc_flag = 0;

/*
 * get promiscuous mode flags
 */
int
dl_gpromisc(int fd, int *promiscp)
{
	int	ret;
	int	cmdlen;

	cmdlen = 0;
	return(*promiscp = dl_ioctl(fd, DLIOCGPROMISC, &cmdlen, NULL));
}

/* 
 * Previleged process can enable promiscuous mode.
 */
int
dl_promiscon(fd)
int	fd;
{
	int	ret = 0;
	int	cmdlen;

	cmdlen = 0;
	if (promisc_flag == 0) 
		if ((ret = dl_ioctl(fd, DLIOCSPROMISC, &cmdlen, NULL)) == 0)
			promisc_flag = 1;
	return ret;
}

/* 
 * Previleged process can disable promiscuous mode.
 */
int
dl_promiscoff(fd)
int	fd;
{
	int	ret = 0;
	int	cmdlen;

	cmdlen = 0;
	if (promisc_flag == 1) 
		if ((ret = dl_ioctl(fd, DLIOCSPROMISC,  &cmdlen, NULL)) == 0)
			promisc_flag = 0;
	return ret;
}

/* 
 * Get MIB variables
 */
int
dl_gmib(int fd, DL_mib_t *mibp)
{
	int cmdlen;
	
	cmdlen = sizeof(DL_mib_t);
	if (dl_ioctl(fd, DLIOCGMIB, &cmdlen,(char *)mibp) < 0) 
		return -1;
	return cmdlen;
}

/* 
 * Previleged process can set values for MIB variable.
 */
int
dl_smib(int fd, DL_mib_t *mibp)
{
	int cmdlen;
	
	cmdlen = sizeof(DL_mib_t);
	if (dl_ioctl(fd, DLIOCSMIB, &cmdlen,(char *)mibp) < 0) 
		return -1;
	return cmdlen;
}

/* 
 * Previleged process can switch the sap type from 
 * DL_CSMACD to RAWSAP.
 */
int
dl_ccsmacdmode(int fd)
{
	int cmdlen;

	cmdlen = 0;
	return dl_ioctl(fd, DLIOCCSMACDMODE, &cmdlen, NULL);
}
/*
 * Gets the value of local packet copy flag.
 */
int
dl_glpcflg(int fd, int *lpcflagp)
{
	int	cmdlen;

	cmdlen = 0;
	return (*lpcflagp = dl_ioctl(fd, DLIOCGLPCFLG, &cmdlen, NULL));
}


/* 
 * Sets the value of local packet copy flag.
 */
int
dl_slpcflg(int fd)
{
	int	cmdlen;

	cmdlen = 0;
	return dl_ioctl(fd, DLIOCSLPCFLG, &cmdlen, NULL);
}

/* 
 * Gets the Ethernet Address in network order.
 */
int
dl_genaddr(int fd, char enaddr[])
{
	int	cmdlen;

	cmdlen = LLC_ADDR_LEN;
	if (dl_ioctl(fd, DLIOCGENADDR, &cmdlen, enaddr) < 0)
		/*
		 * dlpi token driver does not recognize DLIOCGENADDR,
		 * so try DLGADDR
		 */
		if (dl_ioctl(fd, DLGADDR, &cmdlen, enaddr) < 0)
			return(-1);
	return cmdlen;
}

/* 
/* 
 * Previleged process can set the Ethernet address.
 */
int
dl_senaddr(int fd, char enaddr[])
{
	int	cmdlen;

	cmdlen = LLC_ADDR_LEN;
	if (dl_ioctl(fd, DLIOCSENADDR, &cmdlen, enaddr) < 0)
		return -1;
	return cmdlen;
}

/* 
 * Get the list of multicast address.
 */
int
dl_getmulti(fd, maddrs)
int	fd;
char	maddrs[];
{
	int	cmdlen;

	cmdlen = LLC_ADDR_LEN;
	if (dl_ioctl(fd, DLIOCGETMULTI, &cmdlen, maddrs) < 0)
		return -1;
	return cmdlen;
}

/* 
 * Previleged process can add a multicast address.
 */
int
dl_addmulti(int fd, char maddr[])
{
	int	cmdlen;
	
	cmdlen = LLC_ADDR_LEN;
	return(dl_ioctl(fd, DLIOCADDMULTI, &cmdlen, maddr));
}


/* 
 * Previleged process can delete a multicast address. 
 */
int
dl_delmulti(fd, maddr)
int	fd;
char	maddr[];
{
	int	cmdlen;

	cmdlen = LLC_ADDR_LEN;
	return(dl_ioctl(fd, DLIOCDELMULTI, &cmdlen, maddr));
}


/* 
 * Previledge process can reset the controller.
 */
int
dl_reset(int fd)
{
	int	cmdlen;

	cmdlen = 0;
	return(dl_ioctl(fd, DLIOCRESET, &cmdlen, NULL));
}


/* 
 * Previledge process can enable the controller.
 */
int
dl_enable(int fd)
{
	int	cmdlen;

	cmdlen = 0;
	return(dl_ioctl(fd, DLIOCENABLE, &cmdlen, NULL));
}

/*
 * Previleged process can disable the controller.
 */
int
dl_disable(int fd)
{
	int	cmdlen;

	cmdlen = 0;
	return(dl_ioctl(fd, DLIOCDISABLE, &cmdlen, NULL)); 
}

int
dl_inforeq(int fd, dl_info_req_t *inforeq)
{
	
	if (dl_putmsg(fd, (char *)inforeq, sizeof(dl_info_req_t), 
			NULL, 0, RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_inforeq: dl_putmsg failed\n");
		return -1;
	}
	
	return 0;
}

int
dl_infoack(int fd, dl_info_ack_t *ackp)
{
	
	int	acklen;
	int	datalen;
	int	flags;

	acklen = BUFSIZ;
	datalen = 0;
	flags = 0;
	if (dl_getmsg(fd, tmpbuf, &acklen, 
				NULL, &datalen,  &flags, NOBLOCKING) < 0) {
		DL_DEBUG(1,"dl_infoack: dl_getmsg failed\n");
		return -1;
	}

	if (dl_checkack((union DL_primitives *)tmpbuf, acklen, flags,
			 DL_INFO_ACK, sizeof(dl_info_ack_t), RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_infoack: dl_checkack() failed\n");	
		return -1;
	}
	copy((char *)ackp, tmpbuf, sizeof(dl_info_ack_t));
	return 0;
}

dl_attachreq(int fd, u_long ppa)
{
	dl_attach_req_t	attach_req;

	attach_req.dl_primitive = DL_ATTACH_REQ;
	attach_req.dl_ppa = ppa;

	if (dl_putmsg(fd, (char *)&attach_req, sizeof(dl_attach_req_t), 
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_attachreq: dl_putmsg failed\n");
		dl_errno = DL_EPUTMSG;
		return -1;
	}

	return 0;
}

dl_detachreq(int fd)
{
	dl_detach_req_t	detach_req;

	detach_req.dl_primitive = DL_DETACH_REQ;

	if (dl_putmsg(fd, (char *)&detach_req, sizeof(dl_detach_req_t), 
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_detachreq: dl_putmsg failed\n");
		dl_errno = DL_EPUTMSG;
		return -1;
	}

	return 0;
}

/*
 * dl_bindreq - binds a SAP address to a Data Link end point..
 */
int
dl_bindreq(int fd, dl_bind_req_t *bind_req)
{
	if (dl_putmsg(fd, (char *)bind_req, sizeof(dl_bind_req_t), 
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_bindreq: dl_putmsg failed\n");
		dl_errno = DL_EPUTMSG;
		return -1;
	}

	return 0;
}

int
dl_bindack(int fd, dl_bind_ack_t *ackp)
{
	int		acklen;
	int		datalen;
	int		flags;
	int		i;

	acklen = BUFSIZ;
	datalen = 0;
	flags = 0;
	if (dl_getmsg(fd, (char *)tmpbuf, &acklen, 
				NULL, &datalen,  &flags, NOBLOCKING) < 0) {
		DL_DEBUG(1,"dl_bindack: dl_getmsg() failed\n");
		return -1;
	}

	if (dl_checkack((union DL_primitives *)tmpbuf, acklen, flags,
			DL_BIND_ACK, sizeof(dl_bind_ack_t), RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_bindack: dl_checkack failed\n");
		return -1;
	}
	
	copy((char *)ackp, tmpbuf, sizeof(dl_bind_ack_t));
	DL_DEBUG(2,"dl_bindack: The bound DLSAP addr: [ ");
	for (i = 0; i < ackp->dl_addr_length; i++)
		DL_DEBUG1(2,"%x ", *(tmpbuf + i + ackp->dl_addr_offset) & 0xff);
	DL_DEBUG(2,"]\n");

	return 0;
}

int
dl_unbindreq(int fd, dl_unbind_req_t *unbind_req)
{
	if (dl_putmsg(fd, (char *)unbind_req, sizeof(dl_unbind_req_t), 
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_unbindreq: dl_putmsg failed\n");
		return -1;
	}

	return 0;
}

int
dl_okack(int fd, dl_ok_ack_t *ackp)
{
	int		acklen;
	int		datalen;
	int		flags;

	acklen = BUFSIZ;
	datalen = 0;
	flags = 0;
	if (dl_getmsg(fd, tmpbuf, &acklen, 
				NULL, &datalen,  &flags, NOBLOCKING) < 0) {
		DL_DEBUG(1,"dl_okack: dl_getmsg() failed\n");
		return -1;
	}

	if (dl_checkack((union DL_primitives *)tmpbuf, acklen, flags,
			DL_OK_ACK, sizeof(dl_ok_ack_t), RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_okack: dl_checkack() failed\n");
		return -1;
	}
	copy((char *)ackp, tmpbuf, sizeof(dl_ok_ack_t));
	return 0;
}
int
dl_errorack(int fd, dl_error_ack_t *ackp)
{
	int		acklen;
	int		datalen;
	int		flags;

	acklen = sizeof(dl_error_ack_t);
	datalen = 0;
	flags = 0;
	if (dl_getmsg(fd, (char *)ackp, &acklen, 
				NULL, &datalen, &flags, NOBLOCKING) < 0) {
		DL_DEBUG(1,"dl_errorack: dl_getmsg() failed\n");
		return -1;
	}

	if (dl_checkack((union DL_primitives *)ackp, acklen, flags,
			DL_ERROR_ACK, sizeof(dl_error_ack_t), RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_okack: dl_checkack() failed\n");
		return -1;
	}

	return 0;
}

int
dl_subsbindreq(int fd, dl_subs_bind_req_t *subsbind_reqp)
{
	if (dl_putmsg(fd, (char *)subsbind_reqp,
			sizeof(dl_subs_bind_req_t) + sizeof(struct snap_sap),
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_subsbindreq: dl_putmsg");
		dl_unbind(fd);		
		return(-1);
	}
	return 0;
}

int
dl_subsbindack(int fd, dl_subs_bind_ack_t *ackp)
{
	int			acklen;
	int			datalen;
	int			flags;

	acklen = BUFSIZ;
	datalen = 0;
	flags = 0;
	if (dl_getmsg(fd, tmpbuf, &acklen, 
				NULL, &datalen,  &flags, NOBLOCKING) < 0) {
		dl_unbind(fd);		
		dl_errno = DL_EGETMSG;
		-1;
	}

	if (dl_checkack((union DL_primitives *)tmpbuf, acklen, flags, 
			DL_SUBS_BIND_ACK, 
			sizeof(dl_subs_bind_ack_t), RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_subsbindack: dl_checkack() failed\n");
		return -1;
	}
	copy((char *)ackp, tmpbuf, sizeof(dl_subs_bind_ack_t));
	return 0;
}

int
dl_subsunbindreq(int fd, dl_subs_unbind_req_t *req)
{
	int			i, flags;
	struct strbuf		ctlbuf;

	ctlbuf.len = sizeof(dl_subs_unbind_req_t) + sizeof(struct snap_sap);
	ctlbuf.buf = (char *)req;

	if (dl_putmsg(fd, (char *)req,
			sizeof(dl_subs_unbind_req_t) + sizeof(struct snap_sap),
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_subsunbindreq: dl_putmsg");
		return(-1);
	}	
	return 0;
}

/*
 * dl_sndudata: send data
 */
int
dl_sndudata(int fd, char *data, int datalen, char *addr, int addrlen)
{
	dl_unitdata_req_t	*reqp;
	int	i;


	reqp = (dl_unitdata_req_t *)malloc(sizeof(dl_unitdata_req_t) + addrlen);
	if (reqp == NULL) {
		dl_errno = DL_EALLOC;
		return(-1);
	}
	
	reqp->dl_primitive = DL_UNITDATA_REQ;
	reqp->dl_dest_addr_length = addrlen;
	reqp->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
	copy((char *)reqp + sizeof(dl_unitdata_req_t), addr, addrlen);
		
	DL_DEBUG(2,"dl_sndudata: INFO:sending [ ");
	for (i = 0; i < 10; i++)
		DL_DEBUG1(2,"%x ", data[i] & 0xff);
	DL_DEBUG(2,"]\n");
	DL_DEBUG(2,"                       to [");
	for (i = 0; i < addrlen; i++)
		DL_DEBUG1(2,"%x ", addr[i] & 0xff);
	DL_DEBUG(2,"]\n");

	if (dl_putmsg(fd, (char *)reqp, sizeof(dl_unitdata_req_t) + addrlen, 
			data, datalen, 0) < 0) {
		DL_DEBUG(2,"dl_sndudata: ERROR:dl_putmsg failed\n");
		return -1;
	}

	return 0;
}
/*
 * dl_rcvudata: receives data from 
 */
int
dl_rcvudata(int fd,char data[],int *datalen,char addr[],int *addrlen)
{
	dl_unitdata_ind_t	*indp;
	int			indlen;
	int			flags;
	char			*p;
	int			i;	

	indlen = BUFSIZ;
	flags = 0;
	if (dl_getmsg(fd, tmpbuf, &indlen,
			data, datalen, &flags, NOBLOCKING) < 0) {
		return -1;
	}	
	indp = (dl_unitdata_ind_t *)tmpbuf;
	if (indp->dl_primitive != DL_UNITDATA_IND) {
		dl_errno = DL_EPROTO;
		DL_DEBUG1(2,"dl_rcvudata: Primitive: %x", indp->dl_primitive); 
		if (indp->dl_primitive == DL_ERROR_ACK) {
			DL_DEBUG1(1," Error: %x", 
				dl_errno = ((dl_error_ack_t *)indp)->dl_errno);
		}
		DL_DEBUG(2,"\n");
		return -1;
	} 

	p = (char *)indp + indp->dl_src_addr_offset;
	/* fill in the address of the sending DLS..  */
	copy(addr, p, indp->dl_src_addr_length);
	*addrlen = indp->dl_src_addr_length;

	DL_DEBUG(2,"dl_rcvudata: INFO:received [");
	for (i = 0; i < 10; i++)
		DL_DEBUG1(2,"%x ",data[i] & 0xff);
	DL_DEBUG(2,"]\n");

	DL_DEBUG(2,"                      from [");
	for (i = 0; i < indp->dl_src_addr_length; i++ )
		DL_DEBUG1(2,"%x ", p[i] & 0xff);
	DL_DEBUG(2,"]\n");
	p = (char *)indp + indp->dl_dest_addr_offset;
	DL_DEBUG(2,"                        to [");
	for (i = 0; i < indp->dl_dest_addr_length; i++ )
		DL_DEBUG1(2,"%x ", p[i] & 0xff);
	DL_DEBUG(2,"]\n");

	/* return the no of bytes received..  */
	return *datalen;
}

int
dl_checkack(union DL_primitives *ackp, int acksize, int flags,
		int	exp_prim,	/* expected primitive */
		int	exp_size,	/* expected size */
		int	exp_flags) 	/* expected flags */
{
	if (ackp->dl_primitive != (u_long)exp_prim) {
		DL_DEBUG1(1,"dl_checkack: unexpected primitive = %x\n", ackp->dl_primitive);
		if (ackp->dl_primitive != DL_ERROR_ACK) {
			dl_errno = DL_EPROTO;
			return -1;
		}
		else if (acksize < sizeof(dl_error_ack_t)) {
			DL_DEBUG1(1,"dl_checkack: DL_ERROR_ACK response too short len = %d\n", acksize);
			dl_errno = DL_EPROTO;
			return -1;	
		}
		dl_errno = ackp->error_ack.dl_errno;
		DL_DEBUG1(1,"dl_checkack: DL_ERROR_ACK dl_errno=%d\n",dl_errno);
		return -1;

	}
	
	if (acksize < exp_size) {
		DL_DEBUG1(1,"dl_checkack: response too short: len = %d\n", acksize);
		dl_errno = DL_EPROTO;
		return -1;	
	}
/* Don't check it for now as some of the responses from DLPI don't care.
	if (flags != exp_flags) {
		DL_DEBUG(1,"dl_checkack: not a M_PCPROTO message\n");
		dl_errno = DL_EPROTO;
		return -1;
	}
*/
	return 0;
}

/*
 * Send a DL_XID_REQ primitive..
 */
dl_xidreq(int fd, char *addr, int addrlen)
{
	dl_xid_req_t		*dl_reqp;

	dl_reqp = (dl_xid_req_t *)malloc(sizeof(dl_xid_req_t) + addrlen);
	if (dl_reqp == NULL) {
		dl_errno = DL_EALLOC;
		return(-1);
	}
	dl_reqp->dl_primitive = DL_XID_REQ;
	dl_reqp->dl_dest_addr_length = addrlen;
	dl_reqp->dl_dest_addr_offset = sizeof(dl_xid_req_t);
	dl_reqp->dl_flag = DL_POLL_FINAL;

	copy((char *)dl_reqp + dl_reqp->dl_dest_addr_offset, addr, addrlen);

	if (dl_putmsg(fd, (char *)dl_reqp, sizeof(dl_xid_req_t) + addrlen, 
			NULL, 0, RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_xidreq: dl_putmsg failed\n");
		return(-1);
	}
	
	return 0;
}
/*
 * Send a DL_TEST_REQ primitive..
 */
int
dl_testreq(int fd, char *addr, int addrlen)
{
	dl_test_req_t *	test_reqp;

	test_reqp = (dl_test_req_t *)malloc(sizeof(dl_test_req_t) + addrlen);
	if (test_reqp == NULL) {
		dl_errno = DL_EALLOC;
		return(-1);
	}
	test_reqp->dl_primitive = DL_TEST_REQ;
	test_reqp->dl_dest_addr_length = addrlen;
	test_reqp->dl_dest_addr_offset = sizeof(dl_test_req_t);
	test_reqp->dl_flag = DL_POLL_FINAL;

	copy((char *)test_reqp + test_reqp->dl_dest_addr_offset, addr, addrlen);

	if (dl_putmsg(fd, (char *)test_reqp, sizeof(dl_xid_req_t) + addrlen, 
			NULL, 0, RS_HIPRI) < 0) {
		DL_DEBUG(1,"dl_xidreq: dl_putmsg failed\n");
		return(-1);
	}
	
	return 0;
}

dl_testres(int fd, char *addrp, int addrlen)
{
	dl_test_res_t *testresp;

	testresp = (dl_test_res_t *)malloc(sizeof(dl_test_res_t) + addrlen);
	if (testresp == NULL) {
		dl_errno = DL_EALLOC;
		return -1;
	}
					
	/* send DL_TEST_RES to the client..  */
	testresp->dl_primitive = DL_TEST_RES;
	testresp->dl_flag = DL_POLL_FINAL;
	testresp->dl_dest_addr_length = addrlen;
	testresp->dl_dest_addr_offset = sizeof(dl_test_res_t);
	/* fill in the destination address..  */	
	copy((char *)testresp + testresp->dl_dest_addr_offset, addrp, addrlen);

	if (dl_putmsg(fd, (char *)testresp, sizeof(dl_test_res_t) + addrlen,
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_testres: dl_putmsg failed\n");
		return -1;
	}	

	return 0;
}

dl_xidres(int fd, char *addrp, int addrlen)
{
	dl_xid_res_t	*xidresp;

	xidresp = (dl_xid_res_t *)malloc(sizeof(dl_xid_res_t) + addrlen);
	if (xidresp == NULL) {
		dl_errno = DL_EALLOC;
		return -1;
	}
					
	/* send DL_XID_RES to the client..  */
	xidresp->dl_primitive = DL_XID_RES;
	xidresp->dl_flag = DL_POLL_FINAL;
	xidresp->dl_dest_addr_length = addrlen;
	xidresp->dl_dest_addr_offset = sizeof(dl_xid_res_t);
	/* fill in the destination address..  */	
	copy((char *)xidresp + xidresp->dl_dest_addr_offset, addrp, addrlen);

	if (dl_putmsg(fd, (char *)xidresp, sizeof(dl_xid_res_t) + addrlen,
			NULL, 0, 0) < 0) {
		DL_DEBUG(1,"dl_xidres: dl_putmsg failed\n");
		return -1;
	}	

	return 0;
}


static int
sigalrm()
{
	DL_DEBUG(1, "sigalrm:  TIMEOUT");
}

int
dl_putmsg(int fd,
	char	*ctlp,
	int	ctllen,
	char	*datap,
	int	datalen,
	int	flags)
{
	struct strbuf	databuf, ctlbuf;

	ctlbuf.len    = ctllen;
	ctlbuf.buf    = (char *)ctlp;

	databuf.len	= datalen;
	databuf.buf	= datap;
	
	if (putmsg(fd, &ctlbuf, &databuf, flags) < 0) {
		perror("dl_putmsg: putmsg");
		dl_errno = DL_EPUTMSG;
		return -1;
	}
	return 0;
}

int
dl_getmsg(int fd, 
	char *ctlp, 
	int  *ctllen, 
	char *datap, 
	int  *datalen, 
	int  *flags,
	int  blk_flg)
{
	int	rc;
	struct strbuf	databuf, ctlbuf;

	ctlbuf.len    = 0;
	ctlbuf.maxlen = *ctllen;
	ctlbuf.buf    = ctlp;

	databuf.len	= 0;
	databuf.maxlen	= *datalen;
	databuf.buf	= datap;
	

	if (blk_flg == NOBLOCKING) {
		/*
	 	* Start timer.
	 	*/
		signal(SIGALRM, (void (*)(int))sigalrm);
		if (alarm(dl_timeout) < 0) {
			perror("dl_getmsg: alarm");
			dl_errno = DL_EALARM;
			return -1;
		}
	}
	/*
	 * Set flags argument and issue getmsg().
	 */
	if ((rc = getmsg(fd, &ctlbuf, &databuf, flags)) < 0) {
		perror("dl_getmsg: getmsg");
		dl_errno = DL_EGETMSG;
		return -1;
	}

	if (blk_flg == NOBLOCKING) {
		/*
		 * Stop timer.
		 */
		if (alarm(0) < 0) {
			perror("dl_getmsg: alarm");
			dl_errno = DL_EALARM;
			return -1;
		}
	}

	/*
	 * Check for MOREDATA and/or MORECTL.
	 */
	if ((rc & (MORECTL | MOREDATA)) == (MORECTL | MOREDATA)) 
		DL_DEBUG2(1,"dl_getmsg: MORECTL|MOREDATA ctllen=%d,datalen=%d\n", ctlbuf.len, databuf.len);
	if ((rc & MORECTL) == MORECTL)
		DL_DEBUG1(1,"dl_getmsg: MORECTL ctllen=%d\n",ctlbuf.len);
	if ((rc & MOREDATA) == MOREDATA)
		DL_DEBUG1(1,"dl_getmsg: MOREDATA datalen=%d\n",databuf.len);

	/*
	 * Check for at least sizeof (long) control data portion.
	 */
	if (ctlbuf.len < sizeof (long)) {
		DL_DEBUG1(1,"dl_getmsg: response too short: len = %d\n",ctlbuf.len);
		dl_errno = DL_EPROTO;
		return -1;
	}

	*ctllen = ctlbuf.len;
	*datalen = databuf.len;

	return rc;
}

int
dl_ioctl( int fd,int cmd,int *datalen, char *dp)
{
	struct	strioctl	sioc;
	int	rc;

	sioc.ic_cmd = cmd;
	sioc.ic_timout = DL_IOC_TIMEOUT;
	sioc.ic_len = *datalen;
	sioc.ic_dp = dp;
	rc = ioctl(fd, I_STR, &sioc);
	
	*datalen = sioc.ic_len;
	return rc;
}

