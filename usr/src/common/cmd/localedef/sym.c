#ident	"@(#)localedef:sym.c	1.1"
#include <sys/types.h>
#include <widec.h>
#include <string.h>
#include "_colldata.h"
#include "_localedef.h"

typedef enum {
	HASH_ADD,
	HASH_FIND
} Hashvals;

#define SYMHTABSZ	101

struct codent strt_codent = {
	WEOF,
	&strt_codent,
	NULL,
	BLACK
};

/* init to zeros */
struct syment *symhashtab[SYMHTABSZ];
wint_t maxcodept = -1;
static int numcodes = 0;

struct codent *
walk(struct codent *c, struct codent *tail)
{
	struct codent *head; 

	if(c->cd_left == &strt_codent && c->cd_right == &strt_codent) {
		c->cd_codelink = tail;
		return(c);
	}
	if(c->cd_left != &strt_codent)
		head = walk(c->cd_left,c);
	else
		head = c;

	if(c->cd_right != &strt_codent) 
		c->cd_codelink = walk(c->cd_right,tail);
	else
		c->cd_codelink = tail;

	return(head);
}

void
linkcodes()
{
	strt_codent.cd_codelink = walk(strt_codent.cd_left,NULL);
}
	
struct codent *
findcode(CODE code)
{
	struct codent *p = strt_codent.cd_left;

	while (p != &strt_codent)
	{

		if (p->cd_code == code)
			return p;
		else if (p->cd_code > code)
			p = p->cd_left;
		else
			p = p->cd_right;
	}
	return(NULL); 
}

static void
rotate(struct codent *g, struct codent *p, struct codent *c)	/* rotate c above p */
{
	/*
	* 4 cases: p is g's left or right child
	*      and c is p's left or right child
	*/
	if (p == g->cd_left)
		g->cd_left = c;
	else
		g->cd_right = c;
	if (c == p->cd_left)
	{
		p->cd_left = c->cd_right;
		c->cd_right = p;
	}
	else
	{
		p->cd_right = c->cd_left;
		c->cd_left = p;
	}
}

static struct codent *
split(struct codent *gg, struct codent *g, struct codent *p, struct codent *c)
{
	if (c->cd_flags & BLACK)	/* nonleaf node */
	{
		c->cd_left->cd_flags |= BLACK;
		c->cd_right->cd_flags |= BLACK;
		if (g == NULL)
			return c;	/* c is the root */
		c->cd_flags &= ~BLACK;
	}
	if (!(p->cd_flags & BLACK))	/* need to fix tree */
	{
		if ((c == p->cd_left) == (p == g->cd_left))
			c = p;
		else	/* double rotation needed */
			rotate(g, p, c);
		rotate(gg, g, c);
		g->cd_flags &= ~BLACK;
		c->cd_flags |= BLACK;
	}
	return c;
}

struct codent *
addcode(CODE code)
{
	struct codent *gg, *g = 0;	/* greatgrandparent and grandparent */
	struct codent *p = &strt_codent, *c = p->cd_left;	/* parent and child */
	int dir = -1;		/* start on the left */

	while (c != &strt_codent)
	{
		if (!(c->cd_left->cd_flags & BLACK )&& !(c->cd_right->cd_flags & BLACK))
		{
			c = split(gg, g, p, c);
			g = p = c;
		}
		gg = g;
		g = p;
		p = c;
		if (code == c->cd_code) 
			return c;	/* equivalent */
		else if (code < c->cd_code) {
			c = c->cd_left;
			dir = -1;
		}
		else {
			c = c->cd_right;
			dir = 1;
		}
	}
	/*
	* Add new node c below p.
	*/
	c = (struct codent *) getmem(NULL,sizeof(struct codent),B_TRUE,B_TRUE);
	c->cd_code = code;
	c->cd_dispwidth = default_dispwidth;
	c->cd_left = &strt_codent;
	c->cd_right = &strt_codent;
	if(code >maxcodept)
		maxcodept = code;
		if(code > UCHAR_MAX)
			numcodes++;
	if (dir < 0)
		p->cd_left = c;
	else
		p->cd_right = c;
	if (g == NULL)	/* c is the root */
		c->cd_flags |= BLACK;
	else
	{
		c->cd_flags &= ~BLACK;
		(void)split(gg, g, p, c);
	}
	return c;
}


