#ident	"@(#)optim:common/optim.h	1.45"
#ifndef OPTIM_H
#define OPTIM_H

/*	machine independent include file for code improver */

#include <stdio.h>
#include "defs.h"

/* booleans */

typedef int boolean;
#define false	0
#define true	1

/* predefined "opcodes" for various nodes */

#define GHOST	0		/* temporary, to prevent linking node in */
#define TAIL	0		/* end of text list */
#define MISC	1		/* miscellaneous instruction */
#define FILTER	2		/* nodes to be filtered before optim */

extern void ldanal();

typedef struct sdependent {           /*linked list of dependent*/
	struct sdependent *next;      /*nodes, usefull for      */
	struct node *depends;         /*schedule.               */
	} tdependent;

/* struct for tracing */
typedef struct tp
{
	char *value;
	int tag;
	unsigned reg;
	int kind;
} 
trace_type;

typedef struct sp
{
	unsigned int last_set;
	unsigned int last_use;
	unsigned int last_xor;
	unsigned int ps_regs;
	struct sp *next_sp;
}
spy_type;


/* structure of each text node */

typedef struct node {
	struct node *forw;	/* forward link */
	struct node *back;	/* backward link */
	tdependent  *depend[4]; /* dependent instructions */
	unsigned int sets;
	unsigned int uses;
	unsigned int idxs;
	unsigned int idus;
	int         dependents; /* # of dependent sons */
	int         chain_length; /* longest depending path */
	int         usage; /*used for marking at loops.c and sched.c*/
	int         nparents; /* on how many instructions it depends*/
	int			zero_op1;
	int			ebp_offset;
	int 		esp_offset;
	union		x_u {
		int         pairtype;   /* for P5, pairability */
		struct	x_s { /* for P6, several data */
			unsigned int ports : 4;
			unsigned int uops  : 4;
			/*unsigned int latency : 4;*/
			unsigned int weight : 24;
		} rs_info;
	}  exec_info;
	char *ops[6];	/* opcode or label and operand field strings */
	IDTYPE uniqid;		/* unique identification for this node */
	unsigned short op;	/* operation code */
	unsigned short rv_regs;	/* # regs used for return value - calls */
	unsigned
	    nlive:32,	/* registers used by instruction */
	    ndead:32;	/* registers set but not used by instruction */
	unsigned
	    nrlive:32,	/* registers used by instruction */
	    nrdead:32;	/* registers set but not used by instruction */
	USERTYPE userdata;	/* user-defined data for this node */
	int extra;		/* used in enter-leave removal, if non-negative
				    contains the stack size on execution for this instruction */
	int extra2;    /*used together with extra to count changes in index
					register values. */
	int sasm;		/* is instruction a safe asm */
	trace_type trace[8];
    spy_type spy;
	int save_restore; /* for fixed stack use only, mark saves and restores. */
#if EH_SUP
	int in_try_block;
#endif
} NODE;

typedef struct function_data {
    int number;   /* the function number */
    int frame;    /* This number is the initial value of frame size */
    int int_off;  /* this number negative plus fr_sz is the offset for pushing */
    int pars; /* holds number of parameters saved for restoring purposes */
	int regs_spc;  /* holds the number of bytes thath saved resigters take */
	int two_entries; /*does this function have two entry points ? */
} FUNCTION_DATA;

/* values for the extra field above */
#define REMOVE -1
#define NO_REMOVE -2
#define TMPSRET -3

#define opcode	ops[0]
#define op1	ops[1]
#define op2	ops[2]
#define op3	ops[3]
#define op4	ops[4]
#define op5	ops[5]
#define op6	ops[6]

/* block of text */

typedef struct block {
	struct block *next;	/* pointer to textually next block */
	struct block *prev;	/* pointer to textually previous block */
	struct block *nextl;	/* pointer to next executed block if no br */
	struct block *nextr;	/* pointer to next executed block if br */
	struct block *ltest;	/* for loop termination tests */
	NODE *firstn;		/* first text node of block */
	NODE *lastn;		/* last text node of block */
	short index;		/* block index for debugging purposes */
	short length;		/* number of instructions in block */
	short indeg;		/* number of text references */
	short indeg2;		/* number of text references, compare to indeg */
	short marked;		/* marker for various things */
	int	entry_depth;
	int exit_depth;
	trace_type trace0[8];
	spy_type spy0; /* contents of regs on entrance of the basic block */
} BLOCK;

/* structure of non-branch text reference node */

typedef struct ref {
	char *lab;		/* label referenced */
	struct ref *nextref;	/* link to next ref */
	char *switch_table_name;
	BLOCK *switch_entry;
} REF;

