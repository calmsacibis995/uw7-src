#ident	"@(#)snmp_isode.c	1.3"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#define	INCLUDE_ISODE_SNMP
#include "include.h"

#if	defined(PROTO_ISODE_SNMP)
#include "inet.h"
#include "snmp_isode.h"

/*
 *	NOTES:
 *		general startup flow....
 *		initialize smux tcp port.  When the socket is
 *		ready for writing, kick off the registration
 *		process.  
 */

int debug = 0;
int snmp_debug = 0;
int snmp_quantum = 0;
int doing_snmp;
task *snmp_task;
static task_timer *snmp_timer_startup;
trace *snmp_trace_options;
pref_t snmp_preference;
u_short	snmp_port;
OID snmp_nullSpecific = NULLOID;

/* Warn once about connect failure, and after 2 attempts. */
static int snmp_cfail_warn = -2;

static struct smuxEntry *se = NULL;
static char my_desc[LINE_MAX];
static struct type_SNMP_VarBindList *savedVarBinds = NULL;

static struct snmp_tree snmp_trees = {
    &snmp_trees,
    &snmp_trees
};
static struct snmp_tree *snmp_tree_next;


#define	TREE_LIST(srp, tree)	for (srp = (tree)->r_forw; srp != &snmp_trees; srp = srp->r_forw)
#define	TREE_LIST_END(srp, tree)


static const bits snmp_flag_bits[] = {
    { SMUX_TREE_REGISTER,	"Register" },
    { SMUX_TREE_REGISTERED,	"Registered" },
    { SMUX_TREE_OBJECTS,	"Objects" },
    { 0,			NULL }
};


static const bits snmp_error_bits[] = {
    { int_SNMP_error__status_noError,		"noError" },
    { int_SNMP_error__status_tooBig,		"tooBig" },
    { int_SNMP_error__status_noSuchName,	"noSuchName" },
    { int_SNMP_error__status_badValue,		"badValue" },
    { int_SNMP_error__status_readOnly,		"readOnly" },
    { int_SNMP_error__status_genErr,		"genErr" }
};


static const bits snmp_close_bits[] = {
    { int_SNMP_ClosePDU_goingDown,		"goingDown" },
    { int_SNMP_ClosePDU_unsupportedVersion,	"unsupportedVersion" },
    { int_SNMP_ClosePDU_packetFormat,		"packetFormat" },
    { int_SNMP_ClosePDU_protocolError,		"protocolError" },
    { int_SNMP_ClosePDU_internalError,		"internalError" },
    { int_SNMP_ClosePDU_authenticationFailure,	"authenticationFailure" },
};

const bits snmp_trace_types[] = {
    { TR_SNMP_RECV,	"receive" },
    { TR_SNMP_REGISTER,	"register" },
    { TR_SNMP_RESOLVE,	"resolve" },
    { TR_SNMP_TRAP,	"trap" },
    { 0, NULL }
};

static void
snmp_tree_reset __PF0(void)
{
    register struct snmp_tree *srp;
    
    /* reset registration bits on tree(s) */
    TREE_LIST(srp, &snmp_trees) {
	BIT_RESET(srp->r_flags, SMUX_TREE_REGISTERED);
    } TREE_LIST_END(srp, &snmp_trees) ;

    snmp_tree_next = (struct snmp_tree *) 0;
}


static void
snmp_close __PF2(tp, task *,
		 reason, int)
{
    tracef("snmp_close: SEND close %s",
	   trace_state(snmp_close_bits, reason));

    snmp_cfail_warn = 0;

    if (smux_close(reason) == NOTOK) {
	trace_log_tp(tp,
		     0,
		     LOG_WARNING,
		     (": Error: %s[%s], %m",
		      smux_error(smux_errno),
		      smux_info));
    } else {
	trace_tp(tp,
		 TR_STATE,
		 0,
		 (NULL));
    }
}


static void
snmp_terminate __PF1(tp, task *)
{
    if (tp->task_socket >= 0) {
	snmp_close(tp, goingDown);
	task_reset_socket(tp);
    }
    task_delete(tp);
    snmp_task = (task *) 0;
    snmp_timer_startup = (task_timer *) 0;

    snmp_tree_reset();

    /* maybe the commit never arrived */
    if (savedVarBinds) {
	free_SNMP_VarBindList(savedVarBinds);
	savedVarBinds = (struct type_SNMP_VarBindList *) 0;
    }

    if (BIT_TEST(task_state, TASKS_TERMINATE)) {
	trace_freeup(snmp_trace_options);
    }
}


