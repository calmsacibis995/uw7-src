#ident	"@(#)libld:common/ldnrseek.c	1.6"
#include	<stdio.h>
#include	"filehdr.h"
#include	"scnhdr.h"
#include	"synsyms.h"
#include	"ldfcn.h"

int
ldnrseek(ldptr, sectname)

LDFILE	*ldptr;
const char    *sectname;

{

	SCNHDR	shdr;

	if (ldnshread(ldptr, sectname, &shdr) == SUCCESS) {
		if (shdr.s_nreloc != 0) {
		    if (FSEEK(ldptr, shdr.s_relptr, BEGINNING) == OKFSEEK) {
			    return(SUCCESS);
		    }
		}
	}

	return(FAILURE);
}

