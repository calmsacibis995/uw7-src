#ident	"@(#)OSRcmds:ksh/sh/apollo.c	1.1"
#pragma comment(exestr, "@(#) apollo.c 25.2 92/12/11 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * UNIX ksh
 *
 * D. G. Korn
 * Bell Telephone Laboratories
 * adapted from APOLLO changes to Bourne Shell
 *
 */

#include        "defs.h"
#include	<errno.h>

#ifdef apollo
#include "/sys/ins/base.ins.c"
#include "/sys/ins/pad.ins.c"
#include "/sys/ins/error.ins.c"
#include <sys/param.h>	/* for maximum pathname length */
#include <apollo/sys/ubase.h>
#include <apollo/sys/name.h>
#include <apollo/error.h>
#include <string.h>

int
pad_create(fname)
char *fname;
{
	short oldfd = 1;
	short newfd;
        short size = 25;
	long st;

	pad_$create (*fname, (short)strlen(fname), pad_$edit, oldfd, 
	    pad_$bottom, 0, size, newfd, st);
        if (st != 0)
		sh_fail("dm pad",e_open);
	return(newfd);
}

pad_wait(fd)
int fd;
{
	long st;

	pad_$edit_wait((stream_$id_t)fd, st);

        return (st == 0 ? 0 : 1);

}

char *
apollo_error()
{
	extern long unix_proc_$status;
        char subsys[80], module[80], code[80];
        short slen, mlen, clen;
        static char retstr[256];

        error_$get_text (unix_proc_$status, subsys, slen, 
        	module, mlen, code, clen);
	subsys[slen] = module[mlen] = code[clen] = 0;
	if (clen == 0)
		sprintf (code, "status 0x%08x", unix_proc_$status);
	if ( mlen )
		sprintf(retstr, "%s (%s/%s)", code, subsys, module );
	else
		sprintf(retstr, "%s (%s)", code, subsys );		

        return (retstr);
}

/*
 * declarations to support the apollo builtin commands 
 * rootnode, inlib, and ver.
 */

static char last_rootnode[MAXPATHLEN] = "/";
static char do_ver;
static char *preval = NULL, *sysval, *sysid = "SYSTYPE";

/* 
 * code to support the apollo builtin functions rootnode, 
 * inlib, and ver.
 */

int	b_rootnode(argn,com)
char **com;
{
	if (argn == 1) 
	{ 	/* report current setting */
		p_setout(st.standout);
		p_str(last_rootnode, NL);
			return(0);
	}
	if (!is_valid_rootnode(com[1])) 
		sh_cfail(e_rootnode);
	if (rootnode(com[1]) != 0) 
	{
		perror("rootnode: ");	/* ? */
		sh_cfail(e_rootnode);
	}
	if (argn == 2)
		strcpy(last_rootnode, com[1]);
	else 
	{
		sysval = com[1];
		com = &com[2];
		sh.un.com = &com[1]; /* set up arg list for sh_eval */
		sh_eval(com[0]);
		if (rootnode(last_rootnode) != 0) 
			sh_cfail(e_rootnode);
	}
	return(0);
}

int	b_ver(argn,com)
char **com;
{
	char *oldver;
	short i1, i2;
	std_$call unsigned char	c_$decode_version();

	oldver = SYSTYPENOD->value.namval.cp;
	if (argn == 1 || argn > 2) 
	{
		sysval = NULL;
		if (oldver)
			preval = sysval = oldver;
	}
	if (argn == 1) 
	{
		if (!oldver || !sysval)
			sh_cfail(e_nover);
		else 
		{
			p_setout(st.standout);
			p_str(sysval, NL) ;
		}
	}
	else 
	{
		if (!c_$decode_version (*com[1], (short) strlen (com[1]), i1, i2))
			sh_cfail(e_badver);
		else 
		{
			if (argn == 2) 
			{
				short namlen = strlen(sysid);
				short arglen = strlen(com[1]);
				 
				nam_free(SYSTYPENOD);
				nam_fputval(SYSTYPENOD, com[1]);
				nam_ontype(SYSTYPENOD, N_EXPORT | N_FREE);
				ev_$set_var (sysid, &namlen, com[1], &arglen);
			}
			else 
			{
				int fd;
				short namlen = strlen(sysid);
				short arglen = strlen(com[1]);

				sysval = com[1];
				com = &com[2];
				sh.un.com = &com[1]; /* set up arg list for sh_eval */
				ev_$set_var(sysid, &namlen, sysval, &arglen);
				if((fd=path_open(com[0],path_get(com[0]))) < 0)
				{
					arglen = (short)strlen(preval);
					ev_$set_var (sysid, &namlen, preval, &arglen);
					sh_fail(com[0],e_found);
				}
				close(fd);
				sh_eval(com[0]);
				arglen = (short)strlen(preval);
				ev_$set_var (sysid, &namlen, preval, &arglen);
			}
		}
	 }
	return(sh.exitval);
}

/*
 * rootnode.c - a chroot call which doesn't require you to be root...
 */

/*
 *  Changes:
	01/24/88 brian	Initial coding
 */
                  

#ifndef NULL
# define	NULL	((void *) 0)
#endif

extern boolean
unix_fio_$status_to_errno(
		status_$t	& status,
		char		* pn,
		short		& pnlen                  
);

is_valid_rootnode(path)
char	*path;
{
	if (geteuid() == 0)
		return 1;
	return (path[0] == '/' && path[1] == '/' && path[2] != '\0' &&
		strchr(&path[2], '/') == NULL);
}

rootnode(path)
char	*path;
{
        uid_$t		dir_uid, rtn_uid;
	name_$pname_t	new_root_name, rest_path;
	name_$name_t	leaf;
	short		rest_len, leaf_len, err;
	status_$t	status;
        
	strcpy(new_root_name, path);

	name_$resolve_afayc(new_root_name, (short)strlen(new_root_name), 
		&dir_uid, &rtn_uid, rest_path, &rest_len, leaf, &leaf_len, &err, &status);

       	if (status.all != status_$ok) {
		unix_fio_$status_to_errno(status, path, strlen(path));
		return (-1);
	}

	name_$set_diru(rtn_uid, rest_path, (short) rest_len, name_$node_dir_type, &status);
         
       	if (status.all != status_$ok) {
		unix_fio_$status_to_errno(status, path, strlen(path));
		return(-1);
	}
	return(0);
}

#endif /* apollo */

