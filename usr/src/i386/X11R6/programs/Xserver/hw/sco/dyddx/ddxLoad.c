/*
 *	@(#) ddxLoad.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Mar 26 20:54:07 PST 1991	mikep@sco.com
 *	- Added DYNAMIC_LOAD defines and ctype.h
 *	S001	Wed Aug 07          PST 1991	pavelr@sco.com
 *	- Removed DYNAMIC_LOAD defines
 *	S002	Sun Aug 25 23:11:13 PDT 1991	mikep@sco.com
 *	- Added grafinfo support.  Change error messages to print out
 *	mode.
 *	S003	Tue Sep 10 16:46:59 PDT 1991	mikep@sco.com
 *	- Modified the pRequest struct to contain BPP and Padding.
 *	S004	Sat Sep 28 12:54:46 PDT 1991	mikep@sco.com
 *	- Change ddx module name to xxx/XDriver.o
 *	S005	Mon Sep 30 13:09:46 PDT 1991	pavelr@sco.com
 *	- DO NOT use an environment variable to find the ddx routines
 *	S006	Mon Aug 31 19:30:21 PDT 1992	mikep@sco.com
 *	- Move loadDynamicDDX to it's own file.  That way static servers
 *	don't try to use it.
 *	S007	Thu Oct 15 12:11:08 PDT 1992	mikep@sco.com
 *	- Change strdup to Xstrdup.
 *
 */
#include <sys/types.h>
#include <sys/param.h>
#ifdef sco
#include <dirent.h>
#define MAXPATHLEN MAXNAMLEN
#else
#include <sys/dir.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "X.h"
#include "misc.h"
#include "commonDDX.h"

#include "dyddx.h"						/* S002 */
#include "ddxScreen.h"

/***====================================================================***/

int		 ddxWrapX=		0;
int		 ddxWrapY=		0;

int		 ddxDfltDepth=		8;
int		 ddxDfltClass=		PseudoColor;
int		 ddxDfltLoadType=	DDX_LOAD_UNSPEC;

int		 ddxCurrentScreen=	0;

int		 ddxNRequestedScreens=	0;
ddxScreenRequest ddxRequestedScreens[MAXSCREENS];

int		 ddxNumActiveScreens=	0;
ddxScreenInfo	*ddxActiveScreens[MAXSCREENS];

/***====================================================================***/

