#ident	"@(#)getroute.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)getroute.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)getroute.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */
/*
 * getroute.c - a program to retreive and print a remote routing table
 */
/*
getroute.c

     Getroute is  an  SNMP  application  that  retrieves  routing
     information  from  an  entity by traversing the ipRouteDest,
     ipRouteIfIndex, ipRouteMetric1, ipRouteNextHop, ipRouteType,
     and  ipRouteProto variable classes for each route found.  It
     takes as arguments the address of the SNMP entity and a com-
     munity string to provide access to that entity.
*/

#include <locale.h>
#include <unistd.h>

#include "snmpio.h"
#include <arpa/inet.h>

int
chk_oid(OID oid1_ptr,
	OID oid2_ptr);

void print_route_info(VarBindList *vb_list_ptr);

int main(int argc, char *argv[])
{
  int c;
  extern char *optarg;
  extern int optind;

  int errflg = 0;
  int timeout = SECS;

  OID oid_ptr;
  OctetString *community_ptr;
  VarBindList *vb_ptr, *temp_vb_ptr; 
  Pdu *req_pdu_ptr, *resp_pdu_ptr;
  AuthHeader *auth_ptr, *in_auth_ptr;
  long req_id;
  int cc;
  OID init_oid_ptr;

  (void)setlocale(LC_ALL, "");
  (void)setcat("nmgetroute");
  (void)setlabel("NM:getroute");

  while((c = getopt(argc, argv, "T:")) != EOF )
    {
      switch(c)
	{
	case 'T':
	  timeout = atoi(optarg);
	  break;
	  
	case '?':
	  errflg++;
	}
    }

  if(((argc - optind) < 2) || (errflg == 1))
    {
      fprintf(stderr, 
	      gettxt(":1", "Usage: %s [-T timeout] entity_addr community_string\n"), 
	      argv[0]);
      exit(-1);
    }

  /* do whatever initializations and opens are necessary */
  initialize_io(argv[0], argv[optind]);

  req_id = make_req_id(); /* initialize with a pseudo-random value (time) */

  /* start a dummy response PDU */
  /* we will throw this one away ... this one is to jump-start the loop */
  resp_pdu_ptr = make_pdu(GET_NEXT_REQUEST_TYPE, req_id, 0L, 0L,
			  NULL, NULL, 0L, 0L, 0L);

  init_oid_ptr = make_obj_id_from_dot((unsigned char *)"ipRouteDest");

  /* Flesh out packet */
  if ((oid_ptr = 
       make_obj_id_from_dot((unsigned char *)"ipRouteDest")) == NULL) 
    {
      fprintf(stderr, 
	      gettxt(":2", "Cannot translate variable class: %s\n"), 
	      "ipRouteDest");
      exit(-1);
    }

  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
  oid_ptr = NULL;
  link_varbind(resp_pdu_ptr, vb_ptr);
  vb_ptr = NULL;

  if ((oid_ptr = 
       make_obj_id_from_dot((unsigned char *)"ipRouteIfIndex")) == NULL) 
    {
      fprintf(stderr, 
	      gettxt(":2", "Cannot translate variable class: %s\n"), 
	      "ipRouteInIndex");
      exit(-1);
    }

  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
  oid_ptr = NULL;
  link_varbind(resp_pdu_ptr, vb_ptr);
  vb_ptr = NULL;

  if ((oid_ptr = 
       make_obj_id_from_dot((unsigned char *)"ipRouteMetric1")) == NULL) 
    {
      fprintf(stderr, 
	      gettxt(":2", "Cannot translate variable class: %s\n"), 
	      "ipRouteMetric1");
      exit(-1);
    }
  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
  oid_ptr = NULL;
  link_varbind(resp_pdu_ptr, vb_ptr);
  vb_ptr = NULL;

  if ((oid_ptr = 
       make_obj_id_from_dot((unsigned char *)"ipRouteNextHop")) == NULL) 
    {
      fprintf(stderr, 
	      gettxt(":2", "Cannot translate variable class: %s\n"), 
	      "ipRouteNextHop");
      exit(-1);
    }
  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
  oid_ptr = NULL;
  link_varbind(resp_pdu_ptr, vb_ptr);
  vb_ptr = NULL;

  if ((oid_ptr = 
       make_obj_id_from_dot((unsigned char *)"ipRouteType")) == NULL) 
    {
      fprintf(stderr, 
	      gettxt(":2", "Cannot translate variable class: %s\n"), 
	      "ipRouteType");
      exit(-1);
    }
  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
  oid_ptr = NULL;
  link_varbind(resp_pdu_ptr, vb_ptr);
  vb_ptr = NULL;

  if ((oid_ptr = 
       make_obj_id_from_dot((unsigned char *)"ipRouteProto")) == NULL) 
    {
      fprintf(stderr, 
	      gettxt(":2", "Cannot translate variable class: %s\n"), 
	      "ipRouteProto");
      exit(-1);
    }
  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
  oid_ptr = NULL;
  link_varbind(resp_pdu_ptr, vb_ptr);
  vb_ptr = NULL;

  /* Make the PDU packlet */
  build_pdu(resp_pdu_ptr);

  while (1) 
    {
      /*  make a new request pdu using the fields from the current response pdu */
      req_pdu_ptr = make_pdu(GET_NEXT_REQUEST_TYPE, ++req_id, 0L, 0L,
			     NULL, NULL, 0L, 0L, 0L);

      /*  point to the old (response pdu) */
      temp_vb_ptr = resp_pdu_ptr->var_bind_list;

      /* copy the OID fields from the old pdu to the new */
      /* value fields of the varbind are set to NULL on the request */
      while(temp_vb_ptr != NULL) 
	{
	  oid_ptr = make_oid(temp_vb_ptr->vb_ptr->name->oid_ptr, 
			     temp_vb_ptr->vb_ptr->name->length);
	  vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
	  oid_ptr = NULL;
	  /* link it into the varbind of the pdu under construction */
	  link_varbind(req_pdu_ptr, vb_ptr);
	  vb_ptr = NULL;
	  /* repeat for all varbinds in varbindlist */
	  temp_vb_ptr = temp_vb_ptr->next;
	} /* while(temp... */

      /* Make the PDU packlet */
      build_pdu(req_pdu_ptr);

      /*
       * Make the AuthHeader object of your choice, copying the
       * the 'community' and inserting the newly made request PDU
       */
      community_ptr = make_octet_from_text((unsigned char *)argv[optind + 1]);
      auth_ptr = make_authentication(community_ptr);

      /* make final request packet */
      build_authentication(auth_ptr, req_pdu_ptr);

      /*  packet is built ... get it ready to send and send it */

      cc = send_request(fd, auth_ptr);
      if (cc != TRUE) 
	{
	  /* couldn't send */
	  free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
	  auth_ptr = NULL;
	  free_pdu(req_pdu_ptr);  /* does blast PDU and everything under it */
	  req_pdu_ptr = NULL;
	  close_up(fd);
	  exit(-1);
	} /* if (cc ... */

      /* clean up time */
      free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
      auth_ptr = NULL;
      free_pdu(req_pdu_ptr);  /* does blast PDU and everything under it */
      req_pdu_ptr = NULL;

      /*
       * By this point, we should have reclaimed all memory dynamically allocated
       * for the packet building.
       */
      
      /*
       * Now we wait to receive
       */

      cc = get_response(timeout);
      
      /* exit if error in receive routine */
      if (cc == ERROR) 
	{
	  fprintf(stderr, gettxt(":4", "%s: Received an error.\n"), imagename);
	  /* clean up time */
	  free_authentication(auth_ptr);
	  auth_ptr = NULL;
	  free_pdu(req_pdu_ptr);
	  req_pdu_ptr = NULL;
	  free_pdu(resp_pdu_ptr);
	  resp_pdu_ptr = NULL;
	  close_up(fd);
	  exit(-1);
	}
      
      /* retry [gracefully] if a timeout */
      if (cc == TIMEOUT) 
	{
	  fprintf(stderr, 
		  gettxt(":3", "%s: No response. Possible invalid argument. Try again.\n"), 
		  imagename);
	  close_up(fd);
	  exit(-1);
	}
      else 
	{ /*not timed out */

	  /*  free the dynamic memory allocated for the previous response 
	      packet since we didn't timeout, we wont need it any longer */

	  /* blows away packlet, etc, but NOT pdu */
	  free_authentication(auth_ptr); 
	  auth_ptr = NULL;

	  /* does blast PDU and everything under it */
	  free_pdu(resp_pdu_ptr);  
	  resp_pdu_ptr = NULL;

	  /* Now parse the response */
	  if ((in_auth_ptr = 
	       parse_authentication(packet, packet_len)) == NULL) 
	    {
	      fprintf(stderr, gettxt(":5", "%s: Error parsing packet.\n"),
		      imagename);
	      exit(-1);
	    }

	  if ((resp_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) 
	    {
	      fprintf(stderr, 
		      gettxt(":6", "%s: Error parsing pdu packlet.\n"), 
		      imagename);
	      free_authentication(in_auth_ptr);
	      in_auth_ptr = NULL;
	      free_pdu(resp_pdu_ptr);
	      resp_pdu_ptr = NULL;
	      exit(-1);
	    }

	  if (resp_pdu_ptr->type != GET_RESPONSE_TYPE) 
	    {
	      fprintf(stderr, 
		      gettxt(":7", "%s: Received non-GET_RESPONSE_TYPE packet. Exiting.\n"),
		      imagename);
	      free_authentication(in_auth_ptr);
	      in_auth_ptr = NULL;
	      free_pdu(resp_pdu_ptr);
	      resp_pdu_ptr = NULL;
	      exit(-1);
	    }

	  snmpstat->ingetresponses++;
	  
	  /* check for error status stuff... */
      if (resp_pdu_ptr->u.normpdu.error_status != NO_ERROR) 
	{
	  if (resp_pdu_ptr->u.normpdu.error_status == NO_SUCH_NAME_ERROR) 
	    {
	      fprintf(stderr, gettxt(":15", "End of MIB.\n"));
	      free_authentication(in_auth_ptr);
	      in_auth_ptr = NULL;
	      free_pdu(resp_pdu_ptr);
	      resp_pdu_ptr = NULL;
	      snmpstat->innosuchnames++;
	      exit(-1);
	    }
	  
	  fprintf(stderr, gettxt(":8", "Error code set in packet - "));

	  switch ((short) resp_pdu_ptr->u.normpdu.error_status) 
	    {
	    case TOO_BIG_ERROR:
	      fprintf(stderr, gettxt(":9", "Return packet too big.\n"));
	      snmpstat->intoobigs++;
	      break;

	    case BAD_VALUE_ERROR:
	      fprintf(stderr, 
		      gettxt(":10", "Bad variable value. Index: %ld.\n"), 
		      resp_pdu_ptr->u.normpdu.error_index);
	      snmpstat->inbadvalues++;
	      break;

	    case READ_ONLY_ERROR:
	      fprintf(stderr, 
		      gettxt(":11", "Read only variable. Index: %ld.\n"), 
		      resp_pdu_ptr->u.normpdu.error_index);
	      snmpstat->inreadonlys++;
	      break;

	    case GEN_ERROR:
	      fprintf(stderr, gettxt(":12", "General error. Index: %ld.\n"),
		      resp_pdu_ptr->u.normpdu.error_index);
	      snmpstat->ingenerrs++;
	      break;

	    default:
	      fprintf(stderr, gettxt(":13", "Unknown status code. %ld.\n"),
		      resp_pdu_ptr->u.normpdu.error_status);
	      break;
	    }

	  free_authentication(in_auth_ptr);
	  in_auth_ptr = NULL;
	  free_pdu(resp_pdu_ptr);
	  resp_pdu_ptr = NULL;
	  exit (-1);
	} /* if ! NOERROR (was an error) */
      else 
	{ /* no error was found */

	  if (resp_pdu_ptr->u.normpdu.request_id != req_id) 
	    fprintf(stderr, 
		    gettxt(":14", "Request ID mismatch. Got: %ld, Expected: %ld.\n"),
		    resp_pdu_ptr->u.normpdu.request_id, req_id);

	  /* Check for termination case (only checking first one for now) */
	  if (chk_oid(init_oid_ptr, 
		      resp_pdu_ptr->var_bind_list->vb_ptr->name) < 0) 
	    {
	      free_authentication(in_auth_ptr);
	      in_auth_ptr = NULL;
	      free_pdu(resp_pdu_ptr);
	      resp_pdu_ptr = NULL;
	      exit(0);
	    }

	  print_route_info(resp_pdu_ptr->var_bind_list); /* looky, looky */

	  /*  clean up dynamic memory */
	  free_authentication(in_auth_ptr);
	  in_auth_ptr = NULL;
	  /* but do NOT free resp_pdu_ptr ... needed above for forming
	   *  next request
	   */

	} /* end of else no error */
	} /* end of no timeout */
    } /* while (1) */
}

      
int
chk_oid(OID oid1_ptr,
	OID oid2_ptr)
{
  int i;
  
  for (i=0; i < oid1_ptr->length; i++) 
    if (oid1_ptr->oid_ptr[i] < oid2_ptr->oid_ptr[i])
      return(-1);
  return(0);
}

void
print_route_info(VarBindList *vb_list_ptr)
{
  VarBindList *vb_ptr;
#ifdef BSD
  struct in_addr dest_addr, next_addr;
#endif
#ifdef CMUPCIP
  unsigned long dest_addr, next_addr;
#endif
#ifdef FTP
  unsigned long dest_addr, next_addr;
#endif
#ifdef SVR3
  struct in_addr dest_addr, next_addr;
#endif
#ifdef SVR4
  struct in_addr dest_addr, next_addr;
#endif
  int if_num;
  int metric;
  int type;
  int proto;
  int i;

  /* ipRouteDest */
  vb_ptr = vb_list_ptr;
#ifdef BSD
  dest_addr.s_addr =
	 htonl((unsigned long) (vb_ptr->value.os_value->octet_ptr[0] << 24) +
	       (vb_ptr->value.os_value->octet_ptr[1] << 16) +
	       (vb_ptr->value.os_value->octet_ptr[2] << 8)  +
	       (vb_ptr->value.os_value->octet_ptr[3]));
#endif
#ifdef CMUPCIP
  dest_addr = 0;
  for (i=0; i<4; i++) {
	dest_addr = (256 * dest_addr) + vb_ptr->value.os_value->octet_ptr[3-i];
  }
#endif
#ifdef FTP
  dest_addr = 0;
  for (i=0; i<4; i++) {
	dest_addr = (256 * dest_addr) + vb_ptr->value.os_value->octet_ptr[i];
  }
#endif
#ifdef SVR3
  dest_addr.s_addr =
	 htonl((unsigned long) (vb_ptr->value.os_value->octet_ptr[0] << 24) +
	       (vb_ptr->value.os_value->octet_ptr[1] << 16) +
	       (vb_ptr->value.os_value->octet_ptr[2] << 8)  +
	       (vb_ptr->value.os_value->octet_ptr[3]));
#endif
#ifdef SVR4
  dest_addr.s_addr =
	 htonl((unsigned long) (vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8)  +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));
#endif

  vb_ptr = vb_ptr->next;
  if_num = (short) vb_ptr->vb_ptr->value->sl_value;

  vb_ptr = vb_ptr->next;
  metric = (short) vb_ptr->vb_ptr->value->sl_value;

  vb_ptr = vb_ptr->next;
#ifdef BSD
  next_addr.s_addr =
	 htonl((unsigned long) (vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));
#endif
#ifdef CMUPCIP
  next_addr = 0;
  for (i=0; i<4; i++) {
	next_addr = (256 * next_addr) + vb_ptr->vb_ptr->value->os_value->octet_ptr[3-i];
  }
#endif  
#ifdef FTP
  next_addr = 0;
  for (i=0; i<4; i++) {
	next_addr = (256 * next_addr) + vb_ptr->vb_ptr->value->os_value->octet_ptr[i];
  }
#endif  
#ifdef SVR3
  next_addr.s_addr =
	 htonl((unsigned long) (vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));
#endif
#ifdef SVR4
  next_addr.s_addr =
	 htonl((unsigned long) (vb_ptr->vb_ptr->value->os_value->octet_ptr[0] << 24) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[1] << 16) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[2] << 8) +
	       (vb_ptr->vb_ptr->value->os_value->octet_ptr[3]));