static void
snmp_method __PF5(tp, task *,
		  op, const char *,
		  pdu, register struct type_SNMP_GetResponse__PDU *,
		  vp, register struct type_SNMP_VarBindList *,
		  offset, int)
{
    int	idx;
    int	status;
    int done;
    int error__status = int_SNMP_error__status_noError;
    object_instance ois;
    IFP	    method = (IFP) 0;

    idx = 0;
    for (done = 0; vp && !done; vp = vp->next) {
	register OI	oi;
	register OT	ot;
	register struct type_SNMP_VarBind *v = vp->VarBind;

	idx++;

	trace_tp(tp,
		 TR_SNMP_RECV,
		 0,
		 ("snmp_method: processing %d %s %s",
		  snmp_quantum,
		  op,
		  oid2ode(v->name)));
	if (offset == type_SNMP_SMUX__PDUs_get__next__request) {
	    if ((oi = name2inst(v->name)) == NULLOI && (oi = next2inst(v->name)) == NULLOI) {
		error__status = int_SNMP_error__status_noSuchName;
		break;		/* fall out of for loop */
	    }

	    if ((ot = oi->oi_type)->ot_getfnx == NULLIFP) {
		goto get_next;
	    }
	} else {
	    if ((oi = name2inst(v->name)) == NULLOI) {
		error__status = int_SNMP_error__status_noSuchName;
		break;		/* fall out of for loop */
	    }
	    ot = oi->oi_type;
	    if (offset == type_SNMP_SMUX__PDUs_get__request) {	/* GET */
		if (ot->ot_getfnx == NULLIFP) {
		    error__status = int_SNMP_error__status_noSuchName;
		    break;		/* fall out of for loop */
		}
	    } else {						/* SET */
		if (ot->ot_setfnx == NULLIFP) {
#ifdef notdef
		    /* internet theory on this case is that if we
		     * do not have WRITE permission on this object,
		     * then it is not in our "VIEW", and therefor,
		     * we must return noSuchName.
		     * We have left this code here for informational
		     * purposes. (RLF)
		     */

		    if (ot->ot_access & OT_RDONLY || ot->ot_access & OT_WRONLY) {
			error__status = int_SNMP_error__status_readOnly;
		    } else {
			error__status = int_SNMP_error__status_noSuchName;
 		    }
#else	/* notdef */
		    error__status = int_SNMP_error__status_noSuchName;
#endif	/* notdef */
		    break;		/* fall out of for loop */
		}
	    }
	}

try_again: ;
	switch (offset) {
	case type_SNMP_SMUX__PDUs_get__request:
	    if (!(ot->ot_access & OT_RDONLY)) {
		error__status = int_SNMP_error__status_noSuchName;
		done = 1;	/* fall out of for loop */
		continue;	/* drop out of do-while */
	    }
	    method = ot->ot_getfnx;
	    break;
		
	case type_SNMP_SMUX__PDUs_get__next__request:
	    if (!(ot->ot_access & OT_RDONLY)) {
		goto get_next;
	    }
	    method = ot->ot_getfnx;
	    break;
		
	case type_SNMP_PDUs_commit:
	case type_SNMP_PDUs_rollback:
	case type_SNMP_SMUX__PDUs_set__request:
	    if (!(ot->ot_access & OT_WRONLY)) {
#ifdef notdef
		if (!(ot->ot_access & OT_RDONLY)) {
		    error__status = int_SNMP_error__status_noSuchName;
		} else  {
		    error__status = int_SNMP_error__status_readOnly;
 		}
#else	/* notdef */
		error__status = int_SNMP_error__status_readOnly;
#endif	/* notdef */
		done = 1;	/* fall out of for loop */
		continue;	/* drop out of do-while */
	    }
	    method = ot->ot_setfnx;
	    break;
	}

	status = (*method)(oi, v, offset);
	switch (status) {
	case NOTOK:	    /* get-next wants a bump */
	get_next:
	    oi = &ois;
	    for (;;) {
		if ((ot = ot->ot_next) == NULLOT) {
		    error__status = int_SNMP_error__status_noSuchName;
		    done = 1;
		    break;
		}
		oi->oi_name = (oi->oi_type = ot)->ot_name;
		if (ot->ot_getfnx) {
		    goto try_again;
		}
	    }
	    break;

	case int_SNMP_error__status_noError:
	    break;
	    
	default:
	    error__status = status;
	    done = 1;
	    break;
	}

	if (TRACE_TP(tp, TR_SNMP_RECV)) {
	    tracef("snmp_method: completed %d %s",
		   snmp_quantum,
		   oid2ode(v->name));

	    switch (error__status) {
	    default:
		tracef(": %s",
		       trace_state(snmp_error_bits, error__status));
		break;
	    
	    case int_SNMP_error__status_noError:
		/* XXX - OID and print value */
		break;
	    }
	    trace_only_tp(tp,
			  0,
			  (NULL));
	}
    }

    
    switch (offset) {
    case type_SNMP_PDUs_rollback:
    case type_SNMP_PDUs_commit:
	/* No response expected */
	break;

    default:
	/* Generate a response */
	switch (error__status) {
	case int_SNMP_error__status_noError:
	    pdu->error__index = 0;
	    break;

	default:
	    pdu->error__index = idx;
	}
	pdu->error__status = error__status;

	if (smux_response(pdu) == NOTOK) {
	    trace_log_tp(tp,
			 0,
			 LOG_WARNING,
			 ("snmp_method: smux_response: Error: %s [%s], %m",
			  smux_error(smux_errno),
			  smux_info));
	    snmp_restart(tp);
	}
    }

}


