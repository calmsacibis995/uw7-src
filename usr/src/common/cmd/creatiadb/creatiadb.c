/*		copyright	"%c%" 	*/

#ident	"@(#)creatiadb.c	1.6"

/***************************************************************************
 *
 * Command:	creatiadb
 *
 * Fixed Privileges:		None
 * Inheritable Privileges:	P_DACWRITE,P_MACWRITE,P_SETFLEVEL
 *
 * Files:	/etc/group
 *		/etc/passwd
 *		/etc/shadow
 *		/var/adm/creatialog
 *		/etc/security/ia/index
 *		/etc/security/ia/master
 *
 * Notes:	creates the index and master files by reading the
 *		I&A information from the shadow, passwd, audit, and
 *		level files. 
 *
 **************************************************************************/
#include	<stdio.h>
#include	<pwd.h>
#include	<shadow.h>
#include 	<grp.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<signal.h>
#include	<sys/time.h>
#include	<sys/mac.h>
#include	<audit.h>
#include	<ia.h>
#include	<iaf.h>
#include	<sys/fcntl.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>
#include 	<unistd.h>
#include 	<priv.h>
#include 	<locale.h>
#include 	<pfmt.h>
#include 	<time.h>

#define	CREATLOG	"/var/adm/creatialog"

#define MSG_1	":993:Unexpected failure, master and index files unchanged\n"
#define MSG_2	":816:Unexpected failure. Password file(s) missing.\n"
#define	MSG_3	":994:missing or invalid entry in passwd/shadow file\n"
#define	MSG_4	":995:missing or invalid entry \"%s\" in /etc/security/ia/audit\n"
#define	MSG_5	":996:missing or invalid entry \"%s\" in /etc/security/ia/level\n"
#define	no_aud	":997:/etc/security/ia/audit file missing\n"

extern	void	*calloc(),
		*malloc();

extern	char	*dirname();

extern	int	errno,
		creat(),
		lvlin(),
		lvlproc(),
		lvlfile(),
		auditctl();

static	struct	passwd	*pwd;
static	struct	spwd	*sp;
static	struct	index	*indxp;
static	struct	master	mast;
static	struct	adtuser	adt,
			*adtptr = &adt;
static	struct	stat	sbuf;

static	long	ngroups_max;
static	gid_t	*groups;

static	char	lvlfl[64],
		dir[MAXPATHLEN],
		shl[MAXPATHLEN];

static	void	log(),
		bad_news(),
		rm_files(),
		file_error();

static	int	getgrps(),
		rec_mast();
static	long	get_entries();

static	FILE	*fp_itemp,
		*fp_mtemp;

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: None
                 pfmt: None
                 lckpwdf: None
                 lvlin: None
                 access(2): None
                 creat(2): None
                 lvlfile(2): None
                 ulckpwdf: None
                 fopen: None
                 getpwent: None
                 getspent: None
                 getadtent: None
                 fwrite: None
                 fflush: None
                 fclose: None
                 chown(2): None
                 unlink(2): None
                 rename(2): None
                 link(2): None
*/

/* global scope variables set at initialization in main() */
	int 	mac = 0,
		error = 0,
		audit = 0,
		end_of_file = 0;
	level_t	level = 0;

