#ifndef FSDIALOGP_H
#define FS_DIALOGP_H
#ident	"@(#)debugger:libmotif/common/FSdialogP.h	1.2"

class Bar_descriptor;

#define FSDIALOG_TOOLKIT_SPECIFICS	\
private:				\
		Widget	shell;		\
		Widget	*buttons;	\
		Bar_descriptor	*extra_buttons;	\
		int	last_selection;	\
		int	selected;	\
		Boolean cmd_sent;	\
		Boolean	_is_open;	\
public:					\
		void	reset_sensitivity();	\
		void	set_sensitivity(Boolean);	\
		void	handle_select(XmListCallbackStruct *);	\
		void	handle_mselect(XmListCallbackStruct *);

#endif
