#ident	"@(#)trap_rece.c	1.3"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)trap_rece.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)trap_rece.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
trap_rece.c

     Trap_rece is a program to receive  traps  from  remote  SNMP
     trap  generating  entities.  It  binds to the SNMP trap port
     (udp/162) to listen for the traps and thus must  be  run  as
     root.   It  prints to standard output messages corresponding
     to the traps it has received.  The primary purpose  of  this
     program  is  to  demonstrate  how traps are parsed using the
     SNMP library.
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

#define FALSE 0
#define TRUE  1


int main(int argc, char *argv[])
{
  Pdu *in_pdu_ptr;
  AuthHeader *in_auth_ptr;
  unsigned char packet[2048]; /* to play with packet after creation */
  int packet_len;
  struct servent *SimpleServ;
  struct sockaddr_in sin;
  struct sockaddr_in from;
#ifdef BSD
  int fromlen = sizeof(from);
#endif
#if defined(SVR3) || defined(SVR4)
  struct t_unitdata unitdata;
  int flags;
  struct t_bind *req, *ret;
#endif
  int fd; /* network file descriptor */
  int cc;
  int i;
  int debug;
  char buffer[255];

   (void)setlocale(LC_ALL, "");
   (void)setcat("nmtrap");
   (void)setlabel("NM:trap");

  if (argc > 2) {
    fprintf(stderr, gettxt(":4", "Usage: trap_rece [-d]\n"));
    exit(1);
  }
  if (argc == 2) 
    {
      if (strcmp(argv[1],"-d") == 0)
	debug = TRUE;
      else 
	{
	  fprintf(stderr, gettxt(":4", "Usage: trap_rece [-d]\n"));
	  exit(1);
	}
    }
  else  /* argc == 1 */
    debug = FALSE;

  if ((SimpleServ = getservbyname("snmp-trap","udp")) == NULL) {
    fprintf(stderr, gettxt(":5", "trap_rece: Add snmp-trap 162/udp to /etc/services\n"));
    return(-1);
  }

  /* attach to the in.snmpd shared memory */
  {
    int shmid;
    shmid=shmget(SNMPD_SHMKEY, sizeof(snmpstat_type),0);
    snmpstat=(snmpstat_type *)shmat(shmid,0,0);
    if (snmpstat==(snmpstat_type *)(-1))
      snmpstat=(snmpstat_type *)malloc(sizeof(snmpstat_type));
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
#ifdef BSD
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0, 0)) < 0) {
    perror("trap_rece:  port");
    exit(1);
  }
  for (i=0; ((bind(fd,&sin, sizeof(sin), 0) < 0) && ( i < 5)); i++) {
    perror("trap_rece:  bind");
    sleep(3);
  }
