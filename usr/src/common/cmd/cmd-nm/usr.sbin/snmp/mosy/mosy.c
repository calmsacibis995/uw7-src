#ident	"@(#)mosy.c	1.2"
#ident   "$Header$"
#ifndef lint
static char TCPID[] = "@(#)mosy.c 1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
#ifndef lint
static char SysVr3TCPID[] = "@(#)mosy.c     6.1 Lachman System V STREAMS TCP source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/* mosy.c - Managed Object Syntax-compiler (yacc-based) */

/*
 *      System V STREAMS TCP - Release 5.0
 *
 *  Copyright 1992 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 * 
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <ctype.h>
#include <stdio.h>
#include <varargs.h>
#include <locale.h>
#include <unistd.h>

#include "mosy-defs.h"

char *mosyversion = "mosy 8.0 #1  Tue Dec 1 15:14:16 CST 1992";

int Cflag = 0;          /* mosy */
int dflag = 0;
int Pflag = 0;          /* pepy compat... */
int doexternals;
static int linepos = 0;
static int mflag = 0;
static int mosydebug = 0;
static int sflag = 0;
static int delay = 0;
static int iflag = 0;

static  char *eval = NULLCP;

char   *mymodule = "";
OID mymoduleid;

int yysection = 0;
char *yyencpref = "none";
char *yydecpref = "none";
char *yyprfpref = "none";
char *yyencdflt = "none";
char *yydecdflt = "none";
char *yyprfdflt = "none";

static char *yymode = "";

static char autogen[BUFSIZ];

char *sysin = NULLCP;
static char sysout[BUFSIZ];

typedef struct yoi 
   {
   char  *yi_name;

   YV    yi_value;
   } yoi, *OI;

#define  NULLOI    ((OI) 0)


typedef struct yot 
   {
   char  *yo_name;

   YP    yo_syntax;
   YV    yo_value;

   char  *yo_access;
   char  *yo_status;

   char  *yo_descr;
   char  *yo_refer;
   YV    yo_index;
   YV    yo_defval;
   } yot, *OT;

#define  NULLOT    ((OT) 0)


typedef struct ytt 
   {
   char  *yt_name;

   YV    yt_enterprise;
   int   yt_number;

   YV    yt_vars;

   char  *yt_descr;
   char  *yt_refer;
   } ytt, *TT;

#define  NULLTT    ((TT) 0)

typedef struct symlist 
   {
   char   *sy_encpref;
   char   *sy_decpref;
   char   *sy_prfpref;
   char   *sy_module;
   char   *sy_name;
   union 
      {
      OT sy_un_yo;

      TT sy_un_yt;

      OI sy_un_yi;

      YP sy_un_yp;
      } sy_un;

#define  sy_yo     sy_un.sy_un_yo
#define  sy_yt     sy_un.sy_un_yt
#define  sy_yi     sy_un.sy_un_yi
#define  sy_yp     sy_un.sy_un_yp

      struct symlist *sy_next;
      } symlist, *SY;

#define  NULLSY    ((SY) 0)

static   SY   myidentifiers = NULLSY;
static   SY   myobjects = NULLSY;
static   SY   mytraps = NULLSY;
static   SY   mytypes = NULLSY;


SY  new_symbol(), add_symbol();

char   *id2str();

YP  lookup_type();
char   *val2str();

OI  lookup_identifier();
OT  lookup_object();

/* MAIN */

/* ARGSUSED */

