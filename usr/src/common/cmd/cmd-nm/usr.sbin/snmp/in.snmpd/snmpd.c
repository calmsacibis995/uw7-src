#ident	"@(#)snmpd.c	1.8"
#ident   "$Header$"

/*
 * STREAMware TCP
 * Copyright 1987, 1993 Lachman Technology, Inc.
 * All Rights Reserved.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef lint
static char SNMPID[] = "@(#)snmpd.c	1.8";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */
/*
 * snmpd.c - An SNMP server daemon
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#if defined(SVR3) || defined(SVR4)
#include <fcntl.h>
#endif

#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stropts.h>
#include <sys/tiuser.h>
#endif

#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stream.h>
#include <netinet/ip_var.h>    
#include <netdir.h>
#include <sys/netconfig.h>
#if defined(SVR3) || defined(SVR4)
#include <net/route.h>
#endif
#include <netdb.h>
#include <syslog.h>
#include <locale.h>
#include <pfmt.h>
#include <unistd.h>

#include <nlist.h>
#include <sys/ksym.h>
#include <sys/stat.h>

#ifdef NETWARE
#include "netware.h"
#include <sys/sap_app.h>
#include <nwconfig.h>
#endif

#ifdef NEW_MIB
#include <paths.h>
#endif /* NEW_MIB */

/* snmp system header files */
#include "snmp.h"
#include "objects.h"
#include "snmpuser.h"

/* local header files */
#include "snmpd.h"
#include "peer.h"

#ifndef NEW_MIB
#if defined(SVR3) || defined(SVR4)
/* If this structure is changed, check the macros in snmpd.h! */
struct nlist nl[] = {  
  { "ipstat" },
  { "ifnet" },
  { "rthost" },
  { "rtnet" },
  { "icmpstat" },
  { "rtstat" },
  { "rthashsize" },
  { "arptab" },
  { "arptab_size" },
  { "net_to_reg_table" },
  { "reg_route_table" },
  { "reghashsize" },
  { "udpstat" },
  { "tcpstat" },
  { "tcb" },
  { "provider" },
  { "lastprov" },
  { "ntcp" },
  { "ipforwarding" },
  { "udb" },
  { "tcp_minrexmttimeout" },
  { "tcp_maxrexmttimeout" },
  { "ipq_ttl" },
  { "in_ip_ttl" },
  "",
};

char *system_name = "/unix";
char *kmemf = "/dev/kmem";
int kmem;
#endif
#endif /* not NEW_MIB */

struct COMMUNITIES {
  char community_name[64];
  int  privs;
  unsigned long ip_addr;
  struct {
    unsigned char    network[4];
    unsigned char    node[6];
  } ipx_addr;
} communities[25];
int num_communities;

/* Protocol types for the trap communities file */

#define IP_PROTO     0
#define IPX_PROTO    1
#define IPX_PROTO_STRING   "ipx"
#define IP_PROTO_STRING    "ip"

struct TRAP_COMMUNITIES {
  int proto;
  char community_name[64];
  unsigned long ip_addr;
  unsigned short remote_port;
  struct {
    unsigned char    network[4];
    unsigned char    node[6];
    unsigned char    sock[2];
  } ipx_addr;
} trap_communities[25];
int num_trap_communities;

time_t communities_timestamp;
time_t trap_communities_timestamp;

static struct smuxPeer peerque;
struct smuxPeer *PHead = &peerque;

static struct smuxTree treeque;
struct smuxTree *THead = &treeque;

struct smuxReserved reserved[] = {
    "snmp", NULL,
    "smux", NULL,

    NULL
};

/* extern functions */
extern Pdu *make_error_pdu();

/* functions in init_var.c */
VarEntry *init_var(void);

VarEntry *add_var(VarEntry *var_ptr,
		  OID oid_ptr,
		  unsigned int type,
		  unsigned int rw_flag,
		  unsigned int arg,
		  VarBindList *(*funct_get)(),
		  int (*funct_test_set)(),
		  int (*funct_set)());

#ifdef PPP_MIB
VarEntry *init_ppp_ip_mib(VarEntry *var_entry_ptr);
VarEntry *init_ppp_lcp_mib(VarEntry *var_entry_ptr);
#endif /* PPP_MIB */

#ifdef _DLPI
VarEntry *init_ether_mib(VarEntry *var_entry_ptr);
VarEntry *init_token_mib(VarEntry *var_entry_ptr);
#endif /* _DLPI */

/* functions in response.c */
Pdu *make_response_pdu(Pdu * in_pdu_ptr);

/* functions in sets.c */
Pdu *do_sets(Pdu *);

/* functions in smuxd.c */
void do_smux_old(int fd);
int do_smux_new(int fd);

/* globals */
VarEntry *var_list_root;
struct timeval global_tv; /* start time for use in sysLastInit var */
struct timezone global_tz;

#if !defined(TCP40) && !defined(SWTCP21)
/* added to support ifLastChange = timeticks of last change */
long global_if_times[MAXINTERFACES];
#endif
#if !defined(SVR3) && !defined(SVR4)
int global_gateway;
#endif
unsigned char global_sys_descr[256];
unsigned char global_sys_object_ID[256];
unsigned char global_sys_contact[256];
unsigned char global_sys_location[256];

fd_set ifds, sfds;
int nfds = 0;
int trap_fd;

#ifdef NETWARE
int snmp_ipx_fd;
int ipx_trap_fd;
#endif

int auth_traps_enabled = 1;   /* authentication traps are enabled */

#define  OK  0
#define  NOTOK -1

#ifdef BSD
extern int errno;
#endif
#if defined(SVR3) || defined(SVR4)
extern int t_errno;
#endif

#ifdef NEW_MIB
int arp_fd, icmp_fd, ip_fd, rte_fd, tcp_fd, udp_fd;
#endif

int log_level = 0;

int udp_service = 1;
int smux_service = 1;
int ipx_service = 1;

int shmid;     /* in.snmpd's shmid */

/* Buffer to hold incoming SNMP packets */
unsigned char  in_packet[2048];
long  in_packet_len;

/** function prototypes **/
void init_config(void);            /* read snmpd.conf file */
void init_globals(void);           /* init globals */
void init_communities(void);       /* init communities, read snmpd.comm file */
void init_trap_communities(void);  /* init trap comm., read snmpd.trap file */
void getnlist(char *ksystem,  struct nlist *nl); /* get kernel symbols */

int start_snmpd_server (void);     /* start IP */
int start_snmpd_ipx_server(void);  /* start IPX */
int stop_snmpd_ipx_server(void);   /* stop IPX */


/* send UDP trap */
int send_trap (int generic, int specific, VarBindList *varbinds); 

/* send IPX trap */
int send_ipx_trap(int generic,int specific, VarBindList *varbinds);

int udp_process (int snmp_fd);      /* udp packet processor */
int ipx_process(int fd);            /* IPX packet processor */

/* send UDP response */
int send_udp_response (int fd, struct sockaddr_in *to, 
                       unsigned char * out_packet, int out_packet_len);
#ifdef NETWARE
/* send IPX response */
int send_ipx_response(int fd, ipxAddr_t *daddr, unsigned char *buf, int len);
#endif /* NETWARE */

/* process UDP authentication */
int process_authentication (struct sockaddr_in *from, AuthHeader *auth_ptr);

#ifdef NETWARE		
/* process IPX authentication */
int process_ipx_authentication (ipxAddr_t *from, AuthHeader *auth_ptr);
#endif /* NETWARE */

#ifdef NETWARE
int print_ipxaddr(ipxAddr_t *p);      /* print ipx address to screen */
#endif /* NETWARE */

void upperit (char *ptr);             /* change a string  to upper case */

/* Exit of SNMP Agent */
void exitsnmp(int);

int curr_query = 0;			/* useful for multi-varbind requests */

/***** MAIN *****/

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;
    struct sockaddr_in from;
    extern void peer_timeout();

    int pid, i, cc;
    int smux_fd, snmp_fd;
    struct smuxReserved *sr;
    OID name;

    /* file descriptor set for polling */
    fd_set rfds, efds;
    int fd;

   (void)setlocale(LC_ALL, "");
   (void)setcat("nmsnmpd");
   (void)setlabel("NM:snmpd");

    /*  parse the command line options, if any */
   if (argc > 1) {

      if (strcmp(argv[1], "-v") == 0)
         log_level = 1;

   else {
      fprintf(stderr, gettxt(":1", "%s [-v]\n"), argv[0]);
      fprintf(stderr, gettxt(":2", "The optional -v argument enables verbose debug output.\n"));
      exit(-1);
      }
    }

#if defined(SVR3) || defined(SVR4)
    if (log_level == 0) {

   FILE    *snmpd_log;
   /*
    * Background the job
    */
   pid = fork();
   if (pid < 0) {
       perror("snmpd:  fork");
       return(-1);
   }
   if (pid != 0)
       return(0);

   /* Before losing current file context get somewhere for error output */
   if ((snmpd_log = fopen ("/usr/adm/snmpd.log", "w")) == NULL) {
        perror ("snmpd: fopen /usr/adm/snmpd.log");
        exit (-1);
   }

   /* Close all file handles apart from the new error handle */
   for (i = 0; i < 10; i++)
       if (i != fileno(snmpd_log))
	   (void) close(i);

   /* Duplicate error log handle to stdout stderror and stdin */
   dup (fileno(snmpd_log));
   dup (fileno(snmpd_log));
   dup (fileno(snmpd_log));

   /* Will not be writing directly to log file lose the handle */
   fclose (snmpd_log);
   errno = 0;       /* tidy up in case perror called later */

   setsid();
   }
