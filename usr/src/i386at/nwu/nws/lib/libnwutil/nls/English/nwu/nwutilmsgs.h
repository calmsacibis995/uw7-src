 


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



#if !defined(__nwutil_h__)
#define __nwutil_h__

/*  DEVELOPERS:  Do NOT add messages or constants to this file */
/*  */
/*  You must go to file "nls/English/nwu/nwutil.m4" and make additions there. */
/*   */
#define MSG_NWUTIL_REV_SET 1
#define MSG_NWUTIL_REV 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	MSG_NWUTIL_REV_STR "@(#)$Id$" 
/*  */
/* ************************************************* */
/* Message string constants for NWS utility programs */
/* ************************************************* */
#define MSG_ETCINFO_SET 2



#define ETC_OPEN 2
#define ETC_READ 3
#define ETC_DUMP 4
#define ETC_INFO 5
#define ETC_DSTAT 10
#define ETC_ACTIVE 11
#define ETC_MSGS 12
#define ETC_IOCTLS 13
#define ETC_DN_DROP 14
#define ETC_DN_NOT_CLIENT 15
#define ETC_UP_RECD 16
#define ETC_UP_UNK 17
#define ETC_UP_DATA_DROP 18
#define ETC_SSTAT 19
#define ETC_RCD_SVR 20
#define ETC_DRP_SVR 21
#define ETC_UPWD_SVR 22
#define ETC_UPWD_Q 23
#define ETC_UPWD_FLOW 24
#define ETC_UPWD_CHAIN 25
#define ETC_UPWD_CHAINS 26
#define ETC_UPWD_REQU 27
#define ETC_TIOCTL 28
#define ETC_BIOCTL 29
#define ETC_DNWD 30
#define ETC_DNWD_DROP 31
#define ETC_DNBC 32
#define ETC_DNBC_DROP 33
#define ETC_CSTAT 34
#define ETC_WD_SOCK 35
#define ETC_WD_SOCK_DROP 36
#define ETC_WD_REPLY 37
#define ETC_CRETRY 38
#define ETC_CRETRY_SECS 39
#define ETC_MAX_CREQUESTS 40
#define ETC_DNC 41
#define ETC_DNC_ERROR 42
#define ETC_DNC_RETRY 43
#define ETC_DNC_TOTAL_RETRIES 44
#define ETC_UPC 45
#define ETC_UPC_DROP 46
#define ETC_UPC_NOREQ 47
#define ETC_UPC_SENT 48
#define ETC_UPBC 49
#define ETC_UPBC_SENT 50
#define ETC_UPBC_DROP 51
#define ETC_UPBC_Q 52
#define ETC_UPBC_SVC 53
#define ETC_UPBC_REQU 54
#define ETC_ESTAT 55
#define ETC_MAX_ELEN 56
#define ETC_UPE 57
#define ETC_UPE_DROP 58
#define ETC_UPE_SENT 59
#define ETC_SRCH_STATS 60
#define ETC_REQ_TABLE_SIZE 61
#define ETC_MAX_SUCC_SRCH 62
#define ETC_MAX_RBP_COUNT 63
#define ETC_ROOT 64

#define MSG_NWDUMP_SET 3



#define NINFO_OPEN 1
#define NINFO_INFO 2

#define NDMP_OPEN 5
#define NDMP_DUMP 6

#define NDMP_BEGIN_PAREN 7
#define NDMP_END_PAREN 8
#define NDMP_SEPARATOR 9

