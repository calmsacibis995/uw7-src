#pragma ident	"@(#)m1.2libs:Xm/MapEvents.c	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <Xm/XmP.h>

#ifdef _NO_PROTO
typedef Boolean (*XmEventParseProc)();
#else
typedef Boolean (*XmEventParseProc)( String, unsigned, unsigned *);
#endif

typedef struct {
   char       * event;
   XrmQuark     signature;
   int eventType;
   XmEventParseProc parseProc ;
   unsigned int closure;
} EventKey;

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static int StrToHex() ;
static int StrToOct() ;
static int StrToNum() ;
static void FillInQuarks() ;
static Boolean LookupModifier() ;
static String ScanAlphanumeric() ;
static String ScanWhitespace() ;
static Boolean ParseImmed() ;
static Boolean ParseKeySym() ;
static String ParseModifiers() ;
static String ParseEventType() ;
static Boolean _MapEvent() ;

#else

static int StrToHex( 
                        String str) ;
static int StrToOct( 
                        String str) ;
static int StrToNum( 
                        String str) ;
static void FillInQuarks( 
                        EventKey *table) ;
static Boolean LookupModifier( 
                        String name,
                        unsigned int *valueP) ;
static String ScanAlphanumeric( 
                        register String str) ;
static String ScanWhitespace( 
                        register String str) ;
static Boolean ParseImmed( 
                        String str,
                        unsigned int closure,
                        unsigned int *detail) ;
static Boolean ParseKeySym( 
                        String str,
                        unsigned int closure,
                        unsigned *detail) ;
static String ParseModifiers( 
                        register String str,
                        unsigned int *modifiers,
                        Boolean *status) ;
static String ParseEventType( 
                        register String str,
                        EventKey *table,
                        int *eventType,
                        Cardinal *_index,
                        Boolean *status) ;
static Boolean _MapEvent( 
                        register String str,
                        EventKey *table,
                        int *eventType,
                        unsigned *detail,
                        unsigned int *modifiers) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static EventKey modifierStrings[] = {

/* Modifier,	Quark,		Mask */

{"None",	NULLQUARK,	0,		NULL,		None},
{"Shift",	NULLQUARK,	0,		NULL,		ShiftMask},
{"Lock",	NULLQUARK,	0,		NULL,		LockMask},
{"Ctrl",	NULLQUARK,	0,		NULL,		ControlMask},
{"Meta",	NULLQUARK,	0,		NULL,		Mod1Mask},
{"Alt",		NULLQUARK,	0,		NULL,		Mod1Mask},
{"Mod1",	NULLQUARK,	0,		NULL,		Mod1Mask},
{"Mod2",	NULLQUARK,	0,		NULL,		Mod2Mask},
{"Mod3",	NULLQUARK,	0,		NULL,		Mod3Mask},
{"Mod4",	NULLQUARK,	0,		NULL,		Mod4Mask},
{"Mod5",	NULLQUARK,	0,		NULL,		Mod5Mask},
{NULL,		NULLQUARK,	0,		NULL,		0}};

