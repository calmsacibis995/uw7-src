#ident  "@(#)userdel.c	1.3"
#ident  "$Header$"

/*
 * Usage:	userdel [-r] [-n months] login
 *
 * Level:	SYS_PRIVATE
 *
 * Inheritable Privileges:	P_COMPAT,P_DACREAD,P_DACWRITE,P_FILESYS,
 *				P_MACREAD,P_MACWRITE,P_SETFLEVEL
 * Fixed Privileges:		none
 *
 * Notes:	This command deletes user logins from the system.
 *		Arguments are:
 *
 *			-r - removes home directory and its contents
 *
 *			-n [months] - ages the removed uid for `months' value  
 *
 *			login - a string of printable chars except colon (:)
*/

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <pwd.h>
#include <shadow.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <userdefs.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <deflt.h>
#include "users.h"
#include <nwusers.h>
#include "messages.h"
#include "uidage.h"
#include <fcntl.h>
#include <mac.h>
#include <ia.h>
#include <sys/vnode.h>
#include <audit.h>
#include <priv.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/mman.h>

#define	MAX_NVAL	6001

extern	char	*optarg,		/* used by getopt */
		*strdup();

extern	int	optind, opterr;		/* used by getopt */

extern	long	strtol();

extern	int	errmsg(),
		getopt(),
		isbusy(),
		munmap(),
		uid_age(),
		rm_files(),
		edit_group(),
		check_perm(),
		restore_ia(),
		ck_and_set_flevel();

/*
 * This variable must be global so it can be used by the
 * external routine that prints the diagnostic messages
*/
char	*msg_label = "UX:userdel";

struct	passwd	*nis_getpwnam(),
		*nis_getpwent();

static	void	rm_all(),
		bad_news(),
		file_error(),
		replace_all();

static	long	ck_opt(),
		defaults();

static	int		Nisname = 0,
		Nisuser = 0,
		Nisplus = 0;

static	int	rec_pwd(),
		rec_pwd(),
		rm_index(),
		rm_passwd(),
		rm_shadow(),
		rm_nwusers(),
		rm_auditent();

static	char	*Uidage = NULL;

/*
 * Procedure:	main
 *
 * Restrictions:
 *		getpwnam:	None
 *		stat(2):	None
 *		lckpwdf:	None
 *		unlink(2):	None
 *		ulckpwdf:	None
*/

