#ifndef _DL_H_
#define _DL_H_

#include <sys/types.h>

#ifdef _DL_DEBUG_
#define DEBUG(args)	printf args
#else
#define DEBUG(args)	
#endif
extern int dl_debug;
#define DL_DEBUG(level,fmt)		if (dl_debug>=level) printf(fmt); else ;
#define DL_DEBUG1(level,fmt,a1)		if (dl_debug>=level) printf(fmt, a1); else ;
#define DL_DEBUG2(level,fmt,a1,a2)	if (dl_debug>=level) printf(fmt, a1, a2); else ;
#define DL_DEBUG3(level,fmt,a1,a2,a3)	if (dl_debug>=level) printf(fmt, a1, a2, a3); else ;

#define NOBLOCKING	0
#define BLOCKING	1
#define DL_GETMSG_TIMEOUT	15

#define	DL_ADDR_SIZE 	6
#define DL_SAP_SIZE	2
#define DL_MAX_PKTSZ 	150
#define DL_RAW_SAP	0x80
#define DL_ETHER_SAP	0x100
#define DL_SNAP_SAP	0xaa
#define DL_CSMACD_SAP	0x40
#define DL_TEST_SAP 	100 
/* #define DL_TEST_SAP 	0x8141 */
#define DL_TEST_LEN	100
#define DL_MAX_PKT	1518
#define DL_IOC_TIMEOUT	15

#define	dl_open		open
#define dl_close	close
#define	dl_err 		printf

typedef struct {
	char	dl_addr[DL_ADDR_SIZE];
	ushort_t	dl_sap; 	/* ajmer 09/07/94 */
} dl_addr_t;

#define IC_MAX (sizeof(tet_testlist)/(sizeof(struct tet_testlist)))

/*
 * Error code returned by dl calls
 */
#define DL_EALLOC	-1
#define DL_EPUTMSG	-2
#define DL_EGETMSG	-3
#define DL_EPROTO	-4
#define DL_EALARM	-5

/* prototypes */
void			dl_showinfo(int);
int			dl_info(int, dl_info_ack_t *);
int			dl_bind(int, dl_bind_req_t *, dl_bind_ack_t *);
int			dl_unbind(int);
int			dl_subsbind(int, struct snap_sap);
int			dl_subsunbind(int, struct snap_sap);

int 			dl_recv(int ,char *,int ,char *,int);

int 			dl_inforeq(int, dl_info_req_t *);
int 			dl_bindreq(int, dl_bind_req_t *);
int 			dl_unbindreq(int, dl_unbind_req_t *);
int 			dl_subsbindreq(int, dl_subs_bind_req_t *);
int 			dl_subsunbindreq(int ,dl_subs_unbind_req_t *);

int		dl_infoack(int, dl_info_ack_t *);
int		dl_bindack(int, dl_bind_ack_t *);
int		dl_subsbindack(int, dl_subs_bind_ack_t *);
int		dl_okack(int, dl_ok_ack_t *);

int 			dl_gmib(int, DL_mib_t *);
int 			dl_smib(int, DL_mib_t *);
int 			dl_ccsmacdmode(int);
int			dl_promiscon(int fd);
int			dl_promiscoff(int fd);
int 			dl_getmulti(int fd, char maddr[]);
int 			dl_addmulti(int fd, char maddr[]);
int 			dl_delmulti(int fd, char maddr[]);
int 			dl_genaddr(int fd, char enaddr[]);
int 			dl_senaddr(int fd, char enaddr[]);
int 			dl_glpcflg(int fd, int *lpcflagp);
int 			dl_slpcflg(int fd);
int			dl_reset(int fd);
int			dl_enable(int fd);
int			dl_disable(int fd);
#endif