main(argc) 
	int	argc;
{
	int	i;
	long	cnt = 0,
		count = 0,
		offset = 0,
		length = 0;
	level_t	*lvlp,
		proc_lid = 0;
	actl_t	actl;
	char	*Mac_check = "SYS_PRIVATE";
	uinfo_t oldmast;
	char *old_mast_passwd;
	int indexflag = 0; /* checks existance of /etc/security/ia/index file */


	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel("UX:creatiadb");
	/*
	 * first check command syntax for correctness.
	*/
	if (argc > 1) {
		(void) pfmt(stderr, MM_ERROR, ":998:Invalid command syntax\n");
		log(NULL, ":998:Invalid command syntax\n");
		(void) pfmt(stderr, MM_ACTION, ":999:Usage: creatiadb\n");
		exit(1);
	}

	switch (fork()) {
	case 0:
		/*
         	* lock the password file(s)
		*/
        	if (lckpwdf() != 0) {
        		(void) pfmt(stderr, MM_ERROR,":360:Password file(s) busy.\n");
 
			log(NULL, ":360:Password file(s) busy.\n");
        		exit(1);
		}
		/*
		 * get the level of the process for use later.
		*/
		if (lvlproc(MAC_GET, &proc_lid) < 0) {
			errno = 0;
			proc_lid = 0;
		}
		/*
	 	* Determine if the MAC feature is installed.  If
	 	* so,  then  the ``lvlin()'' call  will return 0.
		*/
		if (lvlin(Mac_check, &level) == 0)
			mac = 1;
		/*
	 	* Determine if the AUDIT feature is installed.
	 	* If so, update the appropriate data files with
	 	* audit information.
		*/
		if (access(AUDITMASK, EFF_ONLY_OK) == 0) {
			audit = 1;
		}
		else {
		/*
	 	* The AUDITMASK file didn't exist.  This is OK if
	 	* the Audit feature isn't included in the kernel.
	 	* It's bad if it is included so exit with a diag-
		 * nostic message indicating the problem.
		*/
			if (auditctl(ASTATUS, &actl, sizeof(actl)) < 0) {
				if (!(errno == ENOSYS || errno == ENOPKG)) {
					(void) pfmt(stderr, MM_ERROR, no_aud);
					log(NULL, no_aud);
					exit(1);
				}
				errno = 0;
			} else { 
				/* auditctl(2) did not fail, audit is in the kernel */
				(void) pfmt(stderr, MM_ERROR, no_aud);
				log(NULL, no_aud);
				exit(1);
			}
		}

		/*
	 	* get number of entries in /etc/passwd
		*/
		if ((count = get_entries()) == 0) {
			file_error();
		}
		/*
	 	* see if shadow file exists.
		*/
		if (stat(SHADOW, &sbuf) < 0) {
			file_error();
		}

		if ((stat (INDEX,&sbuf)) == -1)
		{
			/* Determine whether the master file already exists, it 
			 * will not do if this is the first invocation on a freshly 
			 * installed system. 
			 * Need this information later so set a flag
			 */
			
			 indexflag = 1;
		}

		(void) umask(~(sbuf.st_mode & S_IRUSR));

		indxp = (struct index *) calloc((unsigned int)count, sizeof(struct index)); 

		if (indxp == NULL) {
			(void) pfmt(stderr, MM_ERROR, MSG_1);
			log(NULL, MSG_1);
			(void) ulckpwdf();
			exit(1);
		}
		/*
		 * Create temporary master and index files
		*/
		if ((fp_mtemp = fopen(MASTMP, "w")) == NULL)
			file_error();

		if ((fp_itemp = fopen(TMPINDEX, "w")) == NULL)
			file_error();
		/*
		 * get number of supplementry groups and malloc space
		*/
		ngroups_max = sysconf(_SC_NGROUPS_MAX);
		groups = (gid_t *)calloc((unsigned int) ngroups_max, sizeof(gid_t));
		/*
		 * read the passwd, shadow, group, audit, and level files 
		*/

			 
		while (!end_of_file) {
			errno = 0;
			if (((pwd = getpwent()) != NULL) &&
					((sp = getspent()) != NULL)) {
	
				if (strcmp(pwd->pw_name, sp->sp_namp) != 0) {
					log(pwd->pw_name, MSG_3);
					file_error();
				}
				if (audit) {
					if (getadtent(adtptr) != 0) {
						if (strcmp(pwd->pw_name, adtptr->ia_name) != 0) {
							(void) pfmt(stderr, MM_ERROR, MSG_4, pwd->pw_name);
							log(pwd->pw_name, MSG_4);
							file_error();
						}
					}
					else {
						for (i = 0; i < ADT_EMASKSIZE; i++)
							mast.ia_amask[i] = adtptr->ia_amask[i];
					}
				}
				if (mac) {
					struct	stat	macstat;

					if (stat(LVLDIR, &macstat) < 0) {
						if (proc_lid) {
							bad_news();
						}
						else {
							mac = 0;
							mast.ia_lvlcnt = 0;
						}
					}
					else {
						(void) strcpy(lvlfl, LVLDIR);
						(void) strcat(lvlfl, pwd->pw_name);
						if (stat(lvlfl, &macstat) == 0) {
							mast.ia_lvlcnt = (macstat.st_size/sizeof(level_t));
		
		        				if (lvlia(IA_READ, &lvlp, pwd->pw_name, &mast.ia_lvlcnt) != 0) {
		               		 			(void) pfmt(stderr, MM_ERROR, MSG_5, pwd->pw_name);
								log(pwd->pw_name, MSG_5);
								continue;
		        				}
						}
						else {
		               		 		(void) pfmt(stderr, MM_ERROR, MSG_5, pwd->pw_name);
							log(pwd->pw_name, MSG_5);
							continue;
						}
					}
				 } else
					mast.ia_lvlcnt = 0;
	
				(void) strcpy(mast.ia_name,pwd->pw_name);

				if ( indexflag != 1)
				{
					if (ia_openinfo (mast.ia_name, &oldmast) == 0)				
					{
						(void) ia_get_logpwd(oldmast, &old_mast_passwd);
			
						(void) strcpy(mast.ia_pwdp,old_mast_passwd);

					} else (void) strcpy(mast.ia_pwdp,sp->sp_pwdp);
	
				} else (void) strcpy(mast.ia_pwdp,sp->sp_pwdp);
	
				(void) ia_closeinfo (oldmast);
	
				mast.ia_uid = pwd->pw_uid;
				mast.ia_gid = pwd->pw_gid;
				mast.ia_lstchg = sp->sp_lstchg;
				mast.ia_min = sp->sp_min;
				mast.ia_max = sp->sp_max;
				mast.ia_warn = sp->sp_warn;
				mast.ia_inact = sp->sp_inact;
				mast.ia_expire = sp->sp_expire;
				mast.ia_flag = sp->sp_flag;
	
				mast.ia_dirsz = strlen(pwd->pw_dir);
				mast.ia_shsz  = strlen(pwd->pw_shell);
				mast.ia_sgidcnt = 0;
				/*
				 * get supplementary groups
				*/
				mast.ia_sgidcnt = getgrps(mast.ia_name, mast.ia_gid);
	
				(void) strcpy(dir,pwd->pw_dir);
				(void) strcpy(shl,pwd->pw_shell);
	
				length = sizeof(struct master); 
	
				if (fwrite(&mast, sizeof(struct master), 1, fp_mtemp) != 1) 
					file_error();
	
				if (mast.ia_lvlcnt) {
					if (fwrite((char *) &lvlp[0], sizeof(level_t),
						mast.ia_lvlcnt , fp_mtemp) != mast.ia_lvlcnt) {
						file_error();
					}
				}
				if (mast.ia_sgidcnt) {
					if (fwrite((char *) groups, sizeof(gid_t), mast.ia_sgidcnt, 
							fp_mtemp) != mast.ia_sgidcnt) {
						file_error();
					}
				}
				if (fwrite(dir, (mast.ia_dirsz + 1), 1, fp_mtemp) != 1) 
					file_error();
	
				if (fwrite(shl, (mast.ia_shsz + 1), 1, fp_mtemp) != 1) 
					file_error();
	
				length += (mast.ia_dirsz + mast.ia_shsz + 2 
					+ (mast.ia_lvlcnt * sizeof(level_t))
					+ (mast.ia_sgidcnt * sizeof(gid_t)));
	
				(void) strncpy(indxp[cnt].name, pwd->pw_name,32);
				indxp[cnt].offset = offset; 
				indxp[cnt].length = length;
				offset += length;
				cnt++;
	 		}
			else {
				if (errno == 0)
					/* end of file */
					end_of_file = 1;
				else if (errno == EINVAL) {
					/* Bad entry found, skip it */
					error++;
					(void) pfmt(stdout, MM_WARNING, MSG_3);
					log(NULL, MSG_3);
					errno = 0;
				}
				else {
					/* unexpected error found */
					file_error();
				}
			}
		}

		qsort(indxp[0].name, cnt, sizeof(struct index), (int(*)()) strcmp);

		if (fwrite(indxp, sizeof(struct index), cnt, fp_itemp) != cnt) 
			file_error();

		(void) fflush(fp_mtemp);
		if (fclose(fp_mtemp) != 0) {
			file_error();
		}

		(void) fflush(fp_itemp);
		if (fclose(fp_itemp) != 0) {
			file_error();
		}
		/*
		 * The master and index files should have the same attributes
		 * as the ``/etc/shadow'' file.  Since the level is also an
		 * attribute, check to see if the ``/etc/shadow'' file had
		 * a level.  If so, set the level of the temporary master and
		 * index files to the level of ``/etc/shadow''.
		*/
		if (sbuf.st_level) {
			if (proc_lid != sbuf.st_level) {
				(void) lvlfile(MASTMP, MAC_SET, &sbuf.st_level);
				(void) lvlfile(TMPINDEX, MAC_SET, &sbuf.st_level);
			}
		}
		else {
			/*
			 * The ``/etc/shadow'' file did not have a level.
			 * Check to see if the directory that the master and
			 * index files reside has a level.  If so, set the
			 * files to the level of the directory.
			*/
			char	*Master_dir;

			Master_dir = (char *) malloc(strlen(MASTER));
			Master_dir = dirname(strdup(MASTER));

			(void) stat(Master_dir, &sbuf);

			if (sbuf.st_level) {
				if (proc_lid != sbuf.st_level) {
					(void) lvlfile(MASTMP, MAC_SET, &sbuf.st_level);
					(void) lvlfile(TMPINDEX, MAC_SET, &sbuf.st_level);
				}
			}
		}
		if (chown(MASTMP, sbuf.st_uid, sbuf.st_gid) != NULL) {
			file_error();
		}
		if (chown(TMPINDEX, sbuf.st_uid, sbuf.st_gid) != NULL) {
			file_error();
		}
		/*
		 * ignore all signals
		*/
		for (i = 1 ; i < NSIG ; i++)
			(void) sigset(i, SIG_IGN);

		errno = 0 ;		/* For correcting sigset to certain signals */

		if (access(OMASTER, 0) == 0)  {
			if (unlink (OMASTER) < 0) {
				file_error();
			}
		}
		if (access(MASTER, 0) == 0) {
			if (rename(MASTER, OMASTER) == -1) {
				file_error();
			}
		}
		if (rename(MASTMP, MASTER) == -1) {
			if (link(OMASTER, MASTER))
       	                 bad_news();
			file_error();
		}
		if (access(OINDEX, 0) == 0) {
			if (unlink(OINDEX) < 0) {
				(void) unlink(TMPINDEX);
       		         	if (rec_mast())
       		                 	bad_news();
       		         	else
               		       		file_error();
			}
		}
		if (access(INDEX, 0) == 0) {
       		 	if (rename(INDEX, OINDEX) == -1) {
				(void) unlink(TMPINDEX);
       		         	if (rec_mast())
       		                 	bad_news();
				else
               		         	file_error();
			}
		}
		if (rename(TMPINDEX, INDEX) == -1) {
			if (rename(OINDEX, INDEX) == -1) {
       		                 bad_news();
			}
			if (rec_mast())
               		         bad_news();
			else
                       		 file_error();
		}
		exit(0);
		break;
	case -1:
		/* ERROR */
		exit(1);
	default:
		/* PARENT */	
		exit(0);
	}
}


