#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <iforpmapi.h>
#include <licenseIDs.h>
#include <ctype.h>

#define LIC_ID_IMAP 8
#define LIC_VER_IMAP "5.5"

#define SCO_VENDOR 0

/***********************************************************************

  This file contains code for licensing the imapd. The policy hear is to
  use the "y" flag for SCO_VENDOR & LIC_ID_IMAP. The policy does
  not restrict license based on user count. If the product is licensed,
  then the request for a license will always succeded. 

***********************************************************************/

/*********************************************************************** EXT **
 *
 * Function: check_license
 *
 * Purpose:  Entry point for license checking.
 *
 * I/O  : NONE
 *
 *****************************************************************************/


int check_license ()
{
  long status;
  char *res;

  struct sco_license license[] = {
	{ SCO_VENDOR,LIC_ID_IMAP,"5.5",},};

  status = check_any_license(license,1,"imapd",&res);


  if ( status != 1 )
	syslog (LOG_INFO,"Licensing has failed: %s",res);

  return;

}

