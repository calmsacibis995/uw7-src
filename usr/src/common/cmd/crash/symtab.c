#ident	"@(#)crash:common/cmd/crash/symtab.c	1.4.2.3"

/*
 * This file contains code for the crash function nm
 * (which is now identical with ds, ts, symval and sym),
 * as well as all the internal symbol initialization and primitives.
 * But getnm() command processing is now in misc.c,
 * leaving symtab.c as a low-level support module.
 */

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ksym.h>
#include <sys/mod_k.h>
#include <sys/vmparam.h>
#include <sys/fcntl.h>
#include <libelf.h>

#define  _SYMENT
#include "crash.h"

/*
 * These really belong to strcon.c, but db_sym_off() wants to look at them,
 * and symtab.o is more likely to be wanted in a crash nucleus library
 */
vaddr_t savevalue[27];

#define ALIGN(a, b)	((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))
#define PAGEROUNDUP(a)	(((a) + (PAGESIZE-1)) & ~(PAGESIZE-1))

#ifdef KVBASE_IS_VARIABLE
#define PHYSEND		0x0
#else
#define PHYSEND		0x00400000
#endif

#define LC		0x20	/* for lower-casing */

/* bitflags for syment.n_dup */
#define GLOBAL		0
#define LOCAL		0x01
#define NODUP		0x02
#define MAYDUP		0x04
#define UNDUPED		0x08
#define OUROWN		0x10

/* values for _symsrch() internal */
#define EXTERNAL	0
#define SYMINIT		1
#define DONTKSYM	2

extern char mallerr[];

typedef struct syment {			/* shorter than that in syms.h */
	ushort	n_strlen;		/* length of name: over n_zeroes */
	char	n_tdn;			/* "nm -xvp" type: over n_zeroes */
	char	n_dup;			/* uniqueness flag: over n_zeroes */
	char 	*n_strptr;		/* pointer to name: over n_offset */
	vaddr_t	n_value;		/* value of symbol */
} SYMENT;				/* other syms.h fields unused */

typedef struct modent {
	struct modent	*next;
	uint_t		mc_id;
	struct module	*mc_modp;
	char		*mc_name;
	struct modctl	*modctlp;
	vaddr_t		bot;
	vaddr_t		top;
	struct syment	*syments;
	struct syment	*symendp;
	char		*strings;
} MODENT;

static MODENT modents[5];	/* for /unix: more malloced for modules */
static SYMENT *symendp;
static int nodups;

static struct symlist {
	struct syment   sl_ent;
	struct symlist *sl_next;
} *slhead = NULL;

static vaddr_t
getkaddr(char *name, char *tdnp)
{
	vaddr_t addr;
	unsigned long type;
	char *cp;

	if (*name == ';') {
		cp = ++name;
		while (isxdigit(*cp))
			++cp;
		if (*cp != '\0')
			return 0;
	}
	addr = 0;
	if (getksym(name, &addr, &type) != 0)
		return 0;
	if (addr < PHYSEND)
		addr += kvbase;
	else if (addr < kvbase+PHYSEND || addr >= KVPER_ENG_END)
		return 0;
	switch (type) {
	case STT_FUNC:		*tdnp = 'T'; break;
	case STT_OBJECT:	*tdnp = 'D'; break;
	default:		*tdnp = 'N'; break;
	}
	return addr;
}

static vaddr_t
getkname(char *name, vaddr_t addr)
{
	unsigned long offset;
	vaddr_t tmpaddr;
	char *cp;

	if (addr < kvbase || addr >= KVPER_ENG_END)
		return 0;
	if ((tmpaddr = addr) < kvbase+PHYSEND)
		tmpaddr -= kvbase;
	if (getksym(name, &tmpaddr, &offset) != 0)
		return 0;
	if ((addr -= offset) < kvbase || addr >= KVPER_ENG_END)
		return 0;
	cp = name;
	while (isxdigit(*cp))
		++cp;
	if (*cp == '\0') {
		memmove(name + 1, name, cp - name + 1);
		*name = ';';
	}
	return addr;
}

/* check our idea of the symbol against the active kernel's idea */
static boolean_t
checksym(SYMENT *sp, MODENT *mp)
{
	register SYMENT *gsp, *esp;
	char name[MAXSYMNMLEN+1];
	char *cp, tdn;
	boolean_t good;

	gsp = sp;
	esp = mp->syments;
	while (gsp->n_dup & (LOCAL|OUROWN)) {
		if (--gsp < esp) {
			if (esp == sp)	/* no globals to check against */
				return B_TRUE;
			gsp = mp->symendp - 1;
			esp = sp;
		}
	}

	cp = gsp->n_strptr;
	if (gsp->n_dup & UNDUPED)
		cp = strchr(cp, ';') + 1;

	if (mp->mc_id == 0) {
		/*
		 * Lookup by name should be faster since the kernel hashes,
		 * and it searches the undynamic kernel symbols first.
		 */
		good = getkaddr(cp, &tdn) == gsp->n_value;
	}
	else {
		/*
		 * Lookup by name cannot be relied upon, because the kernel
		 * may find that name amongst the undynamic kernel symbols.
		 */
		good = getkname(name, gsp->n_value) == gsp->n_value
			&& strcmp(name, cp) == 0;
	}
	return good;
}

