/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)virtual:Virtual.c	1.26.1.14"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file contains code that converts a virtual
 *	translation table string (i.e., a translation table string
 *	that contains virtual buttons or virtual keys)
 *	into a translation table string	that the Xt Toolkit (TM - MIT)
 *	understands.  This file also contains the code to initialize the
 *	virtual buttons/keys.
 *
 *	A typical virtual key may be defined as follows:
 *
 *		HelpKey: <F1>
 *
 *	The above key definition says that the Function key 'F1' with
 *	with no modifiers is mapped to the virtual "HelpKey."  In this
 *	the above example, we have one one mapping.  Multiple mappings
 *	are achieved by using commas to separate each mapping.
 *		When a virtual key is defined, three internal
 *	representations are created.  For the above example, the defining
 *	of "HelpKey" produces these three virtual expressions:
 *	the Help key.
 *
 *		HelpKey			- used as a KeyPress event
 *		HelpKeyDown		- used as a KeyPress event
 *		HelpKeyUp		- used as a KeyRelease event
 *
 *	A typical virtual button may be defined as follows:
 *
 *		MenuBtn: <Button3>, Shift <Button3>
 *
 *	The above button definition says that the logical button 3 with
 *	no modifiers and the logical button 3 with the Shift modifier
 *	both "map" to the virtual button "MenuBtn".   When a virtual
 *	button is defined, four internal representations are created.
 *	Using the above example, the defining of "MenuBtn" produces
 *	these four virtual expressions:
 *
 *		MenuBtn			- used as a modifier
 *		MenuBtnDown		- used as an event type,
 *					  virtual button press
 *		MenuBtnMotion		- used as an event type,
 *					  virtual button motion
 *		MenuBtnUp		- used as an event type,
 *					  virtual button release
 *	
 *		When a virtual translation string is parsed, virtual
 *	buttons are replaced by their logical representations.  For
 *	example, the virtual translation table:
 *
 *		"! <MenuBtnDown>:       Popup()   \n\
 *		 ! MenuBtn <MenuBtnUp>: Popdown() \n\
 *		 ! <HelpKey>:		DisplayHelp()"
 *
 *	would be converted to the Xt translation table string:
 *
 *		"! <Btn3Down>:             Popup()   \n\
 *		 ! Shift <Btn3Down>:       Popup()   \n\
 *		 ! Button3 <Btn3Up>:       Popdown() \n\
 *		 ! Shift Button3 <Btn3Up>: Popdown() \n\
 *		 ! <Key>F1:		   DisplayHelp()"
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/VirtualP.h>

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

static int      AddProdTreeNode();	/* Adds a node to the virtual
					 * production tree		*/
static void     AddVBMapping();		/* Adds a mapping to a virtual
					 * button			*/
static void     AddVKMapping();		/* Adds a mapping to a virtual
					 * key				*/
static void	AddHashEntry();		/* add entry to hash table	*/
static Boolean  CheckForVBMatch();	/* check keyword against
					 * virtual button list		*/
static Boolean	CheckIfVirtualButton();	/* checks button and state mask	*/
static Boolean	CheckIfVirtualKey();	/* checks keycode and state mask*/
static Boolean  CheckKeyword();		/* checks the keyword when
					 * creating a virtual button	*/
static void     CollapseProdTree();	/* Collapses the virtual
					 * production tree for re-use	*/
static char *	ComposeVBK();		/* composes a virtual button/key*/
static char *	CreateProduction();	/* Creates a production from a
					 * virtual production tree	*/
static void	DeleteHashEntry();	/* delete entry from hash table	*/
static void	DeleteToken();		/* deletes a virtual Token	*/
static char *	FillInProduction();	/* Fills in a production 	*/
static char *	GetTokenMappings();	/* gets a mapping from the db	*/
static VBK *	LookUpHashEntry();	/* Looks up a table entry	*/
static char *	ParseEvent();		/* Parses a virtual button
					 * event sequence		*/
static void     ParseList();		/* Parses a list of virtual
					 * buttons or keys for mappings	*/
static char *	ParseMappings();	/* Parses a mapping string for
					 * virtual buttons/keys		*/

static Boolean  ParseProduction();	/* Parses a production looking
					 * for virtual buttons		*/
static void	PrintInitError OL_ARGS((Display *, OLconst char *));

					/* public procedures		*/

void    _OlAddVirtualMappings();	/* adds virtual btn/key mappings
					 * to the virtual btn/key lists	*/
void    _OlAppAddVirtualMappings();	/* adds virtual btn/key mappings
					 * to the virtual btn/key lists	*/
char *	OlConvertVirtualTranslation();	/* Parses a translation string
					 * looking for virtual buttons	*/
void    _OlDumpVirtualMappings();	/* dumps contents of virtual
					 * buttons to a file		*/
Cardinal _OlGetVirtualMappings();	/* Returns the mappings for a
					 * virtual token.		*/
void	 _OlInitVirtualMappings();	/* Initializes the virtual keys
                                         * and buttons                  */
Boolean _OlIsVirtualButton OL_ARGS ((	/* Is a state and button mask a */
					/*  virtual button's mapping ??	*/
	String			name,
	register unsigned int	state,
	unsigned int		button,
	Boolean			exclusive
	));
Boolean _OlIsVirtualKey OL_ARGS((	/* Is a state & keycode a	*/
					/*  virtual key's mapping ??	*/
	String			name,
	register unsigned int	state,
	KeyCode			keycode,
	Boolean			exclusive
	));
Boolean _OlIsVirtualEvent OL_ARGS((	/* Is an XEvent a virtual button */
					/*  or virtual key event ??	*/
	String		name,	
	XEvent *	xevent,
	Boolean		exclusive
	));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define DB_NOT_INITIALIZED	(VBKHashTable == (VBK **) NULL)
#define SEPARATOR		','
#define BUMP_ESTIMATE		2
#define STACK_SIZE		256
#define ENFORCE_UNIQUE_MAPPING	0


				/* Define parse-aiding macros		*/

#define AT_EOL(c)	     (c == '\0' || c == '\n')
#define IS_ALPHA(c)	     (('A' <= c && c <= 'Z')||('a' <= c && c <= 'z'))
#define IS_DIGIT(c)	     ('0' <= c && c <= '9')
#define SCAN_WHITESPACE(s)   while(*s == ' ' || *s == '\t') ++s
#define SCAN_FOR_CHAR(s, c)  while(!AT_EOL(*s) && *s != c) ++s
#define SCAN_KEYWORD(s)	     while(IS_ALPHA(*s)||IS_DIGIT(*s)||*s == '_') ++s

				/* Define a table of Strings and masks	*/

static TableEntry button_table[] = {
	{ "Button1",	Button1Mask,	Button1, 0 },
	{ "Button2",	Button2Mask,	Button2, 0 },
	{ "Button3",	Button3Mask,	Button3, 0 },
	{ "Button4",	Button4Mask,	Button4, 0 },
	{ "Button5",	Button5Mask,	Button5, 0 }
};

static TableEntry modifier_table[] = {
	{ "Button1",	Button1Mask,	Button1, 0 },
	{ "Button2",	Button2Mask,	Button2, 0 },
	{ "Button3",	Button3Mask,	Button3, 0 },
	{ "Button4",	Button4Mask,	Button4, 0 },
	{ "Button5",	Button5Mask,	Button5, 0 },
	{ "Alt",	Mod1Mask,	0,	0 },
	{ "a",		Mod1Mask,	0,	0 },
	{ "Ctrl",	ControlMask,	0,	0 },
	{ "c",		ControlMask,	0,	0 },
	{ "Lock",	LockMask,	0,	0 },
	{ "l",		LockMask,	0,	0 },
	{ "Meta",	Mod1Mask,	0,	0 },
	{ "m",		Mod1Mask,	0,	0 },
	{ "Mod1",	Mod1Mask,	0,	0 },
	{ "1",		Mod1Mask,	0,	0 },
	{ "Mod2",	Mod2Mask,	0,	0 },
	{ "2",		Mod2Mask,	0,	0 },
	{ "Mod3",	Mod3Mask,	0,	0 },
	{ "3",		Mod3Mask,	0,	0 },
	{ "Mod4",	Mod4Mask,	0,	0 },
	{ "4",		Mod4Mask,	0,	0 },
	{ "Mod5",	Mod5Mask,	0,	0 },
	{ "5",		Mod5Mask,	0,	0 },
	{ "Shift", 	ShiftMask,	0,	0 },
	{ "s", 		ShiftMask,	0,	0 }
};

			/* These are ordered in accordance with the
			 * way they are composed.			*/

#ifndef	sun
static OLconst char
OlCoreVirtualMappings[] = "\
	adjustBtn:	<Button2>	\n\
	menuBtn:	<Button3>	\n\
	selectBtn:	<Button1>	\n\
\
	constrainBtn:	Shift<selectBtn>\n\
	duplicateBtn:	Ctrl<selectBtn>	\n\
	menuDefaultBtn:	Shift<menuBtn>	\n\
	panBtn:		Ctrl<selectBtn>	\n\
\
	cancelKey:	<Escape>	\n\
	copyKey:	Ctrl<c>		\n\
	cutKey:		Ctrl<x>		\n\
	defaultActionKey:	<Return>\n\
	helpKey:	<F1>		\n\
	nextFieldKey:	<Tab>		\n\
	pasteKey:	Ctrl<v>		\n\
	pendingActionKey:	<space>	\n\
	prevFieldKey:	Shift<Tab>	\n\
	propertiesKey:	Ctrl<p>		\n\
	stopKey:	Ctrl<s>		\n\
	undoKey:	Ctrl<u>		\
";
#else
static OLconst char
OlCoreVirtualMappings[] = "\
	adjustBtn:	<Button2>	\n\
	menuBtn:	<Button3>	\n\
	selectBtn:	<Button1>	\n\
\
	constrainBtn:	Shift<selectBtn>\n\
	duplicateBtn:	Ctrl<selectBtn>	\n\
	menuDefaultBtn:	Shift<menuBtn>	\n\
	panBtn:		Ctrl<selectBtn>	\n\
\
	cancelKey:	<Escape>	\n\
	copyKey:	<F16>		\n\
	cutKey:		<F20>		\n\
	defaultActionKey:	<Return>\n\
	helpKey:	<Help>		\n\
	nextFieldKey:	<Tab>		\n\
	pasteKey:	<F18>		\n\
	pendingActionKey:	<space>	\n\
	prevFieldKey:	Shift<Tab>	\n\
	propertiesKey:	<F13>		\n\
	stopKey:	<F11>		\n\
	undoKey:	<F14>		\
";
#endif

