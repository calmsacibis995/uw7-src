/* $XConsortium: xf86Xinput.c /main/3 1996/01/14 19:01:42 kaleb $ */
/*
 * Copyright 1995 by Frederic Lepied, France. <fred@sugix.frmug.fr.net>       
 *                                                                            
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.                   
 *                                                                            
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Xinput.c,v 3.2 1996/01/14 13:35:10 dawes Exp $ */

#include "XI.h"
#include "XIproto.h"
#include "xf86.h"
#include "osdep.h"
#include "xf86Priv.h"
#include "xf86Xinput.h"
#include "xf86_Config.h"

#include <stdarg.h>

extern InputInfo inputInfo;

#ifdef JOYSTICK_SUPPORT
extern DeviceAssocRec   joystick_assoc;
#endif
#ifdef WACOM_SUPPORT
extern DeviceAssocRec   wacom_stylus_assoc;
extern DeviceAssocRec   wacom_cursor_assoc;
extern DeviceAssocRec   wacom_eraser_assoc;
#endif
#ifdef ELOGRAPHICS_SUPPORT
extern DeviceAssocRec	elographics_assoc;
#endif

static int              num_devices;
static LocalDevicePtr	*localDevices;
static int              max_devices;
static int              num_assoc;

static DeviceAssocPtr   deviceAssoc[] =
{
#ifdef JOYSTICK_SUPPORT
  &joystick_assoc,
#endif
#ifdef WACOM_SUPPORT
  &wacom_stylus_assoc,
  &wacom_cursor_assoc,
  &wacom_eraser_assoc,
#endif
#ifdef ELOGRAPHICS_SUPPORT
  &elographics_assoc,
#endif
};

static SymTabRec XinputTab[] = {
  { ENDSECTION, "endsection"},
  { SUBSECTION,	"subsection" },
  { -1,		"" },
};

int
IsCorePointer(DeviceIntPtr	device)
{
  return(device == inputInfo.pointer);
}

int
IsCoreKeyboard(DeviceIntPtr	device)
{
  return(device == inputInfo.keyboard);
}

/***********************************************************************
 *
 * ReadInput --
 *	Wakeup handler to catch input and dispatch it to our
 *	input routines if necessary.
 *
 ***********************************************************************
 */
static void
ReadInput(pointer	block_data,
	  int		select_status,
	  pointer	read_mask)
{
  int			i;
  LocalDevicePtr	local_dev;
  long			devices_with_input[mskcnt];
  extern long		EnabledDevices[];

  if (select_status < 1)
    return;

  MASKANDSETBITS(devices_with_input, ((long *) read_mask), EnabledDevices);
  if (!ANYSET(devices_with_input))
    return;

  for (i = 0; i < num_devices; i++) {
    local_dev = localDevices[i];
    if (local_dev->read_input &&
	(local_dev->fd >= 0) &&
        (GETBIT(((long *) read_mask), local_dev->fd) != 0)) {
      (*local_dev->read_input)(local_dev);
      break;
    }
  }
}

/***********************************************************************
 *
 * configExtendedInputSection --
 *
 ***********************************************************************
 */

void
configExtendedInputSection(LexPtr       val)
{
  int           i;
  int           token;
  extern int    xf86GetToken(SymTabRec tab[]);

  num_assoc = sizeof(deviceAssoc) / sizeof(DeviceAssocPtr);
  num_devices = 0;
  max_devices = 3;
  localDevices = (LocalDevicePtr*) xalloc(sizeof(LocalDevicePtr)*max_devices);
  
  while ((token = xf86GetToken(XinputTab)) != ENDSECTION)
    {
      if (token == SUBSECTION)
        {
          int   found = 0;
          
          if (xf86GetToken(NULL) != STRING)
            xf86ConfigError("SubSection name expected");
          
          for(i=0; !found && i<num_assoc; i++)
            {
              if (StrCaseCmp(val->str, deviceAssoc[i]->config_section_name) == 0)
                {
                  if (num_devices == max_devices) {
                    max_devices *= 2;
                    localDevices = (LocalDevicePtr*) xrealloc(localDevices,
                                                              sizeof(LocalDevicePtr)*max_devices);
                  }
                  localDevices[num_devices] = deviceAssoc[i]->device_allocate();
                  
                  if (localDevices[num_devices] && localDevices[num_devices]->device_config) 
                    {
                      (*localDevices[num_devices]->device_config)(localDevices,
                                                                  num_devices,
                                                                  num_devices+1,
                                                                  val);
                      localDevices[num_devices]->flags |= XI86_CONFIGURED;
                      num_devices++;
                    }
                  found = 1;
                }
            }
          if (!found)
            xf86ConfigError("Invalid SubSection name");
        }
      else
        xf86ConfigError("Xinput keyword section expected");        
    }
}

