/*	copyright	"%c%"	*/

#ident	"@(#)sed:sed.h	1.11.1.8"
#ident "$Header$"
/*
 * sed -- stream  editor
 */


#include	<stdlib.h>
#include	<limits.h>
#include 	<regex.h>
#define	multibyte	(MB_CUR_MAX > 1)
#define MULTI_BYTE_MAX MB_LEN_MAX
#define CEND	16	/* when address is '$' */
#define CLNUM	14	/* when address is number */
#define SUBBOL	1	/* when RE is /^.../ */
#define SUBANY	0	/* when RE is /.../ */

#define DEPTH   20
#define PTRSIZE 200
#define RESIZE  1000
#define ABUFSIZE        20
#define LABSIZE 50
#if defined(RE_DUP_MAX) && LINE_MAX < RE_DUP_MAX
#define LBSIZE	(((RE_DUP_MAX - 1) / 1024 + 1) * 1024)
#else
#define	LBSIZE	LINE_MAX
#endif

extern union reptr     *abuf[];
extern union reptr **aptr;
extern char    genbuf[];
extern char    *lbend;
extern long    lnum;
extern char    linebuf[];
extern char    holdsp[];
extern char    *spend;
extern char    *hspend;
extern int     nflag;
extern int     dopfl;
extern long    *tlno;
extern size_t  nbra;

#define ACOM    01
#define BCOM    020
#define CCOM    02
#define CDCOM   025
#define CNCOM   022
#define COCOM   017
#define CPCOM   023
#define DCOM    03
#define CECOM   015
#define EQCOM   013
#define FCOM    016
#define GCOM    027
#define CGCOM   030
#define HCOM    031
#define CHCOM   032
#define ICOM    04
#define LCOM    05
#define NCOM    012
#define PCOM    010
#define QCOM    011
#define RCOM    06
#define SCOM    07
#define TCOM    021
#define WCOM    014
#define CWCOM   024
#define YCOM    026
#define XCOM    033


union   reptr {
        struct reptr1 {
                char    *ad1;
                char    *ad2;
                char    *re1;
                char    *rhs;
                FILE    *fcode;
                int    gfl;
                char    command;
                char    pfl;
                char    inar;
                char    negfl;
        } r1;
        struct reptr2 {
                char    *ad1;
                char    *ad2;
                union reptr     *lb1;
                char    *rhs;
                FILE    *fcode;
                int    gfl;
                char    command;
                char    pfl;
                char    inar;
                char    negfl;
        } r2;
};
extern union reptr ptrspace[];



struct label {
        char    asc[9];
        union reptr     *chain;
        union reptr     *address;
};



extern int     eargc;

extern union reptr     *pending;
extern char    *badp, *cp;
extern char *respace, *reend;

#ifdef __STDC__
extern const char TMMES[];
extern const char BADOPEN[];
#else
extern char TMMES[];
extern char BADOPEN[];
#endif

char    *compile();
char    *ycomp();
char    *address();
char    *text();
char    *compsub();
struct label    *search();
char    *gline();
char    *place();
