#ident "@(#)isdndialer.c	29.1"
#ident "$Header$"

/*
 * File isdndialer.c
 * ISDN Dialer
 *
 *	Copyright (C) The Santa Cruz Operation, 1996.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated
 *	as Confidential.
 *
 */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <dial.h>
#include <cs.h>
#include <sys/scodlpi.h>
#include <sys/scoisdn.h>

#define	ISDN_MSG_SIZE		1024
#define	CONTROLLER		1
#define POLL_TIMEOUT   		30000 /* 30 seconds */
#define CONNECT_ACTIVE_TIMEOUT	60000 /* 60 seconds */
#define CONNECT_ACTIVE_RETRY	10
#define FILENAME_SIZE 		64
#define B_CHANNEL_SPEED		65536	/* 64K bps */

int	flags = 0;
int	expected_conf	= 1;
int	pipefd;
int	callfd;

int	log;
int debug;

struct	stat	statbuf;

time_t	timestamp;
char	timebuf[64];

#define	LOG	  		1
#define DEBUG		2
#define LOG_DEBUG	3

FILE	*logfd;
char	*logfile = "/usr/adm/log/isdndialer.log";

FILE	*debugfd;
char	*debugfile = "/usr/adm/log/isdndialer.debug";

char	*isdnfileprefix = "/dev/";
char	isdnfile[FILENAME_SIZE];

isdnPLCI_t	connectedPLCI;
isdnNCCI_t	connectedNCCI;

calls_t		*calls;
dial_status_t	dstatus;

isdnByte_t	isdn_msg[ISDN_MSG_SIZE];
isdnByte_t	isdn_data[ISDN_MSG_SIZE];

struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
struct 	pollfd	fds[1];
int    	poll_result;

isdn_pinfo_t pinfo_sync = PINFO_ISDN_SYNC;
isdn_pinfo_t	*call_pinfo; 


void
logger(int type, char *fmt, ...)
{
/* struct	tm	local_time; */
va_list	ap;


/*	timestamp = time(0); */
/*	localtime_r(&timestamp, &local_time); */
/*	strftime(timebuf, 64, "%c", &local_time); */

	if (log && (type & LOG)) {
/*		fprintf(logfd, "%s ", timebuf); */
		va_start(ap, fmt);
		vfprintf(logfd, fmt, ap);
		va_end(ap);
	}

	if (debug && (type & DEBUG)) {
/*		fprintf(debugfd, "%s ", timebuf); */
		va_start(ap, fmt);
		vfprintf(debugfd, fmt, ap);
		va_end(ap);
	}

}


trace_msg(unsigned char *p, int len)
{
int	i;

	logger(DEBUG, "trace_msg: ");
	for (i = 0; i < len; i++) {
		logger(DEBUG, "%2.2X ", *p);
		p++;
	}
	logger(DEBUG, "\n");
}


void
dialer_exit(int status)
{

	if (ics_write_req(pipefd, RECV_DATA) < 0) {
		ics_cli_close(pipefd);
		logger(LOG_DEBUG, "dialer_exit:  RECV_DATA message failed\n");
		logger(LOG_DEBUG, "Isdndialer abnormal exit\n");
		exit(-1);
	}

 	dstatus.status_code = status;
	dstatus.dial_service = DIAL_NULL;
	if (ics_write_data(pipefd, (char *)&dstatus, sizeof(dial_status_t)) < 0) {
		ics_cli_close(pipefd);
		logger(LOG_DEBUG, "dialer_exit:  write message failed\n");
		logger(LOG_DEBUG, "Isdndialer abnormal exit\n");
		exit(-1);
	}

	/* return NCCI, pinfo */
	/* use calls pinfo if call_pinfop not yet assigned */
 	if (call_pinfo != NULL)
		calls->pinfo = (void *)call_pinfo;
	call_pinfo->NCCI = connectedNCCI;

	if (ics_write_calls(pipefd, calls) < 0) {
		logger(LOG_DEBUG, "dialer_exit:  write calls failed\n");
		logger(LOG_DEBUG, "Isdndialer abnormal exit\n");
		exit(-1);
	} 
	logger(DEBUG, "dialer_exit:  status=%d\n", status);
	logger(LOG_DEBUG, "Isdndialer abnormal exit\n");

	ics_cli_close(pipefd);
	if (status < 0)
		exit(status);
	exit(-1);
			
}	


