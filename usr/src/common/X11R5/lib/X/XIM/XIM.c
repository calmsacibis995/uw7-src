#ident	"@(#)R5Xlib:XIM/XIM.c	1.7"
#include <stdio.h>
#include <dlfcn.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include <X11/Xutil.h>
#include <X11/Xos.h>

#ifndef XSINAME
#define XSINAME		"/usr/X/lib/libXsi.so.5.0"
#endif
#ifndef XIMPNAME
#define XIMPNAME	"/usr/X/lib/libXimp.so.5.0"
#endif

#define XSITYPE 1
#define XIMPTYPE 2
/*
 * GLOBALS
 */
static int (* _XIM_XmbTextListToTextProperty)() = NULL;
static int (* _XIM_XmbTextPropertyToTextList)() = NULL;
static char *(* _XIM_XDefaultString)() = NULL;
static void (* _XIM_XwcFreeStringList)() = NULL;
static int (* _XIM_XwcTextListToTextProperty)() = NULL;
static int (* _XIM_XwcTextPropertyToTextList)() = NULL;

static XrmMethods (* _XIM_XrmInitParseInfo)() = NULL;

static void *_XIM_loader_ptr = NULL;
static struct _XLCd *(*_XIM_fun_ptr)();
static int _XIM_lib_type;

/*
 * function definitions
 */
char *_XIM_figurelib();


/*
 * this _XlcDefaultLoader is the one actually called from X11. Its
 * job is to load the appropriate real input method, depending on
 * the XMODIFIERS=@im= environment variable. Currently we can only
 * use the environement variable, and not XsetLocaleModifiers. The load
 * decision is based on the Novell/USL criteria of XMODIFIER=Local being
 * used for european Xsi based compose languages and XMODIFIERS not set
 * indicate Asian Ximp Server based ones. (a lower case "local" is
 * also supported for Ximp compose).
 *
 * We may come here already loaded
 * since the externally visible direct routines provided by an input
 * method (e.g. XmbTextListToTextProperty) may have caused a load.
 */

XLCd
_XlcDefaultLoader(name)
char *name;
{
	XLCd lcd;

	/* 
	 * Get the appropriate real input method loaded
	 */
	if(!_XIM_loader_ptr)	 {
		if(_XIM_loadit()) {
			return (XLCd) NULL;
		}
	}

	/*
	 * call the loader section of the loaded input method. this sets
	 * up all the generic input method part of X11 to the input
	 * method just loaded. 
	 */

	if(_XIM_fun_ptr) {
		lcd = (*_XIM_fun_ptr)(name);
		return(lcd);	
	}
	else {
		return (XLCd) NULL;
	}
}

/*
 * figure out which input method lib to load and load it returning the
 * address of the real _XlcDefaultLoader. So _XlcDefaultLoader notes above
 * for more detail
 * 
 * this is called when either X11 has decided it is input method load time
 * or one of the externally visible direct input method functions are called
 */
_XIM_loadit()
{
	char *libname;

	/*	
	 * figure out the name of the real input method to load
	 */
	if((libname = _XIM_figurelib()) == NULL) {
		return -1;
	}

	/*
	 * actually loadit, store the loader handle in a global area for
	 * all to see. it will also set global on type of input method loaded
	 */
	if ((_XIM_loader_ptr = dlopen(libname, RTLD_NOW)) == (void *)NULL) {
#ifdef DEBUG
		printf("dlopen: %s\n", dlerror());
#endif
		return -1; 
	}

	/*
	 * resolve all the external references directly into the input method.
	 * This lib provides 'redirect stubs' for all these externally
	 * visible routines. we look the 'real' routine names up in what
	 * we loaded and set up pointers so that when the user calls one
	 * of the ones here it is indirectly calling these routines.
	 */
	if(_XIM_resolveexterns()){
		return -1;
	}

	/*
	 * we need to return the address of the 'real' _XlcDefaultLoader
	 * so that it can be called as needed
	 */
	if ((_XIM_fun_ptr = (struct _XLCd *(*)())dlsym(_XIM_loader_ptr, "__XlcDefaultLoader")) == NULL) {
#ifdef DEBUG
		printf("dlsym(_XlcDefaultLoader)%s\n", dlerror());
#endif
		return -1;
	}

	return(0);

}

/*
 * figure out which IM lib to load using the XMODIFIERS environment variable
 * current logic is, im=@Local or None or NONE or _XIM_INPUTMETHOD indicate 
 * Xsi all others are Ximp.
 */
