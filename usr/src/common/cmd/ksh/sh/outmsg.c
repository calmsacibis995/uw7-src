/*		copyright	"%c%"	*/

#ident	"@(#)ksh:sh/outmsg.c	1.1.2.2"

/*
	FileName : outmsg.c
*/
#include <pfmt.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/euc.h>
#include <getwidth.h>
#include <errno.h>
#include "outmsg.h"

/*
 * K Shell
 * Add these functions for messaging
 *
*/

static int	flag=KSH_UNDEF;
static const char S_libc[]={"uxlibc"};

/*--------------------------------------------------------------------
 * Function : set flag for [pfmt]
 *--------------------------------------------------------------------*/
void set_flag(int e_flag)
{
   if (e_flag == KSH_CLEAR_N)	flag = MM_NOSTD;
   else if (flag==KSH_UNDEF)
   {
      switch(e_flag)
      {
	case KSH_ERROR:		flag = MM_ERROR; break;

	case KSH_NOSTD:		flag = MM_NOSTD; break;
 
	case KSH_WARNING:	flag = MM_WARNING; break;

	case KSH_ACTION:	flag = MM_ACTION; break;

	case KSH_INFO:		flag = MM_INFO; break;
      }
   }
}

/*--------------------------------------------------------------------
 * Function : pickup signal messages
 *--------------------------------------------------------------------*/
char *err_no(int func,int no,char *dflt)
{
   char	 	  *msg;
   static char	   txtbuf[256];

   msg = txtbuf;
   switch(func)
   {
	case SIGNAL_M:
	   sprintf(txtbuf,"%s:%d",S_libc,no+ER_EVNTBASE);
	   msg = sh_gettxt(txtbuf,dflt);
	   break;

	case SIGNO_M:
	   strcpy(msg,sh_gettxt(S_SIGMSG));
	   strcat(msg,dflt);
	   break;
   }
   return msg;
}

/*--------------------------------------------------------------------
 * Function : output messages to output device
 *--------------------------------------------------------------------*/
#define	MSG_ACTION	gettxt("uxlibc:73", "TO FIX")
#define	MSG_ERROR	gettxt("uxlibc:74", "ERROR")
#define	MSG_HALT	gettxt("uxlibc:75", "HALT")
#define	MSG_WARNING	gettxt("uxlibc:76", "WARNING")
#define	MSG_INFO	gettxt("uxlibc:77", "INFO")

int erwrite(
int	unit,		/* (I) output device		*/
char	*buf,		/* (I) output message		*/
int	no)		/* (I) N(output message)	*/
{
   int	tmp;
   FILE	*fp;

   if ((unit==1 || unit==2) && (flag != KSH_UNDEF && flag != MM_NOSTD))
   {
	/* simulate partial pfmt functionality */
	extern char	*__pfmt_label;
	char		*severity = NULL;

	if (flag == KSH_UNDEF) flag = MM_NOSTD;

	if (flag & MM_NOSTD)
	{ flag = KSH_UNDEF; return write(unit, buf, no); }

	write(unit, "UX:ksh: ", 8);

	switch(flag & 0xf)
	{
	case MM_ACTION: severity = MSG_ACTION; break;
	case MM_ERROR: severity = MSG_ERROR; break;
	case MM_HALT: severity = MSG_HALT; break;
	case MM_WARNING: severity = MSG_WARNING; break;
	case MM_INFO: severity = MSG_INFO; break;
	}
	if (severity)
	{ write(unit, severity, strlen(severity)); write(unit, ": ", 2); }

	write(unit, buf, no);

	flag = KSH_UNDEF;
	return 0;
   } else {
	flag = KSH_UNDEF;
	return write(unit,buf,no);
   }
}

/*--------------------------------------------------------------------
 * Function : flush and clear FILE area to reuse
 *--------------------------------------------------------------------*/
static void fclose_(FILE *fp)
{
   fflush(fp);
/*
   fp->_base	= 0;
   fp->_ptr	= 0;
   fp->_cnt	= 0;
   fp->_flag	= 0;
*/
}

/*--------------------------------------------------------------------
 * Function : call gettxt
 *--------------------------------------------------------------------*/
char *sh_gettxt(char *msgid,char *d_msg)
{
   int	tmp;
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
