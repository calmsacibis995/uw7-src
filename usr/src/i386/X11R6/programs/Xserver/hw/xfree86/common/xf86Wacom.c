/* $XConsortium: xf86Wacom.c /main/6 1996/01/26 13:37:08 kaleb $ */
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

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Wacom.c,v 3.5 1996/01/24 22:01:41 dawes Exp $ */

/*
 * This driver is only able to handle the Wacom IV protocol.
 */

#include "Xos.h"
#include <signal.h>

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "XI.h"
#include "XIproto.h"

#if defined(sun) && !defined(i386)
#define POSIX_TTY
#include <errno.h>
#include <termio.h>
#include <fcntl.h>
#include <ctype.h>

#include "extio.h"
#else
#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86Xinput.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"
#include "atKeynames.h"
#endif

#include "osdep.h"

/******************************************************************************
 * debugging macro
 *****************************************************************************/
#ifdef DBG
#undef DBG
#endif
#ifdef DEBUG
#undef DEBUG
#endif

static int      debug_level = 0;
#define DEBUG 1
#if DEBUG
#define DBG(lvl, f) {if ((lvl) <= debug_level) f;}
#else
#define DBG(lvl, f)
#endif

/******************************************************************************
 * device records
 *****************************************************************************/
#define DEVICE_ID(flags) ((flags) - ((flags >> 3) << 3))

#define STYLUS_ID		1
#define CURSOR_ID		2
#define ERASER_ID		4
#define ABSOLUTE_FLAG		8
#define FIRST_TOUCH_FLAG	16

typedef struct 
{
  char          *wcmDevice;     /* device file name */
  int		wcmSuppress;	/* transmit position if increment is superior */
  int           wcmOldX;        /* previous X position */
  int           wcmOldY;        /* previous Y position */
  int           wcmOldZ;        /* previous pressure */
  int           wcmOldProximity; /* previous proximity */
  int           wcmOldButtons;  /* previous buttons state */
  int           wcmOldCursorX;  /* previous cursor X position */
  int           wcmOldCursorY;  /* previous cursor Y position */
  int           wcmOldCursorZ;  /* previous cursor pressure */
  int           wcmOldCursorProximity; /* previous cursor proximity */
  int           wcmOldCursorButtons; /* previous cursor buttons state */
  int           wcmMaxX;        /* max X value */
  int           wcmMaxY;        /* max Y value */
  int           wcmMaxZ;        /* max Z value */
  int           wcmResolX;	/* X resolution in points/inch */
  int           wcmResolY;	/* Y resolution in points/inch */
  int           wcmResolZ;	/* Z resolution in points/inch */
  int		flags;		/* various flags */
  LocalDevicePtr wcmCursor;     /* cursor device ptr */
  LocalDevicePtr wcmStylus;     /* stylus device ptr */           
  LocalDevicePtr wcmEraser;     /* eraser device ptr */           
  int           wcmIndex;       /* number of bytes read */
  unsigned char wcmData[7];     /* data read on the device */
} WacomDeviceRec, *WacomDevicePtr;

/******************************************************************************
 * configuration stuff
 *****************************************************************************/
#define CURSOR_SECTION_NAME "wacomcursor"
#define STYLUS_SECTION_NAME "wacomstylus"
#define ERASER_SECTION_NAME "wacomeraser"
#define PORT		1
#define DEVICENAME	2
#define THE_MODE	3
#define SUPPRESS	4
#define DEBUG_LEVEL     5

#if !defined(sun) || defined(i386)
static SymTabRec WcmTab[] = {
  { ENDSUBSECTION,	"endsubsection" },
  { PORT,		"port" },
  { DEVICENAME,		"devicename" },
  { THE_MODE,		"mode" },
  { SUPPRESS,		"suppress" },
  { DEBUG_LEVEL,	"debuglevel" },
  { -1,			"" }
};

#define RELATIVE	1
#define ABSOLUTE	2

static SymTabRec ModeTabRec[] = {
  { RELATIVE,	"relative" },
  { ABSOLUTE,	"absolute" },
  { -1,		"" }
};
  
#endif

/******************************************************************************
 * constant and macros declarations
 *****************************************************************************/
#define BUFFER_SIZE 256		/* size of reception buffer */
#define XI_STYLUS "STYLUS"	/* X device name for the stylus */
#define XI_CURSOR "CURSOR"	/* X device name for the cursor */
#define XI_ERASER "ERASER"	/* X device name for the eraser */
#define MAX_VALUE 100           /* number of positions */
#define MAXTRY 50               /* max number of try to receive magic number */
#define SYSCALL(call) while(((call) == -1) && (errno == EINTR))

#define WC_RESET_IV	"#\r"	/* reset to wacom IV command set */
#define WC_CONFIG	"~R\r"	/* request a configuration string */
#define WC_COORD	"~C\r"	/* request max coordinates */
#define WC_MODEL	"~#\r"	/* request model and ROM version */

