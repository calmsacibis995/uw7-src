 


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



#if !defined(__npsmsgtable_h__)
#define __npsmsgtable_h__

/*  DEVELOPERS:  Do NOT add messages or constants to this file */
/*  */
/*  You must go to file "nls/English/npsmsg.m4" and make additions there. */
/*  */
#define MSG_NPS_REV_SET 1
#define MSG_NPS_REV 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	MSG_NPS_REV_STR "@(#)$Id$" 

#define MSG_NPSD_SET 2


#define NPS_ROUTER 9
#define NPS_IDENTITY 10
#define NPS_CONFIGURATION 11
#define NPS_SAP_SPAWNED 12
#define NPS_DIAG_SPAWNED 13
#define NPS_SPX 14
#define NPS_NBIOS 15
#define NPS_I_NET 16
#define NPS_NO_I_NET 17
#define NPS_NVT 18
#define NPS_NMPS_SPAWNED 19
#define NPS_NETM_EXEC_FAIL 20
#define NPS_NO_SAPD 21
#define NPS_LAN_INFO 22
#define NPS_PPA_INFO 23
#define NPS_IFNAME_INFO 24
#define NPS_DIAG_EXEC_FAIL 25
#define NPS_BLANK_LINE 26
#define NPS_HEADING_TEXT 27
#define NPS_HEADING_DASHES 28
#define NPS_HEADING_FORMAT 29
#define NPS_UNBOUND 30
#define NPS_LINKED 31
#define NPS_NET_SET 32
#define NPS_IDLE 33
#define NPS_UNKNOWN 34
#define NPS_STR_ERROR 40
#define NPS_STR_OK 41
#define NPS_USAGE 50
#define NPS_NO_SOCKETS 58
#define NPS_NO_SHM_NBIOS 60
#define NPS_ADAPTER_FUNCTION 61
#define NPS_CFG_LAN_X_X 62
#define NPS_CFG_X 63
#define NPS_CFG_LAN_X_FRAME 66
#define NPS_CFG_LAN_X_TYPE 67
#define NPS_CFG_LAN_X_ADAPTER 68
#define NPS_ALLOC_INFO 69
#define NPS_ALLOC_FILED 70
#define NPS_NOLOG 71
#define NPS_BAD_CONFIG 72
#define NPS_ERROR_EXIT 73
#define NPS_NVTD_NEEDS_SPX 74
#define NPS_NVTD_FORK_ERR 75
#define NPS_NVTD_ACTIVE_AIX 76
#define NPS_NVTD_INACTIVE_AIX 77
#define NPS_NVTD_EXEC_FAIL 78
#define NPS_KILLING_NPSD 87
#define NPS_NEED_INTERNAL 88
#define NPS_SAP_MAP 89
#define NPS_DUP_NET 90
#define NPS_RUN_UTILITY_AS_ROOT 91
#define NPS_DLPI_PUTMSG 100
#define NPS_DLPI_GETMSG 101
#define NPS_DLPI_INFO 102
#define NPS_DLPI_ATTACH 103
#define NPS_DLPI_ATTACH_PUTMSG 104
#define NPS_DLPI_ATTACH_GETMSG 105
#define NPS_DLPI_DI_GETMSG 106
#define NPS_DLPI_BIND_PUTMSG 107
#define NPS_DLPI_BIND_GETMSG 108
#define NPS_DLPI_BIND 109
#define NPS_DLPI_SUBBIND_PUTMSG 110
#define NPS_DLPI_SUBBIND_GETMSG 111
#define NPS_DLPI_SUBBIND 112
#define NPS_SAP_FAILED 119
#define NPS_OPEN_FAILED 121
#define NPS_LINK_FAILED 122
#define NPS_IOCTL_TO_FAILED 123
#define NPS_PUSH_TO_FAILED 125
#define NPS_IOCTL_FAIL 129
#define NPS_CLOSEXEC 130
#define NPS_DLPI_X 150
#define NPS_DLPI_UNKNOWN 151
#define NPS_DLPI_BEGIN 160
#define NPS_DLPI_ACCESS 170
#define NPS_DLPI_BADADDR 171
#define NPS_DLPI_BADCORR 172
#define NPS_DLPI_BADDATA 173
#define NPS_DLPI_BADPPA 174
#define NPS_DLPI_BADPRIM 175
#define NPS_DLPI_BADQOSPARAM 176
#define NPS_DLPI_BADQOSTYPE 177
#define NPS_DLPI_BADSAP 178
#define NPS_DLPI_BADTOKEN 179
#define NPS_DLPI_BOUND 180
#define NPS_DLPI_INITFAILED 181
#define NPS_DLPI_NOADDR 182
#define NPS_DLPI_NOTINIT 183
#define NPS_DLPI_OUTSTATE 184
#define NPS_DLPI_SYSERR 185
#define NPS_DLPI_UNSUPPORTED 186
#define NPS_DLPI_UNDELIVERABLE 187
#define NPS_DLPI_NOTSUPPORTED 188
#define NPS_DLPI_TOOMANY 189
#define NPS_DLPI_NOTENAB 190
#define NPS_DLPI_BUSY 191
#define NPS_DLPI_NOAUTO 192
#define NPS_DLPI_NOXIDAUTO 193
#define NPS_DLPI_NOTESTAUTO 194
#define NPS_DLPI_XIDAUTO 195
#define NPS_DLPI_TESTAUTO 196
#define NPS_DLPI_PENDING 197
#define MSG_DIAG_SET 3


