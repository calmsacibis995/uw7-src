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

  SCCS  : @(#) error.h 7.1 97/10/22 
  Author: Shawn McMurdo (shawnm)
  Date  : 02-Nov-94, rewritten 28-Aug-96
  File  : error.h

  Description:
	This file contains the error message definitions.

  Modification History:
  S001,	28-Aug-96, shawnm
	rewritten
  S000,	02-Nov-94, shawnm
	created
  -----------------------------------------------------------------------*/

#ifndef SCOSOUND_ERROR_H
#define SCOSOUND_ERROR_H

typedef	struct	{
	String	out_of_memory;
	String	cant_connect;
	String	mismatched_formats;
} ErrorResources;

static	XtResource	errorResources[] = {
	{"errorOutOfMemory", "ErrorOutOfMemory", XtRString, sizeof(String),
		XtOffset(ErrorResources *, out_of_memory), XtRString, 
		"Out of memory."}, 
	{"errorCantConnect", "ErrorCantConnect", XtRString, sizeof(String),
		XtOffset(ErrorResources *, cant_connect), XtRString, 
		"Unable to connect to audio server."}, 
	{"errorMismatchedFormats", "ErrorMismatchedFormats", XtRString,
		sizeof(String), XtOffset(ErrorResources *, mismatched_formats),
		XtRString,
		"Mismatched file and data formats."},
};

#endif /* SCOSOUND_ERROR_H */
