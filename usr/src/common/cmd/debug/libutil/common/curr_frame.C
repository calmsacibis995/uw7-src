#ident	"@(#)debugger:libutil/common/curr_frame.C	1.5"
#include "utility.h"
#include "ProcObj.h"
#include "Frame.h"
#include "Proctypes.h"
#include "global.h"
#include "Interface.h"

#include <signal.h>

// get and set current frame

// in all cases, we assume the higher level functions
// have tested for a null ProcObj

int
curr_frame(ProcObj *pobj)
{
	Frame	*cframe;

	cframe = pobj->curframe();
	if (!stack_count_down)
	{
		if (cframe)
			return cframe->get_level();
		return 0;
	}
	else
	{
		int	count = 0;
		while((cframe = cframe->caller()) != 0)
		{
			count++;
		}
		return count;
	}
}

// total number of valid frames - 1
// actually, returns highest numbered frame,
// where frames are numbered from 0
int
count_frames(ProcObj *pobj)
{
	Frame	*cframe;
	int	count = 0;

	cframe = pobj->topframe();
	while((cframe = cframe->caller()) != 0)
	{
		count++;
		if (prismember(&interrupt, SIGINT))
		{
			// print_stack released SIGINT
			// delete signal from interrupt
			// set to let print_stack print
			// out what we have found so far
			prdelset(&interrupt, SIGINT);
			break;
		}
	}
	return count;
}

int 
set_frame(ProcObj *pobj, int frameno)
{
	Frame	*cframe;
	int	count;

	if (frameno < 0)
		goto range_err;
	cframe = pobj->topframe();
	if (stack_count_down)
	{
		count = count_frames(pobj);
		if (frameno > count)
			goto range_err;
		while(frameno < count)
		{
			cframe = cframe->caller();
			count--;
		}
	}
	else
	{
		for(int i = 0; cframe && (i < frameno); i++)
			cframe = cframe->caller();
		if (!cframe)
			goto range_err;
	}
	if (!pobj->setframe(cframe))
		return 0;

	if (get_ui_type() == ui_gui)
		printm(MSG_set_frame, (unsigned long)pobj, frameno);
	return 1;
range_err:
	printe(ERR_frame_range, E_ERROR, frameno, pobj->obj_name());
	return 0;
}
