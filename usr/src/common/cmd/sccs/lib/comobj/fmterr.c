#ident	"@(#)sccs:lib/comobj/fmterr.c	6.3.1.1"
# include	"../../hdr/defines.h"

void
fmterr(pkt)
register struct packet *pkt;
{
	int	fatal();
	(void) fclose(pkt->p_iop);
	fatal(
		":216:format error at line %d (co4)",pkt->p_slnno);
}