/***********************************************************************
 *
 * InitExtInput --
 * 
 *	Initialize any extended devices we might have. It is called from
 * ddx InitInput.
 *
 ***********************************************************************
 */

void
InitExtInput()
{
  DeviceIntPtr	dev;
  int		i;

  /* Register a Wakeup handler to handle input when generated */
  RegisterBlockAndWakeupHandlers((BlockHandlerProcPtr) NoopDDA, ReadInput,
                                 NULL);

  /* Add each device */
  for (i = 0; i < num_devices; i++) {
    if (localDevices[i]->flags & XI86_CONFIGURED) {
      dev = (DeviceIntPtr) AddInputDevice(localDevices[i]->device_control,
                                          (localDevices[i]->flags & XI86_NO_OPEN_ON_INIT) ? FALSE : TRUE);
      if (dev == NULL)
        FatalError("Too many input devices");
      localDevices[i]->atom = MakeAtom(localDevices[i]->name, strlen(localDevices[i]->name), TRUE);
      dev->public.devicePrivate = (pointer) localDevices[i];
      localDevices[i]->dev = dev;
      RegisterOtherDevice(dev);
    }
  }
}


/***********************************************************************
 *
 * Caller:	ProcXOpenDevice
 *
 * This is the implementation-dependent routine to open an input device.
 * Some implementations open all input devices when the server is first
 * initialized, and never close them.  Other implementations open only
 * the X pointer and keyboard devices during server initialization,
 * and only open other input devices when some client makes an
 * XOpenDevice request.  This entry point is for the latter type of 
 * implementation.
 *
 * If the physical device is not already open, do it here.  In this case,
 * you need to keep track of the fact that one or more clients has the
 * device open, and physically close it when the last client that has
 * it open does an XCloseDevice.
 *
 * The default implementation is to do nothing (assume all input devices
 * are opened during X server initialization and kept open).
 *
 ***********************************************************************
 */

void
OpenInputDevice (dev, client, status)
    DeviceIntPtr dev;
    ClientPtr client;
    int *status;
{
  LocalDevicePtr        local = (LocalDevicePtr)dev->public.devicePrivate;

  if (local->flags & XI86_NO_OPEN_ON_INIT) {
    dev->startup = TRUE;
  }
}


/***********************************************************************
 *
 * Caller:	ProcXChangeKeyboardDevice
 *
 * This procedure does the implementation-dependent portion of the work
 * needed to change the keyboard device.
 *
 * The X keyboard device has a FocusRec.  If the device that has been 
 * made into the new X keyboard did not have a FocusRec, 
 * ProcXChangeKeyboardDevice will allocate one for it.
 *
 * If you do not want clients to be able to focus the old X keyboard
 * device, call DeleteFocusClassDeviceStruct to free the FocusRec.
 *
 * If you support input devices with keys that you do not want to be 
 * used as the X keyboard, you need to check for them here and return 
 * a BadDevice error.
 *
 * The default implementation is to do nothing (assume you do want
 * clients to be able to focus the old X keyboard).  The commented-out
 * sample code shows what you might do if you don't want the default.
 *
 ***********************************************************************
 */

int
ChangeKeyboardDevice (old_dev, new_dev)
     DeviceIntPtr	old_dev;
     DeviceIntPtr	new_dev;
{
  /**********************************************************************
   * DeleteFocusClassDeviceStruct(old_dev);	 * defined in xchgptr.c *
   **********************************************************************/
  return !Success;
}


