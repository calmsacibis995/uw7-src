#pragma ident "@(#)main.c	29.1"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <signal.h>
#include <ucontext.h>
#include <nl_types.h>
#include <errno.h>
#include <locale.h>
#include <hpsl.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>

#undef ERR  /* the version in regset.h isn't what we want */
/* avoid #define ERR in <sys/regset.h> and <curses.h> */
#include <curses.h>     /* for typedef bool needed by term.h */
#include <term.h>

#include "common.h"

extern int errno;
int netisl=0;
int dangerousdetect=0;
int errorsarefatal,bequiet,cflag;
int section;
int fflag,nflag,bflag,qflag,jflag,aflag,yflag;
int Dflag;
char *rootdir;
/* the nullstring is used when we find '' or "" in a bcfg file
 * it can be part of a custom variable.
 * we pass up "" to bcfgparser.y since that is the most common
 * use for nullstrings.
 */
char nullstring[]="\"\"";
char unknowntype[80];  /* global; not on stack */
extern struct bcfgfile bcfgfile[MAXDRIVERS]; 
extern int bcfgfileindex;
u_int moremode, candomore,hflag,iflag,zflag;
u_int lflag;
FILE *logfilefp;
u_int Wflag;
int RMtimeout;
static nl_catd catd = (nl_catd)-2;

/* global hpsl structures */
HpslContInfo_t *hpslcontinfo;
u_int hpslcount=0;    /* number of hot plug controllers on system */
SuspendedDrvInfoPtr_t hpslsuspendeddrvinfoptr;
u_int hpslsuspendedcnt=0;


char *
T(int type)
{
   switch(type) {
      case SINGLENUM: return("SINGLENUM"); break;
      case NUMBERRANGE: return("NUMBERRANGE"); break;
      case STRING: return("STRING"); break;
      case NUMBERLIST: return("NUMBERLIST"); break;
      case STRINGLIST: return("STRINGLIST"); break;
      default: snprintf(unknowntype,80,"Unknown_type_%d",type); 
               return(unknowntype);
               break;
   }
}

extern unsigned int insingle;
extern unsigned int indouble;
extern unsigned int quotestate;
extern unsigned int incomment;

void
usage(int argc, char *argv[])
{
   Pstderr("Usage: %s [option[s]]\n",argv[0]);
   Pstderr("where option is one or more of the following\n");
   Pstderr(" -a           use tab as delimiter in output\n");
   Pstderr(" -b           idinstall/idremove commands should run idbuild\n");
   Pstderr(" -c           (use with -f) and go into interactive mode\n");
   /* default deliminator is space to match the resmgr binary */
   Pstderr(" -d X         set resput delimiter to char X (space is default)\n");
   Pstderr(" -e           first error is treated as fatal\n");
   Pstderr(" -f filename  Process one bcfg file\n");
   Pstderr(" -h           (interactive only) show heading in output\n");
/*   Pstderr(" -i    (interactive only) getstamp shows all info\n"); */
   Pstderr(" -j           (use with -l) show cmd output when running cmds\n");
   /* -z is also known as "policeman mode" - only called at system startup */
/*   Pstderr("-z     ensure resmgr matches files/devices can be opened\n"); */
   Pstderr(" -l filename  log all input and output to filename\n");
   Pstderr(" -m           (interactive only) enable more mode\n"); 
   Pstderr(" -n           (interactive only) no '%s' prompt\n",PROMPT);
   Pstderr(" -q           (interactive only) don't load default bcfgs first\n");
   Pstderr(" -t           TCL mode (sets and unsets many flags)\n");
   Pstderr(" -v           (interactive only) verbose mode - useful\n");
   Pstderr(" -x           (interactive only) do dangerous isaautodetect\n");
   Pstderr(" -y           (interactive only) show status messages on stdout\n");
   Pstderr(" -D dec_number set debugging level to number(bitmask)\n");
   Pstderr(" -W num_secs   set RMopen timeout to num_secs(default=%d secs)\n",
           RMTIMEOUT);
   Pstderr("type 'help' in interactive mode to see more help\n");
   /* in case logging was started with previous -l option, shut it down */
   if (logfilefp != NULL) {
      fclose(logfilefp);   /* since we're about to call _exit */
      logfilefp=NULL;
   }
   _exit(1);  /* no point going through atexit routines */
}

/* doesn't free up space from the rejectlist.  that happens when we
 * quit the program.
 */
int
unloadall(int dolist)
{
   int bcfgloop, loop;

   for (bcfgloop=0; bcfgloop<bcfgfileindex; bcfgloop++) {

      free(bcfgfile[bcfgloop].location);
      bcfgfile[bcfgloop].location = NULL;

      free(bcfgfile[bcfgloop].driverversion);
      bcfgfile[bcfgloop].driverversion = NULL;

      /* free up all memory stored in bcfgfile.variable */
      for (loop=0; loop< MAX_BCFG_VARIABLES; loop++) {
         union primitives *ptr;

         if (bcfgfile[bcfgloop].variable[loop].strprimitive.type != 
             UNDEFINED){
            /* this bcfg file defined this variable.  free up all data 
             * associated with it and re-mark type as UNDEFINED for next
             * bcfgfile which will use this same bcfgindex
             */
            ptr=&bcfgfile[bcfgloop].variable[loop].strprimitive;
            PrimitiveFreeMem(ptr);
         }

         if (bcfgfile[bcfgloop].variable[loop].primitive.type != 
             UNDEFINED){
            /* this bcfg file defined this variable.  free up all data 
             * associated with it and re-mark type as UNDEFINED for next
             * bcfgfile which will use this same bcfgindex
             */
            ptr=&bcfgfile[bcfgloop].variable[loop].primitive;
            PrimitiveFreeMem(ptr);
         }
      }
   }
   bcfgfileindex=0;
   if (dolist) {
      StartList(2,"STATUS",10);
      AddToList(1,"success");
      EndList();
   }
   return(0);
}

void
FreeMemory(void)
{
   struct reject *r,*next;
   
   unloadall(0);   /* use 0 since we're about to exit */
   /* free up reject list */
   if ((r=rejectlist) != NULL) {
      while (r != NULL) {
         next=r->next;
         free(r->pathname);
         free(r);
         r=next;
      }
   }
   return;
}

/* filename is an absolute path to the file 
 * this routine is called if fflag (single bcfg file) and also from readallbcfg
 */
