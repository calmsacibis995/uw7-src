 


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



#if !defined(__npfsutil_h__)
#define __npfsutil_h__

/*  DEVELOPERS:  Do NOT add messages or constants to this file */
/*  */
/*  You must go to file "nls/English/npsmsg.m4" and make additions there. */
/*   */
#define MSG_NPFS_REV_SET 1
#define MSG_NPFS_REV 1 
/* Do NOT touch the following REV string. (MUST match nwumsgs.msg)*/
#define	MSG_NPFS_REV_STR "@(#)$Id$" 
/*  */
/* ************************************************* */
/* Message string constants for NPFS utility programs */
/* ************************************************* */
#define MSG_NPFS_VM_SET 2


#define NPFSVM_TITLE 1
#define NPFSVM_COPYRIGHT 2

#define NPFSVM_MENU_1 3
#define NPFSVM_MENU_2 4
#define NPFSVM_MENU_3 5
#define NPFSVM_MENU_4 6
#define NPFSVM_MENU_5 7

#define NPFSVM_EXISTING_VOLUMES 8
#define NPFSVM_SHOW_VOLUMES 9
#define NPFSVM_SELECT_A_VOLUME 10
#define NPFSVM_ENTER_ADDED_NAME_SPACES 11
#define NPFSVM_ENTER_DELETED_NAME_SPACES 12
#define NPFSVM_OLD_INODES_SAVED 13
#define NPFSVM_OLD_EXTNAMES_SAVED 14
#define NPFSVM_UNSUPPORTED_OPTION 15
#define NPFSVM_ADD_DIR_ENTRY_VERBOSE 16
#define NPFSVM_PROCESS_EMPTY_DIRECTORY_VERBOSE 17
#define NPFSVM_PROCESS_DIRECTORY_VERBOSE 18
#define NPFSVM_DIR_ENTRY_ASSIGNED_BLOCK_VERBOSE 19
#define NPFSVM_DONE_PROCESSING_DIR_VERBOSE 20
#define NPFSVM_NOTHING_ENTERED 21
#define NPFSVM_REMOVING_OLD_INODES_FILE_VERBOSE 22
#define NPFSVM_CHANGE_MOUNT_POINT_QUESTION 23
#define NPFSVM_USAGE_1 24
#define NPFSVM_USAGE_2 25
#define NPFSVM_USAGE_3 26
#define NPFSVM_USAGE_4 27
#define NPFSVM_ENTER_MOUNT_POINT 29
#define NPFSVM_MOUNT_POINT_TOO_LONG 30
#define NPFSVM_MOUNT_POINT_ALREADY_USED 31
#define NPFSVM_SELECT_DIFFERENT_MOUNT_POINT 32
#define NPFSVM_MOUNT_POINT_DOES_NOT_EXIST 33
#define NPFSVM_ADJUST_HIGHEST_ALLOC_BLOCK 34
#define NPFSVM_MANUAL_SET_MOUNT_POINT 35
#define NPFSVM_ADJUST_FIRST_FREE_BLOCK 36
#define NPFSVM_ADJUST_FIRST_DIR_BLOCK 37
#define NPFSVM_ADJUST_NEXT_DIR_BLOCK 38
#define NPFSVM_ADD_BLOCK_TO_FREE_LIST 39
#define NPFSVM_BAD_FREE_DIR_BLOCK_ORDER 40
#define NPFSVM_VERIFY_MORE_THAN_ONE_ERROR 41
#define NPFSVM_ENTER_SELECTION 42




#define NPFSVM_EXIT 50
#define NPFSVM_NO_MEMORY 51
#define NPFSVM_SFD_INIT_FAILURE 52
#define NPFSVM_NO_VOLUMES 53
#define NPFSVM_VOLUME_NOT_VALID 54
#define NPFSVM_VOLTAB_UPDATE_ERROR 55
#define NPFSVM_INODES_FILE_MODIFY_ERROR 56
#define NPFSVM_INVALID_OPTION 57
#define NPFSVM_INODES_EXTEND_FAILURE 58
#define NPFSVM_LONG_MOUNT_POINT_RETRIEVE_ERROR 59
#define NPFSVM_LONG_MOUNT_POINT_ADD_ERROR 60
#define NPFSVM_LONG_UNIX_NAME_RETRIEVE_ERROR 61
#define NPFSVM_LONG_DIR_NAME_RETRIEVE_ERROR 62
#define NPFSVM_LONG_DIR_NAME_ADD_ERROR 63
#define NPFSVM_LONG_OS2_NAME_RETRIEVE_ERROR 64
#define NPFSVM_LONG_OS2_NAME_ADD_ERROR 65
#define NPFSVM_DIR_ENTRY_NOT_FOUND 66
#define NPFSVM_CANT_ADD_DELETE_UNIX_NS 67
#define NPFSVM_CANT_ADD_DELETE_DOS_NS 68
#define NPFSVM_NS_INVALID 69
#define NPFSVM_INVALID_ACTION 70
#define NPFSVM_MAC_NS_ALREADY_ACTIVE 71
#define NPFSVM_MAC_NS_NOT_ACTIVE 72
#define NPFSVM_OS2_NS_ALREADY_ACTIVE 73
#define NPFSVM_OS2_NS_NOT_ACTIVE 74
#define NPFSVM_STAT_FAILED 75
#define NPFSVM_INODES_OPEN_FAILED 76
#define NPFSVM_FILE_ALREADY_EXISTS 77
#define NPFSVM_CANT_CREATE_INODES_FILE 78
#define NPFSVM_CANT_OPEN_EXT_NAMES_FILE 79
#define NPFSVM_CANT_CREATE_EXT_NAMES_FILE 80
#define NPFSVM_UNKNOWN_COMMAND_LINE_ARG 81
#define NPFSVM_UNKNOWN_COMMAND_LINE_OPTION 82
#define NPFSVM_CANT_RUN_WHILE_NETWARE_IS_RUNNING 83
#define NPFSVM_CANT_READ_TRUSTEE_INFO 84
#define NPFSVM_VERIFY_ZERO_ERRORS 85
#define NPFSVM_VERIFY_ONE_ERROR 86
#define NPFSVM_VOLTAB_BACKUP_SAVED 87
#define NPFSVM_CANT_CREATE_MOUNT_POINT 88
/* ************************* end of file ********************* */
#endif /* for __npfsutil_h__ */
