#ident	"@(#)optim:i386/regalutil.c	1.11"
#include "sched.h" /*include optim.h and defs.h */
#include "optutil.h"
#include "regalutil.h"

#define HASH(X)  (((-X) >> 2) & 0x7f) /* hash function is (-x/4)%127 */
#define HASH_SIZE 0x80
AUTO_REG *regals[HASH_SIZE];
static unsigned reg_bits = 1;
#define INVALID     -1
extern int nregs;

/* s is the operand that contains the regal - string of stack */
int hash_regal(s) char *s;
{
    int X=0;

    if ((BY_OFF(s,EBP) || BY_OFF(s,ESP)) && !strstr(s,"RSZ")) {
        X =  (*s == '*') ? (int)strtoul(&s[1],(char **)NULL,0)
			 : (int)strtoul(s,(char **)NULL,0);
        X = HASH(X);
    }
    else {                                         /* regal is a label */
        for ( ; *s != (char) 0 ; s++ )
            X += *s;
        X %= 0x7f ;
    }
    return X;
}


/* This group of functions  are for spilling elimination. A temp is a variable
that for every basic block is set before used. If there is a free register
the temp can be reallocated to this register. In this case rmrdmv() that should
come after this optimization will sometime alienate the temp reg completely.
If is a specific basic block temp is referenced only once (set only) it will
be removed.
*/
/* First few function menage the operands declared REGAL by the compiler. */
static unsigned used_first;
extern int asmflag;
unsigned use_first()
{
 BLOCK *b = b0.next;
 NODE *firsti = n0.forw , *lasti = n0.forw,*p;
 unsigned use,set;
    if (asmflag)
        return used_first = ((unsigned) ~0);
    used_first = 0;
    while (lasti != &ntail) {
        firsti = b->firstn;
        do
            b = b->next;
        while (b && ! islabel(b->firstn));
        lasti = b ? b->firstn : &ntail;
        for (set = use = 0, p = firsti; p != lasti; p = p->forw) {
            if (p->nrlive & ~set)
                use |= p->nrlive;
            else if (p->nrdead & ~use)
                set |= p->nrdead;
        }
        used_first |= use;
    }/*while loop*/
    return used_first;
}/*end use_first */

void
init_reg_bits()
{
	reg_bits = 1; /* function called from optim.c, variable is static. */
}


void
init_regals()
{
    int i;
    reg_bits = 1;
    for (i=0; i < HASH_SIZE; i++)
        regals[i] = NULL;
}/*end init_regals*/


/*This is a costing function for double size parameters: do we want to
**move them to the autos area. The move costs two machine cycles on the
**P5 and four on the i486. If a param is referenced in a loop then move
**it. Else it has to be references enough times to make it worth. A mis
**aligned access is three cycles, and a we can not know if an access to
**a parameter is aligned or not, hence we analize as 1.5. Ergo on i486
**there should be more then 4 accesses and on the P5 more then two.
*/
#define MAXWEIGHT   (MAXINT - 1000)
#define WEIGHT      8
#define MAXLDEPTH   10
void
estimate_double_params()
{
int i,m,x;
struct auto_reg *r;
NODE *p;
int weight = 1;
int depth = 0;
    for (ALLN(p)) {
        if (p->op == LCMT)
            if (*p->opcode == 'H') {
                ++depth;
                if(depth <= MAXLDEPTH)
                    weight *= WEIGHT;
            } else if (*p->opcode == 'E') {
                if(depth <= MAXLDEPTH)
                    weight /= WEIGHT;
                --depth;
            }
        if (OpLength(p) == DoBL &&
            ((m = 1, p->op1 && scanreg(p->op1,false) & frame_reg) ||
            (m = 2, p->op2 && scanreg(p->op2,false) & frame_reg))) {
            x = (int)strtoul(p->ops[m],(char **)NULL,0);
            if (x > 0) {
                for (i=0; i < HASH_SIZE; i++)
                    for (r= regals[i]; r != NULL ; r = r->reg_hash_next)
                        if (r->size == DoBL && r->offset == x && r->valid)
                            r->estim += weight;

            }
        }
    }
}/*end estimate_double_params*/

