#ident	"@(#)defs.c	1.8"

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
#include <cs.h>
#include <sys/ppp.h>

#include "fsm.h"
#include "psm.h"
#include "ppp_proto.h"
#include "ppptalk.h"
#include "ulr.h"
#include "act.h"
#include "auth.h"
#include "pathnames.h"
#include "ppptalk_msg.h"

extern int fline, pline;
extern char host[];
struct psm_entry_s *cp_get_tab(char *proto);
void pmsg(int level, int msgno, char *fmt, ...);
extern struct ulr_prim *gmsg;

struct str2val_s def_types[] = {
	"global",	(void *)DEF_GLOBAL,
	"auth",		(void *)DEF_AUTH,
	"algorithm",	(void *)DEF_ALG,
	"protocol",	(void *)DEF_PROTO,
	"link",		(void *)DEF_LINK,
	"bundle",	(void *)DEF_BUNDLE,
	NULL,		NULL,
};


struct def_s {
	char *tok;
	char *args;
	int flags;
	int lineno;
} def[MAXLINES+1];

#define DEF_USED 1

char def_id[MAXTOK + 1];

int line = 0;

def_new(char *id)
{
	strcpy(def_id, id);
	line = 0;
}


def_store(char *tok, char *args)
{
	char *m;

	if (line == MAXLINES) {
		pmsg(MSG_ERROR, PPPTALK_2MAYLINES,
		     "Too many lines in definition (max = %d).\n", MAXLINES);
		return;
	}

	m = (char *)malloc(strlen(tok) + strlen(args) + 2);
	if (!m) {
		pmsg(MSG_ERROR, PPPTALK_NOMEM,
		     "Out of memory !\n");
		exit(1);
	}

	def[line].tok = m;
	def[line].args = m + strlen(tok) + 1;
	def[line].flags = 0;
	def[line].lineno = fline;

	strcpy(def[line].tok, tok);
	strcpy(def[line].args, args);

	line++;
}

/*
 * Check if we have definitions that haven't been consumed - these are errors
 */
int
def_chk_unused()
{
	int i;
	int bad = 0;

	for (i = 0; i < line; i++) {
		if (def[i].flags & DEF_USED)
			continue;
		bad++;
		pline = def[i].lineno;
		pmsg(MSG_DEF_ERROR, PPPTALK_INVALDEF,
		     "Invalid definition: %s = %s\n", def[i].tok, def[i].args);
	}
	return(bad);
}
/*
 * Search the definition for the specified option.
 * If found return a pointer to the entry, otherwise NULL.
 */

char *
def_findopt(char *entry)
{
	int i;

	for (i = 0; i < line; i++)
		if (strcmp(entry, def[i].tok) == 0) {
			def[i].flags |= DEF_USED;
			pline = def[i].lineno;
			return(def[i].args);
		}
	return(NULL);
}

struct str2val_s debug_levels[] = {
	"none",	(void *)DBG_NONE,
	"low",	(void *)DBG_LOW,
	"med",	(void *)DBG_MED,
	"high",	(void *)DBG_HIGH,
	"wire",	(void *)DBG_WIRE,
	"debug", (void *)DBG_DEBUG,
};


struct str2val_s def_link_types[] = {
	"pstn",	(void *)LNK_ANALOG,
	"isdn-sync",	(void *)LNK_ISDN,
	"isdn-async",	(void *)LNK_ISDNVOC,
	"static",	(void *)LNK_STATIC,
	"tcp",	(void*)LNK_TCP,
	NULL,	NULL,
};

/*
 * True and False definitions
 */
struct str2val_s truefalse[] = {
	"enable", (void *)1,
	"enabled", (void *)1,
	"true",	(void *)1,
	"yes",	(void *)1,
	"on",	(void *)1,
	"disable", (void *)0,
	"disabled", (void *)0,
	"false", (void *)0,
	"no",	(void *)0,
	"off",	(void *)0,
	NULL, NULL,
};

/*
 * Parse an option
 *	opt 	- pointer to option description
 *	v	- pointer to destination value (or string entry)
 *	ch	- Pointer to the config header for this option
 */
