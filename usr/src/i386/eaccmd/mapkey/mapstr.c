#ident	"@(#)mapstr.c	1.5"
#include <ctype.h>
#include <sys/types.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <stdio.h>
#include <errno.h>

struct str_dflt buf;
struct fkeyarg fkey;
extern int optind;

#define DFLT "/usr/lib/keyboard/strings"

char *printc(), *prints();
main(Narg,ppCarg)
int Narg;
char **ppCarg;
{
   int i;
   FILE *pF;
   char *mapfile;
   stridx_t stridx;
   int kern,global,errflg;
   int arg;

   global = kern = errflg = 0;
   mapfile=DFLT;

   while( EOF != (arg = getopt( Narg, ppCarg, "dg")) ){
      switch(arg){
         case 'd':
            kern = 1;
            break;
         case 'g':
            global = 1;
            break;
         default:
            fprintf(stderr,"usage: %s [-dg] [file]\n",ppCarg[0]);
            exit(1);
         }
   }
   if (ioctl(0, KIOCINFO, 0) < 0 ) {
      fprintf(stderr,"mapstr can only be run from a virtual terminal\n");
      fprintf(stderr,"on a graphics workstation.\n");
      fprintf(stderr,"usage: %s [-dg] [file]\n",ppCarg[0]);
      errflg++;
      
   }
   if( kern ){
      if( optind < Narg ){
         fprintf(stderr,"unknown option %s\n", ppCarg[optind]);
         fprintf(stderr,"usage: %s [-dg] [file]\n",ppCarg[0]);
         errflg++;
      }
   } else {
      if( optind < Narg ){
         mapfile = ppCarg[optind++];
         if (optind < Narg ) {
            fprintf(stderr,"unknown option %s\n", ppCarg[optind]);
            fprintf(stderr,"usage: %s [-dg] [file]\n", ppCarg[0]);
            errflg++;
         }
      }
      if( 0 == (pF = fopen(mapfile,"r"))){
         fprintf(stderr,"can't open %s\n",mapfile);
         errflg++;
      }
   }
   if ( errflg ) exit(errflg);
   if( !kern ){
      if (global) {
         buf.str_direction = KD_DFLTGET;
         if (ioctl(0,KDDFLTSTRMAP,&buf) < 0) {
            perror("mapstr: unable to perform KDDFLTSTRMAP ioctl\n");
            exit(1);
         }
      } else
         if (ioctl (0,GIO_STRMAP,&buf.str_map) < 0) {
             perror("mapstr: unable to perform GIO_STRMAP ioctl\n");
            exit(1);
         }
      strindexreset(&buf.str_map,&stridx[0]);
      for( i = 1 ; i <= K_FUNL - K_FUNF ; i++ ){
         int j;
         for (j=0; j<MAXFK; j++)
            fkey.keydef[j]='\0';
         fkey.keynum = i;
         fscanstr(pF,fkey.keydef,&fkey.flen);
         if (!addstring(&buf.str_map,&stridx[0],i,fkey.keydef,fkey.flen)) {
            fprintf(stderr,"mapstr: Not enough space for function key definitions\n");
            exit(1);
         }
      }
      if (global) {
         buf.str_direction = KD_DFLTSET;
         if( ioctl(0,KDDFLTSTRMAP,&buf) < 0 ) {
            perror("KDDFLTSTRMAP ioctl failure");
            exit(1);
         }
      } else
         if (ioctl(0,PIO_STRMAP,&buf.str_map) < 0) {
            perror("PIO_KEYMAP ioctl failure");
            exit(1);
         }
    } else {
      buf.str_direction = KD_DFLTGET;
      if (global) {
         if (ioctl(0,KDDFLTSTRMAP,&buf) < 0) {
            perror("KDDFLTSTRMAP ioctl failure");
            exit(1);
         }
      } else
         if (ioctl(0,GIO_STRMAP,&buf.str_map) < 0) {
            perror("KDDFLTSTRMAP ioctl failure");
            exit(1);
         }
      strindexreset(&buf.str_map,&stridx[0]);
      fprintf(stdout, "String key values\n");
      for( i = 1 ; i <= K_FUNL - K_FUNF ; i++ ){
         int len;

         if (i == K_FUNL - K_FUNF)
            len = STRTABLN - 1 - stridx[i-1] -1;
         else
            len = stridx[i] - stridx[i-1] -1;
         if (len > 0 ) {
            /*fprintf(stdout,"\"%s\"", prints(&buf.str_map[stridx[i-1]]));*/
            fprintstr(stdout,&buf.str_map[stridx[i-1]],i);
            /*fprintfnt(stdout, i);*/
         }
      }
   }
   exit(0);
}

