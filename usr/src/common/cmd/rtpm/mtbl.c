
#ident	"@(#)rtpm:mtbl.c	1.5.1.1"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <mas.h>
#include <assert.h>
#include <metreg.h>
#include <sys/dl.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/ksynch.h>   /* get def for lock_t - used in dpli_ether.h */
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/protosw.h>
#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netdb.h>
#include <sys/sockio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#define _NET_NW_NWPORTABLE_H  /* turn off uint32 typedef */
typedef unsigned short uint16;
typedef short int16;
typedef unsigned char uint8;
typedef char int8;
#include <sys/spx_app.h>
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#include <sys/sap_app.h>
#include <sys/mman.h>
#include "rtpm.h"
#include "mtbl.h"

/*
 * the current lbolt time
 */
extern long currtime;
/*
 * time difference in seconds between the current and previous samples
 */
extern float tdiff;
/*
 * memory and swap stats
 */
extern size_t totalmem;			/* physical memory */
extern uint_t mem_swappg;		/* virtual swap memory	*/
extern ulong_t dsk_swappg;		/* swap disk	*/
extern ulong_t dsk_swappgfree;		/* swap disk	*/
/*
 * lwp statistics
 */
extern struct lwpstat lwp_count;
/*
 * ethernet stats
 */
extern struct net_total net_total;
/*
 * number of ethernet devices
 */
extern int nether_devs;
/*
 * names of ethernet devices
 */
extern char *ether_nm[];
/*
 * ethernet stats
 */
extern DL_mib_t *etherstat[];
/*
 * tcp, udp, ip, icmp stats
 */
extern struct tcpstat  tcpstat;
extern struct udpstat  udpstat;
extern struct ipstat   ipstat;
extern struct icmpstat icmpstat;
/*
 * netware stats
 */
extern SAPD	sap_buf;
extern SAPL	*sap_lan_buf;
extern int ipx_datapackets;
extern int sap_total_servers;
extern int sap_total;
extern int ipx_total;
extern int rip_total;
extern int spx_total;
extern int netware_errs;
extern int sap_unused;
extern int nlans;
extern RouterInfo_t ripbuf;
extern struct {
	IpxLanStats_t	 l;
	IpxSocketStats_t s;
} ipxbuf;
extern spxStats_t	spxbuf;
extern spxConStats_t	*spxconbuf;
extern int ipx_sent_to_TLI;
extern int ipx_total_ioctls;
extern int rip_total_router_packets_sent;
/*
 * EISA bus utilization  - may not be present on all machines
 */
extern dl_t eisa_bus_util_sum;	/* sum of bus idle cycles	*/
extern int eisa_bus_util_sumcnt; /* count of bus samples	*/
/*
 * maximum number of history points to keep
 */
extern int maxhist;
/*
 * metric descriptor ret from mas_open
 */
extern int md;
/*
 * a dummy variable for disabled mets
 */
static int dummy = 0;
/*
 * the number of metrics in mettbl
 */
int nmets;
/*
 * mettbl is the list of everything we care about cooking and displaying
 */
struct metric mettbl[] = { 
{ HZ_TIX, 0, {MAS_NATIVE, 0}, NONE },
{ NCPU, 0, {MAS_SYSTEM, 0}, NONE },
{ NDISK, 0, {MAS_SYSTEM, 0}, NONE },
{ NFSTYP, 0, {MAS_SYSTEM, 0}, NONE },
{ FSNAMES, 1, {NFSTYP, 0}, NONE },
{ KMPOOLS, 0, {MAS_SYSTEM, 0}, NONE },
{ KMASIZE, 1, {KMPOOLS, 0}, NONE },
{ PGSZ, 0, {MAS_NATIVE, 0}, NONE },
{ DS_NAME, 1, {NDISK, 0}, NONE },
{ NETHER, 0, {MAS_SYSTEM, 0}, NONE },
{ ETHNAME, 1, {NETHER, 0}, NONE },
{ SPX_max_connections, 0, {MAS_SYSTEM, 0}, INSTANT },
{ MPC_CPU_USR, 1, {NCPU, 0}, MEAN },
{ MPC_CPU_SYS, 1, {NCPU, 0}, MEAN },
{ MPC_CPU_WIO, 1, {NCPU, 0}, MEAN },
{ MPC_CPU_IDLE, 1, {NCPU, 0}, MEAN },
{ MPF_IGET, 2, {NCPU, NFSTYP}, RATE },
{ MPF_DIRBLK, 2, {NCPU, NFSTYP}, RATE },
{ MPF_IPAGE, 2, {NCPU, NFSTYP}, RATE },
{ MPF_INOPAGE, 2, {NCPU, NFSTYP}, RATE },
{ FREEMEM, 0, {MAS_SYSTEM, 0}, RATE },	/* summed every tick */
{ FREESWAP, 0, {MAS_SYSTEM, 0}, RATE },	/* summed every second */
{ FSWIO, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PHYSWIO, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RUNQUE, 0, {MAS_SYSTEM, 0}, RATE },	/* summed every sec */
{ RUNOCC, 0, {MAS_SYSTEM, 0}, MEAN },
{ SWPQUE, 0, {MAS_SYSTEM, 0}, RATE },	/* summed every second */
{ SWPOCC, 0, {MAS_SYSTEM, 0}, MEAN },
{ PROCFAIL, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCFAIL, 0, {MAS_SYSTEM, 0}, RATE },
{ PROCUSE, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ MPK_MEM, 2, {NCPU, KMPOOLS}, INSTANT },
{ MPK_BALLOC, 2, {NCPU, KMPOOLS}, INSTANT },
{ MPK_RALLOC, 2, {NCPU, KMPOOLS}, INSTANT },
{ MPK_FAIL, 2, {NCPU, KMPOOLS}, INSTANT },
{ MPK_FAIL, 2, {NCPU, KMPOOLS}, RATE },
{ MPV_PREATCH, 1, {NCPU, 0}, RATE },
{ MPV_ATCH, 1, {NCPU, 0}, RATE },
{ MPV_ATCHFREE, 1, {NCPU, 0}, RATE },
{ MPV_ATCHFREE_PGOUT, 1, {NCPU, 0}, RATE },
{ MPV_ATCHMISS, 1, {NCPU, 0}, RATE },
{ MPV_PGIN, 1, {NCPU, 0}, RATE },
{ MPV_PGPGIN, 1, {NCPU, 0}, RATE },
{ MPV_PGOUT, 1, {NCPU, 0}, RATE },
{ MPV_PGPGOUT, 1, {NCPU, 0}, RATE },
{ MPV_SWPOUT, 1, {NCPU, 0}, RATE },
{ MPV_PSWPOUT, 1, {NCPU, 0}, RATE },
{ MPV_VPSWPOUT, 1, {NCPU, 0}, RATE },
{ MPV_SWPIN, 1, {NCPU, 0}, RATE },
{ MPV_PSWPIN, 1, {NCPU, 0}, RATE },
{ MPV_VIRSCAN, 1, {NCPU, 0}, RATE },
{ MPV_VIRFREE, 1, {NCPU, 0}, RATE },
{ MPV_PHYSFREE, 1, {NCPU, 0}, RATE },
{ MPV_PFAULT, 1, {NCPU, 0}, RATE },
{ MPV_VFAULT, 1, {NCPU, 0}, RATE },
{ MPV_SFTLOCK, 1, {NCPU, 0}, RATE },
{ MPS_SYSCALL, 1, {NCPU, 0}, RATE },
{ MPS_FORK, 1, {NCPU, 0}, RATE },
{ MPS_LWPCREATE, 1, {NCPU, 0}, RATE },
{ MPS_EXEC, 1, {NCPU, 0}, RATE },
{ MPS_READ, 1, {NCPU, 0}, RATE },
{ MPS_WRITE, 1, {NCPU, 0}, RATE },
{ MPS_READCH, 1, {NCPU, 0}, RATE },
{ MPS_WRITECH, 1, {NCPU, 0}, RATE },
{ MPF_LOOKUP, 1, {NCPU, 0}, RATE },
{ MPF_DNLC_HITS, 1, {NCPU, 0}, RATE },
{ MPF_DNLC_MISSES, 1, {NCPU, 0}, RATE },
{ FILETBLINUSE, 0, {MAS_SYSTEM, 0}, INSTANT },
{ FILETBLFAIL, 0, {MAS_SYSTEM, 0}, INSTANT },
{ FILETBLFAIL, 0, {MAS_SYSTEM, 0}, RATE },
{ FLCKTBLMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ FLCKTBLINUSE, 0, {MAS_SYSTEM, 0}, INSTANT },
{ FLCKTBLFAIL, 0, {MAS_SYSTEM, 0}, INSTANT },
{ FLCKTBLFAIL, 0, {MAS_SYSTEM, 0}, RATE },
{ FLCKTBLTOTAL, 0, {MAS_SYSTEM, 0}, RATE },
{ MAXINODE, 1, {NFSTYP, 0}, INSTANT },
{ CURRINODE, 1, {NFSTYP, 0}, INSTANT },
{ INUSEINODE, 1, {NFSTYP, 0}, INSTANT },
{ FAILINODE, 1, {NFSTYP, 0}, INSTANT },
{ FAILINODE, 1, {NFSTYP, 0}, RATE },
{ MPS_PSWITCH, 1, {NCPU, 0}, RATE },
{ MPS_RUNQUE, 1, {NCPU, 0}, RATE },		/* summed every second */
{ MPS_RUNOCC, 1, {NCPU, 0}, MEAN },
{ MPB_BREAD, 1, {NCPU, 0}, RATE },
{ MPB_BWRITE, 1, {NCPU, 0}, RATE },
{ MPB_LREAD, 1, {NCPU, 0}, RATE },
{ MPB_LWRITE, 1, {NCPU, 0}, RATE },
{ MPB_PHREAD, 1, {NCPU, 0}, RATE },
{ MPB_PHWRITE, 1, {NCPU, 0}, RATE },
{ MPT_RCVINT, 1, {NCPU, 0}, RATE },
{ MPT_XMTINT, 1, {NCPU, 0}, RATE },
{ MPT_MDMINT, 1, {NCPU, 0}, RATE },
{ MPT_RAWCH, 1, {NCPU, 0}, RATE },
{ MPT_CANCH, 1, {NCPU, 0}, RATE },
{ MPT_OUTCH, 1, {NCPU, 0}, RATE },
{ MPI_MSG, 1, {NCPU, 0}, RATE },
{ MPI_SEMA, 1, {NCPU, 0}, RATE },
{ MPR_LWP_FAIL, 1, {NCPU, 0}, INSTANT },
{ MPR_LWP_FAIL, 1, {NCPU, 0}, RATE },
{ MPR_LWP_USE, 1, {NCPU, 0}, INSTANT },
{ MPR_LWP_MAX, 1, {NCPU, 0}, INSTANT },
{ DS_CYLS, 1, {NDISK, 0}, NONE },
{ DS_FLAGS, 1, {NDISK, 0}, NONE },
{ DS_QLEN, 1, {NDISK, 0}, INSTANT },
{ DS_ACTIVE, 1, {NDISK, 0}, MEAN },
{ DS_RESP, 1, {NDISK, 0}, INSTANT },
{ DS_READ, 1, {NDISK, 0}, RATE },
{ DS_READBLK, 1, {NDISK, 0}, RATE },
{ DS_WRITE, 1, {NDISK, 0}, RATE },
{ DS_WRITEBLK, 1, {NDISK, 0}, RATE },
{ MPC_CPU_USR, 1, {NCPU, 0}, MEAN },
{ MPC_CPU_IDLE, 1, {NCPU, 0}, MEAN },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ PROCMAX, 0, {MAS_SYSTEM, 0}, INSTANT },
{ MPS_READ, 1, {NCPU, 0}, RATE },
{ MPS_READCH, 1, {NCPU, 0}, RATE },
{ MPF_DNLC_HITS, 1, {NCPU, 0}, INSTANT },
{ FREEMEM, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ETH_InUcastPkts, 1, {NETHER, 0}, RATE },
{ ETH_OutUcastPkts, 1, {NETHER, 0}, RATE },
{ ETH_InNUcastPkts, 1, {NETHER, 0}, RATE },
{ ETH_OutNUcastPkts, 1, {NETHER, 0}, RATE },
{ ETH_InOctets, 1, {NETHER, 0}, RATE },
{ ETH_OutOctets, 1, {NETHER, 0}, RATE },
{ ETH_InErrors, 1, {NETHER, 0}, INSTANT },
{ ETH_InErrors, 1, {NETHER, 0}, RATE },
{ ETH_etherAlignErrors, 1, {NETHER, 0}, INSTANT },
{ ETH_etherAlignErrors, 1, {NETHER, 0}, RATE },
{ ETH_etherCRCerrors, 1, {NETHER, 0}, INSTANT },
{ ETH_etherCRCerrors, 1, {NETHER, 0}, RATE },
{ ETH_etherOverrunErrors, 1, {NETHER, 0}, INSTANT },
{ ETH_etherOverrunErrors, 1, {NETHER, 0}, RATE },
{ ETH_etherUnderrunErrors, 1, {NETHER, 0}, INSTANT },
{ ETH_etherUnderrunErrors, 1, {NETHER, 0}, RATE },
{ ETH_etherMissedPkts, 1, {NETHER, 0}, INSTANT },
{ ETH_etherMissedPkts, 1, {NETHER, 0}, RATE },
{ ETH_InDiscards, 1, {NETHER, 0}, INSTANT },
{ ETH_InDiscards, 1, {NETHER, 0}, RATE },
{ ETH_etherReadqFull, 1, {NETHER, 0}, INSTANT },
{ ETH_etherReadqFull, 1, {NETHER, 0}, RATE },
{ ETH_etherRcvResources, 1, {NETHER, 0}, INSTANT },
{ ETH_etherRcvResources, 1, {NETHER, 0}, RATE },
{ ETH_etherCollisions, 1, {NETHER, 0}, INSTANT },
{ ETH_etherCollisions, 1, {NETHER, 0}, RATE },
{ ETH_OutDiscards, 1, {NETHER, 0}, INSTANT },
{ ETH_OutDiscards, 1, {NETHER, 0}, RATE },
{ ETH_OutErrors, 1, {NETHER, 0}, INSTANT },
{ ETH_OutErrors, 1, {NETHER, 0}, RATE },
{ ETH_etherAbortErrors, 1, {NETHER, 0}, INSTANT },
{ ETH_etherAbortErrors, 1, {NETHER, 0}, RATE },
{ ETH_etherCarrierLost, 1, {NETHER, 0}, INSTANT },
{ ETH_etherCarrierLost, 1, {NETHER, 0}, RATE },
{ ETH_OutQlen, 1, {NETHER, 0}, INSTANT },
{ IPR_total, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_badsum, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_badsum, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_tooshort, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_tooshort, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_toosmall, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_toosmall, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_badhlen, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_badhlen, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_badlen, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_badlen, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_unknownproto, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_unknownproto, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_fragments, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_fragments, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_fragdropped, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_fragdropped, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_fragtimeout, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_fragtimeout, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_reasms, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_reasms, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_forward, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_forward, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_cantforward, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_cantforward, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_noroutes, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_noroutes, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_redirectsent, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_redirectsent, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_inerrors, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_inerrors, 0, {MAS_SYSTEM, 0}, RATE },
{ IPR_indelivers, 0, {MAS_SYSTEM, 0}, RATE },
{ IPR_outrequests, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_outerrors, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_outerrors, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_pfrags, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_pfrags, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_fragfails, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_fragfails, 0, {MAS_SYSTEM, 0}, RATE },
{ IP_frags, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPR_frags, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_error, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_error, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_oldicmp, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_oldicmp, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist0, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist0, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist3, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist3, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist4, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist4, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist5, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist5, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist8, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist8, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist11, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist11, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist12, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist12, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist13, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist13, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist14, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist14, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist15, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist15, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist16, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist16, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist17, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist17, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outhist18, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outhist18, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_badcode, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_badcode, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_tooshort, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_tooshort, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_checksum, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_checksum, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_badlen, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_badlen, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist0, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist0, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist3, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist3, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist4, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist4, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist5, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist5, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist8, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist8, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist11, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist11, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist12, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist12, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist13, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist13, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist14, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist14, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist15, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist15, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist16, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist16, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist17, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist17, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_inhist18, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_inhist18, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_reflect, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_reflect, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMPR_intotal, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMPR_outtotal, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMP_outerrors, 0, {MAS_SYSTEM, 0}, INSTANT },
{ ICMPR_outerrors, 0, {MAS_SYSTEM, 0}, RATE },
{ TCPR_sndtotal, 0, {MAS_SYSTEM, 0}, RATE },
{ TCPR_sndpack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCPR_sndbyte, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndrexmitpack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndrexmitpack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndrexmitbyte, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndrexmitbyte, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndacks, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndacks, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_delack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_delack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndurg, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndurg, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndprobe, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndprobe, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndwinup, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndwinup, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndctrl, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndctrl, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_sndrsts, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_sndrsts, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvtotal, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvtotal, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvackpack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvackpack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvackbyte, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvackbyte, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvdupack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvdupack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvacktoomuch, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvacktoomuch, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvpack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvpack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvbyte, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvbyte, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvduppack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvduppack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvdupbyte, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvdupbyte, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvpartduppack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvpartduppack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvpartdupbyte, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvpartdupbyte, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvoopack, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvoopack, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvoobyte, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvoobyte, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvpackafterwin, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvpackafterwin, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvbyteafterwin, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvbyteafterwin, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvwinprobe, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvwinprobe, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvwinupd, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvwinupd, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvafterclose, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvafterclose, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvbadsum, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvbadsum, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvbadoff, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvbadoff, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rcvshort, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rcvshort, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_inerrors, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_inerrors, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_connattempt, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_connattempt, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_accepts, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_accepts, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_connects, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_connects, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_closed, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_closed, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_drops, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_drops, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_conndrops, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_conndrops, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_attemptfails, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_attemptfails, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_estabresets, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_estabresets, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rttupdated, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rttupdated, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_segstimed, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_segstimed, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_rexmttimeo, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_rexmttimeo, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_timeoutdrop, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_timeoutdrop, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_persisttimeo, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_persisttimeo, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_keeptimeo, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_keeptimeo, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_keepprobe, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_keepprobe, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_keepdrops, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_keepdrops, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_linger, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_linger, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_lingerexp, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_lingerexp, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_lingercan, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_lingercan, 0, {MAS_SYSTEM, 0}, RATE },
{ TCP_lingerabort, 0, {MAS_SYSTEM, 0}, INSTANT },
{ TCPR_lingerabort, 0, {MAS_SYSTEM, 0}, RATE },
{ UDP_hdrops, 0, {MAS_SYSTEM, 0}, INSTANT },
{ UDPR_hdrops, 0, {MAS_SYSTEM, 0}, RATE },
{ UDP_badlen, 0, {MAS_SYSTEM, 0}, INSTANT },
{ UDPR_badlen, 0, {MAS_SYSTEM, 0}, RATE },
{ UDP_badsum, 0, {MAS_SYSTEM, 0}, INSTANT },
{ UDPR_badsum, 0, {MAS_SYSTEM, 0}, RATE },
{ UDP_fullsock, 0, {MAS_SYSTEM, 0}, INSTANT },
{ UDPR_fullsock, 0, {MAS_SYSTEM, 0}, RATE },
{ UDP_noports, 0, {MAS_SYSTEM, 0}, INSTANT },
{ UDPR_noports, 0, {MAS_SYSTEM, 0}, RATE },
{ UDPR_indelivers, 0, {MAS_SYSTEM, 0}, RATE },
{ UDP_inerrors, 0, {MAS_SYSTEM, 0}, INSTANT },
{ UDPR_inerrors, 0, {MAS_SYSTEM, 0}, RATE },
{ UDPR_outtotal, 0, {MAS_SYSTEM, 0}, RATE },
{ IPR_sum, 0, {MAS_SYSTEM, 0}, RATE },
{ ICMPR_sum, 0, {MAS_SYSTEM, 0}, RATE },
{ UDPR_sum, 0, {MAS_SYSTEM, 0}, RATE },
{ TCPR_sum, 0, {MAS_SYSTEM, 0}, RATE },
{ NETERR_sum, 0, {MAS_SYSTEM, 0}, INSTANT },
{ NETERRR_sum, 0, {MAS_SYSTEM, 0}, RATE },
{ RCACHE_PERCENT, 1, {NCPU, 0}, INSTANT },
{ WCACHE_PERCENT, 1, {NCPU, 0}, INSTANT },
{ MEM_PERCENT, 0, {MAS_SYSTEM, 0}, INSTANT },
{ MEM_SWAP_PERCENT, 0, {MAS_SYSTEM, 0}, INSTANT },
{ DSK_SWAP_PERCENT, 0, {MAS_SYSTEM, 0}, INSTANT },
{ DSK_SWAPPG, 0, {MAS_SYSTEM, 0}, INSTANT },
{ DSK_SWAPPGFREE, 0, {MAS_SYSTEM, 0}, INSTANT },
{ MEM_SWAPPG, 0, {MAS_SYSTEM, 0}, NONE },
{ TOTALMEM, 0, {MAS_SYSTEM, 0}, INSTANT },

{ STR_STREAM_INUSE, 1, {NCPU, 0}, INSTANT },
{ STR_STREAM_TOTAL, 1, {NCPU, 0}, RATE },
{ STR_QUEUE_INUSE, 1, {NCPU, 0}, INSTANT },
{ STR_STREAM_TOTAL, 1, {NCPU, 0}, RATE },
{ STR_MDBBLK_INUSE, 1, {NCPU, 0}, INSTANT },
{ STR_MDBBLK_TOTAL, 1, {NCPU, 0}, RATE },
{ STR_MSGBLK_INUSE, 1, {NCPU, 0}, INSTANT },
{ STR_MSGBLK_TOTAL, 1, {NCPU, 0}, RATE },
{ STR_LINK_INUSE, 1, {NCPU, 0}, INSTANT },
{ STR_LINK_TOTAL, 1, {NCPU, 0}, RATE },
{ STR_EVENT_INUSE, 1, {NCPU, 0}, INSTANT },
{ STR_EVENT_TOTAL, 1, {NCPU, 0}, RATE },
{ STR_EVENT_FAIL, 1, {NCPU, 0}, INSTANT },
{ STR_EVENT_FAIL, 1, {NCPU, 0}, RATE },
{ SAP_total_servers, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_unused, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_Lans, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_TotalInSaps, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_GSQReceived, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_GSRReceived, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_NSQReceived, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_SASReceived, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_SNCReceived, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_GSIReceived, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_NotNeighbor, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_NotNeighbor, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_EchoMyOutput, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_EchoMyOutput, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_BadSizeInSaps, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_BadSizeInSaps, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_BadSapSource, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_BadSapSource, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_TotalOutSaps, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_NSRSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_GSRSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_GSQSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_SASAckSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_SASNackSent, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_SASNackSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_SNCAckSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_SNCNackSent, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_SNCNackSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_GSIAckSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_BadDestOutSaps, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_BadDestOutSaps, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_SrvAllocFailed, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_SrvAllocFailed, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_MallocFailed, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_MallocFailed, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_TotalInRipSaps, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_TotalInRipSaps, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_BadRipSaps, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_BadRipSaps, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_RipServerDown, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SAP_RipServerDown, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_ProcessesToNotify, 0, {MAS_SYSTEM, 0}, RATE },
{ SAP_NotificationsSent, 0, {MAS_SYSTEM, 0}, RATE },
{ SAPLAN_Network, 1, {SAP_Lans, 0}, NONE },
{ SAPLAN_LanNumber, 1, {SAP_Lans, 0}, NONE },
{ SAPLAN_UpdateInterval, 1, {SAP_Lans, 0}, NONE },
{ SAPLAN_AgeFactor, 1, {SAP_Lans, 0}, NONE },
{ SAPLAN_PacketGap, 1, {SAP_Lans, 0}, NONE },
{ SAPLAN_PacketSize, 1, {SAP_Lans, 0}, MEAN },
{ SAPLAN_PacketsSent, 1, {SAP_Lans, 0}, RATE },
{ SAPLAN_PacketsReceived, 1, {SAP_Lans, 0}, RATE },
{ SAPLAN_BadPktsReceived, 1, {SAP_Lans, 0}, INSTANT },
{ SAPLAN_BadPktsReceived, 1, {SAP_Lans, 0}, RATE },
{ IPXLAN_InProtoSize, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InProtoSize, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBadDLPItype, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InBadDLPItype, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InCoalesced, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InCoalesced, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InPropagation, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InNoPropagate, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InNoPropagate, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InTotal, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBadLength, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InBadLength, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InDriverEcho, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InDriverEcho, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InRip, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InRipDropped, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InRipDropped, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InRipRouted, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InSap, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InSapBad, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InSapBad, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InSapIpx, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InSapNoIpxToSapd, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InSapNoIpxDrop, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InSapNoIpxDrop, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InDiag, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InDiag, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InDiagInternal, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InDiagInternal, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InDiagNIC, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InDiagNIC, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InDiagIpx, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InDiagIpx, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InDiagNoIpx, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InDiagNoIpx, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InNICDropped, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InNICDropped, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcast, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcastInternal, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcastNIC, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcastDiag, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InBroadcastDiag, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcastDiagFwd, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InBroadcastDiagFwd, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcastDiagRoute, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InBroadcastDiagRoute, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcastDiagResp, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InBroadcastDiagResp, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InBroadcastRoute, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_InBroadcastRoute, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InForward, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InRoute, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_InInternalNet, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutPropagation, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutTotalStream, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutTotal, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutSameSocket, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_OutSameSocket, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutFillInDest, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutInternal, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutBadLan, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_OutBadLan, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutSent, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_OutQueued, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_Ioctl, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlSetLans, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlGetLans, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlSetSapQ, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlSetLanInfo, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlGetLanInfo, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlGetNodeAddr, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlGetNetAddr, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlGetStats, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlLink, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlUnlink, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXLAN_IoctlUnknown, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXLAN_IoctlUnknown, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxInData, 0, {MAS_SYSTEM, 0}, RATE },
{ IPX_datapackets, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxOutData, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxOutBadSize, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxOutBadSize, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxOutToSwitch, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOutData, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOutBadState, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxTLIOutBadState, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOutBadSize, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxTLIOutBadSize, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOutBadOpt, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxTLIOutBadOpt, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOutHdrAlloc, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxTLIOutHdrAlloc, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOutToSwitch, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxBoundSockets, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxBoundSockets, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxBind, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIBind, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOptMgt, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIUnknown, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxTLIUnknown, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTLIOutBadAddr, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxTLIOutBadAddr, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSwitchInvalSocket, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxSwitchInvalSocket, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSwitchSumFail, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxSwitchSumFail, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSwitchAllocFail, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxSwitchAllocFail, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSwitchSum, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSwitchEven, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSwitchEvenAlloc, 0, {MAS_SYSTEM, 0}, RATE },
{ DUMMY_1, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxDataToSocket, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxTrimPacket, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSumFail, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxSumFail, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxBusySocket, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxBusySocket, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxSocketNotBound, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxSocketNotBound, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxRouted, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxRoutedTLI, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxRoutedTLIAlloc, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxRoutedTLIAlloc, 0, {MAS_SYSTEM, 0}, RATE },
{ IPX_sent_to_tli, 0, {MAS_SYSTEM, 0}, RATE },
{ IPX_total_ioctls, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxIoctlSetWater, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxIoctlBindSocket, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxIoctlUnbindSocket, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxIoctlStats, 0, {MAS_SYSTEM, 0}, RATE },
{ IPXSOCK_IpxIoctlUnknown, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxIoctlUnknown, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedPackets, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedNoLanKey, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_ReceivedNoLanKey, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedBadLength, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_ReceivedBadLength, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedCoalesced, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedNoCoalesce, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_ReceivedNoCoalesce, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedRequestPackets, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedResponsePackets, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ReceivedUnknownRequest, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_ReceivedUnknownRequest, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_total_router_packets_sent, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_SentAllocFailed, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_SentAllocFailed, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_SentBadDestination, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_SentBadDestination, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_SentRequestPackets, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_SentResponsePackets, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_SentLan0Dropped, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_SentLan0Dropped, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_SentLan0Routed, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_ioctls_processed, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlInitialize, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlGetHashSize, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlGetHashStats, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlDumpHashTable, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlGetRouterTable, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlGetNetInfo, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlCheckSapSource, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlResetRouter, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlDownRouter, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlStats, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_RipxIoctlUnknown, 0, {MAS_SYSTEM, 0}, INSTANT },
{ RIP_RipxIoctlUnknown, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_current_connections, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_current_connections, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_alloc_failures, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_alloc_failures, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_open_failures, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_open_failures, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_ioctls, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_connect_req_count, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_connect_req_fails, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_connect_req_fails, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_listen_req, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_listen_req_fails, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_listen_req_fails, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_send_mesg_count, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_unknown_mesg_count, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_unknown_mesg_count, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_send_bad_mesg, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_send_bad_mesg, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_send_packet_count, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_send_packet_timeout, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_send_packet_timeout, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_send_packet_nak, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_send_packet_nak, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_rcv_packet_count, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_rcv_bad_packet, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_rcv_bad_packet, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_rcv_bad_data_packet, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_rcv_bad_data_packet, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_rcv_dup_packet, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_rcv_dup_packet, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_rcv_packet_sentup, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_rcv_conn_req, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_rcv_conn_req, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_abort_connection, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_abort_connection, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_max_retries_abort, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_max_retries_abort, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_no_listeners, 0, {MAS_SYSTEM, 0}, INSTANT },
{ SPX_no_listeners, 0, {MAS_SYSTEM, 0}, RATE },
{ SPXCON_netaddr, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_nodeaddr, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_sockaddr, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_connection_id, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_o_netaddr, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_o_nodeaddr, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_o_sockaddr, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_o_connection_id, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_con_state, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_con_retry_count, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_retry_time, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_con_type, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_con_ipxChecksum, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_con_window_size, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_con_remote_window_size, 1, {SPX_max_connections, 0}, NONE },
{ SPXCON_con_send_packet_size, 1, {SPX_max_connections, 0}, MEAN },
{ SPXCON_con_rcv_packet_size, 1, {SPX_max_connections, 0}, MEAN },
{ SPXCON_con_round_trip_time, 1, {SPX_max_connections, 0}, MEAN },
{ SPXCON_con_window_choke, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_send_mesg_count, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_unknown_mesg_count, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_unknown_mesg_count, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_send_bad_mesg, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_send_bad_mesg, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_send_packet_count, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_send_packet_timeout, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_send_packet_timeout, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_send_packet_nak, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_send_packet_nak, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_send_ack, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_send_nak, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_send_nak, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_send_watchdog, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_packet_count, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_bad_packet, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_rcv_bad_packet, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_bad_data_packet, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_rcv_bad_data_packet, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_dup_packet, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_rcv_dup_packet, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_packet_outseq, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_rcv_packet_outseq, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_packet_sentup, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_packet_qued, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_ack, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_nak, 1, {SPX_max_connections, 0}, INSTANT },
{ SPXCON_con_rcv_nak, 1, {SPX_max_connections, 0}, RATE },
{ SPXCON_con_rcv_watchdog, 1, {SPX_max_connections, 0}, RATE },
{ SAP_total, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_total, 0, {MAS_SYSTEM, 0}, RATE },
{ IPX_total, 0, {MAS_SYSTEM, 0}, RATE },
{ RIP_total, 0, {MAS_SYSTEM, 0}, RATE },
{ NETWARE_errs, 0, {MAS_SYSTEM, 0}, INSTANT },
{ NETWARE_errs, 0, {MAS_SYSTEM, 0}, RATE },
{ SPX_max_used_connections, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxInBadSize, 0, {MAS_SYSTEM, 0}, INSTANT },
{ IPXSOCK_IpxInBadSize, 0, {MAS_SYSTEM, 0}, RATE },
{ EISA_BUS_UTIL_SUMCNT, 0, {MAS_SYSTEM, 0}, INSTANT },
{ EISA_BUS_UTIL_PERCENT, 0, {MAS_SYSTEM, 0}, INSTANT },
{ LBOLT, 0, {MAS_SYSTEM, 0}, INSTANT },
};
/*
 *	function: 	set_nmets
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	sets the value of nmets to be the size of mettbl
 *	normally this would be coded as sizeof(tbl)/sizeof(struct),
 *	but we need to export the value to metcook.c
 */
