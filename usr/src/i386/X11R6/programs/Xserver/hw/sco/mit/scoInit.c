/*
 *	@(#)scoInit.c	11.10	12/4/97	17:39:09
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * scoInit.c --
 *	Initialization functions for screen/keyboard/mouse, etc.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	??? Sept ??????????????? 1990	mikep@sco.com
 *	- Created File from ddx/sun.
 *
 *	S001	Thu Dec 06 02:16:41 PST 1990	buckm@sco.com
 *	- Extend video mode support.
 *
 *	S002	Wed Jan 23 16:35:19 PST 1991	mikep@sco.com
 *	-  Added NFB ifdefs and code.  Needs to be cleaned up.
 *
 *	S003	Thu Jan 31 19:14:56 PST 1991	buckm@sco.com
 *	- Switch to using GrafInfo instead of VidConf.
 *
 *	S004	Mon Feb 11 21:32:35 PST 1991	mikep@sco.com
 *	- Removed Blockhandlers and garbage pertaining to autorepeat.
 *	in scoScreenInit.
 *
 *	S005	Wed Feb 27 21:37:36 PST 1991	buckm@sco.com
 *	- Don't SetCrtName() here; use stdout if GetCrtName() returns null.
 *
 *	S006	Wed Mar 27 02:05:14 PST 1991	mikep@sco.com
 *	- Change sgi Structures to ddx Structures.   Still a few hanging
 *	around.
 *
 *	S007	Wed Apr 03 01:52:12 PST 1991	mikep@sco.com
 *	- Added calls to grab the screen and start VT processing
 *	This is part of the new screen switch processing.  Note
 *	this is where we assume which fd we are going to ioctl to.
 *
 *	S008	Tue Apr 09 22:58:29 PDT 1991	buckm@sco.com
 *	- Fix usage of grafData fields when setting numPixmapFormats.
 *	- Force scoVidFd to be stdout.
 *
 *	S009	Tue Apr 23 12:13:01 PDT 1991	buckm@sco.com
 *	- Tell kernel we are in graphics mode.
 *
 *	S010	Tue Apr 23 14:41:04 PDT 1991	buckm@sco.com
 *	- Move S009 to after call to ParseGrafInfo().
 *
 *	S011	Tue Apr 23 16:01:26 PDT 1991	buckm@sco.com
 *	- Only do SetText() if DisplayModeChanged.
 *
 *	S012	Thu Apr 26 21:00:00 PDT 1991	pavelr@sco.com
 *	- check return value of ParseGrafInfo, die if NULL
 *
 *	S013	Tue May 07 23:40:41 PDT 1991	mikep@sco.com
 *	- Make some direct calls to MetaWindows to put the screen back
 *	into text mode.  This needs to be cleaned up, once the interface
 *	between the sco and xxxvideo layer is better defined. 
 *
 *	S014	Wed May 08 23:56:25 PDT 1991	mikep@sco.com
 *	- Added call to scoSetSSFD()
 *	- Only call scoEnterVtProcMode() once.
 *
 *	S015	Thu May 09 10:41:59 PDT 1991	buckm@sco.com
 *	- Re-order a few of the calls in scoInitVid:
 *	  the console driver can't deal with switching to a screen
 *	  in graphics mode, so postpone the switch to graphics mode
 *	  ioctl until after we know we are the active screen;
 *	  go into VtProc mode and _then_ wait to become the active
 *	  screen; this eliminates the window between the two.
 *	- Use new routine scoBecomeActiveScreen.
 *
 *	S016	Fri May 10 12:35:41 PDT 1991	buckm@sco.com
 *	- Set ReadyForUnblank flag in scoInitVid.
 *
 *	S017	Sat May 11 13:34:59 PDT 1991	buckm@sco.com
 *	- In scoRestoreVid, don't touch screen if not active.
 *
 *	S018	Sun May 12 00:47:42 PDT 1991	mikep@sco.com
 *	- Move S016 so that resets get the screen unblanked as well.
 *	- Cast to get rid of compiler warning.
 *
 *	S019	Tue Jun 11 18:28:42 PDT 1991	mikep@sco.com
 *	- Drop in new InitOutput() from SGI.  Cleaned up include files.
 *
 *      S020    Wed Jun 12 00:07:23 PDT 1991    mikep@sco.com
 *      - Removed scoScreenInit().
 *
 *      S021    Fri Jun 14 02:35:33 PDT 1991	buckm@sco.com
 *      - Clean up NFB ifdefs.
 *	- Get rid of OLD_SRC.
 *	- Get rid of some globals.
 *
 *	S022	Sun Jun 23 18:15:34 PDT 1991	mikep@sco.com
 *	- Always call miRegisterPointerDevice().  It's harmless.
 *
 *	S023	Fri Aug 09 12:17:08 PDT 1991	mikep@sco.com
 *	- Added Test Extension Mods.
 *
 *	S024	Sun Aug 25 17:48:12 PDT 1991	mikep@sco.com
 *	- Made modifications needed for using scoSysInfo.
 *
 *	S025	Tue Sep 10 14:53:04 PDT 1991	mikep@sco.com
 *	- Remove Pixmap format manipulation in favor of 
 *	ddxAddPixmapFormat().  This must be called for 1 bit.
 *
 *      S026    Wed Sep 11 11:26:26 PDT 1991	buckm@sco.com
 *      - Get rid of SW_CG320 call; we let the ddx drivers
 *	take care of it if they need to.  Only issue mode-change
 *	ioctl to console if needed.
 *
 *	S027	Sat Sep 21 15:57:11 PDT 1991	mikep@sco.com
 *	- Set the currentVT.  This will be needed for Alt-Prnt-Screen
 *
 *	S028	Tue Sep 24 08:37:09 PDT 1991	buckm@sco.com
 *	- Fix S027 so it doesn't sometimes crash server.
 *
 *	S029	Fri Sep 27 16:24:21 1991	staceyc@sco.com
 *	- Initialize screen switch default keys.
 *
 *	S030	Mon Oct 21 01:19:08 PDT 1991	mikep@sco.com
 *	- Move SetInputCheck() into scoMouse.c since the QUEUE isn't
 *	initialized until then.
 *
 *	S031	Mon Aug 31 19:46:11 PDT 1992	mikep@sco.com
 *	- Add dummy function to pull in things that the linker doesn't
 *	see.
 *
 *	S032	Wed Sep 02 22:39:05 PDT 1992	mikep@sco.com
 *	- Use MPX ioctls to lock the server on a processor.  Controlled
 *	by command line args.
 *
 *	S033	Tue Sep 15 19:50:26 PDT 1992	mikep@sco.com
 *	- Remove scoCloseScreen wrapper from this file.  Now in scoScreen.c
 *	- Add call to initialize bit swap array.
 *
 *	S034	Tue Sep 29 22:18:08 PDT 1992	mikep@sco.com
 *	- Call mieqInit().
 *
 *	S035	Thu Oct 08 12:36:53 PDT 1992	buckm@sco.com
 *	- Close file used by S032.
 *
 *	S036	Wed Nov 25 09:26:30 PST 1992	buckm@sco.com
 *	- Unify the initial Crt mucking from scoInitVid()
 *	  and scoOpenKbd() into scoInitCrt().  This will be called
 *	  from OsInit().
 *
 *	S037	Tue Dec 01 09:57:21 PST 1992	buckm@sco.com
 *	- Exit on errors in scoInitCrt(); add user-helpful error message.
 *	- Don't insist that stdin be the console, just a tty.
 *
 *	S038	Fri Dec 18 14:00:00 PST 1992 	chrissc@sco.com
 *	- initialized function pointers for bitmap scaling to
 *	- to scoPopMessage etc for scaling pop up message.
 *
 *      S039    Tue Jul 20 09:57:54 PDT 1993    davidw@sco.com
 *      - Added XPG4 Message Catalogue support.
 *
 *	S040	Thu Oct 14 18:57:19 PDT 1993	buckm@sco.com
 *	- Add checks and error messages for the vx driver and for being root.
 *
 *	S041	Wed Dec 01 13:52:36 PST 1993	buckm@sco.com
 *	- Upgrade the MPX locking to use the Everest ACPU_XLOCK ioctl.
 *
 *	S042	Fri Jan 26 14:35:36 PST 1996	hiramc@sco.COM
 *	- begin modifications to allow compile on Unixware.
 *	- all modifications marked by define(gemini)
 *
 *	S043	Tue Jan 21 16:05:40 PST 1997	kylec@sco.com
 *	- Wait until the graphics screen is active before
 *	  resetting the server.
 *	- Reset screen switch signal handler after screen switch
 *	  has occured.
 *	S044	Tue Feb 25 14:12:16 PST 1997	hiramc@sco.COM
 *	- need to SIG_IGN the SIGUSR2 signal ASAP in OsVendorInit
 *
 *	S045	Sun Mar  2 11:54:49 PST 1997	kylec@sco.com
 *	- set VMEM limits to maximum configured for system.
 *
 *	S046    Thu Apr  3 15:52:24 PST 1997	kylec@sco.com
 *	- add i18n support
 *	S047	Tue Apr 15 09:25:06 PDT 1997	hiramc@sco.COM
 *	- add proper MultiScreen operation, and CON_MEM_MAPPED
 *	S048	Thu Jul 24 15:35:42 PDT 1997	kd@sco.com
 *	- Make a ttyname(3c) return of "/dev/vt00" be treated
 *	equivalent to "/dev/console" in the function that
 *	determines whether to use the active VT or a free
 *	VT to run the X server in..
 *		(Delta done by anthony@sco.com)
 *	S049	Fri Sep 12 14:35:58 PDT 1997	hiramc@sco.COM
 *	- exit code for duplicate server is 2 for Xsco
 *	- so that startx/xinit can find another display
 *	- xinit requires updates to recognize this too.
 *	S050	Wed Oct  8 11:26:40 PDT 1997	hiramc@sco.COM
 *	- when mouse is not working, exit code 3, and
 *	-	Thu Nov 13 16:04:01 PST 1997
 *	- changed to use scoAbortServerN() - was leaving the display
 *	- in graphics mode on exit.
 *	S051	Tue Oct 28 14:07:38 PST 1997	hiramc@sco.COM
 *	- consolidate all mmap calls into hw/sco/grafinfo/MemConMap.c
 *	S052	Thu Oct 30 11:14:58 PST 1997	hiramc@sco.COM
 *	- rework opening sequence to properly start on any console device
 *	- still will not function correctly with -crt option
 *	S053	Tue Nov 11 16:50:58 PST 1997	hiramc@sco.COM
 *	- no more v86.h in Gemini for BL15
 *	- looks like it was useless here anyway, removal changed nothing.
 *	S054	Mon Nov 17 09:18:00 EST 1997   	brianr@sco.com
 *	- We cannot deal with screen switch at signal time.  Wait until
 *	- either scoWakeup() or scoWaitUntilActive() deal with it.
 *	S055	Thu Dec  4 17:36:29 PST 1997	hiramc@sco.COM
 *	- Do NOT honor -crt /dev/console - in this case, go
 *	- find a free VT  (/usr/bin/X11/TestMode does this)
 *	- actually any of /dev/console, /dev/syscon, /dev/systty, /dev/vt00
 */

