#ident	"@(#)nas:common/main.h	1.1"
/*
* common/main.h - common assembler interface for main
*			to implementation-dependent stuff
*/

extern const char impdep_optstr[];	/* getopt() options */
extern const char impdep_usage[];	/* usage string */
extern const char impdep_cmdname[];	/* "as" */

		/* implementation provides */
#ifdef __STDC__
void	impdep_option(int);		/* handle imp-dep option */
void	impdep_init(void);		/* initialize imp-dep stuff */
#else
void	impdep_option(), impdep_init();
#endif