/***********************************************************************
 *
 * Caller:	ProcXChangePointerDevice
 *
 * This procedure does the implementation-dependent portion of the work
 * needed to change the pointer device.
 *
 * The X pointer device does not have a FocusRec.  If the device that
 * has been made into the new X pointer had a FocusRec, 
 * ProcXChangePointerDevice will free it.
 *
 * If you want clients to be able to focus the old pointer device that
 * has now become accessible through the input extension, you need to 
 * add a FocusRec to it here.
 *
 * The XChangePointerDevice protocol request also allows the client
 * to choose which axes of the new pointer device are used to move 
 * the X cursor in the X- and Y- directions.  If the axes are different
 * than the default ones, you need to keep track of that here.
 *
 * If you support input devices with valuators that you do not want to be 
 * used as the X pointer, you need to check for them here and return a 
 * BadDevice error.
 *
 * The default implementation is to do nothing (assume you don't want
 * clients to be able to focus the old X pointer).  The commented-out
 * sample code shows what you might do if you don't want the default.
 *
 ***********************************************************************
 */

int
ChangePointerDevice (old_dev, new_dev, x, y)
     DeviceIntPtr	old_dev, new_dev;
     unsigned char	x, y;
{
  /************************************************************************
    InitFocusClassDeviceStruct(old_dev);	* allow focusing old ptr*
    
    x_axis = x;					* keep track of new x-axis*
    y_axis = y;					* keep track of new y-axis*
    if (x_axis != 0 || y_axis != 1)
    axes_changed = TRUE;			* remember axes have changed*
    else
    axes_changed = FALSE;
   *************************************************************************/
  /*
   * We don't allow axis swap or other exotic features.
   */
  if (x == 0 && y == 1) {
    InitFocusClassDeviceStruct(old_dev);
    return Success;
  }
  else
    return !Success;
}


/***********************************************************************
 *
 * Caller:	ProcXCloseDevice
 *
 * Take care of implementation-dependent details of closing a device.
 * Some implementations may actually close the device, others may just
 * remove this clients interest in that device.
 *
 * The default implementation is to do nothing (assume all input devices
 * are initialized during X server initialization and kept open).
 *
 ***********************************************************************
 */

void
CloseInputDevice (d, client)
     DeviceIntPtr d;
     ClientPtr client;
{
  ErrorF("ProcXCloseDevice to close or not ?\n");
}


/***********************************************************************
 *
 * Caller:	ProcXListInputDevices
 *
 * This is the implementation-dependent routine to initialize an input 
 * device to the point that information about it can be listed.
 * Some implementations open all input devices when the server is first
 * initialized, and never close them.  Other implementations open only
 * the X pointer and keyboard devices during server initialization,
 * and only open other input devices when some client makes an
 * XOpenDevice request.  If some other process has the device open, the
 * server may not be able to get information about the device to list it.
 *
 * This procedure should be used by implementations that do not initialize
 * all input devices at server startup.  It should do device-dependent
 * initialization for any devices not previously initialized, and call
 * AddInputDevice for each of those devices so that a DeviceIntRec will be 
 * created for them.
 *
 * The default implementation is to do nothing (assume all input devices
 * are initialized during X server initialization and kept open).
 * The commented-out sample code shows what you might do if you don't want 
 * the default.
 *
 ***********************************************************************
 */

void
AddOtherInputDevices ()
{
}


/****************************************************************************
 *
 * Caller:	ProcXSetDeviceMode
 *
 * Change the mode of an extension device.
 * This function is used to change the mode of a device from reporting
 * relative motion to reporting absolute positional information, and
 * vice versa.
 * The default implementation below is that no such devices are supported.
 *
 ***********************************************************************
 */

int
SetDeviceMode (client, dev, mode)
     register	ClientPtr	client;
     DeviceIntPtr dev;
     int		mode;
{
  LocalDevicePtr        local = (LocalDevicePtr)dev->public.devicePrivate;

  if (local->switch_mode) {
    return (*local->switch_mode)(client, dev, mode);
  }
  else
    return BadMatch;
}


/***********************************************************************
 *
 * Caller:	ProcXSetDeviceValuators
 *
 * Set the value of valuators on an extension input device.
 * This function is used to set the initial value of valuators on
 * those input devices that are capable of reporting either relative
 * motion or an absolute position, and allow an initial position to be set.
 * The default implementation below is that no such devices are supported.
 *
 ***********************************************************************
 */

int
SetDeviceValuators (client, dev, valuators, first_valuator, num_valuators)
     register ClientPtr	client;
     DeviceIntPtr 	dev;
     int		*valuators;
     int		first_valuator;
     int		num_valuators;
{
  return BadMatch;
}


/***********************************************************************
 *
 * Caller:	ProcXChangeDeviceControl
 *
 * Change the specified device controls on an extension input device.
 *
 ***********************************************************************
 */

