 


/* Copyright 1991, 1992 Unpublished Work of Novell, Inc.
** All Rights Reserved.
**
** This work is an unpublished work and contains confidential,
** proprietary and trade secret information of Novell, Inc. Access
** to this work is restricted to (I) Novell employees who have a
** need to know to perform tasks within the scope of their
** assignments and (II) entities other than Novell who have
** entered into appropriate agreements.
**
** No part of this work may be used, practiced, performed,
** copied, distributed, revised, modified, translated, abridged,
** condensed, expanded, collected, compiled, linked, recast,
** transformed or adapted without the prior written consent
** of Novell.  Any use or exploitation of this work without
** authorization could subject the perpetrator to criminal and
** civil liability.
**
*/



#if !defined(__printmsgtable_h__)
#define __printmsgtable_h__

/*  DEVELOPERS:  Do NOT add messages or constants to this file */
/*  */
/*  You must go to file "nls/English/nprinter/printmsg.m4" and make additions there. */
/*   */
#define MSG_PRINT_REV_SET 1
#define MSG_PRINT_REV 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	MSG_PRINT_REV_STR "@@(#)$Id$" 
/*  */
/* Messages used by nprinter */
/*  */
#define MSG_NPRINT_SET 3


/*  */
/* Normal Messages */
/*  */
#define RPMSG_STARTING 1
#define RPMSG_QUITING 2
#define RPMSG_QUIT 3
#define RPMSG_TERMINATED 4
#define RPMSG_STRING 5

/*  */
/* Verbose messages */
/*  */
#define RPMSG_PRINTER_STARTING 10
#define RPMSG_PRINTER_READY 11
#define RPMSG_PRINTER_STOPPING 12
#define RPMSG_PRINTER_RESTARTING 13
#define RPMSG_NEW_HOST_PRT_NAME 14
/*  */
/* Warning messages */
/*  */
#define RPMSG_NO_ADVERTISE 20
#define RPMSG_NO_SPX_CL_CONNECT 21
#define RPMSG_NO_SPX_RP_CONNECT 22
#define RPMSG_NO_PS_RPRINTER 23
#define RPMSG_RPRINTER_IN_USE 24
#define RPMSG_PSERVER_DOWN 25
/*  */
/* Error messages */
/*  */
#define RPMSG_TOO_MANY_PRINTERS 30
#define RPMSG_NO_IPX_OPEN 31
#define RPMSG_NO_SPX_OPEN 32
#define RPMSG_BAD_PATH 33
#define RPMSG_BAD_OPEN 34
#define RPMSG_INVALID_SWITCH 35
#define RPMSG_BAD_CTF_LINE_SH 36
#define RPMSG_BAD_CTF_LINE_PT 37
#define RPMSG_BAD_CFF_LINE_SH 38
#define RPMSG_BAD_CFF_LINE_NM 39
#define RPMSG_BAD_PCF_LINE_SH 40
#define RPMSG_BAD_PCF_LINE_PT 41
#define RPMSG_BAD_PRT_NAME 42
#define RPMSG_WRITING_DATA 43
#define RPMSG_BAD_PRT_ID 44
#define RPMSG_JOB_NOACTIVE 45
#define RPMSG_NO_QUEUES 46
#define RPMSG_BAD_DEST 47
/*  */
/* Debug messages */
/*  */
#define RPMSG_PSERVER_ADDR 60
#define RPMSG_PSRP_SOCKET 61
#define RPMSG_PSERVER_WAIT_ERROR 62
#define RPMSG_STATUS_NOT_SENT 63
#define RPMSG_COMMAND_NO_RECEIVE 64
#define RPMSG_BAD_RP_COMMAND 65
#define RPMSG_INVALID_ENTRY_STATUS 66
#define RPMSG_CHANGE_ENTRY_STATUS 67
#define RPMSG_PJ_NEW 68
#define RPMSG_PJ_EOJ 69
#define RPMSG_PJ_TIMEOUT 70
#define RPMSG_PJ_END_BY_NEW 71
#define RPMSG_PJ_ABORT 72
#define RPMSG_PJ_PAUSED 73
#define RPMSG_PJ_UNPAUSED 74
#define RPMSG_PJ_SIDEBAND 75
#define RPMSG_NEW_PRINTER_STATUS 76

/*  */
/*  Errors on nwcm access */
/*  */
#define NPRINTER_CANT_FIND_FILE 100
#define NPRINTER_CANT_MAKE_CONFIG_DIR 101
/*  */
/* For stopnp.c */
/*  */
#define NPSTOP_FILE_OPEN_ERR 200
#define NPSTOP_FILE_READ_ERR 201
#define NPSTOP_KILL_ERR 202

/*  */
/* NPRINTER Helps and Parameter Descriptions */
/*  */
#define MSG_NPRINT_DH_SET 4
/*  */
/* Descriptions Messages */
/*  */
#define NWCM_PN_NPRINTER_CONSOLE_DEVICE 1
#define NWCM_PN_NPRINTER_CONFIG_DIRECTORY 2
#define NWCM_PN_NPRINTER_PRT_FILE 3
#define NWCM_PN_NPRINTER_CONTROL_FILE 4
#define NWCM_PN_NPRINTER_CONFIG_FILE 5
/*  */
/* Help Messages */
/*  */
#define NWCM_PH_NPRINTER_CONSOLE_DEVICE 10
#define NWCM_PH_NPRINTER_CONFIG_DIRECTORY 11
#define NWCM_PH_NPRINTER_PRT_FILE 12
#define NWCM_PH_NPRINTER_CONTROL_FILE 13
#define NWCM_PH_NPRINTER_CONFIG_FILE 14

#endif /* for __printmsgtable_h__ */
/* ************************* end of file ********************* */
