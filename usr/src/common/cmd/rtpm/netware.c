#ident	"@(#)rtpm:netware.c	1.3"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/spx_app.h>
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#include <sys/sap_app.h>
#include <curses.h>

#define min( a, b )	( (a) < (b) ? (a) : (b) )

SAPD	sap_buf;
SAPL	*sap_lan_buf = NULL;
int sap_total_servers;
int sap_total;
int ipx_total;
int spx_total;
int rip_total;
int netware_errs;
int sap_unused;
int nlans;
int ipx_datapackets;
int ipx_sent_to_TLI;
int ipx_total_ioctls;
int rip_total_router_packets_sent;
RouterInfo_t ripbuf;
struct {
	IpxLanStats_t	 l;
	IpxSocketStats_t s;
} ipxbuf;
spxStats_t	spxbuf;
spxConStats_t	*spxconbuf = NULL;

#define IPX		"/dev/ipx"
#define RIPX		"/dev/ripx"
#define SPX_DEVICE	"/dev/nspx"

static int maxspxconn = 0;
static int get_spx_info(int);
static int get_spx_conn_info(int, int);
static int ripstats( void );
static int spxstats( void );
static int ipxstats( void );
static int sapstats( void );
int spx_mused_conn = 0;
extern int in_curses;

void *histalloc( size_t sz );

void
netware_stat( void ) {

	ripstats();
	spxstats();
	ipxstats();
	sapstats();

	netware_errs = sap_buf.BadSizeInSaps +
	  sap_buf.BadSizeInSaps + 
	  sap_buf.BadSapSource + sap_buf.BadRipSaps +
	  spxbuf.spx_alloc_failures + spxbuf.spx_open_failures +
	  spxbuf.spx_connect_req_fails + spxbuf.spx_send_bad_mesg +
	  spxbuf.spx_rcv_bad_packet + spxbuf.spx_rcv_bad_data_packet +
	  spxbuf.spx_rcv_dup_packet + spxbuf.spx_abort_connection +
	  spxbuf.spx_max_retries_abort + spxbuf.spx_no_listeners +
	  ipxbuf.l.InProtoSize + ipxbuf.l.InBadDLPItype +
	  ipxbuf.l.InBadLength +
	  ipxbuf.l.InSapBad +
	  ipxbuf.l.InNICDropped + 
	  ipxbuf.l.OutBadLan + ipxbuf.s.IpxOutBadSize +
	  ipxbuf.s.IpxTLIOutBadState + ipxbuf.s.IpxTLIOutBadSize +
	  ipxbuf.s.IpxTLIOutBadOpt + ipxbuf.s.IpxTLIOutHdrAlloc +
	  ipxbuf.s.IpxSwitchSumFail + ipxbuf.s.IpxSwitchAllocFail +
	  ipxbuf.s.IpxSumFail +
	  ipxbuf.s.IpxRoutedTLIAlloc +
	  ripbuf.ReceivedNoLanKey + ripbuf.ReceivedBadLength +
	  ripbuf.ReceivedNoCoalesce + ripbuf.SentAllocFailed +
	  ripbuf.SentBadDestination;
}

static 
spxstats( void ) {
	static int spxFd = -2;
	int i;

	switch( spxFd ) {
	case -1:
		return( -1 );
		/* NOTREACHED */
		break;
	case -2:
		if( (spxFd = open(SPX_DEVICE, O_RDWR )) == -1 ) {
			return( -1 );
		}
		/* FALLTHROUGH */
	default:
		break;
	}
	if ((maxspxconn = get_spx_info(spxFd)) == -1) {
		return(-1); 
	}
	if( !spxconbuf ) {
		spxconbuf = histalloc( maxspxconn*sizeof( spxConStats_t ));
		if( !spxconbuf ) {
			if( in_curses )
				endwin();
			fprintf(stderr,"out of memory\n");
			exit(1);
		}
		memset( spxconbuf, 0, maxspxconn*sizeof( spxConStats_t ));
	}
	for(i=0; i < (int)spxbuf.spx_max_used_connections; i++) {
		if (get_spx_conn_info(spxFd, i) == -1) {
			return(-1); 
		}
	}

	spx_total = spxbuf.spx_rcv_packet_count
	  + spxbuf.spx_send_packet_count;
	return(0);
}

