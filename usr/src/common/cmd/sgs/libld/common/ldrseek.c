#ident	"@(#)libld:common/ldrseek.c	1.6"
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"synsyms.h"
#include	"ldfcn.h"

int
ldrseek(ldptr, sectnum)

LDFILE		*ldptr;
unsigned short	sectnum; 

{

	SCNHDR	shdr;

	if (ldshread(ldptr, sectnum, &shdr) == SUCCESS) {
		if (shdr.s_nreloc != 0) {
		    if (FSEEK(ldptr, shdr.s_relptr, BEGINNING) == OKFSEEK) {
			    return(SUCCESS);
		    }
		}
	}

	return(FAILURE);
}