void
remove_overlapping_regals()
{
AUTO_REG *r1,*r2;
int x1,x2;
    for (x1=0; x1 < HASH_SIZE; x1++)
        for (r1= regals[x1]; r1 != NULL ; r1 = r1->reg_hash_next) {
            if (r1->valid == false || r1->partof) continue;
            for (x2=0; x2 < HASH_SIZE; x2++) {
                for (r2= regals[x2]; r2 != NULL ; r2 = r2->reg_hash_next) {
                    if (r2->valid == false || r2->partof) continue;
                    if (r1->offset == r2->offset && r1->size == r2->size)
                        continue;
                    if ((r1->offset <= r2->offset) &&
                        (r2->offset < (r1->offset + r1->size))) {
                        r1->valid = false;
                        r2->valid = false;
                    }
                }
            }
        }
}/*remove_overlapping_regals*/
int
double_params()
{
int n=0;
int i;
AUTO_REG *r;
const int min = (target_cpu & (P5 | blend)) ? 2 : 4;
    for (i=0; i < HASH_SIZE; i++)
        for (r= regals[i]; r != NULL ; r = r->reg_hash_next)
            if ((r->offset > 0) && (r->size == DoBL)
                && r->valid && r->estim > min)
                n++;
    return n;
}/*end double_params*/

int
is_double_param_regal(s) char *s;
{
int i;
AUTO_REG *r;
const int min = (target_cpu & (P5 | blend)) ? 2 : 4;
int x;  /* offset */

    if (*s == '*')
        s++;
    x = (int)strtoul(s,(char **)NULL,0);
    if (x < 0) return 0;
    for (i=0; i < HASH_SIZE; i++)
        for (r= regals[i]; r != NULL ; r = r->reg_hash_next)
            if ((r->offset == x) && (r->size == DoBL) && (!strcmp(r->reglname,s))
                && r->valid && r->estim > min)
                return r->param2auto;
    return 0;
}/*end is_double_param_regal*/

char *
next_double_param()
{
static int i=0;
static AUTO_REG *r,*r1=NULL;
int first = 1;
const int min = (target_cpu & (P5 | blend)) ? 2 : 4;
    for ( ; i < HASH_SIZE; i++) {
        if (first) {
            r = r1 ? r1 : regals[0];
            first = 0;
        } else {
            r = regals[i];
        }
        for ( ; r != NULL ; r = r->reg_hash_next)
            if ((r->offset > 0) && (r->size == DoBL)
                && r->valid && r->estim > min) {
                r1 = r->reg_hash_next;
                return (r->reglname);
            }
    }
    return (char *) 0;
}/*end next_double_param*/

void
set_param2auto(param,autom) char *param; int autom;
{
int entry = hash_regal(param);
AUTO_REG *r;
    for (r = regals[entry]; r; r = r->reg_hash_next) {
		if (*param == '*')
			param += 1;
		if (!strcmp(r->reglname,param)) {
            r->param2auto = autom;
            return;
        }
    }
    fatal(__FILE__,__LINE__,"didnt find param to set auto\n");
}/*set_param2auto*/


/* Get regal and initialize it and add to the hash table.
** Called from parse_regal to save FP regals which are otherwise ignored,
** and from ratable, to save regals which were saved for raoptim().
** Called also from parse_alias to invalidate FP REGALs marked as ALIASes.
** Invalidate by set the parameter valid to false.
** No assumption on the order between the call from parse_alias and the call
** from parse_regal.
** If an FP regal with size 8 bytes is installed, it's higher half is also
** installed and marked as partof.
** partof is to utilize removal of integer operation working on halfs of the
** regal. Therefore getregal() which is activated from rm_tmpvars(), recognizes
** regals which are partof bigger regals, but isregal doesn't. isregal() is
** used from everywhere else, e.g. schedule(), loop_regal();
*/
void
save_regal(s,size,valid,partof) char *s; int size;
{
    AUTO_REG *r;
    int entry;
    char xtos[FRSIZE]; /* suitable for fixed and non fixed modes */
    int x;

    entry = hash_regal(s);
    for (r= regals[entry]; r != NULL ; r = r->reg_hash_next) {
		if (*s == '*')
            s += 1;
		if (!strcmp(r->reglname,s)) /* If regal was found */ {
            if (!valid) r->valid = false;
            if (r->size != size) r->valid = false;
            return;
        }
    }
    r = GETSTR(AUTO_REG);
	strcpy(r->reglname,s);
    r->offset = BY_OFF(s,frame_reg) ? (x = (int)strtoul(((*s == '*') ? &s[1] : s),(char **)NULL,0))
				    : -1 ;
    r->size = size;
    r->valid = valid;
    r->partof = partof;
    r->param2auto = 0;
    r->reg_hash_next = regals[entry]; /* link to the hash table */
    regals[entry] = r;
    r->bits = 0;
    r->estim = 0;
    if (size == DoBL) {
		/* func_data is not always known at this point so we just copy the string */
		if (fixed_stack) 
			sprintf(xtos,"%d%s",x+4,strstr(s,"+")); 
		else
        	sprintf(xtos,"%d(%%ebp)",x+4);
        save_regal(xtos,LoNG,valid,true);
    }
}

