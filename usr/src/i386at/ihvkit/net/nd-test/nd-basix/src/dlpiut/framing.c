/*
 * File framing.c
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) framing.c 11.1 95/05/01 SCOINC")

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <stropts.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>

#include <sys/scodlpi.h>

#include "dlpiut.h"

static uchar	txframebuf[MAXFRAME];
static int		txframesz;

static uchar	rxframebuf[MAXFRAME];
static int		rxframesz;

static uchar	rxiframebuf[MAXFRAME];
static int		rxiframesz;

static int	padframe(fd_t *fp, uchar *buf, int len);
static int	frame_type_check(fd_t *fp);
static int	build_txhdr(fd_t *fp, int len);
static int	build_rxhdr(fd_t *fp, int len);
static void	dumpframe(uchar *buf, int len);
static void	sigcheck(int x);
static int	frame_cmp(fd_t *fp, uchar *buf, int len, int offset,
												int actual_len, int silent);

int
build_txmsg(register fd_t *fp, register char *msg, int type)
{
	int len;
	int offset;

	len = strlen(msg) + 1;
	if (frame_type_check(fp) == 0)
		return(0);
	offset = build_txhdr(fp, len);
	if (offset < 0)
		return(0);
	txframesz = offset + len + 1;
	txframebuf[offset] = type;
	memcpy(txframebuf + offset + 1, msg, len);
	return(1);
}

int
build_txpattern(register fd_t *fp, int len)
{
	int offset;

	if (frame_type_check(fp) == 0)
		return(0);
	offset = build_txhdr(fp, len);
	if (offset < 0)
		return(0);
	txframesz = offset + len;
	makepattern(txframebuf + offset, len);
	return(1);
}

#ifdef NOT_USED
int
build_rxmsg(register fd_t *fp, register uchar *msg)
{
	int len;
	int offset;

	len = strlen(msg) + 1;
	if (frame_type_check(fp) == 0)
		return(0);

	if (getaddr(fp) == 0)
		return(0);
 
	offset = build_rxhdr(fp, len);
	if (offset < 0)
		return(0);
	rxframesz = offset + len;
	memcpy(rxframebuf + offset, msg, len);

	rxframesz = padframe(fp, rxframebuf, rxframesz);
	return(1);
}
#endif	/* NOT_USED */

int
build_rxpattern(register fd_t *fp, int len)
{
	int offset;

	if (frame_type_check(fp) == 0)
		return(0);


	if (getaddr(fp) == 0)
		return(0);

	offset = build_rxhdr(fp, len);
	if (offset < 0)
		return(0);
	rxframesz = offset + len;
	makepattern(rxframebuf + offset, len);

	rxframesz = padframe(fp, rxframebuf, rxframesz);
	return(1);
}

static int
padframe(register fd_t *fp, uchar *buf, int len)
{
	int min;

	/* add padding if needed (MDI always, DLPI only on ethernet frames) */
	if (fp->f_media == M_ETHERNET &&
		(fp->f_interface == I_MDI || cmd.c_framing == F_ETHERNET)) {

		min = (fp->f_interface == I_MDI) ? EMIN : (EMIN - 14);
		if (len < min) {
			memset(buf + len, 0, min - len);
			len = min;
		}
	}	

	return(len);
}

/*
 * verify framing is valid for media type
 *
 * user's job to match bind type with framing type.
 * this allows us to pass bogus data.
 */

static int
frame_type_check(register fd_t *fp)
{
	static uchar f_ethernet[] =
				{ F_ETHERNET, F_802_3, F_XNS, F_LLC1_802_3, F_SNAP_802_3 };
	static uchar f_token[] = { F_802_5, F_SNAP_802_5, F_LLC1_802_5 };
	register uchar *media;
	int size;
	register int i;

	switch (fp->f_media)
	{
		case M_ETHERNET:
			media = f_ethernet;
			size = sizeof(f_ethernet);
			break;
		case M_TOKEN:
			media = f_token;
			size = sizeof(f_token);
			break;
		default:
			errlogprf("unknown media");
			return(0);
	}
	for (i = 0; i < size; i++)
		if (cmd.c_framing == media[i])
			return(1);
	errlogprf("Invalid framing type = %d", cmd.c_framing);
	return(0);
}

