#ident	"@(#)trap_send.c	1.3"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)trap_send.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
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
static char SNMPID[] = "@(#)trap_send.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
trap_send.c

     Trap_send is a program to send trap messages to  trap  moni-
     toring  stations.   It  takes as arguments the IP address of
     the  monitoring  station (or its nodename) and  the  integer   
     number that corresponds to the trap to be sent.
*/

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#if defined(SVR3) || defined(SVR4)
#include <fcntl.h>
#endif

#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#include <sys/tiuser.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>      
#include <netdb.h>
#include <locale.h>
#include <unistd.h>

#include "snmp.h"
#include "snmpuser.h"

int main(int argc, char *argv[])
{
  OID oid_ptr;
  OctetString *os_ptr;
  VarBindList *vb_ptr;
  OctetString *session_ptr;
  Pdu *pdu_ptr;
  AuthHeader *auth_ptr;
  struct hostent *hp;
  struct servent *SimpleServ;
  struct sockaddr_in sin;
#if defined(SVR3) || defined(SVR4)
  struct t_unitdata unitdata;
#endif
  int fd; /* network file descriptor */
  long trap_type;
  char hostname[40];
  unsigned long local_ip_addr;
  char buffer[80];


  (void)setlocale(LC_ALL, "");
  (void)setcat("nmtrap");
  (void)setlabel("NM:trap");

  if (argc != 4) 
    {
      fprintf(stderr, 
	      gettxt(":1", 
		     "Usage: trap_send dest_addr \042community\042 type\n"));
      exit(1);
    }

  /* attach to the in.snmpd shared memory */
  {
    int shmid;
    shmid=shmget(SNMPD_SHMKEY, sizeof(snmpstat_type),0);
    snmpstat=(snmpstat_type *)shmat(shmid,0,0);
    if (snmpstat==(snmpstat_type *)(-1))
      snmpstat=(snmpstat_type *)malloc(sizeof(snmpstat_type));
  }

  if ((SimpleServ = getservbyname("snmp-trap","udp")) == NULL) 
    {
      fprintf(stderr, 
	      gettxt(":2", 
		     "trap_send: Add snmp-trap 162/udp to /etc/services\n"));
      return(-1);
    }

#ifdef BSD
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0, 0)) < 0) {
    perror("trap_send:  socket");
    return(-1);
  }
#endif
#if defined(SVR3) || defined(SVR4)
#ifdef SVR3
  if ((fd = t_open("/dev/inet/udp", O_RDWR, (struct t_info *)0)) < 0) {
#else
  if ((fd = t_open("/dev/udp", O_RDWR, (struct t_info *)0)) < 0) 
    {
#endif
      t_error("trap_send: t_open");
      return(-1);
    }
  if (t_bind(fd, (struct t_bind *)0, (struct t_bind *)0) < 0) 
    {
      t_error("trap_send: t_bind");
      t_close(fd);
      return(-1);
    }
#endif

  sin.sin_addr.s_addr = inet_addr(argv[1]);
  if (sin.sin_addr.s_addr == -1) 
    {
      hp = gethostbyname(argv[1]);
      if (hp) 
	memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
      else 
	{
	  printf(gettxt(":3", "%s: Host unknown.\n"), argv[1]);
	  exit(1);
	}
    }

#ifdef __NEW_SOCKADDR__
  sin.sin_len = sizeof(struct sockaddr_in);
#endif
#ifdef BSD
  sin.sin_family = AF_INET;
#endif
#if defined(SVR3) || defined(SVR4)
  sin.sin_family = AF_INET;
#endif
  sin.sin_port = SimpleServ->s_port;

  /* start a PDU */
  /* get sysObjectID */
  {
    FILE *pfp;
    char cmdstr[1024];
    char output[1024];
   
    sprintf(cmdstr, "/bin/grep -i objid %s", SNMPD_CONF_FILE);
    pfp=popen(cmdstr, "r");
    fgets(output, 1024, pfp);
    pclose(pfp);
    output[strlen(output)-1]=NULL; /* get rid of the line-feed */
    oid_ptr = make_obj_id_from_dot((unsigned char *)&(output[6]));
  }

  gethostname(hostname, sizeof(hostname));
  local_ip_addr = inet_addr(hostname);
  if (local_ip_addr == -1) 
    {
      hp=gethostbyname(hostname);
      if (hp)
	memcpy(&local_ip_addr, hp->h_addr, hp->h_length);
      else
	{
	  printf(gettxt(":3", "%s: Host unknown.\n"), hostname);
	  exit(1);
	}
    }
  local_ip_addr = ntohl(local_ip_addr);
  
  /* convert ip addr to os_ptr */
  sprintf(buffer,"%02x %02x %02x %02x", ((local_ip_addr>>24)&0xFF),
	  ((local_ip_addr>>16)&0xFF),((local_ip_addr>>8)&0xFF),(local_ip_addr&0xFF));
  os_ptr = make_octet_from_hex((unsigned char *)buffer);
  sscanf(argv[3], "%d", &trap_type);

  pdu_ptr = make_pdu(TRAP_TYPE, 0L, 0L, 0L, oid_ptr, os_ptr, trap_type, 0L, 0L);

  oid_ptr = NULL;
  os_ptr = NULL;

  /* Gotta put some sort of pdu on end - NULL is not option for pdu */
  oid_ptr = make_obj_id_from_dot((unsigned char *)"1.1.1.1");
  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0, 0, NULL, NULL);
  oid_ptr = NULL;
  link_varbind(pdu_ptr, vb_ptr);

  build_pdu(pdu_ptr);
  
  /* 
   * Make the AuthHeader object of your choice, copying the
   * the 'community' and inserting the previously made PDU
   */
  session_ptr = make_octet_from_text((unsigned char *)argv[2]);
  auth_ptr = make_authentication(session_ptr);
  session_ptr = NULL; /* clean up OS */
  
  /* make final packet */
  build_authentication(auth_ptr, pdu_ptr);
  
  /* You'd do your sendto here */ 
#ifdef BSD
  if (sendto(fd, auth_ptr->packlet->octet_ptr, auth_ptr->packlet->length, 
	     0, &sin, sizeof(sin)) <0) 
    {
      perror("trap_send:  send");
      close(fd);
      return(-1);
    }
#endif
#if defined(SVR3) || defined(SVR4)
  unitdata.addr.buf = (char *) &sin;
  unitdata.addr.len = sizeof(sin);
  unitdata.opt.len = 0;
  unitdata.udata.buf = (char *) auth_ptr->packlet->octet_ptr;
  unitdata.udata.len = auth_ptr->packlet->length;
  if (t_sndudata(fd, &unitdata) < 0) 
    {
      t_error("trap_send: t_sndudata");
      t_close(fd);
      return(-1);
    }
#endif
  
  snmpstat->outpkts++;
  /* clean up time */
  free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
  auth_ptr = NULL;
  free_pdu(pdu_ptr);  /* does blast PDU and everything under it */
  pdu_ptr = NULL;
  return (0);
}  