static OLconst char
OlCoreTextVirtualMappings[] = "\
	charFwdKey:	Ctrl<f>, <Right>	\n\
	charBakKey:	Ctrl<b>, <Left>		\n\
	rowDownKey:	Ctrl<n>, <Down>		\n\
	rowUpKey:	Ctrl<p>, <Up>		\n\
	wordFwdKey:	Alt<f>,  Alt<Right>	\n\
	wordBakKey:	Alt<b>,  Alt<Left>	\n\
	lineStartKey:	Ctrl<a>, Ctrl<Left>	\n\
	lineEndKey:	Ctrl<e>, Ctrl<Right>	\n\
	docStartKey:	Alt<Up>,  Alt<less>	\n\
	docEndKey:	Alt<Down>,Alt<greater>	\n\
	delCharFwdKey:	Ctrl<d>, <Delete>	\n\
	delCharBakKey:	Ctrl<h>, <BackSpace>	\n\
	delWordFwdKey:	Alt<d>			\n\
	delWordBakKey:	Alt<h>			\n\
	delLineFwdKey:	Ctrl<k>			\n\
	delLineBakKey:	Alt<k>			\
";

					/* Define local variables	*/

static VirtualProd	template_rec;
static VirtualProd *	template		= &template_rec;
static VBK **		VBKHashTable		= (VBK **) NULL;
static VirtualButton *	VBList			= (VirtualButton *) NULL;
static VirtualKey *	VKList			= (VirtualKey *) NULL;

				/* Define some hash table macros	*/

#define HASH_SIZE		0x7F
#define HASH_TABLE_SIZE		(HASH_SIZE + 1)
#define HASH_QUARK(q)		((int) ((q) & (XrmQuark)HASH_SIZE))

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * AddProdTreeNode - this routine adds a new node to the virtual production
 * tree.  The new node is defined in a global template.
 * No branch in the tree will ever contain more than one mapping of the
 * same virtual expression.  This is because you never mix different
 * mappings of the same virtual expression.   The field "mapping_prod" is
 * used to implement this behavior.  If this field is not NULL, then
 * all subnodes will have the same mapping as the first one encountered
 * matching the virtual expression.
 * The routine returns the total number of characters in the virtual
 * production tree.
 ****************************procedure*header*****************************
 */
static int
AddProdTreeNode(self, branch_length, mapping_prod)
	VirtualProd *self;		/* Node to get new subnode(s)	*/
	int          branch_length;	/* branch length, in chars	*/
	VirtualProd *mapping_prod;	/* Previous node with matching
					 * virtual button		*/
{
	int		i;		/* loop coounter		*/
	int		j;		/* loop counter			*/
	int		m;		/* current mapping index	*/
	int		e_index;	/* element index		*/
	int		tree_size = 0;	/* size of production in chars	*/
	int		iterations;	/* number of subnodes to do	*/
	VirtualButton * vb = (VirtualButton *)NULL;
	VirtualKey *	vk = (VirtualKey *)NULL;
	register VirtualProd * new_node; /* new end node to add		*/

				/* Check to see if this virtual
				 * expression already was used in
				 * this production.			*/

	if (mapping_prod == (VirtualProd *) NULL &&
	    self->btn_or_key != (VBK *)NULL &&
	    template->btn_or_key != (VBK *)NULL &&
	    self->btn_or_key->opaque == template->btn_or_key->opaque) {
		mapping_prod = self;
	}

				/* Get new length of this branch	*/

	branch_length += self->vbk_length + self->text_length;

				/* If subnode[0] is not null, then
				 * only propagate from this node since
				 * we're looking for the bottom of
				 * the tree				*/

	if (self->subnode[0] != (VirtualProd *)NULL) {
	    for (i = 0; i < OL_MAX_VIRTUAL_MAPPINGS && self->subnode[i]; ++i)
		tree_size += AddProdTreeNode(self->subnode[i],
				branch_length, mapping_prod);
	    return(tree_size);
	}

				/* Else ............
				 * 
				 * get information from the button or
				 * the key				*/

	if (template->btn_or_key != (VBK *)NULL) {

		e_index = template->btn_or_key->element_index;

		if (template->btn_or_key->type == OL_VIRTUAL_BUTTON) {
			vb = (VirtualButton *)template->btn_or_key->opaque;
			iterations = (mapping_prod ? 1 : vb->num_mappings);
		}
		else {
			vk = (VirtualKey *)template->btn_or_key->opaque;
			iterations = (mapping_prod ? 1 : vk->num_mappings);
		}
	}
	else {
		iterations = 1;
	}

					/* Initialize the mapping index	*/

	m = (mapping_prod ? mapping_prod->mapping_index : 0);


				/* Add subnode(s) to end of branch	*/

	for(i = 0; i < iterations; ++i, ++m) {

				/* Get a new node to put on the tree	*/

		new_node = XtNew(VirtualProd);

		self->subnode[i] = new_node;

		for (j = 0; j < OL_MAX_VIRTUAL_MAPPINGS; ++j)
			new_node->subnode[j] = (VirtualProd *) NULL;

				/* Update the new node's fields		*/

		new_node->prev		= self;
		new_node->text_start	= template->text_start;
		new_node->text_length	= template->text_length;
		new_node->btn_or_key	= template->btn_or_key;
		new_node->mapping_index	= m;

		if (vk != (VirtualKey *)NULL) {
		    new_node->vbk_text	 = vk->mappings[m].text[e_index];
		    new_node->vbk_length = vk->mappings[m].length[e_index];
		}
		else if (vb != (VirtualButton *)NULL) {
		    new_node->vbk_text	 = vb->mappings[m].text[e_index];
		    new_node->vbk_length = vb->mappings[m].length[e_index];
		}
		else {
		    new_node->vbk_text	 = (char *)NULL;
		    new_node->vbk_length = 0;
		}

			/* Update the total length of the Virtual
			 * production tree (in characters)		*/

		tree_size += branch_length + new_node->text_length +
				new_node->vbk_length;

	} /* End of for() */

	return(tree_size);

} /* END OF AddProdTreeNode() */

/*
 *************************************************************************
 * AddVBMapping - adds a mapping to a virtual button.  If the virtual
 * button does not exist, it is created and placed alphabetically in
 * the list.  If a mapping is the same as another in the list, it
 * is refused.
 * The field "qcomposed" is a quarkified name of the virtual button
 * that was used to add this mapping.
 ****************************procedure*header*****************************
 */
static void
AddVBMapping(qname, modifiers, button, qcomposed, num_estimate)
	XrmQuark	qname;		/* quarked Virtual button name	*/
	Mask		modifiers;	/* modifiers for this button	*/
	Mask		button;		/* button mask			*/
	XrmQuark	qcomposed;	/* quarked name of compose button*/
	Cardinal *	num_estimate;	/* estimated # of mappings ptr	*/
{
	int		i;
	int		l;
	int		m;		/* index into mapping array	*/
	int		normal_length;
	int		neg_length;
	unsigned int	button_type;
	String		text;
	String		button_string;
	char		buf[STACK_SIZE];
	char		neg_string[STACK_SIZE];	/* negated modifier string */
	VBK *		vbk;
	register VirtualButton *self;
#if ENFORCE_UNIQUE_MAPPING
	register VirtualButton *vb;
#endif /* ENFORCE_UNIQUE_MAPPING */

				/* Don't let the modifiers contain the
				 * the button.				*/

	modifiers &= ~button;

#if ENFORCE_UNIQUE_MAPPING
				/* Make sure mapping is unique		*/

	for (vb = VBList; vb != (VirtualButton *)NULL; vb = vb->next) {
		for (i = 0; i < vb->num_mappings; ++i)
			if (vb->mappings[i].button == button &&
			    vb->mappings[i].modifiers == modifiers)
			    {
				OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg1,
					XrmQuarkToString(qname),
					XrmQuarkToString(vb->qname));
				return;
			}
	}
#endif /* ENFORCE_UNIQUE_MAPPING */

				/* check for duplicates in the list	*/

	vbk = LookUpHashEntry(qname);

	if (vbk != (VBK *) NULL) {
		self = (VirtualButton *) vbk->opaque;

					/* Check maximum mapping limit	*/

		if (self->num_mappings == OL_MAX_VIRTUAL_MAPPINGS)
		{
			OlVaDisplayWarningMsg((Display *)NULL,
				OleNfileVirtual,
				OleTmsg2,
				OleCOlToolkitWarning,
				OleMfileVirtual_msg2,
			    OL_MAX_VIRTUAL_MAPPINGS, XrmQuarkToString(qname));
			return;
		}
	}
	else {
		char *name;

			/* If we reach this point, everything is ok.
			 * If self is NULL, then the virtual button
			 * must be added to the list.			*/

		self = XtNew(VirtualButton);

		self->next = VBList;
		VBList = self;

		name = XrmQuarkToString(qname);

		self->qname = qname;

			/* Allocate the memory for the mappings using
			 * the estimated value				*/

		self->num_mappings = 0;
		self->mappings = (VButtonMapping *)
			XtMalloc(*num_estimate * sizeof(VButtonMapping));

					/* Quarkify this virtual button	*/

		for (i=0; i < VB_ELEMENTS; ++i) {
			switch(i) {
			case VB_UP:
				(void)sprintf(buf, "%sUp", name);
				break;
			case VB_NORM_MOD:
				(void)sprintf(buf, name);
				break;
			case VB_NEGATED_MOD:
				(void)sprintf(buf, "~%s", name);
				break;
			case VB_DOWN:
				(void)sprintf(buf, "%sDown", name);
				break;
			case VB_MOTION:
				(void)sprintf(buf, "%sMotion", name);
				break;
			}
			self->quarks[i] = XrmStringToQuark(buf);
			AddHashEntry(self->quarks[i], (OpaqueVBK) self,
				OL_VIRTUAL_BUTTON, i);
		}
	}

				/* Get the button string and its number	*/

	for(i = 0; ; ++i)
		if (button_table[i].mask == button) {
			button_type = button_table[i].button_type;
			button_string = button_table[i].keyword;
			break;
		}

				/* Set some of the fields before we
				 * destroy the modifiers mask		*/

	++self->num_mappings;
	m = self->num_mappings - 1;
	self->mappings[m].modifiers	= modifiers;
	self->mappings[m].button	= button;
	self->mappings[m].button_type	= button_type;
	self->mappings[m].composed	= qcomposed;


				/* Build the normal modifier string and
				 * the negated modifier string at the
				 * same time.				*/
	neg_length = 0;
	normal_length = 0;
	buf[0] = '\0';
	neg_string[0] = '\0';

	for (i=0; i < XtNumber(modifier_table) && modifiers; ++i) {
		if (modifier_table[i].mask & modifiers) {
			modifiers &= ~modifier_table[i].mask;

			text = modifier_table[i].keyword;
			(void)sprintf((buf + normal_length), "%s ", text);
			normal_length += strlen(text) + 1;

			(void)sprintf((neg_string + neg_length), "~%s ", text);
			neg_length += strlen(text) + 2;
		}
	}

	for(i = 0; i < VB_ELEMENTS; ++i) {
		switch(i) {
		case  VB_NORM_MOD:
			l = normal_length + strlen(button_string);
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s%s", buf, button_string);
			break;
		case  VB_NEGATED_MOD:
			l = neg_length + strlen(button_string) + 1;
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s~%s", neg_string, button_string);
			break;
		case  VB_UP:
			l = normal_length + strlen("Btn*Up") + 2;
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s<Btn%uUp>", buf, button_type);
			break;
		case  VB_DOWN:
			l = normal_length + strlen("Btn*Down") + 2;
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s<Btn%uDown>", buf, button_type);
			break;
		case  VB_MOTION:
			l = normal_length + strlen("Btn*Motion") + 2;
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s<Btn%uMotion>", buf, button_type);
			break;
		}
		self->mappings[m].length[i] = --l;
		self->mappings[m].text[i]   = text;
	}

} /* END OF AddVBMapping() */

