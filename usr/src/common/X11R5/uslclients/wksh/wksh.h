
/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:wksh.h	1.3"


#define malloc do_not_redeclare_malloc
#include "name.h"
#undef malloc

#define SUCCESS		0
#define FAIL		(-1)
#define CONSTCHAR	(const char *)

/* bits for the flags field of wtab_t */

#define F_CHILD		1
#define F_TOPLEVEL	2

/*
 * Table types
 */
#define TAB_EMPTY	0
#define TAB_FIELDS	1
#define TAB_ITEMS	2
#define TAB_WIDGET	3

typedef struct {
	const char *name;
	const char *type;
	int size;
} resfixup_t;

/*
 * The callback_tab_t structure is used to map call_data parameters
 * for those callbacks that require them.  For each resource that is
 * of type Callback that has an associated call_data parameter an
 * entry in the class's callback_tab should exist.  The precb_proc()
 * will be exected before the callback is issued, and the postcb_proc()
 * will be issued following the execution of the callback.  The precb_proc()
 * will presumably set up a bunch of environment variables to contain
 * ascii representations of various fields of the call_data.  The postcb_proc()
 * will typically be NULL, but sometimes is useful.  As a convention, the
 * environment variables that are set up to contain the call_data should
 * be of the form:  CALL_DATA_<fieldname-in-upper-case>.  For example, for
 * textField widget of OPEN LOOK, the XtNverification callback has a call_data
 * parameter of type OlTextFieldVerify which has three fields: reason, string,
 * and ok.  For this widget, the precb_proc() will set up three environment
 * variables: CALL_DATA_REASON, CALL_DATA_STRING, and CALL_DATA_OK.
 */

typedef struct {
	const char *resname;
	void (*precb_proc)();
	void (*postcb_proc)();
} callback_tab_t;

typedef struct {
	const char *cname;
	void    (*initfn)();	/* to init convenience vars, etc. */
	WidgetClass class;	/* Class record */
	const resfixup_t  *resfix;	/* fixup list for resources */
	const resfixup_t  *confix;	/* fixup list for constraint resources */
	struct Amemory	*res;	/* Hashed list of resources */
	struct Amemory	*con;	/* Hashed list of constraint resources */
	const callback_tab_t *cbtab;
} classtab_t;

typedef struct wtab {
	int    type;		/* entry type (TAB_) */
	int    size;		/* entry size */
	Widget w;		/* widget pointer */ 
	char   *wname;		/* name of widget */
	char   *widid;		/* id of widget */
	classtab_t   *wclass;	/* widget's class */
	struct wtab *parent;	/* pointer to widget's parent wtab_t */
	char *envar;		/* initial environment variable user gave */
	caddr_t info;		/* Some widgets use this for any other info */
} wtab_t;

typedef struct {
	char *ksh_cmd;
	wtab_t *w;
	const callback_tab_t *cbtab;
} wksh_client_data_t;

typedef struct {
	int fd;			/* the input source, in NDELAY mode */
	char lnbuf[BUFSIZ];	/* a line being built to execute */
	int  lnend;		/* current end of the line */
	char **cmds;		/* the ksh commands to execute given the line */
	int  ncmds;		/* the number of ksh commands */
} inputrec_t;

extern wtab_t *str_to_wtab();
extern classtab_t *str_to_class();
extern XtResource	str_to_resource();
