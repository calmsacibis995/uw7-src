#ident	"@(#)olmisc:OlClientsP.h	1.4"

#ifndef _OlClientsP_h
#define _OlClientsP_h

#include <Xol/OlClients.h>

#ifdef DELIMITER
#undef DELIMITER
#endif

#define DELIMITER	0x1f

#define DEF_STRING(s,d)	        (s == NULL ? d : s)
#define NULL_DEF_STRING(s)      DEF_STRING(s,"")

#endif
