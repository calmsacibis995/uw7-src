#ident	"@(#)crash:common/cmd/crash/abuf.c	1.2.1.1"

/*
 * This file contains code for the crash function: abuf.
 */

#include <stdlib.h>
#include <stdio.h>
#include "crash.h"
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mac.h>
#include <audit.h>
#include <sys/stat.h>
#include <sys/privilege.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/stropts.h>
#include <sys/sysmacros.h>
#include <sys/auditrec.h>

#define BSZ  1		/* byte size field */
#define SSZ  2		/* short size field */
#define LSZ  4		/* long size field */

static vaddr_t Abuf;		/* namelist symbol value */
static vaddr_t Alog;		/* namelist symbol value */
static vaddr_t Actl;		/* namelist symbol value */
static vaddr_t Amac;		/* namelist symbol value */
static vaddr_t Anbuf;		/* namelist symbol value */
static vaddr_t Utsnm;		/* namelist symbol value */

static abufctl_t adtbuf;	/* internal audit buffer structure */
static kabuf_t a_buf;		/* current audit buffer structure */
static kabuf_t a_fbuf;		/* free audit buffer structure */
static kabuf_t a_dbuf;		/* dirty audit buffer structure */
static arecbuf_t a_rec;		/* audit record structure */

static alogctl_t adtlog;	/* internal audit log structure */
static actlctl_t adtctl;	/* internal audit control structure */

static int fd = -1;
static int type = LSZ;		/* default abuf type */
static char mode = 'x';		/* default abuf mode */
static char *logfile;		
static kabuf_t *dbufp, *cbufp, *tbufp;
static arecbuf_t *arecp; 
static arecbuf_t *rbuffer;
static kabuf_t *dbuffer;
static kabuf_t *cbuffer;
static void prabuf();		/* write audit buffer in specified format*/
static void prbinary();		/* write audit buffer in binary format*/
static void bytehead();		/* write byte order and header record info */

/*
 * get data in audit buffer 
 */