char *
_XIM_figurelib()
{
	char *name;
	char *user_mods;
	char *str, *p;
	char strbuf[128];

	/* 
	 * throughout, the default, nothing set kinda case is XIMP (just
	 * a policy decision)
	 */
	if((user_mods = getenv("XMODIFIERS")) == NULL) {
		/*
		 * No XMODIFIERS is the prefered method for getting to
		 * XIMP-front end stuff like 'Vje'
		 */
		_XIM_lib_type = XIMPTYPE;
	}
	else {
		str = user_mods;
    		while (str && (strlen(str) > 0) && strncmp(str+1, "im=", 3))
			str = index(str+1, '@');
    		if (!str) {
		 	strbuf[0] = '\0';
		}
		else {
        		str += 4;
        		p = index(str, '@');
        		if (p) {
            			strncpy(strbuf, str, p - str);
            			strbuf[p - str] = '\0';
        		} else {
            			strcpy(strbuf, str);
        		}
    		}
    		if (strbuf[0] == '\0') {
			/* 
			 * no im= stuff so assume XIMP
			 */
			_XIM_lib_type=XIMPTYPE;
		} else {
			/*
			 * some im= stuff. see what it is set to
			 */
			if(!strcmp(strbuf, "Local") || !strcmp(strbuf, "None")
			    || !strcmp(strbuf, "NONE") || !strcmp(strbuf, "_XIM_INPUTMETHOD")) {
				/* 
				 * 'Local' is the one most likely to mean
				 * Xsi 'compose' stuff. 'None' and 'NONE'
				 * were thrown in as ones that The XSi code
				 * looks for. _XIM_INPUTMETHOD would be used
				 * for front end Xsi servers.
				 */ 
				_XIM_lib_type = XSITYPE;
			}
			else if (!strcmp(strbuf, "local")) {
				/* 
				 * 'local' is used by Ximp to indicate 
				 * to use the local compose files
				 */
				_XIM_lib_type = XIMPTYPE;
			}
			else {
				/*
				 * unknown im=, set to XIMPTYPE
				 */
				_XIM_lib_type = XIMPTYPE;
			}
		}
	}


	if((name = Xmalloc(((_XIM_lib_type == XIMPTYPE) ? sizeof(XIMPNAME) : sizeof(XSINAME) )+ 1)) == NULL) {
		return(NULL);
	}

	strcpy(name, (_XIM_lib_type == XIMPTYPE) ? XIMPNAME : XSINAME);

	return(name);
}

int
_XIM_resolveexterns()
{
	if ((_XIM_XmbTextListToTextProperty = (int(*)())dlsym(_XIM_loader_ptr, "_XmbTextListToTextProperty")) == NULL) {
#ifdef DEBUG
		printf("dlsym(XmbTextListToTextProperty)%s\n", dlerror());
#endif
		return -1;
	}

	if ((_XIM_XmbTextPropertyToTextList = (int(*)())dlsym(_XIM_loader_ptr, "_XmbTextPropertyToTextList")) == NULL) {
#ifdef DEBUG
		printf("dlsym(XmbTextPropertyToTextList)%s\n", dlerror());
#endif
		return -1;
	}

	if ((_XIM_XDefaultString = (char *(*)())dlsym(_XIM_loader_ptr, "_XDefaultString")) == NULL) {
#ifdef DEBUG
		printf("dlsym(XDefaultString)%s\n", dlerror());
#endif
		return -1;
	}

	if ((_XIM_XwcFreeStringList = (void(*)())dlsym(_XIM_loader_ptr, "_XwcFreeStringList")) == NULL) {
#ifdef DEBUG
		printf("dlsym(XwcFreeStringList)%s\n", dlerror());
#endif
		return -1;
	}

	if ((_XIM_XwcTextListToTextProperty = (int(*)())dlsym(_XIM_loader_ptr, "_XwcTextListToTextProperty")) == NULL) {
#ifdef DEBUG
		printf("dlsym(XwcTextListToTextProperty)%s\n", dlerror());
#endif
		return -1;
	}

	if ((_XIM_XwcTextPropertyToTextList = (int(*)())dlsym(_XIM_loader_ptr, "_XwcTextPropertyToTextList")) == NULL) {
#ifdef DEBUG
		printf("dlsym(XwcTextPropertyToTextList)%s\n", dlerror());
#endif
		return -1;
	}


	if ((_XIM_XrmInitParseInfo = (XrmMethods(*)())dlsym(_XIM_loader_ptr, "__XrmInitParseInfo")) == NULL) {
#ifdef DEBUG
		printf("dlsym(_XrmInitParseInfo)%s\n", dlerror());
#endif
		return -1;
	}

	return(0);
}



