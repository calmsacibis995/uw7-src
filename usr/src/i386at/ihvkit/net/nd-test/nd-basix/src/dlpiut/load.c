/*
 * File load.c
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) load.c 11.1 95/05/01 SCOINC")

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <unistd.h>

#include "dlpiut.h"

txstats_t txstats;
rxstats_t rxstats;

static void	printtxstats(void);
static void	printrxstats(void);
static int	dupcheck(int seq);
static int	datacheck(fd_t *fp, load_t *buf);

int
c_sendload()
{
	fd_t *fp;
	int ret;
	int i;
	int min;
	int max;
	int d;
	long initime;
	long startime;
	long endtime;

	register load_t *tdp;
	register load_t *rdp;
	int txcount;
	int rxcount;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	framesize(fp, &min, &max);
	min += sizeof(load_t);
	if (cmd.c_len < min || cmd.c_len > max) {
		error("length must be in the range %d to %d inclusive", min, max);
		return(0);
	}
	if (cmd.c_verify < 0 || cmd.c_verify > 100) {
		error("verify percentage must be in the range 0 to 100");
		return(0);
	}

	initime = time(NULL);
	startime = initime;
	if (cmd.c_multisync)
		startime += LOAD_DELAY;
	endtime = startime + cmd.c_duration;

	strcpy(cmd.c_msg, "startload");
	cmd.c_timeout = LOOP_TIMEOUT;
	cmd.c_match = 1;
	ret = c_syncsend(1);
	if (ret == 0) {
		errlogprf("Initial sync-up not received");
		goto error;
	}

	if (build_txpattern(fp, cmd.c_len) == 0)
		goto error;

	tdp = (load_t *)gettxdataptr(fp);
	txcount = 0;

	rdp = (load_t *)getrxdataptr(fp);
	rxcount = 0;

	memset(&txstats, 0, sizeof(txstats));

	if (cmd.c_multisync)
		while (time(NULL) < startime)
			sleep(1);

	while (time(NULL) < endtime) {

		if (cmd.c_delay)
			nap((long)cmd.c_delay);

		/* send our window */
		tdp->l_sap = cmd.c_sap;
		for (i = 0; i < cmd.c_windowsz; i++) {
			txcount++;
			tdp->l_type = (i == 0) ? LOAD_DATA : LOAD_SDATA;
			tdp->l_seq = txcount;
			ret = txframe(fp);
			if (ret == 0) {
				errlogprf("Transmit error, errno %d", errno);
				break;
			}
		}

		/* wait for ack */
		rxcount++;
getack:
		ret = rxframe(fp, cmd.c_timeout);
		/* no ack received */
		if (ret < 0) {
			txstats.noack++;
			continue;
		}
		if (rdp->l_type != LOAD_ACK) {
			txstats.badack++;
			goto getack;
		}
		if (rdp->l_sap != cmd.c_sap) {
			txstats.sapack++;
			goto getack;
		}
		if (rdp->l_seq == rxcount) {
			txstats.okack++;
			continue;
		}
		if (rdp->l_seq > rxcount) {
			/* very strange, sync-up */
			rxcount = rdp->l_seq;
			txstats.seqackhi++;
		}
		else {
			/* lost one, is ok */
			rxcount = rdp->l_seq;
			txstats.seqacklo++;
		}
	}

error:
	printtxstats();
	return 1;
}

