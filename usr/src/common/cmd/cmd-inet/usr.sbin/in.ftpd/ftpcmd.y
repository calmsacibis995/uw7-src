/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)ftpcmd.y    5.24 (Berkeley) 2/25/91
 */

/*
 * Grammar for FTP commands.
 * See RFC 959.
 */

%{
#ident	"@(#)ftpcmd.y	1.8"

#ifndef lint
static char sccsid[] = "@(#)$Id$ based on ftpcmd.y    5.24 (Berkeley) 2/25/91";
#endif /* not lint */

#include "config.h"
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#ifdef UXW
#include <sys/utsname.h>
#endif
#include <netinet/in.h>
#include <arpa/ftp.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <setjmp.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <time.h>
#include <string.h>
#include <limits.h>
#include "extensions.h"
#include "pathnames.h"

#ifdef	INTL
#  include <locale.h>
#  include "ftpd_msg.h"
   extern nl_catd catd;
#  define MN(num, str)		(num),(str)
#  define MNSTR(num, str)	MSGSTR((num),(str))
#else
#  define MSGSTR(num, str)	(str)
#  define MNSTR(num, str)	(str)
#  define MN(num, str)		(str)
#endif	/* INTL */

extern  int dolreplies;
extern  char ls_long[50];
extern  char ls_short[50];
extern  struct sockaddr_in data_dest;
extern	struct sockaddr_in his_addr;	/* added.  _H*/
extern  int logged_in;
extern  struct passwd *pw;
extern  int anonymous;
extern  int logging;
extern  int log_commands;
extern  int type;
extern  int form;
extern  int debug;
extern  int timeout;
extern  int maxtimeout;
extern  int pdata;
extern  char hostname[], remotehost[];
extern	char guestpw[], authuser[];		/* added.  _H*/
extern  char proctitle[];
extern  char *globerr;
extern  int usedefault;
extern  int transflag;
extern  char tmpline[];
extern  int data;
extern  int errno;
char    **ftpglob();
off_t   restart_point;
int     yyerrorcalled;

extern  char    *strunames[];
extern  char    *typenames[];
extern  char    *modenames[];
extern  char    *formnames[];

static	unsigned short cliport = 0;
static  int cmd_type;
static  int cmd_form;
static  int cmd_bytesz;
char    cbuf[512];
char    *fromname;

static void toolong();
extern void setproctitle();
/* Debian linux bison fix: moved this up, added forward decls */

struct tab {
    char    *name;
    short   token;
    short   state;
    short   implemented;    /* 1 if command is implemented */
#ifdef	INTL
    int     helpnum;
#endif
    char    *help;
};

extern struct tab cmdtab[];
extern struct tab sitetab[];

%}

%token
    A   B   C   E   F   I
    L   N   P   R   S   T

    SP  CRLF    COMMA   STRING  NUMBER

    USER    PASS    ACCT    REIN    QUIT    PORT
    PASV    TYPE    STRU    MODE    RETR    STOR
    APPE    MLFL    MAIL    MSND    MSOM    MSAM
    MRSQ    MRCP    ALLO    REST    RNFR    RNTO
    ABOR    DELE    CWD     LIST    NLST    SITE
    STAT    HELP    NOOP    MKD     RMD     PWD
    CDUP    STOU    SMNT    SYST    SIZE    MDTM

    UMASK   IDLE    CHMOD   GROUP   GPASS   NEWER
    MINFO   INDEX   EXEC    ALIAS   CDPATH  GROUPS
    LANG

    LEXERR

%union {
    char    *String;
    int     Number;
}

%type <String>  STRING password pathname pathstring username
%type <Number>  NUMBER byte_size check_login form_code 
%type <Number>  struct_code mode_code octal_number

%start  cmd_list

%%

cmd_list:   /* empty */
    |   cmd_list cmd
        = {
            fromname = (char *) NULL;
            restart_point = 0;
        }
    |   cmd_list rcmd
    ;

cmd:        USER SP username CRLF
        = {
            user($3);
            if (log_commands) syslog(LOG_INFO, "USER %s", $3);
            free($3);
        }
    |   PASS SP password CRLF
        = {
            if (log_commands)
                if (anonymous)
                    syslog(LOG_INFO, "PASS %s", $3);
                else
                    syslog(LOG_INFO, "PASS password");

            pass($3);
            free($3);
        }
    |   PORT check_login SP host_port CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "PORT");
            usedefault = 0;
            if (pdata >= 0) {
                (void) close(pdata);
                pdata = -1;
            }
