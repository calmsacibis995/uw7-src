/*
 *	@(#)xsrv_msgcat.h	6.1	12/14/95	15:27:45
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S000    Tue Jul 20 09:26:17 PDT 1993    davidw@sco.com
 *      - Created file.
 *	S001	Tue Apr  1 15:28:08 PST 1997	kylec@sco.com
 *	-
 *
 */

#ifndef _XSRV_MSGCAT_H
#define _XSRV_MSGCAT_H

#if defined(usl)

#include <limits.h>
#include <locale.h>
#include <nl_types.h>
#include <langinfo.h>
#include "xsrv_msg.h"

extern nl_catd xsrv_m_catd;

#ifndef NL_CAT_LOCALE
#define NL_CAT_LOCALE 0
#endif /* NL_CAT_LOCALE */

#define XSRV_M_CAT "Xsco"

#define MSGDIX(num,str) catgets(xsrv_m_catd, XSRV_DIX, (num), (str))
#define MSGOS(num,str) catgets(xsrv_m_catd, XSRV_OS, (num), (str))
#define MSGGRAF(num,str) catgets(xsrv_m_catd, XSRV_GRAF, (num), (str))
#define MSGSCO(num,str) catgets(xsrv_m_catd, XSRV_SCO, (num), (str))
#define MSGXKB(num,str) catgets(xsrv_m_catd, XSRV_XKB, (num), (str))

#define MSGSTR_SET(set,num,str) catgets(xsrv_m_catd, (set), (num), (str))

#else /* disable catalogue use */

#include "xsrv_msg.h"

#define MSGDIX(num,str)         (str)
#define MSGOS(num,str)          (str)
#define MSGGRAF(num,str)         (str)
#define MSGSCO(num,str)         (str)
#define MSGXKB(num,str)         (str)
#define MSGSTR_SET(set,num,str) (str)
#define catclose(a)             /* void */

#endif /* usl */

#endif /* _XSRV_MSGCAT_H */

