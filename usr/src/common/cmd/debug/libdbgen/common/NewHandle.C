#ident	"@(#)debugger:libdbgen/common/NewHandle.C	1.3"

#include <setjmp.h>
#include <new>
#include "NewHandle.h"
#include "UIutil.h"

NewHandler	newhandler;

#if COMPILER_SUPPORTS_NAMESPACES
using namespace std;
#endif

void
NewHandler::install_user_handler(void (*handler)())
{
	if (old_handler)
	{
		// nested handlers not allowed
		interface_error("NewHandler::install_user_handler",
			__LINE__, 1);
		return;
	}
	set_new_handler(handler);
	old_handler = user_handler;
	user_handler = handler;
}

void
NewHandler::restore_old_handler()
{
	if (!old_handler)
	{
		interface_error("NewHandler::restore_old_handler",
			__LINE__, 1);
		return;
	}
	set_new_handler(old_handler);
	user_handler = old_handler;
	old_handler = 0;
}

void
NewHandler::invoke_handler()
{
	if (!user_handler)
		return;
	(*user_handler)();
}

// function invoked by user's handler
// doesn't return
void
NewHandler::return_to_saved_env(int retval)
{
	longjmp(user_env, retval);
}
