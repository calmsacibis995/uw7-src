#ident	"@(#)smuxd.c	1.3"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/* smuxd.c - routines specific for handling smux I/o transactions */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

/*
 * Revision History:
 *
 * L000		jont		02oct97
 *	- fixed possible freeing of uninitialised pointer in smux_get_method().
 * L001		jont		02oct97
 *	- properly handle *any* packets that come in while we're looking
 *	  for a response to our set/get request.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <unistd.h>

#if defined(SVR3) || defined(SVR4)
#include <fcntl.h>
#include <sys/stream.h>
#include <sys/tiuser.h>
extern int t_errno;
#endif

#include "snmp.h"
#include "snmpd.h"
#include "peer.h"

#define	OK	 0
#define	NOTOK	-1
#define	MAXCONN  5
/* Global variables */

extern struct smuxPeer *PHead;
extern struct smuxTree *THead;
extern struct smuxReserved reserved[];

extern int trap_fd;
extern int auth_traps_enabled;
extern int log_level;
extern VarEntry *var_list_root;

static unsigned char buffer[1024];

char	*octet2str();
VarEntry	*get_exact_var();

int	do_smux_new(int	smux_fd)
{
    int      fd;
    struct   sockaddr_in in_socket;
    register struct sockaddr_in *isock = &in_socket;

    register struct smuxPeer *pb;
    static   int smux_peerno = 0;

    if((fd = join_tcp_client(smux_fd, isock)) == NOTOK) {
	if(t_errno == TNODATA)
	    return(NOTOK);
	exit(-1);
    }

    if((pb = (struct smuxPeer *) calloc(1, sizeof *pb)) == NULL) {
	(void) t_close (fd);
	return(NOTOK);
    }

    if((pb->pb_data = (struct _data_area *) calloc (1,
			  sizeof (struct _data_area))) == NULL) {
	(void) free (pb);
	(void) t_close (fd);
	return(NOTOK);
    }

    if((pb->pb_data->data = (char *) calloc (1, BUFSIZE)) == NULL) {
	(void) free (pb->pb_data);
	(void) free (pb);
	(void) t_close (fd);
	return(NOTOK);
    }
    pb->pb_data->len = 0;

    pb->pb_address = *isock;
    insque (pb, PHead->pb_back);
    pb->pb_index = ++smux_peerno;

    return (pb->pb_fd = fd);
}


void	do_smux_old (int fd)
{
    register struct smuxPeer	*pb;
    SNMP_SMUX_PDU	*pdu;
    unsigned char	*packet = buffer;
    long 	length;

    for (pb = PHead->pb_forw; pb != PHead; pb = pb->pb_forw)
	if(pb->pb_fd == fd)
	    break;
    
    if(pb == PHead) {
	syslog(LOG_WARNING, gettxt(":101", "Could not find the SMUX peer block.\n"));
	return;
    }

    /* receive the pdu */
    if(log_level)
	   printf(gettxt(":102", "End-point descriptor is %d.\n"), pb->pb_fd);

    if(fetch_smux_packet (pb->pb_fd, pb->pb_data->data, &(pb->pb_data->len),
			  packet, &length) == NOTOK) {
	pb_free(pb);
	return;
    }

    if(log_level) {
	   printf(gettxt(":103", "SMUX PACKET received: "));
	   printf(gettxt(":104", "packet length = %d.\n"), length);
	   print_packet_out (packet, length);
      }

    /* decode the pdu */
    if((pdu = decode_SMUX_PDU (packet, length)) == NULL) {
	pb_free(pb);
	return;
    }

    if(smux_process_new (pb, pdu) == NOTOK) {
	pb_free(pb);
	if(log_level)
	    printf(gettxt(":105", "SMUX process failed.\n"));
    }
    free_SNMP_SMUX_PDU (pdu);
    return;
}


