/*		copyright	"%c%" 	*/

#ident	"@(#)auditmap.c	1.6"

/***************************************************************************
 * Command: auditmap
 * Inheritable Privileges: P_DACREAD,P_MACWRITE,P_SETPLEVEL,P_AUDIT
 *       Fixed Privileges: None
 * Notes: Creates the following files              
 *
 * Files:       /var/audit/auditmap/auditmap               
 *              /var/audit/auditmap/ltf.cat         
 *              /var/audit/auditmap/ltf.alias      
 *              /var/audit/auditmap/ltf.class     
 *              /var/audit/auditmap/lid.internal 
 *                                              
 *
 ***************************************************************************/


/*LINTLIBRARY*/
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <mac.h>
#include <sys/utsname.h>
#include <sys/vnode.h>
#include <sys/privilege.h>
#include <sys/secsys.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include <ia.h>
#include <grp.h>
#include <locale.h>
#include <pfmt.h>
#include <stdlib.h>
#include <ctype.h>
#include "auditmap.h"

extern struct passwd	*getpwent();
extern struct group	*getgrent();
extern int  ia_openinfo();
extern void ia_closeinfo(), ia_get_uid();
extern void  adumprec(), ch_name(), cr_ltdb();
extern int  lck_file();
extern char *argvtostr(), privname(), scallnam();

static void qcopy(), adt_itoa(), cr_map(),
            get_filedata(), getgmtsecoff();
static char *next();
static int calc_hdr(), sizemap();
static setdef_t	*init_sdefs();
extern FILE *fopen();

extern time_t timezone, altzone;
extern int daylight;
extern int num_syscall;

char	*mapdir;
char 	*argvp;
static time_t	gmtsecoff;
static char tz_p[MAXNUM];
struct stat statbuf;
static struct utsname unstr;
static FILE 	*cfp = NULL;
static	char	c_nsets[MAXNUM];
static 	int	nsets = 0;
static	setdef_t	*setdef = (setdef_t *)0; 

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: None
                 auditevt(2): None
                 pfmt: None
                 lvlproc(2): None
                 lvlin: None
                 getopt: None
                 stat(2): None
                 sprintf: None
                 open(2): None
                 fcntl(2): None
                 write(2): None
                 unlink(2): None
                 chown(2): None
*/

