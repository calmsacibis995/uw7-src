/*
 * File execute.c
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) @(#)execute.c	25.1")

#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "dlpiut.h"

static int	c_open(void);
static int	c_close(void);
static int	c_bind(void);
static int	c_sbind(void);
static int	c_unbind(void);
static int	c_addmca(void);
static int	c_delmca(void);
static int	c_getmca(void);
static int	c_getaddr(void);
static int	c_sendloop(void);
static int	c_recvloop(void);
static int	c_send(void);
static int	c_txid(void);
static int	c_rxid(void);
static int	c_ttest(void);
static int	c_rtest(void);
static int	c_sendfile(void);
static int	c_recvfile(void);
static int	c_biloopmode(void);
static int	c_bilooprx(void);
static int	c_bilooptx(void);
static int	c_srclr(void);
static int	c_setsrparms(void);
static int	c_setaddr(void);
static int	c_getraddr(void);
static int	c_setallmca(void);
static int	c_delallmca(void);
static int	c_promisc(void);

/*
 * Execute our command
 * Return true if successful.
 */

int
execute()
{
	switch (cmd.c_cmd)
	{
		case C_OPEN:
			return c_open();
		case C_CLOSE:
			return c_close();
		case C_BIND:
			return c_bind();
		case C_SBIND:
			return c_sbind();
		case C_UNBIND:
			return c_unbind();
		case C_ADDMCA:
			return c_addmca();
		case C_DELMCA:
			return c_delmca();
		case C_GETMCA:
			return c_getmca();
		case C_GETADDR:
			return c_getaddr();
		case C_SYNCSEND:
			return c_syncsend(0);
		case C_SYNCRECV:
			return c_syncrecv(0);
		case C_SENDLOOP:
			return c_sendloop();
		case C_RECVLOOP:
			return c_recvloop();
		case C_SEND:
			return c_send();
		case C_TXID:
			return c_txid();
		case C_RXID:
			return c_rxid();
		case C_TTEST:
			return c_ttest();
		case C_RTEST:
			return c_rtest();
		case C_SRMODE:
			return c_srmode();
		case C_SENDLOAD:
			return c_sendload();
		case C_RECVLOAD:
			return c_recvload();
		case C_SENDFILE:
			return c_sendfile();
		case C_RECVFILE:
			return c_recvfile();
		case C_BILOOPMODE:
			return c_biloopmode();
		case C_BILOOPRX:
			return c_bilooprx();
		case C_BILOOPTX:
			return c_bilooptx();
		case C_SRCLR:
			return c_srclr();
		case C_SETSRPARMS:
			return c_setsrparms();
		case C_SETADDR:
			return c_setaddr();
		case C_GETRADDR:
			return c_getraddr();
		case C_SETALLMCA:
			return c_setallmca();
		case C_DELALLMCA:
			return c_delallmca();
		case C_PROMISC:
			return c_promisc();
	}

	return(0);	/* Fail */
}

static int
c_open()
{
	register fd_t	*fp;
	int				ret;
	char			msg[80];		/* place to build msg */

	for (fp = fds; fp < &fds[MAXFD]; fp++)
		if (fp->f_open == 0)
			break;

	if (fp == &fds[MAXFD]) {
		error("Open failed, no more internal file descriptors");
		return(0);
	}
	fp->f_fd = open(cmd.c_device, 2);
	if (fp->f_fd < 0) {
		error("Unable to open %s", cmd.c_device);
		return(0);
	}
	fp->f_open = 1;
	strcpy(fp->f_name, cmd.c_device);
	fp->f_addrgood = 0;
	sprintf(msg, "%d", fp - fds);
	varout("fd", msg);

	fp->f_interface = cmd.c_interface;
	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_getmedia(fp->f_fd);
		break;
	case I_MDI:
		ret = mdi_getmedia(fp->f_fd);
		break;
	}

	switch (ret)
	{
		case M_ETHERNET :
			fp->f_media = M_ETHERNET;
			sprintf(msg, "%s", "ethernet");
			break;
		case M_TOKEN :
			fp->f_media = M_TOKEN;
			sprintf(msg, "%s", "token");
			break;
		default :
			fp->f_media = M_NONE;
			sprintf(msg, "%s", "none");
			break;
	}

	varout("media", msg);
	return(1);	/* Success */
}

static int
c_close()
{
	register fd_t *fp;

	fp = &fds[cmd.c_fd];
	close(fp->f_fd);
	fp->f_open = 0;
	return 1;
}

static int
c_bind()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];
	ret = api_bind(fp, cmd.c_sap);

	if (cmd.c_error && (ret == 0))
		return(1);
	if ((cmd.c_error) == 0 && ret)
		return(1);

	if (ret == 0)
		error("Bind to %x failed unexpectedly, errno %d", cmd.c_sap, errno);
	else
		error("Bind to %x succeeded unexpectedly", cmd.c_sap);
	return(0);
}

