/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/prv.c	1.1.7.4"
#ident "$Header$"
/*
 *
 * UNIX shell
 *
 */

#include <pfmt.h>
#include <sys/types.h>
#include <priv.h>
#include "defs.h"
#include <sys/secsys.h>

static int showprivs();
static int initsdefs();

extern int privnum();
extern int privname();

/*
** Function to handle the "priv" built-in command.  The syntax of
** the command is:
**
**	priv [+/-pname]... set_name ...
**
** where:
**
**	pname is the name of a process privilege or 'allprivs' which
**	is shorthand for all process privileges.
**
**	set_name is the name of the privilege set in which the
**	changes should be reflected.
**
** The routine scans the list of privileges and calls procprivl()
** with a SET or CLR command for each depending on whether the first
** character is '+' or '-'.
** 
** If no privileges are listed, the command scans through the list
** of set_names and calls procpriv() with a GET command for each set
** and prints out the contents of that set.  If only one set is
** specified, the privileges are printed in the following form:
**
** pname pname ...
**
** If more than one set os specified each list is printed in the
** following form:
**
** set_name: pname pname ...
*/

static	priv_t	prvconvert();
static	void	printit();
static	void	prverr();

static	struct	pm_setdef	*sdefs=(struct pm_setdef *)0;
static	int	nprvs=0;
static	int	nsets=0;

int
syspriv(argv,argc)
char	**argv;
int	argc;
{
	priv_t	prv;
	int	i,j,endpr = 0;
	int	priv_err;

	if(!sdefs){
		if(initsdefs(SYSPRIV) < 0){
			return(ERROR);
		}
	}
	for(i = 1; i < argc; ++i){
		if((*argv[i] != '+') && (*argv[i] != '-')){
			endpr = i;
			break;
		}
	}
	if(!endpr){
		prverr(SYSPRIV,nopset,nopsetid,(char *)0);
		return(ERROR);
	}
	if(endpr == 1){		/*no privilege names, show instead*/
		if(showprivs(argv,argc) < 0)	{
			return(ERROR);
		}
		else	{
			return(0);
		}
	}

	priv_err = 0;

	for(i = 1; i < endpr; ++i){
		for(j = endpr; j < argc; ++j){
			if(prv = prvconvert(argv[i]+1,argv[j],SYSPRIV)){
				if(*argv[i] == '+'){
					if(procprivl(SETPRV,prv,(priv_t)0)<0){
						prverr(SYSPRIV,setprv,setprvid,
								argv[i]);
						priv_err++;
					}
				} else {
					if(procprivl(CLRPRV,prv,(priv_t)0)<0){
						prverr(SYSPRIV,clrprv,clrprvid,
								argv[i]);
						priv_err++;
					}
				}
			} else {
				return(ERROR);
			}
		}
	}
	return(priv_err > 0);
}

static	int
initsdefs(cmd)
int	cmd;
{
	int	i;

	nsets = secsys(ES_PRVSETCNT, 0);
	if(nsets < 0){
		prverr(cmd,prvunsup,prvunsupid,(char *)0);
		return(-1);
	}
	sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t));
	if(!sdefs){
		prverr(cmd,nospace,nospaceid,(char *)0);
		return(-1);
	}
	(void)secsys(ES_PRVSETS, (char *)sdefs);
	nprvs = 0;
	for(i = 0; i < nsets; ++i){
		if(sdefs[i].sd_objtype == PS_PROC_OTYPE){
			nprvs += sdefs[i].sd_setcnt;
		}
	}
	return(0);
}

static	int
setnum(name)
char	*name;
{
	int	i;

	for(i = 0; i < nsets; ++i){
		if(!cf(name,sdefs[i].sd_name)){
			if(sdefs[i].sd_objtype == PS_PROC_OTYPE){
				return(i);
			}
			return(-1);	/*Not a process privilege set*/
		}
	}
	return(-1);
}

static	priv_t
prvconvert(pname,setname,cmd)
char	*pname,*setname;
int	cmd;
{
	int	set,pr;

	if((set = setnum(setname)) < 0){
		prverr(cmd,setnam,setnamid,setname);
		return((priv_t)0);
	}
	if((pr = privnum(pname)) < 0){
		prverr(cmd,prvnam,prvnamid,pname);
		return((priv_t)0);
	}
	return(sdefs[set].sd_mask | pr);
}

/*
** Get the privileges from the process and display the requested
** sets. If no sets are requested, issue an error.
*/
static	int
showprivs(argv,argc)
char	*argv[];
int	argc;
{
static	priv_t	*buff=(priv_t *)0;
	int	count,i,j,set;
	int	labelit;

	if(argc < 2){
		prverr(SYSPRIV,mssgargn,mssgargnid,(char *)0);
		return(-1);
	}
	if(!buff){
		buff = (priv_t *)malloc(nprvs * sizeof(priv_t));
		if(!buff){
			prverr(SYSPRIV,nospace,nospaceid,(char *)0);
			return(-1);
		}
	}
	labelit = (argc > 2);
	count = procpriv(GETPRV,buff,nprvs);
	for(j = 1; j < argc; ++j){
		if((set = setnum(argv[j])) < 0){
			prverr(SYSPRIV,setnam,setnamid,argv[j]);
			return(-1);
		}
		if(labelit){
			prs_buff(argv[j]);
			prs_buff(gettxt(colonid,colon));
		}
		for(i = 0; i < count; ++i){
			printit(buff[i],set);
		}
		prs_buff("\n");
		flushb();
	}
	return(0);
}

static	void
printit(prv,set)
priv_t	prv;
int	set;
{
	char	buff[512];
	int	j;

	for(j = 0; j < nprvs; ++j){
		if((sdefs[set].sd_mask | j) == prv){
			prs_buff(privname(buff,j));
			prs_buff(" ");
		}
	}
}

static void
prverr(cmd,name,nameid,msg)
int cmd;
char	*name,*nameid,*msg;
{
	set_label(cmd);
	pfmt(stderr, MM_ERROR, NULL);
	prs(msg);
	if(name){
		if (msg)
			prs(" - ");
		if (nameid)
			prs(gettxt(nameid, name));
		else
			prs(name);
	}
	prs("\n");
}

/*
** The following routines are used to clear and restore privileges around
** portions of code that must not use privilege.
*/

int
clrprivs(cmd,buf)
int	cmd;
priv_t	*buf;
{
	static	int	cnt = -1;


	if(cnt){
		if(!sdefs){
			if(initsdefs(0) < 0){
				cnt = 0;
				return(0);
			}
		}
		if((cnt = procpriv(GETPRV,buf,nprvs)) < 0){
			error(cmd,getpriv,getprivid);
		}
		if (cnt)
			(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
		return(cnt);
	}
	return(0);
}

void
rstprivs(cmd,buf,cnt)
int	cmd;
priv_t	*buf;
int	cnt;
{
	if (cnt) {
		if(!sdefs){
			if(initsdefs(0) < 0){
				return;
			}
		}
		if(procpriv(PUTPRV,buf,cnt) < 0){
			error_fail(cmd,rstpriv,rstprivid);
		}
	}
}
