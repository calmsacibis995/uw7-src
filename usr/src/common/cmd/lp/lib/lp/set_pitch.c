/*		copyright	"%c%" 	*/

#ident	"@(#)set_pitch.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"
#include "stdlib.h"

#include "lp.h"
#include "lp.set.h"

#if	defined(__STDC__)
int			putp( char * );
#else
int			putp();
#endif

extern char		*tparm();

short			output_res_char		= -1,
			output_res_line		= -1,
			output_res_horz_inch	= -1,
			output_res_vert_inch	= -1;

/**
 ** set_pitch()
 **/

int
#if	defined(__STDC__)
set_pitch (
	char *			str,
	int			which,
	int			putout
)
#else
set_pitch (str, which, putout)
	char			*str;
	int			which,
				putout;
#endif
{
	double			xpi;

	int			ixpi;

        /* Definitions for checking for valid page widths */
        double                  printer_pg_width;
        double                  pg_width_required;
        short                   columns;

	short			*output_res_p,
				*output_res_inch_p;

	unsigned short		xpi_changes_res;

	char			*rest,
				*change_pitch,
				*change_res,
				*p;



	if (which == 'H') {

		(void) tidbit ((char *)0, "cpi", &change_pitch);
		(void) tidbit ((char *)0, "chr", &change_res);

                /* obtain the printer's column value from terminfo */
                (void) tidbit ((char *)0, "cols", &columns);
		output_res_inch_p = &output_res_horz_inch;
		if (output_res_horz_inch == -1)
			(void) tidbit ((char *)0, "orhi", output_res_inch_p);

		output_res_p = &output_res_char;
		if (output_res_char == -1)
			(void) tidbit ((char *)0, "orc", output_res_p);

		(void) tidbit ((char *)0, "cpix", &xpi_changes_res);

	} else {


		(void) tidbit ((char *)0, "lpi", &change_pitch);;
		(void) tidbit ((char *)0, "cvr", &change_res);;
		
		output_res_inch_p = &output_res_vert_inch;
		if (output_res_vert_inch == -1)
			(void) tidbit ((char *)0, "orvi", output_res_inch_p);

		output_res_p = &output_res_line;
		if (output_res_line == -1)
			(void) tidbit ((char *)0, "orl", output_res_p);

		(void) tidbit ((char *)0, "lpix", &xpi_changes_res);;

	}

	xpi = strtod(str, &rest);

        /* check that the page width requested is within the printers page
         * width restrictions. If not, set to previous cpi of printer.
         */

	if (which == 'H') {
         printer_pg_width =   columns/(double)10; /* Obtain pg width in inches */
         pg_width_required =  columns/xpi; /* Obtain pg width to use new cpi */
         
        if (pg_width_required > printer_pg_width)
	    if (change_pitch != NULL)
        	xpi = strtod(change_pitch, &rest);
        }
           /* End code to check cpi against pg width */
          
	if (which == 'H' && STREQU(str, NAME_PICA))
		ixpi = R(xpi = 10);

	else if (which == 'H' && STREQU(str, NAME_ELITE))
		ixpi = R(xpi = 12);

	else if (
		which == 'H'
	     && (
			STREQU(str, NAME_COMPRESSED)
		     || xpi >= N_COMPRESSED
		)
	) {
		if (change_pitch) {

			for (ixpi = MAX_COMPRESSED; ixpi; ixpi--)
				if ((p = tparm(change_pitch, ixpi)) && *p)
					break;
			if (!ixpi)
				ixpi = 10;
			xpi = (double)ixpi;

		} else if (change_res && *output_res_inch_p != -1) {

			for (xpi = MAX_COMPRESSED; xpi >= 1.; xpi -= 1.)
				if (
			(p = tparm(change_res, R(*output_res_inch_p / xpi)))
				     && *p
				)
					break;
			if (xpi < 1.)
				xpi = 10.;
			ixpi = R(xpi);

		} else
			return (E_FAILURE);

	} else {

		if (xpi <= 0)
			return (E_BAD_ARGS);

		switch (*rest) {
		case ' ':
		case 0:
			break;
		case 'c':
			/*
			 * Convert to [lines|chars] per inch.
			 */
			xpi *= 2.54;
			/* fall through */
		/* FALLTHRU */
		case 'i':
			break;
		default:
			return (E_BAD_ARGS);
		}

		ixpi = R(xpi);

	}

	if (
		change_pitch
	     && (p = tparm(change_pitch, ixpi))
	     && *p
	) {

		if (putout)
			putp (p);
		if (xpi_changes_res) {
			if (*output_res_inch_p != -1)
				*output_res_inch_p = R(*output_res_p * xpi);
		} else {
			if (*output_res_p != -1)
				*output_res_p = R(*output_res_inch_p / xpi);
		}
		return (E_SUCCESS);

	} else if (
		change_res
	     && *output_res_inch_p != -1
	     && (p = tparm(change_res, R(*output_res_inch_p / xpi)))
	     && *p
	) {

		if (putout)
			putp (p);
		if (*output_res_p != -1)
			*output_res_p = R(*output_res_inch_p / xpi);
		return (E_SUCCESS);

	} else

		return (E_FAILURE);
}