int
def_parse_opt(struct psm_opttab *opt, unsigned int *v, struct cfg_hdr *ch)
{
	char *p, *p1;
	unsigned int num;
	void *eval;

	p = def_findopt(opt->opt);
	if (!p) {
		switch(opt->type) {
		case BOOLEAN:
		case NUMERIC_F:
		case NUMERIC_R:
			if (opt->flags == MANDATORY && *v == 0) {
				pmsg(MSG_DEF_ERROR, PPPTALK_MAND,
				     "Option %s must be defined\n", 
				     opt->opt);
				return 1;
			};
			break;
		case STRING:
			p1 = (char *)ch + *v;
			if (opt->flags == MANDATORY && *p1 == 0) {
				pmsg(MSG_DEF_ERROR, PPPTALK_MAND,
				     "Option %s must be defined\n", 
				     opt->opt);
				return 1;
			};
			break;
		}
		return 0;
	}

	/*
	 * Okay the option has been defined
	 * 
	 * If NO processing function has been defined, just use the value
	 */
	/*	printf("Option %s defined\n", opt->opt);*/


	switch(opt->type) {
	case NUMERIC_R:
		if (sscanf(p, "%i", &num) != 1) {
			pmsg(MSG_DEF_ERROR, PPPTALK_BADVAL,
			     "Bad value for %s option.\n", 
			     opt->opt);
			return 1;
		}
		/* printf("Value = 0x%x\n", num);*/

		/* Check value is within specified range */
		if (num > (uint_t)opt->arg2 || num < (uint_t)opt->arg1) {
			pmsg(MSG_DEF_ERROR, PPPTALK_BADVALR,
			     "Invalid value for option %s (min %u, max %u)\n",
			     opt->opt,
			     (uint_t)opt->arg1, (uint_t)opt->arg2);
			return 1;
		}
		
		*v = num;
		break;

	case NUMERIC_F:
		eval = (*(int * (*)())(opt->arg1))(opt->arg2, p);
		if ((int)eval < 0) {
			pmsg(MSG_DEF_ERROR, PPPTALK_INVALVAL,
			     "Invalid value for option %s\n",
			     opt->opt);
			return 1;
		}
		/*printf("Value = %d\n", (int)eval);*/
		*v = (unsigned int)eval;
		break;

	case BOOLEAN:
		eval = (void *)def_lookval(truefalse, p);
		*v = (unsigned int)eval;
		break;

	case STRING:
		if (opt->arg1) {

		/*printf("Need to call function to obtain value\n");*/
		
			eval = (*(int * (*)())(opt->arg1))(opt->arg2, p);
			pmsg(MSG_DEF_ERROR, 0,
			     "parse_option: EVAL STRING not implemented\n");
			return 1;
		}

		if (opt->flags == MANDATORY && *p == 0) {
			pmsg(MSG_DEF_ERROR, PPPTALK_NONNULL,
			     "Option %s must be non-null\n", 
			     opt->opt);
			return 1;
		};

		def_insert_str(ch, v, p);
		/* printf("Value = %s\n", p);*/
		break;
	}

	return 0;
}

/*#define CLEANUP*/

int
def_is_used(struct psm_opttab tab[], struct cfg_hdr *ch, int start, int off)
{
	int i = 0;
	int *ip = (int *) &(ch->ch_len) + 1 + start;

	while (tab[i].opt) {

		if (tab[i].type == STRING && *ip == off) {	
#ifdef CLEANUP			
			pmsg(MSG_DEBUG, 0, "Used by  %s (%d)\n", tab[i].opt, *ip);
#endif
			return 1;
		}

		i++;
		ip++;
	}
	return 0;
}

int
def_update_refs(struct psm_opttab tab[], struct cfg_hdr *ch, int start, int off, int len)
{
	int i = 0;
	int *ip = (int *) &(ch->ch_len) + 1 + start;

	while (tab[i].opt) {

		if (tab[i].type == STRING && *ip > off) {	
#ifdef CLEANUP
			pmsg(MSG_DEBUG, 0, "Update  %s (%d now %d)\n", 
			       tab[i].opt, *ip, *ip -len);
#endif
			*ip -= len;
		}

		i++;
		ip++;
	}
}