#include "sco.h"

#if defined(usl)
#include <stdio.h>
#include <limits.h>
#include <string.h>	/*	S047	*/
#include <sys/types.h>
#include <sys/unistd.h>	/*	S047	*/
#include <sys/mman.h>
#include <sys/time.h>           /* S045 */
#include <sys/resource.h>       /* S045 */
#include <sys/kd.h>		/* S047 */
#include <sys/ws/ws.h>		/* S047 */
#else
#include <sys/ci/ciioctl.h>					/* S032 */
#endif

#define MPXNAME "/dev/atp1"					/* S041 */
#define	BASECPU	1						/* S041 */

#define XK_MISCELLANY
#include "X11/keysymdef.h"
#include <servermd.h>
#include "dixstruct.h"
#include "dix.h"
#include "opaque.h"
#include "mipointer.h"
#include "xsrv_msgcat.h"					/* S039 */

#if defined(usl)
#include "scoEvents.h"
#endif

/* This shouldn't be needed.  Remove the Pixmap format stuff */
#include "ddxScreen.h"				/* S006 */

#include "xsrv_msgcat.h"	/* S046 */

extern void scoAbortServerN( int );	/*	S050	*/
extern void scoInitVid();
extern int scoMouseProc();
extern int scoKbdProc();
extern void ProcessInputEvents();

