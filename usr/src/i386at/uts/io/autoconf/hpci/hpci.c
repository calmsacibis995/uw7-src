#ident	"@(#)kern-i386at:io/autoconf/hpci/hpci.c	1.2.2.3"
#ident	"$Header$"

#define _DDI_C

#ifndef _KERNEL_HEADERS

#include <sys/types.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/psm.h>
#include <sys/ipl.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/page.h>
#include <sys/seg.h>
#include <sys/immu.h>
#include <sys/user.h>
#include <sys/systm.h>          /* for Hz */
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/hpci.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <sys/fcntl.h>
#include <sys/moddefs.h>
#include <sys/user.h>
#include <sys/cmn_err.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/ca.h>
#include <sys/pci.h>
#include <sys/ddi.h>
#include <sys/cred.h>
#include <sys/privilege.h>
#include <sys/f_ddi.h>

#else

#include "util/types.h"
#include "util/param.h"
#include "svc/psm.h"
#include "util/types.h"
#include "util/ipl.h"
#include "proc/signal.h"
#include "svc/errno.h"
#include "io/conf.h"
#include "mem/page.h"
#include "proc/seg.h"
#include "mem/immu.h"
#include "proc/user.h"
#include "svc/systm.h"		/* for Hz */
#include "fs/file.h"
#include "io/poll.h"
#include "proc/proc.h"
#include "mem/kmem.h"
#include "fs/fcntl.h"
#include "util/mod/moddefs.h"
#include "io/autoconf/hpci/hpci.h"
#include "proc/user.h"
#include "util/cmn_err.h"
#include "io/autoconf/resmgr/resmgr.h"
#include "io/autoconf/confmgr/confmgr.h"
#include "io/autoconf/confmgr/cm_i386at.h"
#include "io/autoconf/ca/ca.h"
#include "io/autoconf/ca/pci/pci.h"
#include "io/ddi.h"
#include "proc/cred.h"
#include "acc/priv/privilege.h"
#include "io/f_ddi.h"

#endif

#define DRVNAME "hpci_ - Hot Socket Controller Interface driver."

MOD_DRV_WRAPPER(hpci_, NULL, NULL, NULL, DRVNAME);


#define	MAX_HPCD_CNT	64

#define HPCIOPEN        1
#define HPCICLOSE       0

STATIC  	int	hpci_flags = HPCICLOSE;
STATIC  	sv_t	*hpci_svp = NULL;
STATIC		lock_t  hpci_lock;
LKINFO_DECL(hpci_lkinfo, "hpci:user interface lock", LK_SLEEP);

STATIC	 	struct	pollhead *hpci_php = NULL;
STATIC		int	hpci_max = -1;
STATIC		int	hpci_cnt = 0;

#define HPCI_MUTEX_HIER        2
#define HPCI_SYNC_HIER         1
#define HPCIPL          plbase

STATIC	 lock_t *hpci_ddi_notify_mutex;            /* hpci_ddi_notify mutex lock */
STATIC	 lock_t *hpci_postevent_mutex;            /* hpci_postevent mutex lock */

LKINFO_DECL(hpci_ddi_notify_mutex_lkinfo, "HPCI:ddi_notify mutex lock", 0);
LKINFO_DECL(hpci_postevent_mutex_lkinfo, "HPCI:hpci postevent mutex lock", 0);


STATIC struct 	{
	hpcd_info_t   *hpcd_iptr;
} hpcdList[MAX_HPCD_CNT];

extern  int  hpci_evcnt;

int 	registerPciFunctions(socket_info_t *sockInfo);
int 	unregisterPciFunctions(socket_info_t *sockInfo);


/* 
 * The following data structure defines the event pool.
 * Allocated and maintained by the hpci driver only.
 */
typedef struct {
	ushort	 	evp_rd_mark;
	ushort	 	evp_wr_mark;
	hpci_event_t      *evp_events;
} hpci_event_pool_t;

/*
 * The following data structure defines the suspended driver list.
 * Allocated and maintained by the hpci driver only.
 */
typedef struct _drv_suspend {
	struct __conf_cb 	conf_cb;
	struct _drv_suspend    	*next;
	struct _drv_suspend    	*prev;
	hpci_suspend_cmd_t   	info;
	int	ddicmd;
} hpci_drvsuspend_list_t;

STATIC hpci_event_pool_t	hpci_evpool = { 0 };
STATIC hpci_drvsuspend_list_t *hpci_suspend_list = NULL;
STATIC int			   hpci_suspend_count = 0;
STATIC int			   hpci_ui_started = 0;

int hpci_devflag = (D_INITPUB | D_NOSPECMACDATA | D_MP);  /* init of sec flags */
/* public device, no MAC    */
/* checks on data transfer  */

int 	hpci_open(dev_t *, int, int, struct cred *);
int 	hpci_close(dev_t, int, int, struct cred *);
int 	hpci_read(dev_t, struct uio *, struct cred *);
int 	hpci_write(dev_t, struct uio *, struct cred *);
int 	hpci_ioctl(dev_t, int, void *, int, struct cred *, int *);
int 	hpci_chpoll(dev_t, short, int, short *, struct pollhead **);

STATIC int 	hpci_execute_driver_cmd( void *arg, int cmd);
STATIC int 	hpci_suspend_driver_cmd( void *arg);
STATIC int 	hpci_get_hpcs_info(void *arg, int cmd);
STATIC void     hpci_registerFunctions(hpcd_info_t *hpc, int hpcid);
STATIC int 	hpci_get_socket_info(void *arg);
STATIC int 	hpci_get_bus_info(void *arg);
STATIC int 	hpci_get_function_info(void *arg);
STATIC int 	hpci_get_driver_info(void *arg, int cmd);
STATIC int 	hpci_get_event_info(void *arg, int cmd);
STATIC int 	hpci_change_socket_state(void *arg);

void
hpci_init()
{

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "hpci_init is called");
#endif
}