main(int argc, char  **argv, char  **envp)
   {
   register char  *cp;
   register char  *sp;

   (void)setlocale(LC_ALL, "");
   (void)setcat("nmmosy");
   (void)setlabel("NM:mosy");

   fprintf(stderr, gettxt(":2", "Version: %s\n"), mosyversion);

   sysout[0] = '\0';

   for(argc--, argv++; argc > 0; argc--, argv++) 
        {
        cp = *argv;

        if(strcmp(cp, "-i") == 0) 
             {
             iflag++;
             continue;
             }

        if(strcmp(cp, "-d") == 0) 
             {
             dflag++;
             continue;
             }

        if(strcmp(cp, "-m") == 0) 
             {
             mflag++;
             continue;
             }

        if(strcmp(cp, "-o") == 0) 
             {
             if(sysout[0]) 
                  {
            fprintf(stderr, gettxt(":3", "Too many output files.\n"));
                  exit(1);
                  }
       
             argc--, argv++;
       
             if((cp = *argv) == NULL || (*cp == '-' && cp[1] != '\0'))
                  goto usage;

             (void) strcpy(sysout, cp);
             continue;
             }

        if(strcmp(cp, "-s") == 0) 
             {
             sflag++;
             continue;
             }

        if(sysin) 
             {
usage: ;
             fprintf(stderr,
                  gettxt(":1", "Usage: mosy [-d] [-o module.defs] [-s] module.my\n"));
             exit(1);
             }

        if(*cp == '-') 
             {
             if(*++cp != '\0')
                  goto usage;

             sysin = "";
             }

        sysin = cp;

        if(sysout[0])
             continue;

        if(sp = (char *) strrchr(cp, '/'))
             sp++;

        if(sp == NULL || *sp == '\0')
             sp = cp;

        sp += strlen(cp = sp) - 3;

        if(sp > cp && strcmp(sp, ".my") == 0)
             (void) sprintf(sysout, "%.*s.defs", sp - cp, cp);
        else
             (void) sprintf(sysout, "%s.defs", cp);
        }

   switch(mosydebug = (cp = (char *)getenv("MOSYTEST")) && *cp ? atoi(cp) : 0) 
        {
        case 2:
             yydebug++;      /* fall */

        case 1:
             sflag++;        /*   .. */

        case 0:
        break;
        }

   if(sysin == NULLCP)
        sysin = "";

   if(*sysin && freopen(sysin, "r", stdin) == NULL) 
        {
         fprintf(stderr, gettxt(":4", "Unable to read.\n")), perror(sysin);
        exit(1);
        }

   if(strcmp(sysout, "-") == 0)
        sysout[0] = '\0';

   if(*sysout && freopen(sysout, "w", stdout) == NULL) 
        {
         fprintf(stderr, gettxt(":5", "Unable to write.\n")), perror(sysout);
        exit(1);
        }

   if(cp = (char *) strchr(mosyversion, ')'))
        for(cp++; *cp != ' '; cp++)
             if(*cp == '\0') 
                  {
                  cp = NULL;
                  break;
                  }
   if(cp == NULL)
        cp = mosyversion + strlen(mosyversion);

   (void) sprintf(autogen, "%*.*s", 
                  cp - mosyversion, cp - mosyversion, mosyversion);
   printf("-- automatically generated by %s, do not edit!\n\n", autogen);

   initoidtbl();

   exit(yyparse());       /* NOTREACHED */
   }

/* ERRORS */

yyerror(register char   *s)
   {
   yyerror_aux(s);

   if(delay) 
        {
        delay = -1;
        return;
        }

   if(*sysout)
        (void) unlink(sysout);

   exit(1);
   }

#ifndef lint
warning(va_alist)
va_dcl
    {
    char buffer[BUFSIZ];
    char buffer2[BUFSIZ];
    char *cp;
    va_list   ap;

    va_start(ap);

    _asprintf(buffer, NULLCP, ap);

    va_end(ap);

    (void) sprintf(buffer2, "Warning: %s", buffer);
    yyerror_aux(buffer2);
    }

#else

/* VARARGS1 */
warning(char *fmt)
   {
   warning(fmt);
   }
#endif

static yyerror_aux(register char *s)
   {
   if(linepos)
      fprintf(stderr, "\n"), linepos = 0;

   if(eval)
      fprintf(stderr, "%s %s:\n    ", yymode, eval);
   else
      fprintf(stderr, gettxt(":6", "line %d: "), yylineno);

   fprintf(stderr, "%s\n", s);

   if(!eval)
      fprintf(stderr, gettxt(":7", "Last token read was \"%s\"\n"), yytext);
   }

#ifndef  lint
myyerror(va_alist)
va_dcl
   {
   char    buffer[BUFSIZ];
   va_list ap;

   va_start(ap);

   _asprintf(buffer, NULLCP, ap);

   va_end(ap);

   yyerror(buffer);
   }
#else
/* VARARGS */

myyerror(char *fmt)
   {
   myyerror(fmt);
   }
#endif

yywrap() 
   {
   if(linepos)
   fprintf(stderr, "\n"), linepos = 0;

   return 1;
   }

/* ARGSUSED */

yyprint(char *s, int f, int top)
   {
   }

static yyprint_aux(char *s, char *mode)
   {
   int      len;
   static int nameoutput = 0;
   static int outputlinelen = 79;

   if(sflag)
   return;

   if(strcmp(yymode, mode)) 
        {
        if(linepos)
             fprintf(stderr, "\n\n");

        fprintf(stderr, "%s", mymodule);
        nameoutput = (linepos = strlen(mymodule)) + 1;
        fprintf(stderr, " %ss", yymode = mode);
        linepos += strlen(yymode) + 1;
        fprintf(stderr, ":");
        linepos += 2;
        }

   len = strlen(s);
   if(linepos != nameoutput)
        if(len + linepos + 1 > outputlinelen)
             fprintf(stderr, "\n%*s", linepos = nameoutput, "");
   else
       fprintf(stderr, " "), linepos++;
   
   fprintf(stderr, "%s", s);
   linepos += len;
   }

