#ident	"@(#)libld:common/ldfhread.c	1.6"
#include    <stdio.h>
#include    "filehdr.h"
#include	"synsyms.h"
#include    "ldfcn.h"

int
ldfhread(ldptr, filehead)

LDFILE    *ldptr;
FILHDR    *filehead; 

{

    extern int		vldldptr( );

    if (vldldptr(ldptr) == SUCCESS) {
	if (FSEEK(ldptr, 0L, BEGINNING) == OKFSEEK) {
	    if (FREAD((char *)filehead, FILHSZ, 1, ldptr) == 1) {
	    	return(SUCCESS);
	    }
	}
    }

    return(FAILURE);
}