#endif

    /* Create in.snmpd shared memory to store statistics */
    if ((shmid=shmget(SNMPD_SHMKEY, 
		sizeof(snmpstat_type),0666 | IPC_CREAT)) < 0)
    {
	perror("error creating shared memory.");
	exit( -1 );
    }
    if ((snmpstat=(snmpstat_type *)shmat(shmid, 0, 0))==
		(snmpstat_type*)-1)
    {
	perror("error attaching share memory.");
	exit( -1 );
    }
			    
    openlog("in.snmpd", LOG_PID, LOG_DAEMON);

    FD_ZERO (&ifds);
    FD_ZERO (&rfds);
    FD_ZERO (&efds);
    FD_ZERO (&sfds);

    PHead->pb_forw = PHead->pb_back = PHead;
    THead->tb_forw = THead->tb_back = THead;
    for (sr = reserved; sr->rb_text; sr++)
   if (name = make_obj_id_from_dot ((unsigned char *)(sr->rb_text)))
       sr->rb_name = name;

    if ((snmp_fd = start_snmpd_server ()) < 0) {
   perror ("Error opening the UDP endpoint");
        udp_service = 0;
    }
    else {
        if (snmp_fd >= nfds)
       nfds = snmp_fd + 1;
        FD_SET (snmp_fd, &ifds);
    }

    if ((smux_fd = start_smux_server ()) == NOTOK) {
   perror ("No SMUX Service Available");
        smux_service = 0;
    }
    else {
        if (smux_fd >= nfds)
            nfds = smux_fd + 1;
        FD_SET (smux_fd, &ifds);
    }

#ifdef NETWARE
    if ((snmp_ipx_fd = start_snmpd_ipx_server ()) < 0) {
   perror ("No IPX Service Available");
   ipx_service = 0;
    }
    else {
        if (snmp_ipx_fd >= nfds)
             nfds = snmp_ipx_fd + 1;
        FD_SET (snmp_ipx_fd, &ifds);
    }
#endif

    /* if there are no services we may as well get out now */

    if ((udp_service == 0) && (ipx_service == 0)) {
   perror ("No UDP or IPX Service Available");
   exit (-1);
    }

    /* initialize variable list */
    var_list_root = init_var();
#ifdef PPP_MIB
    var_list_root = init_ppp_lcp_mib(var_list_root);
    var_list_root = init_ppp_ip_mib(var_list_root);
#endif /* PPP_MIB */
#ifdef _DLPI
    var_list_root = init_ether_mib(var_list_root);
    var_list_root = init_token_mib(var_list_root);
#endif /* _DLPI */

    add_objects_aux();

    /* initialize the global variables, like kmem, etc */
    init_globals();

#ifdef NEW_MIB
    /* To poll for link-up/down traps */
    if (ip_fd >= nfds)
   nfds = ip_fd + 1;
    FD_SET (ip_fd, &ifds);
#endif

    /* initialize the community/privileges data space */
    init_communities ();
    communities_timestamp = time((long *)0);

    /* initialize the trap recipient data space */
    init_trap_communities ();
    trap_communities_timestamp = time((long *)0);

    init_config ();

#ifdef BSD
    (void) signal (SIGHUP, init_communities); 
    (void) signal (SIGALRM, peer_timeout); 
#endif
#if defined(SVR3) || defined(SVR4)
    (void) sigset (SIGHUP,  (void(*)()) init_communities); 
    (void) sigset (SIGALRM, peer_timeout); 
    (void) sigset (SIGKILL, exitsnmp);
    (void) sigset (SIGTERM, exitsnmp);
#endif

    send_trap (0, 0, NULL);
#ifdef NETWARE
    send_ipx_trap (0, 0, NULL);
#endif

    /* create SNMPD_PID_FILE /tmp/snmpd.pid file */
    {
      FILE * tmpfd;
      tmpfd=fopen(SNMPD_PID_FILE, "w");
      fprintf(tmpfd, "%d", getpid());
      fclose(tmpfd);
    }

/* Let's SAP */
#ifdef NETWARE
{
  char serverName[NWCM_MAX_STRING_SIZE];
  /* 0x026c is the sap type assigned to network management */
  /* 0x900f is the socket assigned to SNMP over IPX, see RFC1420 */
  uint16 SAP_NETMGT_TYPE=0x026c;
  uint16 SAP_NETMGT_SOCKET=0x900f;

  if(!NWCMGetParam( "server_name", NWCP_STRING, serverName))
    SAPAdvertiseMyServer(SAP_NETMGT_TYPE, (uint8 *)serverName,
			 SAP_NETMGT_SOCKET, SAP_ADVERTISE);
#ifdef DEBUG
  else
    fprintf(stderr, "\nNWCMGetParam failed.\n");
#endif
}
#endif /* NETWARE */

/* now the main loop... */
while (1) 
{
   rfds = ifds;
   efds = ifds;
   /* polling and all the related stuff here */
   /* It will timeout every 60 seconds. */
   if (xselect(nfds, &rfds, NULL, &efds, -1) < 0) {
       perror ("Error in polling");
            remove(SNMPD_PID_FILE);
       exit (-1);
   }

   if (udp_service) {
       if (FD_ISSET (snmp_fd, &rfds)) {
           if (udp_process (snmp_fd) < 0) {
               perror ("Error in processing the UDP request");
                    remove(SNMPD_PID_FILE);
          exit (-1);
           }
       }
   }

#ifdef NETWARE
   if (ipx_service) {
            if (FD_ISSET (snmp_ipx_fd, &efds))
      {
         stop_snmpd_ipx_server();
         FD_CLR(snmp_ipx_fd, &ifds);
         perror("IPX stack down, SNMP/IPX stopped");
      }
            else
       if (FD_ISSET (snmp_ipx_fd, &rfds)) {
           if (ipx_process (snmp_ipx_fd) < 0) {
               perror ("Error in processing the IPX request");
                    remove(SNMPD_PID_FILE);
          exit (-1);
           }
       }
   }
#endif

#ifdef NEW_MIB
   if (FD_ISSET (ip_fd, &rfds)) {
       if (process_link_trap () < 0) {
      perror ("Error in processing the link-up/down TRAP \n");
                remove(SNMPD_PID_FILE);
      exit (-1);
       }
   }
#endif

   if (smux_service) {
       if ((smux_fd != -1) && FD_ISSET (smux_fd, &rfds)) {
           if (log_level)
                    printf(gettxt(":3", "SMUX open request received.\n"));
           if ((fd = do_smux_new (smux_fd)) != NOTOK) {
                    if (fd >= nfds)
              nfds = fd + 1;
          FD_SET (fd, &ifds);
          FD_SET (fd, &sfds);
          if (log_level)
              printf(gettxt(":4", "New SMUX end-point opened.\n"));
           }
       }

       if (log_level)
           printf(gettxt(":5", "Checking other SMUX connections nfds(%d).\n"), nfds);

       for (fd = 0; fd < nfds; fd++)
           if (FD_ISSET (fd, &rfds) && FD_ISSET (fd, &sfds)) {
          if (log_level)
              printf(gettxt(":6", "Other SMUX requests being received fd(%d).\n"),fd);
          do_smux_old (fd);
           }
        }

    } /* end of while loop */

} /* end of main */


