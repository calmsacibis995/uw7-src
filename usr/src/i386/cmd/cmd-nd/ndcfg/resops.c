#pragma ident "@(#)resops.c	29.6"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/* XXX need to send up a tcl STATUS message when resmgr is locked
 * by someone else and we are delaying waiting for it to be opened
 * unclear how this will be done so for now I just call Pstderr
 */
#ifdef DMALLOC
#define uint unsigned int
#define ulong unsigned long
#define boolean_t int
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <hpsl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stropts.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/mdi.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/mod.h>  /* MODMAXNAMELEN */
#include <sys/systeminfo.h>
#include <sys/hpci.h>
#include "common.h"

extern char delim;
extern u_int Wflag;
extern int RMtimeout;

/* highest /dev/foo_# or /dev/mdi/foo# that we can ever open */
#define MAXDRIVERSINLINKKIT 100

/* quick implementation of RMabort_trans(key).  This will break if resops.c
 * ever does a RMbegin_trans with multiple keys
 * OR if we ever multithread this program and have two threads calling 
 * RMbegin_trans; we'd need a mutex around this
 */
rm_key_t currentkey;


/* global variable denoting the backup key where all calls to
 * resput will be written to the backup key as well as the
 * regular key.  Will need a lock if multithreading ever happens
 */
rm_key_t g_backupkey=RM_KEY;



/* the following defines allow resops.c to be compiled on a UW2.x-BL6 machine */

#ifndef RM_READ
#define RM_READ 1
#endif

#ifndef RM_RDWR
#define RM_RDWR 1
#endif

#ifdef RMIOC_BEGINTRANS
#define RMbegin_trans(key, mode) (currentkey=key, RMbegin_trans(key, mode))
#else
#define RMbegin_trans(key, mode) 0
#endif

#ifdef RMIOC_ENDTRANS
#define RMend_trans(key) RMend_trans(key) 
#else
#define RMend_trans(key) 0
#endif

#ifdef RMIOC_ABORTTRANS

/* BL8:RMIOC_ABORTTRANS ioctl doesn't clear replay log so it effectively
 * leaves transaction in progress
 */
#define RMabort_trans(key) RMend_trans(key)

#if 0
/* fake this routine until it exists in libresmgr */
int
RMabort_trans(rm_key_t key) {
   int status;
   int resfd;
   struct rm_ioctl_args r;

   resfd=RMgetfd();
   if (resfd < 0) {
      return(EINVAL);
   }

   r.rma.rm_key = key;
   
   status=ioctl(resfd, RMIOC_ABORTTRANS, &r);
   if (status < 0) return(errno);
   return(0);
}
#endif

#else
#define RMabort_trans(key) 0
#endif

void
handler(int sig)
{
   /* write(1,"in handler\n",11); */
}

/* open up the resmgr.
 * Note that the RMopen routine does an open(2) on /dev/resmgr with
 * the desired mode.
 * this may normally fail due to your uid not having sufficient
 * priviledge but if you have the P_DACREAD/P_DACWRITE priviledge the
 * open will then succeed.  We don't explictly check for this priviledge;
 * the higher level commands already check for these priviledges
 * for other reasons and if a user is doing a resdump/resput command
 * then they should have appropriate permissions anyway.
 */
int
OpenResmgr(mode_t mode)
{
   int status,rmmode;
   int timeout,notified;
   struct sigaction act,oact;

   act.sa_handler=handler;
   sigfillset(&act.sa_mask);
   act.sa_flags=0;    /* no SA_RESTART desired here */
#ifdef SA_INTERRUPT
   act.sa_flags |= SA_INTERRUPT;  /* SunOS */
#endif
   TurnOffItimers();  /* because they send SIGALRM which we don't want here */
   if (sigaction(SIGALRM,&act,&oact) == -1) {
      status=errno;
      error(NOTBCFG,"OpenResmgr: sigaction failed with %d",status);
      TurnOnItimers();
      return(status);
   }
   status=1;
   notified=0;
   if (RMtimeout <= 0) RMtimeout=1;   /* always try once */
   timeout=RMtimeout;/* don't modify original RMtimeout */
   while((status != 0) && (timeout > 0)) {
      alarm(0);    /* no alarms here - ok to call */
      /* Gemini BL8+: mode doesn't matter when you open resmgr here
       * UW2.1-Gemini BL7:  mode does matter when you open resmgr here 
       */
      status=RMopen(mode);   /* returns 0 or errno value */
      if (status) {
         if (status == EINVAL) {
            /* we already opened the resmgr! -  but we'll try and bail out */
            notice("OpenResmgr: I've already opened the resmgr - trying again");
            (void) CloseResmgr();  /* commit existing, try open again */
            /* next time through the RMopen "should" succeed */
         } else if (status == ENOSPC) {
            error(NOTBCFG,"OpenResmgr: malloc in libresmgr failed");
            sigaction(SIGALRM,&oact,NULL);  /* restore Hpsl Handler */
            TurnOnItimers();   /* after the sigaction above! */
            return(status);   /* no point in continuing */
         } else if (status == EBUSY) {
            /* someone else has opened up the resmgr */
            if (notified == 0) {
               notified++;
               /* if in tcl mode or verbose mode is 0 then
                * we won't see any calls to notice() hence
                * the call to Pstderr below
                */
               Pstderr("{ ndcfg: STATUS: OpenResmgr: waiting up to %d seconds "
                       "for access to the resmgr because someone else is "
                       "currently using it.  I will try again once per "
                       "second until %d seconds elapse before returning "
                       "failure. }", 
                       timeout, timeout);
            }
            notice("OpenResmgr: waiting to open the resmgr (mode=%d) -- "
                          "%d seconds left",mode,timeout);
            alarm(1);  /* try again later, ok to call */
            pause();  /* wait for _any_ signal to come through */
         } else {
            error(NOTBCFG,"OpenResmgr: error %d(%s) trying to "
                          "open the resmgr (mode=%d)",
                  status,strerror(status),mode);
            sigaction(SIGALRM,&oact,NULL);  /* restore Hpsl Handler */
            TurnOnItimers();   /* after the sigaction above! */
            return(status);  /* no point in continuing */
         }
         timeout--;
      }
   }
   sigaction(SIGALRM,&oact,NULL);   /* restore HpslHandler */
   TurnOnItimers();   /* after the sigaction above! */
   if (status == EINVAL) {
      /* last open attempt (and it's likely that _all_ attempts) failed with
       * EINVAL.  We tried to CloseResmgr each time but that didn't solve the
       * underlying problem.  Give up.
       */
      error(NOTBCFG,"OpenResmgr: tried %d times to open resmgr but got EINVAL",
            RMtimeout);
      return(EINVAL);
   }
   if (timeout <= 0) {
      error(NOTBCFG,"OpenResmgr: someone else using resmgr, giving up after %d"
            " seconds",RMtimeout);
      return(ETIME);
   }
   /* RMopen succeeded to get here */

   return(0);
}

/* NOTE: since RMclose always returns 0 if resmgr was opened we don't check 
 * its returns value (via CloseResmgr) throughout this file
 */
int
CloseResmgr(void)
{
   int status;

   status=RMclose();    /* returns 0 or errno value */
   if (status == EINVAL) {
      /* "Major problem" - coding error on our part */
      notice("CloseResmgr:  I never opened the resmgr!");
      return(0);   /* fake a success so caller won't think an error occurred */
   }
   return(status);
}

/* required because libresmgr doesn't remove the typing information (,s)
 * from the parameter whose values we're trying to delete.
 * only works on one parameter at a time unlike libresmgr RMdelvals which
 * can work with a space delimited list of parameters.
 * "NETCFG_ELEMENT,s" is bigger than RM_MAXPARAMLEN so RMdelvals returns EINVAL
 * ditto with NETINFO_DEVICE,s
 * I bugreported this to mek but it's unclear when fix will occur.
 */
int
MyRMdelvals(rm_key_t key, const char *param_list)
{
   char tmp[VB_SIZE];
   char *commap;

   if (strstr(param_list, " ") != NULL) {
      /* we just hope that all parameters don't have typing information or
       * are less than RM_MAXPARAMLEN characters _with_ typing information
       */
      notice("MyRMdelvals: can't handle parameter list '%s'",param_list);
      return(RMdelvals(key, param_list));
   }

   strncpy(tmp,param_list,VB_SIZE);  /* could be larger than VB_SIZE */
   if ((commap=strstr(tmp, ",")) == NULL) {
      return(RMdelvals(key, param_list));
   }
   *commap='\0';
   return(RMdelvals(key, tmp));
}

/* RMIOC_NEXTKEY ioctl do not require begin_trans/end_trans wrappers
 */
int
MyRMnextkey(rm_key_t *keyp)
{
   int ret;

   /* RMbegin_trans(*keyp, RM_READ); not necessary */
   ret=RMnextkey(keyp);
   /* RMend_trans(*keyp); not necessary */
   return(ret);
}

/* since RMgetbrdkey calls RMIOC_NEXTKEY internally, we should have 
 * begin_trans/end_trans wrappers around it too.
 * the problem is that we don't yet know the key but we yet
 * we want to do a begin_trans to lock down that key!  Since
 * this is impossible, we cannot do a begin_trans
 * on an rm_key_t of NULL, so we comment out the begin_trans/end_trans.
 * this is the wart in the DDI8 libresmgr implementation.
 * thankfully the kernel allows us to do this without a begin_trans first.
 * - mek says use 0 as key for this just to keep kernel happy.
 */
int
MyRMgetbrdkey(char *modname, cm_num_t brdinst, rm_key_t *keyp)
{
   int ret;

   RMbegin_trans(0, RM_READ);
   ret=RMgetbrdkey(modname, brdinst, keyp);
   RMend_trans(0);
   return(ret);
}

/* RMnextparam uses the undocumented RMIOC_NEXTPARAM ioctl that libresmgr
 * doesn't have.  Likewise, the resmgr binary doesn't use it (but idconfupdate
 * does).  key is the key, n is the parameter to retrieve (starts at 0)
 * where says where to copy the string to.  size is how big that location is
 * Unfortunately this routine will not work with RM_KEY because 
 * rm_nextparam -> _rm_lookup_key looks in rm_hashtbl[] instead of rm_kptr[] 
 * which is where all of the RM_KEY parameters are stored :-(
 * Consequently, any parameter stored at key RM_KEY will not be saved to
 * /stand/resmgr by idconfupdate.
 * Late breaking news: nextparam doesn't require begin/end_trans
 */
int
RMnextparam(rm_key_t key, int n, char *where, int size) 
{
   int resfd;
   int ret, serrno;
#ifdef RMIOC_BEGINTRANS
   struct rm_ioctl_args r;

   r.rma.rm_key = key;
   r.mode = RM_READ;
   r.rma.rm_n = n;
#else
   struct rm_args r;

   r.rm_key = key;
   r.rm_n = n;
#endif

   resfd=RMgetfd();
   if (resfd < 0) {
      return(EINVAL);
   }

#if 0
#ifdef RMIOC_BEGINTRANS
   (void) ioctl(resfd, RMIOC_BEGINTRANS, &r); /* not necessary */
#endif
#endif

   serrno = ret = (ioctl(resfd, RMIOC_NEXTPARAM, &r) == 0) ? 0: errno;

#if 0
#ifdef RMIOC_ENDTRANS
   (void) ioctl(resfd, RMIOC_ENDTRANS, &r); /* not necessary */
#endif
#endif

   if (ret == 0) {
#ifdef RMIOC_BEGINTRANS
      strncpy(where, r.rma.rm_param, 
#else
      strncpy(where, r.rm_param,
#endif
              size > RM_MAXPARAMLEN ? RM_MAXPARAMLEN : size);
   }
   errno = serrno;
   return(ret);
}

/* note we don't print value for each parameter found 
 * returns 0 for success else errno
 */
int
DumpAllParams(rm_key_t rmkey)
{
   int n=0,valn;
   int status,GVstatus;
   char name[RM_MAXPARAMLEN];
   char asnum[SIZE];
   char asnumstr[VB_SIZE];
   char asrange[SIZE];
   char asrangestr[VB_SIZE];
   char asstring[SIZE];
   char asstringstr[VB_SIZE];
   char val_bufnum[VB_SIZE];
   char val_bufrange[VB_SIZE];
   char val_bufstr[VB_SIZE];
   char keyasstr[SIZE];
   char valnasstr[SIZE];

   snprintf(keyasstr,SIZE,"%d",rmkey);
   GVstatus=0;
   while ((status=RMnextparam(rmkey, n, name, RM_MAXPARAMLEN)) == 0) {
      /*Pstdout("key=%3d n=%3d param=%-*s\n",rmkey, n, RM_MAXPARAMLEN, name);*/
      snprintf(asnum,SIZE,"%s,n",name);
      snprintf(asrange,SIZE,"%s,r",name);
      snprintf(asstring,SIZE,"%s,s",name);
      valn=0;
again:

      memset(val_bufnum, '\0', VB_SIZE);
      memset(val_bufrange, '\0', VB_SIZE);
      memset(val_bufstr, '\0', VB_SIZE);

      /* Alas, it makes no difference in the order of how these are
       * executed: (string/number/range or any combination).  If it's
       * not a range then the high part of the range will likely be filled
       * in with junk (but hey, what do you expect?)  This is a feature
       * of libresmgr not clearing its internal buffer each time RMgetvals is
       * called
       */
      GVstatus=0;

      RMbegin_trans(rmkey, RM_READ);
      if ((GVstatus=RMgetvals(rmkey,asnum,valn,val_bufnum,VB_SIZE)) != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,"DumpAllParams: RMgetvals failed with %d",GVstatus);
            goto getalldone;
      }
      RMend_trans(rmkey);

      RMbegin_trans(rmkey, RM_READ);
      if ((GVstatus=RMgetvals(rmkey,asrange,valn,val_bufrange,VB_SIZE)) != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,"DumpAllParams: RMgetvals failed with %d",GVstatus);
            goto getalldone;
      }
      RMend_trans(rmkey);

      RMbegin_trans(rmkey, RM_READ);
      if ((GVstatus=RMgetvals(rmkey,asstring,valn,val_bufstr,VB_SIZE)) != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,"DumpAllParams: RMgetvals failed with %d",GVstatus);
            goto getalldone;
      }
      RMend_trans(rmkey);

      if ((strcmp(val_bufnum,"-") != 0) &&
          (strcmp(val_bufrange,"- -") != 0) &&
          (strcmp(val_bufstr,"-") != 0)) {
         char *z;
         /*
          * Pstdout("\tvaln=%d as number: '%s'\n",valn,val_bufnum);
          * Pstdout("\tvaln=%d as range : '%s'\n",valn,val_bufrange);
          * Pstdout("\tvaln=%d as string: '%s'\n",valn,val_bufstr);
          */
         /* make characters printable */
         for (z=val_bufstr;*z!='\0';z++) {
            if (!isprint(*z)) *z='?';
         }
         snprintf(valnasstr,SIZE,"%d",valn);
         AddToList(6,keyasstr,name,valnasstr,
                    val_bufnum,val_bufrange,val_bufstr);
         valn++;
         goto again;
      }
getalldone:
      n++;
   } 
   return(GVstatus);
}

uint_t cm_bustypes;

/* getbustypes set the global variable cm_bustypes which is an undocumented
 * bitmask that contains all installed busses on this machine 
 * same thing as reading _cm_bustypes from /dev/kmem...
 * returns 0 for success
 * else error number from OpenResmgr or RMgetvals
 * 
 * NOTE for UW2.1: boot and autoconf can't agree on how and if we should set
 *                 CM_BUS_ISA.  autoconf says set ISA if CM_BUS_MCA isn't set
 *                 boot says set ISA if either:
 *                              - dual mode microchannel/isa box
 *                              - If, besides PCI and PnP, we haven't found 
 *                                any buses, assume ISA
 *             Also, boot doesn't set ISA if EISA found:  I would have to
 *             set ISA if EISA found in string.
 *             This was fixed in Gemini BL14 where boot sets ISA if machine
 *             is capable of installing an ISA board into it.
 *
 * NOTE2:      since neither boot nor autoconf deal with PCCARD/PCMCIA
 *             devices yet we don't have any way of recognising that the machine
 *             has this bus type in it.  We do 3 things:
 *             1) pretend that if we have an ISA bus we have a PCCARD bus
 *                (this isn't true for dual mode MCA boxes but so what)
 *             2) convert any BUS=PCCARD in .bcfg files back to BUS=ISA in
 *                EnsureBus so that ndcfg will accept it.
 *             3) look for PCCARD/PCMCIA in busstr if Gemini as it may exist
 */
int
getbustypes(void)
{
   int status,ret;
   char string[RM_MAXPARAMLEN+2];
   char busstr[100];

   cm_bustypes = CM_BUS_UNK;  /* initial state:  no known busses */

#ifdef SI_BUSTYPES    /* gemini only */
   /* example string:  PCI2.10,EISA,PnP1.0    */
   busstr[0]=' ';  /* for ISA vs EISA check; make sure if ISA first that no 'E'
                    * character is before it on stack 
                    */
   busstr[1]='\0';
   if ((sysinfo(SI_BUSTYPES, busstr + 1, 99) != -1) && (strlen(busstr) > 1)) {
      char *foo, *newstart;

      /* we need to distinguish ISA from EISA in string - ISA could be first */
      newstart=busstr;
      while ((foo=strstr(newstart, "ISA")) != NULL) {
         if (*(foo - 1) != 'E') {
            cm_bustypes |= CM_BUS_ISA;
            break;
         }
         newstart=foo+1;
      }
      if (strstr(busstr, "EISA") != NULL) {
         cm_bustypes |= CM_BUS_EISA;
      }
      if (strstr(busstr, "PCI") != NULL) {
         cm_bustypes |= CM_BUS_PCI;
      }

      /* this may be added before FCS; I'm not sure what it will say though */
      if ((strstr(busstr, "PCCARD") != NULL) ||
          (strstr(busstr, "PCMCIA") != NULL)) {
         cm_bustypes |= CM_BUS_PCMCIA;
      }
      /* see sprintf(p, ",PnP%B.%B", pnpver >> 4, pnpver & 0x0F); 
       * in usr/src/work/boot/blm/platform.c
       */
      if (strstr(busstr, "PnP") != NULL) {
         cm_bustypes |= CM_BUS_PNPISA;
      }
      if (strstr(busstr, "MCA") != NULL) {
         cm_bustypes |= CM_BUS_MCA;
      }

      /* nothing to do for CM_BUS_SYS */

      /* I2O osmmsg.c function OSMMsgGetOldKeys() now calls 
       * si_add_bustype("I2O"); so don't just look for CM_BUS_PCI anymore
       */
      if (strstr(busstr, "I2O") != NULL) {
         cm_bustypes |= CM_BUS_I2O;   /* gemini only, no #ifdef needed */
      }
   } else {
      /* this should always succeed on Gemini */
      fatal("getbustypes: sysinfo(2) failed");
      /* NOTREACHED */
   }
#else    /* UW2.1, must read RM_KEY to obtain bus information */
   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "getbustypes: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }
   /* since CM_BUSTYPES isn't known to libresmgr we must help it out by
    * giving it a type for this variable
    */
   strcpy(string,CM_BUSTYPES);
   strcat(string,",s");   /* treat CM_BUSTYPES as a string */
   RMbegin_trans(RM_KEY, RM_READ);
   status=RMgetvals(RM_KEY, string, 0, &cm_bustypes, sizeof(cm_bustypes));
   if (status) {
      RMabort_trans(RM_KEY);
      error(NOTBCFG, "getbustypes: RMgetvals returned %d",status);
      (void)CloseResmgr();   /* if we use iopened it check before closing! */
      return(status);
   }
   RMend_trans(RM_KEY);
   (void)CloseResmgr();

#ifdef CM_BUS_I2O   /* not likely to exist on UW2.1 */
   if (cm_bustypes & CM_BUS_PCI) {
      cm_bustypes |= CM_BUS_I2O;
   }
#endif

   /* nothing sets CM_BUS_PNPISA in autoconf and no bcfg file says BUS=PNPISA */

#endif

   /* both autoconf and boot doesn't know about PCCARD/PCMCIA buses yet so
    * pretend that this machine has one if it's feasible that it could
    * this is largly for the Xircom and 3Com drivers:  
    * XPSODI.bcfg and C3589.bcfg from UW2.1.
    * EnsureBus() won't immediately reject these .bcfg files simply by
    * having BUS=PCCARD but will convert them to ISA because of ADDRM=true
    * and AUTOCONF=false
    */
   if (cm_bustypes & CM_BUS_ISA) {
      cm_bustypes |= CM_BUS_PCMCIA;
   }
 
   /* if we didnt' find any buses by now, we have a major problem */
   if (cm_bustypes == CM_BUS_UNK) {
      fatal("getbustypes: no buses found, aborting");
      /* NOTREACHED */
   }

   return(0);
}

/* returns 0 for success else number for failure */
int
resdump(char *mykey)
{
   int status,NKstatus,GVstatus;
   rm_key_t rmkey;
   char val_buf[VB_SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "resdump: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }
   /* since rm_key_t is an unsigned integer, you can't use NEXTKEY on it as
    * it will be treated as a positive number and rm_nextkey will set errno to
    * EINVAL since it considers the key you're requesting to be bigger than 
    * the rm_next_key variable.  So we must treat RM_KEY as special.
    * unfortunately DumpAllParams(RM_KEY) doesn't work.  see comments above.
    */
   StartList(12,"KEY",4,"PARAMNAME",16,"VALN",3,
                "ASNUMBER",12,"ASRANGE",24,"ASSTRING",1); /*1 is intentional*/
   if (mykey == NULL) {   /* dump all */
      rmkey = NULL;

      while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
         if ((status=DumpAllParams(rmkey)) != 0) { /* calls AddToList */
            error(NOTBCFG,"resdump: DumpAllparams returned %d",status);
            CloseResmgr();
            return(status);
         }
      }
   } else {
      rmkey = atoi(mykey);
      if (rmkey < 2) {
         error(NOTBCFG,"keys start at 2");
         (void) CloseResmgr();  /* if we used iopened it check this! */
         return(999);
      }
      if ((status=DumpAllParams(rmkey)) != 0) {      /* calls AddToList */
         error(NOTBCFG,"resdump: DumpAllparams returned %d",status);
         CloseResmgr();
         return(status);
      }
   }
   EndList();
   (void)CloseResmgr();
   return(0);
}

/* add hardware information onto given text description.  caller
 * must ensure that there will be enough space to add this information
 * onto the end of descr.
 * used by resshowunclaimed command and also when writing out 
 * /usr/lib/netconfig/info/netX file DESCRIPTION= text.
 * This routine will call error() if Really Bad Things(tm) happen
 * returns 0 for success
 * else errno number
 */
int
AddHWInfoToName(rm_key_t rmkey, char *descr)
{
   int iopenedit=0,ec,status;
   char brdbustypestr[VB_SIZE];
   char ioaddr[VB_SIZE];        /* ISA only */
   char irq[VB_SIZE];           /* ISA only */
   char memaddr[VB_SIZE];       /* ISA only */
   char dma[VB_SIZE];           /* ISA only */
   char busnumstr[VB_SIZE];     /* PCI only */
   char devnumstr[VB_SIZE];     /* PCI only */
   char funcnumstr[VB_SIZE];    /* PCI/EISA only */
   char subbrdidstr[VB_SIZE];   /* PCI 2.1 only */
   char devconfigstr[VB_SIZE];   /* only used by I2O */
   char slotstr[VB_SIZE];       /* all, note we use resmgr slot for MCA */
   u_int brdbustype;
   char line[SIZE];
   char tmp[30];
   char *foo;

   if (RMgetfd() == -1) {  /* since we're only called from 
                            * resshowunclaimed which does an OpenResmgr
                            * this is largely irrelevant, since we're
                            * never going to set iopenedit to 1
                            */
      iopenedit=1;
      status=OpenResmgr(O_RDONLY);
      if (status) {
         error(NOTBCFG, "AddHWInfoToName: OpenResmgr failed with %d(%s)",
               status,strerror(status));
         return(status);
      }
   }

   line[0]='\0';
   /* get CM_BRDBUSTYPE for this key.  we only support autodetectable 
    * boards, and the autoconf mgr sets these as read-only
    * in the resmgr, so if it's not set, return...
    */
   RMbegin_trans(rmkey, RM_READ);
   ec=RMgetvals(rmkey,CM_BRDBUSTYPE,0,brdbustypestr,VB_SIZE);
   if (ec != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,
         "AddHWInfoToName: RMgetvals for BRDBUSTYPE returned %d",ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(ec);
   }
   RMend_trans(rmkey);

   /* if this isn't a board type we know about then continue on */
   brdbustype=atoi(brdbustypestr);/* will fail with 0 if not set("-") */
   if (brdbustype != CM_BUS_EISA && 
       brdbustype != CM_BUS_MCA  &&
#ifdef CM_BUS_I2O
       brdbustype != CM_BUS_I2O  &&
#endif
       brdbustype != CM_BUS_ISA  &&
       brdbustype != CM_BUS_PCMCIA &&
       brdbustype != CM_BUS_PNPISA &&
       brdbustype != CM_BUS_PCI) {
      /* CM_BUS_UNK, CM_BUS_SYS, or some new bus we don't know about */

      if (iopenedit == 1) (void) CloseResmgr();
      return(0); /* not a board type we know about. This is a non-error.
                  * gethwkey and promiscuous depend on this being a non-error
                  */
   }

   /* now try for all of the probable board ID information.  don't fail
    * if value isn't set or isn't applicable to the given brdbustype,
    * we'll sort it out in the end.
    */
   busnumstr[0]='\0';
   devnumstr[0]='\0';
   funcnumstr[0]='\0';
   devconfigstr[0]='\0';
   subbrdidstr[0]='\0';
   slotstr[0]='\0';
   ioaddr[0]='\0';
   irq[0]='\0';
   memaddr[0]='\0';
   dma[0]='\0';

   RMbegin_trans(rmkey, RM_READ);
   (void) RMgetvals(rmkey,CM_BUSNUM,0,busnumstr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_DEVNUM,0,devnumstr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_FUNCNUM,0,funcnumstr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_CA_DEVCONFIG,0,devconfigstr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_SBRDID,0,subbrdidstr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_SLOT,0,slotstr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_IOADDR,0,ioaddr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_IRQ,0,irq,VB_SIZE);
   (void) RMgetvals(rmkey,CM_MEMADDR,0,memaddr,VB_SIZE);
   (void) RMgetvals(rmkey,CM_DMA,0,dma,VB_SIZE);
   RMend_trans(rmkey);

   if (busnumstr[0] == '-') busnumstr[0]='\0';
   if (devnumstr[0] == '-') devnumstr[0]='\0';
   if (funcnumstr[0] == '-') funcnumstr[0]='\0';
   if (devconfigstr[0] == '-') devconfigstr[0]='\0';
   if (subbrdidstr[0] == '-') subbrdidstr[0]='\0';
   if (slotstr[0] == '-') slotstr[0]='\0';
   if (ioaddr[0] == '-') {
      ioaddr[0]='\0';
   } else {
      /* there's something there so replace the space with a dash */
      if ((foo=strstr(ioaddr," ")) != NULL) *foo='-';
   }
   if (irq[0] == '-') {
      irq[0]='\0';
   } else {
      /* irq isn't a range -- no dash needed */
   }
   if (memaddr[0] == '-') {
      memaddr[0]='\0';
   } else {
      /* there's something there so replace the space with a dash */
      if ((foo=strstr(memaddr," ")) != NULL) *foo='-';
   }
   if (dma[0] == '-') {
      dma[0]='\0';
   } else {
      /* dma isn't a range -- no dash needed */
   }

#ifdef CM_BUS_I2O
   if (brdbustype == CM_BUS_I2O) {

      strcat(line,"-I2O");

      if (strlen(devconfigstr)) {
         u_long devconfig;
         char *strend;
         
         if (((devconfig=strtoul(devconfigstr,&strend,10))==0) &&
              (strend == devconfigstr)) {
            notice("AddHWInfoToName: bad I2O devconfig %s",devconfigstr);
         } else {
            snprintf(tmp,30," IOP=0x%x VER=0x%x TID=0x%x",
               /* yes, this fits in 30.  see ca.c  function ca_print_i2o */
               (devconfig >> 16) & 0xffff, 
               (devconfig & 0xf000) >> 12,
               devconfig & 0xfff);
            strcat(line,tmp);
         }
      }

      /* if these happen to be set we'll include them too */
      if (strlen(slotstr)) {
         snprintf(tmp,30," Slot %s",slotstr); 
         strcat(line,tmp);
      }
      if (strlen(busnumstr)) {
         snprintf(tmp,30," Bus %s",busnumstr); 
         strcat(line,tmp);
      }
      if (strlen(devnumstr)) {
         snprintf(tmp,30," Device %s",devnumstr); 
         strcat(line,tmp);
      }
      if (strlen(funcnumstr)) {
         snprintf(tmp,30," Function %s",funcnumstr); 
         strcat(line,tmp);
      }
      /* skip subbrdidstr */
   } else
#endif
   if (brdbustype == CM_BUS_PCI) {

      /* The idea is "If it's set in the resmgr, add it to the string" */
      strcat(line,"-PCI");

      /* Print slot first if set because users generally know what it is 
       * more than Bus/Device/Function.  Besides, if displaying in 
       * CHARM mode Bus/Device/Function will be truncated and slot wouldn't
       * be visible at all.
       */
      if (strlen(slotstr)) {
         snprintf(tmp,30," Slot %s",slotstr); 
         strcat(line,tmp);
      }
      if (strlen(busnumstr)) {
         snprintf(tmp,30," Bus %s",busnumstr); 
         strcat(line,tmp);
      }
      if (strlen(devnumstr)) {
         snprintf(tmp,30," Device %s",devnumstr); 
         strcat(line,tmp);
      }
      if (strlen(funcnumstr)) {
         snprintf(tmp,30," Function %s",funcnumstr); 
         strcat(line,tmp);
      }
      /* skip subbrdidstr */
   } else if (brdbustype == CM_BUS_EISA) {
      strcat(line,"-EISA");
      if (strlen(slotstr)) {
         snprintf(tmp,30," Slot %s",slotstr); 
         strcat(line,tmp);
      }
      /* Functionnum is always set to 0 in ca/eisa/eisaca.c */
   } else if (brdbustype == CM_BUS_MCA) {
      strcat(line,"-MCA");
      if (strlen(slotstr)) {
         /* CM_SLOT is correct for drivers, but users don't see 0-based
          * slots on MCA boxes.  increment slot by one to match the labels on
          * the back of the box.
          */
         snprintf(tmp,30," Slot %d",atoi(slotstr)+1);
         strcat(line,tmp);
      }
   } else if (brdbustype == CM_BUS_ISA) {
      strcat(line,"-ISA");
      /* do these in order of importance: in CHARM screen can be truncated */
      if (strlen(ioaddr)) {
         strtoupper(ioaddr);
         snprintf(tmp,30," I/O %s",ioaddr);
         strcat(line,tmp);
      }
      if (strlen(irq)) {
         strtoupper(irq);
         snprintf(tmp,30," IRQ %s",irq);
         strcat(line,tmp);
      }
      if (strlen(memaddr)) {
         strtoupper(memaddr);
         snprintf(tmp,30," MEM %s",memaddr);
         strcat(line,tmp);
      }
      if (strlen(dma)) {
         strtoupper(dma);
         snprintf(tmp,30," DMA %s",dma);
         strcat(line,tmp);
      }
   } else if (brdbustype == CM_BUS_PCMCIA) {
      strcat(line,"-PCCARD");
      /* do these in order of importance: in CHARM screen can be truncated */
      if (strlen(ioaddr)) {
         strtoupper(ioaddr);
         snprintf(tmp,30," I/O %s",ioaddr);
         strcat(line,tmp);
      }
      if (strlen(irq)) {
         strtoupper(irq);
         snprintf(tmp,30," IRQ %s",irq);
         strcat(line,tmp);
      }
      if (strlen(memaddr)) {
         strtoupper(memaddr);
         snprintf(tmp,30," MEM %s",memaddr);
         strcat(line,tmp);
      }
      if (strlen(dma)) {
         strtoupper(dma);
         snprintf(tmp,30," DMA %s",dma);
         strcat(line,tmp);
      }
   } else if (brdbustype == CM_BUS_PNPISA) {
      strcat(line,"-PnPISA");
      /* do these in order of importance: in CHARM screen can be truncated */
      if (strlen(ioaddr)) {
         strtoupper(ioaddr);
         snprintf(tmp,30," I/O %s",ioaddr);
         strcat(line,tmp);
      }
      if (strlen(irq)) {
         strtoupper(irq);
         snprintf(tmp,30," IRQ %s",irq);
         strcat(line,tmp);
      }
      if (strlen(memaddr)) {
         strtoupper(memaddr);
         snprintf(tmp,30," MEM %s",memaddr);
         strcat(line,tmp);
      }
      if (strlen(dma)) {
         strtoupper(dma);
         snprintf(tmp,30," DMA %s",dma);
         strcat(line,tmp);
      }
   }
   /* if a new bus comes along we won't print out any of its information
    * and the strcat won't do anything (line set to null at start) 
    */
   strcat(descr,line);   /* caller guarantees there's room! */
   if (iopenedit == 1) CloseResmgr();
   return(0);
}

/* if AUTOCONF=false is set in the bcfg file then the BUS was changed to ISA 
 * earlier in EnsureBus.  This will prevent a PCI/EISA/MCA card from
 * being displayed in the output below.
 * Otherwise, display matching bcfg based on unclaimed boards in the resmgr
 * We check MODNAME and BRDBUSTYPE vs the bcfg file and
 * output the bcfg NAME as well as any other information that was
 * set in the resmgr for the particular bus type.  If it isn't set, then
 * don't add it to the displayed string.
 * topoarg is only set if fromShowTopo is 1.
 * we return the number of boards found or -1 if error.
 * lanwanstr can be NULL if all are desired otherwise just do that particular
 * thing (either LAN or WAN)
 */