extern int scoPopMessage();					/* S038 */
extern int scoHideMessage();					/* S038 */
extern int (*ScalePopMessage)();				/* S038 */
extern int (*ScaleHideMessage)();				/* S038 */
    
extern char *strncpy();
extern GCPtr CreateScratchGC();
    
#if defined(usl)
static void set_iopl( void );
#else
extern scoEventQueue *scoQueue;
#endif /* usl */
    
extern Bool mpxLock;						/* S031 */
    
#ifdef XTESTEXT1						/* S023 */
extern KeyCode xtest_command_key;
#endif
    
    
unsigned long scoGeneration = 0;
  
int scoVidOldMode = 0;
    
scoSysInfoRec scoSysInfo;					/* S024 */
    
extern void scoScreenInit(ScreenPtr pScreen);			/* S024 */
    
#if defined(usl)

extern Bool CRTSpecified;		/* S052	defined in os/utils.c	*/

/*
 * scoVTRequest --
 *      Notice switch requests form the vt manager.
 */
void
scoVTRequest (int signo)
{
  extern void scoVTSwitch();

  OsSignal(SIGUSR2, SIG_IGN); 

#ifdef NOTNOW			/* S054 */
  if (!scoScreenActive())
    {
      scoVTSwitch();
      OsSignal(SIGUSR2, scoVTRequest); 
    }
  else
#endif				/*	S054	*/
    scoSysInfo.switchReqPending = TRUE;
}

