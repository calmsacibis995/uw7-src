#ident	"@(#)util.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)util.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)util.c	2.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */
/*
 * util.c 
 *
 * various utility and common routines
 */

#include <unistd.h>
#include "snmpio.h"

#ifdef BSD
#include <strings.h>
#endif
#if defined(SVR3) || defined(SVR4)
#include <string.h>
#endif

extern char *inet_ntoa();

extern int timeout;
extern long req_id;
extern int nflag;

snmploop(init_oid_ptr, resp_pdu_ptr, community, printer)
	OID init_oid_ptr;
	Pdu *resp_pdu_ptr;
	char *community;
	int (*printer)();
{
	OID oid_ptr;
	OctetString *community_ptr;
	VarBindList *vb_ptr, *temp_vb_ptr;
	Pdu *req_pdu_ptr, *old_resp_pdu_ptr;
	AuthHeader *auth_ptr, *in_auth_ptr;
	int lineno = 0;
	int cc;

	while (1) {
		req_pdu_ptr = make_pdu(GET_NEXT_REQUEST_TYPE, ++req_id, 0L, 0L,
				       NULL, NULL, 0L, 0L, 0L);
		temp_vb_ptr = resp_pdu_ptr->var_bind_list;
		while (temp_vb_ptr != NULL) {
			oid_ptr = make_oid(temp_vb_ptr->vb_ptr->name->oid_ptr,
					   temp_vb_ptr->vb_ptr->name->length);
			vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L,
					      NULL, NULL);
			link_varbind(req_pdu_ptr, vb_ptr);
			temp_vb_ptr = temp_vb_ptr->next;
		}

		build_pdu(req_pdu_ptr);

		community_ptr = make_octet_from_text((unsigned char *)community);
		auth_ptr = make_authentication(community_ptr);
		build_authentication(auth_ptr, req_pdu_ptr);

		cc = send_request(fd, auth_ptr);

		free_authentication(auth_ptr);
		free_pdu(req_pdu_ptr);

		if (cc != TRUE) {
			free_pdu(resp_pdu_ptr);
			free_oid(init_oid_ptr);
			return(-1);
		}

	recv:
		cc = get_response(timeout);

		if (cc == ERROR) {
			fprintf(stderr, gettxt(":70", "%s: get_response returned ERROR\n"),
				imagename);
			free_pdu(resp_pdu_ptr);
			free_oid(init_oid_ptr);
      			close_up(fd);
      			exit(-1);
			}

		if (cc == TIMEOUT) {
			printf(gettxt(":71", "%s: No response. Possible invalid argument. Trying again.\n"), imagename);
      			close_up(fd);
      			exit(-1);
			}

		if ((in_auth_ptr =
		     parse_authentication(packet, packet_len)) == NULL) {
			fprintf(stderr, gettxt(":72", "%s: Error parsing packet.\n"),
				imagename);
			free_oid(init_oid_ptr);
			return(-1);
		}
		old_resp_pdu_ptr = resp_pdu_ptr;
		if ((resp_pdu_ptr = parse_pdu(in_auth_ptr)) == NULL) {
			fprintf(stderr, gettxt(":73", "%s: Error parsing pdu packet.\n"),
				imagename);
			free_authentication(in_auth_ptr);
			free_pdu(old_resp_pdu_ptr);
			free_oid(init_oid_ptr);
			return(-1);
		}
		free_authentication(in_auth_ptr);

		if (resp_pdu_ptr->type != GET_RESPONSE_TYPE) {
			fprintf(stderr,
				gettxt(":74", "%s: Received non GET_RESPONSE_TYPE packet.\n"),
				imagename);
			free_pdu(resp_pdu_ptr);
			free_pdu(old_resp_pdu_ptr);
			free_oid(init_oid_ptr);
			return(-1);
		}
		snmpstat->ingetresponses++;
		/* check for error status stuff... */
		if (resp_pdu_ptr->u.normpdu.error_status != NO_ERROR) {
			if (resp_pdu_ptr->u.normpdu.error_status == NO_SUCH_NAME_ERROR) {
				free_pdu(resp_pdu_ptr);
				free_pdu(old_resp_pdu_ptr);
				free_oid(init_oid_ptr);
				snmpstat->innosuchnames++;
				return(-1);
			}
			fprintf(stderr, gettxt(":75", "%s: Error code set in packet - "),
				imagename);
			switch ((short) resp_pdu_ptr->u.normpdu.error_status) {
			case TOO_BIG_ERROR:
				fprintf(stderr, gettxt(":76", "Return packet too big.\n"));
				snmpstat->intoobigs++;
				break;
			case BAD_VALUE_ERROR:
				fprintf(stderr,
					gettxt(":77", "Bad variable value. Index: %ld.\n"),
				       resp_pdu_ptr->u.normpdu.error_index);
				snmpstat->inbadvalues++;
				break;
			case READ_ONLY_ERROR:
				fprintf(stderr, gettxt(":78", "Read only variable: %ld.\n"),
				       resp_pdu_ptr->u.normpdu.error_index);
				snmpstat->inreadonlys++;
				break;
			case GEN_ERROR:
				fprintf(stderr, gettxt(":79", "General error: %ld.\n"),
				       resp_pdu_ptr->u.normpdu.error_index);
				snmpstat->ingenerrs++;
				break;
			default:
				fprintf(stderr, gettxt(":80", "Unknown status code: %ld.\n"),
				       resp_pdu_ptr->u.normpdu.error_status);
				break;
			}
			free_pdu(resp_pdu_ptr);
			free_pdu(old_resp_pdu_ptr);
			free_oid(init_oid_ptr);
			return(-1);
		}
		/*
		 * Check for termination case (only checking
		 * first one for now) 
		 */
		if (chk_oid(init_oid_ptr, resp_pdu_ptr->var_bind_list->vb_ptr->name) < 0) {
			free_pdu(resp_pdu_ptr);
			free_pdu(old_resp_pdu_ptr);
			free_oid(init_oid_ptr);
			return(0);
		}

		/*
		 * Check the request id to see if it matches.  If it
		 * doesn't, drop this response and wait for another.
		 */
		if (resp_pdu_ptr->u.normpdu.request_id != req_id) {
			free_pdu(resp_pdu_ptr);
			resp_pdu_ptr = old_resp_pdu_ptr;
			goto recv;
		}

		free_pdu(old_resp_pdu_ptr);

		if ((*printer)(resp_pdu_ptr->var_bind_list, lineno++, community) < 0) {
			free_pdu(resp_pdu_ptr);
			free_oid(init_oid_ptr);
			return(-1);
		}
	}
}