int
ResShowUnclaimed(int fromShowTopo, char *topoarg, char *lanwanstr) 
{
   rm_key_t rmkey;
   int NKstatus,status,ec, tmpirq, zz, yy;
   int bcfgloop,found,nthvalue;
   char *maxbdstr;
   int topoloop;
   u_int goodtopo;
   int numfound=0;
   int lanwan;
   int boardnumber,err;

   if (lanwanstr == NULL) {
      lanwan = LAN|WAN;   /* show both */
   } else {
      strtoupper(lanwanstr);
      if (strcmp(lanwanstr,"LAN") == 0) {
         lanwan = LAN;
      } else if (strcmp(lanwanstr,"WAN") == 0) { 
         lanwan = WAN;
      } else {
         error(NOTBCFG,"lanwan string must be 'lan' or 'wan'");
         return(-1);
      }
   }
   rmkey = NULL;
   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResShowUnclaimed: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }
   if (fromShowTopo == 0) {
      StartList(14,"KEY",3,
                "BCFGINDEX",4,
                "DRIVER_NAME",10,
                "NAME",58,
                "NCFGELEMENT",12,
                "BUS",5,
                "TOPOLOGIES",15);
   }
   found=0;
   nthvalue=0;  /* we only check first value for any given parameter */

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      char element[VB_SIZE];
      char elementstr[VB_SIZE];
      char brdid[VB_SIZE];
      char subbrdid[VB_SIZE];
      char brdbustypestr[VB_SIZE];
      rm_key_t junk;
      u_int brdbustype;

      /* since most keys will not have CM_NETCFG_ELEMENT assigned to them 
       * we look for that first for performance reasons.  If CM_NETCFG_ELEMENT
       * is set then continue on.  We can't use MODNAME anymore for criteria.
       */
      snprintf(elementstr,VB_SIZE,"%s,s",CM_NETCFG_ELEMENT);

      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,elementstr,nthvalue,element,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "ResShowUnclaimed: RMgetvals for NETCFG_ELEMENT returned %d",ec);
         (void) CloseResmgr();  /* if we used iopened it check this first! */
         return(-1);
      }
      RMend_trans(rmkey);
      /* CM_NETCFG_ELEMENT is type STR_VAL so we'll get "-" from RMgetvals if
       * it's not set.  If element has been set then continue; we've configured
       * this board with ndcfg earlier.
       * DDI8 suspended drivers will have MODNAME set to - but everything
       * else is still present in the resmgr.  So we look for NETCFG_ELEMENT
       * and if it's set then we know this board has been configured
       * so skip it.  Note we remove NETCFG_ELEMENT at board removal!
       * this also skips any boards that are configured but not suspended.
       */
      if (strcmp(element,"-") != 0) {
         continue;
      }

      /* now get CM_BRDBUSTYPE for this key.  we only support autodetectable 
       * boards, and the autoconf mgr sets these as read-only
       * in the resmgr, so if it's not set, continue on...
       */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_BRDBUSTYPE,nthvalue,brdbustypestr,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "ResShowUnclaimed: RMgetvals for BRDBUSTYPE returned %d",ec);
         (void) CloseResmgr();/* if we ever use iopenedit check this first! */
         return(-1);
      }
      RMend_trans(rmkey);

      /* if this isn't an autodetectable board type then continue on */
      brdbustype=atoi(brdbustypestr);/* will fail with 0 if not set("-") */
      if (brdbustype != CM_BUS_EISA && 
          brdbustype != CM_BUS_MCA  &&
#ifdef CM_BUS_I2O
          brdbustype != CM_BUS_I2O  &&
#endif
          brdbustype != CM_BUS_PCI) continue;  /* not autodetectable board */

      /* ok, we have a smart bus (PCI/EISA/MCA) key.  ensure some other
       * ISA board isn't using its IRQ.  We do this by checking ITYPE.
       */
      if (irqatkey(rmkey, &tmpirq, 0) == -1) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ResBcfgUnclaimed: irqatkey failed");
         (void) CloseResmgr();
         return(-1);
      }
      if (tmpirq != -2) {
         if ((yy=irqsharable(rmkey, tmpirq, 0)) == -1) {
            RMabort_trans(rmkey);
            error(NOTBCFG,"ResBcfgUnclaimed: irqsharable failed");
            (void) CloseResmgr();
            return(-1);
         }
         if (yy == 0) {
            notice("skipping resmgr key %d because somebody else using its IRQ "
                   "and won't share it",rmkey);
            continue;
         }
      }

      /* get the board id */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_BRDID,nthvalue,brdid,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ResShowUnclaimed: RMgetvals for BRDID returned %d",
               ec);
         (void) CloseResmgr();/* if we ever use iopenedit check this first! */
         return(-1);
      }
      RMend_trans(rmkey);
      /* if brdid isn't set we don't abort check -- could have sbrdid only */
      /* For you PCI 2.1 fans, we'll use either board id or subsystem board
       * id when comparing against the bcfg file.  so if brdid isn't
       * set above don't panic 
       */
     
      /* try for the sub board id */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_SBRDID,nthvalue,subbrdid,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ResShowUnclaimed: RMgetvals for SBRDID returned %d",
               ec);
         (void) CloseResmgr();/* if we ever use iopenedit check this first! */
         return(-1);
      }
      RMend_trans(rmkey);
      /* if subbrdid isn't set we don't abort check -- could have brdid only */

      /* if both brdid and subbrdid aren't set then we can worry
       * Since CM_MODNAME, CM_BRDID, and CM_SBRDID are all of type STR_VAL
       * libresmgr will set the buffer to the text "-" if it is not set 
       * in the resmgr. see libresmgr RMgetvals code
       */
      if (!strcmp(brdid,"-") && !strcmp(subbrdid, "-")) {
         error(NOTBCFG,
       "ResShowUnclaimed: both BRDID and SBRDID aren't set for resmgr key %d!",
               rmkey);
         continue;   
      }

      /* we convert board ids to lower case in bcfgops.c.  by converting
       * again here the call to HasString below will not have case
       * conversion problems
       */
      strtolower(brdid);
      strtolower(subbrdid);

      /* try for match with either one against our loaded bcfg files */
      for (bcfgloop=0;bcfgloop<bcfgfileindex; bcfgloop++) {
         if ((strlen(brdid) && 
             (HasString(bcfgloop,N_BOARD_IDS,brdid,0) == 1)) ||
             (strlen(subbrdid) && 
             (HasString(bcfgloop,N_BOARD_IDS,subbrdid,0) == 1))) {
            char keyasstr[10];
            char bcfgloopasstr[10];
            char ncfgelement[20];
            char *driver_name;
            char *topologies;
            char *name;
            char *bus;

            /* final check: match BUS= in bcfg with CM_BRDBUSTYPE */
            bus=StringListPrint(bcfgloop,N_BUS);
            if (bus == NULL) continue; /* BUS is mandatory, shouldn't happen */
            /* we treat CM_BRDBUSTYPE as bitmask here and in EnsureBus */
            if (bcfgbustype(bus) & brdbustype) { /*assumes BUS single valued*/
               char LongName[SIZE+NDNOTEMAXSTRINGSIZE];
               int zz;

               snprintf(keyasstr,10,"%d",rmkey);
               snprintf(bcfgloopasstr,10,"%d",bcfgloop);
               driver_name=StringListPrint(bcfgloop,N_DRIVER_NAME);
               name=StringListPrint(bcfgloop,N_NAME);
               if (name == NULL) {
                  snprintf(LongName,SIZE+NDNOTEMAXSTRINGSIZE,
                           "Unknown Card%s",
                           bcfgfile[bcfgloop].driverversion);
               } else {
                  snprintf(LongName,SIZE+NDNOTEMAXSTRINGSIZE,
                           "%s%s",name,
                           bcfgfile[bcfgloop].driverversion);
               }
               if (AddHWInfoToName(rmkey,LongName) != 0) {
                  notice("ResShowUnclaimed: AddHWInfoToName returned error");
                  free(bus);
                  if (name != NULL) free(name);
                  if (driver_name != NULL) free(driver_name);
                  continue;
               }
               topologies=StringListPrint(bcfgloop,N_TOPOLOGY);

               if (topologies == NULL) {
                  notice("bogus topology for bcfgindex %d",bcfgloop);
                  free(bus);
                  if (name != NULL) free(name);
                  if (driver_name != NULL) free(driver_name);
                  continue;
               }
               /* does this bcfg file support the given topology desired by
                * the user?
                */
               goodtopo=0; /* another reason LAN or WAN can't start with zero */
               for (topoloop=0; topoloop<numtopos; topoloop++) {
                  if (strstr(topologies, topo[topoloop].name) != NULL) {
                     goodtopo |= topo[topoloop].type;
                  }
               }
               /* if no match, this bcfg file doesn't support what we want - 
                * continue on 
                */
               if ((goodtopo & lanwan) == 0) {
                  if (driver_name != NULL) free(driver_name);
                  if (name != NULL) free(name);
                  if (topologies != NULL) free(topologies);
                  free(bus);
                  continue;   /* not a topology type we want to see */
               }

               /* now see if bcfg file cannot support another instance of
                * this driver (i.e. MAX_BD reached).  If so then don't 
                * display board in list.
                */
               if ((zz=AtMAX_BDLimit(bcfgloop)) != 0) { /* error or at limit */ 
                  if (zz == -1) {
                     /* error case */
                     notice("ResShowUnclaimed: AtMAX_BDLimit returned -1");
                     free(bus);
                     if (driver_name != NULL) free(driver_name);
                     if (name != NULL) free(name);
                     if (topologies != NULL) free(topologies);
                     continue;  /* error -- error() already called */
                  }
                  /* at MAX_BD limit */
                  notice("not displaying smart board '%s'(%s); driver at "
                         "MAX_BD limit", LongName,driver_name);
                  if (driver_name != NULL) free(driver_name);
                  if (name != NULL) free(name);
                  if (topologies != NULL) free(topologies);
                  free(bus);
                  continue;   /* at limit so continue */
               }
               /* MAX_BD not defined or we're within limit.  print info.  */
               if (HasString(bcfgloop, N_TYPE, "MDI", 0) == 1) {
                  /* element has netX that _would_ be used if we did a
                   * idinstall command.  If two ndcfg binaries are running
                   * then this can cause problems here as the other ndcfg
                   * can do the idinstall before we do, skewing netX for us
                   */
                  snprintf(ncfgelement,20, "net%d", (err=getlowestnetX()));
                  if (err == -1) {
                     error(NOTBCFG,"ResShowUnclaimed: error in getlowestnetX");
                     if (topologies != NULL) free(topologies);
                     if (driver_name != NULL) free(driver_name);
                     if (name != NULL) free(name);
                     free(bus);
                     (void) CloseResmgr();/*if we use iopenedit check first! */
                     return(-1);
                  }
               } else {
                  /* probably ODI or DLPI, "standard devices" - 0 based */
                  boardnumber=ResmgrGetLowestBoardNumber(driver_name);
                  if (boardnumber == -1) {
                     error(NOTBCFG,
                           "ResShowUnclaimed: ResmgrGetLowestBoardNumber -1");
                     if (topologies != NULL) free(topologies);
                     if (driver_name != NULL) free(driver_name);
                     if (name != NULL) free(name);
                     free(bus);
                     (void) CloseResmgr();/*if we use iopenedit check first! */
                     return(-1);
                  }
                  snprintf(ncfgelement,20, "%s_%d",driver_name,boardnumber);
               }
               if (fromShowTopo == 1) {
                  /* ShowTopo has already called StartList with the following
                   * arguments: StartList(14,"KEY",3,"BCFGINDEX",10,
                   *                      "DRIVER_NAME",10,"NAME",53,
                   *                      "NCFGELEMENT",12,"BUS",5,
                   *                      "TOPOLOGIES",10);
                   * so we change our last argument from topologies to
                   * bus from below
                   * only add to list if this bcfg supports this topology!
                   */
                  if (HasString(bcfgloop,N_TOPOLOGY,topoarg,0) == 1) {
                     AddToList(7,keyasstr,bcfgloopasstr,driver_name,
                                 LongName,ncfgelement,bus,topologies);
                     numfound++;
                  }
               } else {
                  char *tmptmp,*tmpstart,*atopo;
                  int topoloop;
                  char NewLongName[SIZE];
                  
                  /* late breaking requirement:  we want to call AddToList
                   * on each topology in the topologies array and print out
                   * the full topology name to accompany it.  when netcfg
                   * shows the list the user hasn't select which topology
                   * they want yet.   We also pass up a single valued
                   * topology string to accompany the list.
                   */
                  tmpstart=topologies;
                  while ((atopo=strtok_r(tmpstart," \t",&tmptmp)) != NULL) {
                     tmpstart=NULL;
                     for (topoloop=0; topoloop<numtopos; topoloop++) {
                        if (strcmp(topo[topoloop].name, atopo) == 0) {
                           snprintf(NewLongName,SIZE,"%s-%s",
                                    topo[topoloop].fullname,LongName);
                           AddToList(7,keyasstr,bcfgloopasstr,driver_name,
                                       NewLongName,ncfgelement,bus,atopo);
                           break;
                        }
                     }
                  }
                  numfound++;
               }
               if (topologies != NULL) free(topologies);
               if (driver_name != NULL) free(driver_name);
               if (name != NULL) free(name);
               found=1;
            }
            free(bus);
         }
      }
   }
   if (fromShowTopo == 0) {
      if (!found) {
         if (tclmode) {
            strcat(ListBuffer,"{ }");
         } else {
            error(NOTBCFG,"there are no unclaimed boards for loaded bcfgs");
         }
      }
      EndList();
   }
   (void)CloseResmgr();
   return(numfound);
}

#define  UNK_VAL     1
#define  STR_VAL     2
#define  NUM_VAL     3
#define  RNG_VAL     4

struct {
  char *parameter;
  u_int type;
  char val_buf[VB_SIZE];
} list[] = 
{
/* 0 */ {CM_MODNAME,STR_VAL,""},
/* 1 */ {CM_UNIT,NUM_VAL,""}, 
/* 2 */ {CM_IPL,NUM_VAL,""},
/* 3 */ {CM_ITYPE,NUM_VAL,""},
/* 4 */ {CM_IRQ,NUM_VAL,""},
/* 5 */ {CM_IOADDR,RNG_VAL,""},
/* 6 */ {CM_MEMADDR,RNG_VAL,""},
/* 7 */ {CM_DMAC,NUM_VAL,""},
/* 8 */ {CM_BINDCPU, NUM_VAL,""},
/* 9 */ {CM_BRDBUSTYPE, NUM_VAL,""},
/*10 */ {CM_BRDID, STR_VAL,""},
/*11 */ {CM_SCLASSID, STR_VAL,""},
/*12 */ {CM_SLOT, NUM_VAL,""},
/*13 */ {CM_ENTRYTYPE, NUM_VAL,""},
/*14 */ {CM_BUSNUM, NUM_VAL,""},
};

/* returns 0 for success else errno number */
int 
ResmgrDumpAll(void)
{
   rm_key_t rmkey;
   int NKstatus,status;

   rmkey = NULL;
   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResmgrDumpAll: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }
   StartList(32,"KEY",3,
             CM_MODNAME,10, CM_UNIT,3, CM_IPL,3, CM_ITYPE,3, 
             CM_IRQ,3, CM_IOADDR,10, CM_MEMADDR,20, CM_DMAC,3, 
             CM_BINDCPU,3, CM_BRDBUSTYPE,3, CM_BRDID,12, CM_SCLASSID,7,
             CM_SLOT,3, CM_ENTRYTYPE,3, CM_BUSNUM,3);

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      status=ResmgrDumpOne(rmkey,0);
      if (status != 0) {
         error(NOTBCFG,"ResmgrDumpAll: ResmgrDumpOne returned %d",status);
         CloseResmgr();
         return(status);
      }
   }
   EndList();
   (void)CloseResmgr();
   return(0);
}

/* assumes that MyRMnextkey will return an ascending key number
 * normally the case, since kernel rm_newkey() uses rm_next_key++
 * this routine is used by the idinstall command
 *
 * returns resmgr key or -1 if failure
 */
rm_key_t
ResmgrHighestKey(void)
{
   rm_key_t rmkey,status;
   int iopenedit=0, NKstatus;

   status=OpenResmgr(O_RDONLY);/* only called from idinstall cmd in cmdops.c */
   if (status) {
      error(NOTBCFG, "ResmgrHighestKey: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }

   rmkey = NULL;

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      /* do nothing */ ;
   }

   CloseResmgr();
   return(rmkey);
}

/* returns 0 for success or errno number */
int 
ResmgrDumpOne(rm_key_t key,u_int doStartEndList)
{
   u_int loop,status;
   int ec;
   int brdinst;
   char keyasnum[10];
   int numlistparams = sizeof(list) / sizeof(list[0]);

   if (key <= 0) {   /* RM_KEY not supported here...*/
      error(NOTBCFG,"ResmgrDumpOne: %d is not a valid key",key);
      return(-1);
   } 
   brdinst=0; 
   if (doStartEndList) {
      status=OpenResmgr(O_RDONLY);
      if (status) {
         error(NOTBCFG, "ResmgrDumpOne: OpenResmgr failed with %d(%s)",
               status,strerror(status));
         return(status);
      }
   }
   if (doStartEndList) StartList(32,"KEY",3,
             CM_MODNAME,10, CM_UNIT,3, CM_IPL,3, CM_ITYPE,3, 
             CM_IRQ,3, CM_IOADDR,10, CM_MEMADDR,20, CM_DMAC,3, 
             CM_BINDCPU,3, CM_BRDBUSTYPE,3, CM_BRDID,12, CM_SCLASSID, 7,
             CM_SLOT,3, CM_ENTRYTYPE,3, CM_BUSNUM,3);
   for (loop=0;loop<numlistparams;loop++) {
      RMbegin_trans(key, RM_READ);
      ec=RMgetvals(key,list[loop].parameter,brdinst,list[loop].val_buf,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(key);
         error(NOTBCFG,"ResmgrDumpOne: RMgetvals returned %d",ec);
         if (doStartEndList) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(key);
   }
   snprintf(keyasnum,10,"%d",key);
   AddToList(16,keyasnum,
             list[0].val_buf,
             list[1].val_buf,
             list[2].val_buf,
             list[3].val_buf,
             list[4].val_buf,
             list[5].val_buf,
             list[6].val_buf,
             list[7].val_buf,
             list[8].val_buf,
             list[9].val_buf,
             list[10].val_buf,
             list[11].val_buf,
             list[12].val_buf,
             list[13].val_buf,
             list[14].val_buf);
   if (doStartEndList) {
      EndList();
      (void) CloseResmgr();
   }
   return(0);
}

#define  UNKPARAM 22   /* from libresmgr */

/* returns 0 for success else unix errno OR libresmgr "errno" */
int
resget(char *key, char *param)
{
   rm_key_t rmkey;
   int GVstatus,status;
   char val_buf[VB_SIZE];
   char paramnocomma[SIZE];
   char *comma;
   int val_type=UNK_VAL;
   int valn;
   char valnasstr[10];

   rmkey=atoi(key);
   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "resget: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }
   strncpy(paramnocomma,param,SIZE);
   comma=strstr(paramnocomma,",");
   if (comma != NULL) {
      *comma='\0';
   }
   status=0;
   StartList(8,"KEY",5,"PARAM",18,"VALN",5,"VALUE",50);
   valn=0;
again:
   RMbegin_trans(rmkey, RM_READ);
   if ((GVstatus=RMgetvals(rmkey,param,valn,val_buf,VB_SIZE)) != 0) {
      RMend_trans(rmkey);  /* not RMabort_trans */
      status=GVstatus;
      if (GVstatus == UNKPARAM) {
         error(NOTBCFG,"resget: add ,[nsr] after %s",param);
      } else {
         error(NOTBCFG,"resget: RMgetvals failed with %d",GVstatus);
      }
      goto getalldone;
   }
   RMend_trans(rmkey);
   /* libresmgr sets 1st char in val_buf to '-' for STR_VAL/NUM_VAL/RNG_VAL 
    * when value is not found
    */
   if (strncmp(val_buf,"-",1)) {
      snprintf(valnasstr,10,"%d",valn);
      AddToList(4, key, paramnocomma, valnasstr, val_buf);
      valn++;
      goto again;
   }
getalldone:
   if (valn == 0) {
      error(NOTBCFG,"resget: parameter '%s' not set at key %d",
         paramnocomma,rmkey);
      if (status == 0) status=UNKPARAM;   /* well, it's close */
   }
   EndList();
   (void)CloseResmgr();  /* not entirely correct -- could be 1 too */
   return(status);
}

/* paramvalue can have spaces: "FOO,s=this is a test" is quite valid
 * returns 0 for success else error
 * does backup key work as well.
 */
int
resput(char *key, char *paramvalue, int dolist)
{
   rm_key_t rmkey;
   int PVstatus,status;
   char val_buf[VB_SIZE];
   char *equal;

   if (dolist) {  /* we're being called from user issuing cmd.  advance 1 */
      paramvalue++;  /* when we get it from lexer it starts with a space */
   }
   if ((equal=strstr(paramvalue,"=")) == NULL) {
      error(NOTBCFG,"resput: no '=' found in '%s'",paramvalue);
      return(-1);
   }
   if (*(equal+1) == NULL) {
      error(NOTBCFG,"resput: no value in '%s' found",paramvalue);
      return(-1);
   }
   *equal='\0';
   if (strcmp(key,"RM_KEY") == 0) {
      rmkey=RM_KEY;
   } else {
      rmkey=atoi(key);
      if (rmkey == 0) {
         error(NOTBCFG,"resput: invalid key %s",key);
         return(-1);
      }
   }
   status=OpenResmgr(O_RDWR);  /* since we're writing to database */
   if (status) {
      error(NOTBCFG, "resput: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }
   if (dolist) {
      StartList(2,"ADDSTATUS",4);
   }

   /* delete previous values if they exist on the resmgr */
   RMbegin_trans(rmkey, RM_RDWR);
   (void) MyRMdelvals(rmkey,paramvalue);  /* don't care about return value */
   /* we only support getting first value in list (n=0 below) */
   if ((PVstatus=RMputvals_d(rmkey,paramvalue,equal+1,delim)) != 0) {
      RMabort_trans(rmkey);
      if (PVstatus == UNKPARAM) {
         error(NOTBCFG,"resput: paramvalue '%s' needs ,[nsr] typing",
               paramvalue);
      } else {
         error(NOTBCFG,"resput: RMputvals key=%s param='%s' failed with %d(%s)",
               key, paramvalue,PVstatus,strerror(PVstatus));
      }
      goto resputfail;
      /* NOTREACHED */
   }
   RMend_trans(rmkey);

   /* if backups are enabled then write the parameter to backup key too
    * if we're not writing a parameter to key RM_KEY (don't back these up)
    */
   if ((rmkey != RM_KEY) && (g_backupkey != RM_KEY)) {
      if (rmkey == g_backupkey) {  /* you should have disabled backups first */
         error(NOTBCFG,"resput: trying to write to backupkey twice!");
         goto resputfail;
      }
      RMbegin_trans(g_backupkey, RM_RDWR);
      (void) MyRMdelvals(g_backupkey,paramvalue); /* don't care return value */
      /* we only support getting first value in list (n=0 below) */
      if ((PVstatus=RMputvals_d(g_backupkey,paramvalue,equal+1,delim)) != 0) {
         RMabort_trans(g_backupkey);
         if (PVstatus == UNKPARAM) {
            error(NOTBCFG,"resput: paramvalue '%s' needs ,[nsr] typing",
                  paramvalue);
         } else {
            error(NOTBCFG,"resput: RMputvals key=%s param='%s' failed "
                  "with %d(%s)",
                   g_backupkey, paramvalue,PVstatus,strerror(PVstatus));
         }
         goto resputfail;
         /* NOTREACHED */
      }
      RMend_trans(g_backupkey);
   }

   if (dolist) {
      AddToList(1,"success");
   }
putalldone:
   if (dolist) {
      EndList();
   }
   (void)CloseResmgr();  /* not entirely correct could be 1 too */
   return(0);
resputfail:
   (void)CloseResmgr();
   return(PVstatus);
}

/* resmgr should not be open prior to calling this routine
 * since we want O_WRONLY functionality
 * returns 0 for success else errno number
 * new key is stored in keyp
 * if you set doOpenClose is 0 then you must have previously opened it for
 * writing!
 */
int
ResmgrNextKey(rm_key_t *keyp, int doOpenClose)
{
   int NKstatus,status;

   if (doOpenClose == 1) {
      status=OpenResmgr(O_WRONLY); /* creating a new key-mark as writer only */
      if (status != 0) {
         error(NOTBCFG, "ResmgrNextKey: OpenResmgr failed with %d(%s)",
               status,strerror(status));
         return(status);
      }
   }

   /* commented out RMbegin_trans(*keyp, RM_RDWR); 
    * based on later knowledge that RMnewkey does implicit begin_trans
    */
   NKstatus=RMnewkey(keyp);
   if (NKstatus != 0) {
      /* RMabort_trans doesn't make sense here -- what key do you use?!
       * (since we didn't create a new one)
       */
      error(NOTBCFG,"ResmgrNextKey: RMnewkey returned %d",status);
      if (doOpenClose == 1) {
         CloseResmgr();
      }
      return(NKstatus);
   }

   RMend_trans(*keyp);

   if (doOpenClose == 1) {
      CloseResmgr();
   }
   return(0);
}

/* delete a key in the resmgr. 
 * returns 0 for success else errno number
 * does an implicit begin_trans/end_trans for us so we don't have to.
 */
int
ResmgrDelKey(rm_key_t key)
{
   int RKstatus,status;
   status=OpenResmgr(O_WRONLY);  /* must have write access to delete a key */
   if (status != 0) {
      error(NOTBCFG, "ResmgrDelKey: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }

   /* RMbegin_trans(key, RM_RDWR); not necessary */
   RKstatus=RMdelkey(key);
   if (RKstatus != 0) {
      /* RMabort_trans(key); not necessary */
      error(NOTBCFG,"ResmgrDelKey: RMdelkey returned %d",RKstatus);
      CloseResmgr();
      return(RKstatus);
   }

   /* RMend_trans(key); not necessary */
   CloseResmgr();
   return(0);
}

/* resmgr not open in this routine 
 * returns 0 for success else errno
 */
int
ResmgrGetVals(rm_key_t key, const char *param_list, int n,
     char *val_list, int val_size)
{
   int GVstatus,status;
  
   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResmgrGetVals: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }
   RMbegin_trans(key, RM_READ);
   GVstatus=RMgetvals(key, param_list, n, val_list, val_size);
   if (GVstatus) {
      RMabort_trans(key);
      error(NOTBCFG,"ResmgrGetVals: RMgetvals returned %d",GVstatus);
      CloseResmgr();
      return(GVstatus);
   }
   RMend_trans(key);
   CloseResmgr();
   return(0);
}

/* how many times does modname appear in the resmgr? 
 * if we return 0 it could mean that
 * a) modname is bogus
 * b) modname doesn't yet exist in the resmgr
 * this routine is called from assorted places:
 *
 *  - ResBcfgUnclaimed(u_int bcfgindex)                   in resops.c 
 *    (which does an Rmopen)
 *  - AtMAX_BDLimit(int bcfgindex)                        in cmdops.c
 * since some of these do OpenResmgr, we must use iopenedit to check if we
 * need to open the resmgr and close it in case of errors.
 * Note that if we set DRIVER_NAME incorrectly w.r.t the actual driver
 * name in the Master/System file then we'll get a bogus return value.
 * returns -1 for error else number of boards installed
 * In all cases this routine is and must be called with the intent of
 * looking for a NIC.  We used to call MyRMgetbrdkey in a loop to determine
 * the number but this is invalid when you have multiple NICS of the same
 * type and you've only configured one of them.  MODNAME will silently and
 * automatically get added to the remaining entries in the resmgr (since there
 * is a Drvmap file in the link kit) so you can longer call MyRMgetbrdkey
 * What we really want to know is "How many configured nics with the given
 * modname exist on the system" as opposed to "how many times does the
 * given modname appear in the resmgr?"
 * 
 * NOTE:  This routine does *NOT* produce the lowest available NIC number
 *        nor should it be used for that purpose(as was the case in the past)
 *        See ResmgrGetLowestBoardNumber() for that task.
 */
int
ResmgrGetNumBoards(char *modname)
{
   rm_key_t rmkey;
   int brdinst=0,status, ec;
   int iopenedit=0;
   int nthvalue, NKstatus;
   char elementstr[VB_SIZE];
   char element[VB_SIZE];
   char tmpmodname[VB_SIZE];

   if (modname == NULL) {
      error(NOTBCFG,"ResmgrGetNumBoards:  null modname!");
      return(-1);
   }

   if ((strcmp(modname,"-") == 0) || (strcmp(modname,"unused") == 0)) {
      error(NOTBCFG,"ResmgrGetNumBoards: invalid modname '%s'",modname);
      return(-1);
   }

   if (RMgetfd() == -1) {
      iopenedit=1;
      status=OpenResmgr(O_RDONLY);
      if (status) {
         error(NOTBCFG, "ResmgrGetNumBoards: OpenResmgr failed with %d(%s)",
               status,strerror(status));
         return(-1);
      }
   }

   rmkey = NULL;
   nthvalue=0;  /* we only check first value for any given parameter */
   snprintf(elementstr,VB_SIZE,"%s,s",CM_NETCFG_ELEMENT);

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      /* since most keys will not have CM_NETCFG_ELEMENT assigned to them 
       * we look for that first for performance reasons.  If CM_NETCFG_ELEMENT
       * is set then continue on.
       */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,elementstr,nthvalue,element,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "ResmgrGetNumBoards: RMgetvals for NETCFG_ELEMENT returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);
      /* NETCFG_ELEMENT is type STR_VAL so we'll get "-" from RMgetvals if
       * it's not set.
       */
      if (strcmp(element,"-") == 0) {
         /* element not set, not a configured network card, continue */
         continue;
      }

      /* ok, we know it's a nic, but does it have the modname we care about? */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_MODNAME,nthvalue,tmpmodname,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "ResmgrGetNumBoards: RMgetvals for MODNAME returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      if (strcmp(modname,tmpmodname) == 0) {
         /* we found a configured nic that matches the modname argument */
         brdinst++;
      }
   }

   /* don't complain if we didn't find any boards using modname in resmgr.
    * this is the case when called from resshowunclaimed when we print
    * the element that this device would have
    *
    * if ((status == ENOENT) && (brdinst == 0)) {
    *   error(NOTBCFG,"ResmgrGetNumBoards: modname '%s' not found in resmgr",
    *                 modname);
    *   CloseResmgr();
    *   return(-1);
    * }
    * RMend_trans(unknown_key);
    */

   if (iopenedit == 1) {
      CloseResmgr();
   }

   return(brdinst);
}

/* is the given NIC having modname and DEV_NAME matching /dev/mdi/foo#(MDI)
 * or /dev/foo_# (ODI/DLPI) taken in the resmgr?
 * Implementation:
 *    - look for NETCFG_ELEMENT first to ensure it's a NIC
 *    - read DRIVER_TYPE to learn if it's MDI, ODI, DLPI
 *    - build string we're looking for:
 *         ODI, DLPI:   snprintf(foo, SIZE, "/dev/%s_%d",modname,loop);
 *               MDI:   snprintf(foo, SIZE, "/dev/mdi/%s%d",modname,loop);
 *    - read DEV_NAME 
 *    - does DEV_NAME parameter match what we're looking for?
 *      If yes we can immediately return "no"(0) as this DEV_NAME already 
 *      exists in the resmgr
 * NOTES:
 * 1) we can't return "yes"(1) until we've read the entire resmgr!
 * 2) we look at either the real key or the backup key for DEV_NAME -- if it
 *    exists at either of the two then it is "taken"
 * This is an expensive routine to call but it's unfortunately necessary...
 * returns:
 *   -1 if error
 *    0 if not available
 *    1 if available
 */
int
IsDEV_NAMEAvailable(char *wantedmodname, int loop)
{
   rm_key_t rmkey;
   int status, ec;
   int iopenedit=0;
   int nthvalue, NKstatus;
   char tmp[VB_SIZE];
   char element[VB_SIZE];
   char modname[VB_SIZE];
   char drivertype[VB_SIZE];
   char devname[VB_SIZE];
   char wanteddevname[VB_SIZE];

   if (wantedmodname == NULL) {
      error(NOTBCFG,"IsDEV_NAMEAvailable:  null modname!");
      return(-1);
   }

   if ((strcmp(wantedmodname,"-") == 0) || 
       (strcmp(wantedmodname,"unused") == 0)) {
      error(NOTBCFG,"IsDEV_NAMEAvailable: invalid modname '%s'",wantedmodname);
      return(-1);
   }

   if (RMgetfd() == -1) {
      iopenedit=1;
      status=OpenResmgr(O_RDONLY);
      if (status) {
         error(NOTBCFG, "IsDEV_NAMEAvailable: OpenResmgr failed with %d(%s)",
               status,strerror(status));
         return(-1);
      }
   }

   rmkey = NULL;
   nthvalue=0;  /* we only check first value for any given parameter */

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      /* since most keys will not have CM_NETCFG_ELEMENT assigned to them
       * we look for that first for performance reasons.  If CM_NETCFG_ELEMENT
       * is set then continue on.
       */
      snprintf(tmp,VB_SIZE,"%s,s",CM_NETCFG_ELEMENT);
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,tmp,nthvalue,element,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "IsDEV_NAMEAvailable: RMgetvals for NETCFG_ELEMENT returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);
      /* NETCFG_ELEMENT is type STR_VAL so we'll get "-" from RMgetvals if
       * it's not set.
       */
      if (strcmp(element,"-") == 0) {
         /* element not set, not a configured network card, continue */
         continue;
      }

      /* ok, we know it's a nic, but does it have the modname we care about? */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_MODNAME,nthvalue,modname,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "IsDEV_NAMEAvailable: RMgetvals for MODNAME returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      if (strcmp(modname,wantedmodname) == 0) {
         /* we found a configured nic that matches the modname argument
          * now read DRIVER_TYPE and DEV_NAME
          */
         snprintf(tmp,VB_SIZE,"%s,s",CM_DRIVER_TYPE);
         RMbegin_trans(rmkey, RM_READ);
         ec=RMgetvals(rmkey,tmp,nthvalue,drivertype,VB_SIZE);
         if (ec != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,
               "IsDEV_NAMEAvailable: RMgetvals for DRIVER_TYPE returned %d",ec);
            if (iopenedit == 1) (void) CloseResmgr();
            return(-1);
         }
         RMend_trans(rmkey);


         snprintf(tmp,VB_SIZE,"%s,s",CM_DEV_NAME);
         RMbegin_trans(rmkey, RM_READ);
         ec=RMgetvals(rmkey,tmp,nthvalue,devname,VB_SIZE);
         if (ec != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,
               "IsDEV_NAMEAvailable: RMgetvals for DEV_NAME returned %d",ec);
            if (iopenedit == 1) (void) CloseResmgr();
            return(-1);
         }
         RMend_trans(rmkey);

         if (drivertype[0] == '-') {
            notice("IsDEV_NAMEAvailable: DRIVER_TYPE not set at key %d",rmkey);
            continue;
         }
         if (devname[0] == '-') {
            notice("IsDEV_NAMEAvailable: DEV_NAME not set at key %d",rmkey);
            continue;
         }

         if ((strcmp(drivertype,"ODI") == 0) ||
             (strcmp(drivertype,"DLPI") == 0)) {
            snprintf(wanteddevname,VB_SIZE,"/dev/%s_%d",wantedmodname, loop);
         } else 
         if (strcmp(drivertype,"MDI") == 0) {
            snprintf(wanteddevname,VB_SIZE,"/dev/mdi/%s%d",wantedmodname, loop);
         } else {
            error(NOTBCFG,"IsDev_NAMEAvailable: bad DRIVER_TYPE '%s' at key %d",
               drivertype, rmkey);
            if (iopenedit == 1) (void) CloseResmgr();
            return(-1);
         }

         if (strcmp(wanteddevname, devname) == 0) {
            /* a configured NIC with the desired DEV_NAME already exists
             * in the resmgr.  return 0=not available and no need to 
             * proceed further
             */
            if (iopenedit == 1) {
               CloseResmgr();
            }
            return(0); /* this {wantedmodname, loop} combination not avail. */
         }
      }  /* we found a configured nic */
   } /* for every key in the resmgr */

   /* went through all keys in the resmgr without finding a match, return 0
    */
   if (iopenedit == 1) {
      CloseResmgr();
   }

   return(1);    /* it's available */
}


/* what is the lowest available device number that we can use for the given
 * modname?  Typically used to build device names like /dev/foo7 or /dev/foo_7
 * This is not the same as ResmgrGetNumBoards!
 * Implementation:  Walk through the resmgr, looking for the first unused
 * number in DEV_NAME parameter from those configured NICs.
 * Note that DEV_NAME is /dev/mdi/foo# for MDI
 *              and      /dev/foo_#    for ODI and DLPI
 *
 * This is an expensive routine to call but it's unfortunately necessary...
 * returns -1 if error else number
 */
