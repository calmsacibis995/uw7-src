#ident	"@(#)sccs:lib/comobj/sinit.c	6.5.1.1"
# include	"../../hdr/defines.h"

/*
	Does initialization for sccs files and packet.
*/

void
sinit(pkt,file,openflag)
register struct packet *pkt;
register char *file;
{
	extern	char	*satoi();
	register char *p;
	FILE *fdfopen();
	char *getline();
	unsigned	strlen();
	int	fatal(), imatch(), xopen(), fstat();
	void	fmterr();

	zero((char *)pkt, sizeof(*pkt));
	if (size(file) > FILESIZE)
		fatal(":224:too long (co7)");
	if (!sccsfile(file))
		fatal(":225:not an SCCS file (co1)");
	copy(file,pkt->p_file);
	pkt->p_wrttn = 1;
	pkt->do_chksum = 1;	/* turn on checksum check for getline */
	if (openflag) {
		pkt->p_iop = xfopen(file,0);
		setbuf(pkt->p_iop,pkt->p_buf);
		fstat((int)fileno(pkt->p_iop),&Statbuf);
		if (Statbuf.st_nlink > 1)
			fatal(":226:more than one link (co3)");
		if ((p = getline(pkt)) == NULL || *p++ != CTLCHAR || *p++ != HEAD) {
			(void) fclose(pkt->p_iop);
			fmterr(pkt);
		}
		p = satoi(p,&pkt->p_ihash);
		if (*p != '\n')
			fmterr(pkt);
	}
	pkt->p_chash = 0;
}