/*
extern	void hpcd_pci_drv_init();
*/
void
hpci_start()
{
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "hpci_start is called");
#endif
	hpci_evpool.evp_events = (hpci_event_t *)
	    kmem_zalloc(hpci_evcnt*sizeof(hpci_event_t), KM_SLEEP);

	hpci_ddi_notify_mutex = LOCK_ALLOC(HPCI_MUTEX_HIER, HPCIPL,
	    &hpci_ddi_notify_mutex_lkinfo, KM_NOSLEEP);
	hpci_postevent_mutex = LOCK_ALLOC(HPCI_MUTEX_HIER, HPCIPL,
	    &hpci_postevent_mutex_lkinfo, KM_NOSLEEP);
	if(hpci_ddi_notify_mutex == NULL ||
	    hpci_postevent_mutex == NULL)
		cmn_err(CE_PANIC,
		    "hmpci_start: out of memory for hpci locks");
	/* is PLHI the right thing here ? */
	LOCK_INIT(&hpci_lock, HPCI_SYNC_HIER, PLHI, &hpci_lkinfo, KM_SLEEP);

/*
	hpcd_pci_drv_init();
*/
}

int
hpci_open(dev_t *devp, int flag, int otyp, struct cred *cr)
{
	int	dev = getminor(*devp);
	if (dev != 0)
		return ENXIO;
	if (hpci_flags & HPCIOPEN ) {
		return EBUSY;
	}

	hpci_flags =  HPCIOPEN;

	if (hpci_php == NULL)
		hpci_php =
		    (struct pollhead *)kmem_zalloc(sizeof(struct pollhead),
		    KM_SLEEP);
	if ((flag & FNDELAY) != FNDELAY)
	{
		hpci_svp = SV_ALLOC(KM_SLEEP);
		if(hpci_svp == NULL)
		{
			kmem_free(hpci_php, sizeof(struct pollhead));
			return ENOMEM;
		}
		SV_INIT(hpci_svp);
	}
	hpci_evpool.evp_wr_mark = hpci_evpool.evp_rd_mark = 0;
	hpci_ui_started = 1;
	return 0;
}


int
hpci_close(dev_t dev, int flag, int otyp, struct cred *cr)
{
	dev = getminor(dev);
	if (dev != 0)
		return ENXIO;

	hpci_flags = HPCICLOSE;
	hpci_evpool.evp_wr_mark = hpci_evpool.evp_rd_mark = 0;

	kmem_free((_VOID *)hpci_php, sizeof(struct pollhead));
	hpci_php = NULL;

	if(hpci_svp == NULL)
	{
		SV_DEALLOC(hpci_svp);
		hpci_svp = NULL;
	}

	return 0;
}


int
hpci_chpoll(dev_t dev, short events, int anyyet, short *reventsp, struct pollhead **phpp)
{
	dev = getminor(dev);
	if (dev != 0)
		return ENXIO;
	if(hpci_evpool.evp_wr_mark == hpci_evpool.evp_rd_mark)
		*reventsp = 0;
	else	*reventsp |= POLLIN;

	if (*reventsp == 0 && !anyyet)
	{
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, "let select block on %x", hpci_php);
#endif
		*phpp = hpci_php;
	}

	return 0;
}

int
hpci_read(dev_t dev, struct uio *uio_p, struct cred *cred_p)
{
	int  len = uio_p->uio_iovcnt;
	int  error = EAGAIN;
	int  nevents;

	dev = getminor(dev);
	if (dev != 0)
		return(ENXIO);

	len = len / sizeof(socket_info_t);
	if(len <= 0)
		return (0);

	while(len > 0 && hpci_evpool.evp_wr_mark != hpci_evpool.evp_rd_mark)
	{
		error = 0;
		if(hpci_evpool.evp_wr_mark < hpci_evpool.evp_rd_mark)
			nevents = hpci_evcnt - hpci_evpool.evp_rd_mark + hpci_evpool.evp_wr_mark;
		else	nevents = hpci_evpool.evp_wr_mark - hpci_evpool.evp_rd_mark;

		if(len < nevents)
			nevents = len;

		if (uiomove((char *)
		    &hpci_evpool.evp_events[hpci_evpool.evp_rd_mark], 
		    nevents * sizeof(socket_info_t), 
		    UIO_READ, uio_p) == -1)
		{
			return EFAULT;
		}
		hpci_evpool.evp_rd_mark = 
		    (hpci_evpool.evp_rd_mark + nevents) % hpci_evcnt;
		len -= nevents;
	}

	return(error);
}

int
hpci_write(dev_t dev, struct uio *uio_p, struct cred *cred_p)
{
	return 0;
}

int
hpci_get_drv_cap(void *arg)
{
	int 	error;
	hpci_drv_cap_t cap;
	extern 	int ddi_getdrv_cap(char *drvname, ulong_t *drvcap);

	if (copyin((char *) arg, (char *) &cap, sizeof(cap)) == -1)
		return EFAULT;
	error = ddi_getdrv_cap(cap.drv_name, &cap.drv_cap);
	if(error)
		return error;
	if(copyout((char *) &cap, (char *) arg, sizeof(cap)) == -1)
		error = EFAULT;
	return error;
}

