/*		copyright	"%c%" 	*/

#ident	"@(#)tfmlib.c	1.3"
#ident  "$Header$"
#include <ctype.h>
#include <sys/types.h>
#include <priv.h>
#include <sys/secsys.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mac.h>
#include <pfmt.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/signal.h>
#include "err.h"
#include "tfm.h"

#undef	TFM_ROOT	/*This constant should never be used in this file*/

#define	pm_privbit(p)		(pvec_t)(1 << (p))
#define	pm_pos(p)		(pvec_t)((p) & ~PS_TYPE)
/*
**The following error message strings are used to report errors encountered
**by the library routines.
*/
#define SHORTCMD	":31:Insufficient command specification: \"%s\"\n"
#define FULLPATH	":32:Full command pathname must be specified\n"
#define FULLROOT	":33:Full path to TFM database must be specified\n"
#define MALLOC		":2:Memory allocation failed\n"
#define RDCMD		":27:Undefined command name \"%s\"\n"
#define RDROLES		":34:Cannot read role list for user \"%s\"\n"
#define RDUSER		":6:Undefined user \"%s\"\n"
#define RDROLE		":17:Undefined role name \"%s\"\n"
#define RDBASE		":35:TFM database does not exist\n"
#define MKDBASE		":36:Cannot initialize TFM database\n"
#define MKUSER		":37:Cannot add user \"%s\"\n"
#define MKROLE		":38:Cannot add role \"%s\"\n"
#define MAKELOCKU	":39:Cannot alter user \"%s\"\n"
#define	MAKELOCKR	":40:Cannot alter role \"%s\"\n"
#define SETLOCKU	":41:User \"%s\" currently being changed, try again later\n"
#define SETLOCKR	":42:Role \"%s\" currently being changed, try again later\n"
#define KILLR		":43:Cannot remove role \"%s\"\n"
#define KILLU		":44:Cannot remove user \"%s\"\n"
#define WRCMD		":45:Cannot change command \"%s\"\n"
#define WROLES		":46:Cannot change role list for user \"%s\"\n"
#define	BADCMD		":47:Improper command name: \"%s\"\n"

#define NOTEXST		":250:User %s does not exist.\n"
#define PWDBUSY		":251:Password file(s) busy. Try again later\n"
#define UNPRIVILEGED	":252:You are not privileged for this operation\n"
#define PWDINCON	":253:Inconsistent password files\n"

#define	UBRANCH	0	/*User branch flag (for routines that use it)*/
#define	RBRANCH	1	/*Role branch flag (for routines that use it)*/

#define	CMDLEN	14	/*Length (chars) of a TFM command name*/

#define ALLPRIVS	(pm_work(P_ALLPRIVS))
#define DACWRITE	(pm_work(P_DACWRITE))
#define DACREAD		(pm_work(P_DACREAD))
#define MACWRITE	(pm_work(P_MACWRITE))
#define	ENDPRV		0

extern	int	privadd();
extern	int	priv_err();
extern	void	tfm_report();
	
/*
** The following static variable is defined to cut down on static
** memory use and keep the stack small.  It is used as a scratch
** buffer by almost all of the routines in this file.  It is not
** used to communicate between routines.
*/
static	char	path[PATH_MAX];

/*
** Function: trunc()
**
** Notes:
** Truncate a string name with a NULL. This is needed because strncpy() is
** not guaranteed to put a null on the end of the destination string.
*/

static	void
trunc(buf,len)
char		*buf;
unsigned	len;
{
	buf[len - 1] = 0;
}

/*
** Function: privhex()
**
** Notes:
** Convert an unsigned vector of privilege bits to a hexadecimal string
** and place the result in 'str'.
*/
static	void
privhex(str,priv)
char		*str;
unsigned	priv;
{
	(void)sprintf(str,"%8.8X",priv);/*This needs to change if size*/
					/*changes*/
}

/*
** Function: privunhex()
**
** Notes:
** Convert the hexadecimal representation of a privilege vector supplied
** in 'in' to a binary form, and place the result in 'out'.
*/
static	void
privunhex(out,in)
unsigned	*out;
char		*in;
{
	(void)sscanf(in,"%X",out);
}

/*
** Function: tfm_rmpriv()
**
** Notes:
** Given a target command definition and a mask commmand definition,
** delete the privileges in the mask from the target.
*/
void
tfm_rmpriv(targ,mask)
tfm_cmd	*targ,*mask;
{
	unsigned	maskv,targv;

	privunhex(&targv,targ->cmd_priv);
	privunhex(&maskv,mask->cmd_priv);
	targv &= ~maskv;
	privhex(targ->cmd_priv,targv);
}

/*
** Function: tfm_gotpriv()
**
** Notes:
** Determine whether the command definition buffer in 'buf' has any
** privileges turned on.  If so return 1, otherwise return 0.
*/
int
tfm_gotpriv(buf)
tfm_cmd	*buf;
{
	register int	i;

	for(i = 0; i < (sizeof(buf->cmd_priv) - 1) ; ++i){
		if(buf->cmd_priv[i] != '0'){
			return(1);
		}
	}
	return(0);
}

