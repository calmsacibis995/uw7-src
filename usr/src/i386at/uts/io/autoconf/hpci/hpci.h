#ident	"@(#)kern-i386at:io/autoconf/hpci/hpci.h	1.3.2.2"
#ident	"$Header$"

#ifndef _IO_HPCI_HPCI_H
#define _IO_HPCI_HPCI_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/autoconf/confmgr/confmgr.h>	/* REQUIRED */
#include <util/mod/mod.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#else

#include <sys/confmgr.h>	/* REQUIRED */
#include <sys/mod.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef unsigned char uchar; 

/******************************* START: PCI RELATED DEFINITIONS **************************************/

#define	MAX_SOCKET_LABEL_LEN	64

typedef struct __pci_func {
        ushort_t devnum; 	/* PCI device number for this slot assigned by the hpcd driver */
        ushort_t busid;		/* PCI bus id for this slot assigned by the hpcd driver */
        uchar    fnum; 		/* PCI function number assigned by the hpci driver. 	*/		
        uchar 	header; 	/* PCI header  assigned by the hpci driver. */
} pci_function_t;

typedef struct _pci_socket {
	ushort_t 	card_speed; 	/* measured in MHZ. For example 33 for 33MHZ, 66 for 66MHZ */
#define	MEM_32BITS	1
#define	MEM_64BITS	2
	uchar   mem_type; 	/* Memory type this card is using */
       	ushort_t slotnum; 	/* PCI slot number assigned by the hpcd driver */
} pci_socket_t;

typedef struct _function_info {
	struct _function_info *next;
       	 rm_key_t     	rmkey;  /*  resource manager key assigned by the hpci driver  */

	union {
		pci_function_t	pciFunc;
		char		filler[48];  /* place holder for future bus types */
	} ufunc;
	
} function_info_t;

typedef struct _socket_info {
				/* socket label; assigned by the hpcd driver */
	char	label[MAX_SOCKET_LABEL_LEN];
	uchar_t	bustype;	/* PCI,  etc */
	uint_t	mask;	        /* relevant bits in the current_state field assigned by the hpcd driver */
	uint_t	ROmask;         /* Read-only relevant bits and their correspondent values cannot be changed. */
	uint_t	current_state;	/* properties values assigned by the hpcd driver */
	uint_t	prev_state;	/* properties values before the last changes. */
	 int	busid;	        /* the busid */
	 int	hpcid;	        /* the hpcid */

	union {
		pci_socket_t	pci_data;
		char		maxSocket[48];	/* place holder for  future bus types */
	} usocket;
	ushort_t	        function_cnt; 	/* total # of functions; assigned by the hpci driver */
	function_info_t 	*function;	/* linked list of functions, assigned by the hpci driver */

} socket_info_t;
#define pci_socket usocket.pci_data

/* current_state/mask/ROmask */
#define	SOCKET_EMPTY			(1<<0)
#define SOCKET_POWER_ON			(1<<1)
#define SOCKET_GENERAL_FAULT		(1<<2)

#define	SOCKET_PCI_PRSNT_PIN1	 	(1<<30)
#define	SOCKET_PCI_PRSNT_PIN2	 	(1<<29)
#define	SOCKET_PCI_POWER_FAULT	 	(1<<28)
#define	SOCKET_PCI_ATTENTION_STATE 	(1<<27)
#define SOCKET_PCI_HOTPLUG_CAPABLE	(1<<26)
#define SOCKET_PCI_CONFIG_FAULT		(1<<25)
#define SOCKET_PCI_BAD_FREQUENCY	(1<<24)
#define SOCKET_PCI_INTER_LOCKED		(1<<23)

#define IS_MASK_BIT_SET(socketptr, bit)   (socketptr->mask&(bit))
#define IS_FLAGS_BIT_SET(socketptr, bit)  (socketptr->current_state&(bit))
#define IS_SOCKET_BIT_SET(socketptr, bit)   (IS_MASK_BIT_SET(socketptr, bit)& \
                                                 IS_FLAGS_BIT_SET(socketptr, bit))