void 
copy_struct(isdnByte_t **dst, isdnByte_t **src)
{
isdnByte_t	len;

	len = **src;
	**dst = **src;
	*dst += 1;
	*src += 1;
	memcpy(*dst, *src, len);
	*src += len;
	*dst += len;
}


int
wait_for_message(int timeout)
{

	fds[0].fd = callfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	poll_result = poll(fds, 1, timeout);

	if (poll_result < 0) {
		logger(DEBUG, "wait_for_message: poll failed waiting for message");
		return -1;
	}

	if (poll_result == 0) {
		logger(DEBUG, "wait_for_message: Timeout waiting for message\n");
		return -1;
	}

	if (fds[0].revents != POLLIN) {
		logger(DEBUG, "wait_for_message: Bad poll event mask waiting for message\n");
		return -1;
	}

	msg_ctl.maxlen = sizeof(isdn_msg);
	msg_ctl.len = 0;            /* Not used */
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = sizeof(isdn_data);
	msg_data.len = 0;           /* Not used */
	msg_data.buf = (void *)isdn_data;

	if (getmsg(callfd, &msg_ctl, &msg_data, &flags) < 0) {
		logger(DEBUG, "wait_for_message: getmsg failed");
		return -1;
	}

	if (msg_ctl.len <= 0 ) {
		logger(DEBUG, "wait_for_message: No control data in message\n");
		return -1;
	}

	return (0);
}


void
disconnect_resp(isdnWord_t expected_resp)
{
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_DISCONNECT;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_resp;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnPLCI_t *)msg_p = connectedPLCI;
	msg_p += sizeof(isdnPLCI_t);

 	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(LOG_DEBUG, "Disconnect Response putmsg failed\n");
		dialer_exit(D_HUNG);
	}

	dialer_exit(NO_ANS);
}


void
disconnect_ind()
{
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdnWord_t expected_resp;
int	message_retry = 0;

	/* do ind */
	while (message_retry < CONNECT_ACTIVE_RETRY) {

		if (wait_for_message(CONNECT_ACTIVE_TIMEOUT) < 0) {
			logger(LOG_DEBUG, "Timeout waiting for Disconnect Indication\n");
			dialer_exit(D_HUNG);
		}

		hdr_p = (isdn_msg_hdr_t *)isdn_msg;

		if (hdr_p->Cmd != ISDN_DISCONNECT) {
			logger(DEBUG, "Command=%2.2X is not DISCONNECT\n", hdr_p->Cmd);
			message_retry++;
			continue;
		}

		if (hdr_p->SubCmd != ISDN_IND) {
			logger(DEBUG, "Subcommand=%2.2X is not IND\n", hdr_p->SubCmd);
			message_retry++;
			continue;
		}

		break;
	}

	if (message_retry >= CONNECT_ACTIVE_RETRY) {
		logger(LOG_DEBUG,"Disconnect message retry limit reached\n");
		dialer_exit(D_HUNG);
	}
	
	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnPLCI_t)
				+ sizeof(isdnReason_t)
				- sizeof(isdnDword_t))) {
		logger(LOG_DEBUG, "Disconnect Indication message too small\n");
	       	dialer_exit(D_HUNG);
	}

	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	if (*(isdnPLCI_t *)msg_p != connectedPLCI) {
		logger(LOG_DEBUG, "Bad PLCI in Disconnect Indication\n");
	       	dialer_exit(D_HUNG);
	}

	msg_p += sizeof(isdnPLCI_t);

	if (*(isdnReason_t *)msg_p != 0)
		logger(DEBUG, "Disconnect Indication reason=%4.4X\n", *(isdnReason_t *)msg_p);

	disconnect_resp(expected_resp);

}