#define WC_MULTI	"MU1\r"	/* multi mode input */
#define WC_UPPER_ORIGIN	"OC1\r"	/* origin in upper left */
#define WC_SUPPRESS	"SU"	/* suppress mode */
#define WC_ALL_MACRO	"~M0\r"	/* enable all macro buttons */
#define WC_NO_MACRO1	"~M1\r"	/* disable macro buttons of group 1 */
#define WC_RATE 	"IT0\r"	/* max transmit rate (unit of 5 ms) */

static const char * setup_string = WC_MULTI WC_UPPER_ORIGIN WC_ALL_MACRO WC_NO_MACRO1
WC_RATE;

#define COMMAND_SET_MASK	0xc0
#define BAUD_RATE_MASK	0x0a
#define PARITY_MASK	0x30
#define DATA_LENGTH_MASK 0x40
#define STOP_BIT_MASK	0x80

#define HEADER_BIT	0x80
#define ZAXIS_SIGN_BIT	0x40
#define ZAXIS_BIT    	0x04
#define ZAXIS_BITS    	0x3f
#define POINTER_BIT     0x20
#define PROXIMITY_BIT   0x40
#define BUTTON_FLAG	0x08
#define BUTTONS_BITS	0x78

/******************************************************************************
 * external declarations
 *****************************************************************************/
#if defined(sun) && !defined(i386)
#define ENQUEUE suneqEnqueue
#else
#define ENQUEUE xf86eqEnqueue

extern void xf86eqEnqueue(
#if NeedFunctionPrototypes
    xEventPtr /*e*/
#endif
);
#endif

extern void miPointerDeltaCursor(
#if NeedFunctionPrototypes
    int /*dx*/,
    int /*dy*/,
    unsigned long /*time*/
#endif
);

#if !defined(sun) || defined(i386)
/*
 * xf86WcmConfig --
 *      Configure the device.
 */
static Bool
xf86WcmConfig(LocalDevicePtr    *array,
              int               index,
              int               max,
	      LexPtr            val)
{
  LocalDevicePtr        dev = array[index];
  WacomDevicePtr	priv = (WacomDevicePtr)(dev->private);
  int			token;
  int			mtoken;
  
  DBG(1, ErrorF("xf86WcmConfig\n"));
      
  /* Set defaults */
  priv->wcmOldX = -1;
  priv->wcmOldY = -1;
  priv->wcmOldButtons = -1;

  while ((token = xf86GetToken(WcmTab)) != ENDSUBSECTION) {
    switch(token) {
    case DEVICENAME:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Option string expected");
      dev->name = strdup(val->str);
      if (xf86Verbose)
	ErrorF("%s Wacom X device name is %s\n", XCONFIG_GIVEN, dev->name);
      break;
      
    case PORT:
      if (xf86GetToken(NULL) != STRING) xf86ConfigError("Option string expected");
      else {
        int     loop;
       
        /* try to find another wacom device which share the same port */
        for(loop=0; loop<max; loop++) {
          if (loop == index)
            continue;
          if ((array[loop]->device_config == xf86WcmConfig) &&
              (strcmp(((WacomDevicePtr)array[loop]->private)->wcmDevice, val->str) == 0)) {
            DBG(2, ErrorF("xf86WcmConfig wacom port share between %s and %s\n",
                          dev->name, array[loop]->name));
            xfree(priv);
            dev->private = array[loop]->private;
            break;
          }
        }
        if (loop == max) {
          priv->wcmDevice = strdup(val->str);
          if (xf86Verbose)
            ErrorF("%s Wacom port is %s\n", XCONFIG_GIVEN, priv->wcmDevice);
        }
      }
      break;

    case THE_MODE:
      mtoken = xf86GetToken(ModeTabRec);
      if ((mtoken == EOF) || (mtoken == STRING) || (mtoken == NUMBER)) 
	xf86ConfigError("Mode type token expected");
      else {
	switch (mtoken) {
	case ABSOLUTE:
	  dev->private_flags = dev->private_flags | ABSOLUTE_FLAG;
	  break;
	case RELATIVE:
	  dev->private_flags = dev->private_flags & ~ABSOLUTE_FLAG; 
	  break;
	default:
	  xf86ConfigError("Illegal Mode type");
	  break;
	}
      }
      break;

    case SUPPRESS:
      if (xf86GetToken(NULL) != NUMBER) xf86ConfigError("Option number expected");
      priv->wcmSuppress = val->num;
      if (xf86Verbose)
	ErrorF("%s Wacom suppress value is %d\n", XCONFIG_GIVEN, priv->wcmSuppress);      
      break;

    case DEBUG_LEVEL:
	if (xf86GetToken(NULL) != NUMBER)
	    xf86ConfigError("Option number expected");
	debug_level = val->num;
	if (xf86Verbose) {
#if DEBUG
	    ErrorF("%s Wacom debug level sets to %d\n", XCONFIG_GIVEN,
		   debug_level);      
#else
            ErrorF("%s Wacom debug level not sets to %d because debugging is not compiled\n", XCONFIG_GIVEN,
		   debug_level);      
#endif
	}
	break;

    case EOF:
      FatalError("Unexpected EOF (missing EndSubSection)");
      break;

    default:
      xf86ConfigError("Wacom subsection keyword expected");
      break;
    }
  }
  
  DBG(1, ErrorF("xf86WcmConfig name=%s\n", priv->wcmDevice));

  return Success;
}
#endif