/* provide a SYMENT slot when getksym() is being used */
static struct syment *
findsp(register char *name, vaddr_t addr, char tdn)
{
	register struct symlist *tsl;
	register ushort len;

	len = (ushort)strlen(name);
	tsl = slhead;
	while(tsl) {
		if (len == tsl->sl_ent.n_strlen
		&&  *(short *)name == *(short *)tsl->sl_ent.n_strptr
		&&  memcmp(name, tsl->sl_ent.n_strptr, len) == 0) {
			if (tsl->sl_ent.n_value == addr)
				return &tsl->sl_ent;
			goto new;
		}
		tsl = tsl->sl_next;
	}
	tsl = (struct symlist *) malloc(sizeof(struct symlist) + len + 1);
	if (tsl == NULL)
		error(mallerr, sizeof(struct symlist)+len+1, "symlist entry");
	tsl->sl_ent.n_strlen = len;
	tsl->sl_ent.n_strptr = (char *)tsl + sizeof(struct symlist);
	memcpy(tsl->sl_ent.n_strptr, name, len + 1);
	tsl->sl_ent.n_dup = GLOBAL;
	tsl->sl_next = slhead;
	slhead = tsl;
new:
	if (tdn) {
		tsl->sl_ent.n_tdn = tdn;
		tsl->sl_ent.n_value = addr;
	}
	else {
		tsl->sl_ent.n_value = getkaddr(name, &tsl->sl_ent.n_tdn);
		if (tsl->sl_ent.n_value != addr)
			return NULL;
	}
	return &tsl->sl_ent;
}

/* replace an ambiguous name by a unique name */
static void
undupsym(SYMENT *sp, char *prefix)
{
	char hex[9];
	size_t prelen, postlen;
	char *cp, *postfix;

	if (prefix) {
		if ((cp = strchr(prefix, ';')) == NULL)
			prelen = strlen(prefix);
		else
			prelen = cp - prefix;
	}
	else {
		sprintf(prefix = hex, "%08x", sp->n_value);
		prelen = 8;
	}

	postlen = sp->n_strlen;
	postfix = sp->n_strptr;
	if ((sp->n_strptr = malloc(prelen + postlen + 2)) == NULL)
		error(mallerr, prelen + postlen + 2, "undup;sym");
	strncpy(sp->n_strptr, prefix, prelen);
	sp->n_strptr[prelen] = ';';
	strncpy(sp->n_strptr + prelen + 1, postfix, postlen);
	sp->n_strptr[sp->n_strlen = prelen + 1 + postlen] = '\0';

	sp->n_dup ^= MAYDUP|UNDUPED;	/* but will be wiped if no prefix */
}

/* replace NODUPs by MAYDUPs when a new module is loaded */
static void
reset_nodups(void)
{
	register SYMENT *sp, *esp;
	register MODENT *mp;

	for (mp = modents; mp != NULL; mp = mp->next) {
		sp = mp->syments - 1;
		esp = mp->symendp;
		while (++sp < esp) {
			if (sp->n_dup & NODUP) {
				sp->n_dup ^= NODUP|MAYDUP;
				if (--nodups == 0)
					return;
			}
		}
	}
}

/* search symbol table by name */
static struct syment *
_symsrch(register char *name, int internal)
{
	register SYMENT *sp;
	register ushort len;
	MODENT *mp;
	SYMENT *esp;
	MODENT *savmp;
	SYMENT *savsp;
	int found;
	vaddr_t addr;
	char tdn;

	if (name == NULL || *name == '\0')
		return NULL;
	len = (ushort)strlen(name);
again:
	found = 0;
	for (mp = modents; mp != NULL; mp = mp->next) {
		sp = mp->syments - 1;
		esp = mp->symendp;
		while (++sp < esp) {
			if (len == sp->n_strlen
			&&  *(short *)name == *(short *)sp->n_strptr
			&&  memcmp(name, sp->n_strptr, len) == 0) {
				if (!(sp->n_dup & MAYDUP))
					goto chksym;
			}
			else if (!(sp->n_dup & UNDUPED)
			|| strcmp(strchr(sp->n_strptr, ';') + 1, name) != 0)
				continue;
			if (!found) {
				found = 1;
				savsp = sp;
				savmp = mp;
			}
			else if (sp->n_dup & LOCAL) {
				if (savsp->n_dup & LOCAL)
					found++;
			}
			else if (savsp->n_dup & LOCAL) {
				found = 1;
				savsp = sp;
				savmp = mp;
			}
			else
				found++;
		}
	}
	if (found == 1
	&& !(savsp->n_dup & UNDUPED)) {
		sp = savsp;
		mp = savmp;
		sp->n_dup ^= MAYDUP|NODUP;
		nodups++;
	}
	else
		sp = NULL;
chksym:
	if (!active
	||  internal == DONTKSYM
	|| (internal == SYMINIT && sp == NULL))
		return sp;

	if (sp != NULL) {
		if (checksym(sp, mp))
			return sp;
		if (internal == SYMINIT)
			return NULL;
		if (mp->mc_id != 0 && rdmodsyms(NULL))
			goto again;
	}

	if ((addr = getkaddr(name, &tdn)) == 0)
		return NULL;

	if (sp == NULL
	&&  rdmodsyms(NULL))
		goto again;

	return findsp(name, addr, tdn);
}