/*
 * Procedure:	getgrps
 *
 * Restrictions:
                 setgrent: None
                 getgrent: None
 *
 * Notes:	gets all the group entries from ``/etc/group'' for
 *		the user specified by ``name''.
*/
static	int
getgrps(name, gid)
	char	*name;
	gid_t	gid;
{
	register struct group *grp;
	register int i;
	int ngroups = 0;
 
	if (gid >= 0)
		groups[ngroups++] = gid;

	setgrent();

	while (grp = getgrent()) {
		if (grp->gr_gid == gid)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], name))
				continue;
			if (ngroups == ngroups_max) {
				goto toomany;
			}
			groups[ngroups++] = grp->gr_gid;
		}
	}
toomany:
	return ngroups;
}


/*
 * Procedure:	get_entries
 *
 * Restrictions:
                 fopen:  None
                 fgets: None
                 fclose: None

 *
 * Notes:	counts the number of entries in the file ``/etc/passwd''.
*/
static	long
get_entries()
{
	FILE 	*pwdf;
	char	line[BUFSIZ + 1];
	long	cnt = 0;

	if ((pwdf = fopen("/etc/passwd", "r")) == NULL) {
		return 0;
	}
	while (fgets(line, BUFSIZ, pwdf) != NULL)
		cnt++;

	(void) fclose(pwdf);
	return cnt;
}