int
read1bcfg(char *filename)
{
   extern FILE *zzin;
   struct stat sb;

   if (bcfgfileindex == MAXDRIVERS) {
      error(NOTBCFG, "Can't load more bcfgs; at limit of %d", MAXDRIVERS);
      return(0);   /* all done here; return "success" */
   }
   /* we must have absolute path names as later routines to deal with
    * Driver.o will use supplied path name.  Unfortunately, this means
    * that people running ndcfg with -f flag will be forced to supply
    * an absolute path name too.  oh well.
    */
   if (*filename != '/') {
      error(NOTBCFG, 
            "read1bcfg: must provide absolute pathname to file %s",filename);
      return(0);
   }

   if (stat(filename,&sb) == -1) {
      error(NOTBCFG, "read1bcfg: can't find bcfg file %s",filename);
      return(0);
   }

   /* we might want to allow /dev/fd/0 someday, but for now we don't */
   if (!S_ISREG(sb.st_mode)) {
      error(NOTBCFG, "read1bcfg: bcfg file %s is not a regular file",filename);
      return(0);
   }

   zzin=fopen(filename, "r");
   if (!zzin) {
   /* XXX should increment bcfgfileindex when done so that subsequent load
    * operations won't mess up the last successfull index and make it invalid
    * by incrementing bcfghaserrors
    */
      /* can call error here 
       * we must be able to open up the first file to kick start the
       * parser; after this we silently skip any bcfg's we can't open
       * for this reason (and also when running with -f) we call error
       */
      error(NOTBCFG, "read1bcfg: could not open %sfile %s",
              fflag ? "" : "crucial first ",
              filename);
      return(0);   /* all done here; return "success" */
   }
   bcfgfile[bcfgfileindex].location=strdup(filename);  /* for first file */
   bcfglinenum=1;  /* for any calls to zzerror: start of new bcfg file */
   /* reset quote state for parser */
   indouble=0;  
   insingle=0;
   quotestate=NOQUOTES;
   ResetBcfgLexer();
   if (zzparse() != 0) {  /* NOTE: this reads one or *ALL* bcfg's in rootdir
                           * based on fflag  -- zzwrap automatically called.
                           */
      /* uh oh, a syntax error we didn't recover from */
      if (fflag) {
         error(NOTBCFG, "zzparse returned failure for file %s",filename);
      } else {  /* exact file name not known */
         error(NOTBCFG, "zzparse returned failure -- unknown file");
      }
      return(1);   /* return failure up the line */
   }
   /* zzwrap always fcloses zzin [ and advances bcfgfileindex & new zzin ]  */
   return(0);
}

DIR *rootdirp, *drvdirp;
struct dirent *rootdp, *drvdp;
char nextbcfgpath[SIZE]; /* not on stack */

/* read from globals to return next bcfg file name or NULL if done to zzwrap()
 */
char *
getnextbcfg(void)
{
   int len;
   char tmp[SIZE];

   if (fflag) return((char *)NULL); /* in one file mode; nothing else to read */
drvdiragain:
   if (drvdirp == NULL || (drvdp=readdir(drvdirp)) == NULL) {
      /* at end of a driver directory; try new root level directory */
      /* if we're truely at the end then call closedir */
      if (drvdirp != (DIR *) NULL) closedir(drvdirp);
rootdiragain:
      if ((rootdp=readdir(rootdirp)) != NULL) {
         if (!strcmp(rootdp->d_name,".") || !strcmp(rootdp->d_name,"..")) {
            goto rootdiragain;
         }
      } else {   /* no more root level directories to read; all done */
         closedir(rootdirp);
         return((char *)NULL);
      }
      snprintf(tmp,SIZE,"%s/%s",rootdir,rootdp->d_name);
      if ((drvdirp=opendir(tmp)) == NULL) {
         /* we call error but don't associate error with a bcfg so that
          * this bcfg won't be rejected 
          */
         error(NOTBCFG, "getnextbcfg: couldn't open directory %s",tmp);
         /* probably bad perms on dir; silently skip it */
         goto rootdiragain;
      }
      goto drvdiragain;
   }
   if (!strcmp(drvdp->d_name, ".") || !strcmp(drvdp->d_name, "..")) {
      goto drvdiragain;
   }
   /* we follow idinstall's example: name must end in .bcfg to be valid */
   len=strlen(drvdp->d_name);
   if (len > 5 && strcmp(drvdp->d_name + (len-5), ".bcfg") == 0) {
      /* we found a valid name; return to caller */
      snprintf(nextbcfgpath,SIZE,"%s/%s",rootdp->d_name,drvdp->d_name);
      return(nextbcfgpath);
   }
   goto drvdiragain;  /* no match */
}

int
readallbcfg(void)
{
   char fullname[SIZE];
   char *next;
   int ret;

/* reset lexer, reset section names so we're back to INITIAL and not
 * in adapter section, reset version and debug back to 0
 * increment max...index 
 * reset goterrorinthis bcfg
 * also do opendir/readdir then call get1bcfg for all .bcfg files found
 * use a scheme that matches idinstall when installing bcfgs in bcfg.d
 * also check if errorsarefatal and goterrorin this bcfg then fatal(....
 */
   if ((rootdirp=opendir(rootdir)) == NULL) {
      error(NOTBCFG, "Couldn't open directory %s",rootdir);
      return(1);  /* permissions or malloc failure, could use ENOMEM */
   }
   if ((next=getnextbcfg()) == NULL) { /*no bcfg files in this drvr hierarchy*/
      /* closedir(rootdirp); done in getnextbcfg */
      return(0);
   }
   snprintf(fullname,SIZE,"%s/%s",rootdir,next);
   /* we must call yyparse on the first file, then zzwrap will automatically
    * be called for the next file in the series.  zzwrap calls
    * getnextbcfg() to get the next file name to attach to the zzin FILE *
    * so that the lexer can continue processing all bcfgs in this
    * hierarchy so we don't have to fopen them all
    */
   ret=read1bcfg(fullname);  /* kick start lexer; zzwrap does real work */
   /* zzwrap gets called here many times to process other bcfg files in dir */
   /* closedir(rootdirp); done in getnextbcfg */
   return(ret);
}

/* get commands from user */
int
GetCommands(void)
{
   cmdlinenum=1;  /* this is what we read from user for calls to yyerror */
   if (!nflag) printf("%s",PROMPT);
   ClearError();  /* start with clean error buffer */
   PrimeErrorBuffer(2,"Unknown command received");
   if (yyparse() != 0) {
      /* uh-oh, a syntax error we couldn't recover from! */
      error(NOTBCFG, "yyparse returned failure - unknown command");
      return(1);
   };
   /* end of file or user typed quit.  XXX commit any outstanding and leave */
   return(0);
}