#define DIAG_NO_TIME 1
#define DIAG_FORK_FAIL 11
#define DIAG_SESSION 12
#define DIAG_IOCTL_NET 13
#define DIAG_IOCTL 14
#define DIAG_T_RCV 15
#define DIAG_OPEN_FAIL 16
#define DIAG_TOPEN_FAIL 17
#define DIAG_TBIND_FAIL 18
#define DIAG_TALLOC_FAIL 19
#define DIAG_SAPQ_FAIL 20
#define DIAG_SAPR_FAIL 21
#define DIAG_FUNC_FAIL 22
#define DIAG_CFG 23
#define DIAG_UDATA_FAIL 24
#define DIAG_LISTEN_FAIL 25
#define DIAG_POLL 30
#define DIAG_NORECV 31
#define DIAG_BADPKT 32
#define DIAG_ALLOC_LINFO 34
#define DIAG_ALLOC_POLL 35
#define DIAG_NETSTBL 36
#define DIAG_GETNET 37
#define DIAG_SRVRSTBL 38
#define DIAG_SERVERS 40
#define DIAG_SERVERT 41
#define DIAG_SIZE 42
#define DIAG_COUNT 43
#define DIAG_BCALL 44
#define DIAG_MAP 45
#define DIAG_START 50
#define DIAG_ERROR_EXIT 51
#define DIAG_NORMAL_EXIT 52
#define DIAG_KILLING_NPSD 54

#define MSG_SAPD_SET 4