#endif

  vb_ptr = vb_ptr->next;
  type = (short) vb_ptr->vb_ptr->value->sl_value;

  vb_ptr = vb_ptr->next;
  proto = (short) vb_ptr->vb_ptr->value->sl_value;

#ifdef BSD
  printf(gettxt(":16", "Route to: %s "), 
         inet_ntoa(dest_addr));
  printf(gettxt(":18", "via: %s  on if: %d metric: %d type: %d proto: %d.\n"),
	      inet_ntoa(next_addr), if_num, metric, type, proto);
#endif
#ifdef FTP
  printf(gettxt(":16", "Route to: %s "), 
         inet_ntoa(htonl(dest_addr)));
  printf(gettxt(":18", "via: %s  on if: %d metric: %d type: %d proto: %d.\n"),
         inet_ntoa(htonl(next_addr)), if_num, metric, type, proto);
#endif
#ifdef CMUPCIP
  printf(gettxt(":17", "Route to: %a "), 
         dest_addr);
  printf(gettxt(":19", "via: %a  on if: %d metric: %d type: %d proto: %d.\n"),
         next_addr, if_num, metric, type, proto);
#endif
#ifdef SVR3
  printf(gettxt(":16", "Route to: %s "), 
         inet_ntoa(dest_addr));
  printf(gettxt(":18", "via: %s  on if: %d metric: %d type: %d proto: %d.\n"),
	      inet_ntoa(next_addr), if_num, metric, type, proto);
#endif
#ifdef SVR4
  printf(gettxt(":16", "Route to: %s "), 
         inet_ntoa(dest_addr));
  printf(gettxt(":18", "via: %s  on if: %d metric: %d type: %d proto: %d.\n"),
	      inet_ntoa(next_addr), if_num, metric, type, proto);
#endif
}