int
ResmgrGetLowestBoardNumber(char *modname)
{
   int loop,ret;

   for (loop=0; loop < MAXDRIVERSINLINKKIT; loop++) {
      ret=IsDEV_NAMEAvailable(modname, loop);
      if (ret == -1) {
         error(NOTBCFG,
                  "ResmgrGetLowestBoardNumber: error in IsDEV_NAMEAvailable");
         return(-1);
      }
      if (ret == 1) break;  /* it's available, use this loop */
   }
   if (loop == MAXDRIVERSINLINKKIT) {
      error(NOTBCFG,"ResmgrGetLowestBoardNumber: none available out of %d!",
         MAXDRIVERSINLINKKIT);
      return(-1);
   }
   return(loop);
}

/* this routine is used by showalltopologies to determine
 * if any BOARD_ID at the given bcfg index exists in the resmgr
 * as unclaimed. i.e. NETCFG_ELEMENT has not been set at that key yet.
 * Because of how autoconf works, MODNAME may or may not be set
 * at the key (in case of multiple smart bus nics of same type and one has
 * previously been configured by ndcfg the remaining unconfigured boards 
 * will silently and automatically get their MODNAME set)
 * returns -1 if error
 * returns  0 if not found in resmgr
 * returns  1 if found in resmgr and not yet assigned a driver
 * NOTE: It has much of the guts of resshowunclaimed() in it.
 */
int
ResBcfgUnclaimed(u_int bcfgindex)
{
   rm_key_t rmkey,junk;
   int status,NKstatus,GVstatus, tmpirq;
   int bcfgloop,found,nthvalue;
   int iopenedit=0,ec,zz, yy;
   char element[VB_SIZE];
   char elementstr[VB_SIZE];
   char brdid[VB_SIZE];
   char subbrdid[VB_SIZE];
   char brdbustypestr[VB_SIZE];
   u_int brdbustype;

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResBcfgUnclaimed: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }
   iopenedit=1;        /* we're only called from cmdops.c */

   rmkey = NULL;
   nthvalue=0;  /* we only check first value for any given parameter */

   snprintf(elementstr,VB_SIZE,"%s,s",CM_NETCFG_ELEMENT);

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      /* since most keys will not have CM_NETCFG_ELEMENT assigned to them 
       * we look for that first for performance reasons.  If CM_NETCFG_ELEMENT
       * is set then continue on.
       */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,elementstr,nthvalue,element,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "ResUnclaimed: RMgetvals for NETCFG_ELEMENT returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);
      /* NETCFG_ELEMENT is type STR_VAL so we'll get "-" from RMgetvals if
       * it's not set.  If element is set then continue; it's a NIC that
       * we've already configured earlier.
       */
      if (strcmp(element,"-") != 0) {
         continue;
      }

      /* now get CM_BRDBUSTYPE for this key.  we only support autodetectable
       * boards, and the autoconf mgr sets these as read-only
       * in the resmgr, so if it's not set, continue on...
       */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_BRDBUSTYPE,nthvalue,brdbustypestr,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "ResUnclaimed: RMgetvals for BRDBUSTYPE returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      /* if this isn't an autodetectable board type then continue on */
      brdbustype=atoi(brdbustypestr);/* will fail with 0 if not set("-") */
      if (brdbustype != CM_BUS_EISA &&
          brdbustype != CM_BUS_MCA  &&
#ifdef CM_BUS_I2O
          brdbustype != CM_BUS_I2O  &&
#endif
          brdbustype != CM_BUS_PCI) continue;  /* not autodetectable board */

      /* ok, we have a smart bus (PCI/EISA/MCA) key.  ensure some other
       * ISA board isn't using its IRQ.  We do this by checking ITYPE. 
       */
      if (irqatkey(rmkey, &tmpirq, 0) == -1) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ResBcfgUnclaimed: irqatkey failed");
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      if (tmpirq != -2) {
         if ((yy=irqsharable(rmkey, tmpirq, 0)) == -1) {
            RMabort_trans(rmkey);
            error(NOTBCFG,"ResBcfgUnclaimed: irqsharable failed");
            if (iopenedit == 1) (void) CloseResmgr();
            return(-1);
         }
         if (yy == 0) {
            notice("Skipping resmgr key %d because somebody else using its IRQ "
                   "and won't share it",rmkey);
            continue;
         }
      }

      /* get the board id */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_BRDID,nthvalue,brdid,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ResUnclaimed: RMgetvals for BRDID returned %d",
               ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);
      /* if brdid isn't set we don't abort check -- could have sbrdid only */
      /* For you PCI 2.1 fans, we'll use either board id or subsystem board
       * id when comparing against the bcfg file.  so if brdid isn't
       * set above don't panic
       */

      /* try for the sub board id */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_SBRDID,nthvalue,subbrdid,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ResUnclaimed: RMgetvals for SBRDID returned %d",
               ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);
      /* if subbrdid isn't set we don't abort check -- could have brdid only */

      /* if both brdid and subbrdid aren't set then we can worry
       * Since CM_MODNAME, CM_BRDID, and CM_SBRDID are all of type STR_VAL
       * libresmgr will set the buffer to the text "-" if it is not set
       * in the resmgr. see libresmgr RMgetvals code
       */
      if (!strcmp(brdid,"-") && !strcmp(subbrdid, "-")) {
         error(NOTBCFG,
       "ResUnclaimed: both BRDID and SBRDID aren't set for resmgr key %d!",
               rmkey);
         continue;
      }

      /* we convert board ids to lower case in bcfgops.c.  by converting
       * again here the call to HasString below will not have case
       * conversion problems
       */
      strtolower(brdid);
      strtolower(subbrdid);

      /* now for the meat:  does this match at this bcfg index? */
      if ((strlen(brdid) &&
          (HasString(bcfgindex,N_BOARD_IDS,brdid,0) == 1)) ||
          (strlen(subbrdid) &&
          (HasString(bcfgindex,N_BOARD_IDS,subbrdid,0) == 1))) {
         char *bus;
         char *driver_name;

         /* final check: match BUS= in bcfg with CM_BRDBUSTYPE */
         bus=StringListPrint(bcfgindex,N_BUS);
         if (bus == NULL) continue; /* BUS is mandatory, shouldn't happen */
         /* we treat CM_BRDBUSTYPE as bitmask here and in EnsureBus */
         if (bcfgbustype(bus) & brdbustype) { /*assumes BUS single valued*/
            driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);

            /* now see if bcfg file cannot support another instance of
             * this driver (i.e. MAX_BD reached).  If so then don't 
             * display board in list.
             */
            if ((zz=AtMAX_BDLimit(bcfgindex)) != 0) { /* error or at limit */ 
               if (zz == -1) {
                  /* error case */
                  notice("ResBcfgUnclaimed: AtMAX_BDLimit returned -1");
                  if (driver_name != NULL) free(driver_name);
                  free(bus);
                  continue;  /* error -- error() already called */
               }
               /* at MAX_BD limit */
               notice("%s at bcfg MAX_BD; skipping smart card", driver_name);
               if (driver_name != NULL) free(driver_name);
               free(bus);
               continue;   /* at limit so continue */
            }
            /* MAX_BD not defined or we're within limit.  print info.  */
            if (driver_name != NULL) free(driver_name);
            free(bus);
            /* ok, we found and verified this in resmgr.  return success */
            goto success;
            /* NOTREACHED */
         }
         free(bus);
      }
   }
   if (iopenedit == 1) {
      CloseResmgr();
   }
   return(0);
   /* NOTREACHED */

success:
   if (iopenedit == 1) {
      CloseResmgr();
   }
   return(1);
}

/* this routine counts up all of the ODIMEM=true entries in the resmgr
 * used when adding another ODI driver that has ODIMEM=true in its bcfg file
 * returns -1 if error else number found
 */
int
ResODIMEMCount(void)
{
   rm_key_t rmkey;
   int status,NKstatus,GVstatus,iopenedit,nthvalue,ec;
   char odimemstr[SIZE];
   char odimem[VB_SIZE];
   int numfound=0;

   status=OpenResmgr(O_RDONLY);   /* we're only called once from cmdops.c */
   if (status) {
      error(NOTBCFG, "ResODIMEMCount: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }
   iopenedit=1;

   rmkey = NULL;
   nthvalue=0;  /* we only check first value for any given parameter */

   snprintf(odimemstr,SIZE,"%s,s",CM_ODIMEM);

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,odimemstr,nthvalue,odimem,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "ResODIMEMCount: RMgetvals for ODIMEM returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);
      /* CM_ODIMEM,s is type STR_VAL so we'll get "-" from RMgetvals if
       * it's not set
       */
      if (strcmp(odimem,"-") == 0) continue;  /* if not set continue */
      /* if it's set to true we consider it valid - matches DoResmgrStuff */
      if (strcmp(odimem,"true") == 0) numfound++;
   }

   if (iopenedit == 1) {
      CloseResmgr();
   }
   return(numfound);
}

/* walk through resmgr looking for NETCFG_ELEMENT that matches element arg.
 * return the key.  Remember that DoRemainingStuff adds NETCFG_ELEMENT to
 * the resmgr.
 * Since NETCFG_ELEMENT is unique, we stop at the first one found
 * returns -1 for failure
 * otherwise returns the first key containing NETCFG_ELEMENT specified by
 * element variable.  Since element is unique, returning the first one
 * found isn't a problem.
 * 
 * This is a reasonably safe method that won't encounter any problems
 * unless the user uses the resmgr command directly to manipulate
 * entries, leading to duplicate NETCFG_ELEMENT entries in the resmgr
 * (or not finding the entry at all)!
 *
 * returns RM_KEY if not found in resmgr or error
 * else returns key where first NCFG_ELMENT found.
 * If bcfgpath isn't NULL, then we copy the BCFGPATH parameter from
 * the resmgr into bcfgpath.  Sufficient space must be provided.
 */
rm_key_t
resshowkey(int dolist, char *element, char *bcfgpath, int backup)
{
   rm_key_t rmkey;
   int status,NKstatus,GVstatus,iopenedit,nthvalue,ec;
   char ncfgelementstr[SIZE];
   char ncfgelement[VB_SIZE];
   char modname[VB_SIZE];
   char bcfgpathstr[SIZE];
   rm_key_t retvalue=RM_KEY;

   status=OpenResmgr(O_RDONLY); /* we're only called once from cmdparser.y */
   if (status) {
      error(NOTBCFG, "resshowkey: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(RM_KEY);
   }
   iopenedit=1;

   rmkey = NULL;
   nthvalue=0;  /* we only check first value for any given parameter */

   if (dolist == 1) {
      StartList(2,"KEY",5);
   }
   snprintf(ncfgelementstr,SIZE,"%s,s",CM_NETCFG_ELEMENT);
   snprintf(bcfgpathstr,SIZE,"%s,s",CM_BCFGPATH);

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,ncfgelementstr,nthvalue,ncfgelement,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "resshowkey: RMgetvals for NETCFG_ELEMENT returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(RM_KEY);
      }
      RMend_trans(rmkey);
      /* CM_NETCFG_ELEMENT,s is type STR_VAL so we'll get "-" from RMgetvals if
       * it's not set
       */
      if (strcmp(ncfgelement,"-") == 0) continue;  /* if not set continue */

      /* now get MODNAME */

      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey, CM_MODNAME, nthvalue, modname,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"resshowkey: RMgetvals for MODNAME failed");
         if (iopenedit == 1) (void) CloseResmgr();
         return(RM_KEY);
      }
      RMend_trans(rmkey);

      if (strcmp(modname,"-") == 0) continue; /* if not set continue */
      if (strcmp(modname,"unused") == 0) continue;

      /* if we're looking for the backup key and modname doesn't match
       * pattern net%d then continue.  Note that DLPI/ODI drivers don't 
       * have backup keys anymore
       */
      if (backup == 1) {
         /* must be more than 3 characters */
         if (strlen(modname) <= 3) continue;
         /* must start with "net" */
         if (strncmp(modname,"net",3) != 0) continue;
         /* must end with a number.  check first digit */
         if (!isdigit(modname[3])) continue;
         /* [4] can be a null (net0) or another digit (net11) but not a
          * a-zA-Z.  this is locale dependant so use isdigit.
          */
         if ((modname[4] != '\0') && (!isdigit(modname[4]))) {
            continue;
         }
         /* we have *a* backup key to get here.  must still check element 
          * to see if it's a backup key we care about
          */
      }

      /* but if we're not after the backup key and we come across netX
       * (the backup key) then we must also continue
       */
      if (backup == 0) {
         if ((strlen(modname) == 4) &&            /* "net0" through "net9" */
             (strncmp(modname,"net",3) == 0) &&   /* starts with "net" */
             (isdigit(modname[3]))) continue;     /* ends with a number */

         if ((strlen(modname) == 5) &&            /* "net10" through "net99" */
             (strncmp(modname,"net",3) == 0) &&   /* starts with "net" */
             (isdigit(modname[3])) &&             /* 4th char is a number */
             (isdigit(modname[4]))) continue;     /* 5th char is a number */
      }

      /* if element is set to what we want then we're done */
      if (strcmp(ncfgelement,element) == 0) {

         retvalue=rmkey;

         if (dolist == 1) {
            char keyasstr[20];
            snprintf(keyasstr,20,"%d",rmkey);
            AddToList(1,keyasstr);
            EndList();
         }
         if (bcfgpath != NULL) {
            /* also retrieve BCFGPATH from the resmgr and copy into
             * associated bcfgpath parameter
             */
             RMbegin_trans(rmkey, RM_READ);
             ec=RMgetvals(rmkey,bcfgpathstr,nthvalue,bcfgpath,VB_SIZE);
             RMend_trans(rmkey);
         }
         goto done;
      }
      /* to get here we found NETCFG_ELEMENT but it's not what we want */
   }
   /* not found in resmgr */
   if (dolist == 1) {
      if (tclmode) {
         strcat(ListBuffer,"{ }");
      } else {
         error(NOTBCFG,"NETCFG_ELEMENT %s not found in resmgr",element);
      }
      EndList();
   }

done:
   if (iopenedit == 1) {
      CloseResmgr();
   }
   return(retvalue);
}

/* note that if any of irq/dma/ioaddr/dmac isn't set in the resmgr
 * it is because the particular bcfg didn't set it.
 * the idinstall command enforces that if the bcfg sets it that you
 * must supply it as an argument to the idinstall command
 * returns 0 if success else error
 * if myirq, myioaddr, mymemaddr,mydmac aren't NULL then we save
 * the current settings for these parameters at the location specified.
 */
int
showISAcurrent(char *element, int dolist, 
               char *myirq, char *myioaddr, char *mymemaddr, char *mydmac)
{
   char keyasstr[20];
   char irq[VB_SIZE];
   char ioaddr[VB_SIZE];
   char memaddr[VB_SIZE];
   char dmac[VB_SIZE];
   char brdbustypestr[VB_SIZE];
   char bindcpu[VB_SIZE],itype[VB_SIZE],unit[VB_SIZE],ipl[VB_SIZE];
   int ec,status,iopenedit=0;
   u_int brdbustype;
   char *tmp;
   rm_key_t rmkey=resshowkey(0,element,NULL,0); /* OpenResmgr/CloseResmgr */

   if (rmkey == RM_KEY) {   /* failure in resshowkey() */
      notice("showISAcurrent: element %s not found in resmgr - trying backup",
            element);
      /* we can get away with this because this routine only wants to
       * read parameters from the resmgr, and all of these parameters will
       * exist at the backup key for ISA boards
       */
      rmkey=resshowkey(0,element,NULL,1);  /* does a OpenResmgr/CloseResmgr */
   }

   if (rmkey == RM_KEY) {
      error(NOTBCFG,"ShowISAcurrent: element %s not found in resmgr",element);
      return(-1);
   }

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "showISAcurrent: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }
   iopenedit=1;

   /* get CM_BRDBUSTYPE for this key.  if not ISA complain
    */
   RMbegin_trans(rmkey, RM_READ);
   ec=RMgetvals(rmkey,CM_BRDBUSTYPE,0,brdbustypestr,VB_SIZE);
   if (ec != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,
         "showISAcurrent: RMgetvals for BRDBUSTYPE returned %d",ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(ec);
   }
   RMend_trans(rmkey);
   /* complain if not ISA - caller should only call routine with ISA boards */
   brdbustype=atoi(brdbustypestr);/* will fail with 0 if not set("-") */
   if (brdbustype != CM_BUS_ISA) {
      /* rather than return blanks for all ISA characteristics we error() */
      error(NOTBCFG,"showISAcurrent: BRDBUSTYPE for resmgr key %d not ISA!");
      if (iopenedit == 1) (void) CloseResmgr();
      return(-1);  /* not an ISA board which is all we support */
   }
   /* retrieve IRQ DMAC IOADDR MEMADDR BINDCPU IPL ITYPE UNIT from the resmgr
    * we don't display OLDIRQ/OLDIOADDR/OLDMEMADDR/OLDDMAC though
    */
   if (dolist > 0) {
#ifdef SHOW_ADVANCED
      StartList(16,"IRQ",4,"DMAC",5,"IOADDR",15,"MEMADDR",20,"BINDCPU",8,
                    "IPL",4,"ITYPE",6,"UNIT",5);
#else
      StartList(8,"IRQ",4,"DMAC",5,"IOADDR",15,"MEMADDR",20);
#endif
   }

   if (myirq != NULL) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_IRQ,0,myirq,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for IRQ returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      strtolower(myirq);  /* internally to ndcfg we keep everything in lower */
   } else {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_IRQ,0,irq,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for IRQ returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      if (irq[0] == '-') strcpy(irq," ");
      strtoupper(irq);  /* we always display numbers to user in upper */
   }

   if (mydmac != NULL) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_DMAC,0,mydmac,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for DMAC returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      strtolower(mydmac); /* internally to ndcfg we keep everything in lower */
   } else {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_DMAC,0,dmac,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for DMAC returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      if (dmac[0] == '-') strcpy(dmac," ");
      strtoupper(dmac);  /* we always display numbers to user in upper */
   }

   if (myioaddr != NULL) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_IOADDR,0,myioaddr,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for IOADDR returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      strtolower(myioaddr);/* internally to ndcfg we keep everything in lower */

      /* because ioaddr is a range we see if there's a number there and if
       * so then replace the space with a dash
       */
      if (myioaddr[0] != '-') {
         /* there's something there so replace the space with a dash */
         if ((tmp=strstr(myioaddr," ")) != NULL) *tmp='-';
      }
   } else {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_IOADDR,0,ioaddr,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for IOADDR returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      strtoupper(ioaddr);
      /* because ioaddr is a range we see if there's a number there and if
       * so then replace the space with a dash
       */
      if (ioaddr[0] == '-') {
         strcpy(ioaddr," ");
      } else {
         /* there's something there so replace the space with a dash */
         if ((tmp=strstr(ioaddr," ")) != NULL) *tmp='-';
      }
   }

   if (mymemaddr != NULL) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_MEMADDR,0,mymemaddr,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for MEMADDR returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      strtolower(mymemaddr);

      /* because memaddr is a range we see if there's a number there and if
       * so then replace the space with a dash
       */
      if (mymemaddr[0] != '-') {
         /* there's something there so replace the space with a dash */
         if ((tmp=strstr(mymemaddr," ")) != NULL) *tmp='-';
      }
   } else {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_MEMADDR,0,memaddr,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"ShowISAcurrent: RMgetvals for MEMADDR returned %d",ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
      strtoupper(memaddr);
      /* because memaddr is a range we see if there's a number there and if
       * so then replace the space with a dash
       */
      if (memaddr[0] == '-') {
         strcpy(memaddr," ");
      } else {
         /* there's something there so replace the space with a dash */
         if ((tmp=strstr(memaddr," ")) != NULL) *tmp='-';
      }
   }


#ifdef SHOW_ADVANCED 
   RMbegin_trans(rmkey, RM_READ);

   if ((ec=RMgetvals(rmkey,CM_BINDCPU,0,bindcpu,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ShowISAcurrent: RMgetvals for BINDCPU returned %d",ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(ec);
   }
   strtoupper(bindcpu);
   if (bindcpu[0] == '-') strcpy(bindcpu," ");

   if ((ec=RMgetvals(rmkey,CM_IPL,0,ipl,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ShowISAcurrent: RMgetvals for IPL returned %d",ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(ec);
   }
   strtoupper(ipl);
   if (ipl[0] == '-') strcpy(ipl," ");

   if ((ec=RMgetvals(rmkey,CM_ITYPE,0,itype,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ShowISAcurrent: RMgetvals for ITYPE returned %d",ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(ec);
   }
   strtoupper(itype);
   if (itype[0] == '-') strcpy(itype," ");

   if ((ec=RMgetvals(rmkey,CM_UNIT,0,unit,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ShowISAcurrent: RMgetvals for UNIT returned %d",ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(ec);
   }
   strtoupper(unit);
   if (unit[0] == '-') strcpy(unit," ");

   RMend_trans(rmkey);

   if (dolist > 0) {
      AddToList(8,irq,dmac,ioaddr,memaddr,bindcpu,ipl,itype,unit);
   }
#else
   if (dolist > 0) {
      AddToList(4,irq,dmac,ioaddr,memaddr);
   }
#endif

   if (dolist > 0) {
      EndList();
   }
   if (iopenedit == 1) (void) CloseResmgr();
   return(0);
}

/* returns 0 for success else error */
int
showCUSTOMcurrent(char *element)
{
   rm_key_t rmkey=resshowkey(0,element,NULL,0); /* OpenResmgr/CloseResmgr */
   char niccustparm[VB_SIZE];
   char niccustparmstr[SIZE];
   char parmstr[VB_SIZE];
   char customname[10][VB_SIZE];
   char customvalue[10][VB_SIZE];
   char *tmp,*parm,*next;
   int ec,status,iopenedit=0;
   int loop,numcustom;

   if (rmkey == RM_KEY) {
      notice("showCUSTOMcurrent: element %s not found in resmgr - trying "
             "backup",element);
      rmkey=resshowkey(0,element,NULL,1);
   }
   /* we can try backup because this routine only reads parameters
    * that will always exist at the backup key 
    */
   if (rmkey == RM_KEY) {   /* failure in resshowkey() */
      error(NOTBCFG,"showCUSTOMcurrent: NETCFG_ELEMENT %s not found in resmgr",
            element);
      return(-1);
   }

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "showCUSTOMcurrent: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(status);
   }
   iopenedit=1;

   snprintf(niccustparmstr,SIZE,"%s,s",CM_NIC_CUST_PARM);
   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,niccustparmstr,0,niccustparm,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ShowCUSTOMcurrent:RMgetvals for NIC_CUST_PARM returned %d",
            ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(ec);
   }
   RMend_trans(rmkey);
   tmp=niccustparm;


   /* strings are returned as '-' if it doesn't exist in libresmgr */
   if (strcmp(niccustparm,"-") == 0) {
      /* no CUSTOM[] parameters to read */
      StartList(2,"NO_CUSTOM_PARAMS",15);  /* fudge up something */
      if (tclmode) {
         strcat(ListBuffer,"{ }");
      } else {
         notice("no CUSTOM[] parameters for %s, faking success", element);
      }
      goto alldone;
   }

   numcustom=0;
   while (((parm=strtok_r(tmp," ",&next)) != NULL) && (numcustom < 10)) {
      tmp=NULL;   /* for next strtok_r */
      numcustom++;  /* starting index into custom[] is 1 */
      strncpy(&customname[numcustom][0], parm, VB_SIZE);
   }
   StartList(numcustom * 2,&customname[1][0],20,
                          &customname[2][0],20,
                          &customname[3][0],20,
                          &customname[4][0],20,
                          &customname[5][0],20,
                          &customname[6][0],20,
                          &customname[7][0],20,
                          &customname[8][0],20,
                          &customname[9][0],20);

   /* RMgetvals calls strtok which destroys info so we must use reentrant 
    * version strtok_r which doesn't
    */
   for (loop=1; loop <= numcustom; loop++) {
      snprintf(parmstr,VB_SIZE,"%s%s,s",&customname[loop][0],CM_CUSTOM_CHOICE);
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,parmstr,0,&customvalue[loop][0],VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"showCUSTOMcurrent: RMgetvals for %s returned %d",
                    &customname[loop][0],ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(ec);
      }
      RMend_trans(rmkey);
   }
   AddToList(numcustom,
             &customvalue[1][0],
             &customvalue[2][0],
             &customvalue[3][0],
             &customvalue[4][0],
             &customvalue[5][0],
             &customvalue[6][0],
             &customvalue[7][0],
             &customvalue[8][0],
             &customvalue[9][0]);

alldone:
   /* we must call EndList here since the parameters to StartList are
    * on the stack. We can't wait for cmdparser.y to call EndList since
    * the variables used in the printout will be out of scope
    */
   EndList();
   if (iopenedit == 1) (void) CloseResmgr();
   return(0);
}

/* returns 0 for success else error 
 * works with either a real key or the backup
 * key.
 */
int
ResGetNameTypeDeviceDepend(rm_key_t rmkey, int backup, char *modname,
                     char *driver_type, char *devdevice, char *depend)
{
   char drivertypestr[SIZE];
   char devdevicestr[SIZE];
   char tmp[SIZE];
   int status,ec,iopenedit=0;

   if (rmkey < 0) {
      error(NOTBCFG,"ResGetNameType: need legit rmkey");
      return(997);
   }

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResGetNameType: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(996);
   }
   iopenedit=1;

   RMbegin_trans(rmkey, RM_READ);
   if (backup == 0) {
      ec=RMgetvals(rmkey, CM_MODNAME, 0, modname, VB_SIZE);
   } else {
      snprintf(tmp,SIZE,"%s,s",CM_BACKUPMODNAME);
      ec=RMgetvals(rmkey, tmp, 0, modname, VB_SIZE);
   }
   if (ec != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ResGetNameType:RMgetvals for %s returned %d",
            backup ? "BACKUPMODNAME" : "MODNAME" , ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(995);
   }
   
   snprintf(drivertypestr,SIZE,"%s,s",CM_DRIVER_TYPE);
   if ((ec=RMgetvals(rmkey,drivertypestr,0,driver_type,VB_SIZE)) != 0) {
      error(NOTBCFG,"ResGetNameType:RMgetvals for DRIVER_TYPE returned %d",
            ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(994);
   }

   snprintf(devdevicestr,SIZE,"%s,s",CM_NETINFO_DEVICE);
   if ((ec=RMgetvals(rmkey,devdevicestr,0,devdevice,VB_SIZE)) != 0) {
      error(NOTBCFG,"ResGetNameType:RMgetvals for DEV_DEVICE returned %d",
            ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(993);
   }

   snprintf(tmp,SIZE,"%s,s",CM_NIC_DEPEND);
   if ((ec=RMgetvals(rmkey,tmp,0,depend,VB_SIZE)) != 0) {
      error(NOTBCFG,"ResGetNameType:RMgetvals for NIC_DEPEND returned %d",
            ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(992);
   }


   RMend_trans(rmkey);

   if ((strcmp(modname,"-") == 0) || (strcmp(driver_type,"-") == 0) ||
       (strcmp(devdevice,"-") == 0)) {
      error(NOTBCFG,"ResGetNameType: MODNAME/DRIVER_TYPE/DEV_DEVICE isn't set");
      if (iopenedit == 1) (void) CloseResmgr();
      return(991);
   }

   if (iopenedit == 1) (void) CloseResmgr();
   return(0);
}

/* return the number of instances of a driver in the resmgr that
 * uses DRIVER_TYPE indicates as argument.  When this is 0 then we
 * can remove its aux drivers
 * returns -1 if error else number found
 * Note that non-configured nics (multiple smart bus boards) will have 
 * MODNAME set but won't have CM_DRIVER_TYPE set in the resmgr.
 */
int
resgetcount(char *driver_type_arg)
{
   int status,iopenedit=0;
   char drivertypestr[50];
   char drivertypeval[VB_SIZE];
   char modname[VB_SIZE];
   int numfound=0,NKstatus,ec;
   rm_key_t rmkey;

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "resgetcount: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }
   iopenedit=1;

   rmkey = NULL;
   snprintf(drivertypestr,50,"%s,s",CM_DRIVER_TYPE);

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_MODNAME,0,modname,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"resgetcount:RMgetvals for MODNAME returned %d",
               ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      /* if MODNAME isn't set then continue on -- not a valid resmgr entry */
      if (strcmp(modname,"-") == 0) continue;
      if (strcmp(modname,"unused") == 0) continue;

      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,drivertypestr,0,drivertypeval,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"resgetcount:RMgetvals for DRIVER_TYPE returned %d",
               ec);
         if (iopenedit == 1) (void) CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      strtoupper(drivertypeval);
      if (strcmp(drivertypeval,driver_type_arg) == 0) numfound++;
   }
   if (iopenedit == 1) (void) CloseResmgr();
   return(numfound);
}

/* retrieve CM_MDI_NETX parameter from resmgr for key rmkey 
 * returns -1 if error else 0=success
 * this routine must work on either a normal key or the backup key.
 * that is, it cannot go looking for MODNAME since it won't be set
 * for the backup key.
 */
int
ResmgrGetNetX(rm_key_t rmkey, char *netXstr)
{
   int status,ec,iopenedit=0;
   char mdinetx[VB_SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResmgrGetNetX: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }
   iopenedit=1;

   snprintf(mdinetx,VB_SIZE,"%s,s",CM_MDI_NETX);
   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,mdinetx,0,netXstr,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ResmgrGetNetX:RMgetvals for MDI_NETX returned %d",
            ec);
      if (iopenedit == 1) (void) CloseResmgr();
      return(-1);
   }
   RMend_trans(rmkey);
   if (strcmp(netXstr,"-") == 0) {
      error(NOTBCFG,"ResmgrGetNetX: MDI_NETX not set at key %d",rmkey);
      if (iopenedit == 1) (void) CloseResmgr();
      return(-1);
   }
   if (iopenedit == 1) (void) CloseResmgr();
   return(0);
}


/* remove network driver specific information associated with key rmkey
 * if ISA board then delete key
 * if PCI/EISA/MCA then delete parameters only so later resshowunclaimed
 * will be able to find it
 * returns 0 for success
 * returns non-zero for error
 * We know that resmgr key rmkey is specific to a NIC to get here.
 */
int
ResmgrDelInfo(rm_key_t rmkey, 
              int delmodname, int delbindcpu, int delunit, int delipl)
{
   int ec,status;
   char brdbustypestr[VB_SIZE];
   char niccustparmstr[50];
   char niccustparm[VB_SIZE];
   char customtmp[VB_SIZE];
   char *tmp,*parm,*next;
   u_int brdbustype;

   if (rmkey == RM_KEY) {
      notice("ResmgrDelInfo: rmkey is RM_KEY; returning");
      return(0);
   }

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResmgrDelInfo: OpenResmgr for reading failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }

   RMbegin_trans(rmkey, RM_READ);

   ec=RMgetvals(rmkey,CM_BRDBUSTYPE,0,brdbustypestr,VB_SIZE);
   if (ec != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,
         "ResmgrDelInfo: RMgetvals for BRDBUSTYPE returned %d",ec);
      (void) CloseResmgr();
      return(ec);
   }

   /* get custom parameters while resmgr is still opened read-only.  
    * not useful for ISA board since we'll just delete entire key
    * below but important for PCI/EISA/MCA boards
    * If not set then niccustparam will be "-"
    */
   snprintf(niccustparmstr,50,"%s,s",CM_NIC_CUST_PARM);
   if ((ec=RMgetvals(rmkey,niccustparmstr,0,niccustparm,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ResmgrDelInfo:RMgetvals for NIC_CUST_PARM returned %d",
            ec);
      (void) CloseResmgr();
      return(ec);
   }

   RMend_trans(rmkey);
   tmp=niccustparm;

   /* close resmgr for later OpenResmgr(O_WRONLY) here or in ResmgrDelKey */
   (void) CloseResmgr();

   /* if this is an ISA board delete entire key */
   brdbustype=atoi(brdbustypestr);/* will fail with 0 if not set("-") */
   if (brdbustype == CM_BUS_ISA) {
      /* This deletes MODNAME as well as everything else, calling the
       * driver's CFG_REMOVE (ddi8) routine
       * If ISA ever supports hotplug, then we should remove power to the
       * the device here.  But given that this is quite unlikely, we skip
       * hpsl stuff here.
       */
      status=ResmgrDelKey(rmkey);  /* does OpenResmgr(O_WRONLY)/CloseResmgr */
      if (status != 0) {
         error(NOTBCFG,"ResmgrDelInfo: ResmgrDelKey returned %d",status);
      }
      return(status);
   }

   if ((brdbustype != CM_BUS_EISA) && 
       (brdbustype != CM_BUS_MCA)  &&
#ifdef CM_BUS_I2O
       (brdbustype != CM_BUS_I2O)  &&
#endif
       (brdbustype != CM_BUS_PCI)) {
      notice("ResmgrDelInfo: unknown bus type %d at rmkey %d",
             brdbustype,rmkey);
      /* resmgr closed at this point so don't call CloseResmgr */
      return(0);  /* not autodetectable board -- return "success" */
   }

   /* ok, we know we're PCI/MCA/EISA to get here. re-open up resmgr O_WRONLY */
   status=OpenResmgr(O_WRONLY);
   if (status) {
      error(NOTBCFG, "ResmgrDelInfo: OpenResmgr for writing failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }

   /* we don't care if this succeeds or not as not all of these parameters
    * will exist at the given key.  Thankfully we don't need typing 
    * information after the parameter (i.e. ,[nrs]).
    * There's a race condition here until DDI8 if two ndcfg programs are
    * running at the same time and one starts deleting these parameters
    * and the other one is trying to read them.  We really need to "lock down"
    * this rmkey which DDI8 will give us with begin_trans/end_trans.
    */
#define D(A) {int x; \
              RMbegin_trans(rmkey, RM_RDWR); \
              x=MyRMdelvals(rmkey,A); \
              if (x != 0) { \
                 RMabort_trans(rmkey); \
                 error(NOTBCFG, \
                 "ResmgrDelInfo: error deleting parameter '%s' at key %d", \
                 A,rmkey); \
                 return(-1); \
              } \
              RMend_trans(rmkey); \
             }

   /* delete CM_MODNAME parameter first so that driver will get 
    * a CFG_REMOVE call up front
    * remove power if hpsl succeeds
    */
   if (delmodname == 1) {
      int hpslremoved=0;
      HpslSocketInfoPtr_t socketinfo;

      /* if (hpsl_IsInitDone() == HPSL_SUCCESS) { */
      if (hpslcount > 0) {
         notice("ResmgrDelInfo: removing MODNAME and turning off power "
                "via hpsl");
         BlockHpslSignals();
         socketinfo=GetHpslSocket(rmkey);
         if (socketinfo == NULL) {
            /* it's perfectly acceptable for this to fail if the board we're
             * trying to remove is EISA or ISA.  Right now hpsl only works for
             * removing PCI devices, so simply having a hotplug controller 
             * doesn't mean that hpsl will be able to cope with removing 
             * this device because hpsl may not support this device type!  
             * If this happens then revert to removing MODNAME by hand.  But 
             * we want to give hpsl the first shot at it.
             */
            notice("ResmgrDelInfo: couldn't find socket for rmkey %d",rmkey);
            UnBlockHpslSignals();
            goto removeoldway;
         }
         /* socket can't be empty to unbind driver */
         if (socketinfo->socketCurrentState & SOCKET_EMPTY) {
            notice("ResmgrDelInfo: hpsl says socket is empty");
            UnBlockHpslSignals();
            goto removeoldway;
         }
         /* power must be on to unbind_driver */
         if (!socketinfo->socketCurrentState & SOCKET_POWER_ON) {
            if (hpsl_set_state(socketinfo, SOCKET_POWER_ON, HPSL_SET) !=
                                                                HPSL_SUCCESS) {
               notice("ResmgrDelInfo: hpsl_set_state to turn power on "
                      "failed, hpsl_ERR=%d errno=%d", hpsl_ERR, errno);
               UnBlockHpslSignals();
               goto removeoldway;
            }
         }
         /* call driver's CFG_REMOVE */
         if (hpsl_unbind_driver(socketinfo, rmkey) != HPSL_SUCCESS) {
            /* if we're trying to unbind a ddi7 driver this will fail
             * with hpsl_ERR=0, errno=ENODEV.  This is because
             * hpsl_unbind_driver calls hpsl_get_drv_capability and the
             * HPCI_GET_DRV_CAP ioctl has failed with ENODEV in
             * function ddi_getdrv_cap() in util/mod/mod_drv.c
             * Not a big deal, we revert to removing MODNAME by hand
             */
            notice("ResmgrDelInfo: hpsl_unbind_driver failed, "
                   "hpsl_ERR=%d errno=%d",hpsl_ERR, errno);
            /* can't pull power here as MODNAME still set and hpsl_set_state 
             * checks this, setting hpsl_ERR to hpsl_eSOCKETBUSY if we try.
             * probably not a good idea anyway as driver will still need to
             * talk to device and pulling power wouldn't be very sociable.
             */
            UnBlockHpslSignals();
            goto removeoldway;
         }
         /* now that driver isn't using card and is unbound, pull the plug */
         if (hpsl_set_state(socketinfo, SOCKET_POWER_ON, HPSL_CLEAR) !=
                                                                HPSL_SUCCESS) {
            notice("ResmgrDelInfo: hpsl_set_state to turn power off "
                   "failed, hpsl_ERR=%d errno=%d", hpsl_ERR, errno);
            UnBlockHpslSignals();
            goto removeoldway;
         }
         UnBlockHpslSignals();
         hpslremoved++;
      }
removeoldway:
      if (hpslremoved == 0) {
         /* either not using hpsl library or we encountered an error when 
          * trying to use hpsl library.  Note that even if hpslcount is > 0
          * we may be trying to remove a non-PCI card (EISA) on a hotplug system
          * and since hpsl only supports PCI it's perfectly normal for it
          * to fail under these circumstances.
          * hpsl_unbind_driver does update socketInfo->devList[devIx].drvName 
          * and zeros it out, and we obviously don't do this. Hopefully the
          * next time our itimer fires HpslHandler->hpsl_check_async_events
          * will notice the change and update drvName for us so that a later
          * attempt to call hpsl_unbind_driver will suceed.
          */
         notice("ResmgrDelInfo: removing MODNAME by hand");
         D(CM_MODNAME);
      }
   }

   if (delbindcpu == 1) {
      D(CM_BINDCPU);
   }

   if (delunit == 1) {
      D(CM_UNIT);
   }

   if (delipl == 1) {
      D(CM_IPL);
   }

   /* XXX - don't delete ITYPE if EISA board type added it! */
   /* D(CM_ITYPE);  while found in System it is added by autoconf */
   
   /* D(CM_IRQ);  added by autoconf */
   /* D(CM_DMAC);  added by autoconf */
   /* D(CM_IOADDR);  added by autoconf */
   /* D(CM_MEMADDR);  added by autoconf */
   /* CM_BRDBUSTYPE/OLDIRQ/OLDDMAC/OLDIOADDR/OLDMEMADDR are ISA specific and
    * are handled above by deleting entire key
    * (I only add CM_BRDBUSTYPE is for ISA boards-autoconf adds automatically
    * for other types)
    */
   if (strcmp(niccustparm,"-") != 0) {
      /* there are custom parameters (from NIC_CUST_PARM).  Delete them. */
      /* NOTE: MyRMdelvals->RMdelvals calls strtok which destroys info so we 
       * must use reentrant version strtok_r which doesn't
       */
      while ((parm=strtok_r(tmp," ",&next)) != NULL) {
         tmp=NULL;   /* for next strtok_r */
         snprintf(customtmp,VB_SIZE,"%s%s",parm,CM_CUSTOM_CHOICE);
         D(parm);       /* delete custom parameter "FOO" */
         D(customtmp);  /* delete "FOO_" - used by showcustomcurrent */
         /* delete global custom parameters information too */
         snprintf(customtmp,VB_SIZE,"%s%s",CM_CUSTOM_CHOICE,parm);
         D(customtmp);  /* delete "_FOO" - stores if BOARD, DRIVER, GLOBAL */
      }
   }
   D(CM_NIC_CUST_PARM);
   D(CM_NIC_CARD_NAME);
   D(CM_BCFGPATH);
   D(CM_ENTRYTYPE);  /* not added by autoconf so we remove it when done */
   D(CM_ODIMEM);
   /* don't delete IICARD any more as nics postinstall script relies on it
    * being there for netinstall.   See iicard() elsewhere in this file
    * D("IICARD");    says if this key was used for netinstall
    */
   D(CM_TOPOLOGY);
   D(CM_BACKUPKEY);
   D(CM_ODI_TOPOLOGY);    /* ODI drivers only */
   D(CM_MDI_NETX);        /* MDI drivers only */
   D(CM_NETCFG_ELEMENT);    /* uniquely identifies driver */
   D(CM_DRIVER_TYPE);     /* ODI/DLPI/MDI */
   D(CM_NETINFO_DEVICE);
   D(CM_DEV_NAME);
   D(CM_NIC_DEPEND);
   D(CM_PROMISCUOUS);
   D(CM_NDCFG_UNIT);

   /* usr/src/i386/sysinst/desktop/menus/ii_do_netinst may have deposited
    * extra ODI parameters in the resmgr at this key.  Delete them too.
    * If the ODI .bcfg has them as custom parameters they will get re-added
    * again when driver is installed.  We explictly prevent MDI drivers from
    * using these parameters as custom ones in DoRemainingStuff() by deleting
    * them too.
    * see i386at/uts/io/odi/lsl/lslcm.c which reads from ODISTR1 to ODISTR16
    */
   D("ODISTR1");
   D("ODISTR2");
   D("ODISTR3");
   D("ODISTR4");
   D("ODISTR5");
   D("ODISTR6");
   D("ODISTR7");
   D("ODISTR8");
   D("ODISTR9");
   D("ODISTR10");
   D("ODISTR11");
   D("ODISTR12");
   D("ODISTR13");
   D("ODISTR14");
   D("ODISTR15");
   D("ODISTR16");

   (void) CloseResmgr();
   return(0);
}

struct printers {
   u_int bus;
   int (*printer)(rm_key_t, u_int, u_int);
} printers[] = 
{
 {CM_BUS_PCI,  ca_print_pci},
#ifdef CM_BUS_I2O
 {CM_BUS_I2O,  ca_print_i2o},
#endif
 {CM_BUS_EISA, ca_print_eisa},
 {CM_BUS_MCA,  ca_print_mca}
};

int numprinters = sizeof(printers) / sizeof(printers[0]);

int 
ResDumpCA(char *reskey, u_int longoutput, u_int bus)
{
   rm_key_t rmkey,highest;
   int status,loop,NKstatus,ec;
   char brdbustypestr[VB_SIZE];
   char cadevconfigstr[VB_SIZE];
   u_int brdbustype, devconfig;
   int (*printer)(rm_key_t, u_int, u_int);

   printer=NULL;
   /* convert resmgr key if present */
   if (reskey == NULL) {
      rmkey = NULL;
   } else {
      if ((rmkey=atoi(reskey)) <= 0) {
         error(NOTBCFG,"invalid resmgr key %s",reskey);
         return(-1);
      }
   }

   /* does machine have this type of bus ? */
   if ((cm_bustypes & bus) == 0) {
      error(NOTBCFG,"Sorry, this machine doesn't have that type of bus");
      return(-1);
   }

   /* determine our printer */
   for (loop=0;loop<numprinters;loop++) {
      if (printers[loop].bus & bus) {
         printer=printers[loop].printer;
         break;
      }
   }
   if (loop >= numprinters) {
      /* shouldn't happen */
      error(NOTBCFG,"couldn't find printer for bus %d",bus);
      return(-1);
   }

   highest=ResmgrHighestKey();   /* calls OpenResmgr/CloseResmgr */
   if (highest == -1) {
      error(NOTBCFG,"Error in ResmgrHighestKey");
      return(-1);
   }

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG, "ResDumpCA: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(-1);
   }

   if (reskey == NULL) {
      /* show all resmgr keys of this bus type */
      rmkey = NULL;

      while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {

         RMbegin_trans(rmkey, RM_READ);
         ec=RMgetvals(rmkey,CM_BRDBUSTYPE,0,brdbustypestr,VB_SIZE);
         if (ec != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,
               "ResDumpCA: RMgetvals for BRDBUSTYPE returned %d",ec);
            (void) CloseResmgr();
            return(ec);
         }
         RMend_trans(rmkey);

         /* if this isn't the bus type we're looking for then continue on */
         brdbustype=atoi(brdbustypestr);/* will fail with 0 if not set("-") */
         if ((brdbustype & bus) == 0) continue;

         /* get magic devconfig */
         RMbegin_trans(rmkey, RM_READ);
         ec=RMgetvals(rmkey,CM_CA_DEVCONFIG,0,cadevconfigstr,VB_SIZE);
         if (ec != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,
               "ResDumpCA: RMgetvals for DEVCONFIG returned %d",ec);
            (void) CloseResmgr();
            return(ec);
         }
         RMend_trans(rmkey);

         if (strcmp(cadevconfigstr,"-") == 0) {
            /* err, this is an autodetectable bus, the bus type we want,
             * but CA_DEVCONFIG isn't set?  "Major problem" so skip it
             */
            notice("ResDumpCA:DEVCONFIG not set at rmkey %d - skipping",rmkey);
            continue;
         }

         devconfig=atoi(cadevconfigstr);
         /* call our printer */
         status=(*printer)(rmkey, longoutput,devconfig);

         if (status != 0) {
            error(NOTBCFG,"ResDumpCA:  Problem in printer");
            (void) CloseResmgr();
            return(-1);
         }
      }   
   } else {  /* explicit resmgr key given */
      if (rmkey > highest) {
         error(NOTBCFG,"resmgr key must be from 0-%d",highest);
         (void) CloseResmgr();
         return(-1);
      }
         RMbegin_trans(rmkey, RM_READ);
         ec=RMgetvals(rmkey,CM_BRDBUSTYPE,0,brdbustypestr,VB_SIZE);
         if (ec != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,
               "ResDumpCA: RMgetvals for BRDBUSTYPE returned %d",ec);
            (void) CloseResmgr();
            return(ec);
         }
         RMend_trans(rmkey);

         /* if this isn't the bus type we're looking for then continue on */
         brdbustype=atoi(brdbustypestr);/* will fail with 0 if not set("-") */
         if ((brdbustype & bus) == 0) {
            error(NOTBCFG,"bus at resmgr key %d is a different bus",
                  rmkey);
            (void) CloseResmgr();
            return(-1);
         }
         /* get magic devconfig */
         RMbegin_trans(rmkey, RM_READ);
         ec=RMgetvals(rmkey,CM_CA_DEVCONFIG,0,cadevconfigstr,VB_SIZE);
         if (ec != 0) {
            RMabort_trans(rmkey);
            error(NOTBCFG,
               "ResDumpCA: RMgetvals for DEVCONFIG returned %d",ec);
            (void) CloseResmgr();
            return(ec);
         }
         RMend_trans(rmkey);

         if (strcmp(cadevconfigstr,"-") == 0) {
            /* err, this is an autodetectable bus, the bus type we want,
             * but CA_DEVCONFIG isn't set?  "Major problem" we can't skip it
             * since user only wants this key.
             */
            error(NOTBCFG,"ResDumpCA:  DEVCONFIG not set at rmkey %d",rmkey);
            (void) CloseResmgr();
            return(-1);
         }
         devconfig=atoi(cadevconfigstr);
         /* call our printer */
         status=(*printer)(rmkey, longoutput,devconfig);

         if (status != 0) {
            error(NOTBCFG,"ResDumpCA:  Problem in printer");
            (void) CloseResmgr();
            return(-1);
         }
   }
   (void) CloseResmgr();
   return(0);
}

