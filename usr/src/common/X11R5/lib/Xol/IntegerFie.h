#ifndef	NOIDENT
#ident	"@(#)textfield:IntegerFie.h	1.3"
#endif

#ifndef _INTEGERFIELD_H
#define _INTEGERFIELD_H

#include "Xol/TextField.h"

extern WidgetClass			integerFieldWidgetClass;

typedef struct _IntegerFieldClassRec *	IntegerFieldWidgetClass;
typedef struct _IntegerFieldRec *	IntegerFieldWidget;

typedef struct OlIntegerFieldChanged {
	int			value;
	Boolean			changed;
	OlTextVerifyReason	reason;
}			OlIntegerFieldChanged;

#endif