#endif
#if defined(SVR3) || defined(SVR4)
#ifdef SVR3
  if ((fd = t_open("/dev/inet/udp", O_RDWR, (struct t_info *)0)) < 0) {
#else
  if ((fd = t_open("/dev/udp", O_RDWR, (struct t_info *)0)) < 0) {
#endif
    t_error("trap_rece: t_open");
    return(-1);
  }
  if ((req = (struct t_bind *) t_alloc(fd, T_BIND, 0)) == NULL) {
    fprintf(stderr, gettxt(":6", "trap_rece: Couldn't allocate bind request.\n"));
    t_close(fd);
    return(-1);
  }
  if ((ret = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
    fprintf(stderr, gettxt(":7", "trap_rece: Couldn't allocate bind response.\n"));
    t_free((char *)req, T_BIND);
    t_close(fd);
    return(-1);
  }
  req->addr.buf = (char *) &sin;
  req->addr.len = sizeof(sin);
  req->qlen = 0;
  if (t_bind(fd, req, ret) < 0) {
    t_error("trap_rece: t_bind");
    req->addr.buf = (char *) 0;
    t_free((char *)req, T_BIND);
    t_free((char *)ret, T_BIND);
    t_close(fd);
    return(-1);
  }
  if (memcmp(req->addr.buf, ret->addr.buf, req->addr.len) != 0) {
    fprintf(stderr, gettxt(":8", "trap_rece: Couldn't assign requested address.\n"));
    req->addr.buf = (char *) 0;
    t_free((char *)req, T_BIND);
    t_free((char *)ret, T_BIND);
    t_close(fd);
    return(-1);
  }
  req->addr.buf = (char *) 0;
  t_free((char *)req, T_BIND);
  t_free((char *)ret, T_BIND);
#endif

  while (1) {
#ifdef BSD
    packet_len = recvfrom(fd, packet, sizeof(packet), 0, &from, &fromlen);
#endif
#if defined(SVR3) || defined(SVR4)
    unitdata.addr.buf = (char *) &from;
    unitdata.addr.maxlen = sizeof(from);
    unitdata.opt.maxlen = 0;
    unitdata.udata.buf = (char *) packet;
    unitdata.udata.maxlen = sizeof(packet);
    if (t_rcvudata(fd, &unitdata, &flags) < 0) {
      t_error("trap_rece: t_rcvudata");
      t_close(fd);
      return(-1);
    }
    packet_len = unitdata.udata.len;
#endif
    snmpstat->inpkts++;

    if (debug)
      print_packet_out(packet, packet_len); 
	
    /* Now parse the puppy */
    if ((in_auth_ptr = parse_authentication(packet, packet_len)) == NULL) {
      fprintf(stderr, gettxt(":9", "trap_rece: Error parsing packet.\n"));
      exit(-1);
    }
    
    if ((in_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) {
      fprintf(stderr, gettxt(":10", "trap_rece: Error parsing pdu packlet.\n"));
      exit(-1);
    }
    /* See if right type of pdu... */
    if (in_pdu_ptr->type != TRAP_TYPE) {
      printf(gettxt(":11", "Received non-trap message on snmp-trap port!!!\n"));
      printf(gettxt(":12", "From: %s.\n"), inet_ntoa(from.sin_addr));
      print_packet_out(packet, packet_len);
    }
    else {
      snmpstat->intraps++;
      /* Now process trap message */
      if ((cc = make_dot_from_obj_id(in_pdu_ptr->u.trappdu.enterprise, buffer)) != -1) {
	printf(gettxt(":13", "Community: %*.*s.\n"), in_auth_ptr->community->length,
	      in_auth_ptr->community->length,
	      in_auth_ptr->community->octet_ptr);
	      printf(gettxt(":14", "Enterprise: %s.\n"), buffer);
	      printf(gettxt(":15", "Agent-addr: %d.%d.%d.%d.\n"),
	      in_pdu_ptr->u.trappdu.agent_addr->octet_ptr[0],
	      in_pdu_ptr->u.trappdu.agent_addr->octet_ptr[1],
	      in_pdu_ptr->u.trappdu.agent_addr->octet_ptr[2],
	      in_pdu_ptr->u.trappdu.agent_addr->octet_ptr[3]);
	switch (in_pdu_ptr->u.trappdu.generic_trap) {
	case 0:
	  printf(gettxt(":16", "Cold start trap.\n"));
	  break;
	case 1:
	  printf(gettxt(":17", "Warm start trap.\n"));
	  break;
	case 2:
	  printf(gettxt(":18", "Link down trap.\n"));
	  break;
	case 3:
	  printf(gettxt(":19", "Link up trap.\n"));
	  break;
	case 4:
	  printf(gettxt(":20", "Authentication failure trap.\n"));
	  break;
	case 5:
	  printf(gettxt(":21", "EGP Neighbor Loss trap.\n"));
	  break;
	case 6:
	  printf(gettxt(":22", "Enterprise Specific trap: %d.\n"), in_pdu_ptr->u.trappdu.specific_trap);
	  break;
	default:
	  printf(gettxt(":23", "Unknown trap: %d.\n"), in_pdu_ptr->u.trappdu.generic_trap);
	  break;
	};
	
	printf(gettxt(":24", "Time Ticks: %d.\n"), in_pdu_ptr->u.trappdu.time_ticks);

	print_varbind_list(in_pdu_ptr->var_bind_list);
      }
      else
	printf(gettxt(":25", "trap_rece:, in_pdu_ptr->u.trappdu.enterprise:\n"));
    }
    fflush(stdout);

    /* Now free up resources */
    free_pdu(in_pdu_ptr);
    free_authentication(in_auth_ptr);
    
  } /* end of while */
}
