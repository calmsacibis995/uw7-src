#ident	"@(#)hostacc.c	1.3"

/*
 *      @(#) hostacc.c  -       Implementation of host access for the
 *                              experimental FTP daemon developed at
 *                              Washington University.
 *  $Id$
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * AUTHOR
 *      Bart Muijzer    <bartm@cv.ruu.nl>
 *
 * HISTORY
 *      930316  BM      Created
 *      930317  BM      Converted to local naming convention;
 *                      added rhost_ok(), cleanup code in enghacc()
 *      930318  BM      Ported to BSD; fixed memory leaks
 *      930322  BM      Changed algorithm: not in configfile =  allow
 *                                         in configfile and match = allow|deny
 *                                         in configfile and no match = deny
 *
 */

#include "config.h"

#ifdef  HOST_ACCESS

#include "hostacc.h"

#ifdef	INTL
#  include <locale.h>
#  include "ftpd_msg.h"
   extern nl_catd catd;
#else
#  define MSGSTR(num, str)	(str)
#endif	/* INTL */

static  char    linbuf[MAXLEN];  /* Buffer to hold one line of config-file  */
static  char    unibuf[MAXLEN];  /* Buffer to hold unified line             */
static  hacc_t  ha_arr[MAXLIN];  /* Array with host access information      */

static  FILE    *ptFp;           /* FILE * into host access config file     */
static  int     iHaInd = 0;      /* Index in ha_arr                         */
static  int     iHaSize;         /* Will hold actual #elems in ha_arr       */
static  int     iFirstTim = 1;   /* Used by gethacc() to see if index in    */
                                 /* ha_arr needs to be reset                */

/* ------------------------------------------------------------------------ *\
 * FUNCTION  : rhost_ok                                                     *
 * PURPOSE   : Check if a host is allowed to make a connection              *
 * ARGUMENTS : Remote user name, remote host name, remote host address      *
 * RETURNS   : 1 if host is granted access, 0 if not                        *
\* ------------------------------------------------------------------------ */

int     rhost_ok(pcRuser, pcRhost, pcRaddr)
char    *pcRuser,
        *pcRhost,
        *pcRaddr;
{
        hacc_t  *ptHtmp;
        char    *pcHost;
        char    *ha_login;
        int     iInd, iLineMatch = 0, iUserSeen = 0;
        int     m;

        switch(sethacc()){
        case 1:
                /* no hostaccess file; disable mechanism */
                return(1);
                break;
        case -1:
                syslog(LOG_INFO, "rhost_ok: sethacc failed");
                return(0);
                break;
        default:
                break;
        }

        /* user names "ftp" and "anonymous" are equivalent */
        if (!strcasecmp(pcRuser, "anonymous"))
                pcRuser = "ftp";

        while (((ptHtmp = gethacc()) != (hacc_t *)NULL) && !iLineMatch)
        {
                if (strcasecmp(ptHtmp->ha_login, "anonymous"))
                        ha_login = ptHtmp->ha_login;
                else
                        ha_login = "ftp";

                if ((strcasecmp(pcRuser, ha_login)) && strcmp(ha_login, "*"))
                        /* wrong user, check rest of file */
                        continue;

                /*
                 * We have seen a line regarding the current user.
                 * Remember this.
                 */
                iUserSeen = 1;

                for(iInd=0, pcHost=ptHtmp->ha_hosts[0];
                    ((pcHost != NULL) && !iLineMatch);
                    pcHost=ptHtmp->ha_hosts[++iInd])
                {
                        if (isdigit(*pcHost))
                        {
                                iLineMatch = !fnmatch(pcHost, pcRaddr, NULL);
                        }
                        else
                        {
                                iLineMatch = !fnmatch(pcHost, pcRhost, NULL);
                        }
                        if (iLineMatch)
                        {
                                iLineMatch = (ptHtmp->ha_type == ALLOW) ? 1 : 0;
                                goto match;
                        }
                }
        }

match:
        /*
         * At this point, iUserSeen == 1 if we've seen lines regarding
         * the current user, and 0 otherwise. If we reached the end of
         * the config file without a match we allow. Else, we allow or
         * deny according to the rule found.
         */

        if (endhacc())
        {
                syslog(LOG_INFO, "rhost_ok: endhacc failed");
                return(0);
        }

        if (iUserSeen)
                return(ptHtmp == NULL) ? 0 : iLineMatch;
        else
                /* Nothing at all about user in configfile, allow */
                return(1);
}