#define HINIB 0xf0000000

static unsigned long 
hash(unsigned char *p)
{
	int n;
	unsigned long total;

	total = *p;

	while((n = *++p) != '\0') {
		total << 4;
		if((total += n) & HINIB) {
			n = (total & HINIB) >> 27;
			total &= ~HINIB;
			total ^= n;
		}
	}
	return(total);
}


static struct syment *
hashname(unsigned char *name, Hashvals key)
{

	unsigned long hval;
	struct syment *tsym, **ttsym;
	int c;

	hval = hash(name);
	hval %= SYMHTABSZ;

	ttsym = &(symhashtab[hval]);

	while((tsym = *ttsym) != NULL) {
		if((c = strcmp((char *)name,(char *)tsym->sy_name)) == 0) {
			if(key == HASH_ADD)
				return(NULL);
			else
				return(tsym);
		}
		if(c > 0)
			break;
		ttsym = &(tsym->sy_next);
	}
	if(key == HASH_FIND)
		return(NULL);
	tsym = (struct syment *) getmem(NULL,sizeof(struct syment)+strlen((char *)name)+1,TRUE,TRUE);
	tsym->sy_name = (unsigned char *)strcpy((char *)tsym + sizeof(struct syment),(char *)name);
	tsym->sy_next = *ttsym;
	*ttsym = tsym;
	return(tsym);
}

struct syment *
addsym(unsigned char *name)
{
	struct syment *tsym;

	if((tsym = hashname(name, HASH_ADD)) == NULL) {
		diag(ERROR,TRUE,":43:Duplicate definition of %s\n",name);
		return(NULL);
	}
	return(tsym);
}

struct syment *
addpair(unsigned char *name, CODE code, mboolean_t softd)
{
	struct syment *tsym;

	if(softd) {
		if((tsym = hashname(name, HASH_FIND)) != NULL) {
			if(tsym->sy_codent->cd_code != code) 
				diag(WARN,TRUE, ":0:User supplied definition (0x%x) overrides default definition (0x%x) for symbol \"%s\"\n",tsym->sy_codent->cd_code,code,name);
			return(tsym);
		}
	}
	if((tsym = hashname(name, HASH_ADD)) == NULL) {
		diag(ERROR,TRUE,":0:Duplicate definition of %s\n",name);
		return(NULL);
	}
	tsym->sy_codent = addcode(code);
	return(tsym);
}

struct syment *
findsym(unsigned char *name)
{
	return(hashname(name, HASH_FIND));
}

double
density()
{

	if(numcodes == 0)
		return(0.0);
	return((double) numcodes / (double)(maxcodept - UCHAR_MAX));
}
			

#ifdef DEBUG
printhash()
{
	struct syment *tsym;
	int i;
	for(i = 0; i < SYMHTABSZ; i++) {
		if(symhashtab != NULL) {
			tsym = symhashtab[i];
			while(tsym != NULL) {
				printf("i=%d, name=%s, codent=%x\n",i, tsym->sy_name, tsym->sy_codent);
				tsym = tsym->sy_next;
			}
		}
	}
}

printcodes()
{
	struct codent *tcode;
	tcode = strt_codent.cd_codelink;
	while(tcode != NULL) {
		printf("code=0x%x multbeg=0x%x subnbeg=0x%x flags=0x%x weight[0]=0x%x conv=0x%x classes=0x%x\n",tcode->cd_code,tcode->cd_multbeg,tcode->cd_subnbeg,tcode->cd_flags,tcode->cd_weights[0],tcode->cd_conv,tcode->cd_classes);
		if(tcode->cd_orderlink != NULL)
			printf("next=0x%x\n",tcode->cd_orderlink->cd_code);
		tcode = tcode->cd_codelink;
	}
}
#endif
