/*		copyright	"%c%" 	*/

#ident	"@(#)lp.set.c	1.3"
#ident	"$Header$"

/*******************************************************************************
 *
 * FILENAME:    lp.set.c
 *
 * DESCRIPTION: Cause the printer parameters to be output to the printer, these
 *              parameters are:
 *                H - cpi,
 *                V - lpi,
 *                W - width,
 *                L - length,
 *                S - character set.
 *
 * SCCS:	lp.set.c 1.3  8/26/97 at 13:20:38
 *
 * CHANGE HISTORY:
 *
 * 26-08-97  Paul Cunningham        ul94-28782
 *           Change the main() function for program lp.set so that if the string
 *           "default" is passed as the L (page length) string, then it calls
 *           set_size() so that it will set the default page length if the 
 *           printer type is a Unisys AP9210-lj. Note: function set_size() was
 *           also changed.
 *
 *******************************************************************************
 */

#include "stdio.h"
#include "stdlib.h"

#include "lp.h"
#include "lp.set.h"

extern char		*getenv();

/**
 ** main()
 **/

int			main (argc, argv)
	int			argc;
	char			*argv[];
{
	static char		not_set[10]	= "H V W L S";

	int			exit_code;

	char			*TERM		= getenv("TERM");


	if (!TERM || !*TERM || tidbit(TERM, (char *)0) == -1)
		exit (1);

	/*
	 * Very simple calling sequence:
	 *
	 *	lpset horz-pitch vert-pitch width length char-set
	 *
	 * The first four can be scaled with 'i' (inches) or
	 * 'c' (centimeters). A pitch scaled with 'i' is same
	 * as an unscaled pitch.
	 * Blank arguments will skip the corresponding setting.
	 */
	if (argc != 6)
		exit (1);

	exit_code = 0;

	if (argv[1][0]) {
		switch (set_pitch(argv[1], 'H', 1)) {
		case E_SUCCESS:
			not_set[0] = ' ';
			break;
		case E_FAILURE:
			break;
		default:
			exit_code = 1;
			break;
		}
	} else
		not_set[0] = ' ';

	if (argv[2][0]) {
		switch (set_pitch(argv[2], 'V', 1)) {
		case E_SUCCESS:
			not_set[2] = ' ';
			break;
		case E_FAILURE:
			break;
		default:
			exit_code = 1;
			break;
		}
	} else
		not_set[2] = ' ';

	if (argv[3][0]) {
		switch (set_size(argv[3], 'W', 1)) {
		case E_SUCCESS:
			not_set[4] = ' ';
			break;
		case E_FAILURE:
			break;
		default:
			exit_code = 1;
			break;
		}
	} else
		not_set[4] = ' ';

	if (argv[4][0])
	{
	  if ( strcmp( argv[4], "default") != 0)
	  {
		switch (set_size(argv[4], 'L', 1))
		{
		case E_SUCCESS:
			not_set[6] = ' ';
			break;
		case E_FAILURE:
			break;
		default:
			exit_code = 1;
			break;
		}
	  }
	  else
	  {
	    /* MR ul94-28782 - Set default page length,
	     * note: this is required for AP9210 printers only
	     */
	    switch ( set_size( "", 'L', 1)) 
	    {
	      case E_SUCCESS:
	      {
		not_set[6] = ' ';
		break;
	      }
	      case E_FAILURE:
	      {
		break;
	      }
	      default:
	      {
		exit_code = 1;
		break;
	      }
	    }
	  }
	} 
	else
		not_set[6] = ' ';

	if (argv[5][0]) {
		switch (set_charset(argv[5], 1, TERM)) {
		case E_SUCCESS:
			not_set[8] = ' ';
			break;
		case E_FAILURE:
			break;
		default:
			exit_code = 1;
			break;
		}
	} else
		not_set[8] = ' ';

	(void) fprintf (stderr, "%s\n", not_set);

	exit (exit_code);
	/*NOTREACHED*/
}
