#ifndef	NOIDENT
#ident	"@(#)virtual:VirtualP.h	1.12"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This is a private header file for the Virtual Button and
 *	Virtual Key Code.
 *
 ******************************file*header********************************
 */

				/* Define constants for buttons & keys.
				 * These will be used as indices into
				 * arrays				*/

#define VB_UP		0	/* Virtual Button Release		*/
#define VB_DOWN		1	/* Virtual Button Press			*/
#define VB_MOTION	2	/* Virtual Button Motion		*/
#define VB_NORM_MOD	3	/* Virtual Button Normal Modifier	*/
#define VB_NEGATED_MOD	4	/* Virtual Button Negated Modifier	*/
#define VB_ELEMENTS	5	/* Number of Virtual Button choices,
				 * VB_UP, VB_DOWN, VB_MOTION, VB_NORM_MOD
				 * or VB_NEGATED_MOD			*/

#define VK_UP		0	/* Virtual key Release, e.g., "FooUp"	*/
#define VK_DOWN		1	/* Virtual key Press, e.g., "FooDown"	*/
#define VK_IMPLIED_DOWN	2	/* Virtual key Press, e.g., "Foo"	*/
#define VK_ELEMENTS	3	/* Number of Virtual Key choices,
				 * VK_UP, VK_DOWN or VK_IMPLIED_DOWN	*/

typedef struct {
    	XrmQuark	composed;	/* quarkified virtual key name
					 * used to compose this one	*/
        Mask		modifiers;	/* modifier mask		*/
	KeyCode		keycode;	/* the keycode			*/
	KeySym		keysym;		/* the key symbol		*/
    	String		text[VK_ELEMENTS]; /* virtual key mapping text	*/
    	Cardinal	length[VK_ELEMENTS]; /* length of text field	*/
} VKeyMapping;

typedef struct _VirtualKey {
    struct _VirtualKey *next;		/* next in list			*/
    XrmQuark		qname;		/* quarkified name		*/
    Cardinal		num_mappings;	/* number of virtual mappings	*/
    XrmQuark		quarks[VK_ELEMENTS]; /* quarked virtual key	*/
    VKeyMapping *	mappings;	/* mapping array		*/
} VirtualKey;

typedef struct {
    	XrmQuark	composed;	/* quarkified virtual button name
					 * used to compose this one	*/
        Mask		modifiers;	/* modifier mask		*/
        Mask		button;		/* button mask			*/
	unsigned int	button_type;	/* button type (for XEvents)	*/
    	String		text[VB_ELEMENTS]; /* virtual btn mapping text	*/
    	Cardinal	length[VB_ELEMENTS]; /* length of text field	*/
} VButtonMapping;

typedef struct _VirtualButton {
    struct _VirtualButton *next;	/* next in list			*/
    XrmQuark		qname;		/* quarkified name		*/
    Cardinal		num_mappings;	/* number of virtual mappings	*/
    XrmQuark		quarks[VB_ELEMENTS]; /* quarked virtual button	*/
    VButtonMapping *	mappings;	/* mapping array		*/
} VirtualButton;

			/* Define a structure that can hash all of the
			 * elements of both the keys and buttons	*/

typedef XtPointer	OpaqueVBK;

typedef struct _VButtonOrKey {
    struct _VButtonOrKey * next;	/* next one in list		*/
    XrmQuark		qname;		/* quarkified name of this node	*/
    OpaqueVBK		opaque;		/* button or key pointer	*/
    int			type;		/* button or key flag		*/
    int			element_index;	/* element array index		*/
} VBK;


			/* Define a virtual production node.  A tree
			 * of these nodes describes a single production	*/

typedef struct _VProd {
    struct _VProd *	subnode[OL_MAX_VIRTUAL_MAPPINGS]; /* subnodes
					 * in tree			*/
    struct _VProd *	prev;		/* previous node in tree	*/
    char *		text_start;	/* start address of original
					 * production text		*/
    int                 text_length;	/* length of text to copy	*/
    char *		vbk_text;	/* text from virtual btn/key	*/
    int			vbk_length;	/* virtual btn/key text length	*/
    VBK *		btn_or_key;	/* virtual button/key to copy	*/
    int			mapping_index;	/* mapping index		*/
} VirtualProd;


			/* Define a structure that describes partial
			 * translations.  A linked list of these
			 * structures define a complete translation.
			 * Essentially, we have a linked list of trees.
			 * Each tree describes a single production.
			 * Therefore, there are as many trees in the
			 * linked list as there were valid productions
			 * in the initial translation.			*/

typedef struct _PartialTrans {
	struct _PartialTrans *next;	/* next structure in list	*/
	VirtualProd	      root;	/* root of virtual production
					 * tree				*/
	int                   size;	/* size of virtual production
					 * tree in characters		*/
} PartialTrans;

typedef struct {
	char *		keyword;
	Mask		mask;
	unsigned int	button_type;
	XrmQuark	quark;
} TableEntry;