typedef struct switch_tbl {
	REF *first_ref;	/* point to first label of switch in the ref list */
	REF *last_ref;	/* point to last label of switch in the ref list */
	char *switch_table_name; 
	struct switch_tbl *next_switch;
} SWITCH_TBL;

/* externals */

extern NODE n0;			/* header node of text list */
extern NODE ntail;		/* trailer node of text list */
extern NODE *lastnode;		/* pointer to last node on text list */
extern BLOCK b0;
extern REF r0;			/* header node of reference list */
extern REF *lastref;		/* pointer to last label reference */
extern SWITCH_TBL sw0;  /* header of switch table list */
extern int pic_flag;
extern int ieee_flag;
extern int dflag;
extern int lflag;		/* print loop labels to stderr */
extern int ninst;		/* total # of instructions */
extern FUNCTION_DATA func_data;
extern unsigned int frame_reg;

extern int last_branch;
#if EH_SUP
extern int try_block_nesting, try_block_index;
#endif

extern unsigned int regs_for_incoming_args;
extern unsigned int regs_for_out_going_args;

extern void drive_indegree3();
extern void drive_indegree2();
extern void indegree();
extern void sets_and_uses();
extern void window();
extern void regal_ldanal();
extern int is_jmp_ind();
extern NODE *Saveop();
extern boolean sameaddr();
extern char *getspace(), *xalloc();
extern void addref(), fatal(), init(), optim(), prtext(), xfree();
extern void start_switch_tbl(), end_switch_tbl();
extern void bldgr();
extern void set_refs_to_blocks();
extern int same_inst();

enum { blend_opts=0x1, i486_opts=0x2, ptm_opts=0x4, p6_opts=0x8 };  
/* defines target processor optimizations */

/* user-supplied functions or macros */

extern char *getp();
extern char *newlab();

#define saveop(opn, str, len, op) \
	    (void) Saveop((opn), (str), (unsigned)(len), (unsigned short)(op))
#define addtail(p)		/* superfluous */
#define appinst()		/* superfluous */
#define appmisc(str, len)	saveop(0, (str), (len), MISC)
#define appfl(str, len)		saveop(0, (str), (len), FILTER)
#define applbl(str, len) \
	(setlab(Saveop(0, (str), (unsigned)(len),MISC)), --ninst)
#define ALLN(p)			p = n0.forw; p != &ntail; p = p->forw
#define PRINTF			(void) printf
#define FPRINTF			(void) fprintf
#define SPRINTF			(void) sprintf
#define PUTCHAR(c)		(void) putchar(c)
#define DELNODE(p)		((p)->back->forw = (p)->forw, \
				    (p)->forw->back = (p)->back)
#define APPNODE(p, q)		PUTNODE((p), (q), back, forw)
#define INSNODE(p, q)		PUTNODE((p), (q), forw, back)
#if EH_SUP
/* If EH support and not currently processing instructions within a
   set of try or catch block  comments, duplicate the "in_try_block"
   field into the "new" node. */
#define PUTNODE(p, q, f, b)	((p)->f = (q), (p)->b = (q)->b, \
				    (q)->b->f = (p), \
				    (q)->b = (p), \
				    ((try_block_nesting) \
					? ((p)->in_try_block = try_block_index) \
					: ((p)->in_try_block = (q)->in_try_block)))
#else
#define PUTNODE(p, q, f, b)	((p)->f = (q), (p)->b = (q)->b, \
				    (q)->b->f = (p), \
				    (q)->b = (p))
#endif
#define GETSTR(type)		(type *) getspace(sizeof(type))
#define COPY(str, len)	((len) != 0 ? \
	(char *)memcpy(getspace(len), str, (int)(len)) : str)

/* heuristics for branch reversal */
#define CMP_DEREF               0x00000002
#define GUARD_CALL              0x00000004
#define GUARD_LOOP              0x00000008
#define GUARD_SHORT_BLOCK       0x00000010
#define CMP_GLOB_CONST          0x00000020
#define CMP_PARAM_CONST         0x00000040
#define CMP_PTR_ERR             0x00000080
#define CMP_GLOB_PARAM          0x00000100
#define CMP_2_AUTOS             0x00000200
#define CMP_LOGICAL             0x00000400
#define CMP_USE_SHIFT           0x00000800
#define SHORT_IF_THEN_ELSE      0x00001000
#define CMP_FUNC_CONST          0x00002000
#define GUARD_RET               0x00004000
#define COMPOUND                0x40000000
#define LOOP_BR                 0x80000000

#endif