int   start_snmpd_server (void)
{
    int  i, fd;
    struct servent   *SimpleServ;
    struct sockaddr_in   sin;
    struct netconfig *tp;
#ifdef BSD
    /* set up network connection */
    if ((SimpleServ = getservbyname("snmp","udp")) == NULL) {
   syslog(LOG_ERR, gettxt(":7", "Add snmp 161/udp to /etc/inet/services.\n"));
   return(-1);
    }

    bzero((char *)&sin, sizeof(sin));
#ifdef __NEW_SOCKADDR__
    sin.sin_len = sizeof(struct sockaddr_in);
#endif
    sin.sin_family = AF_INET;
    sin.sin_port = SimpleServ->s_port;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
   perror("snmpd: fd");
   exit(1);
    }

    for (i = 0; ((bind(fd, &sin, sizeof(sin)) < 0) && (i < 5)); i++) {
   perror("snmpd: bind");
   sleep(3);
    }
    
    if ((trap_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
   perror("snmpd: trap_fd");
   return(-1);
    }
#endif

#if defined(SVR3) || defined(SVR4)
    struct t_bind  *req, *ret;

    /* set up network connection */
    if ((SimpleServ = getservbyname("snmp","udp")) == NULL) {
   syslog(LOG_ERR, gettxt(":7", "Add snmp 161/udp to /etc/inet/services.\n"));
   return(-1);
    }

    bzero((char *)&sin, sizeof(sin));
#ifdef __NEW_SOCKADDR__
    sin.sin_len = sizeof(struct sockaddr_in);
#endif
    sin.sin_family = AF_INET;
    sin.sin_port = SimpleServ->s_port;

    if ((fd = t_open(_PATH_UDP, O_RDWR|O_NDELAY, (struct t_info *)0)) < 0) {
   syslog(LOG_ERR, gettxt(":8", "t_open of %s failed: %s.\n"), _PATH_UDP, t_errmsg());
   return(-1);
    }

    if ((req = (struct t_bind *) t_alloc(fd, T_BIND, 0)) == NULL) {
   syslog(LOG_ERR, gettxt(":9", "t_alloc for t_bind request failed: %s.\n"), 
         t_errmsg());
   t_close(fd);
   return(-1);
    }
    if ((ret = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
   syslog(LOG_ERR, gettxt(":10", "t_alloc for t_bind response failed: %s.\n"), t_errmsg());
   t_close(fd);
   return(-1);
    }

    req->addr.buf = (char *) &sin;
    req->addr.len = sizeof(sin);
    req->qlen = 0;
    if (t_bind(fd, req, ret) < 0) {
   syslog(LOG_ERR, gettxt(":11", "t_bind failed: %s.\n"), t_errmsg());
   req->addr.buf = (char *) 0;
   t_free((char *)req, T_BIND);
   t_free((char *)ret, T_BIND);
   t_close(fd);
   return(-1);
    }

    if (bcmp(req->addr.buf, ret->addr.buf, req->addr.len) != 0) {
   syslog(LOG_ERR, gettxt(":12", "Couldn't bind to the requested address.\n"));
   req->addr.buf = (char *) 0;
   t_free((char *)req, T_BIND);
   t_free((char *)ret, T_BIND);
   t_close(fd);
   return(-1);
    }
    req->addr.buf = (char *) 0;
    t_free((char *)req, T_BIND);
    t_free((char *)ret, T_BIND);

    if ((trap_fd = t_open(_PATH_UDP, O_RDWR, (struct t_info *) 0)) < 0) {
         syslog(LOG_ERR, gettxt(":8", "t_open of %s failed: %s.\n"), 
               _PATH_UDP, t_errmsg());
   t_close(fd);
   return(-1);
    }

    /* set the TLI allow broadcast ip */
    if ((tp = getnetconfigent("udp")) == NULL) 
      {
	syslog(LOG_ERR,  "getnetconfigent failed: ");
	t_close(fd);
	t_close(trap_fd);
	return(-1);
      }

    if (netdir_options(tp, ND_SET_BROADCAST, trap_fd, NULL) < 0) 
      {
	syslog(LOG_ERR,  "netdir_options failed: ");
	t_close(fd);
	t_close(trap_fd);
	return(-1);
      }


    if (t_bind(trap_fd, (struct t_bind *)0, (struct t_bind *)0) < 0) {
      syslog(LOG_ERR, gettxt(":11", "t_bind failed: %s.\n"), t_errmsg());
      t_close(trap_fd);
      t_close(fd);
      return(-1);
      }
#endif
    if(log_level)
      printf(gettxt(":13", "snmpd server initialized.\n"));
    return (fd);

}

int recv_udp_request (int fd, struct sockaddr_in *from, int *fromlen)
{
  long packet_len;

#ifdef BSD
  if ((packet_len = recvfrom (fd, in_packet, sizeof(in_packet), 0, from,
			      fromlen)) < 0) {
    
    if (errno != EWOULDBLOCK)
      return (-1);
    return (0);
  }
#endif

#if defined(SVR3) || defined(SVR4)
  struct t_unitdata   unitdata;
  int  flags;

  unitdata.addr.buf = (char *) from;
  unitdata.addr.maxlen = sizeof (*from);
  unitdata.opt.maxlen = 0;
  unitdata.udata.buf = (char *) in_packet;
  unitdata.udata.maxlen = sizeof (in_packet);
  if (t_rcvudata (fd, &unitdata, &flags) < 0) 
    {
      if (t_errno != TNODATA) 
	{
	  if (t_errno == TLOOK)
	    if (t_rcvuderr(fd, (struct t_uderr *)0) == 0)
	      return (0);
	  syslog(LOG_WARNING, gettxt(":14", "t_rcvudata failed: %s.\n"), 
		 t_errmsg());
	  return (-1);
	}
      return (0);
    }
  packet_len = unitdata.udata.len;
#endif

  return packet_len;
}


int send_udp_response (int fd, struct sockaddr_in *to, 
                       unsigned char * out_packet, int out_packet_len)
{

#ifdef BSD
  if (sendto (fd, out_packet, out_packet_len, 0, to, tolen) <0) 
    {
      perror ("snmpd:  send");
      close (fd);
      return (-1);
    }
#endif

#if defined(SVR3) || defined(SVR4)
  struct t_unitdata   unitdata;

  unitdata.addr.buf = (char *) to;
  unitdata.addr.len = sizeof (*to);
  unitdata.opt.len = 0;
  unitdata.udata.buf = (char *) out_packet;
  unitdata.udata.len = out_packet_len;
  if (t_sndudata (fd, &unitdata) < 0) 
    {
      t_error ("snmpd:  t_sndudata");
      t_close (fd);
      return (-1);
    }
#endif
  return (0);

}


#ifdef NEW_MIB
int
process_link_trap ()
{
    struct  link_msg {
   int  idx;
   int  up;
    } link_msg;
    struct  strbuf   ctrl, data;
    char    buffer[256];
    OID     oid_ptr;
    int     flags, trap_type;
    VarBindList *vb_ptr;

    ctrl.buf = (char *)buffer;      /* to hold junk ??? */
    ctrl.len = 0;
    ctrl.maxlen = sizeof(buffer);

    data.buf = (char *)&link_msg;   /* Holds the message sent up */
    data.len = 0;
    data.maxlen = sizeof(link_msg);
    flags = 0;

    if(getmsg(ip_fd, &ctrl, &data, &flags) < 0) {
      syslog(LOG_WARNING, gettxt(":15", "process_link_trap: getmsg: %m.\n"));
   close(ip_fd);
   ip_fd = -1;
   return(-1);
    }

    if (link_msg.up)
   trap_type = 3;
    else
   trap_type = 2;

    sprintf(buffer, "ifIndex.%d", link_msg.idx);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, link_msg.idx, NULL, NULL);
    send_trap(trap_type, 0, vb_ptr);
    return(0);

}
#endif

/*----------------------------------------------------------------------------*

Function :	update_communities()

Description :	Function to check and update, if necessary,  the communities.
		This has been split out of udp_process().

Given :		Nothing

Returns :	Nothing ... fiddles with global tables

*----------------------------------------------------------------------------*/
void update_communities()
{
	struct stat communities_stat={0};

#ifdef BSD
	stat("snmpd.communities", &communities_stat);
#endif
#if defined(SVR3) || defined(SVR4)
	stat(SNMPD_COMM_FILE, &communities_stat);
#endif

	if (communities_stat.st_mtime > communities_timestamp) 
	{
		init_communities();
		communities_timestamp = time((long *)0);
	}

/* check to see if trap communities need to be updated */
#ifdef BSD
	stat("snmpd.trap_communities", &communities_stat);
#endif
#if defined(SVR3) || defined(SVR4)
	stat(SNMPD_TRAP_FILE, &communities_stat);
#endif
	if (communities_stat.st_mtime > trap_communities_timestamp) 
	{
		init_communities();
		trap_communities_timestamp = time((long *)0);
	}
	
	return;
}

