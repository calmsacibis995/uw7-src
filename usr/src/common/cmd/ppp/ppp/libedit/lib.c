#ident "@(#)lib.c	1.2"
#ident "$Header$"

#include <termios.h>
#include "edit.h"
#include "history.h"

struct termios tios, save_tios;

char opt_flag;
char *main_prompt = "kdb>> ";
char ed_errbuf[IOBSIZE+1];
static int edit_raw = 0;

edit_cooked_mode()
{
	if (edit_raw)
		tcsetattr(0, TCSANOW, &save_tios);
	edit_raw = 0;
}

edit_raw_mode()
{
	if (!edit_raw) {
		tcgetattr(0, &save_tios);
		tios = save_tios;
		tios.c_cc[VMIN] = 1;
		tios.c_cc[VTIME] = 0;
		tios.c_cc[VINTR] = 0;
		tios.c_cc[VSUSP] = 0;
		tios.c_lflag &= ~(ECHO | ICANON);
		tios.c_iflag &= ~IXON;
		tcsetattr(0, TCSANOW, &tios);
	}
	edit_raw = 1;
}

void
edit_mode(int mode)
{
	opt_flag = mode;
}

int
edit_init(int mode)
{
	hist_init();
	opt_flag = mode;
}

/*
 * read routine with edit modes
 */

int
edit_read(char *buff, int n, char *prompt)
{
	int r, flag;

	pr_prompt(prompt);

	flag = opt_flag & EDITMASK;
	switch(flag) {
		case EMACS:
		case GMACS:
			r = emacs_read(buff,n);
			break;
		case VIRAW:
		case EDITVI:
			r = vi_read(buff,n);
			break;
		default:
			printf("Bad editting mode\n");
			exit(1);
	}

	buff[r] = '\0';
	hist_add(buff);
	return(r);
}


/*
 * print a prompt
 */
pr_prompt(char *string)
{
	int c;
	char *dp = editb.e_prbuff;
	while(c= *string++) {
		if(dp < editb.e_prbuff+PRSIZE)
			*dp++ = c;
		putchar(c);
	}
	*dp = 0;
}

debug_read(char *buf, int n)
{
	*buf = getchar();
	return(1);
}
