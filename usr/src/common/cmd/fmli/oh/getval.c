/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:oh/getval.c	1.19.3.4"

#include	<stdio.h>
#include	<string.h>
#include	"inc.types.h"	/* abs s14 */
#include	<sys/stat.h>
#include	<ctype.h>
#include        <signal.h>
#include        <curses.h>
#include	"wish.h"
#include	"token.h"
#include	"winp.h"
#include	"form.h"
#include	"fm_mn_par.h"
#include	"objform.h"
#include	"var_arrays.h"
#include	"terror.h"
#include	"eval.h"
#include	"ctl.h"
#include 	"moremacros.h"
#include	"interrupt.h"

extern int EV_retcode;

extern void intr_handler();
extern void un_validate();              /* abs s13 */

char *pline();
void part_expand();
void set_def();
void evalattr();


#define SKIPLINES TRUE
#define NONE 0
#define BQMODE 1
#define DQMODE 2

#define getac(fp, Q) ((fp) ? Getc(fp) : *(Q)++)
#define UNGetac(C, fp, Q) ((fp) ? unGetc((C), (fp)) : *(Q)--)


/*
** Evaluate one of the single occurrence attributes.
*/

/* Les: replace with MACRO

char *
sing_eval(fm_mn, fldtype)
struct fm_mn *fm_mn;
int fldtype;
{
	char *fld_eval();

	return(fld_eval(&fm_mn->single, fldtype, fm_mn->seqno));
}
*/

/*
** Evaluate one of the multi-occurrence attributes.
*/

/* Les: replace with 1 line function

char *
multi_eval(fm_mn, fldno, fldtype)
struct fm_mn *fm_mn;
int fldno, fldtype;
{
	char *fld_eval();

	return(fld_eval(fm_mn->multi + fldno, fldtype, fm_mn->seqno));
}
*/

/*
** Evaluate one attribute based on a seqno.
*/
/* only called within this file */
char *
fld_eval(fld, fldtype, seqno, fm_seqno)			/* abs s18 */
struct fld *fld;
int fldtype;
int seqno;
unsigned int *fm_seqno;					/* abs s18 */
{
	char  *eval_string();
	struct attribute *attr, *tmp_attr;
	char  *intr, *onintr;
	int    flags;
        char  *def, *cur;                               /* abs s18 */
        int    a_flags;                                 /* abs s18 */
        unsigned int orig_seq = *fm_seqno;	        /* abs s18 */

	
	if((fld == NULL) || (fld->attrs == NULL))
			return(NULL);
/*	if this type of descriptor can ever be interrupted, then
 	update the interrupt structures based on the values for the
	current field, if defined else with the inherited values 
	If interrupts are suppose to be enabled, set up the
	interrupt handler.
*/
	Cur_intr.skip_eval =  FALSE;
	attr = fld->attrs[fldtype];
        if (attr->testring &&
	    (strcmp(attr->testring, "action") == 0 ||
	    strcmp(attr->testring, "done")   == 0))
	{
	    tmp_attr = fld->attrs[PAR_INTR];
	    if ((intr = tmp_attr->def) == NULL)
		intr = (char *)ar_ctl(AR_cur, CTGETINTR);
	    flags = RET_BOOL;
	    Cur_intr.interrupt = FALSE;	/* dont interrupt eval of intr */
	    Cur_intr.interrupt = (bool)eval_string(intr, &flags);

	    tmp_attr = fld->attrs[PAR_ONINTR];
	    if ((onintr = tmp_attr->def) == NULL)
		onintr = (char *)ar_ctl(AR_cur, CTGETONINTR);
	    Cur_intr.oninterrupt = onintr;
	}


/*
** Decides whether to re_evaluate the attribute and then calls pline()
** to do it.
*/

	if (!(((attr->flags & EVAL_ONCE) && attr->seqno) ||
	   ((attr->flags & EVAL_SOMETIMES) && (attr->seqno >= seqno))))
	{
		if ((attr->flags & FREEIT) && attr->cur)
		{
		    if (((attr->flags & RETS) == RET_LIST) ||
			((attr->flags & RETS) == RET_ARGS))
			listfree(attr->cur);
		    else
			free(attr->cur);
		    attr->cur = NULL;
		}
		attr->seqno = seqno;  			/* abs s18 */
		def=strsave(attr->def);			/* abs s18 */
		a_flags = attr->flags; 			/* abs s18 */
		cur = eval_string(def, &a_flags); 	/* abs s18 */
		if (def)				/* abs s18 */
		    free(def);				/* abs s18 */
		if (*fm_seqno == orig_seq) 		/* abs s18 */
		{
		    attr->cur = cur; 			/* abs s18 */
		    attr->flags = a_flags;		/* abs s18 */
		}
		else  /* reread occurred & free'd fm_mn (incl. attr) abs s18 */
		    return(cur); 			/* abs s18 */

/*		attr->cur = eval_string(attr->def, &attr->flags);
**		attr->seqno = seqno;
*/
	}

	return(attr->cur);
}