static int
c_sbind()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];
	ret = dlpi_sbind(fp->f_fd, cmd.c_sap);

	if (cmd.c_error && (ret == 0))
		return(1);
	if ((cmd.c_error) == 0 && ret)
		return(1);

	if (ret == 0)
		error("Subs bind to %x failed unexpectedly, errno %d", cmd.c_sap, errno);
	else
		error("Subs bind to %x succeeded unexpectedly", cmd.c_sap);
	return(0);
}

/*
 * I_DLPI interface only
 */

static int
c_unbind()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];
	ret = dlpi_unbind(fp->f_fd);

	if (ret == 0) {
		error("Unbind failed errno %d", errno);
		return(0);
	}
	return(1);
}

static int
c_addmca()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_addmca(fp->f_fd, cmd.c_ourdstaddr);
		break;
	case I_MDI:
		ret = mdi_addmca(fp->f_fd, cmd.c_ourdstaddr);
		break;
	}
	if (cmd.c_error && (ret == 0))
		return(1);
	if ((cmd.c_error) == 0 && ret)
		return(1);

	if (ret == 0)
		error("Addmca failed unexpectedly, errno %d", errno);
	else
		error("Addmca succeeded unexpectedly");
	return(0);
}

static int
c_delmca()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_delmca(fp->f_fd, cmd.c_ourdstaddr);
		break;
	case I_MDI:
		ret = mdi_delmca(fp->f_fd, cmd.c_ourdstaddr);
		break;
	}
	if (cmd.c_error && (ret == 0))
		return(1);
	if ((cmd.c_error) == 0 && ret)
		return(1);

	if (ret == 0)
		error("Delmca failed unexpectedly, errno %d", errno);
	else
		error("Delmca succeeded unexpectedly");
	return(0);
}

static int
c_getmca()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_getmca(fp->f_fd);
		break;
	case I_MDI:
		ret = mdi_getmca(fp->f_fd);
		break;
	}
	if (ret == 0)
		error("Getmca failed, errno %d", errno);
	return(ret);
}

static int
c_setaddr()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_setaddr(fp->f_fd, cmd.c_ourdstaddr);
		break;
	case I_MDI:
		ret = mdi_setaddr(fp->f_fd, cmd.c_ourdstaddr);
		break;
	}
	if (cmd.c_error && (ret == 0))
		return(1);
	if ((cmd.c_error) == 0 && ret)
		return(1);

	if (ret == 0)
		error("Setaddr failed unexpectedly, errno %d", errno);
	else
		error("Setaddr succeeded unexpectedly");
	return(0);
}

static int
c_getraddr()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_getraddr(fp->f_fd, 0);
		break;
	case I_MDI:
		ret = mdi_getraddr(fp->f_fd, 0);
		break;
	}
	if (ret == 0)
		error("Getraddr failed, errno %d", errno);
	return(ret);
}

static int
c_getaddr()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_getaddr(fp->f_fd, 0);
		break;
	case I_MDI:
		ret = mdi_getaddr(fp->f_fd, 0);
		break;
	}
	if (ret == 0)
		error("Getaddr failed, errno %d", errno);
	return(ret);
}

static int
c_setallmca()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_enaballmca(fp->f_fd);
		break;
	case I_MDI:
		ret = mdi_setallmca(fp->f_fd);
		break;
	}
	if (ret == 0)
		error("Setallmca failed, errno %d", errno);
	return(ret);
}

static int
c_delallmca()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		ret = dlpi_disaballmca(fp->f_fd);
		break;
	case I_MDI:
		ret = mdi_delallmca(fp->f_fd);
		break;
	}
	if (ret == 0)
		error("Delallmca failed, errno %d", errno);
	return(ret);
}

int
c_syncsend(int silent)
{
	register fd_t	*fp;
	int				len;
	long			timecur;
	long			timestart;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	if (build_txmsg(fp, cmd.c_msg, T_SENDER) == 0)
		return(0);

	timestart = timecur = time(NULL);
	for ( ; (timecur - timestart) <= cmd.c_timeout; timecur = time(NULL))
	{
		if (txframe(fp) == 0) {
			/*
			 * error("Transmit error, errno %d", errno);
			 * return(0);
			 */
			continue;	/* Transmit error but keep trying */
		}

		if ((len = rxframe(fp, SMALL_TIMEOUT)) <= 0)
			continue;	/* no data */

		if (chkrxmsg(fp, T_RECEIVER) == 0)
			continue;	/* Not T_RECEIVER */

		if (cmd.c_match)					/* If we are matching */
			if (matchrxmsg(fp, len) == 0)	/* and there is no match */
				continue;					/* Keep looking */

		/*
		 * Successful
		 */
		outrxmsg(fp, silent);
		return(1);
	}

	error("Transmit msg timeout, no response received");
	return(0);
}

