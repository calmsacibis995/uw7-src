#ident	"@(#)sccs:lib/comobj/chksid.c	6.3.1.1"
# include	"../../hdr/defines.h"

void
chksid(p,sp)
char *p;
register struct sid *sp;
{
	int fatal();
	if (*p ||
		(sp->s_rel == 0 && sp->s_lev) ||
		(sp->s_lev == 0 && sp->s_br) ||
		(sp->s_br == 0 && sp->s_seq))
			(void) fatal(":103:invalid sid (co8)");
}