/*
 *	send a smux register request.
 *	returns:
 *		OK 	== a register request was sent
 *		NOTOK	== nothing left to register, <or>
 *			problems sending the request.
 *			Check smux_state to know which failed.
 *			smux_state & SMUX_CONNECTED == nothing to register
 */

static int
snmp_register __PF2(tp, task *,
		    failure, int)
{
    int mode;
    const char *cmode;

 try_again:
    /* Find something that needs to be registered */
    if (snmp_tree_next) {
	if (failure) {
	    /* Registration failed, remove it from the queue */
	    register struct snmp_tree *drp = snmp_tree_next;

	    snmp_tree_next = drp->r_back;
	    REMQUE(drp);
	    drp->r_forw = drp->r_back = (struct snmp_tree *) 0;
	} else {
	    BIT_FLIP(snmp_tree_next->r_flags, SMUX_TREE_REGISTERED);
	}
    } else {
	snmp_tree_next = &snmp_trees;
    }

    /* Find some work to do */
    TREE_LIST(snmp_tree_next, snmp_tree_next) {
	if (BIT_MATCH(snmp_tree_next->r_flags, SMUX_TREE_REGISTER)
	    != BIT_MATCH(snmp_tree_next->r_flags, SMUX_TREE_REGISTERED)) {
	    break;
	}
    } TREE_LIST_END(snmp_tree_next, snmp_tree_next) ;
    
    if (!snmp_tree_next->r_text) {
	/* No more work to do */
	snmp_tree_next = (struct snmp_tree *) 0;
	return OK;
    }

    /* What are we doing? */
    if (BIT_TEST(snmp_tree_next->r_flags, SMUX_TREE_REGISTERED)) {
	/* Unregistering */
	
	mode = delete;
	cmode = "delete";
    } else {
	/* Registering */
	
	mode = snmp_tree_next->r_mode;
	cmode = snmp_tree_next->r_mode == readWrite ? "readWrite" : "readOnly";
    }

    tracef("snmp_register: REGISTER %s %s",
	   cmode,
	   oid2ode(snmp_tree_next->r_name));

    if (BIT_TEST(task_state, TASKS_TEST)) {
	trace_tp(tp,
		 TR_SNMP_REGISTER,
		 0,
		 (NULL));
	goto try_again;
    }
    
    if (smux_register(snmp_tree_next->r_name,
		      -1,
		      mode) == NOTOK) {
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     (": %s [%s]",
		      smux_error(smux_errno),
		      smux_info));

	snmp_restart(tp);
	return NOTOK;
    }

    trace_tp(tp,
	     TR_SNMP_REGISTER,
	     0,
	     (NULL));
    return OK;
}