/*
** Forces reevaluation of current value for an attribute.
*/
void
de_const(fm_mn, fldno, fldtype)
struct fm_mn *fm_mn;
int fldno, fldtype;
{
	fm_mn->multi[fldno].attrs[fldtype]->seqno = 0;
}
	
/*
 * SET_SINGLE_DEFAULT will generate a new attribute structure
 * and set the "def" portion of the structure to "val" ...
 * (NOTE that the string passed is "strsaved" thus can/should be
 * static)
 */
set_single_default(fm, index, val)	
struct fm_mn *fm;
int index;
char *val;
{
	struct fld *single;
	struct attribute *hold;
	struct attribute *attr;

	if ((int)fm->single.attrs == 0)	/* abs k17 */
	    return(FAIL);		/* abs k17 */
	single = &fm->single;
	attr = single->attrs[index];
	hold = new(struct attribute);
	memcpy(hold, attr, sizeof(struct attribute));
	hold->flags |= FREEIT;
	hold->cur = NULL;
	hold->seqno = 0;
	if (attr->flags & FREEIT)
		freeattr(attr);		/* free old structure */
	set_def(single->attrs[index] = hold, strsave(val));
	return(SUCCESS);	/* abs k17 */
}
	
/*
 * SET_MULTI_DEFAULT will generate a new attribute structure
 * and set the "def" portion of the structure to "val" ...
 * (NOTE that the string passed is "strsaved" thus can/should be
 * static) abs s13.
 */
set_multi_default(fm, field_num, index, val)	
struct fm_mn *fm;
int field_num;
int index;
char *val;
{
	struct attribute *hold;
	struct attribute *attr;

	if ((int)fm->multi[field_num].attrs == 0)
	    return(FAIL);
	attr = fm->multi[field_num].attrs[index];
	hold = new(struct attribute);
	memcpy(hold, attr, sizeof(struct attribute));
	hold->flags |= FREEIT;
	hold->cur = NULL;
	hold->seqno = 0;
	if (attr->flags & FREEIT)
		freeattr(attr);		/* free old structure */
	set_def(fm->multi[field_num].attrs[index] = hold, strsave(val));
	return(SUCCESS);	
}

void
set_def(attr, str)
struct attribute *attr;
char *str;
{
	attr->def = str;
}

/*
** Set current value of an attribute (only used in "value" field of
** form).
*/
void
set_cur(fm_mn, fldno, fldtype, str)
register struct fm_mn *fm_mn;
register int fldno, fldtype;
char *str;
{
	struct attribute *attr;

	attr = fm_mn->multi[fldno].attrs[fldtype];
	if ((attr->flags & FREEIT) && attr->cur) {
		if (((attr->flags & RETS) == RET_LIST) ||
		    ((attr->flags & RETS) == RET_ARGS))
			listfree(attr->cur);
		else
			free(attr->cur);
	}
	attr->cur = str;
	attr->seqno = 1;
	attr->flags |= EVAL_ONCE;
	un_validate(fm_mn, fldno); /* abs s13 */
}

/*
** Set current value of an attribute (only used in "text" objects) 
*/
void
set_sing_cur(fm_mn, desctype, str)
register struct fm_mn *fm_mn;
register int desctype;
char *str;
{
	struct attribute *attr;

	attr = fm_mn->single.attrs[desctype];
	if ((attr->flags & FREEIT) && attr->cur) {
		if (((attr->flags & RETS) == RET_LIST) ||
		    ((attr->flags & RETS) == RET_ARGS))
			listfree(attr->cur);
		else
			free(attr->cur);
	}
	attr->cur = str;
	attr->seqno = 1;
	attr->flags |= EVAL_ONCE;
}

/*
** Get default value of an multi-eval attribute
*/
char *
get_def(fm_mn, fldno, fldtype) 
register struct fm_mn *fm_mn;
register int fldno, fldtype;
{
	return(fm_mn->multi[fldno].attrs[fldtype]->def);
}



/*
** Get define value of an sing-eval attribute
*/
char *
get_sing_def(fm_mn, fldtype) 
register struct fm_mn *fm_mn;
register int fldtype;
{
	return(fm_mn->single.attrs[fldtype]->def);
}




/*
** Free a list of strings generated by parselist.
*/
listfree(list)
char **list;
{
	int i;
	int	lcv;

	if (!list)
		return;
	lcv = array_len(list);
	for (i = 0; i < lcv; i++)
		free(list[i]);
	array_destroy(list);
}
