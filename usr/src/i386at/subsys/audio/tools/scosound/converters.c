/*-------------------------------------------------------------------------
  Copyright (c) 1994-1996      		The Santa Cruz Operation, Inc.
  -------------------------------------------------------------------------
  All rights reserved.  No part of this  program or publication may be
  reproduced, transmitted, transcribed, stored  in a retrieval system,
  or translated into any language or computer language, in any form or
  by any  means,  electronic, mechanical, magnetic, optical, chemical,
  biological, or otherwise, without the  prior written  permission of:

           The Santa Cruz Operation, Inc.  (408) 425-7222
           400 Encinal St, Santa Cruz, CA  95060 USA
  -------------------------------------------------------------------------

  SCCS  : @(#) converters.c 11.1 94/05/05 
  Author: shawnm
  Date  : 05-May-94
  File  : converters.c

  Description:
	This file contains converters.

  Modification History:
  S000,	05-May-94, shawnm
	created
  -----------------------------------------------------------------------*/

/* XXX temp for stderr */
#include <stdio.h>
#include <sys/soundcard.h>

/* 'ix          (Include Files: X)                                       */
#include 	<Xm/Xm.h>
#include 	<Xm/XmP.h>
#include 	<X11/StringDefs.h>

/* 'il          (Include Files: Local)                                   */
#include "scosound.h"

/* 'de          (Defines)                                                */
#define 	WHITE_STR		"white"
#define 	BLACK_STR		"black"
#define 	TRANSPARENT_STR		"transparent"
#define 	URGENT_STR		"urgent"
#define 	FOREGROUND_STR		"foreground"
#define 	BACKGROUND_STR		"background"
#define 	TOPSHADOW_STR		"topShadow"
#define 	BOTTOMSHADOW_STR	"bottomShadow"

#define 	UNSPEC_PM		"unspecified_pixmap"
#define 	XmRBits			"bits"

#define 	ASSIGN_COLOR_SYMBOL(cs, i, nm, pix) {\
	cs[i].name=nm; cs[i].value=NULL; cs[i].pixel=pix; }

/* 'fe          (Function Prototypes: External)                          */
#include 	"converters.h"	

/* 'ff          (Function Prototypes: Forward)                           */
static void 	cvtStringToSoundFileFormat(XrmValuePtr, Cardinal *,
	XrmValuePtr, XrmValuePtr);
static void 	cvtStringToSoundDataFormat(XrmValuePtr, Cardinal *,
	XrmValuePtr, XrmValuePtr);
static void 	cvtStringToLineMode(XrmValuePtr, Cardinal *,
	XrmValuePtr, XrmValuePtr);

/* 'vs          (Variables: Static)                                      */
/* 'fp          (Functions: Public)                                      */

void
RegisterConverters(appContext)
XtAppContext	 appContext;
{
	/* This should be done first so that ours are registered last */
	/* XXX
	XmRegisterConverters();
	_XmRegisterPixmapConverters();
	*/

	XtAppAddConverter(
		appContext,
		XtRString, 
		XtRSoundFileFormat, 
		cvtStringToSoundFileFormat, 
		NULL, 0);

	XtAppAddConverter(
		appContext,
		XtRString, 
		XtRSoundDataFormat, 
		cvtStringToSoundDataFormat, 
		NULL, 0);

}  /* END OF FUNCTION RegisterConverters */


/* 'fs          (Functions: Static)                                      */

#undef done
#define done(address, type) \
        {                                                       \
            toVal->addr = (caddr_t)address;                     \
            toVal->size = sizeof(type);                         \
        }


static void
cvtStringToSoundFileFormat(args, num_args, fromVal, toVal)
XrmValuePtr	 	args;
Cardinal		*num_args;
XrmValuePtr	 	fromVal;
XrmValuePtr	 	toVal;
{
	static int returnFileFormat;
	String s = (String)fromVal->addr;

	returnFileFormat = 0; /* XXX SoundAbbrevToFileFormat(s); */

	if (returnFileFormat == -1)
	{
		if (!strcasecmp(s, "wav"))
		{
			returnFileFormat = 0; /* XXX SoundAbbrevToFileFormat("wave"); */
		}
		else if (!strcasecmp(s, "au"))
		{
			returnFileFormat = 2; /* XXX SoundAbbrevToFileFormat("snd"); */
		}
	}

	if (returnFileFormat == -1)
	{
		/* XXX Use real Warnings */
		fprintf(stderr, "cvtStringToSoundFileFormat failed: %s\n", s);
	}
	else
	{
		done(&returnFileFormat, int);
	}
}  /* END OF FUNCTION cvtStringToSoundFileFormat */


static void
cvtStringToSoundDataFormat(args, num_args, fromVal, toVal)
XrmValuePtr	 	args;
Cardinal		*num_args;
XrmValuePtr	 	fromVal;
XrmValuePtr	 	toVal;
{
	static int returnDataFormat;
	String s = (String)fromVal->addr;

	returnDataFormat = 0; /* XXX AuDefineToFormat(s); */

	if (returnDataFormat == -1)
	{
		/* XXX Use real Warnings */
		fprintf(stderr, "cvtStringToSoundDataFormat failed: %s\n", s);
	}
	else
	{
		done(&returnDataFormat, int);
	}
}  /* END OF FUNCTION cvtStringToSoundDataFormat */