#define SET_MASK_BIT(socketptr, bit)      socketptr->mask |= (bit)
#define SET_FLAGS_BIT(socketptr, bit)     socketptr->current_state |= (bit)
#define SET_RO_MASK_BIT(socketptr, bit)     socketptr->ROmask |= (bit)

#define RESET_MASK_BIT(socketptr, bit)    socketptr->mask &= ~(bit)
#define RESET_FLAGS_BIT(socketptr, bit)   socketptr->current_state &= ~(bit)
#define RESET_RO_MASK_BIT(socketptr, bit)   socketptr->ROmask &= ~(bit)

/******************************* END: SOCKET RELATED DEFINITIONS **************************************/


/*      eventType     */

#define EVENT_STATE_CHANGE  	1
#define EVENT_NEW_SOCKET  	2
#define EVENT_RESCAN 	   	3
#define EVENT_DRVCMD_STATUS   	4 /* when one of the ddi notify operation is completed */


#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <io/autoconf/resmgr/resmgr.h>		/* REQUIRED */

#else

#include <sys/types.h>		/* REQUIRED */
#include <sys/resmgr.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	HPCI_VALLOC(ptr, structure, arrayelement, arraylength) \
		ptr = (structure *) malloc(sizeof(structure) + (arraylength -1)*sizeof(arrayelement)); \
		if(ptr != NULL) { \
			memset(ptr, '\0', (sizeof(structure) + (arraylength -1)*sizeof(arrayelement)));\
			ptr->allocated = arraylength; \
		}
/*
 * hpci driver ioctls
 */

#define	HPCIIOCTL		('S'<<24|'C'<<16|'I'<<8)

/* 
 * Get the number of hot plug controller drivers configured in the system. 
 *
 * Usage:
 * int hpciFd, count;
 * hpciFd = open("/dev/hpci", O_RDWR);
 * ioctl(hpciFd, HPCI_GET_HPCS_COUNT, &count);
 * 
 * Return:
 * 0 if succeeds with the count is set to the numbers of  hot plug controller drivers configured.
 * -1 and errno is set to EFAULT if the address passed is not valid.
 */
#define	HPCI_GET_HPCS_COUNT	(HPCIIOCTL | 1) /* Get the number of hot plug controller drivers

/*
 * Get information about hot plug controller drivers configured in the system. 
 * Usage:
 * 
 * int count;
 * hpcs_uinfo_t *hpcsInfo;
 * ..... use HPCI_GET_HPCS_COUNT to get the count .....
 *
 * HPCI_VALLOC(hpcsInfo, hpcs_uinfo_t, hpcd_uinfo_t, count);
 * ioctl(hpciFd, HPCI_GET_HPCS_INFO, hpcsInfo);
 *
 * Return:
 * 0 if succeeds with the hpcsInfo is filled with information about hot plug controller drivers configured.
 * -1 and errno is set to EFAULT if the address passed is not valid.
 */
#define	HPCI_GET_HPCS_INFO	(HPCIIOCTL | 2)

typedef struct {
	/* Hot Plug Controller driver instance ID */
        short   hpcid;
	/* Number of busses controlled by this instance */
        short   buscnt;
	/* Total Number of sockets on all busses */
        ushort   socketcnt;
} hpcd_uinfo_t;

typedef struct {
	/* allocated:
	 * set by the MACRO HPCI_VALLOC to the actual size 
	 * of the variable length array info (below).
	 */
	int	allocated;

	/* configured:
	 * set by the driver of how many configured in the system.
	 */
	int	configured;

	/* returned:
	 * set by the driver of how many filled and returned in the array info.
	 */
	int	returned;


	hpcd_uinfo_t info[1];
} hpcs_uinfo_t;