int
hpci_ioctl(dev_t dev, int cmd, void *arg, int mode, struct cred *cr, int *rvalp)
{
	int error = 0;

	if(getminor(dev) != 0)
		return(ENXIO);

	switch (cmd)
	{

	case HPCI_GET_HPCS_COUNT :
		if(copyout((char *) &hpci_cnt, (char *) arg, sizeof(int)) == -1)
			error = EFAULT;
		break;
	case HPCI_GET_HPCS_INFO :
		error = hpci_get_hpcs_info(arg, cmd);
		break;

	case HPCI_GET_BUS_INFO:
		error = hpci_get_bus_info(arg);
		break;
	case HPCI_GET_SOCKET_INFO :
		error = hpci_get_socket_info(arg);
		break;

	case HPCI_GET_FUNCTION_INFO:
		error = hpci_get_function_info(arg);
		break;

	case HPCI_SET_SOCKET_STATE :
		error = hpci_change_socket_state(arg);
		break;

	case HPCI_BIND_INSTANCE:
/*
	case HPCI_MODIFY_INSTANCE:
*/
	case HPCI_UNBIND_INSTANCE:
		error = hpci_execute_driver_cmd(arg, cmd);
		break;

	case HPCI_RESUME_INSTANCE:
		error = hpci_resume_driver_cmd(arg);
		break;

	case HPCI_SUSPEND_INSTANCE:
		error = hpci_suspend_driver_cmd(arg);
		break;

	case HPCI_GET_SUSPEND_COUNT:
	case HPCI_GET_SUSPEND_INFO:
		error = hpci_get_driver_info(arg, cmd);
		break;

	case HPCI_GET_EVENT_COUNT:
	case HPCI_GET_EVENT_INFO:
		error = hpci_get_event_info(arg, cmd);
		break;
	case HPCI_GET_DRV_CAP: 
		error = hpci_get_drv_cap(arg);
		break;

	default:
		error = EINVAL;
#ifdef HPCI_DEBUG
		cmn_err(CE_WARN, "hpci_ioctl:  illegal cmd 0x%x, dex 0x%x ", 
		    cmd, dev);
#endif
	}
	return(error);
}
hpci_drvsuspend_list_t *
getDriverSuspendEntry(int	drv_suspend_id)
{
	hpci_drvsuspend_list_t *ptr; 
	pl_t opl;

	opl = LOCK(hpci_ddi_notify_mutex, HPCIPL);
	ptr = hpci_suspend_list;
	while(ptr != NULL)
	{
		if(ptr->info.drv_suspend_id == drv_suspend_id)
		{
			break;
		}
		ptr = ptr->next;
	}
	UNLOCK(hpci_ddi_notify_mutex, opl);
	return ptr;	
}

int
hpci_check_if_suspended(rm_key_t rmkey)
{
	hpci_drvsuspend_list_t *ptr; 
	pl_t opl;
	int  retval = 0;

	opl = LOCK(hpci_ddi_notify_mutex, HPCIPL);
	ptr = hpci_suspend_list;
	while(ptr != NULL)
	{
		if(ptr->info.drv_rmkey == rmkey)
		{
			retval = 1;
			break;
		}
		ptr = ptr->next;
	}
	UNLOCK(hpci_ddi_notify_mutex, opl);
	return retval;	
}

void
hpci_unlink_list(hpci_drvsuspend_list_t *list)
{
	pl_t opl;
	opl = LOCK(hpci_ddi_notify_mutex, HPCIPL);

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "removing driver %s from the list", 
						list->info.drv_name);
#endif
	if(list == hpci_suspend_list)
	{
		if(list->next == NULL)
			hpci_suspend_list = NULL;
		else	{
			hpci_suspend_list = list->next;
			hpci_suspend_list->prev = NULL;
		}
	} else	{
		if(list->next == NULL)
			list->next->prev = NULL;
		else {
			list->next->prev = list->prev;
			list->prev->next = list->next;
		}
	}
	hpci_suspend_count--;

	UNLOCK(hpci_ddi_notify_mutex, opl);
}

void
hpci_link_list(hpci_drvsuspend_list_t *list)
{
	pl_t opl;
	opl = LOCK(hpci_ddi_notify_mutex, HPCIPL);

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "adding driver %s to the list", list->info.drv_name);
#endif
	if(hpci_suspend_list == NULL)
	{
		list->next = list->prev = NULL;
	} else	{
		list->next = hpci_suspend_list;
		hpci_suspend_list->prev = list;
		list->prev = NULL;
	}
	hpci_suspend_list = list;
	hpci_suspend_count++;
	UNLOCK(hpci_ddi_notify_mutex, opl);
}

void
hpci_notify_ddi_callback(hpci_drvsuspend_list_t *list)
{
	hpci_event_t ev;

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "hpci_notify_ddi_callback:back from cmd %x rmkey 0x%x",
		list->conf_cb.cb_type,
		 list->info.drv_rmkey);
#endif

	ev.status 	= list->conf_cb.cb_ret;
	ev.eventType 	= list->conf_cb.cb_type;
	ev.ormkey 	= list->info.drv_rmkey;
	strncpy(ev.drv_name, list->info.drv_name, sizeof(ev.drv_name) -1);

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "hpci_notify_ddi_callback: sending event for remkey 0x%x ", 
				list->info.drv_rmkey);
#endif
	hpci_postevent(-1, (void *) &ev, list->ddicmd);

	if(list->conf_cb.cb_ret)
	{
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, "call back from %x FAILED", 
					list->conf_cb.cb_type); 
#endif
	} else  {

		switch(list->conf_cb.cb_type)
		{
			case CFG_MODIFY:
#ifdef HPCI_DEBUG
				cmn_err(CE_NOTE, "get call back from CFG_MODIFY"); 
#endif
				hpci_notify_mod(CFG_RESUME, list->info.drv_rmkey, 
						list, list->info.drv_name);
			break;
			case CFG_RESUME:
#ifdef HPCI_DEBUG
				cmn_err(CE_NOTE, "get call back from CFG_RESUME"); 
#endif
				hpci_unlink_list(list);
				kmem_free(list, sizeof(hpci_drvsuspend_list_t));
			break;
			case CFG_SUSPEND:
#ifdef HPCI_DEBUG
				cmn_err(CE_NOTE, "get call back from CFG_SUSPEND"); 
#endif
				list->info.status = SUSPEND_DONE;
			break;
			default:
#ifdef HPCI_DEBUG
				cmn_err(CE_NOTE, "hpci_notify_ddi_callback: get call back from CFG_UNKNOWN %x",
				list->conf_cb.cb_type)
#endif
			break;
		}
	}
	SV_SIGNAL(hpci_svp, 0);
}