fscanstr(pF,pCstr, pNchar)
FILE *pF;
char *pCstr;
int *pNchar;
{
   int c;
   int quoteflg;

   *pNchar = 0;
   
   /* read up to the open quote, throwing all the characters away */
   do {
      c= getc(pF);
   } while( c != '\"'  && c != EOF );
   if ( c == EOF ) return;

   /* read up to the closing quote, putting the characters in the string */
   /* quoteflg is used so that a quote \" may be embedded in the key string */
   /* and not taken as a delimiter */
    do {
	if ( (c = readc(pF, &quoteflg)) == EOF ) return;
        if (!isascii(c)) {
	   fprintf(stderr,"illegal non-ASCII Character in hex'%x'\n", c);
	   fprintf(stderr,"configuration aborted\n");
	   exit(1);
	}
       *(pCstr++) = c;
       (*pNchar)++;
	/* If quoteflg set, quote is not a delimiter so continue */
     } while (( c != '"' || quoteflg) && c != EOF);
     *(--pCstr) = '\0'; /* null out the closing quote */
     (*pNchar)--; 	/* adjust the string len */
}
char *Lables[] = { "Function #","Shift Function #","Control Function #",
       "Ctrl/Shft Function #","Home","Up arrow","Page up","-",
       "Left arrow","5","Right arrow","+","End","Down arrow", 
       "Page down","Insert","(null)"};


fprintstr(pF, pCstr, Nchar)
FILE *pF;
char *pCstr;
int Nchar;
{
   if (*pCstr == '\0') return;
   fprintf(pF,"\"%s\"", prints(pCstr));
   if ( Nchar < 13 )
      fprintf(stdout,"\t%s%d\n",Lables[0], Nchar );
   else if ( Nchar < 25 )
      fprintf(stdout,"\t%s%d\n",Lables[1], Nchar-12 );
   else if ( Nchar < 37 )
      fprintf(stdout,"\t%s%d\n",Lables[2], Nchar-24 );
   else if ( Nchar < 49 )
      fprintf(stdout,"\t%s%d\n",Lables[3], Nchar-36 );
   else if ( Nchar < 61 )
      fprintf(stdout,"\t%s\n",Lables[Nchar-45] );
   else if ( Nchar < 97 )
      fprintf(stdout,"\t%s\n",Lables[16] );
}

int
strindexreset(strmap,stridx)
unchar *strmap;
ushort *stridx;
{
   register int strix;      /* Index into string buffer */
   register ushort   key;      /* Function key number */
   register ushort *idxp;
   register unchar *bufp;

   bufp = strmap;
   idxp = stridx;

   bufp[STRTABLN - 1] = '\0';   /* Make sure buffer ends with null */
   strix = 0;         /* Start from beginning of buffer */

   for (key = 0; (int) key < NSTRKEYS; key++) {   
      idxp[key] = strix;   /* Point to start of string */
      while (strix < STRTABLN - 1 && bufp[strix++])
         ;      /* Find start of next string */
   }
}


