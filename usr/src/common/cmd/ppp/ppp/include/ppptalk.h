#ifndef _PPPTALK_H
#define _PPPTALK_H

#ident	"@(#)ppptalk.h	1.3"

#include "fsm.h"

#define MAXTOK 40 /* Maximum length of any token, option or definition name */
#define MAXLINE 2048 /* Maximum length of a line in the configuration */
#define MAXLINES 255 /* Maximum number of lines in a definition */

/*
 * Option table definiitions
 */
enum {
	OPTIONAL = 0,
	MANDATORY
};

enum {
	NUMERIC_F = 0,	/* Use function */
	NUMERIC_R,	/* Has range */
	STRING,
	BOOLEAN,
};

struct psm_opttab {
	char 	*opt;
	int	type;
	int	flags;
	void 	*arg1;
	void	*arg2;
};

/*
 * This structure defines the entry points that must be provided by a Control
 * Protocol for ppptalk support.
 */
struct psm_entry_s {
	struct psm_opttab *psm_opts;
	void    (*psm_defaults)();
	void    (*psm_list)();
	int	(*psm_status)();
	uchar_t (*psm_stats)();
	uint_t	psm_flags;
};

/* 
 * verbosity levels for psm_status 
 */
#define PSM_V_SILENT 0
#define PSM_V_SHORT  1
#define PSM_V_LONG   2

/*
 * Bits for psm_flags
 */
#define PSMF_LINK	0x0001	/* This protocol can be configured for Links */
#define PSMF_BUNDLE	0x0002	/* This protocol can be cfg'ed for Bundles */
#define PSMF_NCP	0x0004	/* This is a Network Control Protocol */
#define PSMF_CCP	0x0008	/* This is a Compression Protocol */
#define PSMF_ECP	0x0010	/* This is an Encryption Protocol */
#define PSMF_AUTH	0x0020	/* This is an Auth proto */
#define PSMF_FSM	0x0040	/* Uses the FSM */

struct str2val_s {
	char	*tv_str;
	void   	*tv_val;
};

char *def_get_str(struct cfg_hdr *ch, unsigned int off);
void def_insert_str(struct cfg_hdr *ch, unsigned int *off, char *src);
int def_lookval(struct str2val_s *tab, char *str);
struct str2val_s *def_looktab(struct str2val_s *tab, char *str);
char *def_get_bool(int bool);


/*
 * Standard FSM options
 */

#define FSM_OPT_NAMES \
	"reqtmout",	NUMERIC_R, OPTIONAL, (void *)1, (void *)100, \
	"maxterm",	NUMERIC_R, OPTIONAL, (void *)1, (void *)100, \
	"maxcfg",	NUMERIC_R, OPTIONAL, (void *)1, (void *)100, \
	"maxfail",	NUMERIC_R, OPTIONAL, (void *)1, (void *)300

#define FSM_OPT_DEFAULTS(fsm) \
        (fsm)->fsm_req_tmout = 3; /* Three seconds */ \
        (fsm)->fsm_max_trm = 2; \
        (fsm)->fsm_max_cfg = 10; \
        (fsm)->fsm_max_fail = 5;


#define FSM_OPT_DISPLAY(fp, fsm) \
	fprintf((fp), "	reqtmout = %d\n", (fsm)->fsm_req_tmout); \
	fprintf((fp), "	maxterm = %d\n", (fsm)->fsm_max_trm); \
	fprintf((fp), "	maxcfg = %d\n", (fsm)->fsm_max_cfg); \
	fprintf((fp), "	maxfail = %d\n", (fsm)->fsm_max_fail); 

/*
 * Some prototyes
 */
char *psm_display_flags(int bits, char *strs[], uint_t flags);
char *fsm_reason(struct proto_state_s *ps);
char *fsm_state(struct proto_state_s *ps);
char *fsm_pktcnts(struct proto_state_s *ps);

#endif /* _PPPTALK_H */