/*
 * transform two ascii hexa representation into an unsigned char
 * most significant byte is the first one
 */
static unsigned char
ascii_to_hexa(char	buf[2])
{
  unsigned char	uc;
  
  if (buf[0] >= 'A') {
    uc = buf[0] - 'A' + 10;
  }
  else {
    uc = buf[0] - '0';
  }
  uc = uc << 4;
  if (buf[1] >= 'A') {
    uc += buf[1] - 'A' + 10;
  }
  else {
    uc += buf[1] - '0';
  }
  return uc;
}

/*
 * send a request and wait for the answer.
 * the answer must begin with the first two chars of the request and must end
 * with \r. The last character in the answer string (\r) is replaced by a \0.
 */
static char *
send_request(int	fd,
	     char	*request,
	     char	*answer)
{
  int	len, nr;
  
  /* send request string */
  SYSCALL(len = write(fd, request, strlen(request)));
  if (len == -1) Error("write");

  do {
    struct timeval	timeout;
    int			err;
    fd_set		readfds;
    int                 maxtry = MAXTRY;
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    SYSCALL(err = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout));
    switch (err) {
    case -1:
      Error("select");
      break;
    case 0:
      ErrorF("Timeout while reading tablet. No tablet connected ???\n");
      return NULL;
      break;
    }

    do {    
      SYSCALL(nr = read(fd, answer, 1));
      if ((nr == -1) && (errno != EAGAIN)) Error("read");
      maxtry--;
      
    } while ((answer[0] != request[0]) && maxtry);

    do {    
      SYSCALL(nr = read(fd, answer+1, 1));
      if ((nr == -1) && (errno != EAGAIN)) Error("read");

      if (answer[1] != request[1])
	answer[0] = answer[1];
      
    } while ((answer[0] == request[0]) &&
	     (answer[1] != request[1]));

  } while ((answer[0] != request[0]) &&
	   (answer[1] != request[1]));

  /* read until carriage return */
  len = 2;
  do {    
    SYSCALL(nr = read(fd, answer+len, 1));
    
    if (nr == -1) {
      if (errno != EAGAIN)
        Error("read");
    }
    else
      len += nr;
  } while (answer[len-1] != '\r');

  answer[len-1] = '\0';
  
  return answer;
}

/*
 * xf86WcmReadInput --
 *      Read the new events from the device, and enqueue them.
 */
