#ident	"@(#)variables.C	1.2"
/*  variables.C
//
//  This file contains all of the supporting functions for the various
//  variable types.
*/


#include	<iostream.h>		//  for cout()

#include	<Xm/Xm.h>

#include	"cDebug.h"		//  for log() functions and defs
#include	"setupAPIs.h"		//  for setupType_t definition






/*  Local variables, functions, etc.					     */

// void	getStringValue (setupObject_t *curObj, char **buff);
// void	getIntValue (setupObject_t *curObj, char **buff);






/* ///////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void getStringValue (setupObject_t *curObj, char **buff)
//
//  DESCRIPTION:
//	The value of the svt_string type variable is retrieved.
//
//  RETURN:
//	The value of the svt_string variable in buff.  If the value
//	cannot be retrieved, buff is left unchanged.
//
/////////////////////////////////////////////////////////////////////////// */

void
getStringValue (setupObject_t *curObj, char **buff)
{
	char	*newBuff = NULL;


	if (setupObjectGetValue (curObj, &newBuff))
	{
		log1 (C_ERR, "setupObjectGetValue() returned non-zero!");
	}
	else
		if (newBuff != NULL)
		{
			*buff = newBuff;
		}

}	//  End  getStringValue ()



/* ///////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void getIntValue (setupObject_t *curObj, char **buff)
//
//  DESCRIPTION:
//	The value of the svt_integer type variable is retrieved.
//
//  RETURN:
//	The value (as a string) of the svt_integer variable is returned in.
//	buff. 
//
/////////////////////////////////////////////////////////////////////////// */

void
getIntValue (setupObject_t *curObj, char **buff)
{
	unsigned long	*longValue = (unsigned long *)0;
	char		*format = NULL;


	(*buff)[0] = '\0';

	if (!(setupObjectGetValue (curObj, &longValue)))
	{
		if (longValue)
		{
			//  Some svt_integer types need to be displayed in hex,
			//  or some other format besides decimal, so we use the
			//  format string, if one is available (which is
			//  compatible with sprintf()).
			if (format = setupObjectGetFormat (curObj))
			{
				log4 (C_API, "format string = ", format,
				", value = ", *longValue);
				sprintf (*buff, format, *longValue);
			}
			else
			{
				sprintf (*buff, "%u", *longValue);
			}
		}
	}

}	//  End  getIntValue ()