static void
snmp_recv __PF1(tp, task *)
{
    struct type_SNMP_SMUX__PDUs *event;

    if (smux_wait(&event, NOTOK) == NOTOK) {
	if (smux_errno == inProgress) {
	    return;
	}
	
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     ("snmp_recv: smux_wait: %s [%s] (%m)",
		      smux_error(smux_errno),
		      smux_info));
	snmp_restart(tp);
	return;
    }

    switch (event->offset) {
    case type_SNMP_SMUX__PDUs_registerResponse:
	{
	    struct type_SNMP_RRspPDU *rsp = event->un.registerResponse;

	    tracef("snmp_recv: registerResponse");
	    if (snmp_tree_next && snmp_tree_next->r_name) {
		tracef(" (%s)",
		       oid2ode(snmp_tree_next->r_name));
	    }
	    trace_tp(tp,
		     TR_SNMP_REGISTER,
		     0,
		     (": %s",
		      trace_state(snmp_error_bits, rsp->parm)));

	    snmp_register(tp, rsp->parm == int_SNMP_RRspPDU_failure);
	}
	break;

    case type_SNMP_SMUX__PDUs_get__request:
	snmp_quantum = event->un.get__request->request__id;
	snmp_method(tp,
		    "get",
		    event->un.get__response, 
		    event->un.get__request->variable__bindings,
		    event->offset);
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	snmp_quantum = event->un.get__next__request->request__id;
	snmp_method(tp,
		    "getNext",
		    event->un.get__response, 
		    event->un.get__next__request->variable__bindings,
		    event->offset);
	break;

    case type_SNMP_SMUX__PDUs_set__request:
	snmp_quantum = event->un.set__request->request__id;
	snmp_method(tp,
		    "set",
		    event->un.get__response, 
		    event->un.set__request->variable__bindings,
		    event->offset);

	/* maybe the commit never arrived */
	if (savedVarBinds) {
	    free_SNMP_VarBindList(savedVarBinds);
	    savedVarBinds = (struct type_SNMP_VarBindList *) 0;
	}

	/* save the varBinds till the commit/rollback arrives */
	savedVarBinds = event->un.get__request->variable__bindings;

	/* dont let smux_wait() free these varBinds */
	event->un.get__request->variable__bindings = (struct type_SNMP_VarBindList *) 0;
	break;

    case type_SNMP_SMUX__PDUs_commitOrRollback:
	{
	    int commitOrRollback = event->un.commitOrRollback->parm;

	    snmp_method(tp,
			commitOrRollback == int_SNMP_SOutPDU_commit ? "commit" : "rollback",
			NULL, savedVarBinds,
			commitOrRollback == int_SNMP_SOutPDU_commit ?
			type_SNMP_PDUs_commit : type_SNMP_PDUs_rollback);
	    free_SNMP_VarBindList(savedVarBinds);
	    savedVarBinds = NULL;
	    break;
	}
	    
    case type_SNMP_SMUX__PDUs_close:
	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("snmp_recv: close: %s",
		  trace_state(snmp_close_bits, event->un.close->parm)));
	snmp_restart(tp);
	break;

    case type_SNMP_SMUX__PDUs_simple:
    case type_SNMP_SMUX__PDUs_registerRequest:
    case type_SNMP_SMUX__PDUs_get__response:
    case type_SNMP_SMUX__PDUs_trap:
	trace_log_tp(tp,
		     0,
		     LOG_WARNING,
		     ("snmp_recv: unexpectedOperation: %d",
		      event->offset));
	snmp_close(tp, protocolError);
	snmp_restart(tp);
	break;

    default: 
	trace_log_tp(tp,
		     0,
		     LOG_WARNING,
		     ("snmp_recv: badOperation: %d",
		      event->offset));
	snmp_close(tp, protocolError);
	snmp_restart(tp);
	break;
    }
}


/*
 *	startup the registration process, stop paying attention to
 *	writes on the socket.
 */
static void
snmp_startup __PF1(tp, task *)
{
    tracef("snmp_startup: OPEN %s \"%s\"", 
	   oid2ode(&se->se_identity),
	   se->se_name);

    if (smux_simple_open(&se->se_identity,
			 my_desc,
			 se->se_password,
			 strlen(se->se_password)) == NOTOK) {
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     (": smux_simple_open: %s [%s]",
		      smux_error(smux_errno),
		      smux_info));
	snmp_restart(tp);
	return;
    }

    trace_tp(tp,
	     TR_STATE,
	     0,
	     (NULL));

    if (snmp_register(tp, FALSE) == NOTOK) {
	return;
    }
    
    task_set_connect(tp, (void (*) ()) 0);
    BIT_RESET(tp->task_flags, TASKF_CONNECT);

    task_set_socket(tp, tp->task_socket);
}


/* 
 * connect to the SMUX tcp port.
 * Initializing tcp port, and smux library internals are
 * intermixed here... so we let the smux library handle everything.
 * { instead of using task_get_socket() to open the port, which
 *   would be the proper way to do things.... }.
 */