/*
 * build the transmit frame header based on the framing and interface types
 * the framing and media types are assumed to be compatible.
 *
 * length is data only, after both the mdi and dlpi headers are built up.
 * see the LLI everest framing reference for a definition of data sizes
 */

static int
build_txhdr(register fd_t *fp, int len)
{
	int offset;
	int rlen;
	register uchar *out;

	out = txframebuf;
	offset = 0;

	/*
	 * media specific header first, only needed for MDI interface
	 */
	if (getaddr(fp) == 0)
		return(-1);

       if ((fp->f_interface == I_MDI) || (cmd.c_biloop)) {
		rlen = cmd.c_route[0] & 0x1f;
		switch (cmd.c_framing) {
		case F_ETHERNET:
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = cmd.c_sap>>8;
			*out++ = cmd.c_sap;
			break;
		case F_802_3:
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 2)>>8;
			*out++ = len + 2;
			break;
		case F_LLC1_802_3:
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 3)>>8;
			*out++ = len + 3;
			*out++ = cmd.c_sap;
			*out++ = cmd.c_sap;
			*out++ = MAC_UI;
			break;
		case F_SNAP_802_3:
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 8)>>8;
			*out++ = len + 8;
			*out++ = SNAP_SAP;
			*out++ = SNAP_SAP;
			*out++ = MAC_UI;

			*out++ = 0;	/* our protocol id */
			*out++ = 0;
			*out++ = 0;

			*out++ = cmd.c_sap>>8;
			*out++ = cmd.c_sap;
			break;
		case F_XNS:
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 2)>>8;
			*out++ = len + 2;
			break;
		case F_802_5:
			*out++ = MAC_AC;
			*out++ = MAC_FC_LLC;
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			out += ADDR_LEN;
			break;
		case F_LLC1_802_5:
			*out++ = MAC_AC;
			*out++ = MAC_FC_LLC;
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			if (rlen)
				*out = *out | 0x80;
			out += ADDR_LEN;
			if (rlen) {
				memcpy(out, cmd.c_route, rlen);
				out += rlen;
			}
			*out++ = cmd.c_sap;
			*out++ = cmd.c_sap;
			*out++ = MAC_UI;
			break;
		case F_SNAP_802_5:
			*out++ = MAC_AC;
			*out++ = MAC_FC_LLC;
			memcpy(out, cmd.c_ourdstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, fp->f_ouraddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = SNAP_SAP;
			*out++ = SNAP_SAP;
			*out++ = MAC_UI;

			*out++ = 0;	/* our protocol id */
			*out++ = 0;
			*out++ = 0;

			*out++ = cmd.c_sap>>8;
			*out++ = cmd.c_sap;
			break;
		}
	}

	switch (cmd.c_framing) {
	case F_ETHERNET:
		break;
	case F_802_3:
		*out++ = cmd.c_sap;
		*out++ = cmd.c_sap;
		break;
	case F_LLC1_802_3:
		break;
	case F_XNS:
		*out++ = 0xff;		/* xns chksum */
		*out++ = 0xff;
		break;
	case F_802_5:
		*out++ = cmd.c_sap;
		*out++ = cmd.c_sap;
		*out++ = MAC_UI;
		break;
	case F_SNAP_802_3:
		break;
	case F_SNAP_802_5:
		break;
	case F_LLC1_802_5:
		break;
	}

	offset = out - txframebuf;
	return(offset);
}

/*
 * build the expected receive frame header based on the framing and
 * interface types.
 * the framing and media types are assumed to be compatible.
 *
 * length is data only, after both the mdi and dlpi headers are built up.
 * see the LLI everest framing reference for a definition of data sizes.
 */