int
hpci_notify_mod(int ddicmd, rm_key_t rmkey, hpci_drvsuspend_list_t *list, char *drv_name)
{

#ifdef HPCI_DEBUG
	char *ptr = drv_name;

	if(ptr == NULL)
		ptr = "NULL";

	cmn_err(CE_NOTE, "hpci_notify_ddi: cmd 0x%x, rmkey 0x%x, drv_name %s",
                ddicmd , rmkey, drv_name);
#endif
	if((ddicmd == CFG_REMOVE || ddicmd == CFG_ADD) && hpci_check_if_suspended(rmkey))
		return 0;
	if(list)
	{
		list->conf_cb.cb_func = (void (*)(void *)) hpci_notify_ddi_callback;
		list->conf_cb.cb_type = 0;
		list->ddicmd = EVENT_DRVCMD_STATUS;
		ddi_notify_mod(ddicmd, rmkey, list->conf_cb.cb_cfgp, drv_name, &list->conf_cb);
	} else ddi_notify_mod(ddicmd, rmkey, NULL, drv_name, NULL);
	return EINPROGRESS;
}

socket_info_t *
LookupSocket(socket_info_t *socket)
{
	hpcd_bus_t	*busList;
	hpcd_socket_t     *socket_list;
	int	      i, busIdx;
	pl_t 	opl;

	if(socket->hpcid <= -1 || socket->hpcid > hpci_max ||
	    hpcdList[socket->hpcid].hpcd_iptr == NULL)
		return NULL;

	opl = (*hpcdList[socket->hpcid].hpcd_iptr->lock_callback)
					(hpcdList[socket->hpcid].hpcd_iptr);
	busList = hpcdList[socket->hpcid].hpcd_iptr->bus_list;
	for(busIdx = 0; busIdx < hpcdList[socket->hpcid].hpcd_iptr->buscnt; busIdx++)
	{
/*
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, "Comparing busid %d, type %d with socket bud id %d, type%d",
				busList->busid, busList->bustype, socket->busid, socket->bustype);
#endif
*/
		if(busList->busid == socket->busid && socket->bustype == busList->bustype)
		{
			socket_list = busList->socket_list;
			for(i=0; i< busList->socket_cnt ; i++, socket_list = socket_list->next)
			{
				switch(busList->bustype)
				{
				case CM_BUS_PCI:
/*
#ifdef HPCI_DEBUG
					cmn_err(CE_NOTE, 
					   "Comparing slotnum %d socket no %d with socket slotnum %d",
						socket_list->info.pci_socket.slotnum,
						i, socket->pci_socket.slotnum);
#endif
*/
					if(socket_list->info.pci_socket.slotnum == socket->pci_socket.slotnum)
					{
						(*hpcdList[socket->hpcid].hpcd_iptr->unlock_callback)
                                        		(hpcdList[socket->hpcid].hpcd_iptr, opl);

						return &socket_list->info;
					}
					break;
				default: /* add code to recognize other bus type sockets, i.e. TBDBUS */
					(*hpcdList[socket->hpcid].hpcd_iptr->unlock_callback)
                                        		(hpcdList[socket->hpcid].hpcd_iptr, opl);
					return NULL;
				}
			}
		}
		busList = busList->next;
	}
	(*hpcdList[socket->hpcid].hpcd_iptr->unlock_callback)
			(hpcdList[socket->hpcid].hpcd_iptr, opl);
	return NULL;
}

int
hpci_execute_driver_cmd( void *arg, int cmd)
{
	int	error = 0;
	hpci_drvcmd_t drvcmd;
	int	ddicmd = -1;
	cm_args_t cma;
	char	  cmabuf[CM_MODNAME_MAX];

	if (copyin((char *) arg, (char *) &drvcmd, sizeof(drvcmd)) == -1)
		return EFAULT;

	cma.cm_key = drvcmd.old_rmkey;
	cma.cm_param = CM_MODNAME;
        cma.cm_val = cmabuf;
	cma.cm_vallen = sizeof(cmabuf);
        cma.cm_n = 0;

	switch(cmd)
	{

	case HPCI_BIND_INSTANCE:
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, "Binding driver %s to resmgr key %d",
			drvcmd.drv_name,
			drvcmd.old_rmkey);
#endif
		if(cm_getval(&cma) == 0)
			return EEXIST;
		cma.cm_val = drvcmd.drv_name;
       		cma.cm_vallen = sizeof(drvcmd.drv_name);
        	cma.cm_n = 0;
		cm_begin_trans(cma.cm_key, RM_RDWR);
        	error = cm_addval(&cma);
	        cm_end_trans(cma.cm_key);
		if(error != 0)
		{
#ifdef HPCI_DEBUG
			cmn_err(CE_NOTE, "Binding failed: error %d", error);
#endif
                        return EINVAL;
		}
		return 0;
	case HPCI_UNBIND_INSTANCE:
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, "Unbinding driver %s from resmgr key %d",
			drvcmd.drv_name,
			drvcmd.old_rmkey);
#endif
		cma.cm_val = drvcmd.drv_name;
       		cma.cm_vallen = sizeof(drvcmd.drv_name);
        	cma.cm_n = 0;
		cm_begin_trans(cma.cm_key, RM_RDWR);
        	error = cm_delval(&cma);
	        cm_end_trans(cma.cm_key);
		if(error != 0)
		{
#ifdef HPCI_DEBUG
			cmn_err(CE_NOTE, "Unbinding failed: error %d", error);
#endif
			return EINVAL;
		}
		return 0;
/*
	case HPCI_MODIFY_INSTANCE:
		ddicmd = CFG_MODIFY;
*/
	default:
		return EINVAL;
	}
}