static void
scoWakeup (pointer blockData, int err, pointer pReadmask)
{
  extern void scoVTSwitch();

  if (scoSysInfo.inputPending)
    ProcessInputEvents();

  if (scoSysInfo.switchReqPending)
    {
      scoVTSwitch();
      OsSignal(SIGUSR2, scoVTRequest);
    }
}


static void
scoBlock (pointer blockData, int err, pointer pReadmask)
{
    return;
}
#endif /* usl */

/*
 * On SCO Openserver, there is nothing in OsVendorInit
 * For Unixware, there is a lot of init code in OsVendorInit
 */
void OsVendorInit(
#if NeedFunctionPrototypes
                  void
#endif
                  )
{
#if defined(usl)
    FILE             	*fp;
    int              	fd, pid;
    caddr_t 		addr;
    char             	fname[PATH_MAX];
    extern char 	*display;
    struct vt_mode   	VT;
    static Bool      	os_vendor_inited = FALSE;
    static struct    	kd_quemode xqueMode;
    struct rlimit 	rl;     /* S045 */
    extern void SetTTYName(char *);	/*	S047  vvv	*/
    extern char * GetTTYName(void);
    char	* TargetTTYName;
    char	CurVtName[PATH_MAX];
    struct	vt_stat vtinfo;		/*	S047   ^^^	*/
    int ioRtn;				/*	S052	*/
    Bool ConsoleDevice;			/*	S052	*/
    Bool DevConsole;			/*	S052	*/

    if (!os_vendor_inited)
    {
        os_vendor_inited = TRUE;

	OsSignal(SIGUSR2, SIG_IGN);	/*	S044	*/

        sprintf(fname, "/dev/X/server.%s.pid", display);

        /*
         * First check whether there is such a file, and if so check
         * for the server whether it is running, or just a dead file ...
         */
        if ((fp = fopen(fname, "r"))) 
        {
            fscanf(fp, "%d", &pid);

            if (kill(pid, 0) == 0) 
            {
                sprintf(fname, "/dev/X/server.%s", display);
                if (open(fname, O_RDWR) != -1) 
                {
                    ErrorF(MSGSCO(XSCO_7,"Server on display %s is already running\n"), 
                           display);
                    exit(2);				/*	S049	*/
                }
            }
        }

        /*
         * Ok, no other server is running, just setup all files for this
         * new one ....
         */
        fclose(fp);
        sprintf(fname, "/dev/X/Nserver.%s",    display); unlink(fname);
        sprintf(fname, "/dev/X/server.%s",     display); unlink(fname);
        sprintf(fname, "/dev/X/server.%s.pid", display); unlink(fname);

        if (!mkdir("/dev/X", 0777)) chmod("/dev/X", 0777);

        if ((fp = fopen(fname, "w"))) 
        {
            fprintf(fp, "%-5ld\n", getpid());
            fclose(fp);
        }
        else
        {
            ErrorF(MSGSCO(XSCO_8,"Cannot write to %s\n"), fname);
        }

							/*	S052 vvv */
        if ((fd = open("/dev/tty", O_WRONLY | O_NOCTTY)) <0) {
		ConsoleDevice = 0;	/* No, it is not a console device */
	} else {

		ioRtn = ioctl( fd, KIOCINFO, 0 );

		if ( ioRtn < 0 ) {
			ConsoleDevice = 0;	/*	No it is not	*/
		} else {
			ConsoleDevice = 1;	/*	Yes it is	*/
		}
		close( fd );
	}

		/*	command line options may have set TTYName	*/
	TargetTTYName = GetTTYName();

	if ( (! strcmp( TargetTTYName, "/dev/console" )) ||
		(! strcmp( TargetTTYName, "/dev/syscon" )) ||
		(! strcmp( TargetTTYName, "/dev/systty" )) ||
		(! strcmp( TargetTTYName, "/dev/vt00" )) ) {
		DevConsole = 1;		/*	Yes, this is the console */
		CRTSpecified = 0;  /*  do not honor -crt /dev/console  S055 */
	} else {
		DevConsole = 0;		/*	No, this is NOT console	*/
	}
        

/*
 *	CRTSpecified - YES - go to it, no questions asked
 *	CRTSpecified - No - then
 *		DevConsole - YES - find free VT
 *		DevConsole - No - then
 *			ConsoleDevice - No - find free VT
 *			ConsoleDevice - YES - assume this is the CRTSpecified
 *						open it and use it.
 */

	if ( (! CRTSpecified) && (DevConsole || (!ConsoleDevice)) ) {
		/*	We need to find a free VT	*/

        if ((fd = open("/dev/console", O_WRONLY | O_NOCTTY)) <0) 
            FatalError(MSGSCO(XSCO_13,"Cannot open %s\n"), "/dev/console" );
        
        if (ioctl(fd, VT_OPENQRY, &scoSysInfo.currentVT) < 0 ||
            scoSysInfo.currentVT == -1) 
            FatalError(MSGSCO(XSCO_14,"Cannot find a free VT\n"));

        close(fd);
        
        sprintf(fname, "/dev/vt%02d", scoSysInfo.currentVT);
        SetTTYName(fname);


        /**************/
        fclose(stdin);
        fclose(stdout);
        /***************/

        /*
         * For XQUEUE device we MUST force a new process group in order to
         * let the kd driver be attached to the mouse device ...
         */
        setpgrp ();

	} else {	/*	no need to look for a free VT, just do it */
        	sprintf(fname, "%s", TargetTTYName );
	}
				/*			S052 ^^^	*/

        if ((scoSysInfo.consoleFd = open(fname, O_RDWR | O_NONBLOCK)) < 0)
            FatalError (MSGSCO(XSCO_13,"Couldn't open %s\n"), fname);

        if ((scoSysInfo.defaultMode = ioctl(scoSysInfo.consoleFd, CONS_GET, 0)) < 0)
            FatalError(MSGSCO(XSCO_15,"CONS_GET failed\n"));

        if (ioctl(scoSysInfo.consoleFd, VT_GETMODE, &VT) < 0) 
            FatalError (MSGSCO(XSCO_16,"VT_GETMODE failed\n"));

        VT.mode = VT_PROCESS;
        VT.relsig = SIGUSR2;
        VT.acqsig = SIGUSR2;

        if (ioctl(scoSysInfo.consoleFd, VT_SETMODE, &VT) < 0) 
            FatalError (MSGSCO(XSCO_17,"VT_SETMODE VT_PROCESS failed\n"));

        if (ioctl(scoSysInfo.consoleFd, KDSETMODE, KD_GRAPHICS) < 0)
            FatalError (MSGSCO(XSCO_18,"KDSETMODE KD_GRAPHICS failed\n"));

        scoSysInfo.screenActive = FALSE;
        scoSysInfo.switchReqPending = FALSE;
        scoSysInfo.inputPending = FALSE;
        scoSysInfo.reset = FALSE;

        /* S045 */
        if (getrlimit(RLIMIT_VMEM, &rl) == 0)
          {
            if (rl.rlim_cur < rl.rlim_max)
              {
                rl.rlim_cur = rl.rlim_max;
                if (setrlimit(RLIMIT_VMEM, &rl) < 0)
                  perror("RLIMIT_VMEM");
              }
          }
        else
          {
            perror("RLIMIT_VMEM");
          }
    }
    else if (scoSysInfo.screenActive == FALSE)
      {
        scoSysInfo.reset = TRUE;
        while (scoSysInfo.screenActive == FALSE)
          pause();                  /* wait until we acquire the screen*/
        scoSysInfo.reset = FALSE;
      }
#endif /* usl */
}