void
disconnect_b3_resp(isdnWord_t expected_resp)
{
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_DISCONNECT_B3;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_resp;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnNCCI_t *)msg_p = connectedNCCI;
	msg_p += sizeof(isdnNCCI_t);

 	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(LOG_DEBUG, "DisconnectB3 Response putmsg failed\n");
		dialer_exit(D_HUNG);
	}

	disconnect_ind();
}



void
disconnect(isdnByte_t *dis_msg)
{
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdnWord_t expected_resp;

	hdr_p = (isdn_msg_hdr_t *)dis_msg;
	if (hdr_p->SubCmd == ISDN_IND)
		if (hdr_p->Cmd == ISDN_DISCONNECT) {
			/* check disconnect message */
			if (hdr_p->Length < (sizeof(isdn_msg_hdr_t)
						+ sizeof(isdnPLCI_t)
						+ sizeof(isdnReason_t)
						- sizeof(isdnDword_t))) {
				logger(LOG_DEBUG, "Disconnect Indication message too small\n");
				dialer_exit(D_HUNG);
			}

			expected_resp = hdr_p->MsgNum;

			msg_p = dis_msg;
			msg_p += sizeof(isdn_msg_hdr_t);
			if (*(isdnPLCI_t *)msg_p != connectedPLCI) {
				logger(LOG_DEBUG, "Bad PLCI in Disconnect Indication\n");
				dialer_exit(D_HUNG);
			}
			msg_p += sizeof(isdnPLCI_t);

			if (*(isdnReason_t *)msg_p != 0)
				logger(DEBUG, "Disconnect Indication reason=%4.4X\n", *(isdnReason_t *)msg_p);
			
			disconnect_resp(expected_resp);

		}
		else {
			/* check disconnect_b3 message */
			if (hdr_p->Length < (sizeof(isdn_msg_hdr_t)
						+ sizeof(isdnNCCI_t)
						+ sizeof(isdnReason_t)
						+ 1 - sizeof(isdnDword_t))) {
				logger(LOG_DEBUG, "Disconnect_B3 Indication message too small\n");
				dialer_exit(D_HUNG);
			}

			expected_resp = hdr_p->MsgNum;

			msg_p = isdn_msg;
			msg_p += sizeof(isdn_msg_hdr_t);
			if (*(isdnNCCI_t *)msg_p != connectedNCCI) {
				logger(LOG_DEBUG, "Bad NCCI in Disconnect_B3 Indication\n");
				dialer_exit(D_HUNG);
			}
			msg_p += sizeof(isdnNCCI_t);

			if (*(isdnReason_t *)msg_p != 0)
				logger(DEBUG, "Disconnect_B3 Indication reason=%4.4X\n", *(isdnReason_t *)msg_p);

			disconnect_b3_resp(expected_resp);
		}
	else {
		logger(LOG_DEBUG, "Disconnect not indication\n");
		dialer_exit(D_HUNG);
	}

}


void
get_call_structure()
{
	logger(DEBUG, "entered get_call_structure\n");
	if ((pipefd = ics_cli_init(OUTISDNPIPE)) < 0) {
		logger(LOG_DEBUG, "isdndial: Cannot open pipe to Connection Server\n");
		exit(-1);
	}
	logger(DEBUG, "get_call_structure: ics_cli_init successful\n");

	if (ics_cli_open(pipefd) < 0) {
		logger(LOG_DEBUG, "isdndial:  Cannot initialize pipe to Connection Server\n");
		exit(-1);
	}
	logger(DEBUG, "get_call_structure: ics_cli_open successful\n");

	if (ics_write_req(pipefd, SEND_DATA) < 0) {
		logger(LOG_DEBUG, "isdndial:  SEND_DATA request failed\n");
		exit(-1);
	}
	logger(DEBUG, "get_call_structure: ics_write_req successful\n");

	if ((calls = ics_read_calls(pipefd)) == NULL) {
		logger(LOG_DEBUG, "isdndial:  read calls structure failed\n");
		exit(-1);
	}
	logger(DEBUG, "get_call_structure: ics_read_calls successful\n");
}