int
hpci_resume_driver_cmd(void *arg)
{
	int	error = 0;
	hpci_resume_cmd_t drvcmd;
	cm_args_t cma;
	char	  cmabuf[CM_MODNAME_MAX];
	hpci_drvsuspend_list_t *list = NULL;

	if (copyin((char *) arg, (char *) &drvcmd, sizeof(drvcmd)) == -1)
		return EFAULT;
	list = getDriverSuspendEntry(drvcmd.drv_suspend_id);
	if(list == NULL)
		return EINVAL;
	if(list->info.status == SUSPEND_IN_PROGRESS)
		return EBUSY;

	cma.cm_key = drvcmd.new_rmkey;
	cma.cm_param = CM_MODNAME;
        cma.cm_val = cmabuf;
	cma.cm_vallen = sizeof(cmabuf);
        cma.cm_n = 0;

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "Before resuming: Binding driver %s to resmgr key %d",
		list->info.drv_name,
		drvcmd.new_rmkey);
#endif
	cm_begin_trans(cma.cm_key, RM_READ);
	if(cm_getval(&cma) == 0){
		cm_end_trans(cma.cm_key);
		return EEXIST;
	}
	cm_end_trans(cma.cm_key);
	
	list->info.drv_rmkey = drvcmd.new_rmkey;

	cma.cm_key = drvcmd.new_rmkey;
	cma.cm_val = list->info.drv_name;
       	cma.cm_vallen = sizeof(list->info.drv_name);
        cma.cm_n = 0;
	cm_begin_trans(cma.cm_key, RM_RDWR);
        error = cm_addval(&cma);
	cm_end_trans(cma.cm_key);
	if(error != 0)
	{
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, "Before resuming:Binding failed: error %d", 
				error);
#endif
		return EINVAL;
	}
	if(hpci_svp)
		SV_INIT(hpci_svp);
	list->conf_cb.cb_ret = EINTR;
	error = hpci_notify_mod(CFG_MODIFY, drvcmd.new_rmkey, list, list->info.drv_name);
	if(!hpci_svp || error != EINPROGRESS)
		return error;
	LOCK(&hpci_lock, PLHI);
	if(SV_WAIT_SIG(hpci_svp, PRIMED, &hpci_lock) == B_FALSE)
		return EINTR;
	return list->conf_cb.cb_ret;
}

int
hpci_suspend_driver_cmd( void *arg)
{
	hpci_drvsuspend_list_t *list;
	hpci_suspend_cmd_t    info;
	int	error = 0;
	cm_args_t cma;
	static int next_suspend_id = 100;

	if (copyin((char *) arg, (char *) &info, sizeof(info)) == -1)
		return EFAULT;
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "SUSPENDING driver %s from resmgr key %d",
			info.drv_name,
			info.drv_rmkey);
#endif
/*
	if(LookupSocket(&info.drv_socket) == NULL)
		return EINVAL;
*/
	list = (hpci_drvsuspend_list_t *)
	    kmem_zalloc(sizeof(hpci_drvsuspend_list_t), KM_SLEEP);
	list->info = info;
	list->info.status = SUSPEND_IN_PROGRESS;
	list->info.drv_suspend_id = next_suspend_id++;

	hpci_link_list(list);

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "Unbinding driver %s from resmgr key %d",
			info.drv_name,
			info.drv_rmkey);
#endif

	cma.cm_key = info.drv_rmkey;
	cma.cm_param = CM_MODNAME;
	cma.cm_val = info.drv_name;
       	cma.cm_vallen = sizeof(info.drv_name);
       	cma.cm_n = 0;
	cm_begin_trans(cma.cm_key, RM_RDWR);
       	error = cm_delval(&cma);
        cm_end_trans(cma.cm_key);

	if(error != 0)
	{
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, 
			"hpci_suspend_driver_cmd: Unbinding failed: error %d", 
			error);
#endif
		return error;
	}
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "Finally suspending driver %s from resmgr key %d",
			info.drv_name,
			info.drv_rmkey);
#endif
	if(hpci_svp)
		SV_INIT(hpci_svp);
	list->conf_cb.cb_ret = EINTR;
	error = hpci_notify_mod(CFG_SUSPEND, info.drv_rmkey, list, info.drv_name);
	if(!hpci_svp || error != EINPROGRESS)
		return error;
	LOCK(&hpci_lock, PLHI);
	if(SV_WAIT_SIG(hpci_svp, PRIMED, &hpci_lock) == B_FALSE)
		return EINTR;
	return list->conf_cb.cb_ret;
}

int
hpci_attach(hpcd_info_t *hpc)
{
	int hpcid;

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "calling hpci_register\n");
#endif
	for(hpcid=0; hpcid<MAX_HPCD_CNT; hpcid++)
	{
		if(hpcdList[hpcid].hpcd_iptr == (hpcd_info_t *) NULL)
		{
			hpcdList[hpcid].hpcd_iptr = hpc;
			if(hpcid > hpci_max)
				hpci_max = hpcid;
			hpci_cnt++;
			hpci_registerFunctions(hpc, hpcid);
#ifdef HPCI_DEBUG
			cmn_err(CE_NOTE, "hpci_register new id %d\n", hpcid);
#endif
			return hpcid;
		}
	}
	return -1;
}

int
hpci_detach(int  hpcid)
{

	if(hpcid <= -1 || hpcid > hpci_max ||
            hpcdList[hpcid].hpcd_iptr == NULL)
                return -1;
	hpcdList[hpcid].hpcd_iptr = (hpcd_info_t *) NULL;
	hpci_cnt--;
	if(hpcid == hpci_max)
		hpci_max = hpcid - 1;
	return 0;
}

void
hpci_emitevent()
{
	int hpcid;
	hpcd_bus_t	*busList;
	hpcd_socket_t     *socket_list;
	socket_info_t      *sockInfo;

	for(hpcid=0; hpcid<MAX_HPCD_CNT; hpcid++)
	{
		if(hpcdList[hpcid].hpcd_iptr)
		{
			busList = hpcdList[hpcid].hpcd_iptr->bus_list;
			socket_list = busList->socket_list;
			sockInfo = &socket_list->info;
			sockInfo->prev_state = sockInfo->current_state;
			if(sockInfo->current_state & (SOCKET_POWER_ON|SOCKET_PCI_POWER_FAULT))
				sockInfo->current_state &= ~(SOCKET_POWER_ON|SOCKET_PCI_POWER_FAULT);
			else	sockInfo->current_state |= (SOCKET_POWER_ON|SOCKET_PCI_POWER_FAULT);
			hpci_postevent(hpcid, sockInfo, EVENT_STATE_CHANGE);
		}
	}

}