static EventKey buttonEvents[] = {

/* Event Name,	Quark,		Event Type,	DetailProc,	Closure */

{"Btn1Down",	NULLQUARK,	ButtonPress,	ParseImmed,	Button1},
{"Button1",	NULLQUARK,	ButtonPress,	ParseImmed,	Button1},
{"Btn1",	NULLQUARK,	ButtonPress,	ParseImmed,	Button1},
{"Btn2Down",	NULLQUARK,	ButtonPress,	ParseImmed,	Button2},
{"Button2",	NULLQUARK,	ButtonPress,	ParseImmed,	Button2},
{"Btn2",	NULLQUARK,	ButtonPress,	ParseImmed,	Button2},
{"Btn3Down",	NULLQUARK,	ButtonPress,	ParseImmed,	Button3},
{"Button3",	NULLQUARK,	ButtonPress,	ParseImmed,	Button3},
{"Btn3",	NULLQUARK,	ButtonPress,	ParseImmed,	Button3},
{"Btn4Down",	NULLQUARK,	ButtonPress,	ParseImmed,	Button4},
{"Button4",	NULLQUARK,	ButtonPress,	ParseImmed,	Button4},
{"Btn4",	NULLQUARK,	ButtonPress,	ParseImmed,	Button4},
{"Btn5Down",	NULLQUARK,	ButtonPress,	ParseImmed,	Button5},
{"Button5",	NULLQUARK,	ButtonPress,	ParseImmed,	Button5},
{"Btn5",	NULLQUARK,	ButtonPress,	ParseImmed,	Button5},
#ifdef FUTURES
{"Btn1Up",	NULLQUARK,	ButtonRelease,	ParseImmed,	Button1},
{"Btn2Up",	NULLQUARK,	ButtonRelease,	ParseImmed,	Button2},
{"Btn3Up",	NULLQUARK,	ButtonRelease,	ParseImmed,	Button3},
{"Btn4Up",	NULLQUARK,	ButtonRelease,	ParseImmed,	Button4},
{"Btn5Up",	NULLQUARK,	ButtonRelease,	ParseImmed,	Button5},
#endif
{NULL,		NULLQUARK,	0,		NULL,		0}};


static EventKey keyEvents[] = {

/* Event Name,	Quark,		Event Type,	DetailProc	Closure */

{"KeyPress",	NULLQUARK,	KeyPress,	ParseKeySym,	0},
{"Key",		NULLQUARK,	KeyPress,	ParseKeySym,	0},
{"KeyDown",	NULLQUARK,	KeyPress,	ParseKeySym,	0},
{"KeyUp",	NULLQUARK,	KeyRelease,	ParseKeySym,	0},
{"KeyRelease",	NULLQUARK,	KeyRelease,	ParseKeySym,	0},
{NULL,		NULLQUARK,	0,		NULL,		0}};

static unsigned int buttonModifierMasks[] = {
    0, Button1Mask, Button2Mask, Button3Mask, Button4Mask, Button5Mask
};

static initialized = FALSE;



/*************************************<->*************************************
 *
 *  Numeric convertion routines
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static int 
#ifdef _NO_PROTO
StrToHex( str )
        String str ;
#else
StrToHex(
        String str )
#endif /* _NO_PROTO */
{
    register char   c;
    register int    val = 0;

    while ((c = *str) != '\0') {
	if ('0' <= c && c <= '9') val = val*16+c-'0';
	else if ('a' <= c && c <= 'f') val = val*16+c-'a'+10;
	else if ('A' <= c && c <= 'F') val = val*16+c-'A'+10;
	else return -1;
	str++;
    }

    return val;
}

static int 
#ifdef _NO_PROTO
StrToOct( str )
        String str ;
#else
StrToOct(
        String str )
#endif /* _NO_PROTO */
{
    register char c;
    register int  val = 0;

    while ((c = *str) != '\0') {
        if ('0' <= c && c <= '7') val = val*8+c-'0'; else return -1;
	str++;
    }

    return val;
}

static int 
#ifdef _NO_PROTO
StrToNum( str )
        String str ;
#else
StrToNum(
        String str )
#endif /* _NO_PROTO */
{
    register char c;
    register int val = 0;

    if (*str == '0') {
	str++;
	if (*str == 'x' || *str == 'X') return StrToHex(++str);
	else return StrToOct(str);
    }

    while ((c = *str) != '\0') {
	if ('0' <= c && c <= '9') val = val*10+c-'0';
	else return -1;
	str++;
    }

    return val;
}


/*************************************<->*************************************
 *
 *  FillInQuarks (parameters)
 *
 *   Description:
 *   -----------
 *     Converts each string entry in the modifier/event tables to a
 *     quark, thus facilitating faster comparisons.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static void 
#ifdef _NO_PROTO
FillInQuarks( table )
        EventKey *table ;
#else
FillInQuarks(
        EventKey *table )
#endif /* _NO_PROTO */
{
    register int i;

    for (i=0; table[i].event; i++)
        table[i].signature = XrmStringToQuark(table[i].event);
}


