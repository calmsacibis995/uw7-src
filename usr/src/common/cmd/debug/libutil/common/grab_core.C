#ident	"@(#)debugger:libutil/common/grab_core.C	1.4"

#include "global.h"
#include "utility.h"
#include "Process.h"
#include "Manager.h"
#include "Proglist.h"
#include "Interface.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

// set up for debugging a post-mortem image

int
grab_core( char *adotout, char *core, const char *srcpath )
{
	int		textfd, corefd = -1;
	Process		*process;
	time_t		SymFilTime;
	struct stat	stbuf;

	if (!adotout || !core)
	{
		printe(ERR_internal, E_ERROR, "grab_core", __LINE__);
		return 0;
	}
	if (stat(adotout,&stbuf) == -1) 
	{
		printe(ERR_no_access, E_ERROR, adotout);
		return 0;
	}

	// Check that core file is newer than symbol file 
	SymFilTime = stbuf.st_mtime;
	if (stat(core,&stbuf) == -1) 
	{
		printe(ERR_no_access, E_ERROR, core);
		return 0;
	}
	if (SymFilTime > stbuf.st_mtime)
	{
		printe(ERR_newer_file, E_WARNING, adotout, core);
	}

	if ( (textfd = open(adotout, O_RDONLY)) == -1 )
	{
		printe(ERR_cant_open, E_ERROR, adotout, 
			strerror(errno));
		return 0;
	}
	else if ((corefd = open(core, O_RDONLY)) == -1 )
	{
		close( textfd );
		printe(ERR_cant_open, E_ERROR, core, strerror(errno));
		return 0;
	}
	process = new Process();
	message_manager->reset_context(process);
	if (!process->grab_core( textfd, corefd, proglist.next_proc(),
		adotout, srcpath ))
	{
		proglist.dec_proc();
		delete process;
		printe(ERR_cant_grab, E_ERROR, adotout);
		close( corefd );
		close( textfd );
		return 0;
	}
	close( corefd );
	close( textfd );
	proglist.set_current(process, 0);
	message_manager->reset_context(0);

	return 1;
}
