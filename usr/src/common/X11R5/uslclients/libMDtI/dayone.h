#ifndef __dayone_h_
#define __dayone_h_

#pragma ident	"@(#)libMDtI:dayone.h	1.10.1.5"

/*
 **************************************************************************
 *
 * Description:
 *              This is the header file for the dayone utility
 *              routines.
 *
 **************************************************************************
 */

#define DAYONE_FILENAME		"dayone:"	/* message catelog file name */

	/* You'll have to update dayone.c:dayone_strings[],
	 * whenever you make a change to any of the following.
	 */
							/* MSGID */
#define	TXT_HARDWARE_SETUP	"Hardware_Setup"	/*  1 */
#define	TXT_ADMIN_TOOLS 	"Admin_Tools"		/*  2 */
#define	TXT_APP_INSTALLER 	"App_Installer"		/*  3 */
#define	TXT_APPLICATIONS 	"Applications"		/*  4 */
#define	TXT_BACKUP_RESTORE 	"Backup-Restore"	/*  5 */
#define	TXT_CALCULATOR 		"Calculator"		/*  6 */
#define	TXT_CARTRIDGE_TAPE	"Cartridge_Tape"	/*  7 */
#define	TXT_CLOCK		"Clock"			/*  8 */
#define	TXT_COLOR		"Color"			/*  9 */
#define	TXT_DEBUG		"Debug"			/* 10 */
#define	TXT_DESKTOP		"Desktop"		/* 11 */
#define	TXT_DIALUP_SETUP	"Dialup_Setup"		/* 12 */
#define	TXT_DISK_A		"Disk_A"		/* 13 */
#define	TXT_DISK_B		"Disk_B"		/* 14 */
#define	TXT_DISKS_ETC		"Disks-etc"		/* 15 */
#define	TXT_EXTRA_ADMIN		"Extra_Admin"		/* 16 */
#define	TXT_FILE_SHARING	"File_Sharing"		/* 17 */
#define	TXT_FOLDER_MAP		"Folder_Map"		/* 18 */
#define	TXT_FONTS		"Fonts"			/* 19 */
#define	TXT_GAMES		"Games"			/* 20 */
#define	TXT_HELP_DESK		"Help_Desk"		/* 21 */
#define	TXT_ICON_EDITOR		"Icon_Editor"		/* 22 */
#define	TXT_ICON_SETUP		"Icon_Setup"		/* 23 */
#define	TXT_INTERNET_SETUP	"Internet_Setup"	/* 24 */
#define	TXT_LOCALE		"Locale"		/* 25 */
#define	TXT_MAIL		"Mail"			/* 26 */
#define	TXT_MAIL_SETUP		"Mail_Setup"		/* 27 */
#define	TXT_MAILBOX		"Mailbox"		/* 28 */
#define	TXT_MOUSE		"Mouse"			/* 29 */
#define	TXT_MSG_MONITOR		"Msg_Monitor"		/* 30 */
#define	TXT_NETWARE		"NetWare"		/* 31 */
#define	TXT_NETWORKING		"Networking"		/* 32 */
#define	TXT_PASSWORD		"Password"		/* 33 */
#define	TXT_PREFERENCES		"Preferences"		/* 34 */
#define	TXT_PRINTER_SETUP	"Printer_Setup"		/* 35 */
#define	TXT_PUZZLE		"Puzzle"		/* 36 */
#define	TXT_REMOTE_ACCESS	"Remote_Access"		/* 37 */
#define	TXT_SCREENLOCK		"ScreenLock"		/* 38 */
#define	TXT_SHUTDOWN		"Shutdown"		/* 39 */
#define	TXT_STARTUP_ITEMS	"Startup_Items"		/* 40 */
#define	TXT_SYSTEM_MONITOR	"System_Monitor"	/* 41 */
#define	TXT_SYSTEM_STATUS	"System_Status"		/* 42 */
#define	TXT_SYSTEM_TUNER	"System_Tuner"		/* 43 */
#define	TXT_TASK_SCHEDULER	"Task_Scheduler"	/* 44 */
#define	TXT_TERMINAL		"Terminal"		/* 45 */
#define	TXT_TEXT_EDITOR		"Text_Editor"		/* 46 */
#define	TXT_UUCP_INBOX		"UUCP_Inbox"		/* 47 */
#define	TXT_USER_SETUP		"User_Setup"		/* 48 */
#define	TXT_WALLPAPER		"Wallpaper"		/* 49 */
#define	TXT_WALLPAPER_INSTALLER	"Wallpaper_Installer"	/* 50 */
#define	TXT_WASTEBASKET		"Wastebasket"		/* 51 */
#define	TXT_XTETRIS		"Xtetris"		/* 52 */
#define	TXT_CDROM		"CD-ROM"		/* 53 */
#define	TXT_DOS			"DOS"			/* 54 */
#define	TXT_ONLINE_DOCS		"Online_Docs"		/* 55 */
#define	TXT_REMOTE_APPS		"Remote_Apps"		/* 56 */
#define	TXT_WIN_SETUP		"Win_Setup"		/* 57 */
#define	TXT_APP_SHARING		"App_Sharing"		/* 58 */
#define	TXT_INSTALL_SERVER	"Install_Server"	/* 59 */
#define	TXT_MHS_SETUP		"MHS_Setup"		/* 60 */
#define	TXT_PROCESSOR_SETUP	"Processor_Setup"	/* 61 */
#define	TXT_DISPLAY_SETUP	"Display_Setup"		/* 62 */
#define	TXT_NETWARE_ACCESS	"NetWare_Access"	/* 63 */
#define	TXT_NETWARE_SETUP	"NetWare_Setup"		/* 64 */
#define	TXT_NETWARE_STATUS	"NetWare_Status"	/* 65 */
#define	TXT_CDROM1		"cdrom1"		/* 66 */
#define	TXT_WIN			"Win"			/* 67 */
#define	TXT_WINDOW		"Window"		/* 68 */
#define	TXT_INET_BROWSER	"Inet_Browser"		/* 69 */
#define	TXT_INSTALL_BROWSER	"Install_Browser"	/* 70 */
#define	TXT_NONE		"None"			/* 71 */
#define	TXT_REMOTE_LOGIN	"Remote_Login"		/* 72 */
#define	TXT_GET_INET_BROWSER	"Get_Inet_Browser"	/* 73 */
#define	TXT_TAPE_2 		"Tape_2"		/* 74 */
#define	TXT_TAPE_3 		"Tape_3"		/* 75 */
#define	TXT_TAPE_4 		"Tape_4"		/* 76 */
#define	TXT_TAPE_5 		"Tape_5"		/* 77 */
#define	TXT_CD_1 		"CD-ROM_1"		/* 78 */
#define	TXT_CD_2 		"CD-ROM_2"		/* 79 */
#define	TXT_WELCOME 		"Welcome"		/* 80 */
#define	TXT_MERGE_SETUP		"Merge_Setup"		/* 81 */
#define	TXT_DS_INSTALL		"DS_Install"		/* 82 */
#define	TXT_DS_REPAIR		"DS_Repair"		/* 83 */
#define	TXT_NETWARE_SETTINGS	"NetWare_Settings"	/* 84 */
#define	TXT_NWS_LICENSING	"NWS_Licensing"		/* 85 */
#define	TXT_NWS_STATUS		"NWS_Status"		/* 86 */
#define	TXT_NETWARE_CLIENT_DISKS	"NetWare_Client_Disks"	/* 87 */
#define	TXT_NWS_VOLUME_SETUP	"NWS_Volume_Setup"	/* 88 */

	/* Defines for devtab.c and their users. Should have a prefix... */
#define	ALIAS			"alias"
#define	DTALIAS			"dtalias"
#define	DESC			"desc"
#define	DTDESC			"dtdesc"
#define	CDEVICE			"cdevice"
#define	BDEVICE			"bdevice"
#define	CTAPE1			"ctape1"
#define	CTAPE2			"ctape2"
#define	DISKETTE		"diskette"
#define	CDROM			"cdrom"
#define	CTAPE3			"ctape3"
#define	CTAPE4			"ctape4"
#define	CTAPE5			"ctape5"

#endif /* __dayone_h_ */
