/*		copyright	"%c%" 	*/

#ident	"@(#)adt_optparse.c	1.3"
#ident  "$Header$"

/**
 ** This file contains option parsing functions.
 **/

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <sys/vnode.h>
#include <sys/privilege.h>
#include <sys/param.h>
#include <sys/proc.h> 
#include <sys/systm.h> 
#include <mac.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include <pfmt.h>
#include "auditrptv4.h"

#define ALLPRIV		"allprivs"
#define ALL		"all"
#define L_SIZE 		256
#define MAXLEV 		16
#define MAXCAT 		64
#define LABNAMES_F 	"/etc/labnames"

extern long		altzone;
/**
 ** external variables -- defined in auditrptv4.c
 **/
extern unsigned int 	uidtab[];	
extern char 		*usertab[];
extern int 		uidnum;	
extern int 		usernum;
extern unsigned int 	ipctab[];	
extern char 		*objtab[];
extern int 		ipcnum;	
extern int 		objnum;	
extern int 		s_priv;	
extern ids_t		*privbegin;

/**
 ** external functions 
 **/
extern int	adt_lvlin();		/* from adt_lvlin.c */

/** 
 ** local functions
 **/
static int 	name_conv(), 
		satoi();

/**
 ** Parse and validate -u option-arguments.
 **/
int
parse_uid(sp)
char *sp;
{
	register char *p;
	int n;

	while (sp) {
		if (p = strchr(sp,','))
			*p++ ='\0';
		if (isalnum(sp[0]) == 0){ /*first char an alphanumeric? */
			(void)pfmt(stderr,MM_ERROR,E_INV_A,"-u");
			return(-1);
		}
		if ((n=satoi(sp))<0) { /* nonnumeric uid : must be login name */
			if (usernum < MXUID)
				usertab[usernum++] = sp;
			else{
				(void)pfmt(stderr,MM_ERROR,E_TOOLONG,"-u");
				return(-1);
			}
		} 
		else {
			if (uidnum < MXUID)
				uidtab[uidnum++] = n;
			else{
				(void)pfmt(stderr,MM_ERROR,E_TOOLONG,"-u");
                                return(-1);
                        }

		}
		sp = p;
	}
	return(0);
}

/**
 ** Parse and validate -f option-arguments.
 ** An option argument may be an IPC object id or an absolute pathname
 **/
int
parse_obj(sp)
char *sp;
{
	register char *p;
	int n;

	while (sp) {
		if (p = strchr(sp,','))
			*p++ = '\0';
		if (*sp == '/') {
			if (objnum < MAXOBJ){
				if ((objtab[objnum]=(char *)malloc(strlen(sp)+1))==NULL) {
					(void)pfmt(stderr,MM_ERROR,E_BDMALLOC,sp);
					return(ADT_MALLOC);
				}
				(void)strcpy(objtab[objnum++],sp);
			}
			else{
				(void)pfmt(stderr,MM_ERROR,E_TOOLONG,"-f");
				return(ADT_BADSYN);
			}
		}
		else {
			/*Check if option argument is an IPC object id or a*/
			/*dynamic loadable module id.                       */
			if ((n=satoi(sp)) >= 0) {
				if (ipcnum < MAXOBJ) 
					ipctab[ipcnum++] = n;
				else{
					(void)pfmt(stderr,MM_ERROR,E_TOOLONG,"-f");
					return(ADT_BADSYN);
				}
			} 
			else {
				(void)pfmt(stderr,MM_ERROR,RE_BAD_NAME,sp);
				return(ADT_BADSYN);
			}
		}
		sp = p;
	} /* end of while loop */
	return (0);
}

/**
 ** Parse and validate -t option-arguments.
 **/
int
parse_objt(sp,type_maskp)
char *sp;
int  *type_maskp;
{
	register char *p;

	while (sp) {
		if (p = strchr(sp,','))
			*p++ ='\0';

	/*The size of a valid object type is 1 char*/ 
	if (strlen(sp) != 1) {
		(void)pfmt(stderr,MM_ERROR,RE_BAD_OTYPE,sp);
		return(-1);
	}

	switch (*sp) {
		case 'f':
			*type_maskp |= REG;
			break;
		case 'c': 
			*type_maskp |= CHAR;
			break;
		case 'b':
			*type_maskp |= BLOCK;
			break;
		case 'd':
			*type_maskp |= DIR;
			break;
		case 'p':
			*type_maskp |= PIPE;
			break;
		case 's':
			*type_maskp |= SEMA;
			break;
		case 'h':
			*type_maskp |= SHMEM;
			break;
		case 'm':
			*type_maskp |= MSG;
			break;
		case 'l':
			*type_maskp |= LINK;
			break;
		default:
			(void)pfmt(stderr,MM_ERROR,RE_BAD_OTYPE,sp);
			return(-1);
		} /* switch */
		sp = p;
	} /* while */

	return (0);
}