main(argc, argv)
	int argc;
	char **argv;
{
	int	i, ch,
		err = 0,
		audit = 0,
		rflag = 0;

	uid_t	rm_uid;

	char	*logname, *rname, lvlfl[64];

	long	uage = MAX_NVAL;

	struct	passwd	*pwd, *pstruct;
	struct	adtuser	adtuser;
	struct	adtuser	*adtp = &adtuser;
	struct	stat	statbuf;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel(msg_label);

	opterr = 0;			/* no print errors from getopt */

	while ((ch = getopt(argc, argv, "n:r")) != EOF) {
		switch(ch) {
		case 'r':
			rflag = 1;
			break;
		case 'n':
			Uidage = optarg;
			break;
		case '?':
			errmsg(M_DUSAGE);
			exit(EX_SYNTAX);
		}
	}
	/*
	 * read the defaults file and return the value used
	 * as the ageduid time for use later.
	*/
	uage = defaults();

	if (optind != argc - 1) {
		errmsg(M_DUSAGE);
		exit(EX_SYNTAX);
	}

	logname = argv[optind];

	if ((pstruct = nis_getpwnam(logname)) == NULL) {
		errmsg(M_EXIST, logname);
		exit(EX_NAME_NOT_EXIST);
	}

	if (isbusy(logname)) {
		errmsg(M_BUSY, logname, "remove");
		exit(EX_BUSY);
	}
	/*
	 * See we're deleting a NIS entry
	 */
	if (*logname == '+' || *logname == '-'){
		Nisname = 1;
		/*
		 * Now see if it is a +user entry
		 */
		if (*logname != '-' && *(logname+1) != '@')
			Nisuser = 1;
		if (!strcmp(logname, "+"))
			Nisplus = 0;
	}
	/*
	 * remove home directory
	*/
	if (rflag) {
		/*
		 * Check Permissions
		*/
		if (Nisname) {
			if (pstruct->pw_dir && *pstruct->pw_dir){
				if (stat(pstruct->pw_dir, &statbuf) == 0) {
					if (rm_files(pstruct->pw_dir) != EX_SUCCESS) 
						errmsg(M_UNABLE_TO_REMOVE_HOME, strerror(errno));
				}
			} else if (Nisuser && !Nisplus){
				/* 
				 * Check to see if the NIS directory 
				 * exists, if so remove it.
				 */
				if ((pwd=getpwnam(logname+1)) != NULL){
					if (stat(pwd->pw_dir, &statbuf) == 0) {
						if (rm_files(pwd->pw_dir) != EX_SUCCESS) 
							errmsg(M_UNABLE_TO_REMOVE_HOME, strerror(errno));
					}
				} else {
					errmsg(M_UNABLE_TO_REMOVE_HOME, strerror(errno));
					errmsg(M_NIS_UNAVAILABLE);
				}
			}
		} else {
			if ( stat(pstruct->pw_dir, &statbuf) == 0) {
		   		if (check_perm(statbuf, pstruct->pw_uid, pstruct->pw_gid,
		                	S_IWOTH|S_IXOTH) == 0) {
		    			if (rm_files(pstruct->pw_dir) != EX_SUCCESS) 
						errmsg(M_UNABLE_TO_REMOVE_HOME, strerror(errno));
		   		}
                   	     else errmsg(M_NO_PERM, logname, pstruct->pw_dir);
			}
		}
	}
	/*
	 * check if user has an entry in the audit file 
	 * Only non-NIS users entries will be
	 * in audit file.
	*/
	if (!Nisname && getadtnam(logname, adtp) == 0)
		audit = 1;
	/*
	 * check if user has a level file
	*/
	(void) strcpy(lvlfl,LVLDIR);
	(void) strcat(lvlfl, logname);
	/*
	 * Lock the password file(s)
	*/
	if (lckpwdf() != 0) {
		errmsg (M_PASSWD_BUSY);
		exit(8);
	}

	/*
	 * ignore all signals
	*/
	for (i = 1; i < NSIG; i++)
		if ( i != SIGCLD )
			(void) sigset (i, SIG_IGN);

	errno = 0;		/* For correcting sigset to SIGKILL */

	if (stat(lvlfl, &statbuf) == 0) {
		if (unlink(lvlfl)) {
			errmsg (M_CANNOT_UNLINK, lvlfl);
			(void) ulckpwdf();
			exit(3);
		}
	}
	/*
	 * remove the entry from the PASSWD file.
	*/
	if (err = rm_passwd(logname, &rm_uid)) {
		file_error(err);
	}
	/*
	 * remove the entry from the SHADOW file.
	*/
	if (err = rm_shadow(logname)) {
		file_error(err);
	}
	/*
	 * remove the entry from the NWUSERS file.
	*/
	if (err = rm_nwusers(logname)) {
		file_error(err);
	}
	if (!Nisname) {
		/*
		 * If the user had an audit entry, remove it from 
		 * the AUDITMASK file.
		*/
		if (audit) {
			if (err = rm_auditent(logname)) {
				file_error(err);
			}
		}
		/*
		 * remove the entry from the INDEX file.
		*/
		if (err = rm_index(logname)) {
			file_error(err);
		}
		/*
		 * remove user from group file
		*/
		(void) edit_group(logname, (char *)0, (int **)0, 1);
	} 

	/*
	 * rename all original files to the "o" files,
	 * rename all "temp" files to the original files.
	*/

	replace_all(audit);

	if (!Nisname)
		(void) uid_age(ADD, rm_uid, uage);

	(void) ulckpwdf();

	rm_tfm(logname);

	exit(0);
	/* NOTREACHED */
}


#define	UIDAGE	"12"

/*
 * Procedure:	defaults
 *
 * Restrictions:
 *		defopen:	P_MACREAD
*/