int
getabuf()
{
	int b, c, flagcnt;
	int nbuf, data;
	int bufusecnt;
	int recusecnt;
	kabuf_t *bufp;

	rbuffer = NULL;
	logfile = NULL;
	b = flagcnt = 0;
	nbuf = data = 0;
	bufusecnt = 0;
	recusecnt = 0;

	Abuf = symfindval("adt_bufctl");
	readmem(Abuf, 1, -1, &adtbuf, sizeof(adtbuf), "adt_bufctl");
	if (adtbuf.a_vhigh == 0) 
		fprintf(fp,"audit buffer mechanism bypassed\n");

	if (!Anbuf)
		Anbuf = symfindval("adt_nbuf");
	readmem(Anbuf, 1, -1, &nbuf, sizeof(int), "adt_nbuf");

	/* Dirty Buffers? */
	if ((dbufp = adtbuf.a_dbufp) != NULL) {
		dbuffer = cr_malloc(sizeof(kabuf_t)*nbuf,"dirty audit buffer");
		tbufp = dbuffer;
		do {
			readmem((vaddr_t)dbufp, 1, -1, (kabuf_t *)tbufp,
				sizeof(kabuf_t), "dirty audit buffer");
			tbufp++;
		} while (dbufp = dbufp->ab_next);
	}
	else
		dbuffer = NULL;

	/* Current Buffer? */
	if ((cbufp = adtbuf.a_bufp) != NULL) {
		cbuffer = readmem((vaddr_t)cbufp, 1, -1, NULL,
			sizeof(kabuf_t), "current audit buffer");
	}
	else
		cbuffer = NULL;

	optind = 1;
	while((c = getopt(argcnt, args, "bcdoxw:")) != EOF) {
		switch(c) {
			/* character format */
                        case 'c' :      mode = 'c';
                                        type = BSZ;
                                        flagcnt++;
                                        break;
			/* decimal format */
                        case 'd' :      mode = 'd';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* octal format */
                        case 'o' :      mode = 'o';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* hexadecimal format */
                        case 'x' :      mode = 'x';
                                        type = LSZ;
                                        flagcnt++;
                                        break;
			/* binary format */
			case 'b' :      mode = 'b';
                                        flagcnt++;
                                        break;
			/* file redirection */
			case 'w' :	logfile = optarg;
                                        break;
                        default  :      longjmp(syn,0);
                                        break;
		}
	}
	if (flagcnt > 1) 
		error("only one mode may be specified: b c d o or x\n");
	if (args[optind])
		longjmp(syn,0);

	if (mode == 'b') {
		if (logfile) {
			bytehead();
			if (tbufp = dbuffer) {
				/* go thru dirty buffer list */
				do {
					prbinary(tbufp->ab_bufp,
						 tbufp->ab_inuse);
					tbufp++;
				} while (tbufp = dbufp->ab_next);
			}
			if (tbufp = cbuffer)
				prbinary(tbufp->ab_bufp, tbufp->ab_inuse);
			
			if ((!active)
			 && (arecp = adtbuf.a_recp) != NULL) {
				do { /* go thru write-thru-record list */
					rbuffer = readmem((vaddr_t)arecp,
						1, -1, rbuffer,
						sizeof(arecbuf_t),
						"write-thru record buffer");
					prbinary(rbuffer->ar_bufp,
						 rbuffer->ar_inuse);
				} while (arecp = rbuffer->ar_next);
			}
		}else
			error("-wlogfile must be given for binary format\n");
	}else {
		if (logfile) {
			optarg = logfile;
			logfile = NULL;
			redirect();
		}
		fprintf(fp,"\tAUDIT BUFFER CONTROL STRUCTURE\n");
		fprintf(fp,"\ta_vhigh  = 0x%x\n",adtbuf.a_vhigh);
		fprintf(fp,"\ta_bsize  = 0x%x\n",adtbuf.a_bsize);
		fprintf(fp,"\ta_mutex  = 0x%x\n",adtbuf.a_mutex);
		fprintf(fp,"\ta_off_sv = 0x%x\n",adtbuf.a_off_sv);
		fprintf(fp,"\ta_buf_sv = 0x%x\n",adtbuf.a_buf_sv);
		fprintf(fp,"\ta_flags  = 0x%x\n",adtbuf.a_flags);
		fprintf(fp,"\ta_addrp  = 0x%x\n",adtbuf.a_addrp);
		fprintf(fp,"\ta_abufp  = 0x%x\n",adtbuf.a_bufp);
		fprintf(fp,"\ta_fbufp  = 0x%x\n",adtbuf.a_fbufp);
		fprintf(fp,"\ta_dbufp  = 0x%x\n",adtbuf.a_dbufp);
		fprintf(fp,"\ta_recp   = 0x%x\n",adtbuf.a_recp);
		fprintf(fp,"\n\tNUMBER OF AUDIT BUFFERS = %d\n\n", nbuf);

		fprintf(fp,"\tBUFFER\t\tADDRESS\t\tINUSE\t\tNEXT\n");
		bufusecnt = b = 0;
		if (tbufp = dbuffer) {
			do {
				b++;
				fprintf(fp, "\t%08d\t%08x\t%08d\t0x%08x\n", b,
					tbufp->ab_bufp, tbufp->ab_inuse,
					tbufp->ab_next);
				bufusecnt += tbufp->ab_inuse;
				tbufp++;
			} while (tbufp = dbufp->ab_next);
		}
		if (tbufp = cbuffer) {
			b++;
			fprintf(fp, "\t%08d\t%08x\t%08d\t0x%08x\n", b,
				tbufp->ab_bufp, tbufp->ab_inuse, tbufp->ab_next);
			bufusecnt += tbufp->ab_inuse;
		}
		if (!b)
			fprintf(fp, "\t0\t\t0x0\t\t0\t\t0x0\n");

		(void)fprintf(fp, "\n\tAudit Buffer Size: %d bytes\n",
			adtbuf.a_bsize);
		(void)fprintf(fp, "\tAmount of Data: %d bytes\n", bufusecnt);
		if (tbufp = dbuffer) {
			do { /* print dirty buffer list */
				prabuf(tbufp->ab_bufp, tbufp->ab_inuse);
				tbufp++;
			} while (tbufp = dbufp->ab_next);
		}
		if (tbufp = cbuffer) /* print current buffer */
			prabuf(tbufp->ab_bufp, tbufp->ab_inuse);
		if ((!active)
		 && (arecp = adtbuf.a_recp) != NULL) {
			do { /* print write-thru record list */
				rbuffer = readmem((vaddr_t)arecp,
					1, -1, rbuffer,
					sizeof(arecbuf_t),
					"write-thru record buffer");
				prabuf(rbuffer->ar_bufp, rbuffer->ar_inuse);
			} while (arecp = rbuffer->ar_next);
		}
	}
	return(0);
}

/*
 * Print or display contents of the audit buffer.
 */
static void
prabuf(addr, size)
long	addr;
int	size;
{
	int	i;
	long	value;
	static	char format[] = "%.*x   ";
	int	precision;

	switch(format[3] = mode) {
	case 'o' :  precision = 11;
	 	    break;
	case 'd' :  precision = 10;
	   	    break;
	case 'x' :  precision = 8;
		    break;
	}

	for (i = 0; i < size; addr += type, i += type) {
		value = 0;
		readmem(addr, 1, -1, &value, type, "audit buffer");
		if ((i % 16) == 0) {
			if (i != 0) 
				(void)fprintf(fp,"\n");
			(void)fprintf(fp, "\t%8.8x:  ", addr);
		}
		if (mode == 'c')
			putch(value);
		else
			raw_fprintf(fp, format, precision, value);
			/* show decimal or octal even in hexmode */
	}
	(void)fprintf(fp, "\n");
}