struct syment *
symsrch(char *name)
{
	return _symsrch(name, EXTERNAL);
}

vaddr_t
symfindval(char *name)
{
	register SYMENT *sp;

	if ((sp = _symsrch(name, EXTERNAL)) != NULL)
		return sp->n_value;
	error("%s not found in symbol table\n", name);
}

/* search symbol table by value */
struct syment *
findsym(register vaddr_t addr)
{
	register SYMENT *sp;
	register MODENT *mp;
	char name[MAXSYMNMLEN+1];
	char tdn;

	if (addr < kvbase || addr >= KVPER_ENG_END)
		return NULL;
again:
	for (mp = modents; mp != NULL; mp = mp->next) {
		if (addr < mp->bot || addr >= mp->top)
			continue;
		if ((sp = mp->symendp) == NULL)
			break;
		while (--sp >= mp->syments) {
			if (sp->n_value <= addr) {
				if ((sp->n_dup & MAYDUP)
				&& _symsrch(sp->n_strptr, DONTKSYM) != sp)
					undupsym(sp, (sp->n_dup & LOCAL)?
						NULL: mp->mc_name);
				goto chksym;
			}
		}
		break;
	}
	sp = NULL;
chksym:
	if (!active)
		return sp;

	if (sp != NULL) {
		if (checksym(sp, mp))
			return sp;
		if (mp->mc_id != 0 && rdmodsyms(NULL))
			goto again;
	}

	if ((addr = getkname(name, addr)) == 0)
		return NULL;

	if (sp == NULL
	&& (mp == NULL || mp->mc_id != 0)
	&&  rdmodsyms(NULL))
		goto again;

	return findsp(name, addr, '\0');
}

char *
db_sym_off(vaddr_t addr)
{
	register struct syment *sp;
	register char *cp;
	static char line[MAXSYMNMLEN+13];
	SYMENT svsp;

	if (addr >= kvbase) {
		sp = findsym(addr);
		if (sp == NULL || addr - sp->n_value >= PAGESIZE) {
			register int best, i;
			/*
			 * Try against strcon.c's saved values $a-$z
			 */
			for (best = 1, i = 2; i < 27; i++) {
				if (addr-savevalue[i] < addr-savevalue[best])
					best = i;
			}
			if (addr - savevalue[best] < PAGESIZE) {
				svsp.n_strlen = 2;
				svsp.n_tdn = sp? sp->n_tdn: 'n';
				svsp.n_strptr = "$?";
				svsp.n_strptr[1] = best + 'a' - 1;
				svsp.n_value = savevalue[best];
				sp = &svsp;
			}
			else if (sp
			&& addr - savevalue[best] <= addr - sp->n_value)
				sp = NULL;
		}
	}
	else
		sp = NULL;

	cp = line + 1;
	if (sp == NULL) {
		line[0] = 0;
  		sprintf(cp,"%08x", addr);
	}
	else {
		line[0] = 1;
		memcpy(cp, sp->n_strptr, sp->n_strlen);
		cp += sp->n_strlen;
		if ((addr -= sp->n_value) != 0) {
			cp += sprintf(cp, "+%x", addr);
			line[0] = 2;
		}
		if ((sp->n_tdn|LC) == 't') {
			*cp++ = '(';
			*cp++ = ')';
		}
		*cp = '\0';
	}
	return line + 1;	/* with indicator byte just behind */
}

vaddr_t
findsymval(vaddr_t addr)
{
	register struct syment *sp;

	return (sp = findsym(addr))? sp->n_value: 0;
}

boolean_t
istextval(vaddr_t addr)
{
	register struct syment *sp;

	if (!(sp = findsym(addr)))
		return B_FALSE;
	if ((sp->n_tdn|LC) != 'n')
		return ((sp->n_tdn|LC) == 't');
	return (sp >= modents[3].syments && sp < modents[3].symendp);
}

/* print result of nm command */
void
prnm(vaddr_t addr)
{
	register SYMENT *sp;

	sp = findsym(addr);
	fprintf(fp, "0x%08x ", addr);

	if (sp == NULL) {
		if (addr < kvbase)
			fprintf(fp, "is below the kernel address space\n");
		else
			fprintf(fp, "is in no range of the symbol table\n");
	}
	else if (sp->n_value == addr)
		fprintf(fp, "%c %s\n", sp->n_tdn, sp->n_strptr);
	else
		fprintf(fp, "%c %s+%x\n", sp->n_tdn, sp->n_strptr,
			addr - sp->n_value);
}

