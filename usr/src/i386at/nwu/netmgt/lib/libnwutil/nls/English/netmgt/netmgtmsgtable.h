 


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



#if !defined(__netmgtmsgtable_h__)
#define __netmgtmsgtable_h__

/*  DEVELOPERS:  Do NOT add messages or constants to this file */
/*  */
/*  You must go to file "nls/English/netmgtmsg.m4" and make additions there. */
/*   */
#define MSG_NETMGT_REV_SET 1
#define MSG_NETMGT_REV 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	MSG_NETMGT_REV_STR "@(#)$Id$" 

#define MSG_NWUMPSD_SET 2


#define NWUMPSD_IDENTITY 1
#define NWUMPSD_KILLING_NPSD 2
#define NWUMPSD_CONFIGURATION 3
#define NWUMPSD_FORK_FAIL 4
#define NWUMPSD_SESSION 5
#define NWUMPSD_HANGUP_SIG 6
#define NWUMPSD_START 7
#define NWUMPSD_CFG_ERROR 8
#define NWUMPSD_ERROR_EXIT 9
#define NWUMPSD_NORMAL_EXIT 10
#define NWUMPSD_BAD_CONFIG 11
#define NWUMPSD_IOCTL_TO_FAILED 12

#define NWUMPSD_LAN_INFO 13
#define NWUMPSD_CFG_LAN_X_X 14
#define NWUMPSD_ABORT 15
#define NWUMPSD_OPEN_FAIL 16
#define NWUMPSD_TOPEN_FAIL 17
#define NWUMPSD_TBIND_FAIL 18
#define NWUMPSD_POLL 19
#define NWUMPSD_POLL_SMUX 20

#define NWUMPSD_SMUX_INIT_ERROR 50
#define NWUMPSD_SMUX_PEER_ERROR 51
#define NWUMPSD_SMUX_SIMPLE_OPEN_FAILED 52
#define NWUMPSD_SMUX_SIMPLE_OPEN 53
#define NWUMPSD_SMUX_REGISTER 54
#define NWUMPSD_SMUX_WAIT 55
#define NWUMPSD_SMUX_REGISTER_FAILED 56
#define NWUMPSD_SMUX_TRAP 57
#define NWUMPSD_SMUX_CLOSE 58
#define NWUMPSD_SMUX_UNEXPECTED_OPERATIONS 59
#define NWUMPSD_SMUX_BAD_OPERATIONS 60
#define NWUMPSD_SMUX_RESPONSE 61

#define NWUMPSD_READOBJECT_ERROR 75
#define NWUMPSD_TEXT2OBJ_FAILED 76
#define NWUMPSD_DEF_PATH 77

#define MSG_NWUMD_SET 3


#define NWUMD_IDENTITY 1
#define NWUMD_DAEMON_GOING_DOWN 2
#define NWUMD_ATTACH_SHAREMEM_SUCCESS 3
#define NWUMD_ATTACH_SHAREMEM_ERROR 4
#define NWUMD_ABORT 5

#define NWUMD_SMUX_INIT_ERROR 50
#define NWUMD_SMUX_PEER_ERROR 51
#define NWUMD_SMUX_SIMPLE_OPEN_FAILED 52
#define NWUMD_SMUX_SIMPLE_OPEN 53
#define NWUMD_SMUX_REGISTER_SUCCESS 54
#define NWUMD_SMUX_WAIT 55
#define NWUMD_SMUX_REGISTER_FAILED 56
#define NWUMD_SMUX_TRAP 57
#define NWUMD_SMUX_CLOSE 58
#define NWUMD_SMUX_UNEXPECTED_OPERATIONS 59
#define NWUMD_SMUX_BAD_OPERATIONS 60
#define NWUMD_SMUX_RESPONSE 61

#define NWUMD_READOBJECT_ERROR 75
#define NWUMD_TEXT2OBJ_ERROR 76
#define NWUMD_XSELECT_ERROR 77
#define NWUMD_SMUX_REGISTER_REQUEST_FAILED 78
#define NWUMD_SENDTRAP 100
#define NWUMD_TRAP_TIME_ERROR 101
/*  */
/* NETMGT Helps and Parameter Descriptions */
/*  */
#define MSG_NETMGT_DH_SET 4
/*  */
/* Descriptions Messages */
/*  */
#define NWCM_PN_NM_ETC_DIRECTORY 1
#define NWCM_PN_NWUMPS 2
#define NWCM_PN_NWUMPS_DAEMON 3
#define NWCM_PN_NWUM 4
#define NWCM_PN_NWUM_DAEMON 5
#define NWCM_PN_NWUM_TRAP_TIME 6
/*  */
/* Help Messages */
/*  */
#define NWCM_PH_NM_ETC_DIRECTORY 10
#define NWCM_PH_NWUMPS 11
#define NWCM_PH_NWUMPS_DAEMON 12
#define NWCM_PH_NWUM 13
#define NWCM_PH_NWUM_DAEMON 14
#define NWCM_PH_NWUM_TRAP_TIME 15

#endif /* for __netmgtmsgtable_h__ */
/* ************************* end of file ********************* */




