/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:MaskArgs.c	1.18"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file contains routines that are used in building
 *	up Arg Lists.
 *
 *******************************file*header*******************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static Boolean    UpdateList();		/* support for _OlComposeArgList()*/

					/* public procedures		*/

void	_OlComposeArgList();		/* Composes an Arg list		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define LAST	1
#define MORE	2
#define IGNORE	3

#define OleMolComposeArgList_invalidParameters	"_OlComposeArgList:\
 OL_COPY_SIZE must specify size greater than zero"

#define OleMolComposeArgList_wrongParameters	"_OlComposeArgList: must\
 specify OL_COPY_SIZE as next field when using rule %s on resource \"%s\""

#define OleMolComposeArgList_nullDestination	"_OlComposeArgList: Attempt\
 to write to NULL memory location for resource \"%s\" with rule %s"

#define OleMolComposeArgList_unknownRule	"_OlComposeArgList:\
 unknown rule used for resource \"%s\""

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * UpdateList - this routine updates the destination list provided by
 * the routine _OlComposeArgList().
 * The function returns True if the calling routine must increment its
 * MaskArg list count; False is returned otherwise.
 *************************************************************************
 */
static Boolean
UpdateList(mask_arg, source_arg, match, buf, count, mlist_status)
	MaskArg	*	mask_arg;
	Arg *		source_arg;
	Boolean		match;
	ArgList		buf;
	Cardinal *	count;
	int		mlist_status;
{
	Arg *		arg_ptr;
	Boolean		new_entry = False;
	register int	c = *count;
	int		size;
	Boolean		increment = False;

	switch(mask_arg->rule) {
	case OL_SOURCE_PAIR:
				/* Don't have to check for match since
				 * we can get here only if a match
				 * occurred				*/
		new_entry = True;
		arg_ptr   = source_arg;
		break;
	case OL_MASK_PAIR:
				/* Don't have to check for match since
				 * we can get here only if a match
				 * occurred				*/
		new_entry = True;
		arg_ptr   = (Arg *) mask_arg;
		break;
	case OL_DEFAULT_PAIR:
		new_entry = True;
		if (match == True)
			arg_ptr = source_arg;
		else
			arg_ptr = (Arg *) mask_arg;
		break;
	case OL_ABSENT_PAIR:
		if (match == True)
			break;
		new_entry = True;
		arg_ptr   = (Arg *) mask_arg;
		break;
	case OL_OVERRIDE_PAIR:
				/* Don't check for match, since this rule
				 * applies whether or not a match occurs*/
		new_entry = True;
		arg_ptr   = (Arg *) mask_arg;
		break;
	case OL_COPY_MASK_VALUE:
						/* Fall Through		*/
	case OL_COPY_SOURCE_VALUE:
				/* Don't have to check for match since
				 * we can get here only if a match
				 * occurred				*/

		if ((mask_arg->rule == OL_COPY_MASK_VALUE &&
		     source_arg->value == (XtArgVal) NULL) ||
		    (mask_arg->rule == OL_COPY_SOURCE_VALUE &&
		     mask_arg->value == (XtArgVal) NULL))
		{
			OlVaDisplayWarningMsg((Display *)NULL,
				OleNolComposeArgList, OleTnullDestination,
				OleCOlToolkitWarning,
				OleMolComposeArgList_nullDestination,
				source_arg->name,
				(mask_arg->rule == OL_COPY_MASK_VALUE ?
				"OL_COPY_MASK_VALUE" : "OL_COPY_SOURCE_VALUE"));
		}
		else if (mlist_status == LAST ||
		    mask_arg[1].rule != OL_COPY_SIZE)
		{
				/* When copying a value, we have to get
				 * the next MaskArg since it has the size
				 * of the value to copy			*/

			OlVaDisplayWarningMsg((Display *)NULL,
				OleNolComposeArgList, OleTwrongParameters,
				OleCOlToolkitWarning,
				OleMolComposeArgList_wrongParameters,
				(mask_arg->rule == OL_COPY_MASK_VALUE ?
				"OL_COPY_MASK_VALUE" : "OL_COPY_SOURCE_VALUE"),
				source_arg->name);
		}
		else if ((size = (int)mask_arg[1].value) <= 0)
		{
			OlVaDisplayWarningMsg((Display *)NULL,
				OleNolComposeArgList, OleTinvalidParameters,
				OleCOlToolkitWarning,
				OleMolComposeArgList_invalidParameters);
		}
		else {
				/* Copy the data from the mask list into
				 * the location defined in the source
				 * list					*/

			if (mask_arg->rule == OL_COPY_MASK_VALUE)
			{
				_OlCopyFromXtArgVal(mask_arg->value,
						(char *) source_arg->value,
						(Cardinal)size);
			}
			else			/* OL_COPY_SOURCE_VALUE	*/
			{
				_OlCopyFromXtArgVal(source_arg->value,
						(char *) mask_arg->value,
						(Cardinal) size);
			}
			increment = True;
		}
		break;
	default:
		OlVaDisplayWarningMsg((Display *)NULL, OleNolComposeArgList,
			OleTunknownRule, OleCOlToolkitWarning,
			OleMolComposeArgList_unknownRule, mask_arg->name);
		break;
	}

	if (new_entry == True) {
		buf[c].name  = arg_ptr->name;
		buf[c].value = arg_ptr->value;
		++*count;
	} /* END OF if (new_entry == True) */

	return(increment);
} /* END OF UpdateList() */