void
prnm_all(void)
{
	register SYMENT *sp, *esp;
	MODENT *mp, *tmp;
	vaddr_t bot;
	char *cp;
	char *agreement;

	if (active)
		(void)rdmodsyms(NULL);
	for (bot = 0; bot < KVPER_ENG_END; bot = mp->top) {
		for (mp = tmp = modents; tmp != NULL; tmp = tmp->next) {
			if (tmp->bot >= bot
			&&  tmp->bot < mp->bot)
				mp = tmp;
		}
		sp = mp->syments - 1;
		esp = mp->symendp;
		while (++sp < esp) {
			cp = sp->n_strptr;
			if (sp->n_dup & UNDUPED) {
				/* lose prefix because MAYDUPs won't have it */
				cp = strchr(cp, ';') + 1;
			}
			agreement = "";
			if (debugmode > 0 && active
			&& !(sp->n_dup & (LOCAL|OUROWN))
			&& !checksym(sp, mp))
				agreement = " but getksym() disagrees";
			fprintf(fp,"0x%08x %c %s%s\n",
				sp->n_value, sp->n_tdn, cp, agreement);
		}
	}
}

static int
symvalcmp(register SYMENT *a, register SYMENT *b)
{
	register int ret;

	/*
	 * Called by qsort() to sort symbols by value.
	 * Order symbols of the same value so that the shortest
	 * and lexicographically least name comes last,
	 * so that's the name chosen by findsym().
	 * Rarely reaches the memcmp(), so don't bother to avoid it.
	 */

	if (ret = (int)(a->n_value - b->n_value))
		return ret;			/* lesser value first */
	if (ret = ((b->n_dup == OUROWN) - (a->n_dup == OUROWN)))
		return ret;			/* invented name first */
	if (ret = (((b->n_tdn|LC) == 'n') - ((a->n_tdn|LC) == 'n')))
		return ret;			/* untyped name first */
	if (ret = ((b->n_tdn >= 'a') - (a->n_tdn >= 'a')))
		return ret;			/* local name first */
	if (ret = ((int)b->n_strlen - (int)a->n_strlen))
		return ret;			/* longer name first */
	return memcmp(b->n_strptr, a->n_strptr, a->n_strlen);
						/* -lexicographically */
}

static int
skeleton_symtab(MODENT *mp)
{
	SYMENT *sp;
	size_t len;
	int nents;
	char *cp;
	int noflesh;

	if (mp->mc_id == 0)
		return 0;

	/*
	 * If the user has not sent us the (right) dynamically
	 * loaded modules, then stack trace is likely to give up
	 * just where it gets interesting, because a text symbol
	 * is not found.  Therefore always (malloc permitting)
	 * provide a skeleton symtab of "symbols" module;smod
	 * module;emod module;sbss and module;ebss.  Assume
	 * module;smod is text unless the next symbol is not.
	 * module;smod also provides a name for any unnamed
	 * variables at the start of the module.
	 */

	nents = (mp->next == &mp[1])? 4: 2;
	if ((cp = strchr(mp->mc_name, ';')) != NULL
	&&   strcmp(cp, ";smod") == 0) {
		/* we've been here already */
		len = cp - mp->mc_name + 6;
		cp = mp->mc_name;
	}
	else {
		len = strlen(mp->mc_name) + 6;
		if ((cp = malloc(nents * len)) == NULL)
			return 0;
		memcpy(cp, mp->mc_name, len - 6);
		memcpy(cp + len - 6, ";smod", 6);
		free(mp->mc_name);
		mp->mc_name = cp;
	}

	if ((noflesh = (mp->syments == NULL))
	&& (mp->syments = malloc(nents * sizeof(SYMENT))) == NULL)
		return 0;

	sp = mp->syments;
	sp->n_strlen = len - 1;
	sp->n_tdn = 't';
	sp->n_dup = OUROWN;
	sp->n_strptr = cp;
	sp->n_value = mp->bot;
	sp++;

	memcpy(cp + len, cp, len);
	cp += len;
	cp[len - 5] = 'e';

	sp->n_strlen = len - 1;
	sp->n_tdn = 'n';
	sp->n_dup = OUROWN;
	sp->n_strptr = cp;
	sp->n_value = mp->top - 1;
	sp++;

	if (noflesh)
		mp->symendp = sp;
	if (nents == 2)
		return 2;
	++mp;
	if (noflesh)
		mp->syments = sp;

	memcpy(cp + len, cp, len - 6);
	cp += len;
	memcpy(cp + len - 6, ";sbss", 6);

	sp->n_strlen = len - 1;
	sp->n_tdn = 'd';
	sp->n_dup = OUROWN;
	sp->n_strptr = cp;
	sp->n_value = mp->bot;
	sp++;

	memcpy(cp + len, cp, len);
	cp += len;
	cp[len - 5] = 'e';

	sp->n_strlen = len - 1;
	sp->n_tdn = 'n';
	sp->n_dup = OUROWN;
	sp->n_strptr = cp;
	sp->n_value = mp->top - 1;
	sp++;

	if (noflesh)
		mp->symendp = sp;
	return 4;
}

