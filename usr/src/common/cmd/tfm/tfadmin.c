/*		copyright	"%c%" 	*/

#ident	"@(#)tfadmin.c	1.3"
#ident  "$Header$"

/***************************************************************************
 * Command: tfadmin
 * Inheritable Privileges: None
 *       Fixed Privileges: P_ALLPRIVS
 * Notes:
 *
 ***************************************************************************/

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <pwd.h>
#include <memory.h>
#include <unistd.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include "err.h"
#include "tfm.h"
#include <audit.h>
#include <libgen.h>
#include <string.h>

#define	NOEXEC		":26:Cannot execute program file\n"
#define	MALLOC		":2:Memory allocation failed\n"
#define NOTFOUND	":27:Undefined command name \"%s\"\n"
#define	NOTALL		":28:User not allowed\n"
#define	NOSETP		":29:Cannot set up maximum privilege set\n"
#define NOTPRIV         ":246:\"%s\" lacks requested privilege for \"%s\"\n"
#define	BADUSG		":8:Incorrect usage\n"
#define	USAGE		":248:\nusage:	tfadmin [role:] cmd [args]\n	tfadmin -t [role:] cmd[:priv[:priv...]]\n"
extern	int	hasprivs(),access(),procpriv(),execv(),privadd(),priv_err();
extern	int	strcmp();
extern	uid_t	getuid();
extern	char	*strncpy();
extern	void	*malloc();
extern	void	free(),exit();

typedef	struct	{
	int		topt;
	int		found;
	tfm_name	role;
	tfm_cname	cmd;
	unsigned	privs;
	tfm_invoke	inv;
} syntax;

static	void	getcmd(),getname(),getinv(),usage(),adumprec();
static	int	options(),isrole(),inroles(),inlist();

static	char	*cmdline = (char *)0;

/*
** Function: main()
**
** Notes:
** Main program entry point, process arguments and call appropriate
** functions to handle request.
*/
main(argc,argv)
int	argc;
char	*argv[];
{
	int		i;
	int		nprvs;
	syntax		opt;
	unsigned	vec;
extern	char		*argvtostr();

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxes");
	(void)setlabel("UX:tfadmin");

	/* save command line arguments for auditdmp(2) */
	if (( cmdline = argvtostr(argv)) == NULL) {
                (void) pfmt(stderr,MM_ERROR,":1:argvtostr failed\n");
		adumprec(ADT_TFADMIN,1,strlen(argv[0]),argv[0]);
                exit(1);
        }

	if(tfm_setup(TFM_ROOT) < 0){
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		(void)tfm_err(ERR_QUIT);
	}
	i = options(argc,argv,&opt);
	if(!opt.found){
		(void)pfmt(stderr,MM_ERROR,NOTFOUND,opt.cmd);
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		exit(1);
	}
	inv_vect(&vec,&opt.inv);
	if(!hasprivs(opt.privs,vec)){
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		exit(1);
	}
	if(access(opt.inv.inv_path,X_OK) < 0){
		(void)pfmt(stderr,MM_ERROR,NOEXEC,argv[i]);
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		exit(1);
	}
	adumprec(ADT_TFADMIN,0,strlen(cmdline),cmdline);
	if(!opt.topt){
/*
** Set up maximum privs and exec the command
*/
		if((nprvs = procpriv(PUTPRV,opt.inv.inv_priv,opt.inv.inv_count)) < 0){
			(void)pfmt(stderr,MM_ERROR,NOSETP);
			adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
			exit(1);
		}
		if(nprvs < opt.inv.inv_count){
			char	*cp;

			cp = strrchr(opt.inv.inv_path, '/');
			(void)pfmt(stdout,MM_WARNING,NOTPRIV,argv[0],++cp);
		}
		argv[i] = opt.inv.inv_path;	/*Set up new command line*/
		(void)execv(argv[i],&argv[i]);	
		(void)pfmt(stderr,MM_ERROR,NOEXEC,argv[i]);	/*exec failed*/
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		exit(1);
	}
	exit(0);
/*NOTREACHED*/
}