/* H* port fix, part B: admonish the twit.
   Also require login before PORT works */
	    if ($2) {
	      if ((cliport > 1023) && (data_dest.sin_addr.s_addr > 0)) {
		reply(200, MSGSTR(MSG_PORT_SUCCESSFUL,
                      "PORT command successful."));
	      } else {
		syslog (LOG_WARNING, "refused PORT %lx,%d from %s",
			data_dest.sin_addr.s_addr,
			cliport, remotehost);
		reply(500, MSGSTR(MSG_ILLEGAL_PORT, "Illegal PORT Command"));
	      }
	    }
        }
    |   PASV check_login CRLF
        = {
/* Require login for PASV, too.  This actually fixes a bug -- telnet to an
   unfixed wu-ftpd and type PASV first off, and it crashes! */
	    if (log_commands) syslog(LOG_INFO, "PASV");
	    if ($2)
		passive();
        }
    |   TYPE SP type_code CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "TYPE %s", typenames[cmd_type]);
            switch (cmd_type) {

            case TYPE_A:
                if (cmd_form == FORM_N) {
                    reply(200, MSGSTR(MSG_TYPE_SET_TO_A, "Type set to A."));
                    type = cmd_type;
                    form = cmd_form;
                } else
                    reply(504, MSGSTR(MSG_FORM_MUST_BE_N, "Form must be N."));
                break;

            case TYPE_E:
                reply(504, MSGSTR(MSG_UNIMPL_E, "Type E not implemented."));
                break;

            case TYPE_I:
                reply(200, MSGSTR(MSG_TYPE_SET_TO_I, "Type set to I."));
                type = cmd_type;
                break;

            case TYPE_L:
#if NBBY == 8
                if (cmd_bytesz == 8) {
                    reply(200,  MSGSTR(MSG_TYPE_SET_TO_L,
                          "Type set to L (byte size 8)."));
                    type = cmd_type;
                } else
                    reply(504, MSGSTR(MSG_BYTE_SIZE, "Byte size must be 8."));
#else /* NBBY == 8 */
                UNIMPLEMENTED for NBBY != 8
#endif /* NBBY == 8 */
            }
        }
    |   STRU SP struct_code CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "STRU %s", strunames[$3]);
            switch ($3) {

            case STRU_F:
                reply(200, MSGSTR(MSG_STRU_F_OK, "STRU F ok."));
                break;

            default:
                reply(504, MSGSTR(MSG_UNIMPL_STRU, "Unimplemented STRU type."));
            }
        }
    |   MODE SP mode_code CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "MODE %s", modenames[$3]);
            switch ($3) {

            case MODE_S:
                reply(200, MSGSTR(MSG_MODE_S_OK, "MODE S ok."));
                break;

            default:
                reply(502, MSGSTR(MSG_UNIMPL_MODE, "Unimplemented MODE type."));
            }
        }
    |   ALLO SP NUMBER CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "ALLO %d", $3);
            reply(202, MSGSTR(MSG_ALLO_IGNORED, "ALLO command ignored."));
        }
    |   ALLO SP NUMBER SP R SP NUMBER CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "ALLO %d R %d", $3, $7);
            reply(202, MSGSTR(MSG_ALLO_IGNORED, "ALLO command ignored."));
        }
    |   RETR check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "RETR %s", $4);
            if ($2 && $4 != NULL)
                retrieve((char *) NULL, $4);
            if ($4 != NULL)
                free($4);
        }
    |   STOR check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "STOR %s", $4);
            if ($2 && $4 != NULL)
                store($4, "w", 0);
            if ($4 != NULL)
                free($4);
        }
    |   APPE check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "APPE %s", $4);
            if ($2 && $4 != NULL)
                store($4, "a", 0);
            if ($4 != NULL)
                free($4);
        }
    |   NLST check_login CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "NLST");
            if ($2)
                send_file_list(".");
        }
    |   NLST check_login SP STRING CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "NLST %s", $4);
            if ($2 && $4)
                send_file_list($4);
            if ($4 != NULL)
                free($4);
        }
    |   LIST check_login CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "LIST");
            if ($2)
        if (anonymous && dolreplies)
                retrieve(ls_long, "");
            else
                retrieve(ls_short, "");
        }
    |   LIST check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "LIST %s", $4);
            if ($2 && $4 != NULL)
        if (anonymous && dolreplies)
                retrieve(ls_long, $4);
            else
                retrieve(ls_short, $4);
            if ($4 != NULL)
                free($4);
        }
    |   STAT check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "STAT %s", $4);
            if ($2 && $4 != NULL)
                statfilecmd($4);
            if ($4 != NULL)
                free($4);
        }
    |   STAT CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "STAT");
            statcmd();
        }
    |   DELE check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "DELE %s", $4);
            if ($2 && $4 != NULL)
                delete($4);
            if ($4 != NULL)
                free($4);
        }
    |   RNTO check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "RNTO %s", $4);
	    if ($2) {
	      if (fromname) {
                renamecmd(fromname, $4);
                free(fromname);
                fromname = (char *) NULL;
              } else {
                reply(503, MSGSTR(MSG_BAD_SEQUENCE,
                      "Bad sequence of commands."));
              }
	    }
            free($4);
        }
    |   ABOR CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "ABOR");
            reply(225, MSGSTR(MSG_ABOR_SUCCESSFUL, "ABOR command successful."));
        }
    |   CWD check_login CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "CWD");
            if ($2)
                cwd(pw->pw_dir);
        }
    |   CWD check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "CWD %s", $4);
            if ($2 && $4 != NULL)
                cwd($4);
            if ($4 != NULL)
                free($4);
        }
    |   HELP CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "HELP");
            help(cmdtab, (char *) NULL);
        }
    |   HELP SP STRING CRLF
        = {
            register char *cp = (char *)$3;

            if (log_commands) syslog(LOG_INFO, "HELP %s", $3);
            if (strncasecmp(cp, "SITE", 4) == 0) {
                cp = (char *)$3 + 4;
                if (*cp == ' ')
                    cp++;
                if (*cp)
                    help(sitetab, cp);
                else
                    help(sitetab, (char *) NULL);
            } else
                help(cmdtab, $3);
            if ($3 != NULL)
                free($3);
        }
    |   NOOP CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "NOOP");
            reply(200, MSGSTR(MSG_NOOP_SUCCESSFUL, "NOOP command successful."));
        }
    |   MKD check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "MKD %s", $4);
            if ($2 && $4 != NULL)
                makedir($4);
            if ($4 != NULL)
                free($4);
        }
    |   RMD check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "RMD %s", $4);
            if ($2 && $4 != NULL)
                removedir($4);
            if ($4 != NULL)
                free($4);
        }
    |   PWD check_login CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "PWD");
            if ($2)
                pwd();
        }
    |   CDUP check_login CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "CDUP");
            if ($2)
                cwd("..");
        }

    |   SITE SP HELP CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE HELP");
            help(sitetab, (char *) NULL);
        }
    |   SITE SP HELP SP STRING CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE HELP %s", $5);
            help(sitetab, $5);
            if ($5 != NULL)
                free($5);
        }
    |   SITE SP UMASK check_login CRLF
        = {
            mode_t oldmask;

            if (log_commands) syslog(LOG_INFO, "SITE UMASK");
            if ($4) {
                oldmask = umask(0);
                (void) umask(oldmask);
                reply(200, MSGSTR(MSG_CURRENT_UMASK,
                      "Current UMASK is %03o"), oldmask);
            }
        }
    |   SITE SP UMASK check_login SP octal_number CRLF
        = {
            mode_t oldmask;
            struct aclmember *entry = NULL;
            int ok = 1;

            if (log_commands) syslog(LOG_INFO, "SITE UMASK %03o", $6);
            if ($4) {
                /* check for umask permission */
                while (getaclentry("umask", &entry) && ARG0 && ARG1 != NULL) {
                    if (type_match(ARG1)) 
                        if (*ARG0 == 'n')  ok = 0;
                }
                if (ok) {
                    if (($6 < 0) || ($6 > 0777)) {
                        reply(501, MSGSTR(MSG_BAD_UMASK, "Bad UMASK value"));
                    } else {
                        oldmask = umask((mode_t)$6);
                        reply(200, MSGSTR(MSG_UMASK_SET_TO,
                              "UMASK set to %03o (was %03o)"), $6, oldmask);
                    }
                } else 
                    reply(553, MSGSTR(MSG_UMASK_PERM_DENIED,
                          "Permission denied. (umask)"));
            }
        }
    |   SITE SP CHMOD check_login SP octal_number SP pathname CRLF
        = {
            struct aclmember *entry = NULL;
            int ok = 1;

            if (log_commands) syslog(LOG_INFO, "SITE CHMOD %03o %s", $6, $8);
            if ($4 && $8) {
                /* check for chmod permission */
                while (getaclentry("chmod", &entry) && ARG0 && ARG1 != NULL) {
                    if (type_match(ARG1)) 
                        if (*ARG0 == 'n')  ok = 0;
                }
                if (ok) {
                    if (($6 < 0) || ($6 > 0777))
                        reply(501, MSGSTR(MSG_CHMOD_BETWEEN,
                              "CHMOD: Mode value must be between 0 and 0777"));
                    else if (chmod($8, (mode_t) $6) < 0)
                        perror_reply(550, $8);
                    else
                        reply(200, MSGSTR(MSG_CHMOD_SUCCESSFUL,
                              "CHMOD command successful."));
                } else
                    reply(553, MSGSTR(MSG_CHMOD_PERM_DENIED,
                          "Permission denied. (chmod)"));
            }
            if ($8 != NULL)
                free($8);
        }
    |   SITE SP IDLE CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE IDLE");
            reply(200, MSGSTR(MSG_IDLE_LIMIT,
                  "Current IDLE time limit is %d seconds; max %d"),
                timeout, maxtimeout);
        }
    |   SITE SP IDLE SP NUMBER CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE IDLE %d", $5);
            if ($5 < 30 || $5 > maxtimeout) {
                reply(501, MSGSTR(MSG_IDLE_MAX,
                      "Maximum IDLE time must be between 30 and %d seconds"),
                    maxtimeout);
            } else {
                timeout = $5;
                (void) alarm((unsigned) timeout);
                reply(200, MSGSTR(MSG_IDLE_MAX_SET,
                      "Maximum IDLE time set to %d seconds"), timeout);
            }
        }
    |   SITE SP GROUP check_login SP username CRLF
        = {
#ifndef NO_PRIVATE
            if (log_commands) syslog(LOG_INFO, "SITE GROUP %s", $6);
            if ($4 && $6) priv_group($6);
            free($6);
#endif /* !NO_PRIVATE */
        }
    |   SITE SP GPASS check_login SP password CRLF
        = {
#ifndef NO_PRIVATE
            if (log_commands) syslog(LOG_INFO, "SITE GPASS password");
            if ($4 && $6) priv_gpass($6);
            free($6);
#endif /* !NO_PRIVATE */
        }
    |   SITE SP NEWER check_login SP STRING CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE NEWER %s", $6);
            if ($4 && $6) newer($6, ".", 0);
            free($6);
        }
    |   SITE SP NEWER check_login SP STRING SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE NEWER %s %s", $6, $8);
            if ($4 && $6 && $8) newer($6, $8, 0);
            free($6);
            free($8);
        }
    |   SITE SP MINFO check_login SP STRING CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE MINFO %s", $6);
            if ($4 && $6) newer($6, ".", 1);
            free($6);
        }
    |   SITE SP MINFO check_login SP STRING SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SITE MINFO %s %s", $6, $8);
            if ($4 && $6 && $8) newer($6, $8, 1);
            free($6);
            free($8);
        }
    |   SITE SP INDEX check_login SP STRING CRLF
        = {
            /* this is just for backward compatibility since we
             * thought of INDEX before we thought of EXEC
             */
            if ($4 != 0 && $6 != NULL) {
                char buf[MAXPATHLEN];
                if (strlen($6) + 7 <= sizeof(buf)) {
                    sprintf(buf, "index %s", (char*)$6);
                    (void) site_exec(buf);
                }
            }
            if ($6 != NULL)
                free($6);
        }
    |   SITE SP EXEC check_login SP STRING CRLF
        = {
            if ($4 != 0 && $6 != NULL) {
                (void) site_exec((char*)$6);
            }
            if ($6 != NULL)
                free($6);
        }
    |   SITE SP LANG SP STRING CRLF
        = {
#ifdef	INTL
            if ($5 != NULL) {
                (void) site_lang((char*)$5);
            }
#endif
            if ($5 != NULL)
                free($5);
        }

    |   STOU check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "STOU %s", $4);
            if ($2 && $4)
                store($4, "w", 1);
            if ($4 != NULL)
                free($4);
        }
    |   SYST CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SYST");
