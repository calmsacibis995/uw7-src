#ident	"@(#)crash:i386at/cmd/crash/resmgr.c	1.1"

/*
 * This file contains code for the crash function resmgr.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include "crash.h"

struct valbuf {
	char		format[10];
	unsigned long	value;
	unsigned long	evalue;
	char		valstr[48];
};
struct param {
	char		*name;
	char		*format;
};
struct nameval {
	char		*name;
	unsigned long	value;
};
struct namevals {
	char		*name;
	struct nameval	*nameval;
};

/*
 * These we actively look for, and print in fixed order for all keys.
 * Others we print only if -f and they are found, in the order found.
 */
#define  KEY		0
#define  MODNAME	1
static struct param params[] = {
	"KEY",		"%3d ",
	"MODNAME",	"%8s ",
	"UNIT",		"%4d ",
	"IPL",		"%3d ",
	"ITYPE",	"%5d ",
	"IRQ",		"%3d ",
	"IOADDR",	"%4x-%-4x ",
   	"MEMADDR",	"%8x-%-8x ",
	"DMAC",		"%4d ",
	"ENTRYTYPE",	"%9d\n",
	 NULL,		 NULL
};

static struct nameval itypes[] = {
	"edge1",	CM_ITYPE_EDGE,
	"edsh2",	2,
	"edsh3",	3,
	"level",	CM_ITYPE_LEVEL,
	 NULL,		0
};

static struct nameval entrytypes[] = {
	"default",	CM_ENTRY_DEFAULT,
	"drvowned",	CM_ENTRY_DRVOWNED,
	 NULL,		0
};

static struct nameval brdbustypes[] = {
	"+ISA",		CM_BUS_ISA,
	"+EISA",	CM_BUS_EISA,
	"+PCI",		CM_BUS_PCI,
	"+PCCARD",	CM_BUS_PCMCIA,
	"+PnPISA",	CM_BUS_PNPISA,
	"+MCA",		CM_BUS_MCA,
	"+SYS",		CM_BUS_SYS,
	 NULL,		0
};

static struct namevals namevals[] = {
	"ITYPE",	itypes,
	"ENTRYTYPE",	entrytypes,
	"BRDBUSTYPE",	brdbustypes,
	 NULL,		NULL,
};

/*
 * Declarations from kern:io/autoconf/resmgr/resmgr.c
 */

struct rm_val {
	void		*val;
	size_t		vlen;
	struct rm_val	*vnext;
};
struct rm_param {
	char		pname[RM_MAXPARAMLEN];
	struct rm_val	*pval;
	struct rm_param	*pnext;
};
struct rm_key {
	rm_key_t	key;
	struct rm_param	*plist;
	struct rm_key	*knext;
};

#define RM_HASHSZ	16
#define RM_HASHIT	(RM_HASHSZ - 1)

/*
 * Some local declarations
 */

static	vaddr_t		Rm_hashtbl;
static	vaddr_t		Rm_next_key;

static	void prresmgr(int, char *modname);
static	boolean_t getvalue(char *, struct valbuf *, struct rm_val **, int);
static	void putvalue(char *, struct valbuf *, size_t *);
static  size_t putkey(rm_key_t);

static	char noformat[] = " %s=%s ";
#define	UNFORMATTED(vbp) (strcmp((vbp)->format, noformat) == 0)
static	struct rm_val last_modname_rm_val;


/* get arguments for resmgr function */
getresmgr(void)
{
	int full = 0;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"fw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		do prresmgr(full, args[optind]);
		while (args[++optind]);
	}
	else
		prresmgr(full, NULL);
}

