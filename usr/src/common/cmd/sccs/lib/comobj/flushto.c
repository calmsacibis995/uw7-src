#ident	"@(#)sccs:lib/comobj/flushto.c	6.3"
# include	"../../hdr/defines.h"

void
flushto(pkt,ch,put)
register struct packet *pkt;
register char ch;
int put;
{
	register char *p;
	void	fmterr(), putline();
	char 	*getline();

	while ((p = getline(pkt)) != NULL && !(*p++ == CTLCHAR && *p == ch))
		pkt->p_wrttn = (char) put;

	if (p == NULL)
		fmterr(pkt);

	putline(pkt,(char *) 0);
}