/* is the given parameter available/taken in the resmgr? 
 * returns -1 if error
 * returns  0 if "false"
 *    (if PARAM_AVAIL, parameter is taken)
 *    (if PARAM_TAKEN, parameter is not taken)
 * returns  1 if "true"
 *    (if PARAM_AVAIL, parameter is available)
 *    (if PARAM_TAKEN, parameter is taken) 
 * note we must check all possible values for parameter in resmgr
 * this is important for memaddr and ioaddr which can have multiple
 * values in the in-core resmgr.
 * If value is a range, then it should be in the form X-Y.
 *
 * Note:  we do not call /etc/conf/bin/idcheck for each possible
 *        value as this would be verrry slow.  We only check the resmgr.
 *        ISA drivers installed with idinstall will automatically
 *        have an entry made in the resmgr from idresadd unless the
 *        driver goes out of its way to call idinstall -N, in which case
 *        you lose.  A slight improvement would be to only call idcheck
 *        _after_ walking through the resmgr first, but we don't do that.
 *
 * For performance reasons upon finding the first occurance of what we're
 * looking for in the resmgr we immediately return.
 */
int
IsParam(int cmd, int paramtype, char *value, int ResmgrAlreadyOpen,
        rm_key_t skipkey,
        char *driver_name)
{
   int status,NKstatus,valn,ec;
   rm_key_t rmkey;
   char *string;
   char *strend,*tmp;
   char contents[VB_SIZE];
   char modname[VB_SIZE];
   u_long desired_low;
   u_long desired_high,desired_diff;
   u_long resmgr_low;
   u_long resmgr_high,resmgr_diff;
   u_int retvalue=0;
   
   /* The RO_ISPARAMTAKENBYDRIVER macro is special and only supports N_PORT
    * for now...
    */
   if (driver_name != NULL && paramtype != N_PORT) {
      fatal("RO_ISPARAMTAKENBYDRIVER doesn't support paramtype %d",paramtype);
      /* NOTREACHED */
   }

   if (value == NULL) {
      error(NOTBCFG,"IsParam: null value");
      return(-1);
   }

   if (strlen(value) == 0) {
      error(NOTBCFG,"IsParam: strlen of value is 0");
      return(-1);
   }

   switch(paramtype) {
      case N_INT:
         string=CM_IRQ;
         /* no matter if we want to know if IRQ 2 is taken or available,
          * treat it as IRQ 9 in the resmgr (see also similiar code for
          * idinstall and idmodify commands)
          */
         if (atoi(value) == 2) value="9";
         break;
      case N_PORT:
         string=CM_IOADDR;
         if (strstr(value,"-") == NULL) {
            /* shouldn't happen since EnsureNumerics takes care of ranges */
            error(NOTBCFG,"IsParam: no '-' present in '%s'",value);
            return(-1);
         }
         break;
      case N_MEM:
         string=CM_MEMADDR;
         if (strstr(value,"-") == NULL) {
            /* shouldn't happen since EnsureNumerics takes care of ranges */
            error(NOTBCFG,"IsParam: no '-' present in '%s'",value);
            return(-1);
         }
         break;
      case N_DMA:
         string=CM_DMAC;
         break;
      default:
         error(NOTBCFG,"IsParam: Unknown paramtype %d",paramtype);
         return(-1);
         /* NOTREACHED */
         break;
   }

   if (cmd == PARAM_TAKEN) {
      retvalue=1;
   }

   strtolower(value);   /* all hex numbers are in lower case from libresmgr */
   

   if ((paramtype == N_MEM) || (paramtype == N_PORT)) {
    /* convert these values now, as while loop wants to use these many times */

      /* can't use atoi since port and mem are given in hex */
      if (((desired_low=strtoul(value,&strend,16))==0) &&
         (strend == value)) {
         error(NOTBCFG,"IsParam: bad argument numbers(1) in %s",value);
         return(-1);
      }
      value=strstr(value,"-")+1;

      if (((desired_high=strtoul(value,&strend,16))==0) &&
         (strend == value)) {
         error(NOTBCFG,"IsParam: bad argument numbers(2) in %s",value);
         return(-1);
      }

      if ((desired_low == 0) || 
          (desired_high == 0) ||
          (desired_low > desired_high)) {  /* acceptable to have low=high */
         error(NOTBCFG,"IsParam: bad argument numbers(3) in %s",value);
         return(-1);
      }
   }

   if (ResmgrAlreadyOpen == 0) {
      status=OpenResmgr(O_RDONLY);
      if (status) {
         error(NOTBCFG, "IsParam: OpenResmgr failed with %d(%s)",
               status,strerror(status));
         return(-1);
      }
   }

   rmkey = NULL;

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {

      if ((skipkey != RM_KEY) && (rmkey == skipkey)) continue;

      valn=0;
again:

      /* if this key has a MODNAME of netX, then we hit our own
       * backup key.  skip it and continue on.
       * if MODNAME is "-" or "unused" then pay attention to it!
       */
      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,CM_MODNAME,valn,modname,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"IsParam: RMgetvals for MODNAME returned %d",ec);
         retvalue=-1;
         goto isparamdone;
      }
      RMend_trans(rmkey);

      /* OLD WAY: if (strcmp(modname,CM_NDCFG) == 0) continue; */

      if ((strlen(modname) == 4) &&            /* "net0" through "net9" */
          (strncmp(modname,"net",3) == 0) &&   /* starts with "net" */
          (isdigit(modname[3]))) continue;     /* ends with a number */

      if ((strlen(modname) == 5) &&            /* "net10" through "net99" */
          (strncmp(modname,"net",3) == 0) &&   /* starts with "net" */
          (isdigit(modname[3])) &&             /* 4th char is a number */
          (isdigit(modname[4]))) continue;     /* 5th char is a number */

      RMbegin_trans(rmkey, RM_READ);
      ec=RMgetvals(rmkey,string,valn,contents,VB_SIZE);
      if (ec != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,
            "IsParam: RMgetvals for %s returned %d",string,ec);
         retvalue=-1;
         goto isparamdone;
      }
      RMend_trans(rmkey);
      /* since ranges return "- -" if not found we key off of first char
       * to determine if not found which is always a '-' for both
       * since number not found or range not found
       */
      if (contents[0] != '-') { /* this {parameter,valueN} set is defined */
         if ((paramtype == N_INT) || (paramtype == N_DMA)) {
            /* direct number, no range problems, can do a strcmp */

            /* if we want to know if this number is taken, we can 
             * immediately return if it is (denoted by a string match)
             */
            if (strcmp(contents,value) == 0) {
               goto isparamdone;
            }
         } else {
            /* paramtype == N_PORT or paramtype == N_MEM, so
             * we must determine if any portion of range in value is
             * contained in the range in contents.  If so, and
             * the user wants to know if PARAM_TAKEN, then close
             * resmgr and immediately return
             */
            /* users can create bogus ranges by using resmgr cmd 
             * directly.  libresmgr will always pass up something,
             * but it might not make sense.  Don't error out if it doesn't
             * make sense
             */

            /* contents contains a hex number; can't use atoi */
            if (((resmgr_low=strtoul(contents,&strend,16))==0) &&
               (strend == contents)) {
               error(NOTBCFG,"IsParam: bad low range resmgr number at key %d "
                             "for %s in '%s'", rmkey, string, contents);
               retvalue=-1;
               goto isparamdone;
            }
            tmp=strstr(contents," ")+1;
            if (((resmgr_high=strtoul(tmp,&strend,16))==0) &&
               (strend == tmp)) {
               error(NOTBCFG,"IsParam: bad high range resmgr number at key %d "
                             "for %s in '%s'", rmkey, string, contents);
               retvalue=-1;
               goto isparamdone;
            }

            /* Pstderr("desired_low=0x%x desired_high=0x%x "
             *         "resmgr_low=0x%x resmgr_high=0x%x\n",
             *         desired_low,desired_high,resmgr_low,resmgr_high);
             */

            if ((resmgr_high == 0) ||   /* it's acceptable to have low of 0 */
                (resmgr_low > resmgr_high)) { /* acceptable to have low=high */
               notice("IsParam: bad resmgr numbers for %s at key %d in '%s'",
                      string, rmkey, contents);
               valn++;
               goto again;   /* try next value */
            }

            desired_diff=desired_high - desired_low;
            resmgr_diff=resmgr_high - resmgr_low;

            /* if any of the ranges touch then they're taken */
            if ((desired_low == resmgr_low) ||
                (desired_low == resmgr_high) ||
                (desired_high == resmgr_low) ||
                (desired_high == resmgr_high)) {
               if ((driver_name != NULL) && 
                   (strcmp(modname,driver_name) == 0)) return(1);
               if (driver_name == NULL) goto isparamdone;
            }

            /* the xxx represents "don't care" in the diagrams below 
             * the dashes signify the range 
             */


            /*     xxx----------------------------resmgr 
             *                    xxx-------------------------desired
             */
            if ((desired_low < resmgr_high) && 
                (desired_low + desired_diff > resmgr_high)) {
               if ((driver_name != NULL) && 
                   (strcmp(modname,driver_name) == 0)) return(1);
               if (driver_name == NULL) goto isparamdone;
            }


            /*     xxx----------------------------desired
             *                    xxx-------------------------resmgr
             */
            if ((resmgr_low < desired_high) &&
                (resmgr_low + resmgr_diff > desired_high)) {
               if ((driver_name != NULL) && 
                   (strcmp(modname,driver_name) == 0)) return(1);
               if (driver_name == NULL) goto isparamdone;
            }
     


            /*        -----------------------------resmgr
             *                  -----desired
             */
            if ((desired_low > resmgr_low) &&
                (desired_high < resmgr_high) &&
                (desired_low < resmgr_high) &&
                (desired_high > resmgr_low)) {
               if ((driver_name != NULL) && 
                   (strcmp(modname,driver_name) == 0)) return(1);
               if (driver_name == NULL) goto isparamdone;
            }



            /*        ----------------------------desired
             *                 ------resmgr
             */
            if ((resmgr_low > desired_low) &&
                (resmgr_high < desired_high) &&
                (resmgr_low < desired_high) &&
                (resmgr_high > desired_low)) {
               if ((driver_name != NULL) && 
                   (strcmp(modname,driver_name) == 0)) return(1);
               if (driver_name == NULL) goto isparamdone;
            }



            /*        -----------------------------xxxresmgr
             *  -------------------xxxdesired
             */
            if ((desired_low < resmgr_low) && 
                (desired_low + desired_diff > resmgr_low)) {
               if ((driver_name != NULL) && 
                   (strcmp(modname,driver_name) == 0)) return(1);
               if (driver_name == NULL) goto isparamdone;
            }


            /*         ----------------------------xxxdesired
             *   -------------------xxxresmgr
             */
            if ((resmgr_low < desired_low) &&
                (resmgr_low + resmgr_diff > desired_low)) {
               if ((driver_name != NULL) && 
                   (strcmp(modname,driver_name) == 0)) return(1);
               if (driver_name == NULL) goto isparamdone;
            }
         }  /* parameter is port or mem */
         valn++;
         goto again;
      } /* if contents are not empty */
      /* to get here contents of parameter is '-' so we know we reached the end
       * of our values.  Indeed, parameter may not have been defined!
       * try next key in resmgr 
       */
   }
   /* reached the end of all keys (and hence all parameters)
    * determine our return type
    */
   if (ResmgrAlreadyOpen == 0) {
      (void) CloseResmgr();
   }
   if (cmd == PARAM_TAKEN) {
      /* in the case of the RO_ISPARAMTAKENBYDRIVER macro, if we make it
       * here, we know we went through the entire resmgr, so no need to
       * check driver_name against anything (indeed, what would we compare it
       * with?!)
       */
      return(0);   /* nope, not taken */
   } else {
      return(1);   /* PARM_AVAIL -- yes, parameter is available */
   }

isparamdone:
   if (ResmgrAlreadyOpen == 0) {
      (void) CloseResmgr();
   }
   /* we'll never get here in case of RO_ISPARAMTAKENBYDRIVER macro as
    * we immediately return "yes" as soon as we found a conflict
    */
   return(retvalue);
}

/* copy all parameters stored at backupkey to realrmkey
 * resmgr is already open O_RDWR
 * so this is just a begin_trans/end_trans job
 * with some RMnextparam glue
 * trick: we know that all parameters we store at the backup key are
 * "ours" (not known by libresmgr except for BRDID) and so are stored
 * as strings.  Thankfully BRDID is also a string.
 * we only reset delim when success.  not a problem if failure since
 * we won't call resput after this routine returns error.
 * since resput calls MyRMdelvals we don't have to worry about copying
 * multi-value parameters.
 */
int
CopyAllParams(rm_key_t realrmkey, rm_key_t backupkey, 
              char *backupkeystr, char *realkeystr)
{
   char name[RM_MAXPARAMLEN];
   int status,GVstatus,PVstatus;
   char val_bufstr[VB_SIZE];
   char asstring[SIZE];
   char asstringstr[VB_SIZE];
   char realmodname[SIZE];
   char tmp[SIZE];
   char backupkeyasstr[10];
   char realkeyasstr[10];
   int n=0;
   char olddelim;

   snprintf(backupkeyasstr,10,"%s",backupkeystr);
   snprintf(realkeyasstr,10,"%s",realkeystr);

   olddelim=delim;
   delim='\1';   /* ASCII 1 = SOH, an unlikely character */

   while ((status=RMnextparam(backupkey, n, name, RM_MAXPARAMLEN)) == 0) {

      /* Likewise, don't store REALKEY at the primary key; it's only
       * stored at the backup key.  BRDID is readonly for autoconf boards
       * so no need to copy it.  Also don't copy ISA params that will be
       * set at the other key in case we're copying an ISA key. Copy UNIT.
       * the only parameters we back up for smart-bus boards are non-hardware
       * specific so things like SLOT DEVNUM BUSNUM etc won't appear at the  
       * backup key so we won't find them here.
       * we copy MODNAME below so don't do it here either.
       */
      if ((strcmp(name,CM_REALKEY) == 0) ||
          (strcmp(name,CM_BRDID) == 0) ||
          (strcmp(name,CM_ITYPE) == 0) ||
          (strcmp(name,CM_IRQ) == 0) ||
          (strcmp(name,CM_MEMADDR) == 0) ||
          (strcmp(name,CM_IOADDR) == 0) ||
          (strcmp(name,CM_DMAC) == 0) ||
          (strcmp(name,CM_BRDBUSTYPE) == 0) ||
          (strcmp(name,CM_MODNAME) == 0)) {
         n++;
         continue;
      }

#if 0
the mere presence of CM_VOLATILE now indicates to idconfupdate 
that this key should not be copied to /stand/resmgr
#ifdef CM_VOLATILE
      /* volatile params shouldn't be copied to the real key; they are only
       * found at the backup key 
       */
      if (strcmp(name,CM_VOLATILE) == 0) {
         n++;
         continue;
      }
#endif
#endif
      /* we'll copy .INSTNUM and PARENT though */

      /* for the few numerics we copy use ,n else sprintf in libresmgr will
       * give us bogus values. none of our other params are ranges (whew)
       * while VOLATILE is numeric, we don't copy it.   Generally,
       * these are numeric parameters that are local to ndcfg or from DSP 
       * System file and are not added by autoconf automatically.
       * note that it's OK for BINDCPU to not exist at the backup key; it
       * is only added at idinstall time if its value is != -1
       */
      if ((strcmp(name,CM_INSTNUM) == 0) ||
          (strcmp(name,CM_PARENT) == 0) ||
          (strcmp(name,CM_NDCFG_UNIT) == 0) ||
          (strcmp(name,CM_ENTRYTYPE) == 0) ||
          (strcmp(name,CM_BINDCPU) == 0) ||
          (strcmp(name,CM_IPL) == 0) ||
          (strcmp(name,CM_UNIT) == 0)) {
         snprintf(asstring,SIZE,"%s,n",name); /* ignore name 'asstring' here */
      } else {
         snprintf(asstring,SIZE,"%s,s",name);
      }
      RMbegin_trans(backupkey, RM_READ);
      if ((GVstatus=RMgetvals(backupkey,asstring,0,val_bufstr,VB_SIZE))!=0){
         RMabort_trans(backupkey);
         error(NOTBCFG,"CopyAllParams: RMgetvals failed with %d",GVstatus);
         return(-1);
      }
      RMend_trans(backupkey);

      if (strcmp(name,CM_BACKUPMODNAME) == 0) {
         strncpy(realmodname,val_bufstr,SIZE);
         n++;
         continue;  /* don't copy BACKUPMODNAME to primary key; it is
                     * only stored at backup key!
                     */
      }

      RMbegin_trans(realrmkey, RM_RDWR);
      (void) MyRMdelvals(realrmkey,asstring);/* don't care about return value */
      RMend_trans(realrmkey);

      RMbegin_trans(realrmkey, RM_RDWR);
      if ((PVstatus=RMputvals_d(realrmkey,asstring,val_bufstr,delim)) != 0) {
         RMabort_trans(realrmkey);
         error(NOTBCFG,"CopyAllParams: RMputvals key=%s param='%s' failed "
               "with %d(%s)",realrmkey,asstring,PVstatus,strerror(PVstatus));
         return(-1);
      }
      RMend_trans(realrmkey);

      n++;
   }
   delim=olddelim;

   /* now add BACKUPKEY, and MODNAME at realrmkey to make it official. */
   snprintf(tmp,SIZE,"%s,s",CM_BACKUPKEY);
   RMbegin_trans(realrmkey, RM_RDWR);
   MyRMdelvals(realrmkey,tmp);
   RMend_trans(realrmkey);

   RMbegin_trans(realrmkey, RM_RDWR);
   RMputvals_d(realrmkey,tmp,backupkeyasstr,delim);
   RMend_trans(realrmkey);

   RMbegin_trans(realrmkey, RM_RDWR);
   MyRMdelvals(realrmkey,CM_MODNAME);
   RMend_trans(realrmkey);

   RMbegin_trans(realrmkey, RM_RDWR);
   RMputvals_d(realrmkey,CM_MODNAME,realmodname,delim);
   RMend_trans(realrmkey);

   /* change REALKEY at backupkey to point back to realrmkey */
   snprintf(tmp,SIZE,"%s,s",CM_REALKEY);
   RMbegin_trans(backupkey, RM_RDWR);
   MyRMdelvals(backupkey,tmp);
   RMend_trans(backupkey);

   RMbegin_trans(backupkey, RM_RDWR);
   RMputvals_d(backupkey,tmp,realkeyasstr,delim);
   RMend_trans(backupkey);

   return(0);
}


/* Read custom parameter information from the resmgr in the quest to find
 * any "PATCH" parameters.  
 * That is, they change a value in /dev/kmem.  Called after driver load
 * routines have been run so that driver symbol table is visible for
 * patching in /dev/kmem.  We could patch the Driver.o but that's for
 * another day.
 * called from DoResmgrAndDevs, which is called from rc2.d/S15nd, so
 * don't call notice or error unless important...
 */
int
DoPatchCustom(rm_key_t rmkey)
{
   int ec,numcustom,loop;
   unsigned long newvalue;
   char *tmp, *parm, *next, *strend;
   char parmstr[VB_SIZE];
   char niccustparmstr[SIZE];
   char niccustparm[VB_SIZE];
   char customname[10][VB_SIZE];    /* MEDIASPEED */
   char customvalue[10][VB_SIZE];   /* 10 */
   char customtype[10][VB_SIZE];    /* BOARD, DRIVER, GLOBAL, PATCH */

   /* resmgr already open */
   
   snprintf(niccustparmstr,SIZE,"%s,s",CM_NIC_CUST_PARM);
   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,niccustparmstr,0,niccustparm,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for NIC_CUST_PARM "
            "returned %d", ec);
      goto patcherror;
   }
   RMend_trans(rmkey);
   tmp=niccustparm;

   if (strcmp(niccustparm,"-") == 0) return(0);  /* no custom params */

   numcustom=0;
   while (((parm=strtok_r(tmp," ",&next)) != NULL) && (numcustom < 10)) {
      tmp=NULL;   /* for next strtok_r */
      numcustom++;  /* starting index into custom[] is 1 */
      strncpy(&customname[numcustom][0], parm, VB_SIZE);
   }

   /* RMgetvals calls strtok which destroys info so we must use reentrant 
    * version strtok_r which doesn't
    */
   for (loop=1; loop <= numcustom; loop++) {

      /* retrieve value for this parameter */
      snprintf(parmstr,VB_SIZE,"%s%s,s",&customname[loop][0],CM_CUSTOM_CHOICE);
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,parmstr,0,&customvalue[loop][0],VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"showCUSTOMcurrent: RMgetvals for %s returned %d",
                    parmstr,ec);
         goto patcherror;
      }
      RMend_trans(rmkey);

      /* retrieve the type for this parameter (BOARD, DRIVER, PATCH, GLOBAL) */
      snprintf(parmstr,VB_SIZE,"%s%s,s",CM_CUSTOM_CHOICE,&customname[loop][0]);
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,parmstr,0,&customtype[loop][0],VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"showCUSTOMcurrent: RMgetvals for %s returned %d",
                    parmstr,ec);
         goto patcherror;
      }
      RMend_trans(rmkey);

      strtoupper(&customtype[loop][0]);
      if (strcmp(&customtype[loop][0],"PATCH") == 0) {
         /* patch values are base 10, not hex or octal */
         if (((newvalue=strtoul(&customvalue[loop][0],&strend,10))==0) &&
              (strend == &customvalue[loop][0])) {
            notice("DoPatchCustom: invalid patch value %s",
                   &customvalue[loop][0]);
            /* continue on to next parameter to patch*/
         } else {
            /* notice("DoPatchCustom: patching %s to new value %d",
             *        &customname[loop][0],newvalue);
             */
            donlist(1, &customname[loop][0], newvalue, 0);
         }
      }
   }

   return(0);   /* success */

patcherror:
   return(ec);

}


/* called by nd script at startup and shutdown time.  Ensures that
 * - all devices can be loaded and/or opened.  Note we only want
 *   to do this when system is coming up, before dlpid has started.
 *   so we look at action argument, which can be one of:
 *   "start" - only valid if dlpid not running.  has optional interfaces
 *             to start, separated by a spaces.  If optional interface
 *             words are present then we are not running from /etc/rc2 script.
 *             still need to ensure that dlpid isn't running before trying
 *             to load/open the MDI device.
 *   "stop"  - can't open raw MDI drivers but can ensure that 
 *   "restart"
 *   "add_backup"
 *   "remove_backup"
 *   
 *   the action variable could look like "start foo bar baz"
 *
 * - all "netX" entries in resmgr actually match reality.  That is,
 *   if the user pulls a card from the system, then the files
 *   /etc/confnet.d/netdrivers and DLPIMDIDIR/dlpimdi will be out of sync.
 *   we whine if this happens and reassign the board parameters back
 *   if an unclaimed (MODNAME is empty) board appears in the resmgr.
 *   This can happen when user pulls card and reboots machine
 *   or by DDI8 PCI hot-plug devices being removed when we shut the machine
 *   down.  The moral is that you need to go through netcfg to remove
 *   the board.
 * return value indicates how ndcfg should exit(2).
 * 
 * Since Token Ring cards must attach to the ring at open(2) time we don't
 * want to call their open routine since this can block for ~30 seconds,
 * slowing the boot process down (since it will have to reattach
 * again when the first stack tries to open the card).
 *
 */