/* PASS1 */

pass1()
   {
   printf("-- object definitions compiled from %s", mymodule);

   if(mymoduleid)
        printf(" %s", oidprint(mymoduleid));

   printf("\n\n");
   }

pass1_oid(char *mod, char *id, YV value)
   {
   register SY        sy;
   register OI        yi;

   if((yi = (OI) calloc(1, sizeof *yi)) == NULLOI)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*yi));

   yi -> yi_name = id;
   yi -> yi_value = value;

   if(mosydebug) 
        {
        if(linepos)
             fprintf(stderr, "\n"), linepos = 0;

        fprintf(stderr, "%s.%s\n", mod ? mod : mymodule, id);
        print_yi(yi, 0);
        fprintf(stderr, "--------\n");
        }
   else
        yyprint_aux(id, "identifier");

   sy = new_symbol(NULLCP, NULLCP, NULLCP, mod, id);
   sy -> sy_yi = yi;
   myidentifiers = add_symbol(myidentifiers, sy);
   }

pass1_obj(char *mod, char *id, YP syntax, YV value, char *aname, char *sname,
         char *descr, char *refer, YV idx, YV defval)
   {
   register SY        sy;
   register OT        yo;


   if((yo = (OT) calloc(1, sizeof *yo)) == NULLOT)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*yo));

   yo -> yo_name = id;
   yo -> yo_syntax = syntax;
   yo -> yo_value = value;
   yo -> yo_access = aname;
   yo -> yo_status = sname;
   yo -> yo_descr = descr;
   yo -> yo_refer = refer;
   yo -> yo_index = idx;
   yo -> yo_defval = defval;

   if(mosydebug) 
        {
        if(linepos)
             fprintf(stderr, "\n"), linepos = 0;

        fprintf(stderr, "%s.%s\n", mod ? mod : mymodule, id);
        print_yo(yo, 0);
        fprintf(stderr, "--------\n");
        }
   else
        yyprint_aux(id, "object");

   sy = new_symbol(NULLCP, NULLCP, NULLCP, mod, id);
   sy -> sy_yo = yo;
   myobjects = add_symbol(myobjects, sy);
   }

pass1_trap(char *mod, char *id, YV enterprise, int number, YV vars, char *descr, char *refer)
   {
   register SY        sy;
   register TT        yt;

   if((yt = (TT) calloc(1, sizeof *yt)) == NULLTT)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*yt));

   yt -> yt_name = id;
   yt -> yt_enterprise = enterprise;
   yt -> yt_number = number;
   yt -> yt_vars = vars;
   yt -> yt_descr = descr;
   yt -> yt_refer = refer;

   if(mosydebug) 
        {
        if(linepos)
             fprintf(stderr, "\n"), linepos = 0;

        fprintf(stderr, "%s.%s\n", mod ? mod : mymodule, id);
        print_yt(yt, 0);
        fprintf(stderr, "--------\n");
        }
   else
        yyprint_aux(id, "trap");

   sy = new_symbol(NULLCP, NULLCP, NULLCP, mod, id);
   sy -> sy_yt = yt;
   mytraps = add_symbol(mytraps, sy);
   }

pass1_type(register char  *encpref, register char  *decpref, register 
            char  *prfpref, register char  *mod, register char  *id, register YP yp)
   {
   register SY        sy;

   if(dflag && lookup_type(mod, id))     /* no duplicate entries, please... */
        return;

   if(mosydebug) 
        {
        if(linepos)
             fprintf(stderr, "\n"), linepos = 0;

        fprintf(stderr, "%s.%s\n", mod ? mod : mymodule, id);
        print_type(yp, 0);
        fprintf(stderr, "--------\n");
        }
   else if(!(yp -> yp_flags & YP_IMPORTED))
        yyprint_aux(id, "type");

   sy = new_symbol(encpref, decpref, prfpref, mod, id);
   sy -> sy_yp = yp;
   mytypes = add_symbol(mytypes, sy);
   }

/* PASS2 */

pass2() 
   {
   register SY        sy;

   if(!sflag)
        (void) fflush(stderr);

   delay = 1;

   yymode = "identifier";

   for(sy = myidentifiers; sy; sy = sy -> sy_next) 
      {
      if(sy -> sy_module == NULLCP)
         yyerror(gettxt(":9", "No module name associated with symbol"));

      do_id(sy -> sy_yi, eval = sy -> sy_name);
      }

   if(myidentifiers)
      printf("\n");

   yymode = "object";

   for(sy = myobjects; sy; sy = sy -> sy_next) 
      {
      if(sy -> sy_module == NULLCP)
         yyerror(gettxt(":9", "No module name associated with symbol"));

      do_obj1(sy -> sy_yo, eval = sy -> sy_name);
      }

   if(myobjects)
      printf("\n\n");

   yymode = "trap";

   for(sy = mytraps; sy; sy = sy -> sy_next) 
      {
      if(sy -> sy_module == NULLCP)
         yyerror(gettxt(":9", "No module name associated with symbol"));

      do_trap1(sy -> sy_yt, eval = sy -> sy_name);
      }

   (void) fflush(stdout);

   if(ferror(stdout))
      myyerror(gettxt(":10", "Write error: %s.\n"), strerror(errno));

   if(!iflag && delay < 0)
      exit(1);

   delay = 0;
   }

