#ident "@(#)ntp_if.h	1.2"

/*
 * Sockets are not standard.
 * So hide uglyness in include file.
 */
#if defined(SYS_CONVEXOS9)
#include "/sys/sync/queue.h"
#include "/sys/sync/sema.h"
#endif

#if defined(SYS_AIX)
#include <sys/time.h>
#include <time.h>
#endif

#if (defined(SYS_SOLARIS) && !defined(bsd)) || defined(SYS_SUNOS4)
#include <sys/sockio.h>
#endif

#if defined(SYS_UNIXWARE2)
#include <sys/sockio.h>
#endif

#if defined(SYS_PTX) || defined(SYS_SINIXM)
#include <sys/stream.h>
#include <sys/stropts.h>
#endif

#if defined(SYS_SVR4)
#if !defined(USE_STREAMS_DEVICE_FOR_IF_CONFIG)
#include <sys/sockio.h>
#else /* USE_STREAMS_DEVICE_FOR_IF_CONFIG */
#include <netinet/ip.h>
#undef SIOCGIFCONF
#undef SIOCGIFFLAGS
#undef SIOCGIFADDR
#undef SIOCGIFBRDADDR
#undef SIOCGIFNETMASK
#define SIOCGIFCONF	IPIOC_GETIFCONF
#define SIOCGIFFLAGS	IPIOC_GETIFFLAGS
#define SIOCGIFADDR	IPIOC_GETIFADDR
#define SIOCGIFBRDADDR IPIOC_GETIFBRDADDR
#define SIOCGIFNETMASK IPIOC_GETIFNETMASK
#endif /* USE_STREAMS_DEVICE_FOR_IF_CONFIG */

#endif /* SYS_SVR4 */

#ifndef SYS_WINNT
#include <net/if.h>
#endif /* SYS_WINNT */