/*
 *************************************************************************
 * AddVKMapping - adds a virtual key to the virtual key database
 * The field "qcomposed" is a quarkified name of the virtual key
 * that was used to add this mapping.
 ****************************procedure*header*****************************
 */
static void
AddVKMapping(display, qname, modifiers, keysym, qcomposed, num_estimate)
	Display *	display;	/* Current display		*/
	XrmQuark	qname;		/* quarked Virtual button name	*/
	Mask		modifiers;	/* modifiers for this button	*/
	KeySym		keysym;		/* key symbol to be added	*/
	XrmQuark	qcomposed;	/* quarked name of compose key	*/
	Cardinal *	num_estimate;	/* estimated # of mappings ptr	*/
{
	int		i;
	int		l;
	int		m;		/* index into mapping array	*/
	int		length;
	String		text;
	String		key_string;
	char		buf[STACK_SIZE];
	VBK *		vbk;
	KeyCode		keycode;
	register VirtualKey *self;
#if ENFORCE_UNIQUE_MAPPING
	register VirtualKey *vk;
#endif /* ENFORCE_UNIQUE_MAPPING */


	keycode = XKeysymToKeycode(display, keysym);

#if ENFORCE_UNIQUE_MAPPING
				/* Make sure mapping is unique		*/

	for (vk = VKList; vk != (VirtualKey *)NULL; vk = vk->next) {
		for (i = 0; i < vk->num_mappings; ++i)
			if (vk->mappings[i].keycode == keycode &&
			    vk->mappings[i].modifiers == modifiers)
			    {
				OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg3,
					XrmQuarkToString(qname),
					XrmQuarkToString(vk->qname));
				return;
			}
	}
#endif /* ENFORCE_UNIQUE_MAPPING */

				/* check for duplicates in the list	*/

	vbk = LookUpHashEntry(qname);

	if (vbk != (VBK *) NULL) {
		self = (VirtualKey *) vbk->opaque;

					/* Check maximum mapping limit	*/

		if (self->num_mappings == OL_MAX_VIRTUAL_MAPPINGS)
		{
			OlVaDisplayWarningMsg((Display *)NULL,
				OleNfileVirtual,
				OleTmsg4,
				OleCOlToolkitWarning,
				OleMfileVirtual_msg4,
			     OL_MAX_VIRTUAL_MAPPINGS, XrmQuarkToString(qname));
			return;
		}
	}
	else {
		char *name;

			/* If we reach this point, everything is ok.
			 * If self is NULL, then the virtual key 
			 * must be added to the list.			*/

		self = XtNew(VirtualKey);

		self->next = VKList;
		VKList = self;

		name = XrmQuarkToString(qname);

		self->qname = qname;

			/* Allocate the memory for the mappings using
			 * the estimated value				*/

		self->num_mappings = 0;
		self->mappings = (VKeyMapping *)
			XtMalloc(*num_estimate * sizeof(VKeyMapping));

					/* Quarkify this virtual button	*/

		for (i=0; i < VK_ELEMENTS; ++i) {
			switch(i) {
			case VK_UP:
				(void)sprintf(buf, "%sUp", name);
				break;
			case VK_IMPLIED_DOWN:
				(void)sprintf(buf, name);
				break;
			case VK_DOWN:
				(void)sprintf(buf, "%sDown", name);
				break;
			}
			self->quarks[i] = XrmStringToQuark(buf);
			AddHashEntry(self->quarks[i], (OpaqueVBK) self,
				OL_VIRTUAL_KEY, i);
		}
	}

				/* Set some of the fields before we
				 * destroy the modifiers mask		*/

	++self->num_mappings;
	m = self->num_mappings - 1;
	self->mappings[m].modifiers	= modifiers;
	self->mappings[m].keycode	= keycode;
	self->mappings[m].keysym	= keysym;
	self->mappings[m].composed	= qcomposed;

					/* Build the modifier string	*/
	length = 0;
	buf[0] = '\0';

	for (i=0; i < XtNumber(modifier_table) && modifiers; ++i) {
		if (modifier_table[i].mask & modifiers) {
			modifiers &= ~modifier_table[i].mask;
			text = modifier_table[i].keyword;
			(void)sprintf((buf + length), "%s ", text);
			length += strlen(text) + 1;
		}
	}

	key_string = XKeysymToString(keysym);

	length += strlen(key_string);

	for(i = 0; i < VK_ELEMENTS; ++i) {
		switch(i) {
		case  VK_UP:
			l = length + strlen("<KeyUp>");
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s<KeyUp>%s", buf, key_string);
			break;
		case  VK_DOWN:
			l = length + strlen("<KeyDown>");
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s<KeyDown>%s", buf, key_string);
			break;
		case  VK_IMPLIED_DOWN:
			l = length + strlen("<Key>");
			++l;
			text = XtMalloc((unsigned int) l * sizeof(char));
			(void)sprintf(text, "%s<Key>%s", buf, key_string);
			break;
		}
		self->mappings[m].length[i] = --l;
		self->mappings[m].text[i]   = text;
	}

} /* END OF AddVKMapping() */

/*
 *************************************************************************
 * AddHashEntry - this routine adds a virtual button or virtual key
 * to a hash table.  Only one of the arguments "button" and "key" will
 * be non-NULL.  The quark name for this node is taken from the non-NULL
 * one.
 *	This routine adds nodes "quark-a-merically" to the buckets, i.e.,
 * in ascending quark order.
 *	The argument "element_index" indicates which element of the
 * virtual button or key this node is for.  For example, a virtual
 * button has the elements VB_UP, VB_DOWN, VB_MOTION, etc.
 ****************************procedure*header*****************************
 */
static void
AddHashEntry(qname, opaque, type, element_index)
	XrmQuark	qname;		/* quarked button or key name	*/
	OpaqueVBK	opaque;		/* 'cast'ed button or key ptr	*/
	int		type;		/* Button or key flag		*/
	int		element_index;	/* which button/key element	*/
{
	int		bucket;
	register VBK *	prev;
	register VBK *	new;
	VBK **		table = VBKHashTable;

	if (!table)
	{
		OlError("AddHashEntry: bad table");
	}

	new = XtNew(VBK);

	new->qname	= qname;
	new->opaque	= opaque;
	new->type	= type;
	new->element_index = element_index;

	bucket = HASH_QUARK(qname);
	prev = table[bucket];

	if (prev == (VBK *)NULL || prev->qname > qname) {
		new->next = prev;
		table[bucket] = new;
	}
	else {
		while (prev->next != (VBK *) NULL &&
		       prev->next->qname < qname) {
			prev = prev->next;
		}
		new->next = prev->next;
		prev->next = new;
	}
} /* END OF AddHashEntry() */

/*
 *************************************************************************
 * CheckForVBMatch - this routine checks to see if a keyword matches a
 * virtual expression.  If a match occurs, the also checks to see if the
 * right match happened.  For example, a keyword might match a virtual
 * button called "FOO", but FOO might be a modifier and the keyword
 * was for an event_type.  In this case, an error would occur.
 * Regardless of errors, the return value always will be true if a
 * match occurs.
 *	If a good match occurs, the routine fills in the fields of the
 * global template and returns TRUE.  When a bad match occurs, the
 * template is not filled-in, the error message pointer is initialized
 * and TRUE is returned.
 *	FALSE is returned when a match does not occur.
 ****************************procedure*header*****************************
 */
static Boolean
CheckForVBMatch(quark, event_type, msg)
	XrmQuark	quark;		/* Quarkified keyword		*/
	Boolean		event_type;	/* keyword for an event type ?	*/
	char **		msg;		/* return error message	pointer	*/
{
	Boolean			is_event_type;
	register VBK *		self;
	extern VirtualProd *	template;

	*msg = (char *) NULL;
	template->btn_or_key = (VBK *) NULL;

	self = LookUpHashEntry(quark);

				/* Check to see if the found virtual
				 * expression is valid.			*/

	if (self != (VBK *) NULL) {
		is_event_type = False;
		if (self->type == OL_VIRTUAL_KEY) {
			is_event_type = True;
		}
		else {
			if (self->element_index != VB_NEGATED_MOD &&
			    self->element_index != VB_NORM_MOD)
				is_event_type = True;
		}

		if (event_type == True) {
			if (is_event_type == False)
				*msg = "Attempt to use a modifier as an event";
		}
		else {
			if (is_event_type == True)
				*msg = "Attempt to use event as a modifier";
		}

			/* put the virtual button/key into the template
			 * if there is no error				*/

		if (*msg == (char *)NULL)
			template->btn_or_key = self;
	}

	return((Boolean) (self != (VBK *)NULL ? True : False));

} /* END OF CheckForVBMatch() */

/*
 *************************************************************************
 * CheckIfVirtualButton - this routine checks to see if a button and its
 * state is a virtual button event.  The field "state" is the state
 * field that would be found in an XButtonEvent or an XMotionEvent
 * structure.  (XCrossingEvents follow the same rules as XMotionEvents.)
 * The field "button" is the button detail that would appear
 * in the XButtonEvent.  If the "button" field is zero, it is assumed
 * that the query is for a motion event.  The field "exclusive" is
 * to determine if a perfect match is desired.  If the virtual button
 * in question satisfies the "state, button and exclusive" criterion,
 * TRUE is returned, else FALSE is returned.  If the virtual button
 * does not exist, a warning message is generated and FALSE is returned.
 ****************************procedure*header*****************************
 */