void
ShowTerminfoFailureReason(void)
{
   extern short term_errno;
   if (term_errno == -1) {  /* not set */
      Pstderr("\n");
   } else {
      switch (term_errno) {
         case UNACCESSIBLE:
            Pstderr("unaccessible\n");
            break;
         case NO_TERMINAL:
            Pstderr("no terminal\n");
            break;
         case CORRUPTED:
            Pstderr("corrupted\n");
            break;
         case ENTRY_TOO_LONG:
            Pstderr("entry too long\n");
            break;
         case TERMINFO_TOO_LONG:
            Pstderr("terminfo too long\n");
            break;
         case TERM_BAD_MALLOC:
            Pstderr("bad malloc\n");
            break;
         case NOT_READABLE:
            Pstderr("not readable\n");
            break;
      }
   }
}
/* we want to know how many lines the user's terminal has.  We
 * use the terminfo routine setupterm since it does the following (in this 
 * order) to determine lines:
 * - reads "lines" from terminfo, using TERM.  note that if terminfo file
 *   terminfo file does not exist, or it is corrupt, we stop here
 *   if TERM isn't set then we revert to "unknown"
 * - does a TIOCGWINSZ ioctl and if ws_row AND ws_column > 0 then update "lines"
 * - if ROWS environment variable set then update "lines"
 * since we only use lines, we don't care if TERM isn't set (defaulting
 * to "unknown"), since setupterm will still fall back to the above
 * alternate methods for computing the number of lines
 * we only care that "lines" gets set by one of the above methods!
 * we don't use error() because we could be in tclmode and we want
 * error message sent to screen
 */
void
get_terminal_info(void)
{
   int status;
   char *term;
   extern char *Def_term;

   /* we print out TERM in output below for aid in debugging */
   if ((term=(char *) getenv("TERM")) == NULL) {
      term=Def_term;   /* "unknown" */
   }
   /* the term means to use supplied term -- it doesn't matter
    * the 2 is the fd that is used with ioctl and ttyname.  Since this routine
    * is only called when not in tcl mode (i.e. not writing to a pipe),
    * 2 will still point to our tty and is valid for these operations
    * if this fd is used to print error messages and we leave it at 1
    * the error text will be intersperced with stdout -- bad
    */
   setupterm(term, 2, &status);
   if (status != 1) {
      if (status == 0) {
         Pstderr("can't find %s in terminfo database: ",term);
      } else
      if (status == -1) {
         Pstderr("can't find terminfo database for %s:",term);
      } else {
         /* to even get here means some other bizarre error return */
         Pstderr("unknown return value %d from setupterm: ",status);
      }
      ShowTerminfoFailureReason();
      /* but in all cases we can't enable more mode */
      candomore=0;  
   } else {
      /* since routine succeeded, check if set or not */
      if (lines == -1) {
         Pstderr("lines capability not set for TERM type \"%s\"\n",term);
         Pstderr(
          "set ROWS, stty rows & columns, or lines# in above terminfo file\n");
         candomore=0;  
      }
   }
}

void
LoadDirHierarchy(char *root)
{
   extern DIR *rootdirp, *drvdirp;
   extern struct dirent *rootdp, *drvdp;

   /* see zzwrap() for same comparision */
   if (bcfgfileindex == MAXDRIVERS) {
      error(NOTBCFG, "Can't load more; at limit of %d", MAXDRIVERS);
      return;
   } else {
      if (*root != '/') {
         error(NOTBCFG, "dir hierarchy to load must be an absolute pathname");
         return;
      }
      rootdir=root;    /* variable rootdir used in readallbcfg/getnextbcfg */
      rootdirp=drvdirp=(DIR *) NULL;
      rootdp=drvdp=(struct dirent *) NULL;
      /* NOTE: WE CALL ZZ PARSER WITHIN THIS YY PARSER ON NEXT LINE */
      if (readallbcfg()) {
         /* some sort of problem happened */
         error(NOTBCFG, "problem loading bcfgs in %s hierarchy", rootdir);
         return;
      }
   }
   return;
}

char delim=' ';

volatile int nomoreitimers=0;

/* set itimer */
void
TurnOnItimers(void)
{
   struct itimerval value;
   struct sigaction act, oact;

   if (nomoreitimers) return;

   timerclear(&value.it_interval);
   timerclear(&value.it_value);
   value.it_interval.tv_sec = 2;
   value.it_interval.tv_usec = 0;
   value.it_value.tv_sec = 2;
   value.it_value.tv_usec = 0;
   setitimer(ITIMER_REAL, &value, NULL);
   return;
} 

/* stop our itimer */
void
TurnOffItimers(void)
{
   struct itimerval value;

   timerclear(&value.it_interval);
   timerclear(&value.it_value);
   value.it_interval.tv_sec = 0;
   value.it_interval.tv_usec = 0;
   value.it_value.tv_sec = 0;
   value.it_value.tv_usec = 0;
   setitimer(ITIMER_REAL, &value, NULL);
   return;
}

void
BlockHpslSignals(void)
{
   sigset_t set;

   if (nomoreitimers) return;

   sigemptyset(&set);
   sigaddset(&set, SIGALRM);    /* our itimers send SIGALRM */
   sigprocmask(SIG_BLOCK, &set, NULL);
}

void
UnBlockHpslSignals(void)
{
   sigset_t set;

   if (nomoreitimers) return;

   sigemptyset(&set);
   sigaddset(&set, SIGALRM);    /* our itimers send SIGALRM */
   sigprocmask(SIG_UNBLOCK, &set, NULL);
}

