#ident	"@(#)setany.c	1.3"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)setany.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)setany.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
setany.c

     Setany does a get request to get the current values  of  the
     variables  to  be  set,  then  performs a set request on the
     variables.   The arguments are the agent address in internet
     dot  notation,  the  community  name  for access to the SNMP
     entity, and for a triplet for each variable to be  set  con-
     sisting  of: the variable name in dot notation, an 'i', 'o',
     'd', 'a', 'c', 'g', or 't'  to  indicate  if  the  variables
     value  is being given as an integer, an octet string (in hex
     notation), an object identifier (in  dot  notation),  an  Ip
     address  (in  dot  notation),  a  counter, a gauge, or time-
     ticks, followed by the value.  For example:
     setany 128.169.1.1 suranet0 "ifAdminStatus.2" -i 3
     to set the adminstrative status of interface 2 to 3 (down).

     The actions that take place during a set request are that  a
     get request is issued for the variable. The set request uses
     this variable name for the set request.
*/

#include <locale.h>
#include <unistd.h>

#include "snmpio.h"

void usage(char *argv[]);
VarBindList *create_vb(char *argv[],
			char name[],
		       	char type[],
		       	char value[]);

int main(int argc, char *argv[])
{
  int c;
  extern char *optarg;
  extern int optind;
  int varind;

  int errflg = 0;
  int timeout = SECS;
 
  OctetString *community_ptr;
  VarBindList *vb_ptr; /* hold up to 30 of 'em */
  Pdu *out_pdu_ptr, *in_pdu_ptr;
  AuthHeader *auth_ptr, *in_auth_ptr;
  int num_args;
  int i;
  long req_id;
  int cc;

  (void)setlocale(LC_ALL, "");
  (void)setcat("nmsetany");
  (void)setlabel("NM:setany");

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

  if(((argc - optind) < 4) || (errflg > 0))
    {
      usage(argv);
    }

  /* do whatever initializations and opens are necessary */
  initialize_io(argv[0], argv[optind]);

  req_id = make_req_id(); /* initialize with a pseudo-random value (time) */

  /* start a PDU */
  out_pdu_ptr = make_pdu(SET_REQUEST_TYPE, ++req_id, 0L, 0L, NULL, NULL, 
		     0L, 0L, 0L);

  num_args = argc - optind - 2;

  if(num_args != ((num_args/3)*3)) 
    {
      fprintf(stderr, 
	      gettxt(":2", "Arguments must be in tuples of name <space> type <space> value.\n"));
      exit(-1);
    }

  varind = optind + 2;

  for (i = 0; i < num_args; i = i + 3) 
    {
      vb_ptr = create_vb(argv, argv[i + varind], 
			 argv[i + varind + 1], 
			 argv[i + varind + 2]);

      if (vb_ptr == NULL) 
	{
	  fprintf(stderr,
		  gettxt(":1", "Unable to create request from the given arguements.\n"));
	  usage(argv);
	}

      /* COPY VarBind into PDU */
      link_varbind(out_pdu_ptr, vb_ptr);
      vb_ptr = NULL;
    }

  /* Make the PDU packlet */
  build_pdu(out_pdu_ptr);

  /*  for debug ???
printf("\nSETANY: Ater build_pdu. LENGTH: %d", out_pdu_ptr->packlet->length);
print_packet_out(out_pdu_ptr->packlet->octet_ptr, out_pdu_ptr->packlet->length);
   */

  /* 
   * Make the AuthHeader object of your choice, copying the
   * the 'community' and inserting the previously made PDU
   */
  community_ptr = make_octet_from_text((unsigned char *)argv[optind + 1]);
  auth_ptr = make_authentication(community_ptr);

  /* make final packet */
  build_authentication(auth_ptr, out_pdu_ptr);

  /*  packet is built ... get it ready to send and send it */

  cc = send_request(fd, auth_ptr);
  if (cc != TRUE) 
    {
      /* couldn't send */
      fprintf(stderr, gettxt(":17", "%s: Unable to send.\n"), imagename);
      free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
      auth_ptr = NULL;
      free_pdu(out_pdu_ptr);  /* does blast PDU and everything under it */
      out_pdu_ptr = NULL;
      close_up(fd);
      exit(-1);
    }

  /* clean up time */
  free_authentication(auth_ptr); /* blows away packlet, etc, but NOT pdu */
  auth_ptr = NULL;
  free_pdu(out_pdu_ptr);  /* does blast PDU and everything under it */
  out_pdu_ptr = NULL;

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
      fprintf(stderr, gettxt(":4", "%s: No response. Possible invalid argument. Try again.\n"), imagename);
      close_up(fd);
      exit(-1);
    }
  
  if (cc == ERROR) 
    {
      fprintf(stderr, gettxt(":5", "%s: Received an error.\n"), imagename);
      close_up(fd);
      exit(-1);
    }

  /*  for debug
printf("\nSETANY: Ater get_response. LENGTH: %d", packet_len);
print_packet_out(packet, packet_len);
   */

  /* Now parse the response */
  if ((in_auth_ptr = parse_authentication(packet, packet_len)) == NULL) 
    {
      fprintf(stderr, gettxt(":6", "%s: Error parsing packet.\n"), imagename);
      exit(-1);
    }

  if ((in_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) 
    {
      fprintf(stderr, gettxt(":7", "%s: Error parsing pdu packlet.\n"), 
	      imagename);
      exit(-1);
    }

  if (in_pdu_ptr->type != GET_RESPONSE_TYPE) 
    {
      fprintf(stderr, 
	      gettxt(":8", "%s: Received non-GET_RESPONSE_TYPE packet. Exiting.\n"),
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
      fprintf(stderr, gettxt(":9", "Error code set in packet - "));

      switch ((short) in_pdu_ptr->u.normpdu.error_status) 
	{
	case TOO_BIG_ERROR:
	  fprintf(stderr, gettxt(":10", "Return packet too big.\n"));
	  snmpstat->intoobigs++;
	  break;

	case NO_SUCH_NAME_ERROR:
	  fprintf(stderr, 
		  gettxt(":11", "No such variable name. Index: %ld.\n"), 
		 in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->innosuchnames++;
	  break;

	case BAD_VALUE_ERROR:
	  fprintf(stderr, gettxt(":12", "Bad variable value. Index: %ld.\n"), 
		 in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->inbadvalues++;
	  break;

	case READ_ONLY_ERROR:
	  fprintf(stderr, gettxt(":13", "Read only variable. Index: %ld.\n"), 
		 in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->inreadonlys++;
	  break;

	case GEN_ERROR:
	  fprintf(stderr, gettxt(":14", "General error. Index: %ld.\n"),
		 in_pdu_ptr->u.normpdu.error_index);
	  snmpstat->ingenerrs++;
	  break;
	  
	default:
	  fprintf(stderr, gettxt(":15", "Unknown status code. %ld.\n"),
		 in_pdu_ptr->u.normpdu.error_status);
	  break;
	};
    }
  else 
    {
      /* Check request-id */
      if (in_pdu_ptr->u.normpdu.request_id != req_id)
	fprintf(stderr,
		gettxt(":16", "Request ID mismatch. Got: %ld, Expected: %ld.\n"),
		in_pdu_ptr->u.normpdu.request_id, req_id);

      print_varbind_list(in_pdu_ptr->var_bind_list); /* looky, looky */

    }
  free_authentication(in_auth_ptr); in_auth_ptr = NULL;
  free_pdu(in_pdu_ptr); in_pdu_ptr = NULL;
  close_up(fd);
  return(0);
}


void usage(char *argv[])
	{
   	fprintf(stderr, gettxt(":1", "Usage: %s [-f defns_file] [-T timeout] entity_addr community_string object_name -{i|o|d|a|c|g|t|s|n} value...\n"), argv[0]);
   	fprintf(stderr, gettxt(":18", "The available flags are:\n"));
   	fprintf(stderr, gettxt(":19", "     -i  The object is an integer. Value is specified as a decimal.\n"));
   	fprintf(stderr, gettxt(":20", "     -o  The object is an octet string. Value is specified as a hexidecimal\n         number enclosed within double quotes.\n"));
   	fprintf(stderr, gettxt(":21", "     -d  The object is an object identifier (in dot notation). Value is\n         specified as a decimal OID (see RFC 1213) enclosed within double\n         quotes.\n"));
   	fprintf(stderr, gettxt(":22", "     -a  The object is an IP address (in dot notation). Value is specified in\n         decimal number in IP address format (xxx.xxx.xxx.xxx) enclosed within\n         double quotes.\n"));
   	fprintf(stderr, gettxt(":23", "     -c  The object is a counter. Value is specified as a decimal integer.\n"));
   	fprintf(stderr, gettxt(":24", "     -g  The object is a gauge. Value is specified as a decimal integer.\n"));
   	fprintf(stderr, gettxt(":25", "     -t  The object is a in clock-ticks (hundredths of a second). Value is\n         specified as a decimal integer.\n"));
   	fprintf(stderr, gettxt(":26", "     -s  The object is a string. Value is specified as a character\n         string enclosed within double quotes.\n"));
   	fprintf(stderr, gettxt(":26", "     -n  The object is a null value. Value is specified as 0.\n"));
	exit(-1);
   	}

VarBindList *create_vb(char *argv[],
			char name[],
		       	char type[],
		       	char value[])
	{
  	OID oid_ptr;
  	VarBindList *vb_ptr;
  	long sl;
	unsigned long ul;
  	OctetString *os_ptr;
  	unsigned char ip_addr_buffer[4];
  	unsigned long temp_ip_addr;
	int length = 0;
  	int i;

  	if((oid_ptr = make_obj_id_from_dot((unsigned char *)name)) == NULL)
		{
	   	fprintf(stderr, gettxt(":3", "Cannot translate variable class: %s\n"), name);
      		exit(1);
      		}
  
  	if(strcmp(type, "-i") == 0)
		{
		if((length = strlen(value)) == 0)
      			usage(argv);

  		for(i = 0; i < length; i ++)
			{
			if(isdigit(value[i]) == 0)
				usage(argv);
			}

    		sscanf(value,"%d", &sl);
    		vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0L, sl, NULL, NULL);
    		oid_ptr = NULL;
    		return(vb_ptr);
  		}
  
  	if(strcmp(type, "-o") == 0) 
		{
		if((length = strlen(value)) == 0)
      			usage(argv);

  		for(i = 0; i < length; i ++)
			{
			if((isxdigit(value[i]) == 0) || (isdigit(value[i]) == 0))
				usage(argv);
			}

    		if((os_ptr = make_octet_from_hex((unsigned char *)value)) != NULL)
			{
	  		vb_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE,
 						0L, 0L, os_ptr, NULL);
	  		oid_ptr = NULL;
	  		return(vb_ptr);
			}

		return(NULL);
  		}

  	if(strcmp(type, "-d") == 0) 
		{
    		oid_ptr = make_obj_id_from_dot((unsigned char *)value);
    		vb_ptr = make_varbind(oid_ptr, OBJECT_ID_TYPE, 0L, 0L, 
					NULL, oid_ptr);
    		oid_ptr = NULL;
    		free_oid(oid_ptr);
    		return(vb_ptr);
  		}

  	if(strcmp(type, "-a") == 0) 
		{
		if((length = strlen(value)) == 0)
      			usage(argv);

  		for(i = 0; i < length; i ++)
			{
			if(isdigit(value[i]) == 0)
				if(value[i] != 0x2e)
					usage(argv);
			}

#ifdef BSD
    		temp_ip_addr = ntohl(inet_addr(value));
#endif
#ifdef FTP
    		temp_ip_addr = ntohl(inet_addr(value));
#endif
#ifdef CMUPCIP
    		temp_ip_addr = resolve_name(value);
#endif
#ifdef SVR3
    		temp_ip_addr = ntohl(inet_addr(value));
#endif
#ifdef SVR4
    		temp_ip_addr = ntohl(inet_addr(value));
#endif
    		ip_addr_buffer[0] = ((temp_ip_addr >> 24) & 0xff);
    		ip_addr_buffer[1] = ((temp_ip_addr >> 16) & 0xff);
    		ip_addr_buffer[2] = ((temp_ip_addr >> 8) & 0xff);
    		ip_addr_buffer[3] = (temp_ip_addr & 0xff);
    		os_ptr = make_octetstring(ip_addr_buffer, 4);
    		vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0L, 
					0L, os_ptr, NULL);
    		oid_ptr = NULL;
    		os_ptr = NULL;
    		return(vb_ptr);
  		}

  	if(strcmp(type, "-c") == 0) 
		{
		if((length = strlen(value)) == 0)
      			usage(argv);

  		for(i = 0; i < length; i ++)
			{
			if(isdigit(value[i]) == 0)
				usage(argv);
			}

    		sscanf(value,"%u", &ul);
    		vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ul, 0L, 
					NULL, NULL);
    		oid_ptr = NULL;
    		return(vb_ptr);
  		}
  
  	if(strcmp(type, "-g") == 0) 
		{
		if((length = strlen(value)) == 0)
      			usage(argv);

  		for(i = 0; i < length; i ++)
			{
			if(isdigit(value[i]) == 0)
				usage(argv);
			}

    		sscanf(value,"%u", &ul);
    		vb_ptr = make_varbind(oid_ptr, GAUGE_TYPE, ul, 0L, 
					NULL, NULL);
    		oid_ptr = NULL;
    		return(vb_ptr);
  		}
  
  	if(strcmp(type, "-t") == 0) 
		{
		if((length = strlen(value)) == 0)
      			usage(argv);

  		for(i = 0; i < length; i ++)
			{
			if(isdigit(value[i]) == 0)
				usage(argv);
			}

    		sscanf(value,"%d", &sl);
    		vb_ptr = make_varbind(oid_ptr, TIME_TICKS_TYPE, 0L, sl,
					NULL, NULL);
    		oid_ptr = NULL;
    		return(vb_ptr);
  		}
  
  	if(strcmp(type, "-s") == 0) 
		{
    		os_ptr = make_octetstring((unsigned char *)value, 
						strlen(value));
    		vb_ptr = make_varbind(oid_ptr, DisplayString, 0L, 0L, 
					os_ptr, NULL);
    		oid_ptr = NULL;

 		/* don't free the octetstring, it is part of the vb! */ 
    		os_ptr = NULL;
    		return(vb_ptr);
  		}

  	if(strcmp(type, "-n") == 0)
		{
    		vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, 
					NULL, NULL);
    		oid_ptr = NULL;
    		return(vb_ptr);
  		}
  
  	return(NULL);
	}