#ifdef unix
#ifdef BSD
            reply(215, "UNIX Type: L%d Version: BSD-%d",
                NBBY, BSD);
#else  /* BSD */
#ifdef UXW
            {
                struct utsname un;

                if (uname(&un) != -1)
                    reply(215, MSGSTR(MSG_SYSTEM_RELEASE,
                          "%s System V/%s Release %s, Version %s."),
                          "UNIX Type: L8 SCO UnixWare",
                          un.machine, un.release, un.version);
                else
                    reply(215, "UNIX Type: L%d", NBBY);
            }
#else  /* UXW */
            reply(215, "UNIX Type: L%d", NBBY);
#endif /* UXW */
#endif /* BSD */
#else  /* unix */
            reply(215, "UNKNOWN Type: L%d", NBBY);
#endif /* unix */
        }

        /*
         * SIZE is not in RFC959, but Postel has blessed it and
         * it will be in the updated RFC.
         *
         * Return size of file in a format suitable for
         * using with RESTART (we just count bytes).
         */
    |   SIZE check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "SIZE %s", $4);
            if ($2 && $4) {
                sizecmd($4);
            }
            if ($4 != NULL)
		 free($4);
        }

        /*
         * MDTM is not in RFC959, but Postel has blessed it and
         * it will be in the updated RFC.
         *
         * Return modification time of file as an ISO 3307
         * style time. E.g. YYYYMMDDHHMMSS or YYYYMMDDHHMMSS.xxx
         * where xxx is the fractional second (of any precision,
         * not necessarily 3 digits)
         */
    |   MDTM check_login SP pathname CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "MDTM %s", $4);
            if ($2 && $4) {
                struct stat stbuf;

                if (stat($4, &stbuf) < 0)
                    perror_reply(550, $4);
                else if ((stbuf.st_mode&S_IFMT) != S_IFREG) {
                    reply(550, MSGSTR(MSG_NOT_PLAIN, "%s: not a plain file."),
                        $4);
                } else {
                    register struct tm *t;
                    struct tm *gmtime();
                    t = gmtime(&stbuf.st_mtime);
                    reply(213,
                        "%04d%02d%02d%02d%02d%02d",
                        t->tm_year+1900, t->tm_mon+1, t->tm_mday,
                        t->tm_hour, t->tm_min, t->tm_sec);
                }
            }
            if ($4 != NULL)
                free($4);
        }
    |   QUIT CRLF
        = {
            if (log_commands) syslog(LOG_INFO, "QUIT");
            reply(221, MSGSTR(MSG_GOODBYE, "Goodbye."));
            dologout(0);
        }
    |   error CRLF
        = {
            yyerrok;
        }
    ;