/*-
 *-----------------------------------------------------------------------
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *
 * Results:
 *	screenInfo init proc field set
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/* S019 Start */
void
InitOutput( ScreenInfo *screenInfo, int argc, char **argv )
{
    int i, j, k;
    int imageByteOrder;
    int bitmapScanlineUnit;
    int bitmapScanlinePad;
    int bitmapBitOrder;
    int numPixmapFormats ;
    extern Bool scoResetServer;

#if !defined(usl)
    ScalePopMessage = scoPopMessage; /* S038 */
    ScaleHideMessage = scoHideMessage; /* S038 */
#endif /* !usl */

    ddxHandleDelayedArgs(argc, argv) ;

    scoInitVid();

    scoSysInfo.numScreens = 0;  /* S024 */

    screenInfo->imageByteOrder = IMAGE_BYTE_ORDER ;
    screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT ;
    screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD ;
    screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER ;

    screenInfo->numPixmapFormats = 0;

    /*
     * We always need at least this
     */
    ddxAddPixmapFormat(1, 1, 32); /* S025 */

    /* try for a dynamic screen */
    if ( (ddxNumActiveScreens<1) && (!ddxOpenScreens()) ) {
	FatalError(MSGSCO(XSCO_19, "Couldn't find any screens to open\n"));
        /* S039 */
	/* NOTREACHED */
    }

    ddxInitPixmapFormats(screenInfo); /* S025 */

    ddxInitBitSwap();           /* S033 */

    if ( ddxNumActiveScreens > 0 ) {
	for ( i = 0 ; i < ddxNumActiveScreens ; i++ )
	    AddScreen( ddxActiveScreens[i]->screenInit, argc, argv ) ;
    }
    else {
        ddxUseMsg() ;
        ErrorF(MSGSCO(XSCO_9,
                      "Fatal Error! X server couldn't find a screen to use\n" )) ;
        catclose(xsrv_m_catd);  /* S039 ^^^ */
        exit( 1 ) ;
        /* NOTREACHED */
    }

    if ( screenInfo->numScreens != scoSysInfo.numScreens) { /* S024 vvv */
	FatalError(MSGSCO(XSCO_20,"scoSysInfoInit() was not called for every screen!\n"));
	/* NOTREACHED */
    }

    /* Now add our wrappers.  We override everyone */
    for (i = 0; i < screenInfo->numScreens; i++) {
	scoScreenInit(screenInfo->screens[i]); /* S033 */
    } /* S024 ^^^ */

    /* S018, S016 - Ready for screen unblank */
    scoStatusRecord |= ReadyForUnblank;

#if defined(usl)
    RegisterBlockAndWakeupHandlers((BlockHandlerProcPtr)scoBlock,
                                   (WakeupHandlerProcPtr)scoWakeup,
                                   (void *)0);
#endif /* usl */

    (void) OsSignal(SIGWINCH, SIG_IGN);

    if (scoResetServer)
        GiveUp(15);

    return ;

} /* S019 End */