static void
xf86WcmReadInput(LocalDevicePtr         local)
{
  WacomDevicePtr	priv = (WacomDevicePtr) local->private;
  int			len, loop;
  int			is_core_pointer, is_absolute;
  int			is_stylus, is_button, is_proximity;
  int			x, y, z, buttons, prox;
  int			*px, *py, *pz, *pbuttons, *pprox;
  DeviceIntPtr		device;
  unsigned char		buffer[BUFFER_SIZE];
  
  DBG(7, ErrorF("xf86WcmReadInput BEGIN device=%s fd=%d\n",
                priv->wcmDevice, local->fd));

  SYSCALL(len = read(local->fd, buffer, sizeof(buffer)));

  if (len <= 0)
    {
      Error("error reading wacom device");
      return;
    }

  for(loop=0; loop<len; loop++) {

    /* Format of 7 byte data packet for Wacom Tablets
       Byte 1
       bit 7  Sync bit always 1
       bit 6  Pointing device detected
       bit 5  Cursor = 0 / Stylus = 1
       bit 4  Reserved
       bit 3  1 if a button on the pointing device has been pressed
       bit 2  Reserved
       bit 1  X15
       bit 0  X14

       Byte 2
       bit 7  Always 0
       bits 6-0 = X13 - X7

       Byte 3
       bit 7  Always 0
       bits 6-0 = X6 - X0

       Byte 4
       bit 7  Always 0
       bit 6  B3
       bit 5  B2
       bit 4  B1
       bit 3  B0
       bit 2  P0
       bit 1  Y15
       bit 0  Y14

       Byte 5
       bit 7  Always 0
       bits 6-0 = Y13 - Y7

       Byte 6
       bit 7  Always 0
       bits 6-0 = Y6 - Y0

       Byte 7
       bit 7 Always 0
       bit 6  Sign of pressure data
       bit 5  P6
       bit 4  P5
       bit 3  P4
       bit 2  P3
       bit 1  P2
       bit 0  P1
       */
  
    if ((priv->wcmIndex == 0) && !(buffer[loop] & HEADER_BIT)) { /* magic bit is not OK */
      DBG(6, ErrorF("xf86WcmReadInput bad magic number 0x%x\n", buffer[loop]));;
      break;
    }

    priv->wcmData[priv->wcmIndex++] = buffer[loop];

    if (priv->wcmIndex == 7) {
      /* the packet is OK */

      x = (((priv->wcmData[0] & 0x3) << 14) + (priv->wcmData[1] << 7) + priv->wcmData[2]);      
      y = (((priv->wcmData[3] & 0x3) << 14) + (priv->wcmData[4] << 7) + priv->wcmData[5]);      
      prox = (priv->wcmData[0] & PROXIMITY_BIT);
	      
      /* check which device we have */
      is_stylus = (priv->wcmData[0] & POINTER_BIT);
	      
      z = ((priv->wcmData[6] & ZAXIS_BITS) * 2) + ((priv->wcmData[3] & ZAXIS_BIT) >> 2);
      if (priv->wcmData[6] & ZAXIS_SIGN_BIT)
	z *= -1;
	  
      is_button = (priv->wcmData[0] & BUTTON_FLAG);
      is_proximity = (priv->wcmData[0] & PROXIMITY_BIT);
      
      buttons = (priv->wcmData[3] & BUTTONS_BITS) >> 3;

      if (is_stylus) {
	pbuttons = &priv->wcmOldButtons;
	px = &priv->wcmOldX;
	py = &priv->wcmOldY;
	pz = &priv->wcmOldZ;
	pprox = &priv->wcmOldProximity;
	if (buttons > 3) {
	  local = priv->wcmEraser;
	}
	else {
	  local = priv->wcmStylus; 
	}
      }
      else {
	pbuttons = &priv->wcmOldCursorButtons;
	px = &priv->wcmOldCursorX;
	py = &priv->wcmOldCursorY;
	pz = &priv->wcmOldCursorZ;
	pprox = &priv->wcmOldCursorProximity;
	local = priv->wcmCursor;
      }
      device = local->dev;
      
      DBG(6, ErrorF("[%s] prox=%s\tx=%d\ty=%d\tz=%d\tbutton=%s\tbuttons=%d\n",
		    is_stylus ? "stylus" : "cursor", prox ? "true" : "false", x, y, z,
		    is_button ? "true" : "false", buttons));

      /* we can have only one device */
      if (!device)
	return;

      is_absolute = (local->private_flags & ABSOLUTE_FLAG);
      is_core_pointer = IsCorePointer(device);

      if (is_core_pointer) {
	x = x * screenInfo.screens[0]->width / priv->wcmMaxX;
	y = y * screenInfo.screens[0]->height / priv->wcmMaxY;
      }
      
      /* coordonates are ready we can send events */
      if (is_proximity) {

	if (!*pprox) {
	  if (!is_core_pointer) {
	    PostProximityEvent(device, 1, 0, 3, x, y, z);
	  }
	  local->private_flags |= FIRST_TOUCH_FLAG;
	  DBG(4, ErrorF("xf86WcmReadInput FIRST_TOUCH_FLAG set\n"));
	}      

	DBG(4, ErrorF("xf86WcmReadInput %s x=%d y=%d z=%d buttons=%d\n",
		      is_stylus ? "stylus" : "cursor", x, y, z, buttons));
    
	if ((*px != x) ||
	    (*py != y) ||
	    (*pz != z)) {
	  if (!is_absolute && (local->private_flags & FIRST_TOUCH_FLAG)) {
	    local->private_flags -= FIRST_TOUCH_FLAG;
	    DBG(4, ErrorF("xf86WcmReadInput FIRST_TOUCH_FLAG unset\n"));
	  }
	  else {
	    if (is_absolute) {
	      PostMotionEvent(device, is_absolute, 0, 3, x, y, z); 
	    }
	    else {
	      PostMotionEvent(device, is_absolute, 0, 3, x - *px, y - *py, z);
	    }
	  }
	}
	if (*pbuttons != buttons) {
	  int		delta = buttons - *pbuttons;
	  int		button = (delta > 0) ? delta : ((delta == 0) ? *pbuttons : -delta);

	  if (is_stylus && (delta == 3)) {
	    PostButtonEvent(device, 1, (delta > 0), 0, 3, x, y, z);
	    PostButtonEvent(device, 2, (delta > 0), 0, 3, x, y, z);
	  }
	  else {
	    PostButtonEvent(device, button, (delta > 0), 0, 3, x, y, z); 
	  }
	}
	*pbuttons = buttons;
	*px = x;
	*py = y;
	*pz = z;
	*pprox = is_proximity;
      }
      else { /* !PROXIMITY */
	if (*pbuttons) {
	  if (is_stylus && (*pbuttons == 3)) {
	    PostButtonEvent(device, 1, 0, 0, 3, *px, *py, *pz);
	    PostButtonEvent(device, 2, 0, 0, 3, *px, *py, *pz);
	  }
	  else {
	    PostButtonEvent(device, *pbuttons, 0, 0, 3, *px, *py, *pz);
	  }
	  *pbuttons = 0;
	}
	if (!is_core_pointer) {
	  if (*pprox) {
	    PostProximityEvent(device, 0, 0, 3, *px, *py, *pz);
	  }
	  if (buttons) {
	    PostButtonEvent(device, z, 1, 0, 3, *px, *py, *pz);
	    PostButtonEvent(device, z, 0, 0, 3, *px, *py, *pz); 
	  }
	}
	*pprox = 0;
      }
  
      DBG(7, ErrorF("xf86WcmEvents END   device=0x%x priv=0x%x",
		    local->dev, priv));
      
      /* reset char count for next read */
      priv->wcmIndex = 0;
    }
  }
}