static int			/* Return: max SPX connection open */
get_spx_info(int spxFd)
{
	struct strioctl strioc;			/* ioctl structure */

	/*
	** Assemble and send spxbuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd = SPX_GET_STATS;
	strioc.ic_timout = INFTIM;
	strioc.ic_len = sizeof( spxbuf );
	strioc.ic_dp = (char *)&spxbuf;
	if (ioctl(spxFd, I_STR, &strioc) == -1) {
	        return(-1); 
	}
	spx_mused_conn = spxbuf.spx_max_used_connections;
	return(spxbuf.spx_max_connections);
}


static int					/* Return: status open */
get_spx_conn_info(int spxFd, int conId)
{
	struct strioctl strioc;			/* ioctl structure */

	/*
	** Assemble and send spxconbuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd = SPX_GET_CON_STATS;
	strioc.ic_timout = INFTIM;
	strioc.ic_len = sizeof( spxConStats_t );
	strioc.ic_dp = (char *)&spxconbuf[conId];

	spxconbuf[conId].con_connection_id = GETINT16(conId);

	if (ioctl(spxFd, I_STR, &strioc) == -1) {
		return(-1); 
	}
	return(0);
}

static int 
ipxstats( void ) {
	static int ipxFd = -2;	/* ipx clone device file descriptor */
	struct strioctl strioc;	/* ioctl structure */

	switch( ipxFd ) {
	case -1:
		return( -1 );
		/* NOTREACHED */
		break;
	case -2:
		if( (ipxFd = open(IPX, O_RDWR )) == -1 ) {
			return( -1 );
		}
		/* FALLTHROUGH */
	default:
		break;
	}

	/*
	** Assemble and send ipxbuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd =  IPX_STATS;
	strioc.ic_timout = INFTIM;
	strioc.ic_len = sizeof(ipxbuf);
	strioc.ic_dp = (char *)&ipxbuf;
	if ( ioctl( ipxFd, I_STR, &strioc ) < 0 ) {
		return ( -1 );
	}
	ipx_total = ipxbuf.l.InTotal + ipxbuf.l.OutTotalStream;
	ipx_datapackets = ipxbuf.s.IpxOutData + ipxbuf.s.IpxTLIOutData;
	ipx_sent_to_TLI = ipxbuf.s.IpxRoutedTLI - ipxbuf.s.IpxRoutedTLIAlloc;
	ipx_total_ioctls = ipxbuf.s.IpxIoctlBindSocket
	   + ipxbuf.s.IpxIoctlUnbindSocket
	   + ipxbuf.s.IpxIoctlSetWater + ipxbuf.s.IpxIoctlStats
	   + ipxbuf.s.IpxIoctlUnknown;

	return(0);
}

static int
ripstats( void ) {
	static int	ripFd = -2; /* rip clone device file descriptor */
	struct strioctl strioc;		/* ioctl structure */

	switch( ripFd ) {
	case -1:
		return( -1 );
		/* NOTREACHED */
		break;
	case -2:
		if( (ripFd = open(RIPX, O_RDWR )) == -1 ) {
			return( -1 );
		}
		/* FALLTHROUGH */
	default:
		break;
	}
	/*
	** Assemble and send infobuf request.
	** Block while waiting for return
	*/
	strioc.ic_cmd =  RIPX_STATS;
	strioc.ic_timout = INFTIM;
	strioc.ic_len = sizeof(ripbuf);
	strioc.ic_dp = (char *)&ripbuf;
	if ( ioctl( ripFd, I_STR, &strioc ) == -1 ) {
	        return ( -1 ); 
	}
	rip_total = ripbuf.ReceivedPackets + ripbuf.SentAllocFailed +
	  ripbuf.SentBadDestination + ripbuf.SentRequestPackets + 
	  ripbuf.SentResponsePackets;
	rip_total_router_packets_sent =
	  ripbuf.SentAllocFailed + ripbuf.SentBadDestination +
	  ripbuf.SentRequestPackets + ripbuf.SentResponsePackets;

	return(0);
}


static int
sapstats() {
	int i;
	static int status = -2;

	switch( status ) {
	case -2:
		if( SAPMapMemory() < 0) {
			status = -1;
			return(-1);
		} 
		status = 0;


		break;
	case -1:return( -1 );
		/* NOTREACHED */
		break;
	default:
		break;
	}
	(void) SAPStatistics( &sap_buf );
	if( !sap_lan_buf ) {
		sap_lan_buf = histalloc( sap_buf.Lans * sizeof( SAPL ) );
		if( !sap_lan_buf ) {
			if( in_curses )
				endwin();
			fprintf(stderr,"out of memory\n");
			exit(1);
		}
		memset( sap_lan_buf, 0, sap_buf.Lans * sizeof( SAPL ) );
	}
	for( i = 0; i < (int)sap_buf.Lans; i++)
		SAPGetLanData( i, &sap_lan_buf[i] );

	if( sap_buf.ServerPoolIdx == 0)
		sap_total_servers = sap_buf.ConfigServers;
	else
		sap_total_servers = sap_buf.ServerPoolIdx -1;
	sap_unused = sap_buf.ConfigServers - sap_total_servers;
	sap_total =  sap_buf.TotalInSaps + sap_buf.TotalOutSaps;
	nlans = sap_buf.Lans;
	return(0);
}