/* 
 * This function cleans a message, removing any
 * unreferenced strings. This is messy but well, it saves on memory & code
 * in the ppp daemon.
 *
 * 	v  	- points to the start of the strings area in the message.
 *	start 	- this is the number fields to skip in the target message
 *		  and is used (non-zero) when an option table is not
 *		  used to fill in all fields.
 */
void
def_cleanup_msg(struct psm_opttab tab[], struct cfg_hdr *ch, int start, char *v)
{
	char *p = v;
	int len, off;

#ifdef CLEANUP
	pmsg(MSG_DEBUG, 0, "def_cleanup_msg: (ch %x, v %x) len is %d\n", ch, v, ch->ch_len);
#endif
	while (p < (char *)ch + ch->ch_len) {
	
		len = strlen(p) + 1;
		off = p - (char *)ch;
#ifdef CLEANUP
		pmsg(MSG_DEBUG, 0, "Check %s (off %d)\n", p, off);
#endif

		if (!def_is_used(tab, ch, start, (p - (char *)ch))) {
#ifdef CLEANUP
			pmsg(MSG_DEBUG, 0, "Need to remove!\n");
#endif

			def_update_refs(tab, ch, start, off, len);
			
			/* Move the memory */

			memcpy(p, p + len, ch->ch_len - len - off);
			ch->ch_len -= len;
		} else {
#ifdef CLEANUP
			pmsg(MSG_DEBUG, 0, "Keep\n");
#endif
			p += len;
		}
	}

#ifdef CLEANUP
	pmsg(MSG_DEBUG, 0, "def_cleanup_msg: len now %d\n", ch->ch_len);
#endif
}



/*
 * Link definition structures and routines
 */
struct str2val_s def_link_flow[] = {
	"none",		(void *)FLOW_NONE,
	"hardware",	(void *)FLOW_HARD,
	"software",	(void *)FLOW_SOFT,
	NULL, NULL,
};


struct psm_opttab link_type_opt = {
	"type",		NUMERIC_F, OPTIONAL,
	                   (void *)def_lookval, (void *)def_link_types,
};

struct psm_opttab link_opts[] = {
	"dev", 		STRING, OPTIONAL, NULL, NULL,
	"push", 	STRING, OPTIONAL, NULL, NULL,
	"pop", 		STRING, OPTIONAL, NULL, NULL,
	"phone", 	STRING, OPTIONAL, NULL, NULL,
	"flow",		NUMERIC_F, OPTIONAL,
                                  (void *)def_lookval, (void *)def_link_flow,
	"protocols", 	STRING, MANDATORY, NULL, NULL,
	"bandwidth", 	NUMERIC_R, OPTIONAL, (void *)0, (void *)0x7ffffff,
	"debug",	NUMERIC_F, OPTIONAL, 
	                   (void *)def_lookval, (void *)debug_levels,
	NULL,0,0,NULL,NULL,
};

/*
 * Link routines
 */
void
set_default_link(struct cfg_link *l)
{
	l->ln_len = l->ln_var - (char *)l;

	l->ln_bandwidth = 0; /* Decided when connection obtained */

	def_insert_str(&(l->ln_ch), &(l->ln_dev), "");

	switch(l->ln_type) {
	case LNK_STATIC:
		def_insert_str(&(l->ln_ch), &(l->ln_push), "asyh");
		def_insert_str(&(l->ln_ch), &(l->ln_pop), "ttcompat ldterm");
		break;
	case LNK_ANALOG:
		def_insert_str(&(l->ln_ch), &(l->ln_push), "asyh");
		def_insert_str(&(l->ln_ch), &(l->ln_pop), "ttcompat ldterm");
		break;
	case LNK_ISDN:
		def_insert_str(&(l->ln_ch), &(l->ln_push), "");
		def_insert_str(&(l->ln_ch), &(l->ln_pop), "");
		break;
	case LNK_ISDNVOC:
		def_insert_str(&(l->ln_ch), &(l->ln_push), "asyh");
		def_insert_str(&(l->ln_ch), &(l->ln_pop), "ttcompat ldterm");
		break;
	case LNK_TCP:
		def_insert_str(&(l->ln_ch), &(l->ln_push), "asyh");
		def_insert_str(&(l->ln_ch), &(l->ln_pop), "ttcompat ldterm");
		break;
	}

	def_insert_str(&(l->ln_ch), &(l->ln_phone), "");
	l->ln_flow = FLOW_HARD;
	def_insert_str(&(l->ln_ch), &(l->ln_protocols), "");
	l->ln_debug = DBG_NONE;
}

