#ident	"@(#)getmany.c	1.3"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)getmany.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)getmany.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 * Getmany.c - program to send get-next-requests and process get-responses
 * until the entire variable class entered is traversed
 */


/*
getmany.c

     Getmany is an SNMP application to retrieve classes of  vari-
     ables  from  an SNMP entity.  The arguments are the entity's
     address, the community string access to the SNMP entity, and
     the  variable  class name(s) is expressed as object identif-
     iers in either dot-notation or as the mib-variables as  they
     appear  in the MIB document.  Getmany retrieves the variable
     class by calling the SNMP entity  with  the  variable  class
     name  to  get the first variable in the class, and then cal-
     ling the entity again using the variable  name  returned  in
     the  previous  call  to  retrieve  the  next variable in the
     class, utilizing the get-next aspect of the variable  retre-
     vial system.  For instance, running the following:

     getmany suzzy public ipRouteDest

     will traverse the gateway's ipRouteDest variable class  (the
     next  gateway  traveling  to in the route for the given net-
     number, which makes up the rest of the  variable  name)  The
     traversing  of  the  variable  space  stops  when all of the
     classes being polled return a variable of a class  different
     than what was requested.  Note that a gateway's entire vari-
     able tree can be traversed with a call of

     getmany suzzy public iso
*/

#include <locale.h>
#include <unistd.h>

#include "snmpio.h"

int chk_oid(OID oid1_ptr, OID oid2_ptr);

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
  int num_requests;
  int i;
  long req_id;
  int cc;
  OID init_oid_ptr;

  (void)setlocale(LC_ALL, "");
  (void)setcat("nmgetmany");
  (void)setlabel("NM:getmany");

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
      fprintf(stderr, 
	      gettxt(":1", "Usage: %s [-f defns_file] [-T timeout] entity_addr community_string object_name...\n"), 
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

  num_requests = argc - optind - 2;
  init_oid_ptr = make_obj_id_from_dot((unsigned char *)argv[optind + 2]);


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
      link_varbind(resp_pdu_ptr, vb_ptr);
      vb_ptr = NULL;
    }

  /* Make the PDU packlet */
  build_pdu(resp_pdu_ptr);
  
  /* we now have a ficticious response pdu as if it came from parsing a 
     response */
  
  while (1) 
    {
      /*make a new request pdu using the fields from the current response pdu*/

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
	{ 
	  /*not timed out */
	  /*free the dynamic memory allocated for the previous response */
	  /* packet since we didn't timeout, we wont need it any longer */
	  free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
	  auth_ptr = NULL;
	  free_pdu(resp_pdu_ptr);  /* does blast PDU and everything under it */
	  resp_pdu_ptr = NULL;

	  /* Now parse the response */
	  if ((in_auth_ptr = parse_authentication(packet, packet_len)) == NULL)
	    {
	      fprintf(stderr, gettxt(":5", "%s: Error parsing packet.\n"), 
		      imagename);
	      exit(-1);
	    }
      
	  if ((resp_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) 
	    {
	      fprintf(stderr, gettxt(":6", "%s: Error parsing pdu packlet.\n"),
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
		  fprintf(stderr,gettxt(":15", "End of MIB.\n"));
		  free_authentication(in_auth_ptr);
		  in_auth_ptr = NULL;
		  free_pdu(resp_pdu_ptr);
		  resp_pdu_ptr = NULL;
		  snmpstat->innosuchnames++;
		  exit(0);
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
		  fprintf(stderr, 
			  gettxt(":12", "General error. Index: %ld.\n"),
			  resp_pdu_ptr->u.normpdu.error_index);
		  snmpstat->ingenerrs++;
		  break;

		default:
		  fprintf(stderr,
			  gettxt(":13", "Unknown status code. %ld.\n"),
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
	      /* Check the request-id now */
	      if (resp_pdu_ptr->u.normpdu.request_id != req_id) 
		fprintf(stderr, 
			gettxt(":14", "Request ID mismatch. Got: %ld, Expected: %ld.\n"),
			resp_pdu_ptr->u.normpdu.request_id, req_id); 
      
	      /*Check for termination case (only checking first one for now) */
	      if (chk_oid(init_oid_ptr, 
			  resp_pdu_ptr->var_bind_list->vb_ptr->name) < 0) 
		{
		  free_authentication(in_auth_ptr);
		  in_auth_ptr = NULL;
		  free_pdu(resp_pdu_ptr);
		  resp_pdu_ptr = NULL;
		  exit(0);
		}
      
	      print_varbind_list(resp_pdu_ptr->var_bind_list); 

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

  for (i=0; i < oid1_ptr->length; i++) {
    if (oid1_ptr->oid_ptr[i] < oid2_ptr->oid_ptr[i])
      return(-1);
  }
    return(0);
}
