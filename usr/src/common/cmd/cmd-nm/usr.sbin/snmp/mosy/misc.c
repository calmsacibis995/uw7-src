#ident	"@(#)misc.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)misc.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
#ifndef lint
static char SysVr3TCPID[] = "@(#)misc.c	6.2 Lachman System V STREAMS TCP source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/* misc.c - PE parser (yacc-based) and other misc routines */

/*
 *      System V STREAMS TCP - Release 5.0
 *
 *  Copyright 1992 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 * 
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include "mosy-defs.h"

#ifdef SVR4
#define   bcopy(b1,b2,length)	(void) memcpy ((b2), (b1), (length))
#endif

/* OID manipulation */
typedef struct oidlist {
    OID		op_oid;
    char	*op_name;
    struct oidlist *op_next;
} oidlist, *OP;
#define NULLOP ((OP) 0)

typedef struct symtable {
    char	*sym_name;
    char	*sym_module;
    OID		sym_oid;
    struct symtable *sym_next;
} symtable, *SYM;
#define NULLSYM ((SYM)0)

static OP	myoids;
static SYM	symtab[MAX_TBLS];


OID
addoid (o1, o2)
OID	o1, o2;
{
    OID		noid;

    if (o1 == NULLOID || o2 == NULLOID)
	return NULLOID;

    noid = (OID) calloc (1, sizeof(*noid));
    if (noid == NULLOID)
	myyerror (gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*noid));

    noid -> oid_nelem = o1->oid_nelem + o2->oid_nelem;
    noid -> oid_elements = (unsigned int *) calloc ((unsigned)noid->oid_nelem,
						    sizeof(unsigned int));
    if (noid -> oid_elements == NULL)
	myyerror (gettxt(":8", "Out of memory: %d needed.\n"), sizeof(noid->oid_nelem));

    bcopy ((char *)o1->oid_elements, (char *)noid->oid_elements,
	   o1->oid_nelem * sizeof(unsigned int));
    bcopy ((char *)o2 -> oid_elements,
	   (char *) &noid -> oid_elements[o1->oid_nelem],
	   o2 -> oid_nelem * sizeof(unsigned int));
    return noid;
}

defineoid (name, oid)
char	*name;
OID	oid;
{
    register char	*p;
    register OP		op;

    if (oid == NULLOID) {
	myyerror (gettxt(":336", "Warning Null oid in defineoid.\n"));
	return;
    }
    for (op = myoids; op; op = op -> op_next)
	if (strcmp (op -> op_name, name) == 0) {
	    if (oid_cmp(op->op_oid, oid) != 0) {
		p = new_string(sprintoid (oid));
		warning (gettxt(":46", "OID name clash %s => %s & %s"),
			  name, p, sprintoid(op->op_oid));
		free (p);
	    }
	    else
		return;
	}
    op = (OP) calloc (1, sizeof *op);
    if (op == NULLOP)
	myyerror (gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*op));
    op -> op_oid = oid_cpy(oid);
    op -> op_name = new_string (name);
    op -> op_next = myoids;
    myoids = op;
}

OID
oidlookup (name)
char	*name;
{
    OP	op;

    for (op = myoids; op; op = op -> op_next)
	if (strcmp ( name, op->op_name) == 0)
	    return oid_cpy(op -> op_oid);

    warning (gettxt(":47", "Unknown Object Identifier '%s'"), name);
    return NULLOID;
}

char *
oidname (oid)
OID	oid;
{
    OP	op;

    for (op = myoids; op; op = op -> op_next)
	if (oid_cmp (op->op_oid, oid) == 0)
	    return op -> op_name;

    return NULLCP;
}

OID
int2oid (n)
int	n;
{
    OID	noid;

    noid = (OID) calloc(1, sizeof(*noid));
    if (noid == NULLOID)
	myyerror (gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*noid));

    noid -> oid_elements = (unsigned int *) calloc (1, sizeof(unsigned int));
    if (noid -> oid_elements == NULL)
	myyerror (gettxt(":8", "Out of memory: %d needed.\n"), sizeof(unsigned int));
    noid -> oid_nelem = 1;
    noid -> oid_elements[0] = n;
    return noid;
}

addtable (name, lt)
char	*name;
int	lt;
{
    SYM	sp;

    sp = (SYM)calloc (1, sizeof *sp);
    sp -> sym_name = new_string (name);
    sp -> sym_next = symtab[lt];
    symtab[lt] = sp;
}

addtableref (name, id, lt)
char	*name;
OID	id;
int	lt;
{
    SYM		sp;
    char	*nm;
    OID		oid;

    nm = name ? new_string (name) : NULLCP;
    oid = id ? oid_cpy (id) : NULLOID;

    for (sp = symtab[lt]; sp; sp = sp -> sym_next)
	if (sp -> sym_module == NULLCP && sp -> sym_oid == NULLOID)
	{
	    sp -> sym_module = nm;
	    sp -> sym_oid = oid;
	}
}