/*
** Function: options()
**
** Notes:
** Process command line options and set up the 'opt' structure with the
** information from the command line.
*/
static	int
options(argc,argv,opt)
int	argc;
char	**argv;
syntax	*opt;
{
	int		i;
	tfm_name	user;

	(void)memset(opt,0,sizeof(syntax));
	getname(&user);
	if(argc < 2){
		usage();
	}
	if(*argv[1] == '-'){
		if(*(++argv[1]) == 't'){
			if (strcmp(argv[1], "t"))
				usage();
			opt->topt = 1;
			i = 2;
		} else {
			usage();
		}
	} else {
		i = 1;
	}
	if(i < argc){
		if(isrole(opt,argv[i])){
			if(++i >= argc){
				usage();
			}
		}
		getcmd(opt,argv[i]);
	} else {
		usage();
	}
	getinv(opt,&user);
	if((*argv[i] == '/') && strcmp(opt->inv.inv_path, argv[i])){
		pfmt(stderr,MM_ERROR,NOTFOUND,argv[i]);
		exit(1);
	}
	return(i);
}

/*
** Function: isrole()
**
** Notes:
** Determine whether the argument contains a role specification, if it
** does, copy that out and return 1 (true) otherwise return 0 (false).
** If the role specification specifies a non-existent role, issue an
** error and quit.
*/
static	int
isrole(opt,str)
syntax	*opt;
char	*str;
{
	char	*p;


	if((*str) == ':')
		usage();
	p = str + strlen(str) - 1;
	if((*p) == ':'){
		*p = 0;
		(void)strncpy(opt->role,str,sizeof(tfm_name));
		opt->role[sizeof(tfm_name) - 1] = 0;
		if(tfm_ckrole(opt->role) < 0) {
			(void)tfm_err(ERR_QUIT);
		}
		return(1);
	} else {
		return(0);
	}
}

/*
** Function: getcmd()
**
** Notes:
** Get a command specification.  If the -t option is used, the command
** specification may contain a list of privileges separated from the
** command name by a colon. If the -t option is not used, then the
** command must be a simple null terminated command name.
*/
static	void
getcmd(opt,str)
syntax	*opt;
char	*str;
{
	char	*p;

	p = str;
	while(*p && (*p != ':')) ++p;
	if(*p){
		if(!opt->topt){
			usage();/*Cannot specify privs without -t option*/
		}
		*(p++) = 0;
	}
	if(*str == '/'){
		str = basename(str);
	}
	(void)strncpy(opt->cmd,str,sizeof(tfm_cname));
	opt->cmd[sizeof(tfm_cname)-1] = 0;
	while(*p){		/*Must be -t opt with privs*/
		str = p;
		while(*p && (*p != ':')) ++p;
		if(*p){
			*(p++) = 0;
		}
		if(privadd(&opt->privs,str) < 0){
			(void)priv_err(ERR_QUIT);
		}
	}
}

/*
** Function: getinv()
**
** Notes:
** Once the role and command information has been extracted, get the
** invocation record for the specified user, role and command.
*/
static	void
getinv(opt,user)
syntax		*opt;
tfm_name	*user;
{
	tfm_cmd		raw;
/*
** If no role was specified, try to get the command from the user first,
** then search the user's roles to see if the command is somewhere in
** there.
*/
	if(!opt->role[0]){
		if(tfm_getucmd(user,&opt->cmd,&raw) > -1){
			cmd2inv(&(opt->inv),&raw);
			opt->found = 1;
			return;
		}
/*
** Not in user, search user's roles.
*/
		if(!inroles(opt,user)){
			return;
		}
	}
/*
** If we got here, either a role was specified, or the command was found
** in one of the user's roles.  In either case, we are going to take the
** invocation record from a role.  First check that the user actually
** has the role.
*/
	if(!inlist(user,&opt->role,sizeof(tfm_name),tfm_roles)){
		return;
	}
/*
** The user has the role, get the command from the role, if this fails
** the command is not defined, so get out.
*/
	if(tfm_getrcmd(&(opt->role),&(opt->cmd),&raw) < 0){
		return;
	}
/*
** We got the command, convert it and return.
*/
	cmd2inv(&opt->inv,&raw);
	opt->found = 1;
}