int smux_process_new (struct smuxPeer *pb,SNMP_SMUX_PDU *pdu)
{
    int		result = OK;
    char	temp_buf[128];
    char       *pass;
    long	length;
    unsigned char	*packet = buffer;

    switch (pdu->offset) {
	case SMUX__PDUs_simple:
	    if(pb->pb_identity)
		goto unexpected;

	    {
		register SMUX_SimpleOpen  *simple = pdu->un.simple;
		register struct smuxEntry *se;

		if(simple->version != SMUX_version_1) {
		    syslog(LOG_WARNING, gettxt(":106", "Bad version of SMUX protocol (%d).\n"),
			simple->version);
		    return (NOTOK);
		}

		pb->pb_identity = simple->identity;
		simple->identity = NULL;

		if(log_level)
		    printf(gettxt(":107", "Identity is %s.\n"), sprintoid(pb->pb_identity));

		if((pb->pb_description = octet2str(simple->description)) == NULL) {
		    syslog(LOG_WARNING, gettxt(":91", "Not enough memory.\n"));
		    return (NOTOK);
		}

		if((pass = octet2str (simple->password)) == NULL)  {
		    syslog(LOG_WARNING, gettxt(":91", "Not enough memory.\n"));
		    return (NOTOK);
		}

		if(((se = getsmuxEntrybyidentity (pb->pb_identity)) == NULL)
			|| strcmp (se->se_password, pass)) {
		    syslog(LOG_WARNING, se ? gettxt(":108", "Bad password.\n") : gettxt(":109", "Bad identity.\n"));
		    if(auth_traps_enabled == 1) {
			send_trap(4, 0, NULL);
#ifdef NETWARE
			send_ipx_trap(4, 0, NULL);
#endif
                    }
		    (void) free (pass);
		    return (NOTOK);
		}
		(void) free (pass);

		if((pb->pb_priority = se->se_priority) < 0)
		    pb->pb_priority = 0;

		break;
	    }

	case SMUX__PDUs_close:
	    if(!pb->pb_identity)
		goto unexpected;
	    return (NOTOK);

	case SMUX__PDUs_registerRequest:
	    if(!pb->pb_identity)
		goto unexpected;

	    {
		register SMUX_RReqPDU *rreq = pdu->un.registerRequest;
		SMUX_RRspPDU rrsp;
		SNMP_SMUX_PDU rsp;
		register struct smuxReserved *sr;
		register struct smuxTree *tb = NULL;
		register struct smuxTree *qb;
		register OID   oid = rreq->subtree;
		VarEntry *var;
		VarEntry *dummy_var;

		for (sr = reserved; sr->rb_text; sr++)
		    if(sr->rb_name &&
		        bcmp((char *) sr->rb_name ->oid_ptr,
			(char *) oid->oid_ptr,
			    (sr->rb_name->length <= oid->length ?
			    sr->rb_name->length : oid->length)
			    * sizeof oid->oid_ptr[0]) == 0) {
			syslog(LOG_WARNING, gettxt(":110", "Reserved subtree cannot be registered.\n"));
			goto no_dice;
		    }

		dummy_var = (VarEntry *) malloc (sizeof(VarEntry));
		dummy_var->class_ptr = oid;

		if(log_level) {
		    printf(gettxt(":111", "Subtree is %s.\n"), sprintoid(oid));
		    printf(gettxt(":112", "Operation is %d.\n"), rreq->operation);
		}
		var = get_exact_var (var_list_root, dummy_var);
		(void) free ((char *) dummy_var);
		if(var == NULL) {

		    if(rreq->operation == register_op_delete) {
			syslog(LOG_WARNING, gettxt(":113", "Sub-tree not registered.\n"));
			goto no_dice;
		    }

		    if((var = (VarEntry *) calloc (1, sizeof *var)) == NULL) {
			syslog(LOG_WARNING, gettxt(":99", "Out of memory: %d needed.\n"), sizeof(*var));
			if(var)
			    free ((char *) var);
			return (NOTOK);
		    }

		    var->class_ptr = rreq->subtree;
		    rreq->subtree = NULL;
		    if(rreq->operation == register_op_readOnly)
			var->rw_flag = READ_ONLY;
		    else
			var->rw_flag = READ_WRITE;

		    (void) add_objects (var);
		}
		else {
		    if(rreq->operation == register_op_delete) {
			for (tb = (struct smuxTree *) var->smux;
				tb ; tb = tb->tb_next)
			    if((tb->tb_peer == pb) &&
				((rreq->priority < 0) ||
			 	(rreq->priority == tb->tb_priority)))
				break;
			if(tb)
			    tb_free (tb);
			else {
			    syslog(LOG_WARNING, gettxt(":114", "No such registration.\n"));
			    var = NULL;
			}
			goto no_dice;
		    }

		    if(var->class_ptr->length > oid->length) {
			   syslog(LOG_WARNING, gettxt(":115", "Bad sub-tree.\n"));
			var = NULL;
			goto no_dice;
		    }
		}

		if((tb = (struct smuxTree *) calloc (1, sizeof *tb)) == NULL) {
			syslog(LOG_WARNING, gettxt(":99", "Out of memory: %d needed.\n"), sizeof(*tb));
		    return (NOTOK);
		}

		if(log_level)
		    printf(gettxt(":116", "Requested priority %d.\n"), rreq->priority);

		if((tb->tb_priority = rreq->priority) < pb->pb_priority)
		    tb->tb_priority = pb->pb_priority;

		if(log_level)
		    printf(gettxt(":117", "Tree priority now %d.\n"), tb->tb_priority);

		for (qb = (struct smuxTree *) var->smux; qb;
			   qb = qb->tb_next) {
		    if(log_level)
			   printf(gettxt(":118", "Existing priority: %d.\n"), qb->tb_priority);
		    if(qb->tb_priority > tb->tb_priority) {
			break;
		    }
		    else {
			if(qb->tb_priority == tb->tb_priority)
			    tb->tb_priority++;
		    }
		}

		if(log_level)
		    printf(gettxt(":117", "Tree priority now %d.\n"), tb->tb_priority);
		tb->tb_peer = pb;

no_dice: ;
		bzero((char *) &rsp, sizeof rsp);
		rsp.offset = SMUX_REG_RESPONSE_TYPE;
		rsp.un.registerResponse = &rrsp;

		bzero((char *) &rrsp, sizeof rrsp);
		rrsp.parm = tb ? tb->tb_priority : RRspPDU_failure;

		if(encode_SMUX_PDU(&rsp) != NOTOK) {

		    if(log_level) {
			   printf(gettxt(":119", "SMUX PACKET being sent: "));
			   printf(gettxt(":104", "packet length = %d.\n"), rsp.packlet->length);
			print_packet_out (rsp.packlet->octet_ptr,
					  rsp.packlet->length);
		    }

		    if(dispatch_smux_packet (pb->pb_fd, 
					     rsp.packlet->octet_ptr, 
                                              rsp.packlet->length) == NOTOK) {
			   syslog(LOG_WARNING, gettxt(":120", "Error sending an SMUX pdu.\n"));
			result = NOTOK;
			return (result);
		    }

		    (void) free_octetstring(rsp.packlet);

		    if(log_level)
			   printf(gettxt(":121", "Dispatched the register-response!!!\n"));

		    if(tb && rreq->operation != register_op_delete) {
			register int i;
			register unsigned int *ip, *jp;
			register struct smuxTree  **qpp;

			tb->tb_subtree = var;

			/* Insert a new element into the single-linked
			 * chain of smuxTree structures.   Elements are
			 * chained in order of increasing priority.
			 */

			for (qpp = (struct smuxTree **) &var->smux;
				qb = *qpp; qpp = &qb->tb_next)
			    if(qb->tb_priority > tb->tb_priority)
				break;
			tb->tb_next = qb;
			*qpp = tb;

			/* Fill in the tb_instance field of the element
			 * with the the size of the object name
			 * concatenated by the object name followed
			 * by its priority.
			 */

			ip = tb->tb_instance;
			jp = (unsigned int *) var->class_ptr->oid_ptr;
			*ip++ = var->class_ptr->length;
			for (i = var->class_ptr->length; i > 0; i--)
			    *ip++ = *jp++;
			*ip++ = tb->tb_priority;
			tb->tb_insize = ip - tb->tb_instance;

			/* Insert the new element into the doubly-linked
			 * chain of all smuxTree structures anchored at
			 * THead. Elements are chained in lexicographic
			 * order of tb_instance so that the get_tbent()
			 * function works correctly for the get-next
			 * operations.
			 */

			for (qb = THead->tb_forw; qb != THead;
				qb = qb->tb_forw)
			    if(elem_cmp (tb->tb_instance,
			    	     tb->tb_insize, qb->tb_instance,
				     qb->tb_insize) < 0)
				break;

			insque (tb, qb->tb_back);
		    }

		}
		else {
		    syslog(LOG_WARNING, gettxt(":122", "Error encoding the SMUX pdu.\n"));
		    result = NOTOK;
		    return (result);
		}
		break;
	    }

	case SMUX_TRAP_TYPE:
	    if(!pb->pb_identity)
		goto unexpected;
		
            send_smux_trap(pdu->un.trap);
            break;

	case SMUX_REG_RESPONSE_TYPE:
	case SMUX_GET_REQUEST_TYPE:
	case SMUX_GET_NEXT_REQUEST_TYPE:
	case SMUX_GET_RESPONSE_TYPE:
	case SMUX_SET_REQUEST_TYPE:
unexpected: ;
	    syslog(LOG_WARNING, gettxt(":123", "Unexpected operation (%d).\n"), pdu->offset);
	    return NOTOK;

	default:
	    syslog(LOG_WARNING, gettxt(":124", "Bad operation (%d).\n"), pdu->offset);
	    return NOTOK;
    }
    return (result);
}


