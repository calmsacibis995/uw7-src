/*
 *	@(#) scoPopUp.c 12.1 95/05/09 SCOINC
 *
 *	Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 *	The information in this file is provided for the exclusive use of the
 *	licensees of The Santa Cruz Operation, Inc.  Such users have the right
 *	to use, modify, and incorporate this code into other products for
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
/*
 *	S000, 04-APR-91, mikep@sco.com
 *		Created file
 *	S001, 08-APR-91, mikep@sco.com
 *		Quick bug fix to get the fg and bg right.
 *	S002, 20-APR-91, mikep@sco.com
 *		Added code to clear the cursor
 *	S003, 09-AUG-91, buckm@sco.com
 *		Really want to be using whitePixel and blackPixel here.
 *	S004, 25-AUG-91, mikep@sco.com
 *		Forget the blank cursor stuff, just turn it off.
 *	S005, 12-SEP-91, mikep@sco.com
 *		Use miPointerOn() to do S004.
 *	S006, 13-MAR-91, mikep@sco.com
 *		Rewrite the font code to deal with R5
 *	S007, 08-JAN-93, chrissc@sco.com
 *		added save under to the pop up window.
 *      S008    Mon Jul 12 10:45:35 PDT 1993    davidw@sco.com
 *      -       Added multi-lined messages support by embedding "\n"'s.
 *	S009, 10-AUG-93, buckm@sco.com
 *		Change mipointer sprite on/off interface.
 *	S010, 26-SEP-94, mikep@sco.com
 *		Fix a bug in strlen() calculation.  Found by insight.
 *	
 */
#include <stdio.h>
#include "sco.h"

#include "dixfontstr.h"
#include "fontstruct.h"

#define BORDER_WIDTH 2
#define NEWLINE "\n"

extern GCPtr CreateScratchGC();

extern int scoStatusRecord;

static XID popUpWid[MAXSCREENS];        /* Resource ID's of PopUp windows */
static WindowPtr pPopUpWin[MAXSCREENS]; /* Pointer to PopUp windows */
static XID scratchGCid[MAXSCREENS];	/* Resource ID's of GC's */
static GCPtr pScratchGC[MAXSCREENS];	/* scratch GC's to draw with */

extern WindowPtr * WindowTable;

/*
 *  scoPopMessage - display a message in a window
 *  
 *  Parameters:
 *    message	pointer to message text
 *
 *    
 *   This function uses nothing but dix to create a message in a window
 *   and present it to the user.  The window size is calculated from the
 *   message length and the default font.  The one assumption being made
 *   is that defaultFont is constant width.  For multi-lined messages, 
 *   separate message strings with NEWLINE's.
 */