static	long
defaults()
{
	FILE	*def_fp;
	char	*Usrdefault = "userdel";

	(void) procprivl(CLRPRV, MACREAD_W, 0);

	if ((def_fp = defopen(Usrdefault)) != NULL) {
		if (!Uidage) {
			if ((Uidage = defread(def_fp, "UIDAGE")) != NULL) {
				if (*Uidage)
					Uidage = strdup(Uidage);
				else
					Uidage = UIDAGE;
			}else
				Uidage = UIDAGE;
			
		}
		(void) defclose(def_fp);

	} else
		Uidage = UIDAGE;

	(void) procprivl(SETPRV, MACREAD_W, 0);

	return ck_opt(Uidage);
}


/*
 * Procedure:	rm_passwd
 *
 * Restrictions:		
 *		stat(2):	None
 *		fopen:		None
 *		setpwent:	None
 *		getpwent:	None
 *		putpwent:	None
 *		fclose:		None
 *		endpwent:	None
 *		getspnam:	None
 *		ulckpwdf:	None
 *		chown(2):	None
 *
 * Notes:	deletes the user from the "/etc/passwd" file.
*/
static	int
rm_passwd(logname, olduid)
	char	*logname;
	uid_t	*olduid;
{
	struct	stat	statbuf;
	struct	passwd	*pw_ptr1p;
	FILE		*fp_temp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;

	if (stat(PASSWD, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & (S_IRUSR|S_IRGRP|S_IROTH)));

 	if ((fp_temp = fopen(PASSTEMP , "w")) == NULL)
		return errno;

	/*
	 * The while loop for reading PASSWD entries
	*/
	errno = error = end_of_file = 0;
	/*
	 * since password file was already opened, just rewind it.
	*/
	nis_setpwent();

	while (!end_of_file) {
		if ((pw_ptr1p = (struct passwd *) nis_getpwent()) != NULL) {
			if (!strcmp(logname, pw_ptr1p->pw_name)) {
				*olduid = pw_ptr1p->pw_uid;
				found = 1;
				continue;
			}
			if (putpwent(pw_ptr1p, fp_temp)) {
				(void) fclose(fp_temp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)			 /* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip*/
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	}

	(void) fclose(fp_temp);
	/*
	 * might as well close the password file as well
	*/
	nis_endpwent();

	if (!found) {
		if (getspnam(logname)) {
			errmsg (M_PASSWD_INCON);
		}
		else
			errmsg (M_NAME_NOT_EXIST, logname);
		rm_all();
		(void) ulckpwdf();
		exit(5);
	}

	if (error >= 1) {
		errmsg (M_BADENTRY,PASSWD);
	}

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, PASSTEMP))
		return errno;

	if (chown(PASSTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}
	return error;
}


/*
 * Procedure:	rm_shadow
 *
 * Restrictions:
 *		stat(2):	None
 *		fopen:		None
 *		getspent:	None
 *		putspent:	None
 *		fclose:		None
 *		endspent:	None
 *		ulckpwdf:	None
 *		chown(2):	None
 *
 * Notes:	deletes the user from the "/etc/shadow" file
*/
static	int
rm_shadow(logname)
	char	*logname;
{
	struct	stat	statbuf;
	struct	spwd	*sp_ptr1p;
	FILE		*fp_temp;
	FILE		*fp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;

	if (stat(SHADOW, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(SHADTEMP , "w")) == NULL)
		return errno;

 	if ((fp = fopen(SHADOW , "r")) == NULL)
		return errno;

	/*
	 * The while loop for reading SHADOW entries
	*/
	errno = error = end_of_file = 0;

	while (!end_of_file) {
		if ((sp_ptr1p = (struct spwd *)fgetspent(fp)) != NULL) {
			if (!strcmp(logname, sp_ptr1p->sp_namp)) {
				found = 1;
				continue;
			}
			if (putspent(sp_ptr1p, fp_temp)) {
				(void) fclose(fp_temp);
				(void) fclose(fp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)			/* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip */
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	}

	(void) fclose(fp);
	(void) fclose(fp_temp);

	/*
	 * close the shadow file as well.
	*/

	if (!found) {
		errmsg (M_NAME_NOT_EXIST, logname);
		rm_all();
		(void) ulckpwdf();
		exit(5);
	}

	if (error >= 1) {
		errmsg (M_BADENTRY,SHADOW);
	}

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, SHADTEMP))
		return errno;

	if (chown(SHADTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}
	return error;
}


/*
 * Procedure:	rm_nwusers
 *
 * Restrictions:
 *		stat(2):	None
 *		fopen:		None
 *		getnwent:	None
 *		fputnwent:	None
 *		fclose:		None
 *		endnwent:	None
 *		ulckpwdf:	None
 *		chown(2):	None
 *
 * Notes:	deletes the hybrid user mapping entry from the 
 *		"/etc/netware/nwusers" file
*/
static	int
rm_nwusers(logname)
	char	*logname;
{
	struct	stat	statbuf;
	struct	hybrid	*np_ptr1p;
	FILE		*fp_temp;
	FILE		*fp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;
	char		*real_logname = logname;

	if (stat(NWUSERS, &statbuf) < 0) 
		return error;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(NWTEMP, "w")) == NULL)
		return errno;

 	if ((fp = fopen(NWUSERS, "r")) == NULL)
		return errno;

	if (Nisname) {
		real_logname = logname+1;
	} 

	/*
	 * The while loop for reading NWUSERS entries
	*/
	errno = error = end_of_file = 0;

	while (!end_of_file) {
		if ((np_ptr1p = (struct hybrid *)fgetnwent(fp)) != NULL) {
			if (!strcmp(real_logname, np_ptr1p->unix_namp)) {
				found = 1;
				continue;
			}
			if (fputnwent(np_ptr1p, fp_temp)) {
				(void) fclose(fp_temp);
				(void) fclose(fp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)			/* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip */
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	}

	(void) fclose(fp);
	(void) fclose(fp_temp);

	if (error >= 1) {
		errmsg (M_NW_BADENTRY);
	}

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, NWTEMP))
		return errno;

	if (chown(NWTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}
	if (chmod(NWTEMP, statbuf.st_mode) < 0) {
		return errno;
	}
	return error;
}
/*
 * Procedure:	rm_auditent
 *
 * Restrictions:
 *		stat(2):	None
 *		fopen:		None
 *		getadtent:	None
 *		fclose:		None
 *		chown(2):	None
 *
 * Notes:	deletes the user's entry from the "/etc/security/ia/audit"
 *		file
*/
static	int
rm_auditent(logname)
	char	*logname;
{
	struct	stat	statbuf;
	struct	adtuser	adtuser;
	struct	adtuser	*adtp = &adtuser;
	FILE		*fp_temp;
	register int	ret = 0,
			error = 0,
			end_of_file = 0;

	if (stat(AUDITMASK, &statbuf) < 0)
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(MASKTEMP , "w")) == NULL) {
		return errno;
	}

	errno = error = end_of_file = 0;

	/*
	 * since the audit file was already opened, just rewind it
	*/
	setadtent();

	while (!end_of_file) {
		if ((ret = getadtent(adtp)) == 0) {
			if (!strcmp(logname, adtp->ia_name)) {
			 	continue;
	      		}
			if (putadtent(adtp, fp_temp)) {
				(void) fclose(fp_temp);
				endadtent();
				return 1;
			}
		}
		else {
			if (ret == -1) 
				end_of_file = 1;
			else if (errno == EINVAL){	/*bad entry found, skip */
				error++;
				errno = 0;
			} else		/* unexpected error found */
				end_of_file = 1;
		}	
	}

	(void) fclose(fp_temp);
	/*
	 * close the audit file as well.
	*/
	endadtent();

	if (error >= 1) {
		errmsg (M_AUDIT_BADENTRY);
	}

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, MASKTEMP))
		return errno;

	if (chown(MASKTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}
	return error;
}


