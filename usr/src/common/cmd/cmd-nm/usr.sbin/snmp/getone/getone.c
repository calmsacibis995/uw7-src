#ident	"@(#)getone.c	1.3"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)getone.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)getone.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */
/*
 * Getone.c - program to send a get-request and process a get-response
 */

/*
getone.c

     Getone is an SNMP application to retrieve a set  of  indivi-
     dual  variables  from  an SNMP entity using a "GET" request.
     The arguments are the entity's address, the community string
     for access to the SNMP entity, and the fully qualified vari-
     able name(s)  expressed as either dot-notation or the  vari-
     able  name  as it appears in the MIB document.  It should be
     noted that since the function is Get, as opposed to  a  Get-
     Next,  the  variable MUST be fully qualified for the request
     to be successful.  For instance

     getone suzzy public sysDescr.0 ifNumber.0

     would return the variables sysDescr.0 and ifNumber.0

     but the call

     getone suzzy public system

     would return an error from the entity  since  it  is  not  a
     fully qualified SNMP variable.
*/

#include <locale.h>
#include <unistd.h>
#include "snmp.h"
#include "snmpio.h"

int main(int argc, char *argv[])
{
  int c;
  extern char *optarg;
  extern int optind;

  int errflg = 0;
  int timeout = SECS;

  OID oid_ptr;
  OctetString *community_ptr;
  VarBindList *vb_ptr; /* hold up to 30 of 'em */
  Pdu *pdu_ptr, *in_pdu_ptr;
  AuthHeader *auth_ptr, *in_auth_ptr;
  int num_requests;
  int i;
  long req_id;
  int cc;

  (void)setlocale(LC_ALL, "");
  (void)setcat("nmgetone");
  (void)setlabel("NM:getone");

  while((c = getopt(argc, argv, "f:T:")) != EOF )
    {
      switch(c)
	{
	case 'f':
	  /* Merge defns file into MIB */
          if( merge_mib_from_file(optarg) == -1 )
		/* It failed ... flag error */
		errflg++;	
	  break;

	case 'T':
	  timeout = atoi(optarg);
	  break;

	case '?':
	  errflg++;
	}
    }

  if(((argc - optind) < 3) || (errflg > 0))
    {
      fprintf(stderr, gettxt(":1", "Usage: %s [-f defns_file] [-T timeout] entity_addr community_string object_name...\n"), argv[0]);
      exit(-1);
    }

  /* do whatever initializations and opens are necessary */
  initialize_io(argv[0], argv[optind]);

  req_id = make_req_id(); /* initialize with a pseudo-random value (time) */

  /* start a PDU */
  pdu_ptr = make_pdu(GET_REQUEST_TYPE, ++req_id, 0L, 0L, NULL, NULL, 
		     0L, 0L, 0L);

  num_requests = argc - optind - 2;

  for (i = 0; i < num_requests; i++) 
    {
      oid_ptr = make_obj_id_from_dot((unsigned char *)argv[i + optind + 2]);

      if (oid_ptr == NULL) 
	{
	  fprintf(stderr, 
		  gettxt(":2", "Cannot translate variable class: %s\n"), 
		  argv[i + optind + 2]);
	  exit(-1);
	}
  
      vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
      oid_ptr = NULL;

      /* COPY VarBind into PDU */
      link_varbind(pdu_ptr, vb_ptr);
      vb_ptr = NULL;
    }

  /* Make the PDU packlet */
  build_pdu(pdu_ptr);

  /* 
   * Make the AuthHeader object of your choice, copying the
   * the 'community' and inserting the previously made PDU
   */
  community_ptr = make_octet_from_text((unsigned char *)argv[optind + 1]);
  auth_ptr = make_authentication(community_ptr);

  /* make final packet */
  build_authentication(auth_ptr, pdu_ptr);


  /*  packet is built ... get it ready to send and send it */

  cc = send_request(fd, auth_ptr);
  if (cc != TRUE) 
    {
      /* couldn't send */
      free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
      auth_ptr = NULL;
      free_pdu(pdu_ptr);  /* does blast PDU and everything under it */
      pdu_ptr = NULL;
      close_up(fd);
      exit(-1);
    }

  /* clean up time */
  free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
  auth_ptr = NULL;
  free_pdu(pdu_ptr);  /* does blast PDU and everything under it */
  pdu_ptr = NULL;

  /*
   * By this point, we should have reclaimed all memory dynamically allocated
   * for the packet building.
   */

  /*
   * Now we wait to receive
   */

  cc = get_response(timeout);

  /* exit [gracefully] if a timeout */

  if (cc == TIMEOUT) 
    {
      fprintf(stderr, 
	      gettxt(":3", "%s: No response. Possible invalid argument. Try again.\n"), 
	      imagename);
      close_up(fd);
      exit(-1);
    }
  if (cc == ERROR) 
    {
      fprintf(stderr, gettxt(":4", "%s: Received an error.\n"), imagename);
      close_up(fd);
      exit(-1);
    }

  /* Now parse the response */
  if ((in_auth_ptr = parse_authentication(packet, packet_len)) == NULL) 
    {
      fprintf(stderr, gettxt(":5", "%s: Error parsing packet.\n"), imagename);
      exit(-1);
    }

  if ((in_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) 
    {
      fprintf(stderr, gettxt(":6", "%s: Error parsing pdu packlet.\n"), 
	      imagename);
      exit(-1);
    }

  if (in_pdu_ptr->type != GET_RESPONSE_TYPE) 
    {
      fprintf(stderr, 
	      gettxt(":7", "%s: Received non-GET_RESPONSE_TYPE packet. Exiting.\n"),
	      imagename);
      free_authentication(in_auth_ptr);
      in_auth_ptr = NULL;
      free_pdu(in_pdu_ptr);
      in_pdu_ptr = NULL;
      exit(-1);
    }
  snmpstat->ingetresponses++;

  /* check for error status stuff... */
  if (in_pdu_ptr->u.normpdu.error_status != NO_ERROR) 
    {
      fprintf(stderr, gettxt(":8", "Error code set in packet - "));

      switch ((short) in_pdu_ptr->u.normpdu.error_status) 
	{
	case TOO_BIG_ERROR:
	  fprintf(stderr, gettxt(":9", "Return packet too big.\n"));
	  snmpstat->intoobigs++;
	  break;

	case NO_SUCH_NAME_ERROR:
	  fprintf(stderr,
		  gettxt(":10", "No such variable name. Index: %ld.\n"), 
		  in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->innosuchnames++;
	  break;
	  
	case BAD_VALUE_ERROR:
	  fprintf(stderr, gettxt(":11", "Bad variable value. Index: %ld.\n"), 
		  in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->inbadvalues++;
	  break;

	case READ_ONLY_ERROR:
	  fprintf(stderr, gettxt(":12", "Read only variable. Index: %ld.\n"), 
		  in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->inreadonlys++;
	  break;

	case GEN_ERROR:
	  fprintf(stderr, gettxt(":13", "General error. Index: %ld.\n"),
		  in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->ingenerrs++;
	  break;

	default:
	  fprintf(stderr, gettxt(":14", "Unknown status code. %ld.\n"),
		  in_pdu_ptr->u.normpdu.error_status);
	  break;
	}
      exit (-1);
    }
  else 
    {
      /* Check the request-id */
      if (in_pdu_ptr->u.normpdu.request_id != req_id)
	fprintf(stderr, 
		gettxt(":15", "Request ID mismatch. Got: %ld, Expected: %ld.\n"),
		in_pdu_ptr->u.normpdu.request_id, req_id);

      print_varbind_list(in_pdu_ptr->var_bind_list);
    }
  free_authentication(in_auth_ptr); in_auth_ptr = NULL;
  free_pdu(in_pdu_ptr); in_pdu_ptr = NULL;
  close_up(fd);
  return(0);
}