/* ARGSUSED */

static do_id(register OI yi, char *id)
   {
   printf("%-20s %s\n", yi -> yi_name, id2str(yi -> yi_value));
   }

/* ARGSUSED */

static do_obj1(register OT yo, char *id)
   {
   register YP        yp,
            yz;
   register YV        yv;

      {
      register char *cp;

      for(cp = yo -> yo_name; *cp; cp++)
         if(*cp == '-') 
            {
            warning(gettxt(":11", "Object type %s contains a '-' in its descriptor.\n"),
                     yo -> yo_name);
            break;
            }
      }
   printf("%-20s %-16s ", yo -> yo_name, id2str(yo -> yo_value));

   if((yp = yo -> yo_syntax) == NULLYP) 
      {
      yyerror(gettxt(":12", "No syntax associated with object type"));
      return;
      }

again: ;
   switch(yp -> yp_code) 
      {
      case YP_INTLIST:
         for(yv = yp -> yp_value; yv; yv = yv -> yv_next)
            if(yv -> yv_code != YV_NUMBER)
               yyerror(gettxt(":13", "Value of enumerated INTEGER is not a number"));
            else if(yv -> yv_number == 0)
               yyerror(gettxt(":14", "Value of enumerated INTEGER is zero"));

            for(yv = yp -> yp_value; yv; yv = yv -> yv_next) 
               {
               register YV    v;

               for(v = yv -> yv_next; v; v = v -> yv_next)
                  if(yv -> yv_number == v -> yv_number)
                     myyerror(gettxt(":15", "Duplicate values in enumerated INTEGER: %d.\n"),
                              yv -> yv_number);

               if(yv -> yv_flags & YV_NAMED)
                  for(v = yv -> yv_next; v; v = v -> yv_next)
                     if((v -> yv_flags & YV_NAMED)
                           && strcmp(yv -> yv_named, v -> yv_named) == 0)
                     	myyerror(gettxt(":16", "Duplicate tags in enumerated INTEGER: %d.\n"),
                                 yv -> yv_named);
               }
       /* and fall... */
      case YP_INT:
         id = "INTEGER";
      break;

      case YP_OCT:
         id = "OctetString";
      break;

      case YP_OID:
         id = "ObjectID";
      break;

      case YP_NULL:
         id = "NULL";
      break;

      case YP_SEQTYPE:
         if((yz = yp -> yp_type) -> yp_code != YP_IDEFINED
               || (yz = lookup_type(yz -> yp_module,
               yz -> yp_identifier)) == NULL
               || yz -> yp_code != YP_SEQLIST)
            yyerror(gettxt(":17", "Value of SYNTAX clause isn't SEQUENCE OF Type, where Type::= SEQUENCE {...}"));
       /* and fall... */
      case YP_SEQLIST:
         id = "Aggregate";
         if(strcmp(yo -> yo_access, "not-accessible"))
            yyerror(gettxt(":18", "Value of ACCESS clause isn't not-accessible"));
      break;

      default:
         id = "Invalid";
         yyerror(gettxt(":19", "Invalid value of SYNTAX clause"));
      break;

      case YP_IDEFINED:
         if(yz = lookup_type(yp -> yp_module, id = yp -> yp_identifier)) 
            {
            yp = yz;
            goto again;
            }

         if(strcmp(id, "Counter") == 0
               && yo -> yo_name[strlen(yo -> yo_name) - 1] != 's')
            warning(gettxt(":20", "Descriptor of counter object type doesn't end in 's'.\n"));
      break;
      }

   if(strcmp(yo -> yo_access, "read-only")
         && strcmp(yo -> yo_access, "read-write")
         && strcmp(yo -> yo_access, "write-only")
         && strcmp(yo -> yo_access, "not-accessible"))
      yyerror(gettxt(":21", "Value of ACCESS clause isn't a valid keyword"));

   if(strcmp(yo -> yo_status, "mandatory")
         && strcmp(yo -> yo_status, "optional")
         && strcmp(yo -> yo_status, "obsolete")
         && strcmp(yo -> yo_status, "deprecated"))
      yyerror(gettxt(":22", "Value of STATUS clause isn't a valid keyword"));

   printf("%-15s %-15s %s\n", id, yo -> yo_access, yo -> yo_status);

   if(yo -> yo_index) 
      {
      if(yp -> yp_code != YP_SEQLIST)
         yyerror(gettxt(":23","INDEX clause should not be present"));
      else
         check_objects(yo -> yo_index, "INDEX", 1);
      }

   if(yp -> yp_code == YP_SEQLIST) 
      {
      for(yz = yp -> yp_type; yz; yz = yz -> yp_next) 
         {
         register YP     y = yz;

check_again: ;
         switch(y -> yp_code) 
            {
            case YP_INT:
            case YP_INTLIST:
            case YP_OCT:
            case YP_OID:
            case YP_NULL:
            break;

            case YP_IDEFINED:
               if(y = lookup_type(y -> yp_module, y -> yp_identifier))
                  goto check_again;
            break;

           default:
               yyerror(gettxt(":24", "Invalid element in SEQUENCE"));
               goto done_sequence;
            }

         if(!(yz -> yp_flags & YP_ID)) 
            {
            yyerror(gettxt(":25", "Element in SEQUENCE missing tag"));
            goto done_sequence;
            }

         if(lookup_object(NULLCP, yz -> yp_id) == NULL)
            myyerror(gettxt(":26", "No object type corresonding to tag in SEQUENCE: %s.\n"),
                     yz -> yp_id);
         }
done_sequence: ;   
      }

   if((yv = yo -> yo_value) -> yv_code != YV_OIDLIST) 
      {
      yyerror(gettxt(":27", "Value of object type isn't an object identifier"));
      return;
      }
   for(yv = yv -> yv_idlist; yv; yv = yv -> yv_next)
      if(yv -> yv_code == YV_NUMBER && yv -> yv_number <= 0)
         myyerror(gettxt(":28", "Object identifier contains non-positive element: %d.\n"),
                  yv -> yv_number);

   if((yv = yo -> yo_value -> yv_idlist)
         && yv -> yv_code == YV_IDEFINED
         && yv -> yv_next
         && yv -> yv_next -> yv_code == YV_NUMBER
         && !yv -> yv_next -> yv_next) 
      {
      OT   ot;

      if((ot = lookup_object(NULLCP, yv -> yv_identifier))) 
         {
         yz = ot -> yo_syntax;
         while(yz -> yp_code == YP_IDEFINED)
            if(!(yz = lookup_type(yz -> yp_module, yz -> yp_identifier)))
               break;

         if(yz && yz -> yp_code == YP_SEQLIST)
            for(yz = yz -> yp_type; yz; yz = yz -> yp_next)
               if((yz -> yp_flags & YP_ID)
                     && strcmp(yo -> yo_name, yz -> yp_id) == 0)
                  break;
         if(!yz)
            myyerror(gettxt(":29", "Object type not contained in defining SEQUENCE: %s.\n"),
                     yv -> yv_identifier);
         }
      }
   }