rcmd:       RNFR check_login SP pathname CRLF
        = {
            char *renamefrom();

            if (log_commands) syslog(LOG_INFO, "RNFR %s", $4);
            restart_point = (off_t) 0;
            if ($2 && $4)
                fromname = renamefrom($4);
            if (fromname == 0 && $4)
                free($4);
        }
    |   REST SP byte_size CRLF
        = {
            long atol();

            fromname = 0;
            restart_point = $3;
            if (log_commands) syslog(LOG_INFO, "REST %d", (int)restart_point);
            reply(350, MSGSTR(MSG_RESTARTING_AT,
            "Restarting at %ld. Send STORE or RETRIEVE to initiate transfer."),
                (long)restart_point);
        }

    |   SITE SP ALIAS CRLF
        = {
           if (log_commands) syslog(LOG_INFO, "SITE ALIAS");
           alias ((char *)NULL);
        }
    |   SITE SP ALIAS SP STRING CRLF
        = {
           if (log_commands) syslog(LOG_INFO, "SITE ALIAS %s", $5);
           alias ($5);
           if ($5 != NULL)
               free($5);
        }
    |   SITE SP GROUPS CRLF
        = {
           if (log_commands) syslog(LOG_INFO, "SITE GROUPS");
           print_groups () ;
        }
    |   SITE SP CDPATH CRLF
        = {
           if (log_commands) syslog(LOG_INFO, "SITE CDPATH");
           cdpath () ;
        }
    ;
        
username:   STRING
    ;

password:   /* empty */
        = {
            $$ = (char *)malloc(1);
            $$[0] = '\0';
        }
    |   STRING
    ;

byte_size:  NUMBER
    ;

host_port:  NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA 
        NUMBER COMMA NUMBER
        = {
            register char *a, *p;

            a = (char *)&data_dest.sin_addr;
            a[0] = $1; a[1] = $3; a[2] = $5; a[3] = $7;
/* H* port fix, part A-1: Check the args against the client addr */
	    p = (char *)&his_addr.sin_addr;
	    if (memcmp (a, p, sizeof (data_dest.sin_addr)))
		memset (a, 0, sizeof (data_dest.sin_addr));	/* XXX */
	    p = (char *)&data_dest.sin_port;
/* H* port fix, part A-2: only allow client ports in "user space" */
	    p[0] = 0; p[1] = 0;
	    cliport = ($9 << 8) + $11;
	    if (cliport > 1023) {
		p[0] = $9; p[1] = $11;
	    }
            data_dest.sin_family = AF_INET;
        }
    ;

form_code:  N
    = {
        $$ = FORM_N;
    }
    |   T
    = {
        $$ = FORM_T;
    }
    |   C
    = {
        $$ = FORM_C;
    }
    ;

type_code:  A
    = {
        cmd_type = TYPE_A;
        cmd_form = FORM_N;
    }
    |   A SP form_code
    = {
        cmd_type = TYPE_A;
        cmd_form = $3;
    }
    |   E
    = {
        cmd_type = TYPE_E;
        cmd_form = FORM_N;
    }
    |   E SP form_code
    = {
        cmd_type = TYPE_E;
        cmd_form = $3;
    }
    |   I
    = {
        cmd_type = TYPE_I;
    }
    |   L
    = {
        cmd_type = TYPE_L;
        cmd_bytesz = NBBY;
    }
    |   L SP byte_size
    = {
        cmd_type = TYPE_L;
        cmd_bytesz = $3;
    }
    /* this is for a bug in the BBN ftp */
    |   L byte_size
    = {
        cmd_type = TYPE_L;
        cmd_bytesz = $2;
    }
    ;

struct_code:    F
    = {
        $$ = STRU_F;
    }
    |   R
    = {
        $$ = STRU_R;
    }
    |   P
    = {
        $$ = STRU_P;
    }
    ;

mode_code:  S
    = {
        $$ = MODE_S;
    }
    |   B
    = {
        $$ = MODE_B;
    }
    |   C
    = {
        $$ = MODE_C;
    }
    ;

pathname:   pathstring
    = {
        /*
         * Problem: this production is used for all pathname
         * processing, but only gives a 550 error reply.
         * This is a valid reply in some cases but not in others.
         */
        if (logged_in && $1 && strncmp($1, "~", 1) == 0) {
            char **globlist;

            globlist = ftpglob($1);
            if (globerr) {
                reply(550, globerr);
                $$ = NULL;
                if (globlist) {
                    blkfree(globlist);
                    free((char *) globlist);
                }
            }
            else if (globlist) {
                $$ = *globlist;
                blkfree(&globlist[1]);
                free((char *) globlist);
            }
            else {
                errno = ENOENT;
                perror_reply(550, $1);
                $$ = NULL;
            }
            free($1);
        } else
            $$ = $1;
    }
    ;

pathstring: STRING
    ;

octal_number:   NUMBER
    = {
        register int ret, dec, multby, digit;

        /*
         * Convert a number that was read as decimal number
         * to what it would be if it had been read as octal.
         */
        dec = $1;
        multby = 1;
        ret = 0;
        while (dec) {
            digit = dec%10;
            if (digit > 7) {
                ret = -1;
                break;
            }
            ret += digit * multby;
            multby *= 8;
            dec /= 10;
        }
        $$ = ret;
    }
    ;

