/*
 * File api_dlpi.c
 * Our dlpi interface set of primitives
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) @(#)api_dlpi.c	27.1")

#include <sys/types.h>
#include <stdio.h>
#include <stropts.h>
#include <memory.h>

#include "sys/scodlpi.h"
#include "dlpiut.h"

static union DL_primitives primbuf;

static struct strbuf primptr = {
	0, 0, (char *)&primbuf
};

static struct strbuf dataptr;

static int	dlpi_chkstrerr(int fd);

int
dlpi_bind(int fd, int sap)
{
	int ret, flags = 0;
	register dl_bind_req_t *brp;
	register dl_bind_ack_t *bap;
	dl_error_ack_t *bep;
	int nsap;

	/* set up the DL_bind_req */
	brp = (dl_bind_req_t *)primptr.buf;
	memset(brp, 0, DL_BIND_REQ_SIZE);
	brp->dl_primitive = DL_BIND_REQ;
	brp->dl_sap = sap;
	brp->dl_xidtest_flg = 3;
	if (cmd.c_framing == F_LLC1_802_3 || cmd.c_framing == F_LLC1_802_5)
		brp->dl_service_mode = DL_CLDLS;
	primptr.len = DL_BIND_REQ_SIZE;
	primptr.maxlen = DL_BIND_REQ_SIZE;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_bind_ack_t *)primptr.buf;
	if (bap->dl_primitive == DL_ERROR_ACK) {
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_sbind(int fd, int sap)
{
	int ret, flags = 0;
	register dl_subs_bind_req_t *brp;
	register dl_subs_bind_ack_t *bap;
	sco_snap_sap_t *sp;
	dl_error_ack_t *bep;
	int nsap;

	/* set up the DL_bind_req */
	brp = (dl_subs_bind_req_t *)primptr.buf;
	memset(brp, 0, DL_SUBS_BIND_REQ_SIZE);
	brp->dl_primitive = DL_SUBS_BIND_REQ;
	brp->dl_subs_sap_offset = DL_SUBS_BIND_REQ_SIZE;
	brp->dl_subs_sap_length = sizeof(sco_snap_sap_t);
	brp->dl_subs_bind_class = DL_HIERARCHICAL_BIND;
	sp = (sco_snap_sap_t *)((uchar *)brp + DL_SUBS_BIND_REQ_SIZE);
	sp->prot_id = 0;
	sp->type = sap;
	primptr.len = DL_SUBS_BIND_REQ_SIZE + sizeof(sco_snap_sap_t);
	primptr.maxlen = DL_SUBS_BIND_REQ_SIZE + sizeof(sco_snap_sap_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_subs_bind_ack_t *)primptr.buf;
	if (bap->dl_primitive == DL_ERROR_ACK) {
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_unbind(int fd)
{
	int ret, flags = 0;
	register dl_unbind_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;

	/* set up the DL_unbind_req */
	brp = (dl_unbind_req_t *)primptr.buf;
	brp->dl_primitive = DL_UNBIND_REQ;
	primptr.len = DL_UNBIND_REQ_SIZE;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive != DL_OK_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_addmca(int fd, uchar *mca)
{
	int ret, flags = 0;
	register dl_enabmulti_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;
	uchar *cp;

	/* set up the DL_enabmulti_req */
	brp = (dl_enabmulti_req_t *)primptr.buf;
	memset(brp, 0, DL_ENABMULTI_REQ_SIZE);
	brp->dl_primitive = DL_ENABMULTI_REQ;
	brp->dl_addr_length = ADDR_LEN;
	brp->dl_addr_offset = DL_ENABMULTI_REQ_SIZE;
	cp = (uchar *)brp;
	cp += DL_ENABMULTI_REQ_SIZE;
	memcpy(cp, mca, ADDR_LEN);
	primptr.len = DL_ENABMULTI_REQ_SIZE + ADDR_LEN;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive != DL_OK_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_delmca(int fd, uchar *mca)
{
	int ret, flags = 0;
	register dl_disabmulti_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;
	uchar *cp;

	/* set up the DL_disabmulti_req */
	brp = (dl_disabmulti_req_t *)primptr.buf;
	memset(brp, 0, DL_DISABMULTI_REQ_SIZE);
	brp->dl_primitive = DL_DISABMULTI_REQ;
	brp->dl_addr_length = ADDR_LEN;
	brp->dl_addr_offset = DL_DISABMULTI_REQ_SIZE;
	cp = (uchar *)brp;
	cp += DL_DISABMULTI_REQ_SIZE;
	memcpy(cp, mca, ADDR_LEN);
	primptr.len = DL_DISABMULTI_REQ_SIZE + ADDR_LEN;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive != DL_OK_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_getmca(int fd)
{
	int ret, flags = 0;
	register dl_mctable_req_t *brp;
	register dl_mctable_ack_t *bap;
	dl_error_ack_t *bep;
	uchar *cp;
	uchar buf[ADDR_LEN*TAB_SIZ + sizeof(dl_mctable_ack_t)];

	brp = (dl_mctable_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_mctable_req_t));
	brp->dl_primitive = DL_MCTABLE_REQ;
	primptr.len = sizeof(dl_mctable_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf) + (ADDR_LEN*TAB_SIZ);
	primptr.buf = (char *)buf;
	ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags);

	primptr.buf = (char *)&primbuf;

	if (ret < 0)
		return(0);

	bap = (dl_mctable_ack_t *)buf;
	if (bap->dl_primitive != DL_MCTABLE_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	cp = buf + bap->dl_mctable_offset;
	return(tablecmp(cp, bap->dl_mctable_len));
}

int
dlpi_enaballmca(int fd)
{
	int ret, flags = 0;
	register dl_enaballmulti_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;
	uchar *cp;

	/* set up the DL_enaballmulti_req */
	brp = (dl_enaballmulti_req_t *)primptr.buf;
	memset(brp, 0, DL_ENABALLMULTI_REQ_SIZE);
	brp->dl_primitive = DL_ENABALLMULTI_REQ;
	primptr.len = DL_ENABALLMULTI_REQ_SIZE;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive != DL_OK_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_disaballmca(int fd)
{
	int ret, flags = 0;
	register dl_disaballmulti_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;
	uchar *cp;

	/* set up the DL_disaballmulti_req */
	brp = (dl_disaballmulti_req_t *)primptr.buf;
	memset(brp, 0, DL_DISABALLMULTI_REQ_SIZE);
	brp->dl_primitive = DL_DISABALLMULTI_REQ;
	primptr.len = DL_DISABALLMULTI_REQ_SIZE;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive != DL_OK_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_setaddr(int fd, uchar *addr)
{
	int ret, flags = 0;
	register dl_set_phys_addr_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;
	uchar	*cp;
	uchar	buf[80];		/* place for received table */
	char	msg[80];		/* place to build addr */

	brp = (dl_set_phys_addr_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_set_phys_addr_req_t));
	brp->dl_primitive = DL_SET_PHYS_ADDR_REQ;
	brp->dl_addr_length = ADDR_LEN;
	brp->dl_addr_offset = DL_SET_PHYS_ADDR_REQ_SIZE;
	cp = (uchar *)brp;
	cp += DL_SET_PHYS_ADDR_REQ_SIZE;
	memcpy(cp, addr, ADDR_LEN);
	primptr.len = DL_SET_PHYS_ADDR_REQ_SIZE + ADDR_LEN;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf);
	ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags);

	if (ret < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive != DL_OK_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("prim 0x%x correct_prim %x dl_errno 0x%x", bap->dl_primitive, bap->dl_correct_primitive, bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_getraddr(int fd, uchar *addr)
{
	int ret, flags = 0;
	register dl_phys_addr_req_t *brp;
	register dl_phys_addr_ack_t *bap;
	dl_error_ack_t *bep;
	uchar	*cp;
	uchar	buf[80];		/* place for received table */
	char	msg[80];		/* place to build addr */

	brp = (dl_phys_addr_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_phys_addr_req_t));
	brp->dl_primitive = DL_PHYS_ADDR_REQ;
	brp->dl_addr_type = DL_FACT_PHYS_ADDR;
	primptr.len = sizeof(dl_phys_addr_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf) + (ADDR_LEN*TAB_SIZ);
	primptr.buf = (char *)buf;
	ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags);

	primptr.buf = (char *)&primbuf;

	if (ret < 0)
		return(0);

	bap = (dl_phys_addr_ack_t *)buf;
	if (bap->dl_primitive != DL_PHYS_ADDR_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	cp = buf + bap->dl_addr_offset;
	if (addr)
		memcpy(addr, cp, ADDR_LEN);
	else {
		sprintf(msg, "%02x%02x%02x%02x%02x%02x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
		varout("addr", msg);
	}
	return(1);
}

int
dlpi_getaddr(int fd, uchar *addr)
{
	int ret, flags = 0;
	register dl_phys_addr_req_t *brp;
	register dl_phys_addr_ack_t *bap;
	dl_error_ack_t *bep;
	uchar	*cp;
	uchar	buf[80];		/* place for received table */
	char	msg[80];		/* place to build addr */

	brp = (dl_phys_addr_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_phys_addr_req_t));
	brp->dl_primitive = DL_PHYS_ADDR_REQ;
	brp->dl_addr_type = DL_CURR_PHYS_ADDR;
	primptr.len = sizeof(dl_phys_addr_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	primptr.maxlen = primptr.len = sizeof(primbuf) + (ADDR_LEN*TAB_SIZ);
	primptr.buf = (char *)buf;
	ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags);

	primptr.buf = (char *)&primbuf;

	if (ret < 0)
		return(0);

	bap = (dl_phys_addr_ack_t *)buf;
	if (bap->dl_primitive != DL_PHYS_ADDR_ACK) {
		/* presumably DL_ERROR_ACK */
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	cp = buf + bap->dl_addr_offset;
	if (addr)
		memcpy(addr, cp, ADDR_LEN);
	else {
		sprintf(msg, "%02x%02x%02x%02x%02x%02x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
		varout("addr", msg);
	}
	return(1);
}

int
dlpi_writeframe(int fd, uchar *frame, int len)
{
	dataptr.buf = (char *)frame;
	dataptr.len = len;
	if (putmsg(fd, NULL, &dataptr, 0) < 0)
		return(0);
	return(dlpi_chkstrerr(fd));
}

/*
 * Return 1 for success or zero for fail
 */
int
dlpi_txframe(int fd, uchar *frame, int len)
{
	register dl_unitdata_req_t *urp;
	int addrlen;

	urp = (dl_unitdata_req_t *)primptr.buf;
	urp->dl_primitive = DL_UNITDATA_REQ;

	memcpy((uchar *)urp + DL_UNITDATA_REQ_SIZE, cmd.c_ourdstaddr, ADDR_LEN);

	addrlen = ADDR_LEN;
	if ((cmd.c_framing == F_LLC1_802_3) || (cmd.c_framing == F_LLC1_802_5)) {
		addrlen++;
		*((uchar *)urp + DL_UNITDATA_REQ_SIZE + ADDR_LEN) = cmd.c_sap;
	}

	urp->dl_dest_addr_length = addrlen;
	urp->dl_dest_addr_offset = DL_UNITDATA_REQ_SIZE;
	primptr.len = DL_UNITDATA_REQ_SIZE + addrlen;
	primptr.maxlen = 0;

	dataptr.buf = (char *)frame;
	dataptr.len = len;
	if (putmsg(fd, &primptr, &dataptr, 0) < 0)
		return(0);
	return(dlpi_chkstrerr(fd));
}

/*
 * Return the recvd byte count or -1 if error
 */
int
dlpi_rxframe(int fd, uchar *frame, int maxlen)
{
	int flags;

	primptr.len = sizeof(primbuf);
	primptr.maxlen = sizeof(primbuf);

	dataptr.buf = (char *)frame;
	dataptr.len = maxlen;
	dataptr.maxlen = maxlen;

	flags = 0;

	if (getmsg(fd, &primptr, &dataptr, &flags) < 0)
		return(-1);
	return(dataptr.len);
}

int
dlpi_sendxid(int fd, int pfbit)
{
	int ret, flags = 0;
	register dl_xid_req_t *brp;
	register dl_xid_res_t *bap;
	dl_error_ack_t *bep;
	int addrlen;

	/* set up the DL_xid_req */
	brp = (dl_xid_req_t *)primptr.buf;

	memset(brp, 0, sizeof(dl_xid_req_t));
	brp->dl_primitive = DL_XID_REQ;
	brp->dl_flag = pfbit;

	memcpy((uchar *)brp + DL_XID_REQ_SIZE, cmd.c_ourdstaddr, ADDR_LEN);

	addrlen = ADDR_LEN;
	if (cmd.c_framing == F_LLC1_802_3 || cmd.c_framing == F_LLC1_802_5) {
		addrlen++;
		*((uchar *)brp + DL_XID_REQ_SIZE + ADDR_LEN) = cmd.c_sap;
	}

	brp->dl_dest_addr_length = addrlen;
	brp->dl_dest_addr_offset = DL_XID_REQ_SIZE;
	primptr.len = DL_XID_REQ_SIZE + addrlen;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_xid_res_t *)primptr.buf;
	if (bap->dl_primitive == DL_ERROR_ACK) {
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_flush(int fd)
{
	if (ioctl(fd, I_FLUSH, FLUSHRW) < 0)
		return(0);
	return(1);
}

int
dlpi_srmode(int fd, int srmode)
{
	int ret, flags = 0;
	register dl_set_srmode_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;

	/* set up the DL_set_srmode */
	brp = (dl_set_srmode_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_set_srmode_req_t));
	brp->dl_primitive = DL_SET_SRMODE_REQ;
	brp->dl_srmode = srmode;
	primptr.len = sizeof(dl_set_srmode_req_t);
	primptr.maxlen = sizeof(dl_set_srmode_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive == DL_ERROR_ACK) {
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}


int
dlpi_srclr(int fd, uchar *addr)
{
	int ret, flags = 0;
	register dl_clr_sr_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;

	uchar *cp;

	brp = (dl_clr_sr_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_clr_sr_req_t));
	brp->dl_primitive = DL_CLR_SR_REQ;

	errlogprf("dlpi_srclr, prim = %x", brp->dl_primitive);

	brp->dl_addr_len = ADDR_LEN;
	brp->dl_addr_offset = DL_CLR_SR_REQ_SIZE;
	cp = (uchar *)brp;
	cp += DL_CLR_SR_REQ_SIZE;
	memcpy(cp, addr, ADDR_LEN);
	primptr.len = DL_CLR_SR_REQ_SIZE + ADDR_LEN;
	primptr.maxlen = DL_CLR_SR_REQ_SIZE + ADDR_LEN;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0) {
		return(0);
	}

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0) {
		return(0);
	}

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive == DL_ERROR_ACK) {
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}


int
dlpi_setsrparms(int fd, uchar *parms)
{
	int ret, flags = 0, i, j;
	register dl_set_srparms_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;

	uchar *cp, *srp;

	brp = (dl_set_srparms_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_set_srparms_req_t));
	brp->dl_primitive = DL_SET_SRPARMS_REQ;
	brp->dl_parms_offset = DL_SET_SRPARMS_REQ_SIZE;
	brp->dl_parms_len = PARMS_LEN;
	cp = (uchar *)brp;
	cp += DL_SET_SRPARMS_REQ_SIZE;

	/*
	 * change byte ordering because each 4 byte will be interpreted
	 * as an integer instead of a character string by the underlying
	 * module.
	 */
	srp = parms;
	for (j=0; j < PARMS_LEN/sizeof(int); j++) {
		for (i=(sizeof(int) - 1); i>=0; --i)
			*cp++ = *(srp + i);
		srp += sizeof(int);
	}

	primptr.len = DL_SET_SRPARMS_REQ_SIZE + PARMS_LEN;
	primptr.maxlen = DL_SET_SRPARMS_REQ_SIZE + PARMS_LEN;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive == DL_ERROR_ACK) {
		bep = (dl_error_ack_t *)bap;
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_biloopmode(int fd, int biloop)
{
	int ret, flags = 0;
	register dl_set_biloopmode_req_t *brp;
	register dl_ok_ack_t *bap;
	dl_error_ack_t *bep;

	/* set up the DL_set_srmode */
	brp = (dl_set_biloopmode_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_set_biloopmode_req_t));
	brp->dl_primitive = DL_DLPI_BILOOPMODE;
	brp->dl_biloopmode = biloop;
	primptr.len = sizeof(dl_set_biloopmode_req_t);
	primptr.maxlen = sizeof(dl_set_biloopmode_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (dl_ok_ack_t *)primptr.buf;
	if (bap->dl_primitive == DL_ERROR_ACK) {
		bep = (dl_error_ack_t *)bap;
		errlogprf("biloopmode dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

/*
 * generic routine to check if an optional error ind was generated
 * failure to obtain next message is considered a success
 * by this routine
 */

static int
dlpi_chkstrerr(int fd)
{
	int n;
	int ret;
	int flags;
	dl_uderror_ind_t *bep;
	struct strpeek sp;

	ret = ioctl(fd, I_NREAD, &n);
	if (ret < 0)
		return(1);
	if (ret == 0)
		return(1);

	/* peek at the message, looking for error ack */
	sp.ctlbuf.maxlen = sizeof(primbuf);
	sp.ctlbuf.len = sizeof(primbuf);
	sp.ctlbuf.buf = (char *)&primbuf;

	sp.databuf.maxlen = 0;
	sp.databuf.len = 0;
	sp.databuf.buf = 0;

	ret = ioctl(fd, I_PEEK, &sp);
	if (ret < 0)
		return(1);

	bep = (dl_uderror_ind_t *)primptr.buf;
	if (bep->dl_primitive != DL_UDERROR_IND)
		return(1);

	/* get the message */
	primptr.len = sizeof(primbuf);
	primptr.maxlen = sizeof(primbuf);

	flags = 0;

	if (getmsg(fd, &primptr, 0, &flags) < 0)
		return(1);
	bep = (dl_uderror_ind_t *)primptr.buf;
	if (bep->dl_primitive == DL_UDERROR_IND) {
		errlogprf("dl_errno 0x%x", bep->dl_errno);
		return(0);
	}
	return(1);
}

int
dlpi_getmedia(int fd)
{
	int ret, flags = 0;
	register dl_get_statistics_req_t *srp;
	register dl_get_statistics_ack_t *sap;
	dl_error_ack_t *sep;
	struct dlpi_stats *ip;
	uchar buf[sizeof(union DL_primitives) + sizeof(struct dlpi_stats)]; 
	uchar dbuf[1024];
	uchar msg[80];		/* place to build msg */

	/* set up the DL_GET_STATISTICS_REQ */
	srp = (dl_get_statistics_req_t *)primptr.buf;
	memset(srp, 0, DL_GET_STATISTICS_REQ_SIZE);
	srp->dl_primitive = DL_GET_STATISTICS_REQ;
	primptr.len = DL_GET_STATISTICS_REQ_SIZE;
	primptr.maxlen = DL_GET_STATISTICS_REQ_SIZE;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(-1);

	/* get the ack */
	primptr.maxlen=primptr.len=sizeof(primbuf) + sizeof(struct dlpi_stats);
	primptr.buf = (char *)buf;	/* temp buf to get control block */

	dataptr.maxlen = dataptr.len = sizeof(dbuf);
	dataptr.buf = (char *)dbuf;	/* temp buf to get data */

	flags = 0;

	if ((ret = getmsg(fd, &primptr, &dataptr, &flags)) < 0)
		return(-1);

	primptr.buf = (char *)&primbuf;	/* reassign to primitve buffer */

	sap = (dl_get_statistics_ack_t *)buf;
	if (sap->dl_primitive != DL_GET_STATISTICS_ACK) {
		sep = (dl_error_ack_t *)sap;
		errlogprf("dl_errno 0x%x", sep->dl_errno);
		return(-1);
	}

	ip = (struct dlpi_stats *)(buf + sap->dl_stat_offset);

	switch(ip->mac_media_type) {
	case DL_CSMACD :
		ret = M_ETHERNET;
		break;
	case DL_TPR :
		ret = M_TOKEN;
		break;
	default :
		error("Unknown media type %d", ip->mac_media_type);
		ret = -1;
		break;
	}
	return(ret);
}

