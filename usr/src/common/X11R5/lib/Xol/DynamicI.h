#ifndef NOIDENT
#ident	"@(#)olmisc:DynamicI.h	1.11"
#endif

#ifndef __DynamicI_h__
#define __DynamicI_h__

/*
 *  Description:
 *		This file contains all private tables for Dynamic.c.
 */


static btn_mapping btn_mappings[] = {
   { Button1, Button1Mask },
   { Button2, Button2Mask },
   { Button3, Button3Mask },
   { Button4, Button4Mask },
   { Button5, Button5Mask },
};

static mapping mappings[] = {
   { "None",       None          },
   { "Button1",    Button1       },
   { "Button2",    Button2       },
   { "Button3",    Button3       },
   { "Button4",    Button4       },
   { "Button5",    Button5       },
   { "Shift",      ShiftMask     },
   { "s",          ShiftMask     },
   { "Ctrl",       ControlMask   },
   { "c",          ControlMask   },
   { "Lock",       LockMask      },
   { "l",          LockMask      },
   { "Alt",        Mod1Mask      },
   { "a",          Mod1Mask      },
   { "Meta",       Mod1Mask      },
   { "m",          Mod1Mask      },
   { "Mod1",       Mod1Mask      },
   { "1",          Mod1Mask      },
   { "Mod2",       Mod2Mask      },
   { "2",          Mod2Mask      },
   { "Mod3",       Mod3Mask      },
   { "3",          Mod3Mask      },
   { "Mod4",       Mod4Mask      },
   { "4",          Mod4Mask      },
   { "Mod5",       Mod5Mask      },
   { "5",          Mod5Mask      },
   { "Super",      Mod1Mask      },
   { "su",         Mod1Mask      },
   { "Hyper",      Mod1Mask      },
   { "h",          Mod1Mask      },
};

/*
 * The following two tables are used by StringToKey().
 * (I18N)
 */

static CharKeysymMap backslash_map[] = {
   { 'b',  XK_BackSpace    },
   { 't',  XK_Tab          },
   { 'n',  XK_Linefeed     },
   { 'v',  XK_Clear        },
   { 'f',  XK_Clear        },
   { 'r',  XK_Return       },
   {   0,  0               }
};

static CharKeysymMap singlechar_map[] = {
   { ' ',  XK_space        },
   { '!',  XK_exclam       },
   { '"',  XK_quotedbl     },
   { '#',  XK_numbersign   },
   { '$',  XK_dollar       },
   { '%',  XK_percent      },
   { '&',  XK_ampersand    },
#if	defined(XK_apostrophe)
   { '\'', XK_apostrophe   },
#else
   { '\'', XK_quoteright   },
#endif
   { '(',  XK_parenleft    },
   { ')',  XK_parenright   },
   { '*',  XK_asterisk     },
   { '+',  XK_plus         },
   { ',',  XK_comma        },
   { '-',  XK_minus        },
   { '.',  XK_period       },
   { '/',  XK_slash        },
   { ':',  XK_colon        },
   { ';',  XK_semicolon    },
   { '<',  XK_less         },
   { '=',  XK_equal        },
   { '>',  XK_greater      },
   { '?',  XK_question     },
   { '@',  XK_at           },
   { '[',  XK_bracketleft  },
   { '\\', XK_backslash    },
   { ']',  XK_bracketright },
   { '^',  XK_asciicircum  },
   { '_',  XK_underscore   },
#if	defined(XK_grave)
   { '`',  XK_grave        },
#else
   { '`',  XK_quoteleft    },
#endif
   { '{',  XK_braceleft    },
   { '|',  XK_bar          },
   { '}',  XK_braceright   },
   { '~',  XK_asciitilde   },
   {   0,  0               }
};