static Boolean
CheckIfVirtualButton(name, state, button_type, exclusive, func_name)
	String			name;	/* Virtual Button Name		*/
	register unsigned int	state;	/* button or key state		*/
	unsigned int		button_type; /* button number or zero	*/
	Boolean			exclusive; /* perfect match desired ??	*/
	OLconst char *		func_name; /* name of calling function	*/
{
	VBK *			vbk;
	register int            m;		/* mapping number	*/
	Mask			button_mask;
	register unsigned int   vb_mask;
	register VirtualButton *self;

	if (DB_NOT_INITIALIZED)
	{
/*		PrintInitError((Display *)NULL, func_name); */
		_OlInitVirtualMappings((Widget)NULL);
	}
	else if (name == (char *) NULL || *name == '\0')
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg5,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg5,
					func_name);
		return(False);
	}
	else if (*name == '~')
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg6,
					OleCOlToolkitWarning,
				OleMfileVirtual_msg6,
				func_name);
		return(False);
	}

				/* Find virtual button in question	*/

	vbk = LookUpHashEntry(XrmStringToQuark(name));

	if (vbk == (VBK *) NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg7,
					OleCOlToolkitWarning,
				OleMfileVirtual_msg7,
				func_name, name);
		return(False);
	}
	else if (vbk->type != OL_VIRTUAL_BUTTON)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg8,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg8,
					func_name, name);
		return(False);
	}
	else
		self = (VirtualButton *) vbk->opaque;

					/* Do the Motion event checking.
					 * This applies to pointer motion
					 * and enter/leave events	*/

	if (button_type == NULL) {
		if (exclusive) {
			for (m=0; m < self->num_mappings; ++m) {
				if (state == (self->mappings[m].modifiers |
				    self->mappings[m].button))
					return(True);
			}
		}
		else {
			for (m=0; m < self->num_mappings; ++m) {
				vb_mask = self->mappings[m].modifiers |
					  self->mappings[m].button;
	
				if (vb_mask == (state & vb_mask))
					return(True);
			}
		}
	}
	else {
					/* Case for XButtonEvents	*/

					/* Convert the button number into
					 * a button mask		*/

		for (m=0, button_mask=0; m < XtNumber(button_table); ++m)
			if (button_table[m].button_type == button_type) {
				button_mask = button_table[m].mask;
				break;
			}

		if (button_mask == 0)
		{
			OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg9,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg9,
					func_name, button_type, name);
			return(False);
		}

					/* Remove the button mask from
					 * the state mask		*/

		state &= ~button_mask;

		if (exclusive) {
			for (m=0; m < self->num_mappings; ++m) {
				if (state == self->mappings[m].modifiers &&
				    button_mask == self->mappings[m].button)
					return(True);
			}
		}
		else {
			for (m=0; m < self->num_mappings; ++m) {
				vb_mask = self->mappings[m].modifiers;

				if (button_mask == self->mappings[m].button &&
				    (vb_mask == (state & vb_mask)))
					return(True);
			}
		}
	}

	return(False);

} /* END OF CheckIfVirtualButton() */

/*
 *************************************************************************
 * CheckIfVirtualKey - this routine checks to see if a keycode and its
 * state is a virtual key mapping.  The field "state" is the state
 * field that would be found in an XKeyEvent structure.
 * The field "keycode" is the keycode detail that would appear
 * in the XKeyEvent.
 * The field "exclusive" is to determine if a perfect match is desired.
 * If the virtual key in question satisfies the "state, key and exclusive"
 * criteria, TRUE is returned, else FALSE is returned.  If the virtual key
 * does not exist, a warning message is generated and FALSE is returned.
 ****************************procedure*header*****************************
 */
static Boolean
CheckIfVirtualKey(name, state, keycode, exclusive, func_name)
	String			name;	/* Virtual Key Name		*/
	register unsigned int	state;	/* key state			*/
	KeyCode			keycode; /* keycode			*/
	Boolean			exclusive; /* perfect match desired ??	*/
	OLconst char *		func_name; /* name of calling function	*/
{
	VBK *			vbk;
	register int            m;		/* mapping number	*/
	register unsigned int   vk_mask;
	register VirtualKey *	self;

	if (DB_NOT_INITIALIZED)
	{
/*		PrintInitError((Display *)NULL, func_name); */
		_OlInitVirtualMappings((Widget)NULL);
	}
	else if (name == (char *) NULL || *name == '\0')
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg10,
					OleCOlToolkitWarning,
			OleMfileVirtual_msg10, func_name);
		return(False);
	}

				/* Find virtual key in question	*/

	vbk = LookUpHashEntry(XrmStringToQuark(name));

	if (vbk == (VBK *) NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg11,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg11,
					func_name, name);
		return(False);
	}
	else if (vbk->type != OL_VIRTUAL_KEY)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg12,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg12,
					func_name, name);
		return(False);
	}
	else
		self = (VirtualKey *) vbk->opaque;

				/* Check the key press/release event	*/

	if (exclusive) {
		for (m=0; m < self->num_mappings; ++m) {
			if (state == self->mappings[m].modifiers &&
			    keycode == self->mappings[m].keycode)
				return(True);
		}
	}
	else {
		for (m=0; m < self->num_mappings; ++m) {
			vk_mask = self->mappings[m].modifiers;

			if (keycode == self->mappings[m].keycode &&
			    (vk_mask == (state & vk_mask)))
				return(True);
		}
	}

	return(False);

} /* END OF CheckIfVirtualKey() */

/*
 *************************************************************************
 * CheckKeyword - checks a keyword to see if it matches one of the buttons
 * or modifiers
 ****************************procedure*header*****************************
 */
static Boolean
CheckKeyword(quark, checking_for_modifier, mask)
	register XrmQuark quark;	/* Quarkified keyword		*/
	Boolean checking_for_modifier;	/* looking for a modifier ??	*/
	Mask *	mask;			/* Mask to return		*/
{
	register int		i;
	TableEntry *		table;


	if (button_table[0].quark == 0) {
						/* Quarkify the tables	*/

		table = button_table;
		for (i=0; i < XtNumber(button_table); ++i, ++table)
			table->quark = XrmStringToQuark(table->keyword);

		table = modifier_table;
		for (i=0; i < XtNumber(modifier_table); ++i, ++table)
			table->quark = XrmStringToQuark(table->keyword);
	}

	*mask = 0;

	if (checking_for_modifier == True) {
		table = modifier_table;
		for (i=0; i < XtNumber(modifier_table); ++i, ++table) {
			if (quark == table->quark) {
				*mask = table->mask;
				return(True);
			}
		}
	}
	else {
		table = button_table;
		for (i=0; i < XtNumber(button_table); ++i, ++table) {
			if (quark == table->quark) {
				*mask = table->mask;
				return(True);
			}
		}
	}
	return(False);

} /* END OF CheckKeyword() */

/*
 *************************************************************************
 * CollapseProdTree - collapses an existing virtual production tree.  It
 * places the nodes on a "free" node list so that they can be reused
 * at a later time.  The root node is never put on the free list.  The
 * first subnode is overloaded since it is used to store the pointer
 * to the next free node in the list.
 ****************************procedure*header*****************************
 */
static void
CollapseProdTree(self, is_root)
	VirtualProd *	self;		/* Current tree node		*/
	Boolean		is_root;	/* is this the root node ?	*/
{
	int i;

	for (i = 0; i < OL_MAX_VIRTUAL_MAPPINGS && self->subnode[i]; ++i)
		CollapseProdTree(self->subnode[i], False);

	if (is_root == False) {
		XtFree((char *)self);
	}
} /* END OF CollapseProdTree() */

/*
 *************************************************************************
 * ComposeVBK - composes a virtual button or key from a previous one
 ****************************procedure*header*****************************
 */
static char *
ComposeVBK(buf, type, display, qname, modifiers, composed, num_estimate)
	char *		buf;		/* buffer to write message	*/
	int *		type;		/* Virtual token type		*/
	Display *	display;	/* Current display		*/
	XrmQuark	qname;		/* Virtual token being composed	*/
	Mask		modifiers;	/* new modifiers		*/
	VBK *		composed;	/* one to compose from		*/
	Cardinal *	num_estimate;	/* estimated # of mappings ptr	*/
{
	register int	i;
	VirtualKey *	vk = (VirtualKey *) composed->opaque;
	VirtualButton *	vb = (VirtualButton *) composed->opaque;

	if (composed->qname == qname) {

		(void)sprintf(buf, "Attempt to compose virtual token \"%s\"\
 with itself", XrmQuarkToString(qname));
		return(buf);
	}
	else if (composed->type == OL_VIRTUAL_KEY &&
		 *type == OL_VIRTUAL_BUTTON) {

		(void)sprintf(buf, "Attempting to give key mapping to virtual\
 button \"%s\"", XrmQuarkToString(qname));
		return(buf);
	}
	else if (composed->type == OL_VIRTUAL_BUTTON &&
		 *type == OL_VIRTUAL_KEY) {

		(void)sprintf(buf, "Attempting to give button\
 mapping to virtual key \"%s\"", XrmQuarkToString(qname));
		return(buf);
	}
	else if ((composed->type == OL_VIRTUAL_KEY &&
		  composed->qname != vk->qname) ||
		 (composed->type == OL_VIRTUAL_BUTTON &&
		  composed->qname != vb->qname)) {

		(void)sprintf(buf, "Invalid virtual expression \"%s\" for\
 virtual token \"%s\"", XrmQuarkToString(composed->qname),
			XrmQuarkToString(qname));
		return(buf);
	}

			/* If we're here, everything
			 * is ok.			*/

	*type = composed->type;

	if (*type == OL_VIRTUAL_KEY) {
		for (i=0; i < vk->num_mappings; ++i) {
			AddVKMapping(display, qname,
				(modifiers | vk->mappings[i].modifiers),
				vk->mappings[i].keysym,
				composed->qname, num_estimate);
		}
	}
	else {
		for (i=0; i < vb->num_mappings; ++i) {
			AddVBMapping(qname,
				(modifiers | vb->mappings[i].modifiers),
				vb->mappings[i].button,
				composed->qname, num_estimate);
		}
	}

	return((char *)NULL);

} /* END OF ComposeVBK() */

/*
 *************************************************************************
 * CreateProduction - used in creating a production from a virtual
 * production tree.  This routine recursively descends all branches
 * below this node until it reaches the end of each.  At that point
 * it calls a function which fills in the production with text.
 ****************************procedure*header*****************************
 */
static char *
CreateProduction(self, p)
	VirtualProd *self;	/* Current Virtual Production node	*/
	char        *p;		/* production write address		*/
{
	int i;

	if (self->subnode[0]) {
		for(i = 0; i < OL_MAX_VIRTUAL_MAPPINGS && self->subnode[i]; ++i)
			p = CreateProduction(self->subnode[i], p);
	}
	else {
		p = FillInProduction(self, p);

				/* Set the last character to newline.	*/

		p[-1] = '\n';
	}

	return(p);

} /* END OF CreateProduction() */

/*
 *************************************************************************
 * DeleteHashEntry - this routine deletes a virtual button or virtual key
 * entry from the hash table that stores both virtual buttons and virtual
 * keys.
 ****************************procedure*header*****************************
 */