static int
xf86WcmGetMotionEvents(DeviceIntPtr	dev,
		       xTimecoord	*buff,
		       unsigned long	start,
		       unsigned long	stop,
		       ScreenPtr	pScreen)
{
  return 0;
}

static void
xf86WcmControlProc(DeviceIntPtr	device,
                    PtrCtrl		*ctrl)
{
  DBG(2, ErrorF("xf86WcmControlProc\n"));
}

static Bool
xf86WcmOpen(LocalDevicePtr	local)
{
  struct termios	termios_tty;
  struct timeval	timeout;
  char			buffer[256];
  int			err;
  WacomDevicePtr	priv = (WacomDevicePtr)local->private;
  int			a, b;
#if defined(sun) && !defined(i386)
  char			*name = getenv("WACOM_DEV");
  
  if (name) {
    priv->wcmDevice = strdup(name);
    ErrorF("xf86WcmOpen port changed to '%s'\n", priv->wcmDevice);
  }
#endif
  
  DBG(1, ErrorF("opening %s\n", priv->wcmDevice));

  SYSCALL(local->fd = open(priv->wcmDevice, O_RDWR|O_NDELAY));
  if (local->fd == -1) {
    Error(priv->wcmDevice);
    return !Success;
  }

#ifdef POSIX_TTY
  err = tcgetattr(local->fd, &termios_tty);
  if (err == -1) {
    Error("tcgetattr");
    return !Success;
  }
  termios_tty.c_iflag = IXOFF;
  termios_tty.c_cflag = B9600|CS8|CREAD ;
  termios_tty.c_lflag = 0;

  termios_tty.c_cc[VINTR] = 0;
  termios_tty.c_cc[VQUIT] = 0;
  termios_tty.c_cc[VERASE] = 0;
#ifdef VWERASE
  termios_tty.c_cc[VWERASE] = 0;
#endif
#ifdef VREPRINT
  termios_tty.c_cc[VREPRINT] = 0;
#endif
  termios_tty.c_cc[VKILL] = 0;
  termios_tty.c_cc[VEOF] = 0;
  termios_tty.c_cc[VEOL] = 0;
#ifdef VEOL2
  termios_tty.c_cc[VEOL2] = 0;
#endif
  termios_tty.c_cc[VSUSP] = 0;
#ifdef VDISCARD
  termios_tty.c_cc[VDISCARD] = 0;
#endif
#ifdef VLNEXT
  termios_tty.c_cc[VLNEXT] = 0; 
#endif
	
  termios_tty.c_cc[VMIN] = 1 ;
  termios_tty.c_cc[VTIME] = 10 ;

  err = tcsetattr(local->fd, TCSANOW, &termios_tty);
  if (err == -1) {
    Error("tcsetattr TCSANOW");
    return !Success;
  }
#else
  Code for OSs without POSIX tty functions
#endif

  DBG(1, ErrorF("initializing tablet\n"));
    
  /* send reset to the tablet */
  SYSCALL(err = write(local->fd, WC_RESET_IV, strlen(WC_RESET_IV)));
  if (err == -1)
    {
      Error("write");
      return !Success;
    }
    
  /* wait 200 mSecs */
  timeout.tv_sec = 0;
  timeout.tv_usec = 200000;
  SYSCALL(err = select(0, NULL, NULL, NULL, &timeout));
  if (err == -1)
    {
      Error("select");
      return !Success;
    }
  
  DBG(2, ErrorF("reading model\n"));
  if (!send_request(local->fd, WC_MODEL, buffer)) 
    return !Success;
  DBG(2, ErrorF("%s\n", buffer));

  if (xf86Verbose)
    ErrorF("%s Wacom tablet model : %s\n", XCONFIG_PROBED, buffer+2);

  DBG(2, ErrorF("reading config\n"));
  if (!send_request(local->fd, WC_CONFIG, buffer))
    return !Success;
  DBG(2, ErrorF("%s\n", buffer));
  sscanf(buffer+19, "%d,%d,%d,%d", &a, &b, &priv->wcmResolX, &priv->wcmResolY);
  
  DBG(2, ErrorF("reading max coordinates\n"));
  if (!send_request(local->fd, WC_COORD, buffer))
    return !Success;
  DBG(2, ErrorF("%s\n", buffer));
  sscanf(buffer+2, "%d,%d", &priv->wcmMaxX, &priv->wcmMaxY);

  DBG(2, ErrorF("setup is max X=%d max Y=%d resol X=%d resol Y=%d\n", priv->wcmMaxX, priv->wcmMaxY,
		priv->wcmResolX, priv->wcmResolY));
  
  /* send a setup string to the tablet */
  SYSCALL(err = write(local->fd, setup_string, strlen(setup_string)));
  if (err == -1) {
    Error("write");
    return !Success;
  }
  else {
    char	buf[20];

    if (priv->wcmSuppress < 0) {
      priv->wcmSuppress = 0;
    }
    else {
      if (priv->wcmSuppress > 100) {
	priv->wcmSuppress = 99;
      }
    }
    sprintf(buf, "%s%d\r",  WC_SUPPRESS, priv->wcmSuppress);
    SYSCALL(err = write(local->fd, buf, strlen(buf)));

    if (err == -1) {
      Error("write");
      return !Success;
    }
  }
    
  if (xf86Verbose)
    ErrorF("%s Wacom tablet maximum X=%d maximum Y=%d X resolution=%d Y resolution=%d suppress=%d\n",
	   XCONFIG_PROBED,  priv->wcmMaxX, priv->wcmMaxY,
	   priv->wcmResolX, priv->wcmResolY, priv->wcmSuppress);
  
  if (err <= 0) {
    SYSCALL(close(local->fd));
    return !Success;
  }

  return Success;
}

