/*
 *	@(#)ddxOutput.c	11.3	12/5/97	14:00:09
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Wed Jun 12 19:34:13 PDT 1991	mikep@sco.com
 *	- Added SGI ifdefs and removed SGI Hardware specific include
 *	files.  Added  boardNames structure.  This should eventually
 *	be replaced by grafinfo parsing.
 *	S001	Sun Aug 25 23:14:41 PDT 1991	mikep@sco.com
 *	- Added grafinfo parsing.  Remove SGI ifdef's.  Rewrote most of
 *	the file.
 *	S002	Tue Sep 10 14:55:11 PDT 1991	mikep@sco.com
 *	- Wrote ddxAddPixmapFormat().  This should be called by every
 *	ddx layer.
 *	- Added support for bit-per-pixel and scanline padding in
 *	ddxFindBoardInfo().
 *	S003	Thu Sep 12 		1991	pavelr
 *	- parse the monitor file
 *	S004	Thu Oct 03 		1991	pavelr
 *	- use scoVidModeStr to get -d arg
 *	S005	Thu Aug 06 13:33:03 PDT 1992	mikep@sco.com
 *	- Change G_MAXMODES to MAXSCREENS
 *	S006	Thu Oct 15 12:11:08 PDT 1992	mikep@sco.com
 *	- Change strdup to Xstrdup.
 *      S007    Wed Jul 06 18:51:49 PDT 1994    davidw@sco.com
 *      - Change mkdev graphics cmnd to something generic & add to catalog.
 *	S008	Fri Feb 28 11:23:37 PST 1997	kylec@sco.com
 *	- Exec grafinit to ensure video configuration is sane.
 *	S009	Sat Nov 15 10:04:54 PST 1997	hiramc@sco.COM
 *	- restore real and effective uid after system(GRAFINIT)
 *	S010	Fri Dec  5 13:32:12 PST 1997	hiramc@sco.COM
 *	- when -d (video mode) is used, we must (for some unknown)
 *	- reason also call grafinit even though it is not necessary.
 *
 */
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include "X.h"
#include "misc.h"
#include "scrnintstr.h"
#include "ddxScreen.h"

#include "grafinfo.h"
#include "xsrv_msgcat.h"				/* S007 */

static	int	nBoards;

static char * modeList[MAXSCREENS];			/* S005 */

static PixmapFormatRec ddxPixmapFormats[MAXFORMATS];
static int ddxNumPixmapFormats = 0;


#define MATCH 1						/* S002 start */
#define NOMATCH 0
#define	BADMATCH -1

#ifndef USRLIBDIR
#define USRLIBDIR "/usr/X11R6.1/lib"
#endif

#define GRAFINIT USRLIBDIR "/vidconf/AOF/bin/grafinit"

/*
 * ddxMatchFormat(int depth, int bpp, int pad)
 *
 * Look through the ddxPixmapFormats for a format which exactly matches
 * the one passed in.  Pximap formats of the same depth MUST have the same
 * bits per pixel and scan line padding.
 *
 */

static int
ddxMatchFormat(int depth, int bpp, int pad)
{
    int i;
    PixmapFormatRec *format;

    for ( i = 0; i < ddxNumPixmapFormats; i++ )
	{
	format = &ddxPixmapFormats[i];
	if ( format->depth == depth )
	    {
	    if (format->bitsPerPixel == bpp && format->scanlinePad == pad)
		return MATCH;
	    else
		{
		ErrorF("Pixmap format (%d,%d,%d) is incompatible with (%d,%d,%d).\n",
		    depth, bpp, pad, 
		    format->depth, format->bitsPerPixel, format->scanlinePad);
		return BADMATCH;
		}
	    }
	}
	    
    return NOMATCH;
} 

/*
 * Add a new pixmap format to the screenInfo structure.   This also checks
 * for an existing format or a non-matching.
 *
 * returns true if the format is successfully added.
 */
Bool
ddxAddPixmapFormat(int depth, int bpp, int pad)
{
    if (ddxNumPixmapFormats == MAXFORMATS)
	{
	ErrorF("Too many pixmap formats\n");
	return FALSE;
	}

    switch (ddxMatchFormat(depth, bpp, pad))
	{
	case MATCH:
	    return TRUE;
	case BADMATCH:
	    return FALSE;
	case NOMATCH:
	    ddxPixmapFormats[ddxNumPixmapFormats].depth = depth;
	    ddxPixmapFormats[ddxNumPixmapFormats].bitsPerPixel = bpp;
	    ddxPixmapFormats[ddxNumPixmapFormats].scanlinePad = pad;
	    ddxNumPixmapFormats++;
	    return TRUE;
	}

    return FALSE;
}