/*-
 *-----------------------------------------------------------------------
 * InitInput --
 *	Initialize all supported input devices...what else is there
 *	besides pointer and keyboard?
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Two DeviceRec's are allocated and registered as the system pointer
 *	and keyboard devices.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
InitInput(argc, argv)
    int     	  argc;
    char    	  **argv;
{
    DevicePtr p, k;
    static Bool done = FALSE;
    char buf[15];

#ifdef XTESTEXT1                /* S023 */
    xtest_command_key = XK_Print;
#endif

	if( XqueCheckMouse() )	/*	S050	*/
		scoAbortServerN(3);	/*	S050	*/

    p = AddInputDevice(scoMouseProc, TRUE);
    k = AddInputDevice(scoKbdProc, TRUE);

    if (!p || !k)
	FatalError(MSGSCO(XSCO_21,"failed to create input devices in InitInput"));

#if defined(usl)
    scoSysInfo.pPointer = (void*)p;
    scoSysInfo.pKeyboard =(void*)k;
#endif

    RegisterPointerDevice(p);
    RegisterKeyboardDevice(k);

    /* 
     * We always use the mi pointer now.
     */
    miRegisterPointerDevice((ScreenPtr)NULL, p);

    if (!mieqInit (k, p))       /* S033 */
	return;

    /* This could be controlled by the lock extension */
    scoSysInfo.switchEnabled = TRUE; /* S024 */

