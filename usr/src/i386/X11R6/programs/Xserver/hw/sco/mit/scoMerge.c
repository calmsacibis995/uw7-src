/*
 *	@(#) scoMerge.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Apr 02 23:09:49 PST 1991	mikep@sco.com
 *	- Created file from mwSwitch.c.   Separated extension from server.
 *	S001	Sun May 12 01:29:58 PDT 1991	mikep@sco.com
 *	- Pulled in merge.c from Locus source.
 *	- Removed global variables
 *	S002, 12-MAY-91, hiramc@sco.COM
 *	- Don't need Xos.h and its inclusion of time.h caused conflicts
 *	- with the time.h that comes in with sco.h
 *	- This is all because /usr/include/sys/time.h by Lachman is not
 *	- protected against multiple inclusion in the standard DevSys
 *	- installation.
 *	S003, Thu May 30 13:52:45 PDT 1991,	stacey@sco.com
 *	- Added code to inform scoScreenSwitchFilter when the screen
 *	  switch mod keys have changed.
 *	- Added a new arg to GetMergeScreenSwitchSeq(str, fd), give it
 *	  a fd that will be valid instead of hunting around for the
 *	  keyboard fd that may not be initialized when used.  All usage
 *	  of this routine seems to be in scoScrSw.c (according to gid).
 *	S004	Thu Sep 26 09:48:11 1991	staceyc@sco.com
 *	- Moved server screen switching code to scoScrSw.c
 *	S005	Tue Oct 01 00:51:41 PDT 1991	mikep@sco.com
 *	- Use consoleFd for ioctl since the keyboard may not be available
 *	yet.
 */

#include "X.h"
#include "input.h"
#include "windowstr.h"
#include <sys/keyboard.h>
#include "pckeys.h"
#include "sco.h"

extern void SetServerSWSequence();

/*
 * Set screen switch sequence. Called by Xsight server extension.
 */
SetSWSequence(string)
char *string;
{
    SetServerSWSequence(string);
    SetMergeScreenSwitchSeq(string);
}

/*
 *  FIXME!!!!!   Obviously this isn't always so.
 */
gDisplaySupportsXsightExtension()
{
	return 1;
}

/* ---------- ioctl cmd defines copied from merge.h ------------------ */
#define GIOC		('M'<<8)	/* Merge IOCTL type		*/

#define MIOC_KDVDCTYPE	(GIOC|24)	/* return display and adaptor type */
#define MIOC_SETSWTCH	(GIOC|25)	/* set state for switch screen seq.*/
#define MIOC_GETSWTCH	(GIOC|26)	/* read kbstate for switch screen seq */
#define MIOC_SETZOOM	(GIOC|30)	/* set the Xsight unzoom sequence */

/*
 * Fill merge screen switch sequence into buffer str.
 * Buffer str should have minimum size of 4 bytes.
 * return 0 if merge not installed.
 */
GetMergeScreenSwitchSeq(str, fd)			/* S003 */
char *str;
int fd;
{
    int swkey_state;

    if(ioctl (fd, MIOC_GETSWTCH, &swkey_state) >= 0) {
	if(swkey_state & ALT)
		*str++ = 'a';
	if(swkey_state & CTRL)
		*str++ = 'c';
	if(swkey_state & SHIFT)
		*str++ = 's';

    	*str = 0;
	return 1;
    } 
    return 0;
}


/* 
 * Set merge's sw sequence. 
 * Return the return value of Merge ioctl.
 */
int
SetMergeScreenSwitchSeq(str)
char *str;
{
    int 	swkey_state = 0;

    for(  ; str != 0 && *str != 0; str++ ) {
	if((*str == 'a') || (*str == 'A'))
		swkey_state |= ALT;
	if((*str == 's') || (*str == 'S'))
		swkey_state |= SHIFT;
	if((*str == 'c') || (*str == 'C'))
		swkey_state |= CTRL;
    }

    /* S005
     * consoleFd is actually stdout.  Is this what Merge wants?
     * Either do this or move scoInitScrSw() call into scoKbdProc()
     * to insure there's a keyboard fd.
     */
    return (ioctl (scoSysInfo.consoleFd, MIOC_SETSWTCH, swkey_state));
}

