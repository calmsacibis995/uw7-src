#pragma	ident	"@(#)builtinext:lib/XBuiltin.c	1.5"

#include <limits.h>
#include <unistd.h>

#define NEED_REPLIES
#define NEED_EVENTS
#include "Xlibint.h"
#include "Xext.h"
#include "extutil.h"
#include "builtinstr.h"

static XExtensionInfo	builtin_info_data;
static XExtensionInfo *	builtin_info		= &builtin_info_data;
static const char *	builtin_extension_name	= BUILTIN_EXT_NAME;

#define BuiltinCheckExtension(dpy,i,val) \
    XextCheckExtension(dpy, i, builtin_extension_name, val)
#define BuiltinSimpleCheckExtension(dpy,i) \
    XextSimpleCheckExtension(dpy, i, builtin_extension_name)

static int	close_display();
static Bool	wire_to_event();
static Status	event_to_wire();
static const XExtensionHooks builtin_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    wire_to_event,			/* wire_to_event */
    event_to_wire,			/* event_to_wire */
    NULL,				/* error */
    NULL				/* error_string */
};
static
XEXT_GENERATE_FIND_DISPLAY(find_display, builtin_info, builtin_extension_name,
			   &builtin_extension_hooks, BuiltinNumberEvents, NULL)
static
XEXT_GENERATE_CLOSE_DISPLAY(close_display, builtin_info)

/*****************************************************************************
 *
 *		PRIVATE FUNCTIONS
 */

static Bool
wire_to_event(Display * dpy, XEvent * lib_event, xEvent * wire_event)
{
    XExtDisplayInfo *	info = find_display (dpy);
    
    BuiltinCheckExtension(dpy, info, False);

    switch ((wire_event->u.u.type & 0x7f) - info->codes->first_event)
    {
    case BuiltinExecNotify:
    {
	XBuiltinExecNotifyEvent * lib = (XBuiltinExecNotifyEvent *)lib_event;
	xBuiltinExecNotifyEvent * wire = (xBuiltinExecNotifyEvent *)wire_event;

	lib->type	= wire->type & 0x7f;
    	lib->serial	= _XSetLastRequestRead(dpy,(xGenericReply*)wire_event);
    	lib->send_event	= (wire->type & 0x80) != 0;
    	lib->display	= dpy;
    	lib->window	= 0;			/* unused */
    	return True;
    }
    case BuiltinExitNotify:
    {
	XBuiltinExitNotifyEvent * lib = (XBuiltinExitNotifyEvent *)lib_event;
	xBuiltinExitNotifyEvent * wire = (xBuiltinExitNotifyEvent *)wire_event;

	lib->type	= wire->type & 0x7f;
    	lib->serial	= _XSetLastRequestRead(dpy,(xGenericReply*)wire_event);
    	lib->send_event	= (wire->type & 0x80) != 0;
    	lib->display	= dpy;
    	lib->window	= 0;			/* unused */
	lib->exit_code	= wire->exit_code;
	return True;
    }
    }
    return False;
}

static Status
event_to_wire(Display * dpy, XEvent * lib_event, xEvent * wire_event)
{
    XExtDisplayInfo *	info = find_display (dpy);

    BuiltinCheckExtension (dpy, info, False);

    switch ((lib_event->type & 0x7f) - info->codes->first_event) {
    case BuiltinExecNotify:
    {
	XBuiltinExecNotifyEvent * lib = (XBuiltinExecNotifyEvent *)lib_event;
	xBuiltinExecNotifyEvent * wire = (xBuiltinExecNotifyEvent *)wire_event;

	wire->type		= lib->type;
	wire->sequenceNumber	= lib->serial & 0xffff;
	return True;
    }
    case BuiltinExitNotify:
    {
	XBuiltinExitNotifyEvent * lib = (XBuiltinExitNotifyEvent *)lib_event;
	xBuiltinExitNotifyEvent * wire = (xBuiltinExitNotifyEvent *)wire_event;

	wire->type		= lib->type;
	wire->sequenceNumber	= lib->serial & 0xffff;
	wire->exit_code		= lib->exit_code;
	return True;
    }
    }
    return False;
}

/*****************************************************************************
 * FormatRunClientReq-
 */
