#ident	"@(#)parse.c	1.6"

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "ppptalk.h"
#include "fsm.h"

int ppp_debug;

/*
 * The following functions are provided for use buy protocol parser modules.
 */
struct str2val_s *
def_looktab(struct str2val_s *tab, char *str)
{
	while (tab->tv_str) {

		if (strcmp(tab->tv_str, str) == 0)
			return(tab);
		tab++;
	}
	return(NULL);
}

int
def_lookval(struct str2val_s *tab, char *str)
{
	while (tab->tv_str) {

		if (strcmp(tab->tv_str, str) == 0)
			return((int)tab->tv_val);
		tab++;
	}
	return(-1);
}

void
def_insert_str(struct cfg_hdr *ch, unsigned int *off, char *src)
{
	char *p;

	p = (char *)ch + ch->ch_len;
	*off = ch->ch_len;
	strcpy(p, src);

	ch->ch_len += strlen(src) + 1;
}

char *
def_get_str(struct cfg_hdr *ch, unsigned int off)
{
	return((char *)ch + off);
}

char *
def_get_bool(int bool)
{
	return bool ? "enabled" : "disabled";
	/*	return bool ? "true" : "false";*/
}

static char *fsm_states[] = {
	"Ready",	/* INITIAL */
	"Negotiating",	/* STARTING */
	"Closed",	/* CLOSED */
	"Failed",	/* STOPPED */
	"Closing",	/* CLOSING */
	"Failed",	/* STOPPING */
	"Negotiating",	/* REQSENT */
	"Negotiating",	/* ACKRCVD */
	"Negotiating",	/* ACKSENT */
	"Succeeded",	/* OPENED */
};

char *
psm_display_flags(int bits, char *strs[], uint_t flags)
{
	char buf[1024];
	uint_t i;

	*buf = 0;
	for (i = 0; i < bits; i++) {
		if (flags & (1 << i)) {
			if (*buf != 0)
				strcat(buf, ", ");
			strcat(buf, strs[i]);
		}
	}
	return buf;
}

/*
 * Display reasons for failure
 */
static char *fsm_reasons[] = {
	"Max Failure",
	"Max Configure",
	"Local Terminated",
	"Remote Terminated",
	"Protocol Rejected",
	"0x0020",
	"0x0040",
	"0x0080",
	"0x0100",
	"0x0200",
	"0x0400",
	"0x0800",
	"0x1000",
	"0x2000",
	"0x4000",
	"0x8000",
};

char *
fsm_reason(struct proto_state_s *ps)
{
	return psm_display_flags(5, fsm_reasons, ps->ps_reason);
}

char *
fsm_state(struct proto_state_s *ps)
{
	return fsm_states[ps->ps_state];
}

char *
fsm_pktcnts(struct proto_state_s *ps)
{
	static char buf[80];

	sprintf(buf, "Rx %d, Rx bad %d, Tx %d",
		ps->ps_rxcnt, ps->ps_rxbad, ps->ps_txcnt);
	return buf;
}

int
psm_assert(char *x, char *f, int l)
{
}