/*
 * Get information about hot plug bus information. 
 * Usage:
 * 
 * int hpcid;
 * hpcs_uinfo_t *hpcsInfo;
 * hpcd_bus_ulist_t *hpcdBusList;
 * ..... use HPCI_GET_HPCS_INFO to get hpcsInfo .....
 *
 * HPCI_VALLOC(hpcdBusList, hpcd_bus_ulist_t, hpcd_bus_uinfo_t, 
 *	hpcsInfo->info[hpcid].buscnt);
 * hpcdBusList->hpcid = thathpcid;
 * ioctl(hpciFd, HPCI_GET_BUS_INFO, hpcdBusList);
 *
 * Return:
 * 0 if succeeds with the hpcd_bus_uinfo_t is filled with information about busses 
 * controlled by this hpcd driver instance.
 * -1 and :
 *	errno is set to EFAULT if the address passed is not valid,
 *	errno is set to EINVAL if hpcid or busid are not valid.
 */
#define	HPCI_GET_BUS_INFO	(HPCIIOCTL | 3)

typedef struct {
        short   	busid;

        ushort          socketcnt;       /* number of sockets attached to this bus */

	/* the following two element is measured in MHZ.
         * For example 33 for 33MHZ, 66 for 66MHZ
         */
        ushort          runningSpeed;    /* the speed currently the bus is running */
        ushort          maxSpeed;        /* the maximum speed the bus can run */

	int		hpcid;
        uchar   	bustype; 	  /* Various types are defined in autoconf/confmgr/cm_i386at.h */
} hpcd_bus_uinfo_t;
	
typedef struct {
	int		allocated;
	int		configured;
	int		returned;

	int		hpcid; 		/* hpcd driver instance id */

	hpcd_bus_uinfo_t 	info[1];
} hpcd_bus_ulist_t;


/*
 * Get the count and information about sockets attached to a hot plug bus.
 * Usage:
 * 
 * hpcd_bus_uinfo_t *hpcdBusInfo;
 * socket_list_t *socketInfoList;
 * ................... 
 *
 * HPCI_VALLOC(socketInfoList, socket_list_t, socket_info_t, hpcdBusInfo->socketcnt);
 * socketInfoList->hpcid = thathpcid;
 * socketInfoList->busid = thatbusid;
 * ioctl(hpciFd, HPCI_GET_SOCKET_INFO, socketInfoList);
 *
 * Return:
 * 0 if succeeds with the socketInfoList is filled with information about sockets:
 *	attached to a bus if busid is >= 0 and hpcid >= 0, 
 *	to all busses controlled by a hpcd driver instance if hpcid >= 0 and busid = -1,
 *	to all busses controlled by all hpcd drivers instance if hpcid = -1
 *
 * -1 and :
 *	errno is set to EFAULT if the address passed is not valid,
 *	errno is set to EINVAL if hpcid or busid are not valid.
 */
#define	HPCI_GET_SOCKET_INFO	(HPCIIOCTL | 4)

typedef struct {
	int		allocated;
	int		configured;
	int		returned;

        short   	hpcid;
        short   	busid;
	socket_info_t 	info[1];
} socket_list_t;



/*
 * Get information about functions attached to a socket. For example, a PCI socket (slot)
 * might have multiple functions such as networking, scsi, etc...
 * Usage:
 * 
 * socket_list_t *socketInfoList;
 * function_list_t *functionList;
 * socket_info_t socket;
 * int idx;
 * ................... 
 * socket = socketInfoList->info[idx];
 * HPCI_VALLOC(functionList, function_list_t, function_info_t, socket.functionCnt);
 * functionList->socket = socket;
 * ioctl(hpciFd, HPCI_GET_FUNCTION_INFO, functionList);
 *
 * Return:
 * 0 if succeeds.
 *
 * -1 and :
 *	errno is set to EFAULT if the address passed is not valid,
 *	errno is set to EINVAL if hpcid or busid are not valid.
 */
typedef struct {
	int     allocated;
	int     configured;
	int     returned;

	socket_info_t socket; /* socket which to get functions attached to */
	function_info_t info[1];
} function_list_t;
#define	HPCI_GET_FUNCTION_INFO	(HPCIIOCTL | 5)

