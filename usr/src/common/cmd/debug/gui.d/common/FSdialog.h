#ifndef FSDIALOG_H
#define FSDIALOG_H
#ident	"@(#)debugger:gui.d/common/FSdialog.h	1.2"

#include "Component.h"
#include "FSdialogP.h"

class Bar_descriptor;
class Vector;

class FileSelectionDialog : public Component
{
	FSDIALOG_TOOLKIT_SPECIFICS

	Callback_ptr	acb;

public:
			FileSelectionDialog(Component *,
				const char *, const char *, Callback_ptr,
				Command_sender *, Help_id = HELP_none,
				Select_mode sm = SM_single,
				Bar_descriptor *bd = 0);
			~FileSelectionDialog();

			// access functions
	Callback_ptr	get_activation_cb() { return acb; }

	int		get_files(Vector *, Boolean sel = FALSE);

			// display functions
	void		popup();
	void		popdown();
	void		wait_for_response();
	void		cmd_done();
};

#endif
