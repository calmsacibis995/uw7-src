#ifndef	_ALERT_SHP
#define	_ALERT_SHP
#ident	"@(#)debugger:libmotif/common/Alert_shP.h	1.2"

// toolkit specific members of the Alert_shell class
// included by ../../gui.d/common/Alert_sh.h

class Alert_shell;

extern Alert_shell *old_alert_shell;

#define ALERT_SHELL_TOOLKIT_SPECIFICS	\
private:				\
	Boolean		urgent_msg;	\
public:					\
	void		raise();

#endif // _ALERT_SHP
