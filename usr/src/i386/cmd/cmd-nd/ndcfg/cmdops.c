#pragma ident "@(#)cmdops.c	29.7"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */
#ifdef DMALLOC
#define uint unsigned int
#define ulong unsigned long
#define boolean_t int
#endif

#define _KMEMUSER

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <priv.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/mod.h>  /* MODMAXNAMELEN */
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/wait.h>
#include <sys/sysi86.h>
#include <sys/privilege.h>
#include <sys/secsys.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ksym.h>
#include <sys/elf.h>
#define _KERNEL
#include <sys/metrics.h>
#undef _KERNEL
#include <sys/vmparam.h>
#include <sys/ksym.h>
#include <sys/dlpi.h>
#include <sys/scodlpi.h>
#include <sys/stropts.h>

#undef ERR  /* the version in regset.h isn't what we want */
/* avoid #define ERR in <sys/regset.h> and <curses.h> */
#include <curses.h> /* for lines */
#include <term.h>   /* for lines */
#include <termios.h>   /* Myraw, Mynoraw */

#include "common.h"

#ifdef DMALLOC_STRDUP
char *
strdup(s1)
const char *s1;
{
   char * s2 = malloc(strlen(s1)+1);

fprintf(stderr,"strdup:'%s' @ 0x%x\n",s1,s2);

   if (s2)
      strcpy(s2, s1);
   return(s2);
}
#endif


u_int tclmode=0;
extern uint_t cm_bustypes; /* from resmgr stored at key RM_KEY */
extern char delim;

/* directory where documentation will reside */
#define DOCDIR "/usr/lib/scohelp/en_US.ISO8859-1/HW_netdrivers"

/* DOCDIR2 is DOCDIR with any forward slashes escaped (for sed) */
#define DOCDIR2 "\\/usr\\/lib\\/scohelp\\/en_US.ISO8859-1\\/HW_netdrivers"

/* MAXNETX says how many netX drivers we can have on the system at any
 * one point in time.  i.e. net0, net1, net2, ... net99
 * DO NOT CHANGE THIS; TOO MANY PLACES ASSUME net0 THROUGH net99 NOW.
 */
#define MAXNETX 100

/* MAXNETXIDTUNES is max number of NETX scope idtune commands. Since
 * they are imbedded in custom parameters we limit to 10
 */
#define MAXNETXIDTUNES 10

char DLM[SIZE];  /* for the modules that will be be our idbuild -M line */

char infopath[SIZE];
char initpath[SIZE];
char removepath[SIZE];
char listpath[SIZE];
char reconfpath[SIZE];
char controlpath[SIZE];

char modlist[SIZE];  /* list of modules */

extern void AddToDLMLink(char *);

struct help help[]=
{
{SEC_GENERAL, "more", "more on|off - turns more on or off"},
{SEC_GENERAL, "debug", "debug decimal_number-set permanent debugging level to "
          "decimal_number\n(#$debug in a bcfg file sets debugging for "
          "that file only)"},
{SEC_GENERAL, "tcl", "tcl 0|1 - turn tcl mode off or on"},
{SEC_FILE, "loaddir", "loaddir pathname - load all bcfg files contained in\n"
          "pathname hierarchy(usually /etc/inst/nics/drivers)"},
{SEC_FILE, "loadfile","loadfile pathname_to_bcfg_file - load the specified\n"
          "bcfg file"},
{SEC_GENERAL, "help",  "shows help on a variety of commands"},
{SEC_BCFG, "count", "count - show number of bcfg files loaded"},
{SEC_ELF, "version", "version - show version string from Driver.o associated\n"
           "with this bcfg file"},
{SEC_FILE,"location","location-show full path name to reach each bcfg \n"
                     "loaded.  see bcfgpathtoindex command."},
{SEC_BCFG, "driver","driver - show DRIVER_NAME= from each bcfg loaded"},
{SEC_SHOW,"showname","showname indexnumber - show the bcfg NAME= for the given "
              "indexnumber"},
{SEC_SHOW, "showlines","showlines <bcfgindex> <bcfgvariable> - show the lines "
              "for bcfgfile variable <bcfgvariable>"},
{SEC_SHOW, "showfailover","showfailover topology - show all index(es) which\n"
              "1) have FAILOVER=true in bcfg AND\n"
              "2) match topology argument (remember TOPOLOGY can\n"
              "   be multivalued in bcfg) AND\n"
              "3) have a valid BUS relative to what we're running " 
              "ndcfg on"},
{SEC_SHOW, "showtopo","showtopo <topo_arg> - shows all cards to install on\n"
              "machine that have topology <topo_arg>"},
{SEC_SHOW, "showserialttys","showserialttys - show all serial device names\n"
              "available for use in /dev/term.  no arguments."},
{SEC_SHOW, "showvariable","showvariable <bcfgindex> <bcfgvariable>\n"
               "show a given variable from a bcfg file.\n"
               "Examples:\n"
               "   showvariable 2 BUS\n"
               "   showvariable 10 NAME"},
{SEC_SHOW, "showbus","showbus indexnumber - show the bcfg BUS= for the given "
              "indexnumber"},
{SEC_SHOW,"showindex","showindex drivername-show index(es) for each drivername "
             "\n(matches with DRIVER_NAME= in bcfg)"},
{SEC_RESMGR, "resmgr", "dump contents of in-core resmgr like resmgr(1M)"},
{SEC_RESMGR, "resdump","resdump [key] - print ALL parameters from in-core "
             "resmgr\n"
             "but not the associated value -- use resget for that.\n"
             "without any arguments dumps contents of in-core resmgr\n"
             "can also dump all parameter/value pairs if given single key arg"},
{SEC_GENERAL,"quiet","quiet - don't print encountered (bcfg) errors to stderr"},
{SEC_GENERAL, "verbose","verbose - print encountered (bcfg) errors to stderr"},
{SEC_GENERAL, "quit","quit - leave this program"},
{SEC_GENERAL, "bye","bye - leave this program"},
{SEC_GENERAL, "exit","exit - leave this program"},
{SEC_GENERAL, "dangerousisaautodetect",
               "dangerousISAautodetect get <bcfgindex>   - retrieve current "
               "params\n"
               "              set <element> p1=v1...pN=vN - set params\n"
               "on firmware"},
{SEC_GENERAL, "auths","auths - print working and max set authorizations"},
{SEC_RESMGR,  "resshowunclaimed","resshowunclaimed [lan|wan] - walk through\n"
              "resmgr searching for unclaimed (no driver assigned yet) \n"
              "BOARD_IDS which match all loaded bcfg BOARD_IDS and BUS.  \n"
              "returns matching index(es) or ERROR if none found"},
{SEC_SHOW,"showalltopologies","showalltopologies [LAN|WAN]\n"
              "shows all possible LAN or WAN topologies(based on argument)\n"
              "that can be used on this machine based on currently loaded\n"
              "bcfg files.  filtered by applicable BUS.\n"
              "use output of this command as input to showtopo command(and\n"
              "also showserialttys command if SERIAL appears in list)"},
{SEC_SHOW, "showrejects","showrejects - show bcfgs not considered valid and "
               "the reason(s):\n"
               "syntax                  -> general syntax error\n"
               "sections                -> problem with sections\n"
               "versions                -> problem with #$version\n"
               "bad_variable            -> invalid variable name\n"
               "multiple_defines        -> variable multiply defined\n"
               "no_multivalue_variables -> some variable illegally defined as "
                     "multivalued\n"
               "not_true/false          -> some variable didn't use \"true\""
               " or \"false\"\n"
               "na_bus                  -> bcfg not applicable for this bus\n"
               "mand._symb._not_defined -> mandatory symbol not defined\n"
               "bad_number              -> bad number"},
{SEC_GENERAL, "bcfghasverify",
              "bcfghasverify <bcfgindex> - does the ISA driver corresponding\n"
              "to the given bcfgindex have a verify routine?"},
{SEC_GENERAL, "isaautodetect",
               "ISAautodetect get <bcfgindex>   - retrieve current params\n"
               "              set <element> p1=v1...pN=vN - set params\n"
               "on firmware"},
{SEC_GENERAL, "getisaparams","getisaparams <bcfgindex> - retrieve all ISA \n"
              "parameters for user to complete.  filtered by already-used\n"
              "IO/IRQ/DMA"},
{SEC_SHOW, "showcustomnum","showcustomnum <bcfgindex> - show how many custom\n"
              "parameters the given bcfg file has"},
{SEC_SHOW, "showcustom","showcustom <bcfgindex> <custom_number> - display a\n"
              "specific custom parameter in its entirety"},
{SEC_GENERAL,"idinstall","idinstall args - install driver into the kernel.\n"
 "Usage: idinstall <bcfgindex> <actual_topology> <failover> <lanwan>\n"
 "   failover is \"0\" if not for failover else device name(netX) for primary\n"
 "   lanwan must be 1 for LAN and 2 for WAN.\n"
 "   [IRQ=x [DMAC=x] [IOADDR=x] [MEMADDR=x]] -ISA only\n"
 "   [KEY=x] - (from resshowunclaimed) PCI/EISA/MCA only\n"
 "   [OLDIOADDR=x] - ISA only\n"
 "   [BINDCPU=x] [UNIT=x] [IPL=x] [ITYPE=x]     - Advanced only\n"
 "   [__CHARM=x] - control mdi_printcfg(D3mdi) x=1(default)=quiet x=0=verbose\n"
 "   [CustomParam1=val1 CustomParam1_=choice1\n"
 "    CustomParam2=val2 CustomParam2_=choice2\n"
 "    CustomParamN=valN CustomParamN_=choiceN]"},
{SEC_GENERAL,"idmodify","idmodify <NETCFG_ELEMENT>\n"
              "PARAM1=NewValue1 PARAM2=NewValue2 PARAM3=NewValue3 ...\n"
              "remember custom parameters need specify both param=newval and\n"
              "param_=newchoice as arguments\n.  Supply the OLDIOADDR=x-y\n"
              "argument if ISA and changing the IO address"},
{SEC_GENERAL,"idremove","idremove <NETCFG_ELEMENT> <failover>\n"
             "failover is either 0 or 1"},
{SEC_GENERAL,"!","!command [arg(s)] - run UNIX command from subshell"},
{SEC_GENERAL,"unloadall","unloadall - unload all bcfg files from memory"},
{SEC_GENERAL,"clear","clear - clear the screen"},
{SEC_GENERAL,"sysdat","sysdat - show sysdat information"},
{SEC_GENERAL,"nlist","nlist symbol [newvalue] - show symbol contents in kmem"},
{SEC_GENERAL,"xid","xid <NETCFG_ELEMENT> - send DL_XID_REQ to device "
             "associated with element"},
{SEC_GENERAL,"test","test <NETCFG_ELEMENT> - send DL_TEST_REQ to device "
             "associated with element"},
{SEC_GENERAL, "getallisaparams","getallisaparams <NETCFG_ELEMENT> - retrieve "
              "all ISA \n"
              "parameters for user to complete.  no filtering takes place.\n"
              "That is, if no parameters are available, nothing is returned\n"
              "this differs from getisaparams which works on a\n"
              "complete *set* of ISA parameters.  getallisaparams just tells\n"
              "you the raw facts -- you must interpret the results\n"
              "typically called when you want to know the alternate choices\n"
              "for an existing element"},
{SEC_RESMGR,"resget","resget key parameter[,type] -get parameter from in-core\n"
              "resmgr at the given key.\n"
              "If the parameter isn't known to libresmgr you must supply\n"
              "the type value, where type is a single character interpreted\n"
              "as follows:\n"
              "  n - numeric\n"
              "  r - numeric range\n"
              "  s - string\n"
              "Example:   resget 3 FOO,s   gets FOO from key 3 as a string"},
{SEC_RESMGR,"resput","resput key parameter[,type]=value   - this command puts\n"
              "the given parameter into the in-core resmgr at the given key.\n"
              "If the parameter isn't known to libresmgr you must supply\n"
              "the type value, where type is a single character interpreted\n"
              "as follows:\n"
              "  n - numeric\n"
              "  r - numeric range\n"
              "  s - string\n"
              "the delimiter is space but can be changed with ndcfg -d X\n"
              "Example:   resput 3 FOO,s=this is my string"},
{SEC_RESMGR,"resshowkey","resshowkey <NETCFG_ELEMENT>    - shows the resmgr\n"
              "key which contains ncfg element NETCFG_ELEMENT"},
{SEC_FILE,    "bcfgpathtoindex","bcfgpathtoindex <path> - show the\n"
              "current bcfg index for path name <path>.  Must be absolute\n"
              "path name.  See location command."},
{SEC_RESMGR,"showisacurrent","showISAcurrent <NETCFG_ELEMENT> - shows the ISA\n"
              "attributes in the resmgr associated with NETCFG_ELEMENT"},
{SEC_RESMGR,"showcustomcurrent","showCUSTOMcurrent <NETCFG_ELEMENT> - shows\n"
              "the CUSTOM parameters and values currently associated with\n"
              "NETCFG_ELEMENT"},
#if 0
  not used and clutters up help output
{SEC_SHOW, "showhelpfile","showhelpfile <bcfgindex> - show the HELPFILE=\n"
              "line in the bcfgfile for <bcfgindex>"},
#endif
{SEC_SHOW,"showdriver","showdriver indexnumber-show the bcfg DRIVER_NAME= for "
              "the given indexnumber"},
{SEC_CA, "pcishort","pcishort [reskey] - show PCI information in short \n"
              "format.  An optional resmgr key can be supplied"},
{SEC_CA, "pcilong","pcilong [reskey] - show PCI information in long \n"
              "format.  An optional resmgr key can be supplied"},
{SEC_CA, "pcivendor","pcivendor vendorid - show PCI vendor for the given "
              "vendorid  - vendorid can be either hex, decimal, or octal"},
{SEC_CA, "eisashort","eisashort [reskey] -show EISA information in short \n"
              "format.  An optional resmgr key can be supplied"},
{SEC_CA, "eisalong", "eisalong [reskey]  -show EISA information in long\n"
              "format.  An optional resmgr key can be supplied"},
{SEC_CA, "i2oshort","i2oshort [reskey] - show I2O information in short \n"
              "format.  An optional resmgr key can be supplied"},
{SEC_CA, "i2olong","i2olong [reskey] - show I2O information in long \n"
              "format.  An optional resmgr key can be supplied"},
{SEC_CA, "mcashort","mcashort [reskey] - show MCA information in short\n"
              "format.  An optional resmgr key can be supplied"},
{SEC_CA, "mcalong","mcalong [reskey] - show MCA information in long\n"
              "format.  An optional resmgr key can be supplied"}, 
{SEC_CA, "orphans","orphans - show smart bus network cards detected in the \n"
              "system that don't have a .bcfg files to use it"},
{SEC_ELF,"getstamp","getstamp filename - get version stamp from filename"},
{SEC_ELF,"stamp","stamp filename text - imbed version information stored\n"
              "in text in filename - text can have spaces in it"},
{SEC_FILE,"elementtoindex","elementtoindex <NETCFG_ELEMENT> - shows\n"
              "the bcfgindex for NETCFG_ELEMENT"},
{SEC_GENERAL,"promiscuous","promiscuous - show if installed NIC(s) are "
             "capable of promiscuous mode"},
{SEC_HPSL,"hpsldump", "hpsldump - show all hot plug structures"},
{SEC_HPSL,"hpslsuspend", "hpslsuspend <NETCFG_ELEMENT> - send CFG_SUSPEND "
              "to driver"},
{SEC_HPSL,"hpslresume", "hpslresume <NETCFG_ELEMENT> - send CFG_RESUME "
              "to driver"},
{SEC_HPSL,"hpslgetstate", "hpslgetstate <NETCFG_ELEMENT> - get current "
              "hpsl statistics for loaded driver represented by element"},
{SEC_HPSL,"hpslcanhotplug", "hpslcanhotplug <NETCFG_ELEMENT> - determine if "
              "driver represented by <NETCFG_ELEMENT> is both ddi 8 and\n"
              "hotplug capable"}, 
{SEC_BCFG,"gethwkey", "gethwkey <NETCFG_ELEMENT> - show all current "
              "hardware attributes for device represented by <NETCFG_ELEMENT>"},
{SEC_GENERAL,NULL,NULL},
};

#define NAMESZ 15 /* module name size, includes the null char */
struct System {    /* System file structure for devices */
   char  name[NAMESZ];  /* module name */
   char  conf;    /* Y/N - Configured in Kernel */
   char  conf_static;   /* Configured statically ($static) */
   long  unit;    /* unit field value */
   short ipl;     /* ipl level for intr handler */
   short itype;      /* type of interrupt scheme */
   short vector;     /* interrupt vector number */
   long  sioa;    /* start I/O address */
   long  eioa;    /* end I/O address */
   unsigned long scma;  /* start controller memory address */
   unsigned long ecma;  /* end controller memory address */
   short dmachan; /* DMA channel */
   int   bind_cpu;   /* bind the module to a cpu number */
   int   over;    /* original version of sdevice file */
};

struct patch {
   char symbol[SIZE];
   unsigned long newvalue;
};

#if 0

use this when we have multiple types, not just STRINGLIST

struct printers {
   char *(*func)(int, int);
} printer[]={
/* defines for primitive types */
      /* see common.h */
 {UndefinedPrint},     /* UNDEFINED 0 */
 {SingleNumPrint},     /* SINGLENUM 1 */
 {NumberRangePrint},   /* NUMBERRANGE 2 */
 {StringPrint},        /* STRING 3 */
 {NumberListPrint},    /* NUMBERLIST 4 */
 {StringListPrint},    /* STRINGLIST 5 */
};
#endif

char *
UndefinedPrint(int index, int variable)
{
   fatal("In UndefinedPrint");
}

char *
SingleNumPrint(int index, int variable)
{
   fatal("In SingleNumPrint");
}

char *
NumberRangePrint(int index, int variable)
{  
   fatal("in NumberRangePrint");
}

char *
StringPrint(int index, int variable)
{
   fatal("in StringPrint");
}

char *
NumberListPrint(int index, int variable)
{
   fatal("in NumberListPrint");
}

/* if StringListPrint returns non-NULL, the caller must call free() on the 
 * return value.  Note this doesn't work for CUSTOM[] variables since 
 * they intentionally span multiple lines and we skip any line that
 * starts with a newline.
 * StringListPrint returns NULL if nothing was found.
 */
char *
StringListPrint(int index, int variable)
{
   stringlist_t *sp;
   u_int size;
   char tmp[LISTBUFFERSIZE]; /* necessary for bcfgs that define lots of text
                              * typically huge MEM= or PORT= lines
                              */
   char *ret;
   int addspace;

   tmp[0]='\0';
   sp=&bcfgfile[index].variable[variable].strprimitive.stringlist;
   if (sp->type != STRINGLIST) return(NULL); /* could be UNDEFINED, etc. */
   while (sp != NULL) {
      if (*sp->string != '\n') {
         strcat(tmp,sp->string);
         /* We want to append a space if
          * a) we're not at the end and
          * b) the next WORD from parser isn't a ' or " all by itself, 
          *    indicating an apostrophe or double quote 
          * c) this character isn't a ' or "
          * bcfglex.l passes up a ' and " as a WORD all by itself
          */
         addspace=1;

         /* if next is null then don't add space */
         if (sp->next == NULL) addspace=0;

         /* if next character is a ' or " then don't add a space */
         if ((sp->next != NULL) && 
             (strlen(sp->next->string) == 1) &&
             ((*sp->next->string == '\'') || 
              (*sp->next->string == '"'))) 
                addspace=0;

         /* if the last word (from parser) in our stringlist is a 
          * newline then don't add space 
          */
         if ((sp->next != NULL) &&
             (strlen(sp->next->string) == 1) &&
             (*sp->next->string == '\n') &&
             (sp->next->next == NULL)) 
                addspace=0;

         /* if this character is a ' or a " then don't add a space */
         if ((strlen(sp->string) == 1) && 
             ((*sp->string == '\'') || (*sp->string == '"'))) 
                addspace=0;

         if (addspace) strcat(tmp," ");
      }
      /* Pstdout("%s",sp->string); */
      sp=sp->next;
   }
   if ((size=strlen(tmp)) == 0) {
      return(NULL); /* can't return "NOT SET" since caller will try to free it*/
   }
   ret=malloc(size+1);  /* need that null byte on the end */
   if (ret == NULL) {
      fatal("StringListPrint: malloc failed!");
      /* NOTREACHED */
   }
   strcpy(ret,tmp);

   return(ret);
}

/* how many separate lines are in this StringList?
 * The only way you'll have multiple lines is if you have enclosed the line 
 * in single or double quotes and have a \n inside the quote.
 * we also take care of the 
 *  EXAMPLE 1:
 *       CUSTOM[x]='
 *       line1
 *       line2
 *       '
 *   (this has 3 lines - skip first blank line)
 * vs.
 *  EXAMPLE 2:
 *       CUSTOM[x]='line1
 *       line2
 *       line3
 *       '
 *   (this has 4 lines)
 * vs
 *  EXAMPLE 3:
 *       CUSTOM[x]='line1'
 *   (this has 1 line)
 * vs
 *  EXAMPLE 4:
 *       CUSTOM[x]='line1
 *       line2'
 *   (this has 2 lines)
 * cases.   We can also use this routine for PRE_SCRIPT, POST_SCRIPT, IDTUNE[x].
 *
 * IMPORTANT NOTE:  Because of the way that we count lines, and in particular
 *  how a line can end with text newline quote OR
 *                          text quote
 *  you cannot use the results of this routine to do a direct comparision
 *  but can only say if StringListNumLines > Some Minimum Number
 *  either that or use StringListLineX to see if the last line is blank!
 */
int
StringListNumLines(int index, int variable)
{
   stringlist_t *sp;
   u_int numlines=0;
   u_int sawbad=0;

   sp=&bcfgfile[index].variable[variable].strprimitive.stringlist;
   if (sp->type != STRINGLIST) return(-1);  /* could be UNDEFINED, etc. */
   /* eat blank lines (as in EXAMPLE 1) until we hit text.  Note this makes
    * things that depend on blank lines invalid!  problem is that some
    * custom parameters start with CUSTOM[x]="line1   while others start with
    *                              CUSTOM[x]="
    *                              line1
    * so to make things consistent we eat blank lines.  this does have
    * unpleasant side effects though as noted above.  This makes things like
    *       CUSTOM[x]="
    *       (blank line)
    *       (blank line)
    *       "
    * look like they have _no_ lines!
    */
   do {
      if ((strlen(sp->string) == 1) && (*sp->string == '\n')) {
         sp=sp->next;
      }
   } while ((strlen(sp->string) == 1) && (*sp->string == '\n'));

   if (sp != NULL) {   /* we could be at the end of the list now */
      numlines++;
   }

   while (sp != NULL) {
      if ((strlen(sp->string) == 1) && (*sp->string == '\n')) {
         numlines++;
      }
      sp=sp->next;
   }
   return(numlines);
}

/* return the line indicated.  must free(3) it unless NULL 
 * lineX starts with 1
 * Because of how lines can end, you should ensure that strlen of
 * what you get back is > 1 and that it isn't a newline before
 * you rely on the contents.
 */
char *
StringListLineX(int index, int variable, int lineX)
{
   stringlist_t *sp;
   char line[SIZE];
   int curline=0;

   line[0]='\0';
   sp=&bcfgfile[index].variable[variable].strprimitive.stringlist;
   if (sp->type != STRINGLIST) {
      notice("StringListLineX(%d,%d,%d): UNDEFINED",
            index,variable,lineX);
      return(NULL);   /* "error" */
   }

   if (lineX <= 0) {
      notice("StringListLineX: lineX is %d",lineX);
      return(NULL);   /* "error" */
   }

   /* get to line we care about */
   do {
      if ((strlen(sp->string) == 1) && (*sp->string == '\n')) {
         sp=sp->next;
      }
   } while ((strlen(sp->string) == 1) && (*sp->string == '\n'));

   if (sp == NULL) {     /* above loop could have taken us to the end */
      notice("StringListLineX: ate blank lines and ended up with nothing left");
      return(NULL);
   }

   curline++;

   while (sp != NULL && curline != lineX) {
      if ((strlen(sp->string) == 1) && (*sp->string == '\n')) {
         curline++;
      }
      sp=sp->next;
   }

   /* now read until we see either end of linked list or a newline */
   while ((sp != NULL) && (*sp->string != '\n')) {
      strcat(line,sp->string);
      sp=sp->next;
      if ((sp != NULL) && (*sp->string != '\n')) strcat(line, " ");
   }
   
   return(strdup(line));
}

/* how many separate words are in this StringList? */
int
StringListNumWords(int index, int variable)
{
   stringlist_t *sp;
   u_int numwords=0;
   u_int sawbad=0;

   sp=&bcfgfile[index].variable[variable].strprimitive.stringlist;
   if (sp->type != STRINGLIST) return(-1); /* could be UNDEFINED, etc. */
   while (sp != NULL) {
      if (sawbad) {   /* the *last* time through we saw an apostrophe */
         numwords--;  /* previous was ' and this is end of contraction */
         sawbad=0;
      }
      if ((strlen(sp->string) == 1) &&
          ((*sp->string == '\n') ||
             (*sp->string == '"')  ||
             (*sp->string == '\''))) {
         if ((*sp->string == '"') || (*sp->string == '\'')) sawbad=1;
         sp=sp->next;
         continue;
      }
      numwords++;
      sp=sp->next;
   }
   return(numwords);
}

/* 
 * originally this routine just returned a char * that you didn't have
 * to free() later but this falls apart since the lexer returns separate
 * words for contractions.  so the word "couldn't" (one word) returns 
 * "couldn", "'", and "t" as 3 separate words by the parser.
 * in order to put these back into one word and allow multiple callers
 * to this routine we follow the model in StringListPrint and force
 * the caller to free() the pointer if not NULL
 * this routine returns a pointer to the word which must be free()ed later
 * or NULL if X is too small (first word is 1) or too large.
 */
char *
StringListWordX(int index, int variable, int X)
{
   register int wordsfound=0;
   stringlist_t *sp;
   int good,apostrophe;
   char c;
   char tmp[SIZE * 4];

   tmp[0]='\0';
   if (X <= 0) {   /* programmer error */
      notice("StringListWordX: called with X<=0 (%d)",X);
      return(NULL);
   }
   sp=&bcfgfile[index].variable[variable].strprimitive.stringlist;
   if (sp->type != STRINGLIST) return(NULL); /* could be UNDEFINED, etc. */
   while ((sp != NULL) && (wordsfound != X)) {
      good=1;
      apostrophe=0;
      if ((strlen(sp->string) == 1) &&
          (((c=*sp->string) == '\n') || 
            (*sp->string == '"') || 
            (*sp->string == '\''))) {
         good=0;
         if ((c == '"') || (c == '\'')) apostrophe=1;
      } 
      if (good) {
         wordsfound++;
      } else if (apostrophe) {
         /* eat next word after apostrophe.  could be a newline or 
          * another apostrophe but we'll punt for those abnormal cases
          */
again:
         if (sp->next == NULL) goto done;
         sp=sp->next;
         if ((strlen(sp->string) == 1) &&
             (((c=*sp->string) == '\n') || 
               (*sp->string == '"') || 
               (*sp->string == '\''))) goto again;
         
      }
      if (wordsfound != X) sp=sp->next;
   }
   if (sp == NULL) return(NULL);
   /* ok, we got to the start of our word.  If the *next* word isn't
    * an apostrophe/single quote/double quote then we're done; return.
    * otherwise, we build our string and return
    */
   if (sp->next == NULL) return(strdup(sp->string)); /* implicit success */
   strcpy(tmp,sp->string);
   if ((strlen(sp->next->string) == 1) &&
       ((*sp->next->string == '\'') ||
        (*sp->next->string == '"' ))) {
      strcat(tmp,sp->next->string);  /* add on the ' or the "  */
      sp=sp->next->next;
      if (sp == NULL) return(strdup(tmp));
      strcat(tmp,sp->string);
   }
   return(strdup(tmp));
done:
   return(NULL);
}

struct termios raw_tty, original_tty;
#define TTYIN 0

void
Myraw(void)
{
   (void) tcgetattr(TTYIN, &original_tty);
   (void) tcgetattr(TTYIN, &raw_tty);    /** again! **/

   raw_tty.c_lflag &= ~(ICANON | ECHO);   /* noecho raw mode        */
   raw_tty.c_cc[VMIN] = '\01';   /* minimum # of chars to queue    */
   raw_tty.c_cc[VTIME] = '\0';   /* minimum time to wait for input */
   (void) tcsetattr(TTYIN, TCSADRAIN, &raw_tty);
}

/* due to a bug in curses noraw() that sets eof to 'D' (not control D)
 * we roll our own
 */
void
Mynoraw(void)
{
   (void) tcsetattr(TTYIN, TCSADRAIN, &original_tty);
}

/* command should not generate any output.  any output sent
 * to stdout or stderr will be treated as an error
 * if the exit code is non-zero
 */

#define CMDBUFSIZE (20 * 1024)  /* must not be bigger than LISTBUFFERSIZE 
                                 * or else calls to error/notice/Pstdout/etc
                                 * will truncate it
                                 */

char cmdoutput[CMDBUFSIZE];

/* runcommand runs the command and doesn't call error() if the
 * binary returns with a non-zero exit status
 * we don't need to call putenv repeatedly but it's all in one place.
 * NOTE:  this routine does not filter out the characters fed to the
 * shell via popen.  This means that the command can contain 
 * environment variables   i.e.    /bin/ls ${HOME}
 * as well as other characters interpreted by the shell ('*', etc.).
 * The command we run may be 
 * - hard coded within ndcfg
 * - from the .bcfg file (PRE_SCRIPT, CONFIG_CMDS etc)
 * Note the argument(s) to the command _can_ be generated by the user
 * if you use a DRIVER/GLOBAL scope custom parameter with freetext(__STRING__)
 * in it.  This means that the user can enter a '*' in the text, which
 * ndcfg will dutifully pass to runcommand to do a popen on 
 * '/etc/conf/bin/idtune -f -c FOO *'
 * which of course the shell will expand appropriately, possibly generating
 * an error.  
 * The Moral?  Be verrry careful with __STRING__ custom parameters, as
 * you may not know what the user typed.
 *
 * NOTE:  be sure that the command you want to run is part of the base package
 *        or that the binary is part of a package found in the nics depend file
 *        for both UW2.1 and Gemini...
 */
int
runcommand(int dolist, char *cmd)
{
   char command[SIZE];
   char buf[BUFSIZ];
   FILE *fp;
   int status;
   extern FILE *logfilefp;
   extern int jflag;

   /* thou shalt use absolute pathnames which lead to thy program lest you
    * encounter the wrath of trojan horses
    */
   if (*cmd != '/') {
      notice("'%s' isn't not absolute pathname - returning",cmd);
      return(-1);
   }

   /* CONFIG_CMDS is used by netinstall and ndcfg, so it may have ${IIROOT}
    * as an environment variable set
    * XXX TODO:  set IIROOT to root directory of driver type
    *  ODI & DLPI:  /etc/inst/nics/drivers
    *  MDI       :  /etc/inst/nd/mdi
    * ISL sets it to /.extra.d
    */
   putenv("IIROOT=/etc/inst/nd/mdi");

   /* see sh const char        sptbnl[]        = " \t\n";
    *        dfault(&ifsnod, sptbnl);
    *        which is how IFS is gets its default setting
    */
   putenv("IFS= \t\n"); /* meager Bourne shell security */
   /* some idtools use absolute path names when using system() but others
    * don't.  so give them a starting point.  who knows what the user's 
    * path is when they run ndcfg?
    */
   putenv("PATH=/sbin:/usr/sbin:/usr/bin:/etc");
   putenv("LD_LIBRARY_PATH=");
   putenv("LD_RUN_PATH=");

   strncpy(command,cmd,SIZE-10);
   /* we must send stderr to same place as stdout because popen doesn't handle
    * this for us
    */
   strcat(command," 2>&1");
   
   /* assorted routines like the odimem/idtune stuff assume that cmdouput
    * will always be zeroed out first as they can call things like atoi
    * on it after runcommand returns.
    */
   cmdoutput[0]='\0';

   if (logfilefp != NULL) {
      fprintf(logfilefp,"runcommand: '%s'\n",cmd);
   }

   if ((fp=popen(command,"r")) == NULL) { 
      /* pipe(2), fork(2), or fdopen(3) probably failed. 
       * we don't know yet if the command even exists
       */
      notice("runcommand: popen failed for command '%s'",command);
      return(-1);
   }
   while (fgets(buf, BUFSIZ, fp) != NULL) {
      /* program isn't supposed to generate lots of output hence small buffer */
      if (strlen(buf)+strlen(cmdoutput) > CMDBUFSIZE) {
         notice("runcommand: not adding following text to cmdoutput: %s",buf);
         goto done;
      }
      strcat(cmdoutput,buf);
   }
done:
   status=pclose(fp);   /* could be -1 */
   if (WIFEXITED(status)) {
      status=WEXITSTATUS(status);
   } else {
      status=-1;  /* probably terminated due to signal */
   }

   if (logfilefp != NULL) {
      if (jflag) {
         fprintf(logfilefp,"runcommand: cmd output:\n%s\n", cmdoutput);
      }
      fprintf(logfilefp, "runcommand: '%s' exit status=%d\n", cmd, status);
   }
   return(status);
}

/* docmd calls runcommand and checks the return status for errors.
 * calls error if status is non zero to indicate problem and
 * also returns that exit status up the line.
 */
int
docmd(int dolist, char *cmd)
{
   int status;

   status=runcommand(dolist,cmd);
   if (status < 0) {
      /* not absolute path name,
       * terminated due to signal,
       * popen failed
       */
      error(NOTBCFG,"problem executing command \"%s\"",cmd);
   } else
   if (status > 0) {
      error(NOTBCFG,"command \"%s\" failed with the following error: %s",
            cmd,cmdoutput);
   }
   return(status);
}

void
showrejects(void)
{
   struct reject *r;
   char tmp[SIZE];

   r=rejectlist;
   if (r == NULL) return;
   StartList(4,"PATHNAME",49,"REASON",30);
   while (r != NULL) {
                 /*1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 */
      snprintf(tmp,SIZE,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
         r->reason & SYNTAX     ? "syntax " : "",
         r->reason & SECTIONS   ? "sections " : "",
         r->reason & VERSIONS   ? "versions " : "",
         r->reason & BADVAR     ? "bad_variable " : "",
         r->reason & MULTDEF    ? "multiple_defines " : "",
         r->reason & MULTVAL    ? "no_multivalue_variables " : "",
         r->reason & TRUEFALS   ? "not_true/false " : "",
         r->reason & NOTBUS     ? "na_bus " : "",
         /* mand_symb_not_def means that a mandatory symbol
          * for the particular #$version is not defined.  It's not specific
          * to any particular bus type(ISA/PCI/EISA/MCA/etc).
          */
         r->reason & NOTDEF     ? "mand_symb_not_def " : "",
         r->reason & BADNUM     ? "bad_number " : "",
         r->reason & BADODI     ? "bad_ODI " : "",
         r->reason & ISANOTDEF  ? "mand_ISA_symb_not_def " : "",
         r->reason & EISANOTDEF ? "mand_EISA_symb_not_def " : "",
         r->reason & PCINOTDEF  ? "mand_PCI_symb_not_def " : "",
         r->reason & MCANOTDEF  ? "mand_MCA_symb_not_def " : "",
#ifdef CM_BUS_I2O
         r->reason & I2ONOTDEF  ? "mand_I2O_symb_not_def " : "",
#else
         "",
#endif
         r->reason & BADELF     ? "bad_ELF " : "",
         r->reason & BADDSP     ? "bad_DSP " : "",
         /* note you shouldn't ever see NOTBCFG and CONT here */
         r->reason & CONT       ? "cont " : "",
         r->reason & NOTBCFG    ? "notbcfg " : "");
      AddToList(2,r->pathname,tmp);
      r = r->next;
   }
   EndList();
}

void 
cmdshowindex(char *driver) 
{
   int loop,found;

   found=0;
   StartList(2,"BCFGINDEX",10);
   for (loop=0;loop<bcfgfileindex;loop++) {
      /* remember that CM_DRIVER_NAME is not multivalued - substring not ok */
      if (HasString(loop,N_DRIVER_NAME,driver,0) == 1) {
         char num[20];
         snprintf(num,20,"%d",loop);
         AddToList(1,num);
         found=1;
      }
   }
   if (!found) {
      error(NOTBCFG, "index(es) for driver %s not found",driver);
   } 
   EndList();
}

struct {
   char *heading;
   u_int fieldsize;
} MyList[100];
int ListCount;


char ListBuffer[LISTBUFFERSIZE];   /* XXX malloc this instead - large .bss */
u_int ListBufferSize;     /* how many chars are in ListBuffer right now */
int currentline;   /* for more mode -- how many lines have been displayed */

/* starts a new list for eventual output to stdout, barring any errors... 
 * order of arguments is char *, u_int
 * as in StartList(4,"NAME",5,"DESCRIPTION,20);
 * 4=number of arguments
 */
void
StartList(u_int numargs, ...)
{
   va_list ap;
   int loop;

   if (numargs & 1) {
      fatal("StartList: numelements must be even!");
      /* NOTREACHED */
   }

   if (nomoreoutput) return;

   /* if stdin is a pipe but stdout is to a screen then clear screen */
   if (!tclmode && !isatty(0) && isatty(1) && !nflag) {
      putp(clear_screen);
   }
   /* we don't want to call ClearError here in StartList but do want to call
    * it in EndList.  Why?
    * If a routine calls StartList after an earlier error(), we'll miss 
    * the error text.  This is important for tcl mode where we won't
    * see the error(s) until someone calls EndList
    * Also, if not in tcl mode and the routine doesn't call StartList at all
    * but just calls error directly and returns, we must have cmdparser.y
    * call EndList so that the error buffer will be cleared for the next
    * command.  We also call EndList in cmdparser.y section cmdlist because
    * a routine might forget to call EndList, and that's crucial for tcl
    * mode and non tcl mode.
    * routine GetCommands calls ClearError() just before calling yyparse so
    * the first command entered will have a clear error buffer.
    */
   ListBuffer[0]='\0';
   ListCount=0;
   currentline=0;    /* for moremode */
   ListBufferSize=0;
   va_start(ap,numargs);
   for (loop=0;loop<numargs/2;loop++) {
      MyList[ListCount].heading=va_arg(ap, char*);
      MyList[ListCount].fieldsize=va_arg(ap, u_int);
      if (hflag && currentline == 0) { /* fields may not always match heading */
         /* hflag set to 0 for tclmode so ok to printf */
         Pstdout("%-*s",MyList[loop].fieldsize,MyList[loop].heading);
      }
      ListCount++;
   }
   if (hflag) {
      Pstdout("\n");
      currentline=1;
   }
   va_end(ap);
}

/* Add more data to our list.  if in tclmode we don't print anything until
 * EndList gets called
 * If not in tclmode then print immediately, checking moremode and lines.
 * all arguments (except for numargs) are char *.
 */
void
AddToList(u_int numargs, ...)
{
   va_list ap;
   int loop;
   char tmp[LISTBUFFERSIZE];
   char junk;
   char *thestring;

   if (numargs != ListCount) {   /* programming error, catch it early on */
      fatal("AddToList: numargs != ListCount!");
   }

   /* if we got SIGINT no point in sending more stuff to screen */
   if (nomoreoutput) return;

   if (OpGotError) return;  /* no sense adding more to our list when we 
                             * have an error that will prevent us from printing
                             * our list anyway.  set when someone calls error()
                             * with reason=NOTBCFG
                             */
   va_start(ap,numargs);
   if (tclmode) {
      strcat(ListBuffer," {");
      for (loop=0;loop<numargs; loop++) {
         thestring=va_arg(ap, char *);
         /* due to assorted routines calling StringListPrint and blindly
          * taking the results to AddToList, we check to make sure we
          * won't be playing with NULL in our snprintf below...
          */
         if (thestring == NULL) thestring="";
         snprintf(tmp,LISTBUFFERSIZE,
                  " {%s {%s}} ",
                  MyList[loop].heading,thestring); 
         /* the 90 is totally arbitrary and will still allow 45 additional
          * calls to AddToList because we will still add a ' {} '
          * to ListBuffer even if the if statement below fails
          * Note we don't call strlen(ListBuffer) over and over again.
          */
         if (ListBufferSize + strlen(tmp) < LISTBUFFERSIZE - 90) {
            strcat(ListBuffer,tmp);
         }
      }
      strcat(ListBuffer,"} ");
      ListBufferSize += strlen(tmp) + 4; /* for the ' {' and '} '  */
   } else {   /* not tclmode */
      extern int aflag;
      for (loop=0;loop<numargs; loop++) {
         thestring=va_arg(ap, char *);
         /* due to assorted routines calling StringListPrint and blindly
          * taking the results to AddToList, we check to make sure we
          * won't be playing with NULL in our printf below...
          */
         if (thestring == NULL) thestring="";
         /* see comment at start of EndList() code.  It applies here too */
         if (aflag) {
            /* use tab as delimiter.  safe to do as bcfglex.l won't return
             * tabs in our stringlist so we know they won't be in data
             * tabs are passed up to bcfgparser.y as WHITESPACE which we
             * throw away
             */
            Pstdout("%s",thestring);
            if (loop != numargs -1) Pstdout("\t");
         } else {
            Pstdout("%-*s",MyList[loop].fieldsize,thestring);
         }
      }
      Pstdout("\n");
      if ((candomore > 0) && (moremode > 0)) {
         int ttyfd;
         if (++currentline % (lines-1) == 0) {
            fflush(stdout);  /* get stuff out of buffer */
            putp(enter_standout_mode);
            printf("--More--"); 
            fflush(stdout);
            putp(exit_standout_mode);
            fflush(stdout);
            Myraw();
            ttyfd=open("/dev/tty",O_RDONLY);  /* stdin could be a pipe */
            read(ttyfd,&junk,1);
            close(ttyfd);
            Mynoraw();
            if (junk != '\n') write(1,"\r        \r",10);
         }
      }
   }
   va_end(ap);
}

/* We cannot call Pstdout here as the general vsnprintf in
 * Pstdout causes an internal vsnprintf buffer to overflow, causing SIGSEGV.
 * this only happens on large lists in ListBuffer in tcl mode.
 * we must use printf here directly and print to logfilefp directly too.
 * N later adds that he fixed this by using a large automatic buffer
 * on the stack instead of a small 1K buffer which was causing the problem
 * we still don't call Pstdout here for performance reasons.
 *
 * EndList is usually called twice for every command entered by the user:
 * the first time when a well behaved routine calls StartList...EndList
 * the second time in cmdparser.y section cmdlist encounters a command
 * OR section cmd encounters an error.
 * 
 * we must call ClearError here since StartList can't.
 */
void
EndList(void)
{
   int length,loop;
   /* if in tcl mode AND someone hasn't already cleared out the buffer
    * by calling EndList previously then print message
    */

   /* allow calls to Pstdout/StartList/AddToList to work again */
   nomoreoutput = 0;

   if (tclmode) {
      if (OpGotError) {
         length=strlen(ErrorMessage);

         /* TCL freaks out when it encounters certain characters in the
          * returned string.  In particular, ErrorMessage can contain
          * the output from a command from runcommand() or docmd(), and
          * the output from the command can have illegal TCL characters.
          * we could escape the characters with a '\' but we instead 
          * substitute them with a space.  This way an error string
          * will not cause tcl to dump core.
          */
         for (loop=0; loop< length; loop++) {
            switch(ErrorMessage[loop]) {
               case '[':
               case ']':
               case '"':
               case '{':
               case '}':
                  ErrorMessage[loop]=' ';
                  break;
               default:
                  break;
            }
         }

         Pstdout("{%s: %s}\n",TCLERRORPREFIX,ErrorMessage);
      } else if (ListBuffer[0] != '\0') {  /* EndList called previously */
         /* Pstdout can now accept LISTBUFFER bytes in its snprintf */
         Pstdout("NOERROR %s\n",ListBuffer);
         ListBuffer[0]='\0';
      }
   }
   fflush(stdout);     /* for all modes in case stdout is a pipe */
   currentline=0;      /* for moremode */
   ListBufferSize=0;   /* zero characters in buffer right now */
   /* must call ClearError to get rid of Primed err message in ErrorMessage */
   ClearError();       /* always, even if we aren't in tcl mode */
}

struct topo topo[] =
{
 /* "ETHER" assumed throughout ndcfg code and in .bcfg files */
 {LAN, "ETHER"     , "Ethernet", 0x01},
 /* "TOKEN" assumed throughout ndcfg code and in .bcfg files*/
 {LAN, "TOKEN"     , "Token-Ring", 0x02},  /* should be Broken-Ring... */
 /* "ISDN" assumed throughout ndcfg code  and in .bcfg files*/
 {WAN, "ISDN"      , "ISDN", 0x04},
 /* "FDDI" assumed throughout ndcfg code  and in .bcfg files*/
 {LAN, "FDDI"      , "FDDI", 0x08},
 {LAN, "ATM"       , "ATM", 0x10},
/* X.25 is a bunch of standards:
 *  International Organization for Standardization (ISO) has published 
 *  - ISO 7776:1986 which is an equivalent to the LAPB standard
 *  - ISO 8208:1989 as an equivalent to the ITU-T 1984 X.25 Recommendation 
 *    packet layer. 
 *  - There are also the CCITT 1980 X.25 Recommendations too.
 * rather than print this mess which nobody will understand we just print X.25
 */
 {WAN, "X25"       , "X.25", 0x20},
/* Frame Relay is a "connection oriented" packet mode data service based on the
 * X.25 LAP-D standards. * Although based on a protocol similar in nature 
 * to X.25, Frame Relay can provide a higher-speed alternative for 
 * customers currently using X.25 services. This is because Frame Relay 
 * eliminates a significant portion of the processing that occurs within 
 * a X.25 network.  When X.25 was developed during the 1960s, it was 
 * necessary to incorporate a high degree of error detection and correction 
 * capability into the standards. This was required due to lower quality 
 * analog facilities that were prevalent in the public networks at the time. 
 * As Frame Relay will be typically used on high quality digital circuits, 
 * most of the error detection and correction features found in the X.25 
 * protocols have been eliminated. 
 */
 {LAN, "FRAMERELAY", "Frame Relay", 0x40},
 /* We want TOPOLOGY=OTHER bcfgs to show up in both LAN _and_ WAN menus 
  * since we don't know topo type it is we OR in both LAN and WAN bits
  */
 {LAN|WAN, "OTHER"     , "Other Topology", 0x80},

 {LAN|WAN, "BARRY", 
    "Barry's Asynchronous Rapidly Recursing Yoyo (BARRY)", 0x100}, 

 {LAN|WAN, "NATHAN", 
    "Nathan's Asynchronous Transfer High-Level Automated Network(NATHAN)", 
    0x200}, 

#define SERIALBIT 0x400
 {WAN, "SERIAL", "Serial", SERIALBIT},
};

u_int numtopos=sizeof(topo)/sizeof(topo[0]);

void
ShowAllTopologies(char *typestr)
{
   u_int bcfgloop,topoloop,bitmask,biggestlength,strlength;
   u_int biggestfullnamelength;
   u_int type,retstatus,isastatus;
   u_int numterms=0;

   strtoupper(typestr);
   if (strcmp(typestr,"LAN") == 0) {
      type=LAN;
   } else if (strcmp(typestr,"WAN") == 0) {
      type=WAN;
   } else {
      error(NOTBCFG,"ShowAllTopologies has a LAN or WAN argument");
      return;
   }
   bitmask=0;
   biggestlength=0;
   biggestfullnamelength=0;
   for (topoloop=0;topoloop<numtopos;topoloop++) {
      if ((strlength=strlen(topo[topoloop].name)) > biggestlength) 
         biggestlength=strlength;
      if ((strlength=strlen(topo[topoloop].fullname)) > biggestfullnamelength)
         biggestfullnamelength=strlength;
   }
   StartList(4, "TOPOLOGY",biggestlength+2,"FULLNAME",biggestfullnamelength+2);
   /* for each element, see if any of the above names exist in TOPOLOGY=
    * if so then OR in the value into bitmask
    */ 
   for (bcfgloop=0;bcfgloop<bcfgfileindex;bcfgloop++) {
      for (topoloop=0;topoloop<numtopos;topoloop++) {
         /* if all conditions are met then we OR in this topology into bitmask:
          * a) topology type is what we're looking for (argument to routine)
          * b) this bcfg not max'd out
          * c) bus type of bcfg file is applicable to this machine
          *    (this is taken care of for us when loading the bcfg file --
          *     see the showrejects command).  No code necessary for it here
          * d) (bus type is ISA and we have ISA bus and an ISA parameter
          *     set is available for this board) or BOARD_ID for bcfg 
          *    found in the resmgr
          *    Remember no MCA machine to date has ISA slots 
          */
         retstatus=0;
         isastatus=999;
         if ((type & topo[topoloop].type) &&
             (HasString(bcfgloop,N_TOPOLOGY,topo[topoloop].name,0) == 1) &&
             (AtMAX_BDLimit(bcfgloop) == 0) &&
             ( (  (HasString(bcfgloop,N_BUS,"ISA",0) == 1) && 
                  ((cm_bustypes & CM_BUS_ISA) == 1) && 
                  ((isastatus=ISAsetavail(bcfgloop)) == 1)
               ) || 
               (  (retstatus=ResBcfgUnclaimed(bcfgloop)) == 1
               )
             )
            ) {
            bitmask |= topo[topoloop].value;
         }
         if (isastatus == -1) {   /* ISAsetavail failure */
            error(NOTBCFG,"ShowAllTopologies: ISAsetavail failure");
            return;
         }
         if (isastatus == 0) {
            /* we know we called ISAsetavail and that a set isn't available
             * to use
             */
            notice("ShowAllTopologies: skipping bcfgindex %d; "
                   "no ISA set available to use",bcfgloop);
         }
         if (retstatus == -1) {   /* ResBcfgUnclaimed failed */
            error(NOTBCFG,"ShowAllTopologies: ResBcfgUnclaimed failed");
            return;
         }
      }
   }
   /* now go through and see what bits were set in our bitmask */
   for (topoloop=0;topoloop<numtopos;topoloop++) {
      if (bitmask & topo[topoloop].value) {
         AddToList(2,topo[topoloop].name,topo[topoloop].fullname);
      }
   }
#if 0
   /* if we didn't find any bcfg's with TOPOLOGY=SERIAL, then check
    * /dev/term for possibilities.  3rd party serial board vendors are 
    * unlikely to include bcfg files in their product...
    * see showserialttys command and ttysrch(4).  Since ISA serial
    * boards don't usually have Drvmap files we can't look for the
    * string "Communications Cards" text which dcu uses either.
    * the /dev/term approach is the best.
    * we stop after 4 terminals which is a performance increase rather than
    * read the entire directory, which could be a lengthy process, even
    * with optimized readdir/getdents caching.
    * NOTE:  no longer executed -- see showserialttys
    */
   if (((bitmask & SERIALBIT) == 0) && (type == WAN)) {
       DIR *dirp;
       struct dirent *direntp;
       
       dirp=opendir("/dev/term");
       if (dirp == NULL) goto done;
       while (((direntp = readdir(dirp)) != NULL) && (numterms < 4)) {
          numterms++;
          /* (void)printf("%s\n", direntp->d_name); */
       }
       closedir(dirp);
       if (numterms > 2) {    /* skip '.' and '..' */
          u_int serialoffset;

          /* ok, we found some terminals in /dev/term.  Now find the
           * offset for the serial driver and 
           * add it to our displayed list
           */
          for (topoloop=0;topoloop<numtopos;topoloop++) {
             if (topo[topoloop].value == SERIALBIT) break;
          }
          AddToList(2,topo[topoloop].name,topo[topoloop].fullname);
       }
   }
done:
#endif

   /* if no bcfgs found for topology type typestr and nothing found in
    * /dev/term then tell user nothing found
    */
   if ((bitmask == 0) && (numterms <= 2)) {  
      if (tclmode) {
         strcat(ListBuffer,"{ }");
      } else {
         error(NOTBCFG,"there are no bcfgs with topologies of type %s",typestr);
      }
   }

   EndList();
   return;
}

/* remember, we filter bcfgs by the busses on the machine earlier.
 * use the showrejects command to see the failed bcfgs and the reason
 * (i.e. na_bus means non-applicable bus for this bcfg)
 * Remember that if bcfg sets AUTOCONF=false you'll see BUS=ISA in output.
 * Added BUS to output below to determine if we should call getISAparams
 * later on if user chooses to add this card.
 * Remember that we can't use idinstall on smart boards (PCI/EISA/MCA) without
 * knowing the resmgr key, so the list produced by showtopo must only
 * include :
 *  - boards discovered by resshowunclaimed
 *  - ISA boards where the KEY= is not needed for later idinstall command.
 * We also don't show the board if there are MAX_BD instances of it
 * in resmgr right now.
 * NOTE: showtopo serial only shows bcfg files with TOPOLOGY=SERIAL.  to
 *       get a list of the devices in /dev/term, you should use 
 *       showserialttys command
 * NOTE: we only show ISA boards if
 *   - MAX_BD not exceeded
 *   - machine has ISA bus
 *   NOT IMPLEMENTED:  have 1 set of {IOADDR,IRQ,DMA,MEMADDR} available on
 *                     system
 */
void
ShowTopo(char *topoarg)
{
   u_int topoloop,bcfgloop,numfound;
   u_int boardnumber;
   int lanwan;
   char *driver_name;
   char *fullname;
   char *topologies;
   char *tmptopo;
   char *bus;
   int numautodetected,err;
   char ncfgelement[12];
   
   strtoupper(topoarg);
   numfound=0;
   for (topoloop=0;topoloop<numtopos;topoloop++) {
      if (!strcmp(topoarg,topo[topoloop].name)) {
         numfound=1;
         lanwan=topo[topoloop].type;
         break;
      }
   }
   if (!numfound) {
      error(NOTBCFG,"Unknown topology '%s';use one of the following:",
            topoarg);
      for (topoloop=0;topoloop<numtopos;topoloop++) {
          error(NOTBCFG,"%s",topo[topoloop].name);
      }
      return;
   }
   numfound=0;
   /* we BUS to list to determine if we should call getISAparams
    * later on
    * we add KEY in case of smart bus (from resshowunclaimed) for later
    * idinstall
    */
   /* DON'T CHANGE THIS ORDER UNLESS YOU CHANGE THE "fromShowTopo == 1"
    * CODE IN RESSHOWUNCLAIMED().
    */
   StartList(14,"KEY",3,"BCFGINDEX",7,"DRIVER_NAME",10,"NAME",53,
             "NCFGELEMENT",12,"BUS",5,"TOPOLOGIES",15);
   /* ResShowUnclaimed will calls AddToList to add the autodetected boards of 
    * this topology to our list will close resmgr when done 
    */
   if (lanwan == LAN) {
      tmptopo="LAN";
   } else if (lanwan == WAN) {
      tmptopo="WAN";
   } else if (lanwan == (LAN|WAN)) {
      tmptopo=NULL;
   } else {
      error(NOTBCFG,"ShowTop - unknown lanwan = %d",lanwan);
      return;
   }
   if ((numautodetected=ResShowUnclaimed(1, topoarg, tmptopo)) == -1) {
      error(NOTBCFG,"ShowTopo - returning due to errors in ResShowUnclaimed");
      return;
   }

   /* so now add the remaining ISA boards to our list, checking if we have
    * an ISA bus on this machine (MCA boxes will fail this test)
    * see confmgr/confmgr_p.c comment and code
    *    if (( _cm_bustypes & CM_BUS_MCA ) == 0 )
    *       _cm_bustypes |= CM_BUS_ISA;
    */
   if ((cm_bustypes & CM_BUS_ISA) == 1) {
      for (bcfgloop=0;bcfgloop<bcfgfileindex;bcfgloop++) {
         /* we must use substring match feature of HasString */
         if ((HasString(bcfgloop,N_BUS,"ISA",0) == 1) &&
             (HasString(bcfgloop,N_TOPOLOGY,topoarg,0) == 1)) {
            /* ok.  this bcfg is ISA and supports this topology.  If we 
             * can allow
             * another instance of this driver then add it to the list
             * we display to the user
             */
            char *maxbdstr;
            char loopstring[20];
            int zz;
            char fullname_version[SIZE+NDNOTEMAXSTRINGSIZE];

            snprintf(loopstring,20,"%d",bcfgloop);
            driver_name=StringListPrint(bcfgloop,N_DRIVER_NAME); 
            if (driver_name == NULL) continue;
            fullname=StringListPrint(bcfgloop,N_NAME);
            bus=StringListPrint(bcfgloop,N_BUS);

            if ((zz=AtMAX_BDLimit(bcfgloop)) != 0) {  /* error or at limit */ 
               if (zz == -1) {
                  if (driver_name != NULL) free(driver_name);
                  if (fullname != NULL) free(fullname);
                  if (bus != NULL) free(bus);
                  return;  /* error -- error() already called */
               }
               notice("not displaying '%s'(%s); driver at MAX_BD limit",
                      fullname,driver_name);   /* ASSUMES: != NULL */
               if (driver_name != NULL) free(driver_name);
               if (fullname != NULL) free(fullname);
               if (bus != NULL) free(bus);
               continue;   /* at limit so continue */
            }

            /* is a complete set of {ira/dma/ioaddr/memaddr} available
             * for the user to choose from?  If not, then don't display
             * this driver, since it cannot be configured into the kernel
             * in the current implementation this can be an expensive(time) 
             * operation so we try and do it last.
             */
            zz=ISAsetavail(bcfgloop);
            if (zz == -1) {
               error(NOTBCFG,"ShowTopo: ISAsetavail returned failure");
               if (driver_name != NULL) free(driver_name);
               if (fullname != NULL) free(fullname);
               if (bus != NULL) free(bus);
               return;
            }
            if (zz == 0) {
               notice("not displaying '%s'(%s); no ISA parameter set available",
                      fullname,driver_name);   /* ASSUMES: != NULL */
               if (driver_name != NULL) free(driver_name);
               if (fullname != NULL) free(fullname);
               if (bus != NULL) free(bus);
               continue;  /* no ISA set available; continue */
            }


            /* NCFGELEMENT represents the element name that this _would_ have
             * if it were selected for use in a future idinstall command
             * the element name hasn't been absolutely assigned yet!
             */
            if (HasString(bcfgloop, N_TYPE, "MDI", 0) == 1) {
               /* NOTE: this is correct IFF another ndcfg isn't running and
                * isn't doing an idinstall command right at this instant, which
                * does /etc/conf/bin/idinstall, which will create a new netX,
                * so when this ndcfg goes to use it the netX will already
                * exist.  Note that a large time can elapse between the
                * showtopo command and the real idinstall command, so 
                * be aware of this!
                */
               snprintf(ncfgelement,12,"net%d",(err=getlowestnetX()));
               if (err == -1) {
                  error(NOTBCFG,"ShowTopo: error in getlowestnetX");
                  if (fullname != NULL) free(fullname);
                  if (driver_name != NULL) free(driver_name);
                  if (bus != NULL) free(bus);
                  return;
               }
            } else {
               /* probably ODI or DLPI, "standard devices" -- 0 based */
               boardnumber=ResmgrGetLowestBoardNumber(driver_name);
               if (boardnumber == -1) {
                  error(NOTBCFG,
                         "ShowTopo: ResmgrGetLowestBoardNumber returned -1");
                  if (fullname != NULL) free(fullname);
                  if (driver_name != NULL) free(driver_name);
                  if (bus != NULL) free(bus);
                  return;
               }
               snprintf(ncfgelement,12,"%s_%d",driver_name,boardnumber);
            }
            topologies=StringListPrint(bcfgloop,N_TOPOLOGY);
            strtoupper(topologies);
            strtoupper(bus);

            snprintf(fullname_version,
                     SIZE+NDNOTEMAXSTRINGSIZE,
                     "%s%s",
                     fullname,
                     bcfgfile[bcfgloop].driverversion);
            /* ISA boards won't have a KEY= so we leave that blank */
            AddToList(7,"",loopstring,driver_name,fullname_version,
                           ncfgelement,bus, topologies);

            numfound++;
            if (topologies != NULL) free(topologies);
            if (fullname != NULL) free(fullname);
            if (driver_name != NULL) free(driver_name);
            if (bus != NULL) free(bus);
         }
      }
   }
   if ((numfound == 0) && (numautodetected == 0)) {
      error(NOTBCFG,"No loaded drivers use topology %s",topoarg);
   }
   EndList();
}

static int nsets=0;
static int nprvs=0;
static struct pm_setdef *sdefs=(struct pm_setdef *)0;
extern int privname();

/* called by printauths() and havepriv() */
static  int
setnum(char *name)
{
   int     i;

   for(i = 0; i < nsets; ++i){
      if(!strcmp(name,sdefs[i].sd_name)){
         if(sdefs[i].sd_objtype == PS_PROC_OTYPE){
            return(i);
         }
         return(-1);     /*Not a process privilege set*/
      }
   }
   return(-1);
}


/* called by the "auths" command.  Similar to the havepriv() command */
int
printauths(void)
{
   int count,i;
   priv_t priv;
   priv_t  *buff=(priv_t *)0;
   char tmp[SIZE];
   int  j;

   nsets = secsys(ES_PRVSETCNT, 0);
   if(nsets < 0){
      error(NOTBCFG,"printauths: secsys ES_PRVSETCNT failed");
      return(-1);
   }
   sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t));
   if(!sdefs){
      error(NOTBCFG,"printauths: malloc failed");
      return(-1);
   }
   (void)secsys(ES_PRVSETS, (char *)sdefs);
   nprvs = 0;
   for(i = 0; i < nsets; ++i){
      if(sdefs[i].sd_objtype == PS_PROC_OTYPE){
         nprvs += sdefs[i].sd_setcnt;  /* NPRIVS(27) per set */
      }
   }

   buff=(priv_t *)malloc(nprvs * sizeof(priv_t));
   if (!buff) {
      free(sdefs);
      error(NOTBCFG,"printauths: malloc failed");
      return(-1);
   }

   count = procpriv(GETPRV,buff,nprvs);

   if (!count) {
      free(sdefs);
      free(buff);
      return(0);  /* nothing to print */
   }

   /* we don't call StartList() here because of formatting issues */

   Pstdout("max set:");
   for(i = 0; i < count; ++i) {
      for(j = 0; j < nprvs; ++j) {
         if((sdefs[setnum("max")].sd_mask | j) == buff[i]) {
            Pstdout("%s ",privname(tmp,j));
         }
      }
   }

   Pstdout("\nworking set:");
   for(i = 0; i < count; ++i) {
      for(j = 0; j < nprvs; ++j) {
         if((sdefs[setnum("work")].sd_mask | j) == buff[i]) {
            Pstdout("%s ",privname(tmp,j));
         }
      }
   }
   Pstdout("\n");

   free(sdefs);
   free(buff);

   return(0);
}

/* does this LWP have the priviledge specified in privneeded?
 * returns 1 if yes, 0 if no, -1 if error
 */
int 
havepriv(int privneeded)
{
   int count,i;
   priv_t priv,needed;
   priv_t  *buff=(priv_t *)0;

   nsets = secsys(ES_PRVSETCNT, 0);
   if(nsets < 0) {
      error(NOTBCFG,"havepriv: secsys ESPRVSETCNT failed");
      return(-1);
   }
   sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t));
   if(!sdefs){
      error(NOTBCFG,"havepriv: malloc for sdefs failed");
      return(-1);
   }
   (void)secsys(ES_PRVSETS, (char *)sdefs);
   nprvs = 0;
   for (i = 0; i < nsets; ++i) {
      if(sdefs[i].sd_objtype == PS_PROC_OTYPE) {
         nprvs += sdefs[i].sd_setcnt;  /* NPRIVS(27) per set */
      }
   }

   buff=(priv_t *)malloc(nprvs * sizeof(priv_t));
   if (!buff) {
      free(sdefs);
      error(NOTBCFG,"havepriv: malloc for buff failed");
      return(-1);
   }

   /* count has the total number of priviledges in all possible sets.  
    * At a minimum this means all priviledges in work and max.
    */
   count = procpriv(GETPRV,buff,nprvs);

   if (!count) {
      free(buff);
      free(sdefs);
      return(0);  /* no privileges, so return 0 */
   }
   if (nprvs < privneeded) {
      notice("havepriv: nprvs(%d) < privneeded(%d)",nprvs,privneeded);
      free(buff);
      free(sdefs);
      return(0);  /* we don't go up that high; return 0 */
   }

   /* needed represents the priviledge we need, shoved into a priv_t */
   needed=sdefs[setnum("work")].sd_mask | privneeded;

   /* for each possible priviledge we have in all of our sets, determine
    * if it's the one we're looking for
    * each priviledge in buff[] has the priviledge OR'ed in with the
    * bitmask, so the above OR is necessary for a direct comparision.
    */
   for(i = 0; i < count; ++i) {
      if(buff[i] == needed) {
         free(sdefs);
         free(buff);
         return(1);
      }
   }

   free(sdefs);
   free(buff);

   return(0);
}

/* ensure that we have sufficient priviledges to perform the tasks
 * necessary.  It is not sufficient to check for uid 0 because
 * - uid 0 may have priviledges removed (unlikely but possible)
 * - system owner may want to perform assorted operations (more likely)
 *   Quite likely to go through adminuser(1M)/tfadmin(1M) first.
 *
 * we must check for the following priviledges:
 * owner  - lets us create files in /etc/conf where we couldn't otherwise
 * dacread  - lets us exec binaries in /etc/conf/bin where we couldn't otherwise
 * dacwrite - lets us to a mkdir in /etc/conf where we couldn't otherwise
 * loadmod  - lets us load modules 
 * setflevel - for lvlfile(2) if we're using a MAC filesystem for idinstall
 *             (macupgrade would suffice in _most_ situations)
 * filesys  - to allow idmknod to create entries in /dev
 * see intro(2) if you're not familiar with these priviledges.
 *
 * In some cases it may not be necessary to have the priviledge
 * (i.e. normal filesystem semantics of owner/group/other suffice) but
 * this won't work for 
 * returns -1 if we don't have sufficient priviledges
 * returns  0 if we do and all is well.
 */
#define CHECK(PRIV,OUTPUTSTRING) if (havepriv(PRIV) == 0) { \
                      error(NOTBCFG, \
                       "Sorry, this command requires the '%s' priviledge", \
                                    OUTPUTSTRING); \
                      return(-1); \
                   }

int
ensureprivs(void)
{

   CHECK(P_OWNER,"owner");
   CHECK(P_DACREAD,"dacread");
   CHECK(P_DACWRITE,"dacwrite");
   CHECK(P_LOADMOD,"loadmod");
   CHECK(P_SETFLEVEL,"setflevel");

   return(0);
}

#undef CHECK

/* returns 0 for success
 * returns -1 for error
 * this routine is called to handle the
 * getisaparams   <bcfgindex>
 * and
 * getallisaparams <element>
 * commands
 */
int
getisaparams(char *bcfgindexorelement, int getallisaparams)
{

   int bcfgindex;
   char *strend;
   int status;
   char *irq,*port,*mem,*dma;
   /* the size for irqbuffer, dmabuffer, portbuffer, membuffer must be at 
    * least as big as VB_SIZE for the call to showISAcurrent
    */
   char irqbuffer[SIZE * 4];
   char dmabuffer[SIZE * 4];
   char portbuffer[LISTBUFFERSIZE];
   char membuffer[LISTBUFFERSIZE];
   char irqalready[50];
   char dmaalready[50];
   char portalready[50];
   char memalready[50];
   u_int irqbuffersize,dmabuffersize,portbuffersize,membuffersize;
   int numint, numport, nummem, numdma, loop;
   rm_key_t skipkey=RM_KEY;

   irqbuffer[0]='\0';
   dmabuffer[0]='\0';
   portbuffer[0]='\0';
   membuffer[0]='\0';

   /* when in getallisaparams mode, show the current settings first in the
    * list.  but don't display them again.  this keeps track of what
    * we have already added to our list so we don't call AddToList with
    * it again
    */
   irqalready[0]='\0';
   dmaalready[0]='\0';
   portalready[0]='\0';
   memalready[0]='\0';

   if ((cm_bustypes & CM_BUS_ISA) == 0) {
      /* MCA machines fall into this category */
      error(NOTBCFG,"this machines doesn't even have an ISA bus!");
      return(-1);
   }

   if (getallisaparams == 0) {
      /* avoid atoi */
      if (((bcfgindex=strtoul(bcfgindexorelement,&strend,10))==0) &&
           (strend == bcfgindexorelement)) {
         error(NOTBCFG,"invalid number %s",bcfgindexorelement);
         return(-1);
      }
      if (bcfgindex >= bcfgfileindex) {
         error(NOTBCFG,"index %d >= maximum(%d)",bcfgindex,bcfgfileindex);
         return(-1);
      }
      if (HasString(bcfgindex, N_BUS, "ISA", 0) != 1) {
         error(NOTBCFG,"bcfgindex %d isn't for ISA bus",bcfgindex);
         return(-1);
      }

      /* check and see if *any* set is possible for this bcfg file
       * a side effect is that this routine effectively goes through the resmgr
       * twice - once by calling ISAsetavail and once below.
       */
      status=ISAsetavail(bcfgindex);
      if (status == -1) {
         error(NOTBCFG,"getisaparams: failure in ISAsetavail");
         return(-1);
      }
      if (status == 0) {
         /* If you went through the showalltopologies/showtopo route,
          * you would never have seen this bcfg file show up in the
          * first place in any of those commands' output.  error out
          * However, if we're trying to modify existing parameters
          * with idmodify we will call elementtoindex followed by
          * getisaparams, and there might not be any parameters left
          * So if we're in tcl mode then fake it by returning nothing
          * but if not tcl mode then error out.
          */
         if (tclmode) {
            StartList(8,"IRQ",80,"DMAC",80,"IOADDR",80,"MEMADDR",80);
            AddToList(4,"","","","");
            EndList();
            /* nothing to free() up before returning */
            return(0);  /* "success" */
         } else {
            error(NOTBCFG,"No unused ISA parameters available on system for "
                          "this bcfg file");
            /* nothing to free() up before returning */
            return(-1);
         }
      }
      /* skipkey stays at RM_KEY */
   } else {
      /* getallisaparams mode - first argument to routine is element */
      skipkey=resshowkey(0,bcfgindexorelement,NULL,0);/*OpenResmgr/CloseResmgr*/
      bcfgindex = elementtoindex(0,bcfgindexorelement);

      if (bcfgindex == -1) {
         error(NOTBCFG,"getallisaparams: couldn't find bcfg index for "
                       "element %s",bcfgindexorelement);
         return(-1);
      }
      if (HasString(bcfgindex, N_BUS, "ISA", 0) != 1) {
         error(NOTBCFG,"getallisaparams: bcfgindex %d isn't for "
                       "ISA bus",bcfgindex);
         return(-1);
      }

      /* we can't try the backup key as we must exclude this key from future
       * calls to IsParam, and backup keys are already ignored by IsParam,
       * so for our purposes here trying the backup key doesn't do anything
       */
      if (skipkey == RM_KEY) {   /* failure in resshowkey() */
         /* a future call to idmodify *might* fail, since we couldn't
          * find the NETCFG_ELEMENT parameter for this element in the resmgr,
          * but we might find its backup key.  This means that we can't
          * know the current ISA board settings either, so skip the
          * call to showISAcurrent.
          * remember this is for ISA boards whose keys shouldn't be 
          * disappearing in the resmgr at whim, unlike PCI/EISA/MCA keys
          */
         notice("getallisaparams: element %s not found in resmgr",
                bcfgindexorelement);
      } else {

         status=showISAcurrent(bcfgindexorelement, 0, 
                               irqbuffer, portbuffer, membuffer, dmabuffer);
         if (status != 0) {
            /* error already called by showISAcurrent */
            error(NOTBCFG,"getallisaparams: failure in showISAcurrent");
            return(-1);
         }

         /* note that irqbuffer, portbuffer, membuffer, dmabuffer can be '-'
          * if the parameter wasn't set in the resmgr
          */
         if (irqbuffer[0] == '-') irqbuffer[0]='\0';
         if (portbuffer[0] == '-') portbuffer[0]='\0';
         if (membuffer[0] == '-') membuffer[0]='\0';
         if (dmabuffer[0] == '-') dmabuffer[0]='\0';

         /* we must call strtolower because later calls to strcmp that 
          * involve irqalready, portalready, memalready, dmaalready compare
          * with .bcfg file values, which, being numerics, were converted
          * to lower case in EnsureNumerics
          * we call strtoupper on irqbuffer, portbuffer, membuffer, dmabuffer
          * at the end of this routine so it doesn't matter what case it's
          * in at this point in time.
          */
         if (strlen(irqbuffer)) {
            strtolower(irqbuffer);
            strncpy(irqalready, irqbuffer, 50);
            strcat(irqbuffer, " ");
         }
         if (strlen(portbuffer)) {
            strtolower(portbuffer);
            strncpy(portalready, portbuffer, 50);
            strcat(portbuffer, " ");
         }
         if (strlen(membuffer)) {
            strtolower(membuffer);
            strncpy(memalready, membuffer, 50);
            strcat(membuffer, " ");
         }
         if (strlen(dmabuffer)) {
            strtolower(dmabuffer);
            strncpy(dmaalready, dmabuffer, 50);
            strcat(dmabuffer, " ");
         }
      }
   }

   /* to get here we know that
    * getisaparams:      we know there is at least one possible set that works
    * getallisaparams:   don't know anything yet.  We have called AddToList
    *                    with current values
    * find all other possibilities.  there may be >=0(getallisaparams) or 
    *                                              >1(getisaparams)
    * for each found, add it to our buffer
    */

   numint=StringListNumWords(bcfgindex, N_INT);
   if (numint > 0) {
      for (loop=1; loop <= numint; loop++) {
         irq=StringListWordX(bcfgindex,N_INT,loop);
         if (irq == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"getisaparams: StringListWordX returned NULL");
            return(-1);
         }

         /* if we've already added this irq to the list don't try again */
         if (strcmp(irq, irqalready) == 0) {
            free(irq);  /* malloced in StringListWordX */
            continue;
         }

         /* since this is only for ISA cards we don't have to worry about
          * ITYPE sharing here -- existence of driver in resmgr is sufficient
          */
         status=ISPARAMAVAILSKIPKEY(N_INT, irq, skipkey);
         if (status == 1) {
            char tmp[50];
            
            snprintf(tmp,50,"%s ",irq);
            strcat(irqbuffer, tmp);
         }
         free(irq);  /* malloced in StringListWordX */
         if (status == -1) {
            error(NOTBCFG,"getisaparams: error in IsParam for INT");
            return(-1);
         }
      }
   }

   numport=StringListNumWords(bcfgindex, N_PORT);
   if (numport > 0) {
      for (loop=1; loop <= numport; loop++) {
         port=StringListWordX(bcfgindex,N_PORT,loop);
         if (port == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"getisaparams: StringListWordX returned NULL");
            return(-1);
         }

         /* if we've already added this ioaddr to the list don't try again */
         if (strcmp(port, portalready) == 0) {
            free(port);  /* malloced in StringListWordX */
            continue;
         }

         status=ISPARAMAVAILSKIPKEY(N_PORT, port, skipkey);
         if (status == 1) {
            char tmp[50];
      /* remember, the idinstall command doesn't want 0x for ioaddr! 
       * also the showisacurrent command doesn't add 0x either
       */
#ifdef NDCFG_DO_HEX
            char *dash;
            
            dash=strstr(port,"-");   /* will have one to get here */
            *dash='\0';
            snprintf(tmp,50,"0x%s-0x%s ",port,dash+1);
#else
            snprintf(tmp,50,"%s ",port);
#endif
            strcat(portbuffer, tmp);
         }
         free(port);  /* malloced in StringListWordX */
         if (status == -1) {
            error(NOTBCFG,"getisaparams: error in IsParam for PORT");
            return(-1);
         }
      }
   }

   nummem=StringListNumWords(bcfgindex, N_MEM);
   if (nummem > 0) {
      for (loop=1; loop <= nummem; loop++) {
         mem=StringListWordX(bcfgindex,N_MEM,loop);
         if (mem == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"getisaparams: StringListWordX returned NULL");
            return(-1);
         }

         /* if we've already added this memaddr to the list don't try again */
         if (strcmp(mem, memalready) == 0) {
            free(mem);  /* malloced in StringListWordX */
            continue;
         }

         status=ISPARAMAVAILSKIPKEY(N_MEM, mem, skipkey);
         if (status == 1) {
            char tmp[50];
      /* remember, the idinstall command doesn't want 0x for memaddr!
       * also the showisacurrent command doesn't add 0x either
       */
#ifdef NDCFG_DO_HEX
            char *dash;

            dash=strstr(mem,"-");  /* will have one to get here */
            *dash='\0';
            snprintf(tmp,50,"0x%s-0x%s ",mem,dash+1);
#else
            snprintf(tmp,50,"%s ",mem);
#endif
            strcat(membuffer, tmp);
         }
         free(mem);  /* malloced in StringListWordX */
         if (status == -1) {
            error(NOTBCFG,"getisaparams: error in IsParam for MEM");
            return(-1);
         }
      }
   }

   numdma=StringListNumWords(bcfgindex, N_DMA);
   if (numdma > 0) {
      for (loop=1; loop <= numdma; loop++) {
         dma=StringListWordX(bcfgindex,N_DMA,loop);
         if (dma == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"getisaparams: StringListWordX returned NULL");
            return(-1);
         }

         /* if we've already added this dma to the list don't try again */
         if (strcmp(dma, dmaalready) == 0) {
            free(dma);  /* malloced in StringListWordX */
            continue;
         }

         status=ISPARAMAVAILSKIPKEY(N_DMA, dma, skipkey);
         if (status == 1) {
            char tmp[50];
            
            snprintf(tmp,50,"%s ",dma);
            strcat(dmabuffer, tmp);
         }
         free(dma);  /* malloced in StringListWordX */
         if (status == -1) {
            error(NOTBCFG,"getisaparams: error in IsParam for DMA");
            return(-1);
         }
      }
   }

   strtoupper(irqbuffer);
   strtoupper(dmabuffer);
   strtoupper(portbuffer);
   strtoupper(membuffer);

   irqbuffersize=strlen(irqbuffer);
   dmabuffersize=strlen(dmabuffer);
   portbuffersize=strlen(portbuffer);
   membuffersize=strlen(membuffer);

   if (irqbuffersize) irqbuffer[irqbuffersize-1]='\0';
   if (dmabuffersize) dmabuffer[dmabuffersize-1]='\0';
   if (portbuffersize) portbuffer[portbuffersize-1]='\0';
   if (membuffersize) membuffer[membuffersize-1]='\0';

   StartList(8,"IRQ",80,"DMAC",80,"IOADDR",80,"MEMADDR",80);

   if (irqbuffersize + dmabuffersize + portbuffersize + membuffersize > 0) {
      AddToList(4,irqbuffer,dmabuffer,portbuffer,membuffer);
   } else {
      if (getallisaparams == 0) {
         /* either a) none of these parameters were defined in the bcfg
          *           file (which EnsureAllAssigned prevents) or
          *        b) no parameters in set are available.  Our earlier
          *           call to ISAsetavail should have told us otherwise
          * so error out if this happens
          * In short, we shouldn't ever get here!
          */
         error(NOTBCFG,"No ISA parameters available for bcfgindex %d",
               bcfgindex);
         return(-1);
      } else {
         /* getallisaparams mode where it's perfectly acceptable to not have
          * anything available.  Can use either " " or NULL in AddToList below
          */
         AddToList(4," "," "," "," ");
      }
   }
   EndList();
   return(0);
}

int
showcustomnum(int dolist, char *bcfgindexstr)
{
   u_int bcfgindex,customnum,loop,numfound;
   char *strend;
   char *customnumstr;
   char custom[20],numfoundasstr[10];

   /* avoid atoi */
   if (((bcfgindex=strtoul(bcfgindexstr,&strend,10))==0) &&
        (strend == bcfgindexstr)) {
      error(NOTBCFG,"invalid number %s",bcfgindexstr);
      return(-1);
   }
   if (bcfgindex >= bcfgfileindex) {
      error(NOTBCFG,"index %d >= maximum(%d)",bcfgindex,bcfgfileindex);
      return(-1);
   }
   /* EnsureNumerics has ensured that it is a valid number.  Note 
    * EnsureNumerics lets number ranges through so we must check
    * for this here
    */
   numfound=0;
   customnumstr=StringListPrint(bcfgindex,N_CUSTOM_NUM);
   if (customnumstr != NULL) {
      char *dash;

      /* see if we have a dash in number (EnsureNumerics doesn't do this) */
      if ((dash=strstr(customnumstr,"-")) != NULL) *dash='\0';  /* whack! */
      customnum=strtoul(customnumstr,NULL,10);  /* will succeed to get here */
      if ((customnum < 0) || (customnum > MAX_CUSTOM_NUM)) {
         free(customnumstr);
         error(NOTBCFG,"CUSTOM_NUM must be between 0 and %d",MAX_CUSTOM_NUM);
         return(-1);
      }
      /* now check that all of the CUSTOM[x] strings exist based on the
       * desired number wanted, specified by CUSTOM_NUM
       * note user may have all 9 CUSTOM[x] entries set but may indicate to
       * use the amount actually specified by CUSTOM_NUM, starting at 1!
       */
      if (customnum) {   /* CUSTOM_NUM=0 in bcfg is valid */
         for (loop=1;loop<=customnum;loop++) {
            char *tmp;
            int offset;

            snprintf(custom,20,"CUSTOM[%d]",loop);
            offset=N2O(custom);  /* can call error */
     
            if (offset == -1) continue;  /* errr, something went wrong here */
            tmp=StringListPrint(bcfgindex,offset); /* does CUSTOM[x] exist? */
            if (tmp != NULL) {
               numfound++;
               free(tmp);
            } else {
               /* we stop at the first CUSTOM[x] not defined -- that's how many
                * "usable" ones there are, starting at 1.  so if 
                * CUSTOM_NUM=3, CUSTOM[1] and CUSTOM[3] are defined but
                * CUSTOM[2] is not, we say that only the first is usable
                * and return 1 from this routine.  Why?
                * new netcfg won't know to skip any "holes" where
                * CUSTOM[x] not defined but CUSTOM[x+1] *is* defined.
                */
               notice(
                 "CUSTOM[%d] not defined but CUSTOM_NUM is %d; stopping search",
                 loop,customnum);  /* you don't see notices in tcl mode */
               break;
            }
         }
      }
      /* note that numfound may be anything from 0-customnum here */
      free(customnumstr);
   }
   if (dolist) {
      StartList(2,"CUSTOM_NUM",10);
      snprintf(numfoundasstr,10,"%d",numfound);
      AddToList(1,numfoundasstr);
      EndList();
   } else {
      ClearError();  /* in case we called error above an in tcl mode */
   }
   return(numfound);
}

void 
showcustom(char *bcfgindexstr,char *customnumstr)
{

   u_int bcfgindex,actualnum,customnum,loop,offset,version;
   char *strend;
   char custom[20];
   stringlist_t *slp;
   char resmgrparam[SIZE];
   char resvalues[SIZE * 8];  /* 8K */
   char choices[SIZE * 8];    /* 8K */
   char choicetitle[SIZE];
   char helppath[SIZE];
   char helptxt[SIZE];
   char basadv[SIZE];
   char topologies[SIZE];
   char bdg[SIZE];

   resmgrparam[0]='\0';
   resvalues[0]='\0';
   choices[0]='\0';
   choicetitle[0]='\0';
   helppath[0]='\0';
   helptxt[0]='\0';
   basadv[0]='\0';
   topologies[0]='\0';
   bdg[0]='\0';

   /* avoid atoi */
   if (((bcfgindex=strtoul(bcfgindexstr,&strend,10))==0) &&
        (strend == bcfgindexstr)) {
      error(NOTBCFG,"invalid number %s",bcfgindexstr);
      return;
   }
   if (bcfgindex >= bcfgfileindex) {
      error(NOTBCFG,"index %d >= maximum(%d)",bcfgindex,bcfgfileindex);
      return;
   }
   actualnum=showcustomnum(0,bcfgindexstr);
   if (actualnum == -1) {
      error(NOTBCFG,"problem getting CUSTOM_NUM for bcfgindex %s",bcfgindexstr);
      return;
   }
   if (((customnum=strtoul(customnumstr,&strend,10))==0) &&
        (strend == customnumstr)) {
      error(NOTBCFG,"invalid custom number %s",customnumstr);
      return;
   }
   if (customnum == 0) {
      error(NOTBCFG,"CUSTOM[x] parameters start with 1");
      return;
   }
   if (actualnum == 0) {
      error(NOTBCFG,"no CUSTOM[x] parameters set for bcfgindex %d",bcfgindex);
      return;
   }
   if (customnum > actualnum) {
      error(NOTBCFG,"highest for bcfgindex %d=CUSTOM[%d]",bcfgindex,actualnum);
      return;
   }

   /* we know the desired CUSTOM[x] value exists to get here */
   StartList(18,"RESMGRPARAM",10,"RESVALUES",30,"CHOICES",30,"CHOICETITLE",30,
                 "HELPPATH",30,"HELPTXT",30,"BASADV",30,"TOPOLOGIES",30,
                 "BDG",30);  /* BOARD, DRIVER, or GLOBAL */
   snprintf(custom,20,"CUSTOM[%d]",customnum);
   offset=N2O(custom);
   if (offset == -1) {
      error(NOTBCFG,"N2O failed for %s",custom);
      return;
   }
   version=bcfgfile[bcfgindex].version;
   slp=&bcfgfile[bcfgindex].variable[offset].strprimitive.stringlist;
   if (slp->type != STRINGLIST) {
      error(NOTBCFG,"primitive type for bcfgindex %d offset %d is %d",
            bcfgindex,offset,slp->type);
      return; /* while not UNDEFINED, could be NUMRANGE, etc. */
   }

   if (*slp->string == '\n') {
      slp=slp->next; /* filler since first one is always a newline */
   }
   while (slp != NULL && *slp->string != '\n')
   {
      strncat(resmgrparam,slp->string,SIZE);
      if ((slp->next != NULL) && (*slp->next->string != '\n')) 
         strcat(resmgrparam," ");
      slp=slp->next;
   }

   if (slp == NULL) {
      error(NOTBCFG,"CUSTOM[%d] ends at line 1",customnum);
      return;
   }

   if (strstr(resmgrparam," ") != NULL) {
      error(NOTBCFG,"can't set multiple resmgr parameters in CUSTOM[x] line 1");
   }
   slp=slp->next;   /* advance past newline */


   while (slp != NULL && *slp->string != '\n')
   {
      strncat(resvalues,slp->string,SIZE * 8);
      if ((slp->next != NULL) && (*slp->next->string != '\n')) 
         strcat(resvalues," ");
      slp=slp->next;
   }
   if (slp == NULL) {
      error(NOTBCFG,"CUSTOM[%d] ends at line 2",customnum);
      return;
   }
   slp=slp->next;   /* advance past newline */


   while (slp != NULL && *slp->string != '\n')
   {
      strncat(choices,slp->string,SIZE * 8);
      if ((slp->next != NULL) && (*slp->next->string != '\n')) 
         strcat(choices," ");
      slp=slp->next;
   }
   if (slp == NULL) {
      error(NOTBCFG,"CUSTOM[%d] ends at line 3",customnum);
      return;
   }
   slp=slp->next;   /* advance past newline */


   while (slp != NULL && *slp->string != '\n')
   {
      strncat(choicetitle,slp->string,SIZE);
      if ((slp->next != NULL) && (*slp->next->string != '\n')) 
         strcat(choicetitle," ");
      slp=slp->next;
   }
   if (slp == NULL) {
      error(NOTBCFG,"CUSTOM[%d] ends at line 4",customnum);
      return;
   }
   slp=slp->next;


   while (slp != NULL && *slp->string != '\n')
   {
      strncat(helppath,slp->string,SIZE);
      if ((slp->next != NULL) && (*slp->next->string != '\n')) 
         strcat(helppath," ");
      slp=slp->next;
   }
   if (slp == NULL) {
      error(NOTBCFG,"CUSTOM[%d] ends at line 5",customnum);
      return;
   }
   slp=slp->next;


   while (slp != NULL && *slp->string != '\n')
   {
      strncat(helptxt,slp->string,SIZE);
      if ((slp->next != NULL) && (*slp->next->string != '\n')) 
         strcat(helptxt," ");
      slp=slp->next;
   }
   if (slp == NULL) {
      error(NOTBCFG,"CUSTOM[%d] ends at line 6",customnum);
      return;
   }
   slp=slp->next;


   if (version == 0) {
      char *tmp;

      strncpy(basadv,"BASIC",SIZE);
      tmp=StringListPrint(bcfgindex,N_TOPOLOGY);
      if (tmp) {
         strncpy(topologies,tmp,SIZE);
         free(tmp);
      } else {
         strncpy(topologies,"ALL_TOPOLOGIES",SIZE);
      }
      strcpy(bdg,"BOARD");
   } else if (version == 1) {
      while (slp != NULL && *slp->string != '\n')
      {
         strncat(basadv,slp->string,SIZE);
         if ((slp->next != NULL) && (*slp->next->string != '\n')) 
            strcat(basadv," ");
         slp=slp->next;
      }
      if (slp == NULL) {
         error(NOTBCFG,"CUSTOM[%d] ends at line 7",customnum);
         return;
      }
      slp=slp->next;



      while (slp != NULL && *slp->string != '\n')
      {
         strncat(topologies,slp->string,SIZE);
         if ((slp->next != NULL) && (*slp->next->string != '\n')) 
            strcat(topologies," ");
         slp=slp->next;
      }
      if (slp == NULL) {
         error(NOTBCFG,"CUSTOM[%d] ends at line 8",customnum);
         return;
      }
      slp=slp->next;



      while (slp != NULL && *slp->string != '\n')
      {
         strncat(bdg,slp->string,SIZE);
         if ((slp->next != NULL) && (*slp->next->string != '\n')) 
            strcat(bdg," ");
         slp=slp->next;
      }
      if (slp == NULL) {
         error(NOTBCFG,"CUSTOM[%d] ends at line 9",customnum);
         return;
      }
      slp=slp->next;


      /* we don't care what lines are past here */
   } else {
      fatal("unknown version %d",version); /* ensurevalid takes care of this */
      /* NOTREACHED */
   }

   AddToList(9,resmgrparam,resvalues,choices,choicetitle,helppath,
                 helptxt,basadv,topologies,bdg);
   EndList();
   return;
}

/* we won't have a '\n' in this stringlist to worry about so we can just
 * look for a space as the delimiter.
 * you cannot use this routine from any stringlist generated from a bcfg
 * file since they will have newlines in them.  Further, bcfg file strings
 * have imbedded spaces in them from when things are double quoted as in
 * FOO="bar baz inc"
 * The long and the short is that you can only call this routine on stringlists
 * generated from cmdparser.y (basically only for the idinstall command)
 */
int
CountStringListArgs(stringlist_t *slp)
{
   int n=0;

   while (slp != NULL) {
      n++;
      slp=slp->next;
   }
   return(n);
}

/* as before, you can only call this routine on stringlists generated
 * from cmdparser.y as we assume spaces and newlines won't exist in 
 * slp->string
 * returns NULL if not found else the start of the text.
 * NOTE:  this routine also deals with free text parameters specified as
 *        arguments on the command line.  look at cmdlex.l for 
 *        WHITESPACE and '{' for clues as to how this trickery is done
 *        as it's not obvious...
 *        Basically if the user uses the idinstall or idmodify commands
 *        and supplies FOO=asdf  BAR={this is a test}
 *        as arguments we want a FindStringListText("FOO=") to return
 *        "asdf" and FindStringListText("BAR=") to return "this is a test".
 */
char *
FindStringListText(stringlist_t *slp, char *param)
{
   int len=strlen(param);

   if (param == NULL) {
      error(NOTBCFG,"FindStringListText: NULL parameter!");
      return(NULL);
   }

   if (strlen(param) == 0) {
      error(NOTBCFG,"FindStringListText: strlen parameter is 0!");
      return(NULL);
   }

   while (slp != NULL) {
      if (!strncmp(slp->string,param,len)) {
         /* ok, we have a match.  but is it FOO=bar or FOO={bar baz}  ?  */
         if (slp->string[len] == '{') {
            /* the text string we want is in the next field */
            if (slp->next == NULL) {
               notice("FindStringListText: no next");
               return(NULL);
            }
            return(slp->next->string);
         } else {
            return(slp->string + len);
         }
      }
      slp=slp->next;
   }
   return(NULL);
}

int
EnsureNotDefined(stringlist_t *slp, char *param)
{
   if (FindStringListText(slp, param) != NULL) {
      error(NOTBCFG,
            "ISA specific parameter %s supplied as arg but bcfg bus != ISA");
      return(1); 
   }
   return(0);
}

/* rather than system() out to do a copy, we roll our own using mmap
 * for speed and to prevent forking. 
 */
int
copyfile(int errifnotthere, char *srcdir, char *srcfile,
                            char *destdir,char *destfile)
{
   struct stat sb;
   char tmp[SIZE];
   char buf[4096];
   caddr_t addr;
   int srcfd, dstfd, amount;

   if (chdir(srcdir) == -1) {
      error(NOTBCFG,"copyfile: couldn't chdir to src dir %s",srcdir);
      return(-1);
   }
   if (stat(srcfile,&sb) == -1) {
      if (errifnotthere == 1) {
         error(NOTBCFG,"copyfile: %s doesn't exist",srcfile);
         return(-1); /* failure */
      }
      return(0);   /* "success" */
   }
   if (!S_ISREG(sb.st_mode)) {
      error(NOTBCFG,"copyfile: %s/%s not regular file",srcdir,srcfile);
      return(-1);
   }
   if (!sb.st_size) {
      /* no work to do.  don't touch file in destdir */
      return(0);   /* "success" */
   }

   if ((srcfd=open(srcfile,O_RDONLY)) == -1) {
      error(NOTBCFG,"copyfile: couldn't open %s/%s for reading; errno=%d",
            srcdir,srcfile,errno);
      return(-1);
   }
   if (chdir(destdir) == -1) {
      error(NOTBCFG,"copyfile: couldn't chdir to dst dir %s",destdir);
      return(-1);
   }
      /* we don't add O_TRUNC -- the file we're trying to create shouldn't
       * exist!  if it does something weird is going on like maybe 
       * trying to make security hole through symlinks?
       */
   if ((dstfd=open(destfile,O_WRONLY|O_CREAT,0600)) == -1) {
      error(NOTBCFG,"copyfile: couldn't open %s/%s for writing; errno=%d",
            destdir,destfile,errno);
      close(srcfd);  /* don't lose this fd! */
      return(-1);
   }
   addr=(caddr_t)mmap((caddr_t) 0, sb.st_size,
                        PROT_READ, MAP_SHARED, srcfd, (off_t) 0);
   if (addr == (caddr_t) -1) {
      error(NOTBCFG,"copyfile: couldn't mmap %s/%s; errno=%d",
            srcdir,srcfile,errno);
      close(srcfd);
      close(dstfd);
      return(-1);
   }
   if ((amount=write(dstfd, addr, sb.st_size)) != sb.st_size) {
      error(NOTBCFG,
            "copyfile: only copied %d of %d bytes from %s/%s to %s/%s errno=%d",
            amount,sb.st_size,srcdir,srcfile,destdir,destfile,errno);
      close(srcfd);
      close(dstfd);
      return(-1);
   }
   munmap(addr, sb.st_size);
   close(dstfd);
   close(srcfd);
   return(0);   /* success */
}

/* returns 0 if no, 1 if yes
 */
int
RealDLPIexists(void)
{
   char tmp[SIZE];
   int status;

   snprintf(tmp,SIZE,"/etc/conf/bin/idcheck -y dlpi");
   /* because we care about the exit status we use runcommand not docmd */
   status=runcommand(0,tmp);
   if ((status == 100) || (status == -1)) {
      notice("RealDLPIexists: idcheck -y dlpi encountered an ERROR!");
   }
   if (status != 1) {
      return(0);
   } else {
      return(1);
   }
}

/* turn on all lines in System file for driver.  must be in
 * correct directory.
 * since ReadSystemFile() and all of ndcfg play with the resmgr
 * directly and don't let idinstall run idresadd, there's no 
 * point in reading in all of the lines in System and turn them
 * all on -- the the first line will suffice.   This won't affect
 * "correct" drivers that only have 1 line in their System file
 * but only affects ill-behaved drivers that have multiple lines
 * in their System file.  Of course, we must copy all $ lines to
 * our new System file.  This isn't a problem since the '$' lines
 * must preceed the actual 12-field line.
 * This effectively means that we "fix" drivers in the location
 * where the DSP lives(isaautodetect), or in our temp directory
 * (idinstall), prior to the idinstall.  This means that .sdevice.d
 * will only have one line (after the idconfupdate) too.
 */
int
TurnOnSdevs(void)
{
   FILE *old;
   FILE *new;
   char line[SIZE];
   char fmt[78];
   struct System S;
   struct System *System;
   struct stat sb;
   int goodfile=0,n;
   char dummy[2];

   System=&S;

   if (stat("System",&sb) == -1) {
      error(NOTBCFG,"TurnOnSdevs: System file doesn't exist");
      return(-1);
   }

   if (!S_ISREG(sb.st_mode)) {
      error(NOTBCFG,"TurnOnSdevs: System file not a regular file");
      return(-1);
   }

   if ((old=fopen("System","r")) == NULL) {
      error(NOTBCFG,"TurnOnSdevs: Couldn't open System file");
      return(-1);
   }
   unlink("System+");
   if ((new=fopen("System+","w")) == NULL) {
      error(NOTBCFG,"TurnOnSdevs: Couldn't create new System file");
      fclose(old);
      return(-1);
   }

   snprintf(fmt, 78, "%%%ds", NAMESZ - 1);
   strcat(fmt, " %c %ld %hd %hd %hd %lx %lx %lx %lx %hd %d %1s");
   while ((goodfile == 0) && (fgets(line, SIZE, old) != NULL)) {
      if ((*line == '*') || 
          (*line == '#') ||
          (*line == '$') ||
          (*line == '\n')) {
         fputs(line,new);
         continue;
      }
      if (isalpha(line[0]) && strlen(line) < 80) {
         n = sscanf(line, fmt,
               System->name,
               &System->conf,
               &System->unit,
               &System->ipl,
               &System->itype,
               &System->vector,
               &System->sioa,
               &System->eioa,
               &System->scma,
               &System->ecma,
               &System->dmachan,
               &System->bind_cpu,
               dummy);
         if (n == 11) {
            System->bind_cpu = -1;   /* not set */
         } else
         if (n != 12) {
            /* wrong number of fields; don't copy it to new file */
            continue;
         }
         goodfile=1;
         System->conf='Y';   /* turn this puppy on */
/*module-name configure unit ipl itype ivec sioa eioa scma ecma dmachan [cpu]*/
         fprintf(new,
                 "%s\t%c\t%ld\t%hd\t%hd\t%hd\t%lx\t%lx\t%lx\t%lx\t%hd\t%d\n",
                 System->name,System->conf,System->unit,System->ipl,
                 System->itype,System->vector,System->sioa,System->eioa,
                 System->scma,System->ecma,System->dmachan,
                 System->bind_cpu);
      }
   }
   fclose(new);
   fclose(old);
   if (goodfile == 1) {
      rename("System+","System");
   } else {
      /* no valid lines found in original System file; delete System+.  There
       * will likely be a problem later on with idmkunix but we'll punt.
       */
      unlink("System+");
   }
   return(0);   
}


/* does the actual idinstall of a driver from destdir
 * returns -1 for failure
 */
int
idinstalldriver(char *destdir, char *modname)
{
   char cmdbuf[SIZE];
   struct stat sb;
   int freshadd=0;
   int status;

   /* I suppose we could use idcheck here but this saves a fork/exec/wait */
   snprintf(cmdbuf,SIZE,"/etc/conf/mdevice.d/%s",modname);
   if (stat(cmdbuf,&sb) == -1) {
      /* (hopefully) file doesn't exist */
      freshadd=1;
   }

   if (chdir(destdir) == -1) {
      error(NOTBCFG,"idinstalldriver: couldn't chdir to %s",destdir);
      return(-1);
   }
   if (TurnOnSdevs() == -1) {
      error(NOTBCFG,"idinstalldriver: couldn't turn on System in %s",destdir);
      return(-1);
   }
   snprintf(cmdbuf,SIZE,"/etc/conf/bin/idinstall -P nics %s -N -e -k %s", 
           freshadd ? "-a" : "-u", modname);
   status=docmd(0,cmdbuf);
   return(status);
}

/* fill in System structure information to later put into the resmgr
 * CM_* fields
 * As it turns out, as long as we run idconfupdate and MODNAME is set
 * and the driver exists in the link kit we don't have to
 * change the 'N' to a 'Y' in the System file ether.  a nice bonus.
 * NOTE:  we only read the first valid line in the System file!
 *        Since we only use ipl, itype, unit, and bind_cpu fields, each
 *        of which (except for bind_cpu) isn't expected to change from
 *        one instance of the driver to the next, only reading the first
 *        line isn't so bad.
 */
int
ReadSystemFile(struct System *System, char *filename)
{
   FILE *fp;
   int found=0;
   char line[SIZE];
   char fmt[78]; 
   char dummy[2];
   int n;

   if ((fp=fopen(filename,"r")) == NULL) {
      error(NOTBCFG,"Couldn't open System file '%s'",filename);
      return(-1);
   }
   snprintf(fmt, 78, "%%%ds", NAMESZ - 1);
   strcat(fmt, " %c %ld %hd %hd %hd %lx %lx %lx %lx %hd %d %1s");
   while ((found == 0) && fgets(line, SIZE, fp) != NULL) {
      if (isalpha(line[0]) && strlen(line) < 80) 
      /* $interface lines don't have tabs & we only want certain lines */
      n = sscanf(line, fmt,
            System->name,
            &System->conf,
            &System->unit,     /* UNIT= in bcfg file takes priority */
            &System->ipl,
            &System->itype,
            &System->vector,
            &System->sioa,
            &System->eioa,
            &System->scma,
            &System->ecma,
            &System->dmachan,
            &System->bind_cpu,
            dummy);
      if (n == 11) {
         System->bind_cpu = -1;   /* not set */
         found = 1;
      } else
      if (n == 12) {
         found = 1;
      }
   }
   fclose(fp);
   return(!found);   
}


/* returns 0 for success, non-zero for failure 
 * this routine does the following DSP functions:
 * - copy ALL DSP files (but not .html or other bcfg files) to a new location
 *   we then issue idinstall from new directory and watch idinstall slurp
 *   up the new files.
 * - Zero/blank out System fields so idinstall calling idresadd
 *   won't add a new entry to the resmgr.  NOT NECESSARY WITH IDINSTALL -N
 * - If Drvmap present mangle it to change the bcfg path name back
 *   there should only be one bcfg file present at this point that
 *   we need to modify.  use bcfgfile[bcfgindex].location as the text
 *   to put in the Drvmap file.  Not necessary with our scheme -- we put
 *   BCFGPATH into the resmgr.  See comments in DoRemainingStuff and 
 *   DoResmgrStuff to this effect.
 * - copy html file hierarchy to new "location" on system
 * - /etc/conf/bin/idinstall -e -a -N the driver (-N says to not call
 *   idresadd - we do it ourselves at (possibly new) key rmkey.
 *   Also add -P nics for all idinstall/idremove commands.
 *   this will copy any *bcfg files found into the /etc/conf/bcfg.d directory
 *   (that's why we had to make a copy of our DSP files!)
 *   the -e is to pacify space checking (a problem across NFS) for add/delete
 * - Lastly, remove the tmp directory.
 * - XXX If PRE_SCRIPT= set then run it ONCE: before the idinstall
 */

int
DoDSPStuff(u_int bcfgindex, rm_key_t rmkey, struct System *System, 
           int *fullrelink)
{
   char sourcedir[SIZE];
   char sourcefile[SIZE];
   char destdir[SIZE];
   char tmpfilename[SIZE];
   char *tmpdirname;
   char *tmp;
   char *driver_name;
   char *lineX;
   char cmdbuf[SIZE];
   int loop,status;
   int numfiles;
   int numlines;
   struct stat sb;
   const char *tempnamdir=TEMPNAMDIR;
   const char *tempnampfx=TEMPNAMPFX;

   Status(81,"Preparing driver for installation");

   /* pkgadd uses /var/tmp, so will we.  Allow TMPDIR environment variable
    * to override.
    */
   driver_name = NULL;
   tmpdirname=tempnam(tempnamdir,tempnampfx);
   if (tmpdirname == NULL) {
      error(NOTBCFG,"tempnam/malloc failed");
      return(1);
   }
   strcpy(destdir,tmpdirname);
   if (mkdir(destdir,0700) == -1) {
      error(NOTBCFG,"mkdir(%s) failed",destdir);
      goto fail;
   } 
   strncpy(sourcedir,bcfgfile[bcfgindex].location,SIZE-10);
   if (sourcedir[0] != '/') {
      /* shouldn't happen */
      error(NOTBCFG,"sourcedir (%s) not absolute",sourcedir);
      goto fail;
   }
   tmp=strrchr(sourcedir,'/');
   if (tmp == NULL) {
      notice("strrchr failed in DoDSPStuff");
      strcpy(sourcedir,"");
   } else {
      strcpy(sourcefile,tmp+1);
      *(tmp+0)=NULL;  /* no trailing / on our directory */
   }

   /* we know the source directory existed once upon a time when we 
    * loaded the bcfgs.  check again, just in case
    */
   if (stat(sourcedir,&sb) == -1) {
      error(NOTBCFG,"can't stat %s; errno=%d",sourcedir,errno);
      goto fail;
   }
   if (!S_ISDIR(sb.st_mode)) {
      error(NOTBCFG,"%s isn't a directory!",sourcedir);
      goto fail;
   }
   /* copy this (and only this!) bcfg file to new directory, keeping
    * the same file name in new directory
    */
   if (copyfile(1,sourcedir,sourcefile,destdir,sourcefile) != 0) {
      error(NOTBCFG,"Problem copying %s/%s to %s/%s",
         sourcedir,sourcefile,destdir,sourcefile);
      goto fail;  /* we'll get multiple errors in log but that's ok */
   };

   /* If we ever decide to handle Driver.mod here we need to
    * - modadmin -u the old driver
    * - copy it into /etc/conf/mod.d
    * - generate the magic lines from its Master file and add to
    *   /etc/conf/mod_register
    * - run idmodreg -f /etc/conf/mod_register
    * - modadmin -l the new driver
    * all this does is just save you from doing an idbuild -M step.
    * also no point in trying to do a COPYFILE("Driver.mod") as idinstall
    * won't accept a Driver.mod for the above reasons.
    */

   /* copy all remaining DSP files that idinstall could want
    * we don't go through FILES= as this lists other bcfgs and html files
    * which we don't want
    * if file isn't present we don't particularly care -- idinstall will
    * sort it out.  The reason for copying all of the files to a separate
    * directory is that the bcfg.d directory will only contain the 
    * bcfgs that are actually in use vs doing a idinstall and having
    * _all_ of the driver's bcfg files wind up in bcfg.d.
    */

   /* we try for anything that could possibly be idinstalled.  Most
    * of these aren't associated with network drivers but do them anyway.
    */
   COPYFILE("Driver.o");
   COPYFILE("Driver_atup.o");
   COPYFILE("Driver_mp.o");
   COPYFILE("Driver_ccnuma.o");
   COPYFILE("Autotune");
   COPYFILE("Drvmap");
   COPYFILE("Dtune");
   COPYFILE("Ftab");
   COPYFILE("Init");
   COPYFILE("Master");
   COPYFILE("Modstub.o");
   COPYFILE("Mtune");
   COPYFILE("Node");
   COPYFILE("Rc");
   COPYFILE("Sassign");
   COPYFILE("Sd");
   COPYFILE("Space.c");
   COPYFILE("Stubs.c");
   COPYFILE("System");
   COPYFILE("firmware.o");
   COPYFILE("msg.o");

   chdir(sourcedir);
   if (ReadSystemFile(System,"System")) {
      error(NOTBCFG,"DoDSPStuff: problem reading System file");
      goto fail;
   }

   chdir(destdir);
   driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
   if (driver_name == NULL) {
      error(NOTBCFG,"DRIVER_NAME for bcfgindex %d is NULL!",bcfgindex);
      goto fail;
   }

   Status(82,"Adding driver '%s' to link kit",driver_name);

   /* this adds the actual hardware ('h' flag in Master) driver to the
    * link kit.  If the System file field two was 'Y' then it will be
    * turned off and taken over by any call to idconfupdate since
    * idconfupdate won't find any MODNAME entries in the resmgr which
    * use that hardware driver, so idconfupdate will turn it off.
    * idconfupdate is called automatically by idcheck which we will
    * call later if this is an MDI driver to find the instance number
    * for netX to use.  Just be aware that no matter what the System file
    * has, it will be turned off by our call to idcheck later on.  We
    * add MODNAME and run idconfupdate right before our final idbuild
    * so that it will be re-enabled at that time.
    */
   status=idinstalldriver(destdir,driver_name);
   AddToDLMLink(driver_name);
   free(driver_name);
   driver_name=NULL;
   if (status != 0) goto fail;  /* error called previously for us */
   /* now do documentation stuff.  Note this can overwrite stuff that
    * is there already
    * We run installf on these files.  Can't use StringListPrint since
    * that reflects the position relative to the DSP and not on the installed
    * system.
    */
   if (chdir(DOCDIR) == -1) goto nextstep;  /* doc directory doesn't exist */
   snprintf(tmpfilename,SIZE,"%s",sourcedir);
   if (chdir(tmpfilename) == -1) goto nextstep;  /* no doc in this DSP */

#if 0
   /* note that files with a suffix of .html won't fit on IHV disk if DOS */
   /* stupid cpio always prints "xx blocks" so we rely on exit status */
   snprintf(tmpfilename,SIZE,
                  "/usr/bin/find . -name \"*.html\" -print 2>/dev/null |"
                  "/usr/bin/cpio -pdum %s >/dev/null 2>&1",DOCDIR);
   status=docmd(0,tmpfilename);
   if (status != 0) {
      error(NOTBCFG,"error copying html files");
      goto fail;
   }

   /* register doc files with installf in new location */
   snprintf(tmpfilename,SIZE,"/usr/bin/find . -name \"*.html\" -print |"
           "/usr/bin/sed -e 's/^/%s/g' -e 's/\\/\\.//g' |"
           "/usr/sbin/installf nics -",DOCDIR2);
   status=docmd(0,tmpfilename);
   if (status != 0) {
      error(NOTBCFG,"error registering doc files with installf");
      goto fail;
   }
#endif
   
nextstep:
   /* add further processing here */

   /* handle PRE_SCRIPT.  PRE_SCRIPT should only move the necessary files
    * into place but not actually run any download code -- that's the job
    * for CONFIG_CMDS, used here and also by netinstall process
    * It's imperative that the PRE_SCRIPT run first to put the files in
    * place before we call the CONFIG_CMDS to run it.
    */
   numlines=StringListNumLines(bcfgindex, N_PRE_SCRIPT);
   if (numlines > 0) {  /* -1 means PRE_SCRIPT is undefined in bcfg file */
      notice("executing PRE_SCRIPT commands...");
      Status(83, "Executing PRE_SCRIPT commands");
      /* since we don't know what the PRE_SCRIPT will do to the system,
       * we set fullrelink to 1.  Most PRE_SCRIPT lines will plop down
       * something in /etc/rc2.d(likely for download code), so all we 
       * really need to do is reboot here and the card should be usable.
       * it's also possible that when we try and open up the card later on
       * near the end of idinstall() that the open will fail.  A good 
       * PRE_SCRIPT will also "start" itself at the same time so that
       * a reboot won't be necessary but we can't guarantee that all will do 
       * so.
       * The problem with this is at ISL time we can't use this because
       * when we do an idbuild -B the module isn't in mod.d to load
       * and the devices aren't created so the CONFIG_CMDS entries
       * wouldn't work.  We solve this by two things:
       * 1) don't set fullrelink to 1 here.  this means we will do a
       *    idbuild -M to create the DLM
       * 2) assuming that if not instantly usable that the .bcfg will also
       *    have a CONFIG_CMDS for us to run which will make things
       *    usable so we don't have to reboot.
       * 
       * notice("setting fullrelink to 1 because PRE_SCRIPT is defined");
       * *fullrelink = 1;
       */

      for (loop=1; loop <= numlines; loop++) {
         lineX=StringListLineX(bcfgindex, N_PRE_SCRIPT, loop);
         if ((lineX != NULL) && (*lineX != '\n') && (*lineX != NULL)) {
            /* niccfg didn't run prescript with any arguments.*/
            if (*lineX == '/') {
               snprintf(tmpfilename,SIZE,"%s",lineX);
            } else {
               snprintf(tmpfilename,SIZE,"/etc/inst/nics/scripts/%s",
                        lineX);
            }
            /* niccfg didn't care about exit status of script so we can't 
             * either.  so we call runcommand instead of docmd
             */
            if (stat(tmpfilename, &sb) == 0) {
               sb.st_mode |= S_IXUSR;   /* only owner needs execute perms */
               chmod(tmpfilename, sb.st_mode); 
               runcommand(0, tmpfilename);
            } else {
               notice("not running PRE_SCRIPT cmd '%s' - does not exist",
                      tmpfilename);
            }
            free(lineX);
         } else {
            if (lineX != NULL) free(lineX);
         }
      }
   }

   /* All done.  now clean up our mess. */
   chdir("/");
   snprintf(tmpfilename,SIZE,"/bin/rm -rf %s",tmpdirname);
   runcommand(0,tmpfilename);  /* it might work, it might not.  we don't care */
   rmdir(destdir);
   free(tmpdirname);  /* tempnam calls malloc */
   if (driver_name != NULL) free(driver_name);
   return(0);  /* success */
   /* NOTREACHED */
fail:
   chdir("/");
   snprintf(tmpfilename,SIZE,"/bin/rm -rf %s",tmpdirname);
   runcommand(0,tmpfilename);  /* it might work, it might not.  we don't care */
   rmdir(destdir);
   free(tmpdirname);
   if (driver_name != NULL) free(driver_name);
   /* remove this key (ISA) or resmgr params associated with 
    * key (PCI/EISA/MCA)  - moved this line to main idinstall fail case 
    * (void) ResmgrDelInfo(rmkey,1...);
    */
   return(1);  /* failure */
}


/* this routine does common work necessary to add a new instance of of the
 * driver for the link kit.  It is called on first, second, third, etc.
 * instance of board in resmgr.   returns 0 for success.
 *
 * - ALL: add CUSTOM[x] parameters to resmgr
 * - ISA: Add IRQ/DMAC/IOADDR/MEMADDR to resmgr at key rmkey if present
 * - ISA: add CM_BRDBUSTYPE to resmgr 
 * - ALL: Add BINDCPU, UNIT, IPL, and ITYPE if present and if not
 *        assume reasonable defaults.  Note that ITYPE is the only one
 *        that is implictly set when drivers do a cm_AT_putconf with
 *        an itype field and setmask has CM_SET_ITYPE bit set.
 *        BINDCPU is only read by the autoconfig/resmgr as is IPL
 *        UNIT is ignored by the autocofig/resmgr entirely but we set it
 *        to numboards for ISA drivers.
 * - ALL: add NIC_CARD_NAME
 * - ALL (if CUSTOM[x]): add NIC_CUST_PARAM
 * XXX ALL: add everything in #ADAPTER section to resmgr?
 * - ALL: add BCFGPATH to resmgr pointing to bcfg file
 * - ALL: add CM_ENTRYTYPE 
 * If type is ODI and ODIMEM=true, add ODIMEM=true to resmgr.  This
 * is for later memory size calculations for idtune for odimem driver.
 * do NOT add CM_MODNAME -- that happens last.  That's the trigger for DDI8.
 */
int
DoResmgrStuff(stringlist_t *slp, u_int bcfgindex, rm_key_t rmkey, int netX,
              u_int boardnumber, struct System *System, int *fullrelink,
              int *numpatches, struct patch *patch,
              int *delbindcpu, int *delunit, int *delipl, char **tunes,
              int *numtunes, char *real_topology)
{
   int numparams,numcustomparams,loop,isabus=0,ityperet,iplret,numlines;
   unsigned long newvalue;
   char tmp[SIZE];
   char niccustparams[SIZE];
   char *irq, *dma, *mem, *port, *bdg, *strend;
   char *oldirq, *olddma, *oldmem, *oldport;
   char *customX, *customtopo;
   char *fullname;
   char *depend;
   char *unit;
   char key[10];
   char bcfgindexstr[10];
   char olddelim;

   olddelim=delim;
   customX = NULL;
   fullname= NULL;
   depend=NULL;
   snprintf(key,10,"%d",rmkey);

   Status(87, "Adding parameters to resource manager");

   if (HasString(bcfgindex, N_BUS, "ISA", 0) == 1) {
      /* double check that this machine has an ISA bus installed on it. 
       * checked at beginning of idinstall command but if called elsewhere
       * we'll print out a notice and continue...
       */
      if ((cm_bustypes & CM_BUS_ISA) == 0) {
         notice("DoResmgrStuff: adding ISA but machine doesn't have ISA bus!");
         goto afterisa;
      }
      /* this is an ISA bus.  Add ISA-specific parameters to resmgr 
       * we checked earlier to ensure bcfg file actually requires these
       * parameters.  Key is brand new so these parameters don't exist yet
       * We know that we're ISA and IRQ doesn't exist in the resmgr(checked
       * earlier in idinstall(), so CM_ITYPE issues don't apply.
       */
      isabus=1;
      if ((irq=FindStringListText(slp, "IRQ=")) != NULL) {
         /* if user gave us IRQ=2, convert it to IRQ=9.  See IsParam() too  */
         if (atoi(irq) == 2) irq="9";
         snprintf(tmp,SIZE,"%s=%s",CM_IRQ,irq);   /* no ,n needed */
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
      if ((dma=FindStringListText(slp, "DMAC=")) != NULL) {
         snprintf(tmp,SIZE,"%s=%s",CM_DMAC,dma);  /* no ,n needed */
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
      if ((port=FindStringListText(slp, "IOADDR=")) != NULL) {
         char *dash;
         snprintf(tmp,SIZE,"%s=%s",CM_IOADDR,port);  /* no ,r needed */
         dash=strchr(tmp,'-');
         if (dash == NULL) {
            error(NOTBCFG,"syntax for IOADDR is IOADDR=num1-num2");
            goto resfail;
         }
         *dash=delim;
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
      if ((mem=FindStringListText(slp, "MEMADDR=")) != NULL) {
         char *dash;
         snprintf(tmp,SIZE,"%s=%s",CM_MEMADDR,mem);  /* no ,r needed */
         dash=strchr(tmp,'-');
         if (dash == NULL) {
            error(NOTBCFG,"syntax for MEMADDR is MEMADDR=num1-num2");
            goto resfail;
         }
         *dash=delim;
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
#if 0
      these are no longer used; a later call to isaautodetect at the end
      of idinstall will look for OLDIOADDR and use the information when
      calling the verify routine.  The others (OLDIRQ, OLDDMAC, OLDMEMADDR)
      aren't relevant to how the verify routine system works.

      Moreover, OLDIOADDR and OLDMEMADDR don't work as shown below because the
      current delimiter at this point isn't '-', which is how these
      parameters come off the idinstall command line as arguments, so
      RMputvals barfs with 22(Invalid argument)

      /* now handle the OLD* strings which require typing since libresmgr
       * has never heard of them
       */
      if ((oldirq=FindStringListText(slp, "OLDIRQ=")) != NULL) {
         snprintf(tmp,SIZE,"%s,n=%s",CM_OLDIRQ,oldirq);   /* ,n = numeric */
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
      if ((olddma=FindStringListText(slp, "OLDDMAC=")) != NULL) {
         snprintf(tmp,SIZE,"%s,n=%s",CM_OLDDMAC,olddma);   /* ,n = numeric */
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
      if ((oldport=FindStringListText(slp, "OLDIOADDR=")) != NULL) {
         char *dash;

         /* ,r = numeric range */
         snprintf(tmp,SIZE,"%s,r=%s",CM_OLDIOADDR,oldport);  
         dash=strchr(tmp,'-');
         if (dash == NULL) {
            error(NOTBCFG,"syntax for OLDIOADDR is OLDIOADDR=num1-num2");
            goto resfail;
         }
         *dash=delim;
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
      if ((oldmem=FindStringListText(slp, "OLDMEMADDR=")) != NULL) {
         char *dash;

         /* ,r = numeric range */
         snprintf(tmp,SIZE,"%s,r=%s",CM_OLDMEMADDR,oldmem);
         dash=strchr(tmp,'-');
         if (dash == NULL) {
            error(NOTBCFG,"syntax for OLDMEMADDR is OLDMEMADDR=num1-num2");
            goto resfail;
         }
         *dash=delim;
         if (resput(key,tmp,0)) goto resfail;  /* error called in resput */
      }
#endif
   }
afterisa:
   /* common resmgr stuff for all bus types follows */

   /* don't set BINDCPU unless it's something other than -1 
    * while cm_get_intr_info() allows -1 as a valid value (and that's what
    * gets used if BINDCPU isn't present in the resmgr) this 
    * tends to confuse users and the dcu which prints a '-' when this hasn't
    * been set yet and otherwise expects a number 0-X.
    */
   if (System->bind_cpu != -1) {
      snprintf(tmp,SIZE,"%s=%d",CM_BINDCPU,System->bind_cpu);/* no ,n needed */
      if (resput(key,tmp,0)) goto resfail;    /* error called in resput */
      *delbindcpu = 1;
   }
   /* CM_UNIT is only really beneficial for ISA boards
    * If UNIT= is set in the bcfg file then shove that into the resmgr
    * for UNIT, like UW2.1 does (see niccfg line 4805)
    * If UNIT= isn't set in the bcfg file, then use the System file 
    * value as the default.  This also follows example set by niccfg.
    */
   unit=StringListPrint(bcfgindex, N_UNIT);
   if (unit == NULL) {
      snprintf(tmp,SIZE,"%s=%d",CM_UNIT,System->unit);   /* no ,n needed */
   } else {
      snprintf(tmp,SIZE,"%s=%d",CM_UNIT,atoi(unit));     /* no ,n needed */
      free(unit);
   }
   if (resput(key,tmp,0)) goto resfail;    /* error called in resput */
   *delunit = 1;

   /* don't add CM_INSTNUM for DDI7 because we need to recompute it if the user
    * pulls the board like DDI8 does.  We don't currently do that. 
    * ifdef CM_INSTNUM
    *    snprintf(tmp,SIZE,"%s,s=%d",CM_INSTNUM, boardnumber);
    *    if (resput(key,tmp,0)) goto resfail;
    * endif
    */

   /* if IPL isn't already set in the resmgr, then add it ourselves
    * with whatever the System file says.  In smart-bus cases it may/will
    * already be set.  Added check to fully emulate hpsl library code routine
    * hpsl_bind_driver() -- in old days we always blindly added IPL.
    */
   if ((iplret=IplSet(rmkey)) == 0) {
      snprintf(tmp,SIZE,"%s=%d",CM_IPL,System->ipl);   /* no ,n needed */
      if (resput(key,tmp,0)) goto resfail;    /* error called in resput */
      *delipl = 1;
   }
   if (iplret == -1) {
      error(NOTBCFG,"DoResmgrStuff: IplSet failed");
      goto resfail;
   }

   /* If ITYPE isn't already set in the resmgr, then add it ourselves with 
    * whatever the System file says.  Most smart-bus boards (PCI, EISA) will
    * automatically have this set by the kernel autoconf subsystem
    * (indeed, it will be _re_-set back to the autoconf value the next time you 
    * reboot anyway so why bother with System in this case?)
    * ISA boards won't have CM_ITYPE set since we add the key so we will
    * add ITYPE to the resmgr.  Also, autoconf won't change CM_ITYPE on us.
    */
   if ((ityperet=ItypeSet(rmkey)) == 0) {
      snprintf(tmp,SIZE,"%s=%d",CM_ITYPE,System->itype); /* no ,n needed */
      if (resput(key,tmp,0)) goto resfail;    /* error called in resput */
   }
   if (ityperet == -1) {
      error(NOTBCFG,"DoResmgrStuff: ItypeSet failed");
      goto resfail;
   }

   /* quite possible the real reason that NAME= cannot have spaces is for
    * RMputvals using space as a delimiter.  obviously someone didn't
    * know that could be changed, which we do now
    */
   olddelim=delim;
   delim='\1';   /* ASCII 1 = SOH, an unlikely character */

   /* Now add the CUSTOM[x] variables to the resmgr.  We can only have
    * 9 CUSTOM variables max.  we use showcustomnum which is a better
    * way of knowing exactly how many we have
    * we then get the variable name, then look for it as an argument
    */
   snprintf(bcfgindexstr,10,"%d",bcfgindex);
   numcustomparams=showcustomnum(0,bcfgindexstr);
   if (numcustomparams == -1) goto resfail; /* error called in showcustomnum */
   if (numcustomparams == 0) goto aftercustom;  /* no CUSTOM[x] entries */
   niccustparams[0]='\0';
   for (loop=1;loop <= numcustomparams; loop++) {
      char customparamstr[20];
      char valueequal[SIZE];
      char *value, *actualvalue;
      char *firstspace;
      int offset;

      snprintf(customparamstr,20,"CUSTOM[%d]",loop);
      offset=N2O(customparamstr);
      customX=StringListPrint(bcfgindex,offset);
      if (customX == NULL) {
         /* shouldn't happen */
         error(NOTBCFG,"couldn't find %s out of %d CUSTOM parameters",
               customparamstr,numcustomparams);
         goto resfail;
      }
      firstspace=strchr(customX,' ');
      if (firstspace == NULL) {
         /* shouldn't happen */
         notice("couldn't find space in %s out of %d:%s",
               customparamstr,numcustomparams,customX);
         goto resfail;  /* don't continue */
      }
      *firstspace='\0';
      /* now customX points to the parameter to add into the resmgr.  Lets
       * also look for it in our parameter list to idinstall cmd.
       */
      snprintf(valueequal,SIZE,"%s=",customX);
      if ((value=FindStringListText(slp, valueequal)) == NULL) {
         /* if this is a version 0 bcfg file or the topology of this custom
          * parameter matches the topology we're using then that's a 
          * problem.  If topology is different then non-error; use default.
          */
         if (bcfgfile[bcfgindex].version == 0) {
            error(NOTBCFG,
                  "mandatory bcfgindex %d %s param (%s=) not supplied as "
                  "argument to idinstall",
                  bcfgindex,customparamstr,customX);
            goto resfail;
         } else if (bcfgfile[bcfgindex].version == 1) {

            /* they only have 9, but sometimes they don't end with FOO"
             * but have FOO\n" (the " on a line all by itself making 10 lines
             */
            if ((StringListNumLines(bcfgindex, offset)) != 9 &&
                (StringListNumLines(bcfgindex, offset) != 10)) {
               error(NOTBCFG,"invalid number of lines for V1 custom param %s",
                     customX);
               goto resfail;
            }

            /* line 8 in version 1 bcfg files has applicable topologies */
            customtopo=StringListLineX(bcfgindex, offset, 8);
            if (customtopo == NULL) {
               error(NOTBCFG,"invalid line 8 custom param '%s'", customX);
               goto resfail;
            }
            strtoupper(customtopo);
            if (strstr(customtopo, real_topology)) {
               /* this custom parameter applies to topo we're installing with
                * and it wasn't specified on the command line. error
                */
               error(NOTBCFG,
                     "mandatory bcfgindex %d %s param (%s=) not supplied as "
                     "argument to idinstall for topology %s",
                     bcfgindex,customparamstr,customX,real_topology);
               free(customtopo);
               goto resfail;
            }
            /* custom parameter doesn't apply to this topo and it wasn't
             * supplied on the command line
             * no error here; continue on but don't add param to resmgr
             * note we don't care if scope is DRIVER, BOARD, GLOBAL, NETX,
             * or even PATCH -- this parameter doesn't apply
             */
            free(customtopo);
            free(customX);
            continue;
         } else { 
            error(NOTBCFG,"unknown .bcfg version %d",
                  bcfgfile[bcfgindex].version);
            goto resfail;
         }
      }

      if (bcfgfile[bcfgindex].version == 1) {
         /* just because custom parameter is supplied on the command line 
          * doesn't mean we should use it for the topology we're installing 
          * with
          */
         if ((StringListNumLines(bcfgindex, offset) != 9) &&
             (StringListNumLines(bcfgindex, offset) != 10)) {
            error(NOTBCFG,"invalid number of lines for custom param %s",
                    customX);
            goto resfail;
         }

         customtopo=StringListLineX(bcfgindex, offset, 8);
         if (customtopo == NULL) {
            error(NOTBCFG,"invalid line 8 custom param '%s'", customX);
            goto resfail;
         }

         if (strstr(customtopo, real_topology) == NULL) {
            notice("custom param %s isn't applicable for topology %s, skipping",
                   customX, real_topology);
            free(customtopo);
            free(customX);
            continue;
         }
         free(customtopo);
      }

      actualvalue=value;
      strcat(niccustparams,customX);
      if (loop != numcustomparams) strcat(niccustparams," ");
      /* NOTE: 
       * - all CUSTOM[x] parameters are stored as _strings_ in the resmgr 
       * - we must supply typing for libresmgr
       * - assumes that delim is SOH here if we want to add spaces as part of
       *   our string in the resmgr
       */
      
      /* occasionally netcfg slips up and feeds us \"\" instead of __STRING__ 
       * so put the proper thing into the resmgr for drivers to read
       */
      if (strcmp(actualvalue,"\\\"\\\"") == 0) {
         actualvalue="__STRING__";
      }

      snprintf(tmp,SIZE,"%s,s=%s",customX,actualvalue);
      if (resput(key,tmp,0)) goto resfail;    /* error called in resput */

      /* ok.  the parameter that the driver wants is in the resmgr.  now
       * ensure the parameter_CHOICE= is supplied on the command line
       * and put that into the resmgr too.  this is crucial for 
       * showcustomcurrent command in identifying what is set.
       */
      snprintf(valueequal,SIZE,"%s%s=",customX,CM_CUSTOM_CHOICE);
      if ((value=FindStringListText(slp, valueequal)) == NULL) {
         error(NOTBCFG,
               "mandatory bcfgindex %d parameter '%s%s=' not supplied as "
               "argument to idinstall",bcfgindex,customX,CM_CUSTOM_CHOICE);
          
         goto resfail;
      }
      snprintf(tmp,SIZE,"%s%s,s=%s",customX,CM_CUSTOM_CHOICE,value);
      if (resput(key,tmp,0)) goto resfail;    /* error called in resput */

      /* to handle global custom parameters, we need to save the type of 
       * parameter this is in the resmgr
       */

      /* how many lines does this custom parameter have: v0 or v1 style? */
      if (bcfgfile[bcfgindex].version == 1) {
         numlines=StringListNumLines(bcfgindex, offset);
         if ((numlines != 9) && (numlines != 10)) {
            error(NOTBCFG,"invalid #lines for bcfgindex %d custom param %s",
                  bcfgindex, customX);
            goto resfail;
         }
         /* v1 style bcfg file; use what it says */
         bdg=StringListLineX(bcfgindex, offset, 9); 
         if (bdg == NULL) {
            notice("bad line 9 for custom parameter %s bcfgindex %d",
                   customparamstr, bcfgindex);
            bdg=strdup("BOARD");
         }
         strtoupper(bdg);
         /* if it's not BOARD, DRIVER, PATCH, or GLOBAL then assume BOARD */
         if ((strcmp(bdg,"BOARD") != 0) &&
             (strcmp(bdg,"DRIVER") != 0) &&
             (strcmp(bdg,"NETX") != 0) &&
             (strcmp(bdg,"PATCH") != 0) &&
             (strcmp(bdg,"GLOBAL") != 0)) {
             notice("bogus BOARD/DRIVER/NETX/PATCH/GLOBAL line 9: %s",bdg);
             free(bdg);
             bdg=strdup("BOARD");
         }
      } else if (bcfgfile[bcfgindex].version == 0) {
         /* v0 style bcfg file; assume custom parameter is per BOARD */
         bdg=strdup("BOARD");  /* since we call free below */
      } else {
         error(NOTBCFG,"DoResmgrStuff: bad version %d",
               bcfgfile[bcfgindex].version);
         goto resfail;
      }

      /* default for custom parameters is BOARD.
       * DRIVER means do an idtune for this parameter.  the assumption is
       * that the idtune will only manipulate parameters in the DSP
       * Dtune/Mtune file, so a full idbuild afterwords is not necessary
       * GLOBAL means do an idtune for the parameter and do a full idbuild -B
       * afterwords.  this allows a GLOBAL parameter to do an idtune
       * for any driver's Dtune/Mtune file -- not just its own.
       * NOTE! these parameters are only set once as we only get to
       * this routine when adding the first instance of the driver to the 
       * system  subsequent multiple additions of this driver won't get here.
       * the good news is that when we do the idbuild -M later on our
       * changes will be picked up by that kernel build.
       */
      if ((strcmp(bdg,"DRIVER") == 0) || 
          (strcmp(bdg,"GLOBAL") == 0)) {
         if (strcmp(actualvalue, "__STRING__") == 0) {
            notice("skipping DRIVER/GLOBAL scope custom; value is __STRING__");
         } else {
            /* we want our changes reflected in next idbuild -M, use -c 
             * it's also possible that driver has an actual text parameter
             * for a kernel tunable (not just a numeric parameter).  
             * If the user says MEDIA=foobar instead of MEDIA=2 then we allow
             * it (although it may not work properly!)
             * XXX TODO  if users enters multiple words that will almost
             * certainly cause problems.  should fix this
             */
            /* because we want changes reflected in next idbuild -M, use -c */
            snprintf(tmp,SIZE,"/etc/conf/bin/idtune -f -c %s %s",
                     customX, actualvalue);
            (void) runcommand(0,tmp); /* don't care if cmd succeeds or fails */
            if (strcmp(bdg,"DRIVER") == 0) {
               char *driver_name;
               /* note we don't (indeed, in most cases we can't) unload driver
                * so we say to reboot machine instead.  driver scopes do an
                * idtune on that particular driver -- it's up to the driver
                * to have a Space.c which will pick up the change
                */
               driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
               if (driver_name != NULL) {
                  char cmd[SIZE];
                  int status;

                  snprintf(cmd,SIZE,"/etc/conf/bin/idbuild -M %s",driver_name);
                  free(driver_name);
                  status=runcommand(0, cmd);
                  notice("setting fullrelink to 1 because of DRIVER custom %s",
                           customX);
                  *fullrelink=1;
               } else {
                  /* shouldn't happen */
                  notice("no driver_name, DRIVER scope, fullrelink now 1");
                  *fullrelink=1;
               }
            } else {
               /* globals can do an idtune on any parameter so we must
                * do full relink since we don't know the affected driver(s)
                */
               notice("setting fullrelink to 1 because of GLOBAL custom %s",
                        customX);
               *fullrelink=1;
            }
         }
      }

      /* Mtune file starts with a line which reads something like:
       * YYYYSRAREDISAB     0               0               1
       * this was converted by the MANGLE macro into something like
       * NET0SRAREDISAB     0               0               1
       * if the custom parameter line 9 (the scope) says NETX
       * and line 1 (the affected kernel parameter) says SRAREDISAB
       * and the user passes in an "idmodify blah blah SRAREDISAB=1"
       * then we build the following command:
       * /etc/conf/bin/idtune -f -c NET0SRAREDISAB 1
       * later on when we do the idbuild -M net0 this will automatically
       * get picked up.  Neat, huh?
       * The only hard part is that the netX driver doesn't exist in the link
       * kit yet, so we build our idtune command and issue it later on
       * in DoRemainingStuff->AddDLPIToLinkKit after the MANGLE macros.
       */
      if (strcmp(bdg,"NETX") == 0) {
         if (HasString(bcfgindex,N_TYPE,"MDI",0) == 1) {

            if (strcmp(actualvalue,"__STRING__") == 0) {
               /* if someone makes param a __STRING__ in the .bcfg file
                * and the user just hits return and doesn't fill in anything
                * then we should skip it and use the default in the Mtune file
                * occasionally netcfg slips and hands us FOO=\"\" instead of
                * FOO=__STRING__
                */
               notice("skipping NETX scope custom %s since value is %s",
                      customX, actualvalue);
            } else {
               /* NETX scope parameters are all numbers and not text strings
                * at all (or multiple words.  If we cannot convert the
                * string into a valid base 10 number then skip it; the 
                * user entered a bogus string or the .bcfg file is bad
                */
               unsigned long foo;
               char *foop;

               if (((foo=strtoul(actualvalue,&foop,10))==0) &&
                     (foop == actualvalue)) {
                  notice("skipping NETX scope custom %s: bad number %s",
                         customX, actualvalue);
               } else {
                  snprintf(tmp,SIZE,
                        "/etc/conf/bin/idtune -f -c NET%d%s %s",
                        netX, customX, actualvalue);
                  if (*numtunes == MAXNETXIDTUNES) {
                     /* by not adding tmp to tunes array we won't do an idtune
                      * on this parameter, keeping it the way it is now (which 
                      * is the default in the Mtune file)
                      */
                     notice("DoResmgrStuff: not adding '%s' to tunes "
                            "array - no room", tmp);
                  } else {
                     tunes[*numtunes]=strdup(tmp);  /* since tmp is on stack */
                     if (tunes[*numtunes] == NULL) {
                        /* malloc failed */
                        notice("DoResmgrStuff: strdup for '%s' failed",tmp);
                     } else {
                        (*numtunes)++;
                     }
                  }
                  /* don't set fullrelink to 1; idbuild -M will handle it and
                   * we will soon add the -M netX to our link line 
                   * in AddDLPIToLinkKit()
                   */
               }
            }
         } else {
            notice("ignoring NETX scope custom parameter for non-MDI driver");
         }
      }

      if (strcmp(bdg,"PATCH") == 0) {
         /* we can't do the patch now since the driver hasn't been 
          * idbuild -M and modadmin -l  (built and loaded)
          * so its symbol table isn't visible, but we can save the 
          * necessary values for patching later on after we have done the
          * build and load
          */
         if (strcmp(actualvalue, "__STRING__") == 0) {
            notice("skipping PATCH parameter; value entered is __STRING__");
         } else {
            if (((newvalue=strtoul(actualvalue,&strend,10))==0) &&
                 (strend == actualvalue)) {
               notice("DoPatchCustom: invalid patch value %s", actualvalue);
               /* continue on to next parameter to patch*/
            } else {
               strncpy(patch[*numpatches].symbol, customX, SIZE);
               patch[*numpatches].newvalue = newvalue;
               *numpatches = (*numpatches) + 1;
            }
         }
      }

      snprintf(tmp,SIZE,"%s%s,s=%s",CM_CUSTOM_CHOICE,customX,bdg);
      free(bdg);
      if (resput(key,tmp,0)) goto resfail;    /* error called in resput */

      free(customX);
      customX=NULL;  /* so resfail won't try to free it again */
   }
   /* do NIC_CUST_PARM parameters since we have CUSTOM[x] params
    * note we can get here if all of the custom parameters are not 
    * applicable to the real_topology we're installing at.  For example,
    * customs may only be applicable to TOKEN but we're installing using
    * ETHER.  Obviously TOPOLOGY in the .bcfg must be multivalued too
    * This effectively means that things that call showcustomcurrent must
    * be prepared to see no custom parameters back.  That is,
    * just because showcustomnum returns a number doesn't necessarily mean
    * that all of those custom parameters are applicable to the topology
    * that netcfg is working with!
    */
   if (strlen(niccustparams) > 0) {
      snprintf(tmp,SIZE,"%s,s=%s",CM_NIC_CUST_PARM,niccustparams);
      if (resput(key,tmp,0)) goto resfail;
   }
   customX = NULL;
   
aftercustom:

   /* do NIC_CARD_NAME */
   fullname=StringListPrint(bcfgindex,N_NAME);
   if (fullname != NULL) {
      snprintf(tmp,SIZE,"%s,s=%s",CM_NIC_CARD_NAME,fullname);
      if (resput(key,tmp,0)) goto resfail;
      free(fullname);
      fullname=NULL;
   }

   /* add the actual topology being used to the resmgr instead of
    * all topologies the bcfg is capable of handling.  crucial for
    * idmodify custom parameter support.  Can't be multi valued!
    */
   snprintf(tmp,SIZE,"%s,s=%s",CM_TOPOLOGY,real_topology);
   if (resput(key,tmp,0)) goto resfail;
   

   /* add NIC_DEPEND for dependent drivers with a trailing space for
    * a later call to strstr to avoid messy substring match problems.
    */
   depend=StringListPrint(bcfgindex,N_DEPEND);
   if (depend != NULL) {
      snprintf(tmp,SIZE,"%s,s=%s ",CM_NIC_DEPEND,depend); /* trailing space */
      if (resput(key,tmp,0)) goto resfail;
      free(depend);
      depend=NULL;
   }

   /* add BCFGPATH.  For identifying new bcfgindex later on
    * in modification since 3rd party drivers and bcfgs may be added
    * or deleted, causing bcfgindex to get shifted.  later on when
    * user chooses to modify existing card the BCFGPATH will be the
    * only way of identifying where the bcfg file is, since the bcfgindex
    * will be different.   See BCFGINDEX comment in DoRemainingStuff() for
    * more information.
    *
    * UW21 uses the bcfg path field in the Drvmap file for essentially the
    * same purpose.  We don't use it.
    *
    */
   snprintf(tmp,SIZE,"%s,s=%s",CM_BCFGPATH,bcfgfile[bcfgindex].location);
   if (resput(key,tmp,0)) goto resfail;

   /* NETCFG_ELEMENT is added to resmgr in DoRemainingStuff */


   /* do CM_ENTRYTYPE.  We want CM_ENTRYTYPE to be set to CM_ENTRY_DRVOWNED
    * so that the dcu won't allow modifications to this key 
    * and idresadd -n won't touch it.  (note that we don't
    * call idresadd in ndcfg).
    */
   snprintf(tmp,SIZE,"%s=%d",CM_ENTRYTYPE,CM_ENTRY_DRVOWNED);
   if (resput(key,tmp,0)) goto resfail;

   /* and finally reset our delimiter back to normal */
   delim=olddelim;

   /*
    * If type is ODI and ODIMEM=true, add ODIMEM=true to resmgr.  This
    * is for later memory size calculations for idtune for odimem driver.
    */
   if ((HasString(bcfgindex,N_TYPE,"ODI",0) == 1) &&
       (HasString(bcfgindex,N_ODIMEM,"true",0) == 1)) {
      snprintf(tmp,SIZE,"%s,s=true",CM_ODIMEM); /*should match ResODIMEMCount*/
      if (resput(key,tmp,0)) goto resfail;
   }

   /* if netisl is 1, put IICARD into the resmgr like UW2.1 netinstall
    * does.  drivers can use this now and later to determine if this 
    * card is/was (being) used for network install purposes.  
    * see UW2.1 usr/src/work/sysinst/desktop/menus/ii_do_netinst
    */
   if (netisl == 1) {
      snprintf(tmp,SIZE,"IICARD,s=1");
      if (resput(key,tmp,0)) goto resfail;
   }

   /* put other stuff to do here */

   /* all done.  clean up */
   return(0);
   /* NOTREACHED */
resfail:
   if (customX != NULL) free(customX);
   if (fullname != NULL) free(fullname);
   if (depend != NULL) free(depend);
   delim=olddelim;
   /* remove this key (ISA) or resmgr params associated with 
    * key (PCI/EISA/MCA)  - moved next line into idinstall main code fail case
    * (void) ResmgrDelInfo(rmkey,1...);
    */
   return(-1);
}

/* add the given module to the list of modules that will be turned
 * into DLMs later on at idbuild time
 */
void
AddToDLMLink(char *module)
{
   if (strlen(DLM) + strlen(module) + 8 > SIZE) {
      error(NOTBCFG,"AddToDLMLink:  Too many modules");
      return;
   }
   strcat(DLM,"-M ");
   strcat(DLM,module);
   strcat(DLM," ");
   strcat(modlist,module);
   strcat(modlist," ");

}

/* install or update the specified driver from the HIERARCHY */
int
InstallOrUpdate(int makedlm, char *module)
{
   int status;
   char dir[SIZE];
   snprintf(dir,SIZE,"%s/%s",HIERARCHY,module);

   status=0;
   if (netisl == 0) {
      status=idinstalldriver(dir,module);
   }
   if ((makedlm != 0) || (netisl != 0)) AddToDLMLink(module);
   return(status);
}

/* netX is only valid and used when realDLPI is 0.  Likewise,
 * tunes is only valid and used when realDLPI is 0
 */
int
AddDLPIToLinkKit(int realDLPI, int netX, char **tunes, int *numtunes)
{
   char tmp[SIZE];
   char *tmpdirname;
   char *sourcedir;
   char *destdir;
   char realdlpilocation[SIZE];
   char dummylocation[SIZE];
   char drvstr[20];
   int status,loop;
   const char *tempnamdir=TEMPNAMDIR;
   const char *tempnampfx=TEMPNAMPFX;

   tmpdirname=NULL;
   snprintf(realdlpilocation, SIZE, "%s/dlpi", NDHIERARCHY);
   if (chdir(realdlpilocation) == -1) {
      error(NOTBCFG,"couldn't chdir into %s for dlpi driver",realdlpilocation);
      goto addfail;
   }
   snprintf(dummylocation, SIZE, "%s/netX", NDHIERARCHY);
   if (chdir(dummylocation) == -1) {
      error(NOTBCFG,"couldn't chdir into %s for netX driver", dummylocation);
      goto addfail;
   }
   tmpdirname=tempnam(tempnamdir,tempnampfx);
   if (tmpdirname == NULL) {
      error(NOTBCFG,"tempnam/malloc failed");
      goto addfail;
   }
   if (mkdir(tmpdirname,0700) == -1) {
      error(NOTBCFG,"mkdir(%s) failed",tmpdirname);
      goto addfail;
   }
   destdir=tmpdirname;           /* used by COPYFILE */
   /* copy the Driver.o from dummy into temp dir */
   if (realDLPI) {
      sourcedir=realdlpilocation; /* used by COPYFILE */
      snprintf(drvstr,20,"dlpi");
   } else {
      sourcedir=dummylocation;    /* used by COPYFILE */
      snprintf(drvstr,20,"net%d",netX);
   }

   /* DSP from either dummy or real dlpi location */
   COPYFILE("Driver.o");

   /* in case we ever compile dlpi/netX for different flavours, do these too */
   COPYFILE("Driver_atup.o");
   COPYFILE("Driver_mp.o");
   COPYFILE("Driver_ccnuma.o");

   COPYFILE("Master");
   COPYFILE("System");
   chdir(tmpdirname);

   if (realDLPI == 0) {
      char NETstr[20];

      snprintf(NETstr,20,"NET%d",netX);

#define MANGLE(A) snprintf(tmp,SIZE,   \
"/usr/bin/sed -e 's/XXXX/%s/g' -e 's/YYYY/%s/g' %s > %s+; /usr/bin/mv %s+ %s",\
                     drvstr, NETstr, A, A, A, A); \
         if ((status=docmd(0,tmp)) != 0) { \
            error(NOTBCFG,"AddDLPIToLinkKit: trouble mangling %s",A); \
            goto addfail; \
         }
   
      COPYFILE("Space.c");
      COPYFILE("Node");
      COPYFILE("Autotune");
      COPYFILE("Dtune");
      COPYFILE("Mtune");
      /* replace all of the XXXX and YYYY chars with drvstr and NETstr 
       * drvstr is net%d (lower case) and NETstr is NET%d (upper case)
       */
      MANGLE("Master")
      MANGLE("System")
      MANGLE("Space.c")
      MANGLE("Node")
      MANGLE("Autotune")
      MANGLE("Dtune")
      MANGLE("Mtune")
#undef MANGLE
      /* while we called getlowestnetX at the beginning of idinstall we
       * *ASSUME* that this is still the case, as idinstalldriver should
       * do a fresh install of the netX driver into the link kit.  If
       * two versions of ndcfg do an idinstall at the same time then
       * this premise obviously falls apart.  We need a huge semaphore
       * at the start of our idinstall() code so that we won't have 
       * contention for multiple netX drivers.
       */
      status=idinstalldriver(tmpdirname,drvstr);
      if (status != 0) {
         error(NOTBCFG,"trouble installing mangled %s into link kit",drvstr);
         goto addfail;
      }

      /* get rid of any occurances of netX keys in resmgr, when we're done
       * with the backup key we'll write out MODNAME=netX, but the
       * key that idresadd just created (yes, it creates one though
       * we specified -N to idinstall) is bogus in that it doesn't have our
       * backup key information there (also has wrong ENTRYTYPE too)
       * dlpiopen calls cm_getnbrd and fails the open if we have > 1 netX
       * driver in the resmgr(2 net0 drivers or 3 net12 drivers).
       */
      if (DelAllKeys(drvstr)) {
         error(NOTBCFG,"trouble removing '%s' from link kit",drvstr);
         goto addfail;
      }
   } else {
      status=idinstalldriver(tmpdirname,drvstr);
      if (status != 0) {
         error(NOTBCFG,"trouble installing mangled %s into link kit",drvstr);
         goto addfail;
      }
   }
   
   AddToDLMLink(drvstr);

   /* if we have any NETX scope custom parameters that we must process do
    * them here now that the driver has been installed.  These are 
    * a series of idtune commands to tweak certain aspects of the
    * netX driver (source routing in particular)
    */
   if (realDLPI == 0) {
      for (loop=0; loop < *numtunes; loop++) {
         status=docmd(0,tunes[loop]);
         if (status != 0) {
            error(NOTBCFG,"AddDLPIToLinkKit: trouble issuing idtune command");
            goto addfail;
         }
         /* space freed at the end of idinstall on either success or failure */
      }
   }

   chdir("/");
   snprintf(tmp,SIZE,"/bin/rm -rf %s",tmpdirname);
   runcommand(0,tmp);  /* it might work, it might not.  we don't care */
   rmdir(tmpdirname);
   if (tmpdirname != NULL) free(tmpdirname);  /* tempnam calls malloc */
   return(0);
   /* NOTREACHED */
fail:
addfail:
   chdir("/");
   if (tmpdirname != NULL) {
      snprintf(tmp,SIZE,"/bin/rm -rf %s",tmpdirname);
      runcommand(0,tmp);  /* it might work, it might not.  we don't care */
      rmdir(tmpdirname);
      free(tmpdirname);  /* tempnam calls malloc */
      tmpdirname=NULL;
   }
   return(-1);
}

/*
 * Create netcfg element script if it doesn't exist
 * For third party (ODI and DLPI) driver compatibility (what else :-)
 */
int
CreateNetcfgScript(char *path)
{
   struct stat sb;
   int fd;

#if 0
   if ((stat(path, &sb)) < 0) {
      if (errno == ENOENT) {
#endif
         if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0755)) == NULL) {
            error(NOTBCFG,"CNS couldn't open %s for writing, errno=%d", 
                          path, errno);
            return(-1);
         }
         /* FYI: /sbin/sh is statically linked, while /bin/sh (really 
          * /usr/bin/sh) uses the /usr/lib/libc.so.1 shared library.  The 
          * primary (but not sole) reason for having a separate, 
          * statically-linked version of the shell in /sbin is so that a 
          * shell is available prior to mounting of /usr.  
          * So use #!/sbin/sh to help exec(2) of this script succeed so
          * exec won't have to fall back on libc for help.
          */
         write(fd, "#!/sbin/sh\n",11);
         write(fd, "exit 0\n", 7);
         close(fd);
#if 0
      } else {
         error(NOTBCFG,"CNS stat(%s) failed, errno=%d", path, errno);
         return(-1);
      }
   }
#endif
   return(0);
}

/* create control file for use by netcfg */
int
CreateNetcfgControlScript(char *path, char *netinfoname)
{
   struct stat sb;
   int fd;
   char tmp[SIZE];

   if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0755)) == NULL) {
      error(NOTBCFG,"CreateNetcfgControlScript: couldn't open %s "
                    "for writing, errno=%d", path, errno);
      return(-1);
   }
   /* /sbin/sh is statically linked, so it doesn't need
    * /usr/lib/libc.so.1, which may not be there yet if /usr is a
    * separate filesystem and it hasn't been mounted yet (we're in single
    * user mode).
    */
   write(fd, "#!/sbin/sh\n",11);
         /*   123456789012345678901234567890123456789012345678901234567890 */
   write(fd, "# this file created automatically by ndcfg.\n", 44);
   write(fd, "/sbin/tfadmin -t NETCFG: /etc/nd >/dev/null 2>&1\n",49);
   write(fd, "if [ $? -eq 0 ]; then\n",22);
   snprintf(tmp,SIZE,"[ \"$5\" = \"Y\" ] && /sbin/tfadmin NETCFG: /etc/nd "
             "$6 %s > /dev/null 2>&1\n", netinfoname);
   write(fd, tmp, strlen(tmp));
   write(fd, "else\n",5);
   snprintf(tmp,SIZE,"[ \"$5\" = \"Y\" ] && /etc/nd $6 %s "
                     "> /dev/null 2>&1\n", netinfoname);
   write(fd, tmp, strlen(tmp));
   write(fd, "fi\n",3);
    
   /* send success from ndcfg to netcfg -- even if ndcfg had problems
    * It's not as if we could do any better next time or the user
    * could fix things...
    */
   write(fd, "exit 0\n", 7);
   close(fd);
   return(0);
}

/*
 * Create netcfg removal element script if it doesn't exist
 * For third party (ODI and DLPI) driver compatibility (what else :-)
 * based on CreateNetcfgScript.
 * Note if two people are running netcfg (not possible currently) there
 * is a race condition in showtopo/resshowunclaimed since they return
 * the next netX or next device to use.  If we are removing one and
 * they are creating the next device to use we have assorted problems.
 * again, only an issue if we ever allow multiple netcfg binaries at once.
 */
int
CreateNetcfgRemovalScript(char *path, char *netinfoname, 
                          u_int failover, u_int lanwan, int netX,
                          char *real_topology)
{
   struct stat sb;
   int fd;
   char tmp[SIZE];

   if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0755)) == NULL) {
      error(NOTBCFG,"CreateNetcfgRemovalScript: couldn't open %s "
                    "for writing, errno=%d", path, errno);
      return(-1);
   }
   /* /sbin/sh is statically linked, so it doesn't need 
    * /usr/lib/libc.so.1, which may not be there yet if /usr is a 
    * separate filesystem and it hasn't been mounted yet (we're in single
    * user mode).
    */
   write(fd, "#!/sbin/sh\n",11);  
         /*   123456789012345678901234567890123456789012345678901234567890123 */
   write(fd, "# this file created automatically by ndcfg.\n", 44);

   write(fd, "/sbin/tfadmin -t NETCFG: /etc/nd >/dev/null 2>&1\n",49);
   write(fd, "if [ $? -eq 0 ]; then\n",22);
   snprintf(tmp,SIZE,"[ \"$5\" = \"Y\" ] && /sbin/tfadmin NETCFG: /etc/nd "
             "stop %s > /dev/null 2>&1\n", netinfoname);
   write(fd, tmp, strlen(tmp));
   write(fd, "else\n",5);
   snprintf(tmp,SIZE,"[ \"$5\" = \"Y\" ] && /etc/nd stop %s "
                     "> /dev/null 2>&1\n", netinfoname);
   write(fd, tmp, strlen(tmp));
   write(fd, "fi\n",3);

   write(fd, "/sbin/tfadmin -t NETCFG: /usr/lib/netcfg/bin/ndcfg "
             ">/dev/null 2>&1\n",67);
   write(fd, "if [ $? -eq 0 ]; then\n",22);
   /* only remove driver when it isn't referenced by another chain */
   snprintf(tmp,SIZE,"[ \"$5\" = \"Y\" ] && /bin/echo idremove %s %d | "
             "/sbin/tfadmin NETCFG: /usr/lib/netcfg/bin/ndcfg "
             ">/dev/null 2>&1\n", netinfoname, failover); 
   write(fd, tmp, strlen(tmp));
   write(fd, "else\n",5);
   snprintf(tmp,SIZE,"[ \"$5\" = \"Y\" ] && /bin/echo idremove %s %d | "
                  "/usr/lib/netcfg/bin/ndcfg >/dev/null 2>&1\n", 
                  netinfoname, failover); 
   write(fd, tmp, strlen(tmp));
   write(fd, "fi\n",3);

   /* for now we only key off of ISDN.  As other WAN entitites (X.25) 
    * materialize then we'll special case them below too but for now 
    * ISDN is the only thing that wants the extra IncomingOutgoing stuff
    * note that we only call netinfo -a on LAN - should this change too?
    */
   if (strcmp(real_topology,"ISDN") != 0) {  /* will be in upper case */
      /* send success from ndcfg to netcfg -- even if ndcfg had problems
       * It's not as if we could do any better next time or the user
       * could fix things...
       */ 
      write(fd, "exit 0\n", 7); 
   } else {
            /*   123456789012345678901234567890123456789012345678901234 */
      write(fd, "/sbin/tfadmin -t NETCFG: /bin/osavtcl >/dev/null 2>&1\n",54);
      write(fd, "if [ $? -eq 0 ]; then\n",22);
      write(fd, "exec /sbin/tfadmin NETCFG: /bin/osavtcl -c '\n",45);
      write(fd, "loadlibindex /usr/lib/sysadm.tlib\n",34);
      write(fd, "set NCFG_DIR /usr/lib/netcfg\n",29);
      write(fd, "source $NCFG_DIR/lib/libSCO.tcl\n",32);
      snprintf(tmp,SIZE,"RemoveIncomingOutgoing \"net%d\"\n",netX);
      write(fd, tmp, strlen(tmp));
      write(fd, "'\n",2);
      write(fd, "else\n",5);
      write(fd, "exec /bin/osavtcl -c '\n",23);
      write(fd, "loadlibindex /usr/lib/sysadm.tlib\n",34);
      write(fd, "set NCFG_DIR /usr/lib/netcfg\n",29);
      write(fd, "source $NCFG_DIR/lib/libSCO.tcl\n",32);
      snprintf(tmp,SIZE,"RemoveIncomingOutgoing \"net%d\"\n",netX);
      write(fd, tmp, strlen(tmp));
      write(fd, "'\n",2);
      write(fd, "fi\n",3);
      /* send failure to netcfg if shell exec failed
       * file probably doesn't exist
       */ 
      write(fd, "exit 1\n",7); /* shouldn't ever be reached but just in case */
   }
   close(fd);
   return(0);
}


/*
 * Returns memsize in bytes
 */
#ifdef _SC_TOTAL_MEMORY
memsize_t
getmemsz(void)
{
   long npages;

   npages = sysconf(_SC_TOTAL_MEMORY);
   return(npages * sysconf(_SC_PAGE_SIZE));
}
#else
long
getmemsz(void)
{
   return(sysi86(SI86MEM));
}
#endif

/*
 *    ODI: Count up how many entries in resmgr have ODIMEM set to true and
 *     if there are any then we must do the following:
 *       adjust with idtune: ODIMEM_NUMBUF, ODIMEM_BUFSIZE, 
 *       ODIMEM_MBLK_NUMBUF 
 *     rebuild kernel with idbuild -B right now if params changed.
 *    GOOD NEWS: We don't have to touch ODIMEM_BUFSIZE!
 * ODI odimem/ODIMEM IDTUNE's are dealt with here
 *
 * this routine pretty much follows UW2.1 niccfg function do_odimem.
 *
 * This routine should not be called when netisl is 1 because
 * - odimem is likely still dynamic and will not have the $static in it.
 * - netisl flavour of the odimem DLM assumes Mtune defaults which is
 *   sufficient for netisl purposes.
 * 
 * returns -1 if error
 *          0 if you don't need to relink your kernel
 *          1 if we ran idtune command and we must relink kernel.
 *
 * note that the UW21 usr/src/i386at/pkg/nics/postinstall script explictly sets 
 * buffers to 0:
 *   # set the number of ODIMEM buffers to 0
 *   /etc/conf/bin/idtune ODIMEM_NUMBUF 0
 *   /etc/conf/bin/idtune ODIMEM_MBLK_NUMBUF 0
 *
 */
int
DoODIMEMTune(void)
{
   char cmd[SIZE];
   u_int status,odimem_buf,odimem_mblk_buf;
   u_int odimem_numbuf_curr,odimem_mblk_numbuf_curr;
#ifdef _SC_TOTAL_MEMORY
   memsize_t mem_size;
#else
   long mem_size;
#endif
   int odimem_cnt=ResODIMEMCount();

   if (odimem_cnt < 0) {
      error(NOTBCFG,"DoODIMEMTune: error in ResODIMEMCount");
      return(-1);  /* some kind of error in ResODIMEMCount */
   }
   if (odimem_cnt > 0) {
      /* mem_size=sysi86(SI86MEM);  obsolete with DDI8 */
      mem_size=getmemsz();

      /* get the current values for ODIMEM_NUMBUF and ODIMEM_MBLK_NUMBUF */
      snprintf(cmd,SIZE,"/etc/conf/bin/idtune -gc ODIMEM_NUMBUF |"
                  "/usr/bin/cut -f1 -d\"\t\"");
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"DoODIMEMTune: cannot get current ODIMEM_NUMBUF "
               "parameter from idtune");
         return(-1);
      }
      odimem_numbuf_curr=atoi(cmdoutput);

      snprintf(cmd,SIZE,"/etc/conf/bin/idtune -gc ODIMEM_MBLK_NUMBUF |"
                  "/usr/bin/cut -f1 -d\"\t\"");
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"DoODIMEMTune: cannot get current ODIMEM_MBLK_NUMBUF "
               "parameter from idtune");
         return(-1);
      }
      odimem_mblk_numbuf_curr=atoi(cmdoutput);

      /* before setting ODIMEM_BUFSIZE, we must make sure that there is enough
       * memory:
       * 16 Meg (16384000) can only handle 1 set of buffers
       * 24 Meg (24576000) can only handle 2 set of buffers
       * 28 Meg (28672000) can only handle 3 set of buffers
       * more than 28 Meg  can handle 4 buffers which is the upper bound
       */
      if (mem_size <= 16384000) {
         odimem_cnt=1;
      } else if (mem_size <= 24576000) {
         if (odimem_cnt > 2) odimem_cnt=2;
      } else if (mem_size <= 28672000) {
         if (odimem_cnt > 3) odimem_cnt=3;
      } else {  /* more than 28 megs of ram */
         if (odimem_cnt > 4) odimem_cnt=4;
      }
      
      odimem_buf=odimem_cnt * 64;
      odimem_mblk_buf=odimem_cnt * 64;

      /* if the tunables are already the same as the existing values,
       * then just return since nothing needs to be rebuilt
       */

      if ((odimem_buf == odimem_numbuf_curr) &&
          (odimem_mblk_buf == odimem_mblk_numbuf_curr)) {
         return(0);
      }

      /* set the tunables */

      snprintf(cmd,SIZE,"/etc/conf/bin/idtune -f ODIMEM_NUMBUF %d",odimem_buf);
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"DoODIMEMTune: couldn't idtune ODIMEM_NUMBUF parameter");
         return(-1);
      }

      snprintf(cmd,SIZE,"/etc/conf/bin/idtune -f ODIMEM_MBLK_NUMBUF %d",
                  odimem_mblk_buf);
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"DoODIMEMTune: couldn't idtune ODIMEM_MBLK_NUMBUF "
                       "parameter");
         return(-1);
      }

      return(1);  /* we must do a full kernel relink for new odimem buffers */
   }
   return(0);
}

/* returns -1 for failure
 *          0 for success  - no need to modify fullrelink
 *          1 for success  - set fullrelink to 1 too
 *
 *     HANDLE ODIMEM SPECIAL:
 *     (see usr/src/i386at/pkg/nics/postinstall for details)
 *      - if not in kernel already OR not static in kernel, then 
 *        add '$static' to System file.  can't be last line - make first one.
 *        idinstall -[au] odimem in kernel.
 *     do NOT add odimem to DLM link line -- it must stay static.
 *     WHY? odimem comes off the distribution as a DLM for NetInstall.
 *      - however in postinstall the System file gets $static added to it.
 *        and from then on it's static to the kernel for future idtunes.
 */
int
EnsureODIMEMstatic(void)
{
   char cmd[SIZE];
   int status;
   struct stat sb;

   if (stat("/etc/conf/sdevice.d/odimem",&sb) == -1) {
      error(NOTBCFG,"EnsureODIMEMstatic:can't find /etc/conf/sdevice.d/odimem");
      return(-1);
   }
   if (!S_ISREG(sb.st_mode)) {
      error(NOTBCFG,"EnsureODIMEMstatic:/etc/conf/sdevice.d/odimem not a file");
      return(-1);
   }
   snprintf(cmd,SIZE,
            "/usr/bin/grep -q '^\\$static' /etc/conf/sdevice.d/odimem");
   status=runcommand(0,cmd);  /* we care about return status ourself here */
   if (status == 0) {
      return(0);  /* nothing to do */
   }
   if (status == -1) {
      error(NOTBCFG,"EnsureODIMEMstatic: problem running '%s'",cmd);
      return(-1);
   }
   
   /* too bad you can't just append $static to the end of the file :-( */
   snprintf(cmd,SIZE,"/usr/bin/ed -s /etc/conf/sdevice.d/odimem <<!\n"
               "/^odimem\n"
               "i\n"
               "\\$static\n"
               ".\n"
               "w\n"
               "q\n"
               "!\n");
   status=docmd(0,cmd);
   return(1);   /* must set fullrelink later on */
}

/* make sure lsl file doesn't have a $depend odimem in it 
 * odimem does have the $depend line in it for netisl purposes but
 * on a running system it should not.
 *
 * returns -1 for failure
 *          0 for success
 *         >0 for command failure
 *
 *     HANDLE LSL SPECIAL:
 *     (see usr/src/i386at/pkg/nics/postinstall for details)
 *      - if not in kernel already OR Master file has text "$depend odimem"
 *        then remove "$depend odimem" and idinstall driver -[au] in kernel.
 *     you SHOULD add lsl to the DLM link line.
 *     WHY? lsl comes off distribution with the $depend odimem line for DLM
 *     NetInstall purposes but this should be removed as odimem will always
 *     be static and in base kernel.
 */
int
EnsureLSLnodepend(void)
{
   char cmd[SIZE];
   int status;
   struct stat sb;

   if (stat("/etc/conf/mdevice.d/lsl",&sb) == -1) {
      error(NOTBCFG,"EnsureLSLnodepend:can't find /etc/conf/mdevice.d/lsl");
      return(-1);
   }
   if (!S_ISREG(sb.st_mode)) {
      error(NOTBCFG,"EnsureLSLnodepend:/etc/conf/mdevice.d/lsl not reg file");
      return(-1);
   }
   snprintf(cmd,SIZE,"/usr/bin/grep -q '^\\$depend odimem' "
               "/etc/conf/mdevice.d/lsl");
   status=runcommand(0,cmd);    /* we care about exit statue ourself */
   if (status == 1) {   /* not found */
      return(0);  /* nothing to do */
   }
   if (status == -1) {
      error(NOTBCFG,"EnsureLSLnodepend: problem running cmd '%s'",cmd);
      return(-1);
   }
   if (status != 0) {
      error(NOTBCFG,"EnsureLSLnodepend: grep returned status %d",status);
      return(-1);
   }
   /* darn, line was found in lsl Master file.  we must remove it */
   
   snprintf(cmd,SIZE,"/usr/bin/ed -s /etc/conf/mdevice.d/lsl <<!\n"
               "/^\\$depend odimem\n"
               "d\n"
               "w\n"
               "q\n"
               "!\n");
   status=docmd(0,cmd);
   return(status);
}

/* this routine handles misc stuff common to ALL drivers to add them into
 * the kernel.  It is called on FIRST, SECOND, THIRD, etc. instance of
 * driver.   returns 0 for success.  This routine must handle:
 * - write out new netcfg info file /usr/lib/netcfg/info/netX.
 * XXX If POST_SCRIPT= set then run it on last deletion of driver at end
 * - check TYPE:
 -     if ODI recalculate ODIMEM for idtune
 * -   if MDI do netX instance stuff in dlpimdi file, netX space.c-DO MANY TIMES
 * -   if MDI idcheck/idinstall the dlpi module if not already there- MANY TIMES
 * - modadmin -U modnames that we're about to do an idbuild -M with.
 * - do the big idbuild -M with all ODI/DLPI/MDI drivers to generate 
 *   DLM's as necessary.  have a common array with all -M foo -M bar drivers
 * - do a bunch of modadmin -l's to load the just-generated drivers
 *   however we must check to see if one instance of module already loaded
 *   and if so then don't modadmin for multiple instances
 * - do_configure:
 *    run /etc/conf/bin/idconfupdate
 *    
 * - run installf -f nics to finish updating the contents file for package nics
 * + try and open(2) the device based on TYPE:
 *   MDI:/dev/mdi/<modname><instance -1>
 *   DLPI:/dev/<modname>_<instance number -1>
 *   ODI:/dev/<modname>_<instance number -1>
 *   we don't do this because it may not work if REBOOT=true is set or
 *   a complete relink is required, etc.
 * - add lines to /etc/slink.d/05dlpi
 * - run /etc/confnet.d/configure or add lines to /etc/slink.d/05dlpi file
 * - run netinfo(1M) -a -d /dev/whatever for higher level protocols
 * - ODI odimem/ODIMEM IDTUNE[]'s are dealt with here
 * - ODI odimem System file checking dealt with here
 * - ODI lsl Master file checking dealt with here
 *
 * XXX if bcfg has any IDTUNE[] values in them then set them.
 */

int
DoRemainingStuff(int bcfgindex, rm_key_t rmkey, int netX, 
                 int *fullrelink, u_int failover, u_int lanwan, 
                 int *removedlpimdi, char *devicename, int *removenetX,
                 int *callednetinfo, char *netinfoname, 
                 char *element, char **tunes, int *numtunes,
                 char *real_topology, int boardnumber)
{
   char tmp[SIZE];
   char key[20];
   FILE *fp;
   char *driver_name;
   char *name;
   char *type;
   char *conformance;
   char *actual_send_speed;
   char *actual_receive_speed;
   char *bus;
   int tmpret;
   int speed=0;
   int fullrelinktmp;
   int type_as_num=0;
   int numfields;
   int pseudo_unit;  /* not a true unit for MDI drivers */
   char pseudo_name[50];
   char device[50];
   char devdevice[50];
   struct stat sb;

   Status(88, "Creating netcfg files");

   controlpath[0]='\0';
   driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
   name=StringListPrint(bcfgindex,N_NAME);
   type=StringListPrint(bcfgindex,N_TYPE);
   /* since conformance is a hex number it will be in lower case */
   conformance=StringListPrint(bcfgindex,N_CONFORMANCE);
   actual_send_speed=StringListPrint(bcfgindex,N_ACTUAL_SEND_SPEED);
   actual_receive_speed=StringListPrint(bcfgindex,N_ACTUAL_RECEIVE_SPEED);
   bus=StringListPrint(bcfgindex,N_BUS);

   /* these should exist, but just in case... */
   mkdir("/usr/lib/netcfg",0755);
   mkdir("/usr/lib/netcfg/info",0755);
   mkdir("/usr/lib/netcfg/init",0755);
   mkdir("/usr/lib/netcfg/remove",0755);
   mkdir("/usr/lib/netcfg/list",0755);
   mkdir("/usr/lib/netcfg/reconf",0755);
   mkdir("/usr/lib/netcfg/control",0755);

   strtoupper(type);
   strtoupper(bus);

   snprintf(key,20,"%d",rmkey);  /* for resput below */

   if ((driver_name == NULL) || (name == NULL) ||
       (type == NULL) || (conformance == NULL) || 
       (actual_send_speed == NULL) || (actual_receive_speed == NULL) ||
       (bus == NULL)) {
      error(NOTBCFG,"DoRemainingStuff:  mandatory parameter not defined");
      goto remainerr;
   }
   /* since we've created a key for our new board but not assigned
    * CM_MODNAME to it yet the count will be correct for generating
    * device names, which start at 0
    */

   if (strcmp(type,"ODI") == 0) {
      type_as_num=1;

      pseudo_unit = boardnumber;
      snprintf(pseudo_name,50,"%s",driver_name);
      snprintf(netinfoname,SIZE,"%s_%d",driver_name,boardnumber); /* not abs. */

      snprintf(device,50,"%s_%d",driver_name,boardnumber);
      snprintf(devdevice,50,"/dev/%s_%d",driver_name,boardnumber);
      snprintf(devicename,50,"/dev/%s_%d",driver_name,boardnumber);/*passed up*/
      snprintf(infopath,SIZE,"%s/%s", NETXINFOPATH,device);
      snprintf(initpath,SIZE,"%s/%s", NETXINITPATH, device);
      snprintf(removepath, SIZE, "%s/%s", NETXREMOVEPATH, device);
      snprintf(listpath, SIZE, "%s/%s", NETXLISTPATH, device);
      snprintf(reconfpath,SIZE, "%s/%s", NETXRECONFPATH, device);
      controlpath[0]='\0';   /* no control script for ODI drivers */

      /* write ODI_TOPOLOGY (now single valued) to resmgr for later idremove 
       * command 
       */
      snprintf(tmp,SIZE,"%s,s=%s",CM_ODI_TOPOLOGY,real_topology);
      if (resput(key,tmp,0)) {
         goto remainerr;
      }

      /* don't tweak odimem System file or do idtune stuff if netisl is 1 */
      if (netisl == 0) {
         /* Ensure odimem is $static.  It's not necessary to do this for 
          * adding each instance of an ODI driver but we want to make
          * certain it doesn't get changed
          * we know odimem is in the link kit by now
          */

         fullrelinktmp=EnsureODIMEMstatic();

         if (fullrelinktmp == -1) {
            error(NOTBCFG,"EnsureODIMEMstatic failed");
            goto remainerr;
         }

         /* don't modify *fullrelink unless we know for sure we must relink */
         if (fullrelinktmp == 1) {
            notice("setting fullrelink to 1 because odimem wasn't $static");
            *fullrelink = 1;
         }

         /* handle odimem idtune stuff which we must increase upon each
          * new instance of an ODIMEM=true driver
          */

         fullrelinktmp=DoODIMEMTune();

         if (fullrelinktmp == -1) {
            error(NOTBCFG,"DoRemainingStuff: DoODIMEMTune failed");
            goto remainerr;
         }

         /* don't modify *fullrelink unless we know for sure we must relink */
         if (fullrelinktmp == 1) {
            notice("settting fullrelink to 1 because we did odimem idtunes");
            *fullrelink = 1;
         }

         /* make sure lsl file doesn't have a $depend odimem in it 
          * don't call this routine if netisl is 1
          */
         if (EnsureLSLnodepend() != 0) {
            error(NOTBCFG,"EnsureLSLnodepend failed");
            goto remainerr;
         }

      }

      /* build NETCFG_ELEMENT to uniquely identify resmgr key.  we write
       * this to the resmgr at the end of idinstall()
       */
      snprintf(element,SIZE,"%s,s=%s",CM_NETCFG_ELEMENT,device);

   } else if (strcmp(type,"DLPI") == 0) {
      type_as_num=2;

      pseudo_unit = boardnumber;
      snprintf(pseudo_name,50,"%s",driver_name);
      snprintf(netinfoname,SIZE,"%s_%d",driver_name,boardnumber); /* not abs. */

      snprintf(device,50,"%s_%d",driver_name,boardnumber);
      snprintf(devdevice,50,"/dev/%s_%d",driver_name,boardnumber);
      snprintf(devicename,50,"/dev/%s_%d",driver_name,boardnumber);/*passed up*/
      snprintf(infopath,SIZE,"%s/%s", NETXINFOPATH,device);
      snprintf(initpath,SIZE, "%s/%s", NETXINITPATH, device);
      snprintf(removepath,SIZE, "%s/%s", NETXREMOVEPATH, device);
      snprintf(listpath,SIZE, "%s/%s", NETXLISTPATH, device);
      snprintf(reconfpath,SIZE, "%s/%s", NETXRECONFPATH, device);
      controlpath[0]='\0';    /* no control script for DLPI drivers */
      /* build NETCFG_ELEMENT to uniquely identify resmgr key.  we write
       * this to the resmgr at the end of idinstall()
       */
      snprintf(element,SIZE,"%s,s=%s",CM_NETCFG_ELEMENT,device);

      /* usr/src/i386/sysinst/desktop/menus/ii_do_netinst doesn't delete */
      if (DelAllVals(rmkey, CM_ODIMEM) != 0) goto remainerr;
      if (DelAllVals(rmkey, CM_ODI_TOPOLOGY) != 0) goto remainerr;

   } else if (strcmp(type,"MDI") == 0) {
      type_as_num=3;

      pseudo_unit=netX;  /* this is the lowest available, not a unit number */
      snprintf(pseudo_name,50,"net");
      snprintf(netinfoname,SIZE,"net%d",netX);  /* not abs. path */

      snprintf(device,50,"%s%d",driver_name,boardnumber);
      snprintf(devdevice,50,"/dev/mdi/%s%d",driver_name,boardnumber);
      snprintf(devicename,50,"/dev/mdi/%s%d",driver_name,boardnumber);

      /* build NETCFG_ELEMENT to uniquely identify resmgr key.  we write
       * this to the resmgr at the end of idinstall()
       */
      snprintf(element,SIZE,"%s,s=net%d",CM_NETCFG_ELEMENT,netX);

      snprintf(infopath,SIZE,"%s/net%d", NETXINFOPATH,netX);
      snprintf(initpath,SIZE, "%s/net%d", NETXINITPATH, netX);
      snprintf(removepath,SIZE, "%s/net%d", NETXREMOVEPATH, netX);
      snprintf(listpath,SIZE, "%s/net%d", NETXLISTPATH, netX);
      snprintf(reconfpath,SIZE, "%s/net%d", NETXRECONFPATH, netX);
      /* MDI drivers get a control script */
      snprintf(controlpath,SIZE, "%s/net%d", NETXCONTROLPATH, netX);

      snprintf(tmp,SIZE,"%s/dlpimdi",DLPIMDIDIR);

      if ((fp=fopen(tmp,"a")) == NULL) {
         error(NOTBCFG,"couldn't open %s for appending",tmp);
         goto remainerr;
      }
      /* dlpimdi file format.  backwards compatible.
       * ndstat and dlpid don't use count so we set it to zero.  Format:
       * netX:driver_name:Y/N:count:failover_driver_name:InFailOverMode?
       * the Y/N was for netboot support and indicated if this device
       * was the "master"  If 'Y' then we add
       *        dlpip /dev/$dlpi \"dlpiMajor,dlpiMinor\"
       * else add the text
       *        dlpip /dev/$dlpi /dev/mdi/$mdi
       * XXX if this netX entry already exists then we must bump up 
       * reference count instead of adding a duplicate line.
       * we shouldn't slam in a new line!
       * NOTE:  dlpimdi is only used for MDI drivers so ODI and DLPI
       *        won't have entries in this file.  
       * NOTE:  Only #$version 1(basically MDI) drivers can be failover 
       *        devices. so if failover
       *        is 1 then we should be editing an *existing* line in
       *        dlpimdi and not add any lines to strcf for it. XXX Do this
       *        someday when implementing failover support.
       *        For now we leave this field blank.
       * XXX OPEN ISSUE:  If failover is 1 exactly *which* line in the
       *        dlpimdi file do we add our field to?  The last one?
       *        SUGGESTION: change failover argument to be either 0
       *        or the *name* of the field to update.
       */
      if (failover == 1) {
         /* see comment above */
         notice("DoRemainingStuff: failover support doesn't exist yet");
      }
      fprintf(fp,"net%d:%s%d:N:0::N\n",netX,driver_name,boardnumber);
      snprintf(tmp,SIZE,"%s,s=net%d",CM_MDI_NETX,netX);
      if (resput(key,tmp,0)) {
         fclose(fp);
         *removedlpimdi=1;
         goto remainerr;
      }
      fclose(fp);
   
      /* indicate that from here forward if we encounter an error
       * that we need to remove the line from the dlpimdi file
       * that we just added.  XXX NOTE:  set this for failover=1 too!
       * the removal of lines from dlpimdi will take place at the very end
       * of the idinstall() routine code 
       */
      *removedlpimdi=1;

      if (RealDLPIexists() != 1) {
         /* we shouldn't get here.  The routine DoExtraDSPStuff should have
          * added the DLPI module into the kernel if it wasn't already
          * present.  Downgraded from "Major problem" to "Slight problem"
          */
         notice("Slight problem.  The real dlpi module doesn't exist in the\n"
            "link kit, required for this MDI driver.  I'll go ahead and add\n"
            "it now.");
         if (AddDLPIToLinkKit(1, -1, NULL, NULL) != 0) goto remainerr;
      }
      /*
       * We always add another dummy netX driver, since this routine is
       * called for each add of each instance of an MDI driver
       * We also pass in any idtunes necessary on this netX driver here
       */
      if (AddDLPIToLinkKit(0, netX, tunes, numtunes) != 0) goto remainerr;

      /* remember that we added the netX driver to the link kit so that
       * if we encounter an error later on we should remove it so that
       * we don't keep this netX in the link kit as unused.  Will eventually
       * lead to problems with net0, net1, net2, etc. clogging up the link
       * kit as the user tries again and again to fix whatever problem 
       * occurred.
       */

      *removenetX = 1;

      /* usr/src/i386/sysinst/desktop/menus/ii_do_netinst doesn't delete */
      if (DelAllVals(rmkey, CM_ODIMEM) != 0) goto remainerr;
      if (DelAllVals(rmkey, CM_ODI_TOPOLOGY) != 0) goto remainerr;

      /* MDI drivers had better not rely on old UW2.1 crufty ODISTRx params
       * which may still be there from ISL.  Note that this completely 
       * prevents MDI drivers from using this name in their bcfg file too
       * since we will be deleting it now even though we added custom parameters
       * to the resmgr already! 
       * see i386at/uts/io/odi/lsl/lslcm.c, it reads from ODISTR1 to ODISTR16
       * could use a loop with snprintf here
       */
      if (DelAllVals(rmkey, "ODISTR1") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR2") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR3") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR4") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR5") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR6") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR7") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR8") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR9") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR10") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR11") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR12") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR13") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR14") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR15") != 0) goto remainerr;
      if (DelAllVals(rmkey, "ODISTR16") != 0) goto remainerr;

      /* end of MDI specific stuff */
   } else {
      if (type == NULL) {
         error(NOTBCFG,"DoRemainingStuff: TYPE isn't set");
         goto remainerr;
      }
      error(NOTBCFG,"DoRemainingStuff: unknown TYPE=%s",type);
      goto remainerr;
   }

      /* COMMON POINT CONTINUES HERE */

   /* write type (ODI/DLPI/MDI) to resmgr for later idremove command */
   snprintf(tmp,SIZE,"%s,s=%s",CM_DRIVER_TYPE,type);
   if (resput(key,tmp,0)) goto remainerr;

   /* write netinfo device name to resmgr for later idremove command
    * this is the name of the driver in /dev and not an absolute pathname
    * like /dev/whatever.  See netdrivers(4)
    */
   snprintf(tmp,SIZE,"%s,s=%s",CM_NETINFO_DEVICE,netinfoname);
   if (resput(key,tmp,0)) goto remainerr;

   /* write out new netcfg info file /usr/lib/netcfg/info/<element> */
   if ((fp=fopen(infopath,"w")) == NULL) {
      error(NOTBCFG,"couldn't open %s for writing, errno=%d",infopath,errno);
      goto remainerr;
   }
   fprintf(fp,"NAME=\"%s\"\n",device);
   /* write DEV_NAME to the resmgr for later idremove command */
   fprintf(fp,"DEV_NAME=\"%s\"\n",devdevice);

   /* NOTE: ResmgrGetLowestBoardNumber relies on the DEV_NAME parameter
    * being correct.  Don't delete this.
    */
   snprintf(tmp,SIZE,"%s,s=%s",CM_DEV_NAME,devdevice);
   if (resput(key,tmp,0)) {
      fclose(fp);
      goto remainerr;
   }

   fprintf(fp,"DESCRIPTION=\"%s\"\n",name);
   /* since the OTHER topology covers both LAN and WAN, we need
    * to know how this device is being installed as.  We don't inherently
    * know from the topology name alone if OTHER is being used for LAN or WAN 
    * purposes (although we do for all of the other topologies).  Hence the
    * need for a new argument to the idinstall command to tell how this
    * is being used.   Plus, since topology is multi valued, a driver
    * could set TOPOLOGY to "ETHER ISDN", in which case we'd need this
    * support anyway.  Note that real_topology is *not* multivalued and
    * represents the exact topology that this board is being installed at
    */
   if (lanwan == LAN) {
      fprintf(fp,"UP=\"dlpi\"\n");
   } else if (lanwan == WAN) {
      fprintf(fp,"UP=\"\"\n");
   } else {
      notice("idinstall: unknown lanwan argument %d",lanwan);
      fprintf(fp,"UP=\"unknown_%d\"\n",lanwan);
   }
   fprintf(fp,"DOWN=\"\"\n");
   fprintf(fp,"NETWORK_MEDIA=\"%s\"\n", real_topology); /* now single valued */
   /* there are rumours that FDDI supports xns but we'll quell those here */
   if (strcmp(real_topology, "ETHER") == 0) {
      fprintf(fp,"NETWORK_FRAME_FMTS=\"%s\"\n","ethernet-II xns llc1 snap");
   } else {
      /* really should check other topologies too */
      fprintf(fp,"NETWORK_FRAME_FMTS=\"%s\"\n","llc1 snap");
   }
   fprintf(fp,"CONFORMANCE=\"%s\"\n",conformance);
   fprintf(fp,"%s_VERSION=\"%s\"\n",type,conformance);

   /* determine speed.  We'll (mostly) emulate OpenServer scheme for now.
    * Specifically:
    * - for a purely programmed I/O ISA card (no DMA, no MEMADDR) set 
    *   speed to 10 ("very_slow").   This is for the e3A/3Com 3C501 card. 
    * - for an ISA card that uses either DMA or MEMADDR, set 
    *   speed to 20 (slow).   Purely memory mapped ISA boards go here.
    * - for an EISA/MCA card, set speed to 30.
    * - for a PCI card, set speed to 40.
    *
    * This isn't accurate by any means, since even 16 bit ISA boards can
    * saturate 10Mbps Ethernet, but provides a relative means for gauging
    * performance until ACTUAL_SEND_SPEED/ACTUAL_RECEIVE_SPEED benchmarks
    * are in place.  There are also some 100Mbit EISA cards and plenty of
    * 10Mbit PCI cards but as I said this isn't a guarantee of anything.
    * 
    * As a generalization, PCI boards are more likely to be 33/66Mhz
    * 100Mbit devices these days. 
    */
   switch(bcfgbustype(bus)) {

      case CM_BUS_ISA:
           /* HasString returns -1 if parameter never defined in .bcfg file */
           if ((HasString(bcfgindex, N_DMA, "__JuNkjUnk", 0) == -1) &&
               (HasString(bcfgindex, N_MEM, "__jUnKJuNk", 0) == -1)) {
              speed=20;
           } else {
              speed=25;
           }
           break;

      case CM_BUS_PCMCIA:
      case CM_BUS_PNPISA:
           speed=25;
           break;

      case CM_BUS_EISA:
      case CM_BUS_MCA:
           speed=30;
           break;

      case CM_BUS_PCI:
           speed=40;
           break;

#ifdef CM_BUS_I2O
      case CM_BUS_I2O:
           speed=50;   /* hey, with an i960 IOP we can fly */
           break;
#endif

      case CM_BUS_UNK:
           /* shouldn't happen but if it does keep speed at 0 */
           break;

      default:
           notice("DoRemainingStuff: unknown return from bcfgbustype");
           /* shouldn't happen but if it does keep speed at 0 */
           break;

   }
   fprintf(fp,"SEND_SPEED=\"%d\"\n",speed);
   fprintf(fp,"RECEIVE_SPEED=\"%d\"\n",speed);

   /* eventually we'll have a benchmark to set this number.  Not yet. */
   fprintf(fp,"ACTUAL_SEND_SPEED=\"%s\"\n",actual_send_speed);
   fprintf(fp,"ACTUAL_RECEIVE_SPEED=\"%s\"\n",actual_receive_speed);
   fprintf(fp,"DRIVER_TYPE=\"%s\"\n",type);
   fprintf(fp,"BUS=\"%s\"\n",bus);
   /* you can't put the resmgr key into the info file because it's misleading.
    * if any autodetected boards a removed then that will skew the
    * resmgr database so later attempts to use the key will fail.
    * rather than give people the impression that it's going to stay valid
    * we comment it out.  use the showinstalled command.
    * fprintf(fp,"RESMGR_KEY=\"%d\"\n",rmkey);
    */
   /* putting BCFGINDEX into the info file is misleading.  It represents
    * the current offset into bcfgfile[] for _this_ installation and is
    * only correct if no additional files were loaded with loaddir or loadfile
    * commands *AND* nobody deletes or adds any further bcfg files or drivers
    * down in the default directories loaded in main:
    *       LoadDirHierarchy("/etc/inst/nics/drivers");
    *       LoadDirHierarchy("/etc/inst/nd/mdi");
    * In short, BCFGINDEX will be wrong if:
    * - if you do a loaddir of a further hierarchy and an idinstall of an
    *   index from that hierarchy then bcfgindex will be invalid next time
    *   around (since you haven't done the loaddir command yet).
    * - if you add or delete any drivers in the default dirs above then 
    *   the next time you run ndcfg the bcfgindex will be wrong, skewed at
    *   some point by the amount of bcfg files added or deleted..
    * 
    * Since this second point is quite likely upon adding any additional
    * drivers via IHV disks, we must put all CUSTOM[] information into
    * the resmgr.  We also don't put BCFGINDEX into the info file for
    * fear of confusing people into thinking it's never going to change.
    * use the showinstalled command which handles this properly.
    *           fprintf(fp,"BCFGINDEX=\"%d\"\n",bcfgindex);
    */
   /* we put BCFGPATH into the resmgr at key rmkey for later modification */
   fprintf(fp,"%s=\"%s\"\n",CM_BCFGPATH,bcfgfile[bcfgindex].location);

   /* INTERFACE_NUMBER is _NOT_ a unit for MDI drivers.  don't think that
    * it is.   It is sufficient to use in creating an interface string though.
    */
   fprintf(fp,"INTERFACE_NUMBER=\"%d\"\n",pseudo_unit);
   fprintf(fp,"INTERFACE_NAME=\"%s\"\n",pseudo_name);

   fclose(fp);

   if (CreateNetcfgScript(initpath) < 0) { 
      goto remainerr;
   }
   if (CreateNetcfgRemovalScript(removepath, netinfoname, 
                                 failover, lanwan, netX, real_topology) < 0) { 
      goto remainerr;
   }
   /* if smart bus and no customs and topology is not ISDN then don't
    * create reconf or list script.  netcfg says if files don't exist then
    * it should stipple(grey) out the corresponding menu choice.
    * don't use HasString since topology is multivalued - use real_topology
    */
   snprintf(tmp,SIZE,"%d",bcfgindex);
   tmpret=0;
   if (!((
       (HasString(bcfgindex, N_BUS, "PCI", 0) == 1)  ||
#ifdef CM_BUS_I2O
       (HasString(bcfgindex, N_BUS, "I2O", 0) == 1)  ||
#endif
       (HasString(bcfgindex, N_BUS, "EISA",0) == 1)  ||
       (HasString(bcfgindex, N_BUS, "MCA", 0) == 1)) &&
       ((tmpret=showcustomnum(0,tmp)) == 0) &&
       (strcmp(real_topology, "ISDN") != 0))) {
      if (CreateNetcfgScript(listpath) < 0) { 
         goto remainerr;
      }
      if (CreateNetcfgScript(reconfpath) < 0) { 
         goto remainerr;
      }
   }
   if (tmpret == -1) {    /* error in showcustomnum */
      error(NOTBCFG,"DoRemainingStuff: error in showcustomnum for script");
      goto remainerr;
   }
   /* if we're dealing with an MDI driver then create a control script for it */
   if (strlen(controlpath) > 0) {
      if (CreateNetcfgControlScript(controlpath, netinfoname) < 0) {
         goto remainerr;
      }
   }

   /* generially tell higher level stacks. don't care if cmd fails. 
    * Do this for all driver types.  For mdi, this is /dev/net0
    * and NOT /dev/mdi/whatever
    * Further inspection shows this should not be an absolute pathname
    * only add to /etc/confnet.d/netdrivers file if lan device
    */
   if (lanwan == LAN) {
      snprintf(tmp,SIZE,"/usr/sbin/netinfo -a -d %s",netinfoname);
      (void) runcommand(0,tmp);   /* we don't care if cmd succeeds or fails */
      *callednetinfo = 1;
   }

   if (driver_name != NULL) free(driver_name);
   if (name != NULL) free(name);
   if (type != NULL) free(type);
   if (conformance != NULL) free(conformance);
   if (actual_send_speed != NULL) free(actual_send_speed);
   if (actual_receive_speed != NULL) free(actual_receive_speed);
   if (bus != NULL) free(bus);
   return(0);   /* success */
   /* NOTREACHED */

remainerr:
   if (driver_name != NULL) free(driver_name);
   if (name != NULL) free(name);
   if (type != NULL) free(type);
   if (conformance != NULL) free(conformance);
   if (actual_send_speed != NULL) free(actual_send_speed);
   if (actual_receive_speed != NULL) free(actual_receive_speed);
   if (bus != NULL) free(bus);

   /* remove this key (ISA) or resmgr params associated with 
    * key (PCI/EISA/MCA)  moved next line into idinstall main code fail case
    * (void) ResmgrDelInfo(rmkey,1...);
    */
   return(-1);
}

/* this routine does remaining stuff necessary for ONLY THE FIRST instance of
 * driver into link kit.  Work that must be done for all drivers is done
 * elsewhere.  In particular, this routine must handle:
 * XXX - copying the EXTRA_FILES found in the bcfg file beyond msg.o/firmware.o
 *       the compaq netflex uses this.  note that idinstall will automatically
 *       add in msg.o and firmware.o from the DSP into the link kit so this
 *       is low priority  be sure and run installf -f nics on them!
 * - idinstalling companion DEPEND= drivers 
 * - check TYPE:
 *     If ODI idinstall -[au] msm ethtsm toktsm odisr fdditsm
 *     and add these to DLM link line.
 *     Also handle lsl and odimem special case stuff here
 *     if DLPI nothing else to add -- you're all done - ONLY DO ONCE
 *     if MDI base driver already added into link kit - ONLY DO ONCE
 * ODI odimem/ODIMEM IDTUNE's are dealt with elsewhere -- each instance of drvr
 */
int
DoExtraDSPStuff(u_int bcfgindex, rm_key_t rmkey, int netX, int *fullrelink,
                char *real_topology)
{
   int numdepend=StringListNumWords(bcfgindex, N_DEPEND);
   char *depend;
   int loop;
   char tmp[SIZE];
   char *dependX;
   int status;
   struct stat sb;

   dependX=NULL;
   if (numdepend > 0) {

      Status(84, "Installing dependent drivers");
      /* if we're doing netisl then we can only handle DLM's.  Right now
       * it's too much work to copy in all of the associated DLMs into
       * mod.d and idmodreg them etc etc. plus modadmin -u and -l issues too
       * Note that normal dependencies (odimem lsl dlpi) are handled elsewhere
       * and should not appear in the DEPEND= line.
       */
      if (netisl == 1) {
         error(NOTBCFG,"DEPEND= isn't supported for netisl=1");
         goto extrafail;
      }
      /* Sigh, we have further driver dependencies
       * We assume that the DEPEND= drivers will all reside in
       * /etc/inst/nics/drivers/<drivername>  and that the System file
       * will have a 'Y' in it so a simple idinstall is all it takes.
       * we also mandate that the driver be able to be turned into a DLM.
       */

      /* one of the DEPEND drivers could be $static in its Master file, so
       * we must set fullrelink so that we won't miss anything
       */

      notice("setting fullrelink to 1 because we encountered DEPEND drivers");
      *fullrelink = 1;

      for (loop=1; loop <= numdepend; loop++) {
         dependX=StringListWordX(bcfgindex,N_DEPEND,loop);
         if (dependX == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"DoExtraDSPStuff: StringListWordX returned NULL");
            goto extrafail;
         }
         snprintf(tmp,SIZE,"/etc/conf/bin/idcheck -y %s",dependX);
         /* because we care about the exit status we use runcommand not docmd */
         status=runcommand(0,tmp);
         if (status == 0) {
            /* darn.  driver isn't installed.  we must install it.
             * we pull ODI drivers' DEPEND files from /etc/inst/nics/drivers
             * we pull DLPI drivers' DEPEND files from /etc/inst/nics/drivers
             * we pull MDI drivers' DEPEND files from /etc/inst/nd/mdi
             */
            if (HasString(bcfgindex,N_TYPE,"ODI",0) == 1) {
               snprintf(tmp,SIZE,"%s/%s",HIERARCHY,dependX);
            } else if (HasString(bcfgindex,N_TYPE,"DLPI",0) == 1) {
               snprintf(tmp,SIZE,"%s/%s",HIERARCHY,dependX);
            } else if (HasString(bcfgindex,N_TYPE,"MDI",0) == 1) {
               snprintf(tmp,SIZE,"%s/mdi/%s",NDHIERARCHY,dependX);
            } else {
               error(NOTBCFG,"DoExtraDSPStuff: unknown driver type");
               free(dependX);
               goto extrafail;
            }
            status=idinstalldriver(tmp,dependX);
            if (status != 0) {
               error(NOTBCFG,"DoExtraDSPStuff: return due to earlier errors");
               free(dependX);
               goto extrafail;
            }
            AddToDLMLink(dependX);
         }
         free(dependX);
         if (status == -1) {
            error(NOTBCFG,"DoExtraDSPStuff: problem running cmd '%s'",tmp);
            goto extrafail;
         }
      }
   }
   /* ok, moving on to the big stuff: TYPE.  Note that #$version 0 drivers
    * have this set in GiveV0Defaults().
    */
   if (HasString(bcfgindex,N_TYPE,"ODI",0) == 1) {

      Status(85, "Installing ODI subsystem");

      if (strcmp(real_topology, "ETHER") == 0) {
         if (InstallOrUpdate(1,"ethtsm") != 0) goto extrafail;
      }
      
      /* odisr registers itself with lsl to watch the frames and include
       * (when necessary) any source routing information in the MAC header
       * its registration only affects the currently loaded drivers so
       * that's why it must be loaded last.  It is optional -- tsm checks
       * to see if source routing has registered itself with lsl before
       * calling its function
       * odisr is applicable to both toktsm and fdditsm.  niccfg didn't
       * load either automatically (although netinstall did load odisr last).
       * we load odisr for both.
       */
      if (strcmp(real_topology, "TOKEN") == 0) {
         if (InstallOrUpdate(1,"toktsm") != 0) goto extrafail;
         if (InstallOrUpdate(1,"odisr") != 0) goto extrafail;
      }

      /* odisr driver can handle both fddi and token ring media.  However, the
       * tokinit script (see nics prototype file for location) says
       * "if the fdditsm module is loaded, then the odisr module CANNOT be 
       * loaded".  Since we don't have any ODI FDDI cards to test, we'll just
       * install odisr into the link kit but not actually load it.  That is,
       * when /etc/init.d/tokinit runs it won't load odisr.  Note we hook the
       * tokinit file up to /etc/rc2.d/S79sr in the nics prototype file.
       */
      if (strcmp(real_topology, "FDDI") == 0) {
         if (InstallOrUpdate(1,"fdditsm") != 0) goto extrafail;
         if (InstallOrUpdate(1,"odisr") != 0) goto extrafail;
      }

      /* XXX fast implementation.  assume that postinstall script has
       * edited lsl's Master file and odimem's System file already.
       * we check for this elsewhere in this code
       */

      if (InstallOrUpdate(1,"msm") != 0) goto extrafail;
      if (InstallOrUpdate(1,"lsl") != 0) goto extrafail;
      /* note odimem may have been made into $static earlier 
       * at isl time by nics postinstall script if not netisl 
       * however if we are doing a netisl then the $static won't be 
       * present in the System file, which is good.
       * odimem can't be a DLM on non-isl for above reasons.
       * the reason why odimem must be $static is because it wants
       * to allocate big chunks of contiguous ram at init time so that
       * later calls to lsl_alloc will be able to grab them from the
       * odimem pool if necessary.
       */
      snprintf(tmp,SIZE,"/etc/conf/bin/idcheck -y odimem");
      status=runcommand(0,tmp);
      if (status == 0) {
         /* darn.  odimem isn't installed.  we must install it and do a 
          * full relink since it needs to be static to obtain and hog
          * contiguous memory.  sigh.
          */
         *fullrelink = 1;
         notice(
             "setting fullrelink to 1 because odimem didn't exist in link kit");
         if (InstallOrUpdate(0,"odimem") != 0) goto extrafail;
      }

   } else if (HasString(bcfgindex,N_TYPE,"DLPI",0) == 1) {
      /* nothing to do - we're all done */
   } else if (HasString(bcfgindex,N_TYPE,"MDI",0) == 1) {
      /* nothing much to do here - base driver already in link kit
       * from DoDSPStuff, but we need to ensure that the "real"
       * dlpi driver exists in the link kit since this is the first
       * instance of _a_ MDI driver.  Note that other MDI drivers
       * may have already been loaded so there's a good chance
       * that the dlpi driver has already been installed so RealDLPIexists()
       * will already return 1 here.
       * NOTE: The "dummy" dlpi driver (netX) is handled elsewhere
       * NOTE: updating the dlpimdi file is also handled elsewhere 
       */
      if (RealDLPIexists() != 1) { /* first ever MDI driver on system? */
         /* real DLPI doesn't exist, add it */

         Status(86, "Installing MDI subsystem");

         if (AddDLPIToLinkKit(1, -1, NULL, NULL) != 0) {
            error(NOTBCFG,"DoExtraDSPStuff: AddDLPIToLinKit failed");
            goto extrafail;
         }
      }
   } else {
      char *driver_type=StringListPrint(bcfgindex,N_TYPE);
      if (driver_type == NULL) {
         error(NOTBCFG,"DoExtraDSPStuff: TYPE isn't set");
         goto extrafail;
      }
      error(NOTBCFG,"DoExtraDSPStuff: unknown TYPE=%s",driver_type);
      free(driver_type);
      goto extrafail;
   }
   return(0);
   /* NOTREACHED */
extrafail:
   /* remove this key (ISA) or resmgr params associated with 
    * key (PCI/EISA/MCA)  moved next line into idinstall main code fail case
    * (void) ResmgrDelInfo(rmkey,1...);
    */
   if (dependX != NULL) free(dependX);
   return(-1);
}


/* returns the lowest unused available netX value for either
 * the real thing or for dummy purposes
 * returns -1 if error
 */
int
getlowestnetX(void)
{
   char tmp[SIZE];
   int loop;
   struct stat sb;
   int status;

   for (loop=0; loop<MAXNETX; loop++) {
      snprintf(tmp,SIZE,"/etc/conf/bin/idcheck -p net%d",loop);
      status=runcommand(0,tmp);
      /* 7 means no DSP in pack.d, no Master, and no System.  ignore the
       * last kernel built with driver and Driver.o part of DSP bits
       */
      if (status == -1) {
         error(NOTBCFG,"getlowestnetX: problem running cmd '%s'",tmp);
         return(-1);
      }
      if ((status & 7) == 0) {
         /* no part of this DSP exists in link kit.  safe to use this as netX.
          */
         return(loop);
      }
   }
   error(NOTBCFG, "getlowestnetX: reached %d without finding free slot; "
                  "returning failure", MAXNETX);
   return(-1);
}

/* rather than open up each DSP's Master file when starting ndcfg, we
 * move the check to the back-end -- when installing the driver
 * this means that if the user messes around with the Master file then
 * a subsequent add of the same driver could have different behaviour but
 * that's life
 */
int
determineddilevel(u_int bcfgindex, char *ddi, int numbytes)
{
   char tmppath[SIZE],line[SIZE],name[80],level[80];
   char *cp;
   FILE *fp;
   int found;

   strncpy(tmppath, bcfgfile[bcfgindex].location,SIZE-10);
   cp=strrchr(tmppath, '/');
   if (cp == NULL) {
      strcpy(tmppath,"Master");
   } else {
      *(cp+1)=NULL;
      strcat(tmppath,"Master");
   }

   if ((fp=fopen(tmppath,"r")) == NULL) {
      error(NOTBCFG,"deteremineddilevel: could not open %s",tmppath);
      return(-1);
   }

   found=0;
   while ((found == 0) && fgets(line, SIZE, fp) != NULL) {
      if ((line[0] == '$') &&
          (strlen(line) > 12) && 
          (strlen(line) < 80) &&
          (sscanf(line, "$interface %s %s\n",name,level) == 2) &&
          (strncmp(name,"ddi",3) == 0)) {
         /* XXX not entirely correct: getinst.c says that an interface line
          * can read $interface ddi 6 7 8 9  but in practice there is only
          * one level after the $interface ddi line.
          */
         if (strlen(level) > numbytes) {
            notice("determineddilevel: not enough bytes(%d) for %s",
                   numbytes, level);
         }
         strncpy(ddi,level,numbytes);
         found++;
      }
   }
   fclose(fp);

   if (found == 0) {
      error(NOTBCFG,"no $interface ddi line in %s",tmppath);
      return(-1);
   }
   return(0);
}

/* This is it.  The big honker.  idinstall the driver into the system
 * given a slew of arguments.  There are MANY ways that this routine
 * can fail -- many beyond my control -- so be sure and look at
 * the return value.
 * By the same reasoning, the possible return values can be
 * - success
 * - must reboot (from REBOOT=true in bcfg
 * IDINSTALL IDINSTALL IDINSTALL IDINSTALL IDINSTALL IDINSTALL IDINSTALL
 * returns 0 for success
 * returns -1 for error
 */
int 
idinstall(union primitives primitive)
{
   stringlist_t *slp;
   rm_key_t rmkey, highest, backupkey;
   char *keystart, *strend, *busstr, *keystr, *lineX, *tmpcp;
   u_int numargs, oldnetisl, bcfgindex, failover,bus,status,ec,boardnumber;
   int netX,numlines,tmpfd;   /* netX is -1 for ODI and DLPI */
   int delmodname=0, delbindcpu=0, delunit=0, delipl=0;
   int fullrelink=0,zz,tmpstatus,callednetinfo;
   u_int isainstall=0, loop,loop2,lanwan, hpsladded;
   char ddi[100];
   char dlpimdifile[SIZE];
   char netinfoname[SIZE];  /* device name we use with netinfo command */
   char element[SIZE];  /* element name for CM_NETCFG_ELEMENT */
   char devicename[50];
   char modname[VB_SIZE];
   char *files, *tmpmodname, *tmpmaxbd, *reboot;
   char *tmp, *irq, *dma, *mem, *port, *charm;
   char *oldirq, *olddma, *oldmem, *oldport;
   struct System System;
   char cmd[SIZE * 2];
   char foofoo[VB_SIZE];
   char *driver_name;
   char *real_topology;
   char rmkey_as_string[10];
   char *fullrelinkstr="N";
   extern int bflag;
   int removedlpimdi=0;
   int removenetX=0;
   int charmmode;
   struct stat sb;
   int numpatches=0;
   int do_rm_on_failure=0;
   int ok_to_ResmgrDelInfo=0;
   struct patch patch[10];  /* each custom parameter scope could be PATCH */
   char olddelim=delim;
   char *tunes[MAXNETXIDTUNES];  /* for NETX scope custom params */
   int numtunes=0;

   Status(80, "Starting installation of driver");

   delim='\1';   /* ASCII 1 = SOH, an unlikely character */

   /* if we ever do a nd start/nd stop/nd restart we don't want policeman mode
    * to hit us.  CheckResmgrAndDevs looks for this environment variable
    */
   putenv("__NDCFG_NOPOLICEMAN=1");
  
   bcfgindex = (u_int) -1;

   infopath[0]='\0';
   initpath[0]='\0';
   removepath[0]='\0';
   listpath[0]='\0';
   reconfpath[0]='\0';
   controlpath[0]='\0';

   callednetinfo=0;
   oldirq=olddma=oldmem=oldport=irq=dma=mem=port=NULL;
   rmkey=RM_KEY;
   backupkey=RM_KEY;
   reboot="N";
   DLM[0]='\0';  /* for the modules that will be be our idbuild -M line */
   modlist[0]='\0';   /* list of modules, space separated */
   devicename[0]='\0';
   if (primitive.type != STRINGLIST) {  /* programmer error from cmdparser.y */
      error(NOTBCFG,"idinstall: arg isn't a stringlist!");
      goto installfail;
   }

   if (ensureprivs() == -1) {
      error(NOTBCFG,
            "idinstall: insufficient priviledge to perform this command");
      goto installfail;
   }

   slp=&primitive.stringlist;

   numargs=CountStringListArgs(slp);
   if (numargs < 5) {   /* PCI/EISA/MCA board with no CUSTOM[x] arguments */
      error(NOTBCFG,"idinstall requires at least 5 arguments");
      goto installfail;
   }

   /* first argument is bcfgindex - avoid atoi */
   if (((bcfgindex=strtoul(slp->string,&strend,10))==0) &&
        (strend == slp->string)) {
      error(NOTBCFG,"invalid bcfgindex %s",slp->string);
      bcfgindex = (u_int) -1;
      goto installfail;
   }
   if (bcfgindex >= bcfgfileindex) {
      error(NOTBCFG,"bcfgindex %d >= maximum(%d)",bcfgindex,bcfgfileindex);
      bcfgindex = (u_int) -1;
      goto installfail;
   }
   
   /* now we have the .bcfg index so allow RM_ON_FAILURE */
   do_rm_on_failure = 1;

   slp=slp->next;
   /* second argument is actual topology (was oldnetisl)
    * when combined with fourth argument (lanwan) we know how this
    * device will be use.  Important as the .bcfg file may have a
    * multivalue TOPOLOGY which we need to sort out later
    */
   real_topology=slp->string;
   strtoupper(real_topology);
   if (HasString(bcfgindex,N_TOPOLOGY,real_topology,0) != 1) {
      /* this bcfg file doesn't support the desired topology the user wants */
      error(NOTBCFG,"bcfgindex %d doesn't support topology %s",
             bcfgindex, real_topology);
      bcfgindex = (u_int) -1;
      goto installfail;
   }

   slp=slp->next;
   /* third argument is failover - avoid atoi */

   /* NOTE: CreateNetcfgRemovalScript() assumes failover is an u_int yet
    * help[] for idinstall shows this as possibly being a string.  if
    * changed to a string be sure and change CreateNetcfgRemovalScript()
    */
   if (((failover=strtoul(slp->string,&strend,10))==0) &&
        (strend == slp->string)) {
      error(NOTBCFG,"invalid failover %s",slp->string);
      goto installfail;
   }
   if (failover < 0 || failover > 1) {
      error(NOTBCFG,"invalid failover %d (must be 0 or 1)",failover);
      goto installfail;
   }
   slp=slp->next;
   /* fourth argument is lanwan - avoid atoi */
   if (((lanwan=strtoul(slp->string,&strend,10)) == 0) &&
        (strend == slp->string)) {
      error(NOTBCFG,"invalid lanwan %s",slp->string);
      goto installfail;
   }
   if ((lanwan != LAN) && (lanwan != WAN)) {
      error(NOTBCFG,"lanwan must be %d(LAN) or %d(WAN)",LAN,WAN);
      goto installfail;
   }
   /* ensure topology for bcfgindex supports the desired topology */
   /* make sure the actual topology given as 2nd argument when combined with
    * lan/wan is valid in the topo array.
    */
   for (loop2=0;loop2<numtopos;loop2++) {
      if ((strcmp(real_topology,topo[loop2].name) == 0) && 
          /* here's why LAN and WAN defines can't start at zero: */
          ((topo[loop2].type & lanwan) > 0)) {
         goto goodtopo;
      }
   }

   /* bad real_topology and/or lanwan specified on command line */
   error(NOTBCFG,"bcfgindex '%d' installed using real topology '%s' doesn't "
                 "support lanwan of '%d'",bcfgindex, real_topology, lanwan);
   goto installfail;
goodtopo:

   slp=slp->next;

   /* netX is only for MDI drivers, not DLPI or ODI.*/
   if (HasString(bcfgindex,N_TYPE,"MDI",0) == 1) {
      /* Note that if two instances of this ndcfg binary are running
       * the value returned from getlowestnetX here can be different
       * than the getlowestnetX in the ShowTopo() routine if the other
       * ndcfg binary does an idinstall command before us!
       */
      if ((netX=getlowestnetX()) == -1) {
         error(NOTBCFG,"idinstall: error in getlowestnetX");
         goto installfail;
      };
   } else {
      netX=-1;
   }

   /* figure out if call to mdi_printcfg should be quiet or not.  mdi_printcfg
    * can be called from either driver's init or load routine so call our
    * routine to tell kernel what to do fairly early on.   
    * Also look for the optional __CHARM argument:
    *          If __CHARM=1 (the default) then be quiet
    *          If __CHARM=0 then ok for mdi_printcfg(D3mdi) to write to console
    */
   charmmode=1;   /* assume the worst as __CHARM argument may not be present */
   if ((charm=FindStringListText(slp, "__CHARM=")) != NULL) {
      if (*charm == '0') charmmode=0;
   }
   if (charmmode == 1) {
      mdi_printcfg(0);   /* mdi_printcfg(D3) should be quiet */
   } else {
      mdi_printcfg(1);   /* ok to write to console */
   }

   /* ok, we're off - look for all FILES= in MANIFEST section */
   if (netisl == 0) { /* normal idinstall - do the check */
      /* are the mandatory DSP files available?  (Driver.o Master System) */
      if (bcfgfile[bcfgindex].fullDSPavail == 0) {
         error(NOTBCFG,"The necessary DSP files were not found");
         goto installfail;
      }
#ifdef ALTERNATE  /* this does work in case you ever need to use it */
   printf("FILES= has %d words\n",StringListNumWords(bcfgindex,N_FILES));
   for (loop=1;loop<=StringListNumWords(bcfgindex,N_FILES); loop++) {
      char *tmp;

      tmp=StringListWordX(bcfgindex,N_FILES,loop);
      printf("FILES[%d]=%s\n",loop,tmp);
      if (tmp != NULL) free(tmp);
   }
#endif
      /* now check the FILES= stuff */
      files=StringListPrint(bcfgindex,N_FILES);
      if (files != NULL) {
         char tmppath[SIZE];
         char *cp;
         char *start=files;
         u_int allfound=1;
         char *file;

         file=strtok(start," ");
         while (file != NULL) {
            strncpy(tmppath,bcfgfile[bcfgindex].location,SIZE-10);
            cp=strrchr(tmppath,'/');
            if (cp == NULL) {
               strcpy(tmppath,file);
            } else {
               *(cp+1)=NULL;
               strcat(tmppath,file);
            }
            /* if file not found and pathname doesn't have "Driver" in it
             * then complain.  This takes care of the numerous .bcfg files
             * that only have Driver.o in the FILES= section instead of
             * Driver_atup.o, Driver_mp.o, etc. which is created by build
             * process.
             */
            if ((stat(tmppath,&sb) == -1) && 
                (strstr(tmppath,"Driver") == NULL)) {
               error(NOTBCFG,"file %s listed in FILES= not present",tmppath);
               allfound=0;
            }
            file=strtok(NULL," ");
         }
         free(files);
         if (!allfound) {
            goto installfail;  /* error() called previously */
         }
      }
   }
   /* can this driver do failover support if desired? */
   if ((failover == 1) && (HasString(bcfgindex,N_FAILOVER,"true",0) != 1)) {
      error(NOTBCFG,"driver does not support failover mode");
      goto installfail;
   }

   /* how about CONFORMANCE?  We only support those ODI drivers as found
    * in UW2.1 -- specifically, ODI versions 1.1, 3.1, 3.2 (current),
    * as set by the transmogrifier in the Master file.  This was 
    * translated by GiveV0Defaults into 0x110, 0x310, and 0x320
    * XXX create a routine EnsureConformance which does this check
    */

   /* determine how many instances of driver are already installed */
   tmpmodname=StringListPrint(bcfgindex,N_DRIVER_NAME);
   if (tmpmodname == NULL) {
      error(NOTBCFG,"DRIVER_NAME not set for bcfgindex %d",bcfgindex);
      goto installfail;
   }
   boardnumber=ResmgrGetLowestBoardNumber(tmpmodname);
   if (boardnumber == -1) {
      error(NOTBCFG,"idinstall: ResmgrGetLowestBoardNumber returned -1");
      free(tmpmodname);
      goto installfail;
   }
   free(tmpmodname);

   /* are we at the MAX_BD limit set by the bcfg file ? */
   zz=AtMAX_BDLimit(bcfgindex);
   if (zz == -1) {
      error(NOTBCFG,"idinstall: AtMAX_BDLimit returned -1");
      goto installfail;
   }
   if (zz == 1) {
      error(NOTBCFG,
     "You've reached the maximum number of boards that this driver can handle");
      goto installfail;
   }

   /* XXX If Advanced options BINDCPU UNIT IPL ITYPE supplied ensure
    * they are valid numbers.  Otherwise use the System file
    * except UNIT which can be overridden from UNIT= in bcfg file
    */

   /* XXX Weed out all arguments (IRQ/DMAC/IOADDR/MEMADDR/OLDIRQ/OLDDMAC/
    * OLDIOADDR/OLDMEMADDR/KEY/BINDCPU/UNIT/IPL/ITYPE
    * so that any arguments left must be CUSTOM[x] variables.  Now
    * go through and ensure that the remaining parameters are indeed
    * CUSTOM[x] parameters and that the value is a legal choice. 
    * this means looking at CUSTOM[x] line 1 and line 2
    * any other parameter should be flagged should produce an error
    */

   /* XXX we need to look at CUSTOM[x] line 9 (BOARD, DRIVER, or GLOBAL)
    * and set parameters accordingly -- possibly for other instances
    * of the driver, or for dlpi("GLOBAL").
    */

   /* XXX put other checks here that can fail before creating new key for ISA
    */

   /* we now check if ISA to see if more arguments required
    * and if PCI/EISA/MCA we ensure that KEY= is supplied
    * the BUS= line was converted to upper case earlier by call to strtoupper
    */
   if ((HasString(bcfgindex, N_BUS, "PCI", 0) == 1)  ||
#ifdef CM_BUS_I2O
       (HasString(bcfgindex, N_BUS, "I2O", 0) == 1)  ||
#endif
       (HasString(bcfgindex, N_BUS, "EISA",0) == 1)  ||
       (HasString(bcfgindex, N_BUS, "MCA", 0) == 1)) {
      /* PCI/EISA/MCA */

      int tmpirq;

      keystr=FindStringListText(slp, "KEY=");
      if (keystr == NULL) {
         error(NOTBCFG, 
               "BUS is non-ISA at bcfgindex %d but no KEY= argument supplied!",
               bcfgindex);
         goto installfail;
      }
      if (((rmkey=strtoul(keystr,&strend,10))==0) && (strend == keystr)) {
         error(NOTBCFG,"bogus KEY=%s",keystr);
         rmkey=RM_KEY;
         goto installfail;
      }
      highest=ResmgrHighestKey();
      if (highest == -1) {
         error(NOTBCFG,"idinstall: ResmgrHighestKey returned -1");
         rmkey=RM_KEY;
         goto installfail;
      }
      if ((rmkey < 1) || (rmkey > highest)) {
         error(NOTBCFG,"idinstall: key must be from 1 to %d",highest);
         rmkey=RM_KEY;
         goto installfail;
      }

      /* ensure that the board id at desired key is supported by .bcfg file */
      ec=ResmgrEnsureGoodBrdid(rmkey, bcfgindex);
      if (ec != 0) {
         error(NOTBCFG,"idinstall: resmgr key %d doesn't support bcfgindex %d!",
               rmkey, bcfgindex);
         rmkey=RM_KEY;
         goto installfail;
      }

      /* ensure that no other driver is using key in resmgr; it should be
       * an unclaimed entry (as the showresunclaimed command would show) 
       * or claimed by driver we're about to use but no NETCFG_ELEMENT set
       * (again, the criteria that resshowunclaimed uses)
       * the latter case occurs with multiple smart bus boards of same type
       * and the first has a Drvmap and has already been installed in system
       * the system silently automatically adds MODNAME
       */
      ec=ResmgrGetVals(rmkey,CM_MODNAME,0,modname,VB_SIZE);
      if (ec != 0) {
         error(NOTBCFG,
            "idinstall: ResmgrGetVals for MODNAME returned %d",ec);
         rmkey=RM_KEY;
         goto installfail;
      }
      /* CM_MODNAME is type STR_VAL so we'll get "-" from RMgetvals if
       * it's not set.  niccfg also sets MODNAME to "unused" under
       * some circumstances, so continue on if it's set to that, as
       * we'll overwrite MODNAME later on at the end of idinstall.
       */
      tmpcp=StringListPrint(bcfgindex, N_DRIVER_NAME);
      if (tmpcp == NULL) {
         error(NOTBCFG,"idinstall: no DRIVER_NAME for bcfgindex %d",bcfgindex);
         rmkey=RM_KEY;
         goto installfail;
      }
      if ((strcmp(modname,"-") != 0) && 
          (strcmp(modname,"unused") != 0) &&
          (strcmp(modname, tmpcp) != 0)) {
         error(NOTBCFG,"resmgr key %d already taken by driver %s",
               rmkey,modname);
         rmkey=RM_KEY;
         goto installfail;
      }
      free(tmpcp);
      
      /* since we allow installing where MODNAME is set to our own driver,
       * we must see if this driver has already been configured by ndcfg
       * by checking for element too
       */
      snprintf(foofoo,VB_SIZE,"%s,s",CM_NETCFG_ELEMENT);
      ec=ResmgrGetVals(rmkey,foofoo,0,cmd,SIZE * 2);
      if (ec != 0) {
         error(NOTBCFG,
            "idinstall: ResmgrGetVals for NETCFG_ELEMENT returned %d",ec);
         rmkey=RM_KEY;
         goto installfail;
      }
      /* if ndcfg has already installed at this key then NETCFG_ELEMENT
       * will not be -
       */
      if (strcmp(cmd, "-") != 0) {
         error(NOTBCFG,"idinstall: ndcfg has already installed key %d",rmkey);
         rmkey=RM_KEY;
         goto installfail;
      }

      /* ensure that no ISA boards are already using the IRQ at this resmgr
       * key.  We do this by looking at IRQ at this key and ITYPE for all keys
       */
      if (irqatkey(rmkey, &tmpirq, 1) == -1) {
         error(NOTBCFG,"idinstall:  failure in irqatkey");
         rmkey=RM_KEY;
         goto installfail;
      }
      if (tmpirq != -2) {
         if ((zz=irqsharable(rmkey, tmpirq, 1)) == -1) {
            error(NOTBCFG,"idinstall:  failure in irqsharable");
            rmkey=RM_KEY;
            goto installfail;
         }
         if (zz == 0) {
            error(NOTBCFG,"idinstall: IRQ at resmgr key %d is in use "
                          "by somebody else that won't share it", rmkey);
            rmkey=RM_KEY;
            goto installfail;
         }
      }

      /* ensure none of IRQ=/DMAC=/IOADDR=/MEMADDR=/
       * OLDIRQ=/OLDDMAC=/OLDIOADDR=/OLDMEMADDR= are supplied arguments;
       * these are ISA specific
       */
      if (EnsureNotDefined(slp,"IRQ=")) goto installfail;  /* error called */
      if (EnsureNotDefined(slp,"DMAC=")) goto installfail; /* error called */
      if (EnsureNotDefined(slp,"IOADDR=")) goto installfail;  /* ditto */
      if (EnsureNotDefined(slp,"MEMADDR=")) goto installfail; /* ditto */
      if (EnsureNotDefined(slp,"OLDIRQ=")) goto installfail;
      if (EnsureNotDefined(slp,"OLDDMAC=")) goto installfail;
      if (EnsureNotDefined(slp,"OLDIOADDR=")) goto installfail;
      if (EnsureNotDefined(slp,"OLDMEMADDR=")) goto installfail;

      /* backup keys are now only for MDI drivers */
      if (HasString(bcfgindex,N_TYPE, "MDI", 0) == 1) {
         char *dn=StringListPrint(bcfgindex, N_DRIVER_NAME); /* will succeed */
         if ((backupkey=CreateNewBackupKey(rmkey, dn, boardnumber, netX)) == 
                                                                      RM_KEY) {
            error(NOTBCFG,"idinstall: couldn't create backup key for smart bus "
                          "card at key=%d",rmkey);
            free(dn);
            rmkey=RM_KEY;
            goto installfail;
         }
         free(dn);
         StartBackupKey(backupkey);
      }
 
   } else if (HasString(bcfgindex,N_BUS, "ISA", 0) == 1) {
      /* ISA card or something which had AUTOCONF=false so it will never have
       * an automatic key in the resmgr.  create one.
       */
      isainstall=1;
      /* make sure we have an ISA bus on this machine before proceeding 
       * MCA boxes will fit into this category.  See confmgr/confmgr_p.c
       * comment and code
       *    if (( _cm_bustypes & CM_BUS_MCA ) == 0 )
       *       _cm_bustypes |= CM_BUS_ISA;
       */
      if ((cm_bustypes & CM_BUS_ISA) == 0) {
         error(NOTBCFG,"idinstall - can't install ISA card as this machine "
                       "doesn't have an ISA bus!");
         goto installfail;
      }
      /* If ISA, we will have more arguments.  we mandate 6 args minimum:
       * IRQ (what EnsureAllAssigned() does) and one other one.
       * The hope is that the other will be IOADDR or MEMADDR but we don't
       * check or care.
       */
      if (numargs < 6) {   /* now we've identified this as ISA */
         error(NOTBCFG,"idinstall for ISA requires at least 6 arguments");
         goto installfail;
      }
      /* KEY= should not be supplied as argument for ISA drivers */
      if (FindStringListText(slp, "KEY=") != NULL) {
         error(NOTBCFG, "ISA drivers do not have a KEY= argument");
         goto installfail;
      }
      /* if INT=  exists in bcfg file, ensure it was supplied as an argument
       *  and the value is within the bcfg file range.
       * if DMA=  exists in bcfg file, ensure it was supplied as an argument
       *  and the value is within the bcfg file range.
       * if PORT= exists in bcfg file, ensure it was supplied as an argument
       *  and the value is within the bcfg file range.
       * if MEM=  exists in bcfg file, ensure it was supplied as an argument
       *  and the value is within the bcfg file range.
       * the above will not be a problem if user called getISAparams first
       * we *should* also ensure that the number isn't taken by anyone else
       * in the link kit but we only check that in getISAparams.  this is better
       * than nothing.  XXX future enhancement
       * Also note that idresadd does a better job since it walks through
       * the resmgr trying to match an unclaimed board with information
       * in the System file.  We leave it to resshowunclaimed to do this
       * and it solely uses the board id.
       */

      /* INT */
      if (HasString(bcfgindex,N_INT,"x",1) != -1) {  /* -1 means undefined */
         /* INT= exist in bcfg file.  is it supplied as argument? */
         if ((irq=FindStringListText(slp, "IRQ=")) == NULL) {
            error(NOTBCFG,"INT= exists in bcfg but IRQ=x arg. not supplied");
            goto installfail;
         }
         strtolower(irq);  /* we store numbers in lower case in bcfg file */

         /* if user gave us IRQ=2, convert it to IRQ=9. See IsParam() too */
         if (atoi(irq) == 2) irq="9";

         if (HasString(bcfgindex,N_INT,irq,0) != 1) {
            /* the IRQ desired by the user isn't specified in the bcfg file */
            if (atoi(irq) == 9) {
               /* last ditch effort:  we know 9 isn't in bcfg file but
                * we need to see if irq 2 is in the bcfg file
                */
               if (HasString(bcfgindex, N_INT, "2", 0) == 0) {
                  /* to get here irq was 2, got converted to 9, and both 
                   * 2 and 9 are not in the bcfg file.  error and bail.
                   */
                  error(NOTBCFG,"IRQ=2/9 value not valid for bcfg file range");
                  goto installfail;
               }
               /* to get here either HasString failed or IRQ 2 is in bcfg file 
                * and 9 isn't.  We assume the latter: No error.
                */
            } else {
               error(NOTBCFG,"IRQ=%s value not valid for bcfg file range",irq);
               goto installfail;
            }
         }

         /* is it already taken in the resmgr? */

         /* since this is only for ISA cards we don't have to worry about
          * ITYPE sharing here -- existence of driver in resmgr is sufficient
          * Moreover, since ISA can't share, if found in the resmgr we don't
          * need to check ITYPE!
          */
         tmpstatus=ISPARAMTAKEN(N_INT, irq);
         if (tmpstatus == -1) {
            error(NOTBCFG,"idinstall: error in Isparam for INT");
            goto installfail;
         } 
         if (tmpstatus == 1) {
            error(NOTBCFG,"idinstall: IRQ %s already taken in resmgr",irq);
            goto installfail;
         }
      } else {    /* INT= not defined in bcfg file.  ensure user doesn't
                   * try and define it as an argument
                   */
         if (FindStringListText(slp, "IRQ=") != NULL) {
            error(NOTBCFG,
                "INT isn't defined in bcfg file so don't pass IRQ as argument");
            goto installfail;
         }
      }
      

      /* DMA */
      if (HasString(bcfgindex,N_DMA,"x",1) != -1) {  /* -1 means undefined */
         /* DMA= exist in bcfg file.  is it supplied as argument? */
         if ((dma=FindStringListText(slp, "DMAC=")) == NULL) {
            error(NOTBCFG,"DMA= exists in bcfg but DMAC=x arg. not supplied");
            goto installfail;
         }
         strtolower(dma);
         if (HasString(bcfgindex,N_DMA,dma,0) != 1) {
            error(NOTBCFG,"DMAC=%s value not valid for bcfg file range",dma);
            goto installfail;
         }
         /* is it already taken in the resmgr? */
         tmpstatus=ISPARAMTAKEN(N_DMA, dma);
         if (tmpstatus == -1) {
            error(NOTBCFG,"idinstall: error in Isparam for DMA");
            goto installfail;
         } 
         if (tmpstatus == 1) {
            error(NOTBCFG,"idinstall: DMAC %s already taken in resmgr",dma);
            goto installfail;
         }
      } else {    /* DMA= not defined in bcfg file.  ensure user doesn't
                   * try and define it as an argument
                   */
         if (FindStringListText(slp, "DMAC=") != NULL) {
            error(NOTBCFG,
               "DMA isn't defined in bcfg file so don't pass DMAC as argument");
            goto installfail;
         }
      }


      /* PORT */
      if (HasString(bcfgindex,N_PORT,"x",1) != -1) {  /* -1 means undefined */
         /* PORT= exist in bcfg file.  is it supplied as argument? */
         if ((port=FindStringListText(slp, "IOADDR=")) == NULL) {
            error(NOTBCFG,"PORT= exists in bcfg but IOADDR=x arg not supplied");
            goto installfail;
         }
         strtolower(port);
         if (HasString(bcfgindex,N_PORT,port,0) != 1) {
            error(NOTBCFG,"IOADDR=%s value not valid for bcfg file range",port);
            goto installfail;
         }
         /* is it already taken in the resmgr? */
         tmpstatus=ISPARAMTAKEN(N_PORT, port);
         if (tmpstatus == -1) {
            error(NOTBCFG,"idinstall: error in Isparam for PORT");
            goto installfail;
         } 
         if (tmpstatus == 1) {
            error(NOTBCFG,"idinstall: IOADDR %s already taken in resmgr",port);
            goto installfail;
         }
      } else {    /* PORT= not defined in bcfg file.  ensure user doesn't
                   * try and define it as an argument
                   */
         if (FindStringListText(slp, "IOADDR=") != NULL) {
            error(NOTBCFG,
            "PORT isn't defined in bcfg file so don't pass IOADDR as argument");
            goto installfail;
         }
      }


      /* MEM */
      if (HasString(bcfgindex,N_MEM,"x",1) != -1) {  /* -1 means undefined */
         /* MEM= exist in bcfg file.  is it supplied as argument? */
         if ((mem=FindStringListText(slp, "MEMADDR=")) == NULL) {
            error(NOTBCFG,"MEM= exists in bcfg but MEMADDR=x arg not supplied");
            goto installfail;
         }
         strtolower(mem);
         if (HasString(bcfgindex,N_MEM,mem,0) != 1) {
            error(NOTBCFG,"MEMADDR=%s value not valid for bcfg file range",mem);
            goto installfail;
         }
         /* is it already taken in the resmgr? */
         tmpstatus=ISPARAMTAKEN(N_MEM, mem);
         if (tmpstatus == -1) {
            error(NOTBCFG,"idinstall: error in Isparam for MEM");
            goto installfail;
         } 
         if (tmpstatus == 1) {
            error(NOTBCFG,"idinstall: MEMADDR %s already taken in resmgr",mem);
            goto installfail;
         }
      } else {    /* MEM= not defined in bcfg file.  ensure user doesn't
                   * try and define it as an argument
                   */
         if (FindStringListText(slp, "MEMADDR=") != NULL) {
            error(NOTBCFG,
            "MEM isn't defined in bcfg file so don't pass MEMADDR as argument");
            goto installfail;
         }
      }

      /* XXX we should also make sure that the value in OLDIRQ/OLDDMAC/OLDIOADDR
       * OLDMEMADDR isn't taken by anything else in the link kit
       * we can't use ISPARAMTAKEN/ISPARAMAVAIL because this is for ISA
       * boards and we haven't yet created a key for this board in the
       * resmgr.  Moreover, you could even expect that in the case of
       * ISA conflicts that the board may exist and conflict already in the
       * resmgr, prompting the OLDIOADDR etc in the first place!
       */

      /* OLDIRQ */
      if ((oldirq=FindStringListText(slp, "OLDIRQ=")) != NULL) {
         strtolower(oldirq);
         if (HasString(bcfgindex,N_INT,oldirq,0) == -1) {
            error(NOTBCFG,"OLDIRQ= supplied but bcfg doesn't use INT=!");
            goto installfail;
         }
         /* XXX irq 2<->9 problems here if both not in bcfg file!!! FIX */
         if (HasString(bcfgindex,N_INT,oldirq,0) == 0) {
            error(NOTBCFG,"OLDIRQ=%s value not valid for bcfg file range",
                  oldirq);
            goto installfail;
         }
      }
    
      /* OLDDMAC */
      if ((olddma=FindStringListText(slp, "OLDDMAC=")) != NULL) {
         strtolower(olddma);
         if (HasString(bcfgindex,N_DMA,olddma,0) == -1) {
            error(NOTBCFG,"OLDDMAC= supplied but bcfg doesn't use DMA=!");
            goto installfail;
         }
         if (HasString(bcfgindex,N_DMA,olddma,0) == 0) {
            error(NOTBCFG,"OLDDMAC=%s value not valid for bcfg file range",
                  olddma);
            goto installfail;
         }
      }

      /* OLDIOADDR */
      if ((oldport=FindStringListText(slp, "OLDIOADDR=")) != NULL) {
         strtolower(oldport);
         if (HasString(bcfgindex,N_PORT,oldport,0) == -1) {
            error(NOTBCFG,"OLDIOADDR= supplied but bcfg doesn't use PORT=!");
            goto installfail;
         }
         if (HasString(bcfgindex,N_PORT,oldport,0) == 0) {
            error(NOTBCFG,"OLDIOADDR=%s value not valid for bcfg file range",
                  oldport);
            goto installfail;
         }
      }

      /* OLDMEMADDR */
      if ((oldmem=FindStringListText(slp, "OLDMEMADDR=")) != NULL) {
         strtolower(oldmem);
         if (HasString(bcfgindex,N_MEM,oldmem,0) == -1) {
            error(NOTBCFG,"OLDMEMADDR= supplied but bcfg doesn't use MEM=!");
            goto installfail;
         }
         if (HasString(bcfgindex,N_MEM,oldmem,0) == 0) {
            error(NOTBCFG,"OLDMEMADDR=%s value not valid for bcfg file range",
                  oldmem);
            goto installfail;
         }
      }


      status=ResmgrNextKey(&rmkey,1); /* does OpenResmgr/CloseResmgr */
      if (status != 0) {
         error(NOTBCFG,"idinstall: couldn't create new key status=%d",status);
         goto installfail;
      }

      /* backup keys are now only for MDI drivers */
      if (HasString(bcfgindex,N_TYPE, "MDI", 0) == 1) {
         char *dn=StringListPrint(bcfgindex, N_DRIVER_NAME); /* will succeed */
         if ((backupkey=CreateNewBackupKey(rmkey, dn, boardnumber, netX)) == 
                                                                      RM_KEY) {
            error(NOTBCFG,"idinstall: couldn't create backup key for ISA "
                          "card at key=%d",rmkey);
            free(dn);
            goto installfail;
         }
         free(dn);
         StartBackupKey(backupkey);
      }

      /* ISA only: add CM_BRDBUSTYPE.  no need to call bcfgbustype()  
       * by adding it first thing we know that the key will be deleted
       * if there are subsequent calls to ResmgrDelInfo(rmkey) in case
       * of failure in DoDSPStuff/DoExtraStuff/DoResmgrStuff/DoRemainingStuff
       * Note that in the case of AUTOCONF=false this won't match the
       * actual bus type of the board if no errors occur later on in these
       * routines but that's not germane to us -- we
       * only care that ResmgrDelInfo will be able to detect this was 
       * ISA and delete it in case of a problem.
       */
      snprintf(cmd,SIZE*2,"%s=%d",CM_BRDBUSTYPE,CM_BUS_ISA);/* no ,n needed */
      snprintf(rmkey_as_string,10,"%d",rmkey);
      if (resput(rmkey_as_string,cmd,0)) {
         error(NOTBCFG,"idinstall: couldn't add BRDBUSTYPE to resmgr");
         goto installfail;
      }

   } else {  /* not PCI/EISA/MCA/ISA   could be PCCARD or PNPISA someday...*/
      error(NOTBCFG,
        "unsupported bus type for idinstall at bcfgindex %d",bcfgindex);
      error(NOTBCFG,
        "(did you forget to set AUTOCONF=false in the bcfg file?)");
      goto installfail;
   }

   /* COMMON INSTALL POINT FOR ISA/PCI/EISA/MCA RESUMES HERE */

   /* kinda lame, but we always read the DSP file (even if driver already
    * installed and numboards > 0) to determine the $interface ddi level
    * this is used for adding CM_NDCFG_UNIT later on
    */
   if (HasString(bcfgindex,N_TYPE,"ODI",0) == 1) {
      /* ODI drivers don't have $interface ddi in Master file.  Instead,
       * they have:
       *   $interface  odieth/oditok/odifddi     with version 3.1/3.2/3.3
       *   $interface  odictok/odiceth/odicfddi  all with version 1.1
       * we really don't care about specific odi interface so we fake a
       * ddi level for the driver: 5
       */
      snprintf(ddi,100,"5");
   } else {
      /* all MDI and DLPI drivers must have $interface ddi line 
       * in their Master file 
       */
      if (determineddilevel(bcfgindex, ddi, sizeof(ddi))) {
         error(NOTBCFG,"idinstall: can't determine ddi level for bcfgindex %d",
               bcfgindex);
         goto installfail;
      }
   }

   if (netisl == 0) {
      int numboards;
      /* we look at the number of instances of the driver in the resmgr
       * to determine if this driver already exists or not.  This is
       * normally correct since idinstall -d will run idresadd -d to delete
       * any resmgr entries for that driver, so this will only be
       * wrong for somebody who deletes the DSP files by hand and forgets
       * to run idresadd -d or resmgr -r -k key, in which case they deserve it.
       * The alternate approach is to run idcheck -y, but that doesn't ensure
       * that the driver has entries in the resmgr.  Sigh.
       */
      driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
      if (driver_name == NULL) {
         error(NOTBCFG,"idinstall: DRIVER_NAME not defined in bcfg file");
         goto installfail;
      }
      numboards=ResmgrGetNumBoards(driver_name);
      free(driver_name);

      if (numboards == -1) {
         error(NOTBCFG,"idinstall: ResmgrGetNumBoards failed");
         goto installfail;
      }

      if (numboards == 0) {
         /* HANDLE STUFF ONLY NECESSARY FOR FIRST INSTANCE OF DRIVER */
         if (DoDSPStuff(bcfgindex,rmkey,&System,&fullrelink)) {
            /* error already called but we'll leave further trace here */
            error(NOTBCFG,"idinstall: DoDSPStuff failed");
            goto installfail;
         }
         if (DoExtraDSPStuff(bcfgindex,rmkey,netX,&fullrelink,real_topology)) {
            /* error already called but we'll leave further trace here */
            error(NOTBCFG,"idinstall: DoExtraDSPStuff failed");
            goto installfail;  
         }
      } else {

         /* there's already at least one instance of this driver in the system.
          * there's a very good chance that dlpid is already bound and using
          * the first mdi device, so any attempt to unload the driver with
          * modadmin -U will likely fail.  If the driver isn't ddi8 then
          * we must set reboot to true to force the driver's load routine to
          * be called again, since when we add MODNAME later on the ddi7
          * driver won't have a clue that we did this and new device won't be
          * usable on system until we reboot.  not necessary for ddi8
          * since it uses CFG_ADD to dynamically add a new instance to system
          */
         if (ddi[0] < '8') {
            notice("already %d instance(s) of driver; setting reboot to true",
                   numboards);
            reboot="Y";
         }

         /* must read System file again, only this time get it from
          * the link kit.  This is to set UNIT,BINDCPU,ITYPE,IPL in the resmgr
          * in DoResmgrStuff.  We don't need to do this on the first
          * add since DoDSPStuff can read the System file from the DSP prior
          * the idinstall command.  Since everything in sdevice.d is
          * considered volitile, we resort to .sdevice.d for the original
          * copy.  Note that this copy will only have one data line in it,
          * courtesy of TurnOnSdevs.
          */
         chdir("/etc/conf/.sdevice.d");
         driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
         if (driver_name == NULL) {
            error(NOTBCFG,"idinstall: DRIVER_NAME not defined in bcfg file");
            goto installfail;
         }
         if (ReadSystemFile(&System,driver_name)) {
            error(NOTBCFG,"idinstall: problem reading .sdevice.d System file");
            free(driver_name);
            goto installfail;
         }
         free(driver_name);
      }
      /* do stuff necessary for ALL instances of driver */

      /* we do all custom params, ndcfg specific params, and resmgr commands
       * below.  So we set variable early on (not completely correct, but 
       * close enough so that if command fails we'll clean up properly
       */
      ok_to_ResmgrDelInfo=1;
      if (DoResmgrStuff(slp, bcfgindex, rmkey, netX, boardnumber, 
                        &System, &fullrelink,
                        &numpatches, patch,
                        &delbindcpu, &delunit, &delipl, tunes, &numtunes,
                        real_topology)) {
         error(NOTBCFG,"idinstall: DoResmgrStuff failed");
         goto installfail;  /* error called previously */
      }
      if (DoRemainingStuff(bcfgindex, rmkey, netX, &fullrelink,
                           failover, lanwan, &removedlpimdi, devicename,
                           &removenetX, &callednetinfo, netinfoname, 
                           element, tunes, &numtunes,
                           real_topology, boardnumber)) {
         error(NOTBCFG,"idinstall: DoRemainingStuff failed");
         goto installfail;
      }
   } else if (netisl == 1) {
      /* XXX if Driver.mod doesn't exist then error out 
       * copy Driver.mod into /etc/conf/mod.d
       * modadmin -l the driver
       *
       * XXX ensure Driver.mod is a valid DLM (check for ELF section SHT_MOD)
       * XXX modadmin -U to unload driver about to be turned into a DLM.
       * copy in the Driver.mod file from bcfg pathname into
       * /etc/conf/mod.d, add a line to /etc/conf/mod_register, run
       * idmodreg -f /etc/conf/mod_register
       * THEN add info to resmgr
       * THEN modadmin -l /etc/conf/mod.d/<drivername>
       * IF we're smart you can figure out the lines to add from the Master file
       * last field which is the major number range.  For example
       * SMC8K  SMC8K LhSc   0   0   298-302
       * gets turned into
       *   5:1:SMC8K:298
       *   5:1:SMC8K:299
       *   5:1:SMC8K:300
       *   5:1:SMC8K:301
       *   5:1:SMC8K:302
       * in /etc/conf/mod_register.  You can also do the modadm(2) call yourself
       * to do the registration.
       */
   }

   /* add MODNAME to resmgr and run idconfupdate.  This will turn the
    * hardware MDI driver back on for the idbuild.  It was previously
    * turned off by idconfupdate (run by previous calls to idcheck for dlpi)
    * close resmgr to commit transactions and for idconfupdate to run
    * if ISA call verify routine on that key -- should succeed now that 
    * all information in resmgr and driver installed.
    * must do this before the idbuild step and not after!
    */
   driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
   if (driver_name == NULL) {
      error(NOTBCFG,"idinstall: DRIVER_NAME not defined");
      goto installfail;
   }

   /* stop our backup process and write MODNAME=netX to the backup key
    * any further resput commands past this point will not go to our
    * backup key.  Specifically, we don't want to write MODNAME=foo
    * giving the illusion of two devices in the resmgr!
    * Note that backups may not be enabled if not MDI driver but it's 
    * still ok to call EndBackup 
    */
   snprintf(cmd,SIZE*2,"net%d",netX);
   EndBackupKey(cmd);

   Status(89, "Adding MODNAME");

   /* ddi7 drivers only call cm_getnbrd at load time so deleting modname
    * from its present value (if already set to driver) won't impact anything
    * ddi8 drivers have CFG_ADD called whenever modname is added which can
    * occur a) when booting and b) whenever we add modname
    * if dcu -S has already added modname to device and the driver's CFG_ADD
    * called mdi_get_unit (which failed previously) we want the addition
    * of MODNAME below to call our CFG_ADD code again.  since we've
    * added DEV_NAME (ddi8 case) to resmgr by now then when the ddi8 driver
    * calls mdi_get_unit it will now succeed.   In order to make this happen
    * we want to remove modname entirely and re-add it back in.
    * We know at this point that modname is either blank("-") or already 
    * set to the driver's name (dcu -S) so this won't hurt things.
    * so from the driver's perspective it can see a CFG_REMOVE at the
    * key followed by a CFG_ADD.   CFG_MODIFY won't get called because it's
    * only for drivers currently in suspended state.  Think of this as a 
    * budget CFG_MODIFY...
    */
   if (toastmodname(rmkey)) {
      error(NOTBCFG,"idinstall: couldn't remove MODNAME at key %d",rmkey);
      goto installfail;
   }

   /* If hpsl_IsInitDone says we have controllers, use it to add MODNAME
    * and turn power on.  If anything falls fall back to old way.  If
    * no hpsl controllers use old way
    */
   hpsladded=0;
   /* if (hpsl_IsInitDone() == HPSL_SUCCESS) open brace */
   if (hpslcount > 0) {   /* if we have at least 1 hot controller on system */
      HpslSocketInfoPtr_t socketinfo;

      notice("idinstall: adding MODNAME and turning on power via hpsl");
      BlockHpslSignals();
      socketinfo=GetHpslSocket(rmkey);
      if (socketinfo == NULL) {
         /* it's perfectly acceptable for this to fail if the board we're
          * trying to add is EISA or ISA.  Right now hpsl only works for
          * adding PCI devices, so simply having a hotplug controller doesn't
          * mean that hpsl will be able to cope with adding this device
          * because hpsl may not support this device type!  If this happens
          * then revert to adding MODNAME by hand.  But we want to give
          * hpsl the first shot at it.
          */
         notice("idinstall: couldn't find socket for rmkey %d",rmkey);
         UnBlockHpslSignals(); 
         goto addoldway;
      }
      if (socketinfo->socketCurrentState & SOCKET_EMPTY) {
         notice("idinstall: hpsl says socket is empty");
         UnBlockHpslSignals();
         goto addoldway;
      }
      if (!socketinfo->socketCurrentState & SOCKET_POWER_ON) {
         if (hpsl_set_state(socketinfo, SOCKET_POWER_ON, HPSL_SET) != 
                                                                HPSL_SUCCESS) {
            notice("idinstall: hpsl_set_state to turn power on "
                   "failed, hpsl_ERR=%d", hpsl_ERR);
            UnBlockHpslSignals(); 
            goto addoldway;
         }
      }
      /* if this is a ddi8 driver hpsl_bind_driver will call driver's 
       * CFG_ADD code 
       */
      if (hpsl_bind_driver(socketinfo, rmkey, driver_name) != HPSL_SUCCESS) {
         notice("idinstall: hpsl_bind_driver failed, hpsl_ERR=%d",hpsl_ERR);
         UnBlockHpslSignals(); 
         goto addoldway;
      }
      UnBlockHpslSignals(); 
      hpsladded++;
   }
addoldway:
   if (hpsladded == 0) {
      /* either not using hpsl library or we encountered an error when trying
       * to use hpsl library.  Our scheme to add IPL is the same as that
       * found in hpsl_bind_driver so we won't be missing anything here
       * by just adding MODNAME ourselves.  However, hpsl_bind_driver does 
       * update  update socketInfo->devList[devIx].drvName with the name we
       * bind to, which we obviously don't do.  *Hopefully* the next
       * time our itimer fires HpslHandler->hpsl_check_async_events
       * will notice the change and update drvName for us so that a later
       * attempt to call hpsl_bind_driver will suceed.
       */
      notice("idinstall: adding MODNAME by hand");
      snprintf(cmd,SIZE * 2,"%s=%s",CM_MODNAME,driver_name);  /* no ,s needed */
      snprintf(rmkey_as_string,10,"%d",rmkey);
      if (resput(rmkey_as_string,cmd,0)) {
         goto installfail;   /* error called in resput */
      }
   }
   /* after this point if we encounter a failure then we need to remove
    * MODNAME since we just set it.
    * Note that ok_to_ResmgrDelInfo was set to 1 earlier -- 
    * used by installfail to see if we should call ResmgrDelInfo
    */
   delmodname = 1;

   /* and write CM_BACKUPMODNAME to our backup key (note that 
    * backups must be disabled at this point else resput will error())
    */
   snprintf(cmd,SIZE * 2,"%s,s=%s",CM_BACKUPMODNAME,driver_name);
   snprintf(rmkey_as_string,10,"%d",backupkey);
   if (resput(rmkey_as_string,cmd,0)) {
      snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
      goto installfail;   /* error called in resput */
   }
   snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */


   /* now write CM_NDCFG_UNIT parameter to resmgr
    * if ddi level less than 8 than use boardnumber else use .INSTNUM
    * which has just been set by the kernel when we added MODNAME above
    */
   if (ddi[0] < '8') {
      /* ddi 5 5mp 6 6mp 7 7.1 7.1mp 7.2mp 7mp  driver -OR- forged ODI driver */
      snprintf(cmd, SIZE * 2, "%s,n=%d", CM_NDCFG_UNIT, boardnumber);
      snprintf(rmkey_as_string,10,"%d",rmkey);
      if (resput(rmkey_as_string,cmd,0)) {
         goto installfail;   /* error called in resput */
      }

      /* and write to backup key */
      snprintf(cmd, SIZE * 2, "%s,n=%d", CM_NDCFG_UNIT, boardnumber);
      snprintf(rmkey_as_string,10,"%d",backupkey);
      if (resput(rmkey_as_string,cmd,0)) {
         snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
         goto installfail;   /* error called in resput */
      }
      snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
   } else {
      /* ddi 8 8mp    driver */
      int instnum;

      /* .INSTNUM is stored as a number and libresmgr doesn't know about it */
      snprintf(foofoo,VB_SIZE,"%s,n",CM_INSTNUM);   /* must add typing */
      ec=ResmgrGetVals(rmkey,foofoo,0,cmd,SIZE * 2);
      if (ec != 0) {
         error(NOTBCFG,
            "idinstall: ResmgrGetVals for INSTNUM returned %d",ec);
         rmkey=RM_KEY;
         goto installfail;
      }
      instnum=atoi(cmd);

      snprintf(cmd, SIZE * 2, "%s,n=%d", CM_NDCFG_UNIT, instnum);
      snprintf(rmkey_as_string,10,"%d",rmkey);
      if (resput(rmkey_as_string,cmd,0)) {
         goto installfail;   /* error called in resput */
      }

      /* and write to backup key */
      snprintf(cmd, SIZE * 2, "%s,n=%d", CM_NDCFG_UNIT, instnum);
      snprintf(rmkey_as_string,10,"%d",backupkey);
      if (resput(rmkey_as_string,cmd,0)) {
         snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
         goto installfail;   /* error called in resput */
      }
      snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
   }

   free(driver_name);

   if (netisl == 0) {
      /* run idconfupdate ourselves after adding MODNAME to re-sync link kit 
       * files with now-updated in-core resmgr database.  This will update the
       * appropriate System file(extra bonus - we didn't have to!).
       * and turn our hardware ('h' flag in Master) driver back on(it 
       * was turned off by previous call to idconfupdate in idcheck)
       * since there is now an entry for the driver in MODNAME in the resmgr.
       */
      snprintf(cmd,SIZE * 2, "/etc/conf/bin/idconfupdate -f");
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"idinstall: command idconfupdate failed");
         goto installfail;
      }
   }

   /* bflag is set when the user instructs us to run idbuild 
    * on the off chance that we only had an ODIMEM=true and IDTUNE but no DLM's
    * (unlikely) we also check fullrelink variable
    */
   if ((netisl == 0) && (bflag != 0) && 
       ((modlist[0] != '\0') || (fullrelink == 1))) {
      /* unload old DLMs.  since it might fail we use runcommand not docmd */
      snprintf(cmd,SIZE * 2,"/sbin/modadmin -U %s",modlist);
      runcommand(0,cmd);  /* can fail if they don't exist hence runcommand */

      /* rebuild the kernel */
      /* if ODIMEM=true and we did IDTUNEs then force idbuild -B to do all */
      if (fullrelink == 0) {/* no complete relink necessary, can just do DLMs */
         snprintf(cmd,SIZE * 2,"/etc/conf/bin/idbuild %s",DLM);
      } else { /* a complete kernel relink is necessary due to idtune/DEPEND/
                * or odimem not present and now required
                * now netcfg pays attention to REBOOTREQUIRED, so don't 
                * do the idbuild -B any more
                */
         if (tclmode) {
            notice("not doing an idbuild -B, leaving that up to netcfg!");
            snprintf(cmd,SIZE * 2,"/bin/true");
         } else {
            snprintf(cmd,SIZE * 2,"/etc/conf/bin/idbuild -B");
         }
      }
      Status(90, "Building drivers");
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"command: idbuild %s failed",DLM);
         goto installfail;
      }

      /* load new DLMs.  this shouldn't fail since we just did idbuild -M */
      if (modlist[0] != '\0') {
         snprintf(cmd,SIZE * 2,"/sbin/modadmin -l %s",modlist);
         Status(91, "Loading drivers");
         if ((status=runcommand(0,cmd)) != 0) {
            /* this will fail if you don't have P_LOADMOD priviledge.
             * can also fail if the load routine fails because
             * the driver failed its presence test (i.e.
             * foo_load->foo_init->presence test->ENODEV.  This is likely
             * for ISA boards that aren't in the system yet. So don't fail
             * the idinstall command if this step fails
             * OR ODI driver, odimem just built, but no reboot yet...
             */
            notice("problems loading the following DLMs (see console "
                   "and /usr/adm/log/osmlog): %s",modlist);
         };
      }

      /* do PATCH global custom parameters here
       * since driver loaded, symbol table visible, so we should be
       * able to patch driver symbols as well.
       */
      if (numpatches) {
         Status(92,"Patching memory");
         for (loop=0; loop < numpatches; loop++) {
            notice("setting PATCH custom parameter %s to %d",
                   patch[loop].symbol, patch[loop].newvalue);
            donlist(1, patch[loop].symbol, patch[loop].newvalue, 0);
         }
      }
      
      /* handle CONFIG_CMDS.  niccfg did not do this at all.  ISL does do this.
       * Traditionally this represents the list of commands to run to get the 
       * card to work.  traditionally associated with download code.  The 
       * PRE_SCRIPT just moves the files into place for a download to take 
       * place but should not run them -- that's the job for CONFIG_CMDS, which
       * we do here.   Note ISL doesn't run CONFIG_CMDS with any arguments,
       * so we don't either.   Should work as driver is loaded at this point.
       * Must do CONFIG_CMDS before we attempt to open up the card.
       * It's imperative that the PRE_SCRIPT run first to put the files in
       * place before we call the CONFIG_CMDS to run it.
       * because CONFIG_CMDS can refer to environment variables like IIROOT
       * to do thinks like a mknod we want to redirect these programs
       * off to /etc/inst/nd/mdi instead so that the mknod will fail.
       * Indeed, we must do this because the idbuild -M earlier created
       * the nodes and we don't want to overwrite them!
       * Note we run CONFIG_CMDS for each add of a device and not just on the
       * first add (where numboards == 0) as we may need to send download code
       * to this new board too.
       */

      /* IIROOT set earlier on */
      numlines=StringListNumLines(bcfgindex, N_CONFIG_CMDS);
      /* numlines is -1 if N_CONFIG_CMDS undefined in bcfg file */
      if (numlines > 0) {  
         Status(93, "Executing CONFIG_CMDS commands");
         notice("executing CONFIG_CMDS commands...");
         for (loop=1; loop <= numlines; loop++) {
            lineX=StringListLineX(bcfgindex, N_CONFIG_CMDS, loop);
            if ((lineX != NULL) && (*lineX != '\n') && (*lineX != NULL)) {
               status=runcommand(0,lineX);
               if (status == -1) {   /* not abs. pathname or got signal */
                  /* error already called at this point if not abs. pathname */
                  error(NOTBCFG,"couldn't execute CONFIG_CMDS %s", lineX);
                  free(lineX);
                  goto installfail;
               }
               /* however, it's possible/likely that CONFIG_CMDS can refer
                * to a path in /.extra.d, which won't exist past the netinstall
                * disks.  So we can't fail if CONFIG_CMDS command failed
                */
               if (status != 0) {
                  notice("ignoring non-zero return(%d) from CONFIG_CMDS cmd %s",
                         status,lineX);
               }
               free(lineX);
            } else {
               if (lineX != NULL) free(lineX);
            }
         }
      }

      /* lastly, try and open up the raw device.  If the open succeeds then
       * we're in business.  If it fails then there should be some reason
       * why it failed on the console and in osmlog.
       * Perhaps it's an ISA board that isn't in the system yet.
       * or it may require download code which hasn't been run yet...
       * Also quite possible that the Node(4) file has a /dev/mdi name
       * different from the devicename we're about to open, since we
       * generate the name in DoRemainingStuff from the driver_name and
       * the instance number..  Note that we also write the device name
       * to the info file as well (as "DEV_NAME=")
       * don't try to open if WAN device (ISDN or X25 or SERIAL) though
       */

      tmpfd = -1;  /* initial starting point, checked later */

      if ((strcmp(real_topology, "TOKEN") != 0) &&
          (lanwan == LAN) &&
          (fullrelink == 0)) {
         Status(94,"Testing device");
         tmpfd=open(devicename,O_RDONLY);
         if (tmpfd == -1) {
            notice("An open of %s failed with errno=%d(%s); check the "
                   "console and /usr/adm/log/osmlog for possible reasons",
                    devicename,errno,strerror(errno));
         } else {
            close(tmpfd);
         }

      } else {
         /* an open of a Broken Ring(tm) MDI device would attempt to 
          * attach to the ring.  This aguably shouldn't happen until 
          * the MAC_BIND_REQ occurs.  In any event,
          * this can be a time-consuming process, so we'll skip this step.
          */
         notice("not opening %s since installed topo=TOKEN or is WAN or we "
                "just did a full idbuild -B",devicename);
      }

      /* if tmpfd is still -1 then either open failed or we never tried;
       * set promiscuous mode accordingly.  NOTE:  backupkey functionality
       * is disabled at this point!
       * Note that device only has 1 chance to set PROMISC because if open
       * failed then we write 'N' to the resmgr so that if Broken ring etc.
       * we won't try to re-open it up later on when system starts in rc2.d!
       */
      if (HasString(bcfgindex, N_PROMISCUOUS, "true",0) == 1) {
         snprintf(cmd, SIZE * 2, "%s,s=Y", CM_PROMISCUOUS);
         /* write "Y" to primary key */
         if (resput(rmkey_as_string,cmd,0)) {
            goto installfail;   /* error called in resput */
         }
         /* write "Y" to backup key.  must re-do snprintf again */
         snprintf(cmd, SIZE * 2, "%s,s=Y", CM_PROMISCUOUS);
         snprintf(rmkey_as_string,10,"%d",backupkey);
         if (resput(rmkey_as_string,cmd,0)) {
            snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
            goto installfail;   /* error called in resput */
         }
         snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
      } else if (HasString(bcfgindex, N_PROMISCUOUS, "false",0) ==1) {
         snprintf(cmd, SIZE * 2, "%s,s=N", CM_PROMISCUOUS);
         /* write "N" to primary key */
         if (resput(rmkey_as_string,cmd,0)) {
            goto installfail;   /* error called in resput */
         }
         /* write "N" to backup key.  must re-do snprintf again */
         snprintf(cmd, SIZE * 2, "%s,s=N", CM_PROMISCUOUS);
         snprintf(rmkey_as_string,10,"%d",backupkey);
         if (resput(rmkey_as_string,cmd,0)) {
            snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
            goto installfail;   /* error called in resput */
         }
         snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
      } else if (tmpfd == -1) {
         /* if open failed or wrong topology type that doesn't 
          * necessarily mean we know conclusively if it supports 
          * promiscuous or not (perhaps download code didn't run?).
          * write as '?' for later
          */
         snprintf(cmd, SIZE * 2, "%s,s=?", CM_PROMISCUOUS);
         /* write "?" to primary key */
         if (resput(rmkey_as_string,cmd,0)) {
            goto installfail;   /* error called in resput */
         }
         /* write "?" to backup key.  must re-do snprintf again */
         snprintf(cmd, SIZE * 2, "%s,s=?", CM_PROMISCUOUS);
         snprintf(rmkey_as_string,10,"%d",backupkey);
         if (resput(rmkey_as_string,cmd,0)) {
            snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
            goto installfail;   /* error called in resput */
         }
         snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
      } else {
         /* open succeeded so device could support promiscuous mode.  Go find
          * out and set PROMISCUOUS in the resmgr one way or the other.
          * ODI and DLPI drivers will fall in this code path
          */
         if (determinepromfromkey(rmkey, backupkey, 1) != 0) {
            error(NOTBCFG,"idinstall: determinepromfromkey failed");
            goto installfail;   /* error called in determinepromfromkey */
         }
      }
   } else if ((netisl == 0) && (bflag == 0)) {
      notice("idinstall: not doing idbuild and loading driver");
      if (numpatches != 0) {
         notice("idinstall: not doing PATCH custom parameters; must reboot");
      }
      if (StringListNumLines(bcfgindex, N_CONFIG_CMDS) > 0) {
         notice("idinstall: not runnning CONFIG_CMDS; may need to reboot!");
      }
      notice("idinstall: can't open up device to test since we didn't idbuild");

      notice("idinstall: setting PROMISCUOUS to '?' - use determineprom cmd "
             "to set");
      /* we didn't idbuild things so we can't open up device to see if 
       * it has promiscuous mode support.  add '?' to resmgr
       */
      snprintf(cmd, SIZE * 2, "%s,s=?", CM_PROMISCUOUS);
      /* write "?" to primary key */
      if (resput(rmkey_as_string,cmd,0)) {
         goto installfail;   /* error called in resput */
      }
      /* write "?" to backup key.  must re do snprintf again */
      snprintf(cmd, SIZE * 2, "%s,s=?", CM_PROMISCUOUS);
      snprintf(rmkey_as_string,10,"%d",backupkey);
      if (resput(rmkey_as_string,cmd,0)) {
         snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
         goto installfail;   /* error called in resput */
      }
      snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
   }

   if (netisl == 1) {
      if (modlist[0] == '\0') {
         error(NOTBCFG,"idinstall with netisl == 1: no modules!");
      } else {
         /* unload old DLMs.  since it might fail we use runcommand not docmd 
          * in actuality it had better fail since we're a netinstall and
          * driver shouldn't exist in mod.d
          */
         snprintf(cmd,SIZE * 2,"/sbin/modadmin -U %s",modlist);
         runcommand(0,cmd);  /* can fail if they don't exist hence runcommand */
         /* copy DLM into mod.d */

         /* do a mknod /dev/whatever c 7 72 -- note that whatever
          * is determined elsewhere and can be /dev/mdi/foo or /dev/foo_0
          */

         /* add a line "5:1:<drivername>:72" to /etc/conf/mod_register
          * and run idmodreg -f on that file.
          * 5=STREAMS character driver
          * 1="registration"
          * 72=module-data=major number for driver
          */

         /* and now load new DLM (list in ODI and MDI case) from mod.d */
         snprintf(cmd,SIZE * 2,"/sbin/modadmin -l %s",modlist);
         if ((status=docmd(0,cmd)) != 0) {
            /* this will fail if you don't have P_LOADMOD priviledge */
            error(NOTBCFG,"problems loading new DLMs: %s",modlist);
            goto installfail;
         };
      }
   }

   if (netisl == 0) {
      /* we've added MODNAME, done the idbuild, loaded and opened the device
       * successfully, so make future calls to resshowunclaimed skip this
       * device by adding CM_NETCFG_ELEMENT.  resshowunclaimed skips anybody
       * that has CM_NETCFG_ELEMENT set to take care of DDI8 suspended drivers
       * so we can only add CM_NETCFG_ELEMENT when we know we don't panic
       * leaving MODNAME around.   Won't call DDI8 CFG_MODIFY because 
       * driver isn't in suspended state 
       * element previously set to "%s,s=net%d",CM_NETCFG_ELEMENT,netX
       *                        or "%s,s=%s",CM_NETCFG_ELEMENT,device
       */
      /* write CM_NETCFG_ELEMENT to primary key */
      strncpy(cmd, element, SIZE * 2);
      if (resput(rmkey_as_string,cmd,0)) {
         goto installfail;   /* error called in resput */
      }
      /* now write CM_NETCFG_ELEMENT to backup key.  must call strncpy again
       * as initial call to resput put a null where the = sign is
       */
      strncpy(cmd, element, SIZE * 2);
      snprintf(rmkey_as_string,10,"%d",backupkey);
      if (resput(rmkey_as_string,cmd,0)) {
         snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */
         goto installfail;   /* error called in resput */
      }
      snprintf(rmkey_as_string,10,"%d",rmkey);  /* leave it as it was */

      /* we've added ELEMENT and PROMISCUOUS to resmgr, so must run 
       * idconfupdate again.  we do so here because the call to isaautodetct
       * could conceivably hang the system if driver has bugs in writing back
       * to EEPROM/NVRAM.
       */
      snprintf(cmd,SIZE * 2, "/etc/conf/bin/idconfupdate -f");
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"idinstall: command idconfupdate failed");
         goto installfail;
      }
   }

#ifdef NEVER
  causes checksum failures
   /* tell installf we're done adding files stuff.  must happen before
    * idconfupdate because idconfupdate tweaks the sdevice.d files
    */
   snprintf(cmd,SIZE * 2,"/usr/sbin/installf -f nics");
   if ((status=docmd(0,cmd)) != 0) {
      error(NOTBCFG,"problems running installf -f nics");
      goto installfail;
   };
#endif

   if (HasString(bcfgindex,N_REBOOT,"true",0) == 1) {
      notice("setting reboot to Y because bcfg says REBOOT=true");
      reboot="Y";
   } 

   if (fullrelink == 1) {
      fullrelinkstr="Y";
      reboot="Y";
   }

   if ((netisl == 0) && (isainstall == 1)) {
      /* automatically set the eeprom/nvram on the board to match
       * what was fed as arguments to this routine (assuming the
       * board has a verify routine)
       * NOTE:  if bflag is not set then we haven't does an idbuild -M
       *        even if bflag IS set we may need to do a full idbuild -B
       *        In either of these cases driver isn't available, so trying to
       *        call its verify routine in isaautodetect is useless
       */
      Status(95, "Programming NVRAM/EEPROM settings on card");
      tmpstatus=isaautodetect(0,bcfgindex, rmkey, primitive);
      if (tmpstatus == -1) {
         error(NOTBCFG,"idinstall: isaautodetect returned error");
         goto installfail;
      }
      /* run idconfupdate (again) in case verify routine calls cm_AT_putconf
       * to change things in the resmgr.  See extensive comment in 
       * wdn driver wdn_verify()- end of MDI_ISAVERIFY_SET mode for reasons.
       * Basically, a modadmin -l -> fooload->fooinit->foo reading firmware
       * and calling cm_AT_putconf based on firmware values will mess up
       * up resmgr, and above call to isaautodetect should have fixed things
       * back to normal in the resmgr, so we save corrected version back on 
       * disk
       */
      snprintf(cmd,SIZE * 2, "/etc/conf/bin/idconfupdate -f");
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"idinstall: command idconfupdate failed "
                       "for isaautodetect");
         goto installfail;
      }
   }

   Status(96,"Device successfully added to system");

   StartList(12,"STATUS",9,"YOURUNIDBUILD",4,
                "FULLRELINKREQUIRED",11,"REBOOTREQUIRED",7,
                "IDBUILDARGS",80,"MODULES",80);
   if (bflag) {
      /* if a full idbuild was necessary we already did idbuild -B above 
       * we add DLM and modlist for informational purposes only --
       * you don't need to do anything with them
       * added the "?" because of #if 0 commenting out the idbuild -B
       */
      AddToList(6,"success","?",fullrelinkstr,reboot,DLM,modlist);
   } else {
      if (modlist[0] == '\0') {
         /* no need to idbuild -M anything -- no modules added */
         AddToList(6,"success","N",fullrelinkstr,reboot,DLM,modlist);
      } else {
         AddToList(6,"success","Y",fullrelinkstr,reboot,DLM,modlist);
      }
   }
   EndList();

   /* ok to print driver messages again.  */
   mdi_printcfg(1);

   /* if we malloced space for NETX scope custom parameters (idtunes)
    * free it here
    */
   if (numtunes > 0) {
      for (loop=0; loop<numtunes; loop++) {
         free(tunes[loop]);
      }
   }

   putenv("__NDCFG_NOPOLICEMAN=0");

   /* change delimiter for future RMputvals back to what it was before */
   delim=olddelim;

   return(0);
   /* NOTREACHED */

installfail:

   notice("idinstall: an error occurred.  Cleaning up...");
   Status(97, "An error occurred while installing the device");

   /* handle RM_ON_FAILURE.   While niccfg only allowed one thing (a directory)
    * we allow multiple lines which can be either files or directories  
    * XXX TODO:  call removef nics on these files too. 
    */
   if ((do_rm_on_failure == 1) && (bcfgindex != (u_int) -1)) {
      numlines=StringListNumLines(bcfgindex, N_RM_ON_FAILURE);
      if (numlines > 0) { /* -1 means RM_ON_FAILURE is undefined in bcfg file */
         notice("removing RM_ON_FAILURE files...");
         for (loop=1; loop <= numlines; loop++) {
            lineX=StringListLineX(bcfgindex, N_RM_ON_FAILURE, loop);
            if ((lineX != NULL) && (*lineX != '\n') && (*lineX != NULL)) {
               snprintf(cmd, SIZE * 2, "/bin/rm -rf %s",lineX);
               status=runcommand(0,cmd);   /* since files may not exist */
               if (status != 0) {
                  /* quite possible we never got far enough to run PRE_SCRIPT
                   * to put files in place.  don't call error if they don't 
                   * exist when trying to remove them
                   */
                  notice("problem removing file/directory %s - not there?",
                         lineX);
               }
               free(lineX);
            } else {
               if (lineX != NULL) free(lineX);
            }
         }  /* for each RM_ON_FAILURE line in .bcfg file */
      }  /* if RM_ON_FAILURE exists in .bcfg file */
   }  /* if it's ok to do rm_on_failure functionality */

   /* ok to print driver messages again.  */
   mdi_printcfg(1);

   putenv("__NDCFG_NOPOLICEMAN=0");
   /* if we got so far as to set the removedlpimdi variable to 1 before
    * our error, then we must remove the line or field from this file
    * only set when adding an MDI driver.
    */

   if (removedlpimdi) {
      snprintf(dlpimdifile,SIZE,"%s/dlpimdi",DLPIMDIDIR);

      /* if dlpimdifile exists then update delete appropriate line or field */
      if ((stat(dlpimdifile,&sb) != -1) && S_ISREG(sb.st_mode)) {

         /* remove slink 05dlpi lines too -- not needed according to davided */

         /* if failover is 0 then remove entire line from dlpimdi file.
          * Yes, if there was a failover device previously configured
          * it's up to netconfig to issue a idremove of *THAT* device
          * first before this one
          */
         if (failover == 0) {
            /* remove entire line from dlpimdi file */
            snprintf(cmd,SIZE * 2,"/usr/bin/ed -s %s <<!\n"
                        "/^net%d:\n"
                        "d\n"
                        "w\n"
                        "q\n"
                        "!\n",
                    dlpimdifile,netX);
            status=docmd(0,cmd);
            if (status != 0) {
               error(NOTBCFG,"couldn't remove net%d from dlpimdi file",netX);
               /* don't return yet; continue on */
            }
         } else {
            /* if failover is 1 remove just remove the failover field 
             * from dlpimdi file 
             */
            snprintf(cmd,SIZE * 2,
                     "/usr/bin/sed -e 's/:net%d:/::/g' %s> %s+;/bin/mv %s+ %s",
                     netX,dlpimdifile,dlpimdifile,dlpimdifile,dlpimdifile);
            status=docmd(0,cmd);
            if (status != 0) {
               error(NOTBCFG,"couldn't remove net%d from dlpimdi file",netX);
               /* don't return yet; continue on */
            }
         }
      }
   }

   /* if we got far enough to create a key, then we need to delete the key
    * (ISA) or remove associated information in resmgr (PCI/EISA/MCA).
    * failure to do this means next time the IRQ/IOADDR/MEMADDR appears
    * to be taken when we try and install the card.  The MODNAME isn't
    * typically set if we get here since we set that last, but IsParam
    * and other programs may think the parameter is taken.
    * this used to be in DoResmgrStuff/DoDSPStuff/DoExtraDSPStuff but that
    * didn't catch things like idbuild/idconfupdate/etc. failures so we
    * moved the call to here instead.
    * BUT:  only remove CM_MODNAME if we actually assigned it.
    */
   if ((rmkey != RM_KEY) && (ok_to_ResmgrDelInfo == 1)) {
      (void) ResmgrDelInfo(rmkey, delmodname, delbindcpu, delunit, delipl);
   }

   /* if we got far enough to create a backup key, delete it.  don't care
    * if routine ResmgrDelKey fails.
    */
   EndBackupKey(NULL); /* turn off backups if they were on, don't set MODNAME */
   if (backupkey != RM_KEY) {
      (void) ResmgrDelKey(backupkey);   /* delete backupkey entirely */
   }

   /* if MDI driver and
    * we have the netX number and
    * we got far enough to create a netX driver in the link kit
    * then delete the driver.
    */
   if ((bcfgindex != (u_int) -1) && 
       (HasString(bcfgindex,N_TYPE,"MDI",0) == 1) &&
       (netX != -1) &&
       (removenetX == 1)) {
      snprintf(cmd,SIZE * 2,"/etc/conf/bin/idinstall -P nics -d net%d",netX);
      status=docmd(0,cmd);
      if (status != 0) {
         error(NOTBCFG,"couldn't idinstall -d net%d from link kit",netX);
         /* don't return yet; continue on */
      }
   }

   /* if LAN and we got far enough to run netinfo then we must remove it */
   if (callednetinfo == 1) {
      /* netinfoname was set to proper device name earlier on */
      snprintf(cmd,SIZE * 2,"/usr/sbin/netinfo -r -d %s",netinfoname);
      (void) runcommand(0,cmd);   /* we don't care if cmd succeeds or fails */
   }

   /* if we possibly created any files then attempt to remove them.
    * this can fail if we got far enough to snprintf the path name but
    * not far enough to create the actual file but so what...
    */
   if (strlen(infopath) > 0) (void) unlink(infopath);
   if (strlen(initpath) > 0) (void) unlink(initpath);
   if (strlen(removepath) > 0) (void) unlink(removepath);
   if (strlen(listpath) > 0) (void) unlink(listpath);
   if (strlen(reconfpath) > 0) (void) unlink(reconfpath);
   if (strlen(controlpath) > 0) (void) unlink(controlpath);

   /* if we malloced space for NETX scope custom parameters (idtunes)
    * free it here
    */
   if (numtunes > 0) {
      for (loop=0; loop<numtunes; loop++) {
         free(tunes[loop]);
      }
   }

   /* TODO XXX:
    * - run POST_SCRIPT if we ran PRE_SCRIPT?
    * - if we did an idtune -f -c customX actualvalue for DRIVER/GLOBAL custom
    *   params then undo them
    * - if we did an idtune to change ODIMEM_NUMBUF or ODIMEM_MBLK_NUMBUF then
    *   undo them
    * - if we installed DEPEND= drivers we should remove each of them
    * - If ODI driver and we did idtune's for odimem then undo them here
    * - if ODI and TOKEN and we installed toktsm and/or odisr then remove them
    * - if ODI and FDDI  and we installed fdditsm and odisr then remove them.
    * - if ODI and ETHER and we installed ethtsm then remove it
    * - if ODI and we installed msm, lsl, or odimem then remove them
    * - if MDI and we installed dlpi and/or dlpibase them remove??? maybe not..
    * - if we did an idinstall of the driver DSP then remove it
    */

   /* change delimiter for future RMputvals back to what it was before */
   delim=olddelim;

   return(-1);
}

/* show ttys available for WAN type connections.  We only show things in
 * the /dev/term directory for now.  For a better way, see
 * ttysrch(4), ttymap(1M), and ttyname(3C).   Unfortunately, we
 * can't walk through Drvmaps looking for "Communications Cards" (which is
 * what the dcu keys from) because ISA boards won't have them and most
 * PCI/EISA/MCA 3rd party serial drivers don't come with Drvmap files either.
 * We generally want to exclude pseudo tty devices.  That is, /dev/pts???, 
 * /dev/pts/*, and /dev/ttyp* (the SCO compat versions of /dev/pts/*).
 * Since the COM1-COM4 devices are mirrored in /dev, we just stick with
 * /dev/term for now, which at least 1 3rd party serial driver also 
 * populates.
 */
void
showserialttys(void)
{
   DIR *dirp;
   struct dirent *direntp;
   u_int numterms=0;
   char name[SIZE];
       
   StartList(2,"TERMNAME",20);

   dirp=opendir("/dev/term");
   if (dirp == NULL) goto noterms;
   while (((direntp = readdir(dirp)) != NULL) ) {
      if ((strcmp(direntp->d_name,".") == 0) ||
          (strcmp(direntp->d_name,"..") == 0)) continue;
      strcpy(name,"/dev/term/");
      strcat(name,direntp->d_name);
      AddToList(1,name);
      numterms++;     
   }
   closedir(dirp);

   if (numterms == 0) goto noterms;

   EndList();
   return;
   /* NOTREACHED */

noterms:

   if (tclmode) {
      strcat(ListBuffer,"{ }");
   } else {
      error(NOTBCFG,"there are no serial devices available");
   }
   EndList();
}

/* translate the supplied path name into a bcfg index.  essential when
 * we install a driver then add more drivers via IHV diskette or remove
 * drivers (actually bcfgs) in HIERARCHY, causing ndcfg to skew the
 * the driver to a new bcfgindex.  this routine tells you the new index
 * to use for subsequent operations that require a bcfgindex so that
 * you continue to use the same driver
 * can print -1 if you do a loaddir/loadfile then idinstall, quit
 * ndcfg, but next time you start ndcfg you forget to do the loaddir again.
 *
 * Always returns first match in bcfgfile[] with supplied path name
 *
 * routine returns -1 if not found else actual index in current bcfgfile[]
 */
int
bcfgpathtoindex(int dolist,char *path)
{
   struct stat sb;
   u_int bcfgloop;

   if (dolist == 1) {
      StartList(2,"INDEX",5);
   }

   if (*path != '/') {
      error(NOTBCFG,"bcfgpathtoindex: path name '%s' must be absolute",path);
      return(-1);   /* EndList is called automatically in cmdparser.y */
   }

   /* did user delete the DSP and the bcfg where driver came from? */
   if (stat(path,&sb) == -1) {
      notice("bcfgpathtoindex: path name '%s' doesn't exist",path);
      if (dolist == 1) {
         AddToList(1,"-1");    /* bcfg file no longer exists */
         EndList();
      }
      return(-1);
   }

   /* sanity check */
   if (!S_ISREG(sb.st_mode)) {
      notice("bcfgpathtoindex: path name '%s' isn't a regular file",path);
      if (dolist == 1) {
         AddToList(1,"-1");    /* treat as if it no longer exists */
         EndList();
      }
      return(-1);
   }

   for (bcfgloop=0; bcfgloop< bcfgfileindex; bcfgloop++) {
      if (strcmp(bcfgfile[bcfgloop].location,path) == 0) {
         char loopasstr[20];

         if (dolist == 1) {
            snprintf(loopasstr,20,"%d",bcfgloop);
            AddToList(1,loopasstr);
            EndList();
         }
         return(bcfgloop);
      }
   }
   if (dolist == 1) {
      AddToList(1,"-1");   /* not found */
      EndList();
   }
   return(-1);
}

/* have we reached the maximum number of boards for this modname
 * in the resmgr?
 * returns -1 for error
 * returns  0 if not at limit
 * returns  1 if at (or above) limit
 * Remember DefineV0Defaults() sets MAX_BD to 4 for v0 drivers!
 */
int
AtMAX_BDLimit(int bcfgindex)
{
   char *maxbdstr,*driver_name;
   u_int maxbd;
   int numboards;
   char *strend;

   maxbdstr=StringListPrint(bcfgindex,N_MAX_BD);

   if (maxbdstr == NULL) {   /* not defined, so no limit.  return 0 */
      return(0);
   }
   /* MAX_BD is defined for this bcfg file */

   if (((maxbd=strtoul(maxbdstr,&strend,10))==0) &&
        (strend == maxbdstr)) {
      /* shouldn't happen; EnsureNumeric takes care of this */
      notice("AtMAX_BDLimit: invalid MAX_BD number for bcfgindex %d:%s",
             bcfgindex,maxbdstr);
      free(maxbdstr);
      return(-1);
   }

   driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
   if (driver_name == NULL) {
      /* shouldn't happen either */
      error(NOTBCFG,"AtMAX_BDLimit: DRIVER_NAME not set for bcfgindex %d",
             bcfgindex);
      free(maxbdstr);
      return(-1);
   }

   numboards = ResmgrGetNumBoards(driver_name);
   if (numboards == -1) {
      error(NOTBCFG,"AtMAX_BDLimit: ResmgrGetNumBoards returned -1");
      free(maxbdstr);
      free(driver_name);
      return(-1);
   }
   /* numboards is 0 if first add of driver and doesn't exist yet in resmgr 
    * MAX_BD is 0-based, so if driver only supports 4 boards, then MAX_BD
    * should be set to 3.  This is for backwards compatability
    */
   if (numboards > maxbd) {
      notice("AtMAX_BDLimit: %s at bcfg MAX_BD(%d); returning 1",
          driver_name,maxbd);
      free(maxbdstr);
      free(driver_name);
      return(1);
   }
   /* MAX_BD is defined but we're still within limit */
   free(maxbdstr);
   free(driver_name);
   return(0);
}

/* translate an NETCFG_ELEMENT to the appropriate bcfg index
 * returns bcfg index or -1 if cannot be found.
 */
int
elementtoindex(int dolist,char *element)
{
   rm_key_t rmkey;
   char bcfgpath[SIZE];
   int bcfgindex;

   bcfgpath[0]='\0';

   if (element == NULL) {
      error(NOTBCFG,"elementtoindex: bad element of NULL");
      return(-1);
   }
   rmkey=resshowkey(0,element,bcfgpath,0);/* calls OpenResmgr/CloseResmgr */

   if (rmkey == RM_KEY) {   /* failure in resshowkey() */
      notice("elementtoindex: couldn't find element %s in resmgr - trying "
             "the backup", element);
      /* try the backup key, since all we want is bcfgpath */
      rmkey=resshowkey(0,element,bcfgpath,1);/* calls OpenResmgr/CloseResmgr */
   }

   if (rmkey == RM_KEY) {
      /* not at regular key and not at backup -- now we got a problem */
      notice("elementtoindex: couldn't find element %s in resmgr",
                    element);
      return(-1);
   }
   if (bcfgpath[0] != '/') {
      error(NOTBCFG,"elementtoindex: invalid BCFGPATH '%s' at rmkey %d",
            bcfgpath,rmkey);
      return(-1);
   }
   /* now find out the bcfgindex for this bcfgpath.  Note that this 
    * can fail if the user deleted the driver from the system in
    * its original directory (/etc/inst/nics/drivers, /etc/inst/nd/mdi)
    * after doing the original install
    */
   bcfgindex=bcfgpathtoindex(0,bcfgpath);   
   if (bcfgindex == -1) {
      /* uhoh, it's gone.  can't proceed further.  Worse, we can't
       * check bcfg.d ourselves since a loadfile() will fail since
       * the directory doesn't have DSP components. the best we can
       * by telling user to restore it themselves to the path name
       * specified.
       * alternate is that it's not an absolute pathname, which indicates
       * that someone has been messing with BCFGPATH in the resmgr
       */
      notice("elementtoindex: bcfg file '%s' for element '%s' is "
            "missing -- restore file to continue(check /etc/conf/bcfg.d)", 
            bcfgpath,element);
      return(-1);
   }
   if (dolist == 1) {
      char indexasstr[10];

      StartList(2,"INDEX",6);
      snprintf(indexasstr,10,"%d",bcfgindex);
      AddToList(1,indexasstr);
      EndList();
   }
   return(bcfgindex);
}

/* modify the existing board on the system 
 * UNKNOWN:  How will this have impact in DDI8 land?
 * XXX how do I invoke a CFG_MODIFY_INSTANCE from here?
 * returns 0 for success
 * returns -1 for error
 * ISA: you must not supply the OLDIOADDR parameter to this command
 *      because of how we pass the primitive argument to isaautodetect
 *
 */
int
idmodify(union primitives primitive) 
{
   rm_key_t rmkey, backupkey, skipkey, keyinuse;
   int numargs,status,fullrelink=0, reboot=0;
   unsigned long newvalue;
   stringlist_t *slp;
   int bcfgindex;
   int loop, ioavail, ec;
   int numcustomparams;
   char *customX=NULL, *strend;
   char *irq, *dma, *mem, *port;
   char *driver_name;
   char tmp[SIZE];
   char niccustparams[SIZE];
   char cmd[SIZE];
   char bcfgpath[SIZE];
   char key[10];
   char bcfgindexstr[10];
   char *element, *customtopo;
   char real_topology[VB_SIZE];
   char currentirq[VB_SIZE];
   char currentioaddr[VB_SIZE];
   char currentmemaddr[VB_SIZE];
   char currentdmac[VB_SIZE];
   char scope[VB_SIZE];
   char olddelim=delim;
   extern int bflag;

   delim='\1';   /* ASCII 1 = SOH, an unlikely character */

   bcfgpath[0]='\0';
   /* since we run idconfupdate at the end which needs to to link kit things,
    * we call common ensureprivs routine.  Side note:  you can open
    * up the resmgr as a normal user if you have P_DACREAD or P_DACWRITE 
    * permission which overrides the normal checks
    */
   if (ensureprivs() == -1) {
      error(NOTBCFG,
            "idmodify: insufficient priviledge to perform this command");
      goto fail;
   }

   if (primitive.type != STRINGLIST) {/* programmer error from cmdparser.y */
      error(NOTBCFG,"idmodify: arg isn't a stringlist!");
      goto fail;
   }

   slp=&primitive.stringlist;
   numargs=CountStringListArgs(slp);
   if (numargs < 2) {
      error(NOTBCFG,"idmodify requires at least 2 arguments");
      goto fail;
   }

   /* first argument is element */
   element=slp->string;
   bcfgindex = elementtoindex(0,element);
   if (bcfgindex == -1) {
      /* notice already called */
      error(NOTBCFG,"idmodify: elementtoindex failed");
      goto fail;
   }
   rmkey=resshowkey(0,element,NULL,0);/* calls OpenResmgr/CloseResmgr */
   backupkey=resshowkey(0,element,NULL,1);  /* calls OpenResmgr/CloseResmgr */

   skipkey = rmkey;   /* could be real key or RM_KEY, doesn't matter */

   if ((rmkey == RM_KEY) && (backupkey == RM_KEY)) {   /* "Major problem" */
      error(NOTBCFG,"idmodify: couldn't find element %s in resmgr",element);
      goto fail;
   }

   if (rmkey == RM_KEY) {   /* failure in resshowkey() - key removed */
      notice("idmodify: couldn't find element %s in resmgr -- using "
                    "backup parameter instead", element);
      /* don't call StartBackupKey because the primary key wasn't found so
       * we will modify the backup key instead in this routine and reassign
       * card parameters to the board when they re-insert it in the system.
       * all resput calls in this routine will not be mirrored to
       * the backup key but go directly to the backup key.
       * we know backup key is good to get here
       */
      snprintf(key,10,"%d",backupkey);
      keyinuse = backupkey;
   } else {
      /* the primary key is good and the backup key may or may not be good too 
       * any modifications to true key should be reflected at the backup key 
       * ...but backup keys are now only found with MDI drivers
       * only start backup key if it's good
       */
      if ((backupkey != RM_KEY) && 
          (HasString(bcfgindex,N_TYPE, "MDI", 0) == 1)) {
         StartBackupKey(backupkey);
      }
      snprintf(key,10,"%d",rmkey);
      keyinuse = rmkey;
   }

   slp=slp->next;

   /* see if the user wants to modify any CUSTOM parameters
    * We can only have 9 CUSTOM variables max.  we use showcustomnum 
    * which is a better
    * way of knowing exactly how many we have
    * we then get the variable name, then look for it as an argument
    */
   snprintf(bcfgindexstr,10,"%d",bcfgindex);
   numcustomparams=showcustomnum(0,bcfgindexstr);
   if (numcustomparams == -1) goto fail; /* error called in showcustomnum */
   if (numcustomparams == 0) goto aftermodifycustom;/* no CUSTOM[x] entries */
   niccustparams[0]='\0';

   snprintf(tmp,SIZE,"%s,s",CM_TOPOLOGY);
   ec=ResmgrGetVals(keyinuse,tmp,0,real_topology,VB_SIZE);
   if (ec != 0) {
      error(NOTBCFG,
         "idmodify: ResmgrGetVals for TOPOLOGY returned %d",ec);
      goto fail;
   }

   /* the idea is to go through all custom parameters, figure out what
    * the resmgr parameter is for that CUSTOM[] entry, then see if the
    * user supplied that as an argument to the idmodify command.  If
    * they did, then change the entry in the resmgr
    */
   for (loop=1;loop <= numcustomparams; loop++) {
      char customparamstr[20];
      char valueequal[SIZE];
      char *value;
      char *firstspace;
      int offset;

      snprintf(customparamstr,20,"CUSTOM[%d]",loop);
      offset=N2O(customparamstr);
      customX=StringListPrint(bcfgindex,offset);
      if (customX == NULL) {
         /* shouldn't happen */
         error(NOTBCFG,"couldn't find %s out of %d CUSTOM parameters",
               customparamstr,numcustomparams);
         goto fail;
      }
      firstspace=strchr(customX,' ');
      if (firstspace == NULL) {
         /* shouldn't happen */
         error(NOTBCFG,"idmodify: couldn't find space in %s out of %d:%s",
               customparamstr,numcustomparams,customX);
         goto fail;  /* don't continue */
      }
      *firstspace='\0';
      /* now customX points to the parameter to modify in the resmgr.  Lets
       * also look for it in our parameter list to idmodify cmd.
       * if the user didn't supply it, don't error like we do in DoResmgrStuff
       */
      snprintf(valueequal,SIZE,"%s=",customX);
      if ((value=FindStringListText(slp, valueequal)) == NULL) {
         /* user doesn't want to modify this CUSTOM[] parameter */
         continue;
      }

      /* occasionally netcfg slips up and gives us this instead of __STRING__ */
      if (strcmp(value, "\\\"\\\"") == 0) {
         value="__STRING__";
      }

      /* just because it's on the command line doesn't mean that it's
       * legal to change it: the parameter might only be for ETHER but yet
       * we installed as TOKEN.  Must look at single valued CM_TOPOLOGY 
       * for answer
       */
      if (bcfgfile[bcfgindex].version == 1) {
         if ((StringListNumLines(bcfgindex, offset) != 9) &&
             (StringListNumLines(bcfgindex, offset) != 10)) {
            error(NOTBCFG,"idmodify: bad # lines in bcfg file for '%s'",
                  customX);
            goto fail;
         }
         /* line 8 in version 1 bcfg files has applicable topologies */
         customtopo=StringListLineX(bcfgindex, offset, 8);
         if (customtopo == NULL) {
            error(NOTBCFG,"idmodify: invalid line 8 custom param %s",customX);
            goto fail;
         }

         if (strstr(customtopo, real_topology) == NULL) {
            /* user has specified a custom parameter on the command line that
             * isn't applicable to the topology that the driver 
             * was installed with.
             */
            notice("idmodify: ignoring custom param '%s=' since not applicable"
                   "to installed topology %s",customX, real_topology);
            free(customtopo);
            continue;
         }
         free(customtopo);
      }


      if (GetParamScope(keyinuse, customX, scope) != 0) {
         error(NOTBCFG,"idmodify: GetParamScope failed");
         goto fail;
      }
      strtoupper(scope);
      /* note that scope could be '-' if parameter is undefined in resmgr */

      if ((strcmp(scope,"DRIVER") == 0) ||
          (strcmp(scope,"GLOBAL") == 0)) {
         if (strcmp(value, "__STRING__") == 0) {
            notice("skipping DRIVER/GLOBAL scope custom; value is __STRING__");
         } else {
            /* because we want changes reflected in next idbuild -M, use -c */
            snprintf(tmp,SIZE,"/etc/conf/bin/idtune -f -c %s %s",
                     customX, value);
            (void) runcommand(0,tmp); /* don't care if cmd succeeds or fails */
            if (strcmp(scope,"DRIVER") == 0) {
               /* note we don't (indeed, in most cases we can't) unload driver 
                * so we say to reboot machine instead.  driver scopes do an
                * idtune on that particular driver -- it's up to the driver
                * to have a Space.c which will pick up the change
                */
               driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME); 
               if (driver_name != NULL) {
                  snprintf(cmd,SIZE,"/etc/conf/bin/idbuild -M %s",driver_name);
                  free(driver_name);
                  status=runcommand(0, cmd);
                  notice("setting reboot to 1 because of DRIVER custom %s",
                           customX);
                  reboot=1;
               } else {
                  /* shouldn't happen */
                  notice("no driver_name, DRIVER scope, fullrelink now 1");
                  fullrelink=1;
               }
            } else {
               /* globals can do an idtune on any parameter so we must 
                * do full relink since we don't know the affected driver(s)
                */
               notice("setting fullrelink to 1 because of GLOBAL custom %s",
                        customX);
               fullrelink=1;
            }
         }
      }

      if (strcmp(scope,"NETX") == 0) {
         if (HasString(bcfgindex,N_TYPE,"MDI",0) == 1) {
            if (strcmp(value,"__STRING__") == 0) {
               /* if someone makes param a __STRING__ in the .bcfg file
                * and the user just hits return and doesn't fill in anything
                * then we should skip it and use the default in the Mtune file
                * occasionally netcfg slips and hands us FOO=\"\" instead of
                * FOO=__STRING__
                */
               notice("skipping NETX scope custom %s since value is %s",
                      customX, value);
            } else {
               /* NETX scope parameters are all numbers and not text strings
                * at all (or multiple words.  If we cannot convert the
                * string into a valid base 10 number then skip it; the
                * user entered a bogus string or the .bcfg file is bad
                */
               unsigned long foo;
               char *foop;

               if (((foo=strtoul(value,&foop,10))==0) &&
                     (foop == value)) {
                  notice("skipping NETX scope custom %s: bad number %s",
                         customX, value);
               } else {
                  char netXstr[VB_SIZE];

                  if (ResmgrGetNetX(keyinuse, netXstr) != -1) {
                     strtoupper(netXstr);
                     snprintf(tmp,SIZE,"/etc/conf/bin/idtune -f -c %s%s %s",
                              netXstr, customX, value);
                     (void) runcommand(0,tmp);/* don't care if succeeds/fails */
                     strtolower(netXstr);
                     snprintf(tmp,SIZE,"/etc/conf/bin/idbuild -M %s",netXstr);
                     (void) runcommand(0,tmp);/* don't care if succeeds/fails */
                     notice("setting reboot to 1 because of NETX "
                            "scope custom '%s'", customX);
                     reboot=1;
                  } else {
                     error(NOTBCFG,"idmodify: can't find MDI_NETX in resmgr");
                     goto fail;
                  }
               }
            }
         } else {
            notice("ignoring NETX scope custom parameter for non-MDI driver");
         }
      }

      if ((strcmp(scope,"PATCH") == 0)) {
         /* patch values are base 10, not hex or octal */
         if (strcmp(value,"__STRING__") == 0) {
            notice("skipping PATCH parameter; value entered is __STRING__");
         } else {
            if (((newvalue=strtoul(value,&strend,10))==0) &&
                 (strend == value)) {
               notice("DoPatchCustom: invalid patch value %s", value);
               /* continue on to next parameter to patch*/
            } else {
               notice("idmodify: patching %s to new value %d",customX,newvalue);
               donlist(1, customX, newvalue, 0);
            }
         }
      }

      /* all CUSTOM[x] parameters are stored as _strings_ in the resmgr */
      /* supply typing for libresmgr */
      snprintf(tmp,SIZE,"%s,s=%s",customX,value);
      if (resput(key,tmp,0)) goto fail;    /* error called in resput */

      /* to get here, user did supply the custom parameter as an argument.
       * ensure the required companion customX_= argument is also
       * supplied.  crucial for showcustomcurrent command.
       * NOTE!: if user didn't supply FOO_=bar and scope is DRIVER or GLOBAL
       * or PATCH then it's too late -- we already patched /dev/kmem and/or
       * ran idtune!  XXX TODO future enhancement: change order so that
       * we don't patch kmem or run idtune if FOO=bar supplied but not FOO_=bar
       */
      snprintf(valueequal,SIZE,"%s%s=",customX,CM_CUSTOM_CHOICE);
      if ((value=FindStringListText(slp, valueequal)) == NULL) {
         /* error out -- this parameter is mandatory when any CUSTOM[] 
          * parameter is to be modified
          */
         error(NOTBCFG,"idmodify: required argument '%s%s=' not supplied "
                       "as argument to idmodify",customX,CM_CUSTOM_CHOICE);
         goto fail;
      }
      snprintf(tmp,SIZE,"%s%s,s=%s",customX,CM_CUSTOM_CHOICE,value);
      if (resput(key,tmp,0)) goto fail;    /* error called in resput */

      free(customX);
   }
aftermodifycustom:

   if ((HasString(bcfgindex, N_BUS, "PCI", 0) == 1)  ||
#ifdef CM_BUS_I2O
       (HasString(bcfgindex, N_BUS, "I2O", 0) == 1)  ||
#endif
       (HasString(bcfgindex, N_BUS, "EISA",0) == 1)  ||
       (HasString(bcfgindex, N_BUS, "MCA", 0) == 1)) {
      /* PCI/EISA/MCA */
      /* ensure none of IRQ=/DMAC=/IOADDR=/MEMADDR=/
       * OLDIRQ=/OLDDMAC=/OLDIOADDR=/OLDMEMADDR= are supplied arguments;
       * these are ISA specific
       */
      if (EnsureNotDefined(slp,"IRQ=")) goto fail;  /* found and error called */
      if (EnsureNotDefined(slp,"DMAC=")) goto fail; /* found and error called */
      if (EnsureNotDefined(slp,"IOADDR=")) goto fail;  /* ditto */
      if (EnsureNotDefined(slp,"MEMADDR=")) goto fail; /* you get the idea */
      if (EnsureNotDefined(slp,"OLDIRQ=")) goto fail;
      if (EnsureNotDefined(slp,"OLDDMAC=")) goto fail;
      if (EnsureNotDefined(slp,"OLDIOADDR=")) goto fail;
      if (EnsureNotDefined(slp,"OLDMEMADDR=")) goto fail;
   } else if (HasString(bcfgindex,N_BUS, "ISA", 0) == 1) {
      /* ISA card
       */

      /* showISAcurrent does OpenResmgr/CloseResmgr */
      if (showISAcurrent(element,0,
              currentirq,currentioaddr,currentmemaddr,currentdmac) != 0) {
         error(NOTBCFG,"idmodify - showISAcurrent failed");
         goto fail;
      }

      /* make sure we have an ISA bus on this machine before proceeding 
       * MCA boxes will fit into this category.  See confmgr/confmgr_p.c
       * comment and code
       *    if (( _cm_bustypes & CM_BUS_MCA ) == 0 )
       *       _cm_bustypes |= CM_BUS_ISA;
       */
      if ((cm_bustypes & CM_BUS_ISA) == 0) {
         error(NOTBCFG,"idmodify - can't modify ISA card as this machine "
                       "doesn't have an ISA bus!");
         goto fail;
      }

      if ((irq=FindStringListText(slp, "IRQ=")) != NULL) {
         strtolower(irq);
         /* if user gave us IRQ=2, convert it to IRQ=9. See IsParam() too */
         if (atoi(irq) == 2) irq="9";

         if (strcmp(currentirq,irq) == 0) goto trydma;

         /* since this is only for ISA cards we don't have to worry about
          * ITYPE sharing here -- existence of driver in resmgr is sufficient
          */
         status=ISPARAMAVAILSKIPKEY(N_INT, irq, skipkey);
         if (status == 1) {
            snprintf(tmp,SIZE,"%s=%s",CM_IRQ,irq);   /* no ,n needed */
            if (resput(key,tmp,0)) goto fail;  /* error called in resput */
         } else
         if (status == -1) { 
            error(NOTBCFG,"idmodify - IsParam for INT returned -1");
            goto fail;
         }
         if (status == 0) {
            error(NOTBCFG,"idmodify - IRQ=%s not available in resmgr",irq);
            goto fail;
         }
      }

trydma:

      if ((dma=FindStringListText(slp, "DMAC=")) != NULL) {
         strtolower(dma);

         if (strcmp(currentdmac, dma) == 0) goto trymemaddr;

         status=ISPARAMAVAILSKIPKEY(N_DMA, dma, skipkey);
         if (status == 1) {
            snprintf(tmp,SIZE,"%s=%s",CM_DMAC,dma);  /* no ,n needed */
            if (resput(key,tmp,0)) goto fail;  /* error called in resput */
         } else
         if (status == -1) {
            error(NOTBCFG,"idmodify - IsParam for DMA returned -1");
            goto fail;
         } else
         if (status == 0) {
            error(NOTBCFG,"idmodify - DMA=%s not available in resmgr",dma);
            goto fail;
         }
      }

trymemaddr:

      if ((mem=FindStringListText(slp, "MEMADDR=")) != NULL) {
         strtolower(mem);

         if (strcmp(currentmemaddr,mem) == 0) goto tryioaddr;

         status=ISPARAMAVAILSKIPKEY(N_MEM, mem, skipkey);
         if (status == 1) {
            char *dash;
            snprintf(tmp,SIZE,"%s=%s",CM_MEMADDR,mem);  /* no ,r needed */
            dash=strchr(tmp,'-');
            if (dash == NULL) {
               error(NOTBCFG,"syntax for MEMADDR is MEMADDR=num1-num2");
               goto fail;
            }
            *dash=delim;
            if (resput(key,tmp,0)) goto fail;  /* error called in resput */
         } else 
         if (status == -1) {
            error(NOTBCFG,"idmodify - IsParam for MEMADDR returned -1");
            goto fail;
         } else
         if (status == 0) {
            error(NOTBCFG,"idmodify - MEMADDR=%s not available in resmgr",mem);
            goto fail;
         }
      }

tryioaddr:

      /* ok, at this point we've changed all of the custom variables
       * and all other ISA parameters except for the I/O address
       * call isaautodetect set mode to change the EEPROM/NVRAM settings
       * to match the resmgr.   we want to do this here because
       * if the user is changing the i/o address we want the verify
       * routine to pull out the _old_ io address from the resmgr
       * but use the new io address stored in _IOADDR provided by 
       * _mdi_AT_verify.  If we encounter a failure in the next few
       * lines then we're hosed as the eeprom will use the new
       * address but that's unlikely to occur.
       * as it turns out isaautodetect will create a new key to 
       * program the eeprom settings based on information provided on
       * the command line.
       */

      /* to prevent confusion in isaautodetect, prevent OLDIOADDR */
      if (FindStringListText(slp, "OLDIOADDR=") != NULL) {
         error(NOTBCFG,"idmodify - the OLDIOADDR parameter cannot be supplied "
               "as an argument");
         goto fail;
      }

      /* if the driver has a verify routine and the verify routine calls
       * mdi_AT_verify to determine its mode and if the mode changes
       * and if the driver calls cm_AT_putconf then the parameter will
       * be changed below.  In particular, in the above senario, 
       * if the user wants to change the i/o address, then the range
       * will be available here but immediately after the isaautodetect
       * line the parameter will be "taken" in the resmgr, and the
       * call to ISPARAMAVAIL(N_PORT, port) below will fail.  Since this
       * is only a problem if the driver calls cm_AT_putconf, we 
       * see if the parameter is available _now_ and if it is available
       * now but changed to "not available" immediately after this call
       * then we assume that the driver called cm_AT_putconf which
       * changed it.  Note this isn't a guarantee, as another
       * driver/user program can be maniuplating another key in the resmgr
       * at the same time this code runs, adding that IOADDR range,
       * but that's rare enough to ignore it.
       */

      if ((port=FindStringListText(slp, "IOADDR=")) != NULL) {
         strtolower(port);

         status=ISPARAMAVAILSKIPKEY(N_PORT, port, skipkey);
         if (status == 1) {
            ioavail=1;   /* io address available before call to isaautodetect */
         } else {
            ioavail=0;   /* io address not avaiable before call */
         }
      }

      /* write back changes to nvram/eeprom */
      status=isaautodetect(1, bcfgindex, rmkey, primitive);
      if (status == -1) {
         error(NOTBCFG,"idmodify - the call to isaautodetect failed");
         goto fail;
      }

      /* now change IOADDR in the resmgr.  Note that the range may not be 
       * available now although it was available before call to isaautodetect
       */

      if ((port=FindStringListText(slp, "IOADDR=")) != NULL) {
         strtolower(port);

         if (strcmp(currentioaddr,port) == 0) goto afterisa;

         status=ISPARAMAVAILSKIPKEY(N_PORT, port, skipkey);
         if (status == 1) {
            char *dash;
            snprintf(tmp,SIZE,"%s=%s",CM_IOADDR,port);  /* no ,r needed */
            dash=strchr(tmp,'-');
            if (dash == NULL) {
               error(NOTBCFG,"syntax for IOADDR is IOADDR=num1-num2");
               goto fail;
            }
            *dash=delim;
            if (resput(key,tmp,0)) goto fail;  /* error called in resput */
         } else
         if (status == -1) {
            error(NOTBCFG,"idmodify - IsParam for IOADDR returned -1");
            goto fail;
         } else {
            /* io address specified on command line isn't available.  this 
             * can be due to bogus arguments to idmodify command or a 
             * driver calling cm_AT_putconf.  See if it was available a
             * few lines earlier.  Not 100% reliable, but "close enough".
             */
            if (ioavail == 1) {
               /* io address was available before our call to isaautodetect
                * to write back parameter but driver called likely called
                * cm_AT_putconf which changed it so that ndcfg deemed range 
                * "no longer available".  this is not a problem, but we'll 
                * log it.
                */
               notice("idmodify: ioaddr=%s was available but not now; driver "
                      "probably called cm_AT_putconf - ignoring", port);
               goto afterisa;
            }
            /* it wasn't available before call to isaautodetect and
             * it isn't available now.  must be bogus arguments to 
             * idmodify command.
             */ 
            if (status == 0) {
               error(NOTBCFG,"idmodify-IOADDR=%s not available in resmgr",port);
               goto fail;
            }
         }
      }

afterisa: ;


   } else {  /* not PCI/EISA/MCA/ISA   could be PCCARD or PNPISA someday...*/
      error(NOTBCFG,
        "unsupported bus type for idmodify at bcfgindex %d",bcfgindex);
      error(NOTBCFG,
        "(did you forget to set AUTOCONF=false in the bcfg file?)");
      goto fail;
   }

   /* Lastly, run idconfupdate to sync up the System file.  This is
    * also necessary as calling the verify routine may have also
    * called cm_AT_putconf, updating the resmgr values
    */
   snprintf(cmd,SIZE,"/etc/conf/bin/idconfupdate -f");
   status=docmd(0,cmd);
   if (status != 0) {
      error(NOTBCFG,"idmodify: command idconfupdate failed");
      goto fail;
   }
   if (fullrelink == 1) {
      reboot == 1;
   }
   if ((bflag != 0) && (fullrelink == 1)) {
      if (tclmode) {
         notice("idmodify: not running idbuild -B, leaving that to netcfg !");
      } else {
         snprintf(cmd,SIZE,"/etc/conf/bin/idbuild -B");
         status=runcommand(0, cmd);
         fullrelink=0;
      }
   }
   StartList(6,"STATUS",10,"FULLRELINKREQUIRED",20,"REBOOTREQUIRED",20);
   AddToList(3,"success",fullrelink == 1 ? "Y" : "N",
                         reboot     == 1 ? "Y" : "N");
   EndList();
   /* turn off backups.  note we may not be in backup mode if not MDI driver
    * Moreover, it's not necessary to change MODNAME either (since it was
    * already set in idinstall) so use NULL.  still safe to call EndBackupKey
    */
   EndBackupKey(NULL);

   /* change delimiter for future RMputvals back to what it was before */
   delim=olddelim;

   return(0);
   /* NOTREACHED */
fail:
   EndBackupKey(NULL); /* turn off backups but don't change MODNAME */
   if (customX != NULL) free(customX);

   /* change delimiter for future RMputvals back to what it was before */
   delim=olddelim;

   return(-1);
}

/* remove an instance of the driver from the system.
 * if last instance of driver and nobody else using companion resources
 * then free up companion resources (msm/toktsm/fdditsm/ethtsm/lsl/odimem/
 * odisr/dlpi module)
 * removes line from dlpimdi file if MDI
 * calls netinfo -r -d /dev/whatever
 * does NOT remove the netconfig info file
 * runs idconfupdate when done.
 * NOTE:  CreateNetcfgRemovalScript() assumes failoverstr is an u_int since
 *        that's how it is passed in as an argument to idinstall!
 */
void 
idremove(char *element, char *failoverstr)
{
   char cmd[SIZE];
   char tmpcmd[SIZE];
   char dlpimdifile[SIZE];
   char driver_type[VB_SIZE];
   char depend[VB_SIZE];
   char modname[VB_SIZE];
   char netXstr[VB_SIZE];
   char devdevice[VB_SIZE];
   u_int driver_count,failover;
   int driver_type_count, numlines;
   int status;
   struct stat sb;
   char *strend, *lineX;
   rm_key_t rmkey=resshowkey(0,element,NULL,0); /* OpenResmgr/CloseResmgr */
   rm_key_t backupkey=resshowkey(0,element,NULL,1); /* calls OpenR/CloseR */
   int bcfgindex=elementtoindex(0,element); /* OpenResmgr/CloseResmgr */
   char *fullrelink;
   extern int bflag;

   /* since we call idinstall -d and idconfupdate, ensure good privs */
   if (ensureprivs() == -1) {
      error(NOTBCFG,
            "idremove: insufficient priviledge to perform this command");
      return;
   }

   fullrelink="N";
   /* avoid atoi */
   if (((failover=strtoul(failoverstr,&strend,10))==0) &&
        (strend == failoverstr)) {
      error(NOTBCFG,"invalid failover number %s",failoverstr);
      return;
   }

   if ((failover != 0) && (failover != 1)) {
      error(NOTBCFG,"failover must be 0 or 1");
      return;
   }

   if (bcfgindex == -1) {
      /* notice already called.  don't call error and return as user may have 
       * deleted the /etc/inst driver.   continue on through routine but
       * just don't do the POST_SCRIPTs later on in this routine.
       */
      notice("idremove: could not find NETCFG_ELEMENT in resmgr or "
                    "driver DSP removed from /etc/inst");
   }

   /* since resshowkey searches for NETCFG_ELEMENT in resmgr we can be
    * reasonably confident that we found our network card
    */
   if (rmkey == RM_KEY) {   /* failure in resshowkey() */
      /* unfortunately, the remainder of this routine requires that
       * we know the key so we can fill in modname, driver_type, and
       * devdevice, as they are used to do many things.   If we don't
       * know the key then we can't actually do anything, so we
       * try at the backup key.  If that doesn't exist then we must 
       * fake a success and hope for the best later on.
       *
       * You'll get here if you have a smart bus board (PCI/EISA/MCA) and
       * you remove it, notice that the stacks complain because they can't
       * open up the minor number they used to, and the user shoves the
       * card in, hoping for the best.  idconfupdate was likely run in the
       * meantime.  When they reboot the card is back in the resmgr but
       * it's now unclaimed which means no modname, NETCFG_ELEMENT, etc.
       * parameters at that resmgr key.  the user will then try to delete
       * the card the official way to continue, getting us to here.
       * the other obvious way is to just delete the NETCFG_ELEMENT parameter.
       *
       * The current autoconf subsystem needs ENTRYTYPE as a means of locking 
       * down this resmgr key and not deleting it.  this will solve the above
       * and the problem of the driver preventing the wrong minor number
       * from being used when a card is removed, skewing the minor numbers.
       */
      notice("idremove: couldn't find element %s in resmgr - trying backup",
             element);
      if (backupkey == RM_KEY) {
         /* now we're in a jam.  no primary key, no backup key.  
          * Note that ODI/DLPI will never have backup key any more.
          * Note much to do here except remove what we can and fake success
          */
         notice("idremove: couldn't find backup key for element %s in resmgr"
                " either -- faking success", element);
         fullrelink="N";
         goto LastDitchRemove;
      }
      notice("idremove: using backup key information from resmgr for "
             "elemement %s", element);
      status=ResGetNameTypeDeviceDepend(backupkey,1,modname,
                                        driver_type,devdevice,depend);
      strtoupper(driver_type);
      if (strcmp(driver_type,"MDI") == 0) {
         if (ResmgrGetNetX(backupkey,netXstr) != 0) {
            error(NOTBCFG,"idremove: couldn't get backup netXstring key=%d",
                  rmkey);
            return;
         }
      }

   } else {

      if (backupkey == RM_KEY) {
         /* normal to not have backup key for ODI or DLPI drivers */
         notice("idremove: FYI: found primary but can't find backup key "
                "for element %s",element); 
         /* not a problem, just means we won't have to remove it later */
      }

      /* ResGetNameType does a OpenResmgr/CloseResmgr */
      status=ResGetNameTypeDeviceDepend(rmkey,0,modname,
                                        driver_type,devdevice,depend);
      if (status != 0) {
         error(NOTBCFG,"idremove: resgettype returned %d",status);
         return;
      }

      strtoupper(driver_type);

      /* retrieve CM_MDI_NETX parameter from resmgr before we delete params */
      if (strcmp(driver_type,"MDI") == 0) {
         if (ResmgrGetNetX(rmkey,netXstr) != 0) {
            error(NOTBCFG,"idremove: couldn't get netXstring at key %d",
                  rmkey);
            return;
         }
      }

      /* now remove this key (ISA) or resmgr params associated with 
       * key (PCI/EISA/MCA) for later resshowunclaimed and get count of 
       * this driver type left in resmgr 
       *
       * not necessary to do this if the element couldn't be found in
       * the resmgr since rmkey isn't actually set to anything, and
       * it's likely that the key is gone anyway
       * ok to delete modname, bindcpu, unit, ipl here as they are all 
       * software entities from original DSP file
       */
      if (ResmgrDelInfo(rmkey, 1, 1, 1, 1) != 0) {
         error(NOTBCFG,"idremove: ResmgrDelKey returned failure");
         return;
      }
   }

   /* now we're done with the backup key.  always delete it if we can.  */
   if (backupkey != RM_KEY) {
      (void) ResmgrDelKey(backupkey);
   }

   /* now update our stats */
   driver_count=ResmgrGetNumBoards(modname);
   if (driver_count == -1) {
      error(NOTBCFG,"idremove: ResmgrGetNumBoards returned -1");
      return;
   }
   driver_type_count=resgetcount(driver_type); /* how many ODI/MDI/etc left? */
   if (driver_type_count < 0) {    /* some sort of error occurred */
      error(NOTBCFG,"idremove: resgetcount for %s failed",driver_type);
      return;
   }

#define TOAST(A) snprintf(cmd,SIZE,"/sbin/modadmin -U %s",A); \
                 runcommand(0,cmd); \
                 snprintf(cmd,SIZE,"/etc/conf/bin/idinstall -P nics -d %s",A);\
                 runcommand(0,cmd);

   /* common stuff to delete for EACH EACH instance of the driver in the resmgr 
    * beyond just the key itself
    */
   snprintf(cmd,SIZE,"/usr/sbin/netinfo -r -d %s",devdevice);
   runcommand(0,cmd);   /* don't care if it succeeds or fails */

   if (strcmp(driver_type,"MDI") == 0) {
      struct stat sb;

      snprintf(infopath,SIZE,"%s/%s",NETXINFOPATH,netXstr);
      snprintf(initpath,SIZE, "%s/%s", NETXINITPATH, netXstr);
      snprintf(removepath,SIZE, "%s/%s", NETXREMOVEPATH, netXstr);
      snprintf(listpath,SIZE, "%s/%s", NETXLISTPATH, netXstr);
      snprintf(reconfpath,SIZE, "%s/%s", NETXRECONFPATH, netXstr);
      snprintf(controlpath,SIZE, "%s/%s", NETXCONTROLPATH, netXstr);

      /* remove the corresponding netX driver */
      TOAST(netXstr);

      snprintf(dlpimdifile,SIZE,"%s/dlpimdi",DLPIMDIDIR);

      /* if dlpimdifile exists then update delete appropriate line or field */
      if ((stat(dlpimdifile,&sb) != -1) && S_ISREG(sb.st_mode)) {

         /* rebuild slink 05dlpi file too -- not needed according to davided */

         /* if failover is 0 then remove entire line from dlpimdi file.
          * Yes, if there was a failover device previously configured
          * it's up to netconfig to issue a idremove of *THAT* device
          * first before this one
          */
         if (failover == 0) {
            /* remove entire line from dlpimdi file */
            snprintf(cmd,SIZE,"/usr/bin/ed -s %s <<!\n"
                        "/^%s:\n"
                        "d\n"
                        "w\n"
                        "q\n"
                        "!\n",
                    dlpimdifile,netXstr);
            status=docmd(0,cmd);
            if (status != 0) {
               error(NOTBCFG,"couldn't remove %s from dlpimdi file",netXstr);
               return;
            }
         } else {
            /* if failover is 1 remove just remove the failover field 
             * from dlpimdi file 
             */
            snprintf(cmd,SIZE,
                     "/usr/bin/sed -e 's/:%s:/::/g' %s> %s+;/bin/mv %s+ %s",
                     netXstr,dlpimdifile,dlpimdifile,dlpimdifile,dlpimdifile);
            status=docmd(0,cmd);
            if (status != 0) {
               error(NOTBCFG,"couldn't remove %s from dlpimdi file",netXstr);
               return;
            }
         }
      }
   } else if (strcmp(driver_type,"ODI") == 0) {

      snprintf(infopath,SIZE,"%s/%s",NETXINFOPATH,element);
      snprintf(initpath,SIZE, "%s/%s", NETXINITPATH, element);
      snprintf(removepath,SIZE, "%s/%s", NETXREMOVEPATH, element);
      snprintf(listpath,SIZE, "%s/%s", NETXLISTPATH, element);
      snprintf(reconfpath,SIZE, "%s/%s", NETXRECONFPATH, element);
      snprintf(controlpath,SIZE, "%s/%s", NETXCONTROLPATH, element);

      status=DoODIMEMTune();
      if (status == -1) {
         error(NOTBCFG,"idremove: DoODIMEMTune failed");
         return;
      }
      if (status == 1) {  /* we ran idtune -- must relink kernel */
         fullrelink="Y";
      }
   } else if (strcmp(driver_type,"DLPI") == 0) {

      snprintf(infopath,SIZE,"%s/%s",NETXINFOPATH,element);
      snprintf(initpath,SIZE, "%s/%s", NETXINITPATH, element);
      snprintf(removepath,SIZE, "%s/%s", NETXREMOVEPATH, element);
      snprintf(listpath,SIZE, "%s/%s", NETXLISTPATH, element);
      snprintf(reconfpath,SIZE, "%s/%s", NETXRECONFPATH, element);
      snprintf(controlpath,SIZE, "%s/%s", NETXCONTROLPATH, element);

   } else {
      error(NOTBCFG,"idremove: unknown driver_type %s",driver_type);
      return;
   }

   /* now do stuff for the LAST LAST instance of the driver in the resmgr */

   if (driver_count == 0) {   /* last instance of this driver removed */
      TOAST(modname);   /* delete the driver from the system */
      /* fullrelink="Y" not necessary since idbuild -M foo doesn't
       * update /stand/unix when adding card, so removal of last 
       * instance of driver shouldn' force a full rebuild.
       */

      /* go through NIC_DEPEND drivers add see if anybody is still using
       * them.  If nobody is using them then idinstall -d the DEPEND= driver
       * you can have two separate drivers both requiring a common DEPEND=
       * driver so you must do it this way.
       */
      if (strcmp(depend,"-") != 0) {
         /* this driver has DEPEND= drivers.  see if we can delete any of them
          * from the system too
          */
         char *tmp, *parm, *next;

         tmp=depend;
         while ((parm=strtok_r(tmp," ",&next)) != NULL) {
            tmp=NULL;   /* for next strtok_r */

            /* at this point backup key is gone as is info at primary key
             * (meaning it's an unclaimed board in the resmgr for smart bus
             * boards)
             */
            if ((status=DependStillNeeded(parm)) == 0) {
               /* nobody else needs this particular driver any more. */
               snprintf(tmpcmd,SIZE,"/etc/conf/bin/idinstall -P nics -d %s",
                  parm);
               runcommand(0,tmpcmd);
            } else {
               if (status == -1) {
                  error(NOTBCFG,"idremove: DependStillNeeded failed");
                  return;
               }
            }
         }
      }  /* bcfg associated with driver had DEPEND= when it was installed */

      /* handle POST_SCRIPT */
      if (bcfgindex == -1) {
         notice("idremove: skipping POST_SCRIPT since no bcfgindex available");
      } else {
         int loop;

         numlines=StringListNumLines(bcfgindex, N_POST_SCRIPT);
         if (numlines > 0) { /*-1 means POST_SCRIPT is undefined in bcfg file*/
            notice("executing POST_SCRIPT commands...");
            for (loop=1; loop <= numlines; loop++) {
               lineX=StringListLineX(bcfgindex, N_POST_SCRIPT, loop);
               if ((lineX != NULL) && (*lineX != '\n') && (*lineX != NULL)) {
                  /* fyi, niccfg didn't run postscript with any arguments. */
                  if (*lineX == '/') {
                     snprintf(tmpcmd,SIZE,"%s",lineX);
                  } else {
                     snprintf(tmpcmd,SIZE,"/etc/inst/nics/scripts/%s",
                              lineX);
                  }
                  /* niccfg didn't care about exit status of script so we can't
                   * either.  so we call runcommand instead of docmd
                   */
                  if (stat(tmpcmd, &sb) == 0) {
                     sb.st_mode |= S_IXUSR; /* only owner needs execute perms */
                     chmod(tmpcmd, sb.st_mode); 
                     runcommand(0, tmpcmd);
                  } else {
                     notice("not running POST_SCRIPT cmd '%s' - does not exist",
                            tmpcmd);
                  }
                  free(lineX);
               } else {
                  if (lineX != NULL) free(lineX);
               }
            }
         }
      }
   }  /* last instance of driver */

   if (driver_type_count == 0) {  /* last ODI or MDI driver on system removed */
      /* no more ODI/MDI/DLPI drivers in system.  Delete aux drivers for
       * each respective driver type.  This isn't entirely correct for ODI as
       * if an ODI ethernet and ODI FDDI driver are on system and
       * user deletes the ODI FDDI driver the "fdditsm" driver will
       * hang out in the link kit until the ODI ethernet driver is
       * deleted later on.  Solution is to add ODI driver subtype 
       * (CM_ODI_TOPOLOGY) containing TOKEN/FDDI/ETHER to resmgr
       * and read it here to determine which aux drivers we can remove
       * here, which we don't do, since that changes our algorithm
       * to removing the last ODI_TOPOLOGY type vs. last ODI driver which
       * is what we use here.
       */

      if (strcmp(driver_type,"ODI") == 0) {
         TOAST("toktsm");
         TOAST("fdditsm");
         TOAST("ethtsm");
         TOAST("msm");
         TOAST("lsl");
         /* our packaging postinstall script goes out of its way to add 
          * to the link kit odimem early on so we shouldn't remove it
          * when last odi driver is removed.   our packaging preremove
          * script will idinstall -d odimem.
          * BL9: don't toast odimem any more...
          * BL15: postinstall doesn't add odimem by default any more,
          *       preremove won't complain if it isn't there, 
          *       and leaving it in the link kit will just waste system memory
          *       since there won't even be an odi driver to use its
          *       hogged contiguous memory.  remove it when done.  if another
          *       ODI driver is loaded then odimem will be re-added to link 
          *       kit (forcing a relink/reboot but it serves you right for
          *       choosing any ODI driver!)
          */
         TOAST("odisr");
         TOAST("odimem"); 
      } else if (strcmp(driver_type,"DLPI") == 0) {
         /* nothing to do */
      } else if (strcmp(driver_type,"MDI") == 0) {
         /* TOAST("dlpi");     BL9: don't remove dlpi or dlpibase */
         /* TOAST("dlpibase"); BL9: don't remove dlpi or dlpibase */
      } else {
         error(NOTBCFG,"idremove: Unknown DRIVER_TYPE %s",driver_type);
         return;
      }
   }

#ifdef NEVER
causes checksum failures
   /* tell installf we're done deleting link kit files.  should do
    * before idconfupdate
    */
   snprintf(cmd,SIZE,"/usr/sbin/installf -f nics");
   if ((status=docmd(0,cmd)) != 0) {
      error(NOTBCFG,"problems running installf -f nics");
      return;
   };
#endif

   /* run idconfupdate after removing/modifying key ourselves to re-sync 
    * link kit files with now-updated in-core resmgr database.  This will 
    * update the appropriate System file(extra bonus - we didn't have to!).
    */
   snprintf(cmd,SIZE,"/etc/conf/bin/idconfupdate -f");
   status=docmd(0,cmd);
   if (status != 0) {
      error(NOTBCFG,"idremove: command idconfupdate failed");
      return;
   }

   /* Lastly, delete info/init/remove/etc files for netconfig */
   unlink(infopath);
   unlink(initpath);
   unlink(removepath);
   unlink(listpath);   /* may not be there */
   unlink(reconfpath);   /* may not be there */
   unlink(controlpath);

FakeRemoveSuccess:
   StartList(4,"STATUS",30,"FULLRELINKREQUIRED",20);

   if (bflag == 0) {
      AddToList(2,"success",fullrelink);
   } else {
      /* I should relink kernel.  See if we need to */
      if (*fullrelink == 'Y') {
         if (tclmode) {
            notice("not running idbuild -B, leaving up to netcfg!");
         } else {
            snprintf(cmd,SIZE,"/etc/conf/bin/idbuild -B");
            status=docmd(0,cmd);
            if (status != 0) {
               error(NOTBCFG,"idremove: command idbuild -B failed");
               return;
            }
            fullrelink="N";  /* since I just did it */
         }
      }
      AddToList(2,"success",fullrelink);
   }

   EndList();
   return;

LastDitchRemove:
   /* we have severe resmgr problems:  neither the primary key nor the
    * backup are available but we still want to remove information from
    * system configuration files 
    */
   notice("idremove: LastDitchRemove: removing %s from system files",element);

   /* ALL: remove from netdrivers file.  note it may not be in there. */
   snprintf(cmd,SIZE,"/usr/sbin/netinfo -r -d %s",element);
   runcommand(0,cmd);   /* don't care if it succeeds or fails */

   /* ALL: delete info/init/remove/etc scripts and files for netconfig 
    * if they exist (some or none of these files may exist)
    */
   snprintf(infopath,SIZE,"%s/%s",NETXINFOPATH, element);
   snprintf(initpath,SIZE, "%s/%s", NETXINITPATH, element);
   snprintf(removepath,SIZE, "%s/%s", NETXREMOVEPATH, element);
   snprintf(listpath,SIZE, "%s/%s", NETXLISTPATH, element);
   snprintf(reconfpath,SIZE, "%s/%s", NETXRECONFPATH, element);
   snprintf(controlpath,SIZE, "%s/%s", NETXCONTROLPATH, element);

   unlink(infopath);
   unlink(initpath);
   unlink(removepath);
   unlink(listpath);   /* may not be there */
   unlink(reconfpath);   /* may not be there */
   unlink(controlpath);

   /* if we're trying to delete an MDI driver then we know the element
    * will be "netX".  The if statement below is a fancy way of saying
    * if this is an MDI driver then...
    */
   if (  (  (strlen(element) == 4) &&            /* "net0" through "net9" */
            (strncmp(element,"net",3) == 0) &&   /* starts with "net" */
            (isdigit(element[3]))                /* ends with a number */
         ) ||
         (  (strlen(element) == 5) &&            /* "net10" through "net99" */
            (strncmp(element,"net",3) == 0) &&   /* starts with "net" */
            (isdigit(element[3])) &&             /* 4th char is a number */
            (isdigit(element[4]))                /* 5th char is a number */
         )
      ) {
      /* delete the backup key whose MODNAME is netX if it exists in resmgr */
      DelAllKeys(element);   

      /* remove the corresponding netX driver from link kit*/
      TOAST(element);

      /* remove the netX line from dlpimdi file */

      snprintf(dlpimdifile,SIZE,"%s/dlpimdi",DLPIMDIDIR);
      /* if dlpimdifile exists then update delete appropriate line or field */
      if ((stat(dlpimdifile,&sb) != -1) && S_ISREG(sb.st_mode)) {

         /* rebuild slink 05dlpi file too -- not needed according to davided */

         /* if failover is 0 then remove entire line from dlpimdi file.
          * Yes, if there was a failover device previously configured
          * it's up to netconfig to issue a idremove of *THAT* device
          * first before this one
          */
         if (failover == 0) {
            /* remove entire line from dlpimdi file */
            snprintf(cmd,SIZE,"/usr/bin/ed -s %s <<!\n"
                        "/^%s:\n"
                        "d\n"
                        "w\n"
                        "q\n"
                        "!\n",
                    dlpimdifile,element);
            status=docmd(0,cmd);
            if (status != 0) {
               error(NOTBCFG,"couldn't remove %s from dlpimdi file",element);
               return;
            }
         } else {
            /* if failover is 1 remove just remove the failover field 
             * from dlpimdi file 
             */
            snprintf(cmd,SIZE,
                     "/usr/bin/sed -e 's/:%s:/::/g' %s> %s+;/bin/mv %s+ %s",
                     element,dlpimdifile,dlpimdifile,dlpimdifile,dlpimdifile);
            status=docmd(0,cmd);
            if (status != 0) {
               error(NOTBCFG,"couldn't remove %s from dlpimdi file",element);
               return;
            }
         }
      }
   }

   /* since we can't remove driver or any of its DEPEND= files no need
    * to play with fullrelink
    * Can't run POST_SCRIPT either since we can't determine bcfgindex.
    * All of these require more knowledge which we simply don't have
    * available.
    */

   goto FakeRemoveSuccess;

#undef TOAST

   /* NOTREACHED */
}


/* returns 0 for success else error number */
int
showhelpfile(char *bcfgindexstr)
{
   u_int bcfgindex, len;
   char *strend;
   char *helpfile;

   /* avoid atoi */
   if (((bcfgindex=strtoul(bcfgindexstr,&strend,10))==0) &&
        (strend == bcfgindexstr)) {
      error(NOTBCFG,"invalid number %s",bcfgindexstr);
      return(-1);
   }
   if (bcfgindex >= bcfgfileindex) {
      error(NOTBCFG,"index %d >= maximum(%d)",bcfgindex,bcfgfileindex);
      return(-1);
   }

   helpfile=StringListPrint(bcfgindex,N_HELPFILE);

   if (helpfile == NULL) {
      /* shouldn't happen - HELPFILE is mandatory for v1 and we define
       * a default helpfile for V0 bcfg files in GiveV0Defaults
       */
      error(NOTBCFG,"HELPFILE not defined for bcfgindex %d",bcfgindex);
      return(-1);
   }

   /* if helpfile isn't one word or doesn't end in .html then fake
    * success.  this takes care of "foo bar" that we shipped in demo
    * .bcfg files as well as "nobook nohelp" that bcfgops.c sets as a default
    * for DLPI/ODI drivers.  netcfg only wants valid file names
    */
   StartList(2,"HELPFILE",30);

   len = strlen(helpfile);
   if ((len > 5) && 
       (strcmp(helpfile + (len-5), ".html") == 0) &&
       (StringListNumWords(bcfgindex, N_HELPFILE) == 1)) {
      AddToList(1,helpfile);
   } else {
      if (tclmode) {
         strcat(ListBuffer,"{ }");
      } else {
         error(NOTBCFG,"skipping invalid helpfile name %s",helpfile);
      }
   }

   EndList();

   free(helpfile);

   return(0);
}

/* determines if a set of ISA parameters is available for configuration
 * for the given bcfg file index
 * The rationale is that if the bcfg file defines that a board needs
 * a set of parameters (typically {IRQ,IOADDR,MEMADDR}) in order to
 * function, so if the set cannot be formed, then this driver cannot
 * be used, since the parameters are already "claimed" in the resmgr.
 * If the bcfg file doesn't define the attribute(i.e. DMA), then it's
 * not required, so we assume that characteristic is "ok" below.
 *
 * ASSUMES:  - machine has ISA bus
 *           - bcfgfile is for ISA bus
 */
int
ISAsetavail(u_int bcfgindex)
{
   char *irq,*port,*mem,*dma;
   int numint, numport, nummem, numdma;
   int intok, portok, memok, dmaok;
   int loop,status;

   intok=portok=memok=dmaok=0;

   numint=StringListNumWords(bcfgindex, N_INT);
   if (numint <= 0) {
      /* undefined or none */
      intok=1;
   } else {
      int anirqavail=0;

      for (loop=1; loop <= numint; loop++) {
         irq=StringListWordX(bcfgindex,N_INT,loop);
         if (irq == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"ISAsetavail: StringListWordX returned NULL");
            return(-1);
         }
         /* since this is only for ISA cards we don't have to worry about
          * ITYPE sharing here -- existence of driver in resmgr is sufficient
          */
         status=ISPARAMAVAIL(N_INT, irq);
         free(irq);  /* malloced in StringListWordX */
         if (status == 1) {
            anirqavail=1;
            break;
         }
         if (status == -1) { 
            error(NOTBCFG,"IsParam for INT returned -1");
            return(-1);
         }
      }
      if (anirqavail == 1) {
         intok=1;
      }
   }

   numport=StringListNumWords(bcfgindex, N_PORT);
   if (numport <= 0) {
      /* undefined or none */
      portok=1;
   } else {
      int aportavail=0;

      for (loop=1; loop <= numport; loop++) {
         port=StringListWordX(bcfgindex,N_PORT,loop);
         if (port == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"ISAsetavail: StringListWordX returned NULL");
            return(-1);
         }
         status=ISPARAMAVAIL(N_PORT, port);
         free(port);  /* malloced in StringListWordX */
         if (status == 1) {
            aportavail=1;
            break;
         }
         if (status == -1) {
            error(NOTBCFG,"IsParam for PORT returned -1");
            return(-1);
         }
      }
      if (aportavail == 1) {
         portok=1;
      }
   }

   nummem=StringListNumWords(bcfgindex, N_MEM);
   if (nummem <= 0) {
      /* undefined or none */
      memok=1;
   } else {
      int amemavail=0;

      for (loop=1; loop <= nummem; loop++) {
         mem=StringListWordX(bcfgindex,N_MEM,loop);
         if (mem == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"ISAsetavail: StringListWordX returned NULL");
            return(-1);
         }
         status=ISPARAMAVAIL(N_MEM, mem);
         free(mem);  /* malloced in StringListWordX */
         if (status == 1) {
            amemavail=1;
            break;
         }
         if (status == -1) {
            error(NOTBCFG,"IsParam for MEM returned -1");
            return(-1);
         }
      }
      if (amemavail == 1) {
         memok=1;
      }
   }

   numdma=StringListNumWords(bcfgindex, N_DMA);
   if (numdma <= 0) {
      /* undefined or none */
      dmaok=1;
   } else {
      int admaavail=0;

      for (loop=1; loop <= numdma; loop++) {
         dma=StringListWordX(bcfgindex,N_DMA,loop);
         if (dma == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"ISAsetavail: StringListWordX returned NULL");
            return(-1);
         }
         status=ISPARAMAVAIL(N_DMA, dma);
         free(dma);  /* malloced in StringListWordX */
         if (status == 1) {
            admaavail=1;
            break;
         }
         if (status == -1) {
            error(NOTBCFG,"IsParam for DMA returned -1");
            return(-1);
         }
      }
      if (admaavail == 1) {
         dmaok=1;
      }
   }
  
   if ((intok == 1) && (portok == 1) && (memok == 1) && (dmaok == 1)) {
      return(1);
   }
   /* a set isn't available to choose from.  return failure */
   return(0);
}

/* does the driver indicated by the following bcfg index have a verify
 * routine?
 * An alternate method would be to look for a Drvmap file but they're
 * optional -- at least all network drivers must have a .bcfg file
 * so we look for the existence of the ISAVERIFY parameter withing
 * that file as an indication that the driver does indeed have a verify
 * routine.  netcfg will use this information in determining if it
 * should prompt the user to search for a board
 * returns 0 for success else -1 for error.
 */
int
bcfghasverify(char *bcfgindexstr)
{
   u_int bcfgindex;
   char *strend;
   char *yes="Y";
   char *no="N";

   if (bcfgindexstr == NULL) {
      error(NOTBCFG,"bcfghasverify: bad bcfgindex of null!");
      return(-1);
   }

   /* avoid atoi */
   if (((bcfgindex=strtoul(bcfgindexstr,&strend,10))==0) &&
        (strend == bcfgindexstr)) {
      error(NOTBCFG,"invalid number %s",bcfgindexstr);
      return(-1);
   }
   if (bcfgindex >= bcfgfileindex) {
      error(NOTBCFG,"index %d >= maximum(%d)",bcfgindex,bcfgfileindex);
      return(-1);
   }

   StartList(2,"ANSWER",7);
   if (HasString(bcfgindex, N_ISAVERIFY, "x", 0) == -1) {
      /* ISAVERIFY parameter never defined in bcfg file: no verify routine */
      strend=no;
   } else {
      strend=yes;
   }
   AddToList(1,strend);
   EndList();
   return(0);
}

void
ClearTheScreen(void)
{
   putp(clear_screen);
   fflush(stdout);
}

/* we could also use the MIOC_READKSYM ioctl but the mmap establishes
 * a permanent mapping; useful for future calls since you wouldn't have
 * system call overhead and can just read from mmaped area.
 * see the gettimeofday source for an alternate implementation.
 * in our example we close the fd and unmap the memory when we're done.
 * right now the sysdat single page mapping only contains hrestime - see 
 * usr/src/work/uts/svc/sysdat.s    This may change in the future
 */
void
sysdat(void)
{
   int fd;
   caddr_t addr;
   timestruc_t *hrestime;
   unsigned long hrtaddr = 0;
   unsigned long hrtinfo;
   char seconds[10];
   char nanoseconds[10];

   /* symbol can't have more than MAXSYMNMLEN characters and if hrestime
    * is multiply defined in kernel then static one in kernel has priority
    */
   if (getksym("hrestime", &hrtaddr, &hrtinfo) == -1) {
      error(NOTBCFG,"couldn't find hrestime in kernel; error=%s",
      strerror(errno));
      return;
   }

   if (hrtinfo != STT_OBJECT) {
      error(NOTBCFG,"hrestime unknown type=%d",hrtinfo);
      return;
   }

   if ((fd=open("/dev/sysdat",O_RDONLY)) == -1) {
      error(NOTBCFG,"could not open /dev/sysdat; error=%s",strerror(errno));
      return;
   }

   addr=mmap(0, PAGESIZE, PROT_READ, MAP_SHARED, fd, (off_t)hrtaddr);
   if ((addr == (caddr_t)-1) || (addr == NULL)) {
      error(NOTBCFG,"sysdat: couldn't mmap /dev/sysdat; error=%s",
         strerror(errno));
      close(fd);
      return;
   }

   close(fd);

   addr = addr + (hrtaddr % PAGESIZE);
   hrestime = (timestruc_t *) addr;

   StartList(4,"SECONDS",10,"NANOSECONDS",10);
   snprintf(seconds,    10,"%d",hrestime->tv_sec);
   snprintf(nanoseconds,10,"%d",hrestime->tv_nsec);
   AddToList(2,seconds,nanoseconds);
   EndList();

   /* gettimeofday keeps the mapping around for future calls to gettimeofday
    * but we unmap ours
    */
   munmap(addr, PAGESIZE);
   return;
}

/* this route is better than nlist for a number of reasons:
 * - nlist only checks static /unix -- won't have DLM symbols in it
 *   (ignoring .unixsyms ELF section on boot floppy used for ISL)
 * - this locks down the module so it won't be unloaded
 * - this is a pseudo-atomic operation: given a symbol and value, do it all
 *   now, as opposed to the nlist/open/lseek/write route
 * internally to the implementation the ioctl is the same as a read/write on
 * kmem
 * NOTE:  routine is called from cmdparser.y, DoResmgrAndDevs(from rc2.d/S15nd)
 *        idinstall, and idmodify -- so don't call notice if
 *        donlist is set to 0.  errors are still ok since they would affect
 *        the operation(patching) of this routine.
 * NOTE!:  Be careful when letting the user type in freetext __STRING__
 *         parameters in .bcfg custom with PATCH scope, as they can patch
 *         /dev/kmem to basically any value!
 */ 
void
donlist(int setmode, char *symbol, unsigned long newvalue, int dolist)
{
   struct mioc_rksym ksym;
   int fd;
   unsigned char tmp[5];
   char tmpint[10];
   char tmplong[10];
   char tmphex[10];
   union {
      unsigned char aschar[4];
      unsigned int  asint;
      unsigned long aslong;
   } buf;

   if ((fd=open("/dev/kmem", setmode ? O_WRONLY : O_RDONLY)) == -1) {
      error(NOTBCFG,"could not open /dev/kmem to %s symbol %s; error=%s",
            setmode ? "write" : "read", symbol, strerror(errno));
      return;
   }

   if (setmode) {
      buf.aslong = newvalue;
   }

   if (strlen(symbol) > MAXSYMNMLEN) {
      error(NOTBCFG,"max symbol name length is %d",MAXSYMNMLEN);
      return;
   }

   ksym.mirk_symname = symbol;
   ksym.mirk_buf = &buf;
   ksym.mirk_buflen = sizeof(buf);

   if (ioctl(fd, setmode ? MIOC_WRITEKSYM : MIOC_READKSYM, &ksym) < 0) {
      error(NOTBCFG,"ioctl on kmem failed with %s",strerror(errno));
      error(NOTBCFG|CONT,"is the driver containing the symbol loaded yet?");
      close(fd);
      return;
   }

   close(fd);

   /* write mode */
   if ((setmode == 1) && (dolist == 1)) {
      StartList(2,"STATUS",10);
      AddToList(1,"success");
      EndList();
      return;
   }
   
   /* read mode */
   if ((setmode == 0) && (dolist == 1)) {
      StartList(10, "STATUS",10,"ASHEX",10,"ASINT",10,"ASLONG",10,"ASCHAR",10);
      tmp[0]=buf.aschar[0];
      tmp[1]=buf.aschar[1];
      tmp[2]=buf.aschar[2];
      tmp[3]=buf.aschar[3];
      tmp[4]='\0';
      snprintf(tmphex,10,"%x",buf.asint);
      snprintf(tmpint,10,"%u",buf.asint);
      snprintf(tmplong,10,"%lu",buf.aslong);
      AddToList(5, "success", tmphex, tmpint, tmplong, tmp);
      EndList();
   }

   return;
}

void
doxid(char *element)
{

   notice("doxid: not implemented yet");

}


void
alarmhandler(int sig)
{
   /* nothing to do here */
}

/* ensure media is LLC
 * bind to an even sap >= 2 and <= 0xfe
 * specify llcmode == LLC_1  (not llcmode == 0 which is LLC_OFF)
 * in the dl_bind_req set dl_service_mode to DL_CLDLS  to get LLC_1 
 *                    set dl_xidtest_flg to DL_AUTO_TEST|DL_AUTO_XID
 * send out as a broadcast frame  to sap 0xff (Broadcast SAP) or
 * 0x0 (null sap) -- null sap is the better 
 */
void
dotest(char *element)
{

#define DL_ADDR_SIZE 6   /* could use MDI_MACADDRISIZE */
   typedef struct {
        char dl_addr[DL_ADDR_SIZE];
        ushort_t dl_sap;
   } dl_addr_t;
   int fd=-1;
   char tmp[SIZE];   /* much bigger than needed: union dl_primitives + 1K */
   char data[4096];
   dl_bind_req_t dl_bind;
   union DL_primitives *rcv;
   dl_bind_ack_t *dl_bind_ptr=NULL;
   dl_error_ack_t *dl_error_ptr=NULL;
   dl_addr_t *addrp;
   int count;
   int flags = 0;
   ulong sap=2;
   unsigned char *address;
   struct strbuf ctlbuf, databuf;
   struct sigaction act, oact;
   dl_test_req_t *testreq;
   ulong nmsgs = 0;
   int bcfgindex=elementtoindex(0,element); /* OpenResmgr/CloseResmgr */

   if (bcfgindex == -1) {
      error(NOTBCFG,"dotest: elementtoindex failed");
      return;
   }

   putenv("__NDCFG_NOPOLICEMAN=1");

   /* establish alarm clock handler */
   act.sa_handler=alarmhandler;
   sigfillset(&act.sa_mask);
   act.sa_flags=0;  /* no SA_RESTART desired here */
#ifdef SA_INTERRUPT
   act.sa_flags |= SA_INTERRUPT;   /* SunOS */
#endif


   /* we change the behaviour of SIGALRM here.  note that :
    * - this prevents hpsl asynch detection handler from running
    * - our itimer is still running to send SIGALRM
    * - upon encountering any error we must restore handler back 
    *   so that hpsl handler can continue receiving SIGALRM
    * - we must turn off itimer so that alarm() starts working again so
    *   that we know that when we call alarm that any SIGALRM will
    *   a)be from alarm(not terribly important) and b) that the signal
    *   won't be received too early.  There's a window here where another
    *   process can send the signal in between the alarm and the getmsg
    *   where we'll think the entire time passed but nothing we can do 
    *   about it.  We just want to prevent itimer signal from firing in window
    * - after we have done the above we can safely call alarm().
    */
   TurnOffItimers();
   if (sigaction(SIGALRM, &act, &oact) == -1) {
      /* shouldn't happen under normal circumstances */
      TurnOnItimers();
      error(NOTBCFG,"dotest: sigaction failed with %s",strerror(errno));
      return;
   }

   if (HasString(bcfgindex, N_TYPE, "MDI", 0) == 1) {
      snprintf(tmp,SIZE,"/etc/nd start %s",element);
      (void) runcommand(0,tmp);   /* could succeed, could fail, don't care! */
   }

#if 0
   rm_key_t rmkey;

   rmkey=resshowkey(0,element,bcfgpath,0);/* calls OpenResmgr/CloseResmgr */
   if (rmkey == RM_KEY) {
      notice("dotest: primary key not found, trying backup key");
      rmkey=resshowkey(0,element,bcfgpath,1);/* calls OpenResmgr/CloseResmgr */
      if (rmkey == RM_KEY) {
         error(NOTBCFG,"couldn't find resmgr key for element %s",element);
         goto dotesterr;
      }
   }
   now read DEV_NAME from that resmgr key to obtain device name.  
   unfortunately this is /dev/mdi/foo0 for MDI drivers.  we need to use the
   DLPI interface to talk to /dev/netX which we could get from MDI_NETX
   parameter for MDI drivers but then we'd need to call elementtoindex.

   we cheat and simply use /dev/element that is passed in to this routine,
   since that will be the proper device name to speak DLPI to for MDI, ODI,
   and DLPI drivers
#endif

   snprintf(tmp,SIZE,"/dev/%s",element);

   /* if we just added the card and FULLRELINK is 1 or MUSTREBOOT is 1
    * then it's ok for an open of the device to fail.  don't kill off
    * netcfg if this happens.   Will occur on first add of any ODI driver
    * since odimem isn't in link kit by default any more
    */
   if ((fd=open(tmp,O_RDWR)) == -1) {
      notice("could not open %s error=%s, faking success",tmp,strerror(errno));
      StartList(2,"STATUS",10);
      AddToList(1,strerror(errno));
      EndList();
      goto dotesterr;
   }

trynewsap:
   dl_bind.dl_primitive= DL_BIND_REQ;
   dl_bind.dl_sap = sap;
   dl_bind.dl_max_conind = 0;   /* must be 0 */
   dl_bind.dl_conn_mgmt = 0;  /* must be 0 */
   dl_bind.dl_service_mode = DL_CLDLS; /* determines if LLC_1 or LLC_OFF */
   dl_bind.dl_xidtest_flg = 0; /* dlpi_send_llc_message ensures that
                                * DL_AUTO_TEST or DL_AUTO_XID isn't set
                                */

   ctlbuf.maxlen = sizeof(dl_bind_req_t);
   ctlbuf.len = sizeof(dl_bind_req_t);
   ctlbuf.buf = (char *)&dl_bind;

   if ((putmsg(fd, &ctlbuf, NULL, flags)) < 0) {
      error(NOTBCFG,"putmsg to %s failed",tmp);
      goto dotesterr;
   }

   ctlbuf.maxlen = sizeof(tmp);
   ctlbuf.len = 0;
   ctlbuf.buf = tmp;

   if ((getmsg(fd, &ctlbuf, NULL, &flags)) < 0)  {
      error(NOTBCFG,"getmsg for DL_BIND_ACK failed");
      goto dotesterr;
   }
   rcv = (union DL_primitives *)tmp;

   if (rcv->dl_primitive == DL_BIND_ACK) {
      /*
       * dl_bind_ptr = (dl_bind_ack_t *)rcv;
       * printf("Primitive is %d\n",dl_bind_ptr->dl_primitive);
       * printf("SAP bound was %d\n",dl_bind_ptr->dl_sap);
       * printf("MAC address length %d\n",dl_bind_ptr->dl_addr_length);
       * printf("MAC address offset is %d\n",
       *    dl_bind_ptr->dl_addr_offset);
       * address = (unsigned char *)dl_bind_ptr;
       * address += dl_bind_ptr->dl_addr_offset;
       * printf ("Address is: ");
       * for (count=0; count < dl_bind_ptr->dl_addr_length; count++)  {
       *        printf("%02X ",address[count]);
       * }
       * printf("\n");
       */ 
   } else if (rcv->dl_primitive == DL_ERROR_ACK) {
      dl_error_ptr = (dl_error_ack_t *)tmp;
      /*
       * printf("Primitive is %d\n",dl_error_ptr->dl_primitive);
       * printf("Primitive in error %d\n", dl_error_ptr->dl_error_primitive);
       * printf("DLPI errno %d\n",dl_error_ptr->dl_errno);
       * printf("Unix errno %d\n",dl_error_ptr->dl_unix_errno);
       */
      notice("test: bind to sap %u failed with error_ack dlpi errno=%d "
             "unix errno=%d", sap,
             dl_error_ptr->dl_errno, dl_error_ptr->dl_unix_errno);

      /* DL_BIND_REQ possible failures:
       * - MDI:
       * DL_NOTINIT: dlpid not running - can't continue
       * DL_INITFAILED:  ddi8 mdi driver is suspended - can't continue
       * DL_OUTSTATE:  no more saps available - can't continue
       * DL_BADSAP:  illegal sap: media doesn't support it or someone already
       *             bound to this sap (we assume the latter) 
       * DL_SYSERR:  streams memory shortage - can't continue
       * DL_BOUND: dl_max_conind or dl_conn_mgmt are nonzero (shouldn't happen)
       * - DLPI:
       * DL_OUTSTATE:  we are already bound on this stream (shouldn't happen)
       * DL_ACCESS:  trying to bind to promiscuous sap (0xffff) and 
       *             not priviledged (shouldn't happen)
       * DL_BOUND:  this sap already bound by someone else
       * DL_BADADDR:  trying to bind to null sap(0) or group sap(odd number) or
       *              global_sap(0xff) (shouldn't happen)
       * - ODI:
       * same as dlpi but adds:
       * DL_SYSERR:  size isn't DL_BIND_REQ_SIZE
       * DL_OUTSTATE: tsm type not supported (only fddi, ether, token supported)
       *
       */
      if (HasString(bcfgindex, N_TYPE, "MDI", 0) == 1) {
         if (dl_error_ptr->dl_errno == DL_NOTINIT) {
            StartList(2,"STATUS",12);
            AddToList(1,"not initialized by dlpid");
            EndList();
            goto dotesterr; 
         }
         if (dl_error_ptr->dl_errno == DL_INITFAILED) {
            StartList(2,"STATUS",12);
            AddToList(1,"driver is suspended");
            EndList();
            goto dotesterr; 
         }
         if (dl_error_ptr->dl_errno == DL_OUTSTATE) {
            StartList(2,"STATUS",12);
            AddToList(1,"no saps available");
            EndList();
            goto dotesterr; 
         }
         if (dl_error_ptr->dl_errno == DL_SYSERR) {
            StartList(2,"STATUS",12);
            AddToList(1,"system error occured");
            EndList();
            goto dotesterr; 
         }
      } else
      if ((HasString(bcfgindex, N_TYPE, "DLPI", 0) == 1) ||
          (HasString(bcfgindex, N_TYPE, "ODI", 0) == 1)) {
         if (dl_error_ptr->dl_errno == DL_SYSERR) {
            StartList(2,"STATUS",12);
            AddToList(1,"system error occurred");
            EndList();
            goto dotesterr; 
         }
         if (dl_error_ptr->dl_errno == DL_OUTSTATE) {
            StartList(2,"STATUS",12);
            /* assume dlpi error never happens since both have DL_OUTSTATE */
            AddToList(1,"tsm type not supported"); 
            EndList();
            goto dotesterr; 
         }
      }

      /* at this point all errors where we cannot continue have been handled.
       * try next available sap
       */

      sap += 2;
      /* for ODI and DLPI they use macro SNAPSAP but all(mdi/odi/dlpi)
       * evaluate the macro to 0xAA 
       */
      if (sap == SNAP_SAP) sap += 2;
      notice("test: trying new sap %u",sap);
      if (sap > 0xfe) {
         StartList(2,"STATUS",12);
         AddToList(1,"bind failed");
         EndList();
         notice("test: no saps available");
         goto dotesterr; 
      }
      goto trynewsap;
   } else {
      error(NOTBCFG,"Unknown primitive %d from dl_bind_ack",
                    rcv->dl_primitive);
      goto dotesterr;
   }

   /* tell stream head to throw away unread data
    * ioctl(fd,I_SRDOPT,RMSGD);
    */
   testreq = (dl_test_req_t *) tmp;
   testreq->dl_primitive = DL_TEST_REQ;
   testreq->dl_flag = DL_POLL_FINAL;
   /* dlpi_send_llc_message ensures that dl_dest_addr_length is 
    * MAC_ADDR_SZ + 1  since we have LLC_1 framing set.
    */
   testreq->dl_dest_addr_length = MAC_ADDR_SZ + 1; /* sizeof(dl_addr_t); */
   testreq->dl_dest_addr_offset = sizeof(dl_test_req_t);
   addrp = (dl_addr_t *)(tmp + sizeof(dl_test_req_t));
   memset(addrp, 0xff, DL_ADDR_SIZE);   /* broadcast address */
   addrp->dl_sap = 0x0;   /* NULL SAP */
 
   ctlbuf.maxlen = sizeof(tmp);
   ctlbuf.len = sizeof(dl_test_req_t) + sizeof(dl_addr_t);
   ctlbuf.buf = tmp;

   if (putmsg(fd, &ctlbuf, NULL, 0) < 0) {
      error(NOTBCFG,"DL_TEST_REQ primitive failed");
      goto dotesterr;
   }

   /* read confirmation of test request as DL_TEST_CON */
   
   ctlbuf.maxlen = sizeof(tmp);
   ctlbuf.len = 0;
   ctlbuf.buf = tmp;
   databuf.maxlen = sizeof(data);
   databuf.len = 0;
   databuf.buf = data;
   flags = 0;

   /* itimers turned off, SIGALRM handler set properly, ok to call alarm */
   alarm(3);   /* wait 3 seconds for replies from network - ok to call */

   if (HasString(bcfgindex, N_TYPE, "MDI", 0) == 1) {
      /* toss funkey mdi_do_loopback() generated DL_TEST_CON 
       * ODI drivers don't send up a loopback frame to throw out
       */
      if (getmsg(fd, &ctlbuf, &databuf, &flags) < 0) {
         if (errno == EINTR) {
            error(NOTBCFG,"getmsg for loopback DL_TEST_CON failed");
            goto dotesterr;
         }
      }
      rcv = (union DL_primitives *)tmp;
      notice("prim %x, clen %d dlen %d",
             rcv->dl_primitive,ctlbuf.len,databuf.len);

      if (rcv->dl_primitive != DL_TEST_CON) {
         error(NOTBCFG,"got 0x%x instead of DL_TEST_CON",rcv->dl_primitive);
         goto dotesterr;
      }
   }

   if (getmsg(fd, &ctlbuf, &databuf, &flags) < 0) {
      alarm(0);  /* turn off any pending alarms - ok to call */
      if (errno != EINTR) {
         error(NOTBCFG,"getmsg for DL_TEST_CON failed");
         goto dotesterr;
      }
      StartList(2,"STATUS",10);
      AddToList(1,"noanswer");
      EndList();
      goto dotesterr;
   }

   alarm(0);   /* getmsg succeeded - turn off any pending alarms - ok to call */
   
   rcv = (union DL_primitives *)tmp;

   if (rcv->dl_primitive != DL_TEST_CON) {
      /* could be DL_ERROR_ACK or something else */
      error(NOTBCFG,"expected DL_TEST_CON got %d", rcv->dl_primitive);
      goto dotesterr;
   }


   /* got a DL_TEST_CON: we don't care where the answers come from */
   StartList(2,"STATUS",10);
   AddToList(1,"success");
   EndList();

dotesterr:   /* error or StartList/Endlist called previously */
   if (fd != -1) close(fd);
   putenv("__NDCFG_NOPOLICEMAN=0");
   sigaction(SIGALRM, &oact, NULL);  /* restore HpslHandler */
   TurnOnItimers();   /* call after call to sigaction above! */
   return;

}