int
hpci_postevent(int hpcid, socket_info_t *evptr, int evtype)
{
	pl_t opl;

	if(hpci_php == NULL || hpci_evpool.evp_events == (hpci_event_t *) NULL)
		return -1;

	opl = LOCK(hpci_postevent_mutex, HPCIPL);
	switch(evtype)
	{
	case EVENT_DRVCMD_STATUS:
		hpci_evpool.evp_events[hpci_evpool.evp_wr_mark] = *((hpci_event_t *) evptr);
		hpci_evpool.evp_events[hpci_evpool.evp_wr_mark].socket.hpcid = -1;
		break;
	case EVENT_STATE_CHANGE:
	case EVENT_NEW_SOCKET:
	case EVENT_RESCAN:
		hpci_evpool.evp_events[hpci_evpool.evp_wr_mark].socket = *evptr;
		hpci_evpool.evp_events[hpci_evpool.evp_wr_mark].socket.hpcid = hpcid;
		break;
	default:
		cmn_err(CE_WARN, "hpci_postevent: hpcid %d unknown event type %d",
		    hpcid, evtype);
		UNLOCK(hpci_postevent_mutex, opl);
		return -1;
	}
	hpci_evpool.evp_events[hpci_evpool.evp_wr_mark].eventType = evtype;
	hpci_evpool.evp_wr_mark = 
	    (hpci_evpool.evp_wr_mark + 1) % hpci_evcnt;
	if(hpci_evpool.evp_wr_mark == hpci_evpool.evp_rd_mark)
		hpci_evpool.evp_rd_mark = 
		    (hpci_evpool.evp_rd_mark + 1) % hpci_evcnt;
	UNLOCK(hpci_postevent_mutex, opl);
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "Calling  pollwakeup %x", hpci_php);
#endif
	pollwakeup(hpci_php, POLLIN);

	return 1;
}

int
hpci_get_hpcs_info(void *arg, int cmd)
{
	hpcd_uinfo_t hpcInfo;
	hpcs_uinfo_t *hpciPtr = (hpcs_uinfo_t *) arg, hpcsInfo;
	int	hpcIdx, hpcCount, busIdx;
	int	error = 0;
	hpcd_bus_t	*busList;
#undef	ENOUGH_INFO_SIZE
#define ENOUGH_INFO_SIZE sizeof(hpcs_uinfo_t) - sizeof(hpcd_uinfo_t)

	hpcCount=0;
	if (copyin((char *) arg, (char *) &hpcsInfo, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;
	for(hpcIdx=0; 
	    hpcIdx <= hpci_max && hpcCount < hpcsInfo.allocated; 
	    hpcIdx++)
	{
		if(hpcdList[hpcIdx].hpcd_iptr != NULL)
		{
			pl_t opl;

			hpcInfo.hpcid = hpcIdx;
			opl = (*hpcdList[hpcIdx].hpcd_iptr->lock_callback)
							(hpcdList[hpcIdx].hpcd_iptr);
			hpcInfo.buscnt = hpcdList[hpcIdx].hpcd_iptr->buscnt;

			busList = hpcdList[hpcIdx].hpcd_iptr->bus_list;
			hpcInfo.socketcnt = 0;
			for(busIdx = 0; busIdx < hpcInfo.buscnt; busIdx++)
			{
				hpcInfo.socketcnt += busList->socket_cnt;
				busList = busList->next;
			}

			(*hpcdList[hpcIdx].hpcd_iptr->unlock_callback)
							(hpcdList[hpcIdx].hpcd_iptr, opl);
			if(copyout((char *) &hpcInfo, 
			    (char *) &hpciPtr->info[hpcCount], 
			    sizeof(hpcInfo)) == -1)
			{
				return EFAULT;
			}
			hpcCount++;
		}
	}
	hpcsInfo.configured = hpci_cnt;
	hpcsInfo.returned = hpcCount;
	if(copyout((char *) &hpcsInfo, (char *) arg, ENOUGH_INFO_SIZE) == -1)
	{
		return EFAULT;
	}
	return error;
}


void
hpci_registerFunctions(hpcd_info_t *hpc, int hpcid)
{
	int b, s;
	hpcd_socket_t	*socket_list;
	hpcd_bus_t		*bus_list;
	int		bcnt, socket_cnt;

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "hpci_registerFunctions()");
#endif
	bcnt = hpc->buscnt;
	for(bus_list= hpc->bus_list, b=0; b < bcnt; b++, bus_list = bus_list->next)
	{
		socket_cnt = bus_list->socket_cnt;
		for(socket_list= bus_list->socket_list, s=0; s < socket_cnt; s++)
		{
			hpci_confupdate(hpcid, &socket_list->info, HPCI_CONFIGURE);
			socket_list->info.hpcid = hpcid;
			socket_list = socket_list->next;
		}
	}
}

void

hpci_confupdate(int hpcid, socket_info_t *sockInfo, int flags)
{
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "hpci_confupdate(flags = %d)", flags);
#endif
	
	switch(sockInfo->bustype)
	{
	case	CM_BUS_PCI:
		if(flags == HPCI_CONFIGURE)
			registerPciFunctions(sockInfo);
		else 	unregisterPciFunctions(sockInfo);
		break;
	case	CM_BUS_EISA:
		break;
	case	CM_BUS_ISA:
		break;
	default:
		break;
	}
}