/* the maxinum number of NICS that we will ever find out of sync
 * in the database
 */
#define MAXFIXUP 50

int
CheckResmgrAndDevs(char *action)
{
   int status,NKstatus,GVstatus,ec,loop;
   rm_key_t backuprmkey, realrmkey;
   char *space,*arg;
   char path[SIZE];
   char niccardname[SIZE];
   char modname[VB_SIZE];
   char ndcfg_unit[VB_SIZE];
   char backupbrdid[VB_SIZE];
   char backupmodname[VB_SIZE];
   char backupelement[VB_SIZE];
   char realelement[VB_SIZE];
   char realbrdid[VB_SIZE];
   char realmodname[VB_SIZE];
   char realkey[VB_SIZE];
   char brdbustypestr[VB_SIZE];
   char topology[VB_SIZE];
   char devdevice[VB_SIZE];
   char backupkeyasstr[10];
   char realkeyasstr[10];
   char tmp[SIZE];
   char olddelim;
   int fd;
   u_int brdbustype,fixupcnt=0,numbernotfixed;
   struct {
      boolean_t hasproblems;
      rm_key_t key;   /* a backup key pointing to something that isn't legit */
      char brdid[VB_SIZE];
      char element[VB_SIZE];
      char niccardname[80];
      char realkeystr[VB_SIZE];
   } fixup[MAXFIXUP];
   extern rm_key_t RealKey2rmkey(char *);

   if ((space=getenv("__NDCFG_NOPOLICEMAN")) != NULL) {
      if (*space == '1') {
         /* we did an internal nd start/nd stop/nd restart command from the
          * ndcfg "idinstall" command
          */
         return(0);
      }
   }
   
   arg=NULL;
   if ((space=strstr(action," ")) != NULL) {
      arg=space+1;
      *space='\0';
   }

   /* is this a start command with no arguments (i.e. from /etc/rc2) ? */
   if ((strcmp(action,"start") != 0) || (arg != NULL)) {
      return(0);
   }
   /* ensure dlpid isn't running */
   snprintf(path,SIZE,"%s/dlpidPIPE",DLPIMDIDIR);
   if ((fd=open(path,O_WRONLY|O_NONBLOCK)) != -1) {
      /* this means that both:
       *  a) dlpimdi file does exist
       *  b) dlpid is already running and reading from fifo
       * any attempt to open up raw MDI device will fail.
       * of course, the dlpimdi file might exist but not be a FIFO, in 
       * which case it's the same result.
       */
      close(fd);
      return(0);
   }
   /* to get here means we have a start argument with no arguments
    * and dlpid isn't running.  We should then be able to open
    * all non-Token-Ring topology drivers successfully (barring any
    * UnixWare 2.1 DLPI-with-microcode-that-downloads-after-S15nd problems or
    * Gemini       MDI-with-microcode-that-downloads-after-S15nd problems)
    * UnixWare 2.1 ODI microcode drivers use firmware.o so they're not a 
    * problem.
    */
   status=OpenResmgr(O_RDWR);   /* for CopyAllParams and RM_RDWR here */
   if (status) {
      error(NOTBCFG,"ndcfg: CheckResmgrAndDevs: OpenResmgr failed with %d(%s)",
            status,strerror(status));
      return(1);
   }

   /* Pass 1: read the database to see what is missing */
   backuprmkey = NULL;
   while ((NKstatus=MyRMnextkey(&backuprmkey)) == 0) {
      RMbegin_trans(backuprmkey, RM_READ);
      if ((ec=RMgetvals(backuprmkey,CM_MODNAME,0,modname,VB_SIZE)) != 0) {
         RMabort_trans(backuprmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for MODNAME returned %d",
               ec);
         goto checkerror;
      }
      RMend_trans(backuprmkey);

      /* OLD WAY: if (strcmp(modname,CM_NDCFG) != 0) continue; */
      if (strlen(modname) <= 3) continue;
      if (strncmp(modname,"net",3) != 0) continue;
      if (!isdigit(modname[3])) continue;
      if ((modname[4] != '\0') && (!isdigit(modname[4]))) {
            continue;
      }
      /* we have *a* backup key to get here. may not be the right one though...
       */
     
      /* we have a backup key; read REALKEY to see what this should point to */
      snprintf(tmp,SIZE,"%s,s",CM_REALKEY);
      RMbegin_trans(backuprmkey, RM_READ);
      if ((ec=RMgetvals(backuprmkey,tmp,0,realkey,VB_SIZE)) != 0) {
         RMabort_trans(backuprmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for REALKEY returned %d",
               ec);
         goto checkerror;
      }
      RMend_trans(backuprmkey);

      if (strcmp(realkey,"-") == 0) {
         notice("CheckResmgrAndDevs: REALKEY not set at resmgr key %d",
                backuprmkey);
         continue;
      }

      /* get remaining information at key backuprmkey.  we then get information
       * at key realrmkey.  In particular, we need MODNAME, BRDBUSTYPE, 
       * and/or BRDID.  
       * we then match
       * from key backup key at backuprmkey             from realrmkey
       *     ----------------------------             --------------
       *           BACKUPMODNAME                         MODNAME
       *  BRDID (if BRDBUSTYPE isn't explictly ISA)       BRDID
       *           NCFG_ELEMENT                        NCFG_ELEMENT
       * if there's a difference then "something happened" to the
       * original key -- the user probably pulled the board from
       * the machine -- so we look for an unclaimed board to
       * re-attach it to (in the hopes that they re-inserted it and
       * now it's just unclaimed in the resmgr).  If we don't find
       * an unclaimed board then we don't start dlpid.
       * note that if they have 2 boards in the system but only 1 was
       * configured then this will have the effect of "silent failover"
       * to the new board.  It's a feature!
       */

      snprintf(tmp,SIZE,"%s",CM_BRDBUSTYPE);  /* no typing needed */
      RMbegin_trans(backuprmkey, RM_READ);
      if ((ec=RMgetvals(backuprmkey,tmp,0,brdbustypestr,VB_SIZE)) != 0) {
         RMabort_trans(backuprmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for backup BRDBUSTYPE "
                       "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(backuprmkey);

      brdbustype=atoi(brdbustypestr); /* will fail with 0 if not set("-") 
                                       * which is quite normal, given that
                                       * we never set it at the backup key
                                       * for smart-bus cards (only ISA)
                                       */
      /* if this is a backup key for an ISA board, then bail.  we can't
       * go any further since it's unlikely we'll find an "unclaimed"
       * board in the resmgr since autoconf never deletes them
       */
      if (brdbustype == CM_BUS_ISA) continue;
      /* ok, we know we have a smart-bus board to get here */

      /* read BRDID */
      snprintf(tmp,SIZE,"%s",CM_BRDID);  /* no typing needed */
      RMbegin_trans(backuprmkey, RM_READ);
      if ((ec=RMgetvals(backuprmkey,tmp,0,backupbrdid,VB_SIZE)) != 0) {
         RMabort_trans(backuprmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for backup BRDID "
                       "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(backuprmkey);

      if (strcmp(backupbrdid,"-") == 0) {
         /* we won't be able to find a match later on.  must continue.
          * Note that we normally write BRDID at our backup key in
          * CreateNewBackupKey() so someone must have deleted it...
          */
         notice("CheckResmgrAndDevs: BRDID not set at backup key %d",
                   backuprmkey);
         continue;
      }

      /* read NCFG_ELEMENT */ 
      snprintf(tmp,SIZE,"%s,s",CM_NETCFG_ELEMENT);
      RMbegin_trans(backuprmkey, RM_READ);
      if ((ec=RMgetvals(backuprmkey,tmp,0,backupelement,VB_SIZE)) != 0) {
         RMabort_trans(backuprmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for backup NCFG_ELEMENT "
                       "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(backuprmkey);

      if (strcmp(backupelement,"-") == 0) {
         notice("CheckResmgrAndDevs: NCFG_ELEMENT not set at backup key %d",
                       backuprmkey);
         continue;
      }

      snprintf(tmp,SIZE,"%s,s",CM_NIC_CARD_NAME);
      RMbegin_trans(backuprmkey, RM_READ);
      if ((ec=RMgetvals(backuprmkey,tmp,0,niccardname,VB_SIZE)) != 0) {
         RMabort_trans(backuprmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for NIC_CARD_NAME "
                       "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(backuprmkey);

      if (strcmp(niccardname,"-") == 0) {
         notice("CheckResmgrAndDevs: NIC_CARD_NAME not set at backup key %d",
                  backuprmkey);
         continue;
      }

      snprintf(tmp,SIZE,"%s,s",CM_BACKUPMODNAME);
      RMbegin_trans(backuprmkey, RM_READ);
      if ((ec=RMgetvals(backuprmkey,tmp,0,backupmodname,VB_SIZE)) != 0) {
         RMabort_trans(backuprmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for BACKUPMODNAME "
                       "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(backuprmkey);

      if (strcmp(backupmodname,"-") == 0) {
         notice("CheckResmgrAndDevs: BACKUPMODNAME not set at backup key %d",
                  backuprmkey);
         continue;
      }

      /* if ((realrmkey=atoi(realkey)) == 0)  */
      if ((realrmkey=RealKey2rmkey(realkey)) == RM_KEY) {
         /* board not found in resmgr -- card probably pulled */
         realmodname[0]='\0';
         realelement[0]='\0';
         realbrdid[0]='\0';
      } else {

         /* now get real brdid and modname at key realrmkey.  if that key 
          * doesn't exist then we'll also get '-' from RMgetvals
          * note autoconf can shuffle things around so while the info at key
          * realrmky may be bogus that doesn't mean that the key containing
          * good data has been deleted either
          */
         snprintf(tmp,SIZE,"%s",CM_BRDID);  /* no typing needed */
         RMbegin_trans(realrmkey, RM_READ);
         if ((ec=RMgetvals(realrmkey,tmp,0,realbrdid,VB_SIZE)) != 0) {
            RMabort_trans(realrmkey);
            error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for real BRDID "
                          "returned %d", ec);
            goto checkerror;
         }
         RMend_trans(realrmkey);

         snprintf(tmp,SIZE,"%s",CM_MODNAME);  /* no typing needed */
         RMbegin_trans(realrmkey, RM_READ);
         if ((ec=RMgetvals(realrmkey,tmp,0,realmodname,VB_SIZE)) != 0) {
            RMabort_trans(realrmkey);
            error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for real MODNAME "
                          "returned %d", ec);
            goto checkerror;
         }
         RMend_trans(realrmkey);

         snprintf(tmp,SIZE,"%s,s",CM_NETCFG_ELEMENT);
         RMbegin_trans(realrmkey, RM_READ);
         if ((ec=RMgetvals(realrmkey,tmp,0,realelement,VB_SIZE)) != 0) {
            RMabort_trans(realrmkey);
            error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for real ELEMENT "
                          "returned %d", ec);
            goto checkerror;
         }
         RMend_trans(realrmkey);

      }

      /* ok, we've got all of our variables.  if things don't match then
       * "something bad happened".  Don't complain just yet as autoconf
       * may simply have substituted another key for this one as things can get
       * shifted around quite easily as other boards get added.
       */

      if ((strcmp(backupmodname,realmodname) != 0) ||
          (strcmp(backupelement,realelement) != 0) ||
          (strcmp(backupbrdid,realbrdid) != 0)) {
         if (fixupcnt >= MAXFIXUP) {
            notice("ignoring problems with resmgr backup key %d",backuprmkey);
            continue;
         }
         fixup[fixupcnt].hasproblems = B_TRUE;
         fixup[fixupcnt].key = backuprmkey;  /* this key had the problem */
         strcpy(fixup[fixupcnt].brdid,backupbrdid);
         strcpy(fixup[fixupcnt].element,backupelement);
         strcpy(fixup[fixupcnt].realkeystr,realkey);
         strncpy(fixup[fixupcnt].niccardname,niccardname,80); /* can be > 80 */
         fixupcnt++;
      }

   }  /* for every key in resmgr */

   /* pass 2: if we had any problems on pass 1 then try and find
    * an unclaimed board that matches and fix it up.  These would
    * represent the board that is re-inserted in the system or the "silent-
    * failover" case mentioned above.
    * note dcu and crew can re-assign MODNAME based on Drvmap file in link kit
    * so we must key off of element instead.
    */
   realrmkey = NULL;
   if (fixupcnt > 0) {
      while ((NKstatus=MyRMnextkey(&realrmkey)) == 0) {
         RMbegin_trans(realrmkey, RM_READ);
         snprintf(tmp,SIZE,"%s,s",CM_NETCFG_ELEMENT);
         if ((ec=RMgetvals(realrmkey,tmp,0,realelement,VB_SIZE)) != 0) {
            RMabort_trans(realrmkey);
            error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for ELEMENT "
                  "returned %d", ec);
            goto checkerror;
         }
         RMend_trans(realrmkey);

         if (strcmp(realelement,"-") != 0) continue; /* already claimed NIC */

         /* nothing set in element -- possible unclaimed board - note it 
          * may not be a nic board though! must get brdid to find out 
          */
         RMbegin_trans(realrmkey, RM_READ);
         if ((ec=RMgetvals(realrmkey,CM_BRDID,0,realbrdid,VB_SIZE)) != 0) {
            RMabort_trans(realrmkey);
            error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for BRDID "
                  "returned %d", ec);
            goto checkerror;
         }
         RMend_trans(realrmkey);

         /* can we find an orphan backup key that this will go to? */
         for (loop=0; loop<fixupcnt; loop++) {
            if (strcmp(fixup[loop].brdid,realbrdid) == 0) {
               if (CopyAllParams(realrmkey,fixup[loop].key, 
                                 fixup[loop].element, /*BACKUPKEY at real key*/
                                 fixup[loop].realkeystr /* REALKEY at backup */
                                ) != 0) {
                  error(NOTBCFG,"CheckResmgrAndDevs: CopyAllParams failed");
                  goto checkerror;
               }
               fixup[loop].hasproblems = B_FALSE;
               break;
            }
         }
      }  /* for every key */     
   }

   /* pass 3: find claimed boards that got their CM_BACKUPKEY or CM_REALKEY
    * messed up by re-associating with CM_NETCFG_ELEMENT
    * for every key that hasn't been fixed yet, try and find its
    * match in the resmgr
    */
   for (loop=0; loop < fixupcnt; loop++) {
      if (fixup[loop].hasproblems == B_TRUE) {
         realrmkey = NULL;
         while ((NKstatus=MyRMnextkey(&realrmkey)) == 0) {
            RMbegin_trans(realrmkey, RM_READ);
            if ((ec=RMgetvals(realrmkey,CM_MODNAME,0,modname,VB_SIZE)) != 0) {
               RMabort_trans(realrmkey);
               error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for MODNAME "
                     "returned %d", ec);
               goto checkerror;
            }
            RMend_trans(realrmkey);

            /* skip unclaimed boards or backup entries; we've already processed
             * them in earlier passes.  old way used CM_NDCFG here
             */
            if ((strlen(modname) == 4) &&           /* "net0" through "net9" */
                (strncmp(modname,"net",3) == 0) &&  /* starts with "net" */
                (isdigit(modname[3]))) continue;    /* ends with a number */

            if ((strlen(modname) == 5) &&           /* "net10" through "net99"*/
                (strncmp(modname,"net",3) == 0) &&  /* starts with "net" */
                (isdigit(modname[3])) &&            /* 4th char is a number */
                (isdigit(modname[4]))) continue;    /* 5th char is a number */

            if ((strcmp(modname,"-") == 0) ||
                (strcmp(modname,"unused") == 0)) continue;

            /* this could be a network board.  look at element to find out */
            snprintf(tmp,SIZE,"%s,s",CM_NETCFG_ELEMENT);
            RMbegin_trans(realrmkey, RM_READ);
            if ((ec=RMgetvals(realrmkey,tmp,0,realelement,VB_SIZE)) != 0) {
               RMabort_trans(realrmkey);
               error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for NETCFG_ELEMENT "
                     "returned %d", ec);
               goto checkerror;
            }
            RMend_trans(realrmkey);

            /* if element isn't set then this isn't a *configured*
             * network driver.   It might not even be a nic board at all!
             */
            if (strcmp(realelement,"-") == 0) continue;

            /* if this isn't the element we're looking for then continue */
            if (strcmp(realelement,fixup[loop].element) != 0) continue;

            snprintf(tmp,SIZE,"%s,n",CM_NDCFG_UNIT);
            RMbegin_trans(realrmkey, RM_READ);
            if ((ec=RMgetvals(realrmkey,tmp,0,ndcfg_unit,VB_SIZE)) != 0) {
               RMabort_trans(realrmkey);
               error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for NDCFG_UNIT "
                     "returned %d", ec);
               goto checkerror;
            }
            RMend_trans(realrmkey);

            if (strcmp(ndcfg_unit,"-") == 0) {
               notice("CheckResmgrAndDevs: NDCFG_UNIT not set at key %d",
                      realrmkey);
               continue;
            }

            /* ok, the fixup[] entry doesn't have the problem we thought it did
             * but the CM_BACKUPKEY and CM_REALKEY are definitely out of sync.
             * fix that and mark this entry as fixed.
             */
            fixup[loop].hasproblems = B_FALSE;

            /* old way: snprintf(backupkeyasstr,10,"%d",fixup[loop].key); */
            snprintf(backupkeyasstr,10,"%s",realelement);
            snprintf(realkeyasstr,10,"%s%s",modname,ndcfg_unit);

            /* update CM_BACKUPKEY with new information */
            snprintf(tmp,SIZE,"%s,s",CM_BACKUPKEY);
            RMbegin_trans(realrmkey, RM_RDWR);
            MyRMdelvals(realrmkey,tmp);
            RMend_trans(realrmkey);
            RMbegin_trans(realrmkey, RM_RDWR);
            RMputvals_d(realrmkey,tmp,backupkeyasstr,delim);
            RMend_trans(realrmkey);

            /* update CM_REALKEY at the backup key with new information */
            snprintf(tmp,SIZE,"%s,s",CM_REALKEY);
            RMbegin_trans(fixup[loop].key, RM_RDWR);
            MyRMdelvals(fixup[loop].key,tmp);
            RMend_trans(fixup[loop].key);
            RMbegin_trans(fixup[loop].key, RM_RDWR);
            RMputvals_d(fixup[loop].key,tmp,realkeyasstr,delim);
            RMend_trans(fixup[loop].key);

         }  /* for every key in resmgr */
      }  /* if this fixup entry still has problems */
   }  /* for every fixup entry */

   /* pass 4: for every network card that we can find (excluding backup
    * keys), try and open up the device unless it is token ring.  Attaching
    * to the ring is time consuming and dlpid will attempt to do this later
    * on(but only if this is an MDI driver). 
    * this has the nice side effect of putting all driver init/load messages
    * at the same place during system boot up, whether the driver is
    * MDI, DLPI, or ODI, and no matter what type of stack will use it.   
    * We have already run CopyAllParams at this point so DEV_NAME is set.
    */
   realrmkey = NULL;
   while ((NKstatus=MyRMnextkey(&realrmkey)) == 0) {
      RMbegin_trans(realrmkey, RM_READ);
      if ((ec=RMgetvals(realrmkey,CM_MODNAME,0,modname,VB_SIZE)) != 0) {
         RMabort_trans(realrmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for MODNAME "
               "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(realrmkey);

      /* skip unclaimed boards or backup entries; we've already processed
       * them in earlier passes.  old way used CM_NDCFG here
       */
      if ((strlen(modname) == 4) &&           /* "net0" through "net9" */
          (strncmp(modname,"net",3) == 0) &&  /* starts with "net" */
          (isdigit(modname[3]))) continue;    /* ends with a number */

      if ((strlen(modname) == 5) &&           /* "net10" through "net99"*/
          (strncmp(modname,"net",3) == 0) &&  /* starts with "net" */
          (isdigit(modname[3])) &&            /* 4th char is a number */
          (isdigit(modname[4]))) continue;    /* 5th char is a number */

      if ((strcmp(modname,"-") == 0) ||
          (strcmp(modname,"unused") == 0)) continue;

      /* this could be a network board.  look at element to find out */
      snprintf(tmp,SIZE,"%s,s",CM_NETCFG_ELEMENT);
      RMbegin_trans(realrmkey, RM_READ);
      if ((ec=RMgetvals(realrmkey,tmp,0,realelement,VB_SIZE)) != 0) {
         RMabort_trans(realrmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for NETCFG_ELEMENT "
               "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(realrmkey);

      /* if element not set then not a network board(or in the case 
       * of multiple nics of the same type this could be a non-configured
       * NIC).  In either case we continue.
       */
      if (strcmp(realelement,"-") == 0) continue;

      /* it _is_ a network board.  But is it the dreaded Broken Ring(tm)? */
      snprintf(tmp,SIZE,"%s,s",CM_TOPOLOGY);
      RMbegin_trans(realrmkey, RM_READ);
      if ((ec=RMgetvals(realrmkey,tmp,0,topology,VB_SIZE)) != 0) {
         RMabort_trans(realrmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for TOPOLOGY "
               "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(realrmkey);

      if (strcmp(topology,"-") == 0) {
         notice("CheckResmgrAndDevs: TOPOLOGY not set at key %d",realrmkey);
         continue;
      }

      /*CM_TOPOLOGY is finally single valued so don't have to substring match*/
      if ((strstr(topology,"TOKEN") != NULL) ||
          (strstr(topology,"token") != NULL)) continue;
   
      /* hurray, it's not.  get the device name to open */
      snprintf(tmp,SIZE,"%s,s",CM_DEV_NAME);
      RMbegin_trans(realrmkey, RM_READ);
      if ((ec=RMgetvals(realrmkey,tmp,0,devdevice,VB_SIZE)) != 0) {
         RMabort_trans(realrmkey);
         error(NOTBCFG,"CheckResmgrAndDevs: RMgetvals for DEV_NAME "
               "returned %d", ec);
         goto checkerror;
      }
      RMend_trans(realrmkey);

      if (strcmp(devdevice,"-") == 0) {
         notice("CheckResmgrAndDevs: DEV_NAME not set at key %d",realrmkey);
         continue;
      }

      if (strncmp(devdevice,"/dev/",5) != 0) {
         notice("CheckResmgrAndDevs: invalid DEV_NAME of '%s'",devdevice);
         continue;
      }
      
      fd=open(devdevice,O_RDONLY);
      if (fd == -1) {
         /* we know the resmgr keys and backup keys are ok.  maybe the
          * user simply renamed the device to prevent the stacks from using
          * the card.  Don't bomb out; just whine a bit
          * could also require download code which hasn't run yet before
          * /etc/rc2.d/S15nd which is how we got here in the first place.
          * Since users will see this don't proceed text with CheckResmgrAndDevs
          * XXX NOTE THIS IS NOT INTERNATIONALIZED
          */
         notice("open of %s failed with \"%s\"",
                devdevice,strerror(errno));
         notice("this failure may affect protocol stacks");
      } else {
         close(fd);
      }

      /* do PATCH custom parameters since they are applicable to all
       * types of boards (ISA, EISA, PCI, MCA).  The driver's load routine
       * should have been called since we just opened up the device, making
       * the driver's symbols visible to be patched via /dev/kmem.
       */
      if ((ec=DoPatchCustom(realrmkey)) != 0) {
         error(NOTBCFG,"CheckResmgrAndDevs: DoPatchCustom returned %d",ec);
         goto checkerror;
      }

   }

   /* All done.  ensure that all problems were fixed.  any hasproblems still
    * set to B_TRUE indicate that no primary key exists, shuffled or unclaimed,
    * that matches this element.  
    */
   if (fixupcnt > 0) {
      for (loop=0; loop<fixupcnt; loop++) {
         if (fixup[loop].hasproblems == B_TRUE) {
            PrimeErrorBuffer(52, "the network adapter(s) named below were "
                     "physically "
                     "removed from\n\rthe system.  Re-insert the adapter, "
                     "run /usr/sbin/netcfg to remove\n\rits information, then "
                     "remove the adapter.\n\r"); 
            /* since calls below to error() go to stderr send to stderr too */
            Pstderr("%s",ErrorMessage); /* set in PrimeErrorBuffer */
            break;
         }
      }
   }

   numbernotfixed=0;
   if (fixupcnt > 0) {
      for (loop=0; loop<fixupcnt; loop++) {
         if (fixup[loop].hasproblems == B_TRUE) {
            /* XXX NOTE THIS IS NOT INTERNATIONALIZED but PrimeErrorBuffer
             * set things up so hopefully this will make some sense to user
             */
            error(NOTBCFG,
                  "The network card '%s'\n\rwith board id %s representing "
                  "/dev/%s was not found in the resmgr!",
                  fixup[loop].niccardname,
                  fixup[loop].brdid,
                  fixup[loop].element);
            numbernotfixed++;
         }
      }
      if (numbernotfixed == 0) {
         /* we had problems (fixupcnt > 0) but they're all fixed now.  
          * run idconfupdate
          * to make our changes permanent.
          */
         snprintf(tmp,SIZE,"/etc/conf/bin/idconfupdate -f"); 
         status=docmd(0,tmp);
         if (status != 0) {
            error(NOTBCFG,"CheckResmgrAndDevs: command idconfupdate failed");
            goto checkerror;
         }
      }
   }
   if (numbernotfixed != 0) goto checkerror;
   (void) CloseResmgr();
   return(0);

checkerror:
   error(NOTBCFG,"\007\007NOT STARTING DLPID UNTIL PROBLEM IS SOLVED\007\007");
   error(NOTBCFG,"\007THIS PREVENTS FRAMES FROM GOING OUT ON WRONG CARD\007");
   nap(5000); /* while we *could* use sleep here since we never set any
               * itimers or changed SIGALRM handler, we shy away from it to
               * reinforce good practice: can't shouldn't sleep/alarm because
               * we use itimers which send SIGALARM which
               * will wake up sleep early since sleep just calls alarm
               */
   (void) CloseResmgr();
   return(1);  /* ndcfg will do an exit 1 which means don't start dlpid */
}

/* start the backup key process */
void
StartBackupKey(rm_key_t the_backup_key)
{
   if (the_backup_key == RM_KEY) {
      notice("StartBackupKey: invalid backupkey");
      return;
   }
   /* we only need to set the global variable; resput does the work */
   if (g_backupkey != RM_KEY) {
      notice("StartBackupKey: already doing backups to key %d!",g_backupkey);
   }
   g_backupkey=the_backup_key;
   return;
}

/* this routine can be called when backups are not enabled */
void
EndBackupKey(char *modname)
{
   char tmp[SIZE];
   char keyasstr[10];

   /* did we call StartBackupKey? */
   if ((g_backupkey != RM_KEY) && (modname != NULL)) {
      snprintf(tmp,SIZE,"%s=%s",CM_MODNAME,modname);
      snprintf(keyasstr,10,"%d",g_backupkey);
      /* must turn off backups *now* for call to resput in next line! */
      g_backupkey=RM_KEY;   
      if (resput(keyasstr,tmp,0)) {  /* does OpenResmgr/CloseResmgr */
         notice("EndBackupKey: resput failed");
      }
   } else {
      /* not in backup mode or we shouldn't change MODNAME from current value */
      g_backupkey=RM_KEY;   /* always turn off backups */
   }
}

/* returns the new rm_key_t if success
 * returns RM_KEY if error
 */
rm_key_t
CreateNewBackupKey(rm_key_t existing, char *driver_name, 
                   int boardnumber, int netX)
{
   rm_key_t newkey;
   char brdid[VB_SIZE];
   char tmp[VB_SIZE];
   int status,ec,nthvalue=0;
   char keyasstr[10];

   if (ResmgrNextKey(&newkey,1)) {  /* does OpenResmgr/CloseResmgr */
      error(NOTBCFG,"CreateNewBackupKey: ResmgrNextKey failed");
      return(RM_KEY);
   }
  
   status=OpenResmgr(O_RDONLY);
   if (status) {
       error(NOTBCFG,"CreateNewBackupKey: OpenResmgr failed with %d(%s)",
             status,strerror(status));
       RMdelkey(newkey);  /* this will probably fail, but try anyway */
       return(RM_KEY);
   }

   /* get the board id if it exists from existing and save to backup key */
   RMbegin_trans(existing, RM_READ);
   ec=RMgetvals(existing,CM_BRDID,nthvalue,brdid,VB_SIZE);
   if (ec != 0) {
      RMabort_trans(existing);
      error(NOTBCFG,"CreateNewBackupKey: RMgetvals for BRDID returned %d",ec);
      RMdelkey(newkey);  /* delete key we just created */
      (void) CloseResmgr();/* if we ever use iopenedit check this first! */
      return(RM_KEY);
   }
   RMend_trans(existing);
   (void)CloseResmgr();   /* must close since resput assumes closed */

   /* now write out BRDID to key we just created.  If existing is a
    * ISA key that we just created then it won't be set, so we'll
    * write out exactly what we got: a literal '-' to BRDID
    */
   snprintf(keyasstr,10,"%d",newkey);
   snprintf(tmp,SIZE,"%s=%s",CM_BRDID,brdid);  /* no typing needed */
   if (resput(keyasstr,tmp,0)) {
      error(NOTBCFG,"CreateNewBackupKey: resput for BRDID failed");
      RMdelkey(newkey);  /* will probably fail but try anyway */
      return(RM_KEY);
   }

#if 0
the mere presence of CM_VOLATILE now indicates to idconfupdate 
that this key should not be copied to /stand/resmgr
#ifdef CM_VOLATILE
   /* add .VOLATILE parameter so that a future idconfupdate -s won't delete 
    * this key since there's no real hardware driver for it.  
    * must have a value of B_TRUE(1), which corresponds to B_TRUE enum in 
    * <sys/types.h>, which is used in new DDI8 call cm_newkey(), used by
    * idconfupdate.  This is only necessary for entries in the resmgr
    * that won't have a hardware device, so we don't have to do it for
    * the primary key.
    * unfortunately libresmgr doesn't know about this parameter so we must
    * supply typing information.
    * we don't use 1 any more because the compiler may change the default
    * starting number for an enum.  Some default to 0, others use 1 for
    * the first enum element.  Also, since an enum may be stored at the 
    * minimum number of bytes required (i.e. could be a character), we 
    * cast to an int.
    */
   snprintf(tmp,SIZE,"%s,n=%d",CM_VOLATILE,(int) B_TRUE);
   if (resput(keyasstr,tmp,0)) {
      error(NOTBCFG,"CreateNewBackupKey: resput for VOLATILE failed");
      RMdelkey(newkey);  /* will probably fail but try anyway */
      return(RM_KEY);
   }
#endif
#endif

   /* add CM_REALKEY=existing at the newkey/backupkey */
   snprintf(keyasstr,10,"%d",newkey);
   snprintf(tmp,SIZE,"%s,s=%s%d",CM_REALKEY,driver_name,boardnumber);
   if (resput(keyasstr,tmp,0)) {
      error(NOTBCFG,"CreateNewBackupKey: resput for REALKEY failed");
      RMdelkey(newkey);  /* will probably fail but try anyway */
      return(RM_KEY);
   }

   /* now add CM_BACKUPKEY=newkey to the existing key that we are backing up */
   snprintf(keyasstr,10,"%d",existing);
   snprintf(tmp,SIZE,"%s,s=net%d",CM_BACKUPKEY,netX);
   if (resput(keyasstr,tmp,0)) {
      error(NOTBCFG,"CreateNewBackupKey: resput for BACKUPKEY failed");
      RMdelkey(newkey);  /* will probably fail but try anyway */
      return(RM_KEY);
   }

   return(newkey);
}

/* determine if ITYPE parameter is set at the given key.  This parameter
 * should not be overwritten if already set.  Certain autoconf boards
 * (PCI/EISA) automatically set this parameter.  Used by the idinstall
 * command
 *  returns -1 if error
 *  returns 0 if not set
 *  returns 1 if set
 */
int
ItypeSet(rm_key_t rmkey)
{
   int status,ec,nthvalue=0;
   char itype[VB_SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
       error(NOTBCFG,"ItypeSet: OpenResmgr failed with %d(%s)",
             status,strerror(status));
       return(-1);
   }

   RMbegin_trans(rmkey, RM_READ);
   ec=RMgetvals(rmkey,CM_ITYPE,nthvalue,itype,VB_SIZE);
   if (ec != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ItypeSet: RMgetvals for ITYPE returned %d",ec);
      goto itypefail;
   }
   RMend_trans(rmkey);
   (void)CloseResmgr();

   if (strcmp(itype,"-") == 0) {
      return(0);
   } else {
      return(1);
   }
   /* NOTREACHED */
itypefail:
   (void)CloseResmgr();
   return(-1);
}


/* determine if IPL parameter is set at the given key.  This parameter
 * should not be overwritten if already set.  Certain autoconf boards
 * (PCI/EISA) automatically set this parameter.  Used by the idinstall
 * command
 *  returns -1 if error
 *  returns 0 if not set
 *  returns 1 if set
 */
int
IplSet(rm_key_t rmkey)
{
   int status,ec,nthvalue=0;
   char ipl[VB_SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
       error(NOTBCFG,"IplSet: OpenResmgr failed with %d(%s)",
             status,strerror(status));
       return(-1);
   }

   RMbegin_trans(rmkey, RM_READ);
   ec=RMgetvals(rmkey,CM_IPL,nthvalue,ipl,VB_SIZE);
   if (ec != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"IplSet: RMgetvals for IPL returned %d",ec);
      goto iplfail;
   }
   RMend_trans(rmkey);
   (void)CloseResmgr();

   if (strcmp(ipl,"-") == 0) {
      return(0);
   } else {
      return(1);
   }
   /* NOTREACHED */
iplfail:
   (void)CloseResmgr();
   return(-1);
}



/* returns -1 for failure
 * Much of the code in DoDSPStuff is replicated here
 * where is the pointer to the bcfg file, which may or may not exist.
 * in the case of dlpi, it won't
 */
int
GoInstallDriver(char *driver_name, char *where)
{
   char sourcedir[SIZE];
   char sourcefile[SIZE];
   char destdir[SIZE];
   char tmpfilename[SIZE];
   char tmpdir[SIZE];
   char bcfgfile[SIZE];
   char cmdbuf[SIZE];
   char *slash;
   char *tmpdirname;
   char *tmp;
   int status;
   struct stat sb;
   const char *tempnamdir=TEMPNAMDIR;
   const char *tempnampfx=TEMPNAMPFX;
   
   /* pkgadd uses /var/tmp, so will we.  Allow TMPDIR environment variable
    * to override.
    */
   strncpy(bcfgfile,where,SIZE);
   tmpdirname=tempnam(tempnamdir,tempnampfx);
   if (tmpdirname == NULL) {
      error(NOTBCFG,"GoInstallDriver: tempnam/malloc failed");
      return(-1);
   }
   strcpy(destdir,tmpdirname);
   if (mkdir(destdir,0700) == -1) {
      error(NOTBCFG,"GoInstallDriver: mkdir(%s) failed",destdir);
      goto fail;
   }
   strncpy(tmpdir,where,SIZE);
   slash=strrchr(tmpdir,'/');
   if (slash == NULL) {
      /* shouldn't happen since we mandate absolute pathnames now */
      error(NOTBCFG,"strrchr failed in GoInstallDriver");
      goto fail;
   } else {
      *(slash)='\0';  /* no trailing /foo.bcfg on our directory */
   }
   strncpy(sourcedir,tmpdir,SIZE-10);
   if (sourcedir[0] != '/') {
      /* shouldn't happen */
      error(NOTBCFG,"GoInstallDriver: sourcedir (%s) not absolute",sourcedir);
      goto fail;
   }

   if (chdir(tmpdir) == -1) {
      /* someone deleted the bcfg file directory after our loaddir */
      error(NOTBCFG,"GoInstallDriver: couldn't chdir to %s",tmpdir);
      goto fail;
   }

   if (stat(sourcedir,&sb) == -1) {
      error(NOTBCFG,"GoInstallDriver: can't stat %s: errno=%d",sourcedir,errno);
      goto fail;
   }

   if (!S_ISDIR(sb.st_mode)) {
      error(NOTBCFG,"GoInstallDriver: %s isn't a directory!",sourcedir);
      goto fail;
   }

   tmp=strrchr(bcfgfile,'/');
   if (tmp == NULL) {
      error(NOTBCFG,"strrchr failed in GoInstallDriver");
      goto fail;
   } else {
      strcpy(sourcefile,tmp+1);
   }
   /* if the bcfgfile exists, then copy it (and only this bcfg file) to
    * the new directory, keeping the same file name in the new directory 
    * in the case of dlpi, its DUMMY.bcfg won't exist
    */
   if (stat(bcfgfile,&sb) != -1) {
      if (copyfile(1,sourcedir,sourcefile,destdir,sourcefile) != 0) {
         error(NOTBCFG,"Problem copying %s/%s to %s/%s",
               sourcedir,sourcefile,destdir,sourcefile);
         goto fail;   /* we'll get multiple erros in log but that's ok */
      }
   }

   /* now copy all other possible files to the tmp directory
    * we try for anything that could possibly be idinstalled.  Most
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

   /* modify the System file in our tmpdir to enable the driver. */
   chdir(tmpdir);
   if (TurnOnSdevs() == -1) {
      error(NOTBCFG,"GoInstallDriver: couldn't turn on System in %s",tmpdir);
      goto fail;
   }
   snprintf(cmdbuf,SIZE,"/etc/conf/bin/idinstall -P nics -a -N -e -k %s", 
           driver_name);
   status=docmd(0,cmdbuf);
   if (status != 0) {
      error(NOTBCFG,"trouble idinstalling %s into link kit",bcfgfile);
      goto fail;
   }

   chdir("/");
   snprintf(tmpfilename,SIZE,"/bin/rm -rf %s",tmpdirname);
   docmd(0,tmpfilename);   /* it might work, it might not.  we don't care */
   rmdir(destdir);
   free(tmpdirname);
   return(0);

fail:
   chdir("/");
   snprintf(tmpfilename,SIZE,"/bin/rm -rf %s",tmpdirname);
   docmd(0,tmpfilename);   /* it might work, it might not.  we don't care */
   rmdir(destdir);
   free(tmpdirname);
   return(-1);
}

/* returns -1 for failure 
 * only for ISA boards with MDI drivers.
 * create a new resmgr key, populate it with each available I/O address
 * that hasn't already been taken, then call the verify routine.
 * If verify routine says this I/O address range is valid then remember it
 * for later.
 * We don't currently add a memory address to the resmgr, so a purely 
 * memory mapped ISA board will not work.  Of course, the dcu won't work
 * either, so I'm not that concerned.
 *
 * XXX should we implement the dcu's idea of silent_verify, as found
 *     in the Drvmap file?  We would have to do this fairly early on
 *
 * NOTE:  this command will not work at netisl=1 time since we assume
 * that the link kit is installed.  It will require further work to 
 * deal with Driver.mod/idmodreg if netisl is a requirement.
 * NOTE:  If you issue an isaautodetect set <element> FOO=bar from cmdparser.y
 *        then this routine will not create a new key, using the existing
 *        parameters at that key (notably IOADDR) to call the verify routine
 *        this method is intended to just change ISA attributes and _NOT_
 *        Custom parameters -- they are not updated at the key if they
 *        are provided as a command line argument from cmdparser.y
 */

#define MAXAUTODETECT 20

/* from mdi.h */
#define MDI_ISAVERIFY_UNKNOWN 0
#define MDI_ISAVERIFY_GET 1 /* retrieve params from eeprom, call again later */
#define MDI_ISAVERIFY_SET 2 /* retrieve params from resmgr & set in firmware */
#define MDI_ISAVERIFY_GET_REPLY 3 /* 2nd time: set params in resmgr from info*/

int
isaautodetect(int fromidmodify,
              int installindex, 
              rm_key_t installkey, 
              union primitives primitive)
{
   struct stat sb;
   stringlist_t *slp;
   u_int z,numargs,numcustom,mustidbuild=0;
   int bcfgindex=99999,fakeioaddr=0;
   int status,numport,loop,customloop;
   char *strend, *port, *slpport, *slpmem, *slpdma, *slpirq, *spacep;
   char *slpcustom;
   char *driver_name=NULL, *s_iowc, *s_memwc, *s_irqwc, *s_dmawc;
   rm_key_t rmkey,putconfkey;
   char bcfgindexstr[10];
   char drivermod[20];
   char tmp[SIZE];
   char cmdbuf[SIZE];
   char irq[VB_SIZE],memaddr[VB_SIZE],dma[VB_SIZE];
   char customname[10][VB_SIZE];
   cm_num_t mode;
   struct mod_verify {
      char mv_modname[MODMAXNAMELEN];   /* mfpd, etc. */
      rm_key_t mv_key;   /* passed to driver xxx_verify routine */
   } mv;
   char *string="success", *oldport;
   char verify_mode[20],putconfkeystr[20]; 
   char olddelim=delim;
   /* we don't allow multiple address ranges for ioaddr and memaddr */
   struct {
      char io[20];
      /* io address is never in conflict since we use RO_ISPARAMAVAIL */
      char irq[20];
      char irqconflict[20];
      char mem[20];
      char memconflict[20];
      char dma[20];
      char dmaconflict[20];
      char customvalue[10][VB_SIZE];
   } found[MAXAUTODETECT];
   u_int numfound=0,n_iowc,n_memwc,n_irqwc,n_dmawc;
   int installedDLPI=0;
   int installedMDI=0;
   int openedresmgr=0;


   mode=MDI_ISAVERIFY_UNKNOWN;
   rmkey=RM_KEY;
   port = NULL;
   if (netisl == 1) {
      /* NOTE:  this command will not work at netisl=1 time since we assume
       * that the link kit is installed.  It will require further work to 
       * deal with Driver.mod/idmodreg if netisl is a requirement.
       */
      error(NOTBCFG,"isaautodetect: not implemented yet");
      return(-1);
   }

   if (ensureprivs() == -1) {
      error(NOTBCFG,"isaautodetect: insufficient priviledge to "
                    "perform this command");
      return(-1);
   }

   /* since showtopo shows BUS we can error since this shouldn't happen
    * Also, idinstall only calls us if we're installing an ISA card
    * on an ISA machine, so checking again won't hurt. 
    */
   if ((cm_bustypes & CM_BUS_ISA) == 0) {
      /* MCA machines fall into this category */
      error(NOTBCFG,"isaautodetect: no ISA bus in this machine!");
      return(-1);
   }

   slp=&primitive.stringlist;

   numargs=CountStringListArgs(slp);
   if (numargs < 2) {
      error(NOTBCFG,"isaautodetect requires at least 2 arguments");
      return(-1);
   }

   /* if we're called by idinstall/idmodify command then our arguments will
    * be different.  Don't do usual checks
    */
   if (installkey != RM_KEY) {
      /* called from idinstall or idmodify command */
      mode=MDI_ISAVERIFY_SET;
      bcfgindex=installindex;
      snprintf(bcfgindexstr,10,"%d",bcfgindex);
      rmkey=RM_KEY;  /* we will create a new key below */
      putconfkey=installkey;
   } else {
      /* called from cmdparser.y: first argument is get/set mode */
      if ((strcmp(slp->string,"get") == 0) ||
          (strcmp(slp->string,"GET") == 0)) {
         mode=MDI_ISAVERIFY_GET;
         slp=slp->next;

         /* putconfkey isn't applicable to "get" mode since driver won't
          * call cm_AT_putconf when retrieving the parameters
          */
         putconfkey = RM_KEY;

         /* second argument is bcfgindex - avoid atoi */
         if (((bcfgindex=strtoul(slp->string,&strend,10))==0) &&
              (strend == slp->string)) {
            error(NOTBCFG,"isaautodetect: invalid bcfgindex %s",slp->string);
            return(-1);
         }

         if (bcfgindex >= bcfgfileindex) {
            error(NOTBCFG,"bcfgindex %d >= maximum(%d)",
                          bcfgindex,bcfgfileindex);
            return(-1);
         }
         snprintf(bcfgindexstr,10,"%d",bcfgindex);
      } else
      if ((strcmp(slp->string,"set") == 0) ||
          (strcmp(slp->string,"SET") == 0)) {
         mode=MDI_ISAVERIFY_SET;
         slp=slp->next;

         /* second argument is element */
         rmkey = resshowkey(0, slp->string, NULL, 0); /*OpenResmgr/CloseResmgr*/
         if (rmkey == RM_KEY) {
            error(NOTBCFG,"isaautodetect: resshowkey failed for element %s",
                  slp->string);
            return(-1);
         }
 
         putconfkey = rmkey;   /* in case driver needs to do cm_AT_putconf 
                                * this is the key it should pass as argument
                                */

         bcfgindex=elementtoindex(0,slp->string); /* OpenResmgr/CloseResmgr */
         if (bcfgindex == -1) {
            /* hmm, user likely removed the bcfg file or ran ndcfg with -q
             * and never issued a loaddir/loadfile command before the
             * isaautodetect command.   notice already called.
             */
            error(NOTBCFG,"isaautodetect: no bcfgindex for element %s",
                          slp->string);
            return(-1);
         }
         snprintf(bcfgindexstr,10,"%d",bcfgindex);
      } else
      if (mode == MDI_ISAVERIFY_UNKNOWN) {
         error(NOTBCFG,"unknown mode '%s' (should be get or set)",slp->string);
         return(-1);
      }
      slp=slp->next;
   }

   /* NOTE: at this point we have properly set the following
    * - mode
    * - bcfgindex
    * - bcfgindexstr
    * - rmkey (if mode is MDI_ISAVERIFY_SET)
    * - putconfkey (although only applicable to MDI_ISAVERIFY_SET)
    * irrespective how how we got to this routine and what the desired mode is.
    */

   /* since showtopo command returns bus type netcfg shouldn't be calling
    * this routine for smart bus cards.  so error
    */
   if (HasString(bcfgindex, N_BUS, "ISA", 0) != 1) {
      error(NOTBCFG,"isaautodetect: bcfgindex %d isn't for ISA bus",
            bcfgindex);
      return(-1);
   }

   if (HasString(bcfgindex, N_ISAVERIFY, "x", 0) == -1) {
      /* .bcfg file indicates driver doesn't have a verify routine by
       * the absence of the ISAVERIFY parameter.  don't
       * bother proceeding further in this routine for either get or set;
       * this will be futile and slow netcfg down.
       */
      notice("isaautodetect: bcfgindex %d indicates no verify routine",
             bcfgindex);
      return(0);
   }

   driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
   if (driver_name == NULL) {
      error(NOTBCFG,"isaautodetect: DRIVER_NAME for bcfgindex %d is NULL",
            bcfgindex);
      return(-1);
   }
   snprintf(drivermod,20,"-M %s",driver_name);

   /* must calculate and fill in custom 1st for any call 
    * to fakesuccess below 
    */
   numcustom=showcustomnum(0, bcfgindexstr);

   for (loop=1; loop <= numcustom; loop++) {
      char *z;
   
      snprintf(tmp,SIZE,"CUSTOM[%d]",loop);
      z=StringListPrint(bcfgindex, N2O(tmp));
      if (z == NULL) {
         error(NOTBCFG,"isaautodetect: StringListPrint(%d,%d) is NULL",
               bcfgindex,N2O(tmp));
         free(driver_name);
         return(-1);
      }
      spacep=strstr(z, " ");
      *spacep=NULL;
      strncpy(&customname[loop][0],z,VB_SIZE);
      free(z);
   }

   /* NOTE: from here below must goto fakesuccess/detectfail to free memory! */
   delim='\1';   /* ASCII 1 = SOH, an unlikely character */

   /* right now our autodetect scheme assumes that we have a bcfg file that
    * has an I/O address.  This means that boards that are purely memory
    * mapped will not work, since we don't put the memory address into the
    * resmgr for drivers to see.  Fail if PORT isn't defined.
    * BTW, this matches the dcu.
    * since netcfg doesn't have any idea if driver is purely memory mapped
    * don't error but fake success
    */
   numport=StringListNumWords(bcfgindex, N_PORT);
   if (numport <= 0) {
      notice("isaautodetect: PORT= not defined for bcfg file - faking success");
      goto fakesuccess;
   }

   if (HasString(bcfgindex, N_TYPE, "MDI", 0) != 1) {
      /* not MDI, so no verify routine.  fake success */
      goto fakesuccess;
   }

   if (StringListNumWords(bcfgindex, N_DEPEND) > 0) {
      /* we don't idinstall any of the DEPEND= drivers in this routine */
      notice("isaautodetect: DEPEND= not supported; faking success");
      goto fakesuccess;
   }

   /* see if this driver is in the link kit yet.  if it isn't in the link
    * kit yet a call to modadm to call its verify routine would also fail.  
    * It would also fail if the driver doesn't have a verify routine
    * We can't use modload(2) and check for ENOENT because the driver may
    * not be able to be made into a DLM(in which case the idbuild -M below
    * will fail -- remember that only DLMs can have verify routine, and
    * that verify routines are _optional_ and not _mandatory_ for drivers).  
    * Sooo, if not present in link kit:
    * a) idinstall it with idinstall -N since we don't want idresadd to
    *    create a new resmgr key(we do it ourselves)
    *    Also idinstall dlpi drivers if necessary
    * b) run /etc/conf/bin/idbuild -M driver_name (infrastructure_names)
    *    and ensure that succeeds
    * c) call our verify routine (which can still fail but hopefully not
    *    with ENOENT any more)
    * d) When done or if error idinstall -d to delete the driver
    *    (remember to only do this if modload fails with ENOENT!)
    */
  
   /* don't use modnew.d or the idcheck program; we want to know if 
    * modadm will work _now_.  
    * idcheck doesn't return information on if an entry exists for it in 
    * mod.d.  this is what we really want to know. 
    */
   snprintf(tmp,SIZE,"/etc/conf/mod.d/%s",driver_name);
   if (stat(tmp,&sb) == -1 || stat("/etc/conf/mod.d/dlpi",&sb) == -1) {
      mustidbuild=1;
   }

   snprintf(tmp,SIZE,"/etc/conf/bin/idcheck -p %s",driver_name);
   status=runcommand(0,tmp);
   /* 7 means no DSP in pack.d, no Master, and no System.  ignore the
    * last kernel built with driver and Driver.o part of DSP bits
    */
   if (status == -1) {
      error(NOTBCFG,"isaautodetect: problem running command '%s'",tmp);
      free(driver_name);
      return(-1);
   }
   if ((status & 7) == 0) {
      /* no part of this DSP exists in link kit.  idinstall -a driver */
      char *slash;

      strncpy(tmp,bcfgfile[bcfgindex].location,SIZE);

      GoInstallDriver(driver_name,tmp);
      installedMDI=1;
   }

   snprintf(tmp,SIZE,"/etc/conf/bin/idcheck -p dlpi");
   status=runcommand(0,tmp);
   if (status == -1) {
      error(NOTBCFG,"isaautodetect: problem running command '%s'",tmp);
      free(driver_name);
      return(-1);
   }
   if ((status & 7) == 0) {
      /* the dlpi module doesn't exist in link kit.  idinstall -a dlpi too */

      snprintf(tmp,SIZE,"%s/dlpi/DUMMY.bcfg",NDHIERARCHY);
      GoInstallDriver("dlpi",tmp);
      installedDLPI=1;
   }

   /* now rebuild module as a DLM.  This can fail if 
    * - driver doesn't have a 'L' flag in its Master file
    * - kernel is hosed
    * We must assume the first case
    */
   if ((installedDLPI == 1) || (installedMDI == 1) || (mustidbuild == 1)) {
      snprintf(cmdbuf,SIZE,"/etc/conf/bin/idbuild %s %s",
         installedDLPI ? "-M dlpi" : "", installedMDI ? drivermod : "");
      if (mustidbuild == 1 && installedDLPI == 0 && installedMDI == 0) {
         snprintf(cmdbuf,SIZE,"/etc/conf/bin/idbuild %s -M dlpi",drivermod);
      }
      status=runcommand(0,cmdbuf);
      if (status != 0) {
         /* don't call error here, since driver may simply not be a DLM */
         notice("isaautodetect: problem running '%s' - probably not a DLM",
                cmdbuf);
         goto fakesuccess;
      }
   }


   s_iowc=s_memwc=s_irqwc=s_dmawc="N";
   n_iowc=n_memwc=n_irqwc=n_dmawc=0;

   if (HasString(bcfgindex, N_ISAVERIFY, "WRITEIOADDR" , 0) == 1) {
      s_iowc="Y";
      n_iowc=1;
   }
   if (HasString(bcfgindex, N_ISAVERIFY, "WRITEIRQ"    , 0) == 1) {
      s_irqwc="Y"; 
      n_irqwc=1;
   }
   if (HasString(bcfgindex, N_ISAVERIFY, "WRITEMEMADDR", 0) == 1) {
      s_memwc="Y";
      n_memwc=1;
   }
   if (HasString(bcfgindex, N_ISAVERIFY, "WRITEDMA"    , 0) == 1) {
      s_dmawc="Y";
      n_dmawc=1;
   }

   if (mode == MDI_ISAVERIFY_GET) {
      status=OpenResmgr(O_RDWR);  /* WRITE for NextKey/writes, READ for reads */
      if (status) {
          error(NOTBCFG,"isaautodetect: OpenResmgr failed with %d(%s)",
                status,strerror(status));
          goto detectfail;
      }
      openedresmgr=1;
   
      /* now go into a big loop, adding the I/O address into the resmgr for the
       * driver, hoping it will say "yes, there's something there"
       */

      snprintf(verify_mode,20,"%d",MDI_ISAVERIFY_GET);

      strncpy(&mv.mv_modname[0],driver_name,MODMAXNAMELEN);
      for (loop=1; loop <= numport; loop++) {

         /* create a new key each time.  driver might scribble params all over 
          * key and we need to keep a clean slate
          */

         /* MDI_ISAVERIFY_GET: create new key */
         status=ResmgrNextKey(&rmkey,0);  /* no OpenResmgr/CloseResmgr */
         if (status != 0) {
            error(NOTBCFG,"isaautodetect: couldn't create new key:%d",status);
            goto detectfail;
         }
         mv.mv_key = rmkey;

         /* add BRDBUSTYPE at new key we just created if ISA */
         if (HasString(bcfgindex, N_BUS, "ISA", 0) == 1) {
            /* BRDBUSTYPE shouldn't be set since it's a new key but clear
             * just in case delete any of its values
             */
            char foo[10];
            RMbegin_trans(rmkey, RM_RDWR);
            MyRMdelvals(rmkey, CM_BRDBUSTYPE);
            RMend_trans(rmkey);

            RMbegin_trans(rmkey, RM_RDWR);
            snprintf(foo,10,"%d",CM_BUS_ISA);
            RMputvals_d(rmkey, CM_BRDBUSTYPE, foo, ' '); /* ' ' ok */
            RMend_trans(rmkey);
         }


         port=StringListWordX(bcfgindex,N_PORT,loop);
         if (port == NULL) {
            /* shouldn't happen since we call StringListNumWords above */
            error(NOTBCFG,"getisaparams: StringListWordX returned NULL");
            RMdelkey(rmkey);
            goto detectfail;
         }

                               /* no OpenResmgr/CloseResmgr*/
         status=RO_ISPARAMAVAILSKIPKEY(N_PORT, port, rmkey); 

         if ((status == 0) && (dangerousdetect > 0)) {
            /* if range is in use by ourself then skip it; we've already
             * configured the board earlier so no point in showing it to
             * the user again as something they can choose.
             */
            if (RO_ISPARAMTAKENBYDRIVER(N_PORT, port, driver_name) == 0) {
               /* parameter is taken in the resmgr but not taken by our driver 
                * so pretend this i/o address is available.  go call the 
                * verify routine on it.  This fully emulates a dos program which
                * walks through all possible addresses looking for the
                * card.  we normally try and be smarter and only do this 
                * when explictly told.
                */
               notice("dangerousisaautodetect: trying i/o %s even though "
                      "taken in resmgr", port);
               status=1;
            } else {
               notice("dangerousisaautodetect: skipping i/o %s: already "
                      "taken by %s driver!", port, driver_name);
            }
         }

         if (status == 1) {
            notice("isaautodetect: calling verify at i/o address %s", port);

            snprintf(tmp,SIZE,"%s,n",CM_ISAVERIFYMODE);
            RMbegin_trans(rmkey, RM_RDWR);
            MyRMdelvals(rmkey, tmp);
            RMend_trans(rmkey);

            RMbegin_trans(rmkey, RM_RDWR);
            RMputvals_d(rmkey, tmp, verify_mode, delim); /* not multi valued */
            RMend_trans(rmkey);

            RMbegin_trans(rmkey, RM_RDWR);
            MyRMdelvals(rmkey, CM_IOADDR);
            RMend_trans(rmkey);

            RMbegin_trans(rmkey, RM_RDWR);
            RMputvals_d(rmkey, CM_IOADDR, port, '-');   /* must use '-' */
            RMend_trans(rmkey);

            /* The modadm call below could conceiveably hang the system so 
             * be a little paranoid here.  Yeah, this will slow things down,
             * but we'd like a little history of what we tried to do and
             * what address caused the hang.  In particular, we want any and
             * all previous calls to notice() to be on the disk.
             */
            fflush((FILE *)NULL);      /* get out of stdio and into kernel */
            sync();                    /* prod kernel into writing bufs */
            if (logfilefp != NULL) {
               fsync(fileno(logfilefp));/* explictly put logfile on disk */
            }

            /* now call verify routine.  Even though we normally rule
             * out io addresses which are known to be taken, there
             * could still be hardware lurking in the i/o address space
             * that hasn't been configured yet with a driver and
             * doesn't like to be poked the way this driver is about
             * to poke it.  So this isn't 100% safe, but the best we can do.
             */
            errno=0;
            status=modadm(MOD_TY_CDEV, MOD_C_VERIFY, &mv);
            /* if status >= 0 then we also know we succeeded */
            if (errno == 0) {
               /* success - we have hardware here */
               if (numfound >= MAXAUTODETECT) {
                  notice("isaautodetect: skipping found port %s-no room",port);
                  free(port);
                  RMdelkey(rmkey);
                  continue;
               }
               strncpy(found[numfound].io,port,20);
               /* now see what other parameters were set.  If not set in
                * resmgr then we assume that the parameter cannot be 
                * read from the firmware.
                * Specifically, we must read:
                * IRQ, MEMADDR, DMA, and any custom parameters.
                * we use ,n and ,r because of how they were added to the resmgr
                * we use CM_ISAVERIFYPREFIX so we can immediately turn around
                * and call ISPARAMAVAIL without this entry in the resmgr
                * skewing the results of ISPARAMAVAIL since it looks for
                * CM_IRQ, DM_MEMADDR, and CM_DMA directly without the leading
                * underscore.
                * I also removed the CM_ since if the value of CM_IRQ and 
                * friends ever changes then this will break mdi_AT_verify in
                * the kernel.  So we look for "_IRQ", "_MEMADDR", and "_DMA"
                * now.
                */

               snprintf(tmp,SIZE,"%s%s,n",CM_ISAVERIFYPREFIX, "IRQ");
               RMbegin_trans(rmkey, RM_READ);
               RMgetvals(rmkey,tmp,0,irq,VB_SIZE);
               RMend_trans(rmkey);
               if (irq[0] == '-') {
                  strncpy(found[numfound].irq,"__SKIP__",20);
                  strncpy(found[numfound].irqconflict,"?",20);
               } else {
                  /* is this irq valid in the bcfg file ? */
                  strtolower(irq);  /* just in case */
                  if (HasString(bcfgindex, N_INT, irq, 0) == 1) {
                     strncpy(found[numfound].irq,irq,20);
                     /* since this is only for ISA cards we don't have to 
                      * worry about ITYPE sharing here -- existence of 
                      * driver in resmgr is sufficient
                      */
                     if (RO_ISPARAMAVAILSKIPKEY(N_INT, irq, rmkey)) {  
                        /*no Open/CloseResmgr*/
                        strncpy(found[numfound].irqconflict,"N",20);
                     } else {
                        strncpy(found[numfound].irqconflict,"Y",20);
                     }
                  } else {
                     notice("isaautodetect: ignoring IRQ %s as not in bcfg %d",
                            irq,bcfgindex);
                     free(port);
                     RMdelkey(rmkey);
                     continue;
                  }
               }

               snprintf(tmp,SIZE,"%s%s,r",CM_ISAVERIFYPREFIX, "MEMADDR");
               RMbegin_trans(rmkey, RM_READ);
               RMgetvals(rmkey,tmp,0,memaddr,VB_SIZE);
               RMend_trans(rmkey);
               if (memaddr[0] == '-') {
                  strncpy(found[numfound].mem,"__SKIP__",20);
                  strncpy(found[numfound].memconflict,"?",20);
               } else {
                  /* RMgetvals will fill in as "X Y".  We want "X-Y" for
                   * later call to idinstall and idmodify as well as 
                   * call to ISPARAMAVAIL which also wants "X-Y"
                   */
                  spacep=strstr(memaddr," ");
                  if (spacep == NULL) {
                     error(NOTBCFG,"RMgetvals didn't add space!");
                     free(port);
                     RMdelkey(rmkey);
                     goto detectfail;
                  }
                  *spacep='-';
                  /* does this memaddr range exist in the bcfg file? */
                  strtolower(memaddr);   /* just in case */
                  if (HasString(bcfgindex, N_MEM, memaddr, 0) == 1) {
                     strncpy(found[numfound].mem,memaddr,20);
                     if (RO_ISPARAMAVAILSKIPKEY(N_MEM, memaddr, rmkey)) {
                        strncpy(found[numfound].memconflict,"N",20);
                     } else {
                        strncpy(found[numfound].memconflict,"Y",20);
                     }
                  } else {
                     notice("isaautodetect: ignoring MEMADDR %s as not "
                            "in bcfg %d",memaddr,bcfgindex);
                     free(port);
                     RMdelkey(rmkey);
                     continue;
                  }
               }
            
               snprintf(tmp,SIZE,"%s%s,n",CM_ISAVERIFYPREFIX, "DMA");
               RMbegin_trans(rmkey, RM_READ);
               RMgetvals(rmkey,tmp,0,dma,VB_SIZE);
               RMend_trans(rmkey);
               if (dma[0] == '-') {
                  strncpy(found[numfound].dma,"__SKIP__",20);
                  strncpy(found[numfound].dmaconflict,"?",20);
               } else {
                  /* is this irq valid in the bcfg file ? */
                  strtolower(dma);  /* just in case */
                  if (HasString(bcfgindex, N_DMA, dma, 0) == 1) {
                     strncpy(found[numfound].dma,dma,20);
                     if (RO_ISPARAMAVAILSKIPKEY(N_DMA, dma, rmkey)) {
                        strncpy(found[numfound].dmaconflict,"N",20);
                     } else {
                        strncpy(found[numfound].dmaconflict,"Y",20);
                     }
                  } else {
                     notice("isaautodetect: ignoring DMA %s as not "
                            "in bcfg %d",dma,bcfgindex);
                     free(port);
                     RMdelkey(rmkey);
                     continue;
                  }
               }

               /* now pull out as many custom parameters as are set in resmgr */
               for (customloop=1; customloop <= numcustom; customloop++) {
                  char x[VB_SIZE];
                  snprintf(tmp,SIZE,"%s,s",&customname[customloop][0]);
                  RMbegin_trans(rmkey, RM_READ);
                  RMgetvals(rmkey,tmp,0,x,VB_SIZE);
                  RMend_trans(rmkey);

                  /* was this parameter set in the resmgr? */
                  if (strcmp(x,"-") == 0) {
                     strncpy(&found[numfound].customvalue[customloop][0],
                             "__SKIP__", VB_SIZE);
                  } else {
                     strncpy(&found[numfound].customvalue[customloop][0],x,
                         VB_SIZE);
                  }
               }

               /* Lastly, increment number of custom parameters we found */
               numfound++;
            } else
            if (errno == EPERM) {
               /* shouldn't happen since we look for P_LOADMOD above */
               error(NOTBCFG,"couldn't call verify - EPERM");
               free(port);  /* malloced in StringListWordX */
               RMdelkey(rmkey);
               goto detectfail;
            } else
            if (errno == ENOSYS) {
               /* driver doesn't have a verify routine; stop here */
               notice("isaautodetect: driver doesn't have a verify routine");
               notice("isaautodetect: stopping search, faking success");
               free(port);  /* malloced in StringListWordX */
               RMdelkey(rmkey);
               goto fakesuccess;
            } else
            if (errno == ENOENT) {
               /* driver/module with name provided does not exist.  
                * the dcu runs /etc/conf/bin/idbuild -M driver_name and
                * tries again.  The -c is passed from idbuild to idmkunix and 
                * is used to suppress mdep checks.  That is, most of the fields
                * in System (notably starting I/O and ending I/O) are not
                * checked for validity or conflicts.  Necessary because driver 
                * doesn't "really" exist yet in the kernel.
                * Unknown: Is this what we get if driver was in link kit to 
                * begin with but isn't a DLM?
                * We can also get here if driver is already in System but
                * turned off with a 'N' so it's not included in kernel relink
                * also quite likely that driver _does_ exist in link kit
                * but has never been made into a dlm with idbuild -M
                * Generally you'll see the error if you run ndcfg -vjl /logfile
                * and look at the idbuild -M output -- maybe idmodreg failure
                */
               notice("isaautodetect: modadm failed with ENOENT");
               notice("isaautodetect: stopping search, faking success");
               free(port);  /* malloced in StringListWordX */
               RMdelkey(rmkey);
               goto fakesuccess;
            } 
            /* remember that drivers can fail their verify routine with
             * arbitrary values if that io address doesn't have anything
             * there.  Convention is to fail with ENODEV, but that's not
             * particularly enforced.  So we keep trying for any other
             * errno value, assuming that it's just the driver saying
             * "nope, nothing here"
             */
         } else {  /* if ISA port is available */
            notice("isaautodetect: skipping verify at i/o address %s", port);
         }
         free(port);  /* malloced in StringListWordX */
         RMdelkey(rmkey);
      }
      rmkey=RM_KEY;
   } else if (mode == MDI_ISAVERIFY_SET) {
      /*
       * if called from idinstall/idmodify, then do the following:
       * - create a new resmgr key(since in idinstall existing key uses the new
       *   address that the driver will end up using but the verify routine
       *   won't be able to find the card at that address yet)
       * - copy all custom parameters supplied as arguments into that key.
       * - if OLDIOADDR argument present, add it as IOADDR in resmgr
       * - if OLDIOADDR argument _not_ present and IOADDR
       *   this argument is optional with idinstall but required with idmodify
       * 
       * if called from cmdparser then convert 
       * element argument to resmgr key using resshowkey.  while idmodify
       * takes a element argument it converts to bcfgindex to make its call
       * to isaautodetect appear to be from idinstall
       *
       * For both of the above, do the following at the resmgr key:
       * - delete all of following parameters from resmgr at that key:
       *   _IOADDR, _DMA, _MEMADDR, _IRQ
       * - if IOADDR argument present, add it as _IOADDR in resmgr 
       * - if IRQ argument present, add as _IRQ in resmgr
       * - if DMAC argument present, add as _DMA in resmgr
       * - if MEMADDR argument present, add as _MEMADDR in resmgr
       * - add PUTCONFKEY to the key we created, and the value will be the
       *   original key.  This is necessary if the driver calls cm_AT_putconf
       *   since it will have to modify the original key and not the one
       *   we create and call the verify routine with!
       * - call modadm to invoke driver's verify routine.  We don't
       *   care if it succeeds or fails, as driver may not even _have_ a
       *   verify routine.
       * if called from idinstall/idmodify, then do the following:
       * - remove resmgr key we created
       * - don't call StartList
       */
      status=OpenResmgr(O_RDWR);  /* WRITE for NextKey/writes, READ for reads */
      if (status) {
          error(NOTBCFG,"isaautodetect: OpenResmgr failed with %d(%s)",
                status,strerror(status));
          goto detectfail;
      }
      openedresmgr=1;
   
      strncpy(&mv.mv_modname[0],driver_name,MODMAXNAMELEN);
      if (installkey != RM_KEY) {  /* if from idinstall or idmodify command */
         /* if no i/o address supplied as argument (i.e. purely memory mapped
          * board) to idinstall, fake success since the dcu implementation
          * of verify routine requires i/o address(although that's stupid)
          * However, if we're called from idmodify, they don't have to
          * supply IOADDR= or OLDIOADDR= as arguments if they're only 
          * changing IRQ, so we must obtain the ioaddr from the existing key.
          * the idea is that if we're from idinstall (not idmodify) and
          * we get here and IOADDR wasn't given as an argument then the
          * board is purely memory mapped.
          */
         if (FindStringListText(slp, "IOADDR=") == NULL) {
            if (fromidmodify == 0) {
               notice("isaautodetect: no IOADDR argument, faking success");
               goto fakesuccess;
            } else {
               /* do nothing; not an error: idmodify doesn't require IOADDR */
            }
         }

         if (fromidmodify == 1) {
            /* must use IOADDR at existing key provided at argument
             *  "installkey" to populate new key we just created 
             * so that the verify routine can find the board at its
             * original address
             */
            fakeioaddr++;
         }

         /* MDI_ISAVERIFY_SET: create new key */
         status=ResmgrNextKey(&rmkey,0);  /* no OpenResmgr/CloseResmgr */
         if (status != 0) {
            error(NOTBCFG,"isaautodetect: couldn't create new key:%d",status);
            goto detectfail;
         }
         mv.mv_key = rmkey;

         /* add BRDBUSTYPE at new key we just created if ISA */
         if (HasString(bcfgindex, N_BUS, "ISA", 0) == 1) {
            /* BRDBUSTYPE shouldn't be set since it's a new key but clear
             * just in case delete any of its values
             */
            char foo[10];
            RMbegin_trans(rmkey, RM_RDWR);
            MyRMdelvals(rmkey, CM_BRDBUSTYPE);
            RMend_trans(rmkey);

            RMbegin_trans(rmkey, RM_RDWR);
            snprintf(foo,10,"%d",CM_BUS_ISA);
            RMputvals_d(rmkey, CM_BRDBUSTYPE, foo, ' '); /* ' ' ok */
            RMend_trans(rmkey);
         }


         /* use OLDIOADDR if supplied or IOADDR if not supplied for CM_IOADDR 
          * parameter in the resmgr.  We also known that OLDIOADDR is not
          * a valid option for the idmodify command (we check this prior
          * to calling isaautodetect in idmodify)
          */
         if ((fakeioaddr == 0) &&
             ((slpport=FindStringListText(slp, "OLDIOADDR=")) != NULL)) {
            strtolower(slpport);
            if (HasString(bcfgindex, N_PORT, slpport, 0) == -1) {
               error(NOTBCFG,"OLDIOADDR= supplied but bcfg doesn't use PORT=!");
               goto detectfail;
            }
            if (HasString(bcfgindex, N_PORT, slpport,0) == 0) {
               error(NOTBCFG,"OLDIOADDR=%s not valid for bcfg file range",
                     slpport);
               goto detectfail;
            }
            RMbegin_trans(rmkey, RM_RDWR);
            MyRMdelvals(rmkey, CM_IOADDR);
            RMend_trans(rmkey);

            snprintf(tmp,SIZE,"%s,r",CM_IOADDR);
            RMbegin_trans(rmkey, RM_RDWR);
            RMputvals_d(rmkey, tmp, slpport, '-'); /* must use '-' */
            RMend_trans(rmkey);
         } else {
            /* idinstall: we know that the user supplied an IOADDR= to get 
             * to this point or 
             * idmodify: fakeioaddr is set indicating we came 
             * from idmodify where we don't need IOADDR=.
             */
            if (fakeioaddr == 0) {
               /* idinstall:  IOADDR will be always be present
                */
               slpport=FindStringListText(slp, "IOADDR=");
               strtolower(slpport);
               RMbegin_trans(rmkey, RM_RDWR);
               MyRMdelvals(rmkey, CM_IOADDR);
               RMend_trans(rmkey);
           
               snprintf(tmp,SIZE,"%s,r",CM_IOADDR);
               RMbegin_trans(rmkey, RM_RDWR); 
               RMputvals_d(rmkey, tmp, slpport, '-'); /* must use '-' */
               RMend_trans(rmkey);
            } else {
               char installkeyioaddr[VB_SIZE];
               /* idmodify and IOADDR may or may not be found as an argument.
                * Not that it matters, as in both cases we must 
                * read IOADDR at key 'installkey' and write to key rmkey so
                * that verify routine will know what board we wish to
                * manipulate
                */ 
               RMbegin_trans(installkey, RM_READ);
               RMgetvals(installkey, CM_IOADDR, 0, installkeyioaddr, VB_SIZE);
               RMend_trans(installkey);

               /* since we just got it from RMgetvals the range is 
                * delimited with a space so must use space with RMputvals_d
                */
               RMbegin_trans(rmkey, RM_RDWR);
               RMputvals_d(rmkey, CM_IOADDR, installkeyioaddr, ' ');/* ' ' ok */
               RMend_trans(rmkey);
            }
         }

         /* ok, we have CM_IOADDR set to the correct value so that
          * the driver's verify routine can find the card at its existing
          * I/O address
          * idmodify:  custom parameters already exist in resmgr and have
          *            already been changed to their updated value
          * idinstall: custom parameters already exist in resmgr 
          * In both cases it's save to read custom parameter info from 
          * key 'installkey'
          */

         /* lastly, add all of the custom parameters at the new key */
         for (loop=1; loop <= numcustom; loop++) {
            snprintf(tmp,SIZE,"%s=",&customname[loop][0]);
            slpcustom=FindStringListText(slp, tmp);
            /* if you're in idmodify you don't have to specify all custom
             * parameters as arguments -- only what you want to change
             */
            if (slpcustom == NULL) {
               if (fromidmodify == 0) {
                  error(NOTBCFG,"isaautodetect: custom parameter '%s' not "
                        "supplied as a argument",tmp);
                  goto detectfail;
               } else {
                  /* idmodify case: must pull custom parameter from key 
                   * 'installkey' and save at key 'rmkey'
                   * we call isaautodetect in idmodify _after_ changing
                   * the custom parameters in the resmgr there at the original
                   * (and backup) keys, so it's safe to read the updated
                   * value from 'installkey'.
                   */
                  char x[VB_SIZE];
                  char customtmp[VB_SIZE];

                  snprintf(x,VB_SIZE,"%s,s",&customname[loop][0]);
                  RMbegin_trans(installkey, RM_READ);
                  RMgetvals(installkey, x, 0, customtmp, VB_SIZE);
                  RMend_trans(installkey);
 
                  RMbegin_trans(rmkey, RM_RDWR);
                  MyRMdelvals(rmkey, x);
                  RMend_trans(rmkey);

                  RMbegin_trans(rmkey, RM_RDWR);
                  /* assumes that delim has been set to SOH  \01 here */
                  RMputvals_d(rmkey, x, customtmp, delim); /*allow spaces here*/
                  RMend_trans(rmkey);
               }
            } else {
               snprintf(tmp,SIZE,"%s,s",&customname[loop][0]);
               RMbegin_trans(rmkey, RM_RDWR);
               MyRMdelvals(rmkey, tmp);
               RMend_trans(rmkey);
            
               RMbegin_trans(rmkey, RM_RDWR);
               /* assumes that delim has been set to SOH  \01 here */
               RMputvals_d(rmkey, tmp, slpcustom, delim); /* allow spaces here*/
               RMend_trans(rmkey);
            }
         }
         /* all custom parameters are saved at key rmkey now */
      } else {
         /* we're being called from cmdparser.y, use rmkey from resshowkey */
         mv.mv_key = rmkey;

         /* write custom parameters to resmgr at existing key if they are 
          * also provided as argument on command line, emulating idmodify
          */
         for (loop=1; loop <= numcustom; loop++) {
            snprintf(tmp,SIZE,"%s=",&customname[loop][0]);
            slpcustom=FindStringListText(slp, tmp);
            if (slpcustom != NULL) {
               char *x;
               snprintf(tmp,SIZE,"%s%s=",&customname[loop][0],CM_CUSTOM_CHOICE);
               x=FindStringListText(slp, tmp);
               if (x == NULL) {
                  error(NOTBCFG,"custom parameter specified without matching "
                        "custom%s parameter",CM_CUSTOM_CHOICE);
                  goto detectfail;
               }
               /* this would be a nice future enhancement but no time today */
               notice("custom parameters not handled yet with isaautodetect "
                      " set <element> FOO=bar -- use idmodify instead");
               continue;
            }
         }
      }

      /* take the IOADDR, DMA, IRQ, MEMADDR if supplied as arguments and
       * put them into the resmgr under their new name starting with
       * an underscore for mdi_AT_verify to read.
       */
      snprintf(tmp,SIZE,"%s%s,r",CM_ISAVERIFYPREFIX, "IOADDR");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);
      /* If we're called from idmodify then it's likely that IOADDR won't
       * be found as an argument.  That's ok -- we have already put CM_IOADDR
       * in the resmgr to pacify the verify routine.  All this means is
       * that mdi_AT_verify will set sioa and eioa to NULL
       */
      if ((slpport=FindStringListText(slp, "IOADDR=")) != NULL) {
         strtolower(slpport);
         RMbegin_trans(rmkey, RM_RDWR);
         RMputvals_d(rmkey,tmp,slpport,'-');  /* must use '-' */
         RMend_trans(rmkey);
      }

      snprintf(tmp,SIZE,"%s%s,r",CM_ISAVERIFYPREFIX, "MEMADDR");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);
      if ((slpmem=FindStringListText(slp, "MEMADDR=")) != NULL) {
         strtolower(slpmem);
         RMbegin_trans(rmkey, RM_RDWR);
         RMputvals_d(rmkey,tmp,slpmem,'-');  /* must use '-' */
         RMend_trans(rmkey);
      }

      snprintf(tmp,SIZE,"%s%s,n",CM_ISAVERIFYPREFIX, "DMA");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);
      if ((slpdma=FindStringListText(slp, "DMAC=")) != NULL) {
         strtolower(slpdma);
         RMbegin_trans(rmkey, RM_RDWR);
         RMputvals_d(rmkey,tmp,slpdma,delim);  /* not multi valued/range */
         RMend_trans(rmkey);
      }

      snprintf(tmp,SIZE,"%s%s,n",CM_ISAVERIFYPREFIX, "IRQ");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);
      if ((slpirq=FindStringListText(slp, "IRQ=")) != NULL) {
         strtolower(slpirq);
         RMbegin_trans(rmkey, RM_RDWR);
         RMputvals_d(rmkey,tmp,slpirq,delim);  /* not multivalued/range */
         RMend_trans(rmkey);
      }

      snprintf(verify_mode,20,"%d",MDI_ISAVERIFY_SET);
      snprintf(tmp,SIZE,"%s,n",CM_ISAVERIFYMODE);
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      RMbegin_trans(rmkey, RM_RDWR);
      RMputvals_d(rmkey, tmp, verify_mode, delim);  /* not multivalued/range*/
      RMend_trans(rmkey);

      /* Lastly, add PUTCONFKEY to resmgr.  If the driver will need to call
       * cm_AT_putconf this is the actual key it should pass as the argument
       * to that routine.  We must do this since we must create a new key
       * and call verify routine with that key instead of the "normal"
       * key that would be passed to the driver, so if the driver calls
       * cm_AT_putconf on the key passed to the verify routine as an argument
       * the changes will be lost if we delete the key in a few lines from
       * now(i.e. if we're called from idinstall or idmodify not from 
       * cmdparser.y isaautodetect SET command)
       */
      snprintf(putconfkeystr,20,"%d",putconfkey);
      snprintf(tmp,SIZE,"PUTCONFKEY,n");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      RMbegin_trans(rmkey, RM_RDWR);
      RMputvals_d(rmkey, tmp, putconfkeystr, delim); /* not multivalued/range */
      RMend_trans(rmkey);

      /* ok, we're all set.  We have set the following parameters in the
       * resmgr in preparation to call the driver's verify routine:
       * - CM_IOADDR
       * - all custom parameters
       * - _IOADDR _DMA _IRQ, _MEMADDR for mdi_AT_verify
       * so now go ahead and call the verify routine.   We don't care
       * if it succeeds or fails as driver may not even have a set routine
       *
       * It's imperative that there we have done an RMend_trans on rmkey
       * as the driver _may_ correctly and legimately issue a begin_trans 
       * on the key.
       */
      status=modadm(MOD_TY_CDEV, MOD_C_VERIFY, &mv);

      /* Lastly, clean up after ourselves */
      snprintf(tmp,SIZE,"%s%s,r",CM_ISAVERIFYPREFIX, "IOADDR");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      snprintf(tmp,SIZE,"%s%s,r",CM_ISAVERIFYPREFIX, "MEMADDR");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      snprintf(tmp,SIZE,"%s%s,n",CM_ISAVERIFYPREFIX, "DMA");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      snprintf(tmp,SIZE,"%s%s,n",CM_ISAVERIFYPREFIX, "IRQ");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      snprintf(tmp,SIZE,"%s,n",CM_ISAVERIFYMODE);
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      snprintf(tmp,SIZE,"PUTCONFKEY,n");
      RMbegin_trans(rmkey, RM_RDWR);
      MyRMdelvals(rmkey, tmp);
      RMend_trans(rmkey);

      if (installkey != RM_KEY) {  /* if we're called from idinstall command */
         /* remove the key we just created */
         /* RMbegin_trans(rmkey, RM_RDWR); not necessary */
         RMdelkey(rmkey);
         /* RMend_trans(rmkey); not necessary */
      }
   }

fakesuccess:
   if (mode == MDI_ISAVERIFY_GET) {
      /* the "WC" means "writeback-capable" and indicates if we can write
       * the parameter back to the firmware or if it's read-only.
       * the "CON" means "conflict" and indicates if this parameter is 
       * known to conflict with something that already exists in the resmgr
       * NOTE: Assumes compiler can handle 43 arguments to a function
       *       (32 is the minimum but a compiler wouldn't have to accept 43
       *       to pass standards tests -- thankfully the Gemini compiler does)
       * NOTE: We already mandate that custom parameters cannot start with
       *       an underscore.  so we start all of the usual ISA parameters
       *       with an underscore to make it easy for netcfg to distingush what 
       *       parameters are ours and which are custom parameters.
       * NOTE: we push all custom paramters on the stack as arguments but
       *       the actual number of them is in numcustom, so StartList 
       *       won't know that we pushed too many arguments on the stack
       */
      StartList(24+(numcustom*2), "_IOADDR",10,"_IOWC" ,7,"_IOCON" ,8,
                                 "_MEMADDR",12,"_MEMWC",7,"_MEMCON",8,
                                 "_IRQ",5 ,"_IRQWC",7,"_IRQCON",8,
                                 "_DMA",5 ,"_DMAWC",7,"_DMACON",8,
                                 &customname[1][0],20,
                                 &customname[2][0],20,
                                 &customname[3][0],20,
                                 &customname[4][0],20,
                                 &customname[5][0],20,
                                 &customname[6][0],20,
                                 &customname[7][0],20,
                                 &customname[8][0],20,
                                 &customname[9][0],20);
      for (loop=0; loop<numfound; loop++) {
         strtoupper(found[loop].io);
         strtoupper(found[loop].mem);
         strtoupper(found[loop].irq);
         strtoupper(found[loop].dma);
         AddToList(12+numcustom,found[loop].io, s_iowc, "N",
                      found[loop].mem, s_memwc, found[loop].memconflict,
                      found[loop].irq, s_irqwc, found[loop].irqconflict,
                      found[loop].dma, s_dmawc, found[loop].dmaconflict,
                      &found[loop].customvalue[1][0],
                      &found[loop].customvalue[2][0],
                      &found[loop].customvalue[3][0],
                      &found[loop].customvalue[4][0],
                      &found[loop].customvalue[5][0],
                      &found[loop].customvalue[6][0],
                      &found[loop].customvalue[7][0],
                      &found[loop].customvalue[8][0],
                      &found[loop].customvalue[9][0]);
      }
      if (numfound == 0) {
         if (tclmode) {
            strcat(ListBuffer,"{ }");  /* so EndList prints something */
         } else {
            error(NOTBCFG,"isaautodetect: no boards found");
         }
      }
      EndList();
   } else if (mode == MDI_ISAVERIFY_SET) {
      /* if we're not called from idinstall command then it's ok to call
       * StartList  otherwise, if we were called from idinstall, then
       * idinstall will call StartList on its own
       */
      if (installkey == RM_KEY) {
         /* we're called from cmdparser.y */
         StartList(2,"STATUS",20);
         AddToList(1,"success");
         EndList();
      }
   }

   if (openedresmgr) (void) CloseResmgr();

   /* if we had to install dlpi in the link kit then remove it */
   if (installedDLPI) {
      snprintf(cmdbuf,SIZE,"/etc/conf/bin/idinstall -P nics -d -N dlpi");
      status=runcommand(0,cmdbuf);
   }

   /* if we had to install the MDI driver in the link kit then remove it too */
   if (installedMDI) {
      /* driver_name is always set if installedMDI to non zero */
      snprintf(cmdbuf,SIZE,"/etc/conf/bin/idinstall -P nics -d -N %s",
               driver_name);
      status=runcommand(0,cmdbuf);
   }

   if (driver_name != NULL) free(driver_name);
   chdir("/");

   delim=olddelim;

   return(0);
   /* NOTREACHED */

detectfail:
   if (rmkey != RM_KEY) RMdelkey(rmkey);
   if (openedresmgr) (void) CloseResmgr();
   if (port != NULL) free(port);
   if (installedDLPI) {
      snprintf(cmdbuf,SIZE,"/etc/conf/bin/idinstall -P nics -d -N dlpi");
      status=runcommand(0,cmdbuf); /* will fail if not there hence runcommand */
   }
   if (installedMDI) {
      snprintf(cmdbuf,SIZE,"/etc/conf/bin/idinstall -P nics -d -N %s",
               driver_name);
      status=runcommand(0,cmdbuf); /* will fail if not there hence runcommand */
   }
   if (driver_name != NULL) free(driver_name);
   chdir("/");

   delim=olddelim;

   return(-1);
}

/* is the driver still required by anybody else? 
 * That is, does the NIC_DEPEND indicate that the given network card
 * dependency driver is still needed by anybody else?
 * (we can do this because we mandate that all DEPEND drivers reside
 * in HIERARCHY or NDHIERARCHY so we know we're not blindly deleting
 * other people's drivers unless they alter NIC_DEPEND in the resmgr)
 * returns 0 if not found, indicating not needed
 * returns 1 if still needed
 * returns -1 if error
 */
int
DependStillNeeded(char *driver)
{
   rm_key_t rmkey;
   int status,NKstatus,ec;
   char *cp;
   char modname[VB_SIZE];
   char nic_depend[VB_SIZE];
   char element[VB_SIZE];
   char tmp[SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
       error(NOTBCFG,"DependStillNeeded: OpenResmgr failed with %d(%s)",
             status,strerror(status));
       return(-1);
   }

   rmkey = NULL;

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_MODNAME,0,modname,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"DependStillNeeded: RMgetvals for MODNAME "
               "returned %d", ec);
         goto dependfail;
      }
      RMend_trans(rmkey);

      if ((strcmp(modname,"-") == 0) ||
          (strcmp(modname,"unused") == 0)) {
         continue;
      }

      snprintf(tmp,SIZE,"%s,s",CM_NETCFG_ELEMENT);
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,tmp,0,element,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"DependStillNeeded: RMgetvals for NETCFG_ELEMENT "
               "returned %d", ec);
         goto dependfail;
      }
      RMend_trans(rmkey);

      if (strcmp(element,"-") == 0) {
         continue;
      }

      /* ok, at this point we're reasonably sure this resmgr key is for
       * a network board.
       */

      snprintf(tmp,SIZE,"%s,s",CM_NIC_DEPEND);
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,tmp,0,nic_depend,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"DependStillNeeded: RMgetvals for NIC_DEPEND "
               "returned %d", ec);
         goto dependfail;
      }
      RMend_trans(rmkey);

      /* we want to avoid substring matches (like ISA inside of EISA)
       * so we added CM_NIC_DEPEND to the resmgr with a trailing space
       * so we can call strstr here
       */
      snprintf(tmp,SIZE,"%s ",driver);
      if ((cp=strstr(nic_depend, tmp)) == NULL) {
         continue;
      }
    
      /* to get here means that we found a match, meaning this driver
       * is still required by someone else.  No point in searching further.
       */
      CloseResmgr();
      return(1);
      /* NOTREACHED */
   }

   /* we've gone through the entire resmgr without finding a match.  indicate
    * that this driver isn't needed any more 
    */
   CloseResmgr();
   return(0);
   /* NOTREACHED */

