 


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



#ifndef __NWCMMSGS_H__
#define __NWCMMSGS_H__

/*  DEVELOPERS:  Do NOT add messages or constants to this file */
/*  */
/*  You must go to file "nls/English/nwcmmsg.m4" and make additions there. */
/*   */
#define MSG_NWCM_REV_SET 1
#define MSG_NWCM_REV 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	MSG_NWCM_REV_STR "@(#)$Id$" 
#define MSG_NWCM_FOLD_SET 3
#define NWCM_FN_NONE 1
#define NWCM_FN_GENERAL 2
#define NWCM_FN_NDS 3
#define NWCM_FN_AFP 4
#define NWCM_FN_SYSTUNE 5
#define NWCM_FN_DEFAULT 6
#define NWCM_FN_LOCALE 7
#define NWCM_FN_IPXSPX 8
#define NWCM_FN_SAP 9
#define NWCM_FN_APPLETALK 10
#define NWCM_FN_ATPS 11
#define NWCM_FN_NWUM 12
#define NWCM_FN_PSERVER 13
#define NWCM_FN_NPRINTER 14
#define NWCM_FN_NVT 15
#define NWCM_FN_TS 16
#define NWCM_FN_NUC 17
#define NWCM_FM_NONE 101
#define NWCM_FM_GENERAL 102
#define NWCM_FM_NDS 103
#define NWCM_FM_AFP 104
#define NWCM_FM_SYSTUNE 105
#define NWCM_FM_DEFAULT 106
#define NWCM_FM_LOCALE 107
#define NWCM_FM_IPXSPX 108
#define NWCM_FM_SAP 109
#define NWCM_FM_APPLETALK 110
#define NWCM_FM_ATPS 111
#define NWCM_FM_NWUM 112
#define NWCM_FM_PSERVER 113
#define NWCM_FM_NPRINTER 114
#define NWCM_FM_NVT 115
#define NWCM_FM_TS 116
#define NWCM_FM_NUC 117

#define MSG_NWNET_DH_SET 4


#define NWCM_PN_DEFAULT 1
#define NWCM_PN_SERVER_NAME 2
#define NWCM_PN_IPX_AUTO_DISCOVERY 3
#define NWCM_PN_IPX_INTERNAL_NETWORK 4
#define NWCM_PN_ROUTER_HASH_BUCKETS 5
#define NWCM_PN_ROUTER_DRIVER_NAME 6
#define NWCM_PN_ROUTER_DRIVER_DAEMON 7
#define NWCM_PN_ROUTER_TYPE 8
#define NWCM_PN_SAP_LOG_FILE 11
#define NWCM_PN_SAP_DUMP_FILE 12
#define NWCM_PN_SAP_TRACK_FILE 13
#define NWCM_PN_SAP_MAX_MESSAGES 14
#define NWCM_PN_SAP_SERVERS 15
#define NWCM_PN_SAP_PRIORITY 16
#define NWCM_PN_SPX 17
#define NWCM_PN_SPX_MAX_SOCKETS 18
#define NWCM_PN_SPX_MAX_CONNECTIONS 21
#define NWCM_PN_NVT 22
#define NWCM_PN_IPX_BOOT 23

#define NWCM_PN_LAN_NETWORK 24
#define NWCM_PN_LAN_ADAPTER 25
#define NWCM_PN_LAN_PPA 26
#define NWCM_PN_LAN_ADAPTER_TYPE 27
#define NWCM_PN_LAN_FRAME_TYPE 28
#define NWCM_PN_LAN_IF_NAME 31

#define NWCM_PN_LAN_RIP_BCST_INTERVAL 32
#define NWCM_PN_LAN_RIP_AGEOUT_INTERVALS 33
#define NWCM_PN_LAN_RIP_MAX_PKT_SIZE 34
#define NWCM_PN_LAN_RIP_SND_CHG_ONLY 35
#define NWCM_PN_LAN_RIP_PKT_DLY 36
#define NWCM_PN_LAN_SAP_BCST_INTERVAL 37
#define NWCM_PN_LAN_SAP_AGEOUT_INTERVALS 38
#define NWCM_PN_LAN_SAP_MAX_PKT_SIZE 41
#define NWCM_PN_LAN_SAP_SND_CHG_ONLY 42
#define NWCM_PN_LAN_SAP_PKT_DLY 43
#define NWCM_PN_LAN_SAP_RPLY_GNS 44
#define NWCM_PN_LAN_KBPS 45
#define NWCM_PN_IPX_MAX_HOPS 46

