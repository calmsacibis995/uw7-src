/*		copyright	"%c%" 	*/

#ident	"@(#)ext.h	1.2"
#ident	"$Header$"

/*
 *
 * External varible declarations - many are defined in glob.c.
 *
 */


extern char	**argv;			/* global so everyone can use them */
extern int	argc;

extern int	x_stat;			/* program exit status */
extern int	debug;			/* debug flag */
extern int	ignore;			/* what we do with FATAL errors */

extern long	lineno;			/* line number */
extern long	position;		/* byte position */
extern char	*prog_name;		/* and program name - for errors */
extern char	*temp_file;		/* temporary file - for some programs */


extern char	*optarg;		/* for getopt() */
extern int	optind;

extern void	*malloc();
extern void	*calloc();
extern char	*tempnam();
extern char	*strtok();
extern long	ftell();
extern double	atof();
extern double	sqrt();
extern double	atan2();

