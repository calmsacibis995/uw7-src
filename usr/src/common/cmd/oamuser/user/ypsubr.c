#ident  "@(#)ypsubr.c	1.3"
#ident  "$Header$"
#include <stdio.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpc/rpc_com.h>
#include <rpc/rpcb_prot.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netconfig.h>
#include <string.h>
#include <pwd.h>
#include "messages.h"
#include <string.h>
#include <stddef.h>
#include	"ypdefs.h"

static char *domain;

/*
 * Checks to see if NIS is up and running
 */
nis_check()
{
	if (domain == NULL) {
		if (yp_get_default_domain(&domain)){
			domain = NULL; /* make sure */
			errmsg(M_UNABLE_TO_GET_NIS_DOMAIN, "get NIS domain");
			return(-1);
		}
	}
	return(0);
}
/*
 * Get the uid and gid of name from NIS
 */
nis_getids(name, uid, gid)
char *name;
int *uid;
int *gid;
{
	register char *bp;
	char *val = NULL;
	int vallen, err;

	if (nis_check() < 0)
		return(0);

	if (err = yp_match(domain, "passwd.byname", name, strlen(name),
				&val, &vallen)) {
			errmsg (M_UNABLE_TO_CONTACT_NIS, yperr_string(err));
		return(0);
	}
	bp = strtok(val, ":"); /* skip name */
	bp = strtok(NULL, ":"); /* skip passwd */
	bp = strtok(NULL, ":"); /* uid */
	if (bp == NULL)
		return(0);
	*uid = atoi(bp);
	bp = strtok(NULL, ":"); /* gid */
	if (bp == NULL)
		return(0);
	*gid = atoi(bp);

	return(1);
}
/*
 * See if any users in the given net group are 
 * currently logged in
 */
nis_getnetgr(netgrp)
char *netgrp;
{
	char *user, *mach, *dom;
	char found=FALSE;

	/*
	 * If the netgroup has a wildcard in the
	 * user field, check the entire passwd NIS map
	 */
	if (innetgr(netgrp, NULL, "*", domain))
		return(nis_getall());

	/*
	 * Run thru all of the users in the net group
	 * to see if they are logged in
	 */
	setnetgrent(netgrp);
	while(getnetgrent(&mach,&user,&dom)) {
		if (user) {
			if (logedin(user))
				found = TRUE;
				break;
		}
	}
	endnetgrent();
	return(found);
}
/*
 * This is the callback routine of the yp_all in nis_getall().
 */
static int
callback(status, key, keylen, val, vallen, found)
	int status;
	char *key;
	int  keylen;
	char *val;
	int  vallen;
	int  *found;
{
	char *bp;

	if (status == YP_TRUE){
		/*
		 * only need to look at the name field
		 */
		if (bp = strtok(val, ":")){
			if (logedin(val)){
				*found = TRUE;
				return(TRUE);
			}
		}
		return(FALSE);
	}
	*found = FALSE;
	return(TRUE);
}
/*
 * See if any of the user in the NIS database are
 * logged in
 */
nis_getall()
{
	struct ypall_callback cbinfo;
	int found, err;

	if (nis_check() < 0)
		return(0);

	cbinfo.foreach = callback;
	cbinfo.data = (char *)&found;

	if (err = yp_all(domain, "passwd.byname", &cbinfo)){
		errmsg(M_UNABLE_TO_CONTACT_NIS, yperr_string(err));
		return(FALSE);
	}
	return(found);
}



/*
 * Get password entry from NIS
 */
nis_getuser(name, pwd)
char *name;
struct passwd *pwd;
{
	register char *bp;
	char *val = NULL;
	int vallen, err;
	char *key = name;

	if (nis_check() < 0)
		return(-1);

	if (err = yp_match(domain, "passwd.byname", key, strlen(key),
				&val, &vallen)) {
		switch (err) {
		
			case YPERR_YPBIND:
				return(err);
			case YPERR_KEY:
				return(err);
			default:
				return(err);
		}

	}
	val[vallen] = '\0';
	pwd->pw_name = name;

	if (bp = strchr(val, '\n'))
                        *bp = '\0';

	if ((bp = strchr(val, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_passwd = bp;

	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_uid = atoi(bp);

	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_gid = atoi(bp);
	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_gecos = pwd->pw_comment  = bp;
	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_dir = bp;
	if ((bp = strchr(bp, ':')) == NULL)
		return(-1);
	*bp++ = '\0';
	pwd->pw_shell  = bp;

	return(0);
}