static OlKeyBinding OlCoreKeyBindings[] = {

/* button equivalents */

   { XtNselectKey,	"<space>,c<space>",	OL_SELECTKEY /* OLM_KSelect */},
   { XtNmenuKey,	"c<m>,n<F4>",		OL_MENUKEY   /* OLM_KMenu */},

   { XtNmenuDefaultKey,	"c s<m>,s<F4>",		OL_MENUDEFAULTKEY /* N.A. */},
   { XtNhorizSBMenuKey,	"a c<r>",		OL_HSBMENU	  /* N.A. */},
   { XtNvertSBMenuKey,	"c<r>",			OL_VSBMENU	  /* N.A. */},
   { XtNduplicateKey,	"a<space>",		OL_DUPLICATEKEY	  /* N.A. */},
   { XtNlinkKey,	"a c<space>",		OL_LINKKEY	  /* N.A. */},

   { XtNadjustKey,	"c s<ampersand>",	OL_ADJUSTKEY	  /* N.A. */},
   { XtNextendKey,	"s<space>",		OLM_KExtend	  /* N.A. */},

/* olwm + olwsm */

   { XtNworkspaceMenuKey,"c<w>",		OL_WORKSPACEMENU  /* N.A. */},
   { XtNnextAppKey,	 "a<Escape>,a<Tab>",   	OL_NEXTAPP
					     /* OLM_KNextFamilyWindow     */},
   { XtNprevAppKey,	 "a s<Escape>,a s<Tab>",OL_PREVAPP
					     /* OLM_KPrevFamilyWindow     */},


   { XtNnextWinKey,	"a<F6>",	OL_NEXTWINDOW /* OLM_KNextWindow */},
   { XtNprevWinKey,	"a s<F6>",	OL_PREVWINDOW /* OLM_KPrevWindow */},
   { XtNwindowMenuKey,	"s<Escape>",	OL_WINDOWMENU /* OLM_KWindowMenu */},

/* traversal */

   { XtNdownKey,	"<Down>",	   OL_MOVEDOWN	/*OLM_KDown */},
   { XtNleftKey,	"<Left>",	   OL_MOVELEFT	/*OLM_KLeft */},
   { XtNmultiDownKey,	"c<Down>",	   OL_MULTIDOWN /*OLM_KNextMenuDown */},
   { XtNmultiLeftKey,	"c<Left>",	   OL_MULTILEFT /*OLM_KPrevMenuLeft */},
   { XtNmultiRightKey,	"c<Right>",	   OL_MULTIRIGHT/*OLM_KNextMenuRight*/},
   { XtNmultiUpKey,	"c<Up>",	   OL_MULTIUP	/*OLM_KPrevMenuUp */},
   { XtNnextFieldKey,	"n<Tab>,c<Tab>",   OL_NEXT_FIELD/*OLM_KNextField */},
   { XtNprevFieldKey,	"s<Tab>,c s<Tab>", OL_PREV_FIELD/*OLM_KPrevField */},
   { XtNrightKey,	"<Right>",	   OL_MOVERIGHT /*OLM_KRight */},
   { XtNupKey,		"<Up>",		   OL_MOVEUP	/*OLM_KUp */},
   { XtNmenubarKey,	"n<F10>",	   OL_MENUBARKEY/*OLM_KMenuBar */},	

/* selection */

#ifndef	sun
   { XtNcopyKey,	"c<Insert>",	OL_COPY		/* OLM_KCopy */},
   { XtNcutKey,		"s<Delete>",	OL_CUT		/* OLM_KCut */},
   { XtNpasteKey,	"s<Insert>",	OL_PASTE	/* OLM_KPaste */},
#else
   { XtNcopyKey,	"n<F16>",	OL_COPY		},
   { XtNcutKey,		"n<F20>",	OL_CUT		},
   { XtNpasteKey,	"n<F18>",	OL_PASTE	},
#endif

   { XtNselCharBakKey,	"s<Left>",	OL_SELCHARBAK	/* N.A. */},
   { XtNselCharFwdKey,	"s<Right>",	OL_SELCHARFWD	/* N.A. */},
   { XtNselLineKey,	"c a<Left>",	OL_SELLINE	/* N.A. */},
   { XtNselLineBakKey,	"s<Home>",	OL_SELLINEBAK	/* N.A. */},
   { XtNselLineFwdKey,	"s<End>",	OL_SELLINEFWD	/* N.A. */},
   { XtNselWordBakKey,	"c s<Left>",	OL_SELWORDBAK	/* N.A. */},
   { XtNselWordFwdKey,	"c s<Right>",	OL_SELWORDFWD	/* N.A. */},
   { XtNselFlipEndsKey,	"a<Insert>",	OL_SELFLIPENDS	/* N.A. */},

/* misc */

   { XtNcancelKey,       "n<Escape>",	OL_CANCEL	/* OLM_KCancel */},
   { XtNdragKey,         "n<F5>",	OL_DRAG		/* N.A. */},
   { XtNdropKey,         "n<F2>",	OL_DROP		/* N.A. */},
   { XtNtogglePushpinKey,"c<t>",	OL_TOGGLEPUSHPIN/* N.A. */},

   { XtNdefaultActionKey,"<Return>,c<Return>",	OL_DEFAULTACTION
					     /* OLM_KActivate   */},

#ifndef	sun
   { XtNhelpKey,	"n<F1>",	OL_HELP		/* OLM_KHelp */},
   { XtNpropertiesKey,	"c<p>",		OL_PROPERTY	/* N.A. */},
   { XtNstopKey,	"c<s>",		OL_STOP		/* N.A. */},
   { XtNundoKey,	"a<BackSpace>",	OL_UNDO		/* OLM_KUndo */},
#else
   { XtNhelpKey,	"n<Help>",	OL_HELP		},
   { XtNpropertiesKey,	"n<F13>",	OL_PROPERTY	},
   { XtNstopKey,	"n<F11>",	OL_STOP		},
   { XtNundoKey,	"n<F14>",	OL_UNDO		},
#endif

/* scrolling */

   { XtNpageDownKey,	   "<Next>",	  OL_PAGEDOWN	/* OLM_KPageDown */},
   { XtNpageLeftKey,	   "c<Prior>",	  OL_PAGELEFT	/* OLM_KPageLeft */},
   { XtNpageRightKey,	   "c<Next>",	  OL_PAGERIGHT	/* OLM_KPageRight */},
   { XtNpageUpKey,	   "<Prior>",	  OL_PAGEUP	/* OLM_KPageUp */},

   { XtNscrollBottomKey,   "a<Next>",	     OL_SCROLLBOTTOM	/* N.A. */},
   { XtNscrollDownKey,	   "c<bracketleft>", OL_SCROLLDOWN	/* N.A. */},
   { XtNscrollLeftKey,	   "a<bracketleft>", OL_SCROLLLEFT	/* N.A. */},
   { XtNscrollLeftEdgeKey, "a s<braceleft>", OL_SCROLLLEFTEDGE	/* N.A. */},
   { XtNscrollRightKey,	   "a<bracketright>",OL_SCROLLRIGHT	/* N.A. */},
   { XtNscrollRightEdgeKey,"a s<braceright>",OL_SCROLLRIGHTEDGE /* N.A. */},
   { XtNscrollTopKey,	   "a<Prior>",	     OL_SCROLLTOP	/* N.A. */},
   { XtNscrollUpKey,	   "c<bracketright>",OL_SCROLLUP	/* N.A. */},

/* text navigation, they were located in Text DB originally.	*/
/* Bring over here because Motif SB needs them...		*/

   { XtNdocEndKey,	"c<End>",	OL_DOCEND	/* OLM_KEndData */},
   { XtNdocStartKey,	"c<Home>",	OL_DOCSTART	/* OLM_KBeginData */},
   { XtNlineEndKey,	"n<End>",	OL_LINEEND	/* OLM_KEndLine */},
   { XtNlineStartKey,	"n<Home>",	OL_LINESTART	/* OLM_KBeginLine */},

/* Motif bindings that do have equivalent O/L bindings */

   { XtNclearKey,	"<F8>",		OLM_KClear	/* N.A. */},
   { XtNdeselectAllKey,	"c<backslash>",	OLM_KDeselectAll/* N.A. */},
   { XtNnextPaneKey,	"<F6>",		OLM_KNextPane	/* N.A. */},
   { XtNprevPaneKey,	"s<F6>",	OLM_KPrevPane	/* N.A. */},
   { XtNreselectKey,	"c s<space>",	OLM_KReselect	/* N.A. */},
   { XtNrestoreKey,	"c s<Insert>",	OLM_KRestore	/* N.A. */},
   { XtNselectAllKey,	"c<slash>",	OLM_KSelectAll	/* N.A. */},
};

