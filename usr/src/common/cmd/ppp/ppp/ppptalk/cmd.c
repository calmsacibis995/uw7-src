#ident	"@(#)cmd.c	1.15"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#include <locale.h>
#include <dial.h>
#include <sys/ppp.h>
#include <sys/ppp_psm.h>

#include "fsm.h"
#include "psm.h"
#include "ppp_proto.h"
#include "ppp_cfg.h"
#include "ppptalk.h"
#include "ulr.h"
#include "act.h"
#include "auth.h"
#include "pathnames.h"
#include "ppptalk_msg.h"
#include "edit.h"

extern int so, ppp_debug;
extern struct str2val_s def_types[];
extern struct str2val_s debug_levels[];
extern struct str2val_s def_link_types[];
extern struct str2val_s truefalse[];
extern struct str2val_s def_link_flow[];
extern struct str2val_s def_chap_alg[];
extern struct str2val_s def_bringup_types[];
extern struct str2val_s bod_types[]; 
extern struct str2val_s auth_protos[];
extern struct str2val_s bundle_types[]; 

FILE *gfp;	/* Global output file pointer */
int verbose = PSM_V_SHORT;	/* Verbosity for stats/status */
	
char *gettok(char *p, char *tok);
struct psm_entry_s *cp_get_tab(char *proto);
char *getinput(FILE *fp, char *prompt);
void pmsg(int level, int msgno, char *fmt, ...);
extern struct ulr_prim *gmsg;

/*
 * Output the link specific options
 */		
void
list_link(FILE *fp, struct cfg_link *l)
{
	int i;

	fprintf(fp, "link %s {\n", l->ln_ch.ch_id);

	fprintf(fp, "\tdev = %s\n", def_get_str(&(l->ln_ch), l->ln_dev));

	for(i = 0; def_link_types[i].tv_str && 
		    (uint_t)def_link_types[i].tv_val != l->ln_type; i++);
	fprintf(fp, "\ttype = %s\n", def_link_types[i].tv_str);
	fprintf(fp, "\tphone = %s\n", def_get_str(&(l->ln_ch), l->ln_phone));
	fprintf(fp, "\tpush = %s\n", def_get_str(&(l->ln_ch), l->ln_push));
	fprintf(fp, "\tpop = %s\n", def_get_str(&(l->ln_ch), l->ln_pop));
	fprintf(fp, "\tprotocols = %s\n",
	       def_get_str(&(l->ln_ch), l->ln_protocols));
	fprintf(fp, "\tbandwidth = %d\n", l->ln_bandwidth);
	for (i = 0; def_link_flow[i].tv_str && def_link_flow[i].tv_val != (void *)l->ln_flow; i++) ;

	fprintf(fp, "\tflow = %s\n", def_link_flow[i].tv_str);
	for(i = 0; debug_levels[i].tv_str && 
		    (uint_t)debug_levels[i].tv_val != l->ln_debug; i++);
	fprintf(fp, "\tdebug = %s\n", debug_levels[i].tv_str);
	fprintf(fp, "}\n");
}

/*
 * Output the protocol specific options, calling the psm list rountine.
 */		
void
list_proto(FILE *fp, struct cfg_proto *p)
{
	struct psm_entry_s *e;
	char *proto_desc = def_get_str(&p->pr_ch, p->pr_name);

	/* Call the protocol specifiec list routine */

	e = cp_get_tab(proto_desc);
	if (!e) {
		pmsg(MSG_ERROR, PPPTALK_PROTONAVAIL,
		     "Support for protocol %s not available\n", proto_desc);
		return;
	}

	if (*e->psm_list) {
		fprintf(fp, "protocol %s {\n", p->pr_ch.ch_id);
		(*e->psm_list)(fp, p);
		fprintf(fp, "}\n");
	}
}

/*
 * Output the algorithm specific options, calling the psm list rountine.
 */		
void
list_alg(FILE *fp, struct cfg_alg *p)
{
	struct psm_entry_s *e;
	char *alg_desc = def_get_str(&p->al_ch, p->al_name);

	/* Call the protocol specifiec list routine */

	e = cp_get_tab(alg_desc);
	if (!e) {
		pmsg(MSG_ERROR, PPPTALK_ALGNAVAIL,
		     "Support for algorithm %s not available\n", alg_desc);
		return;
	}

	if (*e->psm_list) {
		fprintf(fp, "algorithm %s {\n", p->al_ch.ch_id);
		(*e->psm_list)(fp, p);
		fprintf(fp, "}\n");
	}
}

/*
 * Output the bundle specific options.
 */		
void
list_bundle(FILE *fp, struct cfg_bundle *bn)
{
	int i;

	fprintf(fp, "bundle %s {\n", bn->bn_ch.ch_id);
	for(i = 0; bundle_types[i].tv_str && 
		    (uint_t)bundle_types[i].tv_val != bn->bn_type; i++);
	fprintf(fp, "\ttype = %s\n", bundle_types[i].tv_str);

	fprintf(fp, "\tprotocols = %s\n",
	       def_get_str(&(bn->bn_ch), bn->bn_protos));
	fprintf(fp, "\tmrru = %d\n", bn->bn_mrru);
	fprintf(fp, "\tssn = %s\n", def_get_bool(bn->bn_ssn));
	fprintf(fp, "\tmaxlinks = %d\n", bn->bn_maxlinks);
	fprintf(fp, "\tminlinks = %d\n", bn->bn_minlinks);
	fprintf(fp, "\tlinks = %s\n",
	       def_get_str(&(bn->bn_ch), bn->bn_links));
	fprintf(fp, "\ted = %s\n", def_get_bool(bn->bn_ed));
	fprintf(fp, "\tminfrag = %d\n", bn->bn_minfrag);
	fprintf(fp, "\tmaxfrags = %d\n", bn->bn_maxfrags);
	fprintf(fp, "\taddload = %d\n", bn->bn_addload);
	fprintf(fp, "\tdropload = %d\n", bn->bn_dropload);
	fprintf(fp, "\taddsample = %d\n", bn->bn_addsample);
	fprintf(fp, "\tdropsample = %d\n", bn->bn_dropsample);
	fprintf(fp, "\tthrashtime = %d\n", bn->bn_thrashtime);
	fprintf(fp, "\tmaxidle = %d\n", bn->bn_maxidle);
	fprintf(fp, "\tlinkidle = %d\n", bn->bn_mlidle);
	fprintf(fp, "\tnulls = %s\n", def_get_bool(bn->bn_nulls));

	for(i = 0; bod_types[i].tv_str && 
		    (uint_t)bod_types[i].tv_val != bn->bn_bod; i++);
	fprintf(fp, "\tbod = %s\n", bod_types[i].tv_str);

	for(i = 0; debug_levels[i].tv_str && 
		    (uint_t)debug_levels[i].tv_val != bn->bn_debug; i++);
	fprintf(fp, "\tdebug = %s\n", debug_levels[i].tv_str);

	/* Incoming stuff */
	fprintf(fp, "\tlogin = %s\n", def_get_str(&(bn->bn_ch), bn->bn_uid));
	fprintf(fp, "\tcallerid = %s\n",
		def_get_str(&(bn->bn_ch), bn->bn_cid));
	fprintf(fp, "\tauthid = %s\n", 
		def_get_str(&(bn->bn_ch), bn->bn_authid));

	/* Outgoing stuff */
	fprintf(fp, "\tremotesys = %s\n",
	       def_get_str(&(bn->bn_ch), bn->bn_remote));
	fprintf(fp, "\tphone = %s\n",
	       def_get_str(&(bn->bn_ch), bn->bn_phone));
	fprintf(fp, "\tbringup = %s\n",
	       def_bringup_types[bn->bn_bringup - 1].tv_str);

	/* Authentication stuff */
	fprintf(fp, "\trequirepap = %s\n", def_get_bool(bn->bn_pap));
	fprintf(fp, "\trequirechap = %s\n", def_get_bool(bn->bn_chap));
	fprintf(fp, "\tauthname = %s\n", 
	       def_get_str(&(bn->bn_ch), bn->bn_authname));
	fprintf(fp, "\tpeerauthname = %s\n", 
	       def_get_str(&(bn->bn_ch), bn->bn_peerauthname));
	fprintf(fp, "\tauthtmout = %d\n", bn->bn_authtmout);

	fprintf(fp, "}\n");
}

/*
 * Output the authentication specific options.
 */		
