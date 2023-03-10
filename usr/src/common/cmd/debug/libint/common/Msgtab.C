#ident	"@(#)debugger:libint/common/Msgtab.C	1.2"

#include "Msgtab.h"

#include "Msgtypes.h"
#include "Signature.h"
#include "UIutil.h"

#include <stdio.h>
#include <unistd.h>

extern Msgtab mtable[];
// the contents of the table are generated by an awk script from
// Msg.awk.in in inc/common

Message_table Mtable;

Message_table::Message_table()
{
	msgtab = mtable;
}

void
Message_table::init()
{
	// determine whether we have a catalog available -
	// otherwise always use default strings
	char	*default_string = "a";
	char buf[sizeof(CATALOG) + 10];

	sprintf(buf, "%s:%d", CATALOG, 1);
	cat_available = (gettxt(buf, default_string) != default_string);
}

#ifndef NOCHECKS
Signature
Message_table::signature(Msg_id mid)
{
	if ((mid <= MSG_invalid)
		|| (mid >= MSG_last)
		|| (msgtab[mid].signature <= SIG_invalid)
		|| (msgtab[mid].signature >= SIG_last))
		interface_error("Message_table::signature", __LINE__);
	return(msgtab[mid].signature);
}

Msg_class
Message_table::msg_class(Msg_id mid)
{
	if ((mid <= MSG_invalid)
		|| (mid >= MSG_last)
		|| (msgtab[mid].signature <= SIG_invalid)
		|| (msgtab[mid].signature >= SIG_last)
		|| (msgtab[mid].mclass < MSGCL_error))
		interface_error("Message_table::msg_class", __LINE__);
	return(msgtab[mid].mclass);
}
#endif	// NOCHECKS
	
const char *
Message_table::format(Msg_id mid)
{

#ifndef NOCHECKS
	if ((mid <= MSG_invalid)
		|| (mid >= MSG_last)
		|| (msgtab[mid].signature <= SIG_invalid)
		|| (msgtab[mid].signature >= SIG_last)
		|| (msgtab[mid].mclass < MSGCL_error))
		interface_error("Message_table::format", __LINE__);
#endif

	if (!msgtab[mid].format)
		return 0;

	// If the message is not translatable or there is no
	// translated catalog, just return the default message.
	// If there is a translated catalog,
	// read the string from the message database and cache it.
	// The call to gettxt will look like:
	// gettxt("database_name:number", "string")
	// if 1) setlocale has been called to change the locale from "C" to
	//	 something else,
	//    2) there is a file named database_name in the locale's directory
	//	 (see locale(3C)), and
	//    3) it can find the string with that index in the catalog,
	// it will return the translated string
	// if 1 and 2 aren't true, it returns "string" as is
	// if it can't find the string in the database, it returns
	// 	"Message not found!!"

	register Msgtab	*mptr = &msgtab[mid];

	if (!mptr->set_local)
	{
		mptr->set_local = 1;
		if (cat_available && mptr->catindex)
		{
			// enough space for catalog:number
			char buf[sizeof(CATALOG) + 10];	
			sprintf(buf, "%s:%d", CATALOG, mptr->catindex);
			mptr->format = gettxt(buf, mptr->format);
		}
	}
	return mptr->format;
}