/*************************************<->*************************************
 *
 *  LookupModifier (parameters)
 *
 *   Description:
 *   -----------
 *     Compare the passed in string to the list of valid modifiers.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static Boolean 
#ifdef _NO_PROTO
LookupModifier( name, valueP )
        String name ;
        unsigned int *valueP ;
#else
LookupModifier(
        String name,
        unsigned int *valueP )
#endif /* _NO_PROTO */
{
    register int i;
    register XrmQuark signature = XrmStringToQuark(name);

    for (i=0; modifierStrings[i].event != NULL; i++)
	if (modifierStrings[i].signature == signature) {
	    *valueP = modifierStrings[i].closure;
	    return TRUE;
	}

    return FALSE;
}


/*************************************<->*************************************
 *
 *  ScanAlphanumeric (parameters)
 *
 *   Description:
 *   -----------
 *     Scan string until a non-alphanumeric character is encountered.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static String 
#ifdef _NO_PROTO
ScanAlphanumeric( str )
        register String str ;
#else
ScanAlphanumeric(
        register String str )
#endif /* _NO_PROTO */
{
    while (
        ('A' <= *str && *str <= 'Z') || ('a' <= *str && *str <= 'z')
	|| ('0' <= *str && *str <= '9')) str++;
    return str;
}


/*************************************<->*************************************
 *
 *  ScanWhitespace (parameters)
 *
 *   Description:
 *   -----------
 *     Scan the string, skipping over all white space characters.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static String 
#ifdef _NO_PROTO
ScanWhitespace( str )
        register String str ;
#else
ScanWhitespace(
        register String str )
#endif /* _NO_PROTO */
{
    while (*str == ' ' || *str == '\t') str++;
    return str;
}


/*************************************<->*************************************
 *
 *  ParseImmed (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
ParseImmed( str, closure, detail )
        String str ;
        unsigned int closure ;
        unsigned int *detail ;
#else
ParseImmed(
        String str,
        unsigned int closure,
        unsigned int *detail )
#endif /* _NO_PROTO */
{
   *detail = closure;
   return (TRUE);
}


/*************************************<->*************************************
 *
 *  ParseKeySym (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
ParseKeySym( str, closure, detail )
        String str ;
        unsigned int closure ;
        unsigned *detail ;
#else
ParseKeySym(
        String str,
        unsigned int closure,
        unsigned *detail )
#endif /* _NO_PROTO */
{
    char keySymName[100], *start;

    str = ScanWhitespace(str);

    if (*str == '\\') {
	str++;
	keySymName[0] = *str;
/***** no access to str after this statement and str is a value param. ****
	if (*str != '\0' && *str != '\n') str++;
**************************************************************************/
	keySymName[1] = '\0';
	*detail = (unsigned) XStringToKeysym(keySymName);
    } else if (*str == ',' || *str == ':') {
       /* No detail; return a failure */
       *detail = (unsigned) NoSymbol;
       return (FALSE);
    } else {
	start = str;
	while (
		*str != ','
		&& *str != ':'
		&& *str != ' '
		&& *str != '\t'
                && *str != '\n'
		&& *str != '\0') str++;
	(void) strncpy(keySymName, start, str-start);
	keySymName[str-start] = '\0';
	*detail = (unsigned) XStringToKeysym(keySymName);
    }

    if (*detail == (unsigned) NoSymbol)
    {
       if (( '0' <= keySymName[0]) && (keySymName[0] <= '9'))
       {
        int retval = StrToNum(keySymName);
        if (-1 == retval) 
              {
              *detail = 0;
              return (FALSE);
              }
        else
              *detail = retval;
          return (TRUE);
       }
       return (FALSE);
    }
    else
       return (TRUE);
}


