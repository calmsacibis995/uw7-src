/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/showmail.c	1.7.3.5"

#include	<fcntl.h>
#include	<string.h>
#include	<curses.h>
#include	"wish.h"
#include	"vt.h"
#include	"vtdefs.h"
#include        "inc.types.h"   /* abs s14 */

void
showmail(force)
bool	force;
{
	register bool	status;
        static char     mailid[] = ":282";
	static char	mail[]   = "MAIL";
	static char	blanks[10]; /* estimated length of any word for mail */
	static bool	last_status;
	static long	last_check;
	extern time_t	Cur_time;	/* EFT abs k16 */
	extern int	Mail_col;
	extern long	Mail_check;
	extern char	*Mail_file;

	extern char	*gettxt();

        int ct, i18n_len;
        char *i18n_mail;
    

	if (Mail_file == NULL)
		return;
	if (force || Cur_time - last_check >= Mail_check) {
		register int	fd;
		register vt_id	oldvid;
		char	buf[8];

/* Is there an easier way ??? */
		status = ((fd = open(Mail_file, O_RDONLY)) >= 0 && read(fd, buf, sizeof(buf)) == sizeof(buf) && strncmp(buf, "Forward ", sizeof(buf)));
		if (fd >= 0)
			close(fd);
/* ??? */
		if (status == last_status)
			return;
		last_status = status;
/* new */
		{
		WINDOW		*win;
	
		win = VT_array[ STATUS_WIN ].win;
                i18n_mail = gettxt (mailid,mail);
                i18n_len = strlen (i18n_mail);
                for (ct = 0; ct < i18n_len; ++ct)
                    blanks[ct] = ' ';
                blanks[ct] = '\0';
		mvwaddstr( win, 0, Mail_col, status ? i18n_mail : blanks );
		if ( status )
			beep();
		}
/***/
/*
		oldvid = vt_current(STATUS_WIN);
		wgo(0, Mail_col);
		wputs(status ? mail : blanks, NULL);
		if (status)
			beep();
		vt_current(oldvid);
*/
	}
	last_check = Cur_time;
}