check_login:    /* empty */
    = {
        if (logged_in)
            $$ = 1;
        else {
            if (log_commands) syslog(LOG_INFO, "cmd failure - not logged in");
            reply(530, MSGSTR(MSG_PLEASE_LOGIN,
                  "Please login with USER and PASS."));
            $$ = 0;
            yyerrorcalled = 1;
        }
    }
    ;

%%

extern jmp_buf errcatch;

#define CMD 0   /* beginning of command */
#define ARGS    1   /* expect miscellaneous arguments */
#define STR1    2   /* expect SP followed by STRING */
#define STR2    3   /* expect STRING */
#define OSTR    4   /* optional SP then STRING */
#define ZSTR1   5   /* SP then optional STRING */
#define ZSTR2   6   /* optional STRING after SP */
#define SITECMD 7   /* SITE command */
#define NSTR    8   /* Number followed by a string */
#define STR3    9   /* expect STRING followed by optional SP then STRING */

struct tab cmdtab[] = {     /* In order defined in RFC 765 */
    { "USER", USER, STR1, 1,    MN(MSG_CT0, "<sp> username") },
    { "PASS", PASS, ZSTR1, 1,   MN(MSG_CT1, "<sp> password") },
    { "ACCT", ACCT, STR1, 0,    MN(MSG_CT2, "(specify account)") },
    { "SMNT", SMNT, ARGS, 0,    MN(MSG_CT3, "(structure mount)") },
    { "REIN", REIN, ARGS, 0,    MN(MSG_CT4, "(reinitialize server state)") },
    { "QUIT", QUIT, ARGS, 1,    MN(MSG_CT5, "(terminate service)"), },
    { "PORT", PORT, ARGS, 1,    MN(MSG_CT6, "<sp> b0, b1, b2, b3, b4") },
    { "PASV", PASV, ARGS, 1,    MN(MSG_CT7, "(set server in passive mode)") },
    { "TYPE", TYPE, ARGS, 1,    MN(MSG_CT8, "<sp> [ A | E | I | L ]") },
    { "STRU", STRU, ARGS, 1,    MN(MSG_CT9, "(specify file structure)") },
    { "MODE", MODE, ARGS, 1,    MN(MSG_CT10, "(specify transfer mode)") },
    { "RETR", RETR, STR1, 1,    MN(MSG_CT11, "<sp> file-name") },
    { "STOR", STOR, STR1, 1,    MN(MSG_CT11, "<sp> file-name") },
    { "APPE", APPE, STR1, 1,    MN(MSG_CT11, "<sp> file-name") },
    { "MLFL", MLFL, OSTR, 0,    MN(MSG_CT12, "(mail file)") },
    { "MAIL", MAIL, OSTR, 0,    MN(MSG_CT13, "(mail to user)") },
    { "MSND", MSND, OSTR, 0,    MN(MSG_CT14, "(mail send to terminal)") },
    { "MSOM", MSOM, OSTR, 0,    MN(MSG_CT15, "(mail send to terminal or mailbox)") },
    { "MSAM", MSAM, OSTR, 0,    MN(MSG_CT16, "(mail send to terminal and mailbox)") },
    { "MRSQ", MRSQ, OSTR, 0,    MN(MSG_CT17, "(mail recipient scheme question)") },
    { "MRCP", MRCP, STR1, 0,    MN(MSG_CT18, "(mail recipient)") },
    { "ALLO", ALLO, ARGS, 1,    MN(MSG_CT19, "allocate storage (vacuously)") },
    { "REST", REST, ARGS, 1,    MN(MSG_CT20, "(restart command)") },
    { "RNFR", RNFR, STR1, 1,    MN(MSG_CT11, "<sp> file-name") },
    { "RNTO", RNTO, STR1, 1,    MN(MSG_CT11, "<sp> file-name") },
    { "ABOR", ABOR, ARGS, 1,    MN(MSG_CT21, "(abort operation)") },
    { "DELE", DELE, STR1, 1,    MN(MSG_CT11, "<sp> file-name") },
    { "CWD",  CWD,  OSTR, 1,    MN(MSG_CT22, "[ <sp> directory-name ]") },
    { "XCWD", CWD,  OSTR, 1,    MN(MSG_CT22, "[ <sp> directory-name ]") },
    { "LIST", LIST, OSTR, 1,    MN(MSG_CT23, "[ <sp> path-name ]") },
    { "NLST", NLST, OSTR, 1,    MN(MSG_CT23, "[ <sp> path-name ]") },
    { "SITE", SITE, SITECMD, 1, MN(MSG_CT24, "site-cmd [ <sp> arguments ]") },
    { "SYST", SYST, ARGS, 1,    MN(MSG_CT25, "(get type of operating system)") },
    { "STAT", STAT, OSTR, 1,    MN(MSG_CT23, "[ <sp> path-name ]") },
    { "HELP", HELP, OSTR, 1,    MN(MSG_STR, "[ <sp> <string> ]") },
    { "NOOP", NOOP, ARGS, 1,    MN(0, "") },
    { "MKD",  MKD,  STR1, 1,    MN(MSG_CT26, "<sp> path-name") },
    { "XMKD", MKD,  STR1, 1,    MN(MSG_CT26, "<sp> path-name") },
    { "RMD",  RMD,  STR1, 1,    MN(MSG_CT26, "<sp> path-name") },
    { "XRMD", RMD,  STR1, 1,    MN(MSG_CT26, "<sp> path-name") },
    { "PWD",  PWD,  ARGS, 1,    MN(MSG_CT27, "(return current directory)") },
    { "XPWD", PWD,  ARGS, 1,    MN(MSG_CT27, "(return current directory)") },
    { "CDUP", CDUP, ARGS, 1,    MN(MSG_CT28, "(change to parent directory)") },
    { "XCUP", CDUP, ARGS, 1,    MN(MSG_CT28, "(change to parent directory)") },
    { "STOU", STOU, STR1, 1,    MN(MSG_CT11, "<sp> file-name") },
    { "SIZE", SIZE, OSTR, 1,    MN(MSG_CT26, "<sp> path-name") },
    { "MDTM", MDTM, OSTR, 1,    MN(MSG_CT26, "<sp> path-name") },
    { NULL,   0,    0,    0,    MN(0, 0) }
};