#define NDMP_ACTIVE 10
#define NDMP_STATE 11
#define NDMP_MASK 12
#define NDMP_SLINK 13
#define NDMP_SDUMP 14
#define NDMP_MALLOC 15
#define NDMP_DDUMP 16
#define NDMP_DALLOC 17
#define NDMP_DOPEN 18
#define NDMP_DCLOSE 19
#define NDMP_NOT_NWU 20
#define NDMP_NWU 21
#define NDMP_NWU_DAEMON 22
#define NDMP_NWU_ENGINE 23
#define NDMP_NWU_NOT_ENGINE 24
#define NDMP_MAX_ENGINE 25
#define NDMP_REQ_ENGINE 26
#define NDMP_IDLE_ENGINE 27
#define NDMP_DEAD_ENGINE 28
#define NDMP_DEAD_NON_ENGINE 29
#define NDMP_DEAD_DAEMON 30
#define NDMP_NOSTART_ENGINE 31
#define NDMP_UP_MSG 32
#define NDMP_UP_CMSG 33
#define NDMP_UP_SMSG 34
#define NDMP_UP_SQMSG 35
#define NDMP_UP_SMSG_ONQ 36
#define NDMP_UP_IOCTL_RESP 37
#define NDMP_UP_MSG_DROP 38
#define NDMP_DN_HEAD 39
#define NDMP_DN_CLIENT_MSG 40
#define NDMP_DN_SENT 41
#define NDMP_DN_QSENT 42
#define NDMP_DN_ONQ 43
#define NDMP_IOCTL_REQ 44
#define NDMP_DN_DROP 45
#define NDMP_STR_NAME 46
#define NDMP_STR_UNAME 47
#define NDMP_STR_UP_LAN 48
#define NDMP_STR_UP_SENT 49
#define NDMP_STR_UP_QSENT 50
#define NDMP_STR_UP_ONQ 51
#define NDMP_STR_IOCTL_RESP 52
#define NDMP_STR_UP_DROP 53
#define NDMP_STR_DN_HEAD 54
#define NDMP_STR_DN_MSG 55
#define NDMP_STR_DN_QSENT 56
#define NDMP_STR_DN_ONQ 57
#define NDMP_STR_IOCTL_REQ 58
#define NDMP_STR_LAST_DEV 59
#define NDMP_LAST_IOCTL 60
#define NDMP_DEV_NUM 61
#define NDMP_DEV_PID 62
#define NDMP_DEV_STATE 63
#define NDMP_DEV_DN_STR 64
#define NDMP_DEV_UP_STR 65
#define NDMP_DEV_NMXINFO 66
#define NDMP_DEV_PSTYPE 67
#define NDMP_DEV_PSINFO 68
#define NDMP_DEV_HEAD 70
#define NDMP_DEV_IOCTL_REQ 71
#define NDMP_DEV_DN_SENT 72
#define NDMP_DEV_UP_MSGS 73
#define NDMP_DEV_UP_SENT 74
#define NDMP_DEV_UP_QSENT 75
#define NDMP_DEV_IOCTL_RESP 76
#define NDMP_DEV_DN_DROP 77
#define NDMP_DEV_MPT 78
#define NDMP_DEV_QUEUED 79
#define NDMP_ROOT 80


#define MSG_NWENGINE_SET 4



#define NWE_OPEN 01
#define NWE_ENGINE_CNT 02
#define NWE_IOCTL 03
#define NWE_USAGE 10
#define NWE_ARGS 11
#define NWE_NUM 12
#define NWE_ROOT 13

#define MSG_NXINFO_SET 5



#define NX_OPEN 02
#define NX_IOCTL 03
#define NX_ACTIVE 10
#define NX_HASH 11
#define NX_MAX_CLIENTS 12
#define NX_CUR_CLIENTS 13
#define NX_DN_MSG 14
#define NX_DN_DATA 15
#define NX_DN_CONT 16
#define NX_DN_EOM 17
#define NX_UNSOL 18
#define NX_IOCTL_RECV 19
#define NX_DN_DROP 20
#define NX_UP_RECV 21
#define NX_UP_NCP_SENT 22
#define NX_UP_SYS_SENT 23
#define NX_UP_BURST_SENT 24
#define NX_UP_BURST_WRITE 25
#define NX_UP_BURST_FRAG 26
#define NX_UP_BURST_EXPEDITE 27
#define NX_UP_DROP 28
#define NX_UP_BURST_DROP 29
#define NX_CLIENT_BUSY 30
#define NX_SERVER_BUSY 31
#define NX_HASH_ENTRIES 32
#define NX_HASH_MISS 33
#define NX_HOLDOFF 34
#define NX_ROOT 35

/* ************************* end of file ********************* */
#endif /* for __nwutil_h__ */