/*************************************<->*************************************
 *
 *  ParseModifiers (parameters)
 *
 *   Description:
 *   -----------
 *     Parse the string, extracting all modifier specifications.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static String 
#ifdef _NO_PROTO
ParseModifiers( str, modifiers, status )
        register String str ;
        unsigned int *modifiers ;
        Boolean *status ;
#else
ParseModifiers(
        register String str,
        unsigned int *modifiers,
        Boolean *status )
#endif /* _NO_PROTO */
{
    register String start;
    char modStr[100];
    Boolean notFlag;
    unsigned int maskBit;

    /* Initially assume all is going to go well */
    *status = TRUE;
    *modifiers = 0;
 
    /* Attempt to parse the first button modifier */
    str = ScanWhitespace(str);
    start = str;
    str = ScanAlphanumeric(str);
    if (start != str) {
         (void) strncpy(modStr, start, str-start);
          modStr[str-start] = '\0';
          if (LookupModifier(modStr, &maskBit))
          {
	    if (maskBit== None) {
		*modifiers = 0;
                str = ScanWhitespace(str);
	        return str;
            }
         }
         str = start;
    }

   
    /* Keep parsing modifiers, until the event specifier is encountered */
    while ((*str != '<') && (*str != '\0')) {
        if (*str == '~') {
             notFlag = TRUE;
             str++;
          } else 
              notFlag = FALSE;

	start = str;
        str = ScanAlphanumeric(str);
        if (start == str) {
           /* ERROR: Modifier or '<' missing */
           *status = FALSE;
           return str;
        }
        (void) strncpy(modStr, start, str-start);
        modStr[str-start] = '\0';

        if (!LookupModifier(modStr, &maskBit))
        {
           /* Unknown modifier name */
           *status = FALSE;
           return str;
        }

	if (notFlag) 
           *modifiers &= ~maskBit;
	else 
           *modifiers |= maskBit;
        str = ScanWhitespace(str);
    }

    return str;
}


/*************************************<->*************************************
 *
 *  ParseEventType (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static String 
#ifdef _NO_PROTO
ParseEventType( str, table, eventType, _index, status )
        register String str ;
        EventKey *table ;
        int *eventType ;
        Cardinal *_index ;
        Boolean *status ;
#else
ParseEventType(
        register String str,
        EventKey *table,
        int *eventType,
        Cardinal *_index,
        Boolean *status )
#endif /* _NO_PROTO */
{
    String start = str;
    char eventTypeStr[100];
    register Cardinal   i;
    register XrmQuark	signature;

    /* Parse out the event string */
    str = ScanAlphanumeric(str);
    (void) strncpy(eventTypeStr, start, str-start);
    eventTypeStr[str-start] = '\0';

    /* Attempt to match the parsed event against our supported event set */
    signature = XrmStringToQuark(eventTypeStr);
    for (i = 0; table[i].signature != NULLQUARK; i++)
        if (table[i].signature == signature)
        {
           *_index = i;
           *eventType = table[*_index].eventType;

           *status = TRUE;
           return str; 
        }

    /* Unknown event specified */
    *status = FALSE;
    return (str);
}


/*************************************<->*************************************
 *
 *  _MapEvent (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
static Boolean 
#ifdef _NO_PROTO
_MapEvent( str, table, eventType, detail, modifiers )
        register String str ;
        EventKey *table ;
        int *eventType ;
        unsigned *detail ;
        unsigned int *modifiers ;
#else
_MapEvent(
        register String str,
        EventKey *table,
        int *eventType,
        unsigned *detail,
        unsigned int *modifiers )
#endif /* _NO_PROTO */
{
    Cardinal _index;
    Boolean  status;
 
    /* Initialize, if first time called */
    if (!initialized)
    {
       initialized = TRUE;
       FillInQuarks (buttonEvents);
       FillInQuarks (modifierStrings);
       FillInQuarks (keyEvents);
    }

    /* Parse the modifiers */
    str = ParseModifiers(str, modifiers, &status);
    if ( status == FALSE || *str != '<') 
       return (FALSE);
    else 
       str++;

    /* Parse the event type and detail */
    str = ParseEventType(str, table, eventType, &_index, &status);
    if (status == FALSE || *str != '>') 
       return (FALSE);
    else 
       str++;

    /* Save the detail */
    return ((*(table[_index].parseProc))(str, table[_index].closure, detail));
}