int
c_recvload()
{
	fd_t *fp;
	int ret;
	int i;
	int min;
	int max;
	int d;
	long initime;
	long startime;
	long endtime;

	register load_t *tdp;
	register load_t *rdp;
	int txcount;
	int rxcount;
	int window;

	fp = &fds[cmd.c_fd];
	api_flush(fp);

	framesize(fp, &min, &max);
	min += sizeof(load_t);
	if (cmd.c_len < min || cmd.c_len > max) {
		error("length must be in the range %d to %d inclusive", min, max);
		return(0);
	}
	if (cmd.c_verify < 0 || cmd.c_verify > 100) {
		error("verify percentage must be in the range 0 to 100");
		return(0);
	}

	initime = time(NULL);
	startime = initime;
	if (cmd.c_multisync)
		startime += LOAD_DELAY;
	endtime = startime + cmd.c_duration;

	strcpy(cmd.c_msg, "startload");
	cmd.c_timeout = LOOP_TIMEOUT;
	cmd.c_match = 1;
	ret = c_syncrecv(1);
	if (ret == 0) {
		errlogprf("Initial sync-up not received");
		goto error;
	}

	if (build_txpattern(fp, min) == 0)
		goto error;

	if (build_rxpattern(fp, cmd.c_len) == 0)
		goto error;

	tdp = (load_t *)gettxdataptr(fp);
	txcount = 0;

	rdp = (load_t *)getrxdataptr(fp);
	rxcount = 0;

	memset(&rxstats, 0, sizeof(rxstats));

	if (cmd.c_multisync)
		while (time(NULL) < (startime - 1))
			sleep(1);

	while (1) {

		/* get recv window */

		window = 0;
		while (1) {
	
			if (time(NULL) >= endtime)
				goto done;

			ret = rxframe(fp, SMALL_TIMEOUT);
			/* timeout */
			if (ret < 0)
				continue;
			if (rdp->l_sap != cmd.c_sap) {
				rxstats.badsap++;
				continue;
			}
			if (getrxdatalen(fp) != cmd.c_len) {
				rxstats.badlen++;
				continue;
			}
			if (rdp->l_type != LOAD_DATA && rdp->l_type != LOAD_SDATA) {
				rxstats.badtype++;
				continue;
			}
			/* frame counts even if out of sequence */
			rxstats.okframes++;
			/* dup check */
			if (!dupcheck(rdp->l_seq)) {
				rxstats.dupseq++;
				continue;
			}
			rxcount++;
			if (rdp->l_seq < rxcount) {
				rxstats.badseqhi++;
			}
			else if (rdp->l_seq > rxcount) {
				rxstats.badseqlo++;
			}
			rxcount = rdp->l_seq;
			if (rdp->l_type == LOAD_DATA)
				window = rxcount;
			/* data corruption check */
			if (!datacheck(fp, rdp))
				rxstats.baddata++;
			/* send ack if needed */
			if (rxcount == (window + cmd.c_windowsz - 1))
				break;
		}
		/* send ack */
		txcount++;
		tdp->l_type = LOAD_ACK;
		tdp->l_sap = cmd.c_sap;
		tdp->l_seq = txcount;
		ret = txframe(fp);
		if (ret == 0) {
			errlogprf("Ack transmit error, errno %d", errno);
			break;
		}
		window = 0;
	}

error:
done:
	printrxstats();
	return 1;
}

static void
printtxstats()
{
	errlogprf("Transmit statistics");
	errlogprf("ok_acks                         %d", txstats.okack);
	errlogprf("ack_timeouts                    %d", txstats.noack);
	errlogprf("bad_acks                        %d", txstats.badack);
	errlogprf("bad_saps                        %d", txstats.sapack);
	errlogprf("bad_seq_hi                      %d", txstats.seqackhi);
	errlogprf("bad_seq_lo                      %d", txstats.seqacklo);
}

static void
printrxstats()
{
	errlogprf("Receive statistics");
	errlogprf("frames_received                 %d", rxstats.okframes);
	errlogprf("bad_saps_received               %d", rxstats.badsap);
	errlogprf("bad_len_received                %d", rxstats.badlen);
	errlogprf("bad_type_received               %d", rxstats.badtype);
	errlogprf("bad_seq_hi_received             %d", rxstats.badseqhi);
	errlogprf("bad_seq_lo_received             %d", rxstats.badseqlo);
	errlogprf("dup_frames_received             %d", rxstats.dupseq);
	errlogprf("bad_data_received               %d", rxstats.baddata);
}

static int
dupcheck(int seq)
{
	static *seqs;
	static *s1;
	static scount;
	static movsz;
	register int i;

	if (seqs == 0) {
		scount = cmd.c_windowsz * 2;
		seqs = (int *)malloc(sizeof(int) * scount);
		memset(seqs, 0, sizeof(int) * scount);
		movsz = (scount-1) * sizeof(int);
		s1 = seqs + scount - 1;
	}
	for (i = 0; i < scount; i++) {
		if (seqs[i] == seq)
			return(0);
	}
	memcpy(seqs, seqs+1, movsz);
	*s1 = seq;
	return(1);
}

static int
datacheck(fd_t *fp, load_t *buf)
{
	static uchar table[100];
	static count;
	static initf;
	float a;
	float b;
	int i;
	int ret;

	if (cmd.c_verify == 0)
		return(1);

	if (initf == 0) {
		initf = 1;
		a = cmd.c_verify;
		a = 100.0 / cmd.c_verify;
		for (b = 0; b < 99.9; b+= a) {
			i = b;
			table[i] = 1;
		}
	}
	ret = table[count++];
	if (count == 100)
		count = 0;
	if (ret == 0)
		return(1);
	ret = frame_lcmp(fp, cmd.c_len, sizeof(load_t));
	return(ret);
}