void
list_auth(FILE *fp, struct cfg_auth *a)
{
	int i;

	fprintf(fp, "auth %s {\n", a->au_ch.ch_id);

	for(i = 0; auth_protos[i].tv_str && 
		    (uint_t)auth_protos[i].tv_val != a->au_protocol; i++);

	fprintf(fp, "\tprotocol = %s\n", auth_protos[i].tv_str);
	       
	fprintf(fp, "\tname = %s\n", 
	       def_get_str(&(a->au_ch), a->au_name));
	fprintf(fp, "\tpeersecret = %s\n",
	       def_get_str(&(a->au_ch), a->au_peersecret));
	fprintf(fp, "\tlocalsecret = %s\n",
	       def_get_str(&(a->au_ch), a->au_localsecret));
	fprintf(fp, "}\n");
}

/*
 * Output the global specific options.
 */		
void
list_globals(FILE *fp, struct cfg_global *gl)
{
	fprintf(fp, "global %s {\n", gl->gi_ch.ch_id);

	fprintf(fp, "\ttype = %s\n", def_types[gl->gi_type].tv_str);

	switch(gl->gi_type) {
	case DEF_BUNDLE:
		fprintf(fp, "\trequirepap = %s\n", def_get_bool(gl->gi_pap));
		fprintf(fp, "\trequirechap = %s\n", def_get_bool(gl->gi_chap));
		fprintf(fp, "\tauthname = %s\n", 
		       def_get_str(&(gl->gi_ch), gl->gi_authname));
		fprintf(fp, "\tpeerauthname = %s\n", 
		       def_get_str(&(gl->gi_ch), gl->gi_peerauthname));
		fprintf(fp, "\tauthtmout = %d\n", gl->gi_authtmout);
		fprintf(fp, "\tmrru = %d\n", gl->gi_mrru);
		fprintf(fp, "\tssn = %s\n", def_get_bool(gl->gi_ssn));
		fprintf(fp, "\ted = %s\n", def_get_bool(gl->gi_ed));
		break;
	default:
		printf("Not a supported type\n");
		break;
	}

	fprintf(fp, "}\n");
}

/*
 * This table provides a mapping of definition type
 * to the type specific list routine.
 */
void (*list_fn[])() = {
	list_globals,
	list_auth,
	list_alg,
	list_proto,
	list_link,
	list_bundle,
};

/*
 * List the configured ID's for the specified type of definition
 */
void
list_type(int type)
{
	struct ulr_list_cfg *lc = (struct ulr_list_cfg *)gmsg;
	int ret, i;
	struct list_ent *le;
	void *last_cookie;

	fprintf(gfp, "%-40.40s\n", def_types[type].tv_str);
	fprintf(gfp, "----------------------------------------\n");

	/* Initialise the message */
	lc->prim = ULR_LIST_CFG;
	lc->type = type;
	lc->entry.le_cookie = (void *)0; /* Start */

	for (;;) {

		ret = ulr_sr(so, lc, sizeof(struct ulr_list_cfg), ULR_MAX_MSG);
		if (ret) {
			printf("list_type: Error %d\n", ret);
			return;
		}

		if (lc->num == 0)
			return;

		le = &(lc->entry);

		for (i = 0; i < lc->num; i++) {
			fprintf(gfp, "%-40.40s\n", le->le_id);
			last_cookie = le->le_cookie;
			le++;
		}

		lc->entry.le_cookie = last_cookie;
	}
}

/*
 * Given the type and name, display the definitions details
 */
int
list_id(char *id, int type)
{
	struct ulr_cfg *m = (struct ulr_cfg *)gmsg;
	int ret;

	ret = cfg_findid(so, m, id, type);
	if (ret < 0)
		return 1;

	(*list_fn[type])(gfp, &(m->un));
	return 0;
}


void
cmd_list(char *args)
{
	char tok[MAXTOK +1];
	int t;

	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_IDTYPE, "Must specify id or type\n");
		return;
	}

	t = (int)def_lookval(def_types, tok);
	if (t < 0) {
		if (tok[strlen(tok) - 1] == 's') {
			tok[strlen(tok) - 1] = 0;
			t = (int)def_lookval(def_types, tok);
			if (t < 0) {
				pmsg(MSG_ERROR, PPPTALK_INVALTYPEID,
				     "%s is not a valid type/id\n", tok);
				return;
			}
		} else {
			pmsg(MSG_ERROR, PPPTALK_INVALTYPEID,
			     "%s is not a valid type/id\n", tok);
			return;
		}
	}

	if (!args || !*args) {
		list_type(t);
		return;
	}

	if (!list_id(args, t))
		return;

	pmsg(MSG_ERROR, PPPTALK_INVALTYPEID,
	     "%s is not a valid type/id\n", tok);
}

/*
 * Delete a definition
 */
void
cmd_del(char *args)
{
	char tok[MAXTOK +1];
	int ret, t;
	struct ulr_cfg *m = (struct ulr_cfg *)gmsg;

	args = gettok(args, tok);

	if (!tok || *tok == 0 || !args || *args == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTYPEORID,
		     "Must specify definition type and id.\n");
		return;
	}

	t = (int)def_lookval(def_types, tok);
	if (t < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALTYPE,
		     "%s is not a valid type\n", tok);
		return;
	}

	/*
	 * Check the specified ID exists and is the specified type
	 */
	ret = cfg_findid(so, m, args, t);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALDEFID,
		     "%s is not a valid %s definition.\n",
		     args, def_types[t].tv_str);
		return;
	}

	strcpy(m->ucid, args);
	m->type = t;
	m->prim = ULR_DEL_CFG;

	ret = ulr_sr(so, m, sizeof(struct ulr_cfg), ULR_MAX_MSG);
	switch (ret) {
	case 0:
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d).\n", ret);
		break;
	}
}

struct str2val_s cp_opts[] = {
	"up",		(void *)CP_UP,
	"down",		(void *)CP_DOWN,
	"status",	(void *)CP_STATUS,
	NULL,	NULL,
};


void
cmd_reset(char *args)
{
	char tok[MAXTOK +1];
	int  ret, t;
	struct ulr_cfg *m = (struct ulr_cfg *)gmsg;

	/* Get the bundle */
	args = gettok(args, tok);

	if (!tok || *tok == 0 || !args || *args == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTYPEORID,
		     "Must specify definition type and id.\n");
		return;
	}

	t = (int)def_lookval(def_types, tok);
	if (t < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALTYPE,
		     "%s is not a valid type\n", tok);
		return;
	}

	/*
	 * Check the specified ID exists and is the specified type
	 */
	ret = cfg_findid(so, m, args, t);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALDEFID,
		     "%s is not a valid %s definition.\n",
		     args, def_types[t].tv_str);
		return;
	}

	strcpy(m->ucid, args);
	m->prim = ULR_RESET_CFG;
	m->type = t;

	ret = ulr_sr(so, m, sizeof(struct ulr_cfg), sizeof(struct ulr_cfg));

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_WARN, PPPTALK_BADDEF,
		     "Definition incomplete or inconsistent.\n");
		break;
	case ECONNREFUSED:
		pmsg(MSG_WARN, 0,
	 "Incoming bundle must have login, callerid or authid set.\n");
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d).\n", ret);
		break;
	}
}

void
display_dial_error(int err)
{
	switch (err) {
	case INTRPT:
		pmsg(MSG_ERROR, PPPDIAL_INTRPT,
		     "Dial failed - Interrupt occured\n");
		break;
	case D_HUNG:
		pmsg(MSG_ERROR, PPPDIAL_D_HUNG,
		     "Dialer failed\n");
		break;
	case NO_ANS:
		pmsg(MSG_ERROR, PPPDIAL_NO_ANS,
	     "Dial failed - No answer (login or invoke scheme failed)\n");
		break;
	case ILL_BD:
		pmsg(MSG_ERROR, PPPDIAL_ILL_BD,
		     "Dial failed - Illegal baud rate\n");
		break;
	case A_PROB:
		pmsg(MSG_ERROR, PPPDIAL_A_PROB,
		     "Dial failed - ACU problem\n");
		break;
	case L_PROB:
		pmsg(MSG_ERROR, PPPDIAL_L_PROB,
		     "Dial failed - Line problem\n");
		break;
	case NO_Ldv:
		pmsg(MSG_ERROR, PPPDIAL_NO_Ldv,
		     "Dial failed - Can't open Devices file\n");
		break;
	case DV_NT_A:
		pmsg(MSG_ERROR, PPPDIAL_DV_NT_A,
		     "Dial failed - Requested device not available\n");
		break;
	case DV_NT_K:
		pmsg(MSG_ERROR, PPPDIAL_DV_NT_K,
		     "Dial failed - Requested device not known\n");
		break;
	case NO_BD_A:
		pmsg(MSG_ERROR, PPPDIAL_NO_BD_A,
		     "Dial failed - No device available at requested baud\n");
		break;
	case NO_BD_K:
		pmsg(MSG_ERROR, PPPDIAL_NO_BD_K,
		     "Dial failed - No device known at requested baud\n");
		break;
	case DV_NT_E:
		pmsg(MSG_ERROR, PPPDIAL_DV_NT_E,
		     "Dial failed - Requested speed does not match\n");
		break;
	case BAD_SYS:
		pmsg(MSG_ERROR, PPPDIAL_BAD_SYS,
		     "Dial failed - System not in Systems file\n");
		break;
	case CS_PROB:
		pmsg(MSG_ERROR, PPPDIAL_CS_PROB,
	     "Dial failed - Could not connect to the connection server\n");
		break;
	case DV_W_TM:
		pmsg(MSG_ERROR, PPPDIAL_DV_W_TM,
		     "Dial failed - Not allowed to call at this time\n");
		break;
	}
}