/*
 * Procedure:	log
 *
 *
 * Restrictions:
                 localtime: None
                 fopen: None
                 fprintf: None
                 fclose: None

 * Notes:	writes a message out to the log file ``/var/adm/creatialog''
 *		for different reasons.  Needs P_DACWRITE since the file is
 *		owned by sys, group sys, with mode 640 (-rw-r-----).
*/
static	void
log(who, message)
	char	*who,
		*message;
{
	FILE	*logf;
	struct	stat	stat_buf;

	time_t 	now;
	struct	tm *tmp;

	/*
	 * create log file if it doesn't exist. 
	*/
	if (stat(CREATLOG, &stat_buf) < 0) {
		(void) close(creat(CREATLOG, (S_IRUSR|S_IWUSR)));
		(void) stat(CREATLOG, &stat_buf);
	}
	/*
	 * If the log file has no level and the MAC feature
	 * is installed, set it equal to ``level''.
	*/
	if (!stat_buf.st_level && mac) {
		(void) lvlfile(CREATLOG, MAC_SET, &level);
	}

	now = time(0);
	tmp = localtime(&now);

	if ((logf = fopen(CREATLOG, "a")) != NULL) {
		if (who){
			(void)fprintf(logf,"%.2d/%.2d %.2d:%.2d %s ",
			  tmp->tm_mon+1,tmp->tm_mday,tmp->tm_hour,tmp->tm_min,who);
			(void) pfmt(logf, MM_NOSTD, message, who);
		}
		else {
			(void)fprintf(logf,"%.2d/%.2d %.2d:%.2d ",
			  tmp->tm_mon+1,tmp->tm_mday,tmp->tm_hour,tmp->tm_min);
			(void) pfmt(logf, MM_NOSTD, message);
		}
		(void) fclose(logf);
	}
	else {
		pfmt(stdout, MM_WARNING, ":1000:could not log entry to ``%s''\n",
			CREATLOG);
	}
}