#define SAP_NO_TIME 1
#define SAP_ACTIVE 2
#define SAP_READ_FAIL 10
#define SAP_PUTMSG_FAIL 11
#define SAP_CHK_SRC 12
#define SAP_OPEN_FAIL 13
#define SAP_IOCTL_INFO 14
#define SAP_IOCTL_MAXLAN 15
#define SAP_NET_LIST 16
#define SAP_NICE_FAIL 17
#define SAP_FORK_FAIL 18
#define SAP_SESSION 21
#define SAP_MAP 22
#define SAP_SET_RIPQ 23
#define SAP_FAST_INIT 24
#define SAP_HANGUP 30
#define SAP_HANGUP_SIG 31
#define SAP_TERM_SIG 32
#define SAP_PACKET_LEN 33
#define SAP_ALLOC_BUF 34
#define SAP_NO_NET 35
#define SAP_ADDR_CHG 36
#define SAP_ALLOC_LOCALNET 37
#define SAP_DUP_ADDR 38
#define SAP_ALLOC_SRVENT 39
#define SAP_ALLOC_INFO 40
#define SAP_ALARM 41
#define SAP_RESOLVE 42
#define SAP_MAX_BUF 43
#define SAP_POLL_ERR 44
#define SAP_ALLOC_LIST 45
#define SAP_PRIOR_VALUE 47
#define SAP_PRIOR_ADJUST 48
#define SAP_SERVERS 50
#define SAP_NOT_COUNTED 51
#define SAP_POOL_EMPTY 54
#define SAP_POOL_BAD 55
#define SAP_READ_SIZE 57
#define SAP_PACKET_LEN_BAD 58
#define SAP_IN_GSR 61 
#define SAP_IN_GSQ 62 
#define SAP_IN_NSQ 63 
#define SAP_IN_TYPE 64 
#define SAP_OUT_GSR 65 
#define SAP_OUT_GSQ 66 
#define SAP_OUT_NSQ 67 
#define SAP_OUT_NSR 68 
#define SAP_OUT_TYPE 69 
#define SAP_UNXP_NSR 70
#define SAP_UNK 71
#define SAP_END_FILE 72
#define SAP_START 80
#define SAP_ERROR_EXIT 81
#define SAP_NORMAL_EXIT 82
#define SAP_INITIALIZED 83
#define SAP_NOLOG 84
#define SAP_BAD_CONFIG 85
#define SAP_SYSCALL_FAIL 86
#define SAP_KILLING_NPSD 87
#define SAP_SERVER_INFO 90
#define SAP_ADD_LOCAL 91
#define SAP_DELETLE_LOCAL 92
#define SAP_RIP_DELETLE_LOCAL 93
#define SAP_AGE_LOCAL 94

#define MSG_NVTD_SET 5


#define NVT_CFG_X 1
#define NVT_MKDIR_FAIL 2
#define NVT_FSTAT_FAIL 3
#define NVT_FS_NODE_FAIL 4
#define NVT_CHOWN_FAIL 5
#define NVT_CHMOD_FAIL 6
#define NVT_OPEN_FAIL 7
#define NVT_DUP_FAIL 8
#define NVT_STTY_FAIL 9
#define NVT_MAKEUTX_FAIL 10
#define NVT_EXEC_FAIL 11

#define MSG_IPXI_SET 6