/*
 * xf86WcmProc --
 *      Handle the initialization, etc. of a wacom
 */
static int
xf86WcmProc(pWcm, what)
     DeviceIntPtr       pWcm;
     int                what;
{
  CARD8                 map[25];
  int                   nbaxes;
  int                   nbbuttons;
  int                   loop;
  LocalDevicePtr        local = (LocalDevicePtr)pWcm->public.devicePrivate;
  WacomDevicePtr        priv = (WacomDevicePtr)PRIVATE(pWcm);
  
  DBG(2, ErrorF("BEGIN xf86WcmProc dev=0x%x priv=0x%x type=%s private_flags=%d what=%d\n",
                pWcm, priv, (DEVICE_ID(local->private_flags) == STYLUS_ID) ? "stylus" :
		(DEVICE_ID(local->private_flags) == CURSOR_ID) ? "cursor" : "eraser",
		local->private_flags, what));
  
  switch (what)
    {
    case DEVICE_INIT: 
      DBG(1, ErrorF("xf86WcmProc pWcm=0x%x what=INIT\n", pWcm));
      
      nbaxes = 3;
      switch (DEVICE_ID(local->private_flags)) {
      case STYLUS_ID:
      case CURSOR_ID:
	nbbuttons = 24;
	break;

      case ERASER_ID:
	nbbuttons = 1;
	break;
      }
  
      for(loop=1; loop<=nbbuttons; loop++) map[loop] = loop;

      if (InitButtonClassDeviceStruct(pWcm,
				      nbbuttons,
				      map) == FALSE) {
	ErrorF("unable to allocate Button class device\n");
	return !Success;
      }
      
      if (InitFocusClassDeviceStruct(pWcm) == FALSE) {
	ErrorF("unable to init Focus class device\n");
	return !Success;
      }
          
      if (InitPtrFeedbackClassDeviceStruct(pWcm,
					   xf86WcmControlProc) == FALSE) {
	ErrorF("unable to init ptr feedback\n");
	return !Success;
      }
          
      if (InitProximityClassDeviceStruct(pWcm) == FALSE) {
	ErrorF("unable to init proximity class device\n"); 
	return !Success;
      }

      if (InitValuatorClassDeviceStruct(pWcm, 
					nbaxes,
					xf86WcmGetMotionEvents, 
					0, /* numMotionEvents */
					(local->private_flags & ABSOLUTE_FLAG) ? Absolute
					: Relative) /* relatif ou absolute */
	  == FALSE) {
	ErrorF("unable to allocate Valuator class device\n"); 
	return !Success;
      }
      else {
	InitValuatorAxisStruct(pWcm,
			       0,
			       0, /* min val */
			       priv->wcmMaxX, /* max val */
			       priv->wcmResolX * 1000 / 2.54); /* resolution */
	InitValuatorAxisStruct(pWcm,
			       1,
			       0, /* min val */
			       priv->wcmMaxY, /* max val */
			       priv->wcmResolY * 1000 / 2.54); /* resolution */
	InitValuatorAxisStruct(pWcm,
			       2,
			       - priv->wcmMaxZ / 2, /* min val */
			       priv->wcmMaxZ / 2, /* max val */
			       priv->wcmResolZ * 1000 / 2.54); /* resolution */

	AssignTypeAndName(pWcm, local->atom, local->name);
      }
      
      switch (DEVICE_ID(local->private_flags)) {
      case STYLUS_ID:
	priv->wcmStylus = local;
	break;
	
      case CURSOR_ID:
	priv->wcmCursor = local;
	break;

      case ERASER_ID:
	priv->wcmEraser = local;
	break;
      }
      break; 
      
    case DEVICE_ON:
      DBG(1, ErrorF("xf86WcmProc pWcm=0x%x what=ON\n", pWcm));
      if (local->fd < 0) {
	/* check if we haven't already opened with the other device */
	switch (DEVICE_ID(local->private_flags)) {
	case STYLUS_ID:
	  if (priv->wcmCursor && (priv->wcmCursor->fd != -1)) {
	    local->fd = priv->wcmCursor->fd;
	  }
	  else {
	    if (priv->wcmEraser && (priv->wcmEraser->fd != -1)) {
	      local->fd = priv->wcmEraser->fd;
	    }
	    else {
	      if (xf86WcmOpen(local) != Success) {
		SYSCALL(close(local->fd));
                local->fd = -1;
		return !Success;
	      }
	    }
	  }
	  break;

	case CURSOR_ID:
	  if (priv->wcmStylus && (priv->wcmStylus->fd != -1)) {
	    local->fd = priv->wcmStylus->fd;
	  }
	  else {
	    if (priv->wcmEraser && (priv->wcmEraser->fd != -1)) {
	      local->fd = priv->wcmEraser->fd;
	    }
	    else {
	      if (xf86WcmOpen(local) != Success) {
		SYSCALL(close(local->fd));
                local->fd = -1;
		return !Success;
	      }
	    }
	  } 
	  break;

	case ERASER_ID:
	  if (priv->wcmCursor && (priv->wcmCursor->fd != -1)) {
	    local->fd = priv->wcmCursor->fd;
	  }
	  else {
	    if (priv->wcmStylus && (priv->wcmStylus->fd != -1)) {
	      local->fd = priv->wcmStylus->fd;
	    }
	    else {
	      if (xf86WcmOpen(local) != Success) {
		SYSCALL(close(local->fd));
                local->fd = -1;
		return !Success;
	      }
	    }
	  }
	  break;    
	}
      }
      
      if (local->fd >= 0) {                
	AddEnabledDevice(local->fd);
	pWcm->public.on = TRUE;
      }
      else {
	return !Success;
      }
      break;
      
    case DEVICE_OFF:
      DBG(1, ErrorF("xf86WcmProc  pWcm=0x%x what=%s\n", pWcm,
		    (what == DEVICE_CLOSE) ? "CLOSE" : "OFF"));
      if (local->fd >= 0)
	RemoveEnabledDevice(local->fd);
      pWcm->public.on = FALSE;
      break;
      
    case DEVICE_CLOSE:
      DBG(1, ErrorF("xf86WcmProc  pWcm=0x%x what=%s\n", pWcm,
		    (what == DEVICE_CLOSE) ? "CLOSE" : "OFF"));
      SYSCALL(close(local->fd));
      local->fd = -1;
      break;

    default:
      ErrorF("unsupported mode=%d\n", what);
      return !Success;
      break;
    }
  DBG(2, ErrorF("END   xf86WcmProc Success what=%d dev=0x%x priv=0x%x\n",
                what, pWcm, priv));
  return Success;
}