static void
DeleteHashEntry(qname)
	XrmQuark	qname;
{
	VBK **	next_ptr;
	VBK *	self;

	for (	next_ptr = &VBKHashTable[HASH_QUARK(qname)], self = *next_ptr;
		self != (VBK *)NULL && self->qname != qname;
		self = self->next)
	{
		next_ptr = &self->next;
	}

	if (self == (VBK *)NULL)
	{
		OlWarning("Virtual hash table corruption");
		return;
	}

	*next_ptr = self->next;

					/* Now free the hash node	*/
	XtFree((char *) self);
} /* END OF DeleteHashEntry() */

/*
 *************************************************************************
 * DeleteToken - this routine deletes a virtual button or virtual key
 * from the hash tables and key/button lists.  For each virtual button,
 * there are VB_ELEMENTS hash entries and for each virtual key, there
 * are VK_ELEMENTS hash entries.  Once the hash entries have been deleted,
 * the virtual button or virtual key node is deleted from its respective
 * list.
 ****************************procedure*header*****************************
 */
static void
DeleteToken(vbk)
	VBK *		vbk;			/* Node	to delete	*/
{
	Cardinal	i;
	Cardinal	j;

			/* Now remove the virtual button (or key) from
			 * its list.					*/

	if (vbk->type == OL_VIRTUAL_BUTTON)
	{
		VirtualButton *		self;
		VirtualButton **	next_ptr;

		for (	self = VBList, next_ptr = &VBList;
			self != (VirtualButton *)vbk->opaque &&
			self != (VirtualButton *)NULL; self = self->next)
		{
			next_ptr = &self->next;
		}

		if (self == (VirtualButton *)NULL)
		{
			OlError("Virtual Button List corruption");
			return;
		}

		*next_ptr = self->next;

				/* Remove the hash table entries	*/

		for (i=(Cardinal)0; i < VB_ELEMENTS; ++i)
		{
			DeleteHashEntry(self->quarks[i]);
		}

				/* Free the mapping text strings	*/

		for (i=(Cardinal)0; i < self->num_mappings; ++i)
		{
			for (j=(Cardinal)0; j < VB_ELEMENTS; ++j)
			{
				XtFree(self->mappings[i].text[j]);
			}
		}

		if (self->mappings != (VButtonMapping *)NULL)
		{
			XtFree((char *) self->mappings);
		}
		XtFree((char *) self);
	}
	else			/* OL_VIRTUAL_KEY	*/
	{
		VirtualKey *	self;
		VirtualKey **	next_ptr;

		for (	self = VKList, next_ptr = &VKList;
			self != (VirtualKey *)vbk->opaque &&
			self != (VirtualKey *)NULL; self = self->next)
		{
			next_ptr = &self->next;
		}

		if (self == (VirtualKey *)NULL)
		{
			OlError("Virtual Key List corruption");
			return;
		}

		*next_ptr = self->next;

				/* Remove the hash table entries	*/

		for (i=(Cardinal)0; i < VK_ELEMENTS; ++i)
		{
			DeleteHashEntry(self->quarks[i]);
		}

				/* Free the mapping text strings	*/

		for (i=(Cardinal)0; i < self->num_mappings; ++i)
		{
			for (j=(Cardinal)0; j < VK_ELEMENTS; ++j)
			{
				XtFree(self->mappings[i].text[j]);
			}
		}

		if (self->mappings != (VKeyMapping *)NULL)
		{
			XtFree((char *) self->mappings);
		}
		XtFree((char *) self);
	}
} /* END OF DeleteToken() */

/*
 *************************************************************************
 * LookUpHashEntry - looks up a virtual button or virtual string in
 * a table by its quark.  Remember, each bucket in the table has its
 * nodes arranged in ascending quark order.
 ****************************procedure*header*****************************
 */
static VBK *
LookUpHashEntry(qname)
	XrmQuark qname;
{
	register VBK *	self;
	VBK **		table = VBKHashTable;

	if (!table)
	{
		OlError("LookUpHashEntry: bad table");
	}

	self = table[HASH_QUARK(qname)];

	while (self != (VBK *) NULL && self->qname != qname) {
		if (self->qname > qname)
			return((VBK *) NULL);
		self = self->next;
	}

	return(self);
} /* END OF LookUpHashEntry() */

/*
 *************************************************************************
 * FillInProduction - this routine recursively ascends a tree until it
 * reaches the top.  Then as it backs out of its recursive calls, it
 * fills in a previously-allocated memory space (the converted virtual
 * production) with text.
 ****************************procedure*header*****************************
 */
static char *
FillInProduction(self, p)
	VirtualProd *self;		/* Current virtual prod. node	*/
	char        *p;			/* Address to write at		*/
					/* Else ........		*/
{
	if (self->prev)
		p = FillInProduction(self->prev, p);

					/* Translation text		*/
	if (self->text_length) {
		(void)strncpy(p, self->text_start, self->text_length);
		p += self->text_length;
	}

					/* virtual button/key text	*/
	if (self->vbk_length) {
		(void)strcpy(p, self->vbk_text);
		p += self->vbk_length;
	}

	return(p);
} /* END OF FillInProduction() */

/*
 *************************************************************************
 * GetTokenMappings - this routine looks up a mapping in the database.
 * If a mapping is found in the database, it adds it to the virtual
 * database.  If one is not found, it uses the default mappings that
 * were supplied.
 ****************************procedure*header*****************************
 */
static char *
GetTokenMappings(widget, token_name, def_mappings)
	Widget		widget;		/* Application's widget		*/
	char *		token_name;	/* name to get mapping for	*/
	char *		def_mappings;	/* default mappings		*/
{
	XtResource	rsc[1];
	char		stack[STACK_SIZE];
	char		mapping_stack[STACK_SIZE];
	char *		default_addr;
	char *		class_name;
	char *		token_mappings;
	int		eodm=0;		/* end of default mappings	*/

	class_name = ((int)(strlen((OLconst char *)token_name) + 1) >
			(int)STACK_SIZE ? XtNewString(token_name) :
				strcpy(stack, (OLconst char*)token_name));

			/* Make the first character of the class name
			 * be capitalized.				*/

	class_name[0] = toupper(class_name[0]);

				/* Find the end of the default mappings	*/

	while (!AT_EOL(def_mappings[eodm]))
	{
		++eodm;
	}

	default_addr = (char *)((eodm + 1) > sizeof(mapping_stack) ?
			XtMalloc(eodm + 1) : mapping_stack);

	(void) strncpy(default_addr, def_mappings, eodm);
	default_addr[eodm] = '\0';

	rsc[0].resource_name	= token_name;
	rsc[0].resource_class	= class_name;
	rsc[0].default_addr	= default_addr;
	rsc[0].resource_type	= XtRString;
	rsc[0].resource_size	= sizeof(String);
	rsc[0].default_type	= XtRString;
	rsc[0].resource_offset	= (Cardinal)0;

		/* Read resources from the application's database	*/

	XtGetApplicationResources(widget, (XtPointer) &token_mappings,
			rsc, (Cardinal)1, (ArgList)NULL, (Cardinal)0);

				/* Free the class name and the default
				 * mapping string if necessary.		*/

	if (class_name != stack)
	{
		XtFree(class_name);
	}

	if (default_addr != mapping_stack)
	{
		XtFree(default_addr);
	}

						/* Parse the Mappings	*/

	(void)ParseMappings(XtDisplay(widget), token_name, token_mappings);

				/* Return the end of mapping position	*/

	return(def_mappings + eodm);
} /* END OF GetTokenMappings() */

/*
 *************************************************************************
 * ParseEvent - parses a single event sequence looking for virtual
 * expressions.
 *	If one is found, a new VirtualProd node is added to the virtual
 * production tree.  If none are found, no new nodes are added.
 *	The routine updates the size of virtual production tree
 * (in characters).  If an error is encountered, processing stops and
 * the routine returns a non-NULL error message pointer.
 *	The argument "start" points into the current production at the
 * place where the last virtual expression was found.  "p_ptr" points at
 * the place where this routine should begin looking for the next event
 * sequence.  "p_ptr" will always be greater or equal to "start".
 *	This procedure modifies "start, p_ptr, size and has_vexpr".
 ****************************procedure*header*****************************
 */
static char *
ParseEvent(start, p_ptr, root, size, has_vexpr)
	char **		start;	/* address after last virtual expression*/
	char **		p_ptr;	/* start address of event type		*/
	VirtualProd *	root;	/* root of virtual production tree	*/
	int *		size;	/* size of the tree in chars		*/
	Boolean *	has_vexpr; /* does this event type have a virtual
				 * expression in it.			*/
{
	Boolean		negate;
	Boolean		match;
	char *		keyword;
	char *		error = (char *) NULL;
	char		name[100];
	XrmQuark	quark;
	register char *	p = *p_ptr;

						/* parse modifiers	*/

	*has_vexpr = False;

	SCAN_WHITESPACE(p);

			/* Normally, we'd check for the special
			 * modifiers "None" and "Any" if we were
			 * writing our own translation manager and
			 * return if we found them.  But since
			 * we're just copying the event string,
			 * treat them as ordinary citizens.		*/

	if (*p == '!') {
		++p;
		SCAN_WHITESPACE(p);
	}

						/* Parse modifiers until
						 * event type found	*/

	while (*p != '<' && !AT_EOL(*p)) {

			/* Due to the way we hash up the virtual
			 * expressions, the negate flag, '~', counts
			 * as the first character of the keyword. We'll
			 * tack it on just before checking the valid
			 * keyword.					*/

		if (*p == '~') {
			++p;
			negate = True;
		}
		else {
			negate = False;
		}

		keyword = p;
		SCAN_KEYWORD(p);

		if (p == keyword) {
			*p_ptr = p;
			if (negate)
				return("Modifier expected");
			else
				return("Modifier or '<' expected");
	 	}

				/* Tack on the '~' if this is a negated
				 * modifier.				*/

		if (negate == True)
			--keyword;

		(void)sprintf(name, "%.*s",(p - keyword), keyword);
		quark = XrmStringToQuark(name);

	 	if ((match = CheckForVBMatch(quark, False, &error)) == True)
			*has_vexpr = True;

		if (error != (char *) NULL) {
			*p_ptr = p;
			return(error);
		}
		else if (match == True) {
			template->text_start = *start;
			template->text_length = (int) (keyword - *start);

			*size = AddProdTreeNode(root,0,(VirtualProd *) NULL);
			*start = p;
		}
		SCAN_WHITESPACE(p);
	} /* End of looking for modifiers */

	if (*p != '<') {
		*p_ptr = p;
		return("Missing '<'");
	} else
		++p;

						/* Parse XEvent type	*/
	keyword = p;
	SCAN_KEYWORD(p);

	(void)sprintf(name, "%.*s",(p - keyword), keyword);
	quark = XrmStringToQuark(name);

	if ((match = CheckForVBMatch(quark, True, &error)) == True)
		*has_vexpr = True;

	if (error != (char *)NULL) {
		*p_ptr = p;
		return(error);
	}
	else if (match == True) {
		template->text_start = *start;
				/* subtract 1 char for the opening
				 * bracket				*/
		template->text_length = (int) (keyword - *start - 1);
		*size = AddProdTreeNode(root, 0, (VirtualProd *) NULL);

		if (*p == '>') {
			*start = p + 1;		/* add 1 to address to
						 * compensate for the
						 * closing bracket	*/
		}
		else {
			*p_ptr = p;
			return("Missing '>'");
		}
	}

	if (*p != '>') {
		*p_ptr = p;
		return("Missing '>'");
	}
	else
		++p;

	*p_ptr = p;
	return((char *) NULL);

} /* END OF ParseEvent() */