/*
 *************************************************************************
 * _OlComposeArgList - this procedure builds a resource list using the
 * information supplied in the MaskArg list.
 * It's the application's responsibility to free the returned list.
 *
 * Below is the list of allowable build modes.
 *
 *	Build Mode		Action
 *	----------------	-----------------------------------------
 *	OL_SOURCE_PAIR		if mask list resource matches source list
 *				resource, the source name/value pair is
 *				copied into the destination list.
 *	OL_MASK_PAIR		if mask list resource matches source list
 *				resource, the mask name/value pair is
 *				copied into the destination list
 *	OL_ABSENT_PAIR		if mask list resource does not match any
 *				source list resource, the mask name/value
 *				pair is copied into the destination list.
 *	OL_DEFAULT_PAIR		if mask list resource does not match any
 *				source list resources, the mask name/value
 *				pair is copied into the destination list.
 *				But if it does match, the name/value
 *				pair from the source list is copied into
 *				the destination list.
 *	OL_OVERRIDE_PAIR	Always puts the mask name/value into the
 *				destination list whether or not the
 *				resource exists in the source list.
 *	OL_COPY_SOURCE_VALUE	If the mask list resource name matches
 *				a source list resource name, the source arg
 *				list value is copied into the address
 *				specified by the mask arg list value.
 *				This rule does not affect the composed
 *				Arg list.  Note: to use this rule, the
 *				mask arg list value must specify a
 *				non-NULL address.
 *	OL_COPY_MASK_VALUE	If the mask list resource name matches
 *				a source list resource name, the mask arg
 *				list value is copied into the address
 *				specified by the source arg list value.
 *				This rule does not affect the composed
 *				Arg list.  Note: to use this rule, the
 *				source arg list value must specify a
 *				non-NULL address.
 *
 ****************************procedure*header*****************************
 */
