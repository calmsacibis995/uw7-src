/*
 * Copyright 1993 UNIX System Laboratories (USL)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of USL not be used in 
 * advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  USL make 
 * no representations about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * USL DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
 * EVENT SHALL USL BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE.
 */

/************************************************************************

SCCS:   	@(#)etet/src/posix_c/cplusplus/tcmchild.C	1.1
NAME:		'C++' Test Case Manager
PRODUCT:	TET (Test Environment Toolkit)
AUTHOR:		A. Josey, USL
DATE CREATED:	6th May 1993
SYNOPSIS:

	int main(int argc, char **argv);

DESCRIPTION:

	Main() is the main program for the Test Case Manager (TCMCHILD).
	The C++ code linked to the TET C API needs a C++ main.

MODIFICATIONS:

************************************************************************/

#include <unistd.h>
#include <stdlib.h>

extern "C" {

extern	int	tet_childmain(int , char **);

}


/* ARGSUSED */
int
main(int argc, char **argv)
{
	exit (tet_childmain(argc, argv));
}