/*
 *************************************************************************
 * ParseList - this routine parses a string looking for individual
 * sets of virtual buttons or virtual keys and their associated mappings.
 * Once a new virtual button or virtual key name is found, it checks
 * to see if that name already exists.  If it does, an error is produced.
 * If the name is unique another routine is called to do the parsing of
 * the individual mappings for that name.
 *	See routine _OlAddVirtualMappings() for the input string format.
 ****************************procedure*header*****************************
 */
static void
ParseList(widget, s)
	Widget			widget;
	register OLconst char *	s;
{
	VBK *		vbk;
	OLconst char *	first_char;
	char		name_array[STACK_SIZE];
	XrmQuark	quark;

	if (s == (char *) NULL)
	{
		return;
	}

			/* Start at the left, looking for the virtual button
			 * or virtual key name.  Once it's found, parse up
			 * the mapping.					*/

	while (*s != '\0')
	{
	    SCAN_WHITESPACE(s);

	    first_char = s;
	    SCAN_KEYWORD(s);

	    if (s == first_char)
	    {
		OlVaDisplayWarningMsg(XtDisplay(widget),
					OleNfileVirtual,
					OleTmsg13,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg13, *s);

		SCAN_FOR_CHAR(s, '\n');
	    }
	    else
	    {
		(void)strncpy(name_array, first_char, (s - first_char));
		name_array[(int) (s - first_char)] = '\0';

					/* Check for duplicate name.	*/

		quark	= XrmStringToQuark(name_array);
		vbk	= LookUpHashEntry(quark);

		if (vbk != (VBK *) NULL)
		{
				/* Delete the current information	*/

			DeleteToken(vbk);
		}

				/* Do some error checking	*/

		if (*s != ':')
		{
			OlVaDisplayWarningMsg(XtDisplay(widget),
					OleNfileVirtual,
					OleTmsg14,
						OleCOlToolkitWarning,
						OleMfileVirtual_msg14,
					        name_array);

			SCAN_FOR_CHAR(s, '\n');
		}
		else
		{
						/* Good name		*/
			++s;
			s = GetTokenMappings(widget, name_array, (char *)s);
		}
	    }

	    if (*s == '\n')
		++s;

	} /* END OF while (*s != '\0) */

} /* END OF ParseList() */

/*
 *************************************************************************
 * ParseMappings - this routine parses the mapping string of a virtual
 * button or a virtual key.  By the time this routine is called, it has
 * been determined that the virtual button/key name is not a duplicate
 * of a previously defined button/key.
 ****************************procedure*header*****************************
 */
static char *
ParseMappings(display, name, s)
	Display *	display;	/* Our display			*/
	String		name;		/* virtual btn/key name to add	*/
	register char *	s;		/* start address of mapping	*/
{
	Mask	mask;			/* keyword's mask value		*/
	Mask	modifiers;		/* bitwise 'OR'ed mask values	*/
	char	buf[STACK_SIZE];	/* error/name character buffer	*/
	char *	error;			/* non-NULL if error occured	*/
	char *	start = s;		/* save the string's beginning	*/
	char *	keyword;		/* keyword pointer		*/
	KeySym	keysym;
	int	type = OL_NO_VIRTUAL_MAPPING;
	XrmQuark quark;
	XrmQuark qname;
	VBK *	 composed;	/* is button or key a virtual one ?	*/
	Cardinal num_estimate = 0;

	if (s == (char *) NULL || *s == '\0')
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg15,
				OleCOlToolkitWarning,
				OleMfileVirtual_msg15,
				name);
		return(s);
	}


				/* Initialize start up values	*/

	mask		= NULL;
	modifiers	= NULL;
	error		= (char *) NULL;
	qname		= XrmStringToQuark(name);

	SCAN_WHITESPACE(s);

	while (!AT_EOL(*s)) {

			/* If there are no mappings yet, quickly scan the
			 * string to estimate the number of mappings
			 * so that we can allocate enough memory for
			 * them.					*/

	    if (num_estimate == (Cardinal)0) {
		register char * ptr;

					/* Assume at least 1 mapping	*/
		++num_estimate;
			
		for (ptr = s; !AT_EOL(*ptr); ++ptr) {
			if (*ptr == SEPARATOR) {
				++num_estimate;
			}
		}
	    }

					/* Accumulate the modifiers	*/

	    while (*s != '<') {

		keyword = s;
		SCAN_KEYWORD(s);

		(void)sprintf(buf, "%.*s", (int) (s - keyword), keyword);
		quark = XrmStringToQuark(buf);

		if (CheckKeyword(quark, True, &mask) == True) {
			modifiers |= mask;
			SCAN_WHITESPACE(s);
		}
		else {
			(void)sprintf(buf, "Unknown modifier \"%s\"",
				XrmQuarkToString(quark));
			error = buf;
			break;
		}

		SCAN_WHITESPACE(s);
	    }

				/* If we haven't encountered an error,
				 * get the button or key		*/

	    if (error == (char *) NULL) {
		if (*s != '<') {
			error = "Missing '<'";
		}
		else {
			++s;

			keyword = s;
			SCAN_KEYWORD(s);

			(void)sprintf(buf, "%.*s", (int) (s - keyword),
					keyword);
			quark = XrmStringToQuark(buf);

				/* First check to see if the keyword is
				 * a virtual token			*/

			composed = LookUpHashEntry(quark);

			if (composed != (VBK *)NULL) {

			    error = ComposeVBK(buf, &type, display, qname,
						modifiers, composed,
						&num_estimate);

			    modifiers = NULL;

			}
			else if (CheckKeyword(quark, False, &mask) == True) {

				/* If we're here, the keyword is a button
				 * mapping				*/

			    if (type == OL_VIRTUAL_KEY) {
				(void)sprintf(buf, "Attempting to give button\
 mapping to virtual key \"%s\"", name);
				error = buf;
			    }
			    else {
				AddVBMapping(qname, modifiers, mask,
					NULLQUARK, &num_estimate);
				modifiers = NULL;
				type = OL_VIRTUAL_BUTTON;
			    }
			}
			else {
			    keysym = XStringToKeysym(buf);

			    if (keysym == NoSymbol) {
				(void)sprintf(buf, "Unknown virtual\
 button or key type \"%s\" for virtual token \"%s\"",
 XrmQuarkToString(quark), name);
				error = buf;
			    }
			    else if (type == OL_VIRTUAL_BUTTON) {
				(void)sprintf(buf, "Attempting to give key\
 mapping to virtual button \"%s\"", name);
				error = buf;
			    }
			    else {
				AddVKMapping(display, qname, modifiers,
					keysym, NULLQUARK, &num_estimate);
				modifiers = NULL;
				type = OL_VIRTUAL_KEY;
			    }
			} /* End of virtual key decision code */
		}
	    } /* End of code to get button or key */

	    if (error == (char *) NULL) {
		if (*s == '>') {
			++s;
			SCAN_WHITESPACE(s);

				/* Check for more mappings for this
				 * virtual button or key.		*/

			if (*s == ',')
				++s;
			else if (!AT_EOL(*s))
				error = "Expecting ',' or '\\n' or '\\0'";
		}
		else
			error = "Expecting '>'";
	    }

	    if (error != (char *) NULL)
	    {				/* Perform error recovery	*/
		SCAN_FOR_CHAR(s, ',');

		if (*s == ',')
			++s;

		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg16,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg16,
					error, name, (s - start), start);
		error = (char *) NULL;
	    }

	    SCAN_WHITESPACE(s);

	} /* END OF WHILE LOOP */

	return(s);
} /* END OF ParseMappings() */

/*
 *************************************************************************
 * ParseProduction - this routine parses a single production and builds
 * a virtual production tree that describes the original production.
 * If the production contains no virtual expressions, the passed-in
 * virtual production structure is filled in with the original information
 * and the routine returns False to indicate no virtual expressions were
 * found.
 *	If the production contains at least one virtual expression and the
 * production contains no errors, the passed-in virtual structure has
 * its subnode pointers set to branches in the virtual production tree,
 * and the routine returns True.
 *	If at least one virtual expression exists and the production
 * contains an error, passed-in structure is NOT filled-in with the
 * original information and the routine returns True.  This indicates
 * to the calling routine that the production had a virtual expression
 * in it.
 ****************************procedure*header*****************************
 */
static Boolean
ParseProduction(root, production, size)
	VirtualProd *	root;		/* root of the new tree		*/
	char **		production;	/* start address of production	*/
	int *		size;		/* size of the returned
					 * production (in chars)	*/
{
	Boolean		has_vexpr = False;
	char *		ptr;		/* temporary pointer		*/
	char *		start;		/* pointer into current production
					 * after last virtual expression*/
	register char *	p;		/* production pointer		*/
	char *		error = (char *) NULL;	/* error flag		*/
	extern VirtualProd *template;


				/* Initialize the variable "start".  This
				 * variable points at the beginning of
				 * a new event sequence.  Remember, there
				 * may be more than one event sequence
				 * per production.			*/

	p     = *production;
	start = *production;
	*size = 0;

				/* Parse the production.  Stop only at
				 * an end-of-line or a colon.		*/

	while (!AT_EOL(*p) && *p != ':') {

		SCAN_WHITESPACE(p);

		if (*p == '"') {		/* Quoted key sequence	*/
			++p;
			SCAN_FOR_CHAR(p, '"');
			if (*p != '"')
				error = "Unmatched quote, '\"'";
			else
				++p;
		}
		else {				/* Modifier or event type
						 * definition		*/

			ptr = p;	/* put 'p' into non-register var.*/

			error = ParseEvent(&start, &ptr,root,size,&has_vexpr);
			p = ptr;

						/* Skip repeats counts	*/

			if (!error && *p == '(') {
				SCAN_FOR_CHAR(p, ')');
				if (*p == ')')
					++p;
				else
					error = "Missing ')'";
			}
		}

		if (error)
			break;

		SCAN_WHITESPACE(p);

					/* Just Skip over any detail for
					 * this event.			*/

		while (!AT_EOL(*p) && *p != ':' && *p != ',')
			++p;

				/* Check to see if there's another event
				 * in this sequence.			*/

		if (*p != ':') {
			if (*p == ',')	/* Another event	*/
				++p;
			else {
				error = "Missing ':' after event sequence";
				break;
			}
		}
	} /* End of while() */

				/* At this point we finished parsing the
				 * left hand side of the production or
				 * we've encountered an error.		*/

	if (!error && *p != ':')
		error = "Missing ':' after event sequence";

				/* Get to the end of this production.
				 * This eats the right-hand-side for
				 * good productions and for bad 
				 * productions it is the error recovery	*/

	SCAN_FOR_CHAR(p, '\n');

	if (error != (char *)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
			OleNfileVirtual,
			OleTmsg17,
			OleCOlToolkitWarning,
			OleMfileVirtual_msg17, error, (int) (p - *production),
 *production);

				/* When an error occurs, only put the
				 * original production in the root
				 * node if no virtual expressions were
				 * found.				*/

		if (has_vexpr == False) {
			root->text_start	= *production;
			root->text_length	= (int) (p - *production + 1);
			root->vbk_length	= 0;
			*size			= root->text_length;
		}
		else {
			root->text_length	= 0;
			root->vbk_length	= 0;
			*size			= 0;
		}
	}
	else {
		if (has_vexpr == False) {
			root->text_start	= *production;
			root->text_length	= (int) (p - *production + 1);
			root->vbk_length	= 0;
			*size			= root->text_length;
		}
		else {
				/* Append the "right-hand-side" to the
				 * end of the virtual production tree.	*/

			template->text_start	= start;
			template->text_length	= (int) (p - start + 1);
			template->btn_or_key	= (VBK *) NULL;

			*size = AddProdTreeNode(root, 0, (VirtualProd *)NULL);
		}
	}

	*production = p;

	return(has_vexpr);

} /* END OF ParseProduction() */

