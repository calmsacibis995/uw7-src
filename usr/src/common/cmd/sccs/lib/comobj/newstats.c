#ident	"@(#)sccs:lib/comobj/newstats.c	6.4"
# include	"../../hdr/defines.h"

void
newstats(pkt,strp,ch)
register struct packet *pkt;
register char *strp;
register char *ch;
{
	char fivech[6];
	register char *r;
	int i;
	void	putline();

	r = fivech;
	for (i=0; i < 5; i++)
		*r++ = *ch;
	*r = '\0';
	sprintf(strp,"%c%c %s/%s/%s\n",CTLCHAR,STATS,fivech,fivech,fivech);
	putline(pkt,strp);
}