static Bool
ParseScreenArgs(char *arg,ddxScreenRequest *pRequest)
{
char	*tmp,*field,*value;
Bool	ok= TRUE;

    tmp= (char *)ALLOCATE_LOCAL(strlen(arg)+1);
    if (tmp) {
	strcpy(tmp,arg);
	while ((tmp)&&(*tmp)&&(ok)) {
	    field= tmp;
	    value= (char *)index(tmp,'=');
	    if (!value) {
		ErrorF("expected '=' in board specification \"%s\"\n",arg);
		ok= FALSE;
		break;
	    }
	    *value++= '\0';
	    tmp= (char *)index(value,',');
	    if (tmp) {
		*tmp++= '\0';
	    }
	    if (strcmp(field,"type")==0) {
		pRequest->type= Xstrdup(value);	
	    }
	    else if (strcmp(field,"board")==0) {
		if (isdigit(value[0]))
		    pRequest->boardNum= atoi(value);
		else {
		    ErrorF("Board number must be an integer\n");
		    ErrorF("Board number in specification \"%s\" ignored\n",
									arg);
		}
	    }
	    else if (strcmp(field,"class")==0) {
		if (strcmp(value,"StaticGray")==0)
		    pRequest->dfltClass= StaticGray;
		else if (strcmp(value,"GrayScale")==0)
		    pRequest->dfltClass= GrayScale;
		else if (strcmp(value,"StaticColor")==0)
		    pRequest->dfltClass= StaticColor;
		else if (strcmp(value,"PseudoColor")==0)
		    pRequest->dfltClass= PseudoColor;
		else if (strcmp(value,"TrueColor")==0)
		    pRequest->dfltClass= TrueColor;
		else if (strcmp(value,"DirectColor")==0)
		    pRequest->dfltClass= DirectColor;

		else {
		    ErrorF("Unknown class in specification \"%s\"\n",value);
		    ErrorF("ignored\n");
		}
	    }
	    else if (strcmp(field,"depth")==0) {
		if (isdigit(value[0]))
		    pRequest->dfltDepth= atoi(value);
		else {
		    ErrorF("Default visual depth must be an integer\n");
		    ErrorF("Depth in specification \"%s\" ignored\n",arg);
		}
	    }
	    else if (strcmp(field,"left")==0) {
		if (isdigit(value[0]))
		    pRequest->nextLeft= atoi(value);
		else {
		    ErrorF("Adjacent screen specificaiton must be integer\n");
		    ErrorF("Left neighbor in \"%s\" ignored\n",arg);
		}
	    }
	    else if (strcmp(field,"right")==0) {
		if (isdigit(value[0]))
		    pRequest->nextRight= atoi(value);
		else {
		    ErrorF("Adjacent screen specificaiton must be integer\n");
		    ErrorF("Right neighbor in \"%s\" ignored\n",arg);
		}
	    }
	    else if (strcmp(field,"above")==0) {
		if (isdigit(value[0]))
		    pRequest->nextAbove= atoi(value);
		else {
		    ErrorF("Adjacent screen specificaiton must be integer\n");
		    ErrorF("Top neighbor in \"%s\" ignored\n",arg);
		}
	    }
	    else if (strcmp(field,"below")==0) {
		if (isdigit(value[0]))
		    pRequest->nextBelow= atoi(value);
		else {
		    ErrorF("Adjacent screen specificaiton must be integer\n");
		    ErrorF("Bottom neighbor in \"%s\" ignored\n",arg);
		}
	    }
	    else if (strcmp(field,"load")==0) {
		if (strcasecmp(value,"static")==0)
		    pRequest->ddxLoad=	DDX_LOAD_STATIC;
		else if (strcasecmp(value,"dynamic")==0)
		    pRequest->ddxLoad=	DDX_LOAD_DYNAMIC;
		else {
		    ErrorF("Unknown DDX load method \"%s\"\n",value);
		    ErrorF("ignored.\n");
		}
	    }
	}
	DEALLOCATE_LOCAL(tmp);
    }
    else ok= FALSE;
    return(ok);
}

/***====================================================================***/

int
ddxAddScreenRequest( register int argc, register char **argv, int i )
{
ddxScreenRequest	 req;

    if (i>=argc-1) {
	ErrorF("Error!  board specification must follow -hw option\n");
	return(0);
    }
    /* Get to where we want to be */
    argv += i ;
    argc -= i ;

#ifndef NDEBUG
    if ( strcmp( "-hw", *argv ) ) {
	ErrorF( "Bad argument to ddxAddScreenRequest\n" ) ;
	return 0 ;
    }
#endif /* ndef NDEBUG */

    /* The first arg is junk */
    argv++ ;

    req.mode=		NULL;					/* S002 */
    req.type=		NULL;
    req.boardNum=	DDX_BOARD_UNSPEC;
    req.ddxLoad=	DDX_LOAD_UNSPEC;
    req.dfltClass=	DDX_CLASS_UNSPEC;
    req.dfltDepth=	DDX_DEPTH_UNSPEC;
    req.dfltBpp=	DDX_DEPTH_UNSPEC;			/* S003 */
    req.dfltPad=	DDX_DEPTH_UNSPEC;			/* S003 */
    req.nextLeft=	DDX_BOARD_UNSPEC;
    req.nextRight=	DDX_BOARD_UNSPEC;
    req.nextAbove=	DDX_BOARD_UNSPEC;
    req.nextBelow=	DDX_BOARD_UNSPEC;
    req.grafinfo=	(grafData *)Xalloc(sizeof(grafData));	/* S002 */

    if (ParseScreenArgs(argv[0],&req)) {
	if (ddxNRequestedScreens<MAXSCREENS) {
	    ddxRequestedScreens[ddxNRequestedScreens++]= req;
	}
	else {
	    ErrorF("Warning! Too many screens requested.\n");
	    ErrorF("Board number %d (type %s) ignored\n",req.boardNum,
					  (req.type?req.type:"unspecified"));
	}
	return(2);
    }
    return(0);
}

/***====================================================================***/