void
open_device()
{
isdn_register_params_t	rp;
struct	strioctl	strioctl;

	/* open /dev/netx */
	strcpy(isdnfile, isdnfileprefix);
	strcat(isdnfile, calls->device_name);

	if ((callfd = open(isdnfile, O_RDWR)) < 0) {
		logger(LOG_DEBUG, "Cannot open device %s\n", isdnfile);
	       	dialer_exit(DV_NT_K);
	}

	rp.level3cnt = 2;
	rp.datablkcnt = 100;
	rp.datablklen = 2048;
	strioctl.ic_cmd = ISDN_REGISTER;
	strioctl.ic_timout = 0;
	strioctl.ic_dp = (void *)&rp;
	strioctl.ic_len = sizeof(rp);
	if ((ioctl(callfd, I_STR, &strioctl)) < 0 ) {
		logger(LOG_DEBUG, "Register failed\n");
		exit(DV_NT_A);
	}
	logger(DEBUG, "open_device: successful\n");

}

void
connect()
{
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdnByte_t	struct_len;
isdnByte_t	*pinfo_p;
isdnInfo_t	Info;

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT;
	hdr_p->SubCmd = ISDN_REQ;
	hdr_p->MsgNum = expected_conf;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnCtrlr_t *)msg_p = CONTROLLER;
	msg_p += sizeof(isdnCtrlr_t);

/*	trace_msg(calls->pinfo, calls->pinfo_len); */

	call_pinfo = (isdn_pinfo_t *)calls->pinfo;
	if (call_pinfo == NULL) {
		if (strcmp(calls->class, "ISDN_SYNC") == 0)
			call_pinfo = &pinfo_sync;
		else {
			logger(LOG_DEBUG, "NULL pinfo - class %s not supported\n",
															calls->class);
			dialer_exit(D_HUNG);
		}
	}

/*	trace_msg(call_pinfo, sizeof(pinfo_sync)); */

 	*(isdnCIPvalue_t *)msg_p = call_pinfo->CIPvalue;
	msg_p += sizeof(isdnCIPvalue_t);

	*msg_p = strlen(calls->telno) + 1; /* called party number */
	msg_p += 1;

	*msg_p = 0x80; /* type of number and numbering plan identification */
	msg_p += 1;

	strcpy((char *)msg_p, calls->telno);
	msg_p += strlen(calls->telno);

	if (calls->caller_telno == NULL) { /* calling party number */
		*msg_p = 0;
		msg_p += 1;

	} else {
		*msg_p = strlen(calls->caller_telno) + 2;
		msg_p += 1;

		*msg_p = 0x00; /* type of number and numbering plan identification */
		msg_p += 1;

		*msg_p = 0x80; /* presentation */
		msg_p += 1;

		strcpy((char *)msg_p, calls->caller_telno);
		msg_p += strlen(calls->caller_telno);
	}

	*msg_p = 0; /* Called party subaddress */
	msg_p += 1;

	*msg_p = 0; /* Calling party subaddress */
	msg_p += 1;

	pinfo_p = (isdnByte_t *)&call_pinfo->protocol_info;

	copy_struct(&msg_p, &pinfo_p); /* B protocol */

	copy_struct(&msg_p, &pinfo_p); /* BC */

	copy_struct(&msg_p, &pinfo_p); /* LLC */

	copy_struct(&msg_p, &pinfo_p); /* HC */

	copy_struct(&msg_p, &pinfo_p); /* Additional Info */

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);
			/* Length is CAPI so subtract DL_prim length */

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

