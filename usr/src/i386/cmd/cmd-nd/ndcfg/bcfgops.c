#pragma ident "@(#)bcfgops.c	29.1"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/* bcfgops.c   handles all of the grunt work and operations on bcfg files.
XXX check for P_LOADMOD privledge since we will call modadm to call verify!
XXX also put a sleep and inspect with crash that we aren't leaving fds open when we walk through bcfg dir
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/cm_i386at.h>
#include "common.h"

struct reject *rejectlist;
extern int errno;

/* nameoffset[] doesn't have to be parallel the symbol[] array any more;
 * routine N2O does the translation from Name to Offset using this struct.
 */
struct nameoffsettable nameoffset[] =
{
{CM_ADDRM, N_ADDRM},
{CM_AUTOCONF, N_AUTOCONF},
{CM_BOARD_IDS, N_BOARD_IDS},
{CM_BUS, N_BUS},
{CM_CONFIG_CMDS, N_CONFIG_CMDS},
{CM_CONFORMANCE, N_CONFORMANCE},
{CM_CUSTOM1, N_CUSTOM1},
{CM_CUSTOM2, N_CUSTOM2},
{CM_CUSTOM3, N_CUSTOM3},
{CM_CUSTOM4, N_CUSTOM4},
{CM_CUSTOM5, N_CUSTOM5},
{CM_CUSTOM6, N_CUSTOM6},
{CM_CUSTOM7, N_CUSTOM7},
{CM_CUSTOM8, N_CUSTOM8},
{CM_CUSTOM9, N_CUSTOM9},
{CM_CUSTOM_NUM, N_CUSTOM_NUM},
{CM_DEPEND, N_DEPEND},
{CM_DLPI, N_DLPI},
{CM_DMA, N_DMA},
{CM_DRIVER_NAME, N_DRIVER_NAME},
{CM_EXTRA_FILES, N_EXTRA_FILES},
{CM_FAILOVER, N_FAILOVER},
{CM_FILES, N_FILES},
{CM_IDTUNE_ARRAY1, N_IDTUNE_ARRAY1},
{CM_IDTUNE_ARRAY2, N_IDTUNE_ARRAY2},
{CM_IDTUNE_ARRAY3, N_IDTUNE_ARRAY3},
{CM_IDTUNE_ARRAY4, N_IDTUNE_ARRAY4},
{CM_IDTUNE_ARRAY5, N_IDTUNE_ARRAY5},
{CM_IDTUNE_ARRAY6, N_IDTUNE_ARRAY6},
{CM_IDTUNE_ARRAY7, N_IDTUNE_ARRAY7},
{CM_IDTUNE_ARRAY8, N_IDTUNE_ARRAY8},
{CM_IDTUNE_ARRAY9, N_IDTUNE_ARRAY9},
{CM_IDTUNE_NUM, N_IDTUNE_NUM},
{CM_INT, N_INT},
{CM_MAX_BD, N_MAX_BD},
{CM_MEM, N_MEM},
{CM_NAME, N_NAME},
{CM_NET_BOOT, N_NET_BOOT},
{CM_NUM_PORTS, N_NUM_PORTS},
{CM_ODIMEM, N_ODIMEM},
{CM_OLD_DRIVER_NAME, N_OLD_DRIVER_NAME},
{CM_PORT, N_PORT},
{CM_POST_SCRIPT, N_POST_SCRIPT},
{CM_PRE_SCRIPT, N_PRE_SCRIPT},
{CM_REBOOT, N_REBOOT},
{CM_ACTUAL_RECEIVE_SPEED, N_ACTUAL_RECEIVE_SPEED},
{CM_RM_ON_FAILURE, N_RM_ON_FAILURE},
{CM_ACTUAL_SEND_SPEED, N_ACTUAL_SEND_SPEED},
{CM_TOPOLOGY, N_TOPOLOGY},
{CM_TYPE, N_TYPE},
{CM_UNIT, N_UNIT},
{CM_UPGRADE_CHECK, N_UPGRADE_CHECK},
{CM_VERIFY, N_VERIFY},
{CM_WRITEFW, N_WRITEFW},
{CM_HELPFILE, N_HELPFILE},
{CM_ISAVERIFY, N_ISAVERIFY},
{CM_PROMISCUOUS, N_PROMISCUOUS},
};

int numvariables = sizeof(nameoffset) / sizeof(nameoffset[0]);

/* symbol[] shows all possible variables that can be defined in a bcfg file.
 * use routine N2O to determine the offset where each is stored at.
 * NOTE:  If a symbol is ONLY valid for V1, add some code for that 
 * symbol in GiveV0Defaults(void)
 */