void
HpslHandler(int sig, siginfo_t *info, ucontext_t *context)
{
   int ret, count;
   uint_t diff;

   if (info != NULL) {
      if (info->si_code <= 0) {
         notice("HpslHandler: ignoring SIGALRM from pid %d uid %d", 
                info->si_pid, info->si_uid);
         return;
      } 
      /* no codes are defined if > 0 for SIGALRM */
   }

   hpsl_ERR=0;
   ret=hpsl_check_async_events(&count);
   if (ret != HPSL_SUCCESS) {
      notice("HpslHandler: hpsl_check_async_events failed hpsl_ERR=%d, "
             " turning off itimers",hpsl_ERR);
      /* since it failed once, it's likely it will fail again.  prevent
       * wasting CPU cycles and filling up log files
       */
      nomoreitimers=1;
      TurnOffItimers();
      return;
   }
   if (count == 0) {
      return;
   } else
   if (count == EV_RESCAN) {
      hpsl_cleanup();
      hpsl_ERR=0;
      ret=hpsl_init_cont_info(&hpslcontinfo, &hpslcount);
      if (ret != HPSL_SUCCESS) {
         notice("HpslHandler: hpsl_check_async_events failed hpsl_ERR=%d, "
                " turning off itimers",hpsl_ERR);
         /* since it failed once, it's likely it will fail again.  prevent
          * wasting CPU cycles and filling up log files
          */
         hpslcount=0;
         nomoreitimers=1;
         TurnOffItimers();
         return;
      }
      return;
   } else if (count > 0) {
      int contIx, busIx, socketIx;

      /* we have some event.  let's figure out what happened */
      for (contIx = 0; contIx < hpslcount; contIx++) {
         HpslBusInfoPtr_t busList = hpslcontinfo[contIx].hpcBusList;

         for (busIx = 0; busIx < hpslcontinfo[contIx].hpcBusCnt; busIx++) {
            HpslSocketInfoPtr_t socketList = busList[busIx].busSocketList;

            for (socketIx = 0; socketIx < busList[busIx].busSocketCnt; 
                                                                 socketIx++) {
               if (socketList[socketIx].socketEvent == NEW_EVENT) {
                  /* dunno what to do here yet but we may need to tell netcfg
                   * - send signal to our parent if it's netcfg (check /proc)?
                   * - change NOERROR to NOERRORHAVEEVENT in EndList?
                   * note socket state could be going to/from 
                   * SOCKET_INTER_LOCKED (no intervention on our part)
                   * or SOCKET_POWER_ON  (from previous hpsl_set_state call)
                   */
                  notice("hpslHandler: event on c%db%ds%d(%s): new state 0x%x",
                         contIx, busIx, socketIx, 
                         socketList[socketIx].socketLabel,
                         socketList[socketIx].socketCurrentState);
                  diff = socketList[socketIx].socketCurrentState ^
                         socketList[socketIx].socketPreviousState;
                  notice("hpslHandler: change=%s%s%s%s%s%s%s%s%s%s%s",
                     diff & SOCKET_EMPTY ? "EMPTY " : "",
                     diff & SOCKET_POWER_ON ? "POWER " : "",
                     diff & SOCKET_GENERAL_FAULT ? "GENERAL_FAULT " : "",
                     diff & SOCKET_PCI_PRSNT_PIN1 ? "PRSNT_PIN1 " : "",
                     diff & SOCKET_PCI_PRSNT_PIN2 ? "PRSNT_PIN2 " : "",
                     diff & SOCKET_PCI_POWER_FAULT? "POWER_FAULT " : "",
                     diff & SOCKET_PCI_ATTENTION_STATE ? "ATTENTION " : "",
                     diff & SOCKET_PCI_HOTPLUG_CAPABLE ? "HOTPLUG_CAPABLE ":"",
                     diff & SOCKET_PCI_CONFIG_FAULT ? "CONFIG_FAULT " : "",
                     diff & SOCKET_PCI_BAD_FREQUENCY ? "BAD_FREQUENCY " : "",
                     diff & SOCKET_PCI_INTER_LOCKED ? "INTER_LOCKED " : "");
               }
            }
         }
      }
   }
   /* NOTREACHED */
}

u_int nomoreoutput = 0;

void
SigINTHandler(int sig, siginfo_t *info, ucontext_t *context)
{
    nomoreoutput = 1;
}