main(argc,argv)
int argc;
char **argv;
{
	extern int optind;	/*getopt()*/
	extern char *optarg;	/*getopt()*/
	int	c;		/*getopt()*/
	char 	*buf,*bp, *mapfile;
	int	fd,msize,buf_siz,size_map;
	int     Fsz,Isz,Gsz,Csz,Tsz,Psz,Ssz;
	level_t mylvl, audlvl;
	char file_hdr[MAXNUM];
	struct flock alck;
	short mac=0;

	/*Initialize locale information*/
	(void)setlocale(LC_ALL,"");

	/*Initialize message label*/
	(void)setlabel("UX:auditmap");

	/*Initialize catalog*/
	(void)setcat("uxaudit");

        /* make process EXEMPT */
	if (auditevt(ANAUDIT, NULL, sizeof(aevt_t)) == -1) {
         	if (errno == ENOPKG) {
                 	(void)pfmt(stderr, MM_ERROR, NOPKG);
                        exit(ADT_NOPKG);
		}
		else
			if (errno == EPERM) {
                 		(void)pfmt(stderr, MM_ERROR, NOPERM);
                        	exit(ADT_NOPERM);
			}
	}
	

        /* Get the current level of this process */
	if (lvlproc(MAC_GET, &mylvl) == 0) {
		if (lvlin(SYS_AUDIT, &audlvl) == -1) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlin", errno);
			exit(ADT_LVLOPER);
		}
		if (lvlequal(&audlvl, &mylvl)  == 0){
			/* SET level if not SYS_AUDIT */
                        if (lvlproc(MAC_SET, &audlvl) == -1) {
				if (errno == EPERM) {
                 			(void)pfmt(stderr, MM_ERROR, NOPERM);
                        		exit(ADT_NOPERM);
				}
				(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
				exit(ADT_LVLOPER);
			}
		}
		mac=1;
	}else
             	if (errno != ENOPKG) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
			exit(ADT_LVLOPER);
		}
          
	/* save command line arguments */
	if (( argvp = (char *)argvtostr(argv)) == NULL) {
                (void)pfmt(stderr,MM_ERROR,MSG_ARGV);
		adumprec(ADT_MALLOC,strlen(argv[0]),argv[0]);
                exit(ADT_MALLOC);
        }

	mapdir=ADT_MAPDIR;

	/*Process command line*/
	while ((c=getopt(argc,argv,"m:"))!=EOF) {
		switch (c) {
			case 'm':
				mapdir=optarg;
				break;
			case '?':
                		(void)pfmt(stderr,MM_ACTION,MSG_USAGE);
				adumprec(ADT_BADSYN,strlen(argvp),argvp);
				exit(ADT_BADSYN);
		}
	}

	/*The command line is invalid if it contained an arguement*/
	if (optind < argc)
	{
                (void)pfmt(stderr,MM_ACTION,MSG_USAGE);
		adumprec(ADT_BADSYN,strlen(argvp),argvp);
		exit(ADT_BADSYN);
	}

	/*Create audit map files at 660*/
	(void)umask(~0660);

	/*Audit map files are to have the same owner and group */
	/*as the default (/var/audit/auditmap/) directory. This*/  
	/*is true if the default directory is used or the -m   */
	/*option is used.                                      */
	if (stat(ADT_MAPDIR,&statbuf) == -1)
	{
               (void)pfmt(stdout,MM_WARNING,MSG_BADSTAT);
	       statbuf.st_uid=0;
	       statbuf.st_gid=8;
	}

	/*If mac is installed create the ltf.cat, ltf.alias, ltf.class*/ 
        /*and lid.internal files                                      */
	if (mac)
	{
 		cr_ltdb(CAT_FILE,O_CATFILE,CAT,LVLSZ);
 		cr_ltdb(ALIAS_FILE,O_ALIASFILE,ALIASF,LVLSZ);
 		cr_ltdb(CLASS_FILE,O_CLASSFILE,CLSF,LVLSZ);
 		cr_ltdb(LID_FILE,O_LIDFILE,LIDF,LVL_STRUCT_SIZE);
	}

	/*allocate space for the pathname of the map file*/
	msize=strlen(mapdir)+strlen(MAP_FILE)+1;
	if ((mapfile = ((char *)malloc(msize))) ==  NULL) {
                (void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		adumprec(ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	/*concat the full pathname of auditmap file*/
	(void)strcpy(mapfile,mapdir);
	(void)strcat(mapfile,MAP_FILE);
	size_map=0;
	buf=NULL;

	/*Each individual map is represented as:         */
	/*	MAP HEADER:                              */
	/*		map identifier (ascii string)    */
	/*		blank character                  */
	/*		map size (byte count of MAP DATA)*/
	/*                       (ascii string)          */
	/*		blank character                  */
	/*	MAP DATA:                                */
	/*		data (ascii)                     */
	/*		null byte - at end of data       */

	/*For each map calculate the MAP DATA size*/
	Fsz= sizemap(FILEID_R,mapfile);
	Isz= sizemap(A_IDMAP,mapfile);
	Gsz= sizemap(A_GIDMAP,mapfile);
	Csz= sizemap(A_CLASSMAP,mapfile);
	Tsz= sizemap(A_TYPEMAP,mapfile);
	Psz= sizemap(A_PRIVMAP,mapfile);
	Ssz= sizemap(A_SYSMAP,mapfile);

	size_map= Fsz + Isz + Gsz + Csz + Tsz + Psz + Ssz;

	/*For each map calculate the MAP HEADER size*/
	size_map +=calc_hdr(Fsz,FILEID_R);
	size_map +=calc_hdr(Isz,A_IDMAP);
	size_map +=calc_hdr(Gsz,A_GIDMAP);
	size_map +=calc_hdr(Csz,A_CLASSMAP);
	size_map +=calc_hdr(Tsz,A_TYPEMAP);
	size_map +=calc_hdr(Psz,A_PRIVMAP);
	size_map +=calc_hdr(Ssz,A_SYSMAP);

	/*FILE HEADER:                                         */
	/*	FILE ID: A_TBL (ascii string)                  */
	/*	blank character                                */
	/*      FILE SIZE: size of all MAP HEADERS and MAP DATA*/
	/*                 (ascii string)                      */
	/*	blank character                                */
	buf_siz= size_map + (calc_hdr(size_map,A_TBL));

	if ((buf=(char * )malloc(buf_siz)) == NULL){
                (void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		adumprec(ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}
	(void)memset(buf,'\0',buf_siz);
	bp=buf;

	/*Convert A_TBL and size to ascii*/
	(void)memset(file_hdr,'\0',MAXNUM);
	adt_itoa((unsigned long)A_TBL,file_hdr);
	bp += sprintf(bp,"%s ",file_hdr);
	(void)memset(file_hdr,'\0',MAXNUM);
	adt_itoa((unsigned long)size_map,file_hdr);
	bp += sprintf(bp,"%s ",file_hdr);

	/*Create FILEID map*/
	get_filedata(&bp,Fsz);

	/*Create UID Map*/
	cr_map(&bp,A_IDMAP,Isz);

	/*Create GID Map*/
	cr_map(&bp,A_GIDMAP,Gsz);

	/*Create CLASS Map*/
	cr_map(&bp, A_CLASSMAP,Csz);

	/*Create TYPE Map*/
	cr_map(&bp,A_TYPEMAP,Tsz);

	/*Create PRIVILEGE Map*/
	cr_map(&bp,A_PRIVMAP,Psz);

	/*Create SYSCALL map*/
	cr_map(&bp,A_SYSMAP,Ssz);


	/*If the auditmap file already exists - rename it to oauditmap*/
	if ((fd=open(mapfile,O_WRONLY)) >0 )
	{
		/*Establish a write lock on the entire file*/
		alck.l_type=F_WRLCK;
		alck.l_whence=0;  /*start at beginning of file*/
		alck.l_start=0L;  /*relative offset*/
		alck.l_len=0L;    /*until EOF*/

		if (lck_file(fd,mapfile,&alck) == 0)
		{
			/*Rename the existing auditmap file to oauditmap*/
			ch_name(MAP_FILE,O_MAPFILE);
			(void)fcntl(fd,F_UNLCK,&alck);
			(void)close(fd);
		}
		else
		{
			/*The auditmap file is locked by another process*/
			(void)close(fd);
               		(void)pfmt(stdout,MM_WARNING,MSG_NO_MAPFILE);
			adumprec(ADT_SUCCESS,strlen(argvp),argvp); 
			exit(ADT_SUCCESS);
		}
	}
	else
	{
		/*It is not an error if there is no existing auditmap file*/
		/*ENOENT:O_CREAT not set and the file doesn't exist       */
		if (errno != ENOENT)
		{
			if (errno == EACCES) {
				(void)pfmt(stderr, MM_ERROR, NOPERM);
				adumprec(ADT_NOPERM,strlen(argvp),argvp);
                        	exit(ADT_NOPERM);
			}
       	       		(void)pfmt(stderr,MM_ERROR,MSG_NO_WRITE,mapfile);
			adumprec(ADT_FMERR,strlen(argvp),argvp);
			exit(ADT_FMERR);
		}
	}

	/*Write the new auditmap file*/
	if ((fd=open(mapfile,O_WRONLY|O_CREAT, 0660))> 0) 
	{
		/*Establish a write lock on the entire file*/
		alck.l_type=F_WRLCK;
		alck.l_whence=0;  /*start at beginning of file*/
		alck.l_start=0L;  /*relative offset*/
		alck.l_len=0L;    /*until EOF*/

		if (lck_file(fd,mapfile,&alck) == 0)
		{
			/*File is locked*/
			if (write(fd,buf,buf_siz) != buf_siz) {
               			(void)pfmt(stdout,MM_WARNING,MSG_NO_MAPFILE);
				(void)fcntl(fd,F_UNLCK,&alck);
				(void)close(fd);
				(void)unlink(mapfile);
			}
			else
			{
				/*Successful write*/
				(void)fcntl(fd,F_UNLCK,&alck);
				(void)close(fd);
				if (chown(mapfile,statbuf.st_uid,statbuf.st_gid) == -1) {
               				(void)pfmt(stdout,MM_WARNING,MSG_NO_MAPFILE);
					(void)unlink(mapfile);
				}
			}
		}
		else {
			(void)close(fd);
               		(void)pfmt(stdout,MM_WARNING,MSG_NO_MAPFILE);
		}
	}
	else {
		if (errno == EACCES){
			(void)pfmt(stderr, MM_ERROR, NOPERM);
			adumprec(ADT_NOPERM,strlen(argvp),argvp);
                        exit(ADT_NOPERM);
		}
		if (errno == ENOENT)
                	(void)pfmt(stderr,MM_ERROR,MSG_NO_DIR,mapfile);
		else
                	(void)pfmt(stderr,MM_ERROR,MSG_NO_WRITE,mapfile);
		adumprec(ADT_FMERR,strlen(argvp),argvp);
		exit(ADT_FMERR);
	}
	
	adumprec(ADT_SUCCESS,strlen(argvp),argvp); 
	exit(ADT_SUCCESS);

/*NOTREACHED*/
}
void
qcopy(from, to, count)
	register char *from;
	register char *to;
	register uint count;
{

	while (count--)
		*to++ = *from++;
}
/*
 * Procedure:     get_filedata
 *
 * Restrictions:
                 auditctl(2): None
*/
void
get_filedata(bufp,length)
char **bufp;
int length;
{
	actl_t 	actl;		/* auditctl(2) structure */
	char hdr[MAXNUM];
	char	sd_mask[MAXNUM],
		sd_setcnt[MAXNUM],
		sd_name[PRVNAMSIZ];
	int	i;
	setdef_t *sd = setdef;

	/* get current status of auditing */
	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == -1) {
                (void)pfmt(stderr,MM_ERROR,MSG_AUDITCTL);
		adumprec(ADT_BADSTAT,strlen(argvp),argvp);
		exit(ADT_BADSTAT);
	}

	(void)memset(hdr,'\0',MAXNUM);
	adt_itoa((unsigned long)FILEID_R,hdr);
	*bufp += sprintf(*bufp,"%s ",hdr);

	(void)memset(hdr,'\0',MAXNUM);
	adt_itoa((unsigned long)length,hdr);
	*bufp += sprintf(*bufp,"%s ",hdr);

	qcopy(&actl.version[0],*bufp,ADT_VERLEN);
	*bufp +=ADT_VERLEN;

	*bufp +=sprintf(*bufp,"%s%c%s%c",tz_p,'\0',c_nsets,'\0');

	for (i=0; i < nsets; ++i, ++sd){
		if (sd->sd_objtype == PS_FILE_OTYPE){
			(void)memset(sd_mask,'0',MAXNUM);
			(void)memset(sd_setcnt,'0',MAXNUM);
			adt_itoa((unsigned long)sd->sd_mask,sd_mask);
			adt_itoa((unsigned long)sd->sd_setcnt,sd_setcnt);
			(void)strcpy(sd_name,sd->sd_name);
			*bufp += sprintf(*bufp,"%s%c%s%c%s%c", sd_mask,'\0',
				sd_setcnt,'\0',sd_name,'\0');
		}
	}

	*bufp += sprintf(*bufp,"%s %s %s %s %s%c", unstr.sysname,unstr.nodename,
		unstr.release,unstr.version,unstr.machine,'\0');

}

/*
 * Procedure:     cr_map
 *
 * Restrictions:
                 getpwent: None
                 ia_openinfo: None
                 setpwent: None
                 getgrent: None
                 setgrent: None
                 fgets: None
                 fclose: None
*/
void
cr_map(bp,type,length)
char **bp;
int	type;
int	length;
{
	uint	i;
	char 	*p, *name, *events, *class;
	int	number;
	char	white[]={" "};
	char	ALIAS[]={"alias"};
	char	buf[BUFSIZ];
	char 	line[BUFSIZ+1];
	uinfo_t uinfo;
	uid_t	uid;
	struct passwd	*passwd_p;
	gid_t	gid;
	struct group	*group_p;
	short first=1;
	char hdr[MAXNUM];

	(void)memset(hdr,'\0',MAXNUM);
	adt_itoa((unsigned long)type,hdr);
	*bp += sprintf(*bp,"%s ",hdr);

	(void)memset(hdr,'\0',MAXNUM);
	adt_itoa((unsigned long)length,hdr);
	*bp += sprintf(*bp,"%s ",hdr);

	switch(type) {
	case A_IDMAP :
		name=&buf[0];
		while((passwd_p=getpwent()) != NULL){
			(void)strcpy(name,passwd_p->pw_name);
			if(!ia_openinfo(name,&uinfo) && uinfo != NULL){
				ia_get_uid(uinfo,&uid);
				*bp +=(sprintf(*bp,"%s %ld:",name,uid));
				ia_closeinfo(uinfo);
			}
		}	
		setpwent();
		break;
	case A_GIDMAP :
		while((group_p=getgrent()) != NULL){
			name=group_p->gr_name;
			gid=group_p->gr_gid;
			*bp +=(sprintf(*bp,"%s %ld:",name,gid));
		}	
		setgrent();
		break;
	case A_CLASSMAP:
		while ((p = fgets(line, BUFSIZ,cfp)) != NULL) {
			while (*p != NULL) {
				if (isspace(*p)) 
					p++;
				else {
					if (isalpha(*p) ){
					  if (strncmp(p,ALIAS,5) == 0) {
						if (!first)
		  					*bp +=(sprintf(*bp,":"));
						p =(char *) next(p,white);
						class=p;
						p =(char *) next(p,white);
						*bp +=(sprintf(*bp,"%s ",class));
						first=0;
					  }
					  else{
						events=p;
						p =(char *) next(p,white);
				  		*bp +=(sprintf(*bp,"%s ",events));
					  }
					}
					else
						p++;
				}
			}
			(void)memset(line,' ',BUFSIZ);
		}
		*bp +=(sprintf(*bp,":"));
		(void)fclose(cfp);
		break;
	case A_TYPEMAP:
		for ( i=1; i<=ADT_ZNUMOFEVTS; i++){
			*bp +=(sprintf(*bp,"%s %d:",adtevt[i].a_evtnamp,
				adtevt[i].a_evtnum));
		}
		break;
	case A_PRIVMAP:
		name = &buf[0];
		for ( number=0; number<NPRIVS; number++ )
			if (privname(name,number) != NULL)
				*bp += (sprintf(*bp,"%s %d:",name,number));
		if (privname(name,P_ALLPRIVS) != NULL)
			*bp += (sprintf(*bp,"%s %d:",name,P_ALLPRIVS));
		break;
	case A_SYSMAP:
		for ( number=1; number<=num_syscall; number++ )
		{
			if ( scallnam(buf,number) != NULL )
				*bp += (sprintf(*bp,"%s %d:",buf,number));
		}
		break;
	default:
		break;
	}

	/*End each map with a null byte. This is needed when auditmap*/
	/*is parsed by auditrpt().                                   */
	*bp += sprintf(*bp,"%c",'\0');
	**bp = '\0';	/* put in explicitly a NULL which sprintf will
			   have already written */
	*bp++;		/* stop NULL being overwritten on next call */
}

/*
 * Procedure:     sizemap
 *
 * Restrictions:
                 getpwent: None
                 ia_openinfo: None
                 setpwent: None
                 getgrent: None
                 setgrent: None
                 fopen: None
                 fgets: None
                 rewind: None
*/
/* Notes:                                     */
/*The size of each individual map is calculated*/
/*The size is a byte count of the data only.   */
/*This does not include the map header.        */
/***********************************************/
int 
sizemap(type, mapfile)
int 	type;
char	*mapfile;
{
	char	*p,*name,*class,*events;
	char	line[BUFSIZ];
	char	ALIAS[]={"alias"};
	char	white[]={" "};
	char 	namebuf[BUFSIZ];
	char	num[MAXNUM];
	int	length, i, number;
	uinfo_t uinfo;
	uid_t	uid;
	struct passwd	*passwd_p;
	gid_t	gid;
	struct group	*group_p;
	setdef_t	*sd;
	char	sd_mask[MAXNUM],
		sd_setcnt[MAXNUM],
		sd_name[PRVNAMSIZ];
	int	f_nsets=0;

	length=0;

	switch(type) {
	case FILEID_R :
		/*Contains: ADT_VERSION,tz\0,nsets\0,
			(the following line of data nsets times)
			sd_mask\0, sd_setcnt\0,sd_name\0,
			sysname ,nodename ,release ,version ,machine*/

		(void)uname(&unstr);
		getgmtsecoff();
		(void)memset(tz_p,'0',MAXNUM);
		if(gmtsecoff < 0)
			adt_itoa((unsigned long)gmtsecoff,tz_p);
		else
	   	{
			tz_p[0]='-';
			adt_itoa((unsigned long)-gmtsecoff,&tz_p[1]);
	   	}
		/* add size of the audit version and time zone */
		length += (ADT_VERLEN + strlen(tz_p) + 1);

		/* get privilege mechanism info */
		if ((nsets = secsys(ES_PRVSETCNT, 0)) >= 0){
			if((setdef = init_sdefs(nsets,mapfile)) == NULL)
				nsets = -1;
		}

		/* determine what sets to carry in map - only the file-based */
		if (nsets >= 0)
			f_nsets=nsets;
		else
			f_nsets=0;
		sd=setdef;
		for (i=0; i<nsets; ++i, ++sd){
			if (sd->sd_objtype != PS_FILE_OTYPE)
				f_nsets--;
		}
		(void)memset(c_nsets,'0',MAXNUM);
		adt_itoa((unsigned long)f_nsets,c_nsets);

		/* add size of number of sets  to carry - only file-based */
		length += strlen(c_nsets) + 1;

		/* add size of privilege mechanism information */
		sd=setdef;
		for (i=0; i < nsets; ++i, ++sd){
			if (sd->sd_objtype == PS_FILE_OTYPE){
				(void)memset(sd_mask,'0',MAXNUM);
				(void)memset(sd_setcnt,'0',MAXNUM);
				adt_itoa((unsigned long)sd->sd_mask,sd_mask);
				adt_itoa((unsigned long)sd->sd_setcnt,sd_setcnt);
				(void)strcpy(sd_name,sd->sd_name);
				/* add size of this privilege set information */
				length += strlen(sd_mask)+strlen(sd_setcnt)
					+strlen(sd_name)+3;
			}
		}

		/* add size of machine info, 4 spaces*/
		length+= (strlen(unstr.sysname)+strlen(unstr.nodename)+strlen(unstr.release)+strlen(unstr.version)+strlen(unstr.machine)+4);
		break;
	case A_IDMAP :
		name=&namebuf[0];
		while((passwd_p=getpwent()) != NULL){
			(void)strcpy(name,passwd_p->pw_name);
			if(!ia_openinfo(name,&uinfo) && uinfo != NULL){
				ia_get_uid(uinfo,&uid);
				(void)memset(num,'0',MAXNUM);
				adt_itoa((unsigned long)uid,num);
				length+=(strlen(name) + strlen(num) + 2); 
				ia_closeinfo(uinfo);
			}
		}
		if (length == 0)
                	(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,"UID map",mapfile);
		setpwent();
		break;
	case A_GIDMAP :
		name=&namebuf[0];
		while((group_p=getgrent()) != NULL){
			(void)strcpy(name,group_p->gr_name);
			gid=group_p->gr_gid;
			(void)memset(num,'0',MAXNUM);
			adt_itoa((unsigned long)gid,num);
			length+=(strlen(name) + strlen(num) + 2); 
		}
		if (length == 0)
                	(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,"GID map",mapfile);
		setgrent();
		break;
	case A_CLASSMAP:
		if ((cfp=fopen(ADT_CLASSFILE, "r")) == NULL){
			(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,"CLASS map",mapfile);
			break;
		}
		while ((p = fgets(line, BUFSIZ, cfp)) != NULL) {
			while (*p != NULL ) {
				if (isspace(*p)) 
					p++;
				else {
					if (isalpha(*p) ){
					  if (strncmp(p,ALIAS,5) == 0) {
						p =(char *) next(p,white); 
						class=p;
						p =(char *) next(p,white); 
						length += (strlen(class) + 2);
					  }
					  else{
						events=p;
						p =(char *) next(p,white); 
						length+=(strlen(events) + 1);
					  }
					}
					else
						p++;
				}
			}
			(void)memset(line,' ',BUFSIZ);
		}
		rewind(cfp);
		break;
	case A_TYPEMAP:
		for ( i=1; i<= ADT_ZNUMOFEVTS; i++){
			(void)memset(num,'0',MAXNUM);
			adt_itoa((unsigned long)(adtevt[i].a_evtnum),num);
			length+=strlen(num);
			length+=((strlen(adtevt[i].a_evtnamp)) + 2);
		}
		break;
	case A_PRIVMAP:
		name=&namebuf[0];
		/* the privilege numbers go from 0 to NPRIVS, except for the
	         * number corresponding to the pseudo-privilege "allprivs" */
		for ( number=0; number<NPRIVS; number++ ){
			if ( privname(name,number) != NULL ){
				(void)memset(num,'0',MAXNUM);
				adt_itoa((unsigned long)number, num);
				length += ((strlen(num)) + (strlen(name)) + 2);
			}
		}
		if ( privname(name, P_ALLPRIVS) != NULL ){
			(void)memset(num,'0',MAXNUM);
			adt_itoa((unsigned long)P_ALLPRIVS,num);
			length += ((strlen(num)) + (strlen(name)) + 2);
		}
		break;
	case A_SYSMAP:
		/* the system call numbers go from 1 to num_syscall*/
		for ( number=1; number<=num_syscall ; number++ )
		{
			if ( scallnam(namebuf,number) != NULL )
			{
				(void)memset(num,'0',MAXNUM);
				adt_itoa((unsigned long)number, num);
				length += ((strlen(num)) + (strlen(namebuf)) + 2);
			}
		}
		break;
	default:
		break;
	}

	/*Return length of map + 1 (null byte ending each map)*/
	return(length + 1);
}

char *
next(p,string)
register char *p;
register char *string;
{
	while(*p && *p != *string && *p != '\n')
		++p;
	*p++ = '\0';
	return(p);
}
void
adt_itoa(n, s)
unsigned long n;
char s[];
{
	int i, j, p;
	char c;

	i = 0;
	do {	/* generate digits in reverse order */
		s[i++] = n % 10 + '0';	/* get next digit */
	} while ((n /= 10) > 0); 	/* delete it */
	s[i] = '\0';

	for (p=0, j=strlen(s)-1; p<j; p++, j--) 
		c=s[p], s[p]=s[j], s[j]=c;
}
/*
 * Procedure:     adumprec
 *
 * Restrictions:
                 auditdmp(2): None
 Notes:
 USER level interface to auditdmp(2)
 for USER level audit event records

*/

void
adumprec(status,size,argp)
int status;			/* event status */
int size;			/* size of argp */
char *argp;			/* data/arguments */
{
        arec_t rec;

        rec.rtype = ADT_AUDIT_MAP;
        rec.rstatus = status;
        rec.rsize = size;
        rec.argp = argp;

        auditdmp(&rec, sizeof(arec_t)); 
}
/*
 * Procedure:     ch_name
 *
 * Restrictions:
                 rename(2): None

   Notes: Rename a file
*/
void
ch_name(from,to)
char *from;
char *to;
{
	int path_length;
	char *fromfile, *tofile;

	/*allocate space for the pathname of the old file*/
	path_length=strlen(mapdir)+strlen(from)+1;
	if ((fromfile = ((char *)malloc(path_length))) == NULL) {
                (void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		adumprec(ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}
	(void)sprintf(fromfile,"%s%s",mapdir,from);

	/*allocate space for the pathname of the old file*/
	path_length=strlen(mapdir)+strlen(to)+1;
	if ((tofile = ((char *)malloc(path_length))) == NULL) {
                (void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		adumprec(ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}
	(void)sprintf(tofile,"%s%s",mapdir,to);

	if (rename(fromfile,tofile))
                (void)pfmt(stdout,MM_WARNING,MSG_RENAME,fromfile,tofile);
	free(fromfile);
	free(tofile);
}
void getgmtsecoff() {
	time_t clck=0;
	/* set extern timezone, daylight and altzone */
	(void)time(&clck);
	(void *)ctime(&clck);	
	/* get time zone information (gmt offset in seconds) */
	if (daylight)
	{
		if(localtime(&clck)->tm_isdst)
			gmtsecoff=altzone;
		else
			gmtsecoff=timezone;
	}
	else
		gmtsecoff=timezone;
}

/*
 * Procedure:     lck_file
 *
 * Restrictions:
                 fcntl(2): None

   Notes: Establish an advisory file lock
*/

int
lck_file(fd,mapfile,flckp)
int fd;
char *mapfile;
struct flock *flckp;
{
	short attempt=0;

	while (fcntl(fd,F_SETLK, flckp) < 0)
	{
		if ((errno == EACCES) || (errno == EAGAIN))
		{
			/*The file is locked by another process*/
			if (++attempt < MAX_ATTEMPTS)
				(void)sleep(1);
			else
			{
                		(void)pfmt(stdout,MM_WARNING,MSG_FILEBUSY,mapfile);
				return(-1);
			}
		}
		else
		{
                	(void)pfmt(stderr,MM_ERROR,MSG_FCNTL);
			adumprec(ADT_FMERR,strlen(argvp),argvp);
			exit(ADT_FMERR);
		}
	}
	return(0);
}
int
calc_hdr(length,type)
int length;
int type;
{
	char hold[MAXNUM];
	int hdr_size =0;

	/*Convert the map identifier to ascii string and calculate length*/
	(void)memset(hold,'\0',MAXNUM);
	adt_itoa((unsigned long)length,hold);
	hdr_size = strlen(hold) + 1;

	/*Convert the map data size to ascii string and calculate length*/
	(void)memset(hold,'\0',MAXNUM);
	adt_itoa((unsigned long)type,hold);
	hdr_size += strlen(hold) + 1;

	return(hdr_size);
}
/*
* This routine initializes a malloc'ed area that contains  
* particular information about the privilege mechanism that
* is installed.                                            
*/
setdef_t *
init_sdefs(nsets,mapfile)
register int nsets;
char *mapfile;
{
	setdef_t	*sdefs;

	if( (sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t))) == NULL){
                (void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		adumprec(ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}
	if (sdefs) {
		/* if secsys fails, sdefs may be pointing to garbage */
		if (secsys(ES_PRVSETS, (char *)sdefs)){
			free(sdefs);
			(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,"Privilege mechanism information",mapfile);
			sdefs=NULL;
		}
	}
	return sdefs;
}
