#ident	"@(#)pathnames.h	1.4"

/* Copyright (c) 1989 The Regents of the University of California. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)$Id$ based on
 * pathnames.h 5.2 (Berkeley) 6/1/90 
 */

#define _PATH_EXECPATH  "/bin/ftp-exec"

#ifdef USE_ETC
#define _PATH_FTPUSERS  "/etc/ftpusers"
#define _PATH_FTPACCESS "/etc/ftpaccess"
#define _PATH_CVT       "/etc/ftpconversions"
#define _PATH_PRIVATE   "/etc/ftpgroups"
#else
#ifdef USE_ETC_FTPD
#define _PATH_FTPUSERS  "/etc/ftpd/ftpusers"
#define _PATH_FTPACCESS "/etc/ftpd/ftpaccess"
#define _PATH_CVT       "/etc/ftpd/ftpconversions"
#define _PATH_PRIVATE   "/etc/ftpd/ftpgroups"
#else
#define _PATH_FTPUSERS  "/usr/local/lib/ftpd/ftpusers"
#define _PATH_FTPACCESS "/usr/local/lib/ftpd/ftpaccess"
#define _PATH_CVT       "/usr/local/lib/ftpd/ftpconversions"
#define _PATH_PRIVATE   "/usr/local/lib/ftpd/ftpgroups"
#endif
#endif

#ifdef USE_VAR
#ifdef USE_PID
#define _PATH_PIDNAMES  "/var/pid/ftp.pids-%s"
#else
#ifdef VAR_RUN
#define _PATH_PIDNAMES  "/var/run/ftp.pids-%s"
#else
#define _PATH_PIDNAMES  "/var/adm/ftp.pids-%s"
#endif
#endif
#ifdef USE_LOG
#define _PATH_XFERLOG   "/var/log/xferlog"
#else
#define _PATH_XFERLOG   "/var/adm/xferlog"
#endif
#else
#ifndef _PATH_PIDNAMES
#define _PATH_PIDNAMES  "/usr/local/lib/ftpd/pids/%s"
#endif
#ifndef _PATH_XFERLOG
#define _PATH_XFERLOG   "/usr/local/logs/xferlog"
#endif
#endif

#ifdef PATH_UTMP
#define _PATH_UTMP PATH_UTMP
#endif

#ifdef PATH_WTMP
#define _PATH_WTMP PATH_WTMP
#endif

#ifndef _PATH_UTMP
#define _PATH_UTMP      "/etc/utmp"
#endif
#ifndef _PATH_WTMP
#define _PATH_WTMP      "/usr/adm/wtmp"
#endif
#ifndef _PATH_LASTLOG
#define _PATH_LASTLOG   "/usr/adm/lastlog"
#endif

#ifndef _PATH_BSHELL
#define _PATH_BSHELL    "/bin/sh"
#endif

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL   "/dev/null"
#endif

#ifdef  HOST_ACCESS
#ifdef USE_ETC
#define _PATH_FTPHOSTS  "/etc/ftphosts"
#else
#ifdef USE_ETC_FTPD
#define _PATH_FTPHOSTS  "/etc/ftpd/ftphosts"
#else
#define _PATH_FTPHOSTS  "/usr/local/lib/ftpd/ftphosts"
#endif
#endif
#endif

