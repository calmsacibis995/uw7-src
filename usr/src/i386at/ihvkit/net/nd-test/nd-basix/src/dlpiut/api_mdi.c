/*
 * File api_mdi.c
 * Our mdi interface primitives
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) @(#)api_mdi.c	25.1")

#include <sys/types.h>
#include <stdio.h>
#include <stropts.h>
#include <sys/stream.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

#include "sys/mdi.h"
#include "dlpiut.h"

static union MAC_primitives primbuf;

static struct strbuf primptr = {
	0, 0, (char *)&primbuf
};

static struct strbuf dataptr;

typedef int (*qsort_t)(const void *a1, const void *a2);
static int	cmpaddr(const uchar *a1, const uchar *a2);
static int	mdi_chkstrerr(int fd);


int
mdi_bind(int fd, int sap)
{
	int ret, flags = 0;
	struct strbuf bindaddr;
	register mac_bind_req_t *brp;
	register mac_ok_ack_t *bap;
	mac_error_ack_t *bep;

	/* set up the MAC_bind_req */
	brp = (mac_bind_req_t *)primptr.buf;
	brp->mac_primitive = MAC_BIND_REQ;
	brp->mac_cookie = 0;
	primptr.len = MAC_BIND_REQ_SIZE;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(0);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(0);

	bap = (mac_ok_ack_t *)primptr.buf;
	if (bap->mac_primitive != MAC_OK_ACK) {
		/* presumably MAC_ERROR_ACK */
		bep = (mac_error_ack_t *)bap;
		errlogprf("mac_errno 0x%x", bep->mac_errno);
		return(0);
	}
	return(1);
}

int
mdi_addmca(int fd, uchar *mca)
{
	struct strioctl si;
	uchar buf[ADDR_LEN];

	memset(&si, 0, sizeof(struct strioctl));
	memcpy(buf, mca, ADDR_LEN);
	si.ic_cmd = MACIOC_SETMCA;
	si.ic_timout = 0;
	si.ic_dp = (char *)buf;
	si.ic_len = ADDR_LEN;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	return(1);
}

int
mdi_delmca(int fd, uchar *mca)
{
	struct strioctl si;
	uchar buf[ADDR_LEN];

	memset(&si, 0, sizeof(struct strioctl));
	memcpy(buf, mca, ADDR_LEN);
	si.ic_cmd = MACIOC_DELMCA;
	si.ic_timout = 0;
	si.ic_dp = (char *)buf;
	si.ic_len = ADDR_LEN;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	return(1);
}

int
mdi_getmca(int fd)
{
	struct strioctl si;
	uchar buf[ADDR_LEN*TAB_SIZ];

	memset(&si, 0, sizeof(struct strioctl));
	si.ic_cmd = MACIOC_GETMCA;
	si.ic_timout = 0;
	si.ic_dp = (char *)0;
	si.ic_len = ADDR_LEN*TAB_SIZ;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	return(tablecmp(buf, si.ic_len));
}

int
mdi_setallmca(int fd)
{
	struct strioctl si;

	memset(&si, 0, sizeof(struct strioctl));
	si.ic_cmd = MACIOC_SETALLMCA;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	return(1);
}

int
mdi_delallmca(int fd)
{
	struct strioctl si;

	memset(&si, 0, sizeof(struct strioctl));
	si.ic_cmd = MACIOC_DELALLMCA;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	return(1);
}

int
mdi_promisc(int fd)
{
	struct strioctl si;

	memset(&si, 0, sizeof(struct strioctl));
	si.ic_cmd = MACIOC_PROMISC;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	return(1);
}

/*
 * our qsort address comparison routine
 */

static int
cmpaddr(register const uchar *a1, register const uchar *a2)
{
	register int i;

	for (i = 0; i < ADDR_LEN; i++) {
		if (*a1 < *a2)
			return(1);
		if (*a1 > *a2)
			return(-1);
		a1++;
		a2++;
	}
	return(0);
}

/*
 * compare the passed in table with the one in the command structure
 *
 * generate an ERROR if they don't match
 */

int
tablecmp(uchar *ptr, int len)
{
	int l1;
	int l2;

	l1 = len / ADDR_LEN;
	l2 = cmd.c_tabsiz;
	if (l1 == 0 && l2 == 0)
		return(1);
	if (l1 != l2)
		goto nomatch;
	qsort(ptr, l1, ADDR_LEN, (qsort_t)cmpaddr);
	qsort(&cmd.c_table[0][0], l2, ADDR_LEN, (qsort_t)cmpaddr);
	if (memcmp(ptr, &cmd.c_table[0][0], len))
		goto nomatch;
	return(1);

nomatch:
	error("multicast table not correct");
	tableout(ptr, len);
	return(1);
}

