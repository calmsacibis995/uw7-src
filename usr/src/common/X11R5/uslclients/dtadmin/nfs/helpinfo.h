/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/helpinfo.h	1.8"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	helpinfo.h  HelpInfo declarations.  Forward
 *                          declarations are in help.h
 *
 */

#define HYP_RemoteAddHelpSect		 "20"
#define HYP_RemotePropHelpSect		 "120"
#define HYP_LocalAddHelpSect		 "70"
#define HYP_LocalPropHelpSect		 "130"
#define HYP_StatusHelpSect		 "80"
#define HYP_HostHelpSect		 "30"
#define HYP_FindHelpSect		 "100"
#define HYP_FindFolderHelpSect		 "60"
#define HYP_DeleteLocalHelpSect		 "150"
#define HYP_DeleteRemoteHelpSect	 "140"
#define HYP_StopNFSHelpSect		 "85"
#define HYP_CreateMountPointHelpSect	 "24"
#define HYP_CreateFolderHelpSect	 "24"
#define HYP_MainHelpSect		 "10"
#define HYP_TOCHelpSect			 "TOC"
#define HYP_HelpDeskHelpSect		 "HelpDesk"
#define HYP_MountPointNotEmptyHelpSect	 "25"

/* FIX: figure out where context sensitive help should jump to */

#define HYP_HostListHelpSect		"30"
#define HYP_HostPopupMenuHelpSect	"30"
#define HYP_HostPopupHelpSect		"30"
#define HYP_LocalFolderLabelHelpSect	"70"
#define HYP_LocalLocalPathHelpSect	"70"
#define HYP_LocalFindHelpSect		"100"
#define HYP_LocalFindMenuHelpSect	"100"
#define HYP_LocalFindPopupHelpSect	"100"
#define HYP_BootAdvHelpSect		"70"
#define HYP_DefaultAccessHelpSect	"70"
#define HYP_ExceptionTextHelpSect	"70"
#define HYP_ExceptionListHelpSect	"70"
#define HYP_ExceptionHostHelpSect	"70"
#define HYP_LocalGetHostHelpSect	"30"
#define HYP_ExceptionMenuHelpSect	"70"
#define HYP_LocalExtendedHelpSect	"70"
#define HYP_LocalCustomHelpSect		"70"
#define HYP_AdvMenuHelpSect		"70"

extern HelpInfo RemoteAddWindowHelp =
{ FormalClientName, TXT_RemoteAddHelpTitle, HELPPATH, HYP_RemoteAddHelpSect };

extern HelpInfo RemotePropWindowHelp =
{ FormalClientName, TXT_RemotePropHelpTitle, HELPPATH, HYP_RemotePropHelpSect };

extern HelpInfo LocalAddWindowHelp =
{ FormalClientName, TXT_LocalAddHelpTitle, HELPPATH, HYP_LocalAddHelpSect };

extern HelpInfo LocalPropWindowHelp =
{ FormalClientName, TXT_LocalPropHelpTitle, HELPPATH, HYP_LocalPropHelpSect };

extern HelpInfo StatusWindowHelp =
{ FormalClientName, TXT_StatusHelpTitle, HELPPATH, HYP_StatusHelpSect };

extern HelpInfo HostWindowHelp =
{ FormalClientName, TXT_HostHelpTitle, HELPPATH, HYP_HostHelpSect };

extern HelpInfo FindWindowHelp =
{ FormalClientName, TXT_FindHelpTitle, HELPPATH, HYP_FindHelpSect };

extern HelpInfo FindFolderWindowHelp =
{ FormalClientName, TXT_FindFolderHelpTitle, HELPPATH,
      HYP_FindFolderHelpSect };

extern HelpInfo DeleteLocalNoticeHelp =
{ FormalClientName, TXT_DeleteLocalHelpTitle, HELPPATH,
      HYP_DeleteLocalHelpSect }; 

extern HelpInfo DeleteRemoteNoticeHelp =
{ FormalClientName, TXT_DeleteRemoteHelpTitle, HELPPATH,
      HYP_DeleteRemoteHelpSect }; 

extern HelpInfo StopNFSNoticeHelp =
{ FormalClientName, TXT_StopNFSHelpTitle, HELPPATH, HYP_StopNFSHelpSect };

extern HelpInfo CreateMountPointNoticeHelp =
{ FormalClientName, TXT_CreateMountPointHelpTitle, HELPPATH,
      HYP_CreateMountPointHelpSect }; 

extern HelpInfo CreateFolderNoticeHelp =
{ FormalClientName, TXT_CreateFolderHelpTitle, HELPPATH,
      HYP_CreateFolderHelpSect }; 

extern HelpInfo ApplicationHelp =
{ FormalClientName, TXT_MainHelpTitle, HELPPATH, HYP_MainHelpSect };

extern HelpInfo TOCHelp =
{ FormalClientName, TXT_TOCHelpTitle, HELPPATH, HYP_TOCHelpSect };

extern HelpInfo HelpDeskHelp =
{ FormalClientName, TXT_HelpDeskHelpTitle, HELPPATH, HYP_HelpDeskHelpSect };

extern HelpInfo MountPointNotEmptyHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH, HYP_MountPointNotEmptyHelpSect };

/* Context Sensitive Help */

extern HelpInfo HostListHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_HostListHelpSect };

extern HelpInfo HostPopupMenuHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_HostPopupMenuHelpSect };

extern HelpInfo HostPopupHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_HostPopupHelpSect };

extern HelpInfo LocalFolderLabelHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalFolderLabelHelpSect };

extern HelpInfo LocalLocalPathHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalLocalPathHelpSect };

extern HelpInfo LocalFindHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalFindHelpSect };

extern HelpInfo LocalFindMenuHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalFindMenuHelpSect };

extern HelpInfo LocalFindPopupHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalFindPopupHelpSect };

extern HelpInfo BootAdvHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_BootAdvHelpSect };

extern HelpInfo DefaultAccessHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_DefaultAccessHelpSect };

extern HelpInfo ExceptionTextHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_ExceptionTextHelpSect };

extern HelpInfo ExceptionListHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_ExceptionListHelpSect };

extern HelpInfo ExceptionHostHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_ExceptionHostHelpSect };

extern HelpInfo LocalGetHostHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalGetHostHelpSect };

extern HelpInfo ExceptionMenuHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_ExceptionMenuHelpSect };

extern HelpInfo LocalExtendedHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalExtendedHelpSect };

extern HelpInfo LocalCustomHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_LocalCustomHelpSect };

extern HelpInfo AdvMenuHelp =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_AdvMenuHelpSect };

/*****
extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

extern HelpInfo  =
{ FormalClientName, TXT_HelpTitle, HELPPATH,
      HYP_Sect };

***/