#define I_NEWLINE 1
#define I_IPX_OPEN_FAIL 2
#define I_IPX_STATS_FAIL 3
#define I_LAN_ACTIVE 4
#define I_IPX_VERSION 5
#define I_LIPMX_VERSION 6
#define I_LAN_STATS 8
#define I_SOCKET_STATS 9
#define I_IINFO 10
#define I_DLPI_SIZE 11
#define I_DLPI_TYPE 12
#define I_COMB 13
#define I_UP_PROP 14
#define I_UP_NOT_PROP 15
#define I_RECD_LAN 20
#define I_SMALL 21
#define I_ECHO 30
#define I_RIP 31
#define I_RIP_DROP 32
#define I_RIP_ROUTE 33
#define I_SAP 40
#define I_SAP_INVAL 41
#define I_SAP_ROUTE_IPX 42
#define I_SAP_ROUTE_SAPS 43
#define I_SAP_DROP 44
#define I_DIAG_MY_NET 50
#define I_DIAG_INT_NET 51
#define I_DIAG_NIC 52
#define I_DIAG_IPX 53
#define I_DIAG_NO_IPX 54
#define I_NIC_DROPPED 55
#define I_IN_BROADCAST 60
#define I_BROADCAST_INT 61
#define I_BROADCAST_NIC 62
#define I_BROAD_DIAG_NIC 63
#define I_BROAD_DIAG_FWD 64
#define I_BROAD_DIAG_RTE 65
#define I_BROAD_DIAG_RESP 66
#define I_BROAD_ROUTE 67
#define I_PKT_FWD 70
#define I_PKT_ROUTE 71
#define I_PKT_ROUTE_INT 72
#define I_OINFO 80
#define I_OUT_PROP 81
#define I_OUT_TOT_STREAM 82
#define I_OUT_TOTAL 83
#define I_FILL_IN_DEST 100
#define I_OUT_SAME_SOCKET 101
#define I_BAD_LAN 102
#define I_OUT_MAX_SDU 103
#define I_NO_LAN 104
#define I_ROUTE_INT 105
#define I_OUT_SENT 106
#define I_OUT_QUEUED 107
#define I_OUT_PACED 108
#define I_IOCTL_TOT 120
#define I_IOCTL_SET_LANS 121
#define I_IOCTL_GET_LANS 122
#define I_IOCTL_SAP_Q 123
#define I_IOCTL_SET_LINFO 124
#define I_IOCTL_GET_LINFO 125
#define I_IOCTL_GET_NODE 126
#define I_IOCTL_GET_NET 127
#define I_IOCTL_GET_STATS 128
#define I_IOCTL_LINK 129
#define I_IOCTL_UNLINK 130
#define I_IOCTL_UNKNOWN 131
#define I_SOCKETS_BOUND 140
#define I_NON_TLI_BIND 141
#define I_TLI_BIND 142
#define I_TLI_OPTMGT 143
#define I_TLI_UNKNOWN 144
#define I_TOTAL_IPX_PKTS 150
#define I_SWITCH_SUM 151
#define I_SWITCH_SUM_FAIL 152
#define I_SWITCH_EVEN 153
#define I_SWITCH_EVEN_ALLOC 154
#define I_SWITCH_ALLOC_FAIL 155
#define I_SWITCH_INVAL_SOCK 156
#define I_NON_TLI_PKTS 160
#define I_OUT_BAD_SIZE 161
#define I_OUT_TO_SWITCH 162
#define I_TLI_OUT_PKTS 164
#define I_TLI_BAD_STATE 165
#define I_TLI_BAD_SIZE 166
#define I_TLI_BAD_OPT 167
#define I_TLI_FAIL_ALLOC 168
#define I_TLI_TO_SWITCH 169
#define I_IPX_IN 200
#define I_TRIM_PKT 201
#define I_IN_BAD_SIZE 202
#define I_SUM_FAIL 203
#define I_BUSY_SOCK 204
#define I_SOCK_NOT_BOUND 205
#define I_IPX_ROUTED_TLI_ALLOC 206
#define I_DATA_TO_SOCK 210
#define I_IPX_ROUTED 211
#define I_IPX_SENT_TLI 212
#define I_TOTAL_IOCTLS 220
#define I_IOC_SET_WATER 221
#define I_IOC_SET_BIND 222
#define I_IOC_UNBIND 223
#define I_IOC_STATS 224
#define I_IOC_UNKNOWN 227
#define I_ROOT 228

#define MSG_RIPI_SET 7


#define R_NEWLINE 1
#define R_RIPX_OPEN_FAIL 2
#define R_RIPX_STATS_FAIL 3
#define R_RIP_ACTIVE 4
#define R_RIPX_VERSION 5
#define R_RECEIVED_PKTS 10
#define R_NO_LAN_KEY 11
#define R_RECV_BAD_LEN 12
#define R_RECV_COALESCE 13
#define R_COALESCE_FAIL 14
#define R_ROUTER_REQ 15
#define R_ROUTER_RESP 16
#define R_UNKNOWN_REQ 17
#define R_TOT_SENT 20
#define R_SENT_ALLOC_FAIL 21
#define R_SENT_BAD_DEST 22
#define R_SENT_REQ_PKTS 23
#define R_SENT_RESP_PKTS 24
#define R_SENT_LAN0_DROPPED 30
#define R_SENT_LAN0_ROUTED 31
#define R_IOCTLS 40
#define R_IOC_INITIALIZE 41
#define R_IOC_GET_HASH_SIZE 42
#define R_IOC_GET_HASH_STATS 43
#define R_IOC_DUMP_HASH 44
#define R_IOC_GET_ROUTER 45
#define R_IOC_GET_NET_INFO 46
#define R_IOC_CHK_SAP_SRC 47
#define R_IOC_RESET_ROUTER 48
#define R_IOC_DOWN_ROUTER 49
#define R_IOC_STATS 50
#define R_IOC_UNKNOWN 51
#define R_ROOT 52