void
cmd_attach(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	/* Get the bundle */
	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOBUNDLE,
		     "Must specify bundle for request.\n");
		return;
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		return;
	}

	/* Send the attach operation */

	m->prim = ULR_ATTACH;
	strcpy(m->bundle, tok);

	ret = ulr_sr(so, m, sizeof(struct ulr_attach), ULR_MAX_MSG);
	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     tok);
		break;		
	case EINVAL:
		pmsg(MSG_ERROR, PPPTALK_EINVBUNDLE,
		     "Invalid bundle for operation\n");
		break;
	case ENXIO:
		pmsg(MSG_ERROR, PPPTALK_NOLINKDEF, "No links defined\n");
		break;
	case EAGAIN:
		pmsg(MSG_ERROR, PPPTALK_NOLINKAVAIL, "No link available\n");
		break;
	case EIO:
		/* Dial error */
		display_dial_error(m->derror);
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		break;
	}
}

void
cmd_detach(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	/* Get the bundle */
	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOBUNDLE,
		     "Must specify bundle for request.\n");
		return;
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		return;
	}

	/* Send the detach  operation */

	m->prim = ULR_DETACH;
	strcpy(m->bundle, tok);

	ret = ulr_sr(so, m, sizeof(struct ulr_attach), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     tok);
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		break;
	}
}

void
cmd_kill(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	/* Get the bundle */
	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOBUNDLE,
		     "Must specify bundle for request.\n");
		return;
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		return;
	}

	/* Send the kill  operation */

	m->prim = ULR_KILL;
	strcpy(m->bundle, tok);

	ret = ulr_sr(so, m, sizeof(struct ulr_attach), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     tok);
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		break;
	}
}

void
cmd_quit(char *cmd)
{
	extern int quit;

	quit = 1;
}

void
cmd_stop(char *cmd)
{
	int ret;
	extern int quit;

	gmsg->prim = ULR_STOP;
	ret = ulr_sr(so, gmsg, sizeof(*gmsg), ULR_MAX_MSG);
	if (ret) {
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
	}
	quit = 1;
}
void
cmd_log(char *args)
{
	int type, ret;
	struct ulr_log *m = (struct ulr_log *)gmsg;

	strcpy((char *)&m->msg, args);
	m->prim = ULR_LOG;
	ret = ulr_sr(so, m, sizeof(struct ulr_log) + strlen(args) + 1,
		     ULR_MAX_MSG);
	if (ret) {
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
	}
}

/*
 * Bringup another link on a bundle
 */
void
cmd_linkadd(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	args = gettok(args, tok);
	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOBUNDLE,
		     "Must specify bundle for request.\n");
		return;
	}

	/*
	 * Check the specified bundle ID exists and is a DEF_BUNDLE
	 */
	ret = cfg_findid(so, m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		return;
	}

	if (*args) {
		ret = cfg_findid(so, m, args, DEF_LINK);
		if (ret < 0) {
			pmsg(MSG_ERROR, PPPTALK_NOTLINK,
			     "%s is not a link definition.\n", args);
			return;
		}
	}

	/* Send the linkadd  operation */
	strcpy(m->bundle, tok);
	strcpy(m->link, args);
	m->prim = ULR_LINKADD;

	ret = ulr_sr(so, m, sizeof(struct ulr_attach), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     tok);
		break;
	case EINVAL:
		pmsg(MSG_ERROR, PPPTALK_BADLINKBUNDLE,
		     "Invalid link/bundle for operation\n");
		break;
	case ENXIO:
		pmsg(MSG_ERROR, PPPTALK_NOLINKDEF, "No links defined\n");
		break;
	case ENODEV:
		pmsg(MSG_ERROR, PPPTALK_MAXLINKS, "Maxlinks reached.\n");
		break;
	case EAGAIN:
		pmsg(MSG_ERROR, PPPTALK_NOLINKAVAIL, "No link available\n");
		break;
	case EIO:
		/* Dial error */
		display_dial_error(m->derror);
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		break;
	}
}

/*
 * Drop a link from a bundle
 */
void
cmd_linkdrop(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	args = gettok(args, tok);
	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOBUNDLE,
		     "Must specify bundle for request.\n");
		return;
	}

	/*
	 * Check the specified bundle ID exists and is a DEF_BUNDLE
	 */
	ret = cfg_findid(so, m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		return;
	}

	if (*args) {
		ret = cfg_findid(so, m, args, DEF_LINK);
		if (ret < 0) {
			pmsg(MSG_ERROR, PPPTALK_NOTLINK,
			     "%s is not a link definition.\n", args);
			return;
		}
	}

	/* Send the linkdrop  operation */
	strcpy(m->bundle, tok);
	strcpy(m->link, args);
	m->prim = ULR_LINKDROP;

	ret = ulr_sr(so, m, sizeof(struct ulr_attach),
		     sizeof(struct ulr_attach));

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     tok);
		break;
	case ENODEV:
		pmsg(MSG_ERROR, PPPTALK_MINLINKS, "Minlinks reached.\n");
		break;
	case EINVAL:
		pmsg(MSG_ERROR, PPPTALK_BADLINKBUNDLE,
		     "Invalid link/bundle for operation\n");
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		break;
	}
}

/*
 * Kill a link from a bundle
 */
void
cmd_linkkill(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	args = gettok(args, tok);
	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOBUNDLE,
		     "Must specify bundle for request.\n");
		return;
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		return;
	}

	if (!*args) {
		pmsg(MSG_ERROR, PPPTALK_NOLINK,
		     "Must specify link id for request.\n");
		return;
	}
		
	ret = cfg_findid(so, m, args, DEF_LINK);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_NOTLINK,
		     "%s is not a link definition.\n", args);
		return;
	}

	/* Send the linkdrop  operation */
	strcpy(m->bundle, tok);
	strcpy(m->link, args);
	m->prim = ULR_LINKKILL;

	ret = ulr_sr(so, m, sizeof(struct ulr_attach),
		     sizeof(struct ulr_attach));

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     tok);
		break;
	case EINVAL:
		pmsg(MSG_ERROR, PPPTALK_BADLINKBUNDLE,
		     "Invalid link/bundle for operation\n");
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		break;
	}
}

char *
fpprintf(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(gfp, fmt, ap);
	va_end(ap);	
}


/*
 * Get and display status for protocols on the link/bundle
 */
display_psm_status(char *id, int type, int depth)
{
	struct ulr_psm_stat *m = (struct ulr_psm_stat *)gmsg;
	int i = 0, ret;
	struct psm_entry_s *e;
	
	strcpy(m->id, id);
	m->type = type;
	m->prim = ULR_PSM_STATUS;

	fprintf(gfp, "\n");

	do {
		m->cookie = i;

		ret = ulr_sr(so, m, sizeof(struct ulr_psm_stat),
			     ULR_MAX_MSG);
		switch (ret) {
		case 0:
			break;
		default:
			pmsg(MSG_ERROR, PPPTALK_UNEXP,
			     "Unexpected error from pppd (%d)\n", ret);
			return;
		}
		
		e = cp_get_tab(m->psmid);
		if (!e)	
			pmsg(MSG_ERROR, PPPTALK_PROTONAVAIL,
			     "Support for protocol %s not available\n",
			     m->psmid);
		else if (*e->psm_status)
			(*e->psm_status)(&m->data, depth, verbose,
					 (int (*)(...))fpprintf);
		i++;
	} while (m->more);
}