int
main(int argc, char *argv[])
{
   int ch,ret;
   extern char *optarg;
   char filename[SIZE];   /* XXX use pathconf */
   char pathname[SIZE];   /* XXX use pathconf */
   char ndaction[SIZE];
   time_t now;
   struct sigaction act;
   extern int statusfd;

#ifdef DMALLOC
   /* install atexit handler first */
   dmalloc_log_stats();
#endif

   /* did you update all of the tables? */
   /* assert(numvariables == MAX_BCFG_VARIABLES); */
   if (numvariables != MAX_BCFG_VARIABLES) {
      fatal("numvariables(%d) != MAX_BCFG_VARIABLES(%d)",
            numvariables,MAX_BCFG_VARIABLES);
   }
   statusfd=-1;
#if (NETISL == 1)
   netisl++;
#endif
   initsymbols();
   cfghasdebug=cfgdebug=cfgversion=0;
#if NCFGDEBUG == 1
   cfghasdebug++;
#endif
   insingle= 0;
   indouble= 0;
   bequiet = 1;  /* default action */
   candomore=1;  
   hflag=0;
   jflag=0;
   qflag=0;
   yflag=0;
   moremode=0; 
   incomment=0;
   rejectlist = NULL;     /* initially no rejected bcfg files */
   fflag=0;
   bflag=0;
   lflag=0;
   logfilefp=(FILE *)NULL;
   bcfglinenum = 1;
   section = NOSECTION;   /* initial starting section */
   quotestate = NOQUOTES; /* initially not in any quote */
   Wflag=0;
   RMtimeout=RMTIMEOUT;   /* default of 60 secs - see common.h */

   rootdirp=drvdirp=(DIR *) NULL;
   rootdp=drvdp=(struct dirent *) NULL;

   atexit(Cleanup);     /* put this function first so _exithandle will
                         * call it last (LIFO)
                         */
   atexit(ExitIfNOTBCFGError);

   PrimeErrorBuffer(1,"Initialization of ndcfg failed");

   while ((ch = getopt(argc, argv,"?abcd:ef:hijl:mnqvtxz:D:W:")) != EOF) {
      switch ((char) ch) {
         case 'a':
            aflag++;
            break;
         case 'b':
            bflag++;
            break;
         case 'c': 
            cflag++;   /* only useful with -f flag for single file 
                        * as this is default behaviour otherwise 
                        */
            break;
         case 'd':
            delim=*optarg;
            break;
         case 'e':
            errorsarefatal++;
            break;
         case 'f':   /* "one-file" mode */
            fflag++;
            strcpy(filename,optarg);
            break;
         case 'h':   /* show heading in output */
            hflag++;
            break;
         case 'i':   /* getstamp shows all information in .ndnote section */
            iflag++;
            break;
         case 'j':
            jflag++;
            break;
         case 'l':   /* log file */
            lflag++;
            logfilefp=fopen(optarg,"a");
            if (logfilefp == NULL) {
               error(NOTBCFG,"can not append to %s; errno=%d",optarg,errno);
               lflag=0;
            } else {
               setbuf(logfilefp,NULL);
               time(&now);
               fprintf(logfilefp,"Logfile started at %s",ctime(&now));
            }
            break;
         case 'm':   /* more mode on */
            moremode++; 
            break;
         case 'n':   /* no prompt in GetCommands */
            nflag++;
            break;
         case 'q':   /* don't do a loaddir of default dirs initially */
            qflag++;
            break;
         case 't':   /* enable TCL mode */
            tclmode++;
            break;
         case 'D':
            Dflag++;
            cfgdebug=atoi(optarg);
            break;
         case 'W':
            Wflag++;
            RMtimeout=atoi(optarg);
            break;
         case 'v':
            bequiet=0;
            break;
         case 'x':
            dangerousdetect = 1;
            break;
         case 'y':
            yflag++;
            break;
         case 'z':
            zflag++;
            strncpy(ndaction,optarg,SIZE);
            break;
         case '?':
         default:
            usage(argc, argv);
            break;

      }
   }


   if (getenv("__NDCFG_AFLAG") != NULL) aflag++;
   if (getenv("__NDCFG_BFLAG") != NULL) bflag++;
   if (getenv("__NDCFG_CFLAG") != NULL) cflag++;
   /* note lower case 'd' here to change deliminator */
   if (getenv("__NDCFG_dFLAG") != NULL) delim=*getenv("__NDCFG_dFLAG");
   if (getenv("__NDCFG_EFLAG") != NULL) errorsarefatal++;
   if (getenv("__NDCFG_FFLAG") != NULL) {
      fflag++;
      strcpy(filename,getenv("__NDCFG_FFLAG"));
   }
   if (getenv("__NDCFG_HFLAG") != NULL) hflag++;
   if (getenv("__NDCFG_IFLAG") != NULL) iflag++;
   if (getenv("__NDCFG_JFLAG") != NULL) jflag++;
   if (getenv("__NDCFG_LFLAG") != NULL) {
      lflag++;
      logfilefp=fopen(getenv("__NDCFG_LFLAG"),"a");
      if (logfilefp == NULL) {
         error(NOTBCFG,"can not append to %s; errno=%d",
               getenv("__NDCFG_LFLAG"),errno);
         lflag=0;
      } else {
         setbuf(logfilefp,NULL);
         time(&now);
         fprintf(logfilefp,"Logfile started at %s",ctime(&now));            
      }
   }
   if (getenv("__NDCFG_MFLAG") != NULL) moremode++; 
   if (getenv("__NDCFG_NFLAG") != NULL) nflag++;
   if (getenv("__NDCFG_QFLAG") != NULL) qflag++;
   if (getenv("__NDCFG_TFLAG") != NULL) tclmode++;
   if (getenv("__NDCFG_VFLAG") != NULL) bequiet=0;
   if (getenv("__NDCFG_XFLAG") != NULL) dangerousdetect=1;
   if (getenv("__NDCFG_YFLAG") != NULL) yflag++;
   if (getenv("__NDCFG_DFLAG") != NULL) {
            Dflag++;
            cfgdebug=atoi(getenv("__NDCFG_DFLAG"));
   }
   if (getenv("__NDCFG_WFLAG") != NULL) {
            Wflag++;
            RMtimeout=atoi(getenv("__NDCFG_WFLAG"));
   }
   if (getenv("__NDCFG_ZFLAG") != NULL) {
      zflag++;
      strncpy(ndaction,getenv("__NDCFG_ZFLAG"),SIZE);
   }

   if ((Dflag > 0) && (cfghasdebug == 0)) {
      notice("debugging not available");
      Dflag=0;
   } else if ((Dflag > 0) && (bequiet == 0)) {
      notice("cfgdebug now %d",cfgdebug);
   }
   if (fflag && !cflag) bequiet=0;

   /* if stdin or stdout isn't a tty then chances are we won't want
    * to see annoying ndcfg> prompts in our output.  
    */
   if ((!isatty(0)) || (!isatty(1))) nflag=1;

   /* if in TCL mode reset things to a known sane state for new netconfig 
    * we allow Wflag and bflag through thought as we don't reset it
    */
   if (tclmode) {   /* TCL mode, possibly sending output to a pipe */
      /* basically set everything to a rational default that won't
       * interfere with stdout for tcl mode processing but let
       * the  -b
       *      -d X 
       *      -j
       *      -l filename
       *      -W numsecs
       * options through if they were set via cmd line or environment variable
       * the -t option to enter tcl mode is intended for netcfg only, and
       * netcfg shouldn't have to specify a zillion flags on the cmd line.
       */
      aflag=0;
      cflag=0;      /* not applicable to netconfig/tcl mode */
      candomore=0;  /* tclmode doesn't stop to prompt user to hit key ever */
      hflag=0;      /* no headings in table output */
      iflag=0;      /* getstamp doesn't write additional info to stdout */
      moremode=0;   /* forcibly turn off more mode if -m supplied */
      nflag=1;      /* no prompt either */
      bequiet=1;    /* no verbose stuff */
      qflag=0;      /* always load default directories */
      fflag=0;      /* don't go into "one file" mode */
      errorsarefatal=0; /* don't die; just silently ignore them */
      Dflag=0;      /* turn off debugging */
      cfgdebug=0;   /* debugging level to 0 */
      dangerousdetect=0;   /* for isaautodetect */
      yflag=0;      /* no status messages */
      zflag=0;      /* turn off checks */
   } else {     /* probably not a pipe but talking to a real human */
      if (zflag == 0) get_terminal_info();
     /* if (!isatty(0)) nflag++; just in case we *are* reading from pipe */
   }

   /* kick start hpsl.  must call before getbustypes() which calls OpenResmgr
    * and we need to know what to do wrt itimers before then
    */
   hpsl_ERR=0;
#ifndef NDCFG_HOTPLUG_WORKS
   /* as of 7nov97 all of ndcfg's hotplug code hasn't been tested so 
    * for now don't attempt to use it at idinstall time.  
    * It *should* work though - famous last words :-)
    */
   /* note that libhpsl looks for HPSL_DEBUG environment variable too */
   if (getenv("__NDCFG_HOTPLUG") != NULL) {
      ret=hpsl_init_cont_info(&hpslcontinfo, &hpslcount);
   } else {
      hpsl_ERR=hpsl_eNOCONT;
      ret=HPSL_ERROR;  /* pretend no hot plug controllers on system */
   }
#else
   ret=hpsl_init_cont_info(&hpslcontinfo, &hpslcount);
#endif
   if (ret == HPSL_SUCCESS) {

      if ((bequiet == 0) && (zflag == 0)) {
         notice("hpsl_init_cont_info says %d controller(s)",hpslcount);
      }
      /* establish alarm clock handler.  function prototype doesn't understand
       * SA_SIGINFO extra arguments so we cast it to avoid warnings
       */
      act.sa_handler=(void (*)(int))HpslHandler;
      sigfillset(&act.sa_mask);
      act.sa_flags=SA_RESTART|SA_SIGINFO;
#ifdef SA_INTERRUPT
      act.sa_flags |= SA_INTERRUPT;   /* SunOS */
#endif
      if (sigaction(SIGALRM, &act, NULL) == -1) {
         fatal("main: sigaction for SIGALRM failed with %s",strerror(errno));
         /* NOTREACHED */
      }

      TurnOnItimers();
   } else {
      /* probably failed with hpsl_ERR set to hpsl_eNOCONT -- 
       * no hot plug controllers on the system 
       * note that /dev/hpci is an exclusive open device, so it's also 
       * normal for this routine to fail with hpsl_eDEVOPEN if we couldn't 
       * open up /dev/hpci, implying that someone else is using the device
       * at this time.  In any case, we cannot use the hpsl routines
       * to assign and remove MODNAME for this session of ndcfg.
       */
      if ((bequiet == 0) && (zflag == 0)) {
         notice("hpsl_init_cont_info fails with %d",hpsl_ERR);
      }
      hpslcount=0;
      nomoreitimers=1;
   }

   if (getbustypes()) {   /* sets global cm_bustypes */
      fatal("couldn't read CM_BUSTYPES"); 
      /* NOTREACHED */
   };

   if (zflag == 0) {
      InitPCIVendor();/* initialize hash table for pcishort/pcilong commands */
   } else {
      ClearError();
      exit(CheckResmgrAndDevs(ndaction));
      /* NOTREACHED */
   }

   if (fflag) {   /* just read 1 bcfg file and be verbose about it 
                   * typically used by vendors to test their bcfg file
                   */
#ifdef OLDWAY
      getcwd(pathname,SIZE);
      strcat(pathname,filename);
      read1bcfg(pathname);
#endif
      if (read1bcfg(filename)) {
         fatal("severe uncaught error while parsing %s",filename);
         /* NOTREACHED */
      }
      if (!cflag) {
         /* calling exit() below will call atexit->ExitIfNOTBCFGError but it
          * won't call _exit in ExitIfNOTBCFGError since
          * bcfghaserrors is only set when we call error() with reason of
          * anything *except* NOTBCFG.
          * XXX 
          */
         if (bcfgfileindex == 0) {
            /* error(anything but NOTBCFG) called previously 
             * or filename not absolute pathname
             * or problem finding/loading bcfg file
             * or bcfg file not a regular file
             */
            FreeMemory();
            exit(1);
         } else {
            Pstdout("ndcfg: no errors found in %s\n",filename);
            FreeMemory();
            exit(0);
         }
      }
   } else {
      /* we load all drivers in these hierarchies automatically upon startup */
      if (qflag == 0) {
         LoadDirHierarchy("/etc/inst/nd/mdi");       /* Gemini drivers here */
         LoadDirHierarchy("/etc/inst/nics/drivers"); /* UW2.x drivers here */
      }
   }
   fflag=0;  /* allow us to load additional files if -c -f foo supplied */

   act.sa_handler=(void (*)(int))SigINTHandler;
   sigfillset(&act.sa_mask);
   act.sa_flags=SA_RESTART|SA_SIGINFO;
#ifdef SA_INTERRUPT
   act.sa_flags |= SA_INTERRUPT;   /* SunOS */
#endif
   if (sigaction(SIGINT, &act, NULL) == -1) {
      fatal("main: sigaction for SIGINT failed with %s",strerror(errno));
      /* NOTREACHED */
   }

   ret=GetCommands();
   /* XXX check this exit */
   FreeMemory();

#ifdef DMALLOC
   /* prevent atexit(ExitIfNOTBCFGError) routine from doing anything. dmalloc
    * library atexit routine will run before Cleanup so we free
    * stdio buffers obtained in {getc/printf}->_filbuf->_findbuf->malloc 
    * by doing an fclose first before dmalloc atexit routine runs.
    * we still do the fclose in Cleanup.
    */
   fclose(stderr);
   fclose(stdout);
   fclose(stdin);
   OpGotError = 0; /* can't write to stderr any more so don't try later on */
#endif

   exit(ret);
}

