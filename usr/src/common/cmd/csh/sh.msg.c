/*		copyright	"%c%"	*/

#ident	"@(#)csh:common/cmd/csh/sh.msg.c	1.7.2.1"

/*
	FileName : sh.msg.c
*/
#include <stdio.h>
#include <pfmt.h>
#include <string.h>
#include <sys/euc.h>
#include <getwidth.h>
#include <fcntl.h>
#include "sh.h"

/*
 * C Shell
 * Add these functions to internationalize of messages
 *
 * Caution:
 *   This file include stdio.h, but other files never do.
 *   Because Csh has builtin printf,fflush and other same name
 *   functions of stdio.
*/

extern char	S_libc[];
int	pfmt_flag=CSH_UNDEF;

/*--------------------------------------------------------------------
 * Function : set flag for [pfmt]
 *--------------------------------------------------------------------*/
void set_flag(int e_flag)
{
   if (e_flag == CSH_CLEAR_N)	pfmt_flag = MM_NOSTD;
   else if (pfmt_flag==CSH_UNDEF)
   {
      switch(e_flag)
      {
	case CSH_ERROR:		pfmt_flag = MM_ERROR; break;

	case CSH_NOSTD:		pfmt_flag = MM_NOSTD; break;
 
	case CSH_WARNING:	pfmt_flag = MM_WARNING; break;

	case CSH_ACTION:	pfmt_flag = MM_ACTION; break;

	case CSH_INFO:		pfmt_flag = MM_INFO; break;
      }
   }
}

/*--------------------------------------------------------------------
 * Function : pickup signal messages
 *--------------------------------------------------------------------*/
char *err_no(int func,int no)
{
   char	 	  *msg;
   static char	   txtbuf[256];

   msg = txtbuf;
   switch(func)
   {
	case SIGNAL_M:
	   sprintf(txtbuf,"%s:%d",S_libc,no+ER_EVNTBASE);
	   msg = gettxt(txtbuf,mesg[no].pname);
	   break;

	case EXIT_M:
	   sprintf(txtbuf,gettxt(MSG_EXIT),no);
	   break;

	case BUG_M:
	   sprintf(txtbuf,gettxt(ER_BUG_STTS),no);
	   break;
   }
   return msg;
}

/*--------------------------------------------------------------------
 * Function : output messages to output device
 *--------------------------------------------------------------------*/
void erwrite(
int	unit,		/* (I) output device		*/
char	*buf,		/* (I) output message		*/
int	no)		/* (I) N(output message)	*/
{
   int	tmp = errno;
   int	fd = -1;
   FILE	*fp;

   if (pfmt_flag != CSH_UNDEF
   && (unit == SHOUT || unit == 1 || unit == FSHOUT || 
       unit == SHDIAG || unit == 2 || unit == FSHDIAG)
   && (fd = dup(unit)) >= 0
   && (fp = fdopen(fd, fcntl(fd, F_GETFL) & O_APPEND? "a" : "w")) != NULL)
   {
#ifdef DEBUG
        fprintf(fp, "fp=%p unit=%d fd=%d", fp, unit, fd);
        fprintf(fp, "error=%p:%d tmp=%d\n", &errno, errno, tmp);
#endif
	buf[no]='\0';
	pfmt(fp,pfmt_flag,MSG_OUTSTR_F,buf);
	fclose(fp);
   } else {
	write(unit,buf,no);

	if (fd >= 0)
		close(fd);
   }
   pfmt_flag = CSH_UNDEF;
   errno = tmp;
}

/*--------------------------------------------------------------------
 * Function : if you need to save "errno", use this function without gettxt
 *--------------------------------------------------------------------*/
char *sh_gettxt(char *msgid,char *d_msg)
{
   int  tmp;
   char *msg;

   tmp = errno;
   msg = gettxt(msgid,d_msg);
   errno = tmp;
   return msg;
}

static short int eucw1, eucw2, eucw3;
static short int scrw1, scrw2, scrw3;
static eucwidth_t codewidth;

/*--------------------------------------------------------------------
 * Function : get character width
 *--------------------------------------------------------------------*/
sh_getwidth()
{
	getwidth(&codewidth);
	eucw1 = codewidth._eucw1;
	eucw2 = codewidth._eucw2;
	eucw3 = codewidth._eucw3;
	scrw1 = codewidth._scrw1;
	scrw2 = codewidth._scrw2;
	scrw3 = codewidth._scrw3;
}

/*--------------------------------------------------------------------
 * Function : return screen length of string
 *--------------------------------------------------------------------*/
int scrlen(char *s)
{
	int scrw;
	unsigned char c;

	if (!codewidth._multibyte)
		return strlen(s);
	else {
		scrw = 0;
		while ((c = (unsigned char)*s) != 0) {
 			if (c < 0x80) {				/* ASCII char */
				s++;
				scrw++;
			} else if (c == SS2 || c == SS3) {	/* SS2/3 char */
				s += 1 + ((c == SS2) ? eucw2 : eucw3);
				scrw += (c == SS2) ? scrw2 : scrw3;
			} else if (c >= 0240) {			/* SS1 char */
				s += eucw1;
				scrw += scrw1;
			} else {				/* C1 char */
				s++;
				scrw++;
			}
		}
	}
	return scrw;
}