static int
snmp_connect __PF1(tp, task *)
{
    tracef("snmp_connect: CONNECT%s",
	   snmp_debug ? " (debug)" : "");

    if (BIT_TEST(task_state, TASKS_TEST)) {
	tp->task_socket = task_get_socket(tp, PF_INET, SOCK_STREAM, 0);
	assert(tp->task_socket >= 0);
    } else {
	if ((tp->task_socket = smux_init(debug = snmp_debug)) < 0) { 
	    if (smux_errno != systemError
		|| errno != ECONNREFUSED
		|| snmp_cfail_warn++ == 0) {
	    trace_log_tp(tp,
			 0,
			 LOG_WARNING,
			 (": smux_init: %s [%s] (%m)",
			  smux_error(smux_errno),
			  smux_info));
	    }
	    return NOTOK;
	}
	task_set_recv(tp, snmp_recv);
	task_set_connect(tp, snmp_startup);
	BIT_SET(tp->task_flags, TASKF_CONNECT);
    }

    trace_tp(tp,
	     TR_STATE,
	     0,
	     (NULL));
    
    task_set_socket(tp, tp->task_socket);

    if (snmp_timer_startup) {
	task_timer_delete(snmp_timer_startup);
	snmp_timer_startup = (task_timer *) 0;
    }

    return OK;
}


static void
snmp_job __PF2(tip, task_timer *,
	       interval, time_t)
{
    snmp_connect(tip->task_timer_task);
}


static void
snmp_cleanup __PF1(tp, task *)
{
    trace_freeup(tp->task_trace);
    trace_freeup(snmp_trace_options);
}


void
snmp_restart __PF1(tp, task *)
{
    trace_tp(tp,
	     TR_STATE,
	     0,
	     ("snmp_restart: RESTART"));

    /* Reset things */
    if (tp->task_socket != -1) {
	task_reset_socket(tp);
    }
    task_set_recv(tp, (void (*) ()) 0);
    task_set_connect(tp, (void (*) ()) 0);
    BIT_RESET(tp->task_flags, TASKF_CONNECT);
    snmp_cfail_warn = 0;
    snmp_tree_reset();

    /* a constant 60sec timer, to try to startup smux again */
    if (snmp_timer_startup) {
	task_timer_set(snmp_timer_startup,
		       (time_t) 60,
		       (time_t) 0);
    } else {
	snmp_timer_startup = task_timer_create(tp,
					       "Startup",
					       (flag_t) 0, 
					       (time_t) 60,
					       (time_t) 0,
					       snmp_job,
					       (void_t) 0);
    }
}


static void
snmp_dump __PF2(tp, task *,
		fp, FILE *)
{
    struct snmp_tree *tree;

    (void) fprintf(fp,
		   "\tDebugging: %d\tPort: %d\tPreference: %d\n",
		   snmp_debug,
		   ntohs(snmp_port),
		   snmp_preference);

    (void) fprintf(fp,
		   "\tMIB trees active:\n\n");

    TREE_LIST(tree, &snmp_trees) {
	(void) fprintf(fp,
		       "\t%s\t%s\tflags: <%s>\n",
		       tree->r_mode == readWrite ? "readWrite" : "readOnly",
		       tree->r_text,
		       trace_bits(snmp_flag_bits, tree->r_flags));
	if (BIT_TEST(tree->r_flags, SMUX_TREE_OBJECTS)) {
	    register const struct object_table *op = tree->r_table;
	    
	    while (op->ot_object) {
		(void) fprintf(fp,
			       "\t\t%s\t%s\n",
			       op->ot_setfunc ? "readWrite" : "readOnly",
			       op->ot_object);
		op++;
	    }
	    (void) fprintf(fp, "\n");
	}
    } TREE_LIST_END(tree, &snmp_trees) ;
}


void
snmp_var_init __PF0(void)
{
    doing_snmp = TRUE;
    snmp_preference = RTPREF_SNMP;
    snmp_debug = FALSE;
    snmp_port = 0;
}


/*
 *	from-the-top initialization of the SMUX task.
 *	should be called 'smux_init', but conflicts with the smux 
 *	library initialization routine...
 */