dependfail:
   CloseResmgr();
   return(-1);
}

/* MDIPRINTCFG=0 means be quiet: kernel messages should not go to the console
 *                               (although sending output to putbuf is ok)
 * MDIPRINTCFG=1 means ok for mdi_printcfg(D3) to call cmn_err to write
 * to console and putbuf
 * 
 * The goal here is to tell the kernel mdi_printcfg routine that it is
 * or is not ok for it to print its messages to the console.  The kernel
 * should assume it's ok unless told otherwise and if a kernel panic occurs
 * then the kernel should automatically revert to "ok to print" mode.
 *
 * doesn't matter if backups are enabled, resput won't write parameters stored
 * to resmgr key RM_KEY to the backup key too
 * 
 * NOTE1:
 * As of BL15, all drivers (even $interface base) can't do a cm_getval with 
 * cm_key = RM_KEY any more even though user apps can still write to key RM_KEY.
 * Pity that mdi_printcfg(D3mdi) doesn't take a resmgr key as an argument or
 * we'd have an easy solution: just write MDIPRINTCFG to the specific resmgr key
 *
 * Instead, we could use a known key which will always be there.  As of BL15,
 * we could use the copy protection key, key #2.  This has the unfortunate side 
 * effect that RM_KEY doesn't have in that an idconfupdate will write it out
 * to /stand/resmgr for the next boot and if the machine panics
 * we won't have the opportunity to clear the parameter from its original
 * value (unless we use the routine CheckResmgrAndDevs from /etc/rc2.d/S15nd).
 * Not a very elegant solution.
 * 
 * A better solution is to create a non-persistant key whose name is ndcfg
 * and emulate cm_newkey by writing CM_VOLATILE.  The mere presence of 
 * CM_VOLATILE indicates to idconfupdate that this key should *not* be written 
 * to /stand/resmgr.  We then write the MDIPRINTCFG parameter to this key,
 * although there may be issues if drivers call mdi_printcfg from interrupt
 * context and we need to do a cm_getval to read MDIPRINTCFG. We would
 * also need to call cm_getbrdkey("ndcfg") in dlpi's load routine and save
 * that to a global rm_key_t.
 *
 * A much better solution is to skip the resmgr altogether and patch a symbol 
 * in the dlpi driver via /dev/kmem (see donlist() code in cmdops.c) but 
 * it isn't known if the dlpi module will always be loaded when this 
 * routine is called(although it's easy enough to do a modadmin -l dlpi 
 * beforehand).  Also check priviledges to see if we always have sufficient 
 * privs to patch kmem here.  This is the better solution long-term.
 * 
 * NOTE2:
 * No matter what solution we choose, there's still a race condition if 
 * multiple ndcfg's are running and both do an idinstall at the same time
 * since both will be altering the resource that mdi_printcfg in the kernel
 * will read
 *
 * NOTE3: 
 * netcfg passes in __CHARM=0 or __CHARM=1 as arguments to idinstall
 * which indicates if we're running netcfg in graphics mode or in charm mode.  
 * But mdi_printcfg messages only appear on the system *console*, whatever
 * that might be(serial or otherwise).  So we really only need to turn 
 * messages off if we are running netcfg on /dev/console AND __CHARM=1.
 * In any other case messages on the console are deemed acceptable, even if 
 * the console is doing other things at the time.  What's the point of
 * turning off messages on the console if we're running netcfg in CHARM
 * on some other serial tty???
 */
