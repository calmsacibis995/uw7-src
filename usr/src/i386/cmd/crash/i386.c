#ident	"@(#)crash:i386/cmd/crash/i386.c	1.2.1.7"

/*
 * This file contains code for the crash functions: idt, ldt, gdt
 */

#include <sys/param.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/immu.h>
#include <sys/tss.h>
#include <sys/seg.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/reg.h>
#include <sys/vmparam.h>
#include <sys/plocal.h>
#include "crash.h"

#define HEAD1 "SLOT     BASE/SEL LIM/OFF  TYPE       DPL  ACCESSBITS\n"
#define HEAD2 "SLOT     SELECTOR OFFSET   TYPE       DPL  ACCESSBITS\n"

extern	struct user *ubp;

/* get arguments for ldt function */
int
getldt()
{
	int all = 0;
	int c;
	int first = 0;
	int last;
	struct desctab_info di;

	if (!getublock(Cur_proc, Cur_lwp, B_FALSE))
		return;
	readmem((vaddr_t)ubp->u_dt_infop[DT_LDT], 1, -1,
		&di, sizeof di, "desctab info for ldt");
	last = di.di_size - 1;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		first = strcon(args[optind],'d');
		if((first < 0) || (first > last))
			error("entry %d is out of range 0-%d\n",first,last);
		last=first;
		optind++;
	}
	if(args[optind]) {
		last = strcon(args[optind],'d');
		last=first+last-1;
		if (last>=di.di_size) last=di.di_size-1;
		if (args[++optind])
			longjmp(syn,0);
	}
	if (first==last) all=1;
	prdt("LDT",&di,all,first,last);
}

/* get arguments for idt function */
int
getidt()
{
	int all = 0;
	int c;
	int first=0;
	int last=IDTSZ-1;

	optind = 1;
	while ((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		first = strcon(args[optind],'d');
		if ((first < 0) || (first >= IDTSZ))
			error("entry %d is out of range 0-%d\n",first,IDTSZ-1);
		last=first;
		optind++;
	}
	if(args[optind]) {
		last = strcon(args[optind],'d');
		last=first+last-1;
		if (last>=IDTSZ) last=IDTSZ-1;
		if (args[++optind])
			longjmp(syn,0);
	}
	if (first==last) all=1;
	pridt(all,first,last);
}

/* print interrupt descriptor table */
int
pridt(all,first,last)
int all;
int first,last;
{
	int i;
	struct gate_desc *idtp;
	struct gate_desc idt[IDTSZ];
	static vaddr_t Plocal;

	if (!Plocal)
		Plocal = symfindval("l");
	readmem((vaddr_t)&((struct plocal *)Plocal)->idtp,1,-1,
		&idtp,sizeof idtp,"IDT pointer");
	readmem((vaddr_t)idtp,1,-1,idt,sizeof idt,"IDT");
	fprintf(fp,"iAPX386 IDT for engine %d\n", Cur_eng);
	fprintf(fp,HEAD2);
	for (i=first;i<=last;i++)
		prdescr(i,(struct segment_desc *)&idt[i],all);
}

/* get arguments for gdt function */
int
getgdt()
{
	int all = 0;
	int c;
	int first=0;
	int last;
	struct desctab_info di;

	if (!getublock(Cur_proc, Cur_lwp, B_FALSE))
		return;
	readmem((vaddr_t)ubp->u_dt_infop[DT_GDT], 1, -1,
		&di, sizeof di, "desctab info for gdt");
	last = di.di_size - 1;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		first = strcon(args[optind],'d');
		if((first < 0) || (first > last))
			error("entry %d is out of range 0-%d\n",first,last);
		last=first;
		optind++;
	}
	if(args[optind]) {
		last = strcon(args[optind],'d');
		last=first+last-1;
		if (last>=di.di_size) last=di.di_size-1;
		if (args[++optind])
			longjmp(syn,0);
	}
	if (first==last) all=1;
	prdt("GDT",&di,all,first,last);
}