static void
xf86WcmClose(LocalDevicePtr	local)
{
  if (local->fd >= 0) {
    SYSCALL(close(local->fd));
  }
  local->fd = -1;
}

static int
xf86WcmChangeControl(LocalDevicePtr	local,
		     xDeviceCtl		*control)
{
  xDeviceResolutionCtl	*res;
  int			*resolutions;
  char			str[10];
  
  res = (xDeviceResolutionCtl *)control;
	
  if ((control->control != DEVICE_RESOLUTION) ||
      (res->num_valuators < 1))
    return (BadMatch);
  
  resolutions = (int *)(res +1);
  ErrorF("xf86WcmChangeControl changing to %d (suppressing under)\n", resolutions[0]);

  sprintf(str, "SU%d\r", resolutions[0]);
  SYSCALL(write(local->fd, str, strlen(str)));
  
  return(Success);
}

static int
xf86WcmSwitchMode(ClientPtr	client,
		  DeviceIntPtr	dev,
		  int		mode)
{
  LocalDevicePtr        local = (LocalDevicePtr)dev->public.devicePrivate;

  DBG(3, ErrorF("xf86WcmSwitchMode dev=0x%x mode=%s\n", dev, mode));
  
  if (mode == Absolute) {
    local->private_flags = local->private_flags | ABSOLUTE_FLAG;
  }
  else {
    if (mode == Relative) {
      local->private_flags = local->private_flags & ~ABSOLUTE_FLAG; 
    }
    else {
      DBG(1, ErrorF("xf86WcmSwitchMode dev=0x%x invalid mode=%s\n", dev, mode));
      return BadMatch;
    }
  }
  return Success;
}

