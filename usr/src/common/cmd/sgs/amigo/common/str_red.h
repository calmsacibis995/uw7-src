#ident	"@(#)amigo:common/str_red.h	1.1"

typedef struct cand_descr *Cand_descr; 

struct cand_descr {
	int in_table;
	int cand_type;
	Setid iv_id; /* if ind_var or candidate, identifies ind var */
	long update;	/* value if icon, update value if candidate,
			   increment from defining expression, if ind var */
	Cand_descr next; /* Use for linked list of candidates in loop */
	SX temp; /* Associated temp */
	Cgq_index update_index;
};

#define SR_IV_ID(cand) ( cand->iv_id )
#define SR_TEMP(cand) ( cand->temp )
#define SR_UPDATE_INDEX(cand) cand->update_index
#define SR_UPDATE(cand) cand->update
#define SR_NEXT_CAND(cand) cand->next
extern void PROTO(debug_str_cands,	(Cand_descr list));