void
mdi_printcfg(int onoff)
{
   char tmp[SIZE];

   snprintf(tmp, SIZE, "MDIPRINTCFG,n=%d", onoff);
   /* see above comment about the RM_KEY in next line */
   if (resput("RM_KEY", tmp, 0) != 0) {
      error(NOTBCFG,"mdi_printcfg: resput failed");
   }
   return;
}

/* return scope (BOARD, DRIVER, NETX, GLOBAL, PATCH) for given parameter
 */
int
GetParamScope(rm_key_t key, char *customX, char *scope)
{
   int status;
   char tmp[SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
       error(NOTBCFG,"GetParamScope: OpenResmgr failed with %d(%s)",
             status,strerror(status));
       return(-1);
   }

   snprintf(tmp,SIZE,"%s%s,s",CM_CUSTOM_CHOICE,customX);
   RMbegin_trans(key, RM_READ);
   status=RMgetvals(key, tmp, 0, scope, VB_SIZE);
   if (status) {
      RMabort_trans(key);
      error(NOTBCFG, "GetParamScope: RMgetvals for %s returned %d",tmp,status);
      (void)CloseResmgr();
      return(status);
   }
   RMend_trans(key);
   (void)CloseResmgr();
   return(0);
}

int
orphans(void)
{
   error(NOTBCFG,"orphans: not implemented yet");
   return(-1);
}

/* go through the resmgr and print out the resmgr key of the entry
 * that went through netinstall.  used by nics postinstall script
 * instead of relying on the NICS_KEY variable from the ifile which
 * is wrong, as the resmgr can (and did!) get reshuffled across a reboot
 */
int
iicard(void)
{
   rm_key_t rmkey,foundkey;
   int status,NKstatus,ec;
   char tmp[VB_SIZE];
   char iicardstr[SIZE];

   foundkey = RM_KEY;
   snprintf(iicardstr,SIZE,"%s,s",CM_IICARD);

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG,"iicard: OpenResmgr failed with %d(%s)",
             status,strerror(status));

      snprintf(tmp,VB_SIZE,"%d",foundkey);
      StartList(2,"KEY",10);
      AddToList(1,tmp);
      EndList();

      return(-1);
   }

   rmkey = NULL;

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,iicardstr,0,tmp,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"iicard: RMgetvals for IICARD returned %d", ec);
         goto iicardfail;
      }
      RMend_trans(rmkey);

      if (tmp[0] == '-') continue;

      if (tmp[0] != '1') continue;
      
      foundkey=rmkey;
      break;
   }
   (void)CloseResmgr();

   snprintf(tmp,VB_SIZE,"%d",foundkey);
   StartList(2,"KEY",10);
   AddToList(1,tmp);
   EndList();

   return(0);

iicardfail:
   (void)CloseResmgr();
   snprintf(tmp,VB_SIZE,"%d",foundkey);
   StartList(2,"KEY",10);
   AddToList(1,tmp);
   EndList();
   return(-1);
}

int
ResmgrEnsureGoodBrdid(rm_key_t rmkey, u_int bcfgindex)
{
   int status,ec;
   char tmp[VB_SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG,"ResmgrEnsureGoodBrdid: OpenResmgr failed with %d(%s)",
             status,strerror(status));
      return(-1);
   }

   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,CM_BRDID,0,tmp,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"ResmgrEnsureGoodBrdid: RMgetvals for BRDID returned %d",
            ec);
      (void)CloseResmgr();
      return(-1);
   }
   RMend_trans(rmkey);
   (void)CloseResmgr();

   strtolower(tmp);   /* board ids converted to lower case in bcfgops.c */
   if (HasString(bcfgindex, N_BOARD_IDS, tmp, 0) == 1) return(0); 

   return(1);
}

/* what is the irq at the given resmgr key? */
int
irqatkey(rm_key_t rmkey, int *irq, int doopenclose)
{
   int status,ec, tmpirq;
   char tmp[VB_SIZE];
   char *strend;

   if (doopenclose) {
      status=OpenResmgr(O_RDONLY);
      if (status) {
         error(NOTBCFG,"irqatkey: OpenResmgr failed with %d(%s)",
                status,strerror(status));
         return(-1);
      }
   }

   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,CM_IRQ,0,tmp,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"irqatkey: RMgetvals for IRQ returned %d",
            ec);
      if (doopenclose) (void)CloseResmgr();
      return(-1);
   }
   RMend_trans(rmkey);
   if (doopenclose) (void)CloseResmgr();

   if (tmp[0] == '-') {    /* no IRQ set */
      *irq = -2;
      return(-2);
   }

   if (((tmpirq=(int) strtol(tmp,&strend,10))==0) && (strend == tmp)) {
      error(NOTBCFG,"irqatkey: invalid irq %s",tmp);
      return(-1);
   }

   *irq = tmpirq;

   return(0);
}

/* is the given irq sharable?  Used for smart bus boards
 * We cruise through the resmgr, looking for anybody using the
 * irq provided as an argument.  If they are, we look at itype and
 * ensure it's sharable
 * return values:
 *  -1 indicates error
 *   0 indicates irq is not sharable
 *   1 indicates irq is sharable
 */
int
irqsharable(rm_key_t ignorekey, int irq, int doopenclose)
{
   rm_key_t rmkey;
   int status,NKstatus,ec, tmpirq, tmpitype;
   char tmp[VB_SIZE];
   char *strend;

   if (doopenclose) {
      status=OpenResmgr(O_RDONLY);
      if (status) {
         error(NOTBCFG,"irqsharable: OpenResmgr failed with %d(%s)",
                status,strerror(status));
         return(-1);
      }
   }

   rmkey = NULL;

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {

      if (rmkey == ignorekey) continue;

      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_IRQ,0,tmp,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"irqsharable: RMgetvals for IRQ returned %d", ec);
         if (doopenclose) (void)CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      if (tmp[0] == '-') continue;

      if (((tmpirq=(int) strtol(tmp,&strend,10))==0) && (strend == tmp)) {
         notice("irqsharable: bad irq '%s' at rmkey %d",tmp,rmkey);
         continue;
      }

      if (tmpirq != irq) continue;

      /* ok, we found someone who is using the irq we're interested in.
       * if ITYPE indicates it's not sharable, return 0
       */
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_ITYPE,0,tmp,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"irqsharable: RMgetvals for ITYPE returned %d", ec);
         if (doopenclose) (void)CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      if (tmp[0] == '-') {
         /* ITYPE should be set since key is using an IRQ! */
         notice("irqsharable: ITYPE not set at rmkey %d",rmkey);
         continue;   /* we'll be polite and assume it is sharable */
      }

      if (((tmpitype=(int) strtol(tmp,&strend,10))==0) && (strend == tmp)) {
         notice("irqsharable: bad itype '%s' at rmkey %d",tmp,rmkey);
         continue;
      }
      
      /* XXX TODO: should also check for type 2 (only sharable with other
       * instances of same module here 
       */
      if (tmpitype == 1) {
         /* bummer, probably an ISA board using this IRQ */
         if (doopenclose) (void)CloseResmgr();
         return(0);
      }
   }
   if (doopenclose) (void)CloseResmgr();

   /* well, we made it all the way through the resmgr without finding any
    * conflicts.  must be sharable
    */
   return(1);
}

/* print out all of the nics that can do promiscuous mode.  useful for
 * paranoid sysadmins or for utilities (like libpcap, used by tcpdump/arpwatch
 * and others)
 */
int
promiscuous(void)
{
   rm_key_t rmkey;
   int status,NKstatus,ec;
   char tmp[VB_SIZE];
   char promiscstr[100];
   char elementstr[100];
   char fullnamestr[100];
   char promiscuous[VB_SIZE];
   char fullname[VB_SIZE];
   char FullnameWithHW[SIZE];

   status=OpenResmgr(O_RDONLY);
   if (status) {
      error(NOTBCFG,"irqsharable: OpenResmgr failed with %d(%s)",
             status,strerror(status));
      return(-1);
   }

   rmkey = NULL;
   snprintf(promiscstr,100,"%s,s",CM_PROMISCUOUS);
   snprintf(elementstr,100,"%s,s",CM_NETCFG_ELEMENT);
   snprintf(fullnamestr,100,"%s,s",CM_NIC_CARD_NAME);

   StartList(4,"YN?",4,"NAME",75);

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {

      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_MODNAME,0,tmp,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"promiscuous: RMgetvals for MODNAME returned %d", ec);
         (void)CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      /* since we skip backup keys, this means that if user pulls smart bus
       * board from the system then we *may* not be returning complete
       * information, depending on if CheckResmgrAndDevs has run to re-create
       * the primary key
       */

      /* skip ndcfg backup keys, they start with "net" and have a number
       * after them
       */
      if ((strlen(tmp) == 4) &&            /* "net0" through "net9" */
          (strncmp(tmp,"net",3) == 0) &&   /* starts with "net" */
          (isdigit(tmp[3]))) continue;     /* ends with a number */

      if ((strlen(tmp) == 5) &&            /* "net10" through "net99" */
          (strncmp(tmp,"net",3) == 0) &&   /* starts with "net" */
          (isdigit(tmp[3])) &&             /* 4th char is a number */
          (isdigit(tmp[4]))) continue;     /* 5th char is a number */

      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,elementstr,0,tmp,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"promiscuous: RMgetvals for NETCFG_ELEMENT returned %d",
               ec);
         (void)CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      if (tmp[0] == '-') continue;    /* key is not a network card */

      /* ok, we have a network card.  read PROMISCUOUS and NIC_CARD_NAME */
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,promiscstr,0,promiscuous,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"promiscuous: RMgetvals for PROMISCUOUS returned %d",ec);
         (void)CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      /* parameter may not be set in the resmgr */
      if ((promiscuous[0] != 'Y') && (promiscuous[0] != 'N')) {
         promiscuous[0] = '?';
      }

      if (promiscuous[0] == 'Y') {
         strcpy(promiscuous,"Yes");
      }
      if (promiscuous[0] == 'N') {
         strcpy(promiscuous,"No ");
      }

      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,fullnamestr,0,fullname,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"promiscuous: RMgetvals for NIC_CARD_NAME returned %d",
               ec);
         (void)CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      snprintf(FullnameWithHW, SIZE, "%s", fullname);
      if (AddHWInfoToName(rmkey, FullnameWithHW) != 0) {
         error(NOTBCFG,"promiscuous: AddHWInfoToName failed"); 
         (void)CloseResmgr();
         return(-1);
      }

      AddToList(2, promiscuous, FullnameWithHW);

   }

   (void)CloseResmgr();
   EndList();
   return(0);
}

int
determineprom(char *element)
{
   rm_key_t rmkey, backupkey;
   int status;

   rmkey = resshowkey(0, element, NULL, 0);  /* OpenResmgr/CloseResmgr */
   backupkey = resshowkey(0,element,NULL,1);  /* OpenResmgr/CloseResmgr */

   if ((rmkey == RM_KEY) && (backupkey == RM_KEY)) {
      error(NOTBCFG,"determineprom: element %s not found in resmgr",element);
      return(-1);
   }

   /* one of these can be RM_KEY but not both */
   return(determinepromfromkey(rmkey, backupkey, 1));
}

char *
dl_strerror(int errno, int uerrno)
{
   static char buf[100];

   if (errno == DL_SYSERR)
      return strerror(uerrno);
   else {
      switch (errno) {
         case   DL_ACCESS   :   
            return "Improper permissions for request, LLI compatibility ";
         case   DL_BADADDR   :   
            return "DLSAP address in improper format or invalid ";
         case   DL_BADCORR   :   
            return "Sequence number not from outstanding DL_CONN_IND ";
         case   DL_BADDATA   :   
            return "User data exceeded provider limit ";
         case   DL_BADPPA   :   
            return "Specified PPA was invalid ";
         case DL_BADPRIM   :   
            return "Primitive received is not known by DLS provider ";
         case DL_BADQOSPARAM   :   
            return "QOS parameters contained invalid values ";
         case DL_BADQOSTYPE   :   
            return "QOS structure type is unknown or unsupported ";
         case   DL_BADSAP   :   
            return "Bad LSAP selector, LLI compatibility ";
         case DL_BADTOKEN   :   
            return "Token used not associated with an active stream ";
         case DL_BOUND   :   
            return "Attempted second bind with dl_max_conind or "
                   "dl_conn_mgmt > 0 on same DLSAP or PPA ";
         case   DL_INITFAILED   :   
            return "Physical Link initialization failed ";
         case DL_NOADDR   :   
            return "Provider couldn't allocate alternate address ";
         case   DL_NOTINIT   :   
            return "Physical Link not initialized ";
         case   DL_OUTSTATE   :   
            return "Primitive issued in improper state, LLI compatibility ";
         case   DL_UNSUPPORTED   :   
            return "Requested service not supplied by provider ";
         case DL_UNDELIVERABLE :   
            return "Previous data unit could not be delivered ";
         case DL_NOTSUPPORTED  :   
            return "Primitive is known but not supported by DLS provider ";
         case   DL_TOOMANY   :   
            return "limit exceeded   ";
         case DL_NOTENAB   :   
            return "Promiscuous mode not enabled ";
         case   DL_BUSY      :   
            return "Other streams for a particular PPA in the "
                   "post-attached state ";
         case   DL_NOAUTO   :   
            return "Automatic handling of XID & TEST responses not supported ";
         case   DL_NOXIDAUTO   :    
            return "Automatic handling of XID not supported ";
         case   DL_NOTESTAUTO   :   
            return "Automatic handling of TEST not supported ";
         case   DL_XIDAUTO   :   
            return "Automatic handling of XID response ";
         case   DL_TESTAUTO   :   
            return "AUtomatic handling of TEST response";
         case   DL_PENDING   :   
            return "pending outstanding connect indications ";
      }
   }
   snprintf(buf,100,"Unknown error: %d\n",errno);
   return buf;
}