/* print global or local descriptor table */
int
prdt(tabname,dip,all,first,last)
char *tabname;
struct desctab_info *dip;
int all;
int first,last;
{
	struct segment_desc dtent;
	int i;

	if (Cur_proc == -1)
		fprintf(fp,"iAPX386 %s for idle context of engine %d\n",
			tabname, Cur_eng);
	else
		fprintf(fp,"iAPX386 %s for process %d lwp %d\n",
			tabname, Cur_proc, Cur_lwp);
	fprintf(fp,HEAD1);
	for (i=first;i<=last;i++) {
		readmem((vaddr_t)(dip->di_table + i), 1, Cur_proc,
			&dtent, sizeof dtent, tabname);
		prdescr(i,&dtent,all);
	}
}

prdescr(i,t,all)
int i;
struct segment_desc *t;
int all;
{
	int	selec=0;	/* true if selector, false if base */
	int	gran4;		/* true if granularity = 4K */
	char	acess[100];	/* Description of Accessbytes */
	long	base;		/* Base or Selector */
	long	offset;		/* Offset or Limit */
	char	*typ;		/* Type */
	char	*format;
	struct gate_desc *gd = (struct gate_desc *)t;

	if (!(all | (SD_GET_ACC1(t) & 0x80))) return(0);

	base = SD_GET_BASE(t);
	offset = SD_GET_LIMIT(t);
	gran4 = SD_GET_ACC2(t) & 0x8;

	format = hexmode? "  %x ": "  %d ";
	sprintf(acess,format,(SD_GET_ACC1(t) >> 5) & 3);
	format = hexmode? "%s CNT=%x": "%s CNT=%d";

	if (!(SD_GET_ACC1(t) & 0x10)) {
	/* Segment Descriptor */
		selec=1;
		switch (SD_GET_ACC1(t)&0xf) {
		case 0:	typ="SYS 0 ?  "; selec=0; break;
		case 3:
		case 1:	typ="TSS286   "; selec=0; break;
		case 2: typ="LDT      "; selec=0; break;
		case 4: typ="CGATE    ";
			sprintf(acess,format,acess,gd->gd_arg_count);
			break;
		case 5: typ="TASK GATE";
			sprintf(acess,format,acess,gd->gd_arg_count);
			break;
		case 6: typ="IGATE286 ";
			sprintf(acess,format,acess,gd->gd_arg_count);
			break;
		case 7: typ="TGATE286 ";
			sprintf(acess,format,acess,gd->gd_arg_count);
			break;
		case 9:
		case 11:typ="TSS386   "; selec=0; break;
		case 12:typ="CGATE386 ";
gate386:
			offset = gd->gd_offset_low + (gd->gd_offset_high << 16);
			gran4=0;
			sprintf(acess,format,acess,gd->gd_arg_count);
			break;
		case 14:typ="IGATE386 ";
			goto gate386;
		case 15:typ="TGATE386 ";
			goto gate386;
		default:typ="SYS???   ";
		}
	} else if (SD_GET_ACC1(t) & 0x8) {
	/* executable Segment */
		typ="XSEG     ";
		sprintf(acess,"%s%s%s%s%s",acess,
			SD_GET_ACC1(t) & 1 ? " ACCS'D":"",
			SD_GET_ACC1(t) & 2 ? " R&X":" XONLY",
			SD_GET_ACC1(t) & 4 ? " CONF":"",
			SD_GET_ACC2(t) & 4 ? " DFLT":"");
	} else {
	/* Data Segment */
		typ="DSEG     ";
		sprintf(acess,"%s%s%s%s%s",acess,
			SD_GET_ACC1(t) & 1 ? " ACCS'D":"",
			SD_GET_ACC1(t) & 2 ? " R&W":" RONLY",
			SD_GET_ACC1(t) & 4 ? " EXDOWN":"",
			SD_GET_ACC2(t) & 4 ? " BIG ":"");
	}

	fprintf(fp,"%4d     ",i);
	if (selec) fprintf(fp,"    %04x",base&0xffff);
	else fprintf(fp,"%08x",base);
	if (gran4) fprintf(fp," %08x ",offset<<12 );
	else fprintf(fp," %08x ",offset);
	fprintf(fp,"%s  %s%s%s%s\n",typ,acess,gran4 ? " G4096":"",
			SD_GET_ACC2(t) & 1 ? " AVL":"",
			SD_GET_ACC1(t) & 0x80 ? "":"** nonpres **");
}