/*
 * Values for a link/bundle phase
 */
char *phase_name[] = {
	"Dead",
	"Establish",
	"Authenticate",
	"Network",
	"Terminate",
};

/*
 * Display a links name given it's index in the bundle definition
 */
display_link_name(struct cfg_bundle *bn, int lindex, int depth)
{
	ulr_msg_t m;
	int t;
	char *l, *ds;
	char buf[MAXID+1];

	if (depth > 0)
		ds = "    ";
	else
		ds = "";

	if (lindex == 0) {
		fprintf(gfp, "'none'\n");
		return;
	}

	l = def_get_str(&(bn->bn_ch), bn->bn_links);	       

	for (t = 0; t < lindex; t++)
		l = (char*)def_get_element(l, buf);

	if (*buf)
		fprintf(gfp, "%s\n", buf);
	else
		fprintf(gfp, "'all tried'\n");
}

/*
 * Called to display the status of links that are active in a bundle
 */
display_links_status(char *name)
{
	struct ulr_list_links *ll;
	struct ulr_status status;
	int *ip, ret;

	ll = (struct ulr_list_links *)malloc(ULR_MAX_MSG);
	if (!ll) {
		pmsg(MSG_ERROR, 0, "No memory\n");
		return;
	}

	ll->prim = ULR_LIST_LINKS;
	strcpy(ll->bundle, name);

	ret = ulr_sr(so, ll, sizeof(*ll), ULR_MAX_MSG);
	switch (ret) {
	case 0:
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		free(ll);
		return;
	}

	ip = ll->index;
	while (ll->numlinks-- > 0) {

		status.prim = ULR_STATUS;
		status.type = DEF_LINK;
		status.index = *ip;
		
		ret = ulr_sr(so, &status, sizeof(status), sizeof(status));
		switch (ret) {
		case 0:
			fprintf(gfp, "\n");
			display_status(&status.ah, NULL, status.id, 1);
			break;
		default:
			pmsg(MSG_ERROR, PPPTALK_UNEXP,
			     "Unexpected error from pppd (%d)\n", ret);
			free(ll);
			return;
		}
		ip++;
	}
	free(ll);
}

char *link_reasons[] = {
	"Authentication failed",
	"Loopback detected",
	"Dropped - low bandwidth",
	"Hardware Hangup",
	"Dropped - Renegotiation",
	"Dial failed",
	"Dropped - Low Quality",
	"Bad incoming call",
	"Link didn't match bundle ID",
	"User action",
	"Failed to negotiate configured Auth protocol",
	"Authentication timed out",
};

char *bundle_reasons[] = {
	"Idle",
	"User action",
};

display_status(act_hdr_t *ah, struct ulr_cfg *c, char *name, int depth)
{
	act_bundle_t *ab;
	act_link_t *al;
	char *ds;

	if (depth > 0)
		ds = "    ";
	else
		ds = "";

	switch (ah->ah_type) {
	case DEF_LINK:
		al = &ah->ah_link;

		fprintf(gfp, "%sLink %s\n", ds, name);
		fprintf(gfp, "    %sPhase  : %s\n", ds, phase_name[ah->ah_phase]);
		fprintf(gfp, "    %sStatus : ", ds);
		if (al->al_flags & ALF_INCOMING)
			fprintf(gfp, "Incoming ");
		if (al->al_flags & ALF_AUTO)
			fprintf(gfp, "(Auto detected) ");
		if (al->al_flags & ALF_OUTGOING)
			fprintf(gfp, "Outgoing ");
		if (al->al_flags & ALF_DIAL)
			fprintf(gfp, "Dialing ");
		if (al->al_flags & ALF_HANGUP)
			fprintf(gfp, "Hangup ");
		if (al->al_flags == 0)
			fprintf(gfp, "Ready");
		fprintf(gfp, "\n");
		fprintf(gfp, "    %sReason : %s\n", ds,
			psm_display_flags(12, link_reasons, al->al_reason));
		display_psm_status(name, ah->ah_type, depth);
		break;

	case DEF_BUNDLE:
		ab = &ah->ah_bundle;

		fprintf(gfp, "%sBundle %s\n", ds, name);
		fprintf(gfp, "    %sPhase       : %s\n", ds,
		       phase_name[ah->ah_phase]);

		fprintf(gfp, "    %sStatus      : ", ds);
		if (ab->ab_flags & ABF_CALLED_IN)
			fprintf(gfp, "Incoming ");
		if (ab->ab_flags & ABF_CALLED_OUT)
			fprintf(gfp, "Outgoing ");
		if (ppp_debug && (ab->ab_flags & ABF_LDS))
			fprintf(gfp, "(Link Discriminators)");
		fprintf(gfp, "\n");

		fprintf(gfp, "    %sReason      : %s\n", ds,
			psm_display_flags(2, bundle_reasons, ab->ab_reason));

		if (ppp_debug) {
			fprintf(gfp, "    %sOpen Links  : %d\n", ds,
				ab->ab_open_links);
			fprintf(gfp, "    %sTotal Links : %d\n", ds,
				ab->ab_numlinks);
			fprintf(gfp, "    %sLocal MRRU  : %d\n", ds,
				ab->ab_local_mrru);
			fprintf(gfp, "    %sPeer MRRU   : %d\n", ds,
				ab->ab_peer_mrru);
			fprintf(gfp, "    %sMtu         : %d\n", ds,
				ab->ab_mtu);
		}

		fprintf(gfp, "    %sAdded link  : ", ds);
		display_link_name(&c->un.bundle, ab->ab_addindex, depth);

		fprintf(gfp, "    %sDropped link: ", ds);
		display_link_name(&c->un.bundle, ab->ab_dropindex, depth);

		display_psm_status(name, ah->ah_type, depth);
		/*
		 * Get the status of all the bundles links (or at least
		 * the first few hundred)
		 */
		display_links_status(name);
		break;

	default:
		pmsg(MSG_ERROR, 0, "Internal error: unexpected type (%d).\n", ah->ah_type);
		break;
	}
}

/*
 * Get the status for the specified type and id
 */
int
status_id(char *id, int type)
{
	struct ulr_status *m = (struct ulr_status *)gmsg;
	int ret;
	act_hdr_t ah;
	struct ulr_cfg *c;

	c = (struct ulr_cfg *)malloc(ULR_MAX_MSG);
	if (!c) {
		pmsg(MSG_ERROR, 0, "No memory\n");
		return;
	}

	/*
	 * Check the specified ID exists and is the specified type
	 */
	ret = cfg_findid(so, c, id, type);
	if (ret < 0) {
		pmsg(MSG_ERROR, 0, "%s is not a valid %s definition.\n",
		     id, def_types[type].tv_str);
		free(c);
		return;
	}

	strcpy(m->id, id);
	m->prim = ULR_STATUS;
	m->type = type;
	m->index = -1;

	ret = ulr_sr(so, m, sizeof(struct ulr_status), ULR_MAX_MSG);
	switch (ret) {
	case 0:
		break;
	case ENOENT:
		free(c);
		pmsg(MSG_INFO, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     id);
		return;
	default:
		free(c);
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		return;
	}

	ah = m->ah;

	/* Display the results (note: gmsg not now being used) */
	display_status(&ah, c, id, 0);
	free(c);
}

void
status_summary(int type, char *id)
{
	struct ulr_status *m = (struct ulr_status *)gmsg;
	int ret;
	act_hdr_t *ah;
	act_link_t *al;
	act_bundle_t *ab;

	fprintf(gfp, "%-30.30s  ", id);

	strcpy(m->id, id);
	m->prim = ULR_STATUS;
	m->type = type;
	m->index = -1;

	ret = ulr_sr(so, m, sizeof(struct ulr_status), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		fprintf(gfp, "Not active (disabled/bad configuration)\n");
		return;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		return;
	}

	ah = &m->ah;
	switch (type) {
	case DEF_LINK:
		al = &ah->ah_link;

		if (al->al_flags & ALF_INCOMING)
			fprintf(gfp, "Incoming");
		if (al->al_flags & ALF_OUTGOING)
			fprintf(gfp, "Outgoing");
		if (al->al_flags & ALF_DIAL)
			fprintf(gfp, "Dialing ");
		if (al->al_flags & ALF_HANGUP)
			fprintf(gfp, "Hangup  ");
		if (al->al_flags == 0)
			fprintf(gfp, "Ready   ");

		/* Display the phase of negotiation */
		fprintf(gfp, " %-13.13s", phase_name[ah->ah_phase]);
		fprintf(gfp, "\n");
		break;
	case DEF_BUNDLE:
		ab = &ah->ah_bundle;

		fprintf(gfp, "%-13.13s ", phase_name[ah->ah_phase]);
		fprintf(gfp, "%-4.4d  %-4.4d", ab->ab_open_links, ab->ab_open_ncps); 
		if (ab->ab_flags & ABF_CALLED_OUT)
			fprintf(gfp, " Outgoing");
		else if (ab->ab_flags & ABF_CALLED_IN)
			fprintf(gfp, " Incoming");
		fprintf(gfp, "\n");
		break;
	}
}