/*
 * Set the state of a hotplug-capable socket. 
 * Usage:
 * 
 * hpcd_bus_uinfo_t *hpcdBusInfo;
 * socket_list_t *socketInfoList;
 * int idx = -1;
 * socket_info_t changed;
 * ................... 
 * changed.socket_rflags &= ~SOCKET_POWER_ON; *** Turn power off ***
 * changed = socketInfoList->info[idx];
 * ioctl(hpciFd, HPCI_SET_SOCKET_STATE, &changed);
 *
 * Return:
 * 0 if succeeds.
 *
 * -1 and :
 *	errno is set to EFAULT if the address passed is not valid,
 *	errno is set to EINVAL if hpcid or busid are not valid.
 */
#define	HPCI_SET_SOCKET_STATE	(HPCIIOCTL | 6)

/*
 * The following ioctls:
 *	HPCI_BIND_INSTANCE 
 *	HPCI_UNBIND_INSTANCE
 * are used to add, modify, remove or resume an instance of a driver.
 *
 * Usage:
 *
 * hpci_drvcmd_t drvCmd;
 * ...................
 * drvCmd.old_rmkey = socketInfoList->info[idx].pci_data.pci_function->pcif_rmkey;
 * drvCmd.new_rmkey = drvCmd.old_rmkey;
 * strcpy(drvCmd.drv_name, "adsl");
 *
 * ioctl(hpciFd, HPCI_BIND_INSTANCE, &drvCmd)
 * ioctl(hpciFd, HPCI_UNBIND_INSTANCE, &drvCmd)
 *
 * Return:
 * 0 if succeeds.
 *
 * -1 and :
 *      errno is set to EFAULT if the address passed is not valid,
 *      errno is set to EINVAL if hpcid or busid are not valid.
 *      errno is set to EINPROGRESS and an asynchronous EVENT_DRVCMD_DONE 
 *	     event will be sent when the operation is actually completed.
 */
typedef struct _drv_cmd {
        rm_key_t        new_rmkey; /* this field is not used by 
				    * HPCI_UNBIND_INSTANCE, HPCI_BIND_INSTANCE 
				    */
        rm_key_t        old_rmkey; 
	char 		drv_name[CM_MODNAME_MAX];
} hpci_drvcmd_t;

#define	HPCI_BIND_INSTANCE	(HPCIIOCTL | 7) 
/*
#define	HPCI_MODIFY_INSTANCE	(HPCIIOCTL | 8)
*/
#define	HPCI_UNBIND_INSTANCE	(HPCIIOCTL | 9)

/*
 * The HPCI_SUSPEND_INSTANCE ioctl is used to suspend an instance of a driver.
 *
 * Usage:
 *
 * hpci_suspend_cmd_t cmd;
 * ...................
 * cmd.drv_socket = socketInfoList->info[idx];
 * cmd.drv_rmkey = socketInfoList->info[idx].pci_data.pci_function->pcif_rmkey;
 * strcpy(cmd.drv_name, "adsl");
 *
 * ioctl(hpciFd, HPCI_SUSPEND_INSTANCE, &cmd)
 * 
 * Return:
 * 0 if succeeds.
 *
 * -1 and :
 *      errno is set to EFAULT if the address passed is not valid,
 *      errno is set to EINVAL if hpcid or busid are not valid.
 *      errno is set to EINPROGRESS and an asynchronous EVENT_DRVCMD_DONE 
 *	     event will be sent when the operation is actually completed.
 */
#define	HPCI_SUSPEND_INSTANCE	(HPCIIOCTL | 10) 

typedef struct _drv_suspend_instance {
	rm_key_t        drv_rmkey;
        char            drv_name[CM_MODNAME_MAX];
	socket_info_t	drv_socket;
        int		status;
	int		drv_suspend_id;	/* returned id of the resume driver instance */	
} hpci_suspend_cmd_t;
	
/* status */
#define	SUSPEND_IN_PROGRESS	1
#define	SUSPEND_DONE		2