#ifdef XMERGE
    /*
     * initialize screen switching data structures - S029
     */
    scoInitScreenSwitch();

    /*
     *  Hmmmm.  This should really be reset when the server resets, right?
     *  Or maybe just when the user logs out of xdm.  Right now, the
     *  console driver remains that way until reboot
     */
    if( GetMergeScreenSwitchSeq(buf, scoSysInfo.consoleFd))
	    SetServerSWSequence(buf);
#endif
    
#if defined(usl)
    (void) OsSignal(SIGUSR2, scoVTRequest); /* screen-switch */
#endif

}

/*							   vvv S051
 *	memconmap is located in hw/sco/grafinfo/MemConMap.c
 */
void *
memconmap( void *addr, size_t len, int prot, int flags, int fd,
          off_t off, int ioctlFd, int DoConMemMap, int DoZero );

/*							   ^^^ S051 */

#include <sys/vid.h>						/* S027 */
#include <sys/tss.h>
#include <sys/immu.h>
#include <sys/region.h>
#include <sys/proc.h>
#if ! defined(usl)			/*	S053	*/
#include <sys/v86.h>
#endif					/*	S053	*/
#include <sys/sysi86.h>

void
scoInitVid()
{
    struct vid_info info;       /* S027 */
    int mpx_fd;                 /* S032 */

#if defined(usl)
    struct kd_memloc	vt_map;
    struct kd_disparam 	dp;
    int 		disp_type;
    int			fd;
    caddr_t	        addr;
    int			videoFd;	/*	S047	*/
    int			cmmRet;		/*	S047	*/
    pid_t		serverPid;	/*	S047	*/
#endif /* usl */

    if (serverGeneration > 1)
	return;

#if defined(usl)
    /*
     * now force to get the VT
     */
    if (ioctl(scoSysInfo.consoleFd, VT_ACTIVATE, scoSysInfo.currentVT) != 0)
    {
        ErrorF(MSGSCO(XSCO_10,"VT_ACTIVATE failed\n"));
    }

    if (ioctl(scoSysInfo.consoleFd, VT_WAITACTIVE, scoSysInfo.currentVT) != 0)
    {
        ErrorF(MSGSCO(XSCO_11,"VT_WAITACTIVE failed\n"));
    }

    fd = open("/dev/zero",O_RDWR);
    if (fd < 0) {
        FatalError(MSGSCO(XSCO_22,"OsVendorInit: Cannot open /dev/zero\n"));
    }

    addr = memconmap( (void *)0, (size_t) 1024*1024, PROT_READ|PROT_WRITE,
	MAP_PRIVATE|MAP_FIXED, fd, (off_t) 0, 0, 0, 0 );	/* S051 */

    if (addr == (caddr_t)-1) {
        close(fd);
        FatalError(MSGSCO(XSCO_23,"OsVendorInit: Cannot mmap the 1 Meg memory space\n"));
    }

    close(fd);

    fd = open("/dev/pmem",O_RDWR);
    if (fd < 0) {
        FatalError(MSGSCO(XSCO_24,"Cannot open /dev/pmem\n"));
    }


    videoFd = open("/dev/video", O_RDWR);	/*	S047 vvv	*/
    if (videoFd < 0) {
        close(fd);
        FatalError("Cannot open %s\n", "/dev/video");
    }

    addr = memconmap( (void *)0xA0000, (size_t) 0x20000, PROT_READ|PROT_WRITE,
	MAP_SHARED|MAP_FIXED, fd, (off_t) 0xA0000, videoFd, 1, 0 );  /* S051 */

    if (addr == (caddr_t)-1) {
        close(videoFd);		/*	S047	*/
        close(fd);
        FatalError(MSGSCO(XSCO_25,"OsVendorInit: Cannot mmap VGA video memory\n"));
    }

    addr = memconmap( (void *)0xC0000, (size_t) 0x20000, PROT_READ,
	MAP_SHARED|MAP_FIXED, fd, (off_t) 0xC0000, videoFd, 1, 0 ); /* S051 */

    if (addr == (caddr_t)-1) {
        close(videoFd);		/*	S047	*/
        close(fd);
        FatalError(MSGSCO(XSCO_26,"OsVendorInit: Cannot mmap VGA ROM memory\n"));
    }

    close(videoFd);		/*	S047	*/
    close(fd);
    set_iopl();

    scoSysInfo.screenActive = TRUE;

#endif /* usl */

#if !defined(usl)
    scoVidOldMode = ioctl(1, CONS_GET, 0);

    /*
     * this call requests unlimited i/o privileges
     * via setting the IOPL bits in the ps register.
     * i/o is faster when the cpu doesn't have
     * to check the port bitmap in the tss.
     * this is a root-only system call (at least as of unix 3.2v2).
     */
    if (sysi86(SI86V86,V86SC_IOPL,0x3000) < 0) /* S040 vvv */
    {
	char *errstr;

	switch (errno)
        {
          case ENOPKG:
            errstr =
                MSGSYS(XSRV_NOVXDRV,
                      "The X Server requires the 'vx' driver to be linked into the kernel.\n");
            break;

          case EPERM:
            errstr =
                MSGSYS(XSRV_NOTROOT,
                      "The X Server must be run as root or setuid root.\n");
            break;

          default:
            Error("sysi86");	/* prints errno */
            errstr = "sysi86(SI86V86,V86SC_IOPL,0x3000) failed\n";
        }

	FatalError(errstr);

	/* NOTREACHED */

    } /* S040 ^^^ */
    
    /* 
     * If MPX, lock us on to a CPU				S032  vvvv
     */
    if (mpxLock && (mpx_fd = open(MPXNAME, O_RDONLY)) > 0)
    {
	if (ioctl(mpx_fd, ACPU_XLOCK, BASECPU) < 0) /* S041 */
	    ErrorF(MSGSCO(XSCO_12,"Couldn't lock process on %s\n"), MPXNAME);
	close(mpx_fd);          /* S035 */
    } /* S032 ^^^ */
    
    /* Use this fd for Screen Switching */			/* S014 */
    scoSetSSFD(1);

    /* S015 vvvv */
    /* Take over Screen Switching */
    scoEnterVtProcMode();
    
    /* Gain the screen */
    scoBecomeActiveScreen();
    /* S015 ^^^^ */

    info.size = sizeof info;    /* S028 */
    ioctl(1, CONS_GETINFO, &info); /* S027 */
    scoSysInfo.currentVT = info.m_num; /* S027 */
#endif /*	! usl	*/
}

