/*		copyright	"%c%" 	*/
#ident	"@(#)yppasswd.c	1.3"
#ident  "$Header$"


#include <sys/types.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netconfig.h>
#include <pfmt.h>
#include <pwd.h>
#include <shadow.h>
#include <ia.h>
#include "yppasswd.h"

static const char *MSG_YPNOCHG = ":1251:Could not change NIS entry (rpc call failed)\n";
static const char *MSG_YPBADPASSWD = ":1252:Remote server %s would not change entry (password probably is incorrect) \n";
static const char *MSG_YPOK = ":1253:NIS entry changed on %s\n";

static char *domain, *master, *nisval;
static int vallen;
extern int graphics;
extern char *Xopass;
extern struct passwd *p_ptr;
extern FILE *err_iop;
extern char *MSG_NP, *sorry;
extern int std_pwgen();
struct master *nis_getmaster();
#define bcopy(s,d,n) memcpy(d, s, n)

#define NUMCP		13	/* number of characters for valid password */
#define PRT_PWD(pwdp)	{\
	if (*pwdp == NULL) \
		 (void) fprintf(stdout, "NP  ");\
	else if (strlen(pwdp) < (size_t) NUMCP) \
		(void) fprintf(stdout, "LK  ");\
	else		(void) fprintf(stdout, "PS  ");\
}

/*
 * Get the port number of yppasswdd on the NIS server
 */
static
getrpcport(host, prog, vers, proto)
    char *host;
	long prog, vers, proto;
{
    struct sockaddr_in sin;
	struct netconfig *nconf;
    struct netbuf svcaddr;


	if ((nconf = getnetconfigent("udp")) == NULL) {
		return(-1);
	}

	memset((char *)&sin, '\0', sizeof(sin));
	svcaddr.buf = (char *)&sin;
	svcaddr.maxlen = sizeof(sin);

	if (rpcb_getaddr(prog, vers, nconf, &svcaddr, host)) {
		return(ntohs(sin.sin_port));
	}
	return(-1);
}
static char *
pwskip(p)
register char *p;
{
    while( *p && *p != ':' && *p != '\n' )
        ++p;
    if (*p == ':')
        *p++ = 0;
    return(p);
}
static char *
strn(cp, n)
register char *cp;
register int n;
{
 
    if (n > 9)
        cp = strn(cp, n / 10);
    *cp++ = "0123456789"[n % 10];
    *cp = '\0';
    return(cp);
}

/*
 * Search SHADOW for entry.  Differs from getspnam() in that
 * NIS entries are not skipped.
 */
struct spwd*
fgetspnam (name)
	char 	*name;
{
	FILE *sfp;
	register struct spwd *p;

	if ((sfp = fopen(SHADOW, "r")) == NULL){
		return (NULL) ;
	}

	while ((p = fgetspent(sfp)) != NULL && strcmp(name, p->sp_namp));
	fclose (sfp);
	return (p);
}

/*
 * Determine whether name refers to a NIS user and, if it does,
 * what sort of NIS user it is.
 *
 * We check two independent items:
 *	1. Is this a NIS name of the form "+username"?
 *	2. Is the password information for this user administrated locally?
 *
 * Flags are set as follows:
 *	nisnam	Set if name is of the form, "+username"
 *	sdwpwd	Set if password info from shadow is used for this user
 *
 * Returns 1 if name is a NIS name (i.e., +username) or corresponds 
 * to an entry in the NIS passwd map.  Otherwise, returns 0.
 */
int
nis_getflags (name, nisnam, sdwpwd, nispwd)
	char	*name;
	int	*nisnam, *sdwpwd, *nispwd;
{
	struct master *mastp;
	int ret = 0;
	char	*np = name;

	*nisnam = *sdwpwd = *nispwd = 0;

	/* Make sure name isn't NULL */
	if (!name)
		return (ret);

	/* Check to see if this is a NIS name (i.e. begins with '+') */
	if ((*name == '+') && *(name+1)) {
		np++;
		ret = *nisnam = 1;
	}
	/*
	 * At this point, np points to name without prepended '+'
	 * which we'll use to search the NIS passwd map.
	 */
	if (get_nisuser(np)) {
		ret = 1;
		if (np == name) {
			np = (char *) malloc (strlen(name)+2);
			strcpy (np, "+");
			strcat (np, name);
		}
		else {
			np = name;
		}
		/* 
		 * At this point, np points to name with prepended '+'
		 * which we'll use for our search of the shadow file.
		 */
		*sdwpwd = ((mastp = nis_getmaster(np)) && mastp->ia_pwdp && *(mastp->ia_pwdp));

		if (np != name)
			free (np);
	}
	return (ret);
}