static int
build_rxhdr(register fd_t *fp, int len)
{
	int rlen;
	int offset;
	register uchar *out;
	out = rxframebuf;
	offset = 0;

       if ((fp->f_interface == I_MDI) || (cmd.c_biloop)) {
		rlen = cmd.c_route[0] & 0x1f;
		switch (cmd.c_framing) {
		case F_ETHERNET:
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = cmd.c_sap>>8;
			*out++ = cmd.c_sap;
			break;
		case F_802_3:
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 2)>>8;
			*out++ = len + 2;
			break;
		case F_LLC1_802_3:
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 3)>>8;
			*out++ = len + 3;
			*out++ = cmd.c_sap;
			*out++ = cmd.c_sap;
			*out++ = MAC_UI;
			break;
		case F_SNAP_802_3:
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 8)>>8;
			*out++ = len + 8;
			*out++ = SNAP_SAP;
			*out++ = SNAP_SAP;
			*out++ = MAC_UI;

			*out++ = 0;	/* our protocol id */
			*out++ = 0;
			*out++ = 0;

			*out++ = cmd.c_sap>>8;
			*out++ = cmd.c_sap;
			break;
		case F_XNS:
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = (len + 2)>>8;
			*out++ = len + 2;
			break;
		case F_802_5:
			*out++ = MAC_AC;
			*out++ = MAC_FC_LLC;
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			out += ADDR_LEN;
			break;
		case F_LLC1_802_5:
			*out++ = MAC_AC;
			*out++ = MAC_FC_LLC;
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			if (rlen)
				*out = *out | 0x80;
			out += ADDR_LEN;
			if (rlen) {
				memcpy(out, cmd.c_route, rlen);
				out += rlen;
			}
			*out++ = cmd.c_sap;
			*out++ = cmd.c_sap;
			*out++ = MAC_UI;
			break;
		case F_SNAP_802_5:
			*out++ = MAC_AC;
			*out++ = MAC_FC_LLC;
			memcpy(out, cmd.c_odstaddr, ADDR_LEN);
			out += ADDR_LEN;
			memcpy(out, cmd.c_omchnaddr, ADDR_LEN);
			out += ADDR_LEN;
			*out++ = SNAP_SAP;
			*out++ = SNAP_SAP;
			*out++ = MAC_UI;

			*out++ = 0;	/* our protocol id */
			*out++ = 0;
			*out++ = 0;

			*out++ = cmd.c_sap>>8;
			*out++ = cmd.c_sap;
			break;
		}
	}

	switch (cmd.c_framing) {
	case F_ETHERNET:
		break;
	case F_802_3:
		*out++ = cmd.c_sap;
		*out++ = cmd.c_sap;
		break;
	case F_LLC1_802_3:
		break;
	case F_XNS:
		*out++ = 0xff;		/* xns chksum */
		*out++ = 0xff;
		break;
	case F_802_5:
		*out++ = cmd.c_sap;
		*out++ = cmd.c_sap;
		*out++ = MAC_UI;
		break;
	case F_SNAP_802_3:
		break;
	case F_SNAP_802_5:
		break;
	case F_LLC1_802_5:
		break;
	}

	offset = out - rxframebuf;
	return(offset);
}

/*
 * find out our source address
 */

int
getaddr(register fd_t *fp)
{
	int ret;

	if (fp->f_addrgood)
		return(1);

	switch (fp->f_interface) {
		case I_DLPI:
			ret = dlpi_getaddr(fp->f_fd, fp->f_ouraddr);
			break;
		case I_MDI:
			ret = mdi_getaddr(fp->f_fd, fp->f_ouraddr);
			break;
	}
	if (ret)
		fp->f_addrgood = 1;
	else
		error("Get address failed, cannot send/recv data");
	return(ret);
}

static void
dumpframe(uchar *buf, int len)
{
	if (len == 0) {
		errlogprf("Empty frame");
		return;
	}
	dump(buf, len);
}

static void
sigcheck(int x)
{
	alarm(0);
}

/*
 * Transmit a frame and handle loopback if needed
 * blows away receive frame buffer if loopback is enabled
 *
 * Return 1 for success or zero for fail
 */