/*
 * Procedure:	rm_index
 *
 * Restrictions:
 *		stat(2):	None
 *		fopen:		None
 *		open(2):	None
 *		fclose:		None
 *		mmap(2):	None
 *		fwrite:		None
 *		chown(2):	None
 *
 * Notes:	deletes the user from the "/etc/security/ia/index" file
*/
static	int
rm_index(logname)
	char	*logname;
{
	struct	stat	statbuf;
	struct	index	*midxp;
	struct	index	index;
	struct	index	*indxp = &index;
	FILE		*fp_temp;
	register int	cnt = 0,
			i, fd_indx;

	if (stat(INDEX, &statbuf) < 0) {
		return errno;
	}

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(TMPINDEX, "w")) == NULL) {
		return errno;
	}

	cnt = (statbuf.st_size/sizeof(struct index));

        if ((fd_indx = open(INDEX, O_RDONLY)) < 0) {
		(void) fclose(fp_temp);
		return errno;
        }

        if ((midxp = (struct index *)mmap(0, (unsigned int)statbuf.st_size,
		PROT_READ, MAP_SHARED, fd_indx, 0)) < (struct index *) 0) {

		(void) close(fd_indx);
		(void) fclose(fp_temp);
		return errno;
	}

	indxp = (struct index *) malloc(statbuf.st_size - sizeof(struct index));

	if (indxp == NULL) {
		(void) munmap((char *)midxp, (unsigned int)statbuf.st_size);
		(void) close(fd_indx);
		(void) fclose(fp_temp);
		return 1;
	}

	for (i = 0; i < cnt; i++) {
		if (strcmp(midxp->name, logname) == 0)
			midxp++;
		else {
			if (fwrite(midxp, sizeof(struct index), 1, fp_temp) != 1) {
				(void) munmap((char *)midxp, (unsigned int)statbuf.st_size);
				(void) close(fd_indx);
				(void) fclose(fp_temp);
				return 1;
			}
			midxp++;
		}
	}

	(void) munmap((char *)midxp, (unsigned int)statbuf.st_size);
	(void) close(fd_indx);
	(void) fclose(fp_temp);

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, TMPINDEX))
		return errno;

	if (chown(TMPINDEX, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}
	return 0;
}