static OlKeyBinding OlTextKeyBindings[] = {

/* text */

   { XtNdelCharBakKey,	"<BackSpace>",	 OL_DELCHARBAK	/*OLM_KBackSpace */},
   { XtNdelCharFwdKey,	"<Delete>",	 OL_DELCHARFWD	/*OLM_KDelete */},
   { XtNdelLineKey,	"a s<Delete>",	 OL_DELLINE	/*N.A. */},
   { XtNdelLineBakKey,	"c<BackSpace>",	 OL_DELLINEBAK	/*N.A. */},
   { XtNdelLineFwdKey,	"c<Delete>",	 OL_DELLINEFWD	/*OLM_KEraseEndLine */},
   { XtNdelWordBakKey,	"c s<BackSpace>",OL_DELWORDBAK	/*N.A. */},
   { XtNdelWordFwdKey,	"c s<Delete>",	 OL_DELWORDFWD	/*N.A. */},

/* text navigation */

   { XtNpaneEndKey,	"c s<End>",	 OL_PANEEND	/* N.A. */},
   { XtNpaneStartKey,	"c s<Home>",	 OL_PANESTART	/* N.A. */},
   { XtNwordBakKey,	"c<Left>",	 OL_WORDBAK	/* OLM_KPrevWord */},
   { XtNwordFwdKey,	"c<Right>",	 OL_WORDFWD	/* OLM_KNextWord */},

/* text backward compatibilities */

   { XtNcharBakKey,	"<Left>",	 OL_CHARBAK	/* N.A. */},
   { XtNcharFwdKey,	"<Right>",	 OL_CHARFWD	/* N.A. */},
   { XtNrowDownKey,	"<Down>",	 OL_ROWDOWN	/* N.A. */},
   { XtNrowUpKey,	"<Up>",		 OL_ROWUP	/* N.A. */},
   { XtNreturnKey,	"<Return>",	 OL_RETURN	/* OLM_KEnter */},

/* Motif bindings that do have equivalent O/L bindings */

   { XtNaddModeKey,	"s<F8>",	    OLM_KAddMode	/* N.A. */},
   { XtNnextParaKey,	"c<Down>",	    OLM_KNextPara	/* N.A. */},
   { XtNprevParaKey,	"c<Up>",	    OLM_KPrevPara	/* N.A. */},
   { XtNprimaryCopyKey,	"a c<Insert>,c<F11>",OLM_KPrimaryCopy	/* N.A. */},
   { XtNprimaryCutKey,	"a<F11>",	    OLM_KPrimaryCut	/* N.A. */},
   { XtNprimaryPasteKey,"<F11>",	    OLM_KPrimaryPaste	/* N.A. */},
   { XtNquickCopyKey,	"c<F12>",	    OLM_KQuickCopy	/* N.A. */},
   { XtNquickCutKey,	"a<F12>",	    OLM_KQuickCut	/* N.A. */},
   { XtNquickExtendKey,	"s<F12>",	    OLM_KQuickExtend	/* N.A. */},
   { XtNquickPasteKey,	"<F12>",	    OLM_KQuickPaste	/* N.A. */},
};

	/* OPEN LOOK Mouse Bindings...	*/
