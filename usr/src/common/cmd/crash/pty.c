#ident	"@(#)crash:common/cmd/crash/pty.c	1.2.1.1"

/*
 * This file contains code for the crash function pty.
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/termios.h>
#include <sys/strtty.h>
#include <sys/ptms.h>
#include <sys/ptem.h>
#include "crash.h"

extern short print_header;	

/* get arguments for tty function */
int
getpty()
{
	int slot;
	int full = 0;
	int all = 0;
	int count;
	int line_disp = 0;
	int ptem_flag = 0;
	int pckt_flag = 0;
	int c;
	char *type = NULL;
	char *giventype;
	long addr;
	long arg2;
	long arg1;

	char *heading = "SLOT   MWQPTR   SWQPTR  PT_BUFP STATE\n";

	optind = 1;
	while( ( c = getopt( argcnt, args, "efhlsw:t:")) != EOF) {
		switch ( c) {
			case 'e':	all = 1;
					break;
			case 'f':	full = 1;
					break;
			case 't':	type = optarg;
					break;
			case 'w':	redirect();
					break;
			case 'l':	line_disp = 1;
					break;
			case 'h':	ptem_flag = 1;
					break;
			case 's':	pckt_flag = 1;
					break;
			default:	longjmp( syn, 0);
		}
	}

	if ((giventype = type)) {
		addr = getaddr(type);
		count = getcount(type);
	}
	else {
		addr = getaddr(type = "ptms");
		count = getcount("pt");
	}

	/*
	 * We needed a default type so we can interpret a slot number;
	 * but if an address is given instead, we don't want to imply
	 * that it's necessarily in that default ptms table.
	 */
	if (giventype || !args[optind])
		fprintf(fp,"%s TABLE BASE = %x, SIZE = %d\n",type,addr,count);
	if ( !full) {
		print_header = 0;
		fprintf( fp, "%s", heading);
	}

	if ( args[optind]) {
		do {
			if (getargs(count, &arg1, &arg2))
			    for (slot = arg1; slot <= arg2; slot++)
				prspty(1, full, slot, addr,
				    line_disp, ptem_flag, pckt_flag, heading);
			else {
				slot = getslot(arg1, addr,
					sizeof(struct pt_ttys), 0, count);
				if (slot != -1)
					arg1 = addr;
				prspty(1, full, slot, arg1,
				    line_disp, ptem_flag, pckt_flag, heading);
			}
		} while (args[++optind]);
	} else
		for (slot = 0; slot < count; slot++)
			prspty(all, full, slot, addr,
			    line_disp, ptem_flag, pckt_flag, heading);
}

/*
 * print streams pseudo tty table
 */
prspty( all, full, slot, addr, line_disp, ptem_flag, pckt_flag, heading)
int all,full,slot;
long addr;
char *heading;
{
	struct pt_ttys	pt_tty;
	struct queue	q;
	int count;
	long base;
	long offset;


	if ( slot == -1)
		readmem( (long)addr, 1, -1, (char *)&pt_tty, sizeof pt_tty,"ptms_tty structure");
	else
		readmem( (long)(addr + slot * sizeof pt_tty), 1, -1,
			(char *)&pt_tty,sizeof pt_tty,"ptms_tty structure");

	/*
	 * A free entry is one that does not have PTLOCK, PTMOPEN and
	 * PTSOPEN flags all set
	 */
	if ( !( pt_tty.pt_state & (PTMOPEN | PTSOPEN | PTLOCK)) && !all)
		return;

	if ( full || print_header) {
		print_header = 0;
		fprintf( fp, "%s", heading);
	}

	if ( slot == -1)
		fprintf( fp, "  - ");
	else
		fprintf( fp, "%4d", slot);

	fprintf( fp, "%9x%9x%9x",	 pt_tty.ptm_wrq,
					 pt_tty.pts_wrq,
					 pt_tty.pt_bufp);

	fprintf(fp,"%s%s%s\n",
		pt_tty.pt_state & PTMOPEN ? " mopen" : "",
		pt_tty.pt_state & PTSOPEN ? " sopen" : "",
		pt_tty.pt_state & PTLOCK ? " lock" : "");

	if ( line_disp || ptem_flag) {
		offset = (long)(pt_tty.pts_wrq) - (long)(sizeof( struct queue));
		readmem( offset, 1, -1, (char *)&q, sizeof( struct queue), "");
		offset = prsptem( full, ptem_flag, q.q_next);
		if ( line_disp && offset && !prsldterm( full, offset))
			print_header = 1;
	}
	if ( pckt_flag) {
		offset = (long)(pt_tty.ptm_wrq) - (long)(sizeof( struct queue));
		readmem( offset, 1, -1, (char *)&q, sizeof( struct queue), "");
		prspckt( q.q_next);
	}
	return;
}