/*
 * Define a link.
 *
 * Incoming message has current vaues - or m_error non-zero if new
 */
int
def_link(int so, struct ulr_cfg *m)
{
	struct cfg_link *l = &(m->uclink);
	unsigned int *ip = &(l->ln_type);
	int ret, i = 0;

	/* See what link type we are defining */
	if (m->error)
		l->ln_type = LNK_ANALOG;
	
	ret = def_parse_opt(&link_type_opt, ip++, &(l->ln_ch));
	if (ret)
		return(ret);

	/* If we didn't find any current values use the defaults */
	if (m->error)
		set_default_link(l);

	m->prim = ULR_SET_CFG;
	m->type = DEF_LINK;

	while (link_opts[i].opt) {
		ret = def_parse_opt(&link_opts[i], ip, &(l->ln_ch));
		if (ret)
			return(ret);
		ip++;
		i++;
	}

	def_cleanup_msg(link_opts, &(l->ln_ch), 1, l->ln_var);

	/* Now do any range checks on the supplied parameters */

	return(def_chk_unused());
}


/*
 * Protcol definition
 */
#define OPT_PROTOCOL "protocol"

int
def_proto(int so, struct ulr_cfg *m)
{
	struct cfg_proto *p = &(m->ucproto);
	unsigned int *ip = &(p->pr_name);
	int ret, i = 0;
	char *strings;	/* Start of strings */
	struct psm_opttab *opts;
	struct psm_entry_s *e;
	char *proto_desc;

	/* If we didn't find any current values use the defaults */

	m->prim = ULR_SET_CFG;
	m->type = DEF_PROTO;

	proto_desc = def_findopt(OPT_PROTOCOL);

	/* Default protocol */
	if (!proto_desc) {
		if (m->error) {
			pmsg(MSG_ERROR, PPPTALK_NOPROTO,
			     "The protocol type must be defined\n");
			return -1;
		}
		proto_desc = def_get_str(&p->pr_ch, p->pr_name);
	} 
	
	e = cp_get_tab(proto_desc);
	if (!e) {
		pmsg(MSG_ERROR, PPPTALK_PROTONAVAIL,
		     "Support for protocol %s not available\n", proto_desc);
		return -1;
	}

	ASSERT(e);

	if (m->error)
		(*e->psm_defaults)(p);

	strings = (char*)p + p->pr_ch.ch_stroff;

	/* The option table is vectored on protocol */
	i = 0;

	opts = e->psm_opts;

	while (opts[i].opt) {
		ret = def_parse_opt(&opts[i], ip, &(p->pr_ch));
		if (ret)
			return(ret);
		ip++;
		i++;
	}

	def_cleanup_msg(opts, &(p->pr_ch), 0, strings);

	/* Now do any range checks on the supplied parameters */

	return(def_chk_unused());
}
/*
 * Protcol definition
 */
#define OPT_ALG "algorithm"