#define MSG_SAPI_SET 8


#define A_NEWLINE 1
#define A_INVALID_NUMERIC 2
#define A_BAD_NUMERIC 3
#define A_UNKNOWN_ARG 4
#define A_USAGE_HEAD 5
#define A_USAGE_1 6
#define A_USAGE_2 7
#define A_USAGE_3 8
#define A_USAGE_4 9
#define A_USAGE_5 10
#define A_USAGE_6 11
#define A_USAGE_7 12
#define A_USAGE_8 13
#define A_USAGE_9 14
#define A_USAGE_10 15
#define A_USAGE_11 16
#define A_USAGE_12 17
#define A_USAGE_13 18
#define A_USAGE_14 19
#define A_USAGE_15 20
#define A_USAGE_16 21
#define A_USAGE_17 22
#define A_USAGE_18 23
#define A_SAP_STATS_UNAVAIL 30
#define A_TIME_SAP_ACTIVE 31
#define A_SAP_ACTIVE 32
#define A_SAP_INACTIVE 33
#define A_KNOWN_SERVERS 34
#define A_UNUSED_SERV_ENT 35
#define A_KNOWN_LANS 36
#define A_REV_STAMP 37
#define A_TOTAL_SAP_RCVD 38
#define A_GSQ_RCVD 39
#define A_GSR_RCVD 40
#define A_NSQ_RCVD 41
#define A_ADVERT_REQS 42
#define A_NOTIFY_REQS 43
#define A_GET_SHMEM_REQS 44
#define A_NOT_NEIGHBOR 45
#define A_ECHO_OUTPUT 46
#define A_BAD_SIZE 47
#define A_BAD_SAP_SOURCE 48
#define A_TOTAL_OUT_SAPS 49
#define A_NSR_SENT 50
#define A_GSR_SENT 51
#define A_GSQ_SENT 52
#define A_SAS_ACKS 53
#define A_SAS_NACKS 54
#define A_SNC_ACKS 55
#define A_SNC_NACKS 56
#define A_GSI_ACKS 57
#define A_BAD_DEST_OUT 58
#define A_SRV_ALLOC_FAILED 59
#define A_MALLOC_FAILED 60
#define A_IN_RIP_SAPS 61
#define A_BAD_RIP_SAPS 62
#define A_RIP_SERVER_DOWN 63
#define A_NOTIFY_PROCS 64
#define A_NOTIFYS_SENT 65
#define A_SERVER_NAME 70
#define A_SERVER_TYPE 71
#define A_SERVER_HOPS 72
#define A_SERVER_ADDR 73
#define A_HEX_ADDR 74
#define A_HEX_BYTE 75
#define A_SPACE 76
#define A_STRING 77
#define A_GET_ALL_SERV_FAIL 80
#define A_NUM_LOC_SERVERS 90
#define A_NUM_SERVERS 91
#define A_TYPE_COUNT 100
#define A_SERVER_AND_TYPE 101
#define A_NOTIFY_FAILED 110
#define A_GET_CHANGED_FAILED 111
#define A_TOTAL_CHANGED 112
#define A_GET_NEAREST_FAILED 113
#define A_TYPE_NOT_FOUND 114
#define A_GET_BY_NAME_FAILED 115
#define A_IPX_ADDRESS 116
#define A_NETWORK 140
#define A_LAN_NUMBER 141
#define A_UPDATE_INTERVAL 142
#define A_AGE_FACTOR 143
#define A_PACKET_GAP 144
#define A_PACKET_SIZE 145
#define A_PACKETS_SENT 146
#define A_PACKETS_RCVD 147
#define A_BAD_PACKETS 148
#define A_GET_LAN_DATA_FAILED 150
#define A_ROOT 151