int
scoPopMessage(char *message)
{
    int		i, j, result;
    XID		GCid, attributes[4];
    short 	h, w, x, y, tx, ty;
    xCharInfo	*pCI;
    FontInfoRec	*fI;
    Mask 	mask = 0;
    char 	*strPtr, *local_message = NULL;
    short 	mesgLongest = 0;  /* window as wide as the longest substring */
    short	mesgCnt = 0;

    typedef struct _mesgSubStr
    {
        char *mesgSubPtr;
        short mesgSubLen;
        struct _mesgSubStr *next;
    } mesgSubStr;

    mesgSubStr *mesgList = (mesgSubStr *) NULL;
    mesgSubStr *mesgListCur = (mesgSubStr *) NULL;
    mesgSubStr *new_mesg;

    register WindowPtr pParent;
    register ScreenPtr pScreen;

    static lastGeneration = -1;

    if (message == NULL) /* Don't popup a window if nothin to display */
	return;

    /* need a local copy cause strtok() sticks \0's into the message */
    local_message = (char *) ALLOCATE_LOCAL (strlen(message) + 1);/* S010 */
    strcpy(local_message, message);

    /* 
     * Extract sub messages separated by NEWLINE tokens from message.
     * Pass in 'message' to strtok the first time. Pass in NULL to 
     * strtok each time after to get additional sub messages, if any.
     */
    strPtr = strtok(local_message, NEWLINE);
    while (strPtr != NULL)
    {
        new_mesg = (mesgSubStr *) ALLOCATE_LOCAL (sizeof( mesgSubStr ));
	new_mesg->mesgSubPtr = strPtr;
	new_mesg->mesgSubLen = strlen(new_mesg->mesgSubPtr);
	new_mesg->next = mesgListCur;
	mesgListCur = new_mesg;
	mesgCnt++;
	/* track the longest string while building the list */
	if (new_mesg->mesgSubLen > mesgLongest) 
	    mesgLongest = new_mesg->mesgSubLen;
	strPtr = strtok(NULL, NEWLINE);
    }
    mesgList = mesgListCur;	/* Keep a master pointer for re-use */

    /*
     * Put a message on every screen
     */
    for (i = 0; i < screenInfo.numScreens; i++)
	{
	mesgListCur = mesgList;		/* Reset for multiple passes */
	pScreen = screenInfo.screens[i];
	pParent = WindowTable[i];	/* The first entries will always */
					/*  be the root windows */

	/*
	 *  Only create the GC's and windows once.  This really
	 *  shouldn't be this big of a deal, but they do need
	 *  to be recreated once the server is reset.
	 */
	if (pPopUpWin[i] == NULL || lastGeneration != serverGeneration) 
	    {
	    lastGeneration = serverGeneration;
	    
	    /* Can't use GetScratchGC since we want to invert the fg and bg */
	    scratchGCid[i] = FakeClientID(0 /* SERVER_ID */);
	    pScratchGC[i]=CreateScratchGC(pScreen,pScreen->rootDepth);

	    if(pScratchGC[i] == NULL)
		{
		ErrorF("Can't get a GC\n");
		return;
		}

	    /* S003 - draw in black and white */
	    attributes[0] = pScreen->whitePixel;
	    attributes[1] = pScreen->blackPixel;

	    (void)ChangeGC(pScratchGC[i],GCForeground|GCBackground,attributes);

	    /*
	     * From the GC font info, figure out the width and height of the
	     * window.
	     */
	    fI = &pScratchGC[i]->font->info;

	    /* These calculation include the extra spacing needed */
	    ty = FONT_MAX_HEIGHT(fI);
	    tx = FONT_MAX_WIDTH(fI);
	    w = tx * (mesgLongest + 2); /* A char spacing on each side */
	    h = ty * mesgCnt + 4; /* 2 pixels extra for upper and lower */

	    /* Center the window on the screen */
	    x = ((unsigned short)pScreen->width / 2) - (w / 2);
	    y = ((unsigned short)pScreen->height / 2) - (h / 2);

	    /*
	     * Create our popup window.
	     * 	Be sure the background is the same as the GC's
	     */
	    attributes[0] = pScratchGC[i]->bgPixel;
	    attributes[1] = pScratchGC[i]->fgPixel;
	    attributes[2] = TRUE;
	    attributes[3] = TRUE;					/* S007 */

	    mask = CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWSaveUnder;  /* S007 */


	    popUpWid[i] = FakeClientID(0 /* SERVER_ID */);
	    pPopUpWin[i] =
		 CreateWindow(popUpWid[i],
		 pParent,
		 x, y, w, h,
		 BORDER_WIDTH, InputOutput, mask,
		 attributes, pScreen->rootDepth, serverClient, 
		 CopyFromParent, &result);

	    if(pPopUpWin[i] == NULL)
		{
		ErrorF("Can't get a Window\n");
		return;
		}

	    /* This will cause them to be freed at server reset */
	    AddResource(popUpWid[i], RT_WINDOW, pPopUpWin[i]);
	    AddResource(scratchGCid[i], RT_GC, pScratchGC[i]);
	    }
	else
	    {
	    /* Always recalculate these since we must redraw the text */
	    fI = &pScratchGC[i]->font->info;
	    ty = FONT_MAX_HEIGHT(fI);
	    tx = FONT_MAX_WIDTH(fI);
	    }

	    /* Resize the window if the message has changed.  Overkill? */
	    w = tx * (mesgLongest + 2);
	    if (pPopUpWin[i]->winSize.extents.x2 - 
				pPopUpWin[i]->winSize.extents.x1 != w)
	    {
	    h = ty * mesgCnt + 4;
	    x = ((unsigned short)pScreen->width / 2) - (w / 2);
	    y = ((unsigned short)pScreen->height / 2) - (h / 2);

	    attributes[0] = x;
	    attributes[1] = y;
	    attributes[2] = w;
	    attributes[3] = h;

	    ConfigureWindow(pPopUpWin[i], CWX | CWY | CWWidth | CWHeight, 
	    attributes, serverClient);
	    }

	/* Raise the window */
	attributes[0] = Above;
	ConfigureWindow(pPopUpWin[i], CWStackMode, attributes, serverClient);

	MapWindow(pPopUpWin[i], serverClient);

	ValidateGC(pPopUpWin[i], pScratchGC[i]);

	/*
	 * You can only draw on Mapped Windows
	 */
	
	/* Draw each of the message strings to the window - bottom up */
	for(j = mesgCnt; j > 0; j--, mesgListCur = mesgListCur->next)
		(*pScratchGC[i]->ops->ImageText8)(pPopUpWin[i], 
			pScratchGC[i], tx, ty * j, 
			mesgListCur->mesgSubLen, mesgListCur->mesgSubPtr);
	}

    mesgListCur = mesgList;		/* Reset it */
    /* Get rid of the list (as if this were free()) */
    for(j = 0; j < mesgCnt; j++, mesgListCur = mesgListCur->next)
    	DEALLOCATE_LOCAL(mesgListCur);
    DEALLOCATE_LOCAL(local_message);
    /*
     * Turn off the cursor					S004
     */
    miPointerSpriteEnable(NULL, FALSE);				/* S009 */
}

int
scoHideMessage()
{
    register int i;
    register ScreenPtr pScreen;

    for (i = 0; i < screenInfo.numScreens; i++)
	{
	/* If this gets called by accident */
	if (pPopUpWin[i] == NULL)
		continue;
	/*
	 * Unmap the popup window
	 */
	UnmapWindow(pPopUpWin[i], FALSE);
	}

    /*
     * Turn on the cursor					S004
     */
    miPointerSpriteEnable(NULL, TRUE);				/* S009 */
}

