/*		copyright	"%c%" 	*/


#ifndef	LOG_MGMT_H
#define	LOG_MGMT_H
/*==================================================================*/
/*
**
*/
#ident	"@(#)logMgmt.h	1.2"
#ident	"$Header$"

#include	"boolean.h"
#include	"pwd.h"


/*------------------------------------------------------------------*/
/*
*/
#ifdef	__STDC__

extern	void	WriteLogMsg (char *);
extern	void	SetLogMsgTagFn (char *(*) ());
extern	boolean	OpenLogFile (char *);

#else

extern	void	WriteLogMsg ();
extern	void	SetLogMsgTagFn ();
extern	boolean	OpenLogFile ();

#endif
/*==================================================================*/
#endif
