#ident	"@(#)ksh93:src/lib/libast/misc/fs3d.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * 3d fs operations
 * only active for non-shared 3d library
 */

#include <ast.h>
#include <fs3d.h>

int
fs3d(register int op)
{
	register int	cur;
	register char*	v;
	char		val[sizeof(FS3D_off) + 8];

	static int	fsview;
	static char	on[] = FS3D_on;
	static char	off[] = FS3D_off;

	if (fsview < 0) return(0);

	/*
	 * get the current setting
	 */

	if (!fsview && mount(NiL, NiL, 0, NiL))
		goto nope;
	if (FS3D_op(op) == FS3D_OP_INIT && mount(FS3D_init, NiL, FS3D_VIEW, NiL))
		goto nope;
	if (mount(on, val, FS3D_VIEW|FS3D_GET|FS3D_SIZE(sizeof(val)), NiL))
		goto nope;
	if (v = strchr(val, ' ')) v++;
	else v = val;
	if (!strcmp(v, on))
		cur = FS3D_ON;
	else if (!strncmp(v, off, sizeof(off) - 1) && v[sizeof(off)] == '=')
		cur = FS3D_LIMIT(atoi(v + sizeof(off) + 1));
	else cur = FS3D_OFF;
	if (cur != op)
	{
		switch (FS3D_op(op))
		{
		case FS3D_OP_OFF:
			v = off;
			break;
		case FS3D_OP_ON:
			v = on;
			break;
		case FS3D_OP_LIMIT:
			sfsprintf(val, sizeof(val), "%s=%d", off, FS3D_arg(op));
			v = val;
			break;
		default:
			v = 0;
			break;
		}
		if (v && mount(v, NiL, FS3D_VIEW, NiL))
			goto nope;
	}
	fsview = 1;
	return(cur);
 nope:
	fsview = -1;
	return(0);
}
