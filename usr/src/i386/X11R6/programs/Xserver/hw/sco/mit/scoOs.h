/*
 *	@(#) scoOs.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 *	SCO MODIFICATION HISTORY
 *
 *	S003	Fri Jan 26 13:49:12 PST 1996	hiramc@sco.COM
 *	- begin modifications to allow compile on Unixware, note
 *	- all of this will be marked by defined(gemini)
 *	S000	Fri May 03 04:19:18 PDT 1991	mikep@sco.com
 *	- Created file.
 *	S001	Thu May 09 12:21:12 PDT 1991	buckm@sco.com
 *	- There are now two separate screen switch flags.
 *	S002	Fri May 10 12:25:17 PDT 1991	buckm@sco.com
 *	- Add ReadyForUnblank flag.
 */

extern int scoStatusRecord;
extern int Xrenice;

#define 	SSFD		-1

#define		KDSLED	 	KDSETLED
#if !defined(gemini)
#define		CAPS_LOCK	1
#define		NUM_LOCK	2
#define		SCROLL_LOCK	4
#endif

/* defines for global flags */
#define		TtyModeChanged		0x001
#define		DisplayModeChanged	0x002
#define		EventWasOpened		0x004
#define		WaitingSpkTimeout	0x008
#define		AltSysRequest		0x010
#define		TtyScancodeMode		0x020
#define		CatchScreenAcquireSignal 0x040			/* S001 */
#define		CatchScreenReleaseSignal 0x080			/* S001 */
#define		AltKeyBeingPressed	0x100
#define		ShiftKeyBeingPressed	0x200
#define		ReadyForUnblank		0x400			/* S002 */

#define		AllScreens	(ScreenPtr)NULL	