/*
 * Procedure:	rm_all
 *
 * Restrictions:
 *		unlink(2):	None
 *
 * Notes:	does an unconditional unlink (without error checking)
 *		for all temporary files created by userdel.
*/
static	void
rm_all()
{
	(void) unlink(SHADTEMP);
	(void) unlink(PASSTEMP);
	(void) unlink(MASKTEMP);
	(void) unlink(TMPINDEX);
}


/*
 * Procedure:	replace_all
 *
 * Restrictions:
 *		rename(2):	None
 *		link(2):	None
 *		access(2):	None
 *		unlink(2):	None
 *
 * Notes:	renames all the REAL files created to the OLD file,
 * 		renames all the TEMP files created to the REAL file.
*/
static	void
replace_all(audit)
	int	audit;
{
	if (rename(PASSWD, OPASSWD) == -1)
		file_error(errno);

	if (rename(PASSTEMP, PASSWD) == -1) {
		if (link (OPASSWD, PASSWD))
			bad_news();
		file_error(errno);
	}

	if (access(OSHADOW, 0) == 0) {
		if (rename(SHADOW, OSHADOW) == -1) {
			if (rec_pwd()) 
				bad_news();
			else file_error(errno);
		}
	}
		
	if (rename(SHADTEMP, SHADOW) == -1) {
		if (rename(OSHADOW, SHADOW) == -1)
			bad_news();
		if (rec_pwd()) 
			bad_news();
		else file_error(errno);
	}

	if (rename(NWUSERS, ONWUSERS) == -1) {
		if (rename(OSHADOW, SHADOW) == -1)
			bad_news();
		if (rec_pwd()) 
			bad_news();
		else file_error(errno);
	}

	if (rename(NWTEMP, NWUSERS) == -1) {
		if (rename(ONWUSERS, NWUSERS) == -1)
			bad_news();
		if (rename(OSHADOW, SHADOW) == -1)
			bad_news();
		if (rec_pwd()) 
			bad_news();
		else file_error(errno);
	}

	if (Nisname)
		return;

	if (audit) {
		if (access(OAUDITMASK, 0) == 0) {
			if (rename(AUDITMASK, OAUDITMASK) == -1) {
				(void) unlink(MASKTEMP);
				if (rename(OSHADOW, SHADOW) == -1)
					bad_news();
				if (rec_pwd()) 
					bad_news();
				else file_error(errno);
			}
		}

		if (rename(MASKTEMP, AUDITMASK) == -1) {
			(void) unlink(MASKTEMP);
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news();
			if (rec_pwd()) 
				bad_news();
			else file_error(errno);
		}
	}

	if (access(OINDEX, 0) == 0) {
		if (rename(INDEX, OINDEX) == -1) {
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news();
			if (rec_pwd()) 
				bad_news();
			else file_error(errno);
		}
	}

	if (rename(TMPINDEX, INDEX) == -1) {
		if (rename(OINDEX, INDEX) == -1)
			bad_news();

		if (rename(OSHADOW, SHADOW) == -1)
			bad_news();
		if (rec_pwd()) 
			bad_news();
		else file_error(errno);
	}
	restore_ia();/* Restore attributes of master and index files */
	return;
}