/*
 * THe following routines are defined in Xsi and Ximp and advertized
 * to the world. If they are called we will cause a load of the input method
 * even though the user may never call a Xinputmethod routine to do the
 * real load. This is not different that before since we would have
 * pulled the lib in before. Whenever they are called we pass the user on
 * to the routine in the module loaded.
 */

#if NeedFunctionPrototypes
int
XmbTextListToTextProperty(
    Display           *dpy,
    char             **list,
    int                count,
    XICCEncodingStyle  style,
    XTextProperty     *text_prop
)
#else
int
XmbTextListToTextProperty(dpy, list, count, style, text_prop)
    Display           *dpy;
    char             **list;
    int                count;
    XICCEncodingStyle  style;
    XTextProperty     *text_prop;
#endif
{

	if(!_XIM_loader_ptr) {
		if(_XIM_loadit()) {
			return (XConverterNotFound);
		}
	}

	if (_XIM_XmbTextListToTextProperty) {
		return (*_XIM_XmbTextListToTextProperty)(dpy, list, count, style, text_prop);
	}
	else {
		return (XConverterNotFound);
	}

}

#if NeedFunctionPrototypes
int
XmbTextPropertyToTextList(
    Display *dpy,
    XTextProperty *tp,
    char ***list_return,
    int *count_return
)
#else
int
XmbTextPropertyToTextList(dpy, tp, list_return, count_return)
    Display *dpy;
    XTextProperty *tp;
    char ***list_return;
    int *count_return;
#endif
{
	if(!_XIM_loader_ptr) {
		if(_XIM_loadit()) {
			return (XConverterNotFound);
		}
	}

	if (_XIM_XmbTextPropertyToTextList) {
		return(*_XIM_XmbTextPropertyToTextList)(dpy, tp, list_return, count_return);
	} else {
		return (XConverterNotFound);
	}
}


char *
XDefaultString()
{
	if(!_XIM_loader_ptr) {
		if(_XIM_loadit()) {
			return "";
		}
	}

	if (_XIM_XDefaultString) {
		return(*_XIM_XDefaultString)();
	} else {
		return "";
	}
}


void
XwcFreeStringList(list)
    wchar_t **list;
{
	if(!_XIM_loader_ptr) {
		if(_XIM_loadit()) {
			return;
		}
	}

	if (_XIM_XwcFreeStringList) {
		(*_XIM_XwcFreeStringList)(list);
		return;
	} else {
		return;
	}
}

#if NeedFunctionPrototypes
int
XwcTextListToTextProperty(
    Display           *dpy,
    wchar_t          **list,
    int                count,
    XICCEncodingStyle  style,
    XTextProperty     *text_prop
)
#else
int
XwcTextListToTextProperty(dpy, list, count, style, text_prop)
    Display           *dpy;
    wchar_t          **list;
    int                count;
    XICCEncodingStyle  style;
    XTextProperty     *text_prop;
#endif
{
	if(!_XIM_loader_ptr) {
		if(_XIM_loadit()) {
			return;
		}
	}

	if (_XIM_XwcTextListToTextProperty) {
		return(*_XIM_XwcTextListToTextProperty)(dpy, list, count, style, text_prop);
	} else {
		return;
	}
}

#if NeedFunctionPrototypes
int
XwcTextPropertyToTextList(
    Display *dpy,
    XTextProperty *tp,
    wchar_t ***list_return,
    int *count_return
)
#else
int
XwcTextPropertyToTextList(dpy, tp, list_return, count_return)
    Display *dpy;
    XTextProperty *tp;
    wchar_t ***list_return;
    int *count_return;
#endif
{
	if(!_XIM_loader_ptr) {
		if(_XIM_loadit()) {
			return;
		}
	}

	if (_XIM_XwcTextPropertyToTextList) {
		return(*_XIM_XwcTextPropertyToTextList)(dpy, tp, list_return, count_return);
	} else {
		return;
	}
}



XrmMethods
_XrmInitParseInfo(state)
XPointer *state;
{
	if(!_XIM_loader_ptr) {
		if(_XIM_loadit()) {
			return (NULL);
		}
	}

	if (!_XIM_XrmInitParseInfo)
		return(NULL);
	return(*_XIM_XrmInitParseInfo)(state);
}

