#ident	"@(#)xdm:nondesktop.h	1.6"

#ifndef	_NONDESKTOP_H_
#define	_NONDESKTOP_H_

/*
 * This file contains all the message strings for the Desktop UNIX
 * Graphical Login client nondesktop, used by xdm.
 * 
 */

#define MSG_FILENAME		"nondesktop:"
#define FS			"\000"

	/* Note that YOU CAN'T CHANGE/MOVE/DELETE ANY EXISTING MESSAGES,
	 * YOU CAN ONLY DEFINE NEW MSG NUMBER EVEN YOU ARE ONLY CHANGING
	 * THEM, OTHERWISE, YOU WILL NEED TO DELIVER A NEW FILE WITH
	 * A DIFFERENT NAME, sigh... */
#define	MSG_HELP_NON_DTUSER	"1" FS "Press the Exit button\
 to exit from Graphics\n\nPress the Cancel button to return to\
 Graphical Login."

#define	MSG_HELP_DTUSER		"2" FS "Press the Desktop button\
 to begin a Desktop session.\n\nPress the Exit button to exit from\
 Graphics.\n\nPress the Cancel button to return to Graphical Login."

#define	MSG_EXIT_BTN		"3" FS "Exit"
#define	MSG_CANCEL_BTN		"4" FS "Cancel"
#define	MSG_DTUSER_BTN		"5" FS "Start Desktop"
#define	MSG_OBSOLETED_1		"6" FS "Display Terminal"
#define	MSG_HELP_BTN		"7" FS "Help"

#define	MSG_EXIT_MNE		"8" FS "E"
#define	MSG_CANCEL_MNE		"9" FS "C"
#define	MSG_DTUSER_MNE		"10" FS "S"
#define	MSG_OBSOLETED_2		"11" FS "D"
#define MSG_HELP_MNE		"12" FS "H"
#define	MSG_OK_MNE		"13" FS "O"

#define	MSG_NON_DTUSER		"14" FS "You are not a Desktop\
 user.  Do you want to..."

#define	MSG_OBSOLETED_3		"15" FS "OLD MSG_DTUSER"

#define	MSG_OK_BTN		"16" FS "OK"

#define	MSG_DTUSER		"17" FS "Your Desktop Preference\
 indicates that your Desktop should not be started.\nYou may:\n\n\
       1) Start your Desktop anyway or\n\
       2) Click 'Exit' and enter the system through the Console Login."

#endif	/* Don't add anything after this endif */