/* ------------------------------------------------------------------------ *\
 * FUNCTION  : sethacc                                                      *
 * PURPOSE   : Initialize data structures for host access                   *
 * ARGUMENTS : None                                                         *
 * RETURNS   : -1 on failure, 1 if host access file doesn't exist,          *
 *             0 otherwise                                                  *
\* ------------------------------------------------------------------------ */

static  int     sethacc()
{
        int     iHaHind = 0;                    /* Index in list of hosts   */
        char    *pcBegin, *pcEnd, *pcColon;
        char    *pcTmp1, *pcTmp2;

        /* Open config file */
        if ((ptFp = fopen(_PATH_FTPHOSTS, "r")) == NULL)
        {
                if (errno == ENOENT)
                        return(1);
                else {
                fatal(MSGSTR(MSG_CANT_OPEN_HOST_ACC,
                        "Can't open host access file"));
                return (-1);
                }
        }

        while (fgets(linbuf, MAXLEN, ptFp) != NULL)
        {
                iHaHind = 0;

                /* Find first non-whitespace character */
                for (pcBegin=linbuf;
                     ((*pcBegin == '\t') || (*pcBegin == ' '));
                     pcBegin++)
                        ;
                
                /* Get rid of comments */
                if ((pcEnd = strchr(linbuf, '#')) != NULL)
                        *pcEnd = '\0';
                

                /* Skip empty lines */
                if ((pcBegin == pcEnd) || (*pcBegin == '\n'))
                        continue;
                
                /* Substitute all whitespace by a single ":" so we can
                 * easily break on words later on. The easiest way is 
                 * to copy the result into a temporary buffer (called
                 * the "unified buffer" because it will store a line in 
                 * the same format, regardless of the format the original
                 * line was in).
                 * The result will look like: "allow:name:host:host:host"
                 */
                for (pcTmp1=pcBegin, pcTmp2=unibuf; *pcTmp1; pcTmp1++)
                {
                        if (*pcTmp1 != '\t' && *pcTmp1 != ' ' && *pcTmp1 != '\n')
                                *pcTmp2++ = *pcTmp1;
                        else
                                /* whitespace */
                                if (*(pcTmp2-1) == ':')
                                        continue;
                                else
                                        *pcTmp2++ = ':';
                }

                /* Throw away trailing whitespace, now indicated by
                 * the last character of the unified buffer being a 
                 * colon. Remember where the news string ends.
                 */
                pcEnd = (*(pcTmp2 - 1) == ':') ? (pcTmp2 - 1) : pcTmp2;
                *pcEnd = '\0';          /* Terminate new string */
                                
                /* Store what's left of the line into the
                 * hacc_t structure. First the access type,
                 * then the loginname, and finally a list of
                 * hosts to which all this applies.
                 */
                pcBegin = unibuf;
                if (!strncmp(pcBegin, "deny", 4))
                {
                        ha_arr[iHaInd].ha_type = DENY;
                        pcBegin += 5;
                } else 
                        if (!strncmp(pcBegin, "allow", 5))
                        {
                                ha_arr[iHaInd].ha_type = ALLOW;
                                pcBegin += 6;
                        }
                        else {
                                fatal(MSGSTR(MSG_HOST_FORMAT_ERROR,
                                        "Format error in host access file"));
                                return(-1);
                        }

                if((pcColon = strchr(pcBegin, ':')) != NULL)
                        ha_arr[iHaInd].ha_login =
                                strnsav(pcBegin, (pcColon-pcBegin));
                else
                {
                        fatal(MSGSTR(MSG_HOST_FORMAT_ERROR,
                                        "Format error in host access file"));
                        return(-1);
                }

                pcBegin = pcColon+1;
                while ((pcColon = strchr(pcBegin, ':')) != NULL)
                {
                        ha_arr[iHaInd].ha_hosts[iHaHind++] =
                                strnsav(pcBegin, (pcColon-pcBegin));
                        pcBegin = pcColon+1;
                        if (iHaHind >= MAXHST)
                        {
                                fatal(MSGSTR(MSG_LINE_TOO_LONG,
                                        "Line too long"));
                                return(-1);
                        }
                }
                ha_arr[iHaInd].ha_hosts[iHaHind++] =
                                strnsav(pcBegin, (pcEnd-pcBegin));
                ha_arr[iHaInd].ha_hosts[iHaHind] = NULL;

                /*
                 * Check if we need to expand the array with
                 * host access information
                 */
                if (++iHaInd > MAXLIN)
                {
                        fatal(MSGSTR(MSG_CONFIG_TOO_BIG,
                                "Config file too big!!"));
                        return(-1);
                }
        }
        iHaSize = iHaInd;               /* Record current size of ha_arr */
        return ((feof(ptFp)) ? 0 : -1);
}

