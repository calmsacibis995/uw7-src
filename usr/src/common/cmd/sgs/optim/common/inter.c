#ident	"@(#)optim:common/inter.c	1.37"
#include "optim.h"
#include <varargs.h>


extern int in_safe_asm;
extern int in_intrinsic;
NODE *lastnode;	/* reference to node being built by Saveop */

	NODE *
Saveop(opn, str, len, op) /* save part of inst */
     register int opn; 
     register char *str;
     unsigned len;
     unsigned short op; {

	register NODE *p = lastnode;
	if (opn == 0) { /* make a new node and link it in */
		p = lastnode = GETSTR(NODE);
		if ((p->op = op) != GHOST) {
			INSNODE(p, &ntail);
			ninst++;
			if (in_safe_asm) p->op += SAFE_ASM;
		}
		p->extra = NO_REMOVE;
		for (op = 1; (int)op <= (int)MAXOPS + 1; ++op )
			p->ops[op] = NULL;
		p->rv_regs = 0;
		p->nlive = p->ndead = 0;
 		p->nrlive = p->nrdead = 0;
		p->ebp_offset = 0;
		p->esp_offset = 0;
		p->zero_op1 = 0;
		p->sasm = 0;
		p->extra2 = 0;
		p->usage = in_intrinsic;
		p->uniqid = IDVAL;
		p->userdata = USERINITVAL;
		p->save_restore = 0;
#if EH_SUP
		if (try_block_nesting) p->in_try_block = try_block_index;
		else p->in_try_block = 0;
#endif
	}
	if (opn < 0 || opn > MAXOPS)
		fatal(__FILE__,__LINE__,"invalid opn field for %s\n", str);
	if (len) { /* clean space in the end of the strings */
		len = strlen(str) - 1; /* real ( not len) string size - 2 */
		while((( (int) len) >= 0) && isspace(str[len]))
			len--;  /* remove space from the end */
		p->ops[opn] = COPY(str, len+2);
		p->ops[opn][len+1] = '\0'; /* Needed for the case space in the end */
	} else 
		p->ops[opn] = str;
	if (p->ops[opn] && ( !strncmp(p->ops[opn],"%cr",3)
		|| !strncmp(p->ops[opn],"%tr",3) || !strncmp(p->ops[opn],"%dr",3)))
		*p->ops[opn] = '&'; /*cr should not look like a register*/
	if (op == TSRET)  /* TMPRET */
		p->extra = TMPSRET;
	if (opn == 0 && p->ops[0][0] == 'j') { /* a branch */
		p->dependents = last_branch;
		last_branch = 0;
	}
	if (   opn == 0
		&& !strncmp(p->ops[0],"call",4)) {
		p->zero_op1 |= regs_for_out_going_args;
		regs_for_out_going_args = 0;
	}
	return (p);
}
static SWITCH_TBL *s;
	void
start_switch_tbl() {
	s  = GETSTR(SWITCH_TBL);
	s->first_ref = lastref;
}
	void 
end_switch_tbl(str) char *str; {
	s->first_ref = s->first_ref->nextref->nextref;
	s->last_ref = lastref;
	s->switch_table_name = str;
	s->next_switch = sw0.next_switch;
	sw0.next_switch = s;
}
	void
addref(str, len,str2) char *str,*str2; unsigned len; {
/* add text ref to reference list */

	register REF *r = lastref = lastref->nextref = GETSTR(REF);

	r->lab = COPY(str, len);
	r->nextref = NULL;
	r->switch_table_name = str2;
}

	void
prtext() { /* print text list */

	extern void prinst();
	register NODE *p;

	for (ALLN(p)) {
		prinst(p);
	}
}

	boolean
same_inst(p, q) NODE *p, *q; { /* return true iff nodes are the same */


	register char **pp, **qq;
	register int i;

	if (p->op != q->op)
		return (false);

	/* first check for equal numbers of active operands */

	for (pp = p->ops, qq = q->ops, i = MAXOPS + 1;
	    --i >= 0 && (*pp != NULL || *qq != NULL); pp++, qq++)
		if (*pp == NULL || *qq == NULL)
			return (false);

	/* then check for equality of the active operands */

	while (pp > p->ops)
		if (**--pp != **--qq || strcmp(*pp, *qq))
			return (false);
	return (true);
}

	boolean
sameaddr(p, q) NODE *p, *q; { /* return true iff ops[1...] are the same */

	register char **pp, **qq;
	register int i;

	/* first check for equal numbers of active operands */

	for (pp = p->ops, qq = q->ops, i = MAXOPS + 1;
	    --i >= 0 && (*pp != NULL || *qq != NULL); pp++, qq++)
		if (*pp == NULL || *qq == NULL)
			return (false);

	/* then check for equality of the active operands */

	while (pp > p->ops + 1)
		if (**--pp != **--qq || strcmp(*pp, *qq))
			return (false);
	return (true);
}

	char *
xalloc(n) register unsigned n; { /* allocate space */

	extern char *malloc();
	register char *p;

	if ((p = malloc(n)) == NULL)
		fatal(__FILE__,__LINE__,"out of space\n");
	return (p);
}

	void
xfree(p) char *p; { /* free up space allocated by xalloc */

	extern void free();

	free(p);			/* return space */
}

/* VARARGS */
	void
fatal(file,line,va_alist)
char *file; int line;
va_dcl
{
#ifdef DEBUG
	va_list args;
	char *fmt;
	extern void exit();

	va_start(args);
	FPRINTF(stderr, "Optimizer, %s %d: ",file,line);
	fmt=va_arg(args,char *);
	(void)vfprintf(stderr, fmt, args);
	va_end(args);
#else
	fprintf(stderr,"%s %d\n",file,line);
#endif
	exit(2);
}
