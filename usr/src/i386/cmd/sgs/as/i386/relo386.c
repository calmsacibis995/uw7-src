#ident	"@(#)nas:i386/relo386.c	1.6"
/* relo386.c */

/* Support for i386 relocation entries. */


#include <sys/elf_386.h>
#include <unistd.h>
#include "common/as.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/relo.h"
#include "common/sect.h"
#include "common/stmt.h"
#include "common/syms.h"
#include "relo386.h"
#include "stmt386.h"

#include "intemu.h"

/* Generate code for relocatable expression.  Called only from
** common code.
*/

void
#ifdef __STDC__
relocexpr(Eval *vp, const Code *cp, Section *secp)
#else
relocexpr(vp, cp, secp) Eval *vp; Code *cp; Section *secp;
#endif
{
    Uchar *p;
    static const char MSGsmall[] = "relocatable expression needs 4 bytes";
    static const char MSGrange[] = "relocatable expression out of range: %s";

    if ((p = secp->sec_data) == 0)
	fatal(gettxt(":958","relocexpr():called in size context"));

    /* A relocatable expression can only be recorded in a 4 (or
    ** bigger) byte quantity.
    */
    if (cp->code_size < 4) {
	exprerror(cp->data.code_expr, gettxt(":956",MSGsmall));
    }
    else if (vp->ev_nsbit > cp->code_size * CHAR_BIT) {
	exprwarn(cp->data.code_expr, gettxt(":957",MSGrange), num_tohex(vp->ev_int));
    }
    else {
	relocaddr(vp, p += cp->code_addr, secp);
	gen_value(vp, cp->code_size, p);	/* output data bits */
    }
    return;
}


/* Do all the grotty bookkeeping to store relocation information
** for expression vp at location p in secp.  Use relocation type
** "type".
*/

static void
#ifdef __STDC__
dorelo(Eval *vp, Uchar *p, Section *secp, int type)
#else
dorelo(vp, p, secp, type) Eval *vp; Uchar *p; Section *secp; int type;
#endif
{
    Section *rel;

    if (secp->sec_data == 0)
	fatal(gettxt(":959","dorelo():no section data"));

    /* Create a relocation section associated with secp
    ** if none exists.  Make it relocatable without an
    ** explicit addend, because the addend is always present
    ** in the data or text section.
    */
    if ((rel = secp->sec_relo) == 0)
	rel = relosect(secp, SecTy_Reloc);

    /* Ignore temporary symbols, because they never end up
    ** in the object file symbol table.  The relocation will
    ** thus become section-based, rather than symbol-based.
    */
    if (vp->ev_sym != 0 && vp->ev_sym->sym_index == 0)
	vp->ev_sym = 0;
    
    /* Now write relocation info, either section- or symbol-based.
    ** If possible, base relocations on a symbol.  Have to calculate
    ** offset in section for relocated value.  For symbol-based, have
    ** to calculate symbol-based offset.
    */
    if (vp->ev_sym == 0)
	(void) sectrelsec(rel, type, p - secp->sec_data, vp->ev_sec);
    else {
	(void) sectrelsym(rel, type, p - secp->sec_data, vp->ev_sym);
	/* Current value represents
	**	symbol + <offset in section>
	** Subtract symbol's offset in section to give
	**	symbol + <offset from symbol>
	*/
	if (vp->ev_dot)
	    subeval(vp, vp->ev_dot->code_addr);
    }
    return;
}


/* Generate relocation for value vp stored at address p in
** section secp.
*/

void
#ifdef __STDC__
relocaddr(Eval *vp, Uchar *p, Section *secp)
#else
relocaddr(vp, p, secp) Eval *vp; Uchar *p; Section *secp;
#endif
{
    int type;

    /* Determine relocation type.
    ** Correct application of the PIC modifiers is assured by the
    ** parsing and common expression code.  That is, an expression
    ** can only have a modifier applied to an identifier, and there
    ** can only be one such.
    */
    if ((vp->ev_flags & EV_RELOC) == 0)
	fatal(gettxt(":960","relocaddr():not relocatable"));

    switch (vp->ev_pic) {
    case 0: /* no modifier */
	if (vp->ev_flags & EV_G_O_T)
	    type = R_386_GOTPC;
	else
	    type = R_386_32;
	break;
    case ExpOp_Pic_PLT:
	type = R_386_PLT32; 
	break;
    case ExpOp_Pic_GOT:
	type = R_386_GOT32;
	break;
    case ExpOp_Pic_GOTOFF:
	type = R_386_GOTOFF; 
	break;
    case ExpOp_Pic_BASEOFF:
	type = R_386_BASEOFF; 
	break;
    default:
	fatal(gettxt(":961","relocaddr():bad PIC modifier %#x"), vp->ev_pic);
    }
    dorelo(vp, p, secp, type);
    return;
}


void
#ifdef __STDC__
relocpcrel(Eval *vp, Uchar *p, Section *secp)
#else
relocpcrel(vp, p, secp) Eval *vp; Uchar *p; Section *secp;
#endif
{
    int type;

    /* Determine relocation type.
    ** Correct application of the PIC modifiers is assured by the
    ** parsing and the common expression code.  That is, an
    ** expression can only have a modifier applied to an identifier,
    ** and there can only be one such.  Note that this code also
    ** supports vp's being an absolute number!
    */

    switch( vp->ev_pic ){
    case 0:			type = R_386_PC32;  break; /* no modifier */
    case ExpOp_Pic_PLT:		type = R_386_PLT32; break;
    case ExpOp_Pic_GOT:		type = R_386_GOT32; break;
    case ExpOp_Pic_GOTOFF:	type = R_386_GOTOFF; break;
    case ExpOp_Pic_BASEOFF:	type = R_386_BASEOFF; break;
    default:
	fatal(gettxt(":962","relocpcrel():bad PIC modifier %#x"), vp->ev_pic);
    }
    dorelo(vp, p, secp, type);
    return;
}