/*
 * Get a master structure for an NIS user.
 */
struct master*
nis_getmaster (name)
	char *name;
{
    	int err;
	int nisok;
	struct spwd *sptr;
	static struct master *mptr = NULL;

	if (!name || !(*name))
		return (NULL);

	if (mptr) {
		if (strcmp (mptr->ia_name, &name[1]))
			free (mptr);
		else
			return (mptr);
	}

	if ((sptr = fgetspnam(name)) == NULL) {
		return(NULL);
	}

	nisok = get_nisuser(name+1);

	mptr = (struct master *) malloc (sizeof(struct master));
	if (mptr == NULL)
		return (NULL);

	/*
	 * Build a Master structure from data in SHADOW and the
	 * NIS passwd map.  If NIS info is not available, we stuff
	 * in defaults.
	 */
	strcpy(mptr->ia_name, ((nisok) ? p_ptr->pw_name : &name[1]));
	strcpy(mptr->ia_pwdp, sptr->sp_pwdp);
	mptr->ia_uid = (nisok) ? p_ptr->pw_uid : -1;
	mptr->ia_gid = (nisok) ? p_ptr->pw_gid : -1;
	mptr->ia_lstchg = sptr->sp_lstchg;
	mptr->ia_min = sptr->sp_min;
	mptr->ia_max = sptr->sp_max;
	mptr->ia_warn = sptr->sp_warn;
	mptr->ia_inact = sptr->sp_inact;
	mptr->ia_expire = sptr->sp_expire;
	mptr->ia_flag = (long)sptr->sp_flag;
	mptr->ia_dirp = (nisok) ? p_ptr->pw_dir : NULL;
	mptr->ia_dirsz = (nisok) ? strlen(p_ptr->pw_dir) : 0;
	mptr->ia_shellp = (nisok) ? p_ptr->pw_shell : NULL;
	mptr->ia_shsz = (nisok) ? strlen(p_ptr->pw_shell) : 0;

	return(mptr);
}


ping_nis()
{
    register char *p;
    char *buf, uidstr[20];
    int err, buflen, yp_port;

	if (yp_get_default_domain(&domain) ||
			yp_master(domain, "passwd.byname", &master) ||
			(yp_port = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE, IPPROTO_UDP)) < 0 ){
		return(0);
	}
	if (yp_port >= IPPORT_RESERVED) {
		if (!graphics)
			(void)fprintf(stderr, 
				"yppasswd daemon on %s is not running on privileged port\n",
				 master);
		return(0);
	}
	return(1);

}
/*
 * Check to see if name is in NIS database
 */
get_nisuser(name)
char *name;
{
	if (p_ptr && !strcmp (p_ptr->pw_name, name)) 
			return (1);

	if (p_ptr = getpwnam(name))  {
		return (1);
	}
	else {
		return (0);
	}
}
/*
 * Changes NIS password
 */