prsptem(full, print, addr)
int full;
int print;
long addr;
{
	char *heading = "\t  RQADDR MODNAME   MODID DACK_PTR   STATE\n";
	char mname[9];	/* Buffer for the module name */

	struct ptem ptem;
	struct queue q;
	struct qinit qinfo;
	struct module_info minfo;


	/*
	 * Wade through the link-lists to extract the line disicpline
	 * name and id
	 */
	readmem( addr, 1, -1, (char *)&q, sizeof( struct queue), "");
	/*
	 * q_next is zero at the stream head, i.e. no modules have
	 *  been pushed
	 */
	if ( !q.q_next)
		return( 0);
	readmem( (long)(q.q_qinfo), 1, -1, (char *)&qinfo, sizeof( struct qinit), "");
	readmem( (long)(qinfo.qi_minfo), 1, -1, (char *)&minfo, sizeof( struct module_info), "");
	readmem( (long)(minfo.mi_idname), 1, -1, mname, 8, "");
	mname[8] = '\0';

	readmem( (long)(q.q_ptr), 1, -1, (char *)&ptem, sizeof( struct ptem), "");

	if ( print) {

		fprintf( fp, "%s", heading);
		fprintf( fp, "\t%x %-9s%6d%9x         ", addr, mname, minfo.mi_idnum, ptem.ptem_dackp);
		fprintf( fp, "%s%s\n",
			 (ptem.ptem_state == OFLOW_CTL	? " oflow"    : ""),
			 (ptem.ptem_state == STRFLOW	? " strflow"    : ""));
	}

	if ( !full)
		return( (long)q.q_next);

	if ( print) {
		fprintf(fp,"\tcflag: %s%s%s%s%s%s%s%s%s%s%s%s%s%s",
			(ptem.ptem_cflags&CBAUD)==B0    ? " b0"    : "",
			(ptem.ptem_cflags&CBAUD)==B50   ? " b50"   : "",
			(ptem.ptem_cflags&CBAUD)==B75   ? " b75"   : "",
			(ptem.ptem_cflags&CBAUD)==B110  ? " b110"  : "",
			(ptem.ptem_cflags&CBAUD)==B134  ? " b134"  : "",
			(ptem.ptem_cflags&CBAUD)==B150  ? " b150"  : "",
			(ptem.ptem_cflags&CBAUD)==B200  ? " b200"  : "",
			(ptem.ptem_cflags&CBAUD)==B300  ? " b300"  : "",
			(ptem.ptem_cflags&CBAUD)==B600  ? " b600"  : "",
			(ptem.ptem_cflags&CBAUD)==B1200 ? " b1200" : "",
			(ptem.ptem_cflags&CBAUD)==B1800 ? " b1800" : "",
			(ptem.ptem_cflags&CBAUD)==B2400 ? " b2400" : "",
			(ptem.ptem_cflags&CBAUD)==B4800 ? " b4800" : "",
			(ptem.ptem_cflags&CBAUD)==B9600 ? " b9600" : "",
			(ptem.ptem_cflags&CBAUD)==B19200 ? " b19200" : "");
		fprintf(fp,"%s%s%s%s%s%s%s%s%s%s\n",
			(ptem.ptem_cflags&CSIZE)==CS5   ? " cs5"   : "",
			(ptem.ptem_cflags&CSIZE)==CS6   ? " cs6"   : "",
			(ptem.ptem_cflags&CSIZE)==CS7   ? " cs7"   : "",
			(ptem.ptem_cflags&CSIZE)==CS8   ? " cs8"   : "",
			(ptem.ptem_cflags&CSTOPB) ? " cstopb" : "",
			(ptem.ptem_cflags&CREAD)  ? " cread"  : "",
			(ptem.ptem_cflags&PARENB) ? " parenb" : "",
			(ptem.ptem_cflags&PARODD) ? " parodd" : "",
			(ptem.ptem_cflags&HUPCL)  ? " hupcl"  : "",
			(ptem.ptem_cflags&CLOCAL) ? " clocal" : "");

		fprintf( fp, "\tNumber of rows: %d\tNumber of columns: %d\n", ptem.ptem_wsz.ws_row, ptem.ptem_wsz.ws_col);
		fprintf( fp, "\tNumber of horizontal pixels: %d\tNumber of vertical pixels: %d\n", ptem.ptem_wsz.ws_xpixel, ptem.ptem_wsz.ws_ypixel);
	}

	return( (long)q.q_next);
}

prspckt( addr)
long addr;
{
	char *heading = "\t  RQADDR MODNAME   MODID\n";
	char mname[9];	/* Buffer for the module name */

	struct queue q;
	struct qinit qinfo;
	struct module_info minfo;


	/*
	 * Wade through the link-lists to extract the line disicpline
	 * name and id
	 */
	readmem( addr, 1, -1, (char *)&q, sizeof( struct queue), "");
	/*
	 * q_next is zero at the stream head, i.e. no modules have
	 *  been pushed
	 */
	if ( !q.q_next)
		return( 0);

	readmem( (long)(q.q_qinfo), 1, -1, (char *)&qinfo, sizeof( struct qinit), "");
	readmem( (long)(qinfo.qi_minfo), 1, -1, (char *)&minfo, sizeof( struct module_info), "");
	readmem( (long)(minfo.mi_idname), 1, -1, mname, 8, "");
	mname[8] = '\0';



	fprintf( fp, "%s", heading);
	fprintf( fp, "\t%x %-9s%6d\n", addr, mname, minfo.mi_idnum);

	return( 1);
}