int
txframe(register fd_t *fp)
{
	int ret;
	register uchar *cp;
	register uchar *cpi;
	int i;

	if (fp->f_interface == I_MDI)
		ret = mdi_txframe(fp->f_fd, txframebuf, txframesz);
	else
		ret = dlpi_txframe(fp->f_fd, txframebuf, txframesz);

	if ((cmd.c_loop == 0) ||	/* loopback frames are not expected */
		(ret == 0))				/* {mdi,dlpi}_txframe() failed */
		return(ret);

	/*
	 * Get and verify loopback frame, no padding is assumed.
	 * We are expecting that the loopback frame will arrive
	 * prior to the frame from accross the net.  There may
	 * however be unexpected packets on the stream, left
	 * over from previous operations.  Ignore them.
	 */

#ifdef OLD_WAY
	ret = rxframe(fp, SMALL_TIMEOUT);
	if (ret < 0) {
		errlogprf("loopback frame not received.");
		return(0);
	}
	txframesz = padframe(fp, txframebuf, txframesz);
	ret = frame_cmp(fp, txframebuf, txframesz, 0, txframesz, 0);
	if (ret == 0)
		errlogprf("Loopback frame was the problem");
	return(ret);
#else
	do
	{
		/*
		 * Note: the frame may be zero length here
		 * It is OK for rxframe to return zero
		 */
		if (rxframe(fp, SMALL_TIMEOUT) < 0)
		{
			errlogprf("Loopback frame not received.");
			return(0);
		}
		txframesz = padframe(fp, txframebuf, txframesz);
	} while (frame_cmp(fp, txframebuf, txframesz, 0, txframesz, 1) == 0);

	return(1);
#endif
}

/*
 * Return the recvd byte count or -1 if error
 */
int
rxframe(register fd_t *fp, int timeout)
{
	int ret;
	int i;

	signal(SIGALRM, sigcheck);
	alarm(timeout);
	if (fp->f_interface == I_MDI)
		ret = mdi_rxframe(fp->f_fd, rxiframebuf, MAXFRAME);
	else
		ret = dlpi_rxframe(fp->f_fd, rxiframebuf, MAXFRAME);
	rxiframesz = ret;
	alarm(0);
	return(ret);
}

void
outrxmsg(register fd_t *fp, int silent)
{
	uchar *cp;
	int offset;

	if (silent)
		return;
	offset = rxdataoffset(fp) + 1;
	cp = rxiframebuf + offset;
	varout("msg", (char *)cp);
}

/*
 * Verify that the message is the expected type T_SENDER/T_RECEIVER
 * Return 1 if it is the correct type.
 */

int
chkrxmsg(register fd_t *fp, int type)
{
	uchar *cp;
	int offset;

	offset = rxdataoffset(fp);
	cp = rxiframebuf + offset;
	if (*cp != type) {
		/*
		 * This routine is only used during syncup so we ignore
		 * errors due to bogus messages.  Confusion may have
		 * been placed on the net by preceeding test failures.
		 *
		 * error("two senders or masters, please reconfigure");
		 * dumpframe(rxiframebuf, rxiframesz);
		 */
		return(0);
	}
	return(1);
}

/*
 * Return 1 for match
 */
int
matchrxmsg(register fd_t *fp, int len)
{
	uchar *cp;
	int offset;

	offset = rxdataoffset(fp);
	cp = rxiframebuf + offset + 1;
	if (strncmp((char *)cp, cmd.c_msg, len - offset - 1) != 0)
		return(0);
	return(1);
}

uchar *
getrxdataptr(register fd_t *fp)
{
	uchar *cp;
	int offset;

	offset = rxdataoffset(fp);
	cp = rxiframebuf + offset;
	return(cp);
}

uchar *
gettxdataptr(register fd_t *fp)
{
	uchar *cp;
	int offset;

	offset = rxdataoffset(fp);
	cp = txframebuf + offset;
	return(cp);
}

int
getrxdatalen(register fd_t *fp)
{
	int offset;
	int len;

	offset = rxdataoffset(fp);
	len = rxiframesz - offset;
	return(len);
}

int
rxdataoffset(register fd_t *fp)
{
	int offset;

	offset = 0;
	if (fp->f_interface == I_MDI) {
		switch (cmd.c_framing) {
		case F_ETHERNET:
			offset += 14;
			break;
		case F_802_3:
			offset += 14;
			break;
		case F_LLC1_802_3:
			offset += 17;
			break;
		case F_SNAP_802_3:
			offset += 22;
			break;
		case F_XNS:
			offset += 14;
			break;
		case F_802_5:
			offset += 14;
			break;
		case F_LLC1_802_5:
			offset += 17;
			break;
		case F_SNAP_802_5:
			offset += 22;
			break;
		}
	}
	switch (cmd.c_framing) {
	case F_802_3:
		offset += 2;
		break;
	case F_802_5:
		offset += 3;
		break;
	case F_XNS:
		offset += 2;
		break;
	}
	return(offset);
}

