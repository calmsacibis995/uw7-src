/*		copyright	"%c%" 	*/

#ident	"@(#)ismodel.c	1.2"
#ident	"$Header$"

#include "lp.h"
#include "lpadmin.h"

extern int		Access();

int			ismodel (name)
	char			*name;
{
	if (!name || !*name)
		return (0);

	return (Access(makepath(Lp_Model, name, (char *)0), 04) != -1);
}