void
snmp_init __PF0(void)
{
    static int once_only = TRUE;

    /* 
     * should really be more graceful here.
     * for now, if a re-init, shutdown smux first, 
     * then start all over.
     */
    if (snmp_task) {
	snmp_terminate(snmp_task);	/* shut it down */
    }

    if (!doing_snmp) {
	/* We've been turned off! */
	return;
    }

    /* Set tracing */
    trace_inherit_global(snmp_trace_options, snmp_trace_types, (flag_t) 0);
    
    /* lookup gated in snmpd.peers. */
    if ((se = getsmuxEntrybyname("gated")) == NULL) {
	trace_log_tf(snmp_trace_options,
		     0,
		     LOG_WARNING,
		     ("no SMUX entry for %s.  Shutting down SMUX",
		      "gated"));
	doing_snmp = 0;
	return;
    }

    /* one-time configuration */
    if (once_only) {
	once_only = FALSE;
#if	defined(IBM_LIBSMUX)
	/* init internal database */
	if (init_objects() == NOTOK) {
	    trace_log_tf(snmp_trace_options,
			 0,
			 LOG_WARNING, 
			 ("snmp_init: init_objects: %s.  Shutting down SMUX", 
			  PY_pepy));
	    doing_snmp = 0;
	    return;
	}
#else	/* defined(IBM_LIBSMUX) */
#ifdef	ISODE_SNMP_NODEFS
	if (readobjects("./gated.defs") == NOTOK &&
	    readobjects(_PATH_DEFS) == NOTOK &&
	    readobjects("gated.defs") == NOTOK) {
	    trace_log_tf(snmp_trace_options,
			 0,
			 LOG_WARNING,
			 ("snmp_init: readobjects: %s.  Shutting down SMUX",
			  PY_pepy));
	    doing_snmp = 0;
	    return;
	}
#else	/* ISODE_SNMP_NODEFS */
	if (loadobjects((char *) 0) == NOTOK) {
	    trace_log_tf(snmp_trace_options,
			 0,
			 LOG_WARNING,
			 ("snmp_init: loadobjects: %s.  Shutting down SMUX",
			  PY_pepy));
	    doing_snmp = 0;
	    return;
	}
#endif	/* ISODE_SNMP_NODEFS */
#endif	/* defined(IBM_LIBSMUX) */

	if ((snmp_nullSpecific = text2oid("0.0")) == NULLOID) {
	    trace_log_tf(snmp_trace_options,
			 0,
			 LOG_WARNING,
			 ("snmp_init: text2oid(\"0.0\") failed!"));
	    doing_snmp = 0;
	    return;
	}

	sprintf(my_desc, "SMUX GATED version %s, built %s",
		gated_version, 
		build_date);
    }

    /* setup a task to handle smux */
    snmp_task = task_alloc("SMUX",
			   TASKPRI_NETMGMT,
			   snmp_trace_options);
    snmp_task->task_addr = sockdup(inet_addr_loopback);
    if (!snmp_port) {
	snmp_port = task_get_port(snmp_trace_options,
				  "smux", "tcp",
				  htons(SMUX_PORT));
    }
    sock2port(snmp_task->task_addr) = snmp_port;
    task_set_terminate(snmp_task, snmp_terminate);
    task_set_dump(snmp_task, snmp_dump);
    task_set_cleanup(snmp_task, snmp_cleanup);
    snmp_task->task_rtproto = RTPROTO_SNMP;
    BIT_SET(snmp_task->task_flags, TASKF_LOWPRIO);

    if (!task_create(snmp_task)) {
	task_quit(errno);
    }

    if (snmp_connect(snmp_task) == NOTOK) {
	snmp_restart(snmp_task);
    }
}