/* ARGSUSED */

static do_trap1(register TT yt, char *id)
   {
   register YV        yv;
   
   if((yv = yt -> yt_enterprise) == NULLYV) 
      {
      yyerror(gettxt(":30", "No enterprise associated with trap type"));
      goto done_enterprise;
      }

   if(yv -> yv_code != YV_OIDLIST) 
      {
      yyerror(gettxt(":31", "Value of ENTERPRISE clause isn't an object identifier"));
      goto done_enterprise;
      }

   for(yv = yv -> yv_idlist; yv; yv = yv -> yv_next)
      if(yv -> yv_code == YV_NUMBER && yv -> yv_number <= 0)
         myyerror(gettxt(":32", "Object identifier in ENTERPRISE clause contains non-positive element: %d"),
                  yv -> yv_number);

   if((yv = yt -> yt_enterprise -> yv_idlist)
         && yv -> yv_code == YV_IDEFINED
         && !yv -> yv_next
         && !lookup_object(NULLCP, yv -> yv_identifier)
         && !lookup_identifier(NULLCP, yv -> yv_identifier)
         && !importedP(yv -> yv_identifier))
      myyerror(gettxt(":33", "Value in ENTERPRISE clause is undefined: %s"),
               yv -> yv_identifier);

done_enterprise: ;

   if(yt -> yt_number < 0)
      myyerror(gettxt(":34", "Value of trap type isn't non-negative integer"));

   if(yt -> yt_vars)
      check_objects(yt -> yt_vars, "VARIABLES", 0);
   }

/* IDENTIFIER HANDLING */