print_expimp ()
{
    SYM		sp;
    int		ind;
    OID		oid;
    char	*p;

    if (sp = symtab[TBL_EXPORT])
	printf ("\nEXPORTS\n");
	
    for (ind = 0; sp; sp = sp->sym_next) {
	if (ind == 0) {
	    putchar('\t');
	    ind = 8;
	}
	printf("%s", sp -> sym_name);
	ind += strlen (sp -> sym_name);
	if (sp -> sym_next){
	    putchar (',');
	    ind ++;
	}
	else
	    putchar (';');
	if (ind > 72) {
	    putchar ('\n');
	    ind = 0;
	}
	else {
	    putchar (' ');
	    ind ++;
	}
    }
    putchar ('\n');

    if (sp = symtab[TBL_IMPORT]) {
	printf ("\nIMPORTS\n");
	p = sp -> sym_module;
	oid = sp -> sym_oid;
    }
    for (ind = 0; sp; sp = sp -> sym_next) {
	if (ind == 0) {
	    putchar ('\t');
	    ind = 8;
	}
	printf ("%s", sp -> sym_name);
	ind += strlen (sp -> sym_name);
	if (sp -> sym_next) {
	    if (strcmp (p, sp -> sym_next -> sym_module) == 0) {
	        putchar (',');
	        ind ++;
	        if ( ind > 72) {
	            putchar ('\n');
	            ind = 0;
	        }
	        else {
	            putchar (' ');
	            ind ++;
	        }
	    }
	    else {
	        if (ind != 8)
		    printf ("\n\t\t");
	        else	putchar ('\t');
	        printf ("FROM %s", p);
	        if (oid)
		    printf (" %s", oidprint (oid));
	        printf ("\n\t");
	        ind = 8;
	        p = sp -> sym_next -> sym_module;
	        oid = sp -> sym_next -> sym_oid;
	    }
	}
	else {
	    if (ind != 8)
	        printf ("\n\t\t");
	    else
		putchar ('\t');
	    printf ("FROM %s", p);
	    if (oid)
	        printf (" %s", oidprint (oid));
	    printf (";\n");
        }
    }
}

check_impexp (yp)
YP	yp;
{
    SYM		sp;

    for (sp = symtab[TBL_EXPORT]; sp; sp = sp->sym_next)
	if (strcmp (sp -> sym_name, yp -> yp_identifier) == 0)
	{
	    yp -> yp_flags |= YP_EXPORTED;
	    break;
	}

    for (sp = symtab[TBL_IMPORT]; sp; sp = sp -> sym_next)
	if (strcmp (sp -> sym_name, yp -> yp_identifier) == 0) {
	    if (yp->yp_flags & YP_EXPORTED)
		myyerror (gettxt(":48", "Warning: %s imported & exported!"), yp->yp_identifier);
	    yp -> yp_module = sp -> sym_module;
	    yp -> yp_modid = sp -> sym_oid;
/*	    yp -> yp_flags |= YP_IMPORTED;	*/
	}
}

importedP (name)
char   *name;
{
    SYM		sp;

    for (sp = symtab[TBL_IMPORT]; sp; sp = sp -> sym_next)
	if (strcmp (sp -> sym_name, name) == 0)
	    return 1;

    return 0;
}

static struct oidtbl {
    char	*oid_name;
    int		oid_value;
} oidtable[] =  { /* Top level OIDS */
    "ccitt",		0,
    "iso",		1,
    "joint-iso-ccitt",	2,

    NULL,
};

initoidtbl ()
{
    struct oidtbl *op;
    OID		oid;

    for (op = oidtable; op -> oid_name; op++) {
	defineoid (op->oid_name, oid = int2oid(op->oid_value));
	oid_free (oid);
    }
}

char *
oidprint (oid)
OID	oid;
{
    static char buf[BUFSIZ];
    char	*cp;
    char	*p;
    OID		o2;
    unsigned int *ip;
    int		i;

    if (oid == NULLOID)
	return "";

    (void) strcpy (buf, "{ ");
    cp = buf + strlen(buf);

    i = oid->oid_nelem;
    ip = oid->oid_elements;

    p = oidname (o2 = int2oid((int)*ip));
    oid_free (o2);
    if (p) {
	i --;
	ip ++;
	(void) sprintf (cp, "%s ", p);
	cp += strlen(cp);
    }

    for (; i > 0; i--) {
	(void) sprintf (cp, "%d ", *ip++);
	cp += strlen (cp);
    }

    (void) strcat (cp, " }");
    return buf;
}

/* oid_free - free an object identifier */
int
oid_free (oid)
register OID oid;
{
    if (oid == NULLOID)
        return;

    if (oid -> oid_elements)
        free ((char *) oid -> oid_elements);

    free ((char *) oid);
}

char *
sprintb (v, bits)
register int    v;
register char  *bits;
{
    register int    i, j;
    register char   c, *bp;
    static char buffer[BUFSIZ];

    (void) sprintf (buffer, bits && *bits == 010 ? "0%o" : "0x%x", v);
    bp = buffer + strlen (buffer);

    if (bits && *++bits) {
        j = 0;
        *bp++ = '<';
        while (i = *bits++)
            if (v & (1 << (i - 1))) {
                if (j++)
                    *bp++ = ',';
                for (; (c = *bits) > 32; bits++)
                    *bp++ = c;
            }
            else
                for (; *bits > 32; bits++)
                    continue;
        *bp++ = '>';
        *bp = '\0';
    }

    return buffer;
}