/*	trace_msg(msg_ctl.buf, msg_ctl.len); */
	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(LOG_DEBUG, "Connect Request putmsg failed\n");
		dialer_exit(D_HUNG);
	}

	logger(LOG_DEBUG, "calling %s\n", calls->telno);
	if (wait_for_message(POLL_TIMEOUT) < 0) {
		logger(LOG_DEBUG, "Connect confirm poll timeout\n");
		dialer_exit(D_HUNG);
	}

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;

	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnPLCI_t)
		+ sizeof(isdnInfo_t) - sizeof(isdnDword_t))) {
		logger(LOG_DEBUG, "Connect Confirmation message too small\n");
		dialer_exit(D_HUNG);
	}

	if (hdr_p->Cmd != ISDN_CONNECT) {
		logger(LOG_DEBUG, "Connect Confirmation command is not ISDN_CONNECT\n");
		dialer_exit(D_HUNG);
	}

	if (hdr_p->SubCmd != ISDN_CONF) {
		logger(LOG_DEBUG, "Connect Confirmation subcommand is not ISDN_CONF\n");
		dialer_exit(D_HUNG);
	}

	msg_p = isdn_msg + sizeof(isdn_msg_hdr_t);
	connectedPLCI = *(isdnPLCI_t *)msg_p;
	msg_p += sizeof(isdnPLCI_t);

	Info = *(isdnInfo_t *)msg_p;
	if (Info != 0) {
		logger(LOG_DEBUG, "Connect Confirmation error %x\n", Info);
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "Connect successful\n");
}


void
connect_active()
{
int	message_retry = 0;
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdnWord_t	expected_resp;

	while (message_retry < CONNECT_ACTIVE_RETRY) {

		if (wait_for_message(CONNECT_ACTIVE_TIMEOUT) < 0) {
			logger(LOG_DEBUG, "Timeout waiting for Connect Active Indication\n");
			dialer_exit(D_HUNG);
		}

		hdr_p = (isdn_msg_hdr_t *)isdn_msg;

		if (hdr_p->Cmd == ISDN_DISCONNECT) {
			disconnect(isdn_msg);
			logger(LOG_DEBUG, "Network disconnected\n");
			dialer_exit(NO_ANS);
		}

		if (hdr_p->Cmd != ISDN_CONNECT_ACTIVE) {
			logger(DEBUG, "Command=%2.2X is not CONNECT_ACTIVE\n", hdr_p->Cmd);
			message_retry++;
			continue;
		}

		if (hdr_p->SubCmd != ISDN_IND) {
			logger(DEBUG, "Subcommand=%2.2X is not IND\n", hdr_p->SubCmd);
			message_retry++;
			continue;
		}

		break;
	}

	if (message_retry >= CONNECT_ACTIVE_RETRY) {
		logger(LOG_DEBUG, "Connect_Active message retry limit reached\n");
		dialer_exit(D_HUNG);
	}
	
	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnPLCI_t)
				 - sizeof(isdnDword_t))) {
		logger(LOG_DEBUG, "Connect_Active Indication message too small\n");
	       	dialer_exit(D_HUNG);
	}

	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	if (*(isdnPLCI_t *)msg_p != connectedPLCI) {
		logger(LOG_DEBUG, "Bad PLCI in Connect_Active Indication\n");
	       	dialer_exit(D_HUNG);
	}

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT_ACTIVE;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_conf;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnPLCI_t *)msg_p = connectedPLCI;
	msg_p += sizeof(isdnPLCI_t);

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);
			/* Length is CAPI so subtract DL_prim length */

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(LOG_DEBUG, "Connect_Active Response putmsg failed\n");
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "Successful Connect Active\n");

}
		