int
ChangeDeviceControl (client, dev, control)
     register ClientPtr	client;
     DeviceIntPtr	dev;
     xDeviceCtl		*control;
{
  switch (control->control) {
  case DEVICE_RESOLUTION:
    return (BadMatch);
  default:
    return (BadMatch);
  }
}

/*
 * adapted from mieq.c to support extended events
 *
 */
extern	InputInfo 	inputInfo;

#define QUEUE_SIZE  256

typedef struct _Event {
    xEvent	event;
#ifdef XINPUT
  deviceValuator val;
#endif
    ScreenPtr	pScreen;
} EventRec, *EventPtr;

typedef struct _EventQueue {
    long	head, tail;	    /* long for SetInputCheck */
    CARD32	lastEventTime;	    /* to avoid time running backwards */
    Bool	lastMotion;
    EventRec	events[QUEUE_SIZE]; /* static allocation for signals */
    DevicePtr	pKbd, pPtr;	    /* device pointer, to get funcs */
    ScreenPtr	pEnqueueScreen;	    /* screen events are being delivered to */
    ScreenPtr	pDequeueScreen;	    /* screen events are being dispatched to */
} EventQueueRec, *EventQueuePtr;

static EventQueueRec xf86EventQueue;

Bool
xf86eqInit (pKbd, pPtr)
    DevicePtr	pKbd, pPtr;
{
    xf86EventQueue.head = xf86EventQueue.tail = 0;
    xf86EventQueue.lastEventTime = GetTimeInMillis ();
    xf86EventQueue.pKbd = pKbd;
    xf86EventQueue.pPtr = pPtr;
    xf86EventQueue.lastMotion = FALSE;
    xf86EventQueue.pEnqueueScreen = screenInfo.screens[0];
    xf86EventQueue.pDequeueScreen = xf86EventQueue.pEnqueueScreen;
    SetInputCheck (&xf86EventQueue.head, &xf86EventQueue.tail);
    return TRUE;
}

/*
 * Must be reentrant with ProcessInputEvents.  Assumption: xf86eqEnqueue
 * will never be interrupted.  If this is called from both signal
 * handlers and regular code, make sure the signal is suspended when
 * called from regular code.
 */

void
xf86eqEnqueue (e)
    xEvent	*e;
{
    int	oldtail, newtail, prevtail;
    Bool    isMotion;
#ifdef XINPUT
    int     count;

    switch (e->u.u.type)
      {
      case KeyPress:
      case KeyRelease:
      case ButtonPress:
      case ButtonRelease:
      case MotionNotify:
        count = 1;
        break;
      default:
        if (!((deviceKeyButtonPointer *) e)->deviceid & MORE_EVENTS)
          {
            count = 1;
          }
        else
          {
          count = 2;
          }
        break;
      }
#endif

    oldtail = xf86EventQueue.tail;
    isMotion = e->u.u.type == MotionNotify;
    if (isMotion && xf86EventQueue.lastMotion && oldtail != xf86EventQueue.head)
    {
	if (oldtail == 0)
	    oldtail = QUEUE_SIZE;
	oldtail = oldtail - 1;
    }
    else
    {
    	newtail = oldtail + 1;
    	if (newtail == QUEUE_SIZE)
	    newtail = 0;
    	/* Toss events which come in late */
    	if (newtail == xf86EventQueue.head)
	    return;
	xf86EventQueue.tail = newtail;
    }
    
    xf86EventQueue.lastMotion = isMotion;
    xf86EventQueue.events[oldtail].event = *e;
#ifdef XINPUT
    if (count == 2)
    {
      xf86EventQueue.events[oldtail].val = *((deviceValuator *) (((deviceKeyButtonPointer *) e)+1));
    }
#endif
    /*
     * Make sure that event times don't go backwards - this
     * is "unnecessary", but very useful
     */
    if (e->u.keyButtonPointer.time < xf86EventQueue.lastEventTime &&
	xf86EventQueue.lastEventTime - e->u.keyButtonPointer.time < 10000)
    {
	xf86EventQueue.events[oldtail].event.u.keyButtonPointer.time =
	    xf86EventQueue.lastEventTime;
    }
    xf86EventQueue.events[oldtail].pScreen = xf86EventQueue.pEnqueueScreen;
}

/*
 * Call this from ProcessInputEvents()
 */