/*
 ***************************************************************************
 *
 * xf86WcmAllocate --
 *
 ***************************************************************************
 */
static LocalDevicePtr
xf86WcmAllocate(char *  name,
                int     flag)
{
  LocalDevicePtr        local = (LocalDevicePtr) xalloc(sizeof(LocalDeviceRec));
  WacomDevicePtr        priv = (WacomDevicePtr) xalloc(sizeof(WacomDeviceRec));
  
  local->name = name;
  local->flags = XI86_NO_OPEN_ON_INIT;
#if !defined(sun) || defined(i386)
  local->device_config = xf86WcmConfig;
#endif
  local->device_control = xf86WcmProc;
  local->read_input = xf86WcmReadInput;
  local->control_proc = xf86WcmChangeControl;
  local->close_proc = xf86WcmClose;
  local->switch_mode = xf86WcmSwitchMode;
  local->fd = -1;
  local->atom = 0;
  local->dev = NULL;
  local->private = priv;
  local->private_flags = flag;

  priv->wcmDevice = "/dev/ttya"; /* device file name */            
  priv->wcmSuppress = 20;       /* transmit position if increment is superior */
  priv->wcmOldX = -1;           /* previous X position */         
  priv->wcmOldY = -1;           /* previous Y position */         
  priv->wcmOldZ = -1;           /* previous pressure */           
  priv->wcmOldProximity = 0;    /* previous proximity */
  priv->wcmOldButtons = 0;      /* previous buttons state */      
  priv->wcmOldCursorX = -1;     /* previous cursor X position */         
  priv->wcmOldCursorY = -1;     /* previous cursor Y position */         
  priv->wcmOldCursorZ = -1;     /* previous cursor pressure */           
  priv->wcmOldCursorProximity = 0; /* previous cursor proximity */
  priv->wcmOldCursorButtons = 0; /* previous cursor buttons state */ 
  priv->wcmMaxX = 22860;        /* max X value */                 
  priv->wcmMaxY = 15240;        /* max Y value */                 
  priv->wcmMaxZ = 240;          /* max Z value */                 
  priv->wcmResolX = 1270;       /* X resolution in points/inch */
  priv->wcmResolY = 1270;       /* Y resolution in points/inch */
  priv->wcmResolZ = 1270;       /* Z resolution in points/inch */
  priv->flags = 0;              /* various flags */
  priv->wcmCursor = NULL;       /* cursor device ptr */           
  priv->wcmStylus = NULL;       /* stylus device ptr */           
  priv->wcmEraser = NULL;       /* eraser device ptr */           
  priv->wcmIndex = 0;           /* number of bytes read */

  return local;
}


/*
 ***************************************************************************
 *
 * xf86WcmAllocateStylus --
 *
 ***************************************************************************
 */
static LocalDevicePtr
xf86WcmAllocateStylus(char *  name,
                      int     flag)
{
  LocalDevicePtr        local = xf86WcmAllocate(XI_STYLUS, STYLUS_ID);

  ((WacomDevicePtr)local->private)->wcmStylus = local;
  return local;
}

/*
 ***************************************************************************
 *
 * xf86WcmAllocateCursor --
 *
 ***************************************************************************
 */
static LocalDevicePtr
xf86WcmAllocateCursor(char *  name,
                      int     flag)
{
  LocalDevicePtr        local = xf86WcmAllocate(XI_CURSOR, CURSOR_ID);

  ((WacomDevicePtr)local->private)->wcmCursor = local;
  return local;
}

/*
 ***************************************************************************
 *
 * xf86WcmAllocateEraser --
 *
 ***************************************************************************
 */
static LocalDevicePtr
xf86WcmAllocateEraser(char *  name,
                      int     flag)
{
  LocalDevicePtr        local = xf86WcmAllocate(XI_ERASER, ABSOLUTE_FLAG|ERASER_ID);

  ((WacomDevicePtr)local->private)->wcmEraser = local;
  return local;
}

/*
 ***************************************************************************
 *
 * Wacom Stylus device association --
 *
 ***************************************************************************
 */
DeviceAssocRec wacom_stylus_assoc =
{
  STYLUS_SECTION_NAME,          /* config_section_name */
  xf86WcmAllocateStylus         /* device_allocate */
};

/*
 ***************************************************************************
 *
 * Wacom Cursor device association --
 *
 ***************************************************************************
 */
DeviceAssocRec wacom_cursor_assoc =
{
  CURSOR_SECTION_NAME,          /* config_section_name */
  xf86WcmAllocateCursor         /* device_allocate */
};

/*
 ***************************************************************************
 *
 * Wacom Eraser device association --
 *
 ***************************************************************************
 */
DeviceAssocRec wacom_eraser_assoc =
{
  ERASER_SECTION_NAME,          /* config_section_name */
  xf86WcmAllocateEraser         /* device_allocate */
};

/* end of xf86Wacom.c */
