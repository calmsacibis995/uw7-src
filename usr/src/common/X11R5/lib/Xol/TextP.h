#ifndef NOIDENT
#ident	"@(#)oldtext:TextP.h	1.17"
#endif

/**
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **/

#ifndef _OlTextP_h
#define _OlTextP_h

#include <Xol/ManagerP.h>		/* include superclasses' header */
#include <Xol/Text.h>
#include <Xol/BulletinBP.h>
#include <Xol/ScrollbarP.h>

/***********************************************************************
 *
 * Text Widget Private Data
 *
 ***********************************************************************/

/* New fields for the Text widget class record */
typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} TextClassPart;

/* Full class record declaration */
typedef struct _TextClassRec
{
  CoreClassPart			core_class;
  CompositeClassPart  		composite_class;
  ConstraintClassPart		constraint_class;
  ManagerClassPart		manager_class;
  TextClassPart			text_class;
} TextClassRec;

extern TextClassRec	textClassRec;

/* New fields for the Text widget record */
typedef struct _TextPart
{
  /* ScrollBar stuff: */
  int			current_page;
  int			h_granularity;
  int                   h_initial_delay;
  Widget		h_menu_pane;
  int			h_repeat_rate;
  XtCallbackList	h_slider_moved;
  XtCallbackList	paged;
  OlDefine		show_page;
  int			v_granularity;
  int			v_initial_delay;
  Widget		v_menu_pane;
  int			v_repeat_rate;
  XtCallbackList	v_slider_moved;

  /* Text stuff: */
  int			align_horizontal;
  int			align_vertical;
  BulletinBoardWidget	bboard;
  Widget		bb_child;
  Boolean		force_hsb;
  Boolean		force_vsb;
  Pixel			foreground;
  ScrollbarWidget	hsb;
  Boolean		in_init;
  int			init_x;
  int			init_y;
  Boolean		recompute_view_height;
  Boolean		recompute_view_width;
  int			view_height;
  int			view_width;
  ScrollbarWidget	vsb;
  XtCallbackList	mark;
  XtCallbackList	motion_verification;
  XtCallbackList	modify_verification;
  XtCallbackList	leave_verification;
} TextPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _TextRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    TextPart		text;
} TextRec;

#endif