void
connect_b3()
{
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdnInfo_t	Info;

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT_B3;
	hdr_p->SubCmd = ISDN_REQ;
	hdr_p->MsgNum = expected_conf;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnPLCI_t *)msg_p = connectedPLCI;
	msg_p += sizeof(isdnPLCI_t);

	*msg_p = 0; /* empty NCPI structure */
	msg_p += 1;

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);
			/* Length is CAPI so subtract DL_prim length */

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(LOG_DEBUG, "Connect_B3 Request putmsg failed\n");
		dialer_exit(D_HUNG);
	}

	if (wait_for_message(POLL_TIMEOUT) < 0) {
		logger(LOG_DEBUG, "Connect_B3 confirmation poll timeout\n");
		dialer_exit(D_HUNG);
	}

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;

	if (hdr_p->Cmd == ISDN_DISCONNECT) {
		disconnect(isdn_msg);
		logger(LOG_DEBUG, "Network disconnected\n");
		dialer_exit(NO_ANS);
	}

	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnNCCI_t)
		+ sizeof(isdnInfo_t) - sizeof(isdnDword_t))) {
		logger(LOG_DEBUG, "Connect_B3 Confirmation message too small\n");
		dialer_exit(D_HUNG);
	}

	if (hdr_p->Cmd != ISDN_CONNECT_B3) {
		logger(LOG_DEBUG, "Connect_B3 Confirmation command is not CONNECT_B3\n");
		dialer_exit(D_HUNG);
	}

	if (hdr_p->SubCmd != ISDN_CONF) {
		logger(LOG_DEBUG, "Connect_B3 Confirmation subcommand is not CONF\n");
		dialer_exit(D_HUNG);
	}

	msg_p = isdn_msg + sizeof(isdn_msg_hdr_t);
	connectedNCCI = *(isdnNCCI_t *)msg_p;
	msg_p += sizeof(isdnNCCI_t);

	Info = *(isdnInfo_t *)msg_p;
	if (Info != 0) {
		logger(LOG_DEBUG, "Connect_B3 Confirmation error %x\n", Info);
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "Successful Connect_B3\n");

}


void
mdata_ioctl()
{
struct	strioctl	strioctl;
isdnNCCI_t	mdata_NCCI;

	mdata_NCCI = connectedNCCI;
	strioctl.ic_cmd = DLPI_ISDN_MDATA_ON;
	strioctl.ic_timout = 0;
	strioctl.ic_dp = (void *)&mdata_NCCI;
	strioctl.ic_len = sizeof(mdata_NCCI);
	if ((ioctl(callfd, I_STR, &strioctl)) < 0 ) {
		logger(LOG_DEBUG, "DLPI_ISDN_MDATA_ON ioctl failed\n");
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "mdata_ioctl: successful\n");
}