void
set_nmets() {
	nmets = sizeof( mettbl ) / sizeof( struct metric );
}
/*
 *	function: 	set_met_titles
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	initialize the name portion of mettbl.  This is done at
 *	run time to accommodate internationalization.
 */
void
set_met_titles() {
	mettbl[ HZ_IDX ].title = HZ_TITLE;
	mettbl[ NCPU_IDX ].title = NCPU_TITLE;
	mettbl[ NDISK_IDX ].title = NDISK_TITLE;
	mettbl[ NFSTYP_IDX ].title = NFSTYP_TITLE;
	mettbl[ FSNAMES_IDX ].title = FSNAMES_TITLE;
	mettbl[ KMPOOLS_IDX ].title = KMPOOLS_TITLE;
	mettbl[ KMASIZE_IDX ].title = KMASIZE_TITLE;
	mettbl[ PGSZ_IDX ].title = PGSZ_TITLE;
	mettbl[ DS_NAME_IDX ].title = DS_NAME_TITLE;
	mettbl[ NETHER_IDX ].title = NETHER_TITLE;
	mettbl[ ETHNAME_IDX ].title = ETHNAME_TITLE;
	mettbl[ USR_TIME_IDX ].title = USR_TIME_TITLE;
	mettbl[ SYS_TIME_IDX ].title = SYS_TIME_TITLE;
	mettbl[ WIO_TIME_IDX ].title = WIO_TIME_TITLE;
	mettbl[ IDL_TIME_IDX ].title = IDL_TIME_TITLE;
	mettbl[ IGET_IDX ].title = IGET_TITLE;
	mettbl[ DIRBLK_IDX ].title = DIRBLK_TITLE;
	mettbl[ IPAGE_IDX ].title = IPAGE_TITLE;
	mettbl[ INOPAGE_IDX ].title = INOPAGE_TITLE;
	mettbl[ FREEMEM_IDX ].title = FREEMEM_TITLE;
	mettbl[ FREESWAP_IDX ].title = FREESWAP_TITLE;
	mettbl[ FSWIO_IDX ].title = FSWIO_TITLE;
	mettbl[ PHYSWIO_IDX ].title = PHYSWIO_TITLE;
	mettbl[ RUNQ_IDX ].title = RUNQ_TITLE;
	mettbl[ RUNOCC_IDX ].title = RUNOCC_TITLE;
	mettbl[ SWPQ_IDX ].title = SWPQ_TITLE;
	mettbl[ SWPOCC_IDX ].title = SWPOCC_TITLE;
	mettbl[ PROCFAIL_IDX ].title = PROCFAIL_TITLE;
	mettbl[ PROCFAILR_IDX ].title = PROCFAILR_TITLE;
	mettbl[ PROCUSE_IDX ].title = PROCUSE_TITLE;
	mettbl[ PROCMAX_IDX ].title = PROCMAX_TITLE;
	mettbl[ KMEM_MEM_IDX ].title = KMEM_MEM_TITLE;
	mettbl[ KMEM_BALLOC_IDX ].title = KMEM_BALLOC_TITLE;
	mettbl[ KMEM_RALLOC_IDX ].title = KMEM_RALLOC_TITLE;
	mettbl[ KMEM_FAIL_IDX ].title = KMEM_FAIL_TITLE;
	mettbl[ KMEMR_FAIL_IDX ].title = KMEMR_FAIL_TITLE;
	mettbl[ PREATCH_IDX ].title = PREATCH_TITLE;
	mettbl[ ATCH_IDX ].title = ATCH_TITLE;
	mettbl[ ATCHFREE_IDX ].title = ATCHFREE_TITLE;
	mettbl[ ATCHFREE_PGOUT_IDX ].title = ATCHFREE_PGOUT_TITLE;
	mettbl[ ATCHMISS_IDX ].title = ATCHMISS_TITLE;
	mettbl[ PGIN_IDX ].title = PGIN_TITLE;
	mettbl[ PGPGIN_IDX ].title = PGPGIN_TITLE;
	mettbl[ PGOUT_IDX ].title = PGOUT_TITLE;
	mettbl[ PGPGOUT_IDX ].title = PGPGOUT_TITLE;
	mettbl[ SWPOUT_IDX ].title = SWPOUT_TITLE;
	mettbl[ PPGSWPOUT_IDX ].title = PPGSWPOUT_TITLE;
	mettbl[ VPGSWPOUT_IDX ].title = VPGSWPOUT_TITLE;
	mettbl[ SWPIN_IDX ].title = SWPIN_TITLE;
	mettbl[ PGSWPIN_IDX ].title = PGSWPIN_TITLE;
	mettbl[ VIRSCAN_IDX ].title = VIRSCAN_TITLE;
	mettbl[ VIRFREE_IDX ].title = VIRFREE_TITLE;
	mettbl[ PHYSFREE_IDX ].title = PHYSFREE_TITLE;
	mettbl[ PFAULT_IDX ].title = PFAULT_TITLE;
	mettbl[ VFAULT_IDX ].title = VFAULT_TITLE;
	mettbl[ SFTLCK_IDX ].title = SFTLCK_TITLE;
	mettbl[ SYSCALL_IDX ].title = SYSCALL_TITLE;
	mettbl[ FORK_IDX ].title = FORK_TITLE;
	mettbl[ LWPCREATE_IDX ].title = LWPCREATE_TITLE;
	mettbl[ EXEC_IDX ].title = EXEC_TITLE;
	mettbl[ READ_IDX ].title = READ_TITLE;
	mettbl[ WRITE_IDX ].title = WRITE_TITLE;
	mettbl[ READCH_IDX ].title = READCH_TITLE;
	mettbl[ WRITECH_IDX ].title = WRITECH_TITLE;
	mettbl[ LOOKUP_IDX ].title = LOOKUP_TITLE;
	mettbl[ DNLCHITS_IDX ].title = DNLCHITS_TITLE;
	mettbl[ DNLCMISS_IDX ].title = DNLCMISS_TITLE;
	mettbl[ FILETBLINUSE_IDX ].title = FILETBLINUSE_TITLE;
	mettbl[ FILETBLFAIL_IDX ].title = FILETBLFAIL_TITLE;
	mettbl[ FILETBLFAILR_IDX ].title = FILETBLFAILR_TITLE;
	mettbl[ FLCKTBLMAX_IDX ].title = FLCKTBLMAX_TITLE;
	mettbl[ FLCKTBLINUSE_IDX ].title = FLCKTBLINUSE_TITLE;
	mettbl[ FLCKTBLFAIL_IDX ].title = FLCKTBLFAIL_TITLE;
	mettbl[ FLCKTBLFAILR_IDX ].title = FLCKTBLFAILR_TITLE;
	mettbl[ FLCKTBLTOTAL_IDX ].title = FLCKTBLTOTAL_TITLE;
	mettbl[ MAXINODE_IDX ].title = MAXINODE_TITLE;
	mettbl[ CURRINODE_IDX ].title = CURRINODE_TITLE;
	mettbl[ INUSEINODE_IDX ].title = INUSEINODE_TITLE;
	mettbl[ FAILINODE_IDX ].title = FAILINODE_TITLE;
	mettbl[ FAILINODER_IDX ].title = FAILINODER_TITLE;
	mettbl[ MPS_PSWITCH_IDX ].title = MPS_PSWITCH_TITLE;
	mettbl[ MPS_RUNQUE_IDX ].title = MPS_RUNQUE_TITLE;
	mettbl[ MPS_RUNOCC_IDX ].title = MPS_RUNOCC_TITLE;
	mettbl[ MPB_BREAD_IDX ].title = MPB_BREAD_TITLE;
	mettbl[ MPB_BWRITE_IDX ].title = MPB_BWRITE_TITLE;
	mettbl[ MPB_LREAD_IDX ].title = MPB_LREAD_TITLE;
	mettbl[ MPB_LWRITE_IDX ].title = MPB_LWRITE_TITLE;
	mettbl[ MPB_PHREAD_IDX ].title = MPB_PHREAD_TITLE;
	mettbl[ MPB_PHWRITE_IDX ].title = MPB_PHWRITE_TITLE;
	mettbl[ MPT_RCVINT_IDX ].title = MPT_RCVINT_TITLE;
	mettbl[ MPT_XMTINT_IDX ].title = MPT_XMTINT_TITLE;
	mettbl[ MPT_MDMINT_IDX ].title = MPT_MDMINT_TITLE;
	mettbl[ MPT_RAWCH_IDX ].title = MPT_RAWCH_TITLE;
	mettbl[ MPT_CANCH_IDX ].title = MPT_CANCH_TITLE;
	mettbl[ MPT_OUTCH_IDX ].title = MPT_OUTCH_TITLE;
	mettbl[ MPI_MSG_IDX ].title = MPI_MSG_TITLE;
	mettbl[ MPI_SEMA_IDX ].title = MPI_SEMA_TITLE;
	mettbl[ MPR_LWP_FAIL_IDX ].title = MPR_LWP_FAIL_TITLE;
	mettbl[ MPR_LWP_FAILR_IDX ].title = MPR_LWP_FAILR_TITLE;
	mettbl[ MPR_LWP_USE_IDX ].title = MPR_LWP_USE_TITLE;
	mettbl[ MPR_LWP_MAX_IDX ].title = MPR_LWP_MAX_TITLE;
	mettbl[ DS_CYLS_IDX ].title = DS_CYLS_TITLE;
	mettbl[ DS_FLAGS_IDX ].title = DS_FLAGS_TITLE;
	mettbl[ DS_QLEN_IDX ].title = DS_QLEN_TITLE;
	mettbl[ DS_ACTIVE_IDX ].title = DS_ACTIVE_TITLE;
	mettbl[ DS_RESP_IDX ].title = DS_RESP_TITLE;
	mettbl[ DS_READ_IDX ].title = DS_READ_TITLE;
	mettbl[ DS_READBLK_IDX ].title = DS_READBLK_TITLE;
	mettbl[ DS_WRITE_IDX ].title = DS_WRITE_TITLE;
	mettbl[ DS_WRITEBLK_IDX ].title = DS_WRITEBLK_TITLE;
	mettbl[ TOT_CPU_IDX ].title = TOT_CPU_TITLE;
	mettbl[ TOT_IDL_IDX ].title = TOT_IDL_TITLE;
	mettbl[ LWP_SLEEP_IDX ].title = LWP_SLEEP_TITLE;
	mettbl[ LWP_RUN_IDX ].title = LWP_RUN_TITLE;
	mettbl[ LWP_IDLE_IDX ].title = LWP_IDLE_TITLE;
	mettbl[ LWP_ONPROC_IDX ].title = LWP_ONPROC_TITLE;
	mettbl[ LWP_ZOMB_IDX ].title = LWP_ZOMB_TITLE;
	mettbl[ LWP_STOP_IDX ].title = LWP_STOP_TITLE;
	mettbl[ LWP_OTHER_IDX ].title = LWP_OTHER_TITLE;
	mettbl[ LWP_TOTAL_IDX ].title = LWP_TOTAL_TITLE;
	mettbl[ LWP_NPROC_IDX ].title = LWP_NPROC_TITLE;
	mettbl[ TOT_RW_IDX ].title = TOT_RW_TITLE;
	mettbl[ TOT_KRWCH_IDX ].title = TOT_KRWCH_TITLE;
	mettbl[ DNLC_PERCENT_IDX ].title = DNLC_PERCENT_TITLE;
	mettbl[ TOT_KMA_PAGES_IDX ].title = TOT_KMA_PAGES_TITLE;
	mettbl[ ETH_InUcastPkts_IDX ].title = ETH_InUcastPkts_TITLE;
	mettbl[ ETH_OutUcastPkts_IDX ].title = ETH_OutUcastPkts_TITLE;
	mettbl[ ETH_InNUcastPkts_IDX ].title = ETH_InNUcastPkts_TITLE;
	mettbl[ ETH_OutNUcastPkts_IDX ].title = ETH_OutNUcastPkts_TITLE;
	mettbl[ ETH_InOctets_IDX ].title = ETH_InOctets_TITLE;
	mettbl[ ETH_OutOctets_IDX ].title = ETH_OutOctets_TITLE;
	mettbl[ ETH_InErrors_IDX ].title = ETH_InErrors_TITLE;
	mettbl[ ETHR_InErrors_IDX ].title = ETHR_InErrors_TITLE;
	mettbl[ ETH_etherAlignErrors_IDX ].title = ETH_etherAlignErrors_TITLE;
	mettbl[ ETHR_etherAlignErrors_IDX ].title = ETHR_etherAlignErrors_TITLE;
	mettbl[ ETH_etherCRCerrors_IDX ].title = ETH_etherCRCerrors_TITLE;
	mettbl[ ETHR_etherCRCerrors_IDX ].title = ETHR_etherCRCerrors_TITLE;
	mettbl[ ETH_etherOverrunErrors_IDX ].title = ETH_etherOverrunErrors_TITLE;
	mettbl[ ETHR_etherOverrunErrors_IDX ].title = ETHR_etherOverrunErrors_TITLE;
	mettbl[ ETH_etherUnderrunErrors_IDX ].title = ETH_etherUnderrunErrors_TITLE;
	mettbl[ ETHR_etherUnderrunErrors_IDX ].title = ETHR_etherUnderrunErrors_TITLE;
	mettbl[ ETH_etherMissedPkts_IDX ].title = ETH_etherMissedPkts_TITLE;
	mettbl[ ETHR_etherMissedPkts_IDX ].title = ETHR_etherMissedPkts_TITLE;
	mettbl[ ETH_InDiscards_IDX ].title = ETH_InDiscards_TITLE;
	mettbl[ ETHR_InDiscards_IDX ].title = ETHR_InDiscards_TITLE;
	mettbl[ ETH_etherReadqFull_IDX ].title = ETH_etherReadqFull_TITLE;
	mettbl[ ETHR_etherReadqFull_IDX ].title = ETHR_etherReadqFull_TITLE;
	mettbl[ ETH_etherRcvResources_IDX ].title = ETH_etherRcvResources_TITLE;
	mettbl[ ETHR_etherRcvResources_IDX ].title = ETHR_etherRcvResources_TITLE;
	mettbl[ ETH_etherCollisions_IDX ].title = ETH_etherCollisions_TITLE;
	mettbl[ ETHR_etherCollisions_IDX ].title = ETHR_etherCollisions_TITLE;
	mettbl[ ETH_OutDiscards_IDX ].title = ETH_OutDiscards_TITLE;
	mettbl[ ETHR_OutDiscards_IDX ].title = ETHR_OutDiscards_TITLE;
	mettbl[ ETH_OutErrors_IDX ].title = ETH_OutErrors_TITLE;
	mettbl[ ETHR_OutErrors_IDX ].title = ETHR_OutErrors_TITLE;
	mettbl[ ETH_etherAbortErrors_IDX ].title = ETH_etherAbortErrors_TITLE;
	mettbl[ ETHR_etherAbortErrors_IDX ].title = ETHR_etherAbortErrors_TITLE;
	mettbl[ ETH_etherCarrierLost_IDX ].title = ETH_etherCarrierLost_TITLE;
	mettbl[ ETHR_etherCarrierLost_IDX ].title = ETHR_etherCarrierLost_TITLE;
	mettbl[ ETH_OutQlen_IDX ].title = ETH_OutQlen_TITLE;
	mettbl[ IPR_total_IDX ].title = IPR_total_TITLE;
	mettbl[ IP_badsum_IDX ].title = IP_badsum_TITLE;
	mettbl[ IPR_badsum_IDX ].title = IPR_badsum_TITLE;
	mettbl[ IP_tooshort_IDX ].title = IP_tooshort_TITLE;
	mettbl[ IPR_tooshort_IDX ].title = IPR_tooshort_TITLE;
	mettbl[ IP_toosmall_IDX ].title = IP_toosmall_TITLE;
	mettbl[ IPR_toosmall_IDX ].title = IPR_toosmall_TITLE;
	mettbl[ IP_badhlen_IDX ].title = IP_badhlen_TITLE;
	mettbl[ IPR_badhlen_IDX ].title = IPR_badhlen_TITLE;
	mettbl[ IP_badlen_IDX ].title = IP_badlen_TITLE;
	mettbl[ IPR_badlen_IDX ].title = IPR_badlen_TITLE;
	mettbl[ IP_unknownproto_IDX ].title = IP_unknownproto_TITLE;
	mettbl[ IPR_unknownproto_IDX ].title = IPR_unknownproto_TITLE;
	mettbl[ IP_fragments_IDX ].title = IP_fragments_TITLE;
	mettbl[ IPR_fragments_IDX ].title = IPR_fragments_TITLE;
	mettbl[ IP_fragdropped_IDX ].title = IP_fragdropped_TITLE;
	mettbl[ IPR_fragdropped_IDX ].title = IPR_fragdropped_TITLE;
	mettbl[ IP_fragtimeout_IDX ].title = IP_fragtimeout_TITLE;
	mettbl[ IPR_fragtimeout_IDX ].title = IPR_fragtimeout_TITLE;
	mettbl[ IP_reasms_IDX ].title = IP_reasms_TITLE;
	mettbl[ IPR_reasms_IDX ].title = IPR_reasms_TITLE;
	mettbl[ IP_forward_IDX ].title = IP_forward_TITLE;
	mettbl[ IPR_forward_IDX ].title = IPR_forward_TITLE;
	mettbl[ IP_cantforward_IDX ].title = IP_cantforward_TITLE;
	mettbl[ IPR_cantforward_IDX ].title = IPR_cantforward_TITLE;
	mettbl[ IP_noroutes_IDX ].title = IP_noroutes_TITLE;
	mettbl[ IPR_noroutes_IDX ].title = IPR_noroutes_TITLE;
	mettbl[ IP_redirectsent_IDX ].title = IP_redirectsent_TITLE;
	mettbl[ IPR_redirectsent_IDX ].title = IPR_redirectsent_TITLE;
	mettbl[ IP_inerrors_IDX ].title = IP_inerrors_TITLE;
	mettbl[ IPR_inerrors_IDX ].title = IPR_inerrors_TITLE;
	mettbl[ IPR_indelivers_IDX ].title = IPR_indelivers_TITLE;
	mettbl[ IPR_outrequests_IDX ].title = IPR_outrequests_TITLE;
	mettbl[ IP_outerrors_IDX ].title = IP_outerrors_TITLE;
	mettbl[ IPR_outerrors_IDX ].title = IPR_outerrors_TITLE;
	mettbl[ IP_pfrags_IDX ].title = IP_pfrags_TITLE;
	mettbl[ IPR_pfrags_IDX ].title = IPR_pfrags_TITLE;
	mettbl[ IP_fragfails_IDX ].title = IP_fragfails_TITLE;
	mettbl[ IPR_fragfails_IDX ].title = IPR_fragfails_TITLE;
	mettbl[ IP_frags_IDX ].title = IP_frags_TITLE;
	mettbl[ IPR_frags_IDX ].title = IPR_frags_TITLE;
	mettbl[ ICMP_error_IDX ].title = ICMP_error_TITLE;
	mettbl[ ICMPR_error_IDX ].title = ICMPR_error_TITLE;
	mettbl[ ICMP_oldicmp_IDX ].title = ICMP_oldicmp_TITLE;
	mettbl[ ICMPR_oldicmp_IDX ].title = ICMPR_oldicmp_TITLE;
	mettbl[ ICMP_outhist0_IDX ].title = ICMP_outhist0_TITLE;
	mettbl[ ICMPR_outhist0_IDX ].title = ICMPR_outhist0_TITLE;
	mettbl[ ICMP_outhist3_IDX ].title = ICMP_outhist3_TITLE;
	mettbl[ ICMPR_outhist3_IDX ].title = ICMPR_outhist3_TITLE;
	mettbl[ ICMP_outhist4_IDX ].title = ICMP_outhist4_TITLE;
	mettbl[ ICMPR_outhist4_IDX ].title = ICMPR_outhist4_TITLE;
	mettbl[ ICMP_outhist5_IDX ].title = ICMP_outhist5_TITLE;
	mettbl[ ICMPR_outhist5_IDX ].title = ICMPR_outhist5_TITLE;
	mettbl[ ICMP_outhist8_IDX ].title = ICMP_outhist8_TITLE;
	mettbl[ ICMPR_outhist8_IDX ].title = ICMPR_outhist8_TITLE;
	mettbl[ ICMP_outhist11_IDX ].title = ICMP_outhist11_TITLE;
	mettbl[ ICMPR_outhist11_IDX ].title = ICMPR_outhist11_TITLE;
	mettbl[ ICMP_outhist12_IDX ].title = ICMP_outhist12_TITLE;
	mettbl[ ICMPR_outhist12_IDX ].title = ICMPR_outhist12_TITLE;
	mettbl[ ICMP_outhist13_IDX ].title = ICMP_outhist13_TITLE;
	mettbl[ ICMPR_outhist13_IDX ].title = ICMPR_outhist13_TITLE;
	mettbl[ ICMP_outhist14_IDX ].title = ICMP_outhist14_TITLE;
	mettbl[ ICMPR_outhist14_IDX ].title = ICMPR_outhist14_TITLE;
	mettbl[ ICMP_outhist15_IDX ].title = ICMP_outhist15_TITLE;
	mettbl[ ICMPR_outhist15_IDX ].title = ICMPR_outhist15_TITLE;
	mettbl[ ICMP_outhist16_IDX ].title = ICMP_outhist16_TITLE;
	mettbl[ ICMPR_outhist16_IDX ].title = ICMPR_outhist16_TITLE;
	mettbl[ ICMP_outhist17_IDX ].title = ICMP_outhist17_TITLE;
	mettbl[ ICMPR_outhist17_IDX ].title = ICMPR_outhist17_TITLE;
	mettbl[ ICMP_outhist18_IDX ].title = ICMP_outhist18_TITLE;
	mettbl[ ICMPR_outhist18_IDX ].title = ICMPR_outhist18_TITLE;
	mettbl[ ICMP_badcode_IDX ].title = ICMP_badcode_TITLE;
	mettbl[ ICMPR_badcode_IDX ].title = ICMPR_badcode_TITLE;
	mettbl[ ICMP_tooshort_IDX ].title = ICMP_tooshort_TITLE;
	mettbl[ ICMPR_tooshort_IDX ].title = ICMPR_tooshort_TITLE;
	mettbl[ ICMP_checksum_IDX ].title = ICMP_checksum_TITLE;
	mettbl[ ICMPR_checksum_IDX ].title = ICMPR_checksum_TITLE;
	mettbl[ ICMP_badlen_IDX ].title = ICMP_badlen_TITLE;
	mettbl[ ICMPR_badlen_IDX ].title = ICMPR_badlen_TITLE;
	mettbl[ ICMP_inhist0_IDX ].title = ICMP_inhist0_TITLE;
	mettbl[ ICMPR_inhist0_IDX ].title = ICMPR_inhist0_TITLE;
	mettbl[ ICMP_inhist3_IDX ].title = ICMP_inhist3_TITLE;
	mettbl[ ICMPR_inhist3_IDX ].title = ICMPR_inhist3_TITLE;
	mettbl[ ICMP_inhist4_IDX ].title = ICMP_inhist4_TITLE;
	mettbl[ ICMPR_inhist4_IDX ].title = ICMPR_inhist4_TITLE;
	mettbl[ ICMP_inhist5_IDX ].title = ICMP_inhist5_TITLE;
	mettbl[ ICMPR_inhist5_IDX ].title = ICMPR_inhist5_TITLE;
	mettbl[ ICMP_inhist8_IDX ].title = ICMP_inhist8_TITLE;
	mettbl[ ICMPR_inhist8_IDX ].title = ICMPR_inhist8_TITLE;
	mettbl[ ICMP_inhist11_IDX ].title = ICMP_inhist11_TITLE;
	mettbl[ ICMPR_inhist11_IDX ].title = ICMPR_inhist11_TITLE;
	mettbl[ ICMP_inhist12_IDX ].title = ICMP_inhist12_TITLE;
	mettbl[ ICMPR_inhist12_IDX ].title = ICMPR_inhist12_TITLE;
	mettbl[ ICMP_inhist13_IDX ].title = ICMP_inhist13_TITLE;
	mettbl[ ICMPR_inhist13_IDX ].title = ICMPR_inhist13_TITLE;
	mettbl[ ICMP_inhist14_IDX ].title = ICMP_inhist14_TITLE;
	mettbl[ ICMPR_inhist14_IDX ].title = ICMPR_inhist14_TITLE;
	mettbl[ ICMP_inhist15_IDX ].title = ICMP_inhist15_TITLE;
	mettbl[ ICMPR_inhist15_IDX ].title = ICMPR_inhist15_TITLE;
	mettbl[ ICMP_inhist16_IDX ].title = ICMP_inhist16_TITLE;
	mettbl[ ICMPR_inhist16_IDX ].title = ICMPR_inhist16_TITLE;
	mettbl[ ICMP_inhist17_IDX ].title = ICMP_inhist17_TITLE;
	mettbl[ ICMPR_inhist17_IDX ].title = ICMPR_inhist17_TITLE;
	mettbl[ ICMP_inhist18_IDX ].title = ICMP_inhist18_TITLE;
	mettbl[ ICMPR_inhist18_IDX ].title = ICMPR_inhist18_TITLE;
	mettbl[ ICMP_reflect_IDX ].title = ICMP_reflect_TITLE;
	mettbl[ ICMPR_reflect_IDX ].title = ICMPR_reflect_TITLE;
	mettbl[ ICMPR_intotal_IDX ].title = ICMPR_intotal_TITLE;
	mettbl[ ICMPR_outtotal_IDX ].title = ICMPR_outtotal_TITLE;
	mettbl[ ICMP_outerrors_IDX ].title = ICMP_outerrors_TITLE;
	mettbl[ ICMPR_outerrors_IDX ].title = ICMPR_outerrors_TITLE;
	mettbl[ TCPR_sndtotal_IDX ].title = TCPR_sndtotal_TITLE;
	mettbl[ TCPR_sndpack_IDX ].title = TCPR_sndpack_TITLE;
	mettbl[ TCPR_sndbyte_IDX ].title = TCPR_sndbyte_TITLE;
	mettbl[ TCP_sndrexmitpack_IDX ].title = TCP_sndrexmitpack_TITLE;
	mettbl[ TCPR_sndrexmitpack_IDX ].title = TCPR_sndrexmitpack_TITLE;
	mettbl[ TCP_sndrexmitbyte_IDX ].title = TCP_sndrexmitbyte_TITLE;
	mettbl[ TCPR_sndrexmitbyte_IDX ].title = TCPR_sndrexmitbyte_TITLE;
	mettbl[ TCP_sndacks_IDX ].title = TCP_sndacks_TITLE;
	mettbl[ TCPR_sndacks_IDX ].title = TCPR_sndacks_TITLE;
	mettbl[ TCP_delack_IDX ].title = TCP_delack_TITLE;
	mettbl[ TCPR_delack_IDX ].title = TCPR_delack_TITLE;
	mettbl[ TCP_sndurg_IDX ].title = TCP_sndurg_TITLE;
	mettbl[ TCPR_sndurg_IDX ].title = TCPR_sndurg_TITLE;
	mettbl[ TCP_sndprobe_IDX ].title = TCP_sndprobe_TITLE;
	mettbl[ TCPR_sndprobe_IDX ].title = TCPR_sndprobe_TITLE;
	mettbl[ TCP_sndwinup_IDX ].title = TCP_sndwinup_TITLE;
	mettbl[ TCPR_sndwinup_IDX ].title = TCPR_sndwinup_TITLE;
	mettbl[ TCP_sndctrl_IDX ].title = TCP_sndctrl_TITLE;
	mettbl[ TCPR_sndctrl_IDX ].title = TCPR_sndctrl_TITLE;
	mettbl[ TCP_sndrsts_IDX ].title = TCP_sndrsts_TITLE;
	mettbl[ TCPR_sndrsts_IDX ].title = TCPR_sndrsts_TITLE;
	mettbl[ TCP_rcvtotal_IDX ].title = TCP_rcvtotal_TITLE;
	mettbl[ TCPR_rcvtotal_IDX ].title = TCPR_rcvtotal_TITLE;
	mettbl[ TCP_rcvackpack_IDX ].title = TCP_rcvackpack_TITLE;
	mettbl[ TCPR_rcvackpack_IDX ].title = TCPR_rcvackpack_TITLE;
	mettbl[ TCP_rcvackbyte_IDX ].title = TCP_rcvackbyte_TITLE;
	mettbl[ TCPR_rcvackbyte_IDX ].title = TCPR_rcvackbyte_TITLE;
	mettbl[ TCP_rcvdupack_IDX ].title = TCP_rcvdupack_TITLE;
	mettbl[ TCPR_rcvdupack_IDX ].title = TCPR_rcvdupack_TITLE;
	mettbl[ TCP_rcvacktoomuch_IDX ].title = TCP_rcvacktoomuch_TITLE;
	mettbl[ TCPR_rcvacktoomuch_IDX ].title = TCPR_rcvacktoomuch_TITLE;
	mettbl[ TCP_rcvpack_IDX ].title = TCP_rcvpack_TITLE;
	mettbl[ TCPR_rcvpack_IDX ].title = TCPR_rcvpack_TITLE;
	mettbl[ TCP_rcvbyte_IDX ].title = TCP_rcvbyte_TITLE;
	mettbl[ TCPR_rcvbyte_IDX ].title = TCPR_rcvbyte_TITLE;
	mettbl[ TCP_rcvduppack_IDX ].title = TCP_rcvduppack_TITLE;
	mettbl[ TCPR_rcvduppack_IDX ].title = TCPR_rcvduppack_TITLE;
	mettbl[ TCP_rcvdupbyte_IDX ].title = TCP_rcvdupbyte_TITLE;
	mettbl[ TCPR_rcvdupbyte_IDX ].title = TCPR_rcvdupbyte_TITLE;
	mettbl[ TCP_rcvpartduppack_IDX ].title = TCP_rcvpartduppack_TITLE;
	mettbl[ TCPR_rcvpartduppack_IDX ].title = TCPR_rcvpartduppack_TITLE;
	mettbl[ TCP_rcvpartdupbyte_IDX ].title = TCP_rcvpartdupbyte_TITLE;
	mettbl[ TCPR_rcvpartdupbyte_IDX ].title = TCPR_rcvpartdupbyte_TITLE;
	mettbl[ TCP_rcvoopack_IDX ].title = TCP_rcvoopack_TITLE;
	mettbl[ TCPR_rcvoopack_IDX ].title = TCPR_rcvoopack_TITLE;
	mettbl[ TCP_rcvoobyte_IDX ].title = TCP_rcvoobyte_TITLE;
	mettbl[ TCPR_rcvoobyte_IDX ].title = TCPR_rcvoobyte_TITLE;
	mettbl[ TCP_rcvpackafterwin_IDX ].title = TCP_rcvpackafterwin_TITLE;
	mettbl[ TCPR_rcvpackafterwin_IDX ].title = TCPR_rcvpackafterwin_TITLE;
	mettbl[ TCP_rcvbyteafterwin_IDX ].title = TCP_rcvbyteafterwin_TITLE;
	mettbl[ TCPR_rcvbyteafterwin_IDX ].title = TCPR_rcvbyteafterwin_TITLE;
	mettbl[ TCP_rcvwinprobe_IDX ].title = TCP_rcvwinprobe_TITLE;
	mettbl[ TCPR_rcvwinprobe_IDX ].title = TCPR_rcvwinprobe_TITLE;
	mettbl[ TCP_rcvwinupd_IDX ].title = TCP_rcvwinupd_TITLE;
	mettbl[ TCPR_rcvwinupd_IDX ].title = TCPR_rcvwinupd_TITLE;
	mettbl[ TCP_rcvafterclose_IDX ].title = TCP_rcvafterclose_TITLE;
	mettbl[ TCPR_rcvafterclose_IDX ].title = TCPR_rcvafterclose_TITLE;
	mettbl[ TCP_rcvbadsum_IDX ].title = TCP_rcvbadsum_TITLE;
	mettbl[ TCPR_rcvbadsum_IDX ].title = TCPR_rcvbadsum_TITLE;
	mettbl[ TCP_rcvbadoff_IDX ].title = TCP_rcvbadoff_TITLE;
	mettbl[ TCPR_rcvbadoff_IDX ].title = TCPR_rcvbadoff_TITLE;
	mettbl[ TCP_rcvshort_IDX ].title = TCP_rcvshort_TITLE;
	mettbl[ TCPR_rcvshort_IDX ].title = TCPR_rcvshort_TITLE;
	mettbl[ TCP_inerrors_IDX ].title = TCP_inerrors_TITLE;
	mettbl[ TCPR_inerrors_IDX ].title = TCPR_inerrors_TITLE;
	mettbl[ TCP_connattempt_IDX ].title = TCP_connattempt_TITLE;
	mettbl[ TCPR_connattempt_IDX ].title = TCPR_connattempt_TITLE;
	mettbl[ TCP_accepts_IDX ].title = TCP_accepts_TITLE;
	mettbl[ TCPR_accepts_IDX ].title = TCPR_accepts_TITLE;
	mettbl[ TCP_connects_IDX ].title = TCP_connects_TITLE;
	mettbl[ TCPR_connects_IDX ].title = TCPR_connects_TITLE;
	mettbl[ TCP_closed_IDX ].title = TCP_closed_TITLE;
	mettbl[ TCPR_closed_IDX ].title = TCPR_closed_TITLE;
	mettbl[ TCP_drops_IDX ].title = TCP_drops_TITLE;
	mettbl[ TCPR_drops_IDX ].title = TCPR_drops_TITLE;
	mettbl[ TCP_conndrops_IDX ].title = TCP_conndrops_TITLE;
	mettbl[ TCPR_conndrops_IDX ].title = TCPR_conndrops_TITLE;
	mettbl[ TCP_attemptfails_IDX ].title = TCP_attemptfails_TITLE;
	mettbl[ TCPR_attemptfails_IDX ].title = TCPR_attemptfails_TITLE;
	mettbl[ TCP_estabresets_IDX ].title = TCP_estabresets_TITLE;
	mettbl[ TCPR_estabresets_IDX ].title = TCPR_estabresets_TITLE;
	mettbl[ TCP_rttupdated_IDX ].title = TCP_rttupdated_TITLE;
	mettbl[ TCPR_rttupdated_IDX ].title = TCPR_rttupdated_TITLE;
	mettbl[ TCP_segstimed_IDX ].title = TCP_segstimed_TITLE;
	mettbl[ TCPR_segstimed_IDX ].title = TCPR_segstimed_TITLE;
	mettbl[ TCP_rexmttimeo_IDX ].title = TCP_rexmttimeo_TITLE;
	mettbl[ TCPR_rexmttimeo_IDX ].title = TCPR_rexmttimeo_TITLE;
	mettbl[ TCP_timeoutdrop_IDX ].title = TCP_timeoutdrop_TITLE;
	mettbl[ TCPR_timeoutdrop_IDX ].title = TCPR_timeoutdrop_TITLE;
	mettbl[ TCP_persisttimeo_IDX ].title = TCP_persisttimeo_TITLE;
	mettbl[ TCPR_persisttimeo_IDX ].title = TCPR_persisttimeo_TITLE;
	mettbl[ TCP_keeptimeo_IDX ].title = TCP_keeptimeo_TITLE;
	mettbl[ TCPR_keeptimeo_IDX ].title = TCPR_keeptimeo_TITLE;
	mettbl[ TCP_keepprobe_IDX ].title = TCP_keepprobe_TITLE;
	mettbl[ TCPR_keepprobe_IDX ].title = TCPR_keepprobe_TITLE;
	mettbl[ TCP_keepdrops_IDX ].title = TCP_keepdrops_TITLE;
	mettbl[ TCPR_keepdrops_IDX ].title = TCPR_keepdrops_TITLE;
	mettbl[ TCP_linger_IDX ].title = TCP_linger_TITLE;
	mettbl[ TCPR_linger_IDX ].title = TCPR_linger_TITLE;
	mettbl[ TCP_lingerexp_IDX ].title = TCP_lingerexp_TITLE;
	mettbl[ TCPR_lingerexp_IDX ].title = TCPR_lingerexp_TITLE;
	mettbl[ TCP_lingercan_IDX ].title = TCP_lingercan_TITLE;
	mettbl[ TCPR_lingercan_IDX ].title = TCPR_lingercan_TITLE;
	mettbl[ TCP_lingerabort_IDX ].title = TCP_lingerabort_TITLE;
	mettbl[ TCPR_lingerabort_IDX ].title = TCPR_lingerabort_TITLE;
	mettbl[ UDP_hdrops_IDX ].title = UDP_hdrops_TITLE;
	mettbl[ UDPR_hdrops_IDX ].title = UDPR_hdrops_TITLE;
	mettbl[ UDP_badlen_IDX ].title = UDP_badlen_TITLE;
	mettbl[ UDPR_badlen_IDX ].title = UDPR_badlen_TITLE;
	mettbl[ UDP_badsum_IDX ].title = UDP_badsum_TITLE;
	mettbl[ UDPR_badsum_IDX ].title = UDPR_badsum_TITLE;
	mettbl[ UDP_fullsock_IDX ].title = UDP_fullsock_TITLE;
	mettbl[ UDPR_fullsock_IDX ].title = UDPR_fullsock_TITLE;
	mettbl[ UDP_noports_IDX ].title = UDP_noports_TITLE;
	mettbl[ UDPR_noports_IDX ].title = UDPR_noports_TITLE;
	mettbl[ UDPR_indelivers_IDX ].title = UDPR_indelivers_TITLE;
	mettbl[ UDP_inerrors_IDX ].title = UDP_inerrors_TITLE;
	mettbl[ UDPR_inerrors_IDX ].title = UDPR_inerrors_TITLE;
	mettbl[ UDPR_outtotal_IDX ].title = UDPR_outtotal_TITLE;
	mettbl[ IPR_sum_IDX ].title = IPR_sum_TITLE;
	mettbl[ ICMPR_sum_IDX ].title = ICMPR_sum_TITLE;
	mettbl[ UDPR_sum_IDX ].title = UDPR_sum_TITLE;
	mettbl[ TCPR_sum_IDX ].title = TCPR_sum_TITLE;
	mettbl[ NETERR_sum_IDX ].title = NETERR_sum_TITLE;
	mettbl[ NETERRR_sum_IDX ].title = NETERRR_sum_TITLE;
	mettbl[ RCACHE_PERCENT_IDX ].title = RCACHE_PERCENT_TITLE;
	mettbl[ WCACHE_PERCENT_IDX ].title = WCACHE_PERCENT_TITLE;
	mettbl[ MEM_PERCENT_IDX ].title = MEM_PERCENT_TITLE;
	mettbl[ MEM_SWAP_PERCENT_IDX ].title = MEM_SWAP_PERCENT_TITLE;
	mettbl[ DSK_SWAP_PERCENT_IDX ].title = DSK_SWAP_PERCENT_TITLE;
	mettbl[ DSK_SWAPPG_IDX ].title = DSK_SWAPPG_TITLE;
	mettbl[ DSK_SWAPPGFREE_IDX ].title = DSK_SWAPPGFREE_TITLE;
	mettbl[ MEM_SWAPPG_IDX ].title = MEM_SWAPPG_TITLE;
	mettbl[ TOTALMEM_IDX ].title = TOTALMEM_TITLE;
	mettbl[ STR_STREAM_INUSE_IDX ].title = STR_STREAM_INUSE_TITLE;
	mettbl[ STR_STREAM_TOTAL_IDX ].title = STR_STREAM_TOTAL_TITLE;
	mettbl[ STR_QUEUE_INUSE_IDX ].title = STR_QUEUE_INUSE_TITLE;
	mettbl[ STR_QUEUE_TOTAL_IDX ].title = STR_QUEUE_TOTAL_TITLE;
	mettbl[ STR_MDBBLK_INUSE_IDX ].title = STR_MDBBLK_INUSE_TITLE;
	mettbl[ STR_MDBBLK_TOTAL_IDX ].title = STR_MDBBLK_TOTAL_TITLE;
	mettbl[ STR_MSGBLK_INUSE_IDX ].title = STR_MSGBLK_INUSE_TITLE;
	mettbl[ STR_MSGBLK_TOTAL_IDX ].title = STR_MSGBLK_TOTAL_TITLE;
	mettbl[ STR_LINK_INUSE_IDX ].title = STR_LINK_INUSE_TITLE;
	mettbl[ STR_LINK_TOTAL_IDX ].title = STR_LINK_TOTAL_TITLE;
	mettbl[ STR_EVENT_INUSE_IDX ].title = STR_EVENT_INUSE_TITLE;
	mettbl[ STR_EVENT_TOTAL_IDX ].title = STR_EVENT_TOTAL_TITLE;
	mettbl[ STR_EVENT_FAIL_IDX ].title = STR_EVENT_FAIL_TITLE;
	mettbl[ STR_EVENT_FAILR_IDX ].title = STR_EVENT_FAILR_TITLE;
	mettbl[ SAP_total_servers_IDX ].title =
	  SAP_total_servers_TITLE;
	mettbl[ SAPR_total_IDX ].title =
	  SAPR_total_TITLE;
	mettbl[ IPXR_total_IDX ].title =
	  IPXR_total_TITLE;
	mettbl[ RIPR_total_IDX ].title =
	  RIPR_total_TITLE;
	mettbl[ SPXR_total_IDX ].title =
	  SPXR_total_TITLE;
	mettbl[ NETWARE_errs_IDX ].title =
	  NETWARE_errs_TITLE;
	mettbl[ NETWARER_errs_IDX ].title =
	  NETWARER_errs_TITLE;
	mettbl[ SAP_unused_IDX ].title =
	  SAP_unused_TITLE;
	mettbl[ SAP_Lans_IDX ].title =
	  SAP_Lans_TITLE;
	mettbl[ SAPR_TotalInSaps_IDX ].title =
	  SAPR_TotalInSaps_TITLE;
	mettbl[ SAPR_GSQReceived_IDX ].title =
	  SAPR_GSQReceived_TITLE;
	mettbl[ SAPR_GSRReceived_IDX ].title =
	  SAPR_GSRReceived_TITLE;
	mettbl[ SAPR_NSQReceived_IDX ].title =
	  SAPR_NSQReceived_TITLE;
	mettbl[ SAPR_SASReceived_IDX ].title =
	  SAPR_SASReceived_TITLE;
	mettbl[ SAPR_SNCReceived_IDX ].title =
	  SAPR_SNCReceived_TITLE;
	mettbl[ SAPR_GSIReceived_IDX ].title =
	  SAPR_GSIReceived_TITLE;
	mettbl[ SAP_NotNeighbor_IDX ].title =
	  SAP_NotNeighbor_TITLE;
	mettbl[ SAPR_NotNeighbor_IDX ].title =
	  SAPR_NotNeighbor_TITLE;
	mettbl[ SAP_EchoMyOutput_IDX ].title =
	  SAP_EchoMyOutput_TITLE;
	mettbl[ SAPR_EchoMyOutput_IDX ].title =
	  SAPR_EchoMyOutput_TITLE;
	mettbl[ SAP_BadSizeInSaps_IDX ].title =
	  SAP_BadSizeInSaps_TITLE;
	mettbl[ SAPR_BadSizeInSaps_IDX ].title =
	  SAPR_BadSizeInSaps_TITLE;
	mettbl[ SAP_BadSapSource_IDX ].title =
	  SAP_BadSapSource_TITLE;
	mettbl[ SAPR_BadSapSource_IDX ].title =
	  SAPR_BadSapSource_TITLE;
	mettbl[ SAPR_TotalOutSaps_IDX ].title =
	  SAPR_TotalOutSaps_TITLE;
	mettbl[ SAPR_NSRSent_IDX ].title =
	  SAPR_NSRSent_TITLE;
	mettbl[ SAPR_GSRSent_IDX ].title =
	  SAPR_GSRSent_TITLE;
	mettbl[ SAPR_GSQSent_IDX ].title =
	  SAPR_GSQSent_TITLE;
	mettbl[ SAPR_SASAckSent_IDX ].title =
	  SAPR_SASAckSent_TITLE;
	mettbl[ SAP_SASNackSent_IDX ].title =
	  SAP_SASNackSent_TITLE;
	mettbl[ SAPR_SASNackSent_IDX ].title =
	  SAPR_SASNackSent_TITLE;
	mettbl[ SAPR_SNCAckSent_IDX ].title =
	  SAPR_SNCAckSent_TITLE;
	mettbl[ SAP_SNCNackSent_IDX ].title =
	  SAP_SNCNackSent_TITLE;
	mettbl[ SAPR_SNCNackSent_IDX ].title =
	  SAPR_SNCNackSent_TITLE;
	mettbl[ SAPR_GSIAckSent_IDX ].title =
	  SAPR_GSIAckSent_TITLE;
	mettbl[ SAP_BadDestOutSaps_IDX ].title =
	  SAP_BadDestOutSaps_TITLE;
	mettbl[ SAPR_BadDestOutSaps_IDX ].title =
	  SAPR_BadDestOutSaps_TITLE;
	mettbl[ SAP_SrvAllocFailed_IDX ].title =
	  SAP_SrvAllocFailed_TITLE;
	mettbl[ SAPR_SrvAllocFailed_IDX ].title =
	  SAPR_SrvAllocFailed_TITLE;
	mettbl[ SAP_MallocFailed_IDX ].title =
	  SAP_MallocFailed_TITLE;
	mettbl[ SAPR_MallocFailed_IDX ].title =
	  SAPR_MallocFailed_TITLE;
	mettbl[ SAP_TotalInRipSaps_IDX ].title =
	  SAP_TotalInRipSaps_TITLE;
	mettbl[ SAPR_TotalInRipSaps_IDX ].title =
	  SAPR_TotalInRipSaps_TITLE;
	mettbl[ SAP_BadRipSaps_IDX ].title =
	  SAP_BadRipSaps_TITLE;
	mettbl[ SAPR_BadRipSaps_IDX ].title =
	  SAPR_BadRipSaps_TITLE;
	mettbl[ SAP_RipServerDown_IDX ].title =
	  SAP_RipServerDown_TITLE;
	mettbl[ SAPR_RipServerDown_IDX ].title =
	  SAPR_RipServerDown_TITLE;
	mettbl[ SAPR_ProcessesToNotify_IDX ].title =
	  SAPR_ProcessesToNotify_TITLE;
	mettbl[ SAPR_NotificationsSent_IDX ].title =
	  SAPR_NotificationsSent_TITLE;
	mettbl[ SAPLAN_Network_IDX ].title =
	  SAPLAN_Network_TITLE;
	mettbl[ SAPLAN_LanNumber_IDX ].title =
	  SAPLAN_LanNumber_TITLE;
	mettbl[ SAPLAN_UpdateInterval_IDX ].title =
	  SAPLAN_UpdateInterval_TITLE;
	mettbl[ SAPLAN_AgeFactor_IDX ].title =
	  SAPLAN_AgeFactor_TITLE;
	mettbl[ SAPLAN_PacketGap_IDX ].title =
	  SAPLAN_PacketGap_TITLE;
	mettbl[ SAPLAN_PacketSize_IDX ].title =
	  SAPLAN_PacketSize_TITLE;
	mettbl[ SAPLANR_PacketsSent_IDX ].title =
	  SAPLANR_PacketsSent_TITLE;
	mettbl[ SAPLANR_PacketsReceived_IDX ].title =
	  SAPLANR_PacketsReceived_TITLE;
	mettbl[ SAPLAN_BadPktsReceived_IDX ].title =
	  SAPLAN_BadPktsReceived_TITLE;
	mettbl[ SAPLANR_BadPktsReceived_IDX ].title =
	  SAPLANR_BadPktsReceived_TITLE;
	mettbl[ IPXLAN_InProtoSize_IDX ].title =
	  IPXLAN_InProtoSize_TITLE;
	mettbl[ IPXLANR_InProtoSize_IDX ].title =
	  IPXLANR_InProtoSize_TITLE;
	mettbl[ IPXLAN_InBadDLPItype_IDX ].title =
	  IPXLAN_InBadDLPItype_TITLE;
	mettbl[ IPXLANR_InBadDLPItype_IDX ].title =
	  IPXLANR_InBadDLPItype_TITLE;
	mettbl[ IPXLAN_InCoalesced_IDX ].title =
	  IPXLAN_InCoalesced_TITLE;
	mettbl[ IPXLANR_InCoalesced_IDX ].title =
	  IPXLANR_InCoalesced_TITLE;
	mettbl[ IPXLANR_InPropagation_IDX ].title =
	  IPXLANR_InPropagation_TITLE;
	mettbl[ IPXLAN_InNoPropagate_IDX ].title =
	  IPXLAN_InNoPropagate_TITLE;
	mettbl[ IPXLANR_InNoPropagate_IDX ].title =
	  IPXLANR_InNoPropagate_TITLE;
	mettbl[ IPXLANR_InTotal_IDX ].title =
	  IPXLANR_InTotal_TITLE;
	mettbl[ IPXLAN_InBadLength_IDX ].title =
	  IPXLAN_InBadLength_TITLE;
	mettbl[ IPXLANR_InBadLength_IDX ].title =
	  IPXLANR_InBadLength_TITLE;
	mettbl[ IPXLAN_InDriverEcho_IDX ].title =
	  IPXLAN_InDriverEcho_TITLE;
	mettbl[ IPXLANR_InDriverEcho_IDX ].title =
	  IPXLANR_InDriverEcho_TITLE;
	mettbl[ IPXLANR_InRip_IDX ].title =
	  IPXLANR_InRip_TITLE;
	mettbl[ IPXLAN_InRipDropped_IDX ].title =
	  IPXLAN_InRipDropped_TITLE;
	mettbl[ IPXLANR_InRipDropped_IDX ].title =
	  IPXLANR_InRipDropped_TITLE;
	mettbl[ IPXLANR_InRipRouted_IDX ].title =
	  IPXLANR_InRipRouted_TITLE;
	mettbl[ IPXLANR_InSap_IDX ].title =
	  IPXLANR_InSap_TITLE;
	mettbl[ IPXLAN_InSapBad_IDX ].title =
	  IPXLAN_InSapBad_TITLE;
	mettbl[ IPXLANR_InSapBad_IDX ].title =
	  IPXLANR_InSapBad_TITLE;
	mettbl[ IPXLANR_InSapIpx_IDX ].title =
	  IPXLANR_InSapIpx_TITLE;
	mettbl[ IPXLANR_InSapNoIpxToSapd_IDX ].title =
	  IPXLANR_InSapNoIpxToSapd_TITLE;
	mettbl[ IPXLAN_InSapNoIpxDrop_IDX ].title =
	  IPXLAN_InSapNoIpxDrop_TITLE;
	mettbl[ IPXLANR_InSapNoIpxDrop_IDX ].title =
	  IPXLANR_InSapNoIpxDrop_TITLE;
	mettbl[ IPXLAN_InDiag_IDX ].title =
	  IPXLAN_InDiag_TITLE;
	mettbl[ IPXLANR_InDiag_IDX ].title =
	  IPXLANR_InDiag_TITLE;
	mettbl[ IPXLAN_InDiagInternal_IDX ].title =
	  IPXLAN_InDiagInternal_TITLE;
	mettbl[ IPXLANR_InDiagInternal_IDX ].title =
	  IPXLANR_InDiagInternal_TITLE;
	mettbl[ IPXLAN_InDiagNIC_IDX ].title =
	  IPXLAN_InDiagNIC_TITLE;
	mettbl[ IPXLANR_InDiagNIC_IDX ].title =
	  IPXLANR_InDiagNIC_TITLE;
	mettbl[ IPXLAN_InDiagIpx_IDX ].title =
	  IPXLAN_InDiagIpx_TITLE;
	mettbl[ IPXLANR_InDiagIpx_IDX ].title =
	  IPXLANR_InDiagIpx_TITLE;
	mettbl[ IPXLAN_InDiagNoIpx_IDX ].title =
	  IPXLAN_InDiagNoIpx_TITLE;
	mettbl[ IPXLANR_InDiagNoIpx_IDX ].title =
	  IPXLANR_InDiagNoIpx_TITLE;
	mettbl[ IPXLAN_InNICDropped_IDX ].title =
	  IPXLAN_InNICDropped_TITLE;
	mettbl[ IPXLANR_InNICDropped_IDX ].title =
	  IPXLANR_InNICDropped_TITLE;
	mettbl[ IPXLANR_InBroadcast_IDX ].title =
	  IPXLANR_InBroadcast_TITLE;
	mettbl[ IPXLANR_InBroadcastInternal_IDX ].title =
	  IPXLANR_InBroadcastInternal_TITLE;
	mettbl[ IPXLANR_InBroadcastNIC_IDX ].title =
	  IPXLANR_InBroadcastNIC_TITLE;
	mettbl[ IPXLAN_InBroadcastDiag_IDX ].title =
	  IPXLAN_InBroadcastDiag_TITLE;
	mettbl[ IPXLANR_InBroadcastDiag_IDX ].title =
	  IPXLANR_InBroadcastDiag_TITLE;
	mettbl[ IPXLAN_InBroadcastDiagFwd_IDX ].title =
	  IPXLAN_InBroadcastDiagFwd_TITLE;
	mettbl[ IPXLANR_InBroadcastDiagFwd_IDX ].title =
	  IPXLANR_InBroadcastDiagFwd_TITLE;
	mettbl[ IPXLAN_InBroadcastDiagRoute_IDX ].title =
	  IPXLAN_InBroadcastDiagRoute_TITLE;
	mettbl[ IPXLANR_InBroadcastDiagRoute_IDX ].title =
	  IPXLANR_InBroadcastDiagRoute_TITLE;
	mettbl[ IPXLAN_InBroadcastDiagResp_IDX ].title =
	  IPXLAN_InBroadcastDiagResp_TITLE;
	mettbl[ IPXLANR_InBroadcastDiagResp_IDX ].title =
	  IPXLANR_InBroadcastDiagResp_TITLE;
	mettbl[ IPXLAN_InBroadcastRoute_IDX ].title =
	  IPXLAN_InBroadcastRoute_TITLE;
	mettbl[ IPXLANR_InBroadcastRoute_IDX ].title =
	  IPXLANR_InBroadcastRoute_TITLE;
	mettbl[ IPXLANR_InForward_IDX ].title =
	  IPXLANR_InForward_TITLE;
	mettbl[ IPXLANR_InRoute_IDX ].title =
	  IPXLANR_InRoute_TITLE;
	mettbl[ IPXLANR_InInternalNet_IDX ].title =
	  IPXLANR_InInternalNet_TITLE;
	mettbl[ IPXLANR_OutPropagation_IDX ].title =
	  IPXLANR_OutPropagation_TITLE;
	mettbl[ IPXLANR_OutTotalStream_IDX ].title =
	  IPXLANR_OutTotalStream_TITLE;
	mettbl[ IPXLANR_OutTotal_IDX ].title =
	  IPXLANR_OutTotal_TITLE;
	mettbl[ IPXLAN_OutSameSocket_IDX ].title =
	  IPXLAN_OutSameSocket_TITLE;
	mettbl[ IPXLANR_OutSameSocket_IDX ].title =
	  IPXLANR_OutSameSocket_TITLE;
	mettbl[ IPXLANR_OutFillInDest_IDX ].title =
	  IPXLANR_OutFillInDest_TITLE;
	mettbl[ IPXLANR_OutInternal_IDX ].title =
	  IPXLANR_OutInternal_TITLE;
	mettbl[ IPXLAN_OutBadLan_IDX ].title =
	  IPXLAN_OutBadLan_TITLE;
	mettbl[ IPXLANR_OutBadLan_IDX ].title =
	  IPXLANR_OutBadLan_TITLE;
	mettbl[ IPXLANR_OutSent_IDX ].title =
	  IPXLANR_OutSent_TITLE;
	mettbl[ IPXLANR_OutQueued_IDX ].title =
	  IPXLANR_OutQueued_TITLE;
	mettbl[ IPXLANR_Ioctl_IDX ].title =
	  IPXLANR_Ioctl_TITLE;
	mettbl[ IPXLANR_IoctlSetLans_IDX ].title =
	  IPXLANR_IoctlSetLans_TITLE;
	mettbl[ IPXLANR_IoctlGetLans_IDX ].title =
	  IPXLANR_IoctlGetLans_TITLE;
	mettbl[ IPXLANR_IoctlSetSapQ_IDX ].title =
	  IPXLANR_IoctlSetSapQ_TITLE;
	mettbl[ IPXLANR_IoctlSetLanInfo_IDX ].title =
	  IPXLANR_IoctlSetLanInfo_TITLE;
	mettbl[ IPXLANR_IoctlGetLanInfo_IDX ].title =
	  IPXLANR_IoctlGetLanInfo_TITLE;
	mettbl[ IPXLANR_IoctlGetNodeAddr_IDX ].title =
	  IPXLANR_IoctlGetNodeAddr_TITLE;
	mettbl[ IPXLANR_IoctlGetNetAddr_IDX ].title =
	  IPXLANR_IoctlGetNetAddr_TITLE;
	mettbl[ IPXLANR_IoctlGetStats_IDX ].title =
	  IPXLANR_IoctlGetStats_TITLE;
	mettbl[ IPXLANR_IoctlLink_IDX ].title =
	  IPXLANR_IoctlLink_TITLE;
	mettbl[ IPXLANR_IoctlUnlink_IDX ].title =
	  IPXLANR_IoctlUnlink_TITLE;
	mettbl[ IPXLAN_IoctlUnknown_IDX ].title =
	  IPXLAN_IoctlUnknown_TITLE;
	mettbl[ IPXLANR_IoctlUnknown_IDX ].title =
	  IPXLANR_IoctlUnknown_TITLE;
	mettbl[ IPXSOCKR_IpxInData_IDX ].title =
	  IPXSOCKR_IpxInData_TITLE;
	mettbl[ IPXR_datapackets_IDX ].title =
	  IPXR_datapackets_TITLE;
	mettbl[ IPXSOCKR_IpxOutData_IDX ].title =
	  IPXSOCKR_IpxOutData_TITLE;
	mettbl[ IPXSOCK_IpxOutBadSize_IDX ].title =
	  IPXSOCK_IpxOutBadSize_TITLE;
	mettbl[ IPXSOCKR_IpxOutBadSize_IDX ].title =
	  IPXSOCKR_IpxOutBadSize_TITLE;
	mettbl[ IPXSOCK_IpxInBadSize_IDX ].title =
	  IPXSOCK_IpxInBadSize_TITLE;
	mettbl[ IPXSOCKR_IpxInBadSize_IDX ].title =
	  IPXSOCKR_IpxInBadSize_TITLE;
	mettbl[ IPXSOCKR_IpxOutToSwitch_IDX ].title =
	  IPXSOCKR_IpxOutToSwitch_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOutData_IDX ].title =
	  IPXSOCKR_IpxTLIOutData_TITLE;
	mettbl[ IPXSOCK_IpxTLIOutBadState_IDX ].title =
	  IPXSOCK_IpxTLIOutBadState_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOutBadState_IDX ].title =
	  IPXSOCKR_IpxTLIOutBadState_TITLE;
	mettbl[ IPXSOCK_IpxTLIOutBadSize_IDX ].title =
	  IPXSOCK_IpxTLIOutBadSize_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOutBadSize_IDX ].title =
	  IPXSOCKR_IpxTLIOutBadSize_TITLE;
	mettbl[ IPXSOCK_IpxTLIOutBadOpt_IDX ].title =
	  IPXSOCK_IpxTLIOutBadOpt_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOutBadOpt_IDX ].title =
	  IPXSOCKR_IpxTLIOutBadOpt_TITLE;
	mettbl[ IPXSOCK_IpxTLIOutHdrAlloc_IDX ].title =
	  IPXSOCK_IpxTLIOutHdrAlloc_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOutHdrAlloc_IDX ].title =
	  IPXSOCKR_IpxTLIOutHdrAlloc_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOutToSwitch_IDX ].title =
	  IPXSOCKR_IpxTLIOutToSwitch_TITLE;
	mettbl[ IPXSOCK_IpxBoundSockets_IDX ].title =
	  IPXSOCK_IpxBoundSockets_TITLE;
	mettbl[ IPXSOCKR_IpxBoundSockets_IDX ].title =
	  IPXSOCKR_IpxBoundSockets_TITLE;
	mettbl[ IPXSOCKR_IpxBind_IDX ].title =
	  IPXSOCKR_IpxBind_TITLE;
	mettbl[ IPXSOCKR_IpxTLIBind_IDX ].title =
	  IPXSOCKR_IpxTLIBind_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOptMgt_IDX ].title =
	  IPXSOCKR_IpxTLIOptMgt_TITLE;
	mettbl[ IPXSOCK_IpxTLIUnknown_IDX ].title =
	  IPXSOCK_IpxTLIUnknown_TITLE;
	mettbl[ IPXSOCKR_IpxTLIUnknown_IDX ].title =
	  IPXSOCKR_IpxTLIUnknown_TITLE;
	mettbl[ IPXSOCK_IpxTLIOutBadAddr_IDX ].title = 
	  IPXSOCK_IpxTLIOutBadAddr_TITLE;
	mettbl[ IPXSOCKR_IpxTLIOutBadAddr_IDX ].title = 
	  IPXSOCKR_IpxTLIOutBadAddr_TITLE;
	mettbl[ IPXSOCK_IpxSwitchInvalSocket_IDX ].title =
	  IPXSOCK_IpxSwitchInvalSocket_TITLE;
	mettbl[ IPXSOCKR_IpxSwitchInvalSocket_IDX ].title =
	  IPXSOCKR_IpxSwitchInvalSocket_TITLE;
	mettbl[ IPXSOCK_IpxSwitchSumFail_IDX ].title =
	  IPXSOCK_IpxSwitchSumFail_TITLE;
	mettbl[ IPXSOCKR_IpxSwitchSumFail_IDX ].title =
	  IPXSOCKR_IpxSwitchSumFail_TITLE;
	mettbl[ IPXSOCK_IpxSwitchAllocFail_IDX ].title =
	  IPXSOCK_IpxSwitchAllocFail_TITLE;
	mettbl[ IPXSOCKR_IpxSwitchAllocFail_IDX ].title =
	  IPXSOCKR_IpxSwitchAllocFail_TITLE;
	mettbl[ IPXSOCKR_IpxSwitchSum_IDX ].title =
	  IPXSOCKR_IpxSwitchSum_TITLE;
	mettbl[ IPXSOCKR_IpxSwitchEven_IDX ].title =
	  IPXSOCKR_IpxSwitchEven_TITLE;
	mettbl[ IPXSOCKR_IpxSwitchEvenAlloc_IDX ].title =
	  IPXSOCKR_IpxSwitchEvenAlloc_TITLE;
	mettbl[ DUMMY_1_IDX ].title = DUMMY_1_TITLE;
	mettbl[ IPXSOCKR_IpxDataToSocket_IDX ].title =
	  IPXSOCKR_IpxDataToSocket_TITLE;
	mettbl[ IPXSOCKR_IpxTrimPacket_IDX ].title =
	  IPXSOCKR_IpxTrimPacket_TITLE;
	mettbl[ IPXSOCK_IpxSumFail_IDX ].title =
	  IPXSOCK_IpxSumFail_TITLE;
	mettbl[ IPXSOCKR_IpxSumFail_IDX ].title =
	  IPXSOCKR_IpxSumFail_TITLE;
	mettbl[ IPXSOCK_IpxBusySocket_IDX ].title =
	  IPXSOCK_IpxBusySocket_TITLE;
	mettbl[ IPXSOCKR_IpxBusySocket_IDX ].title =
	  IPXSOCKR_IpxBusySocket_TITLE;
	mettbl[ IPXSOCK_IpxSocketNotBound_IDX ].title =
	  IPXSOCK_IpxSocketNotBound_TITLE;
	mettbl[ IPXSOCKR_IpxSocketNotBound_IDX ].title =
	  IPXSOCKR_IpxSocketNotBound_TITLE;
	mettbl[ IPXSOCKR_IpxRouted_IDX ].title =
	  IPXSOCKR_IpxRouted_TITLE;
	mettbl[ IPXSOCKR_IpxRoutedTLI_IDX ].title =
	  IPXSOCKR_IpxRoutedTLI_TITLE;
	mettbl[ IPXSOCK_IpxRoutedTLIAlloc_IDX ].title =
	  IPXSOCK_IpxRoutedTLIAlloc_TITLE;
	mettbl[ IPXSOCKR_IpxRoutedTLIAlloc_IDX ].title =
	  IPXSOCKR_IpxRoutedTLIAlloc_TITLE;
	mettbl[ IPXR_sent_to_tli_IDX ].title =
	  IPXR_sent_to_tli_TITLE;
	mettbl[ IPXR_total_ioctls_IDX ].title =
	  IPXR_total_ioctls_TITLE;
	mettbl[ IPXSOCKR_IpxIoctlSetWater_IDX ].title =
	  IPXSOCKR_IpxIoctlSetWater_TITLE;
	mettbl[ IPXSOCKR_IpxIoctlBindSocket_IDX ].title =
	  IPXSOCKR_IpxIoctlBindSocket_TITLE;
	mettbl[ IPXSOCKR_IpxIoctlUnbindSocket_IDX ].title =
	  IPXSOCKR_IpxIoctlUnbindSocket_TITLE;
	mettbl[ IPXSOCKR_IpxIoctlStats_IDX ].title =
	  IPXSOCKR_IpxIoctlStats_TITLE;
	mettbl[ IPXSOCK_IpxIoctlUnknown_IDX ].title =
	  IPXSOCK_IpxIoctlUnknown_TITLE;
	mettbl[ IPXSOCKR_IpxIoctlUnknown_IDX ].title =
	  IPXSOCKR_IpxIoctlUnknown_TITLE;
	mettbl[ RIPR_ReceivedPackets_IDX ].title =
	  RIPR_ReceivedPackets_TITLE;
	mettbl[ RIP_ReceivedNoLanKey_IDX ].title =
	  RIP_ReceivedNoLanKey_TITLE;
	mettbl[ RIPR_ReceivedNoLanKey_IDX ].title =
	  RIPR_ReceivedNoLanKey_TITLE;
	mettbl[ RIP_ReceivedBadLength_IDX ].title =
	  RIP_ReceivedBadLength_TITLE;
	mettbl[ RIPR_ReceivedBadLength_IDX ].title =
	  RIPR_ReceivedBadLength_TITLE;
	mettbl[ RIPR_ReceivedCoalesced_IDX ].title =
	  RIPR_ReceivedCoalesced_TITLE;
	mettbl[ RIP_ReceivedNoCoalesce_IDX ].title =
	  RIP_ReceivedNoCoalesce_TITLE;
	mettbl[ RIPR_ReceivedNoCoalesce_IDX ].title =
	  RIPR_ReceivedNoCoalesce_TITLE;
	mettbl[ RIPR_ReceivedRequestPackets_IDX ].title =
	  RIPR_ReceivedRequestPackets_TITLE;
	mettbl[ RIPR_ReceivedResponsePackets_IDX ].title =
	  RIPR_ReceivedResponsePackets_TITLE;
	mettbl[ RIP_ReceivedUnknownRequest_IDX ].title =
	  RIP_ReceivedUnknownRequest_TITLE;
	mettbl[ RIPR_ReceivedUnknownRequest_IDX ].title =
	  RIPR_ReceivedUnknownRequest_TITLE;
	mettbl[ RIPR_total_router_packets_sent_IDX ].title =
	  RIPR_total_router_packets_sent_TITLE;
	mettbl[ RIP_SentAllocFailed_IDX ].title =
	  RIP_SentAllocFailed_TITLE;
	mettbl[ RIPR_SentAllocFailed_IDX ].title =
	  RIPR_SentAllocFailed_TITLE;
	mettbl[ RIP_SentBadDestination_IDX ].title =
	  RIP_SentBadDestination_TITLE;
	mettbl[ RIPR_SentBadDestination_IDX ].title =
	  RIPR_SentBadDestination_TITLE;
	mettbl[ RIPR_SentRequestPackets_IDX ].title =
	  RIPR_SentRequestPackets_TITLE;
	mettbl[ RIPR_SentResponsePackets_IDX ].title =
	  RIPR_SentResponsePackets_TITLE;
	mettbl[ RIP_SentLan0Dropped_IDX ].title =
	  RIP_SentLan0Dropped_TITLE;
	mettbl[ RIPR_SentLan0Dropped_IDX ].title =
	  RIPR_SentLan0Dropped_TITLE;
	mettbl[ RIPR_SentLan0Routed_IDX ].title =
	  RIPR_SentLan0Routed_TITLE;
	mettbl[ RIPR_ioctls_processed_IDX ].title =
	  RIPR_ioctls_processed_TITLE;
	mettbl[ RIPR_RipxIoctlInitialize_IDX ].title =
	  RIPR_RipxIoctlInitialize_TITLE;
	mettbl[ RIPR_RipxIoctlGetHashSize_IDX ].title =
	  RIPR_RipxIoctlGetHashSize_TITLE;
	mettbl[ RIPR_RipxIoctlGetHashStats_IDX ].title =
	  RIPR_RipxIoctlGetHashStats_TITLE;
	mettbl[ RIPR_RipxIoctlDumpHashTable_IDX ].title =
	  RIPR_RipxIoctlDumpHashTable_TITLE;
	mettbl[ RIPR_RipxIoctlGetRouterTable_IDX ].title =
	  RIPR_RipxIoctlGetRouterTable_TITLE;
	mettbl[ RIPR_RipxIoctlGetNetInfo_IDX ].title =
	  RIPR_RipxIoctlGetNetInfo_TITLE;
	mettbl[ RIPR_RipxIoctlCheckSapSource_IDX ].title =
	  RIPR_RipxIoctlCheckSapSource_TITLE;
	mettbl[ RIPR_RipxIoctlResetRouter_IDX ].title =
	  RIPR_RipxIoctlResetRouter_TITLE;
	mettbl[ RIPR_RipxIoctlDownRouter_IDX ].title =
	  RIPR_RipxIoctlDownRouter_TITLE;
	mettbl[ RIPR_RipxIoctlStats_IDX ].title =
	  RIPR_RipxIoctlStats_TITLE;
	mettbl[ RIP_RipxIoctlUnknown_IDX ].title =
	  RIP_RipxIoctlUnknown_TITLE;
	mettbl[ RIPR_RipxIoctlUnknown_IDX ].title =
	  RIPR_RipxIoctlUnknown_TITLE;
	mettbl[ SPX_max_connections_IDX ].title =
	  SPX_max_connections_TITLE;
	mettbl[ SPX_current_connections_IDX ].title =
	  SPX_current_connections_TITLE;
	mettbl[ SPXR_current_connections_IDX ].title =
	  SPXR_current_connections_TITLE;
	mettbl[ SPX_max_used_connections_IDX ].title =
	  SPX_max_used_connections_TITLE;
	mettbl[ SPX_alloc_failures_IDX ].title =
	  SPX_alloc_failures_TITLE;
	mettbl[ SPXR_alloc_failures_IDX ].title =
	  SPXR_alloc_failures_TITLE;
	mettbl[ SPX_open_failures_IDX ].title =
	  SPX_open_failures_TITLE;
	mettbl[ SPXR_open_failures_IDX ].title =
	  SPXR_open_failures_TITLE;
	mettbl[ SPXR_ioctls_IDX ].title =
	  SPXR_ioctls_TITLE;
	mettbl[ SPXR_connect_req_count_IDX ].title =
	  SPXR_connect_req_count_TITLE;
	mettbl[ SPX_connect_req_fails_IDX ].title =
	  SPX_connect_req_fails_TITLE;
	mettbl[ SPXR_connect_req_fails_IDX ].title =
	  SPXR_connect_req_fails_TITLE;
	mettbl[ SPXR_listen_req_IDX ].title =
	  SPXR_listen_req_TITLE;
	mettbl[ SPX_listen_req_fails_IDX ].title =
	  SPX_listen_req_fails_TITLE;
	mettbl[ SPXR_listen_req_fails_IDX ].title =
	  SPXR_listen_req_fails_TITLE;
	mettbl[ SPXR_send_mesg_count_IDX ].title =
	  SPXR_send_mesg_count_TITLE;
	mettbl[ SPX_unknown_mesg_count_IDX ].title =
	  SPX_unknown_mesg_count_TITLE;
	mettbl[ SPXR_unknown_mesg_count_IDX ].title =
	  SPXR_unknown_mesg_count_TITLE;
	mettbl[ SPX_send_bad_mesg_IDX ].title =
	  SPX_send_bad_mesg_TITLE;
	mettbl[ SPXR_send_bad_mesg_IDX ].title =
	  SPXR_send_bad_mesg_TITLE;
	mettbl[ SPXR_send_packet_count_IDX ].title =
	  SPXR_send_packet_count_TITLE;
	mettbl[ SPX_send_packet_timeout_IDX ].title =
	  SPX_send_packet_timeout_TITLE;
	mettbl[ SPXR_send_packet_timeout_IDX ].title =
	  SPXR_send_packet_timeout_TITLE;
	mettbl[ SPX_send_packet_nak_IDX ].title =
	  SPX_send_packet_nak_TITLE;
	mettbl[ SPXR_send_packet_nak_IDX ].title =
	  SPXR_send_packet_nak_TITLE;
	mettbl[ SPXR_rcv_packet_count_IDX ].title =
	  SPXR_rcv_packet_count_TITLE;
	mettbl[ SPX_rcv_bad_packet_IDX ].title =
	  SPX_rcv_bad_packet_TITLE;
	mettbl[ SPXR_rcv_bad_packet_IDX ].title =
	  SPXR_rcv_bad_packet_TITLE;
	mettbl[ SPX_rcv_bad_data_packet_IDX ].title =
	  SPX_rcv_bad_data_packet_TITLE;
	mettbl[ SPXR_rcv_bad_data_packet_IDX ].title =
	  SPXR_rcv_bad_data_packet_TITLE;
	mettbl[ SPX_rcv_dup_packet_IDX ].title =
	  SPX_rcv_dup_packet_TITLE;
	mettbl[ SPXR_rcv_dup_packet_IDX ].title =
	  SPXR_rcv_dup_packet_TITLE;
	mettbl[ SPXR_rcv_packet_sentup_IDX ].title =
	  SPXR_rcv_packet_sentup_TITLE;
	mettbl[ SPX_rcv_conn_req_IDX ].title =
	  SPX_rcv_conn_req_TITLE;
	mettbl[ SPXR_rcv_conn_req_IDX ].title =
	  SPXR_rcv_conn_req_TITLE;
	mettbl[ SPX_abort_connection_IDX ].title =
	  SPX_abort_connection_TITLE;
	mettbl[ SPXR_abort_connection_IDX ].title =
	  SPXR_abort_connection_TITLE;
	mettbl[ SPX_max_retries_abort_IDX ].title =
	  SPX_max_retries_abort_TITLE;
	mettbl[ SPXR_max_retries_abort_IDX ].title =
	  SPXR_max_retries_abort_TITLE;
	mettbl[ SPX_no_listeners_IDX ].title =
	  SPX_no_listeners_TITLE;
	mettbl[ SPXR_no_listeners_IDX ].title =
	  SPXR_no_listeners_TITLE;
	mettbl[ SPXCON_netaddr_IDX ].title =
	  SPXCON_netaddr_TITLE;
	mettbl[ SPXCON_nodeaddr_IDX ].title =
	  SPXCON_nodeaddr_TITLE;
	mettbl[ SPXCON_sockaddr_IDX ].title =
	  SPXCON_sockaddr_TITLE;
	mettbl[ SPXCON_connection_id_IDX ].title =
	  SPXCON_connection_id_TITLE;
	mettbl[ SPXCON_o_netaddr_IDX ].title =
	  SPXCON_o_netaddr_TITLE;
	mettbl[ SPXCON_o_nodeaddr_IDX ].title =
	  SPXCON_o_nodeaddr_TITLE;
	mettbl[ SPXCON_o_sockaddr_IDX ].title =
	  SPXCON_o_sockaddr_TITLE;
	mettbl[ SPXCON_o_connection_id_IDX ].title =
	  SPXCON_o_connection_id_TITLE;
	mettbl[ SPXCON_con_state_IDX ].title =
	  SPXCON_con_state_TITLE;
	mettbl[ SPXCON_con_retry_count_IDX ].title =
	  SPXCON_con_retry_count_TITLE;
	mettbl[ SPXCON_con_retry_time_IDX ].title =
	  SPXCON_con_retry_time_TITLE;
	mettbl[ SPXCON_con_state_IDX ].title =
	  SPXCON_con_state_TITLE;
	mettbl[ SPXCON_con_type_IDX ].title =
	  SPXCON_con_type_TITLE;
	mettbl[ SPXCON_con_ipxChecksum_IDX ].title =
	  SPXCON_con_ipxChecksum_TITLE;
	mettbl[ SPXCON_con_window_size_IDX ].title =
	  SPXCON_con_window_size_TITLE;
	mettbl[ SPXCON_con_remote_window_size_IDX ].title =
	  SPXCON_con_remote_window_size_TITLE;
	mettbl[ SPXCON_con_send_packet_size_IDX ].title =
	  SPXCON_con_send_packet_size_TITLE;
	mettbl[ SPXCON_con_rcv_packet_size_IDX ].title =
	  SPXCON_con_rcv_packet_size_TITLE;
	mettbl[ SPXCON_con_round_trip_time_IDX ].title =
	  SPXCON_con_round_trip_time_TITLE;
	mettbl[ SPXCON_con_window_choke_IDX ].title =
	  SPXCON_con_window_choke_TITLE;
	mettbl[ SPXCONR_con_send_mesg_count_IDX ].title =
	  SPXCONR_con_send_mesg_count_TITLE;
	mettbl[ SPXCON_con_unknown_mesg_count_IDX ].title =
	  SPXCON_con_unknown_mesg_count_TITLE;
	mettbl[ SPXCONR_con_unknown_mesg_count_IDX ].title =
	  SPXCONR_con_unknown_mesg_count_TITLE;
	mettbl[ SPXCON_con_send_bad_mesg_IDX ].title =
	  SPXCON_con_send_bad_mesg_TITLE;
	mettbl[ SPXCONR_con_send_bad_mesg_IDX ].title =
	  SPXCONR_con_send_bad_mesg_TITLE;
	mettbl[ SPXCONR_con_send_packet_count_IDX ].title =
	  SPXCONR_con_send_packet_count_TITLE;
	mettbl[ SPXCON_con_send_packet_timeout_IDX ].title =
	  SPXCON_con_send_packet_timeout_TITLE;
	mettbl[ SPXCONR_con_send_packet_timeout_IDX ].title =
	  SPXCONR_con_send_packet_timeout_TITLE;
	mettbl[ SPXCON_con_send_packet_nak_IDX ].title =
	  SPXCON_con_send_packet_nak_TITLE;
	mettbl[ SPXCONR_con_send_packet_nak_IDX ].title =
	  SPXCONR_con_send_packet_nak_TITLE;
	mettbl[ SPXCONR_con_send_ack_IDX ].title =
	  SPXCONR_con_send_ack_TITLE;
	mettbl[ SPXCON_con_send_nak_IDX ].title =
	  SPXCON_con_send_nak_TITLE;
	mettbl[ SPXCONR_con_send_nak_IDX ].title =
	  SPXCONR_con_send_nak_TITLE;
	mettbl[ SPXCONR_con_send_watchdog_IDX ].title =
	  SPXCONR_con_send_watchdog_TITLE;
	mettbl[ SPXCONR_con_rcv_packet_count_IDX ].title =
	  SPXCONR_con_rcv_packet_count_TITLE;
	mettbl[ SPXCON_con_rcv_bad_packet_IDX ].title =
	  SPXCON_con_rcv_bad_packet_TITLE;
	mettbl[ SPXCONR_con_rcv_bad_packet_IDX ].title =
	  SPXCONR_con_rcv_bad_packet_TITLE;
	mettbl[ SPXCON_con_rcv_bad_data_packet_IDX ].title =
	  SPXCON_con_rcv_bad_data_packet_TITLE;
	mettbl[ SPXCONR_con_rcv_bad_data_packet_IDX ].title =
	  SPXCONR_con_rcv_bad_data_packet_TITLE;
	mettbl[ SPXCON_con_rcv_dup_packet_IDX ].title =
	  SPXCON_con_rcv_dup_packet_TITLE;
	mettbl[ SPXCONR_con_rcv_dup_packet_IDX ].title =
	  SPXCONR_con_rcv_dup_packet_TITLE;
	mettbl[ SPXCON_con_rcv_packet_outseq_IDX ].title =
	  SPXCON_con_rcv_packet_outseq_TITLE;
	mettbl[ SPXCONR_con_rcv_packet_outseq_IDX ].title =
	  SPXCONR_con_rcv_packet_outseq_TITLE;
	mettbl[ SPXCONR_con_rcv_packet_sentup_IDX ].title =
	  SPXCONR_con_rcv_packet_sentup_TITLE;
	mettbl[ SPXCONR_con_rcv_packet_qued_IDX ].title =
	  SPXCONR_con_rcv_packet_qued_TITLE;
	mettbl[ SPXCONR_con_rcv_ack_IDX ].title =
	  SPXCONR_con_rcv_ack_TITLE;
	mettbl[ SPXCON_con_rcv_nak_IDX ].title =
	  SPXCON_con_rcv_nak_TITLE;
	mettbl[ SPXCONR_con_rcv_nak_IDX ].title =
	  SPXCONR_con_rcv_nak_TITLE;
	mettbl[ SPXCONR_con_rcv_watchdog_IDX ].title =
	  SPXCONR_con_rcv_watchdog_TITLE;
	mettbl[ EISA_BUS_UTIL_SUMCNT_IDX ].title = 
	  EISA_BUS_UTIL_SUMCNT_TITLE;
	mettbl[ EISA_BUS_UTIL_PERCENT_IDX ].title = 
	  EISA_BUS_UTIL_PERCENT_TITLE;
	mettbl[ LBOLT_IDX ].title = LBOLT_TITLE;
}
/*
 *	function: 	alloc_mets
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	allocate space for each of the instances of the metrics
 *	declared in mettbl.  For a few of the metrics, alter some
 *	of the parameters associated with cooking them so they have
 *	the units/whatever that we want.  For those metrics that are 
 *	kludged up because they aren't in MAS, do some setting of the 
 *	addresses from where they will be cooked.
 */