void
_OlComposeArgList(args, num_args, mask_args, mask_num_args,
		  comp_args, comp_num_args)
	ArgList		args;		/* Source Resource list		*/
	Cardinal	num_args;	/* Number of source resources	*/
	MaskArgList	mask_args;	/* Mask Resource list		*/
	Cardinal	mask_num_args;	/* Number of mask resources	*/
	ArgList *	comp_args;	/* Composed Arg list		*/
	Cardinal *	comp_num_args;	/* Number of composed Args	*/
{
	register int		m;
	register int		s;
	register XrmQuark	quark;
	register MaskArg *	m_ptr;
	register Arg *		s_ptr;
	Cardinal		count = 0;
	int			mlist_status;
	Boolean			match;
	Boolean			increment;
	static ArgList		buf = (ArgList) NULL;
	static unsigned int	buf_size = 0;
	static unsigned int	qsize = 0;
	static XrmQuark *	qlist = (XrmQuark *) NULL;

	if (comp_num_args != (Cardinal *) NULL)
	{
		*comp_num_args = 0;
	}

	if (comp_args != (ArgList *) NULL)
	{
		*comp_args = (ArgList) NULL;
	}

	if (mask_num_args == 0)
	{
		return;
	}
	else if (num_args != (Cardinal)0 && args == (ArgList) NULL)
	{
		OlVaDisplayWarningMsg(	(Display *)NULL,
					OleNolComposeArgList,
					OleTnoSourceList,
					OleCOlToolkitWarning,
					OleMolComposeArgList_noSourceList);
	    	return;
	}
	else if (mask_num_args != (Cardinal)0 &&
		 mask_args == (MaskArgList) NULL)
	{
		OlVaDisplayWarningMsg(	(Display *)NULL,
					OleNolComposeArgList,
					OleTnoMaskList,
					OleCOlToolkitWarning,
					OleMolComposeArgList_noMaskList);
	    	return;
	}


				/* Allocate a large enough array
				 * for both lists			*/

	if ((num_args + mask_num_args) > buf_size) {
		buf_size = num_args + mask_num_args + 50;

		if (buf != (ArgList) NULL)
			free((char *) buf);

		buf = (ArgList) XtMalloc(buf_size * sizeof(Arg));
	}

				/* Quarkify the source list		*/

	if (num_args > qsize) {
		qsize = num_args + 50;

		if (qlist)
			free((char *) qlist);

		qlist = (XrmQuark *) XtMalloc(qsize * sizeof(XrmQuark));
	}

	for (s=0; s < num_args; ++s)
		qlist[s] = XrmStringToQuark(args[s].name);

				/* Now search the list for matches	*/

	for (m_ptr = mask_args, m = 0, increment = False;
	     m < mask_num_args; ++m, ++m_ptr)
	{
		quark = XrmStringToQuark(m_ptr->name);
		match = False;

		mlist_status = ((m+1) == mask_num_args ? LAST : MORE);

		for (s = 0, s_ptr = args; s < num_args; ++s, ++s_ptr)
		{
			if (quark == qlist[s])
			{
				match = True;
				if (UpdateList(m_ptr, s_ptr, True, buf,
					&count, mlist_status) == True)
				{
					increment = True;
				}
			}
		}

				/* Update the destination list for rules
				 * that apply when there is no match	*/

		if (match == False) {
			switch(m_ptr->rule) {
			case OL_COPY_SOURCE_VALUE:
				/* Fall through	*/
			case OL_COPY_MASK_VALUE:
					/* Skip the OL_COPY_SIZE qualifier
					 * that follows the above two rules*/
				increment = True;
				break;
			case OL_ABSENT_PAIR:
				/* Fall through	*/
			case OL_DEFAULT_PAIR:
				/* Fall through	*/
			case OL_OVERRIDE_PAIR:
				(void)UpdateList(m_ptr, s_ptr, False,
					buf, &count, IGNORE);
				break;
			}
		}

		if (increment == True)
		{
			++m;
			++m_ptr;
			increment = False;
		}
	} /* END OF LOOPING OVER MASKARGLIST */

	if (count != (Cardinal)0)
	{
		register char *	dest;
		register char *	src = (char *)buf;

		if (comp_num_args == (Cardinal *) NULL ||
		    comp_args == (ArgList *) NULL)
		{
			OlVaDisplayWarningMsg(	(Display *)NULL,
						OleNolComposeArgList,
						OleTnullListPtr,
						OleCOlToolkitWarning,
						OleMolComposeArgList_nullListPtr);
			return;
		}

		*comp_num_args = count;
		s = count * sizeof(Arg);

		*comp_args = (ArgList) XtMalloc((unsigned) s);

		for (m=0, dest = (char *) *comp_args; m < s; ++m)
			*dest++ = *src++;
	}

} /* END OF _OlComposeArgList() */