int
addstring(strmap, stridx, keynum, str, len)
unchar *strmap;
ushort *stridx;
ushort   keynum, len;
unchar   *str;
{
   register int   amount;      /* Amount to move */
   int        i,      		/* Counter */
            cnt,
         oldlen;      		/* Length of old string */
   register unchar   *oldstr,   /* Location of old string in table */
                         *to,   /* Destination of move */
                       *from;   /* Source of move */
   unchar       *bufend,   	/* End of string buffer */
         	*bufbase,   	/* Beginning of buffer */
         	*tmp;      	/* Temporary pointer into old string */
   ushort       *idxp;

   if ( (int)keynum >= NSTRKEYS)      	/* Invalid key number? */
      return 0;      			/* Ignore string setting */
   len++;            			/* Adjust length to count end null */

   idxp = (ushort *) stridx;
   idxp += keynum - 1;

   oldstr = (unchar *) strmap;
   oldstr += *idxp;

   /* Now oldstr points at beginning of old string for key */

   bufbase = (unchar *) strmap;
   bufend = bufbase + STRTABLN - 1;

   tmp = oldstr;
   while (*tmp++ != '\0')      /* Find end of old string */
      ;

   oldlen =  tmp - oldstr;      /* Compute length of string + null */

   /*
    * If lengths are different, expand or contract table to fit
    */
   if (oldlen > (int) len) {      /* Move up for shorter string? */
      from = oldstr + oldlen;     /* Calculate source */
      to = oldstr + len;   	  /* Calculate destination */
      amount = STRTABLN - (oldstr - bufbase) - oldlen;
      for (cnt = amount; cnt > 0; cnt--)
         *to++ = *from++;
   }
   else 
      if (oldlen < (int) len) { 	   /* Move down for longer string? */
         from = bufend - (len - oldlen);   /* Calculate source */
         to = bufend;      		   /* Calculate destination */
         if (from < (oldstr + len))        /* String won't fit? */
            return 0;   		   /* Return without doing anything */
         amount = STRTABLN - (oldstr - bufbase) - len;   /* Move length */
         while (--amount >= 0)      	   /* Copy whole length */
            *to-- = *from--;   	  	    /* Copy character at a time */
      }

   len--;            /* Remove previous addition for null */
   *(oldstr + len) = '\0';
   while (len--) 
      *oldstr++ = *str++;
   strindexreset(strmap,stridx);      /* Readjust string index table */
   return 1;
}

/*
 *  process characters.  Non-printing characters
 * are converted from printable strings using the
 * standard '\'ing conventions.
 */
readc(fp, quoteflg)
FILE *fp;
int *quoteflg;
{
int c, num, cnt;
   /* quoteflg is used so that a quote \" may be embedded in  */
   /* the key string and is not taken as a delimiter */
    *quoteflg = 0;
    c = getc(fp);
    if ('\\' != c) return(c);
    switch (c=getc(fp)) {
	case '\\' : return('\\'); break;
	case '\'' : return('\''); break;
	case '\"' : *quoteflg = 1;
	    return('"'); break;
	case 'n' : return('\n'); break;
	case 't' : return('\t'); break;
	case 'b' : return('\b'); break;
	case 'r' : return('\r'); break;
	case 'f' : return('\f'); break;
	case 'v' : return('\v'); break;
	case 'a' : return('\a'); break;
	case '7' : case '6' : case '5' : case '4' :
	case '3' : case '2' : case '1' :
	case '0' :  num = 0; cnt=1;
		    do {
			if (c>= '0' && c<='7' && cnt<=3)
			    num = num*8 + (c-'0');
			else {
			     ungetc(c, fp);
			     if (num == '"')
				*quoteflg = 1;
			     return(num);
			}
			c=getc(fp);
			cnt++;
		    } while(1);
	default	: return(c);
    }
}

/*
 *  process strings.  Non-printing characters
 * are converted to printable strings using the
 * standard '\'ing conventions.
 */
char *
prints(sp)
char * sp;
{
    static char ret[1024];
    char *r=ret;

    while(*sp) {
         sprintf(r, "%s", printc(*(sp++)) );
	 r += strlen(r);
    }
    *r='\0';
    return(ret);
}

/*
 *  process characters.  Non-printing characters
 * are converted to printable strings using the
 * standard '\'ing conventions.
 */
char *
printc(k)
int k;
{
    static char ret[256];

    k &= 0xff;
    switch(k) {
       case '\\' : sprintf(ret, "\\\\");  break;
       case '\n' : sprintf(ret, "\\n"); break;
       case '\t' : sprintf(ret, "\\t"); break;
       case '\b' : sprintf(ret, "\\b"); break;
       case '\r' : sprintf(ret, "\\r"); break;
       case '\f' : sprintf(ret, "\\f"); break;
       case '\v' : sprintf(ret, "\\v"); break;
       case '\a' : sprintf(ret, "\\a"); break;
       case '\'' : sprintf(ret, "\\'"); break;
       case '\"' : sprintf(ret, "\\\""); break;
       default   : if (isprint(k))
		      sprintf(ret,"%c",k);
		   else {
			if (k < 010) {
			    sprintf(ret, "\\00%o", k); break;
			}
		    	if (k < 0100) {
                           sprintf(ret, "\\0%o", k); break;
			}
		        sprintf(ret, "\\%o",k); break;
		}
    }
    return(ret);
}