/*************************************<->*************************************
 *
 *  _MapBtnEvent (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
Boolean 
#ifdef _NO_PROTO
_XmMapBtnEvent( str, eventType, button, modifiers )
        register String str ;
        int *eventType ;
        unsigned int *button ;
        unsigned int *modifiers ;
#else
_XmMapBtnEvent(
        register String str,
        int *eventType,
        unsigned int *button,
        unsigned int *modifiers )
#endif /* _NO_PROTO */
{
    if (_MapEvent (str, buttonEvents, eventType, button, modifiers) == FALSE)
       return (FALSE);

    /*
     * The following is a fix for an X11 deficiency in regards to
     * modifiers in grabs.
     */
    if (*eventType == ButtonRelease)
    {
        /* the button that is going up will always be in the modifiers... */
        *modifiers |= buttonModifierMasks[*button];
    }

    return (TRUE);
}



/*************************************<->*************************************
 *
 *  _XmMapKeyEvent (parameters)
 *
 *   Description:
 *   -----------
 *     xxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
Boolean 
#ifdef _NO_PROTO
_XmMapKeyEvent( str, eventType, keysym, modifiers )
        register String str ;
        int *eventType ;
        unsigned *keysym ;
        unsigned int *modifiers ;
#else
_XmMapKeyEvent(
        register String str,
        int *eventType,
        unsigned *keysym,
        unsigned int *modifiers )
#endif /* _NO_PROTO */
{
    Boolean rtn ;
    
    rtn = _MapEvent (str, keyEvents, eventType, keysym, modifiers);
    return ( rtn ) ;
}


/*************************************<->*************************************
 *
 *  _XmMatchBtnEvent (parameters)
 *
 *   Description:
 *   -----------
 *     Compare the passed in event to the event described by the parameter
 *     list.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
Boolean 
#ifdef _NO_PROTO
_XmMatchBtnEvent( event, eventType, button, modifiers )
        XEvent *event ;
        int eventType ;
        unsigned int button ;
        unsigned int modifiers ;
#else
_XmMatchBtnEvent(
        XEvent *event,
        int eventType,
        unsigned int button,
        unsigned int modifiers )
#endif /* _NO_PROTO */
{
   register unsigned int state = event->xbutton.state & (ShiftMask | LockMask | 
						ControlMask | Mod1Mask | 
						Mod2Mask | Mod3Mask | 
						Mod4Mask | Mod5Mask);
   if (((eventType == XmIGNORE_EVENTTYPE)||(event->type == eventType)) &&
       (event->xbutton.button == button) &&
       ((modifiers == AnyModifier)||(state == modifiers)) )
      return (TRUE);
   else
      return (FALSE);
}



/*************************************<->*************************************
 *
 *  _XmMatchKeyEvent (parameters)
 *
 *   Description:
 *   -----------
 *     Compare the passed in event to the event described by the parameter
 *     list.
 *
 *
 *   Inputs:
 *   ------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 * 
 *   Outputs:
 *   -------
 *     xxxxxxxxxxxx = xxxxxxxxxxxxx
 *
 *   Procedures Called
 *   -----------------
 *
 *************************************<->***********************************/
Boolean 
#ifdef _NO_PROTO
_XmMatchKeyEvent( event, eventType, key, modifiers )
        XEvent *event ;
        int eventType ;
        unsigned int key ;
        unsigned int modifiers ;
#else
_XmMatchKeyEvent(
        XEvent *event,
        int eventType,
        unsigned int key,
        unsigned int modifiers )
#endif /* _NO_PROTO */
{
   if ((event->type == eventType) &&
       (event->xkey.keycode == key) &&
       (event->xkey.state == modifiers))
      return (TRUE);
   else
      return (FALSE);
}
