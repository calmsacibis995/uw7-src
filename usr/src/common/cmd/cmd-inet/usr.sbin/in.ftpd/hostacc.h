#ident	"@(#)hostacc.h	1.2"

/*
 *  @(#) hostacc.h  -   Header file used in the implementation of
 *              host access for the experimental FTP daemon
 *              developed at Washington University.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * AUTHOR
 *  Bart Muijzer    <bartm@cv.ruu.nl>
 *
 * HISTORY
 *  930316  BM  Created
 *  930317  BM  Cleanups, better readability
 */

#ifdef  HOST_ACCESS

#include <stdio.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "pathnames.h"          /* From the ftpd sources    */

/*
 * Host Access types, as stored in the ha_type field, 
 * and some other constants. All of this is tunable as
 * long as you don't depend on the values.
 */

#define ALLOW   1
#define DENY    2

#define MAXLEN  1024    /* Maximum length of one line in config file */
#define MAXLIN  100     /* Max. number of non-comment and non-empty  */
                        /* lines in config file                      */
#define MAXHST  10      /* Max. number of hosts allowed on one line  */

/* ------------------------------------------------------------------------- */

/*
 * Structure holding all host-access information 
 */

typedef struct  {
    short   ha_type;            /* ALLOW | DENY             */
    char    *ha_login;          /* Loginname to investigate */
    char    *ha_hosts[MAXHST];  /* Array of hostnames       */
} hacc_t;

/* ------------------------------------------------------------------------ */

int rhost_ok();

static  int sethacc(),
        endhacc();
static  hacc_t  *gethacc();

static  char    *strnsav();
static  void    fatal();

#endif  /* HOST_ACCESS */