#define MSG_SPXI_SET 9


#define S_NEWLINE 1
#define S_SPX_OPEN_FAIL 2
#define S_SPX_STATS_FAIL 3
#define S_SPX_CON_STATS_FAIL 4
#define S_SPX_GENERAL 5
#define S_SPX_VERSION 6
#define S_TIME_ACTIVE 10
#define S_MAX_CONNS 11
#define S_CURRENT_CONNS 12
#define S_SIMUL_CONNS 13
#define S_ALLOC_FAIL 14
#define S_OPEN_FAIL 15
#define S_IOCTLS 16
#define S_CONN_REQS 20
#define S_FAIL_CONNS 21
#define S_LISTENS 22
#define S_LISTENS_FAIL 23
#define S_SEND_COUNT 24
#define S_UNKNOWN_COUNT 25
#define S_BAD_SENDS 26
#define S_IPX_SENDS 27
#define S_TIME_RETRANS 28
#define S_NAK_RETRANS 29
#define S_IPX_RECV 40
#define S_BAD_IPX_RECV 41
#define S_BAD_DATA 42
#define S_DUP_DATA 43
#define S_SENT_UP 44
#define S_IPX_CONNS 45
#define S_ABORT_CONN 46
#define S_RETRY_ABORT 47
#define S_NO_LISTENS 48
#define S_CONN_INACTIVE 60
#define S_CONN_STATS 61
#define S_CONN_ACTIVE 62
#define S_MUL_OPEN 69
#define S_CONN_ADDR 70
#define S_NET_ADDR 71
#define S_NODE_ADDR 72
#define S_SOCKET_ADDR 73
#define S_CONN_NUMBER 74
#define S_OTHER_ADDR 75
#define S_CONN_STATE 80
#define S_MAX_RETRIES 81
#define S_MIN_TIMEOUT 82
#define S_DATA_XFER 90
#define S_SPXII_END 91
#define S_SPX_END 92
#define S_UNKNOWN_END 93
#define S_USE_CHKSUM 94
#define S_NO_CHKSUM 95
#define S_RCV_WIN_SIZE 100
#define S_TRAN_WIN_SIZE 101
#define S_TRAN_PKT_SIZE 102
#define S_RCV_PKT_SIZE 103
#define S_ROUND_TRIP 104
#define S_WIN_CLOSED 105
#define S_FLOW_CNTL 106
#define S_APP_TO_SPX 110
#define S_UNK_FROM_APP 111
#define S_BAD_FROM_APP 112
#define S_SENT_TO_IPX 113
#define S_IPX_RESENDS 114
#define S_IPX_NAK_RESEND 115
#define S_IPX_ACKS 116
#define S_IPX_NACKS 117
#define S_IPX_WATCH 118
#define S_CON_RCV_PACKET 130
#define S_CON_RCV_BAD_PACKET 131
#define S_CON_RCV_BAD_DATA 132
#define S_CON_RCV_DUP 133
#define S_CON_RCV_OUTSEQ 134
#define S_CON_RCV_SENTUP 135
#define S_CON_RCV_QUEUED 136
#define S_CON_RCV_ACK 137
#define S_CON_RCV_NACK 138
#define S_CON_RCV_WATCH 139
#define S_ROOT 140

#define MSG_NWD_SET 10


