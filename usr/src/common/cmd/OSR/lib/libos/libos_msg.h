#ident	"@(#)OSRcmds:lib/libos/libos_msg.h	1.1"
#ifndef _H_LIBOS_MSG
#define _H_LIBOS_MSG
#include <limits.h>
#include <nl_types.h>
#define MF_LIBOS "libos.cat@Unix"
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif
#ifndef MSGSTR
extern char *catgets_safe(nl_catd, int, int, char *);
#ifdef lint
#define MSGSTR(num,str) (str)
#define MSGSTR_SET(set,num,str) (str)
#else
#define MSGSTR(num,str) catgets_safe(catd, MS_LIBOS, (num), (str))
#define MSGSTR_SET(set,num,str) catgets_safe(catd, (set), (num), (str))
#endif
#endif
/* The following was generated from */
/* NLS/en/libos.gen */
 
 
#define MS_LIBOS 1
#define MSG_BADVECTOR 1
#define MSG_NOPWENT 2
#define MSG_NOMEM 3
#define MSG_EACCESS 4
#define MSG_READERR 5
#define MSG_LINETOOLONG 6
#define MSG_DUPGROUP 7
#define MSG_SGL_FULL 8
#define MSG_INVAL_GROUP 9
#define MSG_UNOTMEMBER 10
#define MSG_SYSCONF 11
#define MSG_MALLOC 12
#define MSG_SETGRPS 13
#define MSG_WARNING 14
#define MSG_ERROR 15
#define MSG_ERRORL 16
#endif /* _H_LIBOS_MSG */