/*
 * Get the status for all definitions of the specified type.
 */
void
status_type(int type)
{
	struct ulr_list_cfg *m;
	int ret, i;
	struct list_ent *le;
	void *last_cookie;

	m = (struct ulr_list_cfg *)malloc(ULR_MAX_MSG);
	if (!m) {
		pmsg(MSG_ERROR, 0, "No memory\n");
		return;
	}

	switch (type) {
	case DEF_LINK:
	fprintf(gfp, "%-30.30s  State    Phase\n", def_types[type].tv_str);
	fprintf(gfp, "------------------------------  -----    -----\n");
		break;
	case DEF_BUNDLE:
	fprintf(gfp, "%-30.30s  Phase        Links NCP's State\n", def_types[type].tv_str);
	fprintf(gfp, "------------------------------  ------------ ----- ----- ----\n");
		break;
	}

	/* Initialise the message */

	m->prim = ULR_LIST_CFG;
	m->type = type;
	m->entry.le_cookie = (void *)0; /* Start */

	for (;;) {

		ret = ulr_sr(so, m, sizeof(struct ulr_list_cfg), ULR_MAX_MSG);
		if (ret) {
			pmsg(MSG_ERROR, 0, "status_type: Error %d\n", ret);
			free(m);
			return;
		}

		if (m->num == 0) {
			free(m);
			return;
		}

		le = &(m->entry);
		
		for (i = 0; i < m->num; i++) {
			status_summary(type, le->le_id);
			last_cookie = le->le_cookie;
			le++;
		}
		m->entry.le_cookie = last_cookie;
	}
}

void
cmd_status(char *args)
{
	char tok[MAXTOK +1];
	int t;

	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_NOLINKBUNDLE,
		     "Must specify type (link/bundle).\n");
		return;
	}

	t = (int)def_lookval(def_types, tok);
	if (t < 0) {
		if (tok[strlen(tok) - 1] == 's') {
			tok[strlen(tok) - 1] = 0;
			t = (int)def_lookval(def_types, tok);
			if (t < 0) {
				pmsg(MSG_ERROR, PPPTALK_INVALTYPE,
				     "%s is not a valid type.\n", tok);
				return;
			}
		} else {
			pmsg(MSG_ERROR, PPPTALK_INVALTYPE,
			     "%s is not a valid type.\n", tok);
			return;
		}
	}

	if (!args || !*args) {
		status_type(t);
		return;
	}

	status_id(args, t);
}

void
cmd_load()
{
	FILE *fp;
	char *cmd;
	extern int quit;
	extern int version_checked;
	extern int def_count;
	struct stat sbuf;

	/* Create a config file if one doesn't exist */

	if (stat(PPPCFG, &sbuf) < 0 && errno == ENOENT) {

		/* Don't create anything users can access ! */
		umask(0077);
		fp = fopen(PPPCFG, "a+");
		if (!fp) {
			cmd_log("Unable to create config file");
		} else {
			cmd_log("No config file - auto created");
			fclose(fp);
		}
	}

	/* Open the config file */
	fp = fopen(PPPCFG, "r");
	if (!fp) {
		cmd_log("Error opening config file.\n");
		cmd_stop(NULL);
		exit(1);
	}

	def_count = 0;

	do {
		/* Get command string */

		cmd = getinput(fp, NULL);

		/* Parse & execute command string */

		parsecmd(cmd);

	} while (!quit);

	/* close file */
	fclose(fp);
	set_state(0/* CLEAN */);

	if (!version_checked && def_count > 0) {
		cmd_log("No version found in Config file");
		cmd_stop(NULL);
	}
}

int
save_id(FILE *fp, int type, char *id)
{
	struct ulr_cfg *m = (struct ulr_cfg *)gmsg;
	int ret;

	ret = cfg_findid(so, m, id, type);
	if (ret < 0)
		return 1;

	(*list_fn[type])(fp, &(m->un));
	return 0;
}

void
save_type(FILE *fp, int type)
{
	struct ulr_list_cfg *m;
	int ret, i;
	struct list_ent *le;
	void *last_cookie;

	m = (struct ulr_list_cfg *)malloc(ULR_MAX_MSG);
	if (!m) {
		pmsg(MSG_ERROR, 0, "Couldn't allocate memory\n");
		return;
	}

	/* Initialise the message */

	m->prim = ULR_LIST_CFG;
	m->type = type;
	m->entry.le_cookie = (void *)0; /* Start */

	for (;;) {

		ret = ulr_sr(so, m, sizeof(struct ulr_list_cfg), ULR_MAX_MSG);
		if (ret) {
			free(m);
			pmsg(MSG_ERROR, 0, "save_type: Error %d\n", ret);
			return;
		}

		if (m->num == 0) {
			free(m);
			return;
		}

		le = &(m->entry);

		for (i = 0; i < m->num; i++) {
			save_id(fp, type, le->le_id);
			last_cookie = le->le_cookie;
			le++;
		}

		m->entry.le_cookie = last_cookie;
	}
}


void
cmd_save(char *args)
{
	int t;
	FILE *fp;
	char buf[256];
	extern void user_intr(int sig);
	extern char *ppp_version;

	sigignore(SIGINT);

	umask(0077);
	sprintf(buf, "%s.tmp", PPPCFG);

	/* Open the config file, trunc/creat */
	fp = fopen(buf, "w+");
	if (!fp) {
		pmsg(MSG_ERROR, PPPTALK_ERROPENSAVE,
		     "Error opening config file. Not saved.\n");
		goto exit;
	}

	fprintf(fp, "version %s\n", ppp_version);

	/* Save All */

	for (t = 0 ; t < DEF_MAX; t++) {
		pmsg(MSG_INFO, PPPTALK_SAVETAB,
		     "Saving %s table.\n", def_types[t]);
		/* Save this type */
		save_type(fp, t);
	}

	/* close file */
	fclose(fp);

	if (rename(buf, PPPCFG) < 0) {
		pmsg(MSG_ERROR, PPPTALK_ERRRENAME,
		     "Error renaming config file.\n");
		goto exit;
	}

	set_state(0/* CLEAN */);
 exit:
	signal(SIGINT, user_intr);

}

/*
 * Get and display statistics for protocols on the link/bundle
 */
display_psm_stats(char *id, int type, int depth)
{
	struct ulr_psm_stat *m = (struct ulr_psm_stat *)gmsg;
	int i = 0, ret;
	struct psm_entry_s *e;
	
	m->prim = ULR_PSM_STATS;
	m->type = type;
	strcpy(m->id, id);

	do {
		m->cookie = i;

		ret = ulr_sr(so, m, sizeof(*m), ULR_MAX_MSG);
		switch (ret) {
		case 0:
			break;
		default:
			pmsg(MSG_ERROR, PPPTALK_UNEXP,
			     "Unexpected error from pppd (%d)\n", ret);
			return;
		}

		e = cp_get_tab(m->psmid);
		if (!e)	
			pmsg(MSG_ERROR, PPPTALK_PROTONAVAIL,
			     "Support for protocol %s not available\n",
			     m->psmid);
		else if (*e->psm_stats) {
			fprintf(gfp, "\n");
			(*e->psm_stats)((m->flags & PSM_TX) ?
						&m->txdata : NULL,
					(m->flags & PSM_RX) ?
						&m->rxdata : NULL,
					depth, (int (*)(...))fpprintf);
		}
		i++;
	} while (m->more);
}