/*
 * The HPCI_RESUME_INSTANCE ioctl is used to resume an instance of a driver.
 *
 * Usage:
 *
 * hpci_resume_cmd_t cmd;
 * ...................
 * cmd.drv_suspend_id = some id;
 * cmd.new_rmkey = new key to be resumed on
 *
 * ioctl(hpciFd, HPCI_RESUME_INSTANCE, &cmd)
 * 
 * Return:
 * 0 if succeeds.
 *
 * -1 and :
 *      errno is set to EFAULT if the address passed is not valid,
 *      errno is set to EINVAL if drv_suspend_id or new_rmkey are not valid.
 *      errno is set to EINPROGRESS and an asynchronous EVENT_DRVCMD_DONE 
 *	     event will be sent when the operation is actually completed.
 */
#define	HPCI_RESUME_INSTANCE	(HPCIIOCTL | 11) 

typedef struct _drv_resume_instance {
	rm_key_t        new_rmkey;
	int		drv_suspend_id;		
} hpci_resume_cmd_t;
	
/*
 * The HPCI_GET_SUSPEND_COUNT and HPCI_GET_SUSPEND_INFO ioctls retrieve the count and information
 * about suspended driver instances that were suspended as a result of a previous HPCI_SUSPEND_INSTANCE ioctl.
 *
 * Usage:
 *
 * hpci_drvsuspend_ulist_t *list;
 * int count;
 * ioctl(hpciFd, HPCI_GET_SUSPEND_COUNT, &count);
 * HPCI_VALLOC(list, hpci_drvsuspend_ulist_t, hpci_suspend_cmd_t, count);
 *
 * ioctl(hpciFd, HPCI_GET_SUSPEND_INFO, list);
 * 
 * Return:
 * 0 if succeeds.
 *
 * -1 and :
 *      errno is set to EFAULT if the address passed is not valid,
 */
#define	HPCI_GET_SUSPEND_COUNT	(HPCIIOCTL | 12)
#define	HPCI_GET_SUSPEND_INFO	(HPCIIOCTL | 13)


typedef struct _supdrv_suspend {
	int			allocated;
	int			returned;
	hpci_suspend_cmd_t	info[1];
} hpci_drvsuspend_ulist_t;
	
/*
 * The HPCI_GET_EVENT_COUNT and HPCI_GET_EVENT_INFO ioctls retrieve the count and information
 * about asynchronous events buffered in the hpci driver.
 *
 * Usage:
 *
 * hpci_event_tList *list;
 * int count;
 * ioctl(hpciFd, HPCI_GET_EVENT_COUNT, &count);
 * HPCI_VALLOC(list, hpci_event_tList, hpci_event_t, count);
 *
 * ioctl(hpciFd, HPCI_GET_EVENT_INFO, list);
 * 
 * Return:
 * 0 if succeeds.
 *
 * -1 and :
 *      errno is set to EFAULT if the address passed is not valid,
 */
#define	HPCI_GET_EVENT_COUNT	(HPCIIOCTL | 14) 
#define	HPCI_GET_EVENT_INFO	(HPCIIOCTL | 15)

typedef struct _hpciEvent {
	int	        eventType; 
        socket_info_t        socket;

/* the following are valid only when eventType is EVENT_DRVCMD_STATUS */
	int	        drvCmd;
	rm_key_t        ormkey;
	rm_key_t        nrmkey;
        char            drv_name[CM_MODNAME_MAX];
        int             status;
} hpci_event_t;
	
typedef struct _hpciEventInfo {
	int             allocated;
        int             returned;

	hpci_event_t	info[1];
} hpci_event_tList;

/* 
 * Get the ddi8 driver capabilities
 *
 * Usage:
 * 
 * hpci_drv_cap_t cap; 
 * strcpy(cap.drv_name, "adsl");
 * ioctl(hpciFd, HPCI_GET_DRV_CAP, &cap);
 * 
 * Return:
 * 0 if succeeds. 
 *  -1 and errno set.
 */