/*
 * ddxInitPixmapFormats(ScreenInfo *screenInfo)
 *
 * Copy the static Pixmap Formats into the screenInfo structure.  This
 * needs to be done after every reset since ddxAddPixmapFormat() is
 * only called in the Probe routines.
 */
void
ddxInitPixmapFormats(ScreenInfo *screenInfo)
{
    int i;
    screenInfo->numPixmapFormats = ddxNumPixmapFormats;

    for(i =0; i < ddxNumPixmapFormats; i++)
	screenInfo->formats[i] = ddxPixmapFormats[i];
    
    return;
}
							/* S002 end */

static Bool
ddxFindAvailableBoards()
{
static  Bool been_here= 0;
register char *modelist;
int modes;
extern char *scoVidModeStr;

    if (!been_here) {
	uid_t	InitialRuid;		/*	S009	*/
	uid_t	InitialEuid;		/*	S009	*/

	/*  S004
	 *  check for the scoVideModeStr or else
	 *  Read the grafdev file and find out
	 *  how many boards are installed on the system.
	 */
	if ((modelist = scoVidModeStr) == NULL)		/* S004 */
          {
		InitialRuid = getuid();		/*	S009	*/
		InitialEuid = geteuid();	/*	S009	*/
            setuid(0);
            system(GRAFINIT);   /* S008 */
	    modelist=grafGetTtyMode(GetTTYName());
		setreuid( InitialRuid, InitialEuid );	/*	S009	*/
          } else {				/*	S010	vvv	*/
		InitialRuid = getuid();
		InitialEuid = geteuid();
            setuid(0);
            system(GRAFINIT);
		setreuid( InitialRuid, InitialEuid );
	}					/*	S010	^^^	*/

	if(modelist == NULL) {
	   ErrorF(MSGSTR_SET(XSRV_DYDDX,XDYDDX_1,		/* S007 */
		"No adapters are configured for %s.  Run the video\nconfiguration manager to configure an adapter for this screen.\n"), GetTTYName());
	    return FALSE;
	}

	/*
	 * Walk through the comma separated list of modelist, pulling out 
	 * each one and sticking it in the modeList
	 */
	for(modes=0; modelist && modes < MAXSCREENS; modes++) 
	    {
	    modeList[modes]=modelist;
	    /*
	     * replace the comma with a NULL.  If there's no comma, we're
	     * done, else start on the next one.
	     */
	    if( modelist=(char *)strchr(modelist,',') )
	       *modelist++='\0';
	    }
	    
	if (modes>MAXSCREENS) {
	    ErrorF("Warning! More modes defined (%d) than allowed (%d)\n",
							modes,MAXSCREENS);
	    modes= MAXSCREENS;
	}

	nBoards= modes;
	been_here= 1;
    }
    return(TRUE);
}

/***====================================================================***/

Bool
ddxGetModeName(int index,char *buf)
{
    if (index<nBoards) {
	strcpy(buf,modeList[index]);
	return(TRUE);
    }
    return(FALSE);
}

/***====================================================================***/

int
ddxFindFirstMode(char *mode)
{
register int i;
char	buf[G_MODELENGTH];

    for (i=0;i<nBoards;i++) {
	if (ddxGetModeName(i,buf)&&(!strcmp(buf,mode))) {
	    return(i);
	}
    }
    return(DDX_BOARD_UNSPEC);
}

/***====================================================================***/

int
ddxFindFirstAvailableBoard()
{
int	i;

    if (ddxNRequestedScreens<nBoards)
	 return(ddxNRequestedScreens);
    for(i=0;i<ddxNRequestedScreens;i++) {
	if (ddxRequestedScreens[i].boardNum==DDX_BOARD_UNSPEC)
	    return(i);
    }
    return(DDX_BOARD_UNSPEC);
}
/***====================================================================***/

