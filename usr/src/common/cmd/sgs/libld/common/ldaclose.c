#ident	"@(#)libld:common/ldaclose.c	1.5"
#include    <stdio.h>
#include    "filehdr.h"
#include	"synsyms.h"
#include    "ldfcn.h"

int
ldaclose(ldptr)

LDFILE    *ldptr; 

{

    extern int		vldldptr( );
    extern int		freeldptr( );

    if (vldldptr(ldptr) == FAILURE) {
	return(FAILURE);
    }

    (void) fclose(IOPTR(ldptr));
    (void) freeldptr(ldptr);

    return(SUCCESS);
}