int smux_get_method(VarEntry *var, struct smuxPeer *peer,
		    VarBindList *vb_ptr, int type_search, int request_id)
{
    int		status;
    long	length;
    unsigned char 	*packet = buffer;
    SNMP_SMUX_PDU	 req, *rsp;
    SMUX_GetRequest_PDU  *get_req;
    SMUX_GetResponse_PDU *get_rsp;
    VarBindList *new_vb;

    status = OK;
    rsp = (SNMP_SMUX_PDU *) NULL;				/* L000 */
    bzero ((char *) &req, sizeof req);

    if(type_search == EXACT)
	req.offset = SMUX_GET_REQUEST_TYPE;
    else
	req.offset = SMUX_GET_NEXT_REQUEST_TYPE;

    if((get_req = (SMUX_GetRequest_PDU *) malloc (sizeof(SMUX_GetRequest_PDU))) == NULL) {
      syslog(LOG_WARNING, gettxt(":99", "Out of memory: %d needed.\n"), sizeof(SMUX_GetRequest_PDU));
       return (NOTOK);
    }
    req.un.get__request = get_req;

    req.un.get__request->request__id = request_id;
    req.un.get__request->error__status = 0;
    req.un.get__request->error__index = 0;
    req.un.get__request->variable__bindings = vb_ptr;

    if(encode_SMUX_PDU (&req) == NOTOK) {
	   syslog(LOG_WARNING, gettxt(":122", "Error encoding the SMUX pdu.\n"));
	goto lost_peer;
    }

    if(log_level) {
	   printf(gettxt(":119", "SMUX PACKET being sent: "));
	   printf(gettxt(":104", "packet length = %d.\n"),	req.packlet->length);
	   print_packet_out (req.packlet->octet_ptr, req.packlet->length);
      }

    /* send & receive the stuff here */
    if(dispatch_smux_packet (peer->pb_fd, req.packlet->octet_ptr, 
	req.packlet->length) == NOTOK) {
	syslog(LOG_WARNING, gettxt(":125", "Error dispatching the SMUX pdu.\n"));
	goto lost_peer;
    }

    /* free the packet */
    (void) free_octetstring (req.packlet);

try_again: ;
    if(fetch_smux_packet (peer->pb_fd, peer->pb_data->data, 
			  &(peer->pb_data->len), packet, &length) == NOTOK) {
	   syslog(LOG_WARNING, "Error fetching an SMUX pdu");
	goto lost_peer;
    }

    if(log_level) {
	   printf(gettxt(":103", "SMUX PACKET received: "));
	   printf(gettxt(":104", "packet length = %d.\n"), length);
	   print_packet_out (packet, length);
      }

    if((rsp = decode_SMUX_PDU (packet, length)) == NULL) {
lost_peer: ;
	pb_free (peer);
	status = GEN_ERROR;
	goto out;
    }

    if(rsp->offset != SMUX_GET_RESPONSE_TYPE) {
	if(log_level)
	    printf(gettxt(":126", "The reply is of type %x.\n"), rsp->offset);
								/* L001 vv */
	if(smux_process_new (peer, rsp) == NOTOK)
	    goto lost_peer;
	free_SNMP_SMUX_PDU (rsp); rsp = NULL;
	goto try_again;
								/* L001 ^^ */
    }

    get_rsp = rsp->un.get__response;
    new_vb = get_rsp->variable__bindings;

    free_oid (vb_ptr->vb_ptr->name);  NULLIT(vb_ptr->vb_ptr->name);
    free_oid (vb_ptr->vb_ptr->value->oid_value);
    NULLIT(vb_ptr->vb_ptr->value->oid_value);
    free_octetstring (vb_ptr->vb_ptr->value->os_value);
    NULLIT(vb_ptr->vb_ptr->value->os_value);

    if((vb_ptr->vb_ptr->name = oid_cpy (new_vb->vb_ptr->name)) == NULL) {
	status = GEN_ERROR;
	goto out;
    }

    vb_ptr->vb_ptr->value->ul_value = 0;
    vb_ptr->vb_ptr->value->sl_value = 0;
    vb_ptr->vb_ptr->value->os_value = NULL;
    vb_ptr->vb_ptr->value->oid_value = NULL;

    vb_ptr->vb_ptr->value->type = new_vb->vb_ptr->value->type;
    vb_ptr->vb_ptr->value->ul_value = new_vb->vb_ptr->value->ul_value;
    vb_ptr->vb_ptr->value->sl_value = new_vb->vb_ptr->value->sl_value;

    if(new_vb->vb_ptr->value->os_value)
	if((vb_ptr->vb_ptr->value->os_value = os_cpy (new_vb->vb_ptr->value->os_value)) == NULL) {
	    status = GEN_ERROR;
	    goto out;
	}

    if(new_vb->vb_ptr->value->oid_value)
	if((vb_ptr->vb_ptr->value->oid_value = oid_cpy (new_vb->vb_ptr->value->oid_value)) == NULL) {
	    status = GEN_ERROR;
	    goto out;
	}

    if(log_level) {
	   printf(gettxt(":127", "VAR in the received SMUX packet after copying: \n"));
	   print_varbind_list (vb_ptr);
    }

    switch (status = get_rsp->error__status) {
	case NO_ERROR:
	    {
		VarBindList *vp;

		if((vp = get_rsp->variable__bindings) == NULL) {
		    status = GEN_ERROR;
		    goto out;
		}
		if(((var->class_ptr->length > vp->vb_ptr->name->length)
		   || (cmp_oid_class (var->class_ptr, vp->vb_ptr->name) > 0))
		   && (type_search == NEXT))
		    status = NOTOK;
		else
		    status = OK;
        	break;
	    }
	case NO_SUCH_NAME_ERROR:
	    if(type_search == NEXT) {
		status = NOTOK;
		break;
	    }
	default:
	    break;
    }

out: ;
    if(rsp)
	free_SNMP_SMUX_PDU (rsp);
    if(get_req) {
	get_req->variable__bindings = NULL;
	free_SMUX_GetPDU (get_req);
    }

    return status;
}


