#ident	"@(#)ksh93:src/cmd/ksh93/sh/apollo.c	1.1"
#pragma prototyped
/*
 * UNIX ksh
 *
 * D. G. Korn
 * Bell Telephone Laboratories
 * adapted from APOLLO changes to Bourne Shell
 *
 */

#include	<ast.h>
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

int pad_create(char *fname)
{
	short oldfd = 1;
	short newfd;
        short size = 25;
	long st;

	pad_$create (*fname, (short)strlen(fname), pad_$edit, oldfd, 
	    pad_$bottom, 0, size, newfd, st);
        if (st != 0)
		error(ERROR_system(1),gettxt(e_open_id,e_open),"dm pad");
	return(newfd);
}

pad_wait(int fd)
{
	long st;

	pad_$edit_wait((stream_$id_t)fd, st);

        return (st == 0 ? 0 : 1);

}

char *apollo_error(void)
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

int	b_rootnode(int argn,char *argv[])
{
	if (argn == 1) 
	{ 	/* report current setting */
		sfprintf(sfstdout,"%s\n",last_rootnode);
			return(0);
	}
	if (!is_valid_rootnode(argv[1])) 
		sh_cfail(gettxt(e_rootnode_id,e_rootnode));
	if (rootnode(argv[1]) != 0) 
	{
		perror(gettxt(":208","rootnode: "));	/* ? */
		sh_cfail(gettxt(e_rootnode_id,e_rootnode));
	}
	if (argn == 2)
		strcpy(last_rootnode, argv[1]);
	else 
	{
		sysval = argv[1];
		sh_eval(sh_sfeval(argv),0);
		if (rootnode(last_rootnode) != 0) 
			sh_cfail(gettxt(e_rootnode_id,e_rootnode));
	}
	return(0);
}

int	b_ver(int argn,char *argv[])
{
	char *oldver;
	short i1, i2;
	std_$call unsigned char	c_$decode_version();

	oldver = SYSTYPENOD->namval.cp;
	if (argn == 1 || argn > 2) 
	{
		sysval = NULL;
		if (oldver)
			preval = sysval = oldver;
	}
	if (argn == 1) 
	{
		if (!oldver || !sysval)
			sh_cfail(gettxt(e_nover_id,e_nover));
		else 
		{
			sfprintf(sfstdout,"%s\n",sysval);
		}
	}
	else 
	{
		if (!c_$decode_version (*argv[1], (short) strlen (argv[1]), i1, i2))
			sh_cfail(gettxt(e_badver_id,e_badver));
		else 
		{
			if (argn == 2) 
			{
				short namlen = strlen(sysid);
				short arglen = strlen(argv[1]);
				 
				nv_unset(SYSTYPENOD);
				nv_putval(SYSTYPENOD, argv[1],NV_RDONLY);
				nv_onattr(SYSTYPENOD, NV_EXPORT | NV_NOFREE);
				ev_$set_var (sysid, &namlen, argv[1], &arglen);
			}
			else 
			{
				int fd;
				short namlen = strlen(sysid);
				short arglen = strlen(argv[1]);

				sysval = argv[1];
				argv = &argv[2];
				sh_eval(sh_sfeval(argv),0);
				ev_$set_var(sysid, &namlen, sysval, &arglen);
				if((fd=path_open(argv[0],path_get(argv[0]))) < 0)
				{
					arglen = (short)strlen(preval);
					ev_$set_var (sysid, &namlen, preval, &arglen);
					error(ERROR_system(1),gettxt(e_open_id,e_open),argv[0]);
				}
				close(fd);
				sh_eval(sfopen(argv[0],"s"),0);
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

is_valid_rootnode(const char *path)
{
	if (geteuid() == 0)
		return 1;
	return (path[0] == '/' && path[1] == '/' && path[2] != '\0' &&
		strchr(&path[2], '/') == NULL);
}

rootnode(char * path)
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

/*
 *  Apollo system support library loads into the virtual address space
 */

int	b_inlib(argc,argv)
char **argv;
{
	register char *a1 = argv[1];
	int status;
	short len;
	std_$call void loader_$inlib();
	if(sh.subshell)
		sh_subfork();
	if(a1)
	{
		len = strlen(a1);
		loader_$inlib(*a1, len, status);
		if(status!=0)
			error(3, gettxt(e_badinlib_id,e_badinlib));
	}
	return(0);
}