/*
**	Read symbol table of ELF namelist
*/
static int
rdelfsym(int fd, MODENT *mp)
{
	register Elf32_Sym *sy;
	register SYMENT *sp;
	SYMENT *tsp;
	Elf *elfd;
	Elf_Scn	*scn;
	Elf_Scn *symscn;
	Elf32_Shdr *shp;
	Elf32_Sym *symtab;
	Elf32_Sym *symtabend;
	Elf_Data *data;
	Elf32_Ehdr *ehdr;
	Elf32_Half sci;
	Elf32_Half strsci;
	vaddr_t *scnbase;
	vaddr_t curraddr;
	char *cp;
	size_t mall;
	int okay;
	int bones;
	int type;


        if (elf_version (EV_CURRENT) == EV_NONE)
		fatal("ELF Access Library out of date\n");
	
        if ((elfd = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		prerrmes("elf_begin() failed\n");
		return -1;
	}

	type = elf_kind(elfd);
	if (type != ELF_K_ELF && type != ELF_K_COFF) {
		elf_end(elfd);
		prerrmes("module not in ELF format\n");
		return -1;
	}

	if ((ehdr = elf32_getehdr(elfd)) == NULL
	||   ehdr->e_shnum == 0 || ehdr->e_shnum > SHN_LORESERVE) {
		elf_end(elfd);
		prerrmes("cannot read ELF header\n");
		return -1;
	}

	/* loadable modules are relocatable ELF files */
	if ((mp->mc_id == 0 && ehdr->e_type != ET_EXEC)
	||  (mp->mc_id != 0 && ehdr->e_type != ET_REL)) {
		elf_end(elfd);
		prerrmes("module is wrong ELF type\n");
		return -1;
	}

	if ((scnbase = malloc(mall = ehdr->e_shnum*sizeof(vaddr_t))) == NULL) {
		elf_end(elfd);
		prerrmes(mallerr, mall, "section index");
		return -1;
	}

	curraddr = mp->bot;	/* for text and data */
	scnbase[0] = 0;
	symscn = NULL;
	strsci = SHN_UNDEF;
	for (scn = NULL, sci = 1; sci < ehdr->e_shnum; sci++) {
		if ((scn = elf_nextscn(elfd, scn)) == NULL
		||  (shp = elf32_getshdr(scn)) == NULL) {
			free(scnbase);
			elf_end(elfd);
			prerrmes("cannot read section header\n");
			return -1;
		}
		if (shp->sh_type == SHT_SYMTAB) {
			symscn = scn;	/* Can only do 1 symbol table */
			strsci = shp->sh_link;
		}
		if ((shp->sh_type != SHT_MOD 
		  && shp->sh_type != SHT_PROGBITS
		  && shp->sh_type != SHT_NOBITS)
		|| !(shp->sh_flags & SHF_ALLOC))
			scnbase[sci] = 0;
		else {
			curraddr = ALIGN(curraddr, shp->sh_addralign);
			scnbase[sci] = curraddr;
			curraddr += shp->sh_size;
		}
	}
	if (mp->mc_id != 0
	&&  PAGEROUNDUP(curraddr) != mp->top) {
		free(scnbase);
		elf_end(elfd);
		return 0;	/* "does not match" */
	}
	curraddr = mp[1].bot;	/* for commons */

	if (symscn == NULL) {
		free(scnbase);
		elf_end(elfd);
		prerrmes("cannot find symbol header\n");
		return -1;
	}
	if (strsci == SHN_UNDEF) {
		free(scnbase);
		elf_end(elfd);
		prerrmes("cannot find string header\n");
		return -1;
	}

	/* get symbol table */

	if ((data = elf_getdata(symscn, NULL)) == NULL
	||  (data->d_size == 0) || (data->d_buf == NULL)) {
		free(scnbase);
		elf_end(elfd);
		prerrmes("cannot read symbol table\n");
		return -1;
	}

	symtab = (Elf32_Sym *)data->d_buf;
	symtabend = (Elf32_Sym *)((char *)data->d_buf + data->d_size);

	/* get string table */

	if ((scn = elf_getscn(elfd, strsci)) == NULL
	||  (data = elf_getdata(scn, NULL)) == NULL
	||  (data->d_size == 0) || (data->d_buf == NULL)) {
		free(scnbase);
		elf_end(elfd);
		prerrmes("cannot read string table\n");
		return -1;
	}

	if ((mp->strings = malloc(mall = data->d_size)) == NULL) {
		free(scnbase);
		elf_end(elfd);
		prerrmes(mallerr, mall, "string table");
		return -1;
	}

	(void)memcpy(mp->strings, data->d_buf, data->d_size);

	mall = (4 + symtabend - symtab) * sizeof(SYMENT);
	if ((mp->syments = malloc(mall)) == NULL) {
		free(mp->strings);
		mp->strings = NULL;
		free(scnbase);
		elf_end(elfd);
		prerrmes(mallerr, mall, "symbol table");
		return -1;
	}

	if (nodups)
		reset_nodups();
	bones = skeleton_symtab(mp);
	sp = mp->syments + bones;
	okay = 1;

	/*
	 **	convert ELF symbol table info into SYMENT entries
	 */

	for (sy = symtab; sy < symtabend; sy++) {
		if ((type = ELF32_ST_TYPE(sy->st_info)) > STT_FUNC)
			continue;

		cp = mp->strings + sy->st_name;
		if ((sp->n_strlen = strlen(cp)) == 0)
			continue;
	
		sp->n_strptr = cp;
		while (isxdigit(*cp))
			++cp;
		if (*cp == '\0')
			undupsym(sp, "");

		if (mp->mc_id == 0) {
			if ((sp->n_value = sy->st_value) < kvbase+PHYSEND) {
				if ((sp->n_value == 0) || (sp->n_value >= PHYSEND))
					continue;
				sp->n_value = sp->n_value +kvbase;
			}
			else if (sp->n_value >= KVPER_ENG_END)
				continue;
			sp->n_dup = GLOBAL;
		}
		else if (sy->st_shndx < sci) {
			sp->n_value = sy->st_value + scnbase[sy->st_shndx];
			if (sp->n_value < mp->bot)
				continue;
			if (sp->n_value > mp->top) {
				okay = 0;	/* for "does not match" */
				break;
			}
			/*
			 * Dynamic modules may duplicate global names
			 */
			sp->n_dup = GLOBAL|MAYDUP;
		}
		else if (sy->st_shndx == SHN_COMMON) {
			/* Is the common symbol defined by this module? */
			if ((tsp = _symsrch(sp->n_strptr, DONTKSYM))
			&&  !(tsp->n_dup & LOCAL))
				continue;
			curraddr = ALIGN(curraddr, sy->st_value);
			sp->n_value = curraddr;
			curraddr += sy->st_size;
			sp->n_dup = GLOBAL;
		}
		else
			continue;			/* shouldn't happen */

		switch (type) {
		case STT_FUNC:		sp->n_tdn = 'T'; break;
		case STT_OBJECT:	sp->n_tdn = 'D'; break;
		default:		sp->n_tdn = 'N'; break;
		}
		if (ELF32_ST_BIND(sy->st_info) != STB_GLOBAL) {
			if (sp->n_strptr[0] == '.')
				continue;
			sp->n_tdn += 'a' - 'A';
			sp->n_dup = LOCAL|MAYDUP;
		}

		sp++;
	}

	free(scnbase);
	elf_end(elfd);

	if (okay > 0) {
		if (sp == mp->syments + bones) {
			prerrmes("symbol table is empty\n");
			okay = -1;
		}
		else if (PAGEROUNDUP(curraddr) != mp[1].top) {
			/* caller will report "does not match" */
			okay = 0;
		}
	}
	if (okay <= 0) {
		free(mp->syments);
		free(mp->strings);
		mp->syments = NULL;
		mp->strings = NULL;
		return okay;
	}

	mp->symendp = sp;
	qsort(mp->syments, sp - mp->syments, sizeof(SYMENT), symvalcmp);

	if (mp->mc_id == 0) {
		static char *ends[4] = {"_end", "_edata", "_etext", "stext"};
		/*
		 * Divide unix into high, bss, data, text, low
		 */
		symendp = (SYMENT *)((char *)mp->syments + mall);
		for (sci = 0; sci < 4; sci++, mp++) {
			if ((sp = _symsrch(ends[sci], SYMINIT)) == NULL) {
				free(mp->syments);
				free(mp->strings);
				memset(modents, 0, sizeof(modents));
				modents[0].top = KVPER_ENG_END;
				symendp = NULL;
				break;	/* "does not match" */
			}
			if(sci == 3) sp--; /* one before start of stext */
			mp[1] = *mp;
			mp[1].top = PAGEROUNDUP(sp->n_value);
			mp->syments = mp[1].symendp = ++sp;
			if ((mp->bot = sp->n_value) < mp[1].top)
				mp[1].top = mp->bot;
			mp->next = &mp[1];
		}
		return 0;
	}

	if (mp->next == &mp[1]) {
		/*
		 * Divide module into text+data (0) and commons (1)
		 */
		mp[1].syments = mp->syments;
		mp[1].symendp = mp->symendp;
		mp[1].strings = mp->strings;
		sp = mp->syments;
		if (sp->n_value >= mp->bot && sp->n_value < mp->top) {
			while (sp < mp->symendp && sp->n_value < mp->top)
				++sp;
			mp[1].syments = mp->symendp = sp;
		}
		else {
			sp = mp->symendp;
			while (--sp >= mp->syments && sp->n_value >= mp->bot)
				;
			mp[1].symendp = mp->syments = ++sp;
		}
	}

	if (bones && ((mp->syments)[1].n_tdn|LC) != 't')
		mp->syments->n_tdn = 'd';

	if (active) {
		/*
		 * Check last globals against getksym
		 */
		for (sci = 0; sci < 2; sci++, mp++) {
			if (mp->symendp == NULL
			||  checksym(mp->symendp - 1, mp))
				continue;
			mp -= sci;
			sp = mp[1].syments;
			if (sp == NULL || sp > mp->syments)
				sp = mp->syments;
			free(sp);
			free(mp->strings);
			for (sci = 0; sci < 2; sci++, mp++) {
				mp->symendp = mp->syments = NULL;
				mp->strings = NULL;
			}
			break;	/* "does not match" */
		}
	}
	return 0;
}

/* size.c append l.fields and lm.fields and m.fields to kernel symbol table */
void
addsymfields(char *name, struct offstable *tp)
{
	register SYMENT *symentp, *sp;
	size_t len;

	if ((symentp = _symsrch(name, SYMINIT)) == NULL
	||   symentp <  modents[0].syments
	||   symentp >= (sp = modents[0].symendp))
		return;		/* symbol isn't where we expect it */

	len = strlen(name) + 1;
	if (tp->name[-1] != '.'
	||  strncmp(name, tp->name - len, len - 1) != 0)
		return;		/* crash.mk didn't prefix the fields */

	while ((++tp)->name != NULL) {
		if (sp >= symendp)
			return;	/* we've used up all the spare entries */
		sp->n_strlen = strlen(tp->name) + len;
		sp->n_tdn = 'n';
		sp->n_dup = OUROWN;
		sp->n_strptr = tp->name - len;
		sp->n_value = symentp->n_value + tp->offset;
		++sp;
	}

	modents[0].symendp = sp;
	qsort(symentp, sp - symentp, sizeof(SYMENT), symvalcmp);
}

static int
goodbottop(register MODENT *testmp)
{
	register MODENT *mp;

	if (testmp->bot < kvbase
	||  testmp->top > KVPER_ENG_END
	||  testmp->top <= testmp->bot)
		goto bad;
	for (mp = modents; mp != NULL; mp = mp->next) {
		if (testmp == mp)
			continue;
		if (testmp->bot >= mp->bot) {
			if (testmp->bot < mp->top)
				goto bad;
		}
		else {
			if (testmp->top > mp->bot)
				goto bad;
		}
	}
	return 1;
bad:
	testmp->bot = testmp->top = 0;
	return 0;
}

/* load symbol information for dynamically loaded modules */
int
rdmodsyms(char *lmodpath)
{
	int fd;
	char *path, *lpath, *leaf;
	struct modctl modctl;
	struct modctl *modctlp;
	struct module module;
	struct modctl_list mcl;
	register MODENT *pp, *mp;
	MODENT *ep, *dpp;
	static SYMENT *Modhead;
	static char *modpath;
	static int accessed = 1;
	int mismatch = 0;
	int changed = 0;
	size_t modlen;

	/* note: this intentionally fails if the basic namelist failed */
	if (!Modhead && !(Modhead = _symsrch("modhead", SYMINIT)))
		Modhead = (SYMENT *)(-1);
	if (Modhead == (SYMENT *)(-1))
		return 0;

	if (lmodpath != NULL) {			/* called from cr_open() */
		if (lmodpath == (char *)(-1)) {	/* don't read in modules */
			modpath = lmodpath;
			accessed = 0;
		}
		else {
			modpath = cr_malloc(strlen(lmodpath)+3, "modpath");
			strcpy(modpath, lmodpath);
			strcat(modpath, "/.");
			cr_unfree(modpath);
		}
	}

	/*
	 * Let caller specify invalid modpath to stop us spending
	 * time reading symbols from dynamically loaded modules.
	 * Check this once to avoid repeating the error too much.
	 */
	if (modpath != NULL && modpath != (char *)(-1)) {
		if (access(modpath, X_OK|EFF_ONLY_OK) < 0) {
			if (accessed)
				prerrmes("cannot access modpath %s\n",modpath);
			accessed = 0;
		}
		else
			accessed = 1;
	}

	/*
	 * modhead.mc_next points to the last module loaded:
	 * we need to work the opposite way round, starting
	 * with the first module loaded - though even that
	 * ordering is insufficient, it has to be complicated
	 * by the modctl_list to get the dependencies right:
	 * important for the correct placement of commons
	 * (but I'm not absolutely convinced that even this
	 * is reliable - complicate it if a problem shows up).
	 * Disable SIGINT while we process the modules.
	 */
	for (mp = modents; mp != NULL; ep = mp, mp = mp->next)
		;
	if (intsig)	/* fully initialized */
		(void)signal(SIGINT, SIG_IGN);
	readmem(Modhead->n_value, 1, -1,
		&modctl, sizeof(struct modctl), "modhead");

	/*
	 * First check our list of modules against the kernel,
	 * freeing out of date slots and allocating for new ones.
	 * A weakness here is that we won't notice a reload at new
	 * addresses if the reload happens to get the same mc_modp:
	 * sorry, quit and restart crash if that occurs.
	 */
	while ((modctlp = modctl.mc_next) != NULL) {
		readmem((vaddr_t)modctlp, 1, -1,
			&modctl, sizeof(struct modctl), "modctl structure");
		if (modctl.mc_id == 0)
			continue;
 
		for (mp = modents; mp != NULL; pp = mp, mp = mp->next) {
			if (mp->mc_id == modctl.mc_id)
				break;
		}

		if (mp != NULL
		&& (mp->mc_modp == NULL || mp->mc_modp != modctl.mc_modp)) {
			if (ep == mp)
				ep = pp;
			if (mp->next == &mp[1]) {
				pp->next = mp[1].next;
				if (mp->syments > mp[1].syments)
					mp->syments = mp[1].syments;
			}
			else
				pp->next = mp->next;
			if (mp->syments)
				free(mp->syments);
			if (mp->strings)
				free(mp->strings);
			if (mp->mc_modp != NULL)
				changed = 1;
			free(mp->mc_name);
			free(mp);
			mp = NULL;
		}

		if (mp == NULL && modctl.mc_modp != NULL) {
			if ((mp = malloc(sizeof(MODENT)*2)) == NULL)
				error(mallerr, sizeof(MODENT)*2, "modent");
			memset(mp, 0, sizeof(MODENT) + sizeof(MODENT));
			mp->next = ep->next;
			mp->mc_id = modctl.mc_id;
			mp->mc_name = readstr(modctlp->mc_name, "mc_name");
			cr_unfree(mp->mc_name);
			mp->mc_modp = modctl.mc_modp;
			mp->modctlp = modctlp;
			ep->next = mp;
		}
	}

	/*
	 * Then go down the list of new slots reading in their symbols,
	 * reordering slots if the modctl_list dependencies demand.
	 */
	pp = ep;
	while ((mp = pp->next) != NULL) {
		readmem((vaddr_t)mp->mc_modp, 1, -1,
			&module, sizeof(struct module), "module structure");

		dpp = NULL;
		mcl.mcl_next = module.mod_obj.md_mcl;
		while (mcl.mcl_next != NULL) {
			register MODENT *tpp;
			readmem((vaddr_t)mcl.mcl_next, 1, -1,
				&mcl, sizeof(mcl), "modctl_list");
			for (tpp = mp; tpp->next != NULL; tpp = tpp->next) {
				if (tpp->next->modctlp == mcl.mcl_mcp) {
					dpp = tpp;
					break;
				}
			}
		}

		if (dpp != NULL) {
			/*
			 * Move the last (first loaded) dependency
			 * before mp, then go back to do it instead.
			 */
			pp->next = dpp->next;
			dpp->next = dpp->next->next;
			pp->next->next = mp;
			continue;
		}

		pp = mp;
		if (!(module.mod_flags & MOD_SYMTABOK)) {
			mp->mc_modp = NULL;	/* try again later on */
			continue;
		}

		mp->bot = (vaddr_t)module.mod_obj.md_space;
		mp->top = mp->bot + module.mod_obj.md_space_size;
		if (!goodbottop(mp)) {
			mp->mc_modp = NULL;	/* try again later on */
			continue;
		}

		++mp;
		mp->bot = (vaddr_t)module.mod_obj.md_bss;
		mp->top = mp->bot + module.mod_obj.md_bss_size;
		if (goodbottop(mp--)) {
			mp[1].next = mp->next;
			mp[1].mc_name = mp->mc_name;
			pp = mp->next = &mp[1];
		}

		if (!accessed) {
			if (skeleton_symtab(mp))
				changed = 1;
			continue;
		}

		path = readstr(module.mod_obj.md_path, "module path");
		if (modpath != NULL) {
			if ((leaf = strrchr(path, '/')) == NULL)
				leaf = path;
			lpath = cr_malloc(strlen(modpath)+strlen(leaf)-1,
					"module path");
			strcpy(lpath, modpath);
			strcpy(lpath + strlen(lpath) - 2, leaf);
			cr_free(path);
			path = lpath;
		}

		if ((fd = open(path, O_RDONLY, 0)) < 0)
			prerrmes("cannot open module %s\n", path);
		else if (rdelfsym(fd, mp) < 0)
			prerrmes("cannot get symbols from module %s\n", path);
		else if (mp->syments == NULL) {
			prerrmes("dumpfile does not match module %s\n", path);
			mismatch++;
		}
		close(fd);
		cr_free(path);

		if (mp->syments || skeleton_symtab(mp))
			changed = 1;
	}

	if (mismatch)
		prerrmes("(dumpfile may not match other modules too)\n");
	if (intsig)
		(void)signal(SIGINT, intsig);
	return changed;
}

/* fixup symbol table after we have the real kvbase */
int
fixupsymtab()
{
	register MODENT *mp;
	register SYMENT *sp;
	char name[MAXSYMNMLEN+1];
	char tdn;

	for (mp = modents; mp != NULL; mp = mp->next) {
		if ((sp = mp->symendp) == NULL)
			break;
		while (--sp >= mp->syments) {
			if (sp->n_value <= kvbase) {
				sp->n_value += kvbase;
			}
		}
	}

	mp = modents;
	qsort(mp->syments, mp->symendp - mp->syments, sizeof(SYMENT), symvalcmp);

	return 0;
}
/* symbol table initialization function */
void
rdsymtab(char *namelist)
{
	int fd;

	modents[0].top = KVPER_ENG_END;	/* remainder initially 0 */
	if ((fd = open(namelist, O_RDONLY, 0)) < 0)
		prerrmes("cannot open namelist %s\n", namelist);
	else if (rdelfsym(fd, modents) < 0)
		prerrmes("cannot get symbols from namelist %s\n", namelist);
	else if (modents[0].syments == NULL)
		prerrmes("dumpfile does not match namelist %s\n", namelist);
	close(fd);
}