xf86eqProcessInputEvents ()
{
    EventRec	*e;
    int		x, y;
    xEvent	xe;
#ifdef XINPUT
    DeviceIntPtr                dev;
    int                         id, count;
    deviceKeyButtonPointer      *dev_xe;
    extern DeviceIntPtr         LookupDeviceIntRec();
#endif

    while (xf86EventQueue.head != xf86EventQueue.tail)
    {
	extern int  screenIsSaved;

	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens (SCREEN_SAVER_OFF, ScreenSaverReset);

	e = &xf86EventQueue.events[xf86EventQueue.head];
	/*
	 * Assumption - screen switching can only occur on motion events
	 */
	if (e->pScreen != xf86EventQueue.pDequeueScreen)
	{
	    xf86EventQueue.pDequeueScreen = e->pScreen;
	    x = e->event.u.keyButtonPointer.rootX;
	    y = e->event.u.keyButtonPointer.rootY;
	    if (xf86EventQueue.head == QUEUE_SIZE - 1)
	    	xf86EventQueue.head = 0;
	    else
	    	++xf86EventQueue.head;
	    NewCurrentScreen (xf86EventQueue.pDequeueScreen, x, y);
	}
	else
	{
	    xe = e->event;
	    if (xf86EventQueue.head == QUEUE_SIZE - 1)
	    	xf86EventQueue.head = 0;
	    else
	    	++xf86EventQueue.head;
	    switch (xe.u.u.type) 
	    {
	    case KeyPress:
	    case KeyRelease:
	    	(*xf86EventQueue.pKbd->processInputProc)
				(&xe, (DeviceIntPtr)xf86EventQueue.pKbd, 1);
	    	break;
#ifdef XINPUT
            case ButtonPress:
            case ButtonRelease:
            case MotionNotify:
	    	(*(inputInfo.pointer->public.processInputProc))
				(&xe, (DeviceIntPtr)inputInfo.pointer, 1);
                  break;

	    default:
              dev_xe = (deviceKeyButtonPointer *) e;
              id = dev_xe->deviceid & DEVICE_BITS;
              if (!(dev_xe->deviceid & MORE_EVENTS)) {
                count = 1;
              } else {
                count = 2;
              }
              dev = LookupDeviceIntRec(id);
              if (dev == NULL)
                {
                  ErrorF("LookupDeviceIntRec id=0x%x not found\n", id);
/*                   FatalError("xf86eqProcessInputEvents : device not found.\n");
 */
                  break;
                }
              if (!dev->public.processInputProc)
                {
                  FatalError("xf86eqProcessInputEvents : device has no input proc.\n");
                  break;
                }
              (*dev->public.processInputProc)(&e->event, dev, count);
#else
	    default:
	    	(*xf86EventQueue.pPtr->processInputProc)
				(&xe, (DeviceIntPtr)xf86EventQueue.pPtr, 1);
#endif
	    	break;
	    }
	}
    }
}

/* 
 * convenient functions to post events
 */

void
PostMotionEvent(DeviceIntPtr	device,
		int		is_absolute,
		int		first_valuator,
		int		num_valuators,
		...)
{
  va_list			var;
  int				loop;
  xEvent			xE[2];
  deviceKeyButtonPointer	*xev = (deviceKeyButtonPointer*) xE;
  deviceValuator		*xv = (deviceValuator*) xev+1;

  if (num_valuators > 6) {
    num_valuators = 6;
  }

  va_start(var, num_valuators);

  for(loop=0; loop<num_valuators; loop++) {
    switch (loop) {
    case 0:
      xv->valuator0 = va_arg(var, int);
      break;
    case 1:
      xv->valuator1 = va_arg(var, int);
      break;
    case 2:
      xv->valuator2 = va_arg(var, int);
      break;
    case 3:
      xv->valuator3 = va_arg(var, int);
      break;
    case 4:
      xv->valuator4 = va_arg(var, int);
      break;
    case 5:
      xv->valuator5 = va_arg(var, int);
      break;
    }
  }

  va_end(var);

  if (!IsCorePointer(device)) {
    extern int              DeviceMotionNotify;
    extern int              DeviceValuator;
            
    xev->type = DeviceMotionNotify;
    xev->detail = 0;
    xf86Info.lastEventTime = xev->time = GetTimeInMillis();
    xev->deviceid = device->id | MORE_EVENTS;
            
    xv->type = DeviceValuator;
    xv->deviceid = device->id;

    xv->num_valuators = num_valuators;
    xv->first_valuator = first_valuator;
    xv->device_state = 0;
    
    xf86eqEnqueue(xE);
  }
  else {
    xf86Info.lastEventTime = GetTimeInMillis();
    
    if (num_valuators >= 2) {
      if (is_absolute) {
	miPointerAbsoluteCursor(xv->valuator0, xv->valuator1, xf86Info.lastEventTime); 
      }
      else {
	miPointerDeltaCursor(xv->valuator0, xv->valuator1, xf86Info.lastEventTime);
      }
    }
  }  
}

