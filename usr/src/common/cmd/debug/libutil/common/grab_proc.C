#ident	"@(#)debugger:libutil/common/grab_proc.C	1.8"

#include "Procctl.h"
#include "utility.h"
#include "Process.h"
#include "Manager.h"
#include "Proglist.h"
#include "Parser.h"
#include "Interface.h"
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// grab one or more live processes

int
grab_process( Filelist *files, char *loadfile, int follow, const char
	*srcpath )
{
	Process	*process, *first = 0;
	char	*name;
	int	i = 0;
	int	ret = 1;

	if (loadfile && (access(loadfile, R_OK) == -1))
	{
		printe(ERR_cant_open, E_ERROR, loadfile, 
			strerror(errno));
		return 0;
	}
	while((name = parse_str_var(0, 0, (*files)[i++])) != 0)
	{
		process = new Process();
		message_manager->reset_context(process);
		if (!process->grab(proglist.next_proc(), 
			name, loadfile, 0, follow, srcpath))
		{
			message_manager->reset_context(0);
			printe(ERR_cant_grab, E_ERROR, name);
			proglist.dec_proc();
			delete process;
			ret = 0;
			continue;
		}
		if (!first)
			first = process;
	}
	if (first)
		proglist.set_current(first, 0);
	message_manager->reset_context(0);
	return ret;
}