/* ------------------------------------------------------------------------ *\
 * FUNCTION  : gethacc                                                      *
 * PURPOSE   : return pointer to the next host_access structure             *
 * ARGUMENTS : None                                                         *
 * RETURNS   : NULL on failure, pointervalue otherwise                      *
\* ------------------------------------------------------------------------ */

static  hacc_t  *gethacc()
{
        static  int     iHaInd;
        hacc_t  *ptTmp;

        if ((ptTmp = (hacc_t *)malloc(sizeof(hacc_t))) == NULL)
                return(NULL);

        if (iFirstTim)
        {
                iFirstTim = 0;
                iHaInd = 0;
        }
        if (iHaInd >= iHaSize)
                return ((hacc_t *)NULL);
        else {
#ifdef USG
                memmove(ptTmp, &(ha_arr[iHaInd]), sizeof(hacc_t));
#else
                bcopy(&(ha_arr[iHaInd]), ptTmp, sizeof(hacc_t));
#endif
                iHaInd++;
                return(ptTmp);
        }
}

/* ------------------------------------------------------------------------ *\
 * FUNCTION  : endhacc                                                      *
 * PURPOSE   : Free allocated data structures for host access               *
 * ARGUMENTS : None                                                         *
 * RETURNS   : -1 on failure, 0 otherwise                                   *
\* ------------------------------------------------------------------------ */

static  int     endhacc()
{
        int     iInd;
        hacc_t  *ptHtmp;

	for (ptHtmp = ha_arr;
	     ptHtmp < ha_arr + MAXLIN && ptHtmp->ha_type;
	     ptHtmp++)
	{
		ptHtmp->ha_type = 0;
		if (ptHtmp->ha_login) {
			free(ptHtmp->ha_login);
			ptHtmp->ha_login = NULL;
		}
		for(iInd=0;
		    iInd < MAXHST && ptHtmp->ha_hosts[iInd];
		    iInd++) {
			free(ptHtmp->ha_hosts[iInd]);
			ptHtmp->ha_hosts[iInd] = NULL;
		}
	}

        if (fclose(ptFp))
                return (-1);
        return (0);
}

/* ------------------------------------------------------------------------ */

static  void    fatal(pcMsg)
char    *pcMsg;
{
        syslog(LOG_INFO, "host_access: %s", pcMsg);
}

static  char    *strnsav(pcStr,iLen)
char    *pcStr;
int     iLen;
{
        char    *pcBuf;

        if ((pcBuf = (char *)malloc(iLen+1)) == NULL)
                return(NULL);
        strncpy(pcBuf,pcStr,iLen);
        pcBuf[iLen] = '\0';
        return(pcBuf);
}

#endif  /* HOST_ACCESS */