void
PostProximityEvent(DeviceIntPtr	device,
		   int		is_in,
		   int		first_valuator,
		   int		num_valuators,
		   ...)
{
  va_list			var;
  int				loop;
  xEvent			xE[2];
  deviceKeyButtonPointer	*xev = (deviceKeyButtonPointer*) xE;
  deviceValuator		*xv = (deviceValuator*) xev+1;
  extern int			ProximityIn;
  extern int			ProximityOut;
  extern int			DeviceValuator;

  if (num_valuators > 6) {
    num_valuators = 6;
  }

  va_start(var, num_valuators);

  for(loop=0; loop<num_valuators; loop++) {
    switch (loop) {
    case 0:
      xv->valuator0 = va_arg(var, int);
      break;
    case 1:
      xv->valuator1 = va_arg(var, int);
      break;
    case 2:
      xv->valuator2 = va_arg(var, int);
      break;
    case 3:
      xv->valuator3 = va_arg(var, int);
      break;
    case 4:
      xv->valuator4 = va_arg(var, int);
      break;
    case 5:
      xv->valuator5 = va_arg(var, int);
      break;
    }
  }

  va_end(var);
  
  xev->type = is_in ? ProximityIn : ProximityOut;
  xev->detail = 0;
  xf86Info.lastEventTime = xev->time = GetTimeInMillis();
  xev->deviceid = device->id | MORE_EVENTS;
            
  xv->type = DeviceValuator;
  xv->deviceid = device->id;

  xv->num_valuators = num_valuators;
  xv->first_valuator = first_valuator;
  xv->device_state = 0;
    
  xf86eqEnqueue(xE);
}

void
PostButtonEvent(DeviceIntPtr	device,
		int		button,
		int		is_down,
		int		first_valuator,
		int		num_valuators,
		...)
{
  va_list			var;
  int				loop;
  xEvent			xE[2];
  deviceKeyButtonPointer	*xev = (deviceKeyButtonPointer*) xE;
  deviceValuator		*xv = (deviceValuator*) xev+1;

  if (num_valuators > 6) {
    num_valuators = 6;
  }

  va_start(var, num_valuators);

  for(loop=0; loop<num_valuators; loop++) {
    switch (loop) {
    case 0:
      xv->valuator0 = va_arg(var, int);
      break;
    case 1:
      xv->valuator1 = va_arg(var, int);
      break;
    case 2:
      xv->valuator2 = va_arg(var, int);
      break;
    case 3:
      xv->valuator3 = va_arg(var, int);
      break;
    case 4:
      xv->valuator4 = va_arg(var, int);
      break;
    case 5:
      xv->valuator5 = va_arg(var, int);
      break;
    }
  }

  va_end(var);

  if (!IsCorePointer(device)) {
    extern int              DeviceButtonPress;
    extern int              DeviceButtonRelease;
    extern int              DeviceValuator;
	    
    xev->type = is_down ? DeviceButtonPress : DeviceButtonRelease;
    xev->detail = button;
    
    xf86Info.lastEventTime = xev->time = GetTimeInMillis();
    xev->deviceid = device->id | MORE_EVENTS;
	    
    xv->type = DeviceValuator;
    xv->deviceid = device->id;
    xv->device_state = 0;
    xv->num_valuators = num_valuators;
    xv->first_valuator = first_valuator;
  }
  else {
    int       cx, cy;
    
    GetSpritePosition(&cx, &cy);
	    
    xE->u.u.type = is_down ? ButtonPress : ButtonRelease;
    xE->u.u.detail =  button;
    xE->u.keyButtonPointer.rootY = cx;
    xE->u.keyButtonPointer.rootX = cy;
    xf86Info.lastEventTime = xE->u.keyButtonPointer.time = GetTimeInMillis();
  }
  xf86eqEnqueue(xE);
}

/* end of xf86Xinput.c */