void
alloc_mets() {
	int i, j, k;
/*
 *	set an inital value for tdiff.  This is because 
 *	calc_intv is called to stuff initial values into the metrics
 *	during allocation, so that the resource values can be determined.
 */
	tdiff = 1.0;

	for( i=0; i < nmets; i++ ) {
		metalloc( &mettbl[i] );
		switch( mettbl[i].id ) {
#ifdef TEST_SCROLLING
		case NCPU: ncpu = 8; break; 
		case NDISK: ndisk = 8; break;
#endif
		case MPK_MEM:	/* kept in pages */
			for( j = 0 ; j < mettbl[i].resval[0]+1 ; j++ )
			for( k=0; k < mettbl[i].resval[1]+1 ; k++ )
				mettbl[i].metval[j*(mettbl[i].resval[1]+1)
				  +k].scaleval = (float)pgsz;
			break;
		case RUNOCC:	case SWPOCC: /* convert to percentage */
			mettbl[i].metval->scaleval = 100.0;
			break;
		case MPS_RUNOCC: /* convert to percent */
			for( j = 0 ; j < ncpu; j++ )
				mettbl[i].metval[j].scaleval = 100.0;
			break;
		case NETHER:
			mettbl[i].metval->met_p = (caddr_t)&nether_devs;
			nether = nether_devs;
			break;
		case TOTALMEM:
			mettbl[i].metval->met_p = (caddr_t)&totalmem;
			mettbl[i].metval->scaleval = 1.0/(float)pgsz;
			break;
		case SAP_Lans:
			mettbl[i].metval->met_p = (caddr_t)&nlans;
			mettbl[i].metval->met.sngl = nlans;
			mettbl[i].metval->scaleval = 1.0;
			break;
		case SPX_max_connections:
			mettbl[i].metval->met_p = 
			  (caddr_t)&spxbuf.spx_max_connections;
			mettbl[i].metval->met.sngl =
			  spxbuf.spx_max_connections;
			mettbl[i].metval->scaleval = 1.0;
			break;
		default:
			for( j=0; j < mettbl[i].resval[0]+1 ; j++ )
			for( k=0; k < mettbl[i].resval[1]+1 ; k++ )
				mettbl[i].metval[j*(mettbl[i].resval[1]+1)
				  +k].scaleval = 1.0;
			break;
		}
	}

	mettbl[ TOT_CPU_IDX ].id = TOT_CPU;
	mettbl[ TOT_IDL_IDX ].id = TOT_IDL;
	mettbl[ TOT_RW_IDX ].id = TOT_RW;
	mettbl[ TOT_KRWCH_IDX ].id = TOT_KRWCH;
	mettbl[ DNLC_PERCENT_IDX ].id = DNLC_PERCENT;
	mettbl[ TOT_KMA_PAGES_IDX ].id = TOT_KMA_PAGES;
	mettbl[ LWP_SLEEP_IDX ].id = LWP_SLEEP;
	mettbl[ LWP_SLEEP_IDX ].metval->met_p = (caddr_t)&lwp_count.sleep;
	mettbl[ LWP_RUN_IDX ].id = LWP_RUN;
	mettbl[ LWP_RUN_IDX ].metval->met_p = (caddr_t)&lwp_count.run;
	mettbl[ LWP_ZOMB_IDX ].id = LWP_ZOMB;
	mettbl[ LWP_ZOMB_IDX ].metval->met_p = (caddr_t)&lwp_count.zombie;
	mettbl[ LWP_STOP_IDX ].id = LWP_STOP;
	mettbl[ LWP_STOP_IDX ].metval->met_p = (caddr_t)&lwp_count.stop;
	mettbl[ LWP_IDLE_IDX ].id = LWP_IDLE;
	mettbl[ LWP_IDLE_IDX ].metval->met_p = (caddr_t)&lwp_count.idle;
	mettbl[ LWP_ONPROC_IDX ].id = LWP_ONPROC;
	mettbl[ LWP_ONPROC_IDX ].metval->met_p = (caddr_t)&lwp_count.onproc;
	mettbl[ LWP_OTHER_IDX ].id = LWP_OTHER;
	mettbl[ LWP_OTHER_IDX ].metval->met_p = (caddr_t)&lwp_count.other;
	mettbl[ LWP_TOTAL_IDX ].id = LWP_TOTAL;
	mettbl[ LWP_TOTAL_IDX ].metval->met_p = (caddr_t)&lwp_count.total;
	mettbl[ LWP_NPROC_IDX ].id = LWP_NPROC;
	mettbl[ LWP_NPROC_IDX ].metval->met_p = (caddr_t)&lwp_count.nproc;
	for( i = 0 ; i < nether ; i++ ) {
		mettbl[ETHNAME_IDX].metval[i].met_p = ether_nm[i];
		mettbl[ETH_InUcastPkts_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifInUcastPkts);
		mettbl[ETH_OutUcastPkts_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutUcastPkts);
		mettbl[ETH_InNUcastPkts_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifInNUcastPkts);
		mettbl[ETH_OutNUcastPkts_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutNUcastPkts);
		mettbl[ETH_InOctets_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifInOctets);
		mettbl[ETH_OutOctets_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutOctets);
		mettbl[ETH_InErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifInErrors);
		mettbl[ETHR_InErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifInErrors);
		mettbl[ETH_etherAlignErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherAlignErrors);
		mettbl[ETHR_etherAlignErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherAlignErrors);
		mettbl[ETH_etherCRCerrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherCRCerrors);
		mettbl[ETHR_etherCRCerrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherCRCerrors);
		mettbl[ETH_etherOverrunErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherOverrunErrors);
		mettbl[ETHR_etherOverrunErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherOverrunErrors);
		mettbl[ETH_etherUnderrunErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherUnderrunErrors);
		mettbl[ETHR_etherUnderrunErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherUnderrunErrors);
		mettbl[ETH_etherMissedPkts_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherMissedPkts);
		mettbl[ETHR_etherMissedPkts_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherMissedPkts);
		mettbl[ETH_InDiscards_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifInDiscards);
		mettbl[ETHR_InDiscards_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifInDiscards);
		mettbl[ETH_etherReadqFull_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherReadqFull);
		mettbl[ETHR_etherReadqFull_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherReadqFull);
		mettbl[ETH_etherRcvResources_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherRcvResources);
		mettbl[ETHR_etherRcvResources_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherRcvResources);
		mettbl[ETH_etherCollisions_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherCollisions);
		mettbl[ETHR_etherCollisions_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherCollisions);
		mettbl[ETH_OutDiscards_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutDiscards);
		mettbl[ETHR_OutDiscards_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutDiscards);
		mettbl[ETH_OutErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutErrors);
		mettbl[ETHR_OutErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutErrors);
		mettbl[ETH_etherAbortErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherAbortErrors);
		mettbl[ETHR_etherAbortErrors_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherAbortErrors);
		mettbl[ETH_etherCarrierLost_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherCarrierLost);
		mettbl[ETHR_etherCarrierLost_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifSpecific.etherCarrierLost);
		mettbl[ETH_OutQlen_IDX].metval[i].met_p = 
		   (caddr_t)&(etherstat[i]->ifOutQlen);
	}
	mettbl[IPR_total_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_total;
	mettbl[IP_badsum_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_badsum;
	mettbl[IPR_badsum_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_badsum;
	mettbl[IP_tooshort_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_tooshort;
	mettbl[IPR_tooshort_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_tooshort;
	mettbl[IP_toosmall_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_toosmall;
	mettbl[IPR_toosmall_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_toosmall;
	mettbl[IP_badhlen_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_badhlen;
	mettbl[IPR_badhlen_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_badhlen;
	mettbl[IP_badlen_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_badlen;
	mettbl[IPR_badlen_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_badlen;
	mettbl[IP_unknownproto_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_unknownproto;
	mettbl[IPR_unknownproto_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_unknownproto;
	mettbl[IP_fragments_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragments;
	mettbl[IPR_fragments_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragments;
	mettbl[IP_fragdropped_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragdropped;
	mettbl[IPR_fragdropped_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragdropped;
	mettbl[IP_fragtimeout_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragtimeout;
	mettbl[IPR_fragtimeout_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragtimeout;
	mettbl[IP_reasms_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_reasms;
	mettbl[IPR_reasms_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_reasms;
	mettbl[IP_forward_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_forward;
	mettbl[IPR_forward_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_forward;
	mettbl[IP_cantforward_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_cantforward;
	mettbl[IPR_cantforward_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_cantforward;
	mettbl[IP_noroutes_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_noroutes;
	mettbl[IPR_noroutes_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_noroutes;
	mettbl[IP_redirectsent_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_redirectsent;
	mettbl[IPR_redirectsent_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_redirectsent;
	mettbl[IP_inerrors_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_inerrors;
	mettbl[IPR_inerrors_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_inerrors;
	mettbl[IPR_indelivers_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_indelivers;
	mettbl[IPR_outrequests_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_outrequests;
	mettbl[IP_outerrors_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_outerrors;
	mettbl[IPR_outerrors_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_outerrors;
	mettbl[IP_pfrags_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_pfrags;
	mettbl[IPR_pfrags_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_pfrags;
	mettbl[IP_fragfails_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragfails;
	mettbl[IPR_fragfails_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_fragfails;
	mettbl[IP_frags_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_frags;
	mettbl[IPR_frags_IDX].metval->met_p =
	  (caddr_t)&ipstat.ips_frags;
	mettbl[ICMP_error_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_error;
	mettbl[ICMPR_error_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_error;
	mettbl[ICMP_oldicmp_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_oldicmp;
	mettbl[ICMPR_oldicmp_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_oldicmp;
	mettbl[ICMP_outhist0_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[0];
	mettbl[ICMPR_outhist0_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[0];
	mettbl[ICMP_outhist3_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[3];
	mettbl[ICMPR_outhist3_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[3];
	mettbl[ICMP_outhist4_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[4];
	mettbl[ICMPR_outhist4_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[4];
	mettbl[ICMP_outhist5_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[5];
	mettbl[ICMPR_outhist5_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[5];
	mettbl[ICMP_outhist8_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[8];
	mettbl[ICMPR_outhist8_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[8];
	mettbl[ICMP_outhist11_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[11];
	mettbl[ICMPR_outhist11_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[11];
	mettbl[ICMP_outhist12_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[12];
	mettbl[ICMPR_outhist12_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[12];
	mettbl[ICMP_outhist13_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[13];
	mettbl[ICMPR_outhist13_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[13];
	mettbl[ICMP_outhist14_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[14];
	mettbl[ICMPR_outhist14_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[14];
	mettbl[ICMP_outhist15_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[15];
	mettbl[ICMPR_outhist15_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[15];
	mettbl[ICMP_outhist16_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[16];
	mettbl[ICMPR_outhist16_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[16];
	mettbl[ICMP_outhist17_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[17];
	mettbl[ICMPR_outhist17_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[17];
	mettbl[ICMP_outhist18_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[18];
	mettbl[ICMPR_outhist18_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outhist[18];
	mettbl[ICMP_badcode_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_badcode;
	mettbl[ICMPR_badcode_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_badcode;
	mettbl[ICMP_tooshort_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_tooshort;
	mettbl[ICMPR_tooshort_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_tooshort;
	mettbl[ICMP_checksum_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_checksum;
	mettbl[ICMPR_checksum_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_checksum;
	mettbl[ICMP_badlen_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_badlen;
	mettbl[ICMPR_badlen_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_badlen;
	mettbl[ICMP_inhist0_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[0];
	mettbl[ICMPR_inhist0_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[0];
	mettbl[ICMP_inhist3_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[3];
	mettbl[ICMPR_inhist3_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[3];
	mettbl[ICMP_inhist4_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[4];
	mettbl[ICMPR_inhist4_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[4];
	mettbl[ICMP_inhist5_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[5];
	mettbl[ICMPR_inhist5_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[5];
	mettbl[ICMP_inhist8_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[8];
	mettbl[ICMPR_inhist8_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[8];
	mettbl[ICMP_inhist11_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[11];
	mettbl[ICMPR_inhist11_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[11];
	mettbl[ICMP_inhist12_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[12];
	mettbl[ICMPR_inhist12_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[12];
	mettbl[ICMP_inhist13_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[13];
	mettbl[ICMPR_inhist13_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[13];
	mettbl[ICMP_inhist14_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[14];
	mettbl[ICMPR_inhist14_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[14];
	mettbl[ICMP_inhist15_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[15];
	mettbl[ICMPR_inhist15_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[15];
	mettbl[ICMP_inhist16_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[16];
	mettbl[ICMPR_inhist16_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[16];
	mettbl[ICMP_inhist17_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[17];
	mettbl[ICMPR_inhist17_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[17];
	mettbl[ICMP_inhist18_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[18];
	mettbl[ICMPR_inhist18_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_inhist[18];
	mettbl[ICMP_reflect_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_reflect;
	mettbl[ICMPR_reflect_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_reflect;
	mettbl[ICMPR_intotal_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_intotal;
	mettbl[ICMPR_outtotal_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outtotal;
	mettbl[ICMP_outerrors_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outerrors;
	mettbl[ICMPR_outerrors_IDX].metval->met_p =
	  (caddr_t)&icmpstat.icps_outerrors;
	mettbl[TCPR_sndtotal_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndtotal;
	mettbl[TCPR_sndpack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndpack;
	mettbl[TCPR_sndbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndbyte;
	mettbl[TCP_sndrexmitpack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndrexmitpack;
	mettbl[TCPR_sndrexmitpack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndrexmitpack;
	mettbl[TCP_sndrexmitbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndrexmitbyte;
	mettbl[TCPR_sndrexmitbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndrexmitbyte;
	mettbl[TCP_sndacks_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndacks;
	mettbl[TCPR_sndacks_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndacks;
	mettbl[TCP_delack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_delack;
	mettbl[TCPR_delack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_delack;
	mettbl[TCP_sndurg_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndurg;
	mettbl[TCPR_sndurg_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndurg;
	mettbl[TCP_sndprobe_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndprobe;
	mettbl[TCPR_sndprobe_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndprobe;
	mettbl[TCP_sndwinup_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndwinup;
	mettbl[TCPR_sndwinup_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndwinup;
	mettbl[TCP_sndctrl_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndctrl;
	mettbl[TCPR_sndctrl_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndctrl;
	mettbl[TCP_sndrsts_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndrsts;
	mettbl[TCPR_sndrsts_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_sndrsts;
	mettbl[TCP_rcvtotal_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvtotal;
	mettbl[TCPR_rcvtotal_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvtotal;
	mettbl[TCP_rcvackpack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvackpack;
	mettbl[TCPR_rcvackpack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvackpack;
	mettbl[TCP_rcvackbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvackbyte;
	mettbl[TCPR_rcvackbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvackbyte;
	mettbl[TCP_rcvdupack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvdupack;
	mettbl[TCPR_rcvdupack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvdupack;
	mettbl[TCP_rcvacktoomuch_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvacktoomuch;
	mettbl[TCPR_rcvacktoomuch_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvacktoomuch;
	mettbl[TCP_rcvpack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpack;
	mettbl[TCPR_rcvpack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpack;
	mettbl[TCP_rcvbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbyte;
	mettbl[TCPR_rcvbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbyte;
	mettbl[TCP_rcvduppack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvduppack;
	mettbl[TCPR_rcvduppack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvduppack;
	mettbl[TCP_rcvdupbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvdupbyte;
	mettbl[TCPR_rcvdupbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvdupbyte;
	mettbl[TCP_rcvpartduppack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpartduppack;
	mettbl[TCPR_rcvpartduppack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpartduppack;
	mettbl[TCP_rcvpartdupbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpartdupbyte;
	mettbl[TCPR_rcvpartdupbyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpartdupbyte;
	mettbl[TCP_rcvoopack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvoopack;
	mettbl[TCPR_rcvoopack_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvoopack;
	mettbl[TCP_rcvoobyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvoobyte;
	mettbl[TCPR_rcvoobyte_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvoobyte;
	mettbl[TCP_rcvpackafterwin_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpackafterwin;
	mettbl[TCPR_rcvpackafterwin_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvpackafterwin;
	mettbl[TCP_rcvbyteafterwin_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbyteafterwin;
	mettbl[TCPR_rcvbyteafterwin_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbyteafterwin;
	mettbl[TCP_rcvwinprobe_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvwinprobe;
	mettbl[TCPR_rcvwinprobe_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvwinprobe;
	mettbl[TCP_rcvwinupd_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvwinupd;
	mettbl[TCPR_rcvwinupd_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvwinupd;
	mettbl[TCP_rcvafterclose_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvafterclose;
	mettbl[TCPR_rcvafterclose_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvafterclose;
	mettbl[TCP_rcvbadsum_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbadsum;
	mettbl[TCPR_rcvbadsum_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbadsum;
	mettbl[TCP_rcvbadoff_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbadoff;
	mettbl[TCPR_rcvbadoff_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvbadoff;
	mettbl[TCP_rcvshort_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvshort;
	mettbl[TCPR_rcvshort_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rcvshort;
	mettbl[TCP_inerrors_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_inerrors;
	mettbl[TCPR_inerrors_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_inerrors;
	mettbl[TCP_connattempt_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_connattempt;
	mettbl[TCPR_connattempt_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_connattempt;
	mettbl[TCP_accepts_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_accepts;
	mettbl[TCPR_accepts_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_accepts;
	mettbl[TCP_connects_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_connects;
	mettbl[TCPR_connects_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_connects;
	mettbl[TCP_closed_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_closed;
	mettbl[TCPR_closed_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_closed;
	mettbl[TCP_drops_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_drops;
	mettbl[TCPR_drops_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_drops;
	mettbl[TCP_conndrops_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_conndrops;
	mettbl[TCPR_conndrops_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_conndrops;
	mettbl[TCP_attemptfails_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_attemptfails;
	mettbl[TCPR_attemptfails_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_attemptfails;
	mettbl[TCP_estabresets_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_estabresets;
	mettbl[TCPR_estabresets_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_estabresets;
	mettbl[TCP_rttupdated_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rttupdated;
	mettbl[TCPR_rttupdated_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rttupdated;
	mettbl[TCP_segstimed_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_segstimed;
	mettbl[TCPR_segstimed_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_segstimed;
	mettbl[TCP_rexmttimeo_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rexmttimeo;
	mettbl[TCPR_rexmttimeo_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_rexmttimeo;
	mettbl[TCP_timeoutdrop_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_timeoutdrop;
	mettbl[TCPR_timeoutdrop_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_timeoutdrop;
	mettbl[TCP_persisttimeo_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_persisttimeo;
	mettbl[TCPR_persisttimeo_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_persisttimeo;
	mettbl[TCP_keeptimeo_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_keeptimeo;
	mettbl[TCPR_keeptimeo_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_keeptimeo;
	mettbl[TCP_keepprobe_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_keepprobe;
	mettbl[TCPR_keepprobe_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_keepprobe;
	mettbl[TCP_keepdrops_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_keepdrops;
	mettbl[TCPR_keepdrops_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_keepdrops;
	mettbl[TCP_linger_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_linger;
	mettbl[TCPR_linger_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_linger;
	mettbl[TCP_lingerexp_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_lingerexp;
	mettbl[TCPR_lingerexp_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_lingerexp;
	mettbl[TCP_lingercan_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_lingercan;
	mettbl[TCPR_lingercan_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_lingercan;
	mettbl[TCP_lingerabort_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_lingerabort;
	mettbl[TCPR_lingerabort_IDX].metval->met_p =
	  (caddr_t)&tcpstat.tcps_lingerabort;
	mettbl[UDP_hdrops_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_hdrops;
	mettbl[UDPR_hdrops_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_hdrops;
	mettbl[UDP_badlen_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_badlen;
	mettbl[UDPR_badlen_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_badlen;
	mettbl[UDP_badsum_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_badsum;
	mettbl[UDPR_badsum_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_badsum;
	mettbl[UDP_fullsock_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_fullsock;
	mettbl[UDPR_fullsock_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_fullsock;
	mettbl[UDP_noports_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_noports;
	mettbl[UDPR_noports_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_noports;
	mettbl[UDPR_indelivers_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_indelivers;
	mettbl[UDP_inerrors_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_inerrors;
	mettbl[UDPR_inerrors_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_inerrors;
	mettbl[UDPR_outtotal_IDX].metval->met_p =
	  (caddr_t)&udpstat.udps_outtotal;
	mettbl[IPR_sum_IDX].metval->met_p =
	  (caddr_t)&net_total.ip;
	mettbl[ICMPR_sum_IDX].metval->met_p =
	  (caddr_t)&net_total.icmp;
	mettbl[UDPR_sum_IDX].metval->met_p = (caddr_t)&net_total.udp;
	mettbl[TCPR_sum_IDX].metval->met_p = (caddr_t)&net_total.tcp;
	mettbl[NETERR_sum_IDX].metval->met_p = (caddr_t)&net_total.errs;
	mettbl[NETERRR_sum_IDX].metval->met_p = (caddr_t)&net_total.errs;
 	mettbl[ DSK_SWAPPG_IDX ].metval->met_p = (caddr_t)&dsk_swappg;
 	mettbl[ DSK_SWAPPGFREE_IDX ].metval->met_p = (caddr_t)&dsk_swappgfree;
 	mettbl[ MEM_SWAPPG_IDX ].metval->met_p = (caddr_t)&mem_swappg;

	mettbl[ SAP_total_servers_IDX ].metval->met_p =
	   (caddr_t)&sap_total_servers;
	mettbl[ SAP_unused_IDX ].metval->met_p =
	   (caddr_t)&sap_unused;
	mettbl[ SAPR_TotalInSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.TotalInSaps;
	mettbl[ SAPR_GSQReceived_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.GSQReceived;
	mettbl[ SAPR_GSRReceived_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.GSRReceived;
	mettbl[ SAPR_NSQReceived_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.NSQReceived;
	mettbl[ SAPR_SASReceived_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SASReceived;
	mettbl[ SAPR_SNCReceived_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SNCReceived;
	mettbl[ SAPR_GSIReceived_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.GSIReceived;
	mettbl[ SAP_NotNeighbor_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.NotNeighbor;
	mettbl[ SAPR_NotNeighbor_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.NotNeighbor;
	mettbl[ SAP_EchoMyOutput_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.EchoMyOutput;
	mettbl[ SAPR_EchoMyOutput_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.EchoMyOutput;
	mettbl[ SAP_BadSizeInSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadSizeInSaps;
	mettbl[ SAPR_BadSizeInSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadSizeInSaps;
	mettbl[ SAP_BadSapSource_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadSapSource;
	mettbl[ SAPR_BadSapSource_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadSapSource;
	mettbl[ SAPR_TotalOutSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.TotalOutSaps;
	mettbl[ SAPR_NSRSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.NSRSent;
	mettbl[ SAPR_GSRSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.GSRSent;
	mettbl[ SAPR_GSQSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.GSQSent;
	mettbl[ SAPR_SASAckSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SASAckSent;
	mettbl[ SAP_SASNackSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SASNackSent;
	mettbl[ SAPR_SASNackSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SASNackSent;
	mettbl[ SAPR_SNCAckSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SNCAckSent;
	mettbl[ SAP_SNCNackSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SNCNackSent;
	mettbl[ SAPR_SNCNackSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SNCNackSent;
	mettbl[ SAPR_GSIAckSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.GSIAckSent;
	mettbl[ SAP_BadDestOutSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadDestOutSaps;
	mettbl[ SAPR_BadDestOutSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadDestOutSaps;
	mettbl[ SAP_SrvAllocFailed_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SrvAllocFailed;
	mettbl[ SAPR_SrvAllocFailed_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.SrvAllocFailed;
	mettbl[ SAP_MallocFailed_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.MallocFailed;
	mettbl[ SAPR_MallocFailed_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.MallocFailed;
	mettbl[ SAP_TotalInRipSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.TotalInRipSaps;
	mettbl[ SAPR_TotalInRipSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.TotalInRipSaps;
	mettbl[ SAP_BadRipSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadRipSaps;
	mettbl[ SAPR_BadRipSaps_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.BadRipSaps;
	mettbl[ SAP_RipServerDown_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.RipServerDown;
	mettbl[ SAPR_RipServerDown_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.RipServerDown;
	mettbl[ SAPR_ProcessesToNotify_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.ProcessesToNotify;
	mettbl[ SAPR_NotificationsSent_IDX ].metval->met_p =
	   (caddr_t)&sap_buf.NotificationsSent;
	for( i = 0 ; i < nlans; i++ ) {
		mettbl[ SAPLAN_Network_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].Network;
		mettbl[ SAPLAN_LanNumber_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].LanNumber;
		mettbl[ SAPLAN_UpdateInterval_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].UpdateInterval;
		mettbl[ SAPLAN_AgeFactor_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].AgeFactor;
		mettbl[ SAPLAN_PacketGap_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].PacketGap;
		mettbl[ SAPLAN_PacketSize_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].PacketSize;
		mettbl[ SAPLANR_PacketsSent_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].PacketsSent;
		mettbl[ SAPLANR_PacketsReceived_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].PacketsReceived;
		mettbl[ SAPLAN_BadPktsReceived_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].BadPktsReceived;
		mettbl[ SAPLANR_BadPktsReceived_IDX ].metval[i].met_p =
		   (caddr_t)&sap_lan_buf[i].BadPktsReceived;
	}
	mettbl[ IPXLAN_InProtoSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InProtoSize;
	mettbl[ IPXLANR_InProtoSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InProtoSize;
	mettbl[ IPXLAN_InBadDLPItype_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBadDLPItype;
	mettbl[ IPXLANR_InBadDLPItype_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBadDLPItype;
	mettbl[ IPXLAN_InCoalesced_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InCoalesced;
	mettbl[ IPXLANR_InCoalesced_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InCoalesced;
	mettbl[ IPXLANR_InPropagation_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InPropagation;
	mettbl[ IPXLAN_InNoPropagate_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InNoPropagate;
	mettbl[ IPXLANR_InNoPropagate_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InNoPropagate;
	mettbl[ IPXLANR_InTotal_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InTotal;
	mettbl[ IPXLAN_InBadLength_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBadLength;
	mettbl[ IPXLANR_InBadLength_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBadLength;
	mettbl[ IPXLAN_InDriverEcho_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDriverEcho;
	mettbl[ IPXLANR_InDriverEcho_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDriverEcho;
	mettbl[ IPXLANR_InSap_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSap;
	mettbl[ IPXLAN_InRipDropped_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InRipDropped;
	mettbl[ IPXLANR_InRipDropped_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InRipDropped;
	mettbl[ IPXLANR_InRipRouted_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InRipRouted;
	mettbl[ IPXLANR_InSap_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSap;
	mettbl[ IPXLAN_InSapBad_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSapBad;
	mettbl[ IPXLANR_InSapBad_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSapBad;
	mettbl[ IPXLANR_InSapIpx_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSapIpx;
	mettbl[ IPXLANR_InSapNoIpxToSapd_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSapNoIpxToSapd;
	mettbl[ IPXLAN_InSapNoIpxDrop_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSapNoIpxDrop;
	mettbl[ IPXLANR_InSapNoIpxDrop_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InSapNoIpxDrop;
	mettbl[ IPXLAN_InDiag_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiag;
	mettbl[ IPXLANR_InDiag_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiag;
	mettbl[ IPXLAN_InDiagInternal_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagInternal;
	mettbl[ IPXLANR_InDiagInternal_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagInternal;
	mettbl[ IPXLAN_InDiagNIC_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagNIC;
	mettbl[ IPXLANR_InDiagNIC_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagNIC;
	mettbl[ IPXLAN_InDiagIpx_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagIpx;
	mettbl[ IPXLANR_InDiagIpx_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagIpx;
	mettbl[ IPXLAN_InDiagNoIpx_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagNoIpx;
	mettbl[ IPXLANR_InDiagNoIpx_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InDiagNoIpx;
	mettbl[ IPXLAN_InNICDropped_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InNICDropped;
	mettbl[ IPXLANR_InNICDropped_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InNICDropped;
	mettbl[ IPXLANR_InBroadcast_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcast;
	mettbl[ IPXLANR_InBroadcastInternal_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastInternal;
	mettbl[ IPXLANR_InBroadcastNIC_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastNIC;
	mettbl[ IPXLAN_InBroadcastDiag_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiag;
	mettbl[ IPXLANR_InBroadcastDiag_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiag;
	mettbl[ IPXLAN_InBroadcastDiagFwd_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiagFwd;
	mettbl[ IPXLANR_InBroadcastDiagFwd_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiagFwd;
	mettbl[ IPXLAN_InBroadcastDiagRoute_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiagRoute;
	mettbl[ IPXLANR_InBroadcastDiagRoute_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiagRoute;
	mettbl[ IPXLAN_InBroadcastDiagResp_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiagResp;
	mettbl[ IPXLANR_InBroadcastDiagResp_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastDiagResp;
	mettbl[ IPXLAN_InBroadcastRoute_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastRoute;
	mettbl[ IPXLANR_InBroadcastRoute_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InBroadcastRoute;
	mettbl[ IPXLANR_InForward_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InForward;
	mettbl[ IPXLANR_InRoute_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InRoute;
	mettbl[ IPXLANR_InInternalNet_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.InInternalNet;
	mettbl[ IPXLANR_OutPropagation_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutPropagation;
	mettbl[ IPXLANR_OutTotalStream_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutTotalStream;
	mettbl[ IPXLANR_OutTotal_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutTotal;
	mettbl[ IPXLAN_OutSameSocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutSameSocket;
	mettbl[ IPXLANR_OutSameSocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutSameSocket;
	mettbl[ IPXLANR_OutFillInDest_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutFillInDest;
	mettbl[ IPXLANR_OutInternal_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutInternal;
	mettbl[ IPXLAN_OutBadLan_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutBadLan;
	mettbl[ IPXLANR_OutBadLan_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutBadLan;
	mettbl[ IPXLANR_OutSent_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutSent;
	mettbl[ IPXLANR_OutQueued_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.OutQueued;
	mettbl[ IPXLANR_Ioctl_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.Ioctl;
	mettbl[ IPXLANR_IoctlSetLans_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlSetLans;
	mettbl[ IPXLANR_IoctlGetLans_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlGetLans;
	mettbl[ IPXLANR_IoctlSetSapQ_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlSetSapQ;
	mettbl[ IPXLANR_IoctlSetLanInfo_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlSetLanInfo;
	mettbl[ IPXLANR_IoctlGetLanInfo_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlGetLanInfo;
	mettbl[ IPXLANR_IoctlGetNodeAddr_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlGetNodeAddr;
	mettbl[ IPXLANR_IoctlGetNetAddr_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlGetNetAddr;
	mettbl[ IPXLANR_IoctlGetStats_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlGetStats;
	mettbl[ IPXLANR_IoctlLink_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlLink;
	mettbl[ IPXLANR_IoctlUnlink_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlUnlink;
	mettbl[ IPXLAN_IoctlUnknown_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlUnknown;
	mettbl[ IPXLANR_IoctlUnknown_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.l.IoctlUnknown;
	mettbl[ IPXSOCKR_IpxInData_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxInData;
	mettbl[ IPXR_datapackets_IDX ].metval->met_p =
	   (caddr_t)&ipx_datapackets;
	mettbl[ IPXSOCKR_IpxOutData_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxOutData;
	mettbl[ IPXSOCK_IpxOutBadSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxOutBadSize;
	mettbl[ IPXSOCKR_IpxOutBadSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxOutBadSize;
	mettbl[ IPXSOCK_IpxInBadSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxInBadSize;
	mettbl[ IPXSOCKR_IpxInBadSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxInBadSize;
	mettbl[ IPXSOCKR_IpxOutToSwitch_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxOutToSwitch;
	mettbl[ IPXSOCKR_IpxTLIOutData_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutData;
	mettbl[ IPXSOCK_IpxTLIOutBadState_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadState;
	mettbl[ IPXSOCKR_IpxTLIOutBadState_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadState;
	mettbl[ IPXSOCK_IpxTLIOutBadSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadSize;
	mettbl[ IPXSOCKR_IpxTLIOutBadSize_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadSize;
	mettbl[ IPXSOCK_IpxTLIOutBadOpt_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadOpt;
	mettbl[ IPXSOCKR_IpxTLIOutBadOpt_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadOpt;
	mettbl[ IPXSOCK_IpxTLIOutHdrAlloc_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutHdrAlloc;
	mettbl[ IPXSOCKR_IpxTLIOutHdrAlloc_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutHdrAlloc;
	mettbl[ IPXSOCKR_IpxTLIOutToSwitch_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOutToSwitch;
	mettbl[ IPXSOCK_IpxBoundSockets_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxBoundSockets;
	mettbl[ IPXSOCKR_IpxBoundSockets_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxBoundSockets;
	mettbl[ IPXSOCKR_IpxBind_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxBind;
	mettbl[ IPXSOCKR_IpxTLIBind_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIBind;
	mettbl[ IPXSOCKR_IpxTLIOptMgt_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIOptMgt;
	mettbl[ IPXSOCK_IpxTLIUnknown_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIUnknown;
	mettbl[ IPXSOCKR_IpxTLIUnknown_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTLIUnknown;
	mettbl[ IPXSOCK_IpxTLIOutBadAddr_IDX ].metval->met_p = 
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadAddr;
	mettbl[ IPXSOCKR_IpxTLIOutBadAddr_IDX ].metval->met_p = 
	   (caddr_t)&ipxbuf.s.IpxTLIOutBadAddr;
	mettbl[ IPXSOCK_IpxSwitchInvalSocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchInvalSocket;
	mettbl[ IPXSOCKR_IpxSwitchInvalSocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchInvalSocket;
	mettbl[ IPXSOCK_IpxSwitchSumFail_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchSumFail;
	mettbl[ IPXSOCKR_IpxSwitchSumFail_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchSumFail;
	mettbl[ IPXSOCK_IpxSwitchAllocFail_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchAllocFail;
	mettbl[ IPXSOCKR_IpxSwitchAllocFail_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchAllocFail;
	mettbl[ IPXSOCKR_IpxSwitchSum_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchSum;
	mettbl[ IPXSOCKR_IpxSwitchEven_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchEven;
	mettbl[ IPXSOCKR_IpxSwitchEvenAlloc_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSwitchEvenAlloc;
	mettbl[ DUMMY_1_IDX ].metval->met_p = (caddr_t)&dummy;
	mettbl[ IPXSOCKR_IpxDataToSocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxDataToSocket;
	mettbl[ IPXSOCKR_IpxTrimPacket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxTrimPacket;
	mettbl[ IPXSOCK_IpxSumFail_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSumFail;
	mettbl[ IPXSOCKR_IpxSumFail_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSumFail;
	mettbl[ IPXSOCK_IpxBusySocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxBusySocket;
	mettbl[ IPXSOCKR_IpxBusySocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxBusySocket;
	mettbl[ IPXSOCK_IpxSocketNotBound_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSocketNotBound;
	mettbl[ IPXSOCKR_IpxSocketNotBound_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxSocketNotBound;
	mettbl[ IPXSOCKR_IpxRouted_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxRouted;
	mettbl[ IPXSOCKR_IpxRoutedTLI_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxRoutedTLI;
	mettbl[ IPXSOCK_IpxRoutedTLIAlloc_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxRoutedTLIAlloc;
	mettbl[ IPXSOCKR_IpxRoutedTLIAlloc_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxRoutedTLIAlloc;
	mettbl[ IPXR_sent_to_tli_IDX ].metval->met_p =
		(caddr_t)&ipx_sent_to_TLI;
	mettbl[ IPXR_total_ioctls_IDX ].metval->met_p =
	   (caddr_t)&ipx_total_ioctls;
	mettbl[ IPXSOCKR_IpxIoctlSetWater_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxIoctlSetWater;
	mettbl[ IPXSOCKR_IpxIoctlBindSocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxIoctlBindSocket;
	mettbl[ IPXSOCKR_IpxIoctlUnbindSocket_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxIoctlUnbindSocket;
	mettbl[ IPXSOCKR_IpxIoctlStats_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxIoctlStats;
	mettbl[ IPXSOCK_IpxIoctlUnknown_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxIoctlUnknown;
	mettbl[ IPXSOCKR_IpxIoctlUnknown_IDX ].metval->met_p =
	   (caddr_t)&ipxbuf.s.IpxIoctlUnknown;
	mettbl[ RIPR_ReceivedPackets_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedPackets;
	mettbl[ RIP_ReceivedNoLanKey_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedNoLanKey;
	mettbl[ RIPR_ReceivedNoLanKey_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedNoLanKey;
	mettbl[ RIP_ReceivedBadLength_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedBadLength;
	mettbl[ RIPR_ReceivedBadLength_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedBadLength;
	mettbl[ RIPR_ReceivedCoalesced_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedCoalesced;
	mettbl[ RIP_ReceivedNoCoalesce_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedNoCoalesce;
	mettbl[ RIPR_ReceivedNoCoalesce_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedNoCoalesce;
	mettbl[ RIPR_ReceivedRequestPackets_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedRequestPackets;
	mettbl[ RIPR_ReceivedResponsePackets_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedResponsePackets;
	mettbl[ RIP_ReceivedUnknownRequest_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedUnknownRequest;
	mettbl[ RIPR_ReceivedUnknownRequest_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.ReceivedUnknownRequest;
	mettbl[ RIPR_total_router_packets_sent_IDX ].metval->met_p =
	   (caddr_t)&rip_total_router_packets_sent;
	mettbl[ RIP_SentAllocFailed_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentAllocFailed;
	mettbl[ RIPR_SentAllocFailed_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentAllocFailed;
	mettbl[ RIP_SentBadDestination_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentBadDestination;
	mettbl[ RIPR_SentBadDestination_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentBadDestination;
	mettbl[ RIPR_SentRequestPackets_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentRequestPackets;
	mettbl[ RIPR_SentResponsePackets_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentResponsePackets;
	mettbl[ RIP_SentLan0Dropped_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentLan0Dropped;
	mettbl[ RIPR_SentLan0Dropped_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentLan0Dropped;
	mettbl[ RIPR_SentLan0Routed_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.SentLan0Routed;
	mettbl[ RIPR_ioctls_processed_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlInitialize;
	mettbl[ RIPR_RipxIoctlInitialize_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlInitialize;
	mettbl[ RIPR_RipxIoctlGetHashSize_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlGetHashSize;
	mettbl[ RIPR_RipxIoctlGetHashStats_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlGetHashStats;
	mettbl[ RIPR_RipxIoctlDumpHashTable_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlDumpHashTable;
	mettbl[ RIPR_RipxIoctlGetRouterTable_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlGetRouterTable;
	mettbl[ RIPR_RipxIoctlGetNetInfo_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlGetNetInfo;
	mettbl[ RIPR_RipxIoctlCheckSapSource_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlCheckSapSource;
	mettbl[ RIPR_RipxIoctlResetRouter_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlResetRouter;
	mettbl[ RIPR_RipxIoctlDownRouter_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlDownRouter;
	mettbl[ RIPR_RipxIoctlStats_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlStats;
	mettbl[ RIP_RipxIoctlUnknown_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlUnknown;
	mettbl[ RIPR_RipxIoctlUnknown_IDX ].metval->met_p =
	   (caddr_t)&ripbuf.RipxIoctlUnknown;
	mettbl[ SPX_max_connections_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_max_connections;
	mettbl[ SPX_current_connections_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_current_connections;
	mettbl[ SPXR_current_connections_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_current_connections;
	mettbl[ SPX_alloc_failures_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_alloc_failures;
	mettbl[ SPXR_alloc_failures_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_alloc_failures;
	mettbl[ SPX_open_failures_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_open_failures;
	mettbl[ SPXR_open_failures_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_open_failures;
	mettbl[ SPXR_ioctls_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_ioctls;
	mettbl[ SPXR_connect_req_count_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_connect_req_count;
	mettbl[ SPX_connect_req_fails_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_connect_req_fails;
	mettbl[ SPXR_connect_req_fails_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_connect_req_fails;
	mettbl[ SPXR_listen_req_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_listen_req;
	mettbl[ SPX_listen_req_fails_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_listen_req_fails;
	mettbl[ SPXR_listen_req_fails_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_listen_req_fails;
	mettbl[ SPXR_send_mesg_count_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_mesg_count;
	mettbl[ SPX_unknown_mesg_count_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_unknown_mesg_count;
	mettbl[ SPXR_unknown_mesg_count_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_unknown_mesg_count;
	mettbl[ SPX_send_bad_mesg_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_bad_mesg;
	mettbl[ SPXR_send_bad_mesg_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_bad_mesg;
	mettbl[ SPXR_send_packet_count_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_packet_count;
	mettbl[ SPX_send_packet_timeout_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_packet_timeout;
	mettbl[ SPXR_send_packet_timeout_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_packet_timeout;
	mettbl[ SPX_send_packet_nak_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_packet_nak;
	mettbl[ SPXR_send_packet_nak_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_send_packet_nak;
	mettbl[ SPXR_rcv_packet_count_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_packet_count;
	mettbl[ SPX_rcv_bad_packet_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_bad_packet;
	mettbl[ SPXR_rcv_bad_packet_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_bad_packet;
	mettbl[ SPX_rcv_bad_data_packet_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_bad_data_packet;
	mettbl[ SPXR_rcv_bad_data_packet_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_bad_data_packet;
	mettbl[ SPX_rcv_dup_packet_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_dup_packet;
	mettbl[ SPXR_rcv_dup_packet_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_dup_packet;
	mettbl[ SPXR_rcv_packet_sentup_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_packet_sentup;
	mettbl[ SPX_rcv_conn_req_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_conn_req;
	mettbl[ SPXR_rcv_conn_req_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_rcv_conn_req;
	mettbl[ SPX_abort_connection_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_abort_connection;
	mettbl[ SPXR_abort_connection_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_abort_connection;
	mettbl[ SPX_max_retries_abort_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_max_retries_abort;
	mettbl[ SPXR_max_retries_abort_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_max_retries_abort;
	mettbl[ SPX_no_listeners_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_no_listeners;
	mettbl[ SPXR_no_listeners_IDX ].metval->met_p =
	   (caddr_t)&spxbuf.spx_no_listeners;
	for(i=0; i < maxspxconn; i++) {
		mettbl[ SPXCON_netaddr_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_addr.net;
		mettbl[ SPXCON_nodeaddr_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_addr.node[0];
		mettbl[ SPXCON_sockaddr_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_addr.sock[0];
		mettbl[ SPXCON_connection_id_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_connection_id;
		mettbl[ SPXCON_o_netaddr_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].o_addr.net[0];
		mettbl[ SPXCON_o_nodeaddr_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].o_addr.node[0];
		mettbl[ SPXCON_o_sockaddr_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].o_addr.sock[0];
		mettbl[ SPXCON_o_connection_id_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].o_connection_id;
		mettbl[ SPXCON_con_state_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_state;
		mettbl[ SPXCON_con_retry_count_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_retry_count;
		mettbl[ SPXCON_con_retry_time_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_retry_time;
		mettbl[ SPXCON_con_state_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_state;
		mettbl[ SPXCON_con_type_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_type;
		mettbl[ SPXCON_con_ipxChecksum_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_ipxChecksum;
		mettbl[ SPXCON_con_window_size_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_window_size;
		mettbl[ SPXCON_con_remote_window_size_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_remote_window_size;
		mettbl[ SPXCON_con_send_packet_size_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_packet_size;
		mettbl[ SPXCON_con_rcv_packet_size_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_packet_size;
		mettbl[ SPXCON_con_round_trip_time_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_round_trip_time;
		mettbl[ SPXCON_con_window_choke_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_window_choke;
		mettbl[ SPXCONR_con_send_mesg_count_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_mesg_count;
		mettbl[ SPXCON_con_unknown_mesg_count_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_unknown_mesg_count;
		mettbl[ SPXCONR_con_unknown_mesg_count_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_unknown_mesg_count;
		mettbl[ SPXCON_con_send_bad_mesg_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_bad_mesg;
		mettbl[ SPXCONR_con_send_bad_mesg_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_bad_mesg;
		mettbl[ SPXCONR_con_send_packet_count_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_packet_count;
		mettbl[ SPXCON_con_send_packet_timeout_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_packet_timeout;
		mettbl[ SPXCONR_con_send_packet_timeout_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_packet_timeout;
		mettbl[ SPXCON_con_send_packet_nak_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_packet_nak;
		mettbl[ SPXCONR_con_send_packet_nak_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_packet_nak;
		mettbl[ SPXCONR_con_send_ack_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_ack;
		mettbl[ SPXCON_con_send_nak_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_nak;
		mettbl[ SPXCONR_con_send_nak_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_nak;
		mettbl[ SPXCONR_con_send_watchdog_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_send_watchdog;
		mettbl[ SPXCONR_con_rcv_packet_count_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_packet_count;
		mettbl[ SPXCON_con_rcv_bad_packet_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_bad_packet;
		mettbl[ SPXCONR_con_rcv_bad_packet_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_bad_packet;
		mettbl[ SPXCON_con_rcv_bad_data_packet_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_bad_data_packet;
		mettbl[ SPXCONR_con_rcv_bad_data_packet_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_bad_data_packet;
		mettbl[ SPXCON_con_rcv_dup_packet_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_dup_packet;
		mettbl[ SPXCONR_con_rcv_dup_packet_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_dup_packet;
		mettbl[ SPXCON_con_rcv_packet_outseq_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_packet_outseq;
		mettbl[ SPXCONR_con_rcv_packet_outseq_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_packet_outseq;
		mettbl[ SPXCONR_con_rcv_packet_sentup_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_packet_sentup;
		mettbl[ SPXCONR_con_rcv_packet_qued_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_packet_qued;
		mettbl[ SPXCONR_con_rcv_ack_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_ack;
		mettbl[ SPXCON_con_rcv_nak_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_nak;
		mettbl[ SPXCONR_con_rcv_nak_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_nak;
		mettbl[ SPXCONR_con_rcv_watchdog_IDX ].metval[i].met_p =
		   (caddr_t)&spxconbuf[i].con_rcv_watchdog;
	}

	mettbl[ SAPR_total_IDX ].metval->met_p =
		   (caddr_t)&sap_total;
	mettbl[ SPXR_total_IDX ].metval->met_p =
		   (caddr_t)&spx_total;
	mettbl[ IPXR_total_IDX ].metval->met_p =
		   (caddr_t)&ipx_total;
	mettbl[ RIPR_total_IDX ].metval->met_p =
		   (caddr_t)&rip_total;
	mettbl[ NETWARE_errs_IDX ].metval->met_p =
		   (caddr_t)&netware_errs;
	mettbl[ NETWARER_errs_IDX ].metval->met_p =
		   (caddr_t)&netware_errs;
	mettbl[ SPX_max_used_connections_IDX ].metval->met_p = 
		  (caddr_t)&spxbuf.spx_max_used_connections;

 	mettbl[ EISA_BUS_UTIL_SUMCNT_IDX ].metval->met_p = 
		(caddr_t)&eisa_bus_util_sumcnt;
 	mettbl[ EISA_BUS_UTIL_PERCENT_IDX ].metval->met_p = 
		(caddr_t)&eisa_bus_util_sum;

 	mettbl[ LBOLT_IDX ].metval->met_p = (caddr_t)&currtime;

	/* set object sizes of kludged metrics */

	mettbl[ETH_InUcastPkts_IDX].objsz = sizeof(etherstat[i]->ifInUcastPkts);
	mettbl[ETH_OutUcastPkts_IDX].objsz = sizeof(etherstat[i]->ifOutUcastPkts);
	mettbl[ETH_InNUcastPkts_IDX].objsz = sizeof(etherstat[i]->ifInNUcastPkts);
	mettbl[ETH_OutNUcastPkts_IDX].objsz = sizeof(etherstat[i]->ifOutNUcastPkts);
	mettbl[ETH_InOctets_IDX].objsz = sizeof(etherstat[i]->ifInOctets);
	mettbl[ETH_OutOctets_IDX].objsz = sizeof(etherstat[i]->ifOutOctets);
	mettbl[ETH_InErrors_IDX].objsz = sizeof(etherstat[i]->ifInErrors);
	mettbl[ETHR_InErrors_IDX].objsz = sizeof(etherstat[i]->ifInErrors);
	mettbl[ETH_etherAlignErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherAlignErrors);
	mettbl[ETHR_etherAlignErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherAlignErrors);
	mettbl[ETH_etherCRCerrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherCRCerrors);
	mettbl[ETHR_etherCRCerrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherCRCerrors);
	mettbl[ETH_etherOverrunErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherOverrunErrors);
	mettbl[ETHR_etherOverrunErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherOverrunErrors);
	mettbl[ETH_etherUnderrunErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherUnderrunErrors);
	mettbl[ETHR_etherUnderrunErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherUnderrunErrors);
	mettbl[ETH_etherMissedPkts_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherMissedPkts);
	mettbl[ETHR_etherMissedPkts_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherMissedPkts);
	mettbl[ETH_InDiscards_IDX].objsz = sizeof(etherstat[i]->ifInDiscards);
	mettbl[ETHR_InDiscards_IDX].objsz = sizeof(etherstat[i]->ifInDiscards);
	mettbl[ETH_etherReadqFull_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherReadqFull);
	mettbl[ETHR_etherReadqFull_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherReadqFull);
	mettbl[ETH_etherRcvResources_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherRcvResources);
	mettbl[ETHR_etherRcvResources_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherRcvResources);
	mettbl[ETH_etherCollisions_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherCollisions);
	mettbl[ETHR_etherCollisions_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherCollisions);
	mettbl[ETH_OutDiscards_IDX].objsz = sizeof(etherstat[i]->ifOutDiscards);
	mettbl[ETHR_OutDiscards_IDX].objsz = sizeof(etherstat[i]->ifOutDiscards);
	mettbl[ETH_OutErrors_IDX].objsz = sizeof(etherstat[i]->ifOutErrors);
	mettbl[ETHR_OutErrors_IDX].objsz = sizeof(etherstat[i]->ifOutErrors);
	mettbl[ETH_etherAbortErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherAbortErrors);
	mettbl[ETHR_etherAbortErrors_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherAbortErrors);
	mettbl[ETH_etherCarrierLost_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherCarrierLost);
	mettbl[ETHR_etherCarrierLost_IDX].objsz = sizeof(etherstat[i]->ifSpecific.etherCarrierLost);
	mettbl[ETH_OutQlen_IDX].objsz = sizeof(etherstat[i]->ifOutQlen);
	mettbl[IPR_total_IDX].objsz = sizeof(ipstat.ips_total);
	mettbl[IP_badsum_IDX].objsz = sizeof(ipstat.ips_badsum);
	mettbl[IPR_badsum_IDX].objsz = sizeof(ipstat.ips_badsum);
	mettbl[IP_tooshort_IDX].objsz = sizeof(ipstat.ips_tooshort);
	mettbl[IPR_tooshort_IDX].objsz = sizeof(ipstat.ips_tooshort);
	mettbl[IP_toosmall_IDX].objsz = sizeof(ipstat.ips_toosmall);
	mettbl[IPR_toosmall_IDX].objsz = sizeof(ipstat.ips_toosmall);
	mettbl[IP_badhlen_IDX].objsz = sizeof(ipstat.ips_badhlen);
	mettbl[IPR_badhlen_IDX].objsz = sizeof(ipstat.ips_badhlen);
	mettbl[IP_badlen_IDX].objsz = sizeof(ipstat.ips_badlen);
	mettbl[IPR_badlen_IDX].objsz = sizeof(ipstat.ips_badlen);
	mettbl[IP_unknownproto_IDX].objsz = sizeof(ipstat.ips_unknownproto);
	mettbl[IPR_unknownproto_IDX].objsz = sizeof(ipstat.ips_unknownproto);
	mettbl[IP_fragments_IDX].objsz = sizeof(ipstat.ips_fragments);
	mettbl[IPR_fragments_IDX].objsz = sizeof(ipstat.ips_fragments);
	mettbl[IP_fragdropped_IDX].objsz = sizeof(ipstat.ips_fragdropped);
	mettbl[IPR_fragdropped_IDX].objsz = sizeof(ipstat.ips_fragdropped);
	mettbl[IP_fragtimeout_IDX].objsz = sizeof(ipstat.ips_fragtimeout);
	mettbl[IPR_fragtimeout_IDX].objsz = sizeof(ipstat.ips_fragtimeout);
	mettbl[IP_reasms_IDX].objsz = sizeof(ipstat.ips_reasms);
	mettbl[IPR_reasms_IDX].objsz = sizeof(ipstat.ips_reasms);
	mettbl[IP_forward_IDX].objsz = sizeof(ipstat.ips_forward);
	mettbl[IPR_forward_IDX].objsz = sizeof(ipstat.ips_forward);
	mettbl[IP_cantforward_IDX].objsz = sizeof(ipstat.ips_cantforward);
	mettbl[IPR_cantforward_IDX].objsz = sizeof(ipstat.ips_cantforward);
	mettbl[IP_noroutes_IDX].objsz = sizeof(ipstat.ips_noroutes);
	mettbl[IPR_noroutes_IDX].objsz = sizeof(ipstat.ips_noroutes);
	mettbl[IP_redirectsent_IDX].objsz = sizeof(ipstat.ips_redirectsent);
	mettbl[IPR_redirectsent_IDX].objsz = sizeof(ipstat.ips_redirectsent);
	mettbl[IP_inerrors_IDX].objsz = sizeof(ipstat.ips_inerrors);
	mettbl[IPR_inerrors_IDX].objsz = sizeof(ipstat.ips_inerrors);
	mettbl[IPR_indelivers_IDX].objsz = sizeof(ipstat.ips_indelivers);
	mettbl[IPR_outrequests_IDX].objsz = sizeof(ipstat.ips_outrequests);
	mettbl[IP_outerrors_IDX].objsz = sizeof(ipstat.ips_outerrors);
	mettbl[IPR_outerrors_IDX].objsz = sizeof(ipstat.ips_outerrors);
	mettbl[IP_pfrags_IDX].objsz = sizeof(ipstat.ips_pfrags);
	mettbl[IPR_pfrags_IDX].objsz = sizeof(ipstat.ips_pfrags);
	mettbl[IP_fragfails_IDX].objsz = sizeof(ipstat.ips_fragfails);
	mettbl[IPR_fragfails_IDX].objsz = sizeof(ipstat.ips_fragfails);
	mettbl[IP_frags_IDX].objsz = sizeof(ipstat.ips_frags);
	mettbl[IPR_frags_IDX].objsz = sizeof(ipstat.ips_frags);
	mettbl[ICMP_error_IDX].objsz = sizeof(icmpstat.icps_error);
	mettbl[ICMPR_error_IDX].objsz = sizeof(icmpstat.icps_error);
	mettbl[ICMP_oldicmp_IDX].objsz = sizeof(icmpstat.icps_oldicmp);
	mettbl[ICMPR_oldicmp_IDX].objsz = sizeof(icmpstat.icps_oldicmp);
	mettbl[ICMP_outhist0_IDX].objsz = sizeof(icmpstat.icps_outhist[0]);
	mettbl[ICMPR_outhist0_IDX].objsz = sizeof(icmpstat.icps_outhist[0]);
	mettbl[ICMP_outhist3_IDX].objsz = sizeof(icmpstat.icps_outhist[3]);
	mettbl[ICMPR_outhist3_IDX].objsz = sizeof(icmpstat.icps_outhist[3]);
	mettbl[ICMP_outhist4_IDX].objsz = sizeof(icmpstat.icps_outhist[4]);
	mettbl[ICMPR_outhist4_IDX].objsz = sizeof(icmpstat.icps_outhist[4]);
	mettbl[ICMP_outhist5_IDX].objsz = sizeof(icmpstat.icps_outhist[5]);
	mettbl[ICMPR_outhist5_IDX].objsz = sizeof(icmpstat.icps_outhist[5]);
	mettbl[ICMP_outhist8_IDX].objsz = sizeof(icmpstat.icps_outhist[8]);
	mettbl[ICMPR_outhist8_IDX].objsz = sizeof(icmpstat.icps_outhist[8]);
	mettbl[ICMP_outhist11_IDX].objsz = sizeof(icmpstat.icps_outhist[11]);
	mettbl[ICMPR_outhist11_IDX].objsz = sizeof(icmpstat.icps_outhist[11]);
	mettbl[ICMP_outhist12_IDX].objsz = sizeof(icmpstat.icps_outhist[12]);
	mettbl[ICMPR_outhist12_IDX].objsz = sizeof(icmpstat.icps_outhist[12]);
	mettbl[ICMP_outhist13_IDX].objsz = sizeof(icmpstat.icps_outhist[13]);
	mettbl[ICMPR_outhist13_IDX].objsz = sizeof(icmpstat.icps_outhist[13]);
	mettbl[ICMP_outhist14_IDX].objsz = sizeof(icmpstat.icps_outhist[14]);
	mettbl[ICMPR_outhist14_IDX].objsz = sizeof(icmpstat.icps_outhist[14]);
	mettbl[ICMP_outhist15_IDX].objsz = sizeof(icmpstat.icps_outhist[15]);
	mettbl[ICMPR_outhist15_IDX].objsz = sizeof(icmpstat.icps_outhist[15]);
	mettbl[ICMP_outhist16_IDX].objsz = sizeof(icmpstat.icps_outhist[16]);
	mettbl[ICMPR_outhist16_IDX].objsz = sizeof(icmpstat.icps_outhist[16]);
	mettbl[ICMP_outhist17_IDX].objsz = sizeof(icmpstat.icps_outhist[17]);
	mettbl[ICMPR_outhist17_IDX].objsz = sizeof(icmpstat.icps_outhist[17]);
	mettbl[ICMP_outhist18_IDX].objsz = sizeof(icmpstat.icps_outhist[18]);
	mettbl[ICMPR_outhist18_IDX].objsz = sizeof(icmpstat.icps_outhist[18]);
	mettbl[ICMP_badcode_IDX].objsz = sizeof(icmpstat.icps_badcode);
	mettbl[ICMPR_badcode_IDX].objsz = sizeof(icmpstat.icps_badcode);
	mettbl[ICMP_tooshort_IDX].objsz = sizeof(icmpstat.icps_tooshort);
	mettbl[ICMPR_tooshort_IDX].objsz = sizeof(icmpstat.icps_tooshort);
	mettbl[ICMP_checksum_IDX].objsz = sizeof(icmpstat.icps_checksum);
	mettbl[ICMPR_checksum_IDX].objsz = sizeof(icmpstat.icps_checksum);
	mettbl[ICMP_badlen_IDX].objsz = sizeof(icmpstat.icps_badlen);
	mettbl[ICMPR_badlen_IDX].objsz = sizeof(icmpstat.icps_badlen);
	mettbl[ICMP_inhist0_IDX].objsz = sizeof(icmpstat.icps_inhist[0]);
	mettbl[ICMPR_inhist0_IDX].objsz = sizeof(icmpstat.icps_inhist[0]);
	mettbl[ICMP_inhist3_IDX].objsz = sizeof(icmpstat.icps_inhist[3]);
	mettbl[ICMPR_inhist3_IDX].objsz = sizeof(icmpstat.icps_inhist[3]);
	mettbl[ICMP_inhist4_IDX].objsz = sizeof(icmpstat.icps_inhist[4]);
	mettbl[ICMPR_inhist4_IDX].objsz = sizeof(icmpstat.icps_inhist[4]);
	mettbl[ICMP_inhist5_IDX].objsz = sizeof(icmpstat.icps_inhist[5]);
	mettbl[ICMPR_inhist5_IDX].objsz = sizeof(icmpstat.icps_inhist[5]);
	mettbl[ICMP_inhist8_IDX].objsz = sizeof(icmpstat.icps_inhist[8]);
	mettbl[ICMPR_inhist8_IDX].objsz = sizeof(icmpstat.icps_inhist[8]);
	mettbl[ICMP_inhist11_IDX].objsz = sizeof(icmpstat.icps_inhist[11]);
	mettbl[ICMPR_inhist11_IDX].objsz = sizeof(icmpstat.icps_inhist[11]);
	mettbl[ICMP_inhist12_IDX].objsz = sizeof(icmpstat.icps_inhist[12]);
	mettbl[ICMPR_inhist12_IDX].objsz = sizeof(icmpstat.icps_inhist[12]);
	mettbl[ICMP_inhist13_IDX].objsz = sizeof(icmpstat.icps_inhist[13]);
	mettbl[ICMPR_inhist13_IDX].objsz = sizeof(icmpstat.icps_inhist[13]);
	mettbl[ICMP_inhist14_IDX].objsz = sizeof(icmpstat.icps_inhist[14]);
	mettbl[ICMPR_inhist14_IDX].objsz = sizeof(icmpstat.icps_inhist[14]);
	mettbl[ICMP_inhist15_IDX].objsz = sizeof(icmpstat.icps_inhist[15]);
	mettbl[ICMPR_inhist15_IDX].objsz = sizeof(icmpstat.icps_inhist[15]);
	mettbl[ICMP_inhist16_IDX].objsz = sizeof(icmpstat.icps_inhist[16]);
	mettbl[ICMPR_inhist16_IDX].objsz = sizeof(icmpstat.icps_inhist[16]);
	mettbl[ICMP_inhist17_IDX].objsz = sizeof(icmpstat.icps_inhist[17]);
	mettbl[ICMPR_inhist17_IDX].objsz = sizeof(icmpstat.icps_inhist[17]);
	mettbl[ICMP_inhist18_IDX].objsz = sizeof(icmpstat.icps_inhist[18]);
	mettbl[ICMPR_inhist18_IDX].objsz = sizeof(icmpstat.icps_inhist[18]);
	mettbl[ICMP_reflect_IDX].objsz = sizeof(icmpstat.icps_reflect);
	mettbl[ICMPR_reflect_IDX].objsz = sizeof(icmpstat.icps_reflect);
	mettbl[ICMPR_intotal_IDX].objsz = sizeof(icmpstat.icps_intotal);
	mettbl[ICMPR_outtotal_IDX].objsz = sizeof(icmpstat.icps_outtotal);
	mettbl[ICMP_outerrors_IDX].objsz = sizeof(icmpstat.icps_outerrors);
	mettbl[ICMPR_outerrors_IDX].objsz = sizeof(icmpstat.icps_outerrors);
	mettbl[TCPR_sndtotal_IDX].objsz = sizeof(tcpstat.tcps_sndtotal);
	mettbl[TCPR_sndpack_IDX].objsz = sizeof(tcpstat.tcps_sndpack);
	mettbl[TCPR_sndbyte_IDX].objsz = sizeof(tcpstat.tcps_sndbyte);
	mettbl[TCP_sndrexmitpack_IDX].objsz = sizeof(tcpstat.tcps_sndrexmitpack);
	mettbl[TCPR_sndrexmitpack_IDX].objsz = sizeof(tcpstat.tcps_sndrexmitpack);
	mettbl[TCP_sndrexmitbyte_IDX].objsz = sizeof(tcpstat.tcps_sndrexmitbyte);
	mettbl[TCPR_sndrexmitbyte_IDX].objsz = sizeof(tcpstat.tcps_sndrexmitbyte);
	mettbl[TCP_sndacks_IDX].objsz = sizeof(tcpstat.tcps_sndacks);
	mettbl[TCPR_sndacks_IDX].objsz = sizeof(tcpstat.tcps_sndacks);
	mettbl[TCP_delack_IDX].objsz = sizeof(tcpstat.tcps_delack);
	mettbl[TCPR_delack_IDX].objsz = sizeof(tcpstat.tcps_delack);
	mettbl[TCP_sndurg_IDX].objsz = sizeof(tcpstat.tcps_sndurg);
	mettbl[TCPR_sndurg_IDX].objsz = sizeof(tcpstat.tcps_sndurg);
	mettbl[TCP_sndprobe_IDX].objsz = sizeof(tcpstat.tcps_sndprobe);
	mettbl[TCPR_sndprobe_IDX].objsz = sizeof(tcpstat.tcps_sndprobe);
	mettbl[TCP_sndwinup_IDX].objsz = sizeof(tcpstat.tcps_sndwinup);
	mettbl[TCPR_sndwinup_IDX].objsz = sizeof(tcpstat.tcps_sndwinup);
	mettbl[TCP_sndctrl_IDX].objsz = sizeof(tcpstat.tcps_sndctrl);
	mettbl[TCPR_sndctrl_IDX].objsz = sizeof(tcpstat.tcps_sndctrl);
	mettbl[TCP_sndrsts_IDX].objsz = sizeof(tcpstat.tcps_sndrsts);
	mettbl[TCPR_sndrsts_IDX].objsz = sizeof(tcpstat.tcps_sndrsts);
	mettbl[TCP_rcvtotal_IDX].objsz = sizeof(tcpstat.tcps_rcvtotal);
	mettbl[TCPR_rcvtotal_IDX].objsz = sizeof(tcpstat.tcps_rcvtotal);
	mettbl[TCP_rcvackpack_IDX].objsz = sizeof(tcpstat.tcps_rcvackpack);
	mettbl[TCPR_rcvackpack_IDX].objsz = sizeof(tcpstat.tcps_rcvackpack);
	mettbl[TCP_rcvackbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvackbyte);
	mettbl[TCPR_rcvackbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvackbyte);
	mettbl[TCP_rcvdupack_IDX].objsz = sizeof(tcpstat.tcps_rcvdupack);
	mettbl[TCPR_rcvdupack_IDX].objsz = sizeof(tcpstat.tcps_rcvdupack);
	mettbl[TCP_rcvacktoomuch_IDX].objsz = sizeof(tcpstat.tcps_rcvacktoomuch);
	mettbl[TCPR_rcvacktoomuch_IDX].objsz = sizeof(tcpstat.tcps_rcvacktoomuch);
	mettbl[TCP_rcvpack_IDX].objsz = sizeof(tcpstat.tcps_rcvpack);
	mettbl[TCPR_rcvpack_IDX].objsz = sizeof(tcpstat.tcps_rcvpack);
	mettbl[TCP_rcvbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvbyte);
	mettbl[TCPR_rcvbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvbyte);
	mettbl[TCP_rcvduppack_IDX].objsz = sizeof(tcpstat.tcps_rcvduppack);
	mettbl[TCPR_rcvduppack_IDX].objsz = sizeof(tcpstat.tcps_rcvduppack);
	mettbl[TCP_rcvdupbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvdupbyte);
	mettbl[TCPR_rcvdupbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvdupbyte);
	mettbl[TCP_rcvpartduppack_IDX].objsz = sizeof(tcpstat.tcps_rcvpartduppack);
	mettbl[TCPR_rcvpartduppack_IDX].objsz = sizeof(tcpstat.tcps_rcvpartduppack);
	mettbl[TCP_rcvpartdupbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvpartdupbyte);
	mettbl[TCPR_rcvpartdupbyte_IDX].objsz = sizeof(tcpstat.tcps_rcvpartdupbyte);
	mettbl[TCP_rcvoopack_IDX].objsz = sizeof(tcpstat.tcps_rcvoopack);
	mettbl[TCPR_rcvoopack_IDX].objsz = sizeof(tcpstat.tcps_rcvoopack);
	mettbl[TCP_rcvoobyte_IDX].objsz = sizeof(tcpstat.tcps_rcvoobyte);
	mettbl[TCPR_rcvoobyte_IDX].objsz = sizeof(tcpstat.tcps_rcvoobyte);
	mettbl[TCP_rcvpackafterwin_IDX].objsz = sizeof(tcpstat.tcps_rcvpackafterwin);
	mettbl[TCPR_rcvpackafterwin_IDX].objsz = sizeof(tcpstat.tcps_rcvpackafterwin);
	mettbl[TCP_rcvbyteafterwin_IDX].objsz = sizeof(tcpstat.tcps_rcvbyteafterwin);
	mettbl[TCPR_rcvbyteafterwin_IDX].objsz = sizeof(tcpstat.tcps_rcvbyteafterwin);
	mettbl[TCP_rcvwinprobe_IDX].objsz = sizeof(tcpstat.tcps_rcvwinprobe);
	mettbl[TCPR_rcvwinprobe_IDX].objsz = sizeof(tcpstat.tcps_rcvwinprobe);
	mettbl[TCP_rcvwinupd_IDX].objsz = sizeof(tcpstat.tcps_rcvwinupd);
	mettbl[TCPR_rcvwinupd_IDX].objsz = sizeof(tcpstat.tcps_rcvwinupd);
	mettbl[TCP_rcvafterclose_IDX].objsz = sizeof(tcpstat.tcps_rcvafterclose);
	mettbl[TCPR_rcvafterclose_IDX].objsz = sizeof(tcpstat.tcps_rcvafterclose);
	mettbl[TCP_rcvbadsum_IDX].objsz = sizeof(tcpstat.tcps_rcvbadsum);
	mettbl[TCPR_rcvbadsum_IDX].objsz = sizeof(tcpstat.tcps_rcvbadsum);
	mettbl[TCP_rcvbadoff_IDX].objsz = sizeof(tcpstat.tcps_rcvbadoff);
	mettbl[TCPR_rcvbadoff_IDX].objsz = sizeof(tcpstat.tcps_rcvbadoff);
	mettbl[TCP_rcvshort_IDX].objsz = sizeof(tcpstat.tcps_rcvshort);
	mettbl[TCPR_rcvshort_IDX].objsz = sizeof(tcpstat.tcps_rcvshort);
	mettbl[TCP_inerrors_IDX].objsz = sizeof(tcpstat.tcps_inerrors);
	mettbl[TCPR_inerrors_IDX].objsz = sizeof(tcpstat.tcps_inerrors);
	mettbl[TCP_connattempt_IDX].objsz = sizeof(tcpstat.tcps_connattempt);
	mettbl[TCPR_connattempt_IDX].objsz = sizeof(tcpstat.tcps_connattempt);
	mettbl[TCP_accepts_IDX].objsz = sizeof(tcpstat.tcps_accepts);
	mettbl[TCPR_accepts_IDX].objsz = sizeof(tcpstat.tcps_accepts);
	mettbl[TCP_connects_IDX].objsz = sizeof(tcpstat.tcps_connects);
	mettbl[TCPR_connects_IDX].objsz = sizeof(tcpstat.tcps_connects);
	mettbl[TCP_closed_IDX].objsz = sizeof(tcpstat.tcps_closed);
	mettbl[TCPR_closed_IDX].objsz = sizeof(tcpstat.tcps_closed);
	mettbl[TCP_drops_IDX].objsz = sizeof(tcpstat.tcps_drops);
	mettbl[TCPR_drops_IDX].objsz = sizeof(tcpstat.tcps_drops);
	mettbl[TCP_conndrops_IDX].objsz = sizeof(tcpstat.tcps_conndrops);
	mettbl[TCPR_conndrops_IDX].objsz = sizeof(tcpstat.tcps_conndrops);
	mettbl[TCP_attemptfails_IDX].objsz = sizeof(tcpstat.tcps_attemptfails);
	mettbl[TCPR_attemptfails_IDX].objsz = sizeof(tcpstat.tcps_attemptfails);
	mettbl[TCP_estabresets_IDX].objsz = sizeof(tcpstat.tcps_estabresets);
	mettbl[TCPR_estabresets_IDX].objsz = sizeof(tcpstat.tcps_estabresets);
	mettbl[TCP_rttupdated_IDX].objsz = sizeof(tcpstat.tcps_rttupdated);
	mettbl[TCPR_rttupdated_IDX].objsz = sizeof(tcpstat.tcps_rttupdated);
	mettbl[TCP_segstimed_IDX].objsz = sizeof(tcpstat.tcps_segstimed);
	mettbl[TCPR_segstimed_IDX].objsz = sizeof(tcpstat.tcps_segstimed);
	mettbl[TCP_rexmttimeo_IDX].objsz = sizeof(tcpstat.tcps_rexmttimeo);
	mettbl[TCPR_rexmttimeo_IDX].objsz = sizeof(tcpstat.tcps_rexmttimeo);
	mettbl[TCP_timeoutdrop_IDX].objsz = sizeof(tcpstat.tcps_timeoutdrop);
	mettbl[TCPR_timeoutdrop_IDX].objsz = sizeof(tcpstat.tcps_timeoutdrop);
	mettbl[TCP_persisttimeo_IDX].objsz = sizeof(tcpstat.tcps_persisttimeo);
	mettbl[TCPR_persisttimeo_IDX].objsz = sizeof(tcpstat.tcps_persisttimeo);
	mettbl[TCP_keeptimeo_IDX].objsz = sizeof(tcpstat.tcps_keeptimeo);
	mettbl[TCPR_keeptimeo_IDX].objsz = sizeof(tcpstat.tcps_keeptimeo);
	mettbl[TCP_keepprobe_IDX].objsz = sizeof(tcpstat.tcps_keepprobe);
	mettbl[TCPR_keepprobe_IDX].objsz = sizeof(tcpstat.tcps_keepprobe);
	mettbl[TCP_keepdrops_IDX].objsz = sizeof(tcpstat.tcps_keepdrops);
	mettbl[TCPR_keepdrops_IDX].objsz = sizeof(tcpstat.tcps_keepdrops);
	mettbl[TCP_linger_IDX].objsz = sizeof(tcpstat.tcps_linger);
	mettbl[TCPR_linger_IDX].objsz = sizeof(tcpstat.tcps_linger);
	mettbl[TCP_lingerexp_IDX].objsz = sizeof(tcpstat.tcps_lingerexp);
	mettbl[TCPR_lingerexp_IDX].objsz = sizeof(tcpstat.tcps_lingerexp);
	mettbl[TCP_lingercan_IDX].objsz = sizeof(tcpstat.tcps_lingercan);
	mettbl[TCPR_lingercan_IDX].objsz = sizeof(tcpstat.tcps_lingercan);
	mettbl[TCP_lingerabort_IDX].objsz = sizeof(tcpstat.tcps_lingerabort);
	mettbl[TCPR_lingerabort_IDX].objsz = sizeof(tcpstat.tcps_lingerabort);
	mettbl[UDP_hdrops_IDX].objsz = sizeof(udpstat.udps_hdrops);
	mettbl[UDPR_hdrops_IDX].objsz = sizeof(udpstat.udps_hdrops);
	mettbl[UDP_badlen_IDX].objsz = sizeof(udpstat.udps_badlen);
	mettbl[UDPR_badlen_IDX].objsz = sizeof(udpstat.udps_badlen);
	mettbl[UDP_badsum_IDX].objsz = sizeof(udpstat.udps_badsum);
	mettbl[UDPR_badsum_IDX].objsz = sizeof(udpstat.udps_badsum);
	mettbl[UDP_fullsock_IDX].objsz = sizeof(udpstat.udps_fullsock);
	mettbl[UDPR_fullsock_IDX].objsz = sizeof(udpstat.udps_fullsock);
	mettbl[UDP_noports_IDX].objsz = sizeof(udpstat.udps_noports);
	mettbl[UDPR_noports_IDX].objsz = sizeof(udpstat.udps_noports);
	mettbl[UDPR_indelivers_IDX].objsz = sizeof(udpstat.udps_indelivers);
	mettbl[UDP_inerrors_IDX].objsz = sizeof(udpstat.udps_inerrors);
	mettbl[UDPR_inerrors_IDX].objsz = sizeof(udpstat.udps_inerrors);
	mettbl[UDPR_outtotal_IDX].objsz = sizeof(udpstat.udps_outtotal);
	mettbl[IPR_sum_IDX].objsz = sizeof(net_total.ip);
	mettbl[ICMPR_sum_IDX].objsz = sizeof(net_total.icmp);
	mettbl[UDPR_sum_IDX].objsz = sizeof(net_total.udp);
	mettbl[TCPR_sum_IDX].objsz = sizeof(net_total.tcp);
	mettbl[NETERR_sum_IDX].objsz = sizeof(net_total.errs);
	mettbl[NETERRR_sum_IDX].objsz = sizeof(net_total.errs);
 	mettbl[ DSK_SWAPPG_IDX ].objsz = sizeof(dsk_swappg);
 	mettbl[ DSK_SWAPPGFREE_IDX ].objsz = sizeof(dsk_swappgfree);
 	mettbl[ MEM_SWAPPG_IDX ].objsz = sizeof(mem_swappg);

	mettbl[ SAP_total_servers_IDX ].objsz = sizeof(sap_total_servers);
	mettbl[ SAP_Lans_IDX ].objsz = sizeof(sap_buf.Lans);
	mettbl[ SAP_unused_IDX ].objsz = sizeof(sap_unused);
	mettbl[ SAPR_TotalInSaps_IDX ].objsz = sizeof(sap_buf.TotalInSaps);
	mettbl[ SAPR_GSQReceived_IDX ].objsz = sizeof(sap_buf.GSQReceived);
	mettbl[ SAPR_GSRReceived_IDX ].objsz = sizeof(sap_buf.GSRReceived);
	mettbl[ SAPR_NSQReceived_IDX ].objsz = sizeof(sap_buf.NSQReceived);
	mettbl[ SAPR_SASReceived_IDX ].objsz = sizeof(sap_buf.SASReceived);
	mettbl[ SAPR_SNCReceived_IDX ].objsz = sizeof(sap_buf.SNCReceived);
	mettbl[ SAPR_GSIReceived_IDX ].objsz = sizeof(sap_buf.GSIReceived);
	mettbl[ SAP_NotNeighbor_IDX ].objsz = sizeof(sap_buf.NotNeighbor);
	mettbl[ SAPR_NotNeighbor_IDX ].objsz = sizeof(sap_buf.NotNeighbor);
	mettbl[ SAP_EchoMyOutput_IDX ].objsz = sizeof(sap_buf.EchoMyOutput);
	mettbl[ SAPR_EchoMyOutput_IDX ].objsz = sizeof(sap_buf.EchoMyOutput);
	mettbl[ SAP_BadSizeInSaps_IDX ].objsz = sizeof(sap_buf.BadSizeInSaps);
	mettbl[ SAPR_BadSizeInSaps_IDX ].objsz = sizeof(sap_buf.BadSizeInSaps);
	mettbl[ SAP_BadSapSource_IDX ].objsz = sizeof(sap_buf.BadSapSource);
	mettbl[ SAPR_BadSapSource_IDX ].objsz = sizeof(sap_buf.BadSapSource);
	mettbl[ SAPR_TotalOutSaps_IDX ].objsz = sizeof(sap_buf.TotalOutSaps);
	mettbl[ SAPR_NSRSent_IDX ].objsz = sizeof(sap_buf.NSRSent);
	mettbl[ SAPR_GSRSent_IDX ].objsz = sizeof(sap_buf.GSRSent);
	mettbl[ SAPR_GSQSent_IDX ].objsz = sizeof(sap_buf.GSQSent);
	mettbl[ SAPR_SASAckSent_IDX ].objsz = sizeof(sap_buf.SASAckSent);
	mettbl[ SAP_SASNackSent_IDX ].objsz = sizeof(sap_buf.SASNackSent);
	mettbl[ SAPR_SASNackSent_IDX ].objsz = sizeof(sap_buf.SASNackSent);
	mettbl[ SAPR_SNCAckSent_IDX ].objsz = sizeof(sap_buf.SNCAckSent);
	mettbl[ SAP_SNCNackSent_IDX ].objsz = sizeof(sap_buf.SNCNackSent);
	mettbl[ SAPR_SNCNackSent_IDX ].objsz = sizeof(sap_buf.SNCNackSent);
	mettbl[ SAPR_GSIAckSent_IDX ].objsz = sizeof(sap_buf.GSIAckSent);
	mettbl[ SAP_BadDestOutSaps_IDX ].objsz = sizeof(sap_buf.BadDestOutSaps);
	mettbl[ SAPR_BadDestOutSaps_IDX ].objsz = sizeof(sap_buf.BadDestOutSaps);
	mettbl[ SAP_SrvAllocFailed_IDX ].objsz = sizeof(sap_buf.SrvAllocFailed);
	mettbl[ SAPR_SrvAllocFailed_IDX ].objsz = sizeof(sap_buf.SrvAllocFailed);
	mettbl[ SAP_MallocFailed_IDX ].objsz = sizeof(sap_buf.MallocFailed);
	mettbl[ SAPR_MallocFailed_IDX ].objsz = sizeof(sap_buf.MallocFailed);
	mettbl[ SAP_TotalInRipSaps_IDX ].objsz = sizeof(sap_buf.TotalInRipSaps);
	mettbl[ SAPR_TotalInRipSaps_IDX ].objsz = sizeof(sap_buf.TotalInRipSaps);
	mettbl[ SAP_BadRipSaps_IDX ].objsz = sizeof(sap_buf.BadRipSaps);
	mettbl[ SAPR_BadRipSaps_IDX ].objsz = sizeof(sap_buf.BadRipSaps);
	mettbl[ SAP_RipServerDown_IDX ].objsz = sizeof(sap_buf.RipServerDown);
	mettbl[ SAPR_RipServerDown_IDX ].objsz = sizeof(sap_buf.RipServerDown);
	mettbl[ SAPR_ProcessesToNotify_IDX ].objsz = sizeof(sap_buf.ProcessesToNotify);
	mettbl[ SAPR_NotificationsSent_IDX ].objsz = sizeof(sap_buf.NotificationsSent);
	mettbl[ SAPLAN_Network_IDX ].objsz = sizeof(sap_lan_buf[i].Network);
	mettbl[ SAPLAN_LanNumber_IDX ].objsz = sizeof(sap_lan_buf[i].LanNumber);
	mettbl[ SAPLAN_UpdateInterval_IDX ].objsz = sizeof(sap_lan_buf[i].UpdateInterval);
	mettbl[ SAPLAN_AgeFactor_IDX ].objsz = sizeof(sap_lan_buf[i].AgeFactor);
	mettbl[ SAPLAN_PacketGap_IDX ].objsz = sizeof(sap_lan_buf[i].PacketGap);
	mettbl[ SAPLAN_PacketSize_IDX ].objsz = sizeof(sap_lan_buf[i].PacketSize);
	mettbl[ SAPLANR_PacketsSent_IDX ].objsz = sizeof(sap_lan_buf[i].PacketsSent);
	mettbl[ SAPLANR_PacketsReceived_IDX ].objsz = sizeof(sap_lan_buf[i].PacketsReceived);
	mettbl[ SAPLAN_BadPktsReceived_IDX ].objsz = sizeof(sap_lan_buf[i].BadPktsReceived);
	mettbl[ SAPLANR_BadPktsReceived_IDX ].objsz = sizeof(sap_lan_buf[i].BadPktsReceived);
	mettbl[ IPXLAN_InProtoSize_IDX ].objsz = sizeof(ipxbuf.l.InProtoSize);
	mettbl[ IPXLANR_InProtoSize_IDX ].objsz = sizeof(ipxbuf.l.InProtoSize);
	mettbl[ IPXLAN_InBadDLPItype_IDX ].objsz = sizeof(ipxbuf.l.InBadDLPItype);
	mettbl[ IPXLANR_InBadDLPItype_IDX ].objsz = sizeof(ipxbuf.l.InBadDLPItype);
	mettbl[ IPXLAN_InCoalesced_IDX ].objsz = sizeof(ipxbuf.l.InCoalesced);
	mettbl[ IPXLANR_InCoalesced_IDX ].objsz = sizeof(ipxbuf.l.InCoalesced);
	mettbl[ IPXLANR_InPropagation_IDX ].objsz = sizeof(ipxbuf.l.InPropagation);
	mettbl[ IPXLAN_InNoPropagate_IDX ].objsz = sizeof(ipxbuf.l.InNoPropagate);
	mettbl[ IPXLANR_InNoPropagate_IDX ].objsz = sizeof(ipxbuf.l.InNoPropagate);
	mettbl[ IPXLANR_InTotal_IDX ].objsz = sizeof(ipxbuf.l.InTotal);
	mettbl[ IPXLAN_InBadLength_IDX ].objsz = sizeof(ipxbuf.l.InBadLength);
	mettbl[ IPXLANR_InBadLength_IDX ].objsz = sizeof(ipxbuf.l.InBadLength);
	mettbl[ IPXLAN_InDriverEcho_IDX ].objsz = sizeof(ipxbuf.l.InDriverEcho);
	mettbl[ IPXLANR_InDriverEcho_IDX ].objsz = sizeof(ipxbuf.l.InDriverEcho);
	mettbl[ IPXLANR_InSap_IDX ].objsz = sizeof(ipxbuf.l.InSap);
	mettbl[ IPXLAN_InRipDropped_IDX ].objsz = sizeof(ipxbuf.l.InRipDropped);
	mettbl[ IPXLANR_InRipDropped_IDX ].objsz = sizeof(ipxbuf.l.InRipDropped);
	mettbl[ IPXLANR_InRipRouted_IDX ].objsz = sizeof(ipxbuf.l.InRipRouted);
	mettbl[ IPXLANR_InSap_IDX ].objsz = sizeof(ipxbuf.l.InSap);
	mettbl[ IPXLAN_InSapBad_IDX ].objsz = sizeof(ipxbuf.l.InSapBad);
	mettbl[ IPXLANR_InSapBad_IDX ].objsz = sizeof(ipxbuf.l.InSapBad);
	mettbl[ IPXLANR_InSapIpx_IDX ].objsz = sizeof(ipxbuf.l.InSapIpx);
	mettbl[ IPXLANR_InSapNoIpxToSapd_IDX ].objsz = sizeof(ipxbuf.l.InSapNoIpxToSapd);
	mettbl[ IPXLAN_InSapNoIpxDrop_IDX ].objsz = sizeof(ipxbuf.l.InSapNoIpxDrop);
	mettbl[ IPXLANR_InSapNoIpxDrop_IDX ].objsz = sizeof(ipxbuf.l.InSapNoIpxDrop);
	mettbl[ IPXLAN_InDiag_IDX ].objsz = sizeof(ipxbuf.l.InDiag);
	mettbl[ IPXLANR_InDiag_IDX ].objsz = sizeof(ipxbuf.l.InDiag);
	mettbl[ IPXLAN_InDiagInternal_IDX ].objsz = sizeof(ipxbuf.l.InDiagInternal);
	mettbl[ IPXLANR_InDiagInternal_IDX ].objsz = sizeof(ipxbuf.l.InDiagInternal);
	mettbl[ IPXLAN_InDiagNIC_IDX ].objsz = sizeof(ipxbuf.l.InDiagNIC);
	mettbl[ IPXLANR_InDiagNIC_IDX ].objsz = sizeof(ipxbuf.l.InDiagNIC);
	mettbl[ IPXLAN_InDiagIpx_IDX ].objsz = sizeof(ipxbuf.l.InDiagIpx);
	mettbl[ IPXLANR_InDiagIpx_IDX ].objsz = sizeof(ipxbuf.l.InDiagIpx);
	mettbl[ IPXLAN_InDiagNoIpx_IDX ].objsz = sizeof(ipxbuf.l.InDiagNoIpx);
	mettbl[ IPXLANR_InDiagNoIpx_IDX ].objsz = sizeof(ipxbuf.l.InDiagNoIpx);
	mettbl[ IPXLAN_InNICDropped_IDX ].objsz = sizeof(ipxbuf.l.InNICDropped);
	mettbl[ IPXLANR_InNICDropped_IDX ].objsz = sizeof(ipxbuf.l.InNICDropped);
	mettbl[ IPXLANR_InBroadcast_IDX ].objsz = sizeof(ipxbuf.l.InBroadcast);
	mettbl[ IPXLANR_InBroadcastInternal_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastInternal);
	mettbl[ IPXLANR_InBroadcastNIC_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastNIC);
	mettbl[ IPXLAN_InBroadcastDiag_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiag);
	mettbl[ IPXLANR_InBroadcastDiag_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiag);
	mettbl[ IPXLAN_InBroadcastDiagFwd_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiagFwd);
	mettbl[ IPXLANR_InBroadcastDiagFwd_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiagFwd);
	mettbl[ IPXLAN_InBroadcastDiagRoute_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiagRoute);
	mettbl[ IPXLANR_InBroadcastDiagRoute_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiagRoute);
	mettbl[ IPXLAN_InBroadcastDiagResp_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiagResp);
	mettbl[ IPXLANR_InBroadcastDiagResp_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastDiagResp);
	mettbl[ IPXLAN_InBroadcastRoute_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastRoute);
	mettbl[ IPXLANR_InBroadcastRoute_IDX ].objsz = sizeof(ipxbuf.l.InBroadcastRoute);
	mettbl[ IPXLANR_InForward_IDX ].objsz = sizeof(ipxbuf.l.InForward);
	mettbl[ IPXLANR_InRoute_IDX ].objsz = sizeof(ipxbuf.l.InRoute);
	mettbl[ IPXLANR_InInternalNet_IDX ].objsz = sizeof(ipxbuf.l.InInternalNet);
	mettbl[ IPXLANR_OutPropagation_IDX ].objsz = sizeof(ipxbuf.l.OutPropagation);
	mettbl[ IPXLANR_OutTotalStream_IDX ].objsz = sizeof(ipxbuf.l.OutTotalStream);
	mettbl[ IPXLANR_OutTotal_IDX ].objsz = sizeof(ipxbuf.l.OutTotal);
	mettbl[ IPXLAN_OutSameSocket_IDX ].objsz = sizeof(ipxbuf.l.OutSameSocket);
	mettbl[ IPXLANR_OutSameSocket_IDX ].objsz = sizeof(ipxbuf.l.OutSameSocket);
	mettbl[ IPXLANR_OutFillInDest_IDX ].objsz = sizeof(ipxbuf.l.OutFillInDest);
	mettbl[ IPXLANR_OutInternal_IDX ].objsz = sizeof(ipxbuf.l.OutInternal);
	mettbl[ IPXLAN_OutBadLan_IDX ].objsz = sizeof(ipxbuf.l.OutBadLan);
	mettbl[ IPXLANR_OutBadLan_IDX ].objsz = sizeof(ipxbuf.l.OutBadLan);
	mettbl[ IPXLANR_OutSent_IDX ].objsz = sizeof(ipxbuf.l.OutSent);
	mettbl[ IPXLANR_OutQueued_IDX ].objsz = sizeof(ipxbuf.l.OutQueued);
	mettbl[ IPXLANR_Ioctl_IDX ].objsz = sizeof(ipxbuf.l.Ioctl);
	mettbl[ IPXLANR_IoctlSetLans_IDX ].objsz = sizeof(ipxbuf.l.IoctlSetLans);
	mettbl[ IPXLANR_IoctlGetLans_IDX ].objsz = sizeof(ipxbuf.l.IoctlGetLans);
	mettbl[ IPXLANR_IoctlSetSapQ_IDX ].objsz = sizeof(ipxbuf.l.IoctlSetSapQ);
	mettbl[ IPXLANR_IoctlSetLanInfo_IDX ].objsz = sizeof(ipxbuf.l.IoctlSetLanInfo);
	mettbl[ IPXLANR_IoctlGetLanInfo_IDX ].objsz = sizeof(ipxbuf.l.IoctlGetLanInfo);
	mettbl[ IPXLANR_IoctlGetNodeAddr_IDX ].objsz = sizeof(ipxbuf.l.IoctlGetNodeAddr);
	mettbl[ IPXLANR_IoctlGetNetAddr_IDX ].objsz = sizeof(ipxbuf.l.IoctlGetNetAddr);
	mettbl[ IPXLANR_IoctlGetStats_IDX ].objsz = sizeof(ipxbuf.l.IoctlGetStats);
	mettbl[ IPXLANR_IoctlLink_IDX ].objsz = sizeof(ipxbuf.l.IoctlLink);
	mettbl[ IPXLANR_IoctlUnlink_IDX ].objsz = sizeof(ipxbuf.l.IoctlUnlink);
	mettbl[ IPXLAN_IoctlUnknown_IDX ].objsz = sizeof(ipxbuf.l.IoctlUnknown);
	mettbl[ IPXLANR_IoctlUnknown_IDX ].objsz = sizeof(ipxbuf.l.IoctlUnknown);
	mettbl[ IPXSOCKR_IpxInData_IDX ].objsz = sizeof(ipxbuf.s.IpxInData);
	mettbl[ IPXR_datapackets_IDX ].objsz = sizeof(ipx_datapackets);
	mettbl[ IPXSOCKR_IpxOutData_IDX ].objsz = sizeof(ipxbuf.s.IpxOutData);
	mettbl[ IPXSOCK_IpxOutBadSize_IDX ].objsz = sizeof(ipxbuf.s.IpxOutBadSize);
	mettbl[ IPXSOCKR_IpxOutBadSize_IDX ].objsz = sizeof(ipxbuf.s.IpxOutBadSize);
	mettbl[ IPXSOCK_IpxInBadSize_IDX ].objsz = sizeof(ipxbuf.s.IpxInBadSize);
	mettbl[ IPXSOCKR_IpxInBadSize_IDX ].objsz = sizeof(ipxbuf.s.IpxInBadSize);
	mettbl[ IPXSOCKR_IpxOutToSwitch_IDX ].objsz = sizeof(ipxbuf.s.IpxOutToSwitch);
	mettbl[ IPXSOCKR_IpxTLIOutData_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutData);
	mettbl[ IPXSOCK_IpxTLIOutBadState_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutBadState);
	mettbl[ IPXSOCKR_IpxTLIOutBadState_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutBadState);
	mettbl[ IPXSOCK_IpxTLIOutBadSize_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutBadSize);
	mettbl[ IPXSOCKR_IpxTLIOutBadSize_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutBadSize);
	mettbl[ IPXSOCK_IpxTLIOutBadOpt_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutBadOpt);
	mettbl[ IPXSOCKR_IpxTLIOutBadOpt_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutBadOpt);
	mettbl[ IPXSOCK_IpxTLIOutHdrAlloc_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutHdrAlloc);
	mettbl[ IPXSOCKR_IpxTLIOutHdrAlloc_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutHdrAlloc);
	mettbl[ IPXSOCKR_IpxTLIOutToSwitch_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOutToSwitch);
	mettbl[ IPXSOCK_IpxBoundSockets_IDX ].objsz = sizeof(ipxbuf.s.IpxBoundSockets);
	mettbl[ IPXSOCKR_IpxBoundSockets_IDX ].objsz = sizeof(ipxbuf.s.IpxBoundSockets);
	mettbl[ IPXSOCKR_IpxBind_IDX ].objsz = sizeof(ipxbuf.s.IpxBind);
	mettbl[ IPXSOCKR_IpxTLIBind_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIBind);
	mettbl[ IPXSOCKR_IpxTLIOptMgt_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIOptMgt);
	mettbl[ IPXSOCK_IpxTLIUnknown_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIUnknown);
	mettbl[ IPXSOCKR_IpxTLIUnknown_IDX ].objsz = sizeof(ipxbuf.s.IpxTLIUnknown);
	mettbl[ IPXSOCK_IpxTLIOutBadAddr_IDX ].objsz = 
	   sizeof(ipxbuf.s.IpxTLIOutBadAddr);
	mettbl[ IPXSOCKR_IpxTLIOutBadAddr_IDX ].objsz = 
	   sizeof(ipxbuf.s.IpxTLIOutBadAddr);
	mettbl[ IPXSOCK_IpxSwitchInvalSocket_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchInvalSocket);
	mettbl[ IPXSOCKR_IpxSwitchInvalSocket_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchInvalSocket);
	mettbl[ IPXSOCK_IpxSwitchSumFail_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchSumFail);
	mettbl[ IPXSOCKR_IpxSwitchSumFail_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchSumFail);
	mettbl[ IPXSOCK_IpxSwitchAllocFail_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchAllocFail);
	mettbl[ IPXSOCKR_IpxSwitchAllocFail_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchAllocFail);
	mettbl[ IPXSOCKR_IpxSwitchSum_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchSum);
	mettbl[ IPXSOCKR_IpxSwitchEven_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchEven);
	mettbl[ IPXSOCKR_IpxSwitchEvenAlloc_IDX ].objsz = sizeof(ipxbuf.s.IpxSwitchEvenAlloc);
	mettbl[ DUMMY_1_IDX ].objsz = sizeof(dummy);
	mettbl[ IPXSOCKR_IpxDataToSocket_IDX ].objsz = sizeof(ipxbuf.s.IpxDataToSocket);
	mettbl[ IPXSOCKR_IpxTrimPacket_IDX ].objsz = sizeof(ipxbuf.s.IpxTrimPacket);
	mettbl[ IPXSOCK_IpxSumFail_IDX ].objsz = sizeof(ipxbuf.s.IpxSumFail);
	mettbl[ IPXSOCKR_IpxSumFail_IDX ].objsz = sizeof(ipxbuf.s.IpxSumFail);
	mettbl[ IPXSOCK_IpxBusySocket_IDX ].objsz = sizeof(ipxbuf.s.IpxBusySocket);
	mettbl[ IPXSOCKR_IpxBusySocket_IDX ].objsz = sizeof(ipxbuf.s.IpxBusySocket);
	mettbl[ IPXSOCK_IpxSocketNotBound_IDX ].objsz = sizeof(ipxbuf.s.IpxSocketNotBound);
	mettbl[ IPXSOCKR_IpxSocketNotBound_IDX ].objsz = sizeof(ipxbuf.s.IpxSocketNotBound);
	mettbl[ IPXSOCKR_IpxRouted_IDX ].objsz = sizeof(ipxbuf.s.IpxRouted);
	mettbl[ IPXSOCKR_IpxRoutedTLI_IDX ].objsz = sizeof(ipxbuf.s.IpxRoutedTLI);
	mettbl[ IPXSOCK_IpxRoutedTLIAlloc_IDX ].objsz = sizeof(ipxbuf.s.IpxRoutedTLIAlloc);
	mettbl[ IPXSOCKR_IpxRoutedTLIAlloc_IDX ].objsz = sizeof(ipxbuf.s.IpxRoutedTLIAlloc);
	mettbl[ IPXR_sent_to_tli_IDX ].objsz = sizeof(ipx_sent_to_TLI);
	mettbl[ IPXR_total_ioctls_IDX ].objsz = sizeof(ipxbuf.s.IpxIoctlSetWater);
	mettbl[ IPXSOCKR_IpxIoctlSetWater_IDX ].objsz = sizeof(ipxbuf.s.IpxIoctlSetWater);
	mettbl[ IPXSOCKR_IpxIoctlBindSocket_IDX ].objsz = sizeof(ipxbuf.s.IpxIoctlBindSocket);
	mettbl[ IPXSOCKR_IpxIoctlUnbindSocket_IDX ].objsz = sizeof(ipxbuf.s.IpxIoctlUnbindSocket);
	mettbl[ IPXSOCKR_IpxIoctlStats_IDX ].objsz = sizeof(ipxbuf.s.IpxIoctlStats);
	mettbl[ IPXSOCK_IpxIoctlUnknown_IDX ].objsz = sizeof(ipxbuf.s.IpxIoctlUnknown);
	mettbl[ IPXSOCKR_IpxIoctlUnknown_IDX ].objsz = sizeof(ipxbuf.s.IpxIoctlUnknown);
	mettbl[ RIPR_ReceivedPackets_IDX ].objsz = sizeof(ripbuf.ReceivedPackets);
	mettbl[ RIP_ReceivedNoLanKey_IDX ].objsz = sizeof(ripbuf.ReceivedNoLanKey);
	mettbl[ RIPR_ReceivedNoLanKey_IDX ].objsz = sizeof(ripbuf.ReceivedNoLanKey);
	mettbl[ RIP_ReceivedBadLength_IDX ].objsz = sizeof(ripbuf.ReceivedBadLength);
	mettbl[ RIPR_ReceivedBadLength_IDX ].objsz = sizeof(ripbuf.ReceivedBadLength);
	mettbl[ RIPR_ReceivedCoalesced_IDX ].objsz = sizeof(ripbuf.ReceivedCoalesced);
	mettbl[ RIP_ReceivedNoCoalesce_IDX ].objsz = sizeof(ripbuf.ReceivedNoCoalesce);
	mettbl[ RIPR_ReceivedNoCoalesce_IDX ].objsz = sizeof(ripbuf.ReceivedNoCoalesce);
	mettbl[ RIPR_ReceivedRequestPackets_IDX ].objsz = sizeof(ripbuf.ReceivedRequestPackets);
	mettbl[ RIPR_ReceivedResponsePackets_IDX ].objsz = sizeof(ripbuf.ReceivedResponsePackets);
	mettbl[ RIP_ReceivedUnknownRequest_IDX ].objsz = sizeof(ripbuf.ReceivedUnknownRequest);
	mettbl[ RIPR_ReceivedUnknownRequest_IDX ].objsz = sizeof(ripbuf.ReceivedUnknownRequest);
	mettbl[ RIPR_total_router_packets_sent_IDX ].objsz = sizeof(rip_total_router_packets_sent);
	mettbl[ RIP_SentAllocFailed_IDX ].objsz = sizeof(ripbuf.SentAllocFailed);
	mettbl[ RIPR_SentAllocFailed_IDX ].objsz = sizeof(ripbuf.SentAllocFailed);
	mettbl[ RIP_SentBadDestination_IDX ].objsz = sizeof(ripbuf.SentBadDestination);
	mettbl[ RIPR_SentBadDestination_IDX ].objsz = sizeof(ripbuf.SentBadDestination);
	mettbl[ RIPR_SentRequestPackets_IDX ].objsz = sizeof(ripbuf.SentRequestPackets);
	mettbl[ RIPR_SentResponsePackets_IDX ].objsz = sizeof(ripbuf.SentResponsePackets);
	mettbl[ RIP_SentLan0Dropped_IDX ].objsz = sizeof(ripbuf.SentLan0Dropped);
	mettbl[ RIPR_SentLan0Dropped_IDX ].objsz = sizeof(ripbuf.SentLan0Dropped);
	mettbl[ RIPR_SentLan0Routed_IDX ].objsz = sizeof(ripbuf.SentLan0Routed);
	mettbl[ RIPR_ioctls_processed_IDX ].objsz = sizeof(ripbuf.RipxIoctlInitialize);
	mettbl[ RIPR_RipxIoctlInitialize_IDX ].objsz = sizeof(ripbuf.RipxIoctlInitialize);
	mettbl[ RIPR_RipxIoctlGetHashSize_IDX ].objsz = sizeof(ripbuf.RipxIoctlGetHashSize);
	mettbl[ RIPR_RipxIoctlGetHashStats_IDX ].objsz = sizeof(ripbuf.RipxIoctlGetHashStats);
	mettbl[ RIPR_RipxIoctlDumpHashTable_IDX ].objsz = sizeof(ripbuf.RipxIoctlDumpHashTable);
	mettbl[ RIPR_RipxIoctlGetRouterTable_IDX ].objsz = sizeof(ripbuf.RipxIoctlGetRouterTable);
	mettbl[ RIPR_RipxIoctlGetNetInfo_IDX ].objsz = sizeof(ripbuf.RipxIoctlGetNetInfo);
	mettbl[ RIPR_RipxIoctlCheckSapSource_IDX ].objsz = sizeof(ripbuf.RipxIoctlCheckSapSource);
	mettbl[ RIPR_RipxIoctlResetRouter_IDX ].objsz = sizeof(ripbuf.RipxIoctlResetRouter);
	mettbl[ RIPR_RipxIoctlDownRouter_IDX ].objsz = sizeof(ripbuf.RipxIoctlDownRouter);
	mettbl[ RIPR_RipxIoctlStats_IDX ].objsz = sizeof(ripbuf.RipxIoctlStats);
	mettbl[ RIP_RipxIoctlUnknown_IDX ].objsz = sizeof(ripbuf.RipxIoctlUnknown);
	mettbl[ RIPR_RipxIoctlUnknown_IDX ].objsz = sizeof(ripbuf.RipxIoctlUnknown);
	mettbl[ SPX_max_connections_IDX ].objsz = sizeof(spxbuf.spx_max_connections);
	mettbl[ SPX_max_used_connections_IDX ].objsz = sizeof(spxbuf.spx_max_used_connections);
	mettbl[ SPX_current_connections_IDX ].objsz = sizeof(spxbuf.spx_current_connections);
	mettbl[ SPXR_current_connections_IDX ].objsz = sizeof(spxbuf.spx_current_connections);
	mettbl[ SPX_alloc_failures_IDX ].objsz = sizeof(spxbuf.spx_alloc_failures);
	mettbl[ SPXR_alloc_failures_IDX ].objsz = sizeof(spxbuf.spx_alloc_failures);
	mettbl[ SPX_open_failures_IDX ].objsz = sizeof(spxbuf.spx_open_failures);
	mettbl[ SPXR_open_failures_IDX ].objsz = sizeof(spxbuf.spx_open_failures);
	mettbl[ SPXR_ioctls_IDX ].objsz = sizeof(spxbuf.spx_ioctls);
	mettbl[ SPXR_connect_req_count_IDX ].objsz = sizeof(spxbuf.spx_connect_req_count);
	mettbl[ SPX_connect_req_fails_IDX ].objsz = sizeof(spxbuf.spx_connect_req_fails);
	mettbl[ SPXR_connect_req_fails_IDX ].objsz = sizeof(spxbuf.spx_connect_req_fails);
	mettbl[ SPXR_listen_req_IDX ].objsz = sizeof(spxbuf.spx_listen_req);
	mettbl[ SPX_listen_req_fails_IDX ].objsz = sizeof(spxbuf.spx_listen_req_fails);
	mettbl[ SPXR_listen_req_fails_IDX ].objsz = sizeof(spxbuf.spx_listen_req_fails);
	mettbl[ SPXR_send_mesg_count_IDX ].objsz = sizeof(spxbuf.spx_send_mesg_count);
	mettbl[ SPX_unknown_mesg_count_IDX ].objsz = sizeof(spxbuf.spx_unknown_mesg_count);
	mettbl[ SPXR_unknown_mesg_count_IDX ].objsz = sizeof(spxbuf.spx_unknown_mesg_count);
	mettbl[ SPX_send_bad_mesg_IDX ].objsz = sizeof(spxbuf.spx_send_bad_mesg);
	mettbl[ SPXR_send_bad_mesg_IDX ].objsz = sizeof(spxbuf.spx_send_bad_mesg);
	mettbl[ SPXR_send_packet_count_IDX ].objsz = sizeof(spxbuf.spx_send_packet_count);
	mettbl[ SPX_send_packet_timeout_IDX ].objsz = sizeof(spxbuf.spx_send_packet_timeout);
	mettbl[ SPXR_send_packet_timeout_IDX ].objsz = sizeof(spxbuf.spx_send_packet_timeout);
	mettbl[ SPX_send_packet_nak_IDX ].objsz = sizeof(spxbuf.spx_send_packet_nak);
	mettbl[ SPXR_send_packet_nak_IDX ].objsz = sizeof(spxbuf.spx_send_packet_nak);
	mettbl[ SPXR_rcv_packet_count_IDX ].objsz = sizeof(spxbuf.spx_rcv_packet_count);
	mettbl[ SPX_rcv_bad_packet_IDX ].objsz = sizeof(spxbuf.spx_rcv_bad_packet);
	mettbl[ SPXR_rcv_bad_packet_IDX ].objsz = sizeof(spxbuf.spx_rcv_bad_packet);
	mettbl[ SPX_rcv_bad_data_packet_IDX ].objsz = sizeof(spxbuf.spx_rcv_bad_data_packet);
	mettbl[ SPXR_rcv_bad_data_packet_IDX ].objsz = sizeof(spxbuf.spx_rcv_bad_data_packet);
	mettbl[ SPX_rcv_dup_packet_IDX ].objsz = sizeof(spxbuf.spx_rcv_dup_packet);
	mettbl[ SPXR_rcv_dup_packet_IDX ].objsz = sizeof(spxbuf.spx_rcv_dup_packet);
	mettbl[ SPXR_rcv_packet_sentup_IDX ].objsz = sizeof(spxbuf.spx_rcv_packet_sentup);
	mettbl[ SPX_rcv_conn_req_IDX ].objsz = sizeof(spxbuf.spx_rcv_conn_req);
	mettbl[ SPXR_rcv_conn_req_IDX ].objsz = sizeof(spxbuf.spx_rcv_conn_req);
	mettbl[ SPX_abort_connection_IDX ].objsz = sizeof(spxbuf.spx_abort_connection);
	mettbl[ SPXR_abort_connection_IDX ].objsz = sizeof(spxbuf.spx_abort_connection);
	mettbl[ SPX_max_retries_abort_IDX ].objsz = sizeof(spxbuf.spx_max_retries_abort);
	mettbl[ SPXR_max_retries_abort_IDX ].objsz = sizeof(spxbuf.spx_max_retries_abort);
	mettbl[ SPX_no_listeners_IDX ].objsz = sizeof(spxbuf.spx_no_listeners);
	mettbl[ SPXR_no_listeners_IDX ].objsz = sizeof(spxbuf.spx_no_listeners);
	mettbl[ SPXCON_netaddr_IDX ].objsz = sizeof(spxconbuf[i].con_addr.net);
	mettbl[ SPXCON_nodeaddr_IDX ].objsz = sizeof(spxconbuf[i].con_addr.node[0]);
	mettbl[ SPXCON_sockaddr_IDX ].objsz = sizeof(spxconbuf[i].con_addr.sock[0]);
	mettbl[ SPXCON_connection_id_IDX ].objsz = sizeof(spxconbuf[i].con_connection_id);
	mettbl[ SPXCON_o_netaddr_IDX ].objsz = sizeof(spxconbuf[i].o_addr.net[0]);
	mettbl[ SPXCON_o_nodeaddr_IDX ].objsz = sizeof(spxconbuf[i].o_addr.node[0]);
	mettbl[ SPXCON_o_sockaddr_IDX ].objsz = sizeof(spxconbuf[i].o_addr.sock[0]);
	mettbl[ SPXCON_o_connection_id_IDX ].objsz = sizeof(spxconbuf[i].o_connection_id);
	mettbl[ SPXCON_con_state_IDX ].objsz = sizeof(spxconbuf[i].con_state);
	mettbl[ SPXCON_con_retry_count_IDX ].objsz = sizeof(spxconbuf[i].con_retry_count);
	mettbl[ SPXCON_con_retry_time_IDX ].objsz = sizeof(spxconbuf[i].con_retry_time);
	mettbl[ SPXCON_con_state_IDX ].objsz = sizeof(spxconbuf[i].con_state);
	mettbl[ SPXCON_con_type_IDX ].objsz = sizeof(spxconbuf[i].con_type);
	mettbl[ SPXCON_con_ipxChecksum_IDX ].objsz = sizeof(spxconbuf[i].con_ipxChecksum);
	mettbl[ SPXCON_con_window_size_IDX ].objsz = sizeof(spxconbuf[i].con_window_size);
	mettbl[ SPXCON_con_remote_window_size_IDX ].objsz = sizeof(spxconbuf[i].con_remote_window_size);
	mettbl[ SPXCON_con_send_packet_size_IDX ].objsz = sizeof(spxconbuf[i].con_send_packet_size);
	mettbl[ SPXCON_con_rcv_packet_size_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_packet_size);
	mettbl[ SPXCON_con_round_trip_time_IDX ].objsz = sizeof(spxconbuf[i].con_round_trip_time);
	mettbl[ SPXCON_con_window_choke_IDX ].objsz = sizeof(spxconbuf[i].con_window_choke);
	mettbl[ SPXCONR_con_send_mesg_count_IDX ].objsz = sizeof(spxconbuf[i].con_send_mesg_count);
	mettbl[ SPXCON_con_unknown_mesg_count_IDX ].objsz = sizeof(spxconbuf[i].con_unknown_mesg_count);
	mettbl[ SPXCONR_con_unknown_mesg_count_IDX ].objsz = sizeof(spxconbuf[i].con_unknown_mesg_count);
	mettbl[ SPXCON_con_send_bad_mesg_IDX ].objsz = sizeof(spxconbuf[i].con_send_bad_mesg);
	mettbl[ SPXCONR_con_send_bad_mesg_IDX ].objsz = sizeof(spxconbuf[i].con_send_bad_mesg);
	mettbl[ SPXCONR_con_send_packet_count_IDX ].objsz = sizeof(spxconbuf[i].con_send_packet_count);
	mettbl[ SPXCON_con_send_packet_timeout_IDX ].objsz = sizeof(spxconbuf[i].con_send_packet_timeout);
	mettbl[ SPXCONR_con_send_packet_timeout_IDX ].objsz = sizeof(spxconbuf[i].con_send_packet_timeout);
	mettbl[ SPXCON_con_send_packet_nak_IDX ].objsz = sizeof(spxconbuf[i].con_send_packet_nak);
	mettbl[ SPXCONR_con_send_packet_nak_IDX ].objsz = sizeof(spxconbuf[i].con_send_packet_nak);
	mettbl[ SPXCONR_con_send_ack_IDX ].objsz = sizeof(spxconbuf[i].con_send_ack);
	mettbl[ SPXCON_con_send_nak_IDX ].objsz = sizeof(spxconbuf[i].con_send_nak);
	mettbl[ SPXCONR_con_send_nak_IDX ].objsz = sizeof(spxconbuf[i].con_send_nak);
	mettbl[ SPXCONR_con_send_watchdog_IDX ].objsz = sizeof(spxconbuf[i].con_send_watchdog);
	mettbl[ SPXCONR_con_rcv_packet_count_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_packet_count);
	mettbl[ SPXCON_con_rcv_bad_packet_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_bad_packet);
	mettbl[ SPXCONR_con_rcv_bad_packet_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_bad_packet);
	mettbl[ SPXCON_con_rcv_bad_data_packet_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_bad_data_packet);
	mettbl[ SPXCONR_con_rcv_bad_data_packet_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_bad_data_packet);
	mettbl[ SPXCON_con_rcv_dup_packet_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_dup_packet);
	mettbl[ SPXCONR_con_rcv_dup_packet_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_dup_packet);
	mettbl[ SPXCON_con_rcv_packet_outseq_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_packet_outseq);
	mettbl[ SPXCONR_con_rcv_packet_outseq_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_packet_outseq);
	mettbl[ SPXCONR_con_rcv_packet_sentup_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_packet_sentup);
	mettbl[ SPXCONR_con_rcv_packet_qued_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_packet_qued);
	mettbl[ SPXCONR_con_rcv_ack_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_ack);
	mettbl[ SPXCON_con_rcv_nak_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_nak);
	mettbl[ SPXCONR_con_rcv_nak_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_nak);
	mettbl[ SPXCONR_con_rcv_watchdog_IDX ].objsz = sizeof(spxconbuf[i].con_rcv_watchdog);
	mettbl[ SAPR_total_IDX ].objsz = sizeof(sap_total);
	mettbl[ SPXR_total_IDX ].objsz = sizeof(spx_total);
	mettbl[ IPXR_total_IDX ].objsz = sizeof(ipx_total);
	mettbl[ RIPR_total_IDX ].objsz = sizeof(rip_total);
	mettbl[ NETWARE_errs_IDX ].objsz = sizeof(netware_errs);
	mettbl[ NETWARER_errs_IDX ].objsz = sizeof(netware_errs);
	mettbl[ EISA_BUS_UTIL_SUMCNT_IDX ].objsz = sizeof(eisa_bus_util_sumcnt);
	mettbl[ EISA_BUS_UTIL_PERCENT_IDX ].objsz = sizeof(eisa_bus_util_sum);
}
/*
 *	function: 	metalloc
 *
 *	args:		pointer to a metric entry in mettbl
 *
 *	ret val:	none
 *
 *	allocate a set of struct metvals, one per instance,
 *	plus totals in each dimension.
 */
void
metalloc( struct metric *metp ) {
	int i, j;
	int dimx, dimy;
	check_resource( metp );
	set_size( metp );
	metp->resval[0] = 0;
	metp->resval[1] = 0;
	metp->color = NULL;
	metp->inverted = 0;

	switch( metp->ndim ) {
	case 2:
		for( j = 0; j < nmets; j++ ) {
			if( !mas_resource_cmp( &metp->reslist[1],
			  (resource_t)mettbl[j].id ) ) {
				metp->resval[1]
				  = mettbl[j].metval->met.sngl;
				break;
			}
		}
		/* FALLTHROUGH */
	case 1:
		for( j = 0; j < nmets; j++ ) {
			if( !mas_resource_cmp( &metp->reslist[0],
			  (resource_t)mettbl[j].id ) ) {
				metp->resval[0] = 
				  mettbl[j].metval->met.sngl;
				break;
			}
		}

		/* FALLTHROUGH */
	case 0: 
		break;
	default:
#ifdef DEBUG
/*
 *		don't have to call endwin here, since this is called
 *		before we start curses.
 */
		fprintf(stderr,"DEBUG unsupported resource count in mettbl\n");
		exit(1);
#else
		break;
#endif
	}

	dimx = (metp->resval[0]+1);
	dimy = (metp->resval[1]+1);
	if( !((metp->metval) = (struct metval *)histalloc( dimx * dimy
	    * sizeof( struct metval ) ))) {
/*
 *		don't have to call endwin here, since this is called
 *		before we start curses.
 */
		fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
		exit(1);
	}
	switch( metp->ndim ) {
	case 2:
		for( i = 0; i < dimx; i++ ) {
			for( j = 0; j < dimy; j++ ) {

				if( !((metp->metval[i*dimy+j].cooked) = 
				  (float *)histalloc(maxhist*sizeof(float)))){
/*
 *					don't have to call endwin here, 
 *					since this is called
 *					before we start curses.
 */
					fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
					exit(1);
				}
			}
		}
		for( i = 0; i < dimx-1; i++ ) {
			for( j = 0; j < dimy-1; j++ ) {
				metp->metval[ i * dimy + j ].met_p =
				  mas_get_met_snap( md, metp->id, i, j );
#ifdef DEBUG
				if( !metp->metval[ i * dimy + j ].met_p ) {
/*
 *					don't have to call endwin here, 
 *					since this is called before we 
 *					start curses.
 */
					fprintf(stderr, "DEBUG %s[%d][%d]:instance not registered\n",
					  metp->title, i, j );
				} 
#endif
				calc_intv(metp, &metp->metval[i*dimy+j],
				  metp->objsz );
			}
		}
		break;
	case 1:
		for( i = 0; i < dimx; i++ ) {
			if( !((metp->metval[i].cooked) = 
			  (float *)histalloc(maxhist*sizeof(float)))){
/*
 *				don't have to call endwin here, 
 *				since this is called
 *				before we start curses.
 */
				fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
				exit(1);
			}
		}
		for( i = 0; i < dimx-1; i++ ) {
			metp->metval[ i ].met_p =
			  mas_get_met_snap( md, metp->id, i );
#ifdef DEBUG
			if( !metp->metval[ i ].met_p ) {
/*
 *				don't have to call endwin here, 
 *				since this is called before we 
 *				start curses.
 */
				fprintf(stderr, "DEBUG %s[%d]:instance not registered\n",
				  metp->title, i );
			}
#endif
			calc_intv( metp, &metp->metval[i],metp->objsz);
		}

		break;
	case 0: 
		if( !((metp->metval->cooked) = 
		  (float *)histalloc(maxhist*sizeof(float)))){
/*
 *			don't have to call endwin here, 
 *			since this is called
 *			before we start curses.
 */
			fprintf(stderr,gettxt("RTPM:7", "out of memory\n"));
			exit(1);
		}
		metp->metval[0].met_p = mas_get_met_snap( md, metp->id, 0);
#ifdef DEBUG
		if( !metp->metval[0].met_p ) {
/*
 *			don't have to call endwin here, since this is 
 *			called before we start curses.
 */
			fprintf(stderr, "DEBUG %s:instance not registered\n",
			  metp->title );
		}
#endif
		calc_intv( metp, &metp->metval[0], metp->objsz );
		break;
	default:
#ifdef DEBUG
/*
 *		don't have to call endwin here, since this is 
 *		called before we start curses.
 */
		fprintf(stderr,"DEBUG unsupported resource count in mettbl\n");
		exit(1);
#else
		break;
#endif
	}
}
/*
 *	function: 	check_resource
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	verify that the resource list as specified in mettbl matches
 *	the resource list returned from MAS.  If MAS doesn't recognize
 *	the metric id, assume it's a kludged up metric and let it pass
 *	on through.  If MAS does know the metrics and the resource list
 *	doesn't match, either MAS or rtpm is corrupted.
 */
void
check_resource( struct metric *metp )
{
	metid_t id = metp->id;
	uint32 nres = metp->ndim;
	resource_t *reslist = metp->reslist;
	resource_t *resource_p;
	int i;

	resource_p = mas_get_met_resources( md, id );
	if( !resource_p ) {
		if( mas_errno() == MAS_NOMETRIC ) /* metric not in mas */
			return;
/*
 *		don't have to call endwin here, since this is 
 *		called before we start curses.
 */
		fprintf(stderr,gettxt("RTPM:3", "can't get resource list for %s (id:%d)\n"),
		  metp->title, id);
		exit(1);
	}

	for( i=0; i < nres; i++ ) {
		if( mas_resource_cmp( &resource_p[i], reslist[i] ) ) {
/*
 *			don't have to call endwin here, since this is 
 *			called before we start curses.
 */
			fprintf(stderr, gettxt("RTPM:4", "unexpected resource for %s (id:%d)\nexpected:%d received:%d\n"),
			  metp->title, id, *reslist, *resource_p);
			exit(1);
		}
	}
	if( mas_resource_cmp( &resource_p[i], MAS_NATIVE ) &&
	  mas_resource_cmp( &resource_p[i], MAS_SYSTEM )) {
/*
 *		don't have to call endwin here, since this is 
 *		called before we start curses.
 */
		fprintf(stderr,gettxt("RTPM:5", "extra resources for %s (id:%d)\n"),
		  metp->title, id);
		exit(1);
	}
}
/*
 *	function: 	set_size
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Get the size of the metric object from MAS.  If MAS doesn't 
 *	recognize the metric id, assume it's a kludged up metric and 
 *	arbitrarily set the size to be sizeof(int).
 */
void 
set_size( struct metric *metp ) {
	metid_t id = metp->id;
	uint32 *metsz_p;

	metsz_p = mas_get_met_objsz( md, id );
	if( !metsz_p ) {
		if( mas_errno() == MAS_NOMETRIC ) { /* metric not in mas */
			metp->objsz = sizeof( int );
			return;
		}
/*
 *		don't have to call endwin here, since this is 
 *		called before we start curses.
 */
		fprintf(stderr,gettxt("RTPM:6", "can't get object size for %s (id:%d)\n"),
		  metp->title, id);
		exit(1);
	}
	metp->objsz = *metsz_p;
}
#define HSIZE (4096*32)
void *
histalloc( size_t sz ) {
	int i, fd;
	char *nm;
	int foo = 0;
	static char *next;
	static size_t size_left = 0;
	size_t tsz; 
	void *ret;
	static int malloc_flg = 0;

	if( malloc_flg )
		goto do_malloc;
	if( size_left < sz ) {
		fd = open( (nm = tmpnam( NULL )), O_RDWR | O_CREAT, 0600 );
		if( fd < 0 ) {
			goto do_malloc;
		}
		for( i = 0 ; i < HSIZE; i += 512 ) {
			lseek( fd, i, SEEK_SET );
			if(write( fd, &foo, sizeof(int) ) != sizeof(int) ){
				close(fd);
				unlink(nm);
				goto do_malloc;
			}
		}
		next = mmap( NULL, HSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)0);
		if( next == (caddr_t)(-1) ) {
			close(fd);
			unlink(nm);
			goto do_malloc;
		}
		close( fd );
		unlink(nm);
		size_left = HSIZE;
	}
	ret = (void *)next;
/*
 *	alignment
 */
	tsz = sz % sizeof( double );
	if( tsz )
		sz += sizeof( double ) - tsz;
	size_left -= sz;
	next += sz;
	return( ret );
do_malloc:
	malloc_flg = 1;
	return( malloc( sz ) );
}