int
ddxSetVisualRequest( register int argc, register char **argv, int i )
{
    if (i>=argc-1) {
	ErrorF("Error!  visual class must follow -class option\n");
	return(0);
    }
    i++;
    if (strcasecmp(argv[i],"StaticGray")==0)
	ddxDfltClass= StaticGray;
    else if (strcasecmp(argv[i],"GrayScale")==0)
	ddxDfltClass= GrayScale;
    else if (strcasecmp(argv[i],"StaticColor")==0)
	ddxDfltClass= StaticColor;
    else if (strcasecmp(argv[i],"PseudoColor")==0)
	ddxDfltClass= PseudoColor;
    else if (strcasecmp(argv[i],"TrueColor")==0)
	ddxDfltClass= TrueColor;
    else if (strcasecmp(argv[i],"DirectColor")==0)
	ddxDfltClass= DirectColor;
    else {
	ErrorF("Unknown class \"%s\" specified for -class option\n",argv[i]);
	ErrorF("ignored\n");
	return(0);
    }
    NOTICE1(DEBUG_INIT,"Set server default visual class to %s\n",argv[i]);
    return(2);
}

/***====================================================================***/

int
ddxSetLoadType( register int argc, register char **argv, int i )
{
    if (strcasecmp(argv[i],"-dynamic"))	    ddxDfltLoadType= DDX_LOAD_DYNAMIC;
    else if (strcasecmp(argv[i],"-static")) ddxDfltLoadType= DDX_LOAD_STATIC;
    else return(0);
    return(1);
}

/***====================================================================***/

Bool
ddxOpenScreens()
{
ddxScreenRequest *pRequest= NULL,*pLastRequest;
int		  i;

    if ((ddxNRequestedScreens==0)&&(!ddxRequestAllBoards())) {
	return(0);
    }
    for (i=0;i<ddxNRequestedScreens;i++) {
	pLastRequest= pRequest;
	pRequest= &ddxRequestedScreens[i];
	if ((!pRequest->mode)||(!pRequest->type)||		/* S002 */
				(pRequest->boardNum==DDX_BOARD_UNSPEC)) {
	    if(!ddxFindBoardInfo(pRequest)) {
		ErrorF("Couldn't find board info for %s\n", pRequest->mode);
		continue;
	    }
	}
	if (!loadStaticDDX(pRequest) && !loadDynamicDDX(pRequest)) {
	    ErrorF("Couldn't set mode \"%s\" (type %s, num %d)\n", /* S002 */
		    pRequest->mode, pRequest->type, pRequest->boardNum);
	    pRequest= pLastRequest; /* pLast*SuccessfulRequest */
	}
	else if (pLastRequest!=NULL) {
	    if ((pLastRequest->nextRight==DDX_BOARD_UNSPEC)&&
		(pRequest->nextLeft==DDX_BOARD_UNSPEC)) {
		NOTICE2(DEBUG_INIT,"putting screen %d left of screen %d\n",	
									i-1,i);
		pLastRequest->nextRight=	i;
		pRequest->nextLeft=		i-1;
	    }
	}
    }
    ddxCurrentScreen=	0;
    if (ddxNumActiveScreens>0) {
	/* 12/20/90 (ef) -- this should be in a separate function */
	if (ddxWrapX) {
	    if ((ddxActiveScreens[0]->pRequest->nextLeft==DDX_BOARD_UNSPEC)&&
	        (ddxActiveScreens[ddxNumActiveScreens-1]->pRequest->nextRight==
							DDX_BOARD_UNSPEC)) {
		ddxActiveScreens[0]->pRequest->nextLeft= ddxNumActiveScreens-1;
		ddxActiveScreens[ddxNumActiveScreens-1]->pRequest->nextRight= 0;
		NOTICE1(DEBUG_INIT,"wrapping X from screen 0 to screen %d\n",
							numActiveScreens-1);
	    }
	    else ErrorF("Explicit connection conflicts with wrap x\n");
	}
	if (ddxWrapY) {
	    for (i=0;i<ddxNumActiveScreens;i++) {
		pRequest= ddxActiveScreens[i]->pRequest;
		if ((pRequest->nextAbove==DDX_BOARD_UNSPEC)&&
		    (pRequest->nextBelow==DDX_BOARD_UNSPEC)) {
		    pRequest->nextAbove=	i;
		    pRequest->nextBelow=	i;
		    NOTICE1(DEBUG_INIT,"Wrapping Y on screen %d\n",i);
		}
		else ErrorF("Explicit connection conflicts with wrap y on screen %d\n",i);
	    }
	}
	return 1;
    }
    return 0;
}