/* look for regal in hash table.
** Return true only if found the regal, it is valid and not partof a
** bigger REGAL.
*/
int
isregal(s) char *s;
{
  int entry;
  AUTO_REG *r;

  if (BY_OFF(s,ESP) || BY_OFF(s,EBP))
    if (strtoul(s,(char **)NULL,0) == 0)
        return NULL;
  entry = hash_regal(s);
  if (*s == '*') s += 1;
  for (r= regals[entry]; r != NULL ; r = r->reg_hash_next) {
    if (!strcmp(r->reglname,s)) /* If regal was found */
        if (r->valid && !r->partof)
            return true;
        else
            return false;
  }
  return false;
}


/* Remove any named ALIASed regal from the regal hash table.  Called
** from remove_aliases() in regal.c.
*/
void
remove_aliased_regals(char * s)
{
  int entry;
  AUTO_REG *r;
  char xtos[FRSIZE]; /* suitable for fixed and non fixed modes */

	entry = hash_regal(s);
	for (r= regals[entry]; r != NULL; r = r->reg_hash_next) {
		if (!strcmp(r->reglname,s)) {
			/* regal was found */
			r->reglname[0] = '\0';
			r->valid = false;
			if (r->size == DoBL) {
				/* this regal has a second part */
				if (fixed_stack) {
					sprintf(xtos,"%d%s",r->offset+4,strstr(s,"+"));
				} else {
					sprintf(xtos,"%d(%%ebp)",r->offset+4);
				}
				remove_aliased_regals(xtos);
			}
		}
	}
}  /* remove_unaliased_regals */

/* look for regal in hash table, one op has the form "[*]num(%ebp)".
** or a regular string (for globals).
** If found return the reg structure.
** But only if the REGAL found is valid, and was not invalidated by
** set_regal_bits(). This is done by setting INVALID to reg->bits, which
** invalidates for use here since there are not enough bits to do live-dead
** analisys. It stays regal for isregal.
*/

AUTO_REG *
getregal(p) NODE *p;
{
    int entry;
    AUTO_REG *r;
    char *s = NULL;

    if (p->op1 && isindex(p->op1,fixed_stack?ESP_STR:EBP_STR)
        && (!fixed_stack || strstr(p->op1,".FSZ")))
            s = p->op1;
    else if (p->op2 && isindex(p->op2,fixed_stack?ESP_STR:EBP_STR)
        && (!fixed_stack || strstr(p->op2,".FSZ")))
            s = p->op2;
    else return NULL;
    entry = hash_regal(s);
    for (r= regals[entry]; r != NULL ; r = r->reg_hash_next) {
		if (*s == '*')
            s += 1;
        if ((!strcmp(r->reglname,s)) && r->valid && (r->bits != INVALID))
            return r; /* regal found return it */
    }
    return NULL;
}


static AUTO_REG *
get_high_partof_regal(p) NODE *p;
{
    int entry;
    AUTO_REG *r;
    int x;
    char *s = NULL, *rgl;

    if (p->op1 && isindex(p->op1,fixed_stack?ESP_STR:EBP_STR)
		&& (!fixed_stack || strstr(p->op1,".FSZ")))
           s = p->op1;
    else if (p->op2 && isindex(p->op2,fixed_stack?ESP_STR:EBP_STR)
		&& (!fixed_stack || strstr(p->op2,".FSZ")))
           s = p->op2;
    else return NULL;
    x =  (*s == '*') ? (int)strtoul(&s[1],(char **)NULL,0)
		     : (int)strtoul(s,(char **)NULL,0);
    x += 4;
    rgl = getspace(NEWSIZE);
	if (fixed_stack)
		sprintf(rgl,"%d%s",x,strstr(s,"+"));
	else
    	sprintf(rgl,"%d(%%ebp)",x);
    entry = hash_regal(rgl);
    for (r= regals[entry]; r != NULL ; r = r->reg_hash_next)
        if (!strcmp(r->reglname,rgl) && r->valid && (r->bits != INVALID))
            return r; /* regal found return it */
    return NULL;
}/*end get_high_partof_regal*/

unsigned
get_regal_bits(p) NODE *p;
{
AUTO_REG *r,*r2;
unsigned result;
    r = getregal(p);
    if (!r) return 0;
    result = r->bits;
    if (r->size == DoBL) {
        r2 = get_high_partof_regal(p);
        if (r2) result |= r2->bits;
    }
    return result;
}/*end get_regal_bits*/