/*
 *************************************************************************
 * PrintInitError - this routine prints an error message stating that
 * a virtual database has not been properly initialized.
 ****************************procedure*header*****************************
 */
static void
PrintInitError(display, func_name)
	Display	*	display;
	OLconst char *	func_name;
{
	OlVaDisplayErrorMsg(display,
		OleNfileVirtual,
		OleTmsg18,
		OleCOlToolkitError,
	OleMfileVirtual_msg18, func_name);
} /* END OF PrintInitError() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlAddVirtualMappings - this routine is a simple interface to 
 * _OlAppAddVirtualMappings.  See that procedure for a description.
 ****************************procedure*header*****************************
 */
void
_OlAddVirtualMappings(string)
	String	string;			/* mapping string to be parsed	*/
{
	_OlAppAddVirtualMappings((Widget)NULL, string);
} /* END OF _OlAddVirtualMappings() */

/*
 *************************************************************************
 * _OlAppAddVirtualMappings - this routine is called so that an application
 * writer can add new virtual keys or buttons to the existing
 * virtual key/button database.
 *
 * This routine will initialize the Virtual database if it has not been
 * initialized yet.
 *
 * A virtual key mapping string has the following form:
 *
 *	" UniqueNameA: modifiers <Key>, modifiers <Key> \n
 *	  UnniqueNameB: modifiers <Key>, modifiers <Key> "
 *
 * The virtual key name is followed by a colon ':'.  After the colon
 * list the modifiers (if any).   After the modifiers, list the
 * key in angle brackets, e.g., "<F1>".  The "key" can be a symbol
 * name or a symbol value, e.g., 'A' or '0x041.' It can also be
 * a previously defined virtual key. Different
 * mappings of the same virtual key can occur after a comma ','.
 *
 * A virtual button mapping string has the following form:
 *
 *	" UniqueNameA: modifiers <Button#>, modifiers <Button#> \n
 *	  UnniqueNameB: modifiers <Button#>, modifiers <Button#> "
 *
 * The virtual button name is followed by a colon ':'.  After the colon
 * list the modifiers (if any).  Other buttons (e.g., Button2, Button3,
 * etc.) can be used as modifiers.  After the modifiers, list the
 * mouse button in angle brackets, e.g., "<Button4>".  It can also be
 * a previously defined virtual button. Different
 * mappings of the same virtual button can occur after a comma ','.
 *	A newline character starts the definition of another virtual
 * button or key.  Virtual buttons and virtual keys can appear in
 * the same string.
 *
 * For each virtual token, the procedure looks in the database for
 * a user's mapping.  If none are found, the mappings specified with
 * each token will be used.
 ****************************procedure*header*****************************
 */
void
_OlAppAddVirtualMappings(widget, string)
	Widget		widget;		/* application's widget	or NULL	*/
	OLconst String	string;		/* mapping string to be parsed	*/
{
			/* If widget equals NULL, attempt to get the
			 * default widget				*/

	if (widget == (Widget)NULL)
	{
		widget = XtWindowToWidget(OlDefaultDisplay,
				DefaultRootWindow(OlDefaultDisplay));

		if (widget == (Widget)NULL)
		{
		        OlVaDisplayWarningMsg((Display *)NULL,
				OleNfileVirtual,
				OleTmsg19,
				OleCOlToolkitWarning,
				OleMfileVirtual_msg19);
			return;
		}
	}

	if (string != (String) NULL && *string != '\0')
	{
		if (DB_NOT_INITIALIZED)
		{
				/* Allocate memory for the Hash Table	*/

			VBKHashTable = (VBK **)XtCalloc((Cardinal)
					HASH_TABLE_SIZE, sizeof(VBK *));
		}

		ParseList(widget, string);
	}
} /* END OF _OlAppAddVirtualMappings() */

/*
 *************************************************************************
 * OlConvertVirtualTranslation - parses a translation string containing
 * virtual button/key expressions and returns a translation string that
 * is readable by the Xt Intrinsics.
 *
 * 	1. If the string contains virtual expressions, they are replaced
 *	   by their logical (Xt Intrinsic) representations.
 *	2. If the string does not contain virtual expressions, the
 *	   original string is returned.
 *	3. If the string contains any productions that are invalid,
 *	   those productions are ignored, and are not included in the
 *	   returned string (this only applies if the string contains
 *	   at least one valid virtual expression).
 ****************************procedure*header*****************************
 */
char *
OlConvertVirtualTranslation(translation)
	char *translation;		/* Unparsed translation		*/
{
	OLconst char *	func_name = "OlConvertVirtualTranslation";
	char *		start = translation;
	Boolean		has_vexpr = False;
	register int	i;
	register int	size = 0;
	register char *	ptr;
	PartialTrans *	head = (PartialTrans *) NULL;
	PartialTrans *	self = (PartialTrans *) NULL;
	PartialTrans *	last = (PartialTrans *) NULL;

	if (translation == (char *)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
			OleNfileVirtual,
			OleTmsg20,
			OleCOlToolkitWarning,
	   		OleMfileVirtual_msg20, func_name);
	   return(translation);
	}

	if (DB_NOT_INITIALIZED)
	{
/*		PrintInitError((Display *)NULL, func_name); */
		_OlInitVirtualMappings((Widget)NULL);
	}

				/* Parse Each production individually	*/

	SCAN_WHITESPACE(start);

	while (*start != '\0') {

			/* Get a structure to begin the production
			 * tree.  We'll use a "partial" translation
			 * node						*/

		self = XtNew(PartialTrans);

				/* Save the head of the partial tree
				 * and set the last node in the list	*/

		if (head == (PartialTrans *)NULL)
			head = self;
		else
			last->next = self;

		last = self;

				/* Initialize the structure to NULL	*/

		ptr = (char *) self;
		for (i=0; i < sizeof(PartialTrans); ++i, ++ptr)
			*ptr = (char) NULL;

				/* Parse the productions individually
				 * and record if at least one production
				 * contained a virtual expression	*/

		if (ParseProduction(&self->root, &start, &self->size))
			has_vexpr = True;

			/* Accumulate the size of the generated
			 * translation.					*/

		size += self->size;

		if (*start == '\n')
			++start;

	} /* END OF WHILE LOOP */

			/* At this point we've set up a linked list
			 * of nodes.  Each node contains a tree
			 * describing a single production.  Now, we
			 * check to see if there were any virtual
			 * expressions in the original translation	*/

	if (has_vexpr == True) {

	    if (size > 0) {
					/* Allocate memory		*/

		translation = XtMalloc((unsigned int) size * sizeof(char));

			/* Fill in the translation with the new
			 * productions					*/

		start = translation;
		for (self = head; self; self = self->next)
			start = CreateProduction(&self->root, start);

		translation[size - 1] = '\0';
	    }
	    else {
			/* This is the case when the translation contained
			 * virtual productions and, those productions all
			 * contained errors.				*/

		translation = (char *) NULL;
	    }
	}


			/* Collapse the partial translation list by
			 * collapsing the production trees under
			 * each partial translation node.		*/

	if (head != (PartialTrans *)NULL) {
		do {
			self = head;
			head = head->next;

			CollapseProdTree(&self->root, True);

			XtFree((char *)self);
		} while (head);
	}

	return(translation);

} /* END OF OlConvertVirtualTranslation() */

/*
 *************************************************************************
 * _OlDumpVirtualMappings - dumps the contents of the virtual button list
 * to a file.
 ****************************procedure*header*****************************
 */