/*
 * given our media and framing type, calculate the range of valid frame sizes
 */

void
framesize(register fd_t *fp, int *min, int *max)
{
	int save;

	/* get our low minimum, always base it on complete MDI frame format */
	save = fp->f_interface;
	fp->f_interface = I_MDI;
	*min = rxdataoffset(fp);
	fp->f_interface = save;

	switch (fp->f_media) {
	case M_ETHERNET:
		*max = 1514;
		break;
	case M_TOKEN:
		*max = 2023;
		break;
	}
	*max -= *min;
	*min = 0;
}

void
makepattern(register uchar *cp, int len)
{
	register int c;

	c = 'a';
	while (len-- > 0) {
		*cp++ = c++;
		if (c > 'z')
			c = 'a';
	}
}

/*
 * build expected frame image in rxiframe
 */

int
frame_pcmp(fd_t *fp, int len)
{
	if (build_rxpattern(fp, len) == 0)
		return(0);

	return frame_cmp(fp, rxframebuf, rxframesz, 0, len, 0);
}

/*
 * do not build image, just check it
 */

int
frame_lcmp(fd_t *fp, int len, int offset)
{
	return frame_cmp(fp, rxframebuf, rxframesz, offset, len, 0);
}

/*
 * do a compare of the rxframe v.s. expected frame
 * the expected frame is passed in
 * dump problems to errlog in a readable form
 *
 * Return 1 if they compare correctly
 */

static int
frame_cmp(fd_t *fp, uchar *buf, int len, int offset, int actual_len, int silent)
{
	uchar *ibp;
	register uchar *cp;
	register uchar *cpi;
	int i;
	int save_len;

	if (len != rxiframesz) {
		if (!silent)
		{
			errlogprf("Expected frame size %d, got %d", len, rxiframesz);
			errlogprf("Desired frame (len = %d):", len);
			dumpframe(buf, len);
			errlogprf("Actual frame (len = %d):", rxiframesz);
			dumpframe(rxiframebuf, rxiframesz);
		}
		return(0);
	}

	save_len = len;
	if (fp->f_media == M_ETHERNET 
	    && (fp->f_interface == I_MDI || cmd.c_framing == F_ETHERNET))
	    len = actual_len; /* only bother to compare unpadded data */

	if (len == 0)
		return(1);

	ibp = rxiframebuf + offset;
	buf += offset;
	if (fp->f_media == M_TOKEN && fp->f_interface == I_MDI) {
		buf++;
		ibp++;
		len--;
	}

	/* fast compare for speed */
	if (memcmp(buf, ibp, len) == 0)
		return(1);

	if (silent)
		return(0);

	/* find error */
	len = save_len;
	cp = buf;
	cpi = ibp;
	for (i = 0; i < len; i++) {
		if (*cp != *cpi)
			break;
		cp++;
		cpi++;
	}

	errlogprf("Frame mismatch occurred at displacement %d", i);
	errlogprf("Desired frame (len = %d):", len);
	dumpframe(buf, len);
	errlogprf("Actual frame (len = %d):", len);
	dumpframe(ibp, len);
	return(0);
}

int
api_bind(register fd_t *fp, int sap)
{
	int ret;

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_bind(fp->f_fd, sap);
		break;
	case I_MDI:
		ret = mdi_bind(fp->f_fd, sap);
		break;
	}
	return(ret);
}

#ifdef NOT_USED
int
api_unbind(register fd_t *fp)
{
	int ret;

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_unbind(fp->f_fd);
		if (ret == 0)
			error("Unbind Failed, errno %d", errno);
		break;
	case I_MDI:
		ret = 1;
		break;
	}

	return(ret);
}
#endif	/* NOT_USED */

/*
 * flush any receive messages in the q
 */

void
api_flush(register fd_t *fp)
{
	switch (fp->f_interface)
	{
		case I_DLPI:
			dlpi_flush(fp->f_fd);
			break;
		case I_MDI:
			mdi_flush(fp->f_fd);
			break;
	}
}