extern char zztext[];
extern char yytext[];
extern int yyleng;

/* Pstderr exists to send output to stderr and also to the logfile if
 * set with -l option
 * cannot call error or notice since can be called from error or notice
 */
void
Pstderr(char *fmt, ...)
{
   va_list ap;
   char tmp[LISTBUFFERSIZE + 50];

   va_start(ap, fmt);
   vsnprintf(tmp, LISTBUFFERSIZE + 50, fmt, ap);
   va_end(ap);

   fprintf(stderr, tmp);

   if (logfilefp != NULL) {
      fprintf(logfilefp, tmp);
   }
}

/* Pstdout exists to send output to stdout and also to the logfile if
 * set with -l option
 * cannot call error or notice since can be called from error or notice
 */
void
Pstdout(char *fmt, ...)
{
   va_list ap;
   char tmp[LISTBUFFERSIZE + 50];

   /* if we got SIGINT no point in sending more stuff to screen */
   if (nomoreoutput) return;

   va_start(ap, fmt);
   vsnprintf(tmp, LISTBUFFERSIZE + 50, fmt, ap);
   va_end(ap);

   fprintf(stdout, tmp);

   if (logfilefp != NULL) {
      fprintf(logfilefp, tmp);
   }
}

/* notice is for non-error conditions that do not pertain to a bcfg file
 * name.  Or, if the message *does* pertain to a bcfg file, it isn't important
 * to name the associated bcfg file or flag it as bad.
 * Indeed, we call notice when the file name hasn't even been set yet!
 */
/* assumes everything passed on stack is a char *, so normal 
 * integers pushed on stack to be printed with %x or %d will not work 
 * NOTE THIS ROUTINE DOESN'T WORK WITH TCL ERROR HANDLING LIKE ERROR()
 * 
 * CANNOT CALL ERROR AS IT CAN BE CALLED FROM ERROR!
 */

int innotice;

void
notice(char *fmt, ...)
{
   va_list ap;
   char tmp[SIZE * 10];

   /* if we got SIGINT no point in sending more stuff to screen */
   if (nomoreoutput) return;

   /* eliminate any reentrancy problem since HpslHandler calls notice */
   BlockHpslSignals();   /* so we can safely call notice() in HpslHandler */
   
   va_start(ap,fmt);
   vsnprintf(tmp,SIZE * 10,fmt,ap);
   va_end(ap);

   if (!tclmode || !bequiet) {
      Pstderr("ndcfg: notice: %s\n",tmp);  /* which also logs it */
   } else {
      if (logfilefp != NULL) {
         fprintf(logfilefp,"ndcfg: notice: ");
         fprintf(logfilefp,tmp);
         fprintf(logfilefp,"\n");
      }
   }

   UnBlockHpslSignals();
   return;
}


char ErrorMessage[LISTBUFFERSIZE];  /* The Error Message */
u_int OpGotError;

void
ClearError(void)
{
   ErrorMessage[0]='\0';   /* strcpy(ErrorMessage,""); */
   OpGotError=0;
}


/* called from exit(x).  No more output can happen after this routine
 * is called
 * if DMALLOC is defined then its atexit routine _may_ run next, and
 * everything should be freed up.
 */