int
hpci_get_socket_info(void *arg)
{
	int	error = 0;
	socket_list_t sockInfoList, *sockInfoPtr;
	int	len;
	int     hpcIdx, socketIdx, busIdx;
	hpcd_bus_t 	*busList;
	hpcd_socket_t	*socket_list;
#undef	ENOUGH_INFO_SIZE
#define ENOUGH_INFO_SIZE sizeof(socket_list_t) - sizeof(socket_info_t)


	if (copyin((char *) arg, (char *) &sockInfoList, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;

	len = sockInfoList.allocated;
	sockInfoList.configured = 0;
	sockInfoList.returned = 0;
	sockInfoPtr = (socket_list_t *) arg;

	for(hpcIdx=0; hpcIdx <= hpci_max; hpcIdx++)
	{
#ifdef HPCI_DEBUG
		if( hpcdList[hpcIdx].hpcd_iptr != NULL)
			cmn_err(CE_NOTE, "comparing: hpcid %d wanted hpcid %d",
			    hpcIdx, sockInfoList.busid);
#endif
		if(hpcdList[hpcIdx].hpcd_iptr != NULL &&
		    (sockInfoList.hpcid == -1 || sockInfoList.hpcid == hpcIdx))
		{
			pl_t opl;

			opl = (*hpcdList[hpcIdx].hpcd_iptr->lock_callback)
							(hpcdList[hpcIdx].hpcd_iptr);
			for(busIdx = 0,
			    busList = hpcdList[hpcIdx].hpcd_iptr->bus_list;
			    busIdx < hpcdList[hpcIdx].hpcd_iptr->buscnt;
			    busIdx++,
			    busList = busList->next)
			{
#ifdef HPCI_DEBUG
				cmn_err(CE_NOTE, "comparing: idx %d busid %d wanted busid %d",
				    busIdx, busList->busid, sockInfoList.busid);
#endif

				if(sockInfoList.busid != -1 &&
				    busList->busid != sockInfoList.busid)
				{
					(*hpcdList[hpcIdx].hpcd_iptr->unlock_callback)
						(hpcdList[hpcIdx].hpcd_iptr, opl);
					continue;
				}
				sockInfoList.configured += busList->socket_cnt;
				for(socketIdx = 0,
				    socket_list = busList->socket_list;
				    socketIdx < busList->socket_cnt;
				    socketIdx++,
				    socket_list = socket_list->next)
				{
#ifdef HPCI_DEBUG
					cmn_err(CE_NOTE, "copying: idx %d returned %d allocated %d",
					    socketIdx, sockInfoList.returned, sockInfoList.allocated);
#endif
					if(sockInfoList.returned < sockInfoList.allocated)
					{
						if(copyout((char *) &socket_list->info, 
						    (char *) &sockInfoPtr->info[sockInfoList.returned], 
						    sizeof(socket_info_t)) == -1)
						{
							(*hpcdList[hpcIdx].hpcd_iptr->unlock_callback)(hpcdList[hpcIdx].hpcd_iptr, opl);
							return EFAULT;
						}
						sockInfoList.returned++;
					}
				}
			}
			(*hpcdList[hpcIdx].hpcd_iptr->unlock_callback)(hpcdList[hpcIdx].hpcd_iptr, opl);
		}
	}
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "TOTAL: returned %d configured %d",
	    sockInfoList.returned,
	    sockInfoList.configured);
#endif
	if(copyout((char *) &sockInfoList, (char *) arg, ENOUGH_INFO_SIZE) == -1)
	{
		return EFAULT;
	}
	return error;
}

int
hpci_get_bus_info(void *arg)
{
	int	error = 0;
	hpcd_bus_ulist_t busInfoList, *busInfoPtr;
	int     hpcIdx, busIdx, busCount, len;
	hpcd_bus_t	*busList;
	hpcd_bus_uinfo_t busInfo;
#undef	ENOUGH_INFO_SIZE
#define ENOUGH_INFO_SIZE sizeof(hpcd_bus_ulist_t) - sizeof(hpcd_bus_uinfo_t)


	if (copyin((char *) arg, (char *) &busInfoList, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;

	len = busInfoList.allocated;
	busInfoList.configured = 0;
	busInfoList.returned = 0;
	busInfoPtr = (hpcd_bus_ulist_t *) arg;

	busCount = 0;
	for(hpcIdx=0, busCount=0; hpcIdx <= hpci_max; hpcIdx++)
	{
		if(hpcdList[hpcIdx].hpcd_iptr != NULL &&
		    busInfoList.hpcid == hpcIdx)
		{
			pl_t 	opl;

			opl = (*hpcdList[hpcIdx].hpcd_iptr->lock_callback)
							(hpcdList[hpcIdx].hpcd_iptr);
			busInfoList.configured =  hpcdList[hpcIdx].hpcd_iptr->buscnt;
			for(busIdx = 0,
			    busList = hpcdList[hpcIdx].hpcd_iptr->bus_list;
			    busIdx < hpcdList[hpcIdx].hpcd_iptr->buscnt;
			    busIdx++,
			    busList = busList->next)
			{
				if(busInfoList.returned < busInfoList.allocated)
				{
					busInfo.hpcid = hpcIdx;
					busInfo.busid = busList->busid;
					busInfo.runningSpeed = busList->pci_busdata.runningSpeed;
					busInfo.maxSpeed = busList->pci_busdata.maxSpeed;
					busInfo.socketcnt = busList->socket_cnt;
					busInfo.bustype = busList->bustype;

					if(copyout((char *) &busInfo,
					    (char *) &busInfoPtr->info[busInfoList.returned], 
					    sizeof(hpcd_bus_uinfo_t)) == -1)
					{
						(*hpcdList[hpcIdx].hpcd_iptr->unlock_callback)
							(hpcdList[hpcIdx].hpcd_iptr, opl);
						return EFAULT;
					}
					busInfoList.returned++;
				}
			}
			(*hpcdList[hpcIdx].hpcd_iptr->unlock_callback)
							(hpcdList[hpcIdx].hpcd_iptr, opl);
		}
	}
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "TOTAL: returned %d configured %d",
	    busInfoList.returned,
	    busInfoList.configured);
#endif
	if(copyout((char *) &busInfoList, (char *) arg, ENOUGH_INFO_SIZE) == -1)
	{
		return EFAULT;
	}
	return error;
}

int 	
hpci_get_function_info(void *arg)
{
	int i;
	function_list_t functionList;
	function_list_t *listPtr;
	socket_info_t *socketPtr;
	function_info_t *dptr;
	pl_t 	opl;

#undef	ENOUGH_INFO_SIZE
#define ENOUGH_INFO_SIZE sizeof(function_list_t) - sizeof(function_info_t)

	if (copyin((char *) arg, (char *) &functionList, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;
	socketPtr = LookupSocket(&functionList.socket);
	if(socketPtr == NULL)
		return EINVAL;
	opl = (*hpcdList[functionList.socket.hpcid].hpcd_iptr->lock_callback)
					(hpcdList[functionList.socket.hpcid].hpcd_iptr);
	functionList.configured = socketPtr->function_cnt;
	if(socketPtr->function_cnt < functionList.allocated)
		functionList.returned = socketPtr->function_cnt;
	else	functionList.returned = functionList.allocated;
	listPtr = (function_list_t *) arg;
	dptr = socketPtr->function;
	for(i=0;i < functionList.returned; i++)
	{
		if(copyout((char *) dptr, (char *) &listPtr->info[i], sizeof(function_info_t)) == -1)
		{
			(*hpcdList[functionList.socket.hpcid].hpcd_iptr->unlock_callback)
					(hpcdList[functionList.socket.hpcid].hpcd_iptr, opl);
			return EFAULT;
		}
		dptr = dptr->next;
	}

	(*hpcdList[functionList.socket.hpcid].hpcd_iptr->unlock_callback)
					(hpcdList[functionList.socket.hpcid].hpcd_iptr, opl);
	if(copyout((char *) &functionList, (char *) arg, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;
	return 0;
}

int 	
hpci_get_driver_info(void *arg, int cmd)
{
	hpci_drvsuspend_ulist_t suspendedDriverList;
	hpci_drvsuspend_ulist_t *listPtr;
	hpci_drvsuspend_list_t *linkdlistPtr;
	int i;
#undef	ENOUGH_INFO_SIZE
#define ENOUGH_INFO_SIZE sizeof(hpci_drvsuspend_ulist_t) - sizeof(hpci_suspend_cmd_t)

	if(cmd == HPCI_GET_SUSPEND_COUNT)
	{
		if(copyout((char *) &hpci_suspend_count, (char *) arg, sizeof(int)) == -1)
			return EFAULT;
		return 0;
	}
	if(hpci_suspend_count <= 0)
	{
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE,"hpci_get_driver_info: hpci_suspend_count = %d",
			hpci_suspend_count);
#endif
		return 0;
	}
	if (copyin((char *) arg, (char *) &suspendedDriverList, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;
	listPtr = (hpci_drvsuspend_ulist_t *) arg;
	if(suspendedDriverList.allocated <= 0)
	{
		return EINVAL;
	}
	if(suspendedDriverList.allocated > hpci_suspend_count)
		suspendedDriverList.returned = hpci_suspend_count;
	else	suspendedDriverList.returned = suspendedDriverList.allocated;

	linkdlistPtr = hpci_suspend_list;

	for(i=0; i< suspendedDriverList.returned; i++)
	{
		if(copyout((char *) &linkdlistPtr->info, (char *) &listPtr->info[i],
		    sizeof(hpci_suspend_cmd_t)) == -1)
			return EFAULT;
		linkdlistPtr = linkdlistPtr->next;
	}
	if(copyout((char *) &suspendedDriverList, (char *) arg, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;
	return 0;
}

int 	
hpci_get_event_info(void *arg, int cmd)
{
	hpci_event_tList eventList;
	hpci_event_tList *listPtr;
	int i;
	int count;
	int nextRead;
#undef ENOUGH_INFO_SIZE 
#define ENOUGH_INFO_SIZE sizeof(hpci_event_tList) - sizeof(hpci_event_t)

	if(hpci_evpool.evp_wr_mark < hpci_evpool.evp_rd_mark)
		count = hpci_evcnt - hpci_evpool.evp_rd_mark + hpci_evpool.evp_wr_mark;
	else count = hpci_evpool.evp_wr_mark - hpci_evpool.evp_rd_mark;


	if(cmd == HPCI_GET_EVENT_COUNT)
	{
		if(copyout((char *) &count, (char *) arg, sizeof(int)) == -1)
			return EFAULT;
		return 0;
	}
	if (copyin((char *) arg, (char *) &eventList, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;
	listPtr = (hpci_event_tList *) arg;
	if(eventList.allocated <= 0)
	{
		return EINVAL;
	}
	if(eventList.allocated > count)
		eventList.returned = count;
	else	eventList.returned = eventList.allocated;

	nextRead = hpci_evpool.evp_rd_mark;
	for(i=0; i< eventList.returned; i++)
	{
		if(copyout((char *) &hpci_evpool.evp_events[nextRead++], (char *) &listPtr->info[i],
		    sizeof(hpci_event_t)) == -1)
			return EFAULT;
		if(nextRead >= hpci_evcnt)
			nextRead = 0;
	}
	if(copyout((char *) &eventList, (char *) arg, ENOUGH_INFO_SIZE) == -1)
		return EFAULT;
	hpci_evpool.evp_rd_mark = nextRead;
	return 0;
}

int
hpci_change_socket_state(void *arg)
{
	socket_info_t sockInfo;
	socket_info_t *socketPtr;
	int errno;

	if (copyin((char *) arg, (char *) &sockInfo, sizeof(socket_info_t)) == -1)
		return EFAULT;
	socketPtr = LookupSocket(&sockInfo);
	if(socketPtr == NULL)
		return EINVAL;

	errno = hpcdList[sockInfo.hpcid].hpcd_iptr->modify_callback( 
	    hpcdList[sockInfo.hpcid].hpcd_iptr, 
	    HPCD_STATE_CHANGE_CMD, 
	    &sockInfo, socketPtr);
	if(!errno && copyout((char *) socketPtr, (char *) arg, sizeof(socket_info_t)) == -1)
				return EFAULT;
	return errno;
}
