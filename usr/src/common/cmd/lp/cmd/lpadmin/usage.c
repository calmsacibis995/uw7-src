/*		copyright	"%c%" 	*/

#ident	"@(#)usage.c	1.2"
#ident  "$Header$"

#include "lp.h"
#include "printers.h"
#define WHO_AM_I           I_AM_LPADMIN
#include "oam.h"

/**
 ** usage() - PRINT COMMAND USAGE
 **/

void			usage ()
{
#ifdef NETWORKING
   LP_OUTMSG(INFO, E_ADM_USAGE);
#else
   LP_OUTMSG(INFO, E_ADM_USAGENONET);
#endif
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE1);
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE2);
#if	defined(CAN_DO_MODULES)
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE3);
#endif
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE4);
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE5);
   LP_OUTMSG(INFO|MM_NOSTD, E_ADM_USAGE6);

   return;
}
