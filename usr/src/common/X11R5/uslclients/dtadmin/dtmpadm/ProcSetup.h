#ifndef NOIDENT
#ident	"@(#)ProcSetup.h	1.2"
#endif

#define TIGHTNESS 20
#define BUF_SIZE	  128

#define OFFLINE_ICON	"proc.off32"
#define ONLINE_ICON	"proc.on32"

#define OFFLINE		"OFFLINE"
#define ONLINE		"ONLINE"

typedef struct _menu_item {
    char        *label;         /* the label for the item */
    Widget	widget;	/* the handle of the item on the menu */
    WidgetClass *class;         /* pushbutton, label, separator... */
    char        *mnemonic;      /* mnemonic; NULL if none */
    char        *accelerator;   /* accelerator; NULL if none */
    char        *accel_text;    /* to be converted to compound string */
    void       (*callback)();   /* routine to call; NULL if none */
    XtPointer    callback_data; /* client_data for callback() */
    struct _menu_item *subitems; /* pullright menu items, if not NULL */
} MenuItem;

typedef struct {
	char	*label;
	void	(*callback)();
	caddr_t	data;
} ActionAreaItem;

typedef struct {
	char	*title;
	char	*file;
	char	*section;
} HelpText;

extern void	DisplayHelp(Widget, HelpText *);