void
snmp_trap __PF5(name, const char *,
		enterprise, OID,
		generic, int,
		specific, int,
		bindings, struct type_SNMP_VarBindList *)
{
    if (!snmp_task || snmp_task->task_socket == -1) {
	return;
    }

    if (!enterprise) {
	if (generic == int_SNMP_generic__trap_enterpriseSpecific) {
	    /* Can not generate enterprise specified trap with no enterprise */
	    return;
	}
	enterprise = &se->se_identity;
#if	!defined(SMUX_TRAP_HAS_ENTERPRISE) && !defined(IBM_6611)
    } else {
	/* Can not generate an enterprise specific trap */
	return;
#endif	/* !defined(SMUX_TRAP_HAS_ENTERPRISE) && !defined(IBM_6611) */
    }
    
    tracef("snmp_trap: TRAP enterprise %s type %s (generic %u specific %u)",
	   oid2ode(enterprise),
	   name,
	   generic,
	   specific);

#if	defined(SMUX_TRAP_HAS_ENTERPRISE)
#define	SMUX_TRAP(e, g, s, b)	smux_trap(e, g, s, b)
#else	/* SMUX_TRAP_HAS_ENTERPRISE */	   
#ifdef	IBM_6611
#define	SMUX_TRAP(e, g, s, b)	smux_trap_enterprise(g, s, b, e)
#else	/* IBM_6611 */
#define	SMUX_TRAP(e, g, s, b)	smux_trap(g, s, b)
#endif	/* IBM_6611 */
#endif	/* SMUX_TRAP_HAS_ENTERPRISE */

    if (SMUX_TRAP(enterprise, generic, specific, bindings) == NOTOK) {
	trace_log_tp(snmp_task,
		     0,
		     LOG_WARNING,
		     (": smux_trap: %s [%s] %m",
		      smux_error(smux_errno),
		      smux_info));
	snmp_restart(snmp_task);
	return;
    }

    trace_tp(snmp_task,
	     TR_SNMP_TRAP,
	     0,
	     (NULL));
}


int	
oid2mediaddr __PF4(ip, register unsigned int *,
		   addr, register byte *,
		   len, int,
		   islen, int)
{
    register int i;

    if (islen) {
	len = *ip++;
    } else {
	len = len ? len : 4;		/* len, else ipaddress, is default */
    }

    for (i = len; i > 0; i--) {
	*addr++ = *ip++;
    }

    return len + (islen ? 1 : 0);
}


static int
snmp_tree_resolve __PF1(tree, struct snmp_tree *)
{
    register int fails = 0;
    register int count = 0;
    register OT ot;
    register struct object_table *op = tree->r_table;

    /* Translate tree name */
    ot = text2obj((char *)tree->r_text);
    if (ot) {
	tree->r_name = ot->ot_name;

	trace_tf(snmp_trace_options,
		 TR_SNMP_RESOLVE,
		 0,
		 ("snmp_tree_resolve: RESOLVE %s OID %s",
		  tree->r_text,
		  sprintoid(tree->r_name)));
    } else {
	trace_log_tf(snmp_trace_options,
		     0,
		     LOG_WARNING,
		     ("snmp_tree_resolve: tree %s unable to resolve root",
		      tree->r_text));
	return TRUE;
    }

    /* Translate object names */
    while (op->ot_object) {
	count++;
	ot = op->ot_type = text2obj((char *)op->ot_object);
	if (ot) {
	    ot->ot_getfnx = op->ot_getfunc;
	    if (op->ot_setfunc) {
		ot->ot_setfnx = op->ot_setfunc;
	    }
	    ot->ot_info = (caddr_t) op;

	    trace_tf(snmp_trace_options,
		     TR_SNMP_RESOLVE,
		     0,
		     ("snmp_tree_resolve: RESOLVE %s OID %s",
		      op->ot_object,
		      sprintoid(ot->ot_name)));
	} else {
	    trace_log_tf(snmp_trace_options,
			 0,
			 LOG_WARNING,
			 ("snmp_tree_resolve: tree %s unable to resolve object %s",
			  tree->r_text,
			  op->ot_object));
	    fails++;
	}

	op++;
    }

    if (count > fails) {
	BIT_SET(tree->r_flags, SMUX_TREE_OBJECTS);
	trace_tf(snmp_trace_options,
		 TR_SNMP_RESOLVE,
		 0,
		 ("snmp_tree_resolve: RESOLVE tree %s %d of %d objects",
		  tree->r_text,
		  count - fails,
		  count));
    } else {
	trace_log_tf(snmp_trace_options,
		     0,
		     LOG_WARNING,
		     ("snmp_tree_resolve: tree %s could not resolve any of %d objects",
		      tree->r_text,
		      count));

	return TRUE;
    }

    return FALSE;
}


void
snmp_tree_register __PF1(tree, struct snmp_tree *)
{
    if (!BIT_TEST(tree->r_flags, SMUX_TREE_OBJECTS)) {
	/* If this is the first reference to this tree, translate the names into OID's */

	if (snmp_tree_resolve(tree)) {
	    /* Could not resolve tree, ignore it */
	    return;
	}
    }

    BIT_SET(tree->r_flags, SMUX_TREE_REGISTER);

    if (!BIT_TEST(tree->r_flags, SMUX_TREE_REGISTERED)) {
	/* Register tree */
	
	trace_tf(snmp_trace_options,
		 TR_SNMP_REGISTER,
		 0,
		 ("snmp_tree_register: ADD tree %s  mode %s",
		  tree->r_text,
		  tree->r_mode == readOnly ? "readOnly" : "readWrite"));

	if (!tree->r_forw) {
	    /* Insert into queue */
	    INSQUE(tree, snmp_trees.r_back);
	}

	if (snmp_task && snmp_task->task_socket != -1 && !snmp_task->task_connect_method && !snmp_tree_next) {
	    /* Start registration process */
	    snmp_register(snmp_task, FALSE);
	}
    }
}

