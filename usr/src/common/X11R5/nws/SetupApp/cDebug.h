#ident	"@(#)cDebug.h	1.2"
#ifndef CDEBUG_H
#define CDEBUG_H

/*
//  This file contains the structures, constants, and macros needed for debug
*/

#include	<iostream.h>		//  for cout
#include	<stdio.h>
#include	"setup.h"		//  for AppStruct definition


#ifdef CDEBUG_DEFINITION		// define storage (cDebug.C defines this)
FILE	*log = stdout;			//  stdout used only if cDebugLevel ON
					//  && cLogFile is NULL.
#else					// simply declare 
extern FILE	*log;
#endif // CDEBUG_DEFINITION

extern int	cDebugInit (int debugLevel,char *debugFileName, int maxLevel);
extern void	xError (Display *display, XErrorEvent *errorEvent);


extern AppStruct	app;		//  the struct containing app variables

/*  The following logging macro means the log"X" call should look like:
//
//  logX (level, "param1=", param1, ", param2=", param2);
//	where "X" in logX is the number of parameters you're calling with
//	(up to 6), "level" is any of the defined levels (1-5), and the 
//	remaining arguments are like you would use with cout.		     */

#ifdef	SETUPAPP_DEBUG (debug code compiled in)

//#define	cLog(level, x)	((app.rData.cDebugLevel && (level == \
		    app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? x:0)
//#define	log(level, u,v,w,x,y,z)	((app.rData.cDebugLevel && (level == \
app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<y<<z<<"."<<endl:0)
//#define	log(level, u,v,w,x,y,z)	((app.rData.cDebugLevel && (level == \
app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? (v? (w? (x? (y? (z? (cout<<u<<v<<w<<x<<y<<z<<"."<<endl) : (cout<<u<<v<<w<<x<<y<<"."<<endl)) : (cout<<u<<v<<w<<x<<"."<<endl)) : (cout<<u<<v<<w<<"."<<endl)) : (cout<<u<<v<<"."<<endl)) : (cout<<u<<"."<<endl)):0)
#define	log1(level, u)		 ((app.rData.cDebugLevel && (level ==\
   app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<"."<<endl:0)
#define	log2(level, u,v)	 ((app.rData.cDebugLevel && (level == \
   app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<"."<<endl:0)
#define	log3(level, u,v,w)	 ((app.rData.cDebugLevel && (level == \
   app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<"."<<endl:0)
#define	log4(level, u,v,w,x)	 ((app.rData.cDebugLevel && (level == \
   app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<"."<<endl:0)
#define	log5(level, u,v,w,x,y)	 ((app.rData.cDebugLevel && (level == \
   app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<y<<"."<<endl:0)
#define	log6(level, u,v,w,x,y,z) ((app.rData.cDebugLevel && (level == \
   app.rData.cDebugLevel || app.rData.cDebugLevel == C_ALL))? cout<<u<<v<<w<<x<<y<<z<<"."<<endl:0)

#else	//  SETUPAPP_DEBUG not defined (so debug code is *not* compiled in)

#define	log1(level, u)
#define	log2(level, u,v)
#define	log3(level, u,v,w)
#define	log4(level, u,v,w,x)
#define	log5(level, u,v,w,x,y)
#define	log6(level, u,v,w,x,y,z)

#endif	SETUPAPP_DEBUG

/*  DEBUG levels for this client					     */
#define	C_OFF		0		//  for no debugging (it is off by default)
#define	C_ERR		1		//  for capturing error messages only
#define	C_API		2		//  to trace setup API usage only
#define	C_FUNC		3		//  to trace function calls only
#define	C_PWD		4		//  to trace only password-related stuff
#define	C_ALL		5		//  to capture all debugging statements

#endif	//	CDEBUG_H