int
c_syncrecv(int silent)
{
	register fd_t	*fp;
	int				len;
	long			timecur;
	long			timestart;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	timestart = timecur = time(NULL);
	for ( ; (timecur - timestart) <= cmd.c_timeout; timecur = time(NULL))
	{
		if ((len = rxframe(fp, SMALL_TIMEOUT)) <= 0)
			continue;	/* No data */

		if (chkrxmsg(fp, T_SENDER) == 0)
			continue;	/* Not T_SENDER */

		if (cmd.c_match)					/* If we are matching */
			if (matchrxmsg(fp, len) == 0)	/* and there is no match */
				continue;					/* Keep looking */

		/*
		 * Successful
		 */
		outrxmsg(fp, silent);

		if (build_txmsg(fp, cmd.c_msg, T_RECEIVER) == 0)
			return(0);

		if (txframe(fp) == 0) {
			error("Transmit error, errno %d", errno);
			return(0);
		}
		return(1);
	}

	error("Receive error, timeout");
	return(0);
}

static int
c_sendloop()
{
	register fd_t *fp;
	int ret;
	int i;
	int min;
	int max;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	/*
	 * do an initial sync message, timeouts are small and
	 * unconfigurable because the user should have done
	 * an external sync up first.
	 */
	if (getaddr(fp) == 0)
		goto error;

	/*
	 * Not doing loopback to ourself, sync up with other machine
	 */
	if (memcmp(cmd.c_ourdstaddr, fp->f_ouraddr, ADDR_LEN)) {
		strcpy(cmd.c_msg, "startloop");
		cmd.c_timeout = LOOP_TIMEOUT;
		ret = c_syncsend(1);
		if (ret == 0) {
			errlogprf("Initial sync-up not received");
			goto error;
		}
	}

	framesize(fp, &min, &max);

	for (i = min; i <= max; i++) {
		if (cmd.c_delay)
			nap((long)cmd.c_delay);

		if (build_txpattern(fp, i) == 0)
			goto error;

		ret = txframe(fp);
		if (ret == 0) {
			errlogprf("Transmit error, errno %d", errno);
			goto error;
		}

		ret = rxframe(fp, SMALL_TIMEOUT);
		if (ret < 0) {
			errlogprf("Frame not received, expected size %d", i);
			goto error;
		}

		if (frame_pcmp(fp, i) == 0)
			goto error;
	}

	/* not doing loopback to ourself, sync up with other machine */
	if (memcmp(cmd.c_ourdstaddr, fp->f_ouraddr, ADDR_LEN)) {
		/* do a termination sync message */
		strcpy(cmd.c_msg, "endloop");
		cmd.c_timeout = LOOP_TIMEOUT;
		ret = c_syncsend(1);
		if (ret == 0) {
			errlogprf("other machine did not return ok.");
			goto error;
		}
	}

	return(1);

error:
	error("Loop frame test failed");
	return(0);
}

static int
c_recvloop()
{
	register fd_t *fp;
	int ret;
	int i;
	int min;
	int max;
	uchar *cp;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	/*
	 * do an initial sync message, timeouts are small and
	 * unconfigurable because the user should have done
	 * an external sync up first.
	 */
	strcpy(cmd.c_msg, "startloop");
	cmd.c_timeout = LOOP_TIMEOUT;
	ret = c_syncrecv(1);
	if (ret == 0) {
		errlogprf("Initial sync-up not received");
		goto error;
	}

	framesize(fp, &min, &max);

	for (i = min; i <= max; i++) {
		ret = rxframe(fp, SMALL_TIMEOUT);
		if (ret < 0) {
			errlogprf("Frame not received, expected size %d", i);
			goto error;
		}

		cp = getrxdataptr(fp);
		if (strcmp((char *)cp, "endloop") == 0) {
			errlogprf("Final sync-up received early");
			goto error;
		}

		if (frame_pcmp(fp, i) == 0)
			goto error;

		if (cmd.c_delay)
			nap((long)cmd.c_delay);

		if (build_txpattern(fp, i) == 0)
			goto error;

		ret = txframe(fp);
		if (ret == 0) {
			errlogprf("Transmit error, errno %d", errno);
			goto error;
		}
	}

	/* do a termination sync message */
	strcpy(cmd.c_msg, "endok");
	cmd.c_timeout = LOOP_TIMEOUT;
	ret = c_syncrecv(1);
	if (ret == 0) {
		errlogprf("Final sync-up not received");
		goto error;
	}

	return(1);

error:
	error("Loop frame test failed");
	return(0);
}

/*
 * send a single frame with one byte in it
 */