struct prim {
   int prim;
   char *name;
   char *desc;
} prims[] = {
{DL_INFO_REQ,"DL_INFO_REQ","Information Req, LLI compatibility"},
{DL_INFO_ACK,"DL_INFO_ACK","Information Ack, LLI compatibility"},
{DL_ATTACH_REQ,"DL_ATTACH_REQ","Attach a PPA"},
{DL_DETACH_REQ,"DL_DETACH_REQ","Detach a PPA"},
{DL_BIND_REQ,"DL_BIND_REQ","Bind dlsap address, LLI compatibility"},
{DL_BIND_ACK,"DL_BIND_ACK","Dlsap address bound, LLI compatibility"},
{DL_UNBIND_REQ,"DL_UNBIND_REQ","Unbind dlsap address, LLI compatibility"},
{DL_OK_ACK,"DL_OK_ACK","Success acknowledgment, LLI compatibility"},
{DL_ERROR_ACK,"DL_ERROR_ACK","Error acknowledgment, LLI compatibility"},
{DL_SUBS_BIND_REQ,"DL_SUBS_BIND_REQ","Bind Subsequent DLSAP address"},
{DL_SUBS_BIND_ACK,"DL_SUBS_BIND_ACK","Subsequent DLSAP address bound"},
{DL_SUBS_UNBIND_REQ,"DL_SUBS_UNBIND_REQ","Subsequent unbind"},
{DL_ENABMULTI_REQ,"DL_ENABMULTI_REQ","Enable multicast addresses"},
{DL_DISABMULTI_REQ,"DL_DISABMULTI_REQ","Disable multicast addresses"},
{DL_PROMISCON_REQ,"DL_PROMISCON_REQ","Turn on promiscuous mode"},
{DL_PROMISCOFF_REQ,"DL_PROMISCOFF_REQ","Turn off promiscuous mode"},
{DL_UNITDATA_REQ,"DL_UNITDATA_REQ","datagram send request, LLI compatibility"},
{DL_UNITDATA_IND,"DL_UNITDATA_IND","datagram receive indication, "
                                   "LLI compatibility"},
{DL_UDERROR_IND,"DL_UDERROR_IND","datagram error indication, "
                                 "LLI compatibility"},
{DL_UDQOS_REQ,"DL_UDQOS_REQ","set QOS for subsequent datagram transmissions"},
{DL_CONNECT_REQ,"DL_CONNECT_REQ","Connect request"},
{DL_CONNECT_IND,"DL_CONNECT_IND","Incoming connect indication"},
{DL_CONNECT_RES,"DL_CONNECT_RES","Accept previous connect indication"},
{DL_CONNECT_CON,"DL_CONNECT_CON","Connection established"},
{DL_TOKEN_REQ,"DL_TOKEN_REQ","Passoff token request"},
{DL_TOKEN_ACK,"DL_TOKEN_ACK","Passoff token ack"},
{DL_DISCONNECT_REQ,"DL_DISCONNECT_REQ","Disconnect request"},
{DL_DISCONNECT_IND,"DL_DISCONNECT_IND","Disconnect indication"},
{DL_RESET_REQ,"DL_RESET_REQ","Reset service request"},
{DL_RESET_IND,"DL_RESET_IND","Incoming reset indication"},
{DL_RESET_RES,"DL_RESET_RES","Complete reset processing"},
{DL_RESET_CON,"DL_RESET_CON","Reset processing complete"},
{DL_DATA_ACK_REQ,"DL_DATA_ACK_REQ","data unit transmission request"},
{DL_DATA_ACK_IND,"DL_DATA_ACK_IND","Arrival of a command PDU"},
{DL_DATA_ACK_STATUS_IND,"DL_DATA_ACK_STATUS_IND","Status indication of "
                                                 "DATA_ACK_REQ"},
{DL_REPLY_REQ,"DL_REPLY_REQ","Request a DLSDU from the remote"},
{DL_REPLY_IND,"DL_REPLY_IND","Arrival of a command PDU"},
{DL_REPLY_STATUS_IND,"DL_REPLY_STATUS_IND","Status indication of REPLY_REQ"},
{DL_REPLY_UPDATE_REQ,"DL_REPLY_UPDATE_REQ","Hold a DLSDU for transmission"},
{DL_REPLY_UPDATE_STATUS_IND,"DL_REPLY_UPDATE_STATUS_IND","Status of "
                                                         "REPLY_UPDATE req"},
{DL_XID_REQ,"DL_XID_REQ","Request to send an XID PDU"},
{DL_XID_IND,"DL_XID_IND","Arrival of an XID PDU"},
{DL_XID_RES,"DL_XID_RES","request to send a response XID PDU"},
{DL_XID_CON,"DL_XID_CON","Arrival of a response XID PDU"},
{DL_TEST_REQ,"DL_TEST_REQ","TEST command request"},
{DL_TEST_IND,"DL_TEST_IND","TEST response indication"},
{DL_TEST_RES,"DL_TEST_RES","TEST response"},
{DL_TEST_CON,"DL_TEST_CON","TEST Confirmation"},
{DL_PHYS_ADDR_REQ,"DL_PHYS_ADDR_REQ","Request to get physical addr"},
{DL_PHYS_ADDR_ACK,"DL_PHYS_ADDR_ACK","Return physical addr"},
{DL_SET_PHYS_ADDR_REQ,"DL_SET_PHYS_ADDR_REQ","set physical addr"},
{DL_GET_STATISTICS_REQ,"DL_GET_STATISTICS_REQ","Request to get statistics"},
{DL_GET_STATISTICS_ACK,"DL_GET_STATISTICS_ACK","Return statistics"}
};

static int nprims = sizeof(prims) / sizeof(struct prim);

char *
pprim(int p)
{
   int i;

   for (i=0; i<nprims; i++) {
      if (prims[i].prim == p) {
         return(prims[i].name);
      }
   }
   return("unknown");
}

/*
 * Bind to the given sap.
 */
int
dl_bind(int fd, int sap, char *ebuf) 
{
   dl_bind_req_t bind;
   struct strbuf cbuf,dbuf;
   union DL_primitives *p = (union DL_primitives *) cbuf.buf;
   unsigned char prim_buf [DL_PRIMITIVES_SIZE + ( 2*LLC_LIADDR_LEN )];
   int flag = 0;

   bind.dl_primitive = DL_BIND_REQ;
   bind.dl_max_conind = 0;
   bind.dl_sap = sap;
   bind.dl_service_mode = DL_CLDLS;
   bind.dl_conn_mgmt = 0;

   cbuf.maxlen = sizeof(prim_buf);
   cbuf.buf = (char *) &bind;
   cbuf.len = DL_BIND_REQ_SIZE;

   if (putmsg(fd, &cbuf, NULL, 0) < 0 ) {
      snprintf(ebuf,SIZE,"couldn't bind: %s\n",strerror(errno));
      return 0;
   }

   /* Wait for ACK */
   cbuf.buf = (char *) &prim_buf;

   if (getmsg(fd,&cbuf,NULL,&flag) < 0) {
      snprintf(ebuf,SIZE,"getmsg failed: %s\n",strerror(errno));
      return 0;
   }

   p = (union DL_primitives *) cbuf.buf;

   switch (p->dl_primitive) {
      case DL_BIND_ACK:
         break;

      case DL_ERROR_ACK: 
         {
            int e,u,n;
            e = p->error_ack.dl_error_primitive;
            u = p->error_ack.dl_unix_errno;
            n = p->error_ack.dl_error_primitive;

            if (n != DL_BIND_REQ) {
               snprintf(ebuf,SIZE,"Got error from primitive %s: %s\n",
                       pprim(n), dl_strerror(e,u));
            } else {
               snprintf(ebuf, SIZE, "BIND failed: %s\n",dl_strerror(e,u));
            }
            return 0;
        }
        break;

     default:
        snprintf(ebuf,SIZE,"Got primitive %s from BIND\n", 
                 pprim(p->dl_primitive));
        return 0;
        break;
   }
   return 1;
}

int
mdibind(int s)  /* sap not needed in MDI */
{
   mac_bind_req_t br;
   mac_ok_ack_t ba;
   struct strbuf ctlbuf;
   int flag;
   int r;

   ctlbuf.buf = (char *) &br;
   ctlbuf.len = MAC_BIND_REQ_SIZE;
   br.mac_primitive = MAC_BIND_REQ;

   flag = 0;
   /* send message downstream to bind access point */
   if ((r=putmsg(s, &ctlbuf, (struct strbuf *)0L, flag)) < 0) {
      return(r);
   }

   flag = 0;
   ctlbuf.buf = (char *) &ba;
   ctlbuf.len = MAC_OK_ACK_SIZE;
   ctlbuf.maxlen = sizeof (mac_ok_ack_t);

   if ((r=getmsg(s, &ctlbuf, 0L, &flag)) < 0) {
      return(r);
   }

   if (ba.mac_primitive != MAC_OK_ACK) {
      errno = EPROTO;
      return(-1);
   }

   return 0;
}

/*
 * Perform a STREAMS ioctl.
 */
static int
strioctl(int fd, int cmd, void *buf, size_t len) {
        struct strioctl ioc;

        ioc.ic_timout = 0;
        ioc.ic_cmd = cmd;
        ioc.ic_len = len;
        ioc.ic_dp = (char *) buf;

        return ioctl(fd,I_STR,&ioc);
}

/* given a resmgr key, figure out if the associated device supports
 * promiscuous mode.  The device can be MDI, ODI, or DLPI
 * Assumes that nobody is using the device (i.e. freshly installed or
 * we're in the process of booting -- so an open of the MDI device should
 * succeed.
 * We then write the PROMISCUOUS parameter and return.
 *
 * An alternate method is to create a new parameter PROMISCUOUS=true
 * in the .bcfg file and just write its value to the PROMISCUOUS parameter...
 *
 * backupkey functionality may or may not be enabled at this point so that's
 * why the supplied rmkey should be the primary one.
 * shouldn't call error blindly as this will affect the following ndcfg
 * commands, causing netcfg to die:
 *  - idinstall
 *  - determineprom
 */
int
determinepromfromkey(rm_key_t rmkey, rm_key_t backupkey, int doopenclose)
{
   char devnamestr[100], promiscstr[100], drivertypestr[100];
   char devname[VB_SIZE], promiscuous[VB_SIZE], drivertype[VB_SIZE];
   char ebuf[SIZE];
   char *answer;
   enum {MDI, ODI, DLPI} driver_type;
   int fd, prom, on=1, status, ec, s;

   if ((rmkey == RM_KEY) && (backupkey == RM_KEY)) {
      error(NOTBCFG,"determinepromfromkey no valid key: rmkey=%d backupkey=%d",
            rmkey, backupkey);
      return(-1);
   }

   if (doopenclose) {
      status=OpenResmgr(O_RDWR);  /* since we do RMgetvals and RMputvals */
      if (status) {
         error(NOTBCFG,"determineprom: OpenResmgr failed with %d(%s)",
                status,strerror(status));
         return(-1);
      }
   }

   snprintf(promiscstr, 100, "%s,s", CM_PROMISCUOUS);

   if (rmkey == RM_KEY) rmkey=backupkey; /* backupkey not RM_KEY to get here*/

   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,promiscstr,0,promiscuous,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"determinepromfromkey: RMgetvals for PROMISCUOUS "
                    "returned %d", ec);
      if (doopenclose) (void)CloseResmgr();
      return(-1);
   }
   RMend_trans(rmkey);

   /* since this isn't called when booting we allow user to do it again in
    * case PROMISCUOUS didn't get set in resmgr when installing the card
    */
#if OLDWAY
   /* if positively set then no work to do.Note we can get '?' if not known */
   if ((promiscuous[0] == 'Y') || (promiscuous[0] == 'N')) {
      if (doopenclose) (void)CloseResmgr();
      return(0);    /* no work to do */
   }
#endif
   
   /* DEV_NAME is foo_0 for ODI and DLPI, /dev/mdi/foo0 for MDI */
   snprintf(devnamestr, 100, "%s,s", CM_DEV_NAME);
   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,devnamestr,0,devname,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"determinepromfromkey: RMgetvals for DEV_NAME "
                    "returned %d", ec);
      if (doopenclose) (void)CloseResmgr();
      return(-1);
   }
   RMend_trans(rmkey);

   /* DRIVER_TYPE is either DLPI, MDI, or ODI */
   snprintf(drivertypestr, 100, "%s,s", CM_DRIVER_TYPE);
   RMbegin_trans(rmkey, RM_READ);
   if ((ec=RMgetvals(rmkey,drivertypestr,0,drivertype,VB_SIZE)) != 0) {
      RMabort_trans(rmkey);
      error(NOTBCFG,"determinepromfromkey: RMgetvals for DRIVER_TYPE "
                    "returned %d", ec);
      if (doopenclose) (void)CloseResmgr();
      return(-1);
   }
   RMend_trans(rmkey);
   strtoupper(drivertype);

   if (strcmp(drivertype, "ODI") == 0) {
      driver_type = ODI;
   } else
   if (strcmp(drivertype, "MDI") == 0) {
      driver_type = MDI;
   } else
   if (strcmp(drivertype, "DLPI") == 0) {
      driver_type = DLPI;
   } else {
      notice("determinepromfromkey: unknown driver type %s",drivertype);
      if (doopenclose) (void)CloseResmgr();
      return(0);    /* nothing else we can do, return "success" */
   }

   if (strncmp(devname,"/dev/",5) != 0) {
      notice("determinepromfromkey: invalid DEV_NAME of '%s'",devname);
      if (doopenclose) (void)CloseResmgr();
      return(0);    /* nothing else we can do, return "success" */
   }

   if ((fd=open(devname,O_RDWR)) == -1) {
      notice("determinepromfromkey: open of '%s' failed with %s",
             devname,strerror(errno));
      /* if MDI, dlpi module is probably using the device right now */ 
      if (doopenclose) (void)CloseResmgr();
      return(0);    /* nothing else we can do, return "success" */
   }

   /* from here on if we encounter a failure we must close(fd) 
    * before returning 
    */
   if ((driver_type == ODI) || (driver_type == DLPI)) {
      if (!dl_bind(fd, PROMISCUOUS_SAP, ebuf)) {
         notice("determinepromfromkey: dl_bind failed with %s",ebuf);
         goto promiscuous_no;
      }
      if ((prom=strioctl(fd, DLIOCGPROMISC, NULL, 0)) == -1) {
         notice("determinepromfromkey: DLIOCGPROMISC failed: %s", 
                strerror(errno));
         goto promiscuous_no;
      }
      if (prom > 0) {
         /* already in promiscuous mode; unlikely but possible */
         goto promiscuous_yes;
      }

      /* toggle promiscuous mode on */
      if (strioctl(fd, DLIOCSPROMISC, NULL, 0) == -1) {
         notice("DLIOCSPROMISC failed: %s", strerror(errno));
         goto promiscuous_no;
      }
      /* ok, it's on.  quickly close device which will turn promiscuous off */
      goto promiscuous_yes;
      /* NOTREACHED */
   } else {  /* MDI */
      if (mdibind(fd) < 0) {   
         notice("mdibind failed: %s", strerror(errno));
         goto promiscuous_no;
      }
      if (strioctl(fd, MACIOC_PROMISC, &on, sizeof(int)) == -1) {
         /* commented out since many MDI drivers don't support it */
         /* notice("MACIOC_PROMISC failed: %s", strerror(errno)); */
         goto promiscuous_no;
      }
      goto promiscuous_yes;
      /* NOTREACHED */
   }

promiscuous_yes:
   answer="Y";        /* must match what idinstall() sets */
   goto done;

promiscuous_no:
   answer="N";        /* must match what idinstall() sets */
   /* FALLTHROUGH */

done:
   close(fd);  /* if promisc was on this turns it off for MDI,ODI,DLPI */

   /* PROMISCUOUS may already be in the resmgr.  delete it just to be safe */
   RMbegin_trans(rmkey, RM_RDWR);
   MyRMdelvals(rmkey, promiscstr);
   RMend_trans(rmkey);
   if (backupkey != RM_KEY) {  /* it could be RM_KEY */
      RMbegin_trans(backupkey, RM_RDWR);
      MyRMdelvals(backupkey, promiscstr);
      RMend_trans(backupkey);
   }

   RMbegin_trans(rmkey, RM_RDWR);
   RMputvals_d(rmkey, promiscstr, answer, delim); /*not multivalued/range*/
   RMend_trans(rmkey);
   if (backupkey != RM_KEY) {  /* it could be RM_KEY */
      RMbegin_trans(backupkey, RM_RDWR);
      RMputvals_d(backupkey, promiscstr, answer, delim); /*not multival/range*/
      RMend_trans(backupkey);
   }

   if (doopenclose) (void)CloseResmgr();
   return(0);
}

int
DelAllVals(rm_key_t rmkey, char *param)
{
   int status;

   status=OpenResmgr(O_RDWR);  /* for RMdelvals */
   if (status) {
      error(NOTBCFG,"DelAllVals: OpenResmgr failed with %d(%s)",
             status,strerror(status));
      return(-1);
   }

   RMbegin_trans(rmkey, RM_RDWR);
   MyRMdelvals(rmkey, param);
   RMend_trans(rmkey);

   (void)CloseResmgr();
   return(0);
}

/* given a rmkey, walk through all hpsl structures until we find a match 
 * ENTRY REQUIREMENTS: You have called BlockHpslSignals() before calling this
 *                     routine.  We don't want anything changed by
 *                     libhpsl while we walk the arrays.
 */
HpslSocketInfoPtr_t 
GetHpslSocket(rm_key_t rmkey)
{
   int contIx, busIx, socketIx, deviceIx;

   for (contIx = 0; contIx < hpslcount; contIx++) {
    HpslBusInfoPtr_t busList = hpslcontinfo[contIx].hpcBusList;

    for (busIx = 0; busIx < hpslcontinfo[contIx].hpcBusCnt; busIx++) {
     HpslSocketInfoPtr_t socketList = busList[busIx].busSocketList;

     for (socketIx = 0; socketIx < busList[busIx].busSocketCnt; socketIx++) {
      DevInfoPtr_t devicelist = socketList[socketIx].devList;

      for (deviceIx = 0; deviceIx < socketList[socketIx].devCnt; deviceIx++) {
       if (devicelist[deviceIx].rmKey == rmkey) {
        return(&socketList[socketIx]);
       }
      }
     }
    }
   }
   return(NULL);
}

/* note that libhpsl requires that you set the HPSL_DEBUG environment variable
 * prior to calling hpsl_init_cont_info, so if it's not set now, then
 * this won't do a whole lot of anything either...
 * hpsl_init_cont_info first looks for the file '/tmp/hpsl.#'    
 * where # is process id and 
 * if file exists then append to it 
 * if file doesn't exist then write to stderr
 * we don't create it because of security issues.
 */
int
hpsldump(void)
{
   /* we called hpsl_init_cont_info earlier which set hpslcount */
   if (hpslcount == 0) {
      error(NOTBCFG,"hpsldump:  no hot plug controllers on system ");
      return(-1);
   }
   if (getenv("HPSL_DEBUG") == NULL) {
      notice("HPSL_DEBUG environment variable not set!");
   }
   BlockHpslSignals();
   hpsl_DumpInfo();
   UnBlockHpslSignals();
   StartList(2,"STATUS",10);
   AddToList(1,"success");
   EndList();
   return(0);
}

/* remember to call UnBlockHpslSignals before returning! */
int
hpslsuspend(char *element)
{
   int ret;
   HpslSocketInfoPtr_t socketInfo;
   rm_key_t rmkey=resshowkey(0,element,NULL,0); /* OpenResmgr/CloseResmgr */

   if (rmkey == RM_KEY) {   /* failure in resshowkey() */
      error(NOTBCFG,"hpslsuspend: element %s not found in resmgr", element);
      return(-1);
   }
   BlockHpslSignals();     /* must do while we call libhpsl routines */
   if (hpslcount == 0) {
      notice("hpslsuspend: no hot plug controllers on system, faking success");
      goto suspendfakesuccess;
   }
   if ((socketInfo=GetHpslSocket(rmkey)) == NULL) {
      notice("hpslsuspend: can't find socket for element %s rmkey %d, "
             "faking success",element, rmkey);
      goto suspendfakesuccess;
   }
   /* try to call driver's DDI8 CFG_SUSPEND routine 
    * hpsl_suspend_io assumes that socket isn't empty and currently has power
    * so we head off a potential failure
    */
   if (socketInfo->socketCurrentState & SOCKET_EMPTY) {
      notice("hpslsuspend: socket is empty, faking success");
      goto suspendfakesuccess;
   }
   if (!socketInfo->socketCurrentState & SOCKET_POWER_ON) {
      notice("hpslsuspend: socket doesn't currently have power");
      if (hpsl_set_state(socketInfo, SOCKET_POWER_ON, HPSL_SET) !=
                                                               HPSL_SUCCESS) {
         notice("hpslsuspend: hpsl_set_state to turn power on "
                "failed, hpsl_ERR=%d errno=%d", hpsl_ERR, errno);
         goto suspendfakesuccess;
      }
      goto suspendfakesuccess;
   }
   if ((ret=hpsl_suspend_io(socketInfo, rmkey)) != HPSL_SUCCESS) {
      if (hpsl_ERR == hpsl_eINVALID) {
         /* one of the following happened:
          * - no driver to check hotplug capability 
          * - no DDI8 D_SUSPEND in its drvinfo(D4) drv_flags
          */
         notice("hpslsuspend: not ddi8 driver with D_SUSPEND, faking success");
         goto suspendfakesuccess;
      } 
      /* if driver isn't ddi8 then this will fail with errno=ENODEV and
       * hpsl_ERR = 0.  This is because hpsl_suspend_io calls 
       * hpsl_chk_drv_capability and ioctl HPCI_GET_DRV_CAP has failed
       * with ENODEV in function ddi_getdrv_cap() in util/mod/mod_drv.c
       * Since not ddi8, suspend will never work, must fake success.
       * NOTE:  netcfg should call the "hpslcanhotplug" command prior to
       *        calling hpslsuspend to avoid this situation!
       *        XXX Should probably call error here to enforce this.
       */
      notice("hpslsuspend: hpsl_suspend_io failed with errno=%d hpsl_ERR=%d, "
             "faking success", errno, hpsl_ERR);
      goto suspendfakesuccess;
   }
   /* do other work here as necessary.  if you must call error and return
    * be sure to call UnBlockHpslSignals first
    * note that MODNAME is now '-' for the suspended driver.
    */
suspendfakesuccess:
   UnBlockHpslSignals();
   StartList(2,"STATUS",30);
   AddToList(1,"success");
   EndList();
   return(0);
}

/* returns NULL on failure.  Assumes that we have called BlockHpslSignals
 * previously.
 */
HpslSocketInfoPtr_t
GetHpslSuspendedSocket(rm_key_t rmkey, int *suspId)
{
   u_int loop, deviceIx;
   DevInfoPtr_t devicelist;

   if (hpsl_get_suspended_drvlist(&hpslsuspendeddrvinfoptr, 
                                  &hpslsuspendedcnt) != HPSL_SUCCESS) {
      error(NOTBCFG,"GetSuspendedDrvInfo(%d) failed, hpsl_ERR=%d errno=%d",
            rmkey, hpsl_ERR, errno);
      return(NULL);
   }
   if (hpslsuspendedcnt <= 0) {
      notice("GetSuspendedDrvInfo: no suspended drivers");
      return(NULL);
   }
   notice("there are currently %d suspended drivers",hpslsuspendedcnt);
   for (loop=0; loop < hpslsuspendedcnt; loop++) {
      HpslSocketInfo_t *socketInfo=&hpslsuspendeddrvinfoptr[loop].socketInfo;
      devicelist = socketInfo->devList;
      for (deviceIx = 0; deviceIx < socketInfo->devCnt; deviceIx++) {
         if (devicelist[deviceIx].rmKey == rmkey) {
            *suspId=hpslsuspendeddrvinfoptr[loop].suspId; 
            return(socketInfo);
         }
      }
   }
   return(NULL);
}

/* resume the driver 
 * ENTRANCE CRITERIA:  driver must be currently currently suspended else
 * GetSuspendedDrvInfo will fail.
 * note that MODNAME is '-' if driver is currently suspended
 */ 
int
hpslresume(char *element)
{
   int ret, suspId;
   HpslSocketInfoPtr_t socketinfo;
   rm_key_t rmkey=resshowkey(0,element,NULL,0); /* OpenResmgr/CloseResmgr */

   if (rmkey == RM_KEY) {   /* failure in resshowkey() */
      error(NOTBCFG,"hpslresume: element %s not found in resmgr", element);
      return(-1);
   }
   BlockHpslSignals();     /* must do while we call libhpsl routines */
   if (hpslcount == 0) {
      notice("hpslresume: no hot plug controllers on system, faking success");
      goto resumefakesuccess;
   }
   /* find out instance # and also if this key is in suspended state or not */
   if ((socketinfo=GetHpslSuspendedSocket(rmkey, &suspId)) == NULL) {
      /* if we're trying to resume something that was never suspended (ddi7
       * driver then this is an error.  netcfg should use hpslcanhotplug
       * command prior to calling this routine so we can call error here
       */
      error(NOTBCFG,"hpslresume: can't find suspended socket for "
                    "element %s rmkey %d", element, rmkey);
      UnBlockHpslSignals();
      return(-1);
   }
   /* try to call driver's DDI8 CFG_RESUME routine
    * hpsl_resume_io assumes that socket isn't empty and currently has power
    * so we head off a potential failure
    */
   if (socketinfo->socketCurrentState & SOCKET_EMPTY) {
      notice("hpslresume: socket is empty, faking success");
      goto resumefakesuccess;
   }
   if (!socketinfo->socketCurrentState & SOCKET_POWER_ON) {
      notice("hpslresume: socket doesn't currently have power");
      goto resumefakesuccess;
   }
   if ((ret=hpsl_resume_io(socketinfo, rmkey, suspId)) != HPSL_SUCCESS) {
      if (hpsl_ERR == hpsl_eINVALID) {
         /* one of the following happened:
          * - no driver to check hotplug capability
          * - no DDI8 D_RESUME in its drvinfo(D4) drv_flags
          */
         notice("hpslresume: not ddi8 driver with D_RESUME, faking success");
         goto resumefakesuccess;
      }
      notice("hpslresume: hpsl_suspend_io failed with errno=%d hpsl_ERR=%d, "
             "faking success", errno, hpsl_ERR);
      goto resumefakesuccess;
   }
   /* do other work here as necessary.  if you must call error and return
    * be sure to call UnBlockHpslSignals first
    */
resumefakesuccess:
   UnBlockHpslSignals();
   StartList(2,"STATUS",30);
   AddToList(1,"success");
   EndList();
   return(0);

}


/* assumes element can be found on the normal list instead of the suspended
 * list
 */
int
hpslgetstate(char *element)
{
   HpslSocketInfoPtr_t socketinfo;
   rm_key_t rmkey=resshowkey(0,element,NULL,0); /* OpenResmgr/CloseResmgr */

   if (rmkey == RM_KEY) {   /* failure in resshowkey() */
      error(NOTBCFG,"hpslresume: element %s not found in resmgr", element);
      return(-1);
   }
   if (hpslcount == 0) {
      notice("hpslgetstate: no hot plug controllers on system, faking success");
      goto getstatefakesuccess;
   }
   BlockHpslSignals();
   if ((socketinfo=GetHpslSocket(rmkey)) == NULL) {
      notice("hpslsuspend: can't find socket for element %s rmkey %d, "
             "faking success",element, rmkey);
      goto getstatefakesuccess;
   }
   StartList(24,"STATUS",8,
                "EMPTY",6,"PWR",4,"GFLT",5,"PIN1",5,"PIN2",5,
                "PFLT",5,"ATTN",5,"HPCAPABLE",10,
                "CFLT",5,"FREQ",5,"INTERLOCKED",12);
   AddToList(12,"success",
       socketinfo->socketCurrentState & SOCKET_EMPTY ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_POWER_ON ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_GENERAL_FAULT ? "Y" : "N",
       /* PIN1/PIN2 indicates when card is physically added or removed
        * from machine -- no power to card is necessary for bit to change
        */
       socketinfo->socketCurrentState & SOCKET_PCI_PRSNT_PIN1 ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_PCI_PRSNT_PIN2 ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_PCI_POWER_FAULT ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_PCI_ATTENTION_STATE ? "Y" : "N",
       /* note hpcd_get_current_state() always sets HOTPLUG_CAPABLE */
       socketinfo->socketCurrentState & SOCKET_PCI_HOTPLUG_CAPABLE ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_PCI_CONFIG_FAULT ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_PCI_BAD_FREQUENCY ? "Y" : "N",
       socketinfo->socketCurrentState & SOCKET_PCI_INTER_LOCKED ? "Y" : "N");
   EndList();
   UnBlockHpslSignals();
   return(0);
   /* NOTREACHED */

getstatefakesuccess:
   UnBlockHpslSignals();
   StartList(24,"STATUS",8,
                "EMPTY",6,"PWR",4,"GFLT",5,"PIN1",5,"PIN2",5,
                "PFLT",5,"ATTN",5,"HPCAPABLE",10,
                "CFLT",5,"FREQ",5,"INTERLOCKED",12);
   AddToList(12,"unknown","?","?","?","?","?","?","?","?","?","?","?");
   EndList();
   return(0);
}

/* there are a number of ways to do this, each having pros and cons.
 * - look for '$interface ddi foo' line in Master file 
 * - looking in mod.d special elf symbol
 * - look at _DDI_ symbol/fuction from <sys/ddi.h> - new to Gemini though
 * - wade through /dev/kmem
 * - ask hpsl for capabilities
 * we use the last method because all we really care about is if the
 * driver is hot-plug capable, which is new to ddi8.
 */
int
hpslcanhotplug(char *element)
{
   int fd, bcfgindex;
   char *driver_name, *answer="N";

   if (hpslcount == 0) {
      notice("hpslcanhotplug: no hot plug controllers on system");
      goto canhotplugdone;
   }

   bcfgindex=elementtoindex(0,element); /* OpenResmgr/CloseResmgr */
   if (bcfgindex == -1) {
      /* notice already called */
      error(NOTBCFG,"hpslcanhotplug: elementtoindex failed");
      return(-1);
   }
   driver_name=StringListPrint(bcfgindex,N_DRIVER_NAME);
   if (driver_name == NULL) {
      error(NOTBCFG,"hpslcanhotplug: can't get driver_name for %s (%d)",
            element, bcfgindex);
      return(-1);
   }

   BlockHpslSignals();
   /* can't open /dev/hpci because libhpsl already has it open and it's
    * an exclusive open device (for now)
    * we don't care if routine returns HPCI_ERROR or HPSL_FAIL -- both
    * don't change our answer of "no"
    */
   if (hpsl_check_drv_capability(driver_name, D_HOT) == HPSL_SUCCESS) {
      answer="Y";
   }
   UnBlockHpslSignals();

   free(driver_name);

canhotplugdone:
   StartList(2,"ANSWER",8);
   AddToList(1,answer);
   EndList();
   return(0);
}


/* show the hardware characteristics about an installed card.  The
 * resshowunclaimed command is for cards that aren't installed yet
 * although they display the same information
 */
int
gethwkey(char *element)
{
   /* borrow size from ResShowUnclaimed code */
   char LongName[SIZE+NDNOTEMAXSTRINGSIZE];

   rm_key_t rmkey=resshowkey(0,element,NULL,0); /* OpenResmgr/CloseResmgr */

   LongName[0]='\0';

   if (rmkey == RM_KEY) {   /* failure in resshowkey() for primary */
      /* try backup key */
      notice("gethwkey: element %s not found in resmgr, trying backup",element);
      rmkey=resshowkey(0,element,NULL,1);  /* does a OpenResmgr/CloseResmgr */
   }

   if (rmkey == RM_KEY) {   /* not at primary, not at backup, fake it */
      notice("gethwkey: element %s not found in resmgr, faking success", 
             element);
      goto gethwkeyfakesuccess;
   }

   if (AddHWInfoToName(rmkey,LongName) != 0) {
      error(NOTBCFG,"gethwkey: AddHWInfoTOName failed"); 
      return(-1);
   }
 
gethwkeyfakesuccess:
   StartList(2,"INFO",78);
   AddToList(1,LongName);
   EndList();
   return(0);
}

/* generate the equivalent of the dlpimdi file to stdout for utilities that 
 * depend on it, namely ndstat, dlpid, and /etc/nd
 * note that element can be null
 */
int
dlpimdi(char *element)
{
   error(NOTBCFG,"not implemented yet");
   return(0);

}

/* delete all keys in the resmgr with the given modname
 * doesn't affect the link kit.
 * assumes the resmgr isn't open currently.
 */
int
DelAllKeys(char *modname)
{
   int status, NKstatus, ec;
   char tmp[VB_SIZE];
   rm_key_t rmkey;

   if (modname == NULL) {
      error(NOTBCFG,"DelAllKeys: null modname!");
      return(-1);
   }

   if ((strlen(modname) == 0) ||
       (strcmp(modname,"-") == 0)) {
      error(NOTBCFG,"DelAllKeys: invalid modname '%s'",modname);
      return(-1);
   }

   status=OpenResmgr(O_RDWR);
   if (status) {
      error(NOTBCFG,"DelAllKeys: OpenResmgr failed with %d(%s)",
             status,strerror(status));
      return(-1);
   }

   rmkey = NULL;

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_MODNAME,0,tmp,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         error(NOTBCFG,"DelAllKeys: RMgetvals for MODNAME returned %d", ec);
         (void)CloseResmgr();
         return(-1);
      }
      RMend_trans(rmkey);

      if (strcmp(modname, tmp) == 0) {
         RMdelkey(rmkey);
      }
   }
   (void)CloseResmgr();
   return(0);
}

/* convert the REALKEY=wdn3 to a resmgr key if possible
 * resmgr is already open and should stay open
 * we look for an entry whose MODNAME is wdn and whose NDCFG_UNIT parameter
 * is the number indicated
 * returns RM_KEY if not found
 */
rm_key_t
RealKey2rmkey(char *realkey)
{
   rm_key_t rmkey;
   int loop, unit, length, NKstatus, ec;
   char tmp[VB_SIZE];
   char modname[VB_SIZE];
   char ndcfg_unit[VB_SIZE];
   char junk[VB_SIZE];
   
   if (!isalpha(realkey[0])) return(RM_KEY);   /* must start with character */

   unit=0;
   strncpy(tmp,realkey,VB_SIZE);
   length=strlen(tmp);
   if (length < 2) return(RM_KEY); /* need at least a character and a digit */

   for (loop=length-1; loop>=0; loop--) {
      if (!isdigit(tmp[loop])) {
         unit=atoi(&tmp[loop+1]);
         tmp[loop+1]='\0';
         break;
      }
   }
   /* did we find a number in the realkey passed in as an argument? */
   if (loop == -1 && unit == 0) {
      notice("RealKey2rmkey: bad REALKEY %s",realkey);
      return(RM_KEY);
   }

   /* now go through the resmgr looking for MODNAME=tmp and NDCFG_UNIT=unit */
   rmkey = NULL;

   while ((NKstatus=MyRMnextkey(&rmkey)) == 0) {
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,CM_MODNAME,0,modname,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         return(RM_KEY);
      }
      RMend_trans(rmkey);

      if (strcmp(modname, tmp) != 0) continue;

      snprintf(junk, VB_SIZE, "%s,n", CM_NDCFG_UNIT);
      RMbegin_trans(rmkey, RM_READ);
      if ((ec=RMgetvals(rmkey,junk,0,ndcfg_unit,VB_SIZE)) != 0) {
         RMabort_trans(rmkey);
         return(RM_KEY);
      }
      RMend_trans(rmkey);

      if (atoi(ndcfg_unit) != unit) continue;

      /* MODNAME matches, NDCFG_UNIT matches, we have a winner */
      return(rmkey);
   }
   return(RM_KEY);
}

/* issue an RMdelvals for MODNAME which will call ddi8 driver's CFG_REMOVE
 * code.
 */
int
toastmodname(rm_key_t rmkey)
{
   int status;

   status=OpenResmgr(O_RDWR);
   if (status) {
      error(NOTBCFG,"toastmodname: OpenResmgr failed with %d(%s)",
             status,strerror(status));
      return(-1);
   }

   RMbegin_trans(rmkey, RM_RDWR);
   (void) MyRMdelvals(rmkey,CM_MODNAME);/* don't care about return value */
   RMend_trans(rmkey);

   (void)CloseResmgr();
   return(0);
}