/**
 ** Parse and validate -p option-arguments.
 **/
int
parse_priv(sp)
char *sp;
{
	register char *p;
	int pnum;
	pvec_t 	vecs=0;

	if (strcmp(sp,ALL) == 0) {
		if ((pnum=name_conv(ALLPRIV,s_priv,privbegin)) == -1) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_PRIV,sp);
			return(-1);
		}
		pm_setbits(pnum,vecs);
		return(vecs);
	}

	/*Loop through the user entered list of privileges.             */
	/*If the keyword "all" was intermixed with individual privileges*/
	/*warn the user and continue. An invalid privilege is an error  */
	/*inform the user and terminate processing.                     */
	while (sp) {
		if (p = strchr(sp,','))
			*p++ ='\0';

		if (strcmp(sp,ALL) == 0) {
			(void)pfmt(stdout,MM_WARNING,RW_KEYWORD,ALL);
			sp =p;
			continue;
		}

		if ((pnum=name_conv(sp,s_priv,privbegin)) == -1) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_PRIV,sp);
			return(-1);
		}
		pm_setbits(pnum,vecs);
		sp = p;
	}
	return(vecs);
}

/**
 ** Parse and validate -l option-argument.
 **/
int
parse_lvl(sp)
char *sp;
{
	register char *p;
	level_t lvlno;
	int 	num=0;

	while (sp) {
		if (++num > 1) {
		 	(void)pfmt(stderr,MM_ERROR,E_TOOMANY);
			usage();
			return(-1);
		}	
		if (p=strchr(sp,','))
			*p++ ='\0';
		if ((adt_lvlin(sp,&lvlno))== -1) {
			(void)pfmt(stderr,MM_ERROR,E_BAD_LVL);
			return(-1);
		}
		sp = p;
	}
	return(lvlno);
}

/**
 ** Parse and validate -r option-argument.
 **/
int
parse_range(sp,lvlmin,lvlmax)
char *sp;
level_t *lvlmin, *lvlmax;
{
	register char *p;
	level_t lvlno;

	/*A user may only specify one range*/
	if (strchr(sp,',') != NULL){
		(void)pfmt(stderr,MM_ERROR,E_TOOMANY);
		usage();
		return(-1);
	}
	
	/*The '-' is the deliminator between the minimum level and maximum level*/
	if ( (p=strchr(sp,'-')) == NULL){
		usage();
		return(-1);
	}
	else {
		*p++ ='\0';
		/*The user did not specify a maximum level*/
		if (*p == NULL){
			usage();
			return(-1);
		}
	}
	
	/*Translate the minimum level from text format to internal format*/
	if (adt_lvlin(sp,&lvlno) == -1){
		/* invalid levelmin specified */
		(void)pfmt(stderr,MM_ERROR,E_BAD_MIN);
		return(-1);
	}

	*lvlmin=lvlno;

	/*Translate the maximum level from text format to internal format*/
	if (adt_lvlin(p,&lvlno) == -1){
		/* invalid levelmax specified */
		(void)pfmt(stderr,MM_ERROR,E_BAD_MAX);
		return(-1);
	}

	*lvlmax=lvlno;

	if ((adt_lvldom(*lvlmax,*lvlmin)) > 0)
		return(0);
	else  {
		(void)pfmt(stderr,MM_ERROR,RE_BAD_RANGE);
		return(-1);
	}	
}

/**
 ** Parse and validate -s/-h option-arguments.
 ** Convert date given as [mmdd]MMHH[[cc]yy] to a long integer.
 ** This routine was adapted from the date(1) command (routine setdate).
 **/