/*
 * Procedure:	rec_pwd
 *
 * Restrictions:
 *		unlink(2):	None
 *		link(2):	None
 *
 * Notes:	unlinks the current "/etc/passwd" file and links
 *		the "/etc/opasswd" file to the current "/etc/passwd"
 *		file
*/
static	int
rec_pwd ()
{
	if (unlink(PASSWD) || link(OPASSWD, PASSWD))
		return (-1);
	return (0);
}

/*
 * Procedure:	file_error
 *
 * Notes:	prints a diagnostic message, removes all temporary
 *		files, removes the lock on "/etc/.pwd.lock", and exits.
*/
static	void
file_error(i)
int	i;
{
	errmsg (M_PASSWD_UNCHANGED_WITH_ERRNO, i);
	rm_all();
	(void) ulckpwdf();
	exit(6);
}

/*
 * Procedure:	bad_news
 *
 * Notes:	similar to file_error, except the diagnostic message
 *		is different.
*/
static	void
bad_news()
{
	errmsg (M_PASSWD_MISSING);
	rm_all();
	(void) ulckpwdf();
	exit(7);
}


/*
 * Procedure:	ck_opt
 *
 * Notes:	this routine is used to check to value specified as an
 *		argument to the ``-n'' option.  The valid values for this
 *		option are:
 *
 *			positive integer up to MAX_NVAL,
 *						     -1,
 *						      0
*/
static	long
ck_opt(n_value)
	char	*n_value;
{
	char	*p;
	long	uage = 0;

	if ((uage = strtol(n_value, &p, 10)) < -1 || uage >= MAX_NVAL) {
		errmsg (M_INVALID_AGE);
		exit(EX_SYNTAX);
	}
	if (*p != '\0' || strlen(n_value) <= (size_t) 0) {
		errmsg(M_DUSAGE);
		exit(EX_SYNTAX);
	}

	return uage;
}


/*
 * Procedure:	rm_tfm
 *
 * Restrictions:
 *		stat(2):	None
 *		chdir(2):	None
 *		execl():	None
 *
 * Notes:	deletes the user's TFM entry from the "/etc/security/tfm/user"
 *		database
*/
int
rm_tfm(logname)
	char	*logname;
{

	char	*admin_opt = "-d",
		*admin_cmd = "/usr/bin/adminuser";

	int	status = 0;
	pid_t	pid;
	struct	stat	statbuf;
	register int	error = 0;

	/*
	 * change directory to /etc/security/tfm directory. If
	 * the chdir fails, print out a message and exit.
	*/
	if (chdir("/etc/security/tfm/users") < 0) {
		return errno;
	}

	if (stat(logname, &statbuf) < 0) {
		return error;
	}

	if ((pid = fork()) < 0) {
		errmsg(M_NO_FORK);
		return errno;
	}

	if (pid == (pid_t) 0) {
		/*
		 * in the child
		*/
		(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
		(void) execl(admin_cmd, admin_cmd, admin_opt, logname, (char *)NULL); 
		(void) procprivl(SETPRV, ALLPRIVS_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	else {
		(void) wait(&status);
	}
	return error;
}