static int
FormatRunClientReq(int argc, char * argv[], char * envp[], char * buf)
{
    int		i;
    char **	ep;
    char *	p = buf;

    /* First in request is uid, gid, and cwd, then argv & env strings */
    *(gid_t *)p = getgid();		/* Include the runner's gid */
    p += sizeof(gid_t);
    *(uid_t *)p = getuid();		/* Include the runner's uid */
    p += sizeof(uid_t);
    (void)getcwd(p, PATH_MAX + 1);
    p += strlen(p) + 1;

    /* Put args into request as [len(1), argv(1), len(2), argv(2), ...] */
    for (i = 0; i < argc; i++)
    {
	int len = strlen(argv[i]);
	*p++ = len;
	memcpy(p, argv[i], len);
	p += len;
    }
    *p++ = '\0';			/* terminate argv list */

    /* Shovel in the environment variables */
    for (ep = envp; *ep; ep++)
    {
	int len = strlen(*ep);
	*(ushort *)p = len;
	p += sizeof(ushort);
	memcpy(p, *ep, len);
	p += len;
    }
    *(ushort *)p = 0;			/* terminate list of env vars */
    p += sizeof(ushort);

    return(p - buf);			/* return size */
}

/*****************************************************************************
 *
 *		PUBLIC FUNCTIONS
 */
Bool
XBuiltinQueryExtension(Display * dpy, int * event_base, int * error_base)
{
    XExtDisplayInfo *info = find_display(dpy);

    if (XextHasExtension(info)) {
	*event_base = info->codes->first_event;
	*error_base = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}

Status
XBuiltinQueryVersion(Display * dpy, char * name, unsigned char * ret_valid, int * ret_major_version, int * ret_minor_version)
{
    XExtDisplayInfo *		info = find_display(dpy);
    xBuiltinQueryVersionReq *	req;
    xBuiltinQueryVersionReply	rep;
    int				size;
    Status			status;

    BuiltinCheckExtension(dpy, info, False);

    /* If name is NULL or NULL string, fail immediaetly.
     * (Add 1 to transmit NULL byte)
     */
    if ((name == NULL) || ((size = strlen(name) + 1) == 1))
	return(False);
    
    LockDisplay(dpy);
    GetReq(BuiltinQueryVersion, req);
    req->reqType	= info->codes->major_opcode;
    req->builtinReqType	= X_BuiltinQueryVersion;
    req->length		+= (size+3) >> 2;
    _XSend(dpy, name, size);
    status = _XReply(dpy, (xReply *)&rep, 0, True);
    if (status)
    {
	*ret_major_version	= rep.majorVersion;
	*ret_minor_version	= rep.minorVersion;
	*ret_valid		= rep.valid;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return(status);
}

Status
XBuiltinRunClient(Display * dpy, int argc, char * argv[], char * envp[])
{
    XExtDisplayInfo *		info = find_display(dpy);
    xBuiltinRunClientReq *	req;
    xReply			rep;
    char			data[10240];
    int				size;
    Status			status;

    BuiltinCheckExtension(dpy, info, False);

    size = FormatRunClientReq(argc, argv, envp, data);

    LockDisplay(dpy);
    GetReq(BuiltinRunClient, req);
    req->reqType	= info->codes->major_opcode;
    req->builtinReqType	= X_BuiltinRunClient;
    req->length		+= (size+3) >> 2;
    _XSend(dpy, data, size);
    status = _XReply(dpy, &rep, 0, True);
    UnlockDisplay(dpy);
    SyncHandle();
    return(status);
}

#include <dlfcn.h>
#include <stdio.h>
/*****************************************************************************
 * BuiltinRegisterClient-
 */
void
BuiltinRegisterClient(Display * dpy,
		      void (*MainLoop)(Display *, int), 
		      void (*CleanUp)(Display *))
{
    void * self;
    void (*register_client)(int, Display *,
			    void (*)(Display *, int), void (*)(Display *));

    if ((self = dlopen(NULL, RTLD_LAZY)) &&
	(register_client =
	 (void (*)(int, Display *,
		   void (*)(Display *, int), void (*)(Display *)))
	 dlsym(self, "BIRegisterClient")))
    {
	(*register_client)(dpy->fd, dpy, MainLoop, CleanUp);
	(void)dlclose(self);

    } else
    {
	fprintf(stderr, "%s\n", dlerror());
    }
}