static OlBtnBinding OlCoreBtnBindings[] = {
#ifdef ThreeMouseButton
   { XtNselectBtn,	"n<Button1>",	OL_SELECT	  /* OLM_BSelect */},
   { XtNpasteBtn,	"n<Button2>",	OLM_BPrimaryPaste /* N.A.        */
				     /* OLM_BQuickPaste	   * N.A.        */
				     /* OLM_BDrag	   * N.A.        */},
   { XtNmenuBtn,	"n<Button3>",	OL_MENU		  /* OLM_BMenu   */},
   { XtNadjustBtn,	"c<Button1>",	OL_ADJUST	  /* OLM_BToggle */},
   { XtNconstrainBtn,	"a s<Button1>",	OL_CONSTRAIN	  /* N.A.        */},
   { XtNduplicateBtn,	"a<Button1>",	OL_DUPLICATE	  /* N.A.        */},
   { XtNlinkBtn,	"a c<Button1>",	OL_LINK		  /* N.A.        */},
   { XtNmenuDefaultBtn,	"a<Button3>",	OL_MENUDEFAULT	  /* N.A.        */},
   { XtNpanBtn,		"s<Button2>",	OL_PAN		  /* N.A.        */},
   { XtNcopyBtn,	"c<Button2>",	OLM_BPrimaryCopy  /* N.A.        */
				     /* OLM_BQuickCopy	   * N.A.        */},
   { XtNcutBtn,		"a<Button2>",	OLM_BPrimaryCut   /* N.A.        */
				     /* OLM_BQuickCut	   * N.A.        */},
   { XtNextendBtn,	"s<Button1>",	OLM_BExtend	  /* N.A.        */},
#else /* TwoMouseButton */
   { XtNselectBtn,	"n<Button1>",	OL_SELECT	  /* OLM_BSelect */},
   { XtNpasteBtn,	"n<Button2>",	OLM_BPrimaryPaste /* N.A.        */
				     /* OLM_BQuickPaste	   * N.A.        */
				     /* OLM_BDrag	   * N.A.        */},
   { XtNadjustBtn,	"c<Button1>",	OL_ADJUST	  /* OLM_BToggle */},
   { XtNmenuBtn,	"c s<Button1>",	OL_MENU		  /* OLM_BMenu   */},
   { XtNconstrainBtn,	"a s<Button1>",	OL_CONSTRAIN	  /* N.A.        */},
   { XtNduplicateBtn,	"a<Button1>",	OL_DUPLICATE	  /* N.A.        */},
   { XtNlinkBtn,	"a c<Button1>",	OL_LINK		  /* N.A.        */},
   { XtNmenuDefaultBtn, "a c s<Button1>",OL_MENUDEFAULT	  /* N.A.        */},
   { XtNpanBtn,		"s<Button2>",	OL_PAN		  /* N.A.        */},
   { XtNcopyBtn,	"c<Button2>",	OLM_BPrimaryCopy  /* N.A.        */
				     /* OLM_BQuickCopy	   * N.A.        */},
   { XtNcutBtn,		"a<Button2>",	OLM_BPrimaryCut   /* N.A.        */
				     /* OLM_BQuickCut	   * N.A.        */},
   { XtNextendBtn,	"s<Button1>",	OLM_BExtend	  /* N.A.        */},
#endif
};

#endif /* __DynamicI_h__ */