/* 
 * bilooprx - generates a dummy receive frame.  Uses the write system
 * call in order to generate a M_DATA streams message for DLPI.  DLPI will
 * take this message (when in biloopmode) and put it into its lower read 
 * queue for processing as though it were an incoming frame from the driver.
 * The message is changed to PROTO type before it is put, and changed back to 
 * M_DATA by DLPI (when in biloopmode).  At the same time, any other messages
 * (presumably from the driver) are discarded in order to eliminate any outside
 * "noise" that could interfere with the test results.
 * When testing automatic source routing, omchnaddr is the MAC value for the 
 * test entry; odstaddr will be fp->ouraddr (unless it is a broadcast).
 * The route will be used by the test entry.
 *
 * build_rxhdr is used for generating the frame; framing and sap are needed
 * there.
 */
 
int
bilooprx(fd_t *fp, int biloop, int framing, int sap, uchar *odstaddr,
												uchar *omchnaddr, uchar *route)
{
	if (build_rxpattern(fp, BIFRAME) == 0) {
		error("build_rxpattern failed \n");
		return(0);
	}
	errlogprf("writeframe first 80 octets:");
	dumpframe(rxframebuf, 80);
	/* note that rx/tx and buffer types are reversed below */
	if (dlpi_writeframe(fp->f_fd, rxframebuf, rxframesz) < 0)
              return(0);
	errlogprf("trying getmsg for rxframe");

	signal(SIGALRM, sigcheck);
	alarm(2);

	if ((dlpi_rxframe(fp->f_fd, rxframebuf, rxframesz)) <= 0) {
		error("loopback frame not received.");
		alarm(0);
		return(0);
	}

	alarm(0);
	return(1);
}

/*
 * bilooptx - tests the frame that is generated by DLPI by way of looping
 * it back to the upper DLPI read queue (instead of to the write queue of the 
 * driver) when in biloopmode.  The "returned" frame will be checked against
 * an expected frame that is built separately.  The route that is passed is 
 * the route (or indicates the packet type, e.g., ARE or STE) that is expected.
 * ourdstaddr is MAC address for the test entry.
 *
 * build_txhdr is used with these parameters for building the expected frame -
 * that's also where framing and sap are needed.
 * dlpi_txframe is used to send a "normal" message to DLPI.
 */ 

int
bilooptx(fd_t *fp, int biloop, int framing, int sap, uchar *ourdstaddr,
																uchar *route)
{
	uchar 	biloopmsg[BIFRAME +64];
	int	rxsize;
	uchar *rxbp;
	uchar *txbp;

	if (build_txpattern(fp, BIFRAME) == 0) {
		errlogprf("build_txpattern failed");
		return(0);
	}
	makepattern(biloopmsg, BIFRAME);
	if (dlpi_txframe(fp->f_fd, biloopmsg, BIFRAME) == 0) {
		error("dlpi_txframe failed");
		return(0);
	}
	/* the frame should get looped back to the DLPI upper read queue 
	 * after dlpi had added all the header and route info - compare
	 * with the frame generated by 'build_txpattern'.
	 */

	signal(SIGALRM, sigcheck);
        alarm(2);
	if ((rxsize = dlpi_rxframe(fp->f_fd, rxframebuf, MAXFRAME)) <= 0) {
		error("dlpi_rxframe failed");
		alarm(0);
		return(0);
	}
	if (rxsize != txframesz) {
		error("biloop frame size mismatch");
		errlogprf("Expected frame size %d, got %d", rxframesz, rxsize);
		errlogprf("Desired frame:");
		dumpframe(txframebuf, 80);
		errlogprf("Actual frame:");
		dumpframe(rxframebuf, 80);
		alarm(0);
		return(0);
	}
	rxbp = rxframebuf;
	txbp = txframebuf;
 
        rxbp++;
        txbp++;
        rxsize--;

	if (memcmp(rxbp, txbp, rxsize) == 0) {
		alarm(0);
		return(1);
	}
	error("biloop frame contents mismatch\n");
	errlogprf("Desired frame first 80 octets:");
	dumpframe(txbp, 80);
	errlogprf("Actual frame first 80 octets:");
	dumpframe(rxbp, 80);
	alarm(0);
	return(0);
}