int
mdi_setaddr(int fd, uchar *addr)
{
	struct strioctl si;
	uchar buf[ADDR_LEN];

	memset(&si, 0, sizeof(struct strioctl));
	memcpy(buf, addr, ADDR_LEN);
	si.ic_cmd = MACIOC_SETADDR;
	si.ic_timout = 0;
	si.ic_dp = (char *)buf;
	si.ic_len = ADDR_LEN;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	return(1);
}

int
mdi_getraddr(int fd, uchar *addr)
{
	struct strioctl si;
	uchar	buf[ADDR_LEN];
	char	msg[80];		/* place to build output address */

	memset(&si, 0, sizeof(struct strioctl));
	si.ic_cmd = MACIOC_GETRADDR;
	si.ic_timout = 0;
	si.ic_dp = (char *)buf;
	si.ic_len = ADDR_LEN;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	if (addr)
		memcpy((char *)addr, buf, ADDR_LEN);
	else {
		sprintf(msg, "%02x%02x%02x%02x%02x%02x",
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		varout("addr", msg);
	}
	return(1);
}

int
mdi_getaddr(int fd, uchar *addr)
{
	struct strioctl si;
	uchar	buf[ADDR_LEN];
	char	msg[80];		/* place to build output address */

	memset(&si, 0, sizeof(struct strioctl));
	si.ic_cmd = MACIOC_GETADDR;
	si.ic_timout = 0;
	si.ic_dp = (char *)buf;
	si.ic_len = ADDR_LEN;

	if (ioctl(fd, I_STR, &si) < 0)
		return(0);
	if (addr)
		memcpy((char *)addr, buf, ADDR_LEN);
	else {
		sprintf(msg, "%02x%02x%02x%02x%02x%02x",
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		varout("addr", msg);
	}
	return(1);
}

/*
 * Return 1 for success or zero for fail
 */
int
mdi_txframe(int fd, uchar *frame, int len)
{
	if (write(fd, frame, len) < 0)
		return(0);
	return(mdi_chkstrerr(fd));
}

/*
 * Return the recvd byte count or -1 if error
 */
int
mdi_rxframe(int fd, uchar *frame, int maxlen)
{
	int flags;
	int ret;

	primptr.len = sizeof(primbuf);
	primptr.maxlen = sizeof(primbuf);

	dataptr.buf = (char *)frame;
	dataptr.len = maxlen;
	dataptr.maxlen = maxlen;

	flags = 0;

	if ((ret = getmsg(fd, &primptr, &dataptr, &flags)) != 0)
		return(-1);
	return(dataptr.len);
}

/*
 * generic routine to check if an optional error ind was generated
 * failure to obtain next message is considered a success
 * by this routine
 */

static int
mdi_chkstrerr(int fd)
{
	int n;
	int ret;
	int flags;
	mac_error_ack_t *bep;
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

	bep = (mac_error_ack_t *)primptr.buf;
	if (bep->mac_primitive != MAC_ERROR_ACK)
		return(1);

	/* get the message */
	primptr.len = sizeof(primbuf);
	primptr.maxlen = sizeof(primbuf);

	flags = 0;

	if (getmsg(fd, &primptr, 0, &flags) < 0)
		return(1);
	bep = (mac_error_ack_t *)primptr.buf;
	if (bep->mac_primitive == MAC_ERROR_ACK) {
		errlogprf("mac_errno 0x%x", bep->mac_errno);
		return(0);
	}
	return(1);
}

int
mdi_flush(int fd)
{
	if (ioctl(fd, I_FLUSH, FLUSHRW) < 0)
		return(0);
	return(1);
}

int
mdi_getmedia(int fd)
{
	int ret, flags = 0;
	register mac_info_req_t *irp;
	register mac_info_ack_t *iap;
	mac_error_ack_t *iep;
	uchar msg[80];		/* place to build msg */

	/* set up the MAC_info_req */
	irp = (mac_info_req_t *)primptr.buf;
	irp->mac_primitive = MAC_INFO_REQ;
	primptr.len = MAC_INFO_REQ_SIZE;
	primptr.maxlen = MAC_INFO_REQ_SIZE;

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(-1);

	/* get the ack */
	primptr.maxlen = primptr.len = sizeof(primbuf);
	if ((getmsg(fd, &primptr, (struct strbuf *)0, &flags)) < 0)
		return(-1);

	iap = (mac_info_ack_t *)primptr.buf;
	if (iap->mac_primitive != MAC_INFO_ACK) {
		/* presumably MAC_ERROR_ACK */
		iep = (mac_error_ack_t *)iap;
		errlogprf("mac_errno 0x%x", iep->mac_errno);
		return(-1);
	}

	switch(iap->mac_mac_type) {
	case MAC_CSMACD :
		ret = M_ETHERNET;
		break;
	case MAC_TPR :
		ret = M_TOKEN;
		break;
	default :
		error("Unknown media type %d", iap->mac_mac_type);
		ret = -1;
		break;
	}
	return(ret);
}