struct tab sitetab[] = {
    { "UMASK", UMASK, ARGS, 1,  MN(MSG_ST0, "[ <sp> umask ]") },
    { "IDLE",  IDLE,  ARGS, 1,  MN(MSG_ST1,"[ <sp> maximum-idle-time ]") },
    { "CHMOD", CHMOD, NSTR, 1,  MN(MSG_ST2,"<sp> mode <sp> file-name") },
    { "HELP",  HELP,  OSTR, 1,  MN(MSG_STR,"[ <sp> <string> ]") },
    { "GROUP", GROUP, STR1, 1,  MN(MSG_ST3,"<sp> access-group") },
    { "GPASS", GPASS, STR1, 1,  MN(MSG_ST4,"<sp> access-password") },
    { "NEWER", NEWER, STR3, 1,  MN(MSG_ST5,"<sp> YYYYMMDDHHMMSS [ <sp> path-name ]") },
    { "MINFO", MINFO, STR3, 1,  MN(MSG_ST5,"<sp> YYYYMMDDHHMMSS [ <sp> path-name ]") },
    { "INDEX", INDEX, STR1, 1,  MN(MSG_ST6,"<sp> pattern") },
    { "EXEC",  EXEC,  STR1, 1,  MN(MSG_ST7,"<sp> command [ <sp> arguments ]") },
    { "ALIAS", ALIAS, OSTR, 1,  MN(MSG_ST8,"[ <sp> alias ] ") },
    { "CDPATH", CDPATH, OSTR, 1,MN(MSG_ST9,"[ <sp> ] ") },
    { "GROUPS", GROUPS, OSTR, 1,MN(MSG_ST9,"[ <sp> ] ") },
#ifdef	INTL
    { "LANG",  LANG,  STR1, 1,  MN(MSG_ST10,"<sp> lang-string") },
#endif
    { NULL,    0,     0,    0,  MN(0, 0) }
};

struct tab *
lookup(p, cmd)
    register struct tab *p;
    char *cmd;
{

    for (; p->name != NULL; p++)
        if (strcmp(cmd, p->name) == 0)
            return (p);
    return (0);
}

#include <arpa/telnet.h>

/*
 * getline - a hacked up version of fgets to ignore TELNET escape codes.
 */
char *
getline(s, n, iop)
    char *s;
    int n;
    register FILE *iop;
{
    register c;
    register char *cs;
    char *passtxt = "PASS password\r\n";

    cs = s;
/* tmpline may contain saved command from urgent mode interruption */
    for (c = 0; tmpline[c] != '\0' && --n > 0; ++c) {
        *cs++ = tmpline[c];
        if (tmpline[c] == '\n') {
            *cs++ = '\0';
            if (debug) {
                if (strncasecmp(passtxt, s, 5) == 0)
                    syslog(LOG_DEBUG, "command: %s", passtxt);
                else
                    syslog(LOG_DEBUG, "command: %s", s);
            }
            tmpline[0] = '\0';
            return(s);
        }
        if (c == 0)
            tmpline[0] = '\0';
    }
retry:
    while ((c = getc(iop)) != EOF) {
        c &= 0377;
        if (c == IAC) {
            if ((c = getc(iop)) != EOF) {
            c &= 0377;
            switch (c) {
            case WILL:
            case WONT:
                c = getc(iop);
                printf("%c%c%c", IAC, DONT, 0377&c);
                (void) fflush(stdout);
                continue;
            case DO:
            case DONT:
                c = getc(iop);
                printf("%c%c%c", IAC, WONT, 0377&c);
                (void) fflush(stdout);
                continue;
            case IAC:
                break;
            default:
                continue;   /* ignore command */
            }
            }
        }
        *cs++ = c;
        if (--n <= 0 || c == '\n')
            break;
    }

    if (c == EOF && cs == s) {
        if (ferror(iop) && (errno == EINTR))
            goto retry;
        return (NULL);
    }

    *cs++ = '\0';
    if (debug) {
        if (strncasecmp(passtxt, s, 5) == 0)
            syslog(LOG_DEBUG, "command: %s", passtxt);
        else
            syslog(LOG_DEBUG, "command: %s", s);
    }
    return (s);
}

static void
toolong(a)
int a; /* signal that caused this function to be called */
{
    time_t now;

    reply(421, MSGSTR(MSG_TIMEOUT,
          "Timeout (%d seconds): closing control connection."), timeout);
    (void) time(&now);
    if (logging) {
        syslog(LOG_INFO,
            "User %s timed out after %d seconds at %.24s",
            (pw ? pw -> pw_name : "unknown"), timeout, ctime(&now));
    }
    dologout(1);
}