static OI lookup_identifier(register char *mod, register char *id)
   {
   register SY        sy;

   for(sy = myidentifiers; sy; sy = sy -> sy_next) 
      {
      if(mod) 
         {
         if(strcmp(sy -> sy_module, mod))
            continue;
         }
      else if(strcmp(sy -> sy_module, mymodule)
            && strcmp(sy -> sy_module, "UNIV"))
         continue;

      if(strcmp(sy -> sy_name, id) == 0)
         return sy -> sy_yi;
      }

   return NULLOI;
   }

static char *id2str(register YV yv)
   {
   register char *cp, *dp;
   static char buffer[BUFSIZ];

   if(yv -> yv_code != YV_OIDLIST)
      yyerror(gettxt(":35", "Need an object identifer"));
   
   cp = buffer;

   for(yv = yv -> yv_idlist, dp = ""; yv; yv = yv -> yv_next, dp = ".") 
      {
      (void) sprintf(cp, "%s%s", dp, val2str(yv));
      cp += strlen(cp);
      }

   *cp = '\0';

   return buffer;
   }

/* OBJECT HANDLING */

static OT lookup_object(register char *mod, register char *id)
   {
   register SY        sy;

   for(sy = myobjects; sy; sy = sy -> sy_next) 
      {
      if(mod) 
         {
         if(strcmp(sy -> sy_module, mod))
            continue;
         }
      else if(strcmp(sy -> sy_module, mymodule) 
            && strcmp(sy -> sy_module, "UNIV"))
         continue;

      if(strcmp(sy -> sy_name, id) == 0)
         return sy -> sy_yo;
      }

   return NULLOT;
   }

static check_objects(register YV yv, char *clause, int typesOK)
   {
   if(yv -> yv_code != YV_VALIST) 
      {
      myyerror(gettxt(":36", "Value of %s clause is not a list of object types"), clause);
      return;
      }

   for(yv = yv -> yv_idlist; yv; yv = yv -> yv_next) 
      {
      switch(yv -> yv_code) 
         {
         case YV_IDEFINED:
            if(!typesOK && !(yv -> yv_flags & YV_BOUND))
               goto not_a_type;

            if(lookup_object(yv -> yv_module, yv -> yv_identifier) == NULL
                  && !importedP(yv -> yv_identifier))
               myyerror(gettxt(":37", "Element in %s clause is undefined: %s"), clause,
                        yv -> yv_identifier);
         break;

         case YV_NUMBER:
         case YV_STRING:
         case YV_OIDLIST:
            if(typesOK)
         break;

        /* else fall... */
         default:
not_a_type: ;
            myyerror(gettxt(":38", "Element in %s clause is %s"),
                     clause, 
                     typesOK ? "neither an object type nor a data type"
                     : "not an object type");
         break;
         }
      }
   }

/* TYPE HANDLING */

static YP lookup_type(register char *mod, register char *id)
   {
   register SY        sy;

   for(sy = mytypes; sy; sy = sy -> sy_next) 
      {
      if(mod) 
         {
         if(strcmp(sy -> sy_module, mod))
            continue;
         }
      else if(strcmp(sy -> sy_module, mymodule)
            && strcmp(sy -> sy_module, "UNIV"))
         continue;

      if(strcmp(sy -> sy_name, id) == 0)
         return sy -> sy_yp;
      }

   return NULLYP;
   }

/* VALUE HANDLING */

static char *val2str(register YV yv)
   {
   static char buffer[BUFSIZ];

   switch(yv -> yv_code) 
      {
      case YV_BOOL:
         yyerror(gettxt(":39", "Need a sub-identifier, not a boolean"));

      case YV_NUMBER:
         (void) sprintf(buffer, "%d", yv -> yv_number);
      return buffer;

      case YV_STRING:
         yyerror(gettxt(":40", "Need a sub-identifier, not a string"));

      case YV_IDEFINED:
      return yv -> yv_identifier;

      case YV_IDLIST:
         yyerror(gettxt(":41", "Haven't written symbol table for values yet"));

      case YV_VALIST:
         yyerror(gettxt(":42", "Need a sub-identifier, not a list of values"));

      case YV_OIDLIST:
         yyerror(gettxt(":43", "Need a sub-identifier, not an object identifier"));

      case YV_NULL:
         yyerror(gettxt(":44", "Need a sub-identifier, not NULL"));

      default:
         myyerror(gettxt(":45", "Unknown value: %d"), yv -> yv_code);
      }

/* NOTREACHED */
   }

/* DEBUG */

static print_yi(register OI yi, register int level)
   {
   if(yi == NULLOI)
      return;

   fprintf(stderr, "%*sname=%s\n", level * 4, "", yi -> yi_name);

   if(yi -> yi_value) 
      {
      fprintf(stderr, "%*svalue\n", level * 4, "");
      print_value(yi -> yi_value, level + 1);
      }
   }

