/*----------------------------------------------------------------------
  $Id$

            T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade and Mike Seibel
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   Administration Builiding, AG-44
   Seattle, Washington, 98195, USA
   Internet: lgl@CAC.Washington.EDU
             mikes@CAC.Washington.EDU

   Please address all bugs and comments to "pine-bugs@cac.washington.edu"


   Pine and Pico are registered trademarks of the University of Washington.
   No commercial use of these trademarks may be made without prior written
   permission of the University of Washington.

   Pine, Pico, and Pilot software and its included text are Copyright
   1989-1996 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================
       headers.h

   The include file to always include that includes a few other things
     -  includes the most general system files and other pine include files
     -  declares the global variables
       
 ====*/
         

#ifndef _HEADERS_INCLUDED
#define _HEADERS_INCLUDED

/*----------------------------------------------------------------------
           Include files
 
 System specific includes and defines are in os.h, the source for which
is os-xxx.h. (Don't edit osdep.h; edit os-xxx.h instead.)
 ----*/
#include <stdio.h>

#include "os.h"

/* These includes provide c-client prototypes and such */
#include "../c-client/mail.h"
#include "../c-client/osdep.h"
#include "../c-client/rfc822.h"
#include "../c-client/imap2.h"
#include "../c-client/misc.h"

/* These includes are all ANSI, and OK with all other compilers (so far) */
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>

#include "../pico/pico.h"

#ifdef	ANSI
#define	PROTO(args)	args
#else
#define	PROTO(args)	()
#endif

#include "pine.h"

#include "context.h"

#include "helptext.h"



/*----------------------------------------------------------------------
    The few global variables we use in Pine
  ----*/

extern struct pine *ps_global;

extern char	   *pine_version;	/* pointer to version string	     */

extern int          timeout;		/* referenced in pico, set in pine.c */

extern char         tmp_20k_buf[];

#ifdef DEBUG
extern FILE        *debugfile;		/* file for debug output	  */
extern int          debug;		/* debugging level or none (zero) */
#endif

#endif /* _HEADERS_INCLUDEDS */