void
cmd_stats(char *args)
{
	struct ulr_stats *m = (struct ulr_stats *)gmsg;
	int ret;
	char tok[MAXTOK +1];
	int t;
	struct pc_stats_s *pc;
	struct bl_stats_s *bl;
	struct tm tmtime;
	
	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_STATTYPE,
		     "Syntax error. Must specify type to stat.\n");
		return;
	}

	t = (int)def_lookval(def_types, tok);
	if (t < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALTYPE,
		     "%s is not a valid type (specify link or bundle)\n", tok);
		return;
	}

	/*
	 * Check the specified ID exists and is the specified type
	 */
	ret = cfg_findid(so, m, args, t);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALDEFID,
		     "%s is not a valid %s definition.\n",
		     args, def_types[t].tv_str);
		return;
	}

	m->prim = ULR_STATS;
	strcpy(m->id, args);
	m->type = t;

	ret = ulr_sr(so, m, sizeof(struct ulr_stats), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_INFO, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     args);
		return;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		return;
	}

	/* Display the results */

	switch (t) {
	case DEF_LINK:
		fprintf(gfp, "Link %s\n", args);
		pc = &m->spc;

		if (pc->ps_time == 0)
			strcpy(tok, "Statistics not available\n");
		else {
			localtime_r(&pc->ps_time, &tmtime);
			asctime_r(&tmtime, tok);
		}

		fprintf(gfp, "\tTime Stamp : %s", tok);
		
		fprintf(gfp, "\t%lu packets received\n", pc->ps_inpackets);
		fprintf(gfp, "\t%lu bytes received\n", pc->ps_inoctets);
		fprintf(gfp, "\t%lu packets sent\n", pc->ps_outpackets);
		fprintf(gfp, "\t%lu bytes sent\n", pc->ps_outoctets);
		fprintf(gfp, "\t%lu packets with bad FCS\n", pc->ps_badfcs);
		fprintf(gfp, "\t%lu packets too short\n", pc->ps_tooshort);
		fprintf(gfp, "\t%lu packets too long\n", pc->ps_toolong);
		break;

	case DEF_BUNDLE:
		fprintf(gfp, "Bundle %s\n", args);
		bl = &m->sbl;

		if (bl->bs_time == 0)
			strcpy(tok, "Statistics not available\n");
		else {
			localtime_r(&bl->bs_time, &tmtime);
			asctime_r(&tmtime, tok);
		}

		fprintf(gfp, "\tTime Stamp : %s", tok);

		if (bl->bs_contime == 0)
			strcpy(tok, "\n");
		else {
			localtime_r(&bl->bs_contime, &tmtime);
			asctime_r(&tmtime, tok);
		}

		fprintf(gfp, "\tConnected at : %s", tok);

		fprintf(gfp, "\t%lu bytes sent\n", bl->bs_outoctets);
		fprintf(gfp, "\t%lu bytes received\n", bl->bs_inoctets);

		if (ppp_debug) {
			fprintf(gfp, "\t%lu bandwidth (kbps)\n",
				(bl->bs_bandwidth)/1024);
			fprintf(gfp, "\t%.1f ibw k/s\n",
				(float)bl->bs_ibw / 1024);
			fprintf(gfp, "\t%.1f obw k/s\n",
				(float)bl->bs_obw / 1024);
			fprintf(gfp, "\t%lu abw_up\n", bl->bs_abw_up);
			fprintf(gfp, "\t%lu abw_down\n", bl->bs_abw_down);
		}
		break;
	}

	display_psm_stats(args, t, 0);
}

void
cmd_debug(char *args)
{
	struct ulr_debug *m = (struct ulr_debug *)gmsg;
	int ret;
	char tok[MAXTOK +1];
	int t, l;

	/* Get debug level */

	args = gettok(args, tok);
	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, 0,
		     "Syntax error. Must specify debug level\n");
		return;
	}

	l = (int)def_lookval(debug_levels, tok);
	if (l < 0) {
		pmsg(MSG_ERROR, PPPTALK_BADDEBUG,
		     "%s is not a valid debug level\n", tok);
		return;
	}

	/* Figure out if link or bundle */

	args = gettok(args, tok);
	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPPTALK_DEBUGTYPE,
		     "Syntax error. Must specify type to debug\n");
		return;
	}

	t = (int)def_lookval(def_types, tok);
	if (t < 0) {
		pmsg(MSG_ERROR, 0,
		     "%s is not a valid type (specify link or bundle)\n", tok);
		return;
	}

	/* Now get the ID */

	args = gettok(args, tok);
	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, 0,
		     "Syntax error. Must specify id or type to debug\n");
		return;
	}

	/*
	 * Check the specified ID exists and is the specified type
	 */
	ret = cfg_findid(so, m, tok, t);
	if (ret < 0) {
		pmsg(MSG_ERROR, 0, "%s is not a valid %s definition.\n",
		     tok, def_types[t].tv_str);
		return;
	}

	/* Now set the debug level */

	strcpy(m->id, tok);
	m->prim = ULR_DEBUG;
	m->type = t;
	m->level = l;

	ret = ulr_sr(so, m, sizeof(struct ulr_debug), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPPTALK_NOTACT,
		     "%s not active. Definition incomplete or inconsistent.\n",
		     tok);
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		return;
	}
	

}

extern char *def_prompt;
extern char *cmd_prompt;

#define MAX_PROMPT 256

int
parse_prompt(char *new, char * targ)
{

	char *p = targ;

	while (*new && (targ - p) < MAX_PROMPT) {

		if (*new == '\\') {
			new++;
			switch (*new) {
			case 'n':
				*p++ = '\n';
				break;
			case 't':
				*p++ = '\t';
				break;
			case '\\':
				*p++ = '\\';
				break;
			default:
				pmsg(MSG_ERROR, PPPTALK_DONTKNOW,
				     "Don't know \\%c.", *new);
				return 1;
			}
			new++;
		} else
			*p++ = *new++;
	}

	*p = 0;

	return 0;
}

void
cmd_ch_cmd_prompt(char *args)
{
	static char prompt[MAX_PROMPT+1];
	char tprompt[MAX_PROMPT+1];

	if (parse_prompt(args, tprompt))
		return;

	strcpy(prompt, tprompt);
	cmd_prompt = prompt;
}

void
cmd_ch_def_prompt(char *args)
{
	static char prompt[MAX_PROMPT+1];
	char tprompt[MAX_PROMPT+1];

	if (parse_prompt(args, tprompt))
		return;

	strcpy(prompt, tprompt);
	def_prompt = prompt;
}

void
cmd_shell(char *args)
{
      while (*args && isspace(*args))
	      args++;
                        ;
      if (*args == '\0')
              args = "exec ${SHELL:-/bin/sh}";
      system(args);
}

set_state(int cfgstate)
{
	struct ulr_state *m = (struct ulr_state *)gmsg;
	int ret;

	m->prim = ULR_STATE;
	m->cmd = UDC_SET;
	m->cfgstate = cfgstate;
	ret = ulr_sr(so, m, sizeof(struct ulr_state), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		return;
	}
}

struct ulr_state *
get_state()
{
	struct ulr_state *m = (struct ulr_state *)gmsg;
	int ret;

	m->prim = ULR_STATE;
	m->cmd = UDC_GET;
	ret = ulr_sr(so, m, sizeof(struct ulr_state),
		     ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		return NULL;
	}

	ppp_debug = m->debug;
	return m;
}

void
cmd_cfgstate(char *args)
{
	struct ulr_state *m;

	m = get_state();
	if (!m)
		return;

	if (m->cfgstate & UCFG_MODIFIED)
		pmsg(MSG_INFO, PPPTALK_DIRTY,
		     "Configuration contains unsaved modifications\n");
	else
		pmsg(MSG_INFO, PPPTALK_CLEAN,
		     "No modifications require saving\n");

	if (ppp_debug & DEBUG_PPPD)
		pmsg(MSG_INFO, 0, "PPP Debugging selected.\n");

	if (ppp_debug & DEBUG_ANVL)
		pmsg(MSG_INFO, 0, "ANVL test mode selected.\n");
}

void
cmd_copy(char *args)
{
	char type[MAXTOK + 1], curr_id[MAXTOK + 1];
	int ret, t;
	struct ulr_cfg *m = (struct ulr_cfg *)gmsg;

	/* Get the definition type */
	args = gettok(args, type);
	if (!type || *type == 0) {
		pmsg(MSG_ERROR, 0,
		     "Must specify definition type.\n");
		return;
	}

	t = (int)def_lookval(def_types, type);
	if (t < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALTYPE,
		     "%s is not a valid type\n", type);
		return;
	}

	/* Get the current ID */
	args = gettok(args, curr_id);

	if (!type || *type == 0) {
		pmsg(MSG_ERROR, 0,
		     "Must specify ID of existing definition .\n");
		return;
	}

	/* Check target id exists */
	if (!args || !*args) {
		pmsg(MSG_ERROR, 0,
		     "Must specify ID of target definition.\n");
		return;
	}


	/*
	 * Check the specified ID exists and is the specified type
	 */
	ret = cfg_findid(so, m, curr_id, t);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_INVALDEFID,
		     "%s is not a valid %s definition.\n",
		     curr_id, def_types[t].tv_str);
		return;
	}

	/*
	 * Write the new ID
	 */
 	m->prim = ULR_SET_CFG;
	strcpy(m->ucid, args);

	ret = ulr_sr(so, m, m->uclen + sizeof(struct ulr_cfg), ULR_MAX_MSG);
	switch (ret) {
	case 0:
		break;
	default:
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d).\n", ret);
		break;
	}
}