nis_passwd(adm, flag)
int adm;
int flag;
{
	static char	*pswd, *pw,
		buf[10],
	 	saltc[2],		/* crypt() takes 2 char string as salt */
		pwbuf[10],			
		opwbuf[10];
	time_t 	salt;
	struct yppasswd yppasswd;
	int ok, err, i, c, xdr_yppasswd();
	int uid = getuid();

	/*
	 * Currently, the only supported option is changing the password.  Fail
	 * if any options are set.
	 */
	if (flag != 0) {
		(void) pfmt(err_iop, MM_ERROR, MSG_NP);
		return(1);
	}

	/*
	 * Ping NIS and reject attempt to change password if yppasswdd is not
	 * running on the master server.
	 */
	if (!ping_nis()) {
		(void) pfmt(err_iop, MM_ERROR, MSG_NP);
		return(1);
	}

	/*
	 * The following statement determines who is allowed to
	 * modify a password. the real uid isn't equal to the
	 * user's password that was requested or MAXUID, then
	 * the user doesn't have permission to change the password.
	 *
	 * NOTE:	a real uid of MAXUID indicates that this
	 *		was called from the login scheme.
	 */
	if (!graphics && !adm && uid != p_ptr->pw_uid  && uid != MAXUID) {
		(void) pfmt(err_iop, MM_ERROR, MSG_NP);
		return(1);
	}

	if (!graphics)
		(void) pfmt(err_iop, MM_INFO,
				":366:Changing NIS password for %s on %s \n",
					p_ptr->pw_name, master);
	/*
	 * Prompt for the users new password, check the triviality
	 * and verify the password entered is correct.
	 */
	opwbuf[0] ='\0';
	if (!graphics) {
		if ((pswd = (char *)getpass(gettxt(":378", "Old password:"))) == NULL) {
			(void) pfmt(stderr, MM_ERROR, sorry);
			return 3;
		}
		(void) strcpy(opwbuf, pswd);
		pw = (char *)crypt(opwbuf, p_ptr->pw_passwd);
		if (strcmp(pw, p_ptr->pw_passwd) != 0) {
			(void) pfmt(stderr, MM_ERROR, sorry);
			return 1;
		}
	}
	else {
		(void) strcpy(opwbuf, Xopass);
	}
	/*
	 * get new password 
	 */
	err = std_pwgen(!uid, p_ptr->pw_name, opwbuf, (char *)&buf, (char *)&pwbuf);
	if (err)
		return err;

	/*
	 * Construct salt, then encrypt the new password.
	 */
	(void) time((time_t *)&salt);
	salt += (long)getpid();

	saltc[0] = salt & 077;
	saltc[1] = (salt >> 6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c>'9') c += 7;
		if (c>'Z') c += 6;
		saltc[i] = (char) c;
	}
	pw = (char *)crypt(pwbuf, saltc);
	yppasswd.newpw = *p_ptr;
	yppasswd.newpw.pw_passwd = pw;
	yppasswd.oldpass = opwbuf;
	err = callrpc(master, YPPASSWDPROG, YPPASSWDVERS,
			YPPASSWDPROC_UPDATE, xdr_yppasswd, &yppasswd,
			xdr_int, &ok);
	if (err != 0){
		if (!graphics) {
			clnt_perrno(err);
			(void) pfmt(stderr, MM_ERROR, MSG_YPNOCHG );
		}
		return(1);
	}
	if (ok == 1) {
		if (!graphics) {
			(void) pfmt(stderr, MM_ERROR, MSG_YPBADPASSWD, master);
		}
		return(1);
	}
	if (!graphics) {
		(void) pfmt(stderr, MM_ERROR, MSG_YPOK, master);
	}
	return(0);
}
nis_display()
{
	(void) fprintf(stdout,"%s  ", p_ptr->pw_name);
			PRT_PWD(p_ptr->pw_passwd);
			(void) fprintf(stdout, "\n");
}
xdr_yppasswd(xdrsp, pp)
    XDR *xdrsp;
    struct yppasswd *pp;
{
    if (xdr_wrapstring(xdrsp, &pp->oldpass) == 0)
        return (0);
    if (xdr_passwd(xdrsp, &pp->newpw) == 0)
        return (0);
    return (1);
}

xdr_passwd(xdrsp, pw)
    XDR *xdrsp;
    struct passwd *pw;
{
    if (xdr_wrapstring(xdrsp, &pw->pw_name) == 0)
        return (0);
    if (xdr_wrapstring(xdrsp, &pw->pw_passwd) == 0)
        return (0);
    if (xdr_int(xdrsp, (int *)&pw->pw_uid) == 0)
        return (0);
    if (xdr_int(xdrsp, (int *)&pw->pw_gid) == 0)
        return (0);
    if (xdr_wrapstring(xdrsp, &pw->pw_gecos) == 0)
        return (0);
    if (xdr_wrapstring(xdrsp, &pw->pw_dir) == 0)
        return (0);
    if (xdr_wrapstring(xdrsp, &pw->pw_shell) == 0)
        return (0);
    return(1);
}