void
snmp_tree_unregister __PF1(tree, struct snmp_tree *)
{
    trace_tf(snmp_trace_options,
	     TR_SNMP_REGISTER,
	     0,
	     ("snmp_tree_unregister: DELETE tree %s",
	      tree->r_text));
    
    BIT_RESET(tree->r_flags, SMUX_TREE_REGISTER);

    if (BIT_TEST(tree->r_flags, SMUX_TREE_REGISTERED)) {
	/* Unregister the tree */

	if (!tree->r_forw) {
	    /* Insert into queue */
	    INSQUE(tree, snmp_trees.r_back);
	}

	if (snmp_task && snmp_task->task_socket != -1 && !snmp_task->task_connect_method && !snmp_tree_next) {
	    /* Start un-registration process */
	    snmp_register(snmp_task, FALSE);
	}
    }
}


int
snmp_last_match __PF4(last, unsigned int **,
		      oid, register unsigned int *,
		      len, u_int,
		      isnext, int)
{
    register unsigned int *lp = *last;
    register unsigned int *ip = oid;
    int last_len, last_isnext;
    unsigned int *llp;

    if (*last) {
	last_len = *lp++;

	if (last_len == len) {
	    last_isnext = *lp++;

	    if (last_isnext == isnext) {
		llp = lp + last_len;
		
		while (lp < llp) {
		    if (*lp++ != *ip++) {
			ip = oid;
			goto free_up;
		    }
		}

		return TRUE;
	    }
	}

    free_up:
	snmp_last_free(last);
    }

    /* XXX - Could we figure out the max size? */
    *last = lp = (unsigned int *) task_mem_malloc((task *) 0, (len + 2) * sizeof (int));
    llp = lp + len + 2;
    
    *lp++ = len;
    *lp++ = isnext;

    while (lp < llp) {
	*lp++ = *ip++;
    }

    return FALSE;
}


/**/

int
snmp_varbinds_build (n, vb, v, var_list, tree, build, vp)
int n;
struct type_SNMP_VarBindList *vb;
struct type_SNMP_VarBind * v;
int *var_list;
struct snmp_tree *tree;
_PROTOTYPE(build,
	   int,
	   (OI,
	    struct type_SNMP_VarBind *,
	    int,
	    void_t));
void_t vp;
{
    register int i;
    struct object_instance oia;
    OI oi = &oia;

    /* Clear the VarBinds and VarBindLists */
    for (i = 0; i < n; i++) {
	v[i].name = (type_SNMP_ObjectName *) 0;
	v[i].value = (struct type_SNMP_ObjectSyntax *) 0;
	vb[i].VarBind = (struct type_SNMP_VarBind *) 0;
	vb[i].next = (struct type_SNMP_VarBindList *) 0;
    }
    
    if (!BIT_TEST(tree->r_flags, SMUX_TREE_OBJECTS|SMUX_TREE_REGISTERED)) {
	/* Objects not resolved or tree not registered */
	return NOTOK;
    }
    
    for (i = 0; i < n; i++, vb++, v++) {
	register const struct object_table *ot = &tree->r_table[var_list[i]];

	if (!ot->ot_type) {
	    /* MIB not inited */
	    return NOTOK;
	}
	oi->oi_type = ot->ot_type;
	oi->oi_name = oi->oi_type->ot_name;

	v->name = oi->oi_name;

	if (build(oi, v, ot->ot_info, vp) == NOTOK) {
	    return NOTOK;
	}

	vb->VarBind = v;
	if (i) {
	    vb[-1].next = vb;
	}
    }

    return OK;
}


void
snmp_varbinds_free __PF1(vb, register struct type_SNMP_VarBindList *)
{
    for (;
	 vb && vb->VarBind;
	 vb = vb->next) {
	free_SNMP_ObjectSyntax(vb->VarBind->value);
    }
}
#endif		/* defined(PROTO_ISODE_SNMP) */