/*
 * This command displays the current version .. if a version is 
 * supplied on the command line .. then this is checked against
 * the current version and the result returned.
 */
void
cmd_version(char *version)
{
	extern char *ppp_version;
	extern int version_checked;

	if (*version == 0) {
		pmsg(MSG_INFO, 0, "%s\n", ppp_version);
		return;
	}

	if (strcmp(ppp_version, version) != 0) {

		cmd_log("Incorrect Config file Version - STOP");

		pmsg(MSG_INFO, 0, "Incorrect Version (%s) - STOP\n",
		     ppp_version);

		cmd_stop(NULL);
	} else {
		pmsg(MSG_INFO, 0, "%s\n", ppp_version);
		version_checked = 1;
	}
}

/*
 * Set vi mode
 */
void
cmd_vi()
{
	extern int editor_mode;

	if (editor_mode == 0)
		edit_init(VIRAW);
	else
		edit_mode(VIRAW);
	editor_mode = VIRAW;
}

/*
 * Toggle verbosity
 */
void
cmd_verbose()
{
	if (verbose == PSM_V_SHORT) {
		verbose = PSM_V_LONG;
		pmsg(MSG_INFO, PPPTALK_NOWHI, "Now set to High\n");
	} else {
		verbose = PSM_V_SHORT;
		pmsg(MSG_INFO, PPPTALK_NOWLO, "Now set to Low\n");
	}
}

/*
 * Set emacs mode
 */
void
cmd_emacs()
{
	extern int editor_mode;

	if (editor_mode == 0)
		edit_init(EMACS);
	else
		edit_mode(EMACS);
	editor_mode = EMACS;
}

/*
 * Display a call
 */
void
display_call_summary(struct histent_s *he)
{
	struct tm *tmtime;
	char tok;

	tmtime = localtime(&he->he_time);

	if (he->he_alflags & ALF_OUTGOING)
		fprintf(gfp, "Outgoing ");
	else
		fprintf(gfp, "Incoming ");

	switch (he->he_op) {
	case HOP_ADD:
		if (he->he_alflags & ALF_OUTGOING)
			fprintf(gfp, "Call Placed  ");
		else
			fprintf(gfp, "Call Accepted");
		break;
	case HOP_DROP:
		fprintf(gfp, "Call Dropped");
		break;
	}
	fprintf(gfp, ", %s : %s", he->he_dev, asctime(tmtime));
}

/*
 * Display a call
 */
void
display_call_details(struct histent_s *he)
{
	struct tm *tmtime;
	char tok;
	char *p;

	tmtime = localtime(&he->he_time);

	if (he->he_alflags & ALF_OUTGOING)
		fprintf(gfp, "Outgoing ");
	else
		fprintf(gfp, "Incoming ");

	switch (he->he_op) {
	case HOP_ADD:
		if (he->he_alflags & ALF_OUTGOING)
			fprintf(gfp, "Call Placed  ");
		else
			fprintf(gfp, "Call Accepted");
		break;
	case HOP_DROP:
		fprintf(gfp, "Call Dropped ");
		break;
	}
	fprintf(gfp, " : %s", asctime(tmtime));

	fprintf(gfp, "\n");

	fprintf(gfp, "\tDevice        : %s\n", he->he_dev);
	fprintf(gfp, "\tLink          : %s\n", he->he_linkid);
	fprintf(gfp, "\tBundle        : %s\n", he->he_bundleid);

	p = psm_display_flags(2, bundle_reasons, he->he_abreason);
	if (*p)
		fprintf(gfp, "\tBundle Reason : %s\n", p);

	p = psm_display_flags(12, link_reasons, he->he_alreason);
	if (*p)
		fprintf(gfp, "\tLink Reason   : %s\n", p);

	if (*he->he_uid || (he->he_alflags & ALF_INCOMING))
		fprintf(gfp, "\tlogin         : %s\n", he->he_uid);
	if (*he->he_aid || (he->he_alflags & ALF_INCOMING))
		fprintf(gfp, "\tpeer authname : %s\n", he->he_aid);
	if (*he->he_cid || (he->he_alflags & ALF_INCOMING))
		fprintf(gfp, "\tcallerid      : %s\n", he->he_cid);

	if (he->he_error) {
		fprintf(gfp, "\tOther Errors  : ");
		switch (he->he_error) {
		case ENOENT:
			fprintf(gfp, "No suitable link definition");
			break;
		case ENXIO:
			fprintf(gfp, "Link type not supported");
			break;
		case EBADSLT:
			fprintf(gfp, "No suitable bundle definition");
			break;
		case ENODEV:
			fprintf(gfp, "Dial failure");
			break;
		default:
			fprintf(gfp, "%d", he->he_error);
			break;
		}
		fprintf(gfp, "\n");
	}
	fprintf(gfp, "\n");
}


/*
 * Display call history
 */
void
cmd_calls(char *args)
{
	struct ulr_callhistory *ch = (struct ulr_callhistory *)gmsg;
	struct histent_s *he;
	int ret;
	int num, default_num;


	if (verbose == PSM_V_SHORT)
		default_num = 15;
	else
		default_num = 2;

	if (*args) {
		num = atoi(args);
		if (num < 1)
			num = default_num;
	}else
		num = default_num;

	ch->prim = ULR_CALLHIST;
	ch->cookie = 0;

	fprintf(gfp, "Call History\n");
	fprintf(gfp, "----------------------------------------\n");

	do {
		ret = ulr_sr(so, ch, sizeof(struct ulr_callhistory),
			     ULR_MAX_MSG);
		if (ret) {
			pmsg(MSG_ERROR, PPPTALK_UNEXP,
			     "Unexpected error from pppd (%d).\n", ret);
			break;
		}

		/* Display the calls */
		he = &ch->he;

		for (; ch->num > 0 && num; ch->num--) {
			if (verbose == PSM_V_SHORT)
				display_call_summary(he);
			else
				display_call_details(he);
			he++;
			ch->cookie++;
			num--;
		}
		
	} while (ch->more && num);

	if (ch->more || ch->num > 0)
		fprintf(gfp, "...\n");
}

/*
 * Clear the call history
 */
void
cmd_clear()
{
	struct ulr_prim *p = (struct ulr_prim *)gmsg;
	int ret;

	p->prim = ULR_HISTCLEAR;

	ret = ulr_sr(so, p, sizeof(struct ulr_prim), sizeof(struct ulr_prim));
	if (ret)
		pmsg(MSG_ERROR, PPPTALK_UNEXP,
		     "Unexpected error from pppd (%d).\n", ret);
}

void
cmd_unknown(char *tok, char *args)
{
	pmsg(MSG_ERROR, PPPTALK_CMDUNKNOWN,
	     "%s <- Command not recognised. Type help for commands.\n", tok);
}
	
	
/*
 * Command table
 */
void cmd_help(char *);

