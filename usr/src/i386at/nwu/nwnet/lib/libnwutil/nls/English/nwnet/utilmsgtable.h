 


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



#if !defined(__utilmsgtable_h__)
#define __utilmsgtable_h__

/*  DEVELOPERS:  Do NOT add messages or constants to this file */
/*  */
/*  You must go to file "nls/English/utilmsg.m4" and make additions there. */
/*   */
#define MSG_UTIL_REV_SET 1
#define MSG_UTIL_REV 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	MSG_UTIL_REV_STR "@(#)$Id$" 
/* Defines for MSG_SAPL Domain */
#define MSG_SAPL_SET 2


#define SAPM_SERVTYPE 1
#define SAPM_SERVNAME 2
#define SAPM_INVALFUNC 3
#define SAPM_NORESP 4
#define SAPM_DUPCALLBACK 5
#define SAPM_SIGNAL 6
#define SAPM_NWCM 7
#define SAPM_INVALSOCKET 8
#define SAPM_LOCAL_ENOMEM 9
#define SAPM_NOT_SUPPORTED 10
#define SAPM_TRY_AGAIN 11
#define SAPM_UNUSED_12 12
#define SAPM_UNUSED_13 13
#define SAPM_UNUSED_14 14
#define SAPM_UNUSED_15 15
#define SAPM_UNUSED_16 16
#define SAPM_UNUSED_17 17
#define SAPM_UNUSED_18 18
#define SAPM_UNUSED_19 19
#define SAPM_UNUSED_20 20
#define SAPM_SOCKET 21
#define SAPM_FTOK 22
#define SAPM_SHMGET 23
#define SAPM_SHMAT 24
#define SAPM_IPXOPEN 25
#define SAPM_GETMSG 26
#define SAPM_PUTMSG 27
#define SAPM_OSAPOUTS 28
#define SAPM_RWSAPOUT 29
#define SAPM_BADSAPOUT 30
#define SAPM_UNUSED_31 31
#define SAPM_UNUSED_32 32
#define SAPM_UNUSED_33 33
#define SAPM_UNUSED_34 34
#define SAPM_UNUSED_35 35
#define SAPM_UNUSED_36 36
#define SAPM_UNUSED_37 37
#define SAPM_UNUSED_38 38
#define SAPM_UNUSED_39 39

#define SAPM_ENOMEM 40
#define SAPM_BUSY 41
#define SAPM_NOFIND 42
#define SAPM_NOPERM 43
#define SAPM_PID_INVAL 44
#define SAPM_NAME_ZERO 45
#define SAPM_INSUF_PERM 46
#define SAPM_UNUSED_47 47
#define SAPM_UNUSED_48 48
#define SAPM_UNUSED_49 49

#define SAPS_INV_TYPE 50
#define SAPS_RIP_OPEN 51
#define SAPS_RIP_IOCTL 52
#define SAPS_NWCM_DIR 53
#define SAPS_NWCM_PARM 54
#define SAPS_SAP_FORK 55
#define SAPS_SAP_EXEC 56
#define SAPS_NPSD 57
#define SAPS_SERV 58
#define SAPS_ADDR 59
#define SAPS_NSAP_FORK 60
#define SAPM_UNUSED_61 61
#define SAPM_UNUSED_62 62
#define SAPM_OUT_RANGE 63


#define MSG_NWCM_SET 3


#define NWCM_NOT_FOUND 1
#define NWCM_INVALID_TYPE 2
#define NWCM_INVALID_DATA 3
#define NWCM_NOT_INITIALIZED 4
#define NWCM_INIT_FAILED 5
#define NWCM_SYNTAX_ERROR 6
#define NWCM_CONFIG_READ_ONLY 7
#define NWCM_SYSTEM_ERROR 8
#define NWCM_CONFIG_NOT_LOCKED 9
#define NWCM_LOCK_FAILED 10
#define NWCM_UNLOCK_FAILED 11
#define NWCM_NOT_IMPLEMENTED 12
#define NWCM_FOLDER_OUT_OF_RANGE 13
#define NWCM_CMD_UNKNOWN_OPTION 101
#define NWCM_CMD_ILLEGAL_HELP_OPTION 102
#define NWCM_CMD_FOLDER_OUT_OF_RANGE 103
#define NWCM_CMD_LOOKUP_FAILED 104
#define NWCM_CMD_SETTODEFAULT_FAILED 105
#define NWCM_CMD_MISSING_VALUE_ON_SET 106
#define NWCM_CMD_SETPARAM_FAILED 107
#define NWCM_CMD_UNKNOWN_ARGUMENT 108
#define NWCM_CMD_OPTION_REQ_PARAM 109
#define NWCM_CMD_MISSING_VALUE_ON_VALIDATE 110
#define NWCM_CMD_VALIDATEPARAM_FAILED 111
#define NWCM_CMD_USAGE_1 112
#define NWCM_CMD_USAGE_2 113
#define NWCM_CMD_USAGE_3 114
#define NWCM_CMD_USAGE_4 115
#define NWCM_CMD_USAGE_5 116
#define NWCM_CMD_USAGE_6 117
#define NWCM_CMD_USAGE_7 118
#define NWCM_CMD_USAGE_8 119
#define NWCM_CMD_USAGE_9 120
#define NWCM_CMD_USAGE_10 121
#define NWCM_CMD_USAGE_11 122
#define NWCM_CMD_USAGE_12 123
#define NWCM_CMD_USAGE_13 124
#define NWCM_CMD_USAGE_14 125
#define NWCM_CMD_USAGE_15 126
#define NWCM_CMD_USAGE_16 127
#define NWCM_CMD_USAGE_17 128
#define NWCM_CMD_USAGE_18 129
#define NWCM_CMD_USAGE_19 130
#define NWCM_XCMD_UNKNOWN_FILE_MENU_OPTION 131
#define NWCM_XCMD_UNKNOWN_EDIT_MENU_OPTION 132
#define NWCM_XCMD_UNKNOWN_VIEW_MENU_OPTION 133
#define NWCM_XCMD_UNKNOWN_HELP_MENU_OPTION 134
#define NWCM_XCMD_ENABLED 135
#define NWCM_XCMD_DISABLED 136
#define NWCM_XCMD_GARBAGE_DIALOG_FMT 137
#define NWCM_XCMD_SETFAIL_DIALOG_FMT 138
#define NWCM_XCMD_UPDATE_DIALOG_FMT 140
#define NWCM_XCMD_SETPARAM_UNSUPPORTED_TYPE 141
#define NWCM_NOT_A_STRING_PARAMETER 142
#define NWCM_GET_STRINGS_FAILED 143
#define NWCM_FREE_STRINGS_FAILED 144
#define NWCM_NO_STRING_TABLE 145
#define NWCM_FOLDER_IS_VALID 146
#define NWCM_FOLDER_IS_NOT_VALID 147
#define NWCM_FOLDER_NOT_VALID 148
#define NWCM_DIR_ACCESS 149
#define NWCM_NO_DIR 150
#define NWCM_NO_CHR 151
#define NWCM_CMD_USAGE_20 161
#define NWCM_CMD_USAGE_21 162

/* ************************* end of file ********************* */
#endif /* for __utilmsgtable_h__ */