/*
** The buffer msg is used internally to store error messages to be printed
** by the TFM message routine.  The text of the message is set up by any
** routine that fails, along with the severity and action code.  The
** severity can be ERR_NONE, ERR_MSG, ERR_WARN, or ERR_ERROR, the action
** code can be ERR_QUIT or ERR_CONTINUE.  Setting the severity to ERR_NONE
** clears the error condition and results in a null message if the TFM
** message routine is called, the only routine that should set the
** severity to this value is the TFM message routine because all others
** should expect a message.
*/
static	struct	msg	msgbuf = {ERR_NONE,ERR_CONTINUE,{0}};

/*
** Function: goodname()
**
** Notes:
** 	Check that a command name works as a filename.  This means
** 	that the command name contains no slashes.
*/

static	int
goodname(str)
char	*str;
{
	while(*str){
		if(*str == '/'){
			return(0);
		}
		++str;
	}
	return(1);
}

/*
** Function: parsecmd()
**
** Notes:
** Parse a command line command definition of the form:
**	name:path[:priv][:priv][:...]
** Put the name of the command in 'name.'  Use the remaining information
** to fill out the command definition structure pointed to by 'outbuf.'
*/

#define	NELEM	2	/*Number of strings before the privs*/
#define	CMD	0	/*Position of the command string*/
#define	PATH	1	/*Position of the path string*/