#define	HPCI_GET_DRV_CAP	(HPCIIOCTL | 16)
typedef struct {
	char        	drv_name[CM_MODNAME_MAX];
	ulong_t		drv_cap;
} hpci_drv_cap_t;


#ifdef _KERNEL
#ifdef _KERNEL_HEADERS
#include <util/ipl.h>
#include <util/ksynch.h>
#include <io/autoconf/resmgr/resmgr.h>
#else
#include <sys/ipl.h>
#include <sys/ksynch.h>
#include <sys/resmgr.h>
#endif


#define HPCD_STATE_CHANGE_CMD          1

/* flags for hpci_confupdate */

#define	HPCI_CONFIGURE		1
#define	HPCI_DECONFIGURE	2

typedef struct _hpcd_socket {
	struct _hpcd_socket *next;	/* Next socket in the list, assigned by hpcd drivers */
	socket_info_t       info;	/* The socket information, assigned by hpcd drivers */
} hpcd_socket_t;

typedef struct {
	int	pci_busnum;

	/* the following two elements are measured in MHZ. 
       	 * For example 33 for 33MHZ, 66 for 66MHZ
       	 */
	ushort_t runningSpeed;	 /* the speed currently the bus is running */
	ushort_t maxSpeed;	 /* the maximum speed the bus can run */
} pci_bus_info_t; 

typedef struct _hpcd_bus {		
	struct _hpcd_bus *next;		/* Next bus in the list, assigned by hpcd drivers */
	int            	  bustype;	/* bus type. Possible types are defined in cm_i386at.h */
	int		  cgnum;	/* node number to which this bus is attached */
	int		  busid;	/* a unique id within the set of busses controlled
			 		 * by each hpcd instance; assigned by hpcd 
					 */
	union {
		pci_bus_info_t pcibus; 
		char	 filler[32]; 	/* place holder for other bus types info */
	} ubus;

	int 		socket_cnt;	/* number of sockets on this bus, assigned by hpcd drivers */
	hpcd_socket_t 	*socket_list; 	/* Sockest found on this bus, assigned by hpcd drivers  */
} hpcd_bus_t;
#define	pci_busdata	ubus.pcibus


struct _hpcd_info; 
typedef int (*hpci_op_t)(
	struct _hpcd_info *hpc,	/* pointer to the hpcd_info_t instance itsetlf */
	int 	  	cmd,	/* Currently, only HPCD_STATE_CHANGE_CMD is supsocketed */
	socket_info_t *modified,/* a copy of the socket with the properties the user wants to modifiy   */
	socket_info_t *original	/* a pointer to the socket in the tree */
	);

typedef  pl_t  (*hpcd_lock_op_t)( struct _hpcd_info *);
typedef  int  (*hpcd_unlock_op_t)( struct _hpcd_info *, pl_t);
typedef struct _hpcd_info {	
	void      *private;		/* to be used by the hpcd driver instance itself */
	hpci_op_t modify_callback; 	/*  a callback function to be used by the hpci driver 
                		   	 *  for calling back into the hpcd driver
               			   	 */
	hpcd_lock_op_t	 lock_callback;	/* HPCD interface lock routine. Used by hpci
					 *  to lock the interface data structure before  
					 * accessing it. Note that hpci accesses it ONL
					 * for reading. 
					 */
	hpcd_unlock_op_t unlock_callback; 	/*  HPCD interface unlock routine */
	int            	 buscnt;		/* total number of busses controlled by this hpcd  instance */
	hpcd_bus_t       *bus_list;	  	/* Bus list described above */
} hpcd_info_t;


/* Prototypes for HPCI interface routines */

int 	hpci_attach(hpcd_info_t  *hpcptr);
int	hpci_detach(int hpcid);
void	hpci_confupdate(int hpcid, socket_info_t *socket, int flag);
int	hpci_postevent(int hpcid, socket_info_t *ev, int evtype);

#endif /* _KERNEL */	

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HPCI_HPCI_H */