static print_yo(register OT yo, register int level)
   {
   if(yo == NULLOT)
      return;

   fprintf(stderr, "%*sname=%s\n", level * 4, "", yo -> yo_name);

   if(yo -> yo_syntax) 
      {
      fprintf(stderr, "%*ssyntax\n", level * 4, "");
      print_type(yo -> yo_syntax, level + 1);
      }

   if(yo -> yo_value) 
      {
      fprintf(stderr, "%*svalue\n", level * 4, "");
      print_value(yo -> yo_value, level + 1);
      }
   }

static print_yt(register TT yt, register int level)
   {
   if(yt == NULLTT)
      return;

   fprintf(stderr, "%*sname=%s\n", level * 4, "", yt -> yt_name);

   if(yt -> yt_enterprise) 
      {
      fprintf(stderr, "%*senterprise\n", level * 4, "");
      print_value(yt -> yt_enterprise, level + 1);
      fprintf(stderr, "%*s    number=%d\n", level * 4, "", yt -> yt_number);
      }

   if(yt -> yt_vars) 
      {
      fprintf(stderr, "%*svar\n", level * 4, "");
      print_value(yt -> yt_vars, level + 1);
      }
   }

static print_type(register YP yp, register int level)
   {
   register YP        y;
   register YV        yv;

   if(yp == NULLYP)
      return;

   fprintf(stderr, "%*scode=0x%x flags=%s\n", level * 4, "",
            yp -> yp_code, sprintb(yp -> yp_flags, YPBITS));
   fprintf(stderr,
            "%*sintexp=\"%s\" strexp=\"%s\" prfexp=%c declexp=\"%s\" varexp=\"%s\"\n",
            level * 4, "", yp -> yp_intexp, yp -> yp_strexp, yp -> yp_prfexp,
            yp -> yp_declexp, yp -> yp_varexp);
   fprintf(stderr,
            "%*sstructname=\"%s\" ptrname=\"%s\"\n", level * 4, "",
            yp -> yp_structname, yp -> yp_ptrname);

   if(yp -> yp_action0)
      fprintf(stderr, "%*saction0 at line %d=\"%s\"\n", level * 4, "",
               yp -> yp_act0_lineno, yp -> yp_action0);

   if(yp -> yp_action1)
      fprintf(stderr, "%*saction1 at line %d=\"%s\"\n", level * 4, "",
               yp -> yp_act1_lineno, yp -> yp_action1);

   if(yp -> yp_action2)
      fprintf(stderr, "%*saction2 at line %d=\"%s\"\n", level * 4, "",
               yp -> yp_act2_lineno, yp -> yp_action2);

   if(yp -> yp_action3)
      fprintf(stderr, "%*saction3 at line %d=\"%s\"\n", level * 4, "",
               yp -> yp_act3_lineno, yp -> yp_action3);

   if(yp -> yp_flags & YP_TAG) 
      {
      fprintf(stderr, "%*stag class=0x%x value=0x%x\n", level * 4, "",
               yp -> yp_tag -> yt_class, yp -> yp_tag -> yt_value);
      print_value(yp -> yp_tag -> yt_value, level + 1);
      }

   if(yp -> yp_flags & YP_DEFAULT) 
      {
      fprintf(stderr, "%*sdefault=0x%x\n", level * 4, "", yp -> yp_default);
      print_value(yp -> yp_default, level + 1);
      }

   if(yp -> yp_flags & YP_ID)
      fprintf(stderr, "%*sid=\"%s\"\n", level * 4, "", yp -> yp_id);

   if(yp -> yp_flags & YP_BOUND)
      fprintf(stderr, "%*sbound=\"%s\"\n", level * 4, "", yp -> yp_bound);

   if(yp -> yp_offset)
      fprintf(stderr, "%*soffset=\"%s\"\n", level * 4, "", yp -> yp_offset);

   switch(yp -> yp_code) 
      {
      case YP_INTLIST:
      case YP_BITLIST:
         fprintf(stderr, "%*svalue=0x%x\n", level * 4, "", yp -> yp_value);

         for(yv = yp -> yp_value; yv; yv = yv -> yv_next) 
            {
            print_value(yv, level + 1);
            fprintf(stderr, "%*s----\n", (level + 1) * 4, "");
            }
      break;

      case YP_SEQTYPE:
      case YP_SEQLIST:
      case YP_SETTYPE:
      case YP_SETLIST:
      case YP_CHOICE:
         fprintf(stderr, "%*stype=0x%x\n", level * 4, "", yp -> yp_type);

         for(y = yp -> yp_type; y; y = y -> yp_next) 
            {
            print_type(y, level + 1);
            fprintf(stderr, "%*s----\n", (level + 1) * 4, "");
            }
      break;

      case YP_IDEFINED:
         fprintf(stderr, "%*smodule=\"%s\" identifier=\"%s\"\n",
                  level * 4, "", yp -> yp_module ? yp -> yp_module : "",
                  yp -> yp_identifier);
      break;

      default:
      break;
      }
   }