void
connect_b3_active()
{
int	message_retry = 0;
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdnWord_t	expected_resp;

	while (message_retry < CONNECT_ACTIVE_RETRY) {

		if (wait_for_message(CONNECT_ACTIVE_TIMEOUT) < 0) {
			logger(LOG_DEBUG, "Connect_B3 Active timeout\n");
			dialer_exit(D_HUNG);
		}

		hdr_p = (isdn_msg_hdr_t *)isdn_msg;

		if ((hdr_p->Cmd == ISDN_DISCONNECT)
		     || (hdr_p->Cmd == ISDN_DISCONNECT_B3)) {
			disconnect(isdn_msg);
			logger(LOG_DEBUG, "Network disconnected\n");
			dialer_exit(D_HUNG);
		}

		if (hdr_p->Cmd != ISDN_CONNECT_B3_ACTIVE) {
			logger(DEBUG, "Command=%2.2X is not ISDN_CONNECT_B3_ACTIVE\n", hdr_p->Cmd);
			message_retry++;
			continue;
		}

		if (hdr_p->SubCmd != ISDN_IND) {
			logger(DEBUG,"Subcommand=%2.2X is not ISDN_IND\n", hdr_p->SubCmd);
			message_retry++;
			continue;
		}

		break;
	}

	if (message_retry >= CONNECT_ACTIVE_RETRY) {
		logger(LOG_DEBUG, "Connect_B3_Active message retry limit reached\n");
		dialer_exit(D_HUNG);
	}
	
	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnNCCI_t)
				 + 1 - sizeof(isdnDword_t))) {
		logger(LOG_DEBUG, "Connect_B3_Active Indication message too small\n");
		dialer_exit(D_HUNG);
	}

	mdata_ioctl(); /* M_DATA or M_HANGUP upstream only now */

	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	if (*(isdnNCCI_t *)msg_p != connectedNCCI) {
		logger(LOG_DEBUG, "Bad NCCI in Connect_B3_Active Indication\n");
		dialer_exit(D_HUNG);
	}

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT_B3_ACTIVE;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_conf;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnNCCI_t *)msg_p = connectedNCCI;
	msg_p += sizeof(isdnNCCI_t);

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(LOG_DEBUG, "Connect_B3_Active Response putmsg failed\n");
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "Successful Connect_B3 Active\n");

}


void
return_info()
{

	if (ics_write_req(pipefd, RECV_FD) < 0) {
		logger(LOG_DEBUG, "return_info:  RECV_FD message failed\n");
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "return_info: ics_write_req RECV_FD successful\n");
	
	/* write the fd */
	ics_write_fd(pipefd, callfd);
	close(callfd);
	logger(DEBUG, "return_info: ics_write_fd and close successful\n");

	if (ics_write_req(pipefd, RECV_DATA) < 0) {
		logger(LOG_DEBUG, "return_info:  RECV_DATA message failed\n");
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "return_info: ics_write_req RECV_DATA successful\n");

 	dstatus.status_code = 0;
	dstatus.dial_service = DIAL_ISDN_SYNC;
	if (ics_write_data(pipefd, (char *)&dstatus, sizeof(dial_status_t)) < 0) {
		logger(LOG_DEBUG, "return_info:  write message failed\n");
		dialer_exit(D_HUNG);
	}
	logger(DEBUG, "return_info: ics_write_data successful\n");
	
	/* return speed, pinfo, NCCI */
	calls->speed = B_CHANNEL_SPEED;
	calls->pinfo = (void *)call_pinfo;
	call_pinfo->NCCI = connectedNCCI;

	if (ics_write_calls(pipefd, calls) < 0) {
		logger(LOG_DEBUG, "return_info:  write calls failed\n");
		dialer_exit(D_HUNG);
	} 
	logger(DEBUG, "return_info: ics_write_calls successful\n");

	/* close pipe */
	ics_cli_close(pipefd);
	logger(DEBUG, "return_info:  successful\n");

}

void
init()
{

	log = FALSE;
	debug = FALSE;
	if (stat(logfile, &statbuf) == 0) { /* logfile exists, then log */
		if ((logfd = fopen(logfile, "a")) != NULL) {
			log = TRUE;
			setvbuf(logfd, NULL, _IOLBF, 0);
		}
	}
	if (stat(debugfile, &statbuf) == 0) { /* debugfile exists, then log debug */
		if ((debugfd = fopen(debugfile, "a")) != NULL) {
			debug = TRUE;
			setvbuf(debugfd, NULL, _IOLBF, 0);
		}
	}
	call_pinfo = NULL;
	connectedNCCI = 0;
}


main()
{
	init();
 	logger(LOG_DEBUG, "Isdndialer invoked\n");
	get_call_structure();
 	open_device();
	connect();
	connect_active();
	connect_b3();
	connect_b3_active();
	return_info();
	logger(LOG_DEBUG, "Isdndialer normal exit\n");
	exit(0);
}
