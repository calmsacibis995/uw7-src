#ident	"@(#)ktool:i386at/ktool/scodb/dump.c	1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include	<stdio.h>
#include	<unistd.h>
#include	<sys/scodb/stunv.h>

extern int nstuns;
struct cstun *cs;
char **stnames;

/*************************************************************
*
*	structure of output file:
*
*	int nstun		number of structures
*	struct cstun[nstun]
*	cstun1 name
*	cstel 1a
*	cstel 1b
*	...
*	cstel1a name
*	cstel1b name
*	...
*	cstun2 name
*	cstel 2a
*	cstel 2b
*	...
*	cstel2a name
*	cstel2b name
*	...
*	...
*	...
*	cstun(nstun-1) name
*	cstel (nstun-1)a
*	cstel (nstun-1)b
*	...
*	cstel(nstun-1)a name
*	cstel(nstun-1)b name
*	...
*
*	name offsets are relative to the beginning of the entire file
*
*	Returns value is the number of bytes written.
*/

int
d_cstuns(fp)
	FILE *fp;
{
	long offset;	/* name offset */
	register int i;
	register struct stun *s;
	register struct stel *e;
	extern struct stun *stunlist;
	char *malloc();
	off_t start_off, end_off;

	start_off = ftell(fp);
	fwrite(&nstuns, sizeof nstuns, 1, fp);
	cs = (struct cstun *)malloc(nstuns * sizeof(*cs));
	stnames = (char **)malloc(nstuns * sizeof(*stnames));

	/*
	*	can't quite write the cstuns out yet
	*/
	fseek(fp, nstuns * sizeof(*cs), 1);

	/*
	*	where the cstun names, cstels and cstel names start
	*/
	offset = sizeof(int) + nstuns*sizeof(struct cstun);

	/* must get the names... */
	for (i = 0, s = stunlist;s;i++, s = s->st_next)
		stnames[i] = s->st_name;

	for (i = 0, s = stunlist;s;i++, s = s->st_next) {
		cs[i].cs_flags	= s->st_flags;
		cs[i].cs_size	= s->st_size;
		cs[i].cs_nmel	= s->st_nmel;
		cs[i].cs_nameo	= offset;
			fputs(s->st_name, fp);
			fputc('\0', fp);
			offset += strlen(s->st_name) + 1;
		cs[i].cs_offset = offset;
			offset += s->st_nmel * sizeof(struct cstel);
		for (e = s->st_stels;e;e = e->sl_next) {
			e->sl_nameo = offset;
			offset += strlen(e->sl_name) + 1;
			e->sl_index = e->sl_tag[0] ? sindex(e->sl_tag) : -1;
			fwrite(&e->sl_cstel, CSTEL_SZ, 1, fp);
		}
		for (e = s->st_stels;e;e = e->sl_next) {
			fputs(e->sl_name, fp);
			fputc('\0', fp);
		}
	}

	end_off = ftell(fp);

	/*
	*	write out cstuns
	*/
	fseek(fp, start_off + sizeof(nstuns), 0);
	fwrite(cs, sizeof(*cs), nstuns, fp);
	fseek(fp, end_off, 0);

	return(end_off - start_off);
}

sindex(nam)
	char *nam;
{
	register int i = 0;

	for (i = 0;i < nstuns;i++)
		if (!strcmp(nam, stnames[i]))
			return i;
	return -1;	/* ? */
}

/*************************************************************
*
*	structure of output file:
*
*	int nvari		number of variables
*	cvari 1
*	cvari 2
*	cvari 3
*	...
*	string table
*	...
*
*	name offsets are relative to the beginning of the entire file
*
*	Return value is the number of bytes written.
*/

int
d_cvaris(fp)
	FILE *fp;
{
	long nmo;	/* name offset */
	register struct vari *v;
	off_t start_off, end_off;
	extern struct vari *varilist;
	extern int nvaris;
	extern int nstuns;
	
	start_off = ftell(fp);
	fwrite(&nvaris, sizeof nvaris, 1, fp);
	nmo = sizeof(int) + nvaris*sizeof(struct cvari);
	for (v = varilist;v;v = v->va_next) {
		v->va_index = v->va_tag[0] ? sindex(v->va_tag) : -1;
		v->va_nameo = nmo;
		fwrite(&v->va_cvari, CVARI_SZ, 1, fp);
		nmo += strlen(v->va_name) + 1;
	}
	/* dump out the string table */
	for (v = varilist;v;v = v->va_next) {
		fputs(v->va_name, fp);
		fputc('\0', fp);
	}

	end_off = ftell(fp);

	return(end_off - start_off);
}
