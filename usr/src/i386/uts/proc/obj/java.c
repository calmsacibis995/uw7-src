#ident	"@(#)kern-i386:proc/obj/java.c	1.1"
#ident	"$Header$"

#include <fs/vnode.h>
#include <proc/exec.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define JAVAEXEC	"/usr/bin/javaexec"

MOD_EXEC_WRAPPER(java_, NULL, NULL, "java - exec module");

/*
 * int
 * java_exec(vnode_t *vp, struct uarg *args, int level, long *execsz,
 *		exhda_t *ehdp)
 *	Exec a Java first class executable file.
 *
 * Calling/Exit State:
 *	Called from gexec via execsw[].
 */
/* ARGSUSED */
int
java_exec(vnode_t *vp, struct uarg *args, int level, long *execsz,
	   exhda_t *ehdp)
{
	struct exdata	*edp = &args->execinfop->ei_exdata;
	uint_t	*magicp;		/* magic number */
	int	error;

	/*
	 * Read four bytes to check the magic number because only two bytes
	 * checked so far.
	 */
	if ((error = exhd_read(ehdp, 0, sizeof (int), (void **)&magicp) != 0))
		return (error);	

	/* Check the magic number */
	if (*magicp != 0xbebafeca) /* java magic*/
		return (ENOEXEC);
	
	if ((error = setxemulate(JAVAEXEC, args, execsz)) != 0)
		return error;

	edp->ex_renv |= RE_EMUL;

	return 0;
}