Bool
ddxRequestAllBoards()
{
int	i;
char	buf[G_MODELENGTH];

    if ((nBoards>0)||ddxFindAvailableBoards()) {
	if (nBoards>MAXSCREENS) {
	    ErrorF("System has more boards (%d) than the server can use (%d)\n",
							nBoards,MAXSCREENS);
	    ErrorF("Using the first %d available boards only\n",MAXSCREENS);
	    nBoards= MAXSCREENS;
	}
	for (i=0;i<nBoards;i++) {
	    if (ddxGetModeName(i,buf)) {
		ddxRequestedScreens[i].mode=(char *)Xstrdup(buf);
	    }
	    ddxRequestedScreens[i].type=	NULL;
	    ddxRequestedScreens[i].boardNum=	i;
	    ddxRequestedScreens[i].ddxLoad=	DDX_LOAD_UNSPEC;
	    ddxRequestedScreens[i].dfltClass=	DDX_CLASS_UNSPEC;
	    ddxRequestedScreens[i].dfltDepth=	DDX_DEPTH_UNSPEC;
	    ddxRequestedScreens[i].dfltBpp=	DDX_DEPTH_UNSPEC; /* S003 */
	    ddxRequestedScreens[i].dfltPad=	DDX_DEPTH_UNSPEC; /* S003 */
	    ddxRequestedScreens[i].nextLeft=	DDX_BOARD_UNSPEC;
	    ddxRequestedScreens[i].nextRight=	DDX_BOARD_UNSPEC;
	    ddxRequestedScreens[i].nextAbove=	DDX_BOARD_UNSPEC;
	    ddxRequestedScreens[i].nextBelow=	DDX_BOARD_UNSPEC;
	    ddxRequestedScreens[i].grafinfo=	(grafData*)Xalloc(sizeof(grafData));
	}
	ddxNRequestedScreens=	nBoards;
	return(ddxNRequestedScreens>0);
    }
    return(FALSE);
}

/***====================================================================***/

Bool
ddxFindBoardInfo(ddxScreenRequest *pRequest)
{
Bool	found= FALSE;
char	*buf;
int	val;
char    *filename;


extern char *grafError();

    if (pRequest->type!=NULL && 
		pRequest->mode!=NULL && pRequest->boardNum!=DDX_BOARD_UNSPEC)
	{
	return(TRUE);
	}
    if ((nBoards>0)||ddxFindAvailableBoards()) 
	{
	if (pRequest->mode!=NULL && pRequest->type!=NULL) 
	    {
	    pRequest->boardNum= ddxFindFirstMode(pRequest->mode);
	    found= (pRequest->boardNum!=DDX_BOARD_UNSPEC);
	    }
	else 
	    {
	    if (pRequest->boardNum==DDX_BOARD_UNSPEC)
		pRequest->boardNum= ddxFindFirstAvailableBoard();
	    if (pRequest->boardNum<nBoards) 
		{
		/*
		 * Now read the grafinfo file and get some info.
		 */
		if((filename=grafGetName(pRequest->mode)) &&
		   grafParseFile(filename, pRequest->mode, pRequest->grafinfo)) 
		    {

		    /* S003 */
		    /* get the monitor info - return value can be ignored for
		       now */
		    grafParseMon (pRequest->grafinfo);

		    if (grafGetString(pRequest->grafinfo,"XDRIVER",&buf)) 
			pRequest->type=	(char *)Xstrdup(buf);
		    else
			pRequest->type= DDX_TYPE_DEFAULT;

		    if (grafGetInt(pRequest->grafinfo,"DEPTH",&val)) 
			{					/* S003 vvv */
			if (val < 1 || val > 32 )
			    {
			    ErrorF("ddxFindBoardInfo:Unsupported depth %d\n", val);
			    return FALSE;
			    }
			pRequest->dfltDepth=val;
			/*
			 * I don't like hardcoding these in.  Is there a 
			 * better way?
			 */
			if (val == 1)
			    pRequest->dfltBpp = 1;
			else if ( val <= 8 )
			    pRequest->dfltBpp = 8;
			else if ( val <= 16 )
			    pRequest->dfltBpp = 16;
			else
			    pRequest->dfltBpp = 32;
			    
			}
		    else		/* Assume 4, 8, 32  for now */
			{
			ErrorF("Depth not specified in grafinfo file, using 4\n");
			pRequest->dfltDepth = 4;
			pRequest->dfltBpp = 8;
			}
		    pRequest->dfltPad = 32;			/* S003 ^^^ */

		    if (grafGetString(pRequest->grafinfo,"VISUAL",&buf)) 
			{
			/* It'd be nice if this was shared code */
			if (strcmp(buf,"StaticGray")==0)
			    pRequest->dfltClass= StaticGray;
			else if (strcmp(buf,"GrayScale")==0)
			    pRequest->dfltClass= GrayScale;
			else if (strcmp(buf,"StaticColor")==0)
			    pRequest->dfltClass= StaticColor;
			else if (strcmp(buf,"PseudoColor")==0)
			    pRequest->dfltClass= PseudoColor;
			else if (strcmp(buf,"TrueColor")==0)
			    pRequest->dfltClass= TrueColor;
			else if (strcmp(buf,"DirectColor")==0)
			    pRequest->dfltClass= DirectColor;
			}

		    found= TRUE;
		    }
		else
		    ErrorF("%s %s\n", filename, grafError());

		}
	    }
	}
    return(found);
}