static	int
parsecmd(outbuf,name,inbuf,wantpath)
tfm_cmd		*outbuf;
tfm_cname	*name;
char		*inbuf;
int		wantpath;
{
	char		*p[NELEM],*q;
	unsigned	privs=0;
	register int	i;

	(void)memset((void *)outbuf,0,(unsigned)sizeof(tfm_cmd));
	(void)memset((void *)name,0,(unsigned)sizeof(tfm_cname));
	for(i = 0; i < NELEM; ++i){
		p[i] = (char *)0;
	}
/*
** Find all of the required strings
*/
	q = inbuf;
	for(i = 0; i < (wantpath ? NELEM : NELEM - 1); ++i){
		p[i] = inbuf;
		while(*inbuf && (*inbuf != ':')) ++inbuf;
		if(*inbuf){
			*(inbuf++) = 0;
		}else if(i < (wantpath ? (NELEM - 1) : (NELEM - 2))){
				/*Last colon not required*/
			(void)strcpy(&msgbuf.text[0],SHORTCMD);
			(void)strcpy(&msgbuf.args[0][0],q);
			msgbuf.sev = ERR_ERR;
			msgbuf.act = ERR_QUIT;
			return(-1);	/*Not enough entries*/
		}
	}
/*
** Fill out the privilege vector.
*/
	while(*inbuf){
		q = inbuf;
		while(*inbuf && (*inbuf != ':')) ++inbuf;
		if(*inbuf){
			*(inbuf++) = 0;
		}
		if(privadd(&privs,q) < 0){	/*Unrecognized privilege*/
			return(-1);
		}
	}
/*
** Copy out the required strings.
*/
	privhex(outbuf->cmd_priv,privs);
	if(goodname(p[CMD])){
		(void)strncpy((char *)name,p[CMD],sizeof(tfm_cname));
		trunc((char *)name,sizeof(tfm_cname));
	} else {
		(void)strcpy(&msgbuf.text[0],BADCMD);
		(void)strcpy(&msgbuf.args[0][0],p[CMD]);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	if(wantpath && p[PATH][0] != '/'){	/*Leading '/' is required*/
		(void)strcpy(&msgbuf.text[0],FULLPATH);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	if(p[PATH]){
		(void)strncpy((char *)outbuf->cmd_path,p[PATH],PATH_MAX);
	} else {
		outbuf->cmd_path[0] = 0;
	}
	outbuf->cmd_path[PATH_MAX-1] = 0;
	return(0);
}

/*
** Function: parselist()
**
** Notes:
** Parse a command line argument containing a comma separated list of command
** definitions.  Fill out the header buffer 'head' with the number of
** commands, a pointer to an array containing the names of the commands
** and a pointer to an array of command definition structures.  This is
** the public interface to the command string parsing functions.
*/
int
parselist(str,head,wantpath)
char		*str;
tfm_cmdlst	*head;
int		wantpath;
{
	register int	i;
	register char	*p;

	p = str;
	head->ncmds = 1;
	while(*str){
		if(*str == ','){
			++head->ncmds;
		}
		++str;
	}
/*
** Lint complains about this line, but it is okay.
*/
	head->cmds = (tfm_cmd *)malloc(head->ncmds * sizeof(tfm_cmd));
	head->names = (tfm_cname *)malloc(head->ncmds * sizeof(tfm_cname));
	for(i = 0; i < head->ncmds; ++i){
		str = p;
		while(*str && (*str != ',')) ++str;
		if(*str){
			*(str++) = 0;
		}
		if(parsecmd(&head->cmds[i],&head->names[i],p,wantpath) < 0){
			return(-1);		/*Bad command definition*/
		}
		p = str;
	}
	return(0);
}


/*
** Trusted Facility management library routines.  These routines
** general access to the TFM database and data structures.
*/

/*
** Function: inv_vect()
**
** Notes:
** Make a privilege bitmap from an invocation record.
*/
void
inv_vect(vecp,in)
unsigned	*vecp;
tfm_invoke	*in;
{
	int	i;

	*vecp = 0;
	for(i = 0; i < in->inv_count; ++i){
		*vecp |= pm_privbit(pm_pos(in->inv_priv[i]));
	}
}


/*
** Function: cmd2inv()
**
** Notes:
** Convert a command specification (as read from a file) to an
** invocation specification (used internally to invoke a command).  This
** is a public conversion routine.
*/
void
cmd2inv(out,in)
tfm_invoke	*out;
tfm_cmd		*in;
{
static	int		max_count = -1,max_mask = 0;
	int		n,j;
	setdef_t	*sd;
	unsigned	vec;

	(void)strcpy(out->inv_path,in->cmd_path);
	privunhex(&vec,in->cmd_priv);
	out->inv_priv = 0;
	out->inv_count = 0;
	if(max_count < 0){
		n = secsys(ES_PRVSETCNT,0);
		if(n < 0){
			return;
		}
		sd = malloc(n * sizeof(setdef_t));
		if(!sd){
			return;
		}
		if(secsys(ES_PRVSETS,(void *)sd) < 0){
			return;
		}
		for(j = 0; j < n; ++j){
			if(!strcmp("max",sd[j].sd_name)){
				break;
			}
		}
		if(j >= n){
			free(sd);
			return;
		}
		max_count = sd[j].sd_setcnt;
		max_mask = sd[j].sd_mask;
		free(sd);
	}
	out->inv_priv = malloc(max_count * sizeof(priv_t));
	if(!out->inv_priv){
		return;
	}
	for(j= 0, n=0; (j < max_count) && vec ; ++j, vec >>= 1){
		if(vec & 1){
			out->inv_priv[n++] = max_mask | j;
		}
	}
	out->inv_count = n;
}

/*
** Function: inv2cmd()
**
** Notes:
** Convert an invocation record (used internally to invoke a command) to
** a command definition record (as stored in a command definition file).  This
** is a public conversion routine.
*/
void
inv2cmd(out,in)
tfm_cmd		*out;
tfm_invoke	*in;
{
	unsigned	vec;

	(void)memset((void *)out,0,(unsigned)sizeof(tfm_cmd));
	(void)strcpy(out->cmd_path,in->inv_path);
	inv_vect(&vec,in);
	privhex(out->cmd_priv,vec);
}

/*
** Function: tfm_setup()
**
** Restrictions:
**		lvlfile() - none
**
** Notes:
** Set up the TFM operating environment before anything else. Among
** other things, this routine initializes the variable tfm_root, used to
** specify the root of the TFM database currently in use, and the
** variables, tfm_uid, tfm_gid and tfm_lid.  These contain the owner,
** group and LID to be applied to all TFM Database files.  
**
*/

static	char	*tfm_root=(char *)0;	/*Root of TFM database*/
static	uid_t	tfm_uid = -1;
static	gid_t	tfm_gid = -1;
static	level_t	tfm_lid = 0;

int
tfm_setup(rootdir)
char	*rootdir;
{
	struct	stat	sbuf;
	level_t		plid;

	if(!tfm_root){
		if(*rootdir != '/'){
			(void)strcpy(&msgbuf.text[0],FULLROOT);
			msgbuf.sev = ERR_ERR;
			msgbuf.act = ERR_QUIT;
			return(-1);
		}
		tfm_root = malloc(strlen(rootdir) + 1);
		if(!tfm_root){
			(void)strcpy(&msgbuf.text[0],MALLOC);
			msgbuf.sev = ERR_ERR;
			msgbuf.act = ERR_QUIT;
			return(-1);
		}
		(void)strcpy(tfm_root,rootdir);
	}
	if(!stat(tfm_root, &sbuf)){
		tfm_uid = sbuf.st_uid;
		tfm_gid = sbuf.st_gid;
	}
	if(lvlfile(tfm_root, MAC_GET, &tfm_lid) < 0){
		tfm_lid = 0;
	}
/*
** If the process is running at the same level as the TFM database,
** there is no need for it to set the level on files it creates.
*/
	if(lvlproc(MAC_GET,&plid) == 0){
		if(plid == tfm_lid){
			tfm_lid = 0;
		}
	}
	return(0);
}

/*
** Function: getbuf()
**
** Restrictions:
**		open() - none
**
** Notes:
** General routine to read a file given a path to the file and the
** size of the data buffer.  If the file is inaccessible or too
** small, the buffer is zeroed out and a -1 is returned.
*/
static	int
getbuf(path,buf,size)
char	*path;
char	*buf;
long	size;
{
	register int	fd,ret=0;

	fd = open(path,O_RDONLY);
	if(fd < 0){
		(void)memset((void *)buf,0,(unsigned)size);
		return(-1);
	}
	if(read(fd,buf,(unsigned int)size) != size){
		(void)memset((void *)buf,0,(unsigned)size);
		ret = -1;
	}
	(void)close(fd);
	return(ret);
}

/*
** Function: tfm_getrcmd()
**
** Notes:
** Read the specified command file in the specified role definition.
*/
int
tfm_getrcmd(role,cmd,buf)
tfm_name	*role;
tfm_cname	*cmd;
tfm_cmd		*buf;
{
	register int	ret;

	trunc((char *)role,sizeof(tfm_name));
	trunc((char *)cmd,sizeof(tfm_cname));
	(void)sprintf(path,"%s/roles/%s",tfm_root,(char *)role);
	if(access(path, 0) < 0){
		(void)strcpy(&msgbuf.text[0], RDROLE);
		(void)strcpy(&msgbuf.args[0][0], (char *)role);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	(void)sprintf(path,"%s/roles/%s/cmds/%s",tfm_root,(char *)role,
								(char *)cmd);
	if((ret = getbuf(path,(char *)buf,(long)sizeof(tfm_cmd))) < 0){
		(void)strcpy(&msgbuf.text[0],RDCMD);
		(void)strcpy(&msgbuf.args[0][0],(char *)cmd);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	} 
	return(ret);
}

/*
** Function: tfm_getucmd()
**
** Notes:
** Read the specified command file in the specified user definition.
*/
int
tfm_getucmd(user,cmd,buf)
tfm_name	*user;
tfm_cname	*cmd;
tfm_cmd		*buf;
{
	register int	ret;

	trunc((char *)user,sizeof(tfm_name));
	trunc((char *)cmd,sizeof(tfm_cname));
	(void)sprintf(path,"%s/users/%s",tfm_root,(char *)user);
	if(access(path, 0) < 0){
		(void)strcpy(&msgbuf.text[0], RDUSER);
		(void)strcpy(&msgbuf.args[0][0], (char *)user);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	(void)sprintf(path,"%s/users/%s/cmds/%s",tfm_root,(char *)user,
								(char *)cmd);
	if((ret = getbuf(path,(char *)buf,(long)sizeof(tfm_cmd))) < 0){
		(void)strcpy(&msgbuf.text[0],RDCMD);
		(void)strcpy(&msgbuf.args[0][0],(char *)cmd);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	} 
	return(ret);
}

/*
** Function: tfm_roles()
**
** Restrictions:
**		open() - none
**
** Notes:
** Read 'count' entries from the role list in the specified user
** definition and return the actual number of entries in the file.
** If 'buf' is null or 'count' is 0, don't read anything into buf,
** just return the number of entries in the file.
*/
long
tfm_roles(user,buf,count)
tfm_name	*user;
tfm_name	*buf;
long		count;
{
static	tfm_name	tmp;
	register int	fd;
	register long	ret=0;

	trunc((char *)user,sizeof(tfm_name));
	(void)sprintf(path,"%s/users/%s/roles",tfm_root,(char *)user);
	fd = open(path,O_RDONLY);
	if(fd < 0){
		if(buf){
			(void)memset((void *)buf,0,
				(unsigned)(count * sizeof(tfm_name)));
		}
		(void)strcpy(&msgbuf.text[0],RDROLES);
		(void)strcpy(&msgbuf.args[0][0],(char *)user);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	if(!buf){
		count = 0;
	}
	if(count > 0){
		ret = read(fd,(char *)buf,
					(unsigned int)count*sizeof(tfm_name));
		if(ret < 0){
			(void)memset((void *)buf,0,
				(unsigned)(count * sizeof(tfm_name)));
			(void)strcpy(&msgbuf.text[0],RDROLES);
			(void)strcpy(&msgbuf.args[0][0],(char *)user);
			msgbuf.sev = ERR_ERR;
			msgbuf.act = ERR_QUIT;
			return(-1);
		}			
		ret /= sizeof(tfm_name);
	}
	while(read(fd,(char *)tmp,sizeof(tfm_name)) == sizeof(tfm_name)){
		++ret;
	}
	(void)close(fd);
	return(ret);
}

/*
** Function: getdir()
**
** Restrictions:
**		opendir() - none
**
** Notes:
** Read a directory ignoring "." and ".." and put 'count' names
** into 'buf'. Return the actual number of entries in the
** directory, excluding "." and "..".
*/
static	long
getdir(path,buf,sz,count)
char		*path;
char		*buf;
unsigned	sz;
long		count;
{
	DIR	*dp;
	char	*bp;
	register struct	dirent	*entry;
	register long	ret=0;
	
	dp = opendir(path);
	if(!dp){
		return(-1);
	}
	if(!buf){
		count = 0;
	}
	bp = buf;
	while((entry = readdir(dp)) != (struct dirent *)0){
		if(strcmp(entry->d_name,".") && strcmp(entry->d_name,"..")){
			if(ret < count){
				(void)strncpy(&bp[ret * sz],entry->d_name,sz);
				trunc(&bp[ret * sz], sz);
			}
			++ret;
		}
	}
	(void)closedir(dp);
	return(ret);
}

/*
** Function: tfm_ucmds()
**
** Notes:
** Get the names of the first 'count' commands defined for a given
** user and return the actual number of commands defined for the
** user.
*/
long
tfm_ucmds(user,buf,count)
tfm_name	*user;
char		*buf;
long		count;
{
	trunc((char *)user,sizeof(tfm_name));
	(void)sprintf(path,"%s/users/%s/cmds",tfm_root,(char *)user);
	if((count = getdir(path,buf,sizeof(tfm_cname),count)) < 0){
		(void)strcpy(&msgbuf.text[0],RDUSER);
		(void)strcpy(&msgbuf.args[0][0],(char *)user);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	} 
	return(count);
}

/*
** Function: tfm_rcmds()
**
** Notes:
** Get the names of the first 'count' commands defined for a given
** role and return the actual number of commands defined for the
** role.
*/
long
tfm_rcmds(role,buf,count)
tfm_name	*role;
char		*buf;
long		count;
{
	trunc((char *)role,sizeof(tfm_name));
	(void)sprintf(path,"%s/roles/%s/cmds",tfm_root,(char *)role);
	if((count = getdir(path,buf,sizeof(tfm_cname),count)) < 0){
		(void)strcpy(&msgbuf.text[0],RDROLE);
		(void)strcpy(&msgbuf.args[0][0],(char *)role);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	} 
	return(count);
}

/*
** Function: tfm_getusers()
**
** Notes:
** Get the first 'count' entries in the list of users defined
** in the TFM database.  Return the actual number of users in
** the TFM database.
*/
long
tfm_getusers(buf,count)
char		*buf;
long		count;
{
	(void)sprintf(path,"%s/users",tfm_root);
	if((count = getdir(path,buf,sizeof(tfm_name),count)) < 0){
		(void)strcpy(&msgbuf.text[0],RDBASE);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	}
	return(count);
}

/*
** Function: tfm_getroles()
**
** Notes:
** Get the first 'count' entries in the list of roles defined
** in the TFM database.  Return the actual number of roles in
** the TFM database.
*/
long
tfm_getroles(buf,count)
char		*buf;
long		count;
{
	(void)sprintf(path,"%s/roles",tfm_root);
	if((count = getdir(path,buf,sizeof(tfm_name),count)) < 0){
		(void)strcpy(&msgbuf.text[0],RDBASE);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	}
	return(count);
}

/*
** Function: tfm_ckdb()
**
** Restrictions:
**		access() - none
**
** Notes:
** Find out if a the TFM database exists.  If it does, return 0,
** otherwise, return -1.
*/
int
tfm_ckdb()
{
	if(access(tfm_root,F_OK) < 0){
		(void)strcpy(&msgbuf.text[0],RDBASE);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	return(0);
}

/*
** Function: tfm_ckrole()
**
** Restrictions:
**		access() - none
**
** Notes:
** Find out if a the role named 'role' exists.  Return 0 if it does, -1
** otherwise.
*/
int
tfm_ckrole(role)
tfm_name	*role;
{
	trunc((char *)role,sizeof(tfm_name));
	(void)sprintf(path,"%s/roles/%s",tfm_root,(char *)role);
	if(access(path,F_OK)){
		(void)strcpy(&msgbuf.text[0],RDROLE);
		(void)strcpy(&msgbuf.args[0][0],(char *)role);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	return(0);
}

/*
** Function: tfm_ckuser()
**
** Restrictions:
**		access() - none
**
** Notes:
** Find out if a the user named 'user' exists.  Return 0 if it does, -1
** otherwise.
*/
int
tfm_ckuser(user)
tfm_name	*user;
{
	trunc((char *)user,sizeof(tfm_name));
	(void)sprintf(path,"%s/users/%s",tfm_root,(char *)user);
	if(access(path,F_OK)){
		(void)strcpy(&msgbuf.text[0],RDUSER);
		(void)strcpy(&msgbuf.args[0][0],(char *)user);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	return(0);
}


/*
** Function: newdir()
**
** Restrictions:
**		mkdir() - none
**		rmdir() - none
**		chown() - none
**		lvlfile() - none
**
** Notes:
** Make a directory under the TFM database and set it's ownership and LID.
*/
static	int
newdir(path)
char	*path;
{
	register int	ret=0;
	mode_t		oumask;

	oumask = umask(TFM_UMASK);
	if(mkdir(path,TFM_DMASK) < 0){
		ret = -1;
	} else if(tfm_lid && (lvlfile(path, MAC_SET, &tfm_lid) < 0)){
		(void)rmdir(path);
		ret = -1;
	} else if((tfm_uid != -1) && (tfm_gid != -1) &&
		  (chown(path,tfm_uid,tfm_gid) < 0)){
		(void)rmdir(path);
		ret = -1;
	}
	(void)umask(oumask);
	return(ret);
}

/*
** Function: tfm_newdb()
**
** Notes:
** Create an empty TFM database.
*/
int
tfm_newdb()
{
	if(newdir(tfm_root) < 0){
		(void)strcpy(&msgbuf.text[0],MKDBASE);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	(void)sprintf(path,"%s/users",tfm_root);
	if(newdir(path) < 0){
		(void)strcpy(&msgbuf.text[0],MKDBASE);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	(void)sprintf(path,"%s/roles",tfm_root);
	if(newdir(path) < 0){
		(void)strcpy(&msgbuf.text[0],MKDBASE);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	return(0);
}

/*
** Function: tfm_newuser()
**
** Notes:
** Create a new user in the TFM database.
*/
int
tfm_newuser(user)
tfm_name	*user;
{

	int	i,end_of_file=0,match=0;
	struct	passwd	*pstructp;
	FILE	*fp;


	/*
	 *  check apropriate privelege for access to /etc/passwd and
	 *  /etc/shadow using effective uid
	 */
	if (access(SHADOW,R_OK|EFF_ONLY_OK)) {
		(void)strcpy(&msgbuf.text[0],UNPRIVILEGED);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	/*
	 * Set a lock on the password file so that no other
	 * process can update the "/etc/passwd" and "/etc/shadow"
	 * at the same time.  Also, ignore all signals so the
	 * work isn't interrupted by anyone.
	*/
	if (lckpwdf() != 0) {
		(void)strcpy(&msgbuf.text[0],PWDBUSY);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	for (i = 1; i < NSIG; ++i)
		if ( i != SIGCLD )
			(void) sigset(i, SIG_IGN);

	/*
	 * clear errno since it was set by some of the calls to sigset
	 */
	errno = 0;

 	if ((fp = fopen(PASSWD , "r")) == NULL){
		(void) ulckpwdf();
		(void)strcpy(&msgbuf.text[0],PWDBUSY);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	while (!end_of_file) {
		if ((pstructp=fgetpwent(fp)) != NULL){
			if(!strcmp(user, pstructp->pw_name)){
				match=1;
				end_of_file=1;
				continue;
			}
			else
				continue;
		}
		else {
			if(errno == 0) {	/* EOF */
				end_of_file=1;
				(void)strcpy(&msgbuf.text[0],NOTEXST);
				msgbuf.sev = ERR_ERR;
				msgbuf.act = ERR_QUIT;
			}
			else if (errno==EINVAL) { /* unexpected error */
				errno=0;
				(void)strcpy(&msgbuf.text[0],PWDBUSY);
				msgbuf.sev = ERR_ERR;
				msgbuf.act = ERR_QUIT;
			} else
				end_of_file=1;
		}
	}

	(void) fclose(fp);
	(void) ulckpwdf();

	if (!match)
		return(-1);
	else if (!getspnam(user)) {
		(void)strcpy(&msgbuf.text[0],PWDINCON);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}

	trunc((char *)user,sizeof(tfm_name));
	(void)sprintf(path,"%s/users/%s",tfm_root,(char *)user);
	if(newdir(path) < 0){
		(void)strcpy(&msgbuf.text[0],MKUSER);
		(void)strcpy(&msgbuf.args[0][0],(char *)user);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	(void)strcat(path,"/cmds");
	if(newdir(path) < 0){
		(void)strcpy(&msgbuf.text[0],MKUSER);
		(void)strcpy(&msgbuf.args[0][0],(char *)user);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	return(0);
}

/*
** Function: tfm_newrole()
**
** Notes:
** Create a new role in the TFM database.
*/
int
tfm_newrole(role)
tfm_name	*role;
{
	trunc((char *)role,sizeof(tfm_name));
	(void)sprintf(path,"%s/roles/%s",tfm_root,(char *)role);
	if(newdir(path) < 0){
		(void)strcpy(&msgbuf.text[0],MKROLE);
		(void)strcpy(&msgbuf.args[0][0],(char *)role);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	(void)strcat(path,"/cmds");
	if(newdir(path) < 0){
		(void)strcpy(&msgbuf.text[0],MKROLE);
		(void)strcpy(&msgbuf.args[0][0],(char *)role);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	return(0);
}

/*
** Function: crfile()
**
** Restrictions:
**		access() - none
**		creat() - none
**		fchown() - none
**		unlink() - none
**		lvlfile() - none
**		open() - none
**
** Notes:
** Create a file with the correct attributes for TFM database entries
** and return a pointer to the file.  If the file cannot be created or
** set up correctly, return a -1.
*/
static	int
crfile(path)
char	*path;
{
	int	fd;
	mode_t	oumask;

	oumask = umask(TFM_UMASK);
	if(access(path,0) < 0){
		if((fd = creat(path,TFM_CMASK)) < 0){
			goto out;		/*Error, get out*/
		}
		(void)close(fd);
		if(tfm_lid && (lvlfile(path, MAC_SET, &tfm_lid) < 0)){
			(void)close(fd);
			(void)unlink(path);
			fd = -1;
			goto out;		/*Error, get out*/
		}
		if((tfm_uid != -1) &&
		   (tfm_gid != -1) && 
		   (chown(path,tfm_uid,tfm_gid) < 0)){
			(void)close(fd);
			(void)unlink(path);
			fd = -1;
			goto out;		/*Error, get out*/
		}
	}
	fd = creat(path,TFM_CMASK);		/*Truncate the file*/

out:	(void)umask(oumask);
	return(fd);
}
/*
** Function: lock()
**
** Notes:
** Routine to lock a user or a role.  This is an advisory lock and
** assumes that any other process attempting to change the user or role
** will try to lock it first.
*/
static	int
lock(branch,name)
int	branch;
char	*name;
{
	register int	fd;
	register char	*ep1,*ep2,*bp;

	ep1 = ((branch == UBRANCH) ? MAKELOCKU : MAKELOCKR);
	ep2 = ((branch == UBRANCH) ? SETLOCKU : SETLOCKR);
	bp = ((branch == UBRANCH) ? "users" : "roles");
	(void)sprintf(path,"%s/%s/%s/lock",tfm_root,bp,name);
	if((fd = crfile(path)) < 0){
		(void)strcpy(&msgbuf.text[0],ep1);
		(void)strcpy(&msgbuf.args[0][0],name);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	if(lockf(fd,F_TLOCK,0) < 0){
		(void)strcpy(&msgbuf.text[0],ep2);
		(void)strcpy(&msgbuf.args[0][0],name);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		(void)close(fd);
		return(-1);
	}
	return(fd);
}

/*
** Function: newfile()
**
** Notes:
** Create a file with appropriate attributes and write a buffer
** of 'size' bytes into it.
*/
static	int
newfile(path,buf,size)
char	*path;
char	*buf;
long	size;
{
	int		fd;

	if((fd = crfile(path)) < 0){
		return(-1);
	}
	if(write(fd,buf,(unsigned int)size) != size){
		(void)unlink(path);
		return(-1);
	}
	(void)close(fd);
	return(0);
}


/*
** Function: cleandir()
**
** Restrictions:
**		unlink() - none
**
** Notes:
** Remove all files within a directory.  This routine assumes that the
** directory contains only files, no subdirectories.  Given that we know
** the layout of the TFM directory tree, this assumption works.
*/
static	int
cleandir(path)
char	*path;
{
static	char		pth[PATH_MAX];
	char		*buf;
	register long	i,sz,osz=0;
	register int	error=0;
start:
	while((sz = getdir(path,(char *)0,0,0l)) != osz){/*Until stable*/
		if((sz < 0) && (errno != ENOENT) && (errno != ENOTDIR)){
			return(-1);
		}
		osz = sz;
	}
	if(sz == 0){
		return(0);
	}
	buf = (char *)malloc(((unsigned int)sz) * sizeof(tfm_cname));
	if(!buf){
		return(-1);
	}
	sz = getdir(path,buf,sizeof(tfm_cname),sz);
	if(sz != osz){
		osz = 0;
		free((void *)buf);
		goto start;	/*Size is not stable, keep trying*/
	}
	for(i = 0; i < sz; ++i){
		(void)sprintf(pth,"%s/%s",path,&buf[i*sizeof(tfm_cname)]);
		if((unlink(pth) < 0) && (errno != ENOENT)){
			error = -1;
		}
	}
	if((error == 0) && (getdir(path,(char *)0,0,0l) > 0)){
		osz = 0;
		free((void *)buf);
		goto start;	/*size changed, try again*/
	}
	return(error);
}

/*
** Function: killru()
**
** Restrictions:
**		rmdir() - none
**
** Notes:
** Kill a role or user definition.
*/
static	int
killru(branch,name)
int	branch;
char	*name;
{
	register char	*ep,*bp;
	register int	ret=0;

	ep = ((branch == UBRANCH) ? KILLU : KILLR);
	bp = ((branch == UBRANCH) ? "users" : "roles");
	(void)sprintf(path,"%s/%s/%s/cmds",tfm_root,bp,name);
	if(cleandir(path) < 0){
		ret = -1;
	} else if((rmdir(path) < 0) && (errno != ENOENT)){
		ret = -1;
	} else {
		(void)sprintf(path,"%s/%s/%s",tfm_root,bp,name);
		if(cleandir(path) < 0){
			ret = -1;
		} else if(rmdir(path) < 0){
			ret = -1;
		}
	}
	if(ret < 0){
		(void)strcpy(&msgbuf.text[0],ep);
		(void)strcpy(&msgbuf.args[0][0],name);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	}
	return(ret);
}

/*
** Function: tfm_killrole()
**
** Notes:
** Kill a role definition.
*/
int
tfm_killrole(r)
tfm_name	*r;
{
	register int	lk;
	register int	ret=0;

	trunc((char *)r,sizeof(tfm_name));
				/*Lock the role, lk is a file descriptor*/
	if((lk = lock(RBRANCH,(char *)r)) < 0){
		return(-1);	/*Can't get it, never mind*/
	}
	ret = killru(RBRANCH,(char *)r);
	(void)close(lk);
	return(ret);
}
	
/*
** Function: tfm_killuser()
**
** Notes:
** Kill a user definition.
*/
int
tfm_killuser(u)
tfm_name	*u;
{
	register int	lk;
	register int	ret=0;

	trunc((char *)u,sizeof(tfm_name));
				/*Lock the user, lk is a file descriptor*/
	if((lk = lock(UBRANCH,(char *)u)) < 0){
		return(-1);	/*Can't get it, never mind*/
	}
	ret = killru(UBRANCH,(char *)u);
	(void)close(lk);
	return(ret);
}

/*
** Function: putbuf()
**
** Restrictions:
**		unlink() - none
**
** Notes:
** General routine to handle write or remove operations.  If the pointer
** passed in the 'buf' parameter is not a NULL pointer, then 'size' bytes
** from 'buf' should be written to the file named by the 'pth' parameter,
** otherwise the file named by the 'pth' parameter should be removed.
*/
static	int
putbuf(pth,buf,size)
char	*pth;
char	*buf;
long	size;
{
	if(buf){
		return(newfile(pth,buf,size));
	}
	return(unlink(pth));
}

/*
** Function: tfm_putucmd()
**
** Notes:
** Create, replace or delete a command definition within the specified
** user.
*/
int
tfm_putucmd(user,cmd,buf)
tfm_name	*user;
tfm_cname	*cmd;
tfm_cmd		*buf;
{
	register int	lk,ret;

	trunc((char *)user,sizeof(tfm_name));
	trunc((char *)cmd,sizeof(tfm_cname));
				/*Lock the user, lk is a file descriptor*/
	if((lk = lock(UBRANCH,(char *)user)) < 0){
		return(-1);	/*Can't get it, never mind*/
	}
	(void)sprintf(path,"%s/users/%s/cmds/%s",tfm_root,
						(char *)user,(char *)cmd);
	ret = putbuf(path,(char *)buf,(long)sizeof(tfm_cmd));
	if(ret < 0){
		(void)strcpy(&msgbuf.text[0],WRCMD);
		(void)strcpy(&msgbuf.args[0][0],(char *)cmd);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	}
	(void)close(lk);	/*release lock*/
	return(ret);
}

/*
** Function: tfm_putrcmd()
**
** Notes:
** Create, replace or delete a command definition within the specified
** role.
*/
int
tfm_putrcmd(role,cmd,buf)
tfm_name	*role;
tfm_cname	*cmd;
tfm_cmd		*buf;
{
	register int	lk,ret;


	trunc((char *)role,sizeof(tfm_name));
	trunc((char *)cmd,sizeof(tfm_cname));
				/*Lock the role, lk is a file descriptor*/
	if((lk = lock(RBRANCH,(char *)role)) < 0){
		return(-1);	/*Can't get it, never mind*/
	}
	(void)sprintf(path,"%s/roles/%s/cmds/%s",tfm_root,(char *)role,
								(char *)cmd);
	ret = putbuf(path,(char *)buf,(long)sizeof(tfm_cmd));
	if(ret < 0){
		(void)strcpy(&msgbuf.text[0],WRCMD);
		(void)strcpy(&msgbuf.args[0][0],(char *)cmd);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	}
	(void)close(lk);	/*release lock*/
	return(ret);
}

/*
** Function: tfm_putroles()
**
** Notes:
** Create, replace or delete a role list within the specified user.
*/
int
tfm_putroles(user,buf,count)
tfm_name	*user;
tfm_namelist	*buf;
long		count;
{
	register int		i,lk,ret;

	if(!(buf && (count > 0))){
		count = 0;
		buf = (tfm_namelist *)0;
	}
	trunc((char *)user,sizeof(tfm_name));
					/*Lock the user, lk is an fd*/
	if((lk = lock(UBRANCH,(char *)user)) < 0){
		return(-1);		/*Can't lock, forget it*/
	}
	for(i = 0; i < count; ++i){
		trunc((char *)buf[i],sizeof(tfm_name));
	}
	(void)sprintf(path,"%s/users/%s/roles",tfm_root,(char *)user);
	ret = putbuf(path,(char *)buf,(long)(count * sizeof(tfm_name)));
	if(ret < 0){
		(void)strcpy(&msgbuf.text[0],WROLES);
		(void)strcpy(&msgbuf.args[0][0],(char *)user);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
	}
	(void)close(lk);
	return(ret);
}

/*
** Function: tfm_err()
**
** Notes:
** This routine sets up messages posted by the TFM routines
** for printing.  argument is the requested action to be taken after
** the message has been posted. 
**
** The action can be ERR_CONTINUE, ERR_QUIT or ERR_UNKNOWN. If it
** is ERR_CONTINUE, this routine will return a 1 after printing the
** message, if ERR_QUIT this routine will exit with a 1 after
** printing the message.  If the action is ERR_UNKNOWN the action
** specified by the failing routine will be taken.  If no error was
** posted by any TFM routine, this routine calls the privilege
** reporting routine (priv_err()) to see if the error originated in
** the privilege library.  If that routine returns a 0  this routine
** prints an empty error message to indicate that an unknown error
** has occurred.
*/
int
tfm_err(action)
int	action;
{
	if(msgbuf.sev == ERR_NONE){
		if(priv_err(action)){	/*maybe in privlib*/
			return(1);		/*got it*/
		}
	}
	if(action != ERR_UNKNOWN){
		msgbuf.act = action;
	}
	tfm_report(&msgbuf);
	return(1);			/*error posted by this library*/
}
