/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:getwidth.c	1.2"
#endif

#if defined(sun) /* or other porting that doesn't care I18N */

#include <stdio.h>

void getwidth()
{
	fprintf(stderr, "getwidth: Dummy version\n");
}

#else

#include <stdlib.h>
#include <sys/euc.h>
#include "_wchar.h"
#include <ctype.h>

void getwidth(eucstruct)
eucwidth_t *eucstruct;
{
	eucstruct->_eucw1 = eucw1;
	eucstruct->_eucw2 = eucw2;
	eucstruct->_eucw3 = eucw3;
	eucstruct->_multibyte = multibyte;
	if (_ctype[520] > 3 || eucw1 > 2)
		eucstruct->_pcw = sizeof(unsigned long);
	else
		eucstruct->_pcw = sizeof(unsigned short);
	eucstruct->_scrw1 = scrw1;
	eucstruct->_scrw2 = scrw2;
	eucstruct->_scrw3 = scrw3;
}

#endif
