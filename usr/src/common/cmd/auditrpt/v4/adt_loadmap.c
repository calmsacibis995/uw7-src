/*		copyright	"%c%" 	*/

#ident	"@(#)adt_loadmap.c	1.3"
#ident  "$Header$"

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/privilege.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <mac.h>
#include <string.h>
#include <pfmt.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include "auditrptv4.h"

#define  MFILE  "/auditmap"

/**
 ** external variables - used in adt_print.c
 **/
	int		nsets=0;		/* number of file-based privilege sets */
	setdef_t	*setdef=(setdef_t *)0;	/* privilege mechanism info */
/**
 ** external variables - defined in auditrptv4.c
 **/
extern 	int 	s_user, 
		s_grp, 
		s_class, 
		s_type, 
		s_priv, 
		s_scall;

extern	ids_t 	*uidbegin, 
		*gidbegin,
		*typebegin, 
		*privbegin,
		*scallbegin;

extern	cls_t	*clsbegin;

extern	char	*mach_info;
extern  env_info_t	env;

/**
 ** local variables and functions 
 **/
static int 	error = 0;

static char 	*loadmap();

int
adt_loadmap(mapdir)
char *mapdir;
{
	FILE 	*fp;
	int 	msize;
	char	*mapfile;
	uint 	tot, datasize, size;
	int  	i;
	char	*cbp, *hold;
	char	*begin;
	char 	tz[MAXNUM];
	setdef_t	*sd;
	char	c_nsets[MAXNUM],
		sd_mask[MAXNUM],
		sd_setcnt[MAXNUM];
	int 	tot_size, type, ct;
	char	c;

	s_user=s_grp=s_class=s_type=s_priv=s_scall=type=0;
	uidbegin=gidbegin=typebegin=privbegin=scallbegin=NULL; 
	/* allocate space for map file pathname */
	msize=strlen(mapdir)+strlen(MFILE)+1;
	if ((mapfile = ((char *)malloc(msize))) ==  (char *)NULL) {
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		return(ADT_MALLOC);
	}
	/*get the full pathname of auditmap file */
	(void)strcpy(mapfile,mapdir);
	(void)strcat(mapfile,MFILE);

	if ((fp=fopen(mapfile,"r")) == NULL){
		(void)pfmt(stdout,MM_WARNING,RW_NO_MAP,mapfile);
		free(mapfile);
		return(0);
	}
	free(mapfile);
	tot_size=0;
	/* get the record type and size */
	(void)fscanf(fp,"%d%c%d%c",&type,&c,&tot_size,&c);

	if (type != A_TBL){
		(void)pfmt(stderr,MM_ERROR,E_MAP_RTYPE,type);
		return(ADT_BADMAP);
	}
	if ((hold=cbp=(char *)malloc(tot_size)) == NULL){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		return(ADT_MALLOC);
	}
	if (fread(cbp,tot_size,1,fp) != 1){	/* cbp == beginning of data */
		(void)pfmt(stderr,MM_ERROR,E_FMERR);
		free(hold);
		return(ADT_FMERR);
	}

	tot=0;
	while  ( tot < tot_size ) {

		(void)sscanf(cbp,"%d%c%d%c",&type,&c,&datasize,&c);
		/*To get pass map type and size*/
		ct = strcspn(cbp," ") + 1;
		cbp += ct;
		tot += ct;
		ct = strcspn(cbp," ") + 1;
		cbp += ct;
		tot += ct;
		switch (type) {
			case A_FILEID:
			case FILEID_R:  		 
				begin=cbp;
				(void)strncpy(env.version, begin, ADT_VERLEN);
				begin+=ADT_VERLEN;
				(void)strcpy(tz,begin);
				env.gmtsecoff=atol(tz);
				begin+= strlen(tz) + 1;

				(void)strcpy(c_nsets,begin);
				nsets=atoi(c_nsets);
				begin+=strlen(c_nsets) + 1;

				if (nsets > 0){
					if ((setdef=(setdef_t *)malloc(nsets*sizeof(setdef_t)))==NULL){
						(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
						error=ADT_MALLOC;
						tot = tot_size;
						break;
					}
				}
				for (i=0, sd=setdef; i < nsets; ++i, ++sd){

					(void)strcpy(sd_mask,begin);
					sd->sd_mask=atol(sd_mask);
					begin+=strlen(sd_mask) + 1;
				
					(void)strcpy(sd_setcnt,begin);
					sd->sd_setcnt=atoi(sd_setcnt);
					begin+=strlen(sd_setcnt) + 1;
				
					(void)strcpy(sd->sd_name,begin);
					begin+=strlen(sd->sd_name) + 1;
				}
				size=strlen(begin) + 1;

				if((mach_info=(char *)calloc(size,sizeof(char))) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					error=ADT_MALLOC;
					tot = tot_size;
				}
				else{
					(void)strncpy(mach_info,begin,size);
				}
				break;
			case A_GIDMAP:  		 
				gidbegin=(ids_t *)loadmap(type,cbp,&s_grp);
				break;
			case A_IDMAP:  		 
				uidbegin=(ids_t *)loadmap(type,cbp,&s_user);
				break;
			case A_TYPEMAP:  		 
				typebegin=(ids_t *)loadmap(type,cbp,&s_type);
				break;
			case A_PRIVMAP:  		 
				privbegin=(ids_t *)loadmap(type,cbp,&s_priv);
				break;
			case A_SYSMAP:  		 
				scallbegin=(ids_t *)loadmap(type,cbp,&s_scall);
				break;
			case A_CLASSMAP:  		 
				clsbegin=(cls_t *)loadmap(type,cbp,&s_class);
				break;
			default:
				(void)pfmt(stderr,MM_ERROR,E_MAP_RTYPE,type);
				error=ADT_BADMAP;
				tot=tot_size;
				break;
			}
		tot+=datasize;
		cbp+=datasize;
	}
	free(hold);
	return(error);
}

char *
loadmap(type, buf, number)
int	type;
char 	*buf;
int	*number;
{
	char	*sp,*recp;
	int	i,buflen;
	ids_t 	*mp, *beginp;
	ent_t	*cur;
	cls_t 	*first,*last;
	static 	int isize=sizeof(ids_t);

	i=buflen=0; 
	mp=beginp=(ids_t *)NULL;
	switch(type) {
		case A_IDMAP  :
		case A_GIDMAP :
		case A_PRIVMAP:
		case A_TYPEMAP:
		case A_SYSMAP :
			/* don't try to read if map data is empty */
			if (*buf != NULL){
				/* copy input buffer because strtok modifies it */
				buflen=strlen(buf);
				if ((recp=(char * )malloc(buflen + 1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					error=ADT_MALLOC;
					return(NULL);
				}
				(void)strncpy(recp,buf,buflen + 1);
				/* count the number of records */
				sp = recp;
				while (strtok(recp,":") != NULL){
					recp=NULL;
					i++;
				}
				free(sp);
				/*	allocate enough structures		*/
				if ((beginp=malloc((i+1) * isize)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					error=ADT_MALLOC;
					return(NULL);
				}
				mp=beginp;
				recp=buf;
				/* load structures with data from input buffer 	*/
				while ((sp=(char *)strtok(recp,":")) != NULL){
					(void)sscanf(sp,"%s%d",mp->name,&mp->id);
					mp++;
					recp=NULL;
				}
			}
			if (i)
				*number=i;
			else 
				*number=0;
			return((char*)beginp);
		case A_CLASSMAP:
			/*	copy input buffer because strtok modifies  */
			first=last=NULL;
			cur=NULL;
			buflen=strlen(buf);
			if ((recp=(char * )malloc(buflen + 1)) == NULL){
				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
				error=ADT_MALLOC;
				return(NULL);
			}
			(void)strncpy(recp,buf,buflen + 1);
			/* count the number of records */
			sp = recp;
			while (strtok(recp,":") != NULL){
				recp=NULL;
				i++;
			}
			free(sp);
			recp=buf;
			while ((sp=strtok(recp,":")) != NULL){
				if (first == NULL){
					if ((first=malloc(sizeof(cls_t)))==NULL){
						(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
						error=ADT_MALLOC;
						return(NULL);
					}
					(void)sscanf(sp,"%s",first->alias);
					sp+=strlen(first->alias)+1;
					last=first;
				}
				else{
					last->next=malloc(sizeof(cls_t));
					if (last->next == NULL){
						(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
						error=ADT_MALLOC;
						return(NULL);
					}
					(void)sscanf(sp,"%s",last->next->alias);
					sp+=strlen(last->next->alias)+1;
					last=last->next;
				}
				if((last->tp=malloc(sizeof(ent_t)))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					error=ADT_MALLOC;
					return(NULL);
				}
				cur=last->tp;
				while (sscanf(sp,"%s",cur->type) != EOF){
					sp+=strlen(cur->type)+1;
					cur->tp=malloc(sizeof(ent_t));
					if (cur->tp == NULL){
						(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
						error=ADT_MALLOC;
						return(NULL);
					}
					cur=cur->tp;
				}
				cur->tp=NULL;
				recp=NULL;
			}
			if (i)
				*number=i;
			else 
				*number=0;
			return((char*)first);
		default:
			return(NULL);
			/*NOTREACHED*/
			break;
	}
}