unsigned int
bits_of_high_part(p) NODE *p;
{
AUTO_REG *reg;
    reg = get_high_partof_regal(p);
    if (!reg)  return 0;
    if (reg->bits == 0) {
        if (reg_bits) {
            reg->bits = reg_bits;
            reg_bits <<= 1;
        } else {
            reg->bits = (unsigned)INVALID;
            return 0;
        }
    }
    return reg->bits;
}/*end bits_of_high_part*/

/* Set p->nrlive and p->nrdead to the bit corresponding to the REGAL operand
** in p. If no REGAL operand, set both to zero. If already processed 32 REGAL
** in the current function - no bits for more REGALS, invalidate all the coming
** ones.
** Invalidate a REGAL if it's address is taken. Workaround to a forsaken bug.
*/
void
set_regal_bits(p) NODE *p;
{
    AUTO_REG *reg = NULL;
    unsigned bits =0;

    if (p->op == ASMS) {
        p->nrlive = (unsigned) ~0;
        p->nrdead = 0;
        return;
    }
    if (!(reg = getregal(p))) {
        p->nrlive = 0;
        p->nrdead = 0;
        return;
    }
    if ( p->op == LEAL || p->op == LEAW || !reg_bits)
    {   reg->bits =(unsigned) INVALID; /* Mark this register as not valid */
#ifdef DEBUG
        if (fflag)
            fprintf(stderr,"kill regal offset = %d\n", reg->offset);
#endif
        p->nrlive = 0;
        p->nrdead = 0;
        if (reg->size == DoBL) {
            if ((reg = get_high_partof_regal(p)) != NULL)
                reg->bits = (unsigned)INVALID;
        }
        return;
    }
    if (reg->bits)
        bits= reg->bits;
    else if (reg_bits)
    {   bits = reg->bits = reg_bits;
#ifdef DEBUG
        if (fflag)
            fprintf(stderr,"regal offset= %d, bit is %8.8x\n",reg->offset,bits);
#endif
        reg_bits <<= 1;
    }
    if (OpLength(p) == DoBL) {
        bits |= bits_of_high_part(p);
	}

    if ( MEM & muses(p)) {
        p->nrlive = bits;
        p->nrdead = 0;
    }
    else if (MEM & msets(p)) {
        if (OpLength(p) != reg->size) {
            p->nrlive |= bits;
            p->nrdead = 0;
        } else {
            p->nrlive = 0;
            p->nrdead = bits;
        }
    }
    return;
}

boolean
live_at(live,at) NODE *live,*at;
{
AUTO_REG *r = getregal(live);
    if (!r) return true; /* it is not a REGAL, assume it is live. */
    return (r->bits & at->nrlive);
}/*end live_at*/

boolean
needed_at(p,b) NODE *p; BLOCK *b;
{
NODE *q = first_non_label(b);
AUTO_REG *r = getregal(p);
char *op;
	if (!r) return true; /* it is not a REGAL, assume it is live. */
	if (r->bits & q->nrlive) return true;
	if (ismem(p->op1)) op = p->op1; 
	else op = p->op2;
	if ((q->op1 && !strcmp(q->op1,op)) || (q->op2 && !strcmp(q->op2,op)))
		return true;
	return false;
}


