#ifndef	NOIDENT
#ident	"@(#)textfield:StepField.h	1.3"
#endif

#ifndef _STEPFIELD_H
#define _STEPFIELD_H

#include "Xol/TextField.h"

extern WidgetClass			stepFieldWidgetClass;

typedef struct _StepFieldClassRec *	StepFieldWidgetClass;
typedef struct _StepFieldRec *		StepFieldWidget;

typedef struct OlTextFieldStepped {
	OlSteppedReason		reason;
	Cardinal		count;
}			OlTextFieldStepped,
		      * OlTextFieldSteppedPointer;

#endif