/*
 * Procedure:	rec_mast
 *
 *
 * Restrictions:
                 unlink(2): None
                 link(2): None
 * Notes:	returns a failure if the unlink of MASTER or the
 *		link of OMASTER to MASTER fails.  This will only be
 *		called if an error occurs.
*/
static	int
rec_mast()
{
	if (unlink(MASTER) || link(OMASTER, MASTER))
		return -1;
	return 0;
}


/*
 * Procedure:	file_error
 *
 * Restrictions:
                 pfmt:  None
                 ulckpwdf: None
 *
 * Notes:	One of several ways the ``creatiadb'' process can
 *		exit.  This routine cleans up (as best as possible),
 *		prints a diagnostic message, and exits with a return
 *		code of 1.
*/
static	void
file_error()
{
	rm_files();
 	(void) pfmt(stderr, MM_ERROR, MSG_1);
	log((char *) NULL, MSG_1);
	(void) ulckpwdf();
	exit(1);
}


/*
 * Procedure:	bad_news
 *
 * Restrictions:
                 pfmt: None
                 ulckpwdf: None
 *
 * Notes:	One of several ways the ``creatiadb'' process can
 *		exit.  This routine cleans up (as best as possible),
 *		prints a diagnostic message, and exits with a return
 *		code of 2.
*/
static	void
bad_news()
{
	rm_files();
 	(void) pfmt(stderr, MM_ERROR, MSG_2);
	log(NULL, MSG_2);
	(void) ulckpwdf();
	exit(2) ;
}


/*
 * Procedure:	rm_files
 *
 * Restrictions:
                 fclose: None
                 unlink(2): None
 *
 * Notes:	silently closes the two temporary files and tries
 *		to remove them.  The removal may fail if the process
 *		doesn't have the necessary access.
*/
static	void
rm_files()
{
	(void) fclose(fp_mtemp);
	(void) fclose(fp_itemp);
	(void) unlink(MASTMP);
	(void) unlink(TMPINDEX);
}