void
Cleanup(void)
{
   extern void EndStatus(void);

   /* tell kernel that any call to mdi_printcfg can write to screen */
   mdi_printcfg(1);

   /* free up memory that hpsl library uses if we allocated any
    * can't use global variables "drvCnt" or "drvNames" since both only 
    * incremented(drvCnt)/set(drvNames) when we call hpsl_resume_io 
    * and hpsl_bind_driver
    * The problem is that hpsl_cleanup blindly frees pointers that may not
    * have been set so we can't call it if not using hpsl library
    */
   /* if (hpsl_IsInitDone() == HPSL_SUCCESS) { */
   if (hpslcount > 0) {
      TurnOffItimers();
      hpsl_cleanup();   /* free() all memory used by hpsl library */
   }

   if (logfilefp != NULL) {
      fclose(logfilefp);
      logfilefp=NULL;
   }
   /* if catopen succeeded then call catclose */
   if ((catd != (nl_catd) -2) && (catd != (nl_catd) -1)) {
      (void) catclose(catd);  /* don't care about return value */
   }
   EndStatus();
   fclose(stderr);
   fclose(stdout);
   fclose(stdin);
}


/* called by atexit too.  Terminates program if error(NOTBCFG,... previously
 * called.  we print out error message buffer contents too
 */
void
ExitIfNOTBCFGError(void)
{
   /* if not in tcl mode then calls to error() will immediately go to stderr 
    * but only if verbose mode was set
    * if in tcl mode then we must print our error buffer like EndList() does.
    * if not in tcl mode and in quiet mode then print error that user missed
    * earlier.
    * OpGotError variable is set for any call to error with reason of NOTBCFG
    */
   if (OpGotError) {
      if (tclmode) {
         Pstderr("{%s: %s}\n",TCLERRORPREFIX,ErrorMessage);
      } else if (bequiet) {
         /* user did not see message since not in verbose mode. we print it 
          * now
          */
         Pstderr("%s\n",ErrorMessage);
      }
      currentline=0;
      ClearError();
      if (logfilefp != NULL) {
         fclose(logfilefp);
         logfilefp=NULL;
      }
      _exit(1);  /* can't use exit since we may be in atexit already */
      /* NOTREACHED */
   }
}

/* assumes everything passed on stack is a char *, so normal 
 * integers pushed on stack to be printed with %x or %d will not work 
 */
void
error(uint_t reason, char *fmt, ...)
{
   va_list ap;
   int nobanner=0;
   int noerrnl=0;
   char tmp[SIZE * 10];

   /* if we got SIGINT no point in sending more stuff to screen */
   if (nomoreoutput) return;

   if (reason & CONT) {
      nobanner=1;
      reason &= ~CONT;
   }

   if (reason & NOERRNL) {
      noerrnl=1;
      reason &= ~NOERRNL;
   }

   if (bequiet) {
      if (reason != NOTBCFG) {
         if (logfilefp != NULL) {
            fprintf(logfilefp,"ndcfg: error in %s: ",
                    bcfgfile[bcfgfileindex].location);
            /* from showrejects() code */
            fprintf(logfilefp,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
                    reason & SYNTAX     ? "syntax " : "",
                    reason & SECTIONS   ? "sections " : "",
                    reason & VERSIONS   ? "versions " : "",
                    reason & BADVAR     ? "bad_variable " : "",
                    reason & MULTDEF    ? "multiple_defines " : "",
                    reason & MULTVAL    ? "no_multivalue_variables " : "",
                    reason & TRUEFALS   ? "not_true/false " : "",
                    reason & NOTBUS     ? "na_bus " : "",
                    reason & NOTDEF     ? "mand_symb_not_def " : "",
                    reason & BADNUM     ? "bad_number " : "",
                    reason & BADODI     ? "bad_ODI " : "",
                    reason & ISANOTDEF  ? "mand_ISA_symb_not_def " : "",
                    reason & EISANOTDEF ? "mand_EISA_symb_not_def " : "",
                    reason & PCINOTDEF  ? "mand_PCI_symb_not_def " : "",
                    reason & MCANOTDEF  ? "mand_MCA_symb_not_def " : "",
#ifdef CM_BUS_I2O
                    reason & I2ONOTDEF  ? "mand_I2O_symb_not_def " : "",
#else
                    "",
#endif
                    reason & BADELF     ? "bad_ELF " : "",
                    reason & BADDSP     ? "bad_DSP " : "",
                    reason & CONT       ? "cont " : "",
                    reason & NOTBCFG    ? "notbcfg " : "");
         }
         rejectbcfg(reason);
      } else {   
         /* reason is NOTBCFG, meaning this is a general error 
          * not related to bcfg issues which we should keep track of
          * remember bequiet just means don't display it to user too
          */
         int length;

         va_start(ap, fmt);
         vsnprintf(tmp,SIZE * 10,fmt,ap);
         va_end(ap);
         if ((length=strlen(ErrorMessage))) strcat(ErrorMessage," - ");
         if (length + strlen(tmp) > LISTBUFFERSIZE) {
            notice("not enough room to add '%s' onto ErrorMessage[]",tmp);
         } else {
            strcat(ErrorMessage,tmp);  /* add new error text after old one */
         }
         OpGotError++;
      }
      return;
   }
   va_start(ap, fmt);
   vsnprintf(tmp,SIZE * 10,fmt,ap);
   va_end(ap);
   /* errors go to stderr -- too bad we must push arguments ourselves */
   if (!nobanner) {
      if (reason != NOTBCFG) {  /* so we have a filename to use */
         Pstderr("ndcfg: error in %s: ",bcfgfile[bcfgfileindex].location);
      } else {  /* no errorsallowed, so not processing a bcfg, so no filename */
         Pstderr("ndcfg: error: ");
      }
   }
   Pstderr("%s",tmp);
   if (!noerrnl) {
      Pstderr("\n");
   }
   /* do not exit
    * we can call error as a means of printing multiple lines for the
    * same error string too
    */
   /* we encountered an error.  note this is not a direct count of how many
    * errors since we can call this routine for multi-line prints that 
    * as a whole constitute one error
    */
   if (reason != NOTBCFG) {   
      rejectbcfg(reason);
   } else {   
      /* reason *is* NOTBCFG, meaning this is a general error 
       * not related to bcfg issues which we should keep track of
       * remember bequiet just means don't display it to user too
       */
      int length;

      if ((length=strlen(ErrorMessage))) strcat(ErrorMessage," - ");
      if (length + strlen(tmp) > LISTBUFFERSIZE) {
         notice("not enough room to add '%s' onto ErrorMessage[]",tmp);
      } else {
         strcat(ErrorMessage,tmp);  /* add new error text after old one */
      }
      OpGotError++;
   }
}