scoRestoreVid()
{                               /* S026 */
#if defined(usl)

	xUnMapAll();		/* S051 */

    if ((scoSysInfo.defaultMode > 0) && (ioctl(1, CONS_GET, 0) != scoSysInfo.defaultMode))
	ioctl(1, MODESWITCH | scoSysInfo.defaultMode, 0);
#else
    if ((scoVidOldMode > 0) && (ioctl(1, CONS_GET, 0) != scoVidOldMode))
	ioctl(1, MODESWITCH | scoVidOldMode, 0);
#endif
}

/* S024 vvvv */
/*
 *  scoSysInfoInit(ScreenPtr pScreen, scoScreenInfo *pSysInfo)
 *
 *  This call initializes the SCO SysInfo  structure from the
 *  deviced dependent layer IE xxxInit.c.
 */
void
scoSysInfoInit(ScreenPtr pScreen, scoScreenInfo *pSysInfo)
{
    int screen = pScreen->myNum;

    scoSysInfo.scoScreens[screen] = 
        (scoScreenInfo *) Xalloc(sizeof(scoScreenInfo));

    *scoSysInfo.scoScreens[screen] = *pSysInfo; /* struct copy */

    scoSysInfo.scoScreens[screen]->pScreen = pScreen;

    scoSysInfo.numScreens++;

}
/* S024 ^^^^ */

/*  									S031
 *  This just forces the linker to pull in the .o file with these 
 *  references
 */
__dummyFunctionForLinker()
{
#ifdef XTEST
    XTestGenerateEvent();
#endif
}


#if defined(usl)

#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>

#define	SI86IOPL	   112		/* in sysi86.h, for ESMP */

static void
set_iopl()
{
    _abi_sysi86(SI86IOPL, PS_IOPL>>12);
}
#endif /* usl */