int
def_alg(int so, struct ulr_cfg *m)
{
	struct cfg_alg *p = &(m->ucalg);
	unsigned int *ip = &(p->al_name);
	int ret, i = 0;
	char *strings;	/* Start of strings */
	struct psm_opttab *opts;
	struct psm_entry_s *e;
	char *alg_desc;

	/* If we didn't find any current values use the defaults */

	m->prim = ULR_SET_CFG;
	m->type = DEF_ALG;

	alg_desc = def_findopt(OPT_ALG);

	/* Default protocol */
	if (!alg_desc) {
		if (m->error) {
			pmsg(MSG_ERROR, PPPTALK_NOALG,
			     "The algorithm type must be defined\n");
			return -1;
		}
		alg_desc = def_get_str(&p->al_ch, p->al_name);
	} 
	
	e = cp_get_tab(alg_desc);
	if (!e) {
		pmsg(MSG_ERROR, PPPTALK_ALGNAVAIL,
		     "Support for algorithm %s not available\n", alg_desc);
		return -1;
	}

	ASSERT(e);

	if (m->error)
		(*e->psm_defaults)(p);

	strings = (char*)p + p->al_ch.ch_stroff;

	/* The option table is vectored on protocol */
	i = 0;

	opts = e->psm_opts;

	while (opts[i].opt) {
		ret = def_parse_opt(&opts[i], ip, &(p->al_ch));
		if (ret)
			return(ret);
		ip++;
		i++;
	}

	def_cleanup_msg(opts, &(p->al_ch), 0, strings);
	
	/* Now do any range checks on the supplied parameters */

	return(def_chk_unused());
}

struct str2val_s bod_types[] = {
	"in",	(void *)BOD_IN,
	"out",	(void *)BOD_OUT,
	"any",	(void *)BOD_ANY,	
	"none",	(void *)BOD_NONE,
	NULL,	NULL,
};

struct str2val_s bundle_types[] = {
	"disabled",		(void *)BT_NONE,
	"in",			(void *)BT_IN,
	"out",			(void *)BT_OUT,
	"bi-directional",	(void *)BT_INOUT,
	"bi",			(void *)BT_INOUT,
	NULL,	NULL,
};

/*
 * Supported CHAP algorithms
 */
struct str2val_s def_chap_alg[] = {
	"MD5",	(void *)CHAP_MD5,
	NULL, NULL,
};

/*
 * Outgoing definition structures and routines
 */
struct str2val_s def_bringup_types[] = {
	"automatic",	(void *)OUT_AUTOMATIC,
	"manual",	(void *)OUT_MANUAL,
	NULL,	NULL,
};

/*
 * Bundle definition structures and routines
 */
struct psm_opttab bundle_opts[] = {
	"type",		NUMERIC_F, OPTIONAL,
			   (void *)def_lookval, (void *)bundle_types,
	"protocols", 	STRING, MANDATORY, NULL, NULL,
	"mrru",		NUMERIC_R, OPTIONAL, (void *)256, (void *)16384,
	"ssn", 		BOOLEAN, OPTIONAL, NULL, NULL,
	"maxlinks", 	NUMERIC_R, OPTIONAL, (void *)1, (void *)1024,
	"minlinks", 	NUMERIC_R, OPTIONAL, (void *)1, (void *)1024,
	"links", 	STRING, OPTIONAL, NULL, NULL,
	"ed", 		BOOLEAN, OPTIONAL, NULL, NULL,
	"minfrag", 	NUMERIC_R, OPTIONAL, (void *)50, (void *)4096,
	"maxfrags", 	NUMERIC_R, OPTIONAL, (void *)1, (void *)100,
	"linkidle",	NUMERIC_R, OPTIONAL, (void *)1, (void *)30,
	"nulls",	BOOLEAN, OPTIONAL, NULL, NULL,
	"addload", 	NUMERIC_R, OPTIONAL, (void *)1, (void *)100,
	"dropload", 	NUMERIC_R, OPTIONAL, (void *)1, (void *)100,
	"addsample", 	NUMERIC_R, OPTIONAL, (void *)2, (void *)4000,
	"dropsample", 	NUMERIC_R, OPTIONAL, (void *)2, (void *)4000,
	"thrashtime", 	NUMERIC_R, OPTIONAL, (void *)5, (void *)4000,
	"maxidle", 	NUMERIC_R, OPTIONAL, (void *)0, (void *)32767,
	"bod", 		NUMERIC_F, OPTIONAL,
			   (void *)def_lookval, (void *)bod_types,
	"debug",	NUMERIC_F, OPTIONAL, 
	                   (void *)def_lookval, (void *)debug_levels,
	/* Incoming type fields */
	"login",	STRING, OPTIONAL, NULL, NULL,
	"callerid",	STRING, OPTIONAL, NULL, NULL,
	"authid",	STRING, OPTIONAL, NULL, NULL,