#define	SET        0
#define	COMMIT     1
#define	ROLLBACK   2

int smux_set_method (VarEntry *var, struct smuxPeer *peer,
		     VarBindList *vb_ptr, int offset, int request_id)
{
    int		status;
    long	length;
    unsigned char	*packet = buffer;
    SNMP_SMUX_PDU	 req, *rsp;
    SMUX_GetRequest_PDU	*set;
    SMUX_GetRequest_PDU	*get;

    status = OK;
    rsp = (SNMP_SMUX_PDU *) NULL;				/* L000 */
    bzero ((char *) &req, sizeof req);

    if((set = (SMUX_SetRequest_PDU *) malloc (sizeof(SMUX_SetRequest_PDU))) == NULL) {
	   syslog(LOG_WARNING, gettxt(":99", "Out of memory: %d needed.\n"), sizeof(SMUX_SetRequest_PDU));
       return (NOTOK);
    }
    req.un.set__request = set;

    switch (offset) {
	case SET:
	    req.offset = SMUX_SET_REQUEST_TYPE;
	    req.un.set__request->request__id = request_id;
	    req.un.set__request->error__status = 0;
	    req.un.set__request->error__index = 0;
	    req.un.set__request->variable__bindings = vb_ptr;
	    peer->pb_sendCoR = 1;
	    break;

	case COMMIT:
	    req.un.commitOrRollback->parm = SOutPDU_commit;
	    goto stuff_remain;

	case ROLLBACK:
	    req.un.commitOrRollback->parm = SOutPDU_rollback;
stuff_remain: ;
	    req.offset = SMUX_SOUT_TYPE;
	    if(!peer->pb_sendCoR)
		return (status);
	    peer->pb_sendCoR = 0;
	    break;
    }

    if(encode_SMUX_PDU (&req) == NOTOK) {
	   syslog(LOG_WARNING, gettxt(":122", "Error encoding the SMUX pdu.\n"));
	goto lost_peer;
    }

    /* send and receive the stuff here */
    /* check return values for errors later */

    if(log_level) {
	   printf(gettxt(":119", "SMUX PACKET being sent: "));
	   printf(gettxt(":104", "packet length = %d.\n"), req.packlet->length);
	print_packet_out(req.packlet->octet_ptr, req.packlet->length);
    }

    if(dispatch_smux_packet (peer->pb_fd, req.packlet->octet_ptr, 
                              req.packlet->length) == NOTOK) {
	   syslog(LOG_WARNING, gettxt(":125", "Error dispatching the SMUX pdu.\n"));
	goto lost_peer;
    }

    /* free the packet */
    (void) free_octetstring (req.packlet);
 
    if((offset == COMMIT) || (offset == ROLLBACK)) {
	if(set) {
	    set->variable__bindings = NULL;
	    free_SMUX_GetPDU (set);
	}
	return status;
    }

try_again: ;
    if(fetch_smux_packet (peer->pb_fd, peer->pb_data->data,
			  &(peer->pb_data->len), packet, &length) == NOTOK) {
	   syslog(LOG_WARNING, gettxt(":128", "Error fetching an SMUX pdu.\n"));
	goto lost_peer;
    }

    if(log_level) {
	   printf(gettxt(":103", "SMUX PACKET received: "));
	   printf(gettxt(":104", "packet length = %d.\n"), length);
	   print_packet_out(packet, length);
      }

    if((rsp = decode_SMUX_PDU (packet, length)) == NULL) {
lost_peer: ;
	pb_free (peer);
	status = GEN_ERROR;
	goto out;
    }

    if(rsp->offset != SMUX_GET_RESPONSE_TYPE) {
								/* L001 vv */
	if(smux_process_new (peer, rsp) == NOTOK)
	    goto lost_peer;
	free_SNMP_SMUX_PDU (rsp); rsp = NULL;
	goto try_again;
								/* L001 ^^ */
    }
    get = rsp->un.get__response;

    switch (status = get->error__status) {
        case NO_ERROR:
	    {
		VarBindList *vp;

		if((vp = get->variable__bindings) == NULL) {
		    status = GEN_ERROR;
		    goto out;
		}
		status = OK;
        	break;
	    }
        default:
            break;
    }

out: ;
    if(rsp)
	free_SNMP_SMUX_PDU (rsp);
    if(set) {
	set->variable__bindings = NULL;
	free_SMUX_GetPDU (set);
    }

    return status;
}