/* find reg that is free from p to last_p instruction block  */
int
find_free_reg(p,b,reg_bit) NODE *p;BLOCK *b; unsigned reg_bit;
{

    int first_reg = 0,last_reg = nregs;
    int i;
    AUTO_REG *r;
    unsigned int bits = 0;
    unsigned live = EAX | EDX | ECX | EBX | ESI | EDI | EBI;
    static int reg_regs[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBI};
    static int fix_regs[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBP};
    static int *regs = reg_regs;
    NODE *jtarget = NULL;

    if (fixed_stack) {
        regs = fix_regs;
        live = EAX | EDX | ECX | EBX | ESI | EDI | EBP;
    }
    if (suppress_enter_leave) nregs=6 , last_reg=6;
    else nregs = 7;
    if (! (p->nrdead & reg_bit)) /* /REGAL  was live before */
        return 0;
    if (isfp(p)) return 0;
    if (OpLength(p) == ByTE)
        last_reg = 4; /* no ESI or EDI */
    if (isuncbr(p) || islabel(p->forw)) /* last instruction should have deleted if not used */
        return 0;
    for(p = p->forw ; p != &ntail ; p = p->forw) {
        if (p->op == ASMS)
            return 0;
        live &= ~(p->nlive | p->uses | p->sets);
        if (p->op == CALL || p->op == LCALL)
            first_reg = 3;
        if ((reg_bit & used_first) && isbr(p)) {
            if (isuncbr(p) || (b->nextr == NULL && (b->nextl == NULL ||
                ( !isuncbr(b->lastn))))) /* a return, or an unconditional indexed
                                        jump, or a switch. */
                return 0;
            if (b->nextr) {
                jtarget = b->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
            }
            if ((r = getregal(jtarget)) != NULL) {
                bits = r->bits;
                bits |= bits_of_high_part(jtarget);
                if (((bits | jtarget->nrlive) & reg_bit) & ~jtarget->nrdead)
                    return 0; /* used (and not killed) or live in jump target */
            }
            else if (reg_bit & jtarget->nrlive)
                return 0; /* live in jump target */
        }
        r = getregal(p);
        if (r) {
            bits = r->bits;
            bits |= bits_of_high_part(p);
        }
        if (r && (bits & reg_bit)) {
            if (isfp(p))
                return 0;
            if (OpLength(p) == ByTE) /* If byte register is needed */
                last_reg = 4; /* no ESI or EDI */
        }
        if (!(reg_bit & p->nrlive))
            break;
        if (p == b->lastn) {
            if (isuncbr(p) || islabel(p->forw)) {
                return 0;
            }
            b = b->next;
        }
    }
    for (i = first_reg ; i < last_reg; i++)
        if ((regs[i] & live) == regs[i]){
#ifdef DEBUG
            if (fflag) {
                fprintf(stderr,"last checked: ");
                fprinst(p);
            }
#endif
            return regs[i];
        }
    return 0;
}


void
minimize_fsz()
{
	NODE *p;
	char *s;
	int number,max = 0,max_param = 0;
	int tmp_fsz;
	int fp_inst=0;
	int remainder = 0;

	COND_RETURN("minimize_fsz");

	/*in case two entries, incomming args in the first entry look like
	**outgoing args and it makes the frame look bigger than what is necessary.
	**simply do not go over this part, there can be nothing interesting there.
	*/
	p = n0.forw;
	if (func_data.two_entries) {
		do
			p = p->forw;
		while (!islabel(p));
	} /*p now points to the second entry*/
	for ( ; p && p != &ntail ; p = p->forw) {
		if (isfp(p))
			fp_inst = 1;
		else
			fp_inst = 0;
		if ((((s = p->op1) != NULL) && *p->op1 == '-' && strstr(p->op1,".FSZ")) ||
			(((s = p->op2) != NULL) && *p->op2 == '-' && strstr(p->op2,".FSZ"))) {
			number = (int)strtoul(s + 1,(char **)NULL,0);
			remainder = number & 0x03;
			if (remainder != 0) number += (4 - remainder);
			if (fp_inst)
				number += 4; /* if it is a fp inst, then the higher part must have room too */
			if (strstr(s,"RSZ"))
				number +=func_data.regs_spc;
			if (number > max) 
				max = number;
			continue;
		}
		/* this is an outgoing param */
		if ((((s=p->op2) != NULL) && isdigit(*s) && usesreg(s,"%esp") && !strstr(s,".FSZ")) ||
			(((s=p->op1) != NULL) && isdigit(*s) && usesreg(s,"%esp") && !strstr(s,".FSZ"))) { 
			number = (int)strtoul(s,(char **)NULL,0);
			remainder = number & 0x03;
			if (remainder != 0) number += (4 - remainder);
#if WAS
			This is wrong. The same way that it need to be 4 for 0(%esp),
			it also needs to be 8 if the biggest is 4(%esp) and so forth.
			if (number == 0) /* although it is 0(%esp) there must be room */
				number = 4;
#endif
			number += 4;
			if (fp_inst)
				number += 4;
			if (number > max_param)
				max_param = number;
			if (p->op == LEAL) {
				if (p->save_restore == -1) max_param = func_data.frame;
				else if (p->save_restore > 0) {
					if (max_param < p->save_restore)
						max_param = p->save_restore;
				}
			}
			continue;
		}
		if (max_param == 0 &&
			((p->op1 && !strcmp(p->op1,"(%esp)"))
			|| ((p->op2 && !strcmp(p->op2,"(%esp)"))))) 
			max_param = OpLength(p);

	}
	tmp_fsz = max_param + max;
	if (func_data.frame > tmp_fsz)
		func_data.frame = tmp_fsz ;
}