	/* Outgoing type fields */
	"remotesys",	STRING, OPTIONAL, NULL, NULL,
	"phone",	STRING, OPTIONAL, NULL, NULL,
	"bringup",	NUMERIC_F, OPTIONAL,
	                    (void *)def_lookval, (void *)def_bringup_types,

	/* Auth fields */
	"requirepap",	BOOLEAN, OPTIONAL, NULL, NULL,
	"requirechap",	BOOLEAN, OPTIONAL, NULL, NULL,
	"authname",	STRING, OPTIONAL, NULL, NULL,
	"peerauthname",	STRING, OPTIONAL, NULL, NULL,
	"chapalg",	NUMERIC_F, OPTIONAL,
	                     (void *)def_lookval, (void *)def_chap_alg,
	"authtmout",	NUMERIC_R, OPTIONAL, (void *)5, (void *)300,

	NULL, 0, 0, NULL, NULL,
};

int
set_default_bundle(struct cfg_bundle *b)
{
	b->bn_len = b->bn_var - (char *)b;

	def_insert_str(&(b->bn_ch), &(b->bn_protos), "");
	b->bn_mrru = DEFAULT_MRRU;
	b->bn_ssn = DEFAULT_SSN;

	b->bn_maxlinks = DEFAULT_MAXLINKS;
	b->bn_minlinks = DEFAULT_MINLINKS;

	def_insert_str(&(b->bn_ch), &(b->bn_links), "");

	b->bn_ed = DEFAULT_ED;
	b->bn_minfrag = DEFAULT_MINFRAG;
	b->bn_maxfrags = DEFAULT_MAXFRAGS;
	b->bn_mlidle = DEFAULT_MLIDLE;
	b->bn_nulls = DEFAULT_MLNULLS;
	b->bn_addload = DEFAULT_ADDLOAD;
	b->bn_dropload = DEFAULT_DROPLOAD;
	b->bn_addsample = DEFAULT_ADDSAMPLE;
	b->bn_dropsample = DEFAULT_DROPSAMPLE;
	b->bn_thrashtime = DEFAULT_THRASHTIME;
	b->bn_maxidle = DEFAULT_MAXIDLE;
	b->bn_bod = DEFAULT_BOD;
	b->bn_debug = DBG_NONE;

	b->bn_type = DEFAULT_BTYPE;

	def_insert_str(&(b->bn_ch), &(b->bn_uid), "");
	def_insert_str(&(b->bn_ch), &(b->bn_cid), "");
	def_insert_str(&(b->bn_ch), &(b->bn_authid), "");

	def_insert_str(&(b->bn_ch), &(b->bn_remote), "");
	def_insert_str(&(b->bn_ch), &(b->bn_phone), "");
	b->bn_bringup = DEFAULT_BRINGUP;

	b->bn_pap = DEFAULT_PAP;
	b->bn_chap = DEFAULT_CHAP;
	def_insert_str(&(b->bn_ch), &(b->bn_authname), "");
	def_insert_str(&(b->bn_ch), &(b->bn_peerauthname), "");
	b->bn_chapalg = CHAP_MD5;
	b->bn_authtmout = DEFAULT_AUTHTMOUT;
	
}