/*----------------------------------------------------------------------------*

Function :	udp_process()

Description :	Reads a UDP SNMP packet
		Gets or Sets the attributes defined by it
		Builds and sends a response packet

Note :		This function used to be held together with numerous goto's and
		didn't handle error cases. It is now better but still a mess. 

Given :		File descriptor of UDP socket

Returns :	0  = Sent a good response packet even if it was an error
		-1 = Could send no response

*----------------------------------------------------------------------------*/
int udp_process (int   snmp_fd)
{
	Pdu	*out_pdu_ptr=NULL; 
	Pdu	*in_pdu_ptr=NULL; 
	Pdu	*error_pdu_ptr=NULL;
	AuthHeader *out_auth_ptr=NULL, *in_auth_ptr=NULL;
#ifdef __NEW_SOCKADDR__
	struct sockaddr_in  sin={sizeof(struct sockaddr_in), 0};
	struct sockaddr_in  from={sizeof(struct sockaddr_in), 0};
#else
	struct sockaddr_in  sin={0};
	struct sockaddr_in  from={0};
#endif
	int		fromlen = sizeof(from);
	int  		privs = 0;
	OctetString	*community_ptr = NULL;
	int		status = NO_ERROR;

	/* get a packet from udp port. The packet is stored in in_packet */
	if ((in_packet_len = recv_udp_request (snmp_fd, &from, &fromlen )) <= 0)
		return (in_packet_len);
  
	snmpstat->inpkts++;

	/* check and update communities */
	update_communities();

	if (log_level) 
	{
		printf(gettxt(":16", "\nINCOMING PACKET : %s.\n"), 
						inet_ntoa(from.sin_addr));
		printf(gettxt(":17", "in_packet_len = %d.\n"), in_packet_len);
		print_packet_out(in_packet, in_packet_len);
	}

	/* Allocate all the structures that will needed for the response. If 
	any of this fails then we will not be able to reply, even with an error 
	packet, so give up now */

	/* Allocate in_auth_ptr */
	if ((in_auth_ptr = parse_authentication(in_packet, in_packet_len)) 
									== NULL)
	{
		syslog(LOG_WARNING, gettxt(":18", 
					"%s: Error parsing packet.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Allocate community_ptr */
	if ((community_ptr = make_octetstring (
			in_auth_ptr->community->octet_ptr, 
			in_auth_ptr->community->length)) == NULL) 
	{
		syslog(LOG_WARNING, gettxt(":25","make_octetstring failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Allocates out_auth_ptr */
	if((out_auth_ptr = make_authentication(community_ptr)) == NULL) 
	{
		syslog(LOG_WARNING, gettxt(":26", 
					"make_authentication failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Allocate in_pdu_ptr */
	if((in_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) 
	{
		syslog(LOG_WARNING, gettxt(":20", 
					"%s: Error parsing pdu packlet.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* If we get here then we have enough allocated to send replies */

	/* Validate the in_auth_pointer */
	if (in_auth_ptr->version != VERSION)
	{
		status = GEN_ERROR;
		goto send_reply;
	}

	/* Check the authentication (community string) */
	if((privs = process_authentication (&from, in_auth_ptr)) == NONE) 
	{
		syslog(LOG_WARNING, gettxt(":19", 
				"Authentication failure from: %s.\n"), 
				inet_ntoa(from.sin_addr));
		if (auth_traps_enabled == 1)
			send_trap(4, 0, NULL);
		snmpstat->inbadcommunityuses++;
		status = GEN_ERROR;
		goto send_reply;
	}

	if (log_level) 
	{
		printf(gettxt(":21", "\nTHE VARIABLES IN REQUEST:\n"));
		print_varbind_list(in_pdu_ptr->var_bind_list);
	}

	/* Check what kind of packet we got */
	switch (in_pdu_ptr->type)
	{
		case GET_REQUEST_TYPE:
			snmpstat->ingetrequests++;
			break;

		case GET_NEXT_REQUEST_TYPE:
			snmpstat->ingetnexts++;
			break;

		case SET_REQUEST_TYPE:
			snmpstat->insetrequests++;
			/* Check that the community has READ_WRITE */
			if (privs!= READ_WRITE)    
			{
				snmpstat->inbadcommunityuses++;
				status = READ_ONLY_ERROR;
				goto send_reply;
			}

			/* Do the setting . Returns NULL if it worked and
			a pointer to a error pdu if it didn't */
			error_pdu_ptr = do_sets (in_pdu_ptr);
			break;

		default:
			status = GEN_ERROR;
			goto send_reply;
	}

	/* Generate a response PDU or pass on an error PDU */
	if( error_pdu_ptr != NULL )
		out_pdu_ptr = error_pdu_ptr;
	else
		if ((out_pdu_ptr = make_response_pdu (in_pdu_ptr)) == NULL)
		{
			syslog(LOG_WARNING, 
			       gettxt(":22", "make_response_pdu failed.\n"));
			status = GEN_ERROR;
			goto send_reply;
		}

  
	/* Log the results */
	if (log_level) 
	{
		printf(gettxt(":24", "\nTHE VARIABLES IN RESPONSE:\n"));
		print_varbind_list(out_pdu_ptr->var_bind_list);
	}

send_reply:

	/* If we get here then either ...
		1. 	All went well, status == NO_ERROR, out_pdu_ptr points 
			to a response PDU.
		2.	Something failed in a sub function, status == NO_ERROR
			and out_pdu_ptr points to its error PDU.
		or 3. 	There was an internal problem, status != NO_ERROR 
			out_pdu_ptr == NULL ... make an error PDU */

	if( status != NO_ERROR )
		out_pdu_ptr = make_error_pdu( GET_RESPONSE_TYPE,
			in_pdu_ptr->u.normpdu.request_id,
			status, 0, in_pdu_ptr);

	/* Last check for a null output pdu pointer */
	if( out_pdu_ptr == NULL )
		goto tidy_exit;

	/* Build outgoing PDU */
	if( build_pdu (out_pdu_ptr) == -1) 
	{
		syslog(LOG_WARNING, gettxt(":23", "build_pdu failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Build authentification for outgoing PDU */
	if(build_authentication (out_auth_ptr, out_pdu_ptr) == -1) 
	{
		syslog(LOG_WARNING,gettxt(":27", 
					"build_authentication failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

 	/* Log outgoing packet */ 
	if (log_level) 
	{
		printf(gettxt(":28", "\nOUTGOING PACKET:\n"));
		printf(gettxt(":29", "out_packet_len = %d.\n"), 
					out_auth_ptr->packlet->length);
		print_packet_out(out_auth_ptr->packlet->octet_ptr,
				       out_auth_ptr->packlet->length); 
	}
 
	/* Send the packet */
	if( send_udp_response (snmp_fd, &from, out_auth_ptr->packlet->octet_ptr,
			  out_auth_ptr->packlet->length) < 0 )
	{
		status = GEN_ERROR;
	}
	else
	{
		/* We sent something i.e. did our job even if it was an error */
		status = NO_ERROR;
		snmpstat->outpkts++;
	}

tidy_exit: ;

	/* This is where we get either after the response has been sent or at
	any time there is a non-recoverable error. Check which structures have 
	been allocated and free them. Setting the pointers to NULL may seem
	unnecessary but the libraries 'free' functions do not match with its
	'alloc' functions and if this isn't done odd things may happen */

	if( out_auth_ptr != NULL )
	{
		free_authentication (out_auth_ptr);
		/* This also free's the community string */
		community_ptr = NULL;
		out_auth_ptr = NULL;
	}
	if( community_ptr != NULL )
	{
		free_octetstring (community_ptr);
		community_ptr = NULL;
	}
	if( out_pdu_ptr != NULL )
	{
		free_pdu (out_pdu_ptr);
		out_pdu_ptr = NULL;
	}
	if( in_pdu_ptr != NULL )
	{
		free_pdu (in_pdu_ptr);
		in_pdu_ptr = NULL;
	}
	if( in_auth_ptr != NULL )
	{
		free_authentication (in_auth_ptr);
		in_auth_ptr = NULL;
	}

	if ( status != NO_ERROR )
		return ( -1 );
  
	return (0);
}


void init_globals (void)
{
    int i;
#ifdef NEW_MIB
    struct strioctl strioc;

    /* open all the end-points */
    if((arp_fd = open (_PATH_ARP, O_RDONLY)) < 0)
      syslog(LOG_WARNING, gettxt(":30", "Open of %s failed: %m.\n"), _PATH_ARP);

    if((icmp_fd = open (_PATH_ICMP, O_RDONLY)) < 0)
      syslog(LOG_WARNING, gettxt(":30", "Open of %s failed: %m.\n"), _PATH_ICMP);
    else {
   strioc.ic_cmd = SIOCSMGMT;
   strioc.ic_dp = (char *)0;
   strioc.ic_len = 0;
   strioc.ic_timout = -1;

   if (ioctl(icmp_fd, I_STR, &strioc) < 0) {
       syslog(LOG_WARNING, gettxt(":31", "icmp: ioctl SIOCSMGMT failed: %m.\n"));
       (void) close (icmp_fd);
       icmp_fd = -1;
   }
    }

    if((ip_fd = open (_PATH_IP, O_RDWR)) < 0)
      syslog(LOG_WARNING, gettxt(":30", "Open of %s failed: %m.\n"), _PATH_IP);
    else {
   /* Set up to receive link-up/down traps */
   strioc.ic_cmd = SIOCSIPTRAP;
   strioc.ic_dp = (char *)0;
   strioc.ic_len = 0;
   strioc.ic_timout = -1;

   if (ioctl(ip_fd, I_STR, &strioc) < 0) {
       syslog(LOG_WARNING, gettxt(":32", "ip: ioctl SIOCSIPTRAP failed: %m.\n"));
       (void) close (ip_fd);
       ip_fd = -1;
   }
    }

    if((rte_fd = open (_PATH_ROUTE, O_WRONLY)) < 0)
      syslog(LOG_WARNING, gettxt(":30", "Open of %s failed: %m.\n"), _PATH_ROUTE);

    if ((tcp_fd = open (_PATH_TCP, O_RDONLY)) < 0)
      syslog(LOG_WARNING, gettxt(":30", "Open of %s failed: %m.\n"), _PATH_TCP);
    else {
   strioc.ic_cmd = SIOCSMGMT;
   strioc.ic_dp = (char *)0;
   strioc.ic_len = 0;
   strioc.ic_timout = -1;

   if (ioctl(tcp_fd, I_STR, &strioc) < 0) {
       syslog(LOG_WARNING, gettxt(":33", "tcp: ioctl SIOCSMGMT failed: %m.\n"));
       (void) close (tcp_fd);
       tcp_fd = -1;
   }
    }

    if ((udp_fd = open (_PATH_UDP, O_RDONLY)) < 0)
      syslog(LOG_WARNING, gettxt(":30", "Open of %s failed: %m.\n"), _PATH_UDP);
    else {
   strioc.ic_cmd = SIOCSMGMT;
   strioc.ic_dp = (char *)0;
   strioc.ic_len = 0;
   strioc.ic_timout = -1;

   if (ioctl(udp_fd, I_STR, &strioc) < 0) {
       syslog(LOG_WARNING, gettxt(":34", "udp: ioctl SIOCSMGMT failed: %m.\n"));
       (void) close (udp_fd);
       udp_fd = -1;
   }
    }

#else
    getnlist (system_name, nl);

/*  if (nl[0].n_type == 0)   for everything else */
    if(nl[0].n_value == 0) {  /* for sun386i */
      syslog(LOG_ERR, gettxt(":35", "init_globals: %s: No namelist\n"), system_name);
   exit(1);
    }

    kmem = open(kmemf,2);

    if (kmem < 0) {
      syslog(LOG_ERR, gettxt(":36", "init_globals: Cannot open kmem: %m.\n"));
   exit(1);
    }
#endif  /* not NEW_MIB */

    /* OK, now time stamp for sysLastInit */
    gettimeofday(&global_tv, &global_tz);

    /* and initialize the array of times for interface state changes */
#if !defined(TCP40) && !defined(SWTCP21)
    for (i=0; i < MAXINTERFACES; i++) {
        global_if_times[i] = 0;  /* previous state changed at  */ 
            /* time = 0 timeticks      */
    }
#endif 

}


#if defined(SVR3) || defined(SVR4)
void
#endif
init_communities () 
{
    FILE *fp;
    char buffer[255];
    char temp_community_name[64];
    char temp_privs_str[64];
    int  temp_privs;
    char temp_addr_str[64];
    char temp_proto[64];
    unsigned long temp_addr;
    char c;
    char byte[3];
    int bval;
    int cc, i, j;

    num_communities = 0;
#if defined(SVR3) || defined(SVR4)
    if ((fp = fopen(SNMPD_COMM_FILE,"r")) == NULL) {
      syslog(LOG_ERR, gettxt(":37", "init_communities: Error opening /etc/netmgt/snmpd.comm: %m.\n"));
   exit(-1);
    }
#endif

    while(fgets(buffer,255,fp) != NULL) {

   if ((buffer[0] == '#') || (buffer[0] == '\n')) 
       continue;

   cc = sscanf (buffer,"%s %s %s %s", temp_proto, temp_community_name,
           temp_addr_str, temp_privs_str);
   
   if (cc != 4) {
       syslog(LOG_WARNING, gettxt(":38", "init_communities: Config error with: %s.\n"),
           buffer);
       continue;
   }

   upperit(temp_privs_str);
    
   if (strcmp(temp_privs_str, "NONE") == 0)
       temp_privs = NONE;
   else if (strcmp(temp_privs_str, "READ") == 0)
       temp_privs = READ_ONLY;
   else if (strcmp(temp_privs_str, "WRITE") == 0)
       temp_privs = READ_WRITE;
   else {
       syslog(LOG_WARNING, gettxt(":39", "init_communities: Bad privileges type with: %s.\n"),
              buffer);
       continue;
   }

   strcpy (communities[num_communities].community_name, temp_community_name);
   communities[num_communities].privs = temp_privs;

        if (strcmp(temp_proto,IPX_PROTO_STRING) == 0) {

            /* Parse the network portion of the address (the address field) */
            byte[2] = '\0';
            i = 0;
            j = 0;
            while (1) {
                byte[0] = temp_addr_str[i];
                byte[1] = temp_addr_str[i + 1];
                i += 2;
                sscanf (byte, "%x", &bval);
                communities[num_communities].ipx_addr.network[j++] = bval;
                if (temp_addr_str[i] == ':') {
                    break;
                }
            }

            /* Parse the node portion of the address (the address field) */
            i++;
            j = 0;
            while (1) {
                byte[0] = temp_addr_str[i];
                byte[1] = temp_addr_str[i + 1];
                i += 2;
                sscanf (byte, "%x", &bval);
                communities[num_communities].ipx_addr.node[j++] = bval;
                if ((temp_addr_str[i] == ' ') ||
                    (temp_addr_str[i] == '\t') ||
                    (temp_addr_str[i] == '\0')) {
                     break;
                }
            }

            num_communities++;

        }

        if (strcmp(temp_proto,IP_PROTO_STRING) == 0) {

       temp_addr = inet_addr(temp_addr_str);
       if (temp_addr == -1)  {
           syslog(LOG_WARNING, gettxt(":40", "init_communities: Bad IP address with: %s.\n"),
           buffer);
           continue;
       }

       communities[num_communities].ip_addr = temp_addr;

            num_communities++;

   } 

        /* Other protocols later? */

    } /* end of while */   
  
  fclose(fp);

}


void init_trap_communities (void) 
{
  FILE *fp;
  char buffer[255];
  char temp_proto[64];
  char temp_community_name[64];
  char temp_port_str[64];
  int  temp_port;
  char temp_addr_str[64];
  unsigned long temp_addr;
  char c;
  char byte[3];
  int bval;
  int cc, i, j;

  num_trap_communities = 0;
#if defined(SVR3) || defined(SVR4)
  if ((fp = fopen(SNMPD_TRAP_FILE, "r")) == NULL) 
    {
      syslog(LOG_ERR, gettxt(":41", "init_trap_communities: Error opening /etc/netmgt/snmpd.trap: %m.\n"));
      exit (-1);
    }
#endif
  
  while(fgets(buffer,255,fp) != NULL) 
    {
      if ((buffer[0] == '#') || (buffer[0] == '\n')) 
	continue;

      cc = sscanf (buffer,"%s %s %s %s", temp_proto, temp_community_name,
		   temp_addr_str, temp_port_str);

      if (cc != 4) 
	{
	  syslog(LOG_WARNING, gettxt(":42", "init_trap_communities:  Config error with: %s.\n"),
		 buffer);
	  continue;
	}

      if (strcmp(temp_proto,IPX_PROTO_STRING) == 0) 
	{
	  trap_communities[num_trap_communities].proto = IPX_PROTO;

	  strcpy (trap_communities[num_trap_communities].community_name,
		  temp_community_name);

	  /* Parse the network portion of the address (the address field) */
	  byte[2] = '\0';
	  i = 0;
	  j = 0;
	  while (1) 
	    {
	      byte[0] = temp_addr_str[i];
	      byte[1] = temp_addr_str[i + 1];
	      i += 2;
	      sscanf (byte, "%x", &bval);
	      trap_communities[num_trap_communities].ipx_addr.network[j++] = 
		bval;
	      if (temp_addr_str[i] == ':') 
		break;
            }

	  /* Parse the node portion of the address (the address field) */
	  i++;
	  j = 0;
	  while (1) 
	    {
	      byte[0] = temp_addr_str[i];
	      byte[1] = temp_addr_str[i + 1];
	      i += 2;
	      sscanf (byte, "%x", &bval);
	      trap_communities[num_trap_communities].ipx_addr.node[j++] = bval;
	      if ((temp_addr_str[i] == ' ') ||
		  (temp_addr_str[i] == '\t') ||
		  (temp_addr_str[i] == '\0')) 
		break;
            }

	  /* Parse the socket portion of the address (the port field) */
	  j = 0;
	  i = 0;
	  while (1) 
	    {
	      byte[0] = temp_port_str[i];
	      byte[1] = temp_port_str[i + 1];
	      i += 2;
	      sscanf (byte, "%x", &bval);
	      trap_communities[num_trap_communities].ipx_addr.sock[j++] = bval;
	      if ((temp_addr_str[i] == ' ') ||
		  (temp_addr_str[i] == '\t') ||
		  (temp_addr_str[i] == '\n') ||
		  (temp_addr_str[i] == '\0')) 
		break;
	    }

	  num_trap_communities++;
	  continue;

        } /* if IPX protocol */

        if (strcmp(temp_proto,IP_PROTO_STRING) == 0) 
	  {
	    trap_communities[num_trap_communities].proto = IP_PROTO;

	    temp_addr = inet_addr(temp_addr_str);
	    if (temp_addr == -1)  
	      {
		syslog(LOG_WARNING, gettxt(":43", "init_trap_communities: Bad IP address with: %s.\n"),
		       buffer);
		continue;
	      }

	    cc = sscanf(temp_port_str,"%d", &temp_port);
	    if (cc != 1) 
	      {
		syslog(LOG_WARNING, gettxt(":44", "init_trap_communities:  Bad port number: %s.\n"), temp_port_str);
		continue;
	      }
	    strcpy (trap_communities[num_trap_communities].community_name,
		    temp_community_name);
	    trap_communities[num_trap_communities].remote_port = temp_port;
	    trap_communities[num_trap_communities].ip_addr = temp_addr;
	    num_trap_communities++;
            continue;
	  } /* if IP protocol */
      /* Other protocols later? */
    } /* end of while */   
  fclose(fp);
}

void upperit (char *ptr)
{
  while (*ptr != '\0' && *ptr != '=') 
    {
      if (islower(*ptr))
	*ptr = toupper(*ptr);
      ptr++;
    }
}

void init_config(void)
{
    FILE *fp;
    char buffer[256];

    /* Do the defaults */
    strcpy ((char *)global_sys_descr,"Generic SNMPD Version 9.4.0.3");
    strcpy ((char *)global_sys_object_ID,"SNMP_Research_UNIX_agent.9.4.0.3");
#if !defined(SVR3) && !defined(SVR4)
    global_gateway = 2;    /* Host */
#endif

#if defined(SVR3) || defined(SVR4)
    if ((fp = fopen(SNMPD_CONF_FILE, "r")) == NULL) {
   syslog(LOG_ERR, gettxt(":45", "Error opening /etc/netmgt/snmpd.conf: %m.\n"));
   exit(1);
    }
#endif
  
    while (fgets(buffer, 255, fp) != NULL) {
   if (buffer[0] == '#')
       continue;

   /* some DainBramaged machines leave a
      line-feed on end.  Strip if off */

   if (buffer[strlen(buffer) -1] == 0x0A)
       buffer[strlen(buffer)-1] = 0;
   upperit(buffer);
   if (strncmp(buffer,"DESCR=", 6) == 0) 
       strcpy((char *)global_sys_descr, &buffer[6]);
   else if (strncmp(buffer,"OBJID=",6) == 0)
       strcpy((char *)global_sys_object_ID, &buffer[6]);
#if !defined(SVR3) && !defined(SVR4)
   else if (strncmp(buffer,"FORWARD=", 8) == 0) {
       if (strcmp(&buffer[8], "TRUE") == 0)
      global_gateway = 1;  /* gateway */
   }
#endif
   else if (strncmp(buffer,"CONTACT=",8) == 0)
       strcpy((char *)global_sys_contact, &buffer[8]);
   else if (strncmp(buffer,"LOCATION=",9) == 0)
       strcpy((char *)global_sys_location, &buffer[9]);
    }          /* end of while */
    fclose(fp);
    if (log_level) {
   printf(gettxt(":46","System Description = %s.\n"), global_sys_descr);
   printf(gettxt(":47", "System Object Identifier = %s.\n"), global_sys_object_ID);
#if !defined(SVR3) && !defined(SVR4)
   printf(gettxt(":48", "Forwarding = "));
   if (global_gateway == 1) {
       printf(gettxt(":49", "TRUE (gateway).\n"));
   }
   else {
       printf(gettxt(":50", "FALSE (host).\n"));
   }
#endif
    }
}

int process_authentication (struct sockaddr_in *from, AuthHeader *auth_ptr)
{
    struct COMMUNITIES *comp;
    int matched_one = 0;
    int privs = NONE;

    if (from == NULL)
        return (READ_ONLY);

    /* now back to the normal stuff */

    for (comp = communities; comp < &communities[num_communities]; comp++) {
   if ((auth_ptr->community->length == strlen(comp->community_name))
      && (strncmp((char *)auth_ptr->community->octet_ptr,
         comp->community_name,
         auth_ptr->community->length) == 0)) {
       matched_one = 1;
       if (from && ((comp->ip_addr == 0)
       || (comp->ip_addr == from->sin_addr.s_addr))) {
      if (privs != READ_WRITE) {
          privs = comp->privs;
      }
             }
   }
    }
    if (!matched_one)
   snmpstat->inbadcommunitynames++;

    /* do the proper magic to decrypt (none, for now) */
    return(privs);
} /* end of process_authentication() */


int send_trap (int generic, int specific, VarBindList *varbinds)
{
    struct servent *SimpleServ;
    int i;
    struct timeval tv;
    struct timezone tz;
    long timeticks;
    int t1, t2;
    unsigned long local_ip_addr;
    struct hostent *hp;
    struct sockaddr_in sin;
    OctetString *os_ptr, *community_ptr;
    OID oid_ptr;
    VarBindList *vb_ptr;
    Pdu *pdu_ptr;
    AuthHeader *auth_ptr;
    char hostname[40];
    char buffer[80];
#if defined(SVR3) || defined(SVR4)
    struct t_unitdata unitdata;
#endif
    if (udp_service == 0)
        return (-1);

    /* take time hack */
    gettimeofday(&tv, &tz);
    t1 = ((tv.tv_sec - global_tv.tv_sec) * 100);
    t2 = ((tv.tv_usec - global_tv.tv_usec) / 10000); 
    timeticks = ((tv.tv_sec - global_tv.tv_sec) * 100) +
      ((tv.tv_usec - global_tv.tv_usec) / 10000); 
    
    gethostname(hostname, sizeof(hostname));
    local_ip_addr = inet_addr(hostname);
    if (local_ip_addr == -1) {
   hp = gethostbyname(hostname);
   if (hp) 
       bcopy(hp->h_addr, &local_ip_addr, hp->h_length);
   else {
       syslog(LOG_ERR, gettxt(":51", "%s: Host unknown.\n"), hostname);
       exit(1);
   }
    }

    local_ip_addr = ntohl(local_ip_addr);

    for (i = 0; i < num_trap_communities; i++) {

        if (trap_communities[i].proto != IP_PROTO)
            continue;

   bzero((char *)&sin, sizeof(sin));
   sin.sin_addr.s_addr = trap_communities[i].ip_addr; /* already net ordered */
#ifdef __NEW_SOCKADDR__
   sin.sin_len = sizeof(struct sockaddr_in);
#endif
   sin.sin_family = AF_INET;
   sin.sin_port = htons(trap_communities[i].remote_port);
   /* start a PDU */
   oid_ptr = make_obj_id_from_dot(global_sys_object_ID);

   /* convert ip addr to os_ptr */
   sprintf(buffer, "%02x %02x %02x %02x", ((local_ip_addr>>24)&0xFF),
      ((local_ip_addr>>16)&0xFF),((local_ip_addr>>8)&0xFF),
      (local_ip_addr&0xFF));
   os_ptr = make_octet_from_hex((unsigned char *)buffer);
   pdu_ptr = make_pdu(TRAP_TYPE, 0L, 0L, 0L, oid_ptr, os_ptr,
      generic, specific, timeticks);
   oid_ptr = NULL;
   os_ptr = NULL;

   /* Gotta put some sort of pdu on end - NULL pdu not allowed */
   if (varbinds == NULL) {
       oid_ptr = make_obj_id_from_dot((unsigned char *)("1.3.6.1"));
       vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0, 0, NULL, NULL);
       oid_ptr = NULL;
       link_varbind(pdu_ptr, vb_ptr);
   }
   else {
       link_varbind(pdu_ptr, varbinds);
       vb_ptr = NULL;
   }

   build_pdu(pdu_ptr);
  
   /* 
    * Make the AuthHeader object of your choice, copying the
    * the 'community' and inserting the previously made PDU
    */
   community_ptr = make_octet_from_text ((unsigned char *)trap_communities[i].community_name);
   auth_ptr = make_authentication(community_ptr);
   community_ptr = NULL; /* clean up OS */
    
   /* make final packet */
   build_authentication(auth_ptr, pdu_ptr);
  
#ifdef BSD
   if (sendto (trap_fd, packet, packet_len, 0, &sin,
           sizeof(sin)) <0) {
       perror("snmpd:  trap_fd send");
       close(trap_fd);

       free_authentication(auth_ptr); auth_ptr = NULL;
       free_pdu(pdu_ptr); pdu_ptr = NULL;

       return(-1);
   }
#endif
#if defined(SVR3) || defined(SVR4)
   unitdata.addr.buf = (char *) &sin;
   unitdata.addr.len = sizeof(sin);
   unitdata.opt.len = 0;
   unitdata.udata.buf = (char *) auth_ptr->packlet->octet_ptr;
   unitdata.udata.len = auth_ptr->packlet->length;

   if (t_sndudata(trap_fd, &unitdata) < 0) {
       syslog(LOG_WARNING, gettxt(":52", "t_sndudata (trap_fd) failed: %s.\n"), t_errmsg());

       free_authentication(auth_ptr); auth_ptr = NULL;
       free_pdu(pdu_ptr); pdu_ptr = NULL;

       return(-1);
   }
#endif
   snmpstat->outpkts++;
    
   /* clean up time */
   free_authentication(auth_ptr); auth_ptr = NULL;
   free_octetstring(pdu_ptr->packlet);
   if (pdu_ptr->type == TRAP_TYPE) {
       free_oid(pdu_ptr->u.trappdu.enterprise);
       free_octetstring(pdu_ptr->u.trappdu.agent_addr);
   }
   free_varbind_list(vb_ptr);
   pdu_ptr->var_bind_list = NULL;
   pdu_ptr->var_bind_end_ptr = NULL;
   free(pdu_ptr); pdu_ptr = NULL;
    } /* end of for */
}

/*
 * getnlist - use getksym to get symbol values
 */
void getnlist(char *ksystem,  struct nlist *nl)
{
  u_long tmp;
  while(nl->n_name[0] != '\0') 
    {
      (void) getksym(nl->n_name, (unsigned long *)(&nl->n_value), &tmp);
      nl++;
    }
}

/*
 * t_errmsg - return TLI error message or system error message
 */

char *
t_errmsg()
{
   extern int errno, t_nerr, sys_nerr;
   extern char *t_errlist[], *sys_errlist[];
   static char ebuf[64];

   if (t_errno == TSYSERR) {
      if (errno < sys_nerr)
         return sys_errlist[errno];
      else {
         sprintf(ebuf, gettxt(":53", "System error %d.\n"), errno);
         return ebuf;
      }
   }
   else {
      if (t_errno < t_nerr)
         return t_errlist[t_errno];
      else {
         sprintf(ebuf, gettxt(":54", "TLI error %d.\n"), t_errno);
         return ebuf;
      }
   }
}

#ifdef NETWARE
int start_snmpd_ipx_server(void)
{
   int             fd;
   struct t_bind   bind,
                   rbind,
                  *bp = &bind,
                  *rbp = &rbind;
   ipxAddr_t       localaddr,
                   boundaddr;
   struct servent *SimpleServ;


   if ((SimpleServ = getservbyname("snmp", "ipx")) == NULL) {
      syslog(LOG_ERR, gettxt(":55", "Add snmp 36879/ipx to /etc/inet/services.\n"));
      return (-1);
   }

   if ((fd = t_open(_PATH_IPX, O_RDWR | O_NDELAY, (struct t_info *) 0)) < 0) {
      syslog(LOG_ERR, gettxt(":8", "t_open of %s failed: %s.\n"), _PATH_IPX, t_errmsg());
      return (-1);
   }

   localaddr.sock[0] = (0xff & SimpleServ->s_port);
   localaddr.sock[1] = (0xff & (SimpleServ->s_port >> 8));

   bp->addr.len = sizeof(ipxAddr_t);
   bp->addr.maxlen = sizeof(ipxAddr_t);
   bp->addr.buf = (char *) &localaddr;
   bp->qlen = 0;

   rbp->addr.len = sizeof(ipxAddr_t);
   rbp->addr.maxlen = sizeof(ipxAddr_t);
   rbp->addr.buf = (char *) &boundaddr;
   rbp->qlen = 0;

   if (t_bind(fd, bp, rbp) < 0) {
      syslog(LOG_ERR, gettxt(":56", "t_bind failed for IPX endpoint: %s.\n"), t_errmsg());
      t_close(fd);
      return (-1);
   }

   if (log_level) {
      printf(gettxt(":57", "IPX: SNMP t_bind returned socket 0x%0.2x%0.2x.\n"),
             ((ipxAddr_t *) (rbp->addr.buf))->sock[0],
             ((ipxAddr_t *) (rbp->addr.buf))->sock[1]);
      printf(gettxt(":58", "IPX endpoint initialized fd(%d).\n"), fd);
   }

   if ((ipx_trap_fd = t_open(_PATH_IPX, O_RDWR, (struct t_info *) 0)) < 0) {
      syslog(LOG_ERR, gettxt(":8", "t_open of %s failed: %s.\n"), _PATH_IPX,
         t_errmsg());
      t_close(fd);
      return (-1);
   }

   bzero(&localaddr, sizeof(ipxAddr_t));
   bzero(&boundaddr, sizeof(ipxAddr_t));
   bzero(bp, sizeof(struct t_bind));
   bzero(rbp, sizeof(struct t_bind));

   /* Socket number is set to zero above to indicate a dynamic binding */

   bp->addr.len = sizeof(ipxAddr_t);
   bp->addr.maxlen = sizeof(ipxAddr_t);
   bp->addr.buf = (char *) &localaddr;
   bp->qlen = 0;

   rbp->addr.len = sizeof(ipxAddr_t);
   rbp->addr.maxlen = sizeof(ipxAddr_t);
   rbp->addr.buf = (char *) &boundaddr;
   rbp->qlen = 0;

   if (t_bind(ipx_trap_fd, bp, rbp) < 0) {
      syslog(LOG_ERR, gettxt(":11", "t_bind failed: %s.\n"), t_errmsg());
      t_close(ipx_trap_fd);
      t_close(fd);
      return (-1);
   }

   if (log_level) {
      printf(gettxt(":59", "IPX: SNMP-TRAP t_bind returned socket 0x%0.2x%0.2x.\n"),
             ((ipxAddr_t *) (rbp->addr.buf))->sock[0],
             ((ipxAddr_t *) (rbp->addr.buf))->sock[1]);
      printf(gettxt(":60", "IPX trap endpoint initialized fd(%d).\n"), ipx_trap_fd);
   }

   return (fd);
}

/* stop IPX */
int stop_snmpd_ipx_server(void)
{
   ipx_service=0;
   t_close(snmp_ipx_fd);
   t_close(ipx_trap_fd);
}

/*----------------------------------------------------------------------------*

Function :	ipx_process()

Description :	Reads a IPX SNMP packet
		Gets or Sets the attributes defined by it
		Builds and sends a response packet

Note :		Reworked to resemble UDP version which handles error 
		conditions better. Since we have no way to generate IPX
		SNMP requests this is currently untested.

Given :		File descriptor of IPX socket

Returns :	0  = Sent a good response packet even if it was an error
		-1 = Could send no response

*----------------------------------------------------------------------------*/
int ipx_process (int snmp_fd)
{
	struct t_unitdata ud;
	unsigned char   packet_type;
	int             in_packet_len;
	ipxAddr_t       saddr;
	int             flags = 0;
	Pdu	*out_pdu_ptr=NULL; 
	Pdu	*in_pdu_ptr=NULL; 
	Pdu	*error_pdu_ptr=NULL;
	AuthHeader *out_auth_ptr=NULL, *in_auth_ptr=NULL;
	int  		privs = 0;
	OctetString	*community_ptr = NULL;
	int		status = NO_ERROR;

	/* get a packet from ipx port. The packet is stored in in_packet */
	ud.opt.len = sizeof(packet_type);
	ud.opt.maxlen = sizeof(packet_type);
	ud.opt.buf = (char *) &packet_type;

	ud.addr.len = sizeof(ipxAddr_t);
	ud.addr.maxlen = sizeof(ipxAddr_t);
	ud.addr.buf = (char *) &saddr;

	ud.udata.len = IPX_MAX_DATA;
	ud.udata.maxlen = IPX_MAX_DATA;
	ud.udata.buf = (char *) in_packet;

	if (log_level)
		printf(gettxt(":61", "IPX process entered.\n"));

	if (t_rcvudata(snmp_fd, &ud, &flags) < 0) 
	{
		if (t_errno != TNODATA) 
		{
			if (t_errno == TLOOK)
				if (t_rcvuderr(snmp_fd,(struct t_uderr *) 0) 
									== 0)
					return (0);
			syslog(LOG_WARNING, gettxt(":62", 
				"t_rcvudata failed on IPX endpoint: %s.\n"),
			t_errmsg());
		 return (-1);
		}
	}
	in_packet_len = ud.udata.len;

	snmpstat->inpkts++;

	/* check and update communities */
	update_communities();

	if (log_level) {
		printf(gettxt(":63", "IPX process packet received.\n"));
		printf(gettxt(":64", "packet_len = %d.\n"), in_packet_len);
		printf(gettxt(":65", "packet_type = %d.\n"), packet_type);
		print_ipxaddr(&saddr);
		print_packet_out(in_packet, in_packet_len);
	}

	/* Allocate all the structures that will needed for the response. If 
	any of this fails then we will not be able to reply, even with an error 
	packet, so give up now */

	/* Allocate in_auth_ptr */
	if ((in_auth_ptr = parse_authentication(in_packet, in_packet_len)) 
									== NULL)
	{
		syslog(LOG_WARNING, gettxt(":66", 
				"Error parsing packet in authentication.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Allocate community_ptr */
	if ((community_ptr = make_octetstring (
			in_auth_ptr->community->octet_ptr, 
			in_auth_ptr->community->length)) == NULL) 
	{
		syslog(LOG_WARNING, gettxt(":25","make_octetstring failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Allocates out_auth_ptr */
	if((out_auth_ptr = make_authentication(community_ptr)) == NULL) 
	{
		syslog(LOG_WARNING, gettxt(":26", 
					"make_authentication failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Allocate in_pdu_ptr */
	if((in_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) 
	{
		syslog(LOG_WARNING, gettxt(":20", 
					"%s: Error parsing pdu packlet.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* If we get here then we have enough allocated to send replies */

	/* Validate the in_auth_pointer */
	if (in_auth_ptr->version != VERSION)
	{
		syslog(LOG_WARNING, gettxt(":67", "Version mismatch.\n"));
		status = GEN_ERROR;
		goto send_reply;
	}

	/* Check the authentication (community string) */
	if ((privs = process_ipx_authentication(&saddr, in_auth_ptr)) == NONE)
	{
		syslog(LOG_WARNING, gettxt(":68", "Authentication error.\n"));
		if (auth_traps_enabled == 1)
			send_trap(4, 0, NULL);
		snmpstat->inbadcommunityuses++;
		status = GEN_ERROR;
		goto send_reply;
	}

	if (log_level) 
	{
		printf(gettxt(":21", "\nTHE VARIABLES IN REQUEST:\n"));
		print_varbind_list(in_pdu_ptr->var_bind_list);
	}

	/* Check what kind of packet we got */
	switch (in_pdu_ptr->type)
	{
		case GET_REQUEST_TYPE:
			snmpstat->ingetrequests++;
			break;

		case GET_NEXT_REQUEST_TYPE:
			snmpstat->ingetnexts++;
			break;

		case SET_REQUEST_TYPE:
			snmpstat->insetrequests++;
			/* Check that the community has READ_WRITE */
			if (privs!= READ_WRITE)    
			{
				snmpstat->inbadcommunityuses++;
				status = READ_ONLY_ERROR;
				goto send_reply;
			}

			/* Do the setting . Returns NULL if it worked and
			a pointer to a error pdu if it didn't */
			error_pdu_ptr = do_sets (in_pdu_ptr);
			break;

		default:
			status = GEN_ERROR;
			goto send_reply;
	}

	/* Generate a response PDU or pass on an error PDU */
	if( error_pdu_ptr != NULL )
		out_pdu_ptr = error_pdu_ptr;
	else
		if ((out_pdu_ptr = make_response_pdu (in_pdu_ptr)) == NULL)
		{
			syslog(LOG_WARNING, 
			       gettxt(":22", "make_response_pdu failed.\n"));
			status = GEN_ERROR;
			goto send_reply;
		}

  
	/* Log the results */
	if (log_level) 
	{
		printf(gettxt(":24", "\nTHE VARIABLES IN RESPONSE:\n"));
		print_varbind_list(out_pdu_ptr->var_bind_list);
	}

send_reply:

	/* If we get here then either ...
		1. 	All went well, status == NO_ERROR, out_pdu_ptr points 
			to a response PDU.
		2.	Something failed in a sub function, status == NO_ERROR
			and out_pdu_ptr points to its error PDU.
		or 3. 	There was an internal problem, status != NO_ERROR 
			out_pdu_ptr == NULL ... make an error PDU */

	if( status != NO_ERROR )
		out_pdu_ptr = make_error_pdu( GET_RESPONSE_TYPE,
			in_pdu_ptr->u.normpdu.request_id,
			status, 0, in_pdu_ptr);

	/* Last check for a null output pdu pointer */
	if( out_pdu_ptr == NULL )
		goto tidy_exit;

	/* Build outgoing PDU */
	if( build_pdu (out_pdu_ptr) == -1) 
	{
		syslog(LOG_WARNING, gettxt(":23", "build_pdu failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

	/* Build authentification for outgoing PDU */
	if(build_authentication (out_auth_ptr, out_pdu_ptr) == -1) 
	{
		syslog(LOG_WARNING,gettxt(":27", 
					"build_authentication failed.\n"));
		status = GEN_ERROR;
		goto tidy_exit;
	}

 	/* Log outgoing packet */ 
	if (log_level) 
	{
		printf(gettxt(":69", "OUTGOING PACKET TO IPX ENDPOINT:\n"));
		printf(gettxt(":70", "spacket_len = %d.\n"), 
					out_auth_ptr->packlet->length);
		print_ipxaddr(&saddr);
		print_packet_out(out_auth_ptr->packlet->octet_ptr,
					out_auth_ptr->packlet->length);
	}
 
	/* Send the packet */
	if(send_ipx_response(snmp_fd, &saddr, out_auth_ptr->packlet->octet_ptr, 
					out_auth_ptr->packlet->length) < 0 )

	{
		status = GEN_ERROR;
	}
	else
	{
		/* We sent something i.e. did our job even if it was an error */
		status = NO_ERROR;
		snmpstat->outpkts++;
	}

tidy_exit: ;

	/* This is where we get either after the response has been sent or at
	any time there is a non-recoverable error. Check which structures have 
	been allocated and free them. Setting the pointers to NULL may seem
	unnecessary but the libraries 'free' functions do not match with its
	'alloc' functions and if this isn't done odd things may happen */

	if( out_auth_ptr != NULL )
	{
		free_authentication (out_auth_ptr);
		/* This also free's the community string */
		community_ptr = NULL;
		out_auth_ptr = NULL;
	}
	if( community_ptr != NULL )
	{
		free_octetstring (community_ptr);
		community_ptr = NULL;
	}
	if( out_pdu_ptr != NULL )
	{
		free_pdu (out_pdu_ptr);
		out_pdu_ptr = NULL;
	}
	if( in_pdu_ptr != NULL )
	{
		free_pdu (in_pdu_ptr);
		in_pdu_ptr = NULL;
	}
	if( in_auth_ptr != NULL )
	{
		free_authentication (in_auth_ptr);
		in_auth_ptr = NULL;
	}

	if ( status != NO_ERROR )
		return ( -1 );
  
	return (0);
}

int process_ipx_authentication (ipxAddr_t *from, AuthHeader *auth_ptr)
{
    struct COMMUNITIES *comp;
    int matched_one = 0;
    int privs = NONE;

    for (comp = communities; comp < &communities[num_communities]; comp++) {
   if ((auth_ptr->community->length == strlen(comp->community_name))
      && (strncmp((char *)auth_ptr->community->octet_ptr,
         comp->community_name,
         auth_ptr->community->length) == 0)) {
       matched_one = 1;
       if ((comp->ipx_addr.network[0] == 0 &&
      comp->ipx_addr.network[1] == 0 &&
      comp->ipx_addr.network[2] == 0 &&
      comp->ipx_addr.network[3] == 0 &&
      comp->ipx_addr.node[0] == 0 &&
      comp->ipx_addr.node[1] == 0 &&
      comp->ipx_addr.node[2] == 0 &&
      comp->ipx_addr.node[3] == 0 &&
      comp->ipx_addr.node[4] == 0 &&
      comp->ipx_addr.node[5] == 0) || 
      (comp->ipx_addr.network[0] == from->net[0] &&
      comp->ipx_addr.network[1] == from->net[1] &&
      comp->ipx_addr.network[2] == from->net[2] &&
      comp->ipx_addr.network[3] == from->net[3] &&
      comp->ipx_addr.node[0] == from->node[0] &&
      comp->ipx_addr.node[1] == from->node[1] &&
      comp->ipx_addr.node[2] == from->node[2] &&
      comp->ipx_addr.node[3] == from->node[3] &&
      comp->ipx_addr.node[4] == from->node[4] &&
      comp->ipx_addr.node[5] == from->node[5])) {
         if (privs != READ_WRITE) {
             privs = comp->privs;
      }
             }
   }
    }
    if (!matched_one)
   snmpstat->inbadcommunitynames++;

    /* do the proper magic to decrypt (none, for now) */
    return(privs);
} /* end of process_ipx_authentication() */

/* send IPX response */
int send_ipx_response(int fd, ipxAddr_t *daddr, unsigned char *buf, int len)
{

   struct t_unitdata ud;
   unsigned char   packet_type = IPX_PACKET_TYPE;

   ud.opt.len = sizeof(packet_type);
   ud.opt.maxlen = sizeof(packet_type);
   ud.opt.buf = (char *) &packet_type;

   ud.addr.len = sizeof(ipxAddr_t);
   ud.addr.maxlen = sizeof(ipxAddr_t);
   ud.addr.buf = (char *) daddr;

   ud.udata.len = len;
   ud.udata.maxlen = IPX_MAX_DATA;
   ud.udata.buf = (char *) buf;

   if (t_sndudata(fd, &ud) < 0) {
      t_error("snmpd:  t_sndudata for ipx endpoint");
      t_close(fd);
      return (-1);
   }
   return (0);
}

/* send IPX trap */
int send_ipx_trap(int generic,int specific, VarBindList *varbinds)
{
   struct servent *SimpleServ;
   int             i;
   struct timeval  tv;
   struct timezone tz;
   long            timeticks;
   int             t1,
                   t2;
   unsigned long   local_ip_addr;
   struct hostent *hp;
   OctetString    *os_ptr,
                  *community_ptr;
   OID              oid_ptr;
   VarBindList *vb_ptr;
   Pdu            *pdu_ptr;
   AuthHeader     *auth_ptr;
   char            buffer[80];

   struct t_unitdata ud;
   unsigned char  packet_type = IPX_PACKET_TYPE;

   if (ipx_service == 0)
      return (-1);

   /* take time hack */
   gettimeofday(&tv, &tz);
   t1 = ((tv.tv_sec - global_tv.tv_sec) * 100);
   t2 = ((tv.tv_usec - global_tv.tv_usec) / 10000);
   timeticks = ((tv.tv_sec - global_tv.tv_sec) * 100) +
      ((tv.tv_usec - global_tv.tv_usec) / 10000);

   local_ip_addr = 0; /* RFC1420 */

   for (i = 0; i < num_trap_communities; i++) {

      if (trap_communities[i].proto != IPX_PROTO)
         continue;


      /* start a PDU */
      oid_ptr = make_obj_id_from_dot(global_sys_object_ID);

      /* convert ip addr to os_ptr */
      sprintf(buffer, "%02x %02x %02x %02x", ((local_ip_addr >> 24) & 0xFF),
         ((local_ip_addr >> 16) & 0xFF), ((local_ip_addr >> 8) & 0xFF),
         (local_ip_addr & 0xFF));

      os_ptr = make_octet_from_hex((unsigned char *)buffer);
      pdu_ptr = make_pdu(TRAP_TYPE, 0L, 0L, 0L, oid_ptr, os_ptr,
               generic, specific, timeticks);
      oid_ptr = NULL;
      os_ptr = NULL;

      /* Gotta put some sort of pdu on end - NULL pdu not allowed */
      if (varbinds == NULL) {
         oid_ptr = make_obj_id_from_dot((unsigned char *)"1.3.6.1");
         vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0, 0, NULL, NULL);
         oid_ptr = NULL;
         link_varbind(pdu_ptr, vb_ptr);
      } else {
         link_varbind(pdu_ptr, varbinds);
         vb_ptr = NULL;
      }

      build_pdu(pdu_ptr);

      /*
       * Make the AuthHeader object of your choice, copying the the
       * 'community' and inserting the previously made PDU
       */
      community_ptr = make_octet_from_text((unsigned char *)trap_communities[i].community_name);
      auth_ptr = make_authentication(community_ptr);
      community_ptr = NULL;   /* clean up OS */

      /* make final packet */
      build_authentication(auth_ptr, pdu_ptr);

      ud.opt.len = sizeof(packet_type);
      ud.opt.maxlen = sizeof(packet_type);
      ud.opt.buf = (char *) &packet_type;

      ud.addr.len = sizeof(ipxAddr_t);
      ud.addr.maxlen = sizeof(ipxAddr_t);
      ud.addr.buf = (char *)&(trap_communities[i].ipx_addr);

      ud.udata.len = auth_ptr->packlet->length;
      ud.udata.maxlen = IPX_MAX_DATA;
      ud.udata.buf = (char *) auth_ptr->packlet->octet_ptr;

      if (t_sndudata(ipx_trap_fd, &ud) < 0) {
         syslog(LOG_WARNING, gettxt(":71", "t_sndudata (ipx_trap_fd) failed: %s.\n"),
            t_errmsg());
         free_authentication(auth_ptr);
         auth_ptr = NULL;
         free_pdu(pdu_ptr);
         pdu_ptr = NULL;

         return (-1);
      }

      snmpstat->outpkts++;

      /* clean up time */
      free_authentication(auth_ptr);
      auth_ptr = NULL;
      free_octetstring(pdu_ptr->packlet);
      if (pdu_ptr->type == TRAP_TYPE) {
         free_oid(pdu_ptr->u.trappdu.enterprise);
         free_octetstring(pdu_ptr->u.trappdu.agent_addr);
      }
      free_varbind_list(vb_ptr);
      pdu_ptr->var_bind_list = NULL;
      pdu_ptr->var_bind_end_ptr = NULL;
      free(pdu_ptr);
      pdu_ptr = NULL;
   } /* end of for */
}

int print_ipxaddr(ipxAddr_t *p)
{
  printf(gettxt(":72", "net[%0.2x %0.2x %0.2x %0.2x]\n"), p->net[0], p->net[1],
	 p->net[2], p->net[3]);
  printf(gettxt(":73", "node[%0.2x %0.2x %0.2x %0.2x %0.2x %0.2x]\n"), p->node[0], p->node[1],
	 p->node[2], p->node[3], p->node[4], p->node[5]);
  printf(gettxt(":74", "sock[%0.2x %0.2x]\n"), p->sock[0], p->sock[1]);
}

#endif            /* NETWARE */


send_smux_trap(SMUX_Trap_PDU * pdu)
{
  int i;
  OctetString *community_ptr;
  Pdu *pdu_ptr;
  AuthHeader *auth_ptr;

  if (udp_service != 0)
    {
      struct sockaddr_in sin;
      struct t_unitdata unitdata;
      char hostname[40];
      unsigned long local_ip_addr;
      struct hostent *hp;
      char ip_addr_buff[12];

      gethostname(hostname, sizeof(hostname));
      local_ip_addr = inet_addr(hostname);
      if (local_ip_addr == -1) 
   {
     hp = gethostbyname(hostname);
     if (hp)
       bcopy(hp->h_addr, &local_ip_addr, hp->h_length);
     else
       {
         syslog(LOG_ERR, gettxt(":51", "%s: Host unknown.\n"), hostname);
         exit(1);
       }
   }
      local_ip_addr=ntohl(local_ip_addr); 
      sprintf(ip_addr_buff, "%02x %02x %02x %02x", ((local_ip_addr>>24)&0xFF),
         ((local_ip_addr>>16)&0xFF), ((local_ip_addr>>8)&0xFF),
              (local_ip_addr&0xFF));

      /* Compose the SNMP/IP trap pdu */
      pdu_ptr = make_pdu(TRAP_TYPE, 0L, 0L, 0L, 
          pdu->enterprise, 
          make_octet_from_hex((unsigned char *)ip_addr_buff),
          pdu->generic__trap, 
          pdu->specific__trap, 
          pdu->time__stamp);

      /* Gotta put some sort of pdu on end - NULL pdu not allowed */
      if (pdu->variable__bindings == NULL) 
   link_varbind(pdu_ptr,
           make_varbind(pdu->enterprise, NULL_TYPE, 0, 0, 
              NULL, NULL));
      else 
   link_varbind(pdu_ptr, pdu->variable__bindings);
   
      build_pdu(pdu_ptr);
      
      for (i = 0; i < num_trap_communities; i++) 
   {
     if (trap_communities[i].proto != IP_PROTO)
       continue;

     bzero((char *)&sin, sizeof(sin));
     sin.sin_addr.s_addr = trap_communities[i].ip_addr; 
     /* already net ordered */
      
#ifdef __NEW_SOCKADDR__
     sin.sin_len = sizeof(struct sockaddr_in);
#endif
     sin.sin_family = AF_INET;

     sin.sin_port = htons(trap_communities[i].remote_port);
     /* 
      * Make the AuthHeader object of your choice, copying the
      * the 'community' and inserting the previously made PDU
      */
     community_ptr = make_octet_from_text 
       ((unsigned char *)trap_communities[i].community_name);
     auth_ptr = make_authentication(community_ptr);
     community_ptr = NULL; /* clean up OS */
    
     /* make final packet */
     build_authentication(auth_ptr, pdu_ptr);
  
     /* You'd do your sendto here */ 
     unitdata.addr.buf = (char *) &sin;
     unitdata.addr.len = sizeof(sin);
     unitdata.opt.len = 0;
     unitdata.udata.buf = (char *) auth_ptr->packlet->octet_ptr;
     unitdata.udata.len = auth_ptr->packlet->length;
     if (t_sndudata(trap_fd, &unitdata) < 0) 
       {
         syslog(LOG_WARNING, 
           gettxt(":75", "t_sndudata (trap_fd) failed: %s.\n"), t_errmsg());
       
         free_authentication(auth_ptr); auth_ptr = NULL;
         free_pdu(pdu_ptr); pdu_ptr = NULL;
         return(-1);
       }

     snmpstat->outpkts++;
      
     /* clean up time */
     free_authentication(auth_ptr); 
   } /* end of for */
   free_octetstring(pdu_ptr->packlet);
   free(pdu_ptr);
      } /* end of if udp_service!=0 */

#ifdef NETWARE
  if (ipx_service != 0)
    {
      struct t_unitdata ud;
      unsigned char packet_type = IPX_PACKET_TYPE;
      
      /* Compose the SNMP/IP trap pdu */
      pdu_ptr = make_pdu(TRAP_TYPE, 0L, 0L, 0L, 
          pdu->enterprise, 
          make_octet_from_hex((unsigned char *)"00 00 00 00"), /* RFC1420 */
          pdu->generic__trap, 
          pdu->specific__trap, 
          pdu->time__stamp);
      /* Gotta put some sort of pdu on end - NULL pdu not allowed */
      if (pdu->variable__bindings == NULL) 
   link_varbind(pdu_ptr,
           make_varbind(pdu->enterprise, NULL_TYPE, 0, 0, 
              NULL, NULL));
      else 
   link_varbind(pdu_ptr, pdu->variable__bindings);
      
      build_pdu(pdu_ptr);
      
      for (i = 0; i < num_trap_communities; i++) 
   {
     if (trap_communities[i].proto != IPX_PROTO)  /* skip none ipx */
       continue;
     
          community_ptr = make_octet_from_text
               ((unsigned char *)trap_communities[i].community_name);
     auth_ptr = make_authentication(community_ptr);
     community_ptr=NULL;

          build_authentication(auth_ptr,pdu_ptr);

     ud.opt.len = sizeof(packet_type);
     ud.opt.maxlen = sizeof(packet_type);
     ud.opt.buf = (char *) &packet_type;

     ud.addr.len = sizeof(ipxAddr_t);
     ud.addr.maxlen = sizeof(ipxAddr_t);
     ud.addr.buf = (char *) &(trap_communities[i].ipx_addr);
     ud.udata.maxlen = IPX_MAX_DATA;
     ud.udata.len = auth_ptr->packlet->length;
     ud.udata.buf = (char *) auth_ptr->packlet->octet_ptr;
     
     if (t_sndudata(ipx_trap_fd, &ud) < 0) 
       {
         syslog(LOG_WARNING, gettxt(":71", "t_sndudata (ipx_trap_fd) failed: %s.\n"),
           t_errmsg());
         free_authentication(auth_ptr);
         free_pdu(pdu_ptr);
         pdu_ptr = NULL;
         
         return (-1);
       }
     snmpstat->outpkts++;
     
          /* clean up time */
     free_authentication(auth_ptr);
   } /* end of for */
      free_octetstring(pdu_ptr->packlet);
      free(pdu_ptr);
    } /* end of if ipx_server!=0 */
#endif /* NETWARE */
}

void exitsnmp(int sig)
{
  remove(SNMPD_PID_FILE);
  exit(0);
}