void
_OlDumpVirtualMappings(fp, long_form)
	FILE    *fp;			/* destination file		*/
	Boolean  long_form;		/* Long dump form ???		*/
{
	register int            i, j;
	register VirtualButton *vb;
	register VirtualKey *	vk;

	if (DB_NOT_INITIALIZED)
	{
/*		PrintInitError((Display *)NULL, (OLconst char *) */
/*				"_OlDumpVirtualMappings");	 */
		_OlInitVirtualMappings((Widget)NULL);
	}

	(void)fprintf(fp, "\n**** Virtual Mapping Dump -- %s ****\n",
		(long_form == True ? "long form" : "short form"));

	if (long_form) {
	  for (vb = VBList; vb; vb = vb->next) {
	    (void)fprintf(fp, "\n Virtual Button: \"%s\"",
					XrmQuarkToString(vb->qname));
	    (void)fprintf(fp, " has %d %s:", vb->num_mappings,
		(vb->num_mappings > 1 ? "mappings" : "mapping"));
	    (void)fprintf(fp, "\n\tValid Virtual Expressions:");
	    for (i = 0; i < VB_ELEMENTS; ++i)
	    	(void)fprintf(fp, "\n\t\t %s", XrmQuarkToString(vb->quarks[i]));
	    for (i = 0; i < vb->num_mappings; ++i) {
		(void)fprintf(fp, "\n\tMapping #%d:", i+1);
	 	(void)fprintf(fp,
			"\tmod mask = %lX, btn mask = %lX, btn type %u",
			vb->mappings[i].modifiers, vb->mappings[i].button,
			(unsigned)vb->mappings[i].button_type);
		for (j=0; j < VB_ELEMENTS; ++j) {
		    (void)fprintf(fp, "\n\t\t\t %2d:",
				vb->mappings[i].length[j]);
		    (void)fprintf(fp, " %s", vb->mappings[i].text[j]);
		}
	    }
	  }

	  for (vk = VKList; vk; vk = vk->next) {
	    (void)fprintf(fp, "\n Virtual Key: \"%s\"",
					XrmQuarkToString(vk->qname));
	    (void)fprintf(fp, " has %d %s:", vk->num_mappings,
		(vk->num_mappings > 1 ? "mappings" : "mapping"));
	    (void)fprintf(fp, "\n\tValid Virtual Expressions:");
	    for (i = 0; i < VK_ELEMENTS; ++i)
	    	(void)fprintf(fp, "\n\t\t %s", XrmQuarkToString(vk->quarks[i]));
	    for (i = 0; i < vk->num_mappings; ++i) {
		(void)fprintf(fp, "\n\tMapping #%d:", i+1);
	 	(void)fprintf(fp, "\tmod mask = %lX, keycode = %u, keysym %lX",
			vk->mappings[i].modifiers, vk->mappings[i].keycode,
			vk->mappings[i].keysym);
		for (j=0; j < VK_ELEMENTS; ++j) {
		    (void)fprintf(fp, "\n\t\t\t %2d:",
				vk->mappings[i].length[j]);
		    (void)fprintf(fp, " %s", vk->mappings[i].text[j]);
		}
	    }
	  }
	}
	else {
						/* Short form listing */

	  for (vb = VBList; vb; vb = vb->next) {
	  	(void)fprintf(fp, "\n Virtual Button \"%s\" has %d %s",
			XrmQuarkToString(vb->qname),
			vb->num_mappings,
			(vb->num_mappings > 1 ? "mappings" : "mapping"));
	  }

	  for (vk = VKList; vk; vk = vk->next) {
	  	(void)fprintf(fp, "\n Virtual Key \"%s\" has %d %s",
			XrmQuarkToString(vk->qname),
			vk->num_mappings,
			(vk->num_mappings > 1 ? "mappings" : "mapping"));
	  }
	}
	(void)fprintf(fp, "\n");
} /* END OF _OlDumpVirtualMappings() */

/*
 *************************************************************************
 * _OlGetVirtualMappings - returns the number of mappings for a
 * specified virtual token.  The routine will fill in an application-
 * supplied array with the virtual token's information if one is supplied.
 ****************************procedure*header*****************************
 */
Cardinal
_OlGetVirtualMappings(name, list, list_length)
	String			name;		/* virtual token name	*/
	_OlVirtualMapping	list[];		/* array to be filled in*/
	Cardinal		list_length;	/* array size		*/
{
	Cardinal	i;
	Cardinal	max;
	VBK *		self;
	OLconst char *	func_name = "_OlGetVirtualMappings";

	if (DB_NOT_INITIALIZED)
	{
/*		PrintInitError((Display *)NULL, func_name); */
		_OlInitVirtualMappings((Widget)NULL);
	}
	else if (list_length > (Cardinal)0 && list == (_OlVirtualMapping *)NULL)
	{
		OlVaDisplayErrorMsg((Display *)NULL,
			OleNfileVirtual,
			OleTmsg21,
			OleCOlToolkitError,
			OleMfileVirtual_msg21,
			func_name);
	}
	else if (name == (String)NULL || *name == '\0')
	{
		OlVaDisplayErrorMsg((Display *)NULL,
			OleNfileVirtual,
			OleTmsg22,
			OleCOlToolkitError,
			OleMfileVirtual_msg22, func_name);
	}

	self = LookUpHashEntry(XrmStringToQuark(name));

	if (self == (VBK *) NULL)
		return((Cardinal)0);

	max = list_length;

	if (self->type == OL_VIRTUAL_BUTTON) {
		VirtualButton *	vb = (VirtualButton *) self->opaque;

		if (vb->num_mappings < max)
			max = vb->num_mappings;

		for (i=(Cardinal)0; i < max; ++i) {
		    list[i].type	= self->type;
		    list[i].modifiers	= vb->mappings[i].modifiers;
		    list[i].detail	= (unsigned int)vb->mappings[i].button;
		    list[i].composed	=
				XrmQuarkToString(vb->mappings[i].composed);
		}

		return(vb->num_mappings);
	}
	else {				/* Default to virtual Key	*/

		VirtualKey * vk = (VirtualKey *) self->opaque;

		if (vk->num_mappings < max)
			max = vk->num_mappings;

		for (i=(Cardinal)0; i < max; ++i) {
		    list[i].type	= self->type;
		    list[i].modifiers	= vk->mappings[i].modifiers;
		    list[i].detail	= (unsigned int)vk->mappings[i].keycode;
		    list[i].composed	=
				XrmQuarkToString(vk->mappings[i].composed);
		}

		return(vk->num_mappings);
	}
} /* END OF _OlGetVirtualMappings() */

/*
 *************************************************************************
 * _OlInitVirtualMappings - initializes the virtual button structure list
 * with the minimum required virtual buttons and virtual keys.
 * This function is not really necessary since it only calls another
 * routine.  The reason this functiion remains is because it was an
 * external in prior releases and some customers are calling it.
 ****************************procedure*header*****************************
 */
void
_OlInitVirtualMappings(widget)
    Widget			widget;
{
#define MORESLOTS	2

	static Widget *		list = NULL;
	static short		list_slots_left = 0;
	static short		list_alloced = 0;
	static short		num_list = 0;

	int			i;
				/* Add the virtual mappings     */

	_OlAppAddVirtualMappings(widget, (String)OlCoreVirtualMappings);
	_OlAppAddVirtualMappings(widget, (String)OlCoreTextVirtualMappings);

				/* make sure we hasn't register the	*/
				/* dynamic callback yet. if it already	*/
				/* did, then we are done		*/
	for (i = 0; i < num_list; i++)
		if (list[i] == widget)
		{
			return;
		}
				/* add itself into `list' and do the	*/
				/* registration				*/
	if (list_slots_left == 0)
	{
		list_alloced += MORESLOTS;
		list_slots_left += MORESLOTS;
		list = (Widget *) XtRealloc((char *)list, list_alloced*sizeof(Widget));
	}
	list[num_list++] = widget;
				/* add itself into the dynamic list.	*/
				/* so it can update the virtual DB	*/
				/* dynamically, it helps the rountines	*/
				/* like _OlIsVirtualButton/Key/Event,	*/
				/* but not OlConvertVirtualTranslation. */
				/* To ensure the dynamic features work	*/
				/* correctly, widget writers should use */
				/* our latest event scheme.		*/
	OlRegisterDynamicCallback(
		(OlDynamicCallbackProc)_OlInitVirtualMappings,
		(XtPointer)widget);

#undef MORESLOTS
} /* END OF _OlInitVirtualMappings() */

/*
 *************************************************************************
 * _OlIsVirtualButton - this routine checks to see if a button and its
 * state is a virtual button event.  See function CheckIfVirtualButton()
 * for a full description.
 ****************************procedure*header*****************************
 */
Boolean
#ifdef OlNeedFunctionPrototypes
_OlIsVirtualButton(
	String			name,
	register unsigned int	state,
	unsigned int		button,
	Boolean			exclusive
	)
#else
_OlIsVirtualButton(name, state, button, exclusive)
	String			name;	/* Virtual Button Name		*/
	register unsigned int	state;	/* button or key state		*/
	unsigned int		button;	/* button number or zero	*/
	Boolean			exclusive; /* perfect match desired ??	*/
#endif
{
	return(CheckIfVirtualButton(name, state, button, exclusive,
		(OLconst char *)"_OlIsVirtualButton"));

} /* END OF _OlIsVirtualButton() */

/*
 *************************************************************************
 * _OlIsVirtualKey - this routine checks to see if a keycode and its
 * state is a virtual key mapping.  See function CheckIfVirtualKey() for
 * a full description.
 ****************************procedure*header*****************************
 */
Boolean
#ifdef OlNeedFunctionPrototypes
_OlIsVirtualKey(
	String			name,
	register unsigned int	state,
	KeyCode			keycode,
	Boolean			exclusive
	)
#else
_OlIsVirtualKey(name, state, keycode, exclusive)
	String			name;		/* Virtual Key Name	*/
	register unsigned int	state;		/* key state		*/
	KeyCode			keycode;	/* keycode		*/
	Boolean			exclusive;	/* perfect match desired ?? */
#endif
{
	return(CheckIfVirtualKey(name, state, keycode, exclusive,
		(OLconst char *)"_OlIsVirtualKey"));

} /* END OF _OlIsVirtualKey() */

/*
 *************************************************************************
 * _OlIsVirtualEvent - this sees if an XEvent is a virtual button or
 * virtual key event.  The function returns true if it is, False if it
 * is not.
 *	For virtual buttons, this routine checks the following events:
 *		ButtonPress, ButtonRelease, ButtonMotion, EnterNotify,
 *		LeaveNotify
 *	For virtual keys, this routine checks the following events:
 *		KeyPress, KeyRelease
 ****************************procedure*header*****************************
 */
Boolean
#ifdef OlNeedFunctionPrototypes
_OlIsVirtualEvent(
	String		name,
	XEvent *	xevent,
	Boolean		exclusive
	)
#else
_OlIsVirtualEvent(name,	xevent, exclusive)
	String		name;		/* virtual button/key name	*/
	XEvent *	xevent;		/* The XEvent struct pointer	*/
	Boolean		exclusive;	/* only true if perfect match	*/
#endif
{
	Boolean	status;
	OLconst char *	func_name = "_OlIsVirtualEvent";

	if (xevent == (XEvent *)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL,
					OleNfileVirtual,
					OleTmsg23,
					OleCOlToolkitWarning,
					OleMfileVirtual_msg23);
		return((Boolean)False);
	}

	switch (xevent->xany.type) {
	case ButtonPress:
							/* Fall through	*/
	case ButtonRelease:
		status = CheckIfVirtualButton(name, xevent->xbutton.state,
				xevent->xbutton.button, exclusive, func_name);
		break;
	case EnterNotify:
							/* Fall through	*/
	case LeaveNotify:
		status = CheckIfVirtualButton(name, xevent->xcrossing.state,
				(unsigned int)NULL, exclusive, func_name);
		break;
	case MotionNotify:
		status = CheckIfVirtualButton(name, xevent->xmotion.state,
				(unsigned int)NULL, exclusive, func_name);
		break;
	case KeyPress:
							/* Fall through	*/
	case KeyRelease:
		status = CheckIfVirtualKey(name, xevent->xkey.state,
				xevent->xkey.keycode, exclusive, func_name);
		break;
	default:
		status = False;

		OlVaDisplayWarningMsg((Display *)NULL,
			OleNfileVirtual,
			OleTmsg24,
			OleCOlToolkitWarning,
			OleMfileVirtual_msg24, func_name);
		break;
	}
	return(status);
} /* END OF _OlIsVirtualEvent() */