chk_oid(oid1_ptr, oid2_ptr)
	OID oid1_ptr;
	OID oid2_ptr;
{
	int i;

	for (i = 0; i < oid1_ptr->length; i++) {
		if (oid1_ptr->oid_ptr[i] < oid2_ptr->oid_ptr[i])
			return(-1);
	}
	return(0);
}

targetdot(dotname, name, lineno)
	char *dotname;
	OID name;
	int lineno;
{
	char buffer[128];

	if (make_dot_from_obj_id(name, buffer) ||
	    strncmp(dotname, buffer, strlen(dotname))) {
		if (lineno == 0) {
			printf(gettxt(":81", "Not enough information available, %s missing.\n"),
			       dotname);
		}
		return(0);
	}
	return(1);
}

doreq(dots, community, printer)
	char *dots[];
	char *community;
	int (*printer)();
{
	int index;
	OID oid_ptr;
	VarBindList *vb_ptr;
	Pdu *pdu_ptr;
	OID init_oid_ptr;

	/* start a PDU */
	pdu_ptr = make_pdu(GET_NEXT_REQUEST_TYPE, ++req_id, 0L, 0L, NULL, NULL,
			   0L, 0L, 0L);
	if (pdu_ptr == NULL) {
		return(-1);
	}

	index = 0;
	if ((init_oid_ptr = make_obj_id_from_dot((unsigned char *)dots[index])) == NULL) {
		fprintf(stderr, gettxt(":82", "Cannot translate variable class: %s.\n"),
			dots[index]);
		free_pdu(pdu_ptr);
		return(-1);
	}

	/* Flesh out packet */
	while (*dots[index] != '\0') {
		if ((oid_ptr = make_obj_id_from_dot((unsigned char *)dots[index])) == NULL) {
		   fprintf(stderr, gettxt(":82", "Cannot translate variable class: %s.\n"),
				dots[index]);
			free_oid(init_oid_ptr);
			free_pdu(pdu_ptr);
			return(-1);
		}
		vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
		if (vb_ptr == NULL) {
			free_oid(oid_ptr);
			free_oid(init_oid_ptr);
			free_pdu(pdu_ptr);
			return(-1);
		}
		link_varbind(pdu_ptr, vb_ptr);
		index++;
	}

	return(snmploop(init_oid_ptr, pdu_ptr, community, printer));
}