parse_hour(stp,limitp)
char *stp;
long *limitp;
{
#	define year_size(A)	(((A) % 4) ? 365 : 366)
	static 	short month_size[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	register int k=0;
	int hh, min, dd, mm, yy;
	int minidx = 6;
	long clck, ct;
	struct tm *ctp;		/* current time struct */
	char *str;

	hh=min=dd=mm=yy=0;

	/*Validate that string is composed of decimal-digit characters*/
	str=stp;
	while (*str) {
		if (!isdigit(*str)) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_TIME2);
			return(-1);
		}
		str++;
	}

	(void)time(&ct);	/* get current time */
	ctp=localtime(&ct);	/* put it in the structure */


	/** 
 	 ** Convert a date of format [mmdd]HHMM[[cc]yy] to an 
	 ** integer representing seconds since 1970
	 **/ 
	switch(strlen(stp)) {
	case 12:
		/* User entered: mmddMMHHccyy*/
		yy = atoi(&stp[8]);
		stp[8] = '\0';
		break;
	case 10:
		/* User entered: mmddMMHHyy*/
		yy = 1900 + atoi(&stp[8]);
		stp[8] = '\0';
		break;
	case 8:
		/* User entered: mmddMMHH*/
		yy = 1900 + ctp->tm_year;
		break;
	case 4:
		/* User entered: MMHH*/
		yy = 1900 + ctp->tm_year;
		mm = ctp->tm_mon + 1;
		dd = ctp->tm_mday;
		minidx = 2;
		break;
	default:
		(void)pfmt(stderr,MM_ERROR,RE_BAD_TIME2);
		return(-1);
	}

	/*Extract minutes*/
	min = atoi(&stp[minidx]);
	stp[minidx] = '\0';

	/*Extract hours*/
	hh = atoi(&stp[minidx-2]);
	stp[minidx-2] = '\0';

	if (!dd) {
		/*if dd is 0, extract the day and month from the*/
		/*value supplied by the user.                   */
		dd = atoi(&stp[2]);
		stp[2] = '\0';
		mm = atoi(&stp[0]);
	}

	/*Consistant with date (1M)*/
	if(hh == 24)
	{
		hh = 0;
		dd++;
	}

	/*  Validate date elements  */
	if(!((mm >= 1 && mm <= 12) && (hh >= 0 && hh <= 23) && 
		(min >= 0 && min <= 59) && (yy > 1970))) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_TIME2);
			return(-1);
	}
	if ((mm == 2 ) && (year_size(yy) == 366)) {
		if (dd > 29) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_TIME2);
			return(-1);
		}
	}
	else {
		if (dd > month_size[mm -1]) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_TIME2);
			return(-1);
		}
	}
			

 	/*Calculate, in days, the number of years between the*/
	/*requested year (yy) and 1970.                      */
	for(clck=0,k=1970; k<yy; k++) {
		clck += year_size(k);
	}

	/*  Adjust for leap year  */
	if (year_size(yy) == 366 && mm >= 3)
		clck += 1;

	/*Calculate the number of days between Jan.1 and the*/
	/*the requested month (mm).                         */
	while(--mm)
		clck += month_size[mm - 1];

	/*Calculate the days, hour and minutes*/
	clck += (dd - 1);
	clck *= 24;
	clck += hh;
	clck *= 60;
	clck += min;
	clck *= 60;

	/** 
	 **convert to GMT assuming standard time 
	 ** correction is made in localtime(3C)
	 **/
	clck += timezone;

	/* correct if daylight savings time in effect */
	if (localtime(&clck)->tm_isdst)
		clck = clck - (timezone - altzone); 

	*limitp = clck;	/* s_time or h_time */
	return(0);
}

/** 
 ** local utility routines
 **/

/**
 ** Strict atoi conversion : return a negative number if non-digits in string
 **/
int
satoi(s)
char *s;
{
	register int n;
	for (n=0; *s; s++) {
		if (*s < '0' || *s > '9')
			return(-1);
		n = 10*n + (*s - '0');
	}
	return(n);
}

/**
 ** Convert name to id, using the auditmap referenced by tblp(beginning of map)
 ** and s_tbl(size of map).
 **/
int
name_conv(namep, s_tbl, tblp)
char *namep;
int s_tbl;
ids_t *tblp;
{
	int	id;
	int 	 j;
   
	for (j=0; j <= s_tbl; j++, tblp++) {
		if (strcmp(namep,tblp->name) == 0){
			id=tblp->id;
			return(id);
		}
	}
	return(-1);
}
