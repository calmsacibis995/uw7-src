/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/machdep.c	1.2.1.2"
#ident  "$Header$"

/*
 *	@(#) machdep.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*
 *	THIS FILE CONTAINS CODE SPECIFIC TO THE PC/AT COMPUTER.
 *	IT MAY REQUIRE MODIFICATION WHEN MOVING UNIX TO DIFFERENT
 *	MACHINE ARCHITECTURES OR CONFIGURATIONS.
 */


/*
 *	inttochar(twobytes,i)  --  writes the integer i into the two bytes.
 */

inttochar(twobytes,i)
char twobytes[];
unsigned i;
{
	twobytes[0] = (char) ( i       & 0xff);
	twobytes[1] = (char) ((i >> 8) & 0xff);
}



/*
 *	longtochar(fourbytes,i)  --  writes the long integer i into four bytes.
 */

longtochar(fourbytes,i)
char fourbytes[];
long i;
{
	fourbytes[0] = (char) ( i        & 0xff);
	fourbytes[1] = (char) ((i >> 8)  & 0xff);
	fourbytes[2] = (char) ((i >> 16) & 0xff);
	fourbytes[3] = (char) ((i >> 24) & 0xff);
}