struct symbol symbol[] =
{
/* ADDRM, CONFIG_CMDS, EXTRA_FILES used by UW2.1 Network Install only 
 * so we put them in their own stanza section titled NETINSTALL:
 */
{CM_ADDRM          ,V0   ,NETSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,OPT ,OPT},
/* ISA cards can never define AUTOCONF.  this is true
 * for both $version 0 and $version 1
 * pci/mca/eisa $version 0 bcfgs don't define autoconf unless the card doesn't
 * perform autoconfig.  In effect if not explicitly define then pretend
 * value is "true" for these buses.
 */
{CM_AUTOCONF       ,V0|V1,DRVSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,OPT ,OPT},
/* reqisa & reqbcfg is OPT for BOARD_IDS because ISA cards won't have them */
{CM_BOARD_IDS      ,V0|V1,ADPSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
/* CM_BUS is NOT multivalued; bcfgbustype() and EnsureBus assume this too */
{CM_BUS            ,V0|V1,ADPSEC,NOMULTI ,NONUM ,NA ,NOTF ,NOCUSTOM ,MAND,OPT},
{CM_CONFIG_CMDS    ,V0|V1,MANSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_CONFORMANCE    ,V1   ,ADPSEC,NOMULTI ,YESNUM,HEX,NOTF ,NOCUSTOM ,MAND,OPT},
/* CUSTOM[x] in DRVSEC since CFG_CUSTOM doesn't go into resmgr but the
 * line xx parameter *does* go into the resmgr
 */
{CM_CUSTOM1        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM2        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM3        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM4        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM5        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM6        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM7        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM8        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
{CM_CUSTOM9        ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,YESCUSTOM,OPT ,OPT},
/* XXX ensure 1 <= CUSTOM_NUM <= 9 */
/* NOTE: dcu sources in the bcfg file and treats CUSTOM_NUM as a number to see 
 * if set and >= 1
 */
/* CUSTOM_NUM in DRVSEC since CFG_CUSTOM_NUM doesn't go into resmgr */
{CM_CUSTOM_NUM     ,V0|V1,DRVSEC,NOMULTI ,YESNUM,DEC,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_DEPEND         ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_DLPI           ,V0   ,DRVSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,OPT ,OPT},
{CM_DMA            ,V0|V1,ADPSEC,YESMULTI,YESNUM,DEC,NOTF ,NOCUSTOM ,OPT ,MAND},
{CM_DRIVER_NAME    ,V0|V1,DRVSEC,NOMULTI ,NONUM ,NA ,NOTF ,NOCUSTOM ,MAND,OPT},
{CM_EXTRA_FILES    ,V0|V1,MANSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
/* FAILOVER is new to #$version 1 bcfg files and mandatory */
{CM_FAILOVER       ,V1   ,DRVSEC,NOMULTI ,NONUM, NA ,YESTF,NOCUSTOM ,MAND,OPT},
{CM_FILES          ,V1   ,MANSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,MAND,OPT},
{CM_HELPFILE       ,V1   ,DRVSEC,YESMULTI,NONUM, NA ,NOTF ,NOCUSTOM ,MAND,OPT},
{CM_IDTUNE_ARRAY1  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY2  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY3  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY4  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY5  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY6  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY7  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY8  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_IDTUNE_ARRAY9  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
/* XXX ensure 1 <= IDTUNE_NUM <= 9 */
{CM_IDTUNE_NUM     ,V0|V1,DRVSEC,NOMULTI ,YESNUM,DEC,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_INT            ,V0|V1,ADPSEC,YESMULTI,YESNUM,DEC,NOTF ,NOCUSTOM ,OPT ,MAND},
{CM_ISAVERIFY      ,V1   ,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT, OPT},
{CM_MAX_BD         ,V1   ,ADPSEC,NOMULTI ,YESNUM,DEC,NOTF ,NOCUSTOM ,MAND,OPT},
{CM_MEM            ,V0|V1,ADPSEC,YESMULTI,YESNUM,HEX,NOTF ,NOCUSTOM ,OPT ,MAND},
{CM_NAME           ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,MAND,OPT},
{CM_NET_BOOT       ,V1   ,ADPSEC,NOMULTI ,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_NUM_PORTS      ,V0|V1,ADPSEC,NOMULTI ,YESNUM,DEC,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_ODIMEM         ,V0   ,DRVSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,OPT ,OPT},
/* OLD_DRIVER_NAME isn't supported in $version 1.  maybe in $version 2 */
{CM_OLD_DRIVER_NAME,V0   ,DRVSEC,NOMULTI ,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_PORT           ,V0|V1,ADPSEC,YESMULTI,YESNUM,HEX,NOTF ,NOCUSTOM ,OPT ,MAND},
{CM_POST_SCRIPT    ,V0|V1,MANSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_PRE_SCRIPT     ,V0|V1,MANSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
/* PROMISCUOUS can either be true, false, or unknown (version 0 default) */
{CM_PROMISCUOUS    ,V1   ,DRVSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,MAND,OPT},
{CM_REBOOT         ,V0|V1,DRVSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,OPT ,OPT},
{CM_ACTUAL_RECEIVE_SPEED,V1,ADPSEC,NOMULTI,YESNUM,DEC,NOTF,NOCUSTOM ,MAND,OPT},
{CM_RM_ON_FAILURE  ,V0|V1,DRVSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_ACTUAL_SEND_SPEED,V1 ,ADPSEC,NOMULTI ,YESNUM,DEC,NOTF ,NOCUSTOM ,MAND,OPT},
/* TOPOLOGY must be in ADPSEC so that it can go into resmgr.  
 * this gives chance for driver to sort it out if multivalued for further
 * CUSTOM[x] line xx prompts which depend on a certain TOPOLOGY
 */
{CM_TOPOLOGY       ,V0|V1,ADPSEC,YESMULTI,NONUM ,NA ,NOTF ,NOCUSTOM ,MAND,OPT},
{CM_TYPE           ,V1   ,DRVSEC,NOMULTI ,NONUM ,NA ,NOTF ,NOCUSTOM ,MAND,OPT},
/* UNIT should go in adapter since it will modify CM_UNIT in resmgr */
{CM_UNIT           ,V0|V1,ADPSEC,NOMULTI ,YESNUM,DEC,NOTF ,NOCUSTOM ,OPT ,OPT},
{CM_UPGRADE_CHECK  ,V0   ,DRVSEC,NOMULTI ,NONUM ,NA ,NOTF ,NOCUSTOM ,OPT ,OPT},
/* no documentation on VERIFY; it existed once upon a time in niccfg */
{CM_VERIFY         ,V0   ,DRVSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,OPT ,OPT},
/* WRITEFW is mandatory for $version 1 *ISA* drivers  (not ALL $v1 bcfgs!)
 * true means that driver must program NVRAM/EEPROM on card so if board not 
 * found then abort the add operation
 * false means that driver does not need to program NVRAM/EEPROM so if a board
 * isn't found then continue with the installation
 */
{CM_WRITEFW        ,V1   ,DRVSEC,NOMULTI ,NONUM ,NA ,YESTF,NOCUSTOM ,OPT ,MAND}
};

int numsymbols = sizeof(symbol) / sizeof(symbol[0]);

int bcfgfileindex = 0;   /* index into bcfgfile[] */
/* this give a large .bss size but I don't want to malloc these just yet 
 * XXX do malloc in main for this later on
 */
struct bcfgfile bcfgfile[MAXDRIVERS];   /* limit checked in zzwrap */

/* N2O translates from a given symbol name to its offset in the table.
 * so that we can assign its value in bcfgfile[bcfgfileindex].variable[]
 * this eliminates the 1:1 requirement we had earlier.
 */
int
N2O(char *name)
{
   int loop;

   for (loop = 0; loop < numvariables; loop++) {
      if (!strcmp(name, nameoffset[loop].name)) {
         return(nameoffset[loop].offset);  /* not loop! */
      }
   }
   /* to get here we must have played with our header file but forgot to
    * update the table  changed from fatal to error
    */
   error(NOTBCFG,
         "N2O: couldn't find offset in nameoffsettable[] for %s", name);
   return(-1);  /* to keep compiler happy */
}

void
strtoupper(char *str)
{
        register unsigned char  c;

        while ((c = *str) != '\0')
        {
                /* don't convert the 'x' in 0xabcdef to upper */
                if ((c == 'x') || (c == '%' && *(str + 1) != '\0'))
                        ++str;
                else
                        *str = toupper(c);
                ++str;
        }
}

void
strtolower(char *str)
{
        register unsigned char  c;

        while ((c = *str) != '\0')
        {
                if (c == '%' && *(str + 1) != '\0')
                        ++str;
                else
                        *str = tolower(c);
                ++str;
        }
}

/* convert the BUS= in bcfg file to CM_BUS_* equivalent for later
 * bit operations.   BUS= is NOT multivalued in 1 bcfg file; you must
 * have multiple bcfgs for each bus.  see CM_BUS in symbol[] NOMULTI 
 */
uint_t
bcfgbustype(char *bus)
{
   strtoupper(bus);

   if (!strcmp(bus,"ISA")) {
      return(CM_BUS_ISA);
   } else
   if (!strcmp(bus,"EISA")) {
      return(CM_BUS_EISA);
   } else
   if (!strcmp(bus,"PCI")) {
      return(CM_BUS_PCI);
   } else
   if (!strcmp(bus,"PCCARD")) {    /* NOTE: NOT PCMCIA! */
      return(CM_BUS_PCMCIA);
   } else
   if (!strcmp(bus,"PNPISA")) {
      return(CM_BUS_PNPISA);
   } else
   if (!strcmp(bus,"MCA")) {
      return(CM_BUS_MCA);
#ifdef CM_BUS_I2O
   } else
   if (!strcmp(bus,"I2O")) {
      return(CM_BUS_I2O);
#endif
   } else {
      notice("bcfgbustype: unknown bus '%s'",bus);
      return(CM_BUS_UNK);   /* unknown bus to get here */
   }
   /* dunno about CM_BUS_SYS */
}

void
initsymbols(void)
{
   int loop1,loop2;

   for (loop1=0;loop1<MAXDRIVERS;loop1++) {
      for (loop2=0;loop2<numvariables; loop2++) {
         bcfgfile[loop1].variable[loop2].strprimitive.type=UNDEFINED;
         bcfgfile[loop1].variable[loop2].primitive.type=UNDEFINED;
      }
   }
}

char unknownsection[80]; /* not on stack */

char *
SectionName(int x)
{
   switch(x) {
      case DRVSEC: return("DRIVER:"); break;
      case ADPSEC: return("ADAPTER:"); break;
      case NETSEC: return("NETINSTALL:"); break;
      case MANSEC: return("MANIFEST:"); break;
      default: snprintf(unknownsection,80,"Unknown section name for %d\n",x);
               return(unknownsection);
               break;
   }
}


uint_t bcfghaserrors;

void
rejectbcfg(uint_t reason)
{
   bcfghaserrors |= reason;
}

/* translate lex current section name into symbol[] section name */
int 
CurrentSection(void)
{
   /* note there's no way to return NETSEC from here! */
   switch(section) {
      case NOSECTION:
         return(-1);
         break;
      case MANIFEST:
         return(MANSEC);
         break;
      case DRIVER: 
         return(DRVSEC);
         break;
      case ADAPTER: 
         return(ADPSEC);
         break;
   }
   fatal("unknown current section %d",section);
}

int
ensurev1(void)
{
   extern char zztext[];   /* renamed by Makefile sed script from yytext */

   if (cfgversion == 0) {
      error(SECTIONS, "sections('%s') not allowed in $version 0 bcfg files",
              zztext);
      return(0);
   }
   return(1);
}

void
ensurevalid(void)
{
   if (cfgversion < MINCFGVERSION || cfgversion > MAXCFGVERSION) {
      error(VERSIONS,
            "$version number at line %d must be between %d and %d",
              bcfglinenum, MINCFGVERSION, MAXCFGVERSION);
   }
  /* when you go to #$version 2 bcfg files you should tweak GiveV0Defaults() */
}

/* eventually when above symbol[] is big enough we'll have two tables
 * one for $version 0 and one for $version 1 symbols.  For now we don't
 * so we do a boring linear search through table every time.
 */
int
FindSymbolIndex(char *variable)
{
   int loop;

   for (loop=0;loop<numsymbols;loop++) {
      if (!strcmp(symbol[loop].name,variable)) return(loop);
   }
   return(-1);
}

/* returns 0 for success, 1 for error */
int
AddStringList(char *variable,union primitives strprimitive)
{
   stringlist_t *slp;
   int index,version,variableoffset;
   union primitives *upp;

   upp=&strprimitive;
   slp=&strprimitive.stringlist;
   /* first see if variable name we're trying to assign is valid */
   if ((index=FindSymbolIndex(variable)) == -1) {
      error(BADVAR, "invalid variable name '%s' at line %d", 
         variable,bcfglinenum);
      return(1);   /* can't go any further in this routine */
   }
   version=symbol[index].version;
   if ((version & V0) && !(version & V1) && (cfgversion == 1)) {
      error(BADVAR,
            "variable '%s' at line %d is only valid in $version 0 cfg files",
            variable, bcfglinenum);
      return(1);  /* no point in continuing through this routine */
   }
   if ((version & V1) && !(version & V0) && (cfgversion == 0)) {
      error(BADVAR,
            "variable '%s' at line %d is only valid in $version 1 cfg files",
            variable, bcfglinenum);
      return(1);  /* no point in continuing through this routine */
   }
#if NCFGDEBUG == 1
   /* complain if we crank up MAXCFGVERSION without adding further checks
    * above for later $versions.
    */
   if (version & V2) /*increase this after you've added more checks */ {
      error(NOTBCFG,"add more checks to AddStringList!");
      error(NOTBCFG,"add more checks to EnsureAllAssigned!");
      exit(1);  /* calls atexit->ExitIfNOTBCFGError->_exit(1) */
   }
#endif
   variableoffset=N2O(variable);
   if (bcfgfile[bcfgfileindex].variable[variableoffset].strprimitive.type 
       != UNDEFINED) {
      error(MULTDEF,
          "variable '%s' previously defined; delete one of the definitions",
          variable);
      return(1);  /* no point in continuing through this routine */
   }
   if (cfgversion == 1 && CurrentSection() != symbol[index].section) {
      error(BADVAR|NOERRNL,
               "You can't define variable '%s' in this section", variable);
      /* CONT means no banner in line below since it's a continuation line */
      error(BADVAR|CONT, "       It can only be defined in section '%s'", 
               SectionName(symbol[index].section));
      return(1);  /* no point in continuing through this routine */
   }
   if (symbol[index].ismulti == NOMULTI) {
      if (slp->next != NULL) {
         error(MULTVAL, "Variable '%s' on line %d can not be multivalued", 
                 variable,bcfglinenum);
         return(1);  /* no point in continuing through this routine */
      }
      if (symbol[index].istf == YESTF) {
         int good=0;
         /* must be lower case because when evaluated by dcu they turn out
          * to be /bin/true and /bin/false
          * if you change case here also change GiveV0Defaults()
          */
         if (!strcmp(slp->string,"true"))  good++;
         if (!strcmp(slp->string,"false")) good++;
         if (!good) {
            error(TRUEFALS, 
                  "variable '%s' at line %d can only be 'true' or 'false'",
                    variable,bcfglinenum);
            return(1);  /* no point in continuing through this routine */
         }
      }
   }
   /* Pstdout("defining %s at offset %d\n",variable,variableoffset); */
   /* ok, we'll add this to our symbol table, in a slightly nicer form */
   bcfgfile[bcfgfileindex].variable[variableoffset].strprimitive = *upp;
   /* symbol[index].strprimitive = *upp;    union copy */
 /*memcpy(&symbol[index].strprimitive,&strprimitive,sizeof(union primitives));*/
   /* if not a CUSTOM variable then we should get rid of C/R or comma
    *
    */
   return(0);
}

/* DoAssignment called from parser when it has sufficient information to
 * complete an assignment statement.  Unfortunately, we have to 
 * massage the data a bit.  The union primitives stuff is left from
 * an earlier version of this parser which may return later; see
 * the file "parser.y.before" for more information.
 */
/* NOTE: now the parser only passes up STRINGLIST types in primitive */
int
DoAssignment(char *variable,union primitives primitive)
{
singlenum_t *snp;
numrange_t *nrp;
string_t *sp;
numlist_t *nlp;
stringlist_t *slp;
int ret=0;

   /* Pstdout("\nvariable=%s; type=%d\n",variable,primitive.type); */
   switch (primitive.type) {
      case SINGLENUM:
         snp=&primitive.num;
         Pstdout("singlenum: number=%d\n",snp->number);
         break;

      case NUMBERRANGE:
         nrp=&primitive.numrange;
         Pstdout("numrange: low=%d  high=%d\n",nrp->low, nrp->high);
         break;
   
      case STRING:
         sp=&primitive.string;
         Pstdout("string: %s\n",sp->words);
         break;

      case NUMBERLIST:
         nlp=&primitive.numlist;
         while (nlp != (struct numberlist *)NULL) {
            if (nlp->ishighvalid)
               Pstdout("numberlist: low=%d high=%d\n",nlp->low, nlp->high);
            else
               Pstdout("numberlist: %d\n",nlp->low); 
            nlp=nlp->next;
          }
         break;

      case STRINGLIST:
#ifdef DEBUG
         slp=&primitive.stringlist;
         while (slp != (struct stringlist *)NULL) {
            Pstderr("stringlist: %s\n",slp->string);
            slp=slp->next;
         }
#endif
         if (AddStringList(variable, primitive) != 0) {
            /* we encountered an error; parser should free up primitive mem */
            ret=1;
         }
         break;
   
      default:
         fatal("DoAssignment: unknown type %d",primitive.type);
         break;
   }
   return(ret);
}

/* note we assume BUS is of type STRINGLIST here.  This is normally the case
 * for our parser (although it really should be type STRING)
 * we also assume BUS is not multi-valued (enforced in bcfgops.c symbol[]).
 */
void
EnsureBus(void)
{
   int offset;
   char *thisbus;
   uint_t convertedbus;
   extern uint_t cm_bustypes;

   offset=N2O(CM_BUS);
   if (bcfgfile[bcfgfileindex].variable[offset].strprimitive.type==UNDEFINED) {
      /* error(NOTBUS, "BUS not defined");  done by EnsureAllAssigned */
      return;
   }
   /* we know we defined BUS to get here */
   thisbus=
       bcfgfile[bcfgfileindex].variable[offset].strprimitive.stringlist.string;
   convertedbus=bcfgbustype(thisbus);
   /* if the BUS= in this bcfg file isn't one that this machine has, 
    * skip this bcfg file.  Note we fake CM_BUS_PCMCIA support.
    */
   if (!(cm_bustypes & convertedbus)) {
      error(NOTBUS, "skipping bcfg since only for non-applicable bus %s",
            thisbus);
      return;  /* no point in continuing - we can never use this driver */
   }
   /* handle AUTOCONF=false case for certain boards
    * that aren't really smart.  In particular, the following:
    * BUS=PCCARD for C3589/C3589.bcfg:NAME=3COM_ETHERLINK_III_PCMCIA
    *            and XPSODI/XPSODI.bcfg:NAME=XIRCOM_ETHER_IIps_PCMCIA
    *  All PCCARD devices are converted back to ISA here.
    * BUS=EISA   for IBM164/TCM619B.bcfg:NAME=3COM_TokenLink3_3C619B
    * BUS=EISA   for IBM164/TCM679.bcfg:NAME=3COM_TokenLink3_3C679
    * we turn them into dumb boards by setting BUS= to ISA here
    * then call EnsureAllAssigned again to ensure that 
    * the IRQ/DMAC/IOADDR/MEMADDR defines are set.
    * After this point they are treated as an ISA board (since
    * resshowunclaimed will never find them in the resmgr)
    * pseudo-hack: rather than realloc the space, we do a blind strcpy
    * since all other supported bus types (EISA, PCI, MCA, PCCARD) use
    * more characters so it will fit.  Must be upper case. 
    * We treat the obsolete ADDRM=true as the same as AUTOCONF=false
    *
    * HasString() returns 1 in line below for AUTOCONF when AUTOCONF defined 
    * and has the value "false" 
    */
   if ((HasString(bcfgfileindex,N_AUTOCONF,"false",0) == 1) ||
       (HasString(bcfgfileindex,N_ADDRM,"true",0) == 1) ||
       (convertedbus == CM_BUS_PCMCIA)) {
      if ((cm_bustypes & CM_BUS_ISA) == 0) {
         error(NOTBUS, "bcfg says AUTOCONF=false or ADDRM=true or BUS=PCCARD"
                       "but no ISA bus present on machine - can't convert!");
         return; /* no point in continuing - we can never use this driver */
      }
      strcpy(thisbus,"ISA");  /* using strcpy instead of free old/malloc new/
                               * strcpy ISA into new/reassign string is hack 
                               * must malloc new space we always free() later
                               */
      /* I'm sorry, but if you have a PCI board that doesn't show up
       * in the resmgr then there's no way to know if this thing
       * will conflict with anything else on the system.  I don't
       * care if the driver will add the IO range at init time --
       * you must define INT= and PORT= (and optionally MEM=)
       * so we call EnsureAllAssigned which will check for these
       * two things.  This only affects UW2.1 Compaq_Embedded_AMD_PCnet/pnt
       * driver which has a bogus bcfg file 
       */
      EnsureAllAssigned();  /* based on "new" BUS=. can call Error() */
   } else {
      if ((convertedbus == CM_BUS_EISA) ||
          (convertedbus == CM_BUS_PCI)  ||
#ifdef CM_BUS_I2O
          (convertedbus == CM_BUS_I2O)  ||
#endif
          (convertedbus == CM_BUS_MCA)) {
         char *brdid;
         /* since AUTOCONF is true (or not set), and we know we're a smart
          * bus, and we should see its board id in the resmgr.  
          * so make sure BOARD_IDS is set.  Otherwise we'll never see
          * this board with resshowunclaimed.
          */
         brdid=StringListPrint(bcfgfileindex,N_BOARD_IDS);
         if (brdid == NULL) {
            char msg[100];
            snprintf(msg,100,"bcfg bus=%s, no AUTOCONF=false, and "
                             "BOARD_IDS not set", thisbus);
            if (convertedbus == CM_BUS_EISA) error(EISANOTDEF,msg);
            if (convertedbus == CM_BUS_PCI) error(PCINOTDEF,msg);
#ifdef CM_BUS_I2O
            if (convertedbus == CM_BUS_I2O) error(I2ONOTDEF,msg);
#endif
            if (convertedbus == CM_BUS_MCA) error(MCANOTDEF,msg);
         } else {
            struct stringlist *next=&bcfgfile[bcfgfileindex].variable[N_BOARD_IDS].strprimitive.stringlist;

            free(brdid);  /* from above StringListPrint */
         
            /* now convert the board id to lower case to match EnsureNumerics
             * which also converts everything to lower case internally
             * we know there's something there since StringListPrint didn't
             * return NULL.
             */

            while (next != NULL) {
               strtolower(next->string);
               next=next->next;
            }
         }
      }
   }
}

void
EnsureAllAssigned(void)
{
   int loop,version,variableoffset;
   int oktoprint=1;
   char *bus,*irq;
   char *brdid;

   for (loop = 0; loop < numsymbols;loop++) {
      variableoffset=N2O(symbol[loop].name);
      if (bcfgfile[bcfgfileindex].variable[variableoffset].strprimitive.type 
          == UNDEFINED && symbol[loop].reqbcfg == MAND) {
         version=symbol[loop].version;
         if (((version & V0) && (cfgversion == 0)) ||
             ((version & V1) && (cfgversion == 1))) {
            if (oktoprint) {
               oktoprint--;
               error(NOTDEF|NOERRNL,
                    "The following version %s bcfg symbol(s) are not defined: ",
                    cfgversion == 0 ? "0" : "1");
            }
            /* negative means no leading ERROR: banner */
            error(NOTDEF|CONT|NOERRNL, "%s ",symbol[loop].name);  
         }
      }
   }
   if (!oktoprint) {
      error(NOTDEF|CONT|NOERRNL, "\n");   /* add on the '\n' */
   }
   /* check for .reqisa symbols defined in symbol[] here? 
    * answer is no as this forces user to define them to nothing
    * in bcfgfile even if HW doesn't use it.  this would break
    * backwards compatability and EnsureV0Defaults would have to
    * convert V0 bcfgs foward to v1.  Example: board doesn't
    * use DMA, so why should it have to define DMA="" in bcfg file? 
    * later on the idinstall command doesn't mandate that it
    * be supplied either.  ditto with showISAparams.
    *
    * However, if BUS is ISA we do check for INT=, the bare
    * minimum for all ISA boards (as a purely memory mapped board won't
    * set PORT= in its bcfg file and a programmed I/O board doesn't have to
    * define MEM= in its bcfg file).  This will eliminate the 
    * Compaq_Embedded_AMD_PCnet UW2.1 driver (BUS=PCI and AUTOCONF=false) 
    * since BUS gets changed back to ISA (AUTOCONF=false) and its bcfg
    * then doesn't define INT=, causing it to be failed.
    */
   bus=StringListPrint(bcfgfileindex,N_BUS);
   if (bus != NULL) {
      if ((!strcmp(bus,"ISA")) || (!strcmp(bus,"isa"))) {
         char *irq;
         irq=StringListPrint(bcfgfileindex,N_INT);
         if (irq == NULL) {
            error(ISANOTDEF, "INT= not defined");
            /* don't return */
         } else free(irq);
         /* ensure board ids isn't defined since we're ISA */
         brdid=StringListPrint(bcfgfileindex,N_BOARD_IDS);
         if (brdid != NULL) {
            error(BADVAR, "BOARD_IDS not applicable for ISA bus");
            free(brdid);
         }
      }  /* if ISA bus */
      free(bus);
   }
}

void
EnsureNumerics(void)
{
   int loop,loop2,base,variableoffset;
   union primitives primitive;
   char *start;
   char *end;
   char **endp=&end;
   stringlist_t *slp;

   for (loop = 0; loop < numsymbols; loop++) {
      /* Pstdout("strprimitive.type=%d\n",symbol[loop].strprimitive.type); */
      variableoffset=N2O(symbol[loop].name);
      if (symbol[loop].isnumeric == YESNUM &&  /* put check first for speed */
         bcfgfile[bcfgfileindex].variable[variableoffset].strprimitive.type 
                != UNDEFINED) {  /* most will end up being defined */
         slp=&bcfgfile[bcfgfileindex].variable[variableoffset].strprimitive.stringlist;
         base=0; /* 0="figure it out from string itself":needs 0x, leading 0 */
         if (symbol[loop].decorhex == DEC) base=10;
         else 
         if (symbol[loop].decorhex == HEX) base=16;
         else 
         if (symbol[loop].decorhex == OCT) base=8;
         do { 
            u_long value, length;

            start=slp->string;
           
            strtolower(start);   /* convert all hex to lower case 
                                  * internally for later call to IsParam
                                  * (for idinstall command and for ISA ops.)
                                  * this will not affect PCI board ids
                                  * since they aren't considered numbers
                                  * but strings.
                                  */

            length=strlen(start);
            if (*start == '\n') goto skip;  /* lexer passes up \n as word */
NumRangeAgain:
            if (*start == '-') {
               error(BADNUM,
                        "no negative numbers ('%s') allowed in variable %s",
                       start,symbol[loop].name);
               goto nextsymbol; /* can't continue */
            }
/* XXX use sigprocmask to block signals here */
            errno=0;  /* right before strtoul */
            value=strtoul(start, endp, base);
            if (value == 0 && errno > 0) {
               error(BADNUM,
                   "couldn't convert '%s' as a base %d number in variable %s",
                   start, base, symbol[loop].name);
               goto nextsymbol; /* can't continue */
            }
            if (*end == '\0') {
            ; /* end of our number (either a single number or as a range) */
            } else 
            if (*end == '-') { /* numeric range, have 1st half, need 2nd half */
               start=end+1;
               end=0;
               if (*start == '\0') {
                  error(BADNUM,
                        "missing right hand side value for range in %s",
                          symbol[loop].name);
                  goto nextsymbol; /* can't continue */
               }
               goto NumRangeAgain;
            } else {
               error(BADNUM, "bad character '%c' in variable %s",
                       *end,symbol[loop].name);
               goto nextsymbol; /* can't continue */
            }
skip:
            slp=slp->next;
         } while (slp != NULL);
nextsymbol:
         ;
      } /* if symbol should be numeric and if it's defined */
   } /* for all symbols */
}

/* we always use STRINGLIST to emulate bcfgparser.y 
 * space for value must be malloced before calling this routine
 * we allow multiple definitions by adding onto linked list
 * in the proper order
 */
void
DefineSymbol(u_int bcfgindex, u_int offset, char *value)
{
   struct stringlist *firstone;
   /* has anything been defined here? */
   firstone=&bcfgfile[bcfgindex].variable[offset].strprimitive.stringlist;
   if (firstone->type == UNDEFINED) {
      /* our union already has room for one stringlist so no need to malloc 
       * more 
       */
      firstone->type = STRINGLIST;
      /* must use strdup since PrimitiveFreeMem wants to free() first one */
      firstone->string = strdup(value);
      firstone->uma = NULL;  /* we didn't malloc this entry */
      firstone->next = NULL;
   } else if (firstone->type == STRINGLIST) {
      /* one has been defined, we must malloc another, walk through list,
       * and add just malloced one onto the end. See bcfgparser.y section
       * ManyWords: ManyWords whitespace word
       * for original code this comes from
       */
      union primitives *sl;
      struct stringlist *walk;

      sl=(union primitives *) malloc(sizeof(union primitives));
      if (!sl) {
         fatal("DefineSymbol: couldn't malloc for stringlist");
         /* NOTREACHED */
      }
      sl->stringlist.uma = sl;
      sl->stringlist.type = STRINGLIST;
      sl->stringlist.string = strdup(value);
      sl->stringlist.next=(struct stringlist *)NULL; /* new one always last */
      /* now we must add this onto end of existing stringlist */
      if (firstone->next == NULL) {
         /* only one there now, so tack this onto the end */
         firstone->next = &sl->stringlist;
      } else {
         /* multiple elements there, must traverse to the end of the list */
         walk=firstone->next;
         while (walk->next != NULL) walk=walk->next;
         walk->next = &sl->stringlist;
      }
   } else {
      /* uhoh.  we're trying to add a STRINGLIST onto some other type
       * should never happen
       */
      fatal("DefineSymbol: can't add STRINGLIST onto existing type %d!",
            firstone->type);
      /* NOTREACHED */
   }
}

/* subok is for substing matches
 * if subok == 1 then we use simple strstr, so if searching for ISA and
 * we encounter EISA we return success since ISA is a substring of EISA 
 * if subok == 0 then we ensure text is surrounded by space or null
 * NOTE: if a user defines something like FOO="this is a very long string"
 * in the bcfg file then this entire string will be stored in the
 * stringlist
 * returns -1 if parameter doesn't exist in bcfg file (never defined)
 * returns  0 if parameter does exist but string not found
 * returns  1 if parameter does exist and string was found
 */
int
HasString(u_int bcfgindex, u_int offset, char *value, int subok)
{
   struct stringlist *next;

   /* when initializing bcfgindex == bcfgfileindex */
   if (bcfgindex > bcfgfileindex) {
      notice("HasString(%u,%u,%s,%d): index too big",
             bcfgindex, offset, value, subok);
      return(-1);
   }

   /* if variable isn't a STRINGLIST (i.e. UNDEFINED, NUMBERLIST, etc) return
    * failure.
    */
   if (bcfgfile[bcfgindex].variable[offset].strprimitive.type != STRINGLIST) {
      return(-1);
   }
   if (strlen(value) == 0) {
      /* probable programmer error */
      notice("HasString(%d,%d,null length,%d) - returning -1",
             bcfgindex, offset, subok);
      return(-1);  
   }
   next=&bcfgfile[bcfgindex].variable[offset].strprimitive.stringlist;
   if (subok) {  /* put check outside loop since subok doesn't change */
      while (next != NULL) {
         if (strstr(next->string,value) != NULL) {
            return(1);
         }
         next=next->next;
      }
   } else { /* substring matches not ok -- must be a word on its own */
      while (next != NULL) {
         char *cp,*start;
         /* originally, we checked the entire line with
          *    if (strcmp(next->string,value) == 0) {
          *      return(1);
          *    }
          * but this isn't entirely correct.  What we want is to use
          * substring matching but ensure that there is either a space
          * or null surrounding the text (i.e. it's a word all by itself and
          * not part of surrounding text like ISA->EISA)
          * we can't use snprintf(foo,X," %s ") and search for foo with strstr
          * since that won't match the pattern at the end of a string.
          * not we must continue to call strstr until we process entire string:
          * example where we're searching for ISA:
          *    "EISA EISA EISA EISA ISA"
          * strstr will find many fake matches before the real one.
          */
         start = next->string;
         while ((cp=strstr(start,value)) != NULL) {
            /* ok, we have a basic substring match.  but what about
             * space or null surrounding text? 
             */
            char spacebefore[SIZE],spaceafter[SIZE];
            char *foundbefore, *foundafter; 
            char foo;

            /* first, check for exact match - no spaces on either side  */
            if (cp == next->string) {
               foo=*(cp+strlen(value));
               if (foo == '\0' || foo == ' ') return(1);
            }
            
            /* now try search with spaces on either side */
            snprintf(spacebefore,SIZE," %s",value);
            foundbefore=strstr(start, spacebefore);
            snprintf(spaceafter,SIZE,"%s ",value);
            foundafter =strstr(start, spaceafter);

            /* if foundbefore is not NULL, then as long as *(cp+strlen(value))
             * is a space or NULL(end of line case), then we have a success
             */
            if (foundbefore != NULL) {
               foo=*(cp+strlen(value));
               if (foo == ' ' || foo == '\0') return(1);
            }

            /* if foundafter is not NULL, then as long as char before
             * is valid and is a space then we have a success
             */
            if (foundafter != NULL) {
               if (foundafter == next->string) return(1);/*match at beginning*/
               if (*(foundafter - 1) == ' ') return(1);
            }
            start += strlen(value);
         }
         next=next->next;  /* try next string in our list */
      }
   }
   /* we reached the end without finding a match so return failure */
   return(0);
}

/* this routine makes a V0 (UW2.1) bcfg file appear to be a V1 file for
 * later parsing.  It is necessary for those symbols that are both 
 * mandatory and only defined in V1 bcfg files, specifically:
 * -CM_CONFORMANCE    ,V1  - keyed from DLPI= and Master file
 * -CM_FAILOVER       ,V1  - set to false
 * -CM_FILES          ,V1  - we fake with readdir
 * -CM_MAX_BD         ,V1  - set to 4
 * -CM_ACTUAL_RECEIVE_SPEED,V1-set to 0 since not known for v0 files
 * -CM_ACTUAL_SEND_SPEED   ,V1-set to 0 since not known for v0 files
 * -CM_TYPE           ,V1  - either DLPI or ODI, keyed from ODIMEM or DLPI
 * -CM_HELPFILE       ,V1  - set to"HW_network ncfgN.configuring_hardware.html"
 * -CM_WRITEFW        ,V1  - set to false only (never true!) if BUS=ISA
 * -CM_PROMISCUOUS    ,V1  - does driver support promiscuous mode?
 * We know the V0 bcfg file doesn't have any errors (wrong bus, typos, etc) 
 * to get here, so we can check the other defined symbols to see how to 
 * define them.  Remember to define things here in keeping with the rules 
 * set in symbol[] !
 * Eventually when we go to #$version 2 this routine will need to 
 * be modified to make #$version 1 bcfg files look like #$version 2.
 * NOTE:  Since CONFORMANCE is stored as a number, if you type in a hex
 *        number like 0xabc make sure that it's all in lower case
 *        (see YESNUM and HEX in symbol[])
 */
void
GiveV0Defaults(void)
{
   char tmppath[SIZE], *cp;

   /* since we rely on certain symbols being defined here if there's any
    * previous error where a symbol might not be defined we immediately
    * return -- this bcfg is already marked as bad. 
    */
   if (bcfghaserrors) return;
     
   if (HasString(bcfgfileindex, N_DLPI, "true", 0) == 1) {
      /* ok.  we know this is a UW2.1 DLPI driver or MDI driver with a
       * intentionally wrong bcfg file (no #$version1 and they incorrectly
       * have a DLPI=true keyword when the driver is MDI).  We doc the 
       * DLPI= keyword for MDI drivers as "hands off", so we assume 
       * it's DLPI from here on out.  If this was MDI in disguise, you lose.
       */
      DefineSymbol(bcfgfileindex, N_CONFORMANCE, "0x02");/*DL_CURRENT_VERSION*/
      DefineSymbol(bcfgfileindex, N_TYPE, "DLPI");
   } else {
      /* darn.  this is either a UW2.1 ODI driver or a MDI driver which forgot 
       * to have a #$version 1 in its bcfg file.  could also be a v0 DLPI driver
       * that forgot to set DLPI= too.  we look in the Master file
       * to figure out which is the case (and to also set CONFORMANCE).
       *
       * NOTE BELOW:  The closest way to get the CONFORMANCE number is from the
       * DSP Master file.  ODIMEM is only set for ODI drivers that wish to be 
       * *statically* linked, so can't key on that alone to determine if bcfg
       * indicates ODI or not.  The only way is to parse the Master file.
       * Long explanation follows:
       * the foo.LAN file which is a Novell NLM must have the following 
       * lines in its (spec documented in postscript files "32sp_3.prn" and 
       * "32sp_d.prn") source code in file xmog.tar:
       *   OSDATA   segment rw public 'DATA'
       *   HSMSPEC  db      'HSM_SPEC_VERSION: 3.2',0   <-must be 1st in OSDATA
       * xmog takes this and *should* take the 3.2 and move it into
       * the $interface line in the Master file.  Unfortunately, xmog
       * isn't that smart (from looking at the executable - no src available
       * for nlm2elf and hsmconfig) and instead uses a dumb template Master file
       * which is hard coded to "3.2" -- Novell updated the xmog distribution 
       * with the new version.  In other words, if you look at a UW2.1 box in
       * /etc/inst/nics/drivers/ * / Master, you'll see a bunch of $interface
       * lines which effectively tell the version of xmog that was used
       * to convert the driver and NOT the actual HSMSPEC from the assembler
       * (although you can still strings the Driver.o to determine this).
       * At any rate, the final version UW2.1 xmog supports is 3.2, which we 
       * use below.  We expect to see $interface odiSOMETHING NUMBER
       * If we don't find an odiSOMETHING then we know this is not really
       * an ODI driver but an MDI driver which forgot #$version 1.
       * If we do find odiSOMETHING then we use the first version number to
       * indicate the CONFORMANCE number to use, since even if there are
       * multiple $interface odieth/$interface oditok the number
       * must be the same (from the above xmog process).  See UW2.1 
       * IBMEST/IBMLST/IBMMPCO Master files for an example of this.
       *
       * Of the 53 UW2.1 ODI drivers, 36 are 3.2, 13 are 3.1, and 5 are 1.1, so
       * if something goes wrong below, we just assume 3.2. 
       */
      char line[SIZE],name[SIZE],level[SIZE],level2[SIZE];
      FILE *fp;
      u_int found;

      strncpy(tmppath,bcfgfile[bcfgfileindex].location,SIZE-10);
      cp=strrchr(tmppath, '/');
      if (cp == NULL) {
         strcpy(tmppath,"Master");
      } else {
         *(cp+1)=NULL;
         strcat(tmppath,"Master");
      }
      if ((fp=fopen(tmppath,"r")) == NULL) {
         /* some sort of problem (doesn't exist?).  whine and take a guess */
         error(NOTBCFG, "GiveV0Defaults: couldn't open %s",tmppath);
         DefineSymbol(bcfgfileindex, N_CONFORMANCE, "0x0320");
         DefineSymbol(bcfgfileindex, N_TYPE, "ODI");
         goto next;
      }
      found=0;
      while ((found == 0) && fgets(line, SIZE, fp) != NULL) {
         /* $interface lines don't have tabs & we only want certain lines */
         if ((line[0] == '$') && 
             (strlen(line) > 12) && (strlen(line) < 80) &&
             (sscanf(line, "$interface %s %s\n",name,level) == 2) &&
             (strncmp(name,"odi",3) == 0)) {
              char *dot;
            /* must use strdup since variable level on stack */
            strcpy(level2,"0x");
            dot=strtok(level,".");
            strcat(level2,dot);
            strcat(level2,strtok(NULL,"."));
            strcat(level2,"0");
            strcpy(level,level2);
            /* don't need to strdup level any more here since we call
             * strdup in DefineSymbol.  level is on stack.
             */
            DefineSymbol(bcfgfileindex, N_CONFORMANCE, level);
            DefineSymbol(bcfgfileindex, N_TYPE, "ODI");
            found++;
         }
      }
      fclose(fp);
      if (found == 0) {
         /* this is either one of two things:
          * a) an MDI driver in disguise since we know we are a #$version 0 
          *    driver and
          *    - "DLPI=true" not found in bcfg file
          *    - but Master file doesn't have "$interface odiSOMETHING number"
          *      which is always set by the xmog program.
          *    so we flag this bcfg as a bad ODI driver and return
          *    This is unlikely since we would have encountered an error earlier
          *    on, causing the bcfghaserrors variable to be set for this bcfg,
          *    and we'd never get to this point.
          * b) in the case of UW2.1, the SMC_9332 and SMC_8432 bcfg files
          *    (which use the DLPI smpw0 driver) forgot to add a DLPI=true line
          *    to their bcfg files, also causing a BADODI error, since the
          *    Master file (correctly) doesn't have $interface odiSOMETHING
          *    We also treat this as a bad ODI driver.  The DLPI= is used
          *    by NetInstall process to determine if it should modadmin -l
          *    in the odimem net lsl msm drivers but is not used by niccfg 
          *    or the dcu, effectively making it optional.
          * So rather than call error(BADODI, "bad ODI driver");return;
          * we just pretend this is a DLPI driver as per the above since
          * xmog always adds the $interface line and I doubt someone removed it.
          */
#if 0
         see above comments why we don't do this anymore
         error(BADODI, "bad ODI driver");
         /* pointless to call DefineSymbol here */
         return;
#endif
         /* DL_CURRENT_VERSION is 0x02 */
         DefineSymbol(bcfgfileindex, N_CONFORMANCE, "0x02");
         DefineSymbol(bcfgfileindex, N_TYPE, "DLPI");
         goto next;
      }
   }
next:
   /* define other V1 symbols here */
   /* inspection shows that odi/lsl/lslxmorg.c calls cm_getnbrd for ODI
    * drivers and most DLPI drivers call cm_getnbrd too.  So we increase
    * the limit from 4 to 999 (as if a machine will really have this many
    * slots :-)
    */
   DefineSymbol(bcfgfileindex, N_MAX_BD, "999");
   DefineSymbol(bcfgfileindex, N_FAILOVER, "false");
   /* WRITEFW is only mandatory for V1 *ISA* drivers (not all v1 drivers!) */
   if (HasString(bcfgfileindex, N_BUS, "ISA", 0) == 1) {/* BUS= value was made
                                                   * upper case in EnsureBus()
                                                   */
      DefineSymbol(bcfgfileindex, N_WRITEFW, "false");
   }
   DefineSymbol(bcfgfileindex, N_ACTUAL_RECEIVE_SPEED, "0"); /* 0=unknown */
   DefineSymbol(bcfgfileindex, N_ACTUAL_SEND_SPEED, "0"); /* 0=unknown */
   /* set PROMISCUOUS to something in case someone does a showvariable command
    * on it.  if it's chosen for idinstall then cmdops.c will check it for
    * "true" or "false", so this doesn't hurt.
    */
   DefineSymbol(bcfgfileindex, N_PROMISCUOUS, "unknown");/* for showvariable */
   DefineSymbol(bcfgfileindex, N_HELPFILE, "HW_network ncfgN.configuring_hardware.html");
   /* Lastly, do FILES= */
   strncpy(tmppath,bcfgfile[bcfgfileindex].location,SIZE-10);
   cp=strrchr(tmppath, '/');
   if (cp == NULL) {
      strcpy(tmppath,"");
   } else {
      *(cp+1)=NULL;
   }
   /* add all files in tmppath to FILES= */
   ProcessDirectory(bcfgfileindex, tmppath, "");
}

/* add all files found in tmppath (and below) to FILES= for given bcfgindex */
void
ProcessDirectory(int bcfgindex, char *root, char *start)
{
   DIR *dirp;
   struct dirent *dp;
   char path[SIZE+50];
   char startpath[SIZE];

   if (strlen(root)+strlen(start) > SIZE) return;  /* not enough space */
   strcpy(startpath,root);
   strcat(startpath,start);
   if ((dirp=opendir(startpath)) == NULL) {
      /* if tmppath is the bcfg directory, this shouldn't happen, since we 
       * were able to read the bcfg file from this directory a little while 
       * ago.  the user must have chmod'ed the directory to 000 or done a 
       * big rm to delete the dir.
       * however, if this is some doc subdirectory then it could very well
       * have bad permissions -- so we skip it
       */
      error(NOTBCFG, "Couldn't open directory %s",startpath);
      /* we'll define FILES to something for debugging purposes
       * this will add a bogus file name to FILES which will be
       * found as "missing" later on in the idinstall command
       * so user will be unable to install this driver.
       */
      /* start is relative to DSP, startpath is absolute pathname */
      snprintf(path,SIZE,"couldn't_open_directory_%s ",startpath);
      /* since path[] on stack must call strdup below */
      /* strdup of path not necessary now as DefineSymbol calls strdup now */
      DefineSymbol(bcfgindex, N_FILES, path);
      return;
   } else {   /* opendir succeeded, so process everything in the directory */
      struct stat sb;
      while ((dp=readdir(dirp)) != NULL) {
         if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
            continue;
         }
         snprintf(path,SIZE,"%s/%s",startpath,dp->d_name);
         if (stat(path,&sb) == -1) continue;
         if (S_ISDIR(sb.st_mode)) {
            if (strlen(start))
               snprintf(path,SIZE,"%s/%s",start,dp->d_name);
            else
               snprintf(path,SIZE,"%s",dp->d_name);
            ProcessDirectory(bcfgindex,root,path);  /* recursive call */
         } else if (S_ISREG(sb.st_mode)) {
            if (strlen(start))
               /* add space for looks */
               snprintf(path,SIZE,"%s/%s ",start,dp->d_name);
            else
               /* add space for looks */
               snprintf(path,SIZE, "%s ",dp->d_name);
            /* since path[] on stack must call strdup below. not any more. */
            DefineSymbol(bcfgindex, N_FILES, path);
         } else {
            /* we skip any FIFOS, special device nodes, etc. with a notice */
            error(NOTBCFG, "GiveV0Defaults: skipping %s for FILES=",path);
         }
      }
      closedir( dirp );
   }
alldone:;
   return;
}

void
PrimitiveFreeMem(union primitives *ptr)
{
   ulong t;

   switch((t=ptr->type)) {
      case STRINGLIST:
      {
         struct stringlist *next1,*next2;
         union primitives *z;

         free(ptr->stringlist.string);   /* free this text string */
         /* if we malloced a union then we must free union too */
         if (ptr->stringlist.uma != NULL) {
            free(ptr->stringlist.uma);
            ptr->stringlist.uma = NULL;
         }
         next1=ptr->stringlist.next;

         while (next1 != NULL) {   /* if next has something there */
            next2=next1->next;     /* remember what it is */
            free(next1->string);   /* free up string malloc'ed by parser */
            z=next1->uma;
            next1->uma = NULL;
#if 0
            free(next1);           /* free up union malloc'ed by parser */
#endif
            if (z != NULL) free(z); 
            next1=next2;           /* advance to next one */
         }
   
      }
      break;

      case SINGLENUM:     /* nothing to do or free up */
      case NUMBERRANGE:   /* nothing to do or free up*/
      break; 

      case STRING:
         /* parser malloc'ed the "words" space; we must free it up */
         free(ptr->string.words);
      break;

      case NUMBERLIST:
      {
         struct numberlist *next1,*next2;

         next1=ptr->numlist.next;

         while (next1 != NULL) {   /* if next has something there */
            next2=next1->next;     /* remember what it is */
            free(next1);           /* free up union malloc'ed by parser */
            next1=next2;           /* advance to next one */
         }
      }
      break;

      default:
         fatal("unknown bcfgfile strprimitive type %d",t);
         /* NOTREACHED */
      break;
   }
   ptr->type = UNDEFINED;
}

/* the lexer normally calls yywrap to handle EOF conditions (e.g., to
 * connect to a new file, as we do in this case.)  However, we call
 * ours zzwrap for handling multiple bcfg files
 */
int
zzwrap(void)   /* our bcfg wrapper routine */
{
   extern FILE *zzin;
   extern int errorsarefatal;
   char *next;
   extern char *getnextbcfg(void);
   char fullname[SIZE];
   extern char *rootdir;
   extern int Dflag;
   extern u_int indouble, insingle, quotestate;
   struct stat sb;
   char tmppath[SIZE], *cp;
   int ret;

   bcfglinenum=1;   /* we're done with this bcfg and can't call zzerror again */
   /* reset quote state for parser */
   indouble=0;  
   insingle=0;
   if (quotestate != NOQUOTES) {
      char *type="double *and* single";   /* for YSYD */

      if (quotestate == NSYD) type="double";
      if (quotestate == YSND) type="single";
      /* set bcfghaserrors variable for if below; don't attempt other checks */
      error(SYNTAX, "end of file reached - missing an ending %s quote", type);
   } else {
      /* quotestate=NOQUOTES; */
      fclose(zzin);  /* no matter if reading 1 bcfg or many */
      EnsureAllAssigned();  /* can call Error() */
      EnsureNumerics();  /* can call Error() */
      EnsureBus();   /* can call Error() */
      /* we call GiveV0Defaults last to catch any earlier errors */
      if (cfgversion == 0) GiveV0Defaults();  /* make it look like a V1 file; 
                                               * GiveV0Defaults can call Error()
                                               */

   }
   /* dealing with the filesystem is expensive.  In particular, if 
    * we're going to reject the bcfg in a few lines anyway don't
    * bother looking for the Driver.o or the ELF version information
    * since it might not exist.  Also, the bcfg may not define TYPE,
    * which GetDriverVersionInfo currently needs.  And GiveV0Defaults()
    * doesn't supply defaults if the bcfg has any errors already. 
    * with all of these combined we must check to see if the bcfg has errors
    * before proceeding through this section
    */
   if (!bcfghaserrors) {
      /* now figure out if we have a Driver.o, indicating a full DSP package
       * we should also check for Master/System/Node/etc. but we'll just
       * look for Driver.o.  This is used to set bcfgfile[].fullDSPavail.
       * 
       * TODO XXX:  we should run idtype or check cf.d/type to see what the
       *            current type is set for and look for those Driver.o types.
       *            The type file can contain multiple types (mp ccnuma)
       */
      strncpy(tmppath,bcfgfile[bcfgfileindex].location,SIZE-10);
      cp=strrchr(tmppath, '/');
      if (cp == NULL) {
         strcpy(tmppath,"Driver.o");
      } else {
         *(cp+1)=NULL;
         strcat(tmppath,"Driver.o");
      }
      ret=stat(tmppath,&sb);
      if (ret == -1) {
         /* no Driver.o, look for Driver_atup.o */
         strncpy(tmppath,bcfgfile[bcfgfileindex].location,SIZE-10);
         cp=strrchr(tmppath, '/');
         if (cp == NULL) {
            strcpy(tmppath,"Driver_atup.o");
         } else {
            *(cp+1)=NULL;
            strcat(tmppath,"Driver_atup.o");
         }
         ret=stat(tmppath,&sb);
      }
      if (ret == -1) {
         /* no Driver.o, no Driver_atup.o, look for Driver_mp.o */
         strncpy(tmppath,bcfgfile[bcfgfileindex].location,SIZE-10);
         cp=strrchr(tmppath, '/');
         if (cp == NULL) {
            strcpy(tmppath,"Driver_mp.o");
         } else {
            *(cp+1)=NULL;
            strcat(tmppath,"Driver_mp.o");
         }
         ret=stat(tmppath,&sb);
      }
      if (ret == -1) {
         /* no Driver.o, Driver_atup.o, Driver_mp.o: look for Driver_ccnuma.o */
         strncpy(tmppath,bcfgfile[bcfgfileindex].location,SIZE-10);
         cp=strrchr(tmppath, '/');
         if (cp == NULL) {
            strcpy(tmppath,"Driver_ccnuma.o");
         } else {
            *(cp+1)=NULL;
            strcat(tmppath,"Driver_ccnuma.o");
         }
         ret=stat(tmppath,&sb);
      }
      if ((ret != -1) && S_ISREG(sb.st_mode)) {
         /* XXX we assume other DSP files (Master/System/etc.) are 
          * present too.  This is separate and different than FILES=
          * and should be fixed.
          * we should check for all *mandatory* DSP files here
          * XXX TODO: if we don't find Driver.o and 
          *              we do find Driver_atup.o we immediately get here
          *              and if idtype says mp,
          *              but we never checked for the existence of Driver_mp.o
          *              so a relink would fail.  Do this check later.
          */
         bcfgfile[bcfgfileindex].fullDSPavail=1;
         /* now set .driverversion for this driver.  Note that we open
          * the first Driver[_something].o that we found to obtain the
          * version, irrespective of the idtype string in use.  This is
          * only an issue when idtype says "mp" but we check Driver_atup.o
          * for its version.  We assume that the driver version will be
          * the same for all types (atup, mp, ccnuma) of the driver.
          */
         if (GetDriverVersionInfo(bcfgfileindex,tmppath) == -1) {
            error(BADELF,"GetDriverVersionInfo failed");
         }
      } else {
         /* our Driver[_something].o not found or not regular file; 
          * no full DSP available so an idinstall of this driver later 
          * on would probably fail.  It's highly unlikely that all of the
          * driver code exists in the Space.c file...
          */
         bcfgfile[bcfgfileindex].fullDSPavail=0;
         error(BADDSP,"No DSP available for this driver");
      }
   }

   /* Finally, see if we previously encountered any errors and actually
    * do something about it.
    */
   if (bcfghaserrors) {  /* i.e. someone called rejectbcfg previously */
      int loop;
   
      /* free up all memory stored in bcfgfile.variable */
      for (loop=0; loop< MAX_BCFG_VARIABLES; loop++) {
         union primitives *ptr;

         if (bcfgfile[bcfgfileindex].variable[loop].strprimitive.type != 
             UNDEFINED){
            /* this bcfg file defined this variable.  free up all data 
             * associated with it and re-mark type as UNDEFINED for next
             * bcfgfile which will use this same bcfgindex
             */
            ptr=&bcfgfile[bcfgfileindex].variable[loop].strprimitive;
            PrimitiveFreeMem(ptr);
         }

         if (bcfgfile[bcfgfileindex].variable[loop].primitive.type != 
             UNDEFINED){
            /* this bcfg file defined this variable.  free up all data 
             * associated with it and re-mark type as UNDEFINED for next
             * bcfgfile which will use this same bcfgindex
             */
            ptr=&bcfgfile[bcfgfileindex].variable[loop].primitive;
            PrimitiveFreeMem(ptr);
         }
      }
      /* reset other counters */
      {
         struct reject *tmp;
         tmp=(struct reject *)malloc(sizeof(struct reject));
         if (tmp == NULL) {
            fatal("zzwrap: malloc for struct reject failed");
            /* NOTREACHED */
         }
         tmp->next = rejectlist;
         tmp->pathname = bcfgfile[bcfgfileindex].location;
         if (bcfgfile[bcfgfileindex].driverversion != NULL) {
            free(bcfgfile[bcfgfileindex].driverversion);
            bcfgfile[bcfgfileindex].driverversion = NULL;
         }
         tmp->reason = bcfghaserrors;
         rejectlist = tmp;
      }
      bcfghaserrors=0; /* reset global to start clean next time through */
      bcfgfile[bcfgfileindex].version=-1;  /* not really needed */
      bcfgfile[bcfgfileindex].driverversion = NULL;
      bcfgfile[bcfgfileindex].fullDSPavail=-1; /* not really needed */
      /* lastly, see if we should stop right now */
      if (errorsarefatal) {
         fatal("FATAL: got an error and -e supplied");
      }
   } else {    /* no errors found in this bcfg file */
      bcfgfile[bcfgfileindex].version=cfgversion;
      bcfgfileindex++;   /* since no errors found */
      /* can we proceed further? */
      if (bcfgfileindex == MAXDRIVERS) {
         extern DIR *rootdirp, *drvdirp;

         /* even though we are associated with a bcfg don't mark failure */
         error(NOTBCFG, "zzwrap: recompile with MAXDRIVERS higher than %d",
                 MAXDRIVERS);
         /* here we must short-circuit some of the routines:
          * 1) closedir the directories that getnextbcfg has open
          * 2) turn off debug
          * 3) return 1 to indiciate all done 
          */
         closedir(rootdirp);
         closedir(drvdirp);
         /* always change global version and debug variables back to default */
         if (!Dflag) cfgdebug=0; /* turn off debug for bcfgs unless -D */
         cfgversion=0;  /* assume next bcfg will be a UW2.1 bcfg file */
         return(1);  /* there may be more files to process but we simply
                      * cannot handle any more without recompiling this a.out!
                      */
      }
   }
   /* in all cases change global version and debug variables back to default */
   if (!Dflag) cfgdebug=0; /* turn off debug for other bcfgs unless -D */
   cfgversion=0;  /* assume next bcfg will be a UW2.1 bcfg file */

anotherfile:
   /* remember that we're in the zzlex() loop and we've just returned from
    * a call to zzlook. zzlex calls zzwrap which gets us here.  We must
    * tell next pass through zzlook() that we must go back to our
    * INITIAL state so that if this bcfg had an error the next one will
    * start off on the right foot and not immediately encounter an error
    * because we're still in the wrong state
    */
   ResetBcfgLexer(); /* change state back to INITIAL for next call to zzlook */

   if ((next=getnextbcfg()) == NULL) {
      return(1);  /* no more files to process */
   }
   snprintf(fullname,SIZE,"%s/%s",rootdir,next);
   zzin=fopen(fullname, "r");  /* next file for lexer to process */
   if (zzin == (FILE *) NULL) {
      error(NOTBCFG, "zzwrap: could not open %s",fullname);
      goto anotherfile;
   }
   /* ok, we could open up the file.  mark it in bcfgfile[] for later */
   bcfgfile[bcfgfileindex].location=strdup(fullname);
   return(0);   /* tell lexer to continue scanning; zzin has new file
                 * and bcfgfileindex has been incremented as necessary
                 */
}