static void
prresmgr(int full, char *modname)
{
	struct valbuf valbuf, valbuf2;
	size_t len1, len2, rhspad, linelen;
	struct param *paramp;
	boolean_t morebrds;
	int brdinst;
	rm_key_t key, rm_next_key;
	struct rm_key rm_key;
	struct rm_key *rm_hashtbl[RM_HASHSZ];
	struct rm_param rm_param;
	struct rm_val *vfirst;

	if (!Rm_hashtbl)
		Rm_hashtbl = symfindval("rm_hashtbl");
	readmem(Rm_hashtbl,1,-1,
		rm_hashtbl,sizeof(rm_hashtbl),"rm_hashtbl");
	if (!Rm_next_key)
		Rm_next_key = symfindval("rm_next_key");
	readmem(Rm_next_key,1,-1,
		&rm_next_key,sizeof(rm_next_key),"rm_next_key");

	/*
	 * Print header with appropriate padding
	 */
	for (paramp = params; paramp->name != NULL; paramp++) {
		rhspad = 1;
		len1 = strlen(paramp->name);
		len2 = sprintf(valbuf.valstr, paramp->format, 0, 0);
		while (len1 + rhspad < len2) {
			if (++len1 + rhspad == len2)
				break;
			if (paramp->format[3] == '-')
				rhspad++;
		}
		fprintf(fp, "%*s%*s",
			len1, paramp->name, rhspad, valbuf.valstr+len2-1);
	}

	morebrds = B_FALSE;
	last_modname_rm_val.val = NULL;

	for (key = RM_KEY+1; key < rm_next_key; key += !morebrds) {
		if (morebrds) {
			brdinst++;
			morebrds = B_FALSE;
		}
		else {
			brdinst = 0;
			rm_key.key = RM_KEY;
			rm_key.knext = rm_hashtbl[key & RM_HASHIT];
			while (rm_key.key != key && rm_key.knext != NULL)
				readmem((vaddr_t)rm_key.knext,1,-1,
					&rm_key,sizeof(rm_key),"rm_key");
			if (rm_key.key != key || rm_key.plist == NULL)
				continue;
		}

		paramp = params;
		while ((++paramp)->name != NULL) {
			vfirst = NULL;
			rm_param.pnext = rm_key.plist;
			while (rm_param.pnext != NULL) {
				readmem((vaddr_t)rm_param.pnext,1,-1,
					&rm_param,sizeof(rm_param),"rm_param");
				if (strcmp(rm_param.pname, paramp->name) == 0) {
					vfirst = rm_param.pval;
					break;
				}
			}
			strcpy(valbuf.format, paramp->format);
			getvalue(paramp->name, &valbuf, &vfirst, brdinst);
			if (paramp == params + MODNAME) {
				if (modname
				&&  strcmp(modname, valbuf.valstr) != 0) {
					rm_key.plist = NULL;
					break;
				}
				putkey(brdinst? RM_KEY: key);
			}
			fprintf(fp, valbuf.format, valbuf.value, valbuf.evalue);
			if (vfirst != NULL)
				morebrds = B_TRUE;
		}

		if (!full)
			continue;

		linelen = 0;
		strcpy(valbuf.format, noformat);
		rm_param.pnext = rm_key.plist;

		while (rm_param.pnext != NULL) {
			readmem((vaddr_t)rm_param.pnext,1,-1,
				&rm_param,sizeof(rm_param),"rm_param");
			if ((vfirst = rm_param.pval) == NULL)
				continue;
			paramp = params;
			while ((++paramp)->name != NULL)
				if (strcmp(rm_param.pname, paramp->name) == 0)
					break;
			if (paramp->name != NULL)
				continue;
			rm_param.pname[RM_MAXPARAMLEN-1] = '\0'; /* safety */
			if (getvalue(rm_param.pname,&valbuf,&vfirst,brdinst))
				putvalue(rm_param.pname, &valbuf, &linelen);
			/*
			 * If we only find there is another board when
			 * we get to these auxilary params, don't start
			 * a new entry for it, just concatenate entries.
			 */
			while (vfirst != NULL && !morebrds) {
				getvalue(rm_param.pname, &valbuf, &vfirst, 0);
				putvalue(rm_param.pname, &valbuf, &linelen);
			}
		}
		if (linelen)
			fprintf(fp, "\n");
	}
}