def_bundle(int so, struct ulr_cfg *m)
{
	struct cfg_bundle *b = &(m->ucbundle);
	unsigned int *ip = &(b->bn_type);
	int ret, i = 0;

	/* If we didn't find any current values use the defaults */
	if (m->error)
		set_default_bundle(b);

	m->prim = ULR_SET_CFG;
	m->type = DEF_BUNDLE;

	while (bundle_opts[i].opt) {
		ret = def_parse_opt(&bundle_opts[i], ip, &(b->bn_ch));
		if (ret)
			return(ret);
		ip++;
		i++;
	}

	def_cleanup_msg(bundle_opts, &(b->bn_ch), 0, b->bn_var);
	
	/* Now do any range checks on the supplied parameters */

	return(def_chk_unused());
}

struct str2val_s auth_protos[] = {
	"pap",	(void *)PROTO_PAP,
	"chap",	(void *)PROTO_CHAP,
	NULL,	NULL,
};

/*
 * Authentication definition structures and routines
 */
struct psm_opttab auth_opts[] = {
	"protocol",	NUMERIC_F, OPTIONAL,
	                   (void *)def_lookval, (void *)auth_protos,
	"name",		STRING, MANDATORY, NULL, NULL,
	"peersecret",	STRING, OPTIONAL, NULL, NULL,
	"localsecret",	STRING, OPTIONAL, NULL, NULL,
	NULL, 0, 0, NULL, NULL,
};

void
set_default_auth(struct cfg_auth *a)
{
	a->au_len = a->au_var - (char *)a;
	def_insert_str(&(a->au_ch), &(a->au_name), "");
	def_insert_str(&(a->au_ch), &(a->au_peersecret), "");
	def_insert_str(&(a->au_ch), &(a->au_localsecret), "");
	a->au_protocol = PROTO_CHAP;
}

int
def_auth(int so, struct ulr_cfg *m)
{
	struct cfg_auth *a = &(m->ucauth);
	unsigned int *ip = &(a->au_protocol);
	int ret, i = 0;

	/* If we didn't find any current values use the defaults */
	if (m->error)
		set_default_auth(a);

	m->prim = ULR_SET_CFG;
	m->type = DEF_AUTH;

	while (auth_opts[i].opt) {
		ret = def_parse_opt(&auth_opts[i], ip, &(a->au_ch));
		if (ret)
			return(ret);
		ip++;
		i++;
	}

	def_cleanup_msg(auth_opts, &(a->au_ch), 0, &a->au_var[0]);

	/* Now do any range checks on the supplied parameters */

	return(def_chk_unused());
}

struct psm_opttab global_opts[] = {
	"type", 	NUMERIC_F, MANDATORY,
	                     (void *)def_lookval, (void *)def_types,
	"requirepap", 	BOOLEAN, OPTIONAL, NULL, NULL,
	"requirechap", 	BOOLEAN, OPTIONAL, NULL, NULL,
	"authname", 	STRING, OPTIONAL, NULL, NULL,
	"peerauthname",	STRING, OPTIONAL, NULL, NULL,
	"chapalg", 	NUMERIC_F, OPTIONAL,
	                     (void *)def_lookval, (void *)def_chap_alg,
	"authtmout",	NUMERIC_R, OPTIONAL, (void *)5, (void *)300,
	"mrru",		NUMERIC_R, OPTIONAL, (void *)256, (void *)16384,
	"ssn",		BOOLEAN, OPTIONAL, NULL, NULL,
	"ed",		BOOLEAN, OPTIONAL, NULL, NULL,
	NULL, 0, 0, NULL, NULL,
};
void

set_default_global(struct cfg_global *gl)
{
	gl->gi_len = gl->gi_var - (char *)gl;

	gl->gi_type = DEF_BUNDLE;
	gl->gi_pap = DEFAULT_INC_PAP;
	gl->gi_chap = DEFAULT_INC_CHAP;
	
	def_insert_str(&(gl->gi_ch), &(gl->gi_authname), host);
	def_insert_str(&(gl->gi_ch), &(gl->gi_peerauthname), "");

	gl->gi_chapalg = CHAP_MD5;
	gl->gi_authtmout = DEFAULT_AUTHTMOUT;
	gl->gi_mrru = DEFAULT_MRRU;
	gl->gi_ssn = DEFAULT_SSN;
	gl->gi_ed = DEFAULT_ED;
}

#define OPT_TYPE "type"