static int
c_send()
{
	register fd_t *fp;
	int ret;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	if (build_txpattern(fp, cmd.c_len) == 0)
		goto error;

	ret = txframe(fp);
	if (ret == 0) {
		errlogprf("Transmit error, errno %d", errno);
		goto error;
	}
	return(1);

error:
	error("Transmit frame failed");
	return(0);
}

static int
c_txid()
{
	register fd_t *fp;
	int ret = 1;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface)
	{
		case I_DLPI:
			ret = dlpi_sendxid(fp->f_fd, 1);
			break;
		case I_MDI:
			break;
	}

	if (ret == 0)
		error("txid failed, errno %d", errno);

	return(ret);
}

static int
c_rxid()
{
	return(1);	/* Always successful */
}

static int
c_ttest()
{
	return(1);	/* Always successful */
}

static int
c_rtest()
{
	return(1);	/* Always successful */
}

int
c_srmode()
{
	register fd_t *fp;

	fp = &fds[cmd.c_fd];
	return dlpi_srmode(fp->f_fd, cmd.c_srmode);
}

static int
c_sendfile()
{
	register fd_t *fp;
	int ret;
	int min;
	int max;
	int fd;
	int len;
	uchar *cp;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	if (getaddr(fp) == 0)
		goto error;

	fd = open(cmd.c_file, 0);
	if (fd < 0) {
		error("Unable to open %s, errno %d", cmd.c_file, errno);
		return(0);
	}

	strcpy(cmd.c_msg, "startfile");
	cmd.c_timeout = LOOP_TIMEOUT;
	ret = c_syncsend(1);
	if (ret == 0) {
		errlogprf("Initial sync-up not received");
		goto error;
	}

	cp = gettxdataptr(fp);

	if (build_txpattern(fp, 257) == 0)
		goto error;

	while ((len = read(fd, (char *)cp + 2, 255)) > 0)
	{
		*cp = T_FILE;
		*(cp+1) = len;
		ret = txframe(fp);
		if (ret == 0)
		{
			errlogprf("Transmit error, errno %d", errno);
			goto error;
		}
	}

	/*
	 * do a termination sync message
	 */
	build_txmsg(fp, "endfile", T_SENDER);
	txframe(fp);
	return(1);

error:
	error("file transfer failed");
	return(0);
}

static int
c_recvfile()
{
	register fd_t *fp;
	int ret;
	int i;
	int len;
	int fd;
	int min;
	int max;
	uchar *cp;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	fd = creat(cmd.c_file, 0666);
	if (fd < 0) {
		error("Unable to create %s, errno %d", cmd.c_file, errno);
		return(0);
	}

	strcpy(cmd.c_msg, "startfile");
	cmd.c_timeout = LOOP_TIMEOUT;
	ret = c_syncrecv(1);
	if (ret == 0) {
		errlogprf("Initial sync-up not received");
		goto error;
	}

	cp = getrxdataptr(fp);

	while (1) {
		ret = rxframe(fp, SMALL_TIMEOUT);
		if (ret < 0) {
			errlogprf("Frame not received, expected size %d", i);
			goto error;
		}

		if (*cp != 'X')
			break;
		len = *(cp+1);
		write(fd, (char *)cp + 2, len);
	}

	close(fd);
	return(1);

error:
	error("file transfer failed");
	return(0);
}

static int
c_biloopmode()
{
	register fd_t *fp;

	fp = &fds[cmd.c_fd];
	return dlpi_biloopmode(fp->f_fd, cmd.c_biloop);
}

static int
c_bilooprx()
{
	register fd_t *fp;

	fp = &fds[cmd.c_fd];
	return bilooprx(fp, cmd.c_biloop, cmd.c_framing,
					cmd.c_sap, cmd.c_odstaddr, cmd.c_omchnaddr, cmd.c_route);
}

static int
c_bilooptx()
{
	register fd_t *fp;

	fp = &fds[cmd.c_fd];
	return bilooptx(fp, cmd.c_biloop, cmd.c_framing,
									cmd.c_sap, cmd.c_ourdstaddr, cmd.c_route);
}

static int
c_srclr()
{
	register fd_t *fp;

	fp = &fds[cmd.c_fd];
	return dlpi_srclr(fp->f_fd, cmd.c_ourdstaddr);
}

static int
c_setsrparms()
{
	register fd_t *fp;

	fp = &fds[cmd.c_fd];
	return dlpi_setsrparms(fp->f_fd, cmd.c_parms);
}

static int
c_promisc()
{
	register fd_t *fp;
	int ret = 0;

	fp = &fds[cmd.c_fd];

	switch (fp->f_interface) {
	case I_DLPI:
		break;
	case I_MDI:
		ret = mdi_promisc(fp->f_fd);
		break;
	}
	if (ret == 0)
		error("Promisc failed, errno %d", errno);
	return(ret);
}