/* no multi-line calls here; we call exit 
 * fatal should not be called because of anything to do with bcfg files
 * it should only be called for internal error conditions
 * as such we don't print out a file name
 */
/* assumes everything passed on stack is a char *, so normal 
 * integers pushed on stack to be printed with %x or %d will not work 
 */
void
fatal(char *fmt, ...)
{
va_list ap;
char tmp[SIZE];

   BlockHpslSignals();  /* don't get interrupted by hpsl in pause() call */

   /* we ignore bequiet as this is a fatal event */
   va_start(ap,fmt);
   vsnprintf(tmp,SIZE,fmt,ap);
   va_end(ap);
   /* fatal messages go to stderr, except for tclmode, where we print to both 
    * to try and be as obnoxious as possible
    * when netcfg is running stderr will go to the user's screen so we'll
    * hopefully have some record of what happened there too.
    *
    * fatal messages are printed immediately, so while unlikely we could be 
    * in the middle of tcl output, so this could mess up our output.  But
    * some fatal event occurred, so we try and let the world know about it.
    * if logging is enabled then we'll know what happened.
    */

   if (tclmode) {
      /* send to stdout */
      Pstdout("{%s: fatal: %s - %s}",TCLERRORPREFIX,ErrorMessage,tmp);
      /* repeat to stderr */
      Pstderr("{%s: fatal: %s - %s}",TCLERRORPREFIX,ErrorMessage,tmp);
   } else {
      /* send to stdout */
      Pstdout("fatal: %s - %s\n",ErrorMessage,tmp);
      /* repeat to stderr */
      Pstderr("fatal: %s - %s\n",ErrorMessage,tmp);
   }

   fflush(stdout);
   fflush(stderr);
   if (logfilefp != NULL) {
      fclose(logfilefp);   /* since we're about to call _exit */
      logfilefp=NULL;
   }

   if (isatty(1)) {
      _exit(1);  /* no point in hanging or going through atexit functions */
   } else {
      /* probably writing to a pipe; block until we get a signal.  This
       * give netcfg a chance to read the error.  stdio buffers are 
       * flushed at this point.  We don't check if tclmode
       */
      pause(); /* wait for signal to arrive (other than blocked HPSL signal) */
      _exit(1); /* then die */
   }
   /* NOTREACHED */
}

zzerror(char *s)  /* our bcfg parser error routine */
{
   error(SYNTAX,"Error found at line %d token '%s':%s:", 
                  bcfglinenum, zztext, s);
}

yyerror(char *s) /* our cmd parser error routine */
{

   error(NOTBCFG,"ignoring %s at line %d:\"%s\"",s, cmdlinenum,yytext);
   if (logfilefp != NULL) {
      time_t now;

      time(&now);
      fprintf(logfilefp,"%s ignoring '%s' at line %d at %s",
          PROMPT,yytext,cmdlinenum,ctime(&now));
   }
}


#define CATNAME "lli.cat@sa"
#define set_id 6         /* see nd.msg */

/* the motivation is that we must internationalise ndcfg, but as of this
 * writing there are over 670 separate calls to error that would have
 * to be internationalized.  Rather than do each and every one which
 * would take too much time, I prime the error buffer with a more general
 * internalized string first so that if the command fails then we'll print
 * that out first, followed by the specific English-only error message that
 * is from the call(s) to error().
 * This routine is based on cmd-nd/intl/intl.c routine catfprintf
 */
void
PrimeErrorBuffer(int msg_id, char *fmt, ...)
{
   va_list ap;
   char *s;
   char tmp[SIZE];

   va_start(ap, fmt);
   vsnprintf(tmp,SIZE,fmt,ap);
   va_end(ap);

   ClearError();

   if (catd == (nl_catd) -2) {
      setlocale(LC_ALL, "");
      catd=catopen(CATNAME, NL_CAT_LOCALE);
      if ((catd == (nl_catd) -1) && (errno != ENOENT)) {
         notice("lli.cat: Unable to open message catalogue (%s)", CATNAME);
      }
   }
   s=tmp;  /* our default string */
   /* if there's any point in calling catgets then do so */
   if (catd != (nl_catd) -1) {
      s = catgets(catd, set_id, msg_id, tmp);
   }
   /* add s (either our localised or default message in tmp) onto default
    * error buffer in case command fails
    */
   strcpy(ErrorMessage,s);
   return;
}

int statusfd;

/* start sending status messages to filename*/
int
StartStatus(char *filename)
{
	/* filename should already exist... */
   if ((statusfd=open(filename,O_WRONLY|O_APPEND)) == -1) {
		error(NOTBCFG,"StartStatus: %s doesn't exist, errno=%d",filename,errno);
      return(-1);
	}
   StartList(2,"STATUS",10);
   AddToList(1,"success");
   EndList();
   return(0);
}

void
EndStatus(void)
{
   if (statusfd != -1) {
      close(statusfd);
	}
   return;
}

void
Status(int msg_id, char *fmt, ...)
{
   int length,loop;
   va_list ap;
   char *s;
   char tmp[SIZE];
   char tmp2[SIZE];

   if ((statusfd == -1) && (yflag == 0)) return;

   va_start(ap, fmt);
   vsnprintf(tmp,SIZE,fmt,ap);
   va_end(ap);

   if (catd == (nl_catd) -2) {
      setlocale(LC_ALL, "");
      catd=catopen(CATNAME, NL_CAT_LOCALE);
      if ((catd == (nl_catd) -1) && (errno != ENOENT)) {
         notice("lli.cat: Unable to open message catalogue (%s)", CATNAME);
      }
   }
   s=tmp;  /* our default string */
   /* if there's any point in calling catgets then do so */
   if (catd != (nl_catd) -1) {
      s = catgets(catd, set_id, msg_id, tmp);
   }
   if (yflag) {
      Pstdout("STATUS: %s",s);   /* will also send to logfile */
   } 

   /* give string s (either our localised or default message in tmp) to netcfg
    */
   if (statusfd != -1) {

      /* TCL freaks out when it encounters certain characters in the
       * returned string.  In particular, ErrorMessage can contain
       * the output from a command from runcommand() or docmd(), and
       * the output from the command can have illegal TCL characters.
       * we could escape the characters with a '\' but we instead 
       * substitute them with a space.  This way an error string
       * will not cause tcl to dump core.
       */
      length=strlen(s);
      for (loop=0; loop< length; loop++) {
         switch(s[loop]) {
            case '[':
            case ']':
            case '"':
            case '{':
            case '}':
               s[loop]=' ';
               break;
            default:
               break;
         }
      }
      snprintf(tmp2,SIZE,"{STATUS {%s}}\n",s);
      length=strlen(tmp2);

      write(statusfd, tmp2, length);
   }
   return;
}