int
def_global(int so, struct ulr_cfg *m)
{
	struct cfg_global *gl = &(m->ucglobal);
	unsigned int *ip = &(gl->gi_type);
	int ret, i = 0;
	char *type_desc;

	type_desc = def_findopt(OPT_TYPE);
	if (!type_desc) {
		if (m->error) {
			pmsg(MSG_ERROR, PPPTALK_NOTYPE,
			     "The type option must be defined\n");
			return -1;
		}
		type_desc = def_types[gl->gi_type].tv_str;
	} 

	if (strcmp(m->ucid, type_desc) != 0) {
		pmsg(MSG_ERROR, PPPTALK_GLOBALMATCH,
		     "global definition tags must match type option.\n");
		return -1;
	}

	/* If we didn't find any current values use the defaults */
	if (m->error)
		set_default_global(gl);

	m->prim = ULR_SET_CFG;
	m->type = DEF_GLOBAL;

	while (global_opts[i].opt) {
		ret = def_parse_opt(&global_opts[i], ip, &(gl->gi_ch));
		if (ret)
			return(ret);
		ip++;
		i++;
	}

	def_cleanup_msg(global_opts, &(gl->gi_ch), 0, gl->gi_var);
	
	/* Now do any range checks on the supplied parameters */

	if (gl->gi_type != DEF_BUNDLE) {
		pmsg(MSG_ERROR, PPPTALK_BUNDLESUPPT,
		     "Only bundle type global definitions are supported.\n");
		return -1;
	}

	return(def_chk_unused());
}
/*
 * Main definition processing function
 * 
 * Check the syntax of the stored definition,
 * the send the definition to the PPP Daemon.
 *
 * Valid definition types are
 *	Global, Authentication, Algorithm, Link, Protocol,  Bundle.
 *
 */
int (*def_fn[])() = {
	def_global,
	def_auth,
	def_alg,
	def_proto,
	def_link,
	def_bundle,
};

#define OPT_DEFINITION	"definition"

def_process()
{
	int i, ret, t, l;
	char *p;
	struct ulr_cfg *msg = (struct ulr_cfg *)gmsg;
	extern int so;
	extern def_count;

	p = def_findopt(OPT_DEFINITION);
	if (!p) {
		pmsg(MSG_ERROR, 0,
		     "Internal error - couldn't find 'definition'\n");
		goto exit;
	}

	/*
	 * Get the definition type, we need this to determine which
	 * set of options are legal
	 */
	l = (int)def_lookval(def_types, p);
	if (l < 0) {
		pmsg(MSG_ERROR, PPPTALK_BADOPTVAL,
		     "%s is not a valid %s value\n",
		     p, OPT_DEFINITION);
		goto exit;
	}

	t = cfg_findid(so, msg, def_id, l);
	if (t >= 0 && t != l) {
		pmsg(MSG_ERROR, 0, "Internal error\n");
		goto exit;
	}
	t = l;

	ret = (*def_fn[t])(so, msg);
	if (!ret) {
		ret = ulr_sr(so, msg, msg->uclen + sizeof(struct ulr_cfg),
			     ULR_MAX_MSG);
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
			     "Unexpected error from pppd (%d)\n", ret);
			break;
		}
	}
	def_count++;
	
exit:
	/* Free off the defs */
	for (i = 0; i < line; i++)
		free(def[i].tok);
	return(ret);
}

/*
 * Copy next element from s to d (it is assumed that the
 * buffer we are given is large enough to hold the element, MAXID).
 * Return a pointer to character after next element
 */
char *
def_get_element(char *s, char *d)
{
	int len = 0;

	while (*s && !isspace(*s) && len < MAXID)
		d[len++] = *s++;

	d[len] = 0;

	if (len == MAXID) {
		if (*s && !isspace(*s)) {
			pmsg(MSG_WARN, 0,
			       "Truncated element in list to %s\n", d);
		}

		/* Too long */
		while (*s && !isspace(*s))
			s++;
	}

	while(*s && isspace(*s))
		s++;

	return s;
}