static boolean_t
getvalue(char *pname, struct valbuf *vbp, struct rm_val **vfirstp, int brdinst)
{
	struct namevals *nbsp;
	struct nameval *nbp;
	struct rm_val rm_val;
	char *vsp, *typ;
	int ninst;

	ninst = -1;
	rm_val.vnext = *vfirstp;
	while (rm_val.vnext != NULL && ninst++ < brdinst)
		readmem((vaddr_t)rm_val.vnext,1,-1,
			&rm_val,sizeof(rm_val),"rm_val");
	*vfirstp = rm_val.vnext;

	if (UNFORMATTED(vbp)) {
		if (ninst < brdinst)
			return B_FALSE;
		brdinst = 0;	/* for default '-' below */
	}
	else if (ninst < brdinst)
		rm_val.val = NULL;
	if (pname == params[MODNAME].name) {
		if (rm_val.val == NULL || rm_val.vlen == 0)
			rm_val = last_modname_rm_val;
		else
			last_modname_rm_val = rm_val;
	}

	vsp = vbp->valstr;
	typ = vbp->format;
	while (!isalpha(*typ))
		++typ;

	if (rm_val.val != NULL && rm_val.vlen != 0) {
		*(unsigned long *)vsp = 0;
		*((unsigned long *)vsp + 1) = 0;
		if (rm_val.vlen >= sizeof(vbp->valstr))
			rm_val.vlen = sizeof(vbp->valstr) - 1;
		readmem((vaddr_t)rm_val.val,1,-1,vsp,rm_val.vlen,"values");
		vsp[rm_val.vlen] = '\0';

		/*
		 * Do we have a name for this value?
		 */
		vbp->value = *(unsigned long *)vsp;
		vbp->evalue = *((unsigned long *)vsp + 1);
		*vsp = '\0';
		for (nbsp = namevals; nbsp->name; nbsp++)
			if (strcmp(pname, nbsp->name) == 0)
				break;
		for (nbp = nbsp->nameval; vbp->value && nbp->name; nbp++) {
			if (nbp->name[0] == '+') {
				if (vbp->value & nbp->value) {
					vbp->value &= ~nbp->value;
					strcat(vsp, nbp->name + !*vsp);
				}
			}
			else {
				if (vbp->value == nbp->value) {
					vbp->value = 0;
					strcpy(vsp, nbp->name);
				}
			}
		}

		if (*vsp) {	/* yes, value is named */
			if (vbp->value) {
				sprintf(vsp + strlen(vsp),
					(hexmode || vbp->value <= 9)?
					"+%x": "+0x%x", vbp->value);
			}
			*typ = 's';
		}
		else {		/* no, restore original string */
			*(unsigned long *)vsp = vbp->value;
			if (UNFORMATTED(vbp)) {
				/*
				 * Does this look like a good string?
				 * If not, replace it by number string
				 * (always give string so strlen works).
				 */
				while (isprint(*vsp))
					++vsp;
				if (*vsp != '\0'
				||   vsp - vbp->valstr != rm_val.vlen - 1) {
					sprintf(vbp->valstr,
						hexmode? "%x":
						((vbp->value<=99)?"%d":"0x%x"),
						vbp->value);
				}
			}
		}
	}
	else {
		/*
		 * No value: replace by default string
		 */
		*typ = 's';
		*vsp = brdinst? ' ': '-';
		vsp[1] = '\0';
		if (typ[1] == '-') {	/* range */
			typ[1] = *vsp;
			typ[5] = *typ;
		}
	}

	if (*typ == 's')
		vbp->value = vbp->evalue = (unsigned long)vbp->valstr;
	return B_TRUE;
}

static void
putvalue(char *pname, struct valbuf *vbp, size_t *linelenp)
{
	struct valbuf valbuf;
	struct rm_val *vfirst;

	/*
	 * Print a variable length " NAME=value ", avoiding 80-col wrap,
	 * inserting spaces below key and repeating modname on new line.
	 */

	if (*linelenp + strlen(noformat) +
	    strlen(pname) + strlen((char *)vbp->value) >= 82) {
		fprintf(fp, "\n");
		*linelenp = 0;
	}
	if (*linelenp == 0) {
		*linelenp = putkey(RM_KEY);
		strcpy(valbuf.format, params[MODNAME].format);
		vfirst = NULL;	/* just pick up last modname again */
		getvalue(params[MODNAME].name, &valbuf, &vfirst, 1);
		*linelenp += fprintf(fp, valbuf.format, valbuf.value);
	}
	*linelenp += fprintf(fp, vbp->format, pname, vbp->value);
}

static size_t
putkey(rm_key_t key)
{
	char format[10];
	char *typ;

	/*
	 * Print the key, taking its format from the table, or print
	 * the right number of spaces instead if this is a repeat line.
	 */

	typ = strcpy(format, params[KEY].format);
	if (key == RM_KEY) {
		while (!isalpha(*typ))
			++typ;
		*typ = 's';
		key = (rm_key_t)"";
	}
	return fprintf(fp, format, key);
}