struct cmdtab {
	char *cmd;
	void (*fn)();
	short flags;	/* Static */
#define CT_ROOT		0x0001	/* Must be root to use this command */
#define CT_ANVL		0x0002	/* Command for anvl test mode */
	char *args;
	char *help;
} cmdtab[] = {
	"attach", cmd_attach, 0,
		"[bundle ID]",
		"Make an outgoing connection using the specified bundle",
	"calls", cmd_calls, 0,
		"[num calls]",
		"Display a call history",
	"clear", cmd_clear, CT_ROOT,
		"",
		"Clear the call history",
	"copy", cmd_copy, CT_ROOT,
		"[bundle|link|protocol|auth|algorithm] [ID] [New ID]",
		"Copy a configuration definition from ID to new ID",
	"debug", cmd_debug, CT_ROOT,
		"[none|low|med|high|wire] [bundle|link] [ID]",
		"Set debug level for a link or bundle.",
	"defprompt", cmd_ch_def_prompt, 0,
		"[string]",
		"Set the definition prompt",
	"del",	cmd_del, CT_ROOT,
 		"[link|protocol|bundle|auth|algorithm|global] [ID]",
		"Delete a configured definition",
	"detach", cmd_detach, 0,
		"[bundle ID]",
		"Disconnect links from  the specified bundle",
	"emacs", cmd_emacs, 0,
		"",
		"Set emacs style command line editing mode",
	"help",	cmd_help, 0,
		"",
		"Displays help",
	"kill", cmd_kill, CT_ROOT | CT_ANVL,
		"[bundle ID]",
		"Disconnect links from  the specified bundle",
	"list",	cmd_list, CT_ROOT,
		"[link|protocol|bundle|auth|algorithm|global] [ID]",
		"List definitions or a definitions options",
	"linkadd", cmd_linkadd, 0,
		"[bundle ID] [link ID]",
		"Bringup a link on a bundle",
	"linkdrop", cmd_linkdrop, 0,
		"[bundle ID] [link ID]",
		"Drop a link from a bundle",
	"linkkill", cmd_linkkill, CT_ROOT | CT_ANVL,
		"[bundle ID] [link ID]",
		"Kill a link from a bundle",
	"log", cmd_log, CT_ROOT,
		"[message to log]",
		"Log a message to the log file",
	"prompt", cmd_ch_cmd_prompt, 0,
		"[string]",
		"Set the command prompt",
	"quit",	cmd_quit, 0,
		"",
		"Exit this program",
	"save",	cmd_save, CT_ROOT,
		"",
		"Save configuration",
	"state", cmd_cfgstate, 0,
		"",
		"Get configuration state (modified or not)",
	"stats", cmd_stats, 0,
		"[bundle|link] [ID]",
		"Obtain a link or bundles statistics",
	"status", cmd_status, 0,
		"[bundle|link] [ID]",
		"Obtain a link or bundles status",
	"stop", cmd_stop, CT_ROOT,
		"",
		"Stop the PPP Daemon",
	"reset", cmd_reset, CT_ROOT,
		"[bundle|link] [ID]",
		"Cause a link or bundle to use an updated configuration",
	"verbose", cmd_verbose, 0,
		"",
		"Toggle verbosity for status/calls",
	"version", cmd_version, CT_ROOT,
		"[version]",
		"Display or check version number",
	"vi", cmd_vi, 0,
		"",
		"Set vi style command line editing mode",
	"!",	cmd_shell, CT_ROOT,
		"",
		"Shell escape",
	NULL,	cmd_unknown, 0, NULL, NULL,
};

display_help(struct psm_opttab *opt)
{
	fprintf(gfp, "  %-15.15s %-10.10s \n",
	       opt->opt, opt->flags == OPTIONAL ? "Optional" : "Mandatory");
}

display_help_table(struct psm_opttab *opt)
{
	while (opt->opt)
		display_help(opt++);
}

void
help_globals(char *args)
{
}

void
help_auth(char *args)
{
	extern struct psm_opttab auth_opts[];
	display_help_table(auth_opts);
}

void
help_proto(char *args)
{
	char tok[MAXTOK +1];
	struct psm_entry_s *e;

	args = gettok(args, tok);

	if (!*tok) {
		pmsg(MSG_ERROR, PPPTALK_NOPROTOALG,
		     "Must specifiy protocol/algorithm type\n");
		return;
	}

	/* Call the protocol specifiec list routine */

	e = cp_get_tab(tok);
	if (!e) {
		pmsg(MSG_ERROR, PPPTALK_PROTONAVAIL,
		     "Support for protocol %s not available\n", tok);
		return;
	}

	if (e->psm_opts) {
		display_help_table(e->psm_opts);
	}
}

void
help_link(char *args)
{
	extern struct psm_opttab link_opts[], link_type_opt;
	display_help(&link_type_opt);
	display_help_table(link_opts);
}

void
help_bundle(char *args)
{
	extern struct psm_opttab bundle_opts[];
	display_help_table(bundle_opts);
}

/*
 * Provide help for the specified definition
 */
void (*help_fn[])() = {
	help_globals,
	help_auth,
	help_proto,
	help_proto,
	help_link,
	help_bundle,
};

/*
 * Returns true if user is really root
 */
static int
is_really_root()
{
	return (getuid() == 0);
}

/*
 * Returns 0 - command not allowed by user
 * 	   1 - command allowed for user
 */
int
help_allow(int flags)
{
	if ((flags & CT_ROOT) && !is_really_root())
		return 0;

	if ((flags & CT_ANVL) && !(ppp_debug & DEBUG_ANVL))
		return 0;

	return 1;
}

void
help_cmd(char *args)
{
	char tok[MAXTOK +1];
	int t;
	struct cmdtab *tab = cmdtab;

	args = gettok(args, tok);

	/* Is this first argument a command we know ? */

	while(tab->cmd) {
		if (help_allow(tab->flags) && strcmp(tok, tab->cmd) == 0) {
			fprintf(gfp, "Command     : %s %s\n",
				tab->cmd, tab->args);
			fprintf(gfp, "Description : %s\n", tab->help);
			return;
		}
		tab++;
	}

	/* Is this first argument a deinition type */

	t = (int)def_lookval(def_types, tok);
	if (t >= 0) {
		(*help_fn[t])(args);
		return;
	}

	pmsg(MSG_ERROR, PPPTALK_NOMOREHELP,
	     "No further help for '%s' is available\n", tok);
}

void
cmd_help(char *args)
{
	struct cmdtab *tab = cmdtab;

	if (*args) {
		help_cmd(args);
		return;
	}

	pmsg(MSG_INFO, PPPTALK_CMDSTITLE, "Commands\n");
	while(tab->cmd) {
		if (help_allow(tab->flags))
			fprintf(gfp, "  %-20.20s %s\n", tab->cmd, tab->args);
		tab++;
	}

	pmsg(MSG_INFO, PPPTALK_HELPTXT,
	     "Type 'help [command]' for more detail.\n");
}

/*
 * Perform single line definitions
 */
int
cmddef(char *tok, char *args)
{
	int t;
	char buf[MAXTOK +1];

	t = (int)def_lookval(def_types, tok);
	if (t < 0)
		return 0; /* Don't recognize */

	/* Do the definition */
	args = gettok(args, buf);
	if (*buf == 0) {
		pmsg(MSG_ERROR, PPPTALK_DEFID,
		     "Must specify id for definition.\n");
		return 1;
	}

	def_new(buf);
	def_store("definition", tok);

	args = gettok(args, buf);
	if (*buf == 0) {
		pmsg(MSG_ERROR, PPPTALK_DEFOPT,
		     "Must specify option for definition.\n");
		return 1;
	}

	if (*args == '=') {
		args++;
		args = (char *)trimline(args);
	} else {
		pmsg(MSG_ERROR, PPPTALK_DEFFMT,
		     "Options must be option = value\n");
		return 1;
	}

	def_store(buf, args);
	
	def_process();
	return 1;
}

/*
 * This function looks for a piped command.
 */
char *
check_piped(char *args)
{
	char *p;

	p = args + strlen(args) - 1;

	while (p > args && *p != '|')
		p--;

	if (*p == '|') {
		*p++ = 0;
		trimline(args);
		return p;
	} else
		return NULL;
}

/*
 * Search the command table for a match. If found
 * call the handler function.
 */
cmdlookup(char *tok, char *args)
{
	struct cmdtab *tab;
	int piped = 0;
	char *pipecmd;

	gfp = stdout;
			
	/* Lookup */
	tab = cmdtab;
	while (tab->cmd) {
		if (help_allow(tab->flags) && strcmp(tab->cmd, tok) == 0) {
			
			/* Check if piped */
			if (pipecmd = check_piped(args)) {
				piped = 1;
				gfp = popen(pipecmd, "w");
			}

			(*tab->fn)(args);

			if (piped) {
				fclose(gfp);
				waitpid(-1, NULL, 0);
			}
			return;
		}
		tab++;
	}

	if (*tok == '!') {
		char buf[MAXLINE +1 ];
		sprintf(buf, "%s %s", tok+1, args);
		cmd_shell(buf);
		return;
	}

	/*
	 * Check if this is a singal line definition change
	 * e.g. link tty0 dev = /dev/tty01h
	 */

	if (cmddef(tok, args))
		return;

	/* Tab now points to the last table entry .. cmd_unknown() */

	(*tab->fn)(tok, args);
}

