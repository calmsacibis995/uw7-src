#include <syslog.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <iforpmapi.h>
#include <ctype.h>


/***********************************************************************

  This file contains code for licensing the imapd. The policy hear is to
  use a unique license ID, therfore using our own user count. The default
  policy for the PMD is to NOT keep track of the number of licenses that
  have been allocated. A policy module would need to be added to the
  PMD to dictate policy.

***********************************************************************/

#define LIC_ID_IMAP 158
#define LIC_VER_IMAP "5.5"


static ifor_pm_trns_hndl_t lichandle;

/*********************************************************************** INT **
 *
 * Function: license_handler
 *
 * Purpose: Call back function for Policy Magager
 *
 * I/O	: msg         - Error or status messages from the PMD
 *        flags       - Contains a set of bit flags which indicate the 
 *			presence or absence of the various messages and 
 *			actions to perform. 
 *        trns_handle - Ignore
 *        vmsg        - Returns the annotation line (Third line)
 *
 *****************************************************************************/

void
license_handler(msg, flags, trns_handle, vmsg)
  char *msg;
  unsigned int flags;
  ifor_pm_trns_hndl_t trns_handle;
  char *vmsg;
{
  if (flags & IFOR_PM_VND_MSG)
    syslog (LOG_INFO, "%s", vmsg);
  if (flags & IFOR_PM_WARN)
    syslog (LOG_INFO, "WARNING: %s", msg);
  if (flags & IFOR_PM_CONTINUE)
    syslog (LOG_INFO, "CONTINUE: %s", msg);
/*
  if (flags & IFOR_PM_EXIT)
    syslog (LOG_INFO, "EXIT!: %s", msg);
*/
}

/*********************************************************************** INT **
 *
 * Function:  license_error
 *
 * Purpose: General I/O on license error 
 *
 * I/O	: code   - Status code returned from PM function call 
 *        errmsg - Error message to display
 *
 *****************************************************************************/

void
license_error(code, errmsg)
  long code;
  char *errmsg;
{
  switch (code)
    {
      case IFOR_PM_FATAL:
        syslog (LOG_INFO, "Fatal licensing error");
        if (errmsg && errmsg[0])
          syslog (LOG_INFO, "\t%s", errmsg);
        /* exit (33); Convention */
        break;

      case IFOR_PM_NO_INIT:
        syslog(LOG_INFO, "Warning: IFOR/PM not initialized.");
        if (errmsg && errmsg[0])
          syslog(LOG_INFO, "\t%s", errmsg);
        break;

      case IFOR_PM_BAD_PARAM:
        syslog(LOG_INFO, "Warning: Bad IFOR/PM parameter.");
        if (errmsg && errmsg[0])
          syslog(LOG_INFO, "\t%s", errmsg);
        break;

      case IFOR_PM_REINIT:
        syslog(LOG_INFO, "Warning: IFOR/PM already initialized.");
        if (errmsg && errmsg[0])
          syslog(LOG_INFO, "\t%s", errmsg);
        break;
    }
}

/*********************************************************************** EXT **
 *
 * Function: check_license
 *
 * Purpose:  Entry point for license checking.
 *
 * I/O	: NONE 
 *
 *****************************************************************************/

int check_license ()
{
  long status;

  status = ifor_pm_init_sco (license_handler, IFOR_PM_NO_SIGNAL);
  if (status != IFOR_PM_OK)
    license_error (status, "Initializing IFOR/PMAPI");
  if ((status == IFOR_PM_OK) || (status == IFOR_PM_REINIT))
    {
      status = ifor_pm_request_sco (LIC_ID_IMAP, LIC_VER_IMAP, IFOR_PM_SYNC, &lichandle);
      if (status != IFOR_PM_OK)
        license_error (status, "IFOR/PMAPI request failed");
    }

  return;
}