#define NWCM_PN_DIAG 47
#define NWCM_PN_DIAG_DAEMON 48
#define NWCM_PN_DIAG_LOG 51
#define NWCM_PN_BINARY_DIRECTORY 52
#define NWCM_PN_LOG_DIRECTORY 53
#define NWCM_PN_NETBIOS 54
#define NWCM_PN_NETBIOS_SHIM 55
#define NWCM_PN_SAP_FILE_COMPATIBILITY 56
#define NWCM_PN_SAP_FAST_INIT 57
#define NWCM_PN_SAP_REMOTE_APPS 58
#define NWCM_PN_SAP_INSTALL_SERVER 61
#define NWCM_PN_SAP_UNIXWARE 62
#define NWCM_PH_DEFAULT 101
#define NWCM_PH_SERVER_NAME 102
#define NWCM_PH_IPX_AUTO_DISCOVERY 103
#define NWCM_PH_IPX_INTERNAL_NETWORK 104
#define NWCM_PH_ROUTER_HASH_BUCKETS 105
#define NWCM_PH_ROUTER_DRIVER_NAME 106
#define NWCM_PH_ROUTER_DRIVER_DAEMON 108
#define NWCM_PH_ROUTER_TYPE 111
#define NWCM_PH_SAP_LOG_FILE 112
#define NWCM_PH_SAP_DUMP_FILE 113
#define NWCM_PH_SAP_TRACK_FILE 114
#define NWCM_PH_SAP_MAX_MESSAGES 115
#define NWCM_PH_SAP_SERVERS 116
#define NWCM_PH_SAP_PRIORITY 117
#define NWCM_PH_SPX 121
#define NWCM_PH_SPX_MAX_SOCKETS 122
#define NWCM_PH_SPX_MAX_CONNECTIONS 123
#define NWCM_PH_NVT 124
#define NWCM_PH_IPX_BOOT 125
#define NWCM_PH_LAN_NETWORK 131
#define NWCM_PH_LAN_ADAPTER 132
#define NWCM_PH_LAN_PPA 133
#define NWCM_PH_LAN_ADAPTER_TYPE 134
#define NWCM_PH_LAN_FRAME_TYPE 135
#define NWCM_PH_LAN_IF_NAME 136
#define NWCM_PH_LAN_RIP_BCST_INTERVAL 137
#define NWCM_PH_LAN_RIP_AGEOUT_INTERVALS 138
#define NWCM_PH_LAN_RIP_MAX_PKT_SIZE 141
#define NWCM_PH_LAN_RIP_SND_CHG_ONLY 142
#define NWCM_PH_LAN_RIP_PKT_DLY 143
#define NWCM_PH_LAN_SAP_BCST_INTERVAL 144
#define NWCM_PH_LAN_SAP_AGEOUT_INTERVALS 145
#define NWCM_PH_LAN_SAP_MAX_PKT_SIZE 146
#define NWCM_PH_LAN_SAP_SND_CHG_ONLY 147
#define NWCM_PH_LAN_SAP_PKT_DLY 148
#define NWCM_PH_LAN_SAP_RPLY_GNS 151
#define NWCM_PH_LAN_KBPS 152
#define NWCM_PH_IPX_MAX_HOPS 153
#define NWCM_PH_DIAG 154
#define NWCM_PH_DIAG_DAEMON 155
#define NWCM_PH_DIAG_LOG 156
#define NWCM_PH_BINARY_DIRECTORY 161
#define NWCM_PH_LOG_DIRECTORY 162
#define NWCM_PH_NETBIOS 163
#define NWCM_PH_NETBIOS_SHIM 164
#define NWCM_PH_SAP_FILE_COMPATIBILITY 165
#define NWCM_PH_SAP_FAST_INIT 166
#define NWCM_PH_SAP_REMOTE_APPS 167
#define NWCM_PH_SAP_INSTALL_SERVER 168
#define NWCM_PH_SAP_UNIXWARE 171
#define NW_NODEV 200
#define NW_BADDEV 201
#define NW_NAMLEN 202
#define NW_NAMCHR 203
#define NW_DUP_NET 230

#define NW_NO_INT_NET 231
#define NW_FRAME_MATCH 232


#endif /* __NWCMMSG_H__ */