/*
** Function: inroles()
**
** Notes:
** Search the roles defined for the user for the specified command, and,
** if it is found, set the options role name to the first role in which
** the command is found.
*/
static	int
inroles(opt,u)
syntax		*opt;
tfm_name	*u;
{
	tfm_name	*list;
	int		i;
	long		sz;

	if((sz = tfm_roles(u,(tfm_name *)0,0)) < 1){
		return(0);
	}
	list = (tfm_name *)malloc((unsigned)(sz * sizeof(tfm_name)));
	if(!list){
		(void)pfmt(stderr,MM_ERROR,MALLOC);
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		exit(1);
	}
	sz = tfm_roles(u,list,sz);
	for(i = 0; i < sz; ++i){
		if(inlist(&list[i],&(opt->cmd),sizeof(tfm_cname),tfm_rcmds)){
			(void)strncpy(opt->role,list[i],sizeof(tfm_name));
			opt->role[sizeof(tfm_name) - 1] = 0;
			return(1);
		}
	}
	return(0);
}

/*
** Function: inlist()
**
** Notes:
** This function searches a generic name list for a name and returns 1
** (true) if the name is found and 0 (false) if it is not.  The argument
** 'dom' specifies the name of the domain to be retrieved by the
** function specified in 'func'.  The argument 'targ' specifies the
** target string value, and the the argument 'len' specifies the basic
** list element size.  For example, in the case of the tfm_rcmds()
** function, 'dom' is the name of the role, for which a list of commands
** is requested.  The argument 'targ' is the command name for which
** inlist() is searching. The argument 'len' is sizeof(tfm_cname) to allow
** correct indexing into the list, and 'targ' is a command name.
*/
static	int
inlist(dom,targ,len,func)
tfm_name	*dom;
char		*targ;
unsigned	len;
long		(*func)();
{
	char		*list;
	int		i;
	long		sz;

/*
** Determine the size of the domain.
*/
	if((sz = (*func)(dom,(tfm_name *)0,0)) < 1){
		return(0);
	}
/*
** Allocate space for the list.
*/
	list = (char *)malloc((unsigned)(sz * len));
	if(!list){
		(void)pfmt(stderr,MM_ERROR,MALLOC);
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		exit(1);
	}
/*
** Retrieve the domain.
*/
	sz = (*func)(dom,list,sz);
/*
** Search the domain for the name specified in 'targ'
*/
	for(i = 0; i < sz; ++i){
		if(!strcmp((char *)targ,&list[i * len])){
/*
** Found the name, free the list and get out.
*/
			free((void *)list);
			return(1);
		}
	}
/*
** The name is not in this domain, free the list and get out.
*/
	free((void *)list);
	return(0);
}

/*
** Function: getname()
**
** Notes:
** This routine gets the name of the invoking user based on the real
** user-id of the process.  If the user's name cannot be found, the
** numeric representation of the user-id is given instead.  If the user
** does not exist in the TFM database, an error is printed and the
** routine exits with an error code.  The restriction on getpwuid() that
** prevents MACREAD privilege from being used is applied generally to
** all TFM commands in the call to tfm_setup().  Therefore, if we got
** this far, P_MACREAD is not in the working set of the process.  There
** is no need for an explicit restriction here.
*/
static	void
getname(user)
tfm_name	*user;
{
static	char		buf[BUFSIZ];
	struct	passwd	*p;
	char		*np;
	int		id;

	id = getuid();
	p = getpwuid(id);
		if(!p){
			(void)sprintf(buf,"%d",id);/*Error, use UID number*/
			np = buf;
		} else {
			np = p->pw_name;
		}
		(void)strncpy((char *)user,np,sizeof(tfm_name));
		(*user)[sizeof(tfm_name) - 1] = 0;
	endpwent();
	if(tfm_ckuser(user) < 0){
		(void)pfmt(stderr,MM_ERROR,NOTALL);
		adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
		exit(1);
	}
}

/*
** Function: usage()
**
** Notes:
** Print the usage message and exit with an error.
*/
static	void
usage()
{
	pfmt(stderr, MM_ERROR, BADUSG);
	pfmt(stderr, MM_ACTION, USAGE);
	adumprec(ADT_TFADMIN,1,strlen(cmdline),cmdline);
	exit(1);
}


/*
** Function: adumprec()
**
** Restrictions:
**		auditdmp() - none
**
** Notes:
** USER level interface to auditdmp(2) for USER level audit event
** records.
*/
void
adumprec(rtype,status,size,argp)
int rtype;			/* event type */
int status;			/* event status */
int size;			/* size of argp */
char *argp;			/* data/arguments */
{
extern	int	auditdmp();
        arec_t rec;

        rec.rtype = rtype;
        rec.rstatus = status;
        rec.rsize = size;
        rec.argp = argp;
        auditdmp(&rec, sizeof(arec_t));
}

