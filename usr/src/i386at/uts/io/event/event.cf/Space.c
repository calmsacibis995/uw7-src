#ident	"@(#)Space.c	1.2"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/immu.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <sys/stream.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/genvid.h>
#include <sys/cmn_err.h>
#include <sys/ddi.h>
#include <sys/termios.h>
#include <sys/xque.h>
#include <sys/event.h>
#include <sys/session.h>
#include <config.h>


/*
 * evchan_dev[] and evchan_cnt are defined in master.d file 
 */

struct evchan evchan_dev[EVENT_UNITS] =  { 0 };	
int evchan_cnt = EVENT_UNITS;
 