static print_value(register YV yv, register int level)
   {
   register YV        y;

   if(yv == NULLYV)
      return;

   fprintf(stderr, "%*scode=0x%x flags=%s\n", level * 4, "",
            yv -> yv_code, sprintb(yv -> yv_flags, YVBITS));

   if(yv -> yv_action)
      fprintf(stderr, "%*saction at line %d=\"%s\"\n", level * 4, "",
               yv -> yv_act_lineno, yv -> yv_action);

   if(yv -> yv_flags & YV_ID)
      fprintf(stderr, "%*sid=\"%s\"\n", level * 4, "", yv -> yv_id);

   if(yv -> yv_flags & YV_NAMED)
      fprintf(stderr, "%*snamed=\"%s\"\n", level * 4, "", yv -> yv_named);

   if(yv -> yv_flags & YV_TYPE) 
      {
      fprintf(stderr, "%*stype=0x%x\n", level * 4, "", yv -> yv_type);
      print_type(yv -> yv_type, level + 1);
      }

   switch(yv -> yv_code) 
      {
      case YV_NUMBER:
      case YV_BOOL:
         fprintf(stderr, "%*snumber=0x%x\n", level * 4, "", yv -> yv_number);
      break;

      case YV_STRING:
         fprintf(stderr, "%*sstring=\"%s\"\n", level * 4, "", yv -> yv_string);
      break;

      case YV_IDEFINED:
         if(yv -> yv_flags & YV_BOUND)
            fprintf(stderr, "%*smodule=\"%s\" identifier=\"%s\"\n",
                     level * 4, "", yv -> yv_module, yv -> yv_identifier);
         else
            fprintf(stderr, "%*sbound identifier=\"%s\"\n",
                     level * 4, "", yv -> yv_identifier);
      break;

      case YV_IDLIST:
      case YV_VALIST:
      case YV_OIDLIST:
         for(y = yv -> yv_idlist; y; y = y -> yv_next) 
            {
            print_value(y, level + 1);
            fprintf(stderr, "%*s----\n", (level + 1) * 4, "");
            }
      break;

      default:
      break;
      }
   }

/* SYMBOLS */

static SY new_symbol(register char *encpref, register char *decpref, 
                     register char *prfpref, register char *mod, 
                     register char *id)
   {
   register SY    sy;

   if((sy = (SY) calloc(1, sizeof *sy)) == NULLSY)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*sy));

   sy -> sy_encpref = encpref;
   sy -> sy_decpref = decpref;
   sy -> sy_prfpref = prfpref;
   sy -> sy_module = mod;
   sy -> sy_name = id;

   return sy;
   }

static SY add_symbol(register SY s1, register SY s2)
   {
   register SY        sy;

   if(s1 == NULLSY)
      return s2;

   for(sy = s1; sy -> sy_next; sy = sy -> sy_next)
      continue;

   sy -> sy_next = s2;

   return s1;
   }

/* TYPES */

YP new_type(int code)
   {
   register YP    yp;

   if((yp = (YP) calloc(1, sizeof *yp)) == NULLYP)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*yp));

   yp -> yp_code = code;

   return yp;
   }

YP add_type(register YP yp1, register YP yp2)
   {
   register YP        yp;

   for(yp = yp1; yp -> yp_next; yp = yp -> yp_next)
      continue;

   yp -> yp_next = yp2;

   return yp1;
   }

/* VALUES */

YV new_value(int code)
   {
   register YV    yv;

   if((yv = (YV) calloc(1, sizeof *yv)) == NULLYV)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*yv));

   yv -> yv_code = code;

   return yv;
   }

YV add_value(register YV yp1, register YV yp2)
   {
   register YV        yv;

   for(yv = yp1; yv -> yv_next; yv = yv -> yv_next)
      continue;

   yv -> yv_next = yp2;

   return yp1;
   }

/* TAGS */

YT new_tag(PElementClass class)
   {
   register YT    yt;

   if((yt = (YT) calloc(1, sizeof *yt)) == NULLYT)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*yt));

   yt -> yt_class = class;

   return yt;
   }

/* STRINGS */

char *new_string(register char *s)
   {
   register char  *p;

   if((p = (char *) malloc((unsigned) (strlen(s) + 1))) == NULLCP)
      myyerror(gettxt(":8", "Out of memory: %d needed.\n"), sizeof(*p));

   (void) strcpy(p, s);
   return p;
   }