char *
inetname(addr, port, proto)
	unsigned long addr;
	int port;
	char *proto;
{
	char *cp;
	struct in_addr in;
	struct servent *sp;
	struct hostent *hp;
	static char line[80];

	if (addr != INADDR_ANY) {
		in.s_addr = htonl(addr);
		if (nflag ||
		    (hp = gethostbyaddr((char *)&in, sizeof(in), AF_INET)) == NULL) {
			cp = inet_ntoa(in);
		} else {
#ifdef BSD
			if ((cp = index(hp->h_name, '.')) != NULL)
#endif
#if defined(SVR3) || defined(SVR4)
			if ((cp = strchr(hp->h_name, '.')) != NULL)
#endif
				*cp = '\0';
			cp = hp->h_name;
		}
	} else {
		cp = "*";
	}

	sprintf(line, "%.20s.", cp);
#ifdef BSD
	cp = index(line, '\0');
#endif
#if defined(SVR3) || defined(SVR4)
	cp = strchr(line, '\0');
#endif

	if (port != 0) {
#ifdef SVR3
		port = htons((ushort) port) & 0xffff;
#endif
#if defined(BSD) || defined(SVR4)
		port = htons((u_short) port) & 0xffff;
#endif
		if (nflag || (sp = getservbyport(port, proto)) == NULL) {
#ifdef SVR3
			sprintf(cp, "%u", ntohs((ushort) port) & 0xffff);
#endif
#if defined(BSD) || defined(SVR4)
			sprintf(cp, "%u", ntohs((u_short) port) & 0xffff);
#endif
		} else {
			sprintf(cp, "%.20s", sp->s_name);
		}
	} else {
		sprintf(cp, "%.20s", "*");
	}

	return(line);
}

/*
 * Convert the given IP address to the appropriate host name.
 * Addresses which do not convert directly to hostnames are
 * converted to dot format.
 */
char *
hostname(addr)
	unsigned long addr;
{
	char *cp;
	struct hostent *hp;
	struct in_addr in;
	static char name[50];
	static unsigned long old_addr = (unsigned long) -1;

	if (old_addr == addr) {
		return(name);
	}

	in.s_addr = htonl(addr);
	if (!nflag && (hp = gethostbyaddr((char *)&in, sizeof(in), AF_INET)) != NULL) {
#ifdef BSD
		if ((cp = index(hp->h_name, '.')) != NULL)
#endif
#if defined(SVR3) || defined(SVR4)
		if ((cp = strchr(hp->h_name, '.')) != NULL)
#endif
			*cp = '\0';
		cp = hp->h_name;
	} else {
		cp = inet_ntoa(in);
	}
	strcpy(name, cp);
	old_addr = addr;
	return(name);
}

char *
map_mib(value, map, size_map)
	int value;
	char *map[];
	int size_map;
{
	int nelem;

	nelem = size_map / sizeof(map[0]);
	if (value > 0 && value < nelem)
		return(map[value]);
	return(map[0]);
}