/*
 * create a file that contains the contents
 * of the audit buffer in binary format.
 */
static void
prbinary(addr, size)
long	addr;
int	size;
{
	char	*buf;

	if ((fd = open(logfile, O_WRONLY | O_APPEND | O_CREAT, 0600)) >= 0) {
		buf = readmem(addr, 1, -1, NULL, size, "audit buffer");
		if (write(fd, buf, size) == -1) {
			close(fd);
			fd = -1;
			error("error writing buffer to file %s\n", logfile);
		}
		cr_free(buf);
	} else
		error("error opening file %s\n", logfile);
	close(fd);
	fd = -1;
}

static void
bytehead()
{
 	char		*ap, *wap, *byordbuf;
	int		size, mac;
 	idrec_t		*id;
	struct utsname	utsname;
	struct stat	statbuf;
	char		sp[]={' '};
	int		ts, ss, rs;

	if (fd != -1) {
		close(fd);
		fd = -1;
	}
	byordbuf = cr_malloc(ADT_BYORDLEN + ADT_VERLEN, "byordbuf");
	ss = ts = rs = 0;
	/* check if the file exists */
	if (stat(logfile, &statbuf) == -1) {
		if ((fd = open(logfile, O_WRONLY | O_CREAT, 0600)) >= 0) {
			/* write out machine byte ordering info */
			strncpy(byordbuf, ADT_BYORD, ADT_BYORDLEN);
		
			if (!Actl)
				Actl = symfindval("adt_ctl");
			readmem(Actl,1,-1,&adtctl,sizeof(adtctl),"adt_ctl");
			strncpy(byordbuf + ADT_BYORDLEN, adtctl.a_version, 
				ADT_VERLEN);

			if ((write(fd, byordbuf, sizeof(char)*ADT_BYORDLEN + 
				ADT_VERLEN)) != (sizeof(char)*ADT_BYORDLEN +
				ADT_VERLEN)) {
					close(fd);
					error("error writing buffer to file %s\n",logfile);
			}

			cr_free(byordbuf);
		        size = sizeof(idrec_t) + sizeof(struct utsname);
			ap = cr_malloc(size, "ap");
			(void)memset(ap, '\0', size);
		
			/* write out audit log header record info */
			wap = ap;
			id = (idrec_t *)wap;
			id->cmn.c_rtype = id->cmn.c_event = FILEID_R;
		
			if (!Alog)
				Alog = symfindval("adt_logctl");
			readmem(Alog,1,-1,&adtlog,sizeof(adtlog),"adt_logctl");
			id->cmn.c_seqnum = adtlog.a_seqnum;
			id->cmn.c_crseqnum = FILEID_R;
			strncpy(id->spec.i_mmp, adtlog.a_mmp, ADT_DATESZ);
			strncpy(id->spec.i_ddp, adtlog.a_ddp, ADT_DATESZ);
		
			id->cmn.c_pid = 0;
			id->cmn.c_time.tv_sec = 0;
			id->cmn.c_time.tv_nsec = 0;
			id->cmn.c_status = 0;
		
			if (!Amac)
				Amac = symfindval("mac_installed");
			id->spec.i_flags = ADT_ON;
			readmem(Amac,1,-1,&mac,sizeof(mac),"mac_installed");
			if (mac)
				id->spec.i_flags |= ADT_MAC_INSTALLED;
			wap += sizeof(idrec_t);
		
			if (!Utsnm)
				Utsnm = symfindval("utsname");
			readmem(Utsnm,1,-1,&utsname,sizeof(utsname),"utsname");
			ss = strlen(utsname.sysname);
			strcpy(wap,utsname.sysname);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.nodename);
			strcpy(wap, utsname.nodename);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.release);
			strcpy(wap,utsname.release);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.version);
			strcpy(wap, utsname.version);
			wap += ss;
			*wap = *sp;
			wap++;
			ts += ss + 1;

			ss = strlen(utsname.machine);
			strcpy(wap, utsname.machine);
			wap += ss;
			*wap = '\0';
			wap++;
			ts += ss + 1;

			rs = ROUND2WORD(ts);
			id->cmn.c_size = (sizeof(idrec_t) + rs);	
			if (write(fd, ap, id->cmn.c_size) != id->cmn.c_size) {
				close(fd);
				error("error writing buffer to file %s\n",logfile);
			}
			cr_free(ap);
		} else 
			error("error opening file %s\n",logfile);
	} else 
		error("file %s already exists, try another!\n", logfile);
}