#define NWD_PRIV 1
#define NWD_RETRY 2
#define NWD_TIMEOUT 3
#define NWD_AUTOD 4
#define NWD_CFG_X 5
#define NWD_FRAME 6
#define NWD_FRAME_CHOOSE 7
#define NWD_USAGE 10
#define NWD_USAGEc 11
#define NWD_USAGEa 12
#define NWD_USAGEd 13
#define NWD_USAGED 14
#define NWD_USAGEb 15
#define NWD_USAGEB 16
#define NWD_USAGEP 17
#define NWD_USAGEp 18
#define NWD_USAGEe 19
#define NWD_USAGEf 20
#define NWD_USAGEr 21
#define NWD_USAGEt 22
#define NWD_USAGEv 23
#define NWD_USAGEu 24
#define NWD_INT_NET 25
#define NWD_FRAME1 26
#define NWD_INVENT 27
#define NWD_GOT_NET 29
#define NWD_DUP_NET 30
#define NWD_UPD_CFG 31
#define NWD_TRY 32
#define NWD_BIND_RESP 33
#define NWD_SUBBIND_RESP 34
#define NWD_DLPI_RESP 35
#define NWD_TMP_FILE 40
#define NWD_NO_DEV 41
#define NWD_OPEN 42
#define NWD_RIP_FAIL 43
#define NWD_BIND_REQ 44
#define NWD_BIND_ACK 45
#define NWD_SUBBIND_REQ 46
#define NWD_SUBBIND_ACK 47
#define NWD_GNS_FAIL 49
#define NWD_DLPI_REQ 50
#define NWD_DLPI_ACK 51
#define NWD_UNB_REQ 52
#define NWD_UNB_ACK 53
#define NWD_UNB_RESP 54

#define MSG_SUTIL_SET 11


#define P_EXCLUSIVE 1
#define P_UNKNOWN 2
#define P_USAGE_HEAD 10
#define P_USAGE_1 11
#define P_USAGE_2 12
#define P_USAGE_3 13
#define P_USAGE_4 14
#define P_USAGE_5 15
#define P_USAGE_6 16
#define P_MISSING_ARG1 17
#define P_MISSING_ARG2 18
#define P_INFO_UNAVAIL 20
#define P_NO_ACTION 21
#define P_NO_PERM 22
#define P_DELETE_FAIL 23
#define P_ADD_FAIL 24
#define P_ROOT 25

#define MSG_RROUTER_SET 12


#define RRT_OPEN_FAILED 1
#define RRT_RESET_ROUTER 2
#define RRT_ROOT 3

#define MSG_DROUTER_SET 13


#define DROUT_USAGE1 1
#define DROUT_USAGE2 2
#define DROUT_USAGE3 3
#define DROUT_USAGE4 4
#define DROUT_NEWLINE 10
#define DROUT_HEAD1 11
#define DROUT_HEAD2 12
#define DROUT_FORMAT 13
#define DROUT_EOT 14
#define DROUT_COLUMN 15
#define DROUT_ALLOC 16
#define DROUT_OPEN_FAIL 20
#define DROUT_IOCTL 21
#define DROUT_FUNC_FAIL 23
#define DROUT_ROOT 24

#define MSG_STRT_SET 14


#define NSTRT_CFG_X 1
#define NSTRT_NPS_EXEC_FAIL 2
#define NSTRT_ROOT 3

#define MSG_STAT_SET 15


#define NSTAT_BAD_CONFIG 1
#define NSTAT_ROOT 2

#define MSG_STOP_SET 16


#define NSTOP_BAD_CONFIG 1
#define NSTOP_BAD_INIT 2
#define NSTOP_ROOT 3
#define MSG_NUCSAPD_SET 17


#define NSAP_USAGE 1
#define NSAP_GET_SRV 5
#define NSAP_OPEN 6
#define NSAP_SHM 7
#define NSAP_FORK 8
#define NSAP_OUT 9
#define NSAP_BCAST 10

#endif /* for __npsmsgtable_h__ */