yylex()
{
    static int cpos, state;
    register char *cp, *cp2;
    register struct tab *p;
    int n;
    char c, *copy();
    void upper();

    for (;;) {
        switch (state) {

        case CMD:
	    yyerrorcalled = 0;
            (void) signal(SIGALRM, toolong);
            (void) alarm((unsigned) timeout);
            if (is_shutdown(!logged_in, 0) != 0) {
                reply(221, MSGSTR(MSG_SERVER_SHUTTING,
                      "Server shutting down.  Goodbye."));
                dologout(0);
            }
            setproctitle(MSGSTR(MSG_IDLE, "%s: IDLE"), proctitle);
            if (getline(cbuf, sizeof(cbuf)-1, stdin) == NULL) {
                reply(221, MSGSTR(MSG_SAY_GOODBYE,
                      "You could at least say goodbye."));
                dologout(0);
            }
            (void) alarm(0);
            if ((cp = strchr(cbuf, '\r'))) {
                *cp++ = '\n';
                *cp = '\0';
            }
            if ((cp = strpbrk(cbuf, " \n")))
                cpos = cp - cbuf;
            if (cpos == 0)
                cpos = 4;
            c = cbuf[cpos];
            cbuf[cpos] = '\0';
            upper(cbuf);
            p = lookup(cmdtab, cbuf);
            cbuf[cpos] = c;
            if (strncasecmp(cbuf, "PASS", 4) != 0 &&
		strncasecmp(cbuf, "SITE GPASS", 10) != 0) {
		if (cp = strchr(cbuf, '\n'))
		    *cp = '\0';
                setproctitle("%s: %s", proctitle, cbuf);
		if (cp)
		    *cp = '\n';
	    }
            if (p != 0) {
                if (p->implemented == 0) {
                    nack(p->name);
                    longjmp(errcatch,0);
                    /* NOTREACHED */
                }
                state = p->state;
                yylval.String = p->name;
                return (p->token);
            }
            break;

        case SITECMD:
            if (cbuf[cpos] == ' ') {
                cpos++;
                return (SP);
            }
            cp = &cbuf[cpos];
            if ((cp2 = strpbrk(cp, " \n")))
                cpos = cp2 - cbuf;
            c = cbuf[cpos];
            cbuf[cpos] = '\0';
            upper(cp);
            p = lookup(sitetab, cp);
            cbuf[cpos] = c;
            if (p != 0) {
#ifndef PARANOID		/* what GOOD is SITE *, anyways?!  _H*/
                if (p->implemented == 0) {
#else
		if (1) {
		    syslog(LOG_WARNING, "refused SITE %s %s from %s@%s",
			p->name, &cbuf[cpos],
			anonymous ? guestpw : authuser, remotehost);
#endif /* PARANOID */
                    state = CMD;
                    nack(p->name);
                    longjmp(errcatch,0);
                    /* NOTREACHED */
                }
                state = p->state;
                yylval.String = p->name;
                return (p->token);
            }
            state = CMD;
            break;

        case OSTR:
            if (cbuf[cpos] == '\n') {
                state = CMD;
                return (CRLF);
            }
            /* FALLTHROUGH */

        case STR1:
        case ZSTR1:
        dostr1:
            if (cbuf[cpos] == ' ') {
                cpos++;
                state = state == OSTR ? STR2 : ++state;
                return (SP);
            }
            break;

        case ZSTR2:
            if (cbuf[cpos] == '\n') {
                state = CMD;
                return (CRLF);
            }
            /* FALLTHROUGH */

        case STR2:
            cp = &cbuf[cpos];
            n = strlen(cp);
            cpos += n - 1;
            /*
             * Make sure the string is nonempty and \n terminated.
             */
            if (n > 1 && cbuf[cpos] == '\n') {
                cbuf[cpos] = '\0';
                yylval.String = copy(cp);
                cbuf[cpos] = '\n';
                state = ARGS;
                return (STRING);
            }
            break;

        case NSTR:
            if (cbuf[cpos] == ' ') {
                cpos++;
                return (SP);
            }
            if (isdigit(cbuf[cpos])) {
                cp = &cbuf[cpos];
                while (isdigit(cbuf[++cpos]))
                    ;
                c = cbuf[cpos];
                cbuf[cpos] = '\0';
                yylval.Number = atoi(cp);
                cbuf[cpos] = c;
                state = STR1;
                return (NUMBER);
            }
            state = STR1;
            goto dostr1;

        case STR3:
            if (cbuf[cpos] == ' ') {
                cpos++;
                return (SP);
            }

            cp = &cbuf[cpos];
            cp2 = strpbrk(cp, " \n");
            if (cp2 != NULL) {
                c = *cp2;
                *cp2 = '\0';
            }
            n = strlen(cp);
            cpos += n;
            /*
             * Make sure the string is nonempty and SP terminated.
             */
            if ((cp2 - cp) > 1) {
                yylval.String = copy(cp);
                cbuf[cpos] = c;
                state = OSTR;
                return (STRING);
            }
            break;

        case ARGS:
            if (isdigit(cbuf[cpos])) {
                cp = &cbuf[cpos];
                while (isdigit(cbuf[++cpos]))
                    ;
                c = cbuf[cpos];
                cbuf[cpos] = '\0';
                yylval.Number = atoi(cp);
                cbuf[cpos] = c;
                return (NUMBER);
            }
            switch (cbuf[cpos++]) {

            case '\n':
                state = CMD;
                return (CRLF);

            case ' ':
                return (SP);

            case ',':
                return (COMMA);

            case 'A':
            case 'a':
                return (A);

            case 'B':
            case 'b':
                return (B);

            case 'C':
            case 'c':
                return (C);

            case 'E':
            case 'e':
                return (E);

            case 'F':
            case 'f':
                return (F);

            case 'I':
            case 'i':
                return (I);

            case 'L':
            case 'l':
                return (L);

            case 'N':
            case 'n':
                return (N);

            case 'P':
            case 'p':
                return (P);

            case 'R':
            case 'r':
                return (R);

            case 'S':
            case 's':
                return (S);

            case 'T':
            case 't':
                return (T);

            }
            break;

        default:
            fatal(MSGSTR(MSG_STATE_SCANNER, "Unknown state in scanner."));
        }
	if (yyerrorcalled == 0)
	  {
	    if ((cp = strchr(cbuf, '\n')) != NULL)
	      *cp = '\0';
	    reply(500, MSGSTR(MSG_NOT_UNDERSTOOD,
                  "'%s': command not understood."), cbuf);
	  }
        state = CMD;
        longjmp(errcatch,0);
    }
}

void
upper(s)
   char *s;
{
    while (*s != '\0') {
        if (islower(*s))
            *s = toupper(*s);
        s++;
    }
}

char *
copy(s)
    char *s;
{
    char *p;

    p = (char *)malloc((unsigned) strlen(s) + 1);
    if (p == NULL)
        fatal(MSGSTR(MSG_RAN_OUT_MEMORY, "Ran out of memory."));
    (void) strcpy(p, s);
    return (p);
}

help(ctab, s)
    struct tab *ctab;
    char *s;
{
    struct aclmember *entry = NULL;
    struct tab *c;
    int width, NCMDS;
    char *type;

    if (ctab == sitetab)
        type = "SITE ";
    else
        type = "";
    width = 0, NCMDS = 0;
    for (c = ctab; c->name != NULL; c++) {
        int len = strlen(c->name);

        if (len > width)
            width = len;
        NCMDS++;
    }
    width = (width + 8) &~ 7;
    if (s == 0) {
        register int i, j, w;
        int columns, lines;

	if (ctab == sitetab)
            lreply(214, MSGSTR(MSG_SITE_RECOGNIZED,
         "The following SITE commands are recognized (* =>'s unimplemented)."));
        else
            lreply(214, MSGSTR(MSG_RECOGNIZED,
              "The following commands are recognized (* =>'s unimplemented)."));
        columns = 76 / width;
        if (columns == 0)
            columns = 1;
        lines = (NCMDS + columns - 1) / columns;
        for (i = 0; i < lines; i++) {
            printf("   ");
            for (j = 0; j < columns; j++) {
                c = ctab + j * lines + i;
                printf("%s%c", c->name,
                    c->implemented ? ' ' : '*');
                if (c + lines >= &ctab[NCMDS])
                    break;
                w = strlen(c->name) + 1;
                while (w < width) {
                    putc(' ',stdout);
                    w++;
                }
            }
            printf("\r\n");
        }
        (void) fflush(stdout);
        if ( (getaclentry("email", &entry)) && ARG0 )
	  reply(214, MSGSTR(MSG_COMMENTS, "Direct comments to %s."), ARG0);
        else
	  reply(214, MSGSTR(MSG_COMMENTS_BUGS,
                "Direct comments to ftp-bugs@%s."), hostname);
        return;
    }
    upper(s);
    c = lookup(ctab, s);
    if (c == (struct tab *)NULL) {
        reply(502, MSGSTR(MSG_UNKNOWN_CMD, "Unknown command %s."), s);
        return;
    }
    if (c->implemented)
        reply(214, MSGSTR(MSG_SYNTAX, "Syntax: %s%s %s"),
              type, c->name, MNSTR(c->helpnum, c->help));
    else
        reply(214, MSGSTR(MSG_UNIMPLEMENTED,
              "%s%-*s\t%s; unimplemented."), type, width,
            c->name, MNSTR(c->helpnum, c->help));
}

sizecmd(filename)
char *filename;
{
    switch (type) {
    case TYPE_L:
    case TYPE_I: {
        struct stat stbuf;
        if (stat(filename, &stbuf) < 0 ||
            (stbuf.st_mode&S_IFMT) != S_IFREG)
            reply(550, MSGSTR(MSG_NOT_PLAIN,"%s: not a plain file."), filename);
        else
#if OFFSET_SIZE == 8
        reply(213, "%qu", stbuf.st_size);
#else
        reply(213, "%lu", stbuf.st_size);
#endif
        break;
    }
    case TYPE_A: {
        FILE *fin;
        register int c;
        register long count;
        struct stat stbuf;
        fin = fopen(filename, "r");
        if (fin == NULL) {
            perror_reply(550, filename);
            return;
        }
        if (fstat(fileno(fin), &stbuf) < 0 ||
            (stbuf.st_mode&S_IFMT) != S_IFREG) {
            reply(550, MSGSTR(MSG_NOT_PLAIN,"%s: not a plain file."), filename);
            (void) fclose(fin);
            return;
        }

        count = 0;
        while((c=getc(fin)) != EOF) {
            if (c == '\n')  /* will get expanded to \r\n */
                count++;
            count++;
        }
        (void) fclose(fin);

        reply(213, "%ld", count);
        break;}
    default:
        reply(504, MSGSTR(MSG_UNIMPL_SIZE,
              "SIZE not implemented for Type %c."), "?AEIL"[type]);
    }
}

site_exec(cmd)
char *cmd;
{
    char buf[MAXPATHLEN];
    char *sp = (char *) strchr(cmd, ' '), *slash, *t;
    FILE *cmdf, *ftpd_popen();

#ifdef PARANOID
    syslog (LOG_CRIT, "REFUSED SITE_EXEC (slipped through!!): %s", cmd);
#else

    /* sanitize the command-string */
    
    if (sp == 0)  {
        while ((slash = strchr (cmd, '/')) != 0)
            cmd = slash + 1;
    } else {
        while (sp && (slash = (char *) strchr(cmd, '/')) 
               && (slash < sp))
            cmd = slash+1;
    }
    
    for (t = cmd;  *t && !isspace(*t);  t++) {
        if (isupper(*t)) {
            *t = tolower(*t);
        }
    }

    /* build the command */
    if (strlen(_PATH_EXECPATH) + strlen(cmd) + 2 > sizeof(buf))
        return;
    sprintf(buf, "%s/%s", _PATH_EXECPATH, cmd);

    cmdf = ftpd_popen(buf, "r", 0);
    if (!cmdf) {
        perror_reply(550, cmd);
        if (log_commands)
            syslog(LOG_INFO, "SITE EXEC (FAIL: %m): %s", cmd);
    } else {
        int lines = 0;

        lreply(200, cmd);
        while (fgets(buf, sizeof buf, cmdf)) {
            int len = strlen(buf);

            if (len>0 && buf[len-1]=='\n')
                buf[--len] = '\0';
            lreply(200, buf);
            if (++lines >= 20) {
                lreply(200, MSGSTR(MSG_TRUNCATED, "*** Truncated ***"));
                break;
            }
        }
        reply(200, MSGSTR(MSG_END_OF, " (end of '%s')"), cmd);
        if (log_commands)
            syslog(LOG_INFO, "SITE EXEC (lines: %d): %s", lines, cmd);
        ftpd_pclose(cmdf);
    }
#endif /* PARANOID */
}

alias (s)
char *s;
{
    struct aclmember *entry = NULL;

    if (s != (char *)NULL) {
        while (getaclentry("alias", &entry) && ARG0 && ARG1 != NULL)
            if (!strcmp(ARG0, s)) {
                reply (214, MSGSTR(MSG_ALIAS_IS,
                       "%s is an alias for %s."), ARG0, ARG1);
                return;
            }
        reply (502, MSGSTR(MSG_UNKNOWN_ALIAS, "Unknown alias %s."), s);
        return;
    }

    lreply(214, MSGSTR(MSG_ALIASES_AVAIL,
           "The following aliases are available."));

    while (getaclentry("alias", &entry) && ARG0 && ARG1 != NULL)
        printf ("   %-8s %s\r\n", ARG0, ARG1);
    (void) fflush (stdout);

    reply(214, "");
}

cdpath ()
{
    struct aclmember *entry = NULL;

    lreply(214, MSGSTR(MSG_CDPATH_IS, "The cdpath is:"));
    while (getaclentry("cdpath", &entry) && ARG0 != NULL)
        printf ("  %s\r\n", ARG0);
    (void) fflush (stdout);
    reply(214, "");
}

print_groups()
{
    gid_t  groups[NGROUPS_MAX];
    int    ngroups = 0;

    if ( (ngroups = getgroups(NGROUPS_MAX, groups)) < 0 ) {
        return;
    }

    lreply(214, MSGSTR(MSG_GROUP_MEMBERSHIP, "Group membership is:"));
    ngroups--;

    for (; ngroups >= 0; ngroups--)
        lreply(214, "  %d", groups[ngroups]);

    (void) fflush (stdout);
    reply(214, "");
}