/* Test command */
int
gettest()
{
	int p1;
	char c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		p1 = strcon(args[optind++],'d');
		if (args[optind])
			longjmp(syn,0);
		debugmode=p1;
	}
	fprintf(fp,"Debug Mode %d\n",debugmode);
}

/* get arguments for panic function */
int
getpanic()
{
	char c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind])
		longjmp(syn,0);
	prpanic();
}

int
prpanic()
{
	extern	struct panic_data panic_data;
	static	vaddr_t Putbuf, Putbufsz, Putbufwpos;
	kcontext_t kc;
	char	*putbuf;
	int	putbufsz;
	int	putbufwpos;
	int	engid;
	vaddr_t kublock;

	if (!Putbufwpos) {
		Putbuf = symfindval("putbuf");
		Putbufsz = symfindval("putbufsz");
		Putbufwpos = symfindval("putbufwpos");
	}

	/*
	 * Try malloc() before cr_malloc(): we don't want
	 * to give up on printing panic data just because
	 * putbufsz got corrupted with a ridiculous value
	 */
	readmem(Putbufsz,1,-1,&putbufsz,sizeof(int),"putbufsz");
	if ((putbuf = malloc(putbufsz+1)) == NULL)
		prerrmes("cannot malloc(%u) for putbuf\n", putbufsz+1);
	else {
		register char *cp, *ce;

		free(putbuf);
		putbuf = cr_malloc(putbufsz+1, "putbuf");
		readmem(Putbufwpos,1,-1,
			&putbufwpos,sizeof(int),"putbufwpos");
		if (putbufwpos < 0 || putbufwpos >= putbufsz)
			putbufwpos = 0;
		readmem(Putbuf+putbufwpos,1,-1,
			putbuf,putbufsz-putbufwpos,"putbuf");
		readmem(Putbuf,1,-1,
			putbuf+putbufsz-putbufwpos,putbufwpos,"putbuf");
		putbuf[putbufsz] = '\0';
		fprintf(fp,"SYSTEM MESSAGES:\n");
		ce = putbuf + putbufsz;
		for(cp = putbuf; cp < ce; cp++)	/* omit truncated line */
			if(*cp == '\n')
				break;
		while (cp < ce) {
			if (*cp == '\n') {
				while (*cp == '\n' || *cp == '\0')
					if (++cp >= ce)
						break;
				if (cp >= ce)
					break;
				fprintf(fp,"\n");
			}
			if(isprint(*cp) || isspace(*cp))
				fprintf(fp,"%c",*cp);
			else if (*cp)
				fprintf(fp,"[0x%x]",*cp&0xff);
			++cp;
		}
		fprintf(fp,"\n\n");
	}

	if( panic_data.pd_rp ) {
		readmem((vaddr_t)panic_data.pd_rp,1,-1,&kc, 
			sizeof(kcontext_t), "panic_kcontext");
		fprintf(fp,"PANIC REGISTERS:\n");
		printregs((vaddr_t)panic_data.pd_rp, NULL, &kc);
		if ((engid = eng_to_id(panic_data.pd_engine)) != -1) {
			get_context(engid);
			if (kublock = getublock(Cur_proc, Cur_lwp, B_FALSE))
				prtrace(Cur_proc, Cur_lwp, kublock, UNKNOWN);
			pr_context();
		}
	} else
		fprintf(fp,"no panic data\n");
}
