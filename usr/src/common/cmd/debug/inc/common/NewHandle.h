#ifndef _NewHandle_h
#define _NewHandle_h
#ident	"@(#)debugger:inc/common/NewHandle.h	1.1"

#include <setjmp.h>

// This class provides a common interface for debug consumers
// that want to install their own new handlers.  It provides
// for setting/restoring handlers and for a setjmp/longjmp
// environment

class NewHandler {
	void	(*user_handler)();
	void	(*old_handler)();
	jmp_buf	user_env;
public:
		NewHandler() { user_handler = old_handler = 0; }
		~NewHandler() {}
	void	install_user_handler(void (*)());
	void	restore_old_handler();
	void	return_to_saved_env(int retval);
	void	invoke_handler();
	jmp_buf	*get_jmp_buf() { return &user_env; }
};

extern NewHandler	newhandler;

#endif
