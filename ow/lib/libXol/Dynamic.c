#pragma ident	"@(#)Dynamic.c	302.36	94/02/03 lib/libXol SMI"	/* olmisc:Dynamic.c 1.69	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <libintl.h>
#include <widec.h>
#include <wctype.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MTSAFE
#include <thread.h>
#endif

#include <Xol/Converters.h>
#include <Xol/DynamicP.h>
#include <Xol/TextEditP.h>
#include <Xol/TextLineP.h>
#include <Xol/Flat.h>
#include <Xol/OpenLookI.h>
#include <Xol/OpenLookP.h>
#include <Xol/ParseAccI.h>
#include <Xol/RootShellP.h>
#include <Xol/Util.h>
#include <Xol/memutil.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlIm.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XSunExt.h>

extern int	_XDefaultError(Display* dpy, XErrorEvent* event);
					/* From lib/libX11/XlibInt.c */


/*
 *	private tables
 */

static ModifierInfo mod_info[] = {
   {Alt,   "Alt",   XK_Alt_L,    XK_Alt_R,   (KeyCode)0, (KeyCode)0,    None },
   {Meta,  "Meta",  XK_Meta_L,   XK_Meta_R,  (KeyCode)0, (KeyCode)0,    None },
   {Hyper, "Hyper", XK_Hyper_L,  XK_Hyper_R, (KeyCode)0, (KeyCode)0,    None },
   {Super, "Super", XK_Super_L,  XK_Super_R, (KeyCode)0, (KeyCode)0,    None },
   {NumLock, "NumLock", XK_Num_Lock, XK_Num_Lock, (KeyCode)0, (KeyCode)0,None },
   {ModeSwitch, "ModeSwitch", XK_Mode_switch, XK_Mode_switch, 
						(KeyCode)0, (KeyCode)0,None}
};

static KeypadInfo keypad_table[] = {
    { XK_KP_0, NULL, 0 },
    { XK_KP_1, NULL, 0 },
    { XK_KP_2, NULL, 0 },
    { XK_KP_3, NULL, 0 },
    { XK_KP_4, NULL, 0 },
    { XK_KP_5, NULL, 0 },
    { XK_KP_6, NULL, 0 },
    { XK_KP_7, NULL, 0 },
    { XK_KP_8, NULL, 0 },
    { XK_KP_9, NULL, 0 },
    { XK_KP_Equal, NULL, 0 },
    { XK_KP_Multiply, NULL, 0 },
    { XK_KP_Add, NULL, 0 },
    { XK_KP_Separator, NULL, 0 },
    { XK_KP_Subtract, NULL, 0 },
    { XK_KP_Decimal, NULL, 0 },
    { XK_KP_Divide, NULL, 0 },
    { XK_KP_Space, NULL, 0 },
    { XK_KP_Tab, NULL, 0 },
    { XK_KP_Enter, NULL, 0 },
    { XK_KP_F1, NULL, 0 },
    { XK_KP_F2, NULL, 0 },
    { XK_KP_F3, NULL, 0 },
    { XK_KP_F4, NULL, 0 },
};

static btn_mapping btn_mappings[] = {
   { Button1, Button1Mask },
   { Button2, Button2Mask },
   { Button3, Button3Mask },
   { Button4, Button4Mask },
   { Button5, Button5Mask },
};

/*
 * The modifier masks for Alt/Meta/Super/Hyper are initialized here to
 * Mod1Mask (an arbitrary choice). These masks are fixed up to the actual
 * server-dependent mask when _OlGetModifierMapping() is called from
 * OlPostInitialize().
 * I've re-ordered the entries in this table by the frequency they occur
 * in the default (Sun) bindings so on average a match will be found quicker.
 * 	--- jsc
 */

static mapping mappings[] = {
   { "c",          ControlMask   },
   { "a",          Mod1Mask      },
   { "s",          ShiftMask     },
   { "n",          None          },
   { "m",          Mod1Mask      },
   { "Button1",    Button1       },
   { "Button3",    Button3       },
   { "Button2",    Button2       },
   { "Ctrl",       ControlMask   },
   { "Alt",        Mod1Mask      },
   { "Shift",      ShiftMask     },
   { "None",       None          },
   { "Lock",       LockMask      },
   { "l",          LockMask      },
   { "Meta",       Mod1Mask      },
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
   { "Button4",    Button4       },
   { "Button5",    Button5       },
   { "ModeSwitch", Mod1Mask      }
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

   { XtNselectKey,       "n<space>",  	      OL_SELECTKEY	},
   { XtNmenuKey,         "a<space>",          OL_MENUKEY	},
   { XtNmenuDefaultKey,  "c<space>",          OL_MENUDEFAULTKEY	},
   { XtNhorizSBMenuKey,  "a<h>",              OL_HSBMENU	},
   { XtNvertSBMenuKey,   "a<v>",              OL_VSBMENU	},
   { XtNadjustKey,       "a<Insert>",         OL_ADJUSTKEY	},

/* olwm + olwsm */
   { XtNworkspaceMenuKey,"a s<m>",            OL_WORKSPACEMENU	},
   { XtNnextAppKey,      "a<n>",              OL_NEXTAPP	},
   { XtNnextWinKey,      "a<w>",              OL_NEXTWINDOW	},
   { XtNprevAppKey,      "a s<n>",            OL_PREVAPP	},
   { XtNprevWinKey,      "a s<w>",            OL_PREVWINDOW	},
   { XtNwindowMenuKey,   "a<m>",              OL_WINDOWMENU	},

/* traversal */

   { XtNdownKey,         "<Down>",            OL_MOVEDOWN	},
   { XtNleftKey,         "<Left>",            OL_MOVELEFT	},
   { XtNmultiDownKey,    "c<Down>",           OL_MULTIDOWN	},
   { XtNmultiLeftKey,    "c<Left>",           OL_MULTILEFT	},
   { XtNmultiRightKey,   "c<Right>",          OL_MULTIRIGHT	},
   { XtNmultiUpKey,      "c<Up>",             OL_MULTIUP	},
   { XtNnextFieldKey,    "n<Tab>,c<Tab>",     OL_NEXTFIELD	},
   { XtNprevFieldKey,    "s<Tab>,c s<Tab>",   OL_PREVFIELD	},
   { XtNrightKey,        "<Right>",           OL_MOVERIGHT	},
   { XtNupKey,           "<Up>",              OL_MOVEUP		},

/* selection */

   { XtNcopyKey,         "n<F16>, m<c>",      OL_COPY		},
   { XtNcutKey,          "n<F20>, m<x>",      OL_CUT		},
   { XtNpasteKey,        "n<F18>, c<y>",      OL_PASTE		},
   { XtNpasteKey,        "m<v>",      	      OL_PASTE		},
   { XtNselCharBakKey,   "s<Left>, s c<b>",   OL_SELCHARBAK	},
   { XtNselCharFwdKey,   "s<Right>, s c<f>",  OL_SELCHARFWD	},
   { XtNselLineKey,      "c a<Left>",         OL_SELLINE	},
   { XtNselLineBakKey,   "s<R7>, s c<p>",     OL_SELLINEBAK	},
   { XtNselLineBakKey,   "s<Home>",           OL_SELLINEBAK	},
   { XtNselLineFwdKey,   "s<R13>, s c<n>",    OL_SELLINEFWD	},
   { XtNselLineFwdKey,   "s<End>",            OL_SELLINEFWD	},
   { XtNselWordBakKey,   "c s<Left>",         OL_SELWORDBAK	},
   { XtNselWordFwdKey,   "c s<Right>",        OL_SELWORDFWD	},

/* misc */

   { XtNcancelKey,       "n<Escape>",         OL_CANCEL		},
   { XtNdefaultActionKey,"<Return>,c<Return>",OL_DEFAULTACTION	},

   { XtNhelpKey,         "n<Help>",           OL_HELP		},
   { XtNpropertiesKey,   "n<F13>",            OL_PROPERTY	},
   { XtNstopKey,         "n<F11>",            OL_STOP		},
   { XtNtogglePushpinKey,"c<t>",              OL_TOGGLEPUSHPIN	},
   { XtNundoKey,         "n<F14>",            OL_UNDO		},
   { XtNfindKey,         "n<F19>, m<f>",      OL_FIND		},
   { XtNagainKey,        "n<F12>",            OL_AGAIN		},

/* scrolling */

   { XtNpageDownKey,     "a<R15>, a<Next>",   OL_PAGEDOWN	},
   { XtNpageLeftKey,     "a c<R9>,a c<Prior>", OL_PAGELEFT	},
   { XtNpageRightKey,    "a c<R15>,a c<Next>", OL_PAGERIGHT	},
   { XtNpageUpKey,       "a<R9>, a<Prior>",   OL_PAGEUP		},
   { XtNscrollBottomKey, "a c<R13>,a c<End>", OL_SCROLLBOTTOM	},
   { XtNscrollDownKey,   "a<Down>",           OL_SCROLLDOWN	},
   { XtNscrollLeftKey,   "a<Left>",           OL_SCROLLLEFT	},
   { XtNscrollLeftEdgeKey,"a<R7>, a<Home>",   OL_SCROLLLEFTEDGE	},
   { XtNscrollRightKey,  "a<Right>",          OL_SCROLLRIGHT	},
   { XtNscrollRightEdgeKey,"a<R13>, a<End>",  OL_SCROLLRIGHTEDGE},
   { XtNscrollTopKey,    "a c<R7>,a c<Home>", OL_SCROLLTOP	},
   { XtNscrollUpKey,     "a<Up>",             OL_SCROLLUP	},
};

static OlKeyBinding OlTextKeyBindings[] = {

/* text */

   { XtNdelCharBakKey,   "<BackSpace>,<Delete>", OL_DELCHARBAK	},
   { XtNdelCharFwdKey,   "s<BackSpace>,c<d>", OL_DELCHARFWD },
   { XtNdelLineKey,      "m<BackSpace>,m<Delete>", OL_DELLINE   },
   { XtNdelLineBakKey,   "c<BackSpace>,c<u>", OL_DELLINEBAK	},
   { XtNdelLineFwdKey,   "c<Delete>,c<k>",    OL_DELLINEFWD	},
   { XtNdelWordBakKey,   "c s<BackSpace>,c<w>", OL_DELWORDBAK	},
   { XtNdelWordFwdKey,   "c s<Delete>",       OL_DELWORDFWD	},

/* text navigation */

   { XtNdocEndKey,       "c<R13>, c<End>",    OL_DOCEND		},
   { XtNdocStartKey,     "c<R7>, c<Home>",    OL_DOCSTART	},
   { XtNlineEndKey,      "n<R13>, c<e>",      OL_LINEEND	},
   { XtNlineEndKey,      "n<End>",            OL_LINEEND	},
   { XtNlineStartKey,    "n<R7>, c<a>",       OL_LINESTART	},
   { XtNlineStartKey,    "n<Home>",           OL_LINESTART	},
   { XtNpaneEndKey,      "c s<R13>, c s<End>", OL_PANEEND	},
   { XtNpaneStartKey,    "c s<R7>,c s<Home>", OL_PANESTART	},
   { XtNwordBakKey,      "c<Left>",           OL_WORDBAK	},
   { XtNwordFwdKey,      "c<Right>",          OL_WORDFWD	},

/* text backward compatibilities */

   { XtNcharBakKey,      "<Left>, c<b>",      OL_CHARBAK	},
   { XtNcharFwdKey,      "<Right>, c<f>",     OL_CHARFWD	},
   { XtNrowDownKey,      "<Down>, c<n>",      OL_ROWDOWN	},
   { XtNrowUpKey,        "<Up>, c<p>",        OL_ROWUP		},
   { XtNreturnKey,       "<Return>, <KP_Enter>", OL_RETURN	},

};

static OlBtnBinding OlCore3BtnBindings[] = {
   { XtNselectBtn,       "n<Button1>",        OL_SELECT      },
   { XtNadjustBtn,       "n<Button2>",        OL_ADJUST      },
   { XtNmenuBtn,         "n<Button3>",        OL_MENU        },
   { XtNconstrainBtn,    "s<Button1>",        OL_CONSTRAIN   },
   { XtNduplicateBtn,    "c<Button1>",        OL_DUPLICATE   },
   { XtNpanBtn,          "c s<Button1>",      OL_PAN         },
   { XtNmenuDefaultBtn,  "c<Button3>",        OL_MENUDEFAULT },
   { NULL,		 "",                  NULL }
};

static OlBtnBinding OlCore2BtnBindings[] = {
   { XtNselectBtn,       "n<Button1>",        OL_SELECT      },
   { XtNadjustBtn,       "s<Button1>",        OL_ADJUST      },
   { XtNmenuBtn,         "n<Button2>",        OL_MENU        },
   { XtNconstrainBtn,    NULL,        	      OL_CONSTRAIN   },
   { XtNduplicateBtn,    "c<Button1>",        OL_DUPLICATE   },
   { XtNpanBtn,          "c s<Button1>",      OL_PAN         },
   { XtNmenuDefaultBtn,  "c<Button2>",        OL_MENUDEFAULT },
   { NULL,		 "",                  NULL }
};

static OlBtnBinding OlCore1BtnBindings[] = {
   { XtNselectBtn,       "n<Button1>",        OL_SELECT      },
   { XtNadjustBtn,       "s<Button1>",        OL_ADJUST      },
   { XtNmenuBtn,         "a<Button1>",        OL_MENU        },
   { XtNconstrainBtn,    NULL,        	      OL_CONSTRAIN   },
   { XtNduplicateBtn,    "c<Button1>",        OL_DUPLICATE   },
   { XtNpanBtn,          NULL,      	      OL_PAN         },
   { XtNmenuDefaultBtn,  "s c<Button1>",      OL_MENUDEFAULT },
   { NULL,		 "",                  NULL }
};

static OlBtnBinding *OlCoreBtnBindings;

/*
 *************************************************************************
 *
 * Forward Procedure Declarations
 *
 **************************forward*declarations***************************
 */

static XContext		ComposeContextType (void);
static void		FreeComposeData (Widget, XtPointer, XtPointer);

static int		BinarySearch (KeySym, Modifiers, Token *,
					      int, OlKeyBinding *);
static void		BuildIEDBstack (Widget, XtPointer);
static int		CompareFunc (const void *, const void *);
static int		CompareUtil (KeySym, Modifiers,
					     KeySym, Modifiers);
static OlInputEvent	DoLookup (Widget, XEvent *, KeySym *,
				  String *, Cardinal *, Boolean, XtPointer);
static OlInputEvent	DoLookupForTextEdit (Widget, XEvent *, KeySym *,
				  String *, Cardinal *, Boolean, XtPointer);
void			DynamicHandler (Widget);
static XtPointer	GetMoreMem (XtPointer, int, int,
					    short *, short *);
static void		InitSortedKeyDB (OlVirtualEventTable);
static Boolean		IsComposeBtnOrKey (PerDisplayVEDBInfoPtr,
						   Boolean, String,
						   int *, int *);
static Boolean		IsKPKey (Display *, KeyCode, KeySym *);
static Bool		IsSameKeyEvent (Display *, XEvent *, char *);
static void		_OlChangeMapping (String, Modifiers);
static String		_OlCheckModifierHasName (Display *, Modifiers);
static void		_OlFixupMappings (ModifierType, Modifiers);
static void		_OlGetDontCareBits (Display *);
static void		_OlGetKeypadKeycodes (Display *);
static int		_OlTranslateKPKeySym ( KeySym, char *, int);

static Boolean		ParseKeysymList (String * , Modifiers *,
						 String *);
static void		StringToKeyDefOrBtnDef (Display * , Boolean,
							String , XtPointer);
static void		UpdateIEDB (Widget, OlVirtualEventTable);
static OlVirtualName	WhatOlBtn (Display *,
					   unsigned int, unsigned int);
static OlVirtualName	WhatOlKey (Widget, XEvent *, Boolean);

static Boolean          VETsEqual(const OlVirtualEventTable,
                                  const OlVirtualEventTable
                        );


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

	/* Dynamic change in progress flag */
Boolean				_OlDynResProcessing = False;

static Modifiers		dont_care_bits;

static int               	num_dynamic_callbacks;
static DynamicCallback * 	dynamic_callbacks;
static char              	buffer_return[BUFFER_SIZE];

			/* set from InitSortedKeyDB and use by CompareFunc */
static OlKeyBinding * 		current_key_bindings_to_sort = NULL;

static	PerDisplayVEDBInfoPtr	_AllocPDVEDBInfo ( Display *);

static	struct	{
	unsigned int		num;
	PerDisplayVEDBInfoPtr	*dbs;
} dpy_dbs = { (unsigned int)0, (PerDisplayVEDBInfoPtr *)NULL };

static	OlClassSearchInfo       wc_list = (OlClassSearchInfo)NULL;
static	short                   wc_list_slots_left = 0;
static	short                   wc_list_slots_alloced = 0;
static	short                   wc_list_entries = 0;

static OlVirtualEventTable
_CopyVEDBToDpy (OlVirtualEventTable db, Display *dpy)
{
	PerDisplayVEDBInfoPtr	dst_ve_db;
	OlVirtualEventTable	ret_db = (OlVirtualEventTable)NULL;

	if (db == (OlVirtualEventTable)NULL) return (db);

	if (db->dpy == dpy) {
		db->refcnt++;
		return (db);
	}

	dst_ve_db = _AllocPDVEDBInfo(dpy);

	if (dst_ve_db->appl_shell == (Widget)NULL) return (ret_db);

	ret_db = (OlVirtualEventTable)XtCalloc(1, sizeof(OlVirtualEventInfo));

	ret_db->dpy  = dpy;

	ret_db->db_type = db->db_type;

	ret_db->refcnt = 1;

	if (db->num_key_bindings > 0) {
		ret_db->key_bindings = (OlKeyBinding *)
			XtCalloc(db->num_key_bindings, sizeof(OlKeyBinding));

		memcpy((XtPointer)ret_db->key_bindings,
		       (XtPointer)db->key_bindings,
		       db->num_key_bindings * sizeof(OlKeyBinding));
	} else
		ret_db->btn_bindings = (OlBtnBinding *)NULL;

	if (db->num_btn_bindings > 0) {
		ret_db->btn_bindings = (OlBtnBinding *)
			XtCalloc(db->num_btn_bindings, sizeof(OlBtnBinding));

		memcpy((XtPointer)ret_db->btn_bindings,
		       (XtPointer)db->btn_bindings,
		       db->num_btn_bindings * sizeof(OlKeyBinding));
	} else
		ret_db->btn_bindings = (OlBtnBinding *)NULL;

	ret_db->num_key_bindings = db->num_key_bindings;
	ret_db->num_btn_bindings = db->num_btn_bindings;

	UpdateIEDB(dst_ve_db->appl_shell, ret_db);

	if (IsCoreDB(db->db_type) &&
	    dst_ve_db->OlCoreDB == (OlVirtualEventTable)NULL) {
                dst_ve_db->OlCoreDB = ret_db;
		ret_db->refcnt++;
	}
 
        if (IsTextDB(db->db_type) &&
	    dst_ve_db->OlTextDB == (OlVirtualEventTable)NULL) {
                dst_ve_db->OlTextDB = ret_db;
		ret_db->refcnt++;
	}
 
        dst_ve_db->avail_dbs =
                (OlVirtualEventTable *) GetMoreMem(
                                        (XtPointer) dst_ve_db->avail_dbs,
                                        (int) sizeof(OlVirtualEventTable *),
                                        MORESLOTS,
                                        &dst_ve_db->avail_dbs_slots_left,
                                        &dst_ve_db->avail_dbs_slots_alloced);
 
        dst_ve_db->avail_dbs [dst_ve_db->avail_dbs_entries++] = ret_db;
        dst_ve_db->avail_dbs_slots_left--;
	ret_db->refcnt++;

	return(ret_db);
}


static void
_FreePDVEDBInfo (Display *dpy)
{
	register int 			i;
	register short			p;
	register PerDisplayVEDBInfoPtr	ve_db = (PerDisplayVEDBInfoPtr)NULL;

#undef	VECTOR_INCR
#define	VECTOR_INCR	5

#define	FREEDB(db)						  \
	XtFree((char *)(db)->key_bindings);			  \
	XtFree((char *)(db)->btn_bindings);			  \
	XtFree((char *)(db)->sorted_key_db);			  \
	XtFree((char *)(db))

	for (i = 0; i < dpy_dbs.num; i++) {
		 if ((ve_db = dpy_dbs.dbs[i])->dpy == dpy) break;
	}

	if (ve_db == (PerDisplayVEDBInfoPtr)NULL) return;	/* no match */

	XtFree((char *)ve_db->db_stack);
	for (p = 0; p < ve_db->avail_dbs_entries; p++) {
		if(ve_db->avail_dbs[p] != (OlVirtualEventTable)NULL &&
		   --(ve_db->avail_dbs[p]->refcnt) <= 0) {
			FREEDB(ve_db->avail_dbs[p]);
		}
	}

	XtFree((char *)ve_db->avail_dbs);

	if (ve_db->OlCoreDB != (OlVirtualEventTable)NULL &&
	    --(ve_db->OlCoreDB->refcnt) <= 0) {
		FREEDB(ve_db->OlCoreDB);
	}
	if (ve_db->OlTextDB != (OlVirtualEventTable)NULL &&
	    --(ve_db->OlTextDB->refcnt) <= 0) {
		FREEDB(ve_db->OlTextDB);
	}

	for (p = 0; p < ve_db->wid_list_entries; p++) {
		if (ve_db->wid_list[p].db != (OlVirtualEventTable)NULL &&
		    --(ve_db->wid_list[p].db->refcnt) <= 0) {
			FREEDB(ve_db->wid_list[p].db);
		}
	}

	XtFree((char *)ve_db->wid_list);

	for (p = 0; p < ve_db->wc_list_entries; p++) {
		if (ve_db->wc_list[p].db != (OlVirtualEventTable)NULL &&
		    --(ve_db->wc_list[p].db->refcnt) <= 0) {
			FREEDB(ve_db->wc_list[p].db);
		}
	}

	XtFree((char *)ve_db->wc_list);

	XtFree((char *)ve_db);
	
	/* phew .... talk about fragged! */

	dpy_dbs.num--;

	if (i < dpy_dbs.num) {
		memcpy((XtPointer)(dpy_dbs.dbs + i),
		       (XtPointer)(dpy_dbs.dbs + i + 1),
		       sizeof(PerDisplayVEDBInfoPtr *));
	}

	dpy_dbs.dbs[dpy_dbs.num] = (PerDisplayVEDBInfoPtr)NULL;

	if (dpy_dbs.num == 0) {
		XtFree((char *)dpy_dbs.dbs);
		dpy_dbs.dbs = (PerDisplayVEDBInfoPtr *)NULL;
	}

	for (i = 0; i < wc_list_entries; i++) {
		if (wc_list[i].db->dpy == dpy) {
			wc_list[i].db->dpy     = (Display *)NULL;
			wc_list[i].db->db_type = UnboundDB;
		}
	}
}

void
_OlCleanupPDVEDB (Display *dpy)
{
	_FreePDVEDBInfo(dpy);
}

static PerDisplayVEDBInfoPtr
_AllocPDVEDBInfo (Display *dpy)
{
	register int 			i;
	register PerDisplayVEDBInfoPtr	ve_db = (PerDisplayVEDBInfoPtr)NULL;
	Widget				dsw,
					appl_shell;
	Arg				args[1];

	if (dpy == (Display *)NULL) dpy = toplevelDisplay;


	if (dpy == (Display *)NULL) return(ve_db);

	for (i = 0; i < dpy_dbs.num; i++) {
		if (dpy_dbs.dbs[i]->dpy == dpy) { 
			ve_db = dpy_dbs.dbs[i];
			break;
		}
	}

	if (ve_db == (PerDisplayVEDBInfoPtr)NULL) { /* not found */
#define	SizeOfModInfo	 sizeof(mod_info)
#define	SizeOfKPInfo	 sizeof(keypad_table)
#define	SizeOfPDVEDBInfo (sizeof(PerDisplayVEDBInfo) + SizeOfModInfo + SizeOfKPInfo)

		ve_db = (PerDisplayVEDBInfoPtr) XtCalloc(1, SizeOfPDVEDBInfo);
		ve_db->doing_copy = False;

		ve_db->dpy          = dpy;
		ve_db->mod_info     = (ModifierInfo *)((char *)ve_db +
					       sizeof(PerDisplayVEDBInfo));
		ve_db->keypad_table = (KeypadInfo  *)((char *)(ve_db->mod_info)
							+ SizeOfModInfo);

		memcpy((XtPointer)ve_db->mod_info, (const XtPointer)mod_info, 
			SizeOfModInfo);
		memcpy((XtPointer)ve_db->keypad_table,
			(const XtPointer)keypad_table,
			SizeOfKPInfo);

		if (VECTORFULL(dpy_dbs.num, VECTOR_INCR) || dpy_dbs.num == 0) {
			int len = sizeof(PerDisplayVEDBInfoPtr) *
				  (dpy_dbs.num + VECTOR_INCR);

			dpy_dbs.dbs = (PerDisplayVEDBInfoPtr *)
					XtRealloc( (char *)dpy_dbs.dbs, len);
		}

		dpy_dbs.num++;
		dpy_dbs.dbs[i] = ve_db;

		_OlGetModifierMapping(dpy);
	}

	/* duplicate wc search info - when we can successfully update IEDB */


	if (!ve_db->doing_copy && wc_list_entries > ve_db->wc_list_entries) {
		int	current_wc_list_entries = ve_db->wc_list_entries;

		if (ve_db->appl_shell == (Widget)NULL) {
			dsw = _OlGetDisplayShellOfScreen(
						DefaultScreenOfDisplay(dpy));

			if (dsw == (Widget)NULL) return (ve_db);

			XtSetArg(args[0], XtNapplShell, &appl_shell);
			XtGetValues(dsw, args, XtNumber(args));

			if ((ve_db->appl_shell = appl_shell) == (Widget)NULL)
				return (ve_db);
		}

		for (i = wc_list_entries; i > current_wc_list_entries;) {
			while (wc_list_entries - ve_db->wc_list_entries >
			       ve_db->wc_list_slots_left) {
				short save_left = ve_db->wc_list_slots_left;

				ve_db->wc_list_slots_left = 0;

				ve_db->wc_list = (OlClassSearchInfo)
					GetMoreMem(
						(XtPointer) ve_db->wc_list,
						(int) sizeof(OlClassSearchRec),
						MORESLOTS,
						&ve_db->wc_list_slots_left,
						&ve_db->wc_list_slots_alloced);

				ve_db->wc_list_slots_left += save_left;
			}

			ve_db->wc_list [--i].wc = wc_list[i].wc;

			ve_db->doing_copy = True; /* lock out recursion */
			ve_db->wc_list [i].db = _CopyVEDBToDpy(wc_list[i].db,
							       ve_db->dpy);
			ve_db->doing_copy = False; /* unlock */
			ve_db->wc_list_entries++;
			ve_db->wc_list_slots_left--;
		}
	}
	return (ve_db);
}

/*
 * LookupOlInputEvent
 *
 * The \fILookupOlInputEvent\fR function is used to decode the \fIevent\fR
 * for widget \fIw\fR to an \fIOlInputEvent\fR.  The event passed should
 * be a ButtonPress, ButtonRelease, or KeyPress event.  The function attempts
 * to decode this event based on the settings of the OPEN LOOK(tm) defined
 * dynamic mouse and keyboard settings.
 *
 * If the event is a KeyPress, the function may return the \fIkeysym\fR,
 * \fIbuffer\fR, and/or \fIlength\fR of the buffer returned from a call to
 * XLookupString(3X).  It returns these values if non-NULL values are 
 * provided by the caller.
 *
 * See also:
 *
 * OlReplayBtnEvent(3), OlDetermineMouseAction(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */
OlInputEvent
LookupOlInputEvent (Widget w, XEvent *event, KeySym *keysym, char **buffer, int *length)
{
	OlInputEvent retval;

	GetToken();
	retval =  (DoLookup(
		w, event, keysym, (String *)buffer,
				(Cardinal *) length, True, OL_DEFAULT_IE));
	ReleaseToken();
	return retval;
} /* end of LookupOlInputEvent */

/*
 * OlCallDynamicCallbacks
 *
 * The \fIOlCallDynamicCallbacks\fR procedure is used to trigger
 * the calling of the functions registered on the dynamic callback list.
 * This procedure is called automatically whenever the RESOURCE_MANAGER
 * property of the Root Window is updated.  It may also
 * be called to force a synchronization of the dynamic settings.
 *
 * See also:
 *
 * OlRegisterDynamicCallback(3), OlUnregisterDynamicCallback(3),
 * OlGetApplicationResources(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */
void
OlCallDynamicCallbacks (void)
{
	register int	i;

	GetToken();
	for (i = 0; i < num_dynamic_callbacks; i++)
   		(*dynamic_callbacks[i].CB)(dynamic_callbacks[i].data);
	ReleaseToken();
} /* end of OlCallDynamicCallbacks */

/**
 ** _OlCanonicalKeysym()
 **/
KeySym
_OlCanonicalKeysym (Display *display, KeySym keysym, KeyCode *p_keycode, Modifiers *p_modifiers)
{
	KeySym *		syms;

	KeyCode			keycode;
	KeyCode			foo_min_keycode;

	Modifiers		modifiers;

	int			per;
	int			min_keycode;
	int			max_keycode;


	/*
	 * This routine reduces a keysym to a canonical (or consistent)
	 * form. Generally this means converting a shifted-keysym into
	 * its unshifted form and ORing the ShiftMask into the modifiers
	 * mask. Thus, for example:
	 *
	 *	<M>	->	Shift<m>
	 *	<F14>	->	Shift<F2>
	 *
	 * However, certain shifted-keysyms look better the way they
	 * are, and we leave them alone:
	 *
	 *	<!>, not Shift<1>
	 *	<">, not Shift<'>
	 *	etc.
	 *
	 * (I18N)
	 */


	/*
	 * Grumble...the Intrinsics don't give us "max_keycode",
	 * so we have to beg Xlib for it. Fortunately, this doesn't
	 * involve a server request.
	 *
	 * Arrgh...and to top it off, Xt wants a (KeyCode *) while
	 * Xlib wants an (int *), for the same value ("min_keycode").
	 */
	syms = XtGetKeysymTable(display, &foo_min_keycode, &per);
	XDisplayKeycodes (display, &min_keycode, &max_keycode);

	modifiers = (p_modifiers? *p_modifiers : 0);
	for (
		keycode = (KeyCode)min_keycode;
		keycode <= (KeyCode)max_keycode;
		keycode++, syms += per
	) {
		if (per > 1 && keysym == syms[1]) {
			if (!isascii(keysym) || isalpha(keysym)) {
				if (syms[0] != syms[1]) {
					modifiers |= ShiftMask;
					keysym = syms[0];
				}
			} else if (isascii(keysym) && !isalpha(keysym)) {
				modifiers &= ~ShiftMask;
			}
			break;
		} else if (per == 1 || syms[1] == NoSymbol) {
			KeySym			lower;
			KeySym			upper;

			XtConvertCase (display, syms[0], &lower, &upper);
			if (keysym == lower)
				break;
			else if (keysym == upper) {
				if (!isascii(keysym) || isalpha(keysym)) {
					modifiers |= ShiftMask;
					keysym = lower;
				} else if (isascii(keysym) && !isalpha(keysym))
				{
					modifiers &= ~ShiftMask;
				}
				break;
			}
		} else if (keysym == syms[0]) {
		    if (per > 1 && (modifiers & ShiftMask) &&
			isascii(keysym) && !isalpha(keysym) &&
			syms[1] != NoSymbol && syms[0] != syms[1]) {
			keysym = syms[1];
			modifiers &= ~ShiftMask;
		    }
		    break;
		}
	}

	if (p_keycode)
		*p_keycode = (keycode <= (KeyCode)max_keycode? keycode : 0);
	if (p_modifiers)
		*p_modifiers = modifiers;

	return (keysym);
} /* end of _OlCanonicalKeysym */

/*
** This routine adds the given wc into wc_list.
*/
void
OlClassSearchIEDB (WidgetClass wc, OlVirtualEventTable db)
{
	int		i, n;
	Cardinal	num;
	Widget		*widgets;
	register	PerDisplayVEDBInfoPtr	ve_db;

	if (wc == NULL || db == NULL)
		return;

	GetToken();
	if (wc_list != NULL) {
		for (i = 0; i < wc_list_entries; i++) {
			if (wc_list[i].wc == wc && VETsEqual(wc_list[i].db, db))
                                goto propagate_new_wc_db_to_dpys;
                }
 
	}

	wc_list = (OlClassSearchInfo) GetMoreMem(
				(XtPointer) wc_list,
				(int) sizeof(OlClassSearchRec),
				MORESLOTS,
				&wc_list_slots_left,
				&wc_list_slots_alloced);

	wc_list [wc_list_entries].wc = wc;
	wc_list [wc_list_entries++].db = db;
	wc_list_slots_left--;
	/* db->dpy = (Display *)NULL; */
	db->refcnt++;

propagate_new_wc_db_to_dpys:
 
	_OlGetListOfDisplayShells(&widgets, &num);

	for (n = 0; n < num; n++) {
		ve_db = _AllocPDVEDBInfo(XtDisplayOfObject(widgets[n]));

		if (ve_db->wc_list != NULL) {
			for (i = 0; i < ve_db->wc_list_entries; i++)
                                if (ve_db->wc_list[i].wc == wc &&
                                    VETsEqual(ve_db->wc_list[i].db, db))
                                        goto next;
		}

		ve_db->wc_list = (OlClassSearchInfo) GetMoreMem(
					(XtPointer) ve_db->wc_list,
					(int) sizeof(OlClassSearchRec),
					MORESLOTS,
					&ve_db->wc_list_slots_left,
					&ve_db->wc_list_slots_alloced);

		ve_db->wc_list [ve_db->wc_list_entries].wc = wc;
		ve_db->wc_list [ve_db->wc_list_entries++].db =
			 _CopyVEDBToDpy(db, ve_db->dpy);
		ve_db->wc_list_slots_left--;

		next:;
	}
	ReleaseToken();
} /* end of OlClassSearchIEDB */

/*
** a convenience routine to register the Text DB - class
*/
void
OlClassSearchTextDB (WidgetClass wc)
{
	OlVirtualEventTable	OlTextDB;

	GetToken();
	OlTextDB = OlCreateInputEventDB((Widget)NULL,
					(OlKeyOrBtnInfo) OlTextKeyBindings,
					XtNumber(OlTextKeyBindings),
					(OlKeyOrBtnInfo) NULL, 0);

	OlClassSearchIEDB(wc, OlTextDB);
	ReleaseToken();
} /* end of OlClassSearchTextDB */

static void
_CleanupVEDBRefCB(Widget w, XtPointer clientd, XtPointer calld)
{
	OlVirtualEventTable	db = (OlVirtualEventTable)clientd;

	if (--(db->refcnt) <= 0) {
		FREEDB(db);
	}
}

OlVirtualEventTable
OlCreateInputEventDB (Widget w, OlKeyOrBtnInfo key_info, int num_key_info, OlKeyOrBtnInfo btn_info, int num_btn_info)
{
	int			i;
	OlVirtualEventTable	db;
	OlKeyBinding *		key_bindings = NULL;
	OlBtnBinding *		btn_bindings = NULL;
	PerDisplayVEDBInfoPtr	ve_db = (PerDisplayVEDBInfoPtr)NULL;
	OlDBType		type = WidgetDB;

	static const char *F1Key = "n<F1>";


	GetToken();
	if (w != (Widget)NULL)
		ve_db = _AllocPDVEDBInfo(XtDisplayOfObject(w));
	else
		type = UnboundDB;

	if (num_key_info == 0 && num_btn_info == 0) {
		ReleaseToken();
		return (NULL);
	}

#define BINDINGS	key_bindings[i]
#define NAME		BINDINGS.name
#define DFT		BINDINGS.default_value
#define OLCMD		BINDINGS.ol_event
#define USED		BINDINGS.def.used

	if ((OlKeyBinding *)key_info == OlCoreKeyBindings &&
	    ((OlBtnBinding *)btn_info == OlCore3BtnBindings ||
	     (OlBtnBinding *)btn_info == OlCore2BtnBindings ||
	     (OlBtnBinding *)btn_info == OlCore1BtnBindings))
		type |= CoreDB;
	else if ((OlKeyBinding *)key_info == OlTextKeyBindings &&
		 (OlBtnBinding *)btn_info == (OlBtnBinding *)NULL)
			type |= TextDB;

	if (num_key_info != 0) {
		key_bindings = (OlKeyBinding *) MALLOC(
					sizeof(OlKeyBinding) *
					num_key_info);

		if (IsCoreDB(type) || IsTextDB(type)) {
			OlKeyBinding	*key_data = (OlKeyBinding *)key_info;
		/* Ugh .. hack below since OlCreateInputEventDB gets called 
		 * with	w = NULL from OlClassSearchTextDB !! Anyway, dpy
		 * is used only for CoreDB ... so no problemo
		 */
			Display *dpy = w ? XtDisplay(w) : NULL;

			for (i = 0; i < num_key_info; i++) {
				NAME	= key_data[i].name;
				DFT	= key_data[i].default_value;
				OLCMD	= key_data[i].ol_event;
				USED	= 0;

				if 	(IsCoreDB(type) && 
					(strcmp(NAME, XtNhelpKey) == 0) &&
					(XKeysymToKeycode(dpy, XK_Help) == 0)) {
		/* For keyboards with no Help key (read: x86 keyboards) ,
		 * use F1 to simulate it
		 */
						DFT = F1Key;
				}
			}
		} else {
			for (i = 0; i < num_key_info; i++) {
				NAME	= key_info[i].name;
				DFT	= key_info[i].default_value;
				OLCMD	= key_info[i].virtual_name;
				USED	= 0;
			}
		}
#undef BINDINGS
	}

	if (num_btn_info != 0)
	{
#define BINDINGS	btn_bindings[i]
		btn_bindings = (OlBtnBinding *) MALLOC(
					sizeof(OlBtnBinding) *
					num_btn_info);
		if (IsCoreDB(type)) {
			OlBtnBinding	*btn_data = (OlBtnBinding *)btn_info;

			for (i = 0; i < num_btn_info; i++) {
				NAME	= btn_data[i].name;
				DFT	= btn_data[i].default_value;
				OLCMD	= btn_data[i].ol_event;
				USED	= 0;
			}
		} else {
			for (i = 0; i < num_btn_info; i++) {
				NAME	= btn_info[i].name;
				DFT	= btn_info[i].default_value;
				OLCMD	= btn_info[i].virtual_name;
				USED	= 0;
			}
		}
	}
#undef BINDINGS
#undef NAME
#undef DFT
#undef OLCMD
#undef USED

	db = (OlVirtualEventTable) MALLOC(sizeof(OlVirtualEventInfo));

	db->db_type	     = type;
	db->refcnt 	     = 0;
	db->key_bindings     = key_bindings;
	db->btn_bindings     = btn_bindings;
	db->num_key_bindings = (char)num_key_info;
	db->num_btn_bindings = (char)num_btn_info;
	db->sorted_key_db    = NULL;

	if (w == (Widget)NULL) {
		db->dpy	 = (Display *)NULL;
		ReleaseToken();
		return (db);
	} else {
		db->dpy = XtDisplayOfObject(w);
		db->refcnt++;

		XtAddCallback(w,
			      XtNdestroyCallback,
			      _CleanupVEDBRefCB, 
			      (XtPointer)db
		);
	}

	ve_db->avail_dbs =
		(OlVirtualEventTable *) GetMoreMem(
					(XtPointer) ve_db->avail_dbs,
					(int) sizeof(OlVirtualEventTable *),
					MORESLOTS,
					&ve_db->avail_dbs_slots_left,
					&ve_db->avail_dbs_slots_alloced);

	ve_db->avail_dbs [ve_db->avail_dbs_entries++] = db;
	ve_db->avail_dbs_slots_left--;

	db->refcnt++;

	UpdateIEDB(w, db);

	ReleaseToken();
	return (db);
} /* end of OlCreateInputEventDB */

/*
 * VETsEqual
 *
 * compare two Virtaul Event Tables for equivalence.
 *
 */
 
static Boolean
VETsEqual(const OlVirtualEventTable db1, const OlVirtualEventTable db2)
{
        unsigned int n;
 
        if (db1 == (OlVirtualEventTable)NULL ||
            db2 == (OlVirtualEventTable)NULL)
                return ((Boolean)(db1 == db2));
        else
                if (db1 == db2) return ((Boolean)True);
 
        if (db1->num_key_bindings != db2->num_key_bindings ||
            db1->num_btn_bindings != db2->num_btn_bindings)
                return ((Boolean)False);
 
        if (db1->key_bindings == db2->key_bindings &&
            db1->btn_bindings == db2->btn_bindings)
                return ((Boolean)True);
 
#define BINDINGS(db)    (db)->key_bindings[n]
#define NAME(db)        BINDINGS(db).name
#define DFT(db)         BINDINGS(db).default_value
#define OLCMD(db)       BINDINGS(db).ol_event
#define USED(db)        BINDINGS(db).def.used
 
        for (n = 0; n < (unsigned int)db1->num_btn_bindings; n++) {
                if (strcmp(NAME(db1), NAME(db2)) != 0   ||
                    strcmp(DFT(db1),  DFT(db2))  != 0   ||
                    OLCMD(db1) != OLCMD(db2))
                        return((Boolean)False);
        }
 
#undef  BINDINGS
#define BINDINGS(db)    (db)->btn_bindings[n]
        for (n = 0; n < (unsigned int)db1->num_btn_bindings; n++) {
                if (strcmp(NAME(db1), NAME(db2)) != 0   ||
                    strcmp(DFT(db1),  DFT(db2))  != 0   ||
                    OLCMD(db1) != OLCMD(db2))
                        return((Boolean)False);
        }
 
#undef BINDINGS
#undef NAME
#undef DFT
#undef OLCMD
#undef USED
        return ((Boolean)True);
}

/*
 * OlDetermineMouseAction
 *
 * The \fIOlDetermineMouseAction\fR function is used to determine
 * the kind of mouse gesture that is being attempted: MOUSE_CLICK,
 * MOUSE_MULTI_CLICK, or MOUSE_MOVE.  This function is normally
 * called immediately upon reciept of a mouse button press event.
 * It uses the current settings for mouseDampingFactor and
 * multiClickTimeout to determine the kind of gesture being made.
 *
 * Note:
 *
 * This function performs an active pointer grab.  This grab is released
 * for the CLICK type actions but not for MOUSE_MOVE.  It is the
 * responsibility of the caller to ungrab the pointer if the
 * action if MOUSE_MOVE.
 *
 * See also:
 *
 * OlDragAndDrop(3), OlGrabDragPointer(3), OlUngrabDragPointer(3)
 *
 * Synopsis:
 *
 *#include <OpenLook.h>
 *#include <Dynamic.h>
 * ...
 */

#ifdef MTSAFE
typedef struct _mouse_state_data {
	Time last_click_time;
	int  last_x_root;
	int  last_y_root;
	Window last_root;
}	MouseStateData;

void free_thr_specific_data(void *ptr)
{
	if(ptr != NULL)
		XtFree((char *)ptr);
}
#endif

ButtonAction
OlDetermineMouseAction (Widget w, XEvent *event)
{
#ifdef MTSAFE
	static	thread_key_t	*keyp = NULL; 	
	MouseStateData *thr_specific_data;
#endif
	static Time	last_click_time = 0;
	static int	last_x_root = 0, last_y_root = 0;
	static Window	last_root = (Window)0;
	ButtonAction	action          = NOT_DETERMINED;
	XEvent		newevent;
	Cardinal	mouse_damping_factor, multi_click_timeout;

	GetToken();

#ifdef MTSAFE
	if (keyp == NULL) {
		keyp = (thread_key_t *)XtMalloc(sizeof(thread_key_t));
		thr_keycreate(keyp, free_thr_specific_data);
	}
	thr_getspecific(*keyp, (void **)&thr_specific_data);
	if(thr_specific_data == NULL) {
		thr_specific_data = (MouseStateData *)
					XtCalloc(1,sizeof(MouseStateData));
		thr_setspecific(*keyp, (void *)thr_specific_data);
	}
	last_click_time = thr_specific_data->last_click_time;
	last_x_root = thr_specific_data->last_x_root;
	last_y_root = thr_specific_data->last_y_root;
	last_root = thr_specific_data->last_root; 
#endif 
	mouse_damping_factor = _OlGetMouseDampingFactor(w);
	multi_click_timeout  = _OlGetMultiClickTimeout(w);
	while (XGrabPointer(XtDisplayOfObject(w), XtWindowOfObject(w), False,
		ButtonMotionMask | ButtonReleaseMask, GrabModeAsync,
		GrabModeAsync, None, None, CurrentTime) != GrabSuccess)
			;

	while (action == NOT_DETERMINED)
	{
		XWindowEvent(XtDisplayOfObject(w), XtWindowOfObject(w),
			ButtonMotionMask | ButtonReleaseMask, &newevent);

		if ((newevent.type == MotionNotify) &&
			((ABS_DELTA(newevent.xmotion.x, event-> xbutton.x) >=
					mouse_damping_factor) ||
    			 (ABS_DELTA(newevent.xmotion.y, event-> xbutton.y) >=
					mouse_damping_factor)))
				action = MOUSE_MOVE;
		else if ((newevent.type == ButtonRelease) &&
			 (newevent.xbutton.button == event-> xbutton.button))
         			action = MOUSE_CLICK;
	}
	if (action == MOUSE_CLICK)
	{
		if ( (newevent.xbutton.time - last_click_time <=
			multi_click_timeout) && 
			(last_root == newevent.xbutton.root) &&
			/* These checks for the mouse damping factor may
			 * not be necessary (the above code checks the
			 * motionnotify event) but just in case....
			 */
			(ABS_DELTA(newevent.xbutton.x_root, last_x_root) <
					mouse_damping_factor) &&
			(ABS_DELTA(newevent.xbutton.y_root, last_y_root) <
					mouse_damping_factor))
      				action = MOUSE_MULTI_CLICK;
		/* Hold onto:
		 * Previous time click, previous position (x,y), and previous
		 * root window.
		 */
			last_click_time = newevent.xbutton.time;
			last_x_root = newevent.xbutton.x_root;
			last_y_root = newevent.xbutton.y_root;
			last_root = newevent.xbutton.root;
#ifdef MTSAFE
		thr_specific_data->last_click_time = last_click_time;
		thr_specific_data->last_x_root = last_x_root;
		thr_specific_data->last_y_root = last_y_root;
		thr_specific_data->last_root = last_root;
		thr_setspecific(*keyp, (void *)thr_specific_data);
#endif
		XUngrabPointer(XtDisplayOfObject(w), CurrentTime);
	}
	ReleaseToken();
	return (action);
} /* end of OlDetermineMouseAction */


void
_OlGetModifierMapping (Display *dpy)
{
    XModifierKeymap *modifier_map;
    static Modifiers mod_masks[] = { None/* not used */, Mod1Mask, Mod2Mask,
					Mod3Mask, Mod4Mask, Mod5Mask };
    int i, j, start_index;
    PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(dpy);

    /*
     * Get the keycodes corresponding to the various keysyms
     */
    for (i = 0; i < XtNumber(mod_info); i++) {
        ve_db->mod_info[i].left_keycode = XKeysymToKeycode(dpy,
                                                ve_db->mod_info[i].left_keysym);
        ve_db->mod_info[i].right_keycode = XKeysymToKeycode(dpy,
                                                ve_db->mod_info[i].right_keysym);
    }

    /*
     * Read the server's modifier mapping
     */
    modifier_map = XGetModifierMapping(dpy);

    /*
     * Skip shift, lock and control as they have their own 'standard'
     * modifier masks
     */
    start_index = modifier_map->max_keypermod * Mod1MapIndex;

    for (i = start_index; i < modifier_map->max_keypermod * 8; i++) {
        KeyCode kc = modifier_map->modifiermap[i];
        int this_mod = ((i - start_index) / modifier_map->max_keypermod) + 1;

        if (!kc) continue;

        for (j = 0; j < XtNumber(mod_info); j++) {
            if (ve_db->mod_info[j].modifier == None &&
                (kc == ve_db->mod_info[j].left_keycode ||
                 kc == ve_db->mod_info[j].right_keycode)) {
#ifdef DEBUG4
                printf("%s on Mod%d\n", mod_info[j].modifier_name, this_mod);
#endif
                ve_db->mod_info[j].modifier = mod_masks[this_mod];
		_OlFixupMappings(ve_db->mod_info[j].modifier_type,
				 ve_db->mod_info[j].modifier);
            }
        }
    }

#define META_INDEX 1
#define ALT_INDEX 0
	if (_OlCtrlAltMetaKey(dpy) == True) {
		/* 
		 * Substitute Meta with Cntrl-Alt.
		 */
		 ve_db->mod_info[META_INDEX].modifier = 
					(ControlMask | ve_db->mod_info[ALT_INDEX].modifier);
		_OlFixupMappings(ve_db->mod_info[META_INDEX].modifier_type,
						 ve_db->mod_info[META_INDEX].modifier);
	}
#undef ALT_INDEX
#undef META_INDEX


    _OlGetDontCareBits(dpy);
    _OlGetKeypadKeycodes(dpy);

    XFreeModifiermap(modifier_map);
}


static void
_OlFixupMappings (ModifierType modifier_type, Modifiers modifier_mask)
{

    switch (modifier_type) {
	case Alt:
	    _OlChangeMapping("Alt", modifier_mask);
	    _OlChangeMapping("a", modifier_mask);
	    break;
	case Meta:
	    _OlChangeMapping("Meta", modifier_mask);
	    _OlChangeMapping("m", modifier_mask);
	    break;
	case Hyper:
	    _OlChangeMapping("Hyper", modifier_mask);
	    _OlChangeMapping("h", modifier_mask);
	    break;
	case Super:
	    _OlChangeMapping("Super", modifier_mask);
	    _OlChangeMapping("s", modifier_mask);
	    break;
	case NumLock:
	    _OlChangeMapping("NumLock", modifier_mask);
	    break;
	case ModeSwitch:
	    _OlChangeMapping("ModeSwitch", modifier_mask);
	    break;
    }
}

static void
_OlChangeMapping (String s, Modifiers m)
{
    int i;

    for (i = 0; i < XtNumber(mappings); i++)
	if (STREQU(mappings[i].s, s))
	    mappings[i].m = m;
}

Modifiers
_OlGetModifierBinding(Display *dpy, ModifierType modifier_type)
{
    int i;
    PerDisplayVEDBInfoPtr ve_db = _AllocPDVEDBInfo(dpy);

    for (i = 0; i < XtNumber(mod_info); i++)
	if (ve_db->mod_info[i].modifier_type == modifier_type)
	    return (ve_db->mod_info[i].modifier);
    return ((Modifiers)0);
}
    

/*
 * We want to set the dont_care_bits to be LockMask OR'd with whatever
 * modifier bit NumLock is on. We should know what modifier NumLock is
 * on by the mod_info structure
 */
static void
_OlGetDontCareBits (Display *dpy)
{
	int 			i;
	PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(dpy);

	dont_care_bits = LockMask;
	for (i = 0; i < XtNumber(mod_info); i++)
	    if (ve_db->mod_info[i].modifier_type == NumLock ||
			ve_db->mod_info[i].modifier_type == ModeSwitch)
		dont_care_bits |= ve_db->mod_info[i].modifier;
}

String
_OlGetModifierNames (Display* dpy, Modifiers modifier)
{
	static String mod1_name = "Mod1";
	static String mod2_name = "Mod2";
	static String mod3_name = "Mod3";
	static String mod4_name = "Mod4";
	static String mod5_name = "Mod5";
	String	mod_name;

	if ((mod_name =  _OlCheckModifierHasName(dpy, modifier)) == NULL) {
		mod_name = (modifier == Mod1Mask ? mod1_name :
						(modifier == Mod2Mask ? mod2_name :
							(modifier == Mod3Mask ? mod3_name :
								(modifier == Mod4Mask ? mod4_name : mod5_name))));
	}
	return mod_name;
}

static void
_OlGetKeypadKeycodes (Display *dpy)
{
	int 			i;
	PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(dpy);

	for (i = 0; i < XtNumber(keypad_table); i++)
	    XtKeysymToKeycodeList(dpy, ve_db->keypad_table[i].keysym,
		                  &ve_db->keypad_table[i].keycodelist,
				  &ve_db->keypad_table[i].keycount);
}

static Boolean
IsKPKey (Display *dpy, KeyCode keycode, KeySym *keysym_return)
{
	int 			i, j;
	PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(dpy);

	for (i = 0; i < XtNumber(keypad_table); i++)
	    for (j = 0; j < ve_db->keypad_table[i].keycount; j++)
		if (ve_db->keypad_table[i].keycodelist[j] == keycode) {
		    *keysym_return = ve_db->keypad_table[i].keysym;
		    return True;
		}
	return False;
}

static String
_OlCheckModifierHasName (Display *dpy, Modifiers modifier)
{
	int 			i;
	PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(dpy);

	for (i = 0; i < XtNumber(mod_info); i++)
	    if (ve_db->mod_info[i].modifier == modifier)
			return ve_db->mod_info[i].modifier_name;

	return NULL;
}

/**
 ** _OlKeysymToSingleChar()
 **/

int
_OlKeysymToSingleChar (KeySym keysym)
{
	register Cardinal	i;


	for (i = 0; i < XtNumber(singlechar_map); i++)
		if (keysym == singlechar_map[i].keysym)
			return (singlechar_map[i].single);
	return (0);
} /* _OlKeysymToSingleChar */

void
OlLookupInputEvent (Widget w, XEvent *xevent,
		    OlVirtualEvent virtual_event_ret, XtPointer db_flag)
{
	virtual_event_ret->consumed     = False;
	virtual_event_ret->xevent       = xevent;
	virtual_event_ret->dont_care    = dont_care_bits;
	virtual_event_ret->keysym       = NoSymbol;
	virtual_event_ret->buffer       = NULL;
	virtual_event_ret->length       = 0;
	if ((XtIsSubclass(w, textEditWidgetClass) == True) ||
	    (XtIsSubclass(w, textLineWidgetClass) == True))
		virtual_event_ret->virtual_name =
					(OlVirtualName) DoLookupForTextEdit(
					w,
					virtual_event_ret->xevent,
					&virtual_event_ret->keysym,
					&virtual_event_ret->buffer,
					&virtual_event_ret->length,
					True,
				        OL_DEFAULT_IE);
	else
		virtual_event_ret->virtual_name =
					(OlVirtualName) DoLookup(
					w,
					virtual_event_ret->xevent,
					&virtual_event_ret->keysym,
					&virtual_event_ret->buffer,
					&virtual_event_ret->length,
					False,
					db_flag);
	if (w != (Widget)NULL &&
	    xevent != (XEvent *)NULL &&
	    XtIsSubclass(w, flatWidgetClass) == True)
	{
#define X_N_Y(etype)  (Position)xevent->etype.x, (Position)xevent->etype.y
		switch(xevent->type) {
		case KeyPress:		/* FALLTHROUGH */
		case KeyRelease:	/* FALLTRHOUGH */
		case FocusIn:		/* FALLTHROUGH */
		case FocusOut:
			virtual_event_ret->item_index =
				OlFlatGetFocusItem(w);
			break;
		case ButtonPress:	/* FALLTHROUGH */
		case ButtonRelease:
			virtual_event_ret->item_index =
				OlFlatGetItemIndex(w, X_N_Y(xbutton));
			break;
		case MotionNotify:
			virtual_event_ret->item_index =
				OlFlatGetItemIndex(w, X_N_Y(xmotion));
			break;
		case EnterNotify:	/* FALLTHROUGH */
		case LeaveNotify:
			virtual_event_ret->item_index =
				OlFlatGetItemIndex(w, X_N_Y(xcrossing));
			break;
		default:
			virtual_event_ret->item_index = (Cardinal)OL_NO_ITEM;
			break;
		}
#undef X_N_Y
	}
	else
	{
		virtual_event_ret->item_index = (Cardinal)OL_NO_ITEM;
	}
} /* end of OlLookupInputEvent */

/*
 *************************************************************************
 * _OlInitDynamicHandler - this routine initializes the dynamic hander.
 * It is called at application startup time.
 *************************************************************************
 */
void
_OlInitDynamicHandler (Widget w)
{
    PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(XtDisplay(w));
    Widget			ssw;
    
    ssw = _OlGetScreenShellOfWidget(w);
    
    if (ssw != (Widget)NULL) {
	Arg	arg;
	
	XtSetArg(arg, XtNapplShell, &w);
	XtGetValues(ssw, &arg, 1);
    } else {
	w = _OlGetShellOfWidget(w);
    }
    
    if (ve_db->OlCoreDB == (OlVirtualEventTable)NULL) {
	int i;
	
	/* Get the number of physical buttons on the pointer */
	i = XGetPointerMapping(XtDisplay(w),
			       (unsigned char *)0, 0);
	switch (i) {
	    case 1:
		OlCoreBtnBindings = OlCore1BtnBindings;
		break;
	    case 2:
		OlCoreBtnBindings = OlCore2BtnBindings;
		break;
	    case 3:
	    default:
		OlCoreBtnBindings = OlCore3BtnBindings;
		break;
	    }
	for (i = 0; OlCoreBtnBindings[i].name != NULL; i++)
	    ;

	ve_db->OlCoreDB =
	    OlCreateInputEventDB(w,
				 (OlKeyOrBtnInfo) OlCoreKeyBindings,
				 XtNumber(OlCoreKeyBindings),
				 (OlKeyOrBtnInfo) OlCoreBtnBindings,
				 i);
	ve_db->OlCoreDB->refcnt++;
    }
    
    if (ve_db->OlTextDB == (OlVirtualEventTable)NULL) {
	ve_db->OlTextDB =
	    OlCreateInputEventDB(w,
				 (OlKeyOrBtnInfo) OlTextKeyBindings,
				 XtNumber(OlTextKeyBindings),
				 NULL, 0);
	ve_db->OlTextDB->refcnt++;
    }
} /* end of _OlInitDynamicHandler */

/*
 * OlRegisterDynamicCallback
 *
 * The \fIOlRegisterDynamicCallback\fR procedure is used to add
 * a function to the list of registered callbacks to be called
 * whenever the procedure \fIOlCallDynamicCallbacks\fR is invoked.
 * The OlCallDynamicCallback procedure is invoked whenever the
 * RESOURCE_MANAGER property of the Root Window is updated and after
 * the dynamic resources registered using the \fIOlGetApplicationResources\fR
 * have been refreshed.  The OlCallDynamicCallbacks procedure may
 * also be called directly by either the application or other routines in
 * the widget libraries.  The callbacks registered are guaranteed to
 * be called in FIFO order of registration and will be called as
 *
 * .so CWstart
 *  (*CB)(data);
 * .so CWend
 *
 * See also:
 *
 * OlUnregisterDynamicCallback(3), OlCallDynamicCallbacks(3)
 * OlGetApplicationResources(3)
 *
 * Synopsis:
 *
 *#include <Dynamic.h>
 * ...
 */
void
OlRegisterDynamicCallback (OlDynamicCallbackProc CB, XtPointer data)
{
	register int	i;

	GetToken();
	i = num_dynamic_callbacks++;
	if (dynamic_callbacks == NULL)
		dynamic_callbacks = (DynamicCallback *)
			MALLOC(num_dynamic_callbacks * sizeof(DynamicCallback));
	else
		dynamic_callbacks = (DynamicCallback *)
			REALLOC((char *)dynamic_callbacks,
			  num_dynamic_callbacks * sizeof(DynamicCallback));

	dynamic_callbacks[i].CB = CB;
	dynamic_callbacks[i].data = data;
	ReleaseToken();

} /* end of OlRegisterDynamicCallback */

/**
 ** _OlSingleCharToKeysym()
 **/

KeySym
_OlSingleCharToKeysym (int chr)
{
	register Cardinal	i;


	for (i = 0; i < XtNumber(singlechar_map); i++)
		if (chr == singlechar_map[i].single)
			return (singlechar_map[i].keysym);
	return (NoSymbol);
} /* _OlSingleCharToKeysym */

/**
 ** _OlStringToOlBtnDef()
 **/

/* ARGSUSED */
Boolean
_OlStringToOlBtnDef (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass(display, "stringToOlBtnDef", (String)0);

	OlBtnDef		bd;


	if (*num_args)
		ConversionError (
			"wrongParameters",
			"String to OlBtnDef conversion needs no args",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	memset(&bd, 0, sizeof(bd));

	if (from->addr)
		StringToKeyDefOrBtnDef (display, False,
				(String)from->addr, (XtPointer)&bd);
	else
		bd.used = 0;

		/* enable NULL binding */
	if (!bd.used && from->addr[0] != 0) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalSyntax",
			"String to OlBtnDef found illegal syntax: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (OlBtnDef, bd);
} /* end of _OlStringToOlBtnDef */

/* ARGSUSED */
Boolean
_OlStringToOlKeyDef (Display *display, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
	DeclareConversionClass(display, "stringToOlKeyDef", (String)0);

	OlKeyDef		kd;

	if (*num_args)
		ConversionError (
			"wrongParameters",
			"String to OlKeyDef conversion needs no args",
			(String *)0,
			(Cardinal *)0
		);
		/*NOTREACHED*/

	memset(&kd, 0, sizeof(kd));

	if (from->addr && from->addr[0])
		StringToKeyDefOrBtnDef (display, True,
				(String)from->addr, (XtPointer)&kd);
	else
		kd.used = 0;

		/* enable NULL binding */
	if (!kd.used && from->addr[0] != 0) {
		static String		_args[2]  = { 0 , 0 };
		Cardinal		_num_args = 1;

		_args[0] = (String)from->addr;
		ConversionWarning (
			"illegalSyntax",
			"String to OlKeyDef found illegal syntax: %s",
			_args,
			&_num_args
		);
		return (False);
	}

	ConversionDone (OlKeyDef, kd);
} /* end of _OlStringToOlKeyDef */

OlVirtualName
_OlStringToVirtualKeyName (String str)
{
	Cardinal		i, j;
	Widget			dsw;
	PerDisplayVEDBInfoPtr	ve_db;

	ve_db = _AllocPDVEDBInfo(toplevelDisplay);
	for (i = 0; i < ve_db->avail_dbs_entries; i++)
		for (j = 0; j < ve_db->avail_dbs[i]->num_key_bindings; j++)
			if (STREQU(str, ve_db->avail_dbs[i]->key_bindings[j].name))
				return (ve_db->avail_dbs[i]->key_bindings[j].ol_event);
	return (OL_UNKNOWN_KEY_INPUT);
} /* end of _OlStringToVirtualKeyName */

/*
 * OlUnregisterDynamicCallback
 *
 * The \fIOlUnregisterDynamicCallback\fR procedure is used to remove
 * a function from the list of registered callbacks to be called
 * whenever the procedure \fIOlCallDynamicCallbakcs\fR is invoked.
 *
 * See also:
 *
 * OlRegisterDynamicCallback(3), OlCallDynamicCallbacks(3)
 * OlGetApplicationResources(3)
 *
 * Synopsis:
 *
 *#include <Dynamic.h>
 * ...
 */
int
OlUnregisterDynamicCallback (OlDynamicCallbackProc CB, XtPointer data)
{
	int	i;
	int	retval;

	GetToken();
	for (i = 0; i < num_dynamic_callbacks; i++)
		if (dynamic_callbacks[i].CB == CB &&
			dynamic_callbacks[i].data == data)
				break;

	if (i == num_dynamic_callbacks)
		retval = 0;
	else
	{
		num_dynamic_callbacks--;
		if (num_dynamic_callbacks == 0)
      		{
      			FREE((char *)dynamic_callbacks);
      			dynamic_callbacks = (DynamicCallback *)NULL;
      		}
   		else
      		{
      			if (i < num_dynamic_callbacks)
         			memmove((void*)&dynamic_callbacks[i],
					(const void*)&dynamic_callbacks[i + 1],
               				(size_t)(num_dynamic_callbacks - i) *
					sizeof(DynamicCallback));
      			dynamic_callbacks = (DynamicCallback *)
         			REALLOC((char *)dynamic_callbacks,
					num_dynamic_callbacks *
					sizeof(DynamicCallback));
      		}
   		retval = 1;
   	}
	ReleaseToken();
	return (retval);
} /* end of OlUnregisterDynamicCallback */

/*
** This routine adds the given w into wid_list.
*/
void
OlWidgetSearchIEDB (Widget w, OlVirtualEventTable db)
{
	int				i;
	register PerDisplayVEDBInfoPtr	ve_db;

	if (w == NULL)
		return;

	GetToken();
	ve_db = _AllocPDVEDBInfo(XtDisplayOfObject(w));
	if (ve_db->wid_list != NULL)
	{
		for (i = 0; i < ve_db->wid_list_entries; i++)
			if (ve_db->wid_list[i].w == w && 
			    VETsEqual(ve_db->wid_list[i].db, db)) {
				ReleaseToken();
				return;
			}
	}
	ve_db->wid_list = (OlWidgetSearchInfo) GetMoreMem(
					(XtPointer) ve_db->wid_list,
					(int) sizeof(OlWidgetSearchRec),
					MORESLOTS,
					&ve_db->wid_list_slots_left,
					&ve_db->wid_list_slots_alloced);

	ve_db->wid_list [ve_db->wid_list_entries].w = w;
	ve_db->wid_list [ve_db->wid_list_entries++].db = db;
	ve_db->wid_list_slots_left--;
	db->refcnt++;
	ReleaseToken();
} /* end of OlWidgetSearchIEDB */

/*
** a convenience routine to register the Text DB - instance
*/
void
OlWidgetSearchTextDB (Widget w)
{
	register PerDisplayVEDBInfoPtr	ve_db;

	GetToken();
	ve_db = _AllocPDVEDBInfo(XtDisplayOfObject(w));
	OlWidgetSearchIEDB(w, ve_db->OlTextDB);
	ReleaseToken();
} /* end of OlWidgetSearchTextDB */

/*
** This routine returns an index which points to the given list
**	if the search succeed otherwise it returns -1.
*/
static int
BinarySearch (KeySym keysym, Modifiers modifier, Token *list, int num_tokens, OlKeyBinding *key_bindings)
{
#define YY(i)		key_bindings[i].def
#define KEYSYM_T        YY(list[j].i).keysym[list[j].j]
#define MOD_T           YY(list[j].i).modifier[list[j].j]

	int	k, m, j;
	int	retval;

	k = 1;
	m = num_tokens;
	while (k <= m)
	{
		j = (k+m) / 2;
		if ((retval = CompareUtil (keysym, modifier,
					   KEYSYM_T, MOD_T)) == 0)
			return (j);
		if (retval < 0)
			m = j - 1;
		else
			k = j + 1;
	}

	return (-1);

#undef YY
#undef KEYSYM_T
#undef MOD_T
} /* end of BinarySearch */

/*
** build a stack w.r.t db_flag, return num_entries in stack
**	If the db_flag == OL_DEFAULT_IE, then
**		the stack is according to the registering order
**		(Last in First out) in the Class list, the Widget
**		list, and OlCoreDB.
**	else
**		only one DB will be pushed into stack.
*/
static void
BuildIEDBstack (Widget w, XtPointer db_flag)
{
	int			i, j;
	PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(XtDisplayOfObject(w));

	ve_db->db_stack = (OlVirtualEventTable *) GetMoreMem (
				(XtPointer) ve_db->db_stack,
			    	(int) sizeof (OlVirtualEventTable *),
			    	MORESLOTS,
			    	&ve_db->db_stack_slots_left,
				&ve_db->db_stack_slots_alloced);
	ve_db->db_stack_entries = 0;
	ve_db->db_stack_slots_left = ve_db->db_stack_slots_alloced;

		if (db_flag == OL_TEXT_IE) {
			ve_db->db_stack[ve_db->db_stack_entries++] = ve_db->OlTextDB;
			return;
		}

		if (db_flag == OL_CORE_IE) {
			ve_db->db_stack[ve_db->db_stack_entries++] = ve_db->OlCoreDB;
			return;
		}

		if (db_flag != OL_DEFAULT_IE) {
			ve_db->db_stack[ve_db->db_stack_entries++] = 
				(OlVirtualEventTable) db_flag;
			return;
		}

/* OL_DEFAULT_IE */
/* search for wid_list first. have to search all entries because
	mutiple DBs may be registered by the same widget */

	for (i = ve_db->wid_list_entries - 1; i >= 0; i--)
	{
		if (ve_db->wid_list[i].w == w)
		{
				/* no need to push if it is there already */
			for (j = 0; j < ve_db->db_stack_entries; j++)
				if (ve_db->db_stack[j] == ve_db->wid_list[i].db)
					continue;
			ve_db->db_stack = 
				(OlVirtualEventTable *) GetMoreMem (
					(XtPointer) ve_db->db_stack,
					(int) sizeof (OlVirtualEventTable *),
					MORESLOTS,
					&ve_db->db_stack_slots_left,
					&ve_db->db_stack_slots_alloced);

			ve_db->db_stack[ve_db->db_stack_entries++] = 
				ve_db->wid_list[i].db;

			ve_db->db_stack_slots_left--;
		}
	}
	
/* search for wc_list next. have to search all entries because
	mutiple DBs may be registered by the same widget class */

	for (i = ve_db->wc_list_entries - 1; i >= 0; i--) {
		if (XtIsSubclass(w, ve_db->wc_list[i].wc)) {
			for (j = 0; j < ve_db->db_stack_entries; j++)
				if (ve_db->db_stack[j] == ve_db->wc_list[i].db)
						continue;

			ve_db->db_stack = (OlVirtualEventTable *) GetMoreMem (
				(XtPointer) ve_db->db_stack,
			    	(int) sizeof (OlVirtualEventTable *),
			    	MORESLOTS,
			    	&ve_db->db_stack_slots_left,
				&ve_db->db_stack_slots_alloced);

			ve_db->db_stack[ve_db->db_stack_entries++] = 
				ve_db->wc_list[i].db;
			ve_db->db_stack_slots_left--;
		}
	}

/* push coreDB at end */

	ve_db->db_stack = (OlVirtualEventTable *) GetMoreMem (
					(XtPointer) ve_db->db_stack,
					(int) sizeof (OlVirtualEventTable *),
					MORESLOTS,
					&ve_db->db_stack_slots_left,
					&ve_db->db_stack_slots_alloced);

	ve_db->db_stack[ve_db->db_stack_entries++] = ve_db->OlCoreDB;

		/* re-initialize it to avoid the additional allocation */
	ve_db->db_stack_slots_left = ve_db->db_stack_slots_alloced;

} /* end of BuildIEDBstack */

/*
 * This routine compares two given tokens by calling CompareUtil().
 *
 * note: token.i points to current_key_bindings[].
 *	 token.j points to current_key_bindings[token.i].modifiers[] and
 *			   current_key_bindings[token.i].keysym[].
 *
 * see qsort(3C) for more details
 */
static int
CompareFunc (const void *v1, const void *v2)
{
    Token *t1 = (Token *)v1;
    Token *t2 = (Token *)v2;

#define XX(i)		current_key_bindings_to_sort[i]
#define YY(i)		current_key_bindings_to_sort[i].def
#define KEYSYM1		YY(t1->i).keysym[t1->j]
#define KEYSYM2		YY(t2->i).keysym[t2->j]
#define MOD1		YY(t1->i).modifier[t1->j]
#define MOD2		YY(t2->i).modifier[t2->j]

	int     retval;

        retval = CompareUtil (KEYSYM1, MOD1, KEYSYM2, MOD2);

#ifdef DEBUG4
 if (retval == 0)	/* equal */
	fprintf (stderr, "%s and %s have the same bindings\n",
				XX(t1->i).name, XX(t2->i).name);
#endif

	return (retval);

#undef XX
#undef YY
#undef KEYSYM1
#undef KEYSYM2
#undef MOD1
#undef MOD2
} /* end of CompareFunc */

/*
** This routine uses the following semantics to do the comparsion.
**
**		 KEYSYM1 == KEYSYM2  KEYSYM1 < KEYSYM2  KEYSYM1 > KEYSYM2
** MOD1 == MOD2  0		     -1			1
** MOD1 <  MOD2  -1		     -1			1
** MOD1 >  MOD2  1		     -1			1
**
*/
static int
CompareUtil (KeySym keysym1, Modifiers mod1, KeySym keysym2, Modifiers mod2)
{
	if (keysym1 < keysym2)
		return (-1);

	if (keysym1 > keysym2)
		return (1);

	/*
	 * else..keysyms are equal
	 */

	if (mod1 < mod2)
		return (-1);

	if (mod1 > mod2)
		return (1);

	/*
	 * else..mod1 and mod2 are equal
	 */
	return(0);
} /* end of CompareUtil */

/*
 * _OlTranslateKPKeySym - This function is taken from the Xlib function
 * XTranslateKeySym(). It EXPECTS a keypad keysym and fills the provided
 * buffer with the corresponding string. The length of this string is
 * returned. Two restrictions to be aware of:
 * It EXPECTS the buffer to have space for 1 character.
 * It does NOT allow rebinding of KP keysyms.
 */

static int
_OlTranslateKPKeySym (KeySym keysym, char *buffer, int nbytes)
{
    register unsigned char c;
    unsigned long hiBytes;

    /* We MUST only be passed keypad (XK_KP_mumble) keysyms */
    /* if X keysym, convert to ascii by grabbing low 7 bits */

    hiBytes = keysym >> 8;

    if (keysym == XK_KP_Space)
        c = XK_space & 0x7F; /* patch encoding botch */
    else if (hiBytes == 0xFF)
        c = keysym & 0x7F;
    else
        c = keysym & 0xFF;
    buffer[0] = c;
    return 1;
}

/*
 * Return an XContext type for compose (see DoLookup)
 */
static XContext
ComposeContextType (void)
{
static	XContext compose_context_type = (XContext) NULL;

	if (compose_context_type == (XContext) NULL)
		compose_context_type = XUniqueContext();

	return (compose_context_type);
}

/*
 * Free data structure for compose held against widget.
 * Done here as a destroy callback.
 */
static void
FreeComposeData (Widget w, XtPointer client_data, XtPointer call_data)
{
        XContext        cctype;
        ComposeDataPtr  cdp;

	/* Look up context type for Compose */
	cctype = ComposeContextType();

	if (XFindContext(XtDisplayOfObject(w),
		         XtWindowOfObject(w), cctype, (caddr_t *) &cdp)
			== (XContext) 0)
	{
		/* Context found: Free data strored there & delete context */

		XtFree( (char *) cdp );
		XDeleteContext( XtDisplayOfObject(w), XtWindowOfObject(w), cctype);
	}


}

static OlInputEvent
DoLookup (Widget w, XEvent *event, KeySym *keysym, String *buffer,
	  Cardinal *length, Boolean textedit_flag, XtPointer db_flag)
{
	OlInputEvent	retval = OL_UNKNOWN_INPUT;

	XComposeStatus	status_return;
	char		buf[10];
	KeySym		keysym_return;
	int		length_return;
	XEvent		newevent;
	Modifiers	num_lock;
	Arg		args[1];
        Display		*dpy;
        Window		win;
#ifdef	DO_COMPOSE
	static	Boolean	led_on = False;	/* yet another static! */
#endif

	XContext	cctype;
	ComposeDataPtr	cdp;

	status_return.compose_ptr = buf;
	status_return.chars_matched = 0;
	
	BuildIEDBstack(w, db_flag);

	switch (event->type)
	{
	    case KeyPress:
        /*
         * To conform with 'standard' Xlib, we now need to do our own
	 * processing of NumLock. If this key is down AND the keyevent is
	 * from a keypad key, we assume the user wants unmodified numeric
	 * keypad behaviour and we simply return OL_UNKNOWN_KEY_INPUT.
	 * This bypasses the binding mechanisms for such keys.
         */
           	num_lock = _OlGetModifierBinding(event->xany.display,NumLock);
        	if ((num_lock & event->xkey.state) &&
                       	IsKPKey(event->xany.display,
				event->xkey.keycode, &keysym_return)) {
		    length_return = _OlTranslateKPKeySym(keysym_return,
						buffer_return, BUFFER_SIZE);
		    /*
		     * Special case the KP Enter key so that we don't get
		     * a ^M displayed
		     */
		    if (keysym_return == XK_KP_Enter)
			retval = OL_RETURN;
		    else
		        retval = OL_UNKNOWN_KEY_INPUT;
		}
		else {		/* handle compose sequences */
		    Boolean	composing;

      		    retval = WhatOlKey(w, event, textedit_flag);

		    dpy = XtDisplayOfObject(w);
		    win = XtWindowOfObject(w);

		    cctype = ComposeContextType();

		    composing = ( XFindContext(dpy, win,
					cctype, (caddr_t *) &cdp) ?
					False : True );

		    if (composing)
		    {
			if (((XKeyEvent *)event)->time == cdp->time)
			{
				/* not a new event; use stored data */

			        length_return =	cdp->buf_len;
			        strcpy(buffer_return, cdp->buf);
			        keysym_return =	cdp->keysym;
			}
			else
			{
				/* new event; look up string & store */

				cdp->buf_len = 
					XLookupString((XKeyEvent *) event,
                                                cdp->buf,
                                                BUFFER_SIZE,
                                                &cdp->keysym,
                                                &cdp->cstatus);
 
				(cdp->buf)[cdp->buf_len] = '\0';  

				cdp->time = ((XKeyEvent *)event)->time;

			        length_return =	cdp->buf_len;
			        strcpy(buffer_return, cdp->buf);
			        keysym_return =	cdp->keysym;

				if (cdp->cstatus.chars_matched == 0)
				{
				    /* compose finished - remove
				       stored context and the callback
				       that would have cleared it on death
				       of widget */
					
#ifdef	DO_COMPOSE
				    XtSetArg(args[0], XtNcomposeLED,
					     (XtPointer)(led_on = False));
				    XtSetValues(cdp->vendor, args, 1); 
							/* turn off */
#endif	/* DO_COMPOSE */

				    if (XtHasCallbacks(w, XtNdestroyCallback) ==
					    XtCallbackHasSome )
				    {
					XtRemoveCallback(w, XtNdestroyCallback,
					    (XtCallbackProc) FreeComposeData,
					    (XtPointer) cdp );
				    }

				    XtFree( (char *) cdp );

				    XDeleteContext(dpy, win, cctype);

				}

			}
		    }
		    else	/* !composing */
		    {
			length_return = XLookupString((XKeyEvent *) event,
						buffer_return,
						BUFFER_SIZE,
						&keysym_return,
						&status_return);
			
			if (status_return.chars_matched > 0)
			{

				/* Now in compose sequence so
				   allocate, fill & store ComposeData struct */

				cdp = (ComposeDataPtr) 
					XtMalloc(sizeof(ComposeData));

				cdp->widget = w;
				cdp->vendor = _OlGetShellOfWidget(w);
				cdp->time = ((XKeyEvent *)event)->time;
				buffer_return[length_return] = '\0';
				strcpy(cdp->buf, buffer_return);
				cdp->buf_len = length_return;
				cdp->keysym = keysym_return;
				cdp->cstatus.compose_ptr =
					status_return.compose_ptr;
				cdp->cstatus.chars_matched =
					status_return.chars_matched;

				if (XSaveContext(dpy, win, cctype,
						 (char *)cdp))
				   OlError(dgettext(OlMsgsDomain,
					"DoLookup: XSaveContext failed."));

				/* If widget dies during a compose the 
				   strored context should be cleaned up */

				XtAddCallback(w, XtNdestroyCallback,
				    (XtCallbackProc) FreeComposeData, 
				    (XtPointer) cdp );
			
#ifdef	DO_COMPOSE
				XtSetArg(args[0], XtNcomposeLED, 
					 (XtPointer)(led_on = True));
				XtSetValues(cdp->vendor, args, 1);
						 /* turn on */
#endif	/* DO_COMPOSE */
			}
#ifdef	DO_COMPOSE
			else if (led_on) {
				XtSetArg(args[0], XtNcomposeLED, 
					 (XtPointer)(led_on = False));
				XtSetValues(_OlGetShellOfWidget(w), args, 1);
			}
#endif	/* DO_COMPOSE */
		    }
		}	/* end of compose handling */

		buffer_return[length_return] = '\0';
      		if (keysym != NULL)
         		*keysym = keysym_return;
      		if (buffer != NULL)
	 		*buffer = buffer_return;
      		if (length != NULL)
         		*length = length_return;
      		if (IsDampableKey(textedit_flag, keysym_return))
        		while (XCheckIfEvent(
           				XtDisplayOfObject(w),
					&newevent, IsSameKeyEvent,
					(char *) event))
           			;
      		break;
   	    case ButtonPress:
   	    case ButtonRelease:
			/* WhatOlBtn returns OL_UNKNOWN_INPUT because*/
			/* crossing/motion aren't the button events  */

		retval = WhatOlBtn (	event->xany.display,
					event->xbutton.state,
					event->xbutton.button);
		if (retval == OL_UNKNOWN_INPUT)
      			retval = OL_UNKNOWN_BTN_INPUT;
      		break;
	    case EnterNotify:
	    case LeaveNotify:
#ifdef	DO_COMPOSE
		dpy = XtDisplayOfObject(w);
		win = XtWindowOfObject(w);

		cctype = ComposeContextType();

		if (!XFindContext(dpy, win, cctype, (caddr_t *) &cdp)) {
			led_on = (event->type == EnterNotify);
			XtSetArg(args[0], XtNcomposeLED, (XtPointer)led_on);
			XtSetValues(cdp->vendor, args, 1); /* turn on/off */
		}
#endif	/* DO_COMPOSE */
		if (textedit_flag == True)
			break;
		retval = WhatOlBtn (event->xany.display,
				    event->xcrossing.state, 0);
		break;
	    case MotionNotify:
		if (textedit_flag == True)
			break;
		retval = WhatOlBtn (event->xany.display,
				    event->xmotion.state, 0);
		break;
   	    default:
      		break;
   	}
	return (retval);
} /* end of DoLookup */

/*
 * DynamicHandler - this routine is used to monitor changes of the
 * RESOURCE_MANAGER property that occur on the RootWindow.  When this
 * property changes, several application databases need to be
 * re-initialized.
 */
/* ARGSUSED */
void
DynamicHandler (Widget w)
{
	int	i;
	PerDisplayVEDBInfoPtr	ve_db = _AllocPDVEDBInfo(XtDisplay(w));
	Widget			ssw;

	ssw = _OlGetScreenShellOfWidget(w);

	if (ssw != (Widget)NULL) {
		Arg	arg;

		XtSetArg(arg, XtNapplShell, &w);
		XtGetValues(ssw, &arg, 1);
	} else {
		w = _OlGetShellOfWidget(w);
	}

		/* so we know we are doing the dynamic changes */
	_OlDynResProcessing = True;

	for (i = 0; i < ve_db->avail_dbs_entries; i++)
		UpdateIEDB(w, ve_db->avail_dbs[i]);

		/* Re-initialize the global application's
		 * resources.
		 */

	_OlInitAttributes(ssw);

		/* Now that we've re-initialized the
		 * application's resources, update
		 * local copies of them.
		 */

		/* Do OPENLOOK dynamic resource processing */

	_OlDynResProc(XtScreen(ssw));

		/* always turn it off when operaiton is completed */
	_OlDynResProcessing = False;
} /* end of DynamicHandler */

/*
**	This routine will alloc more memory if necessary.
*/
static XtPointer
GetMoreMem (XtPointer list, int size, int more_slots, short int *num_slots_left, short int *num_slots_alloced)
{
	XtPointer	new_list;

		/* XtRealloc() will call XtMalloc() if list is NULL */
	if (list == NULL || *num_slots_left == 0)
	{
		*num_slots_left += more_slots;
		*num_slots_alloced += more_slots;
		new_list = XtRealloc (list, size * (*num_slots_alloced));
	}
	else
		new_list = list;

	return (new_list);
} /* end of GetMoreMem */

/*
** Produce db->sorted_key_db by a given db
*/
static void
InitSortedKeyDB (OlVirtualEventTable db)
{
#define SORTEDDB	db->sorted_key_db
#define NUMKEYS		db->sorted_key_db[0].i
#define DEFUSED		db->key_bindings[i].def.used
#define TABLE		SORTEDDB[NUMKEYS]

	int		num_keys = 0;
	int		i, j;

	if (db == NULL || db->num_key_bindings == 0)
		return;

	num_keys = db->num_key_bindings * MAXDEFS;

	if (SORTEDDB == NULL)
		SORTEDDB = (Token *) MALLOC (sizeof(Token) * (num_keys + 1));

	NUMKEYS = 0;

	for (i = 0; i < db->num_key_bindings; i++)
	{
		for (j = 0; j < DEFUSED; j++)
		{
			NUMKEYS++;
			TABLE.i = (short) i;
			TABLE.j = (short) j;
		}
	}
	current_key_bindings_to_sort = db->key_bindings;
	qsort (&db->sorted_key_db[1], NUMKEYS, sizeof(Token), CompareFunc);

#undef SORTEDDB
#undef NUMKEYS
#undef DEFUSED
#undef TABLE
} /* end of InitSortedKeyDB */

static Boolean
IsComposeBtnOrKey (PerDisplayVEDBInfoPtr ve_db, Boolean is_keydef,
		   String name, int *which_db, int *which_binding)
{
#define NUMKEYS		ve_db->avail_dbs[i]->num_key_bindings
#define KEYNAME		ve_db->avail_dbs[i]->key_bindings[j].name
#define KEYUSED		ve_db->avail_dbs[i]->key_bindings[j].def.used
#define NUMBTNS		ve_db->avail_dbs[i]->num_btn_bindings
#define BTNNAME		ve_db->avail_dbs[i]->btn_bindings[j].name
#define BTNUSED		ve_db->avail_dbs[i]->btn_bindings[j].def.used

	int		i, j;
	Boolean		ret_val = False,
			done_flag = False;

	for (i = 0; i < ve_db->avail_dbs_entries; i++)
	{
		if (is_keydef == True)
		{
			for (j = 0; j < NUMKEYS; j++)
			{
				if (STREQU(name, KEYNAME))
				{
					done_flag = True;
					if (KEYUSED != 0)
						ret_val = True;
					break;
				}
			}
		}
		else	/* is_btn_def */
		{
			for (j = 0; j < NUMBTNS; j++)
			{
				if (STREQU(name, BTNNAME))
				{
					done_flag = True;
					if (BTNUSED != 0)
						ret_val = True;
					break;
				}
			}
		}
		if (done_flag == True)
			break;
	}
	if (done_flag == True)
	{
		if (ret_val == False)
		{
			char		buff[256];

			(void)sprintf (buff, dgettext(OlMsgsDomain,
				"IsComposeBtnOrKey: no definition for %1$s yet"),
				name);
			OlWarning (buff);
		}
		else
		{
			*which_db = i;
			*which_binding = j;
		}
	}
	return (ret_val);

#undef NUMKEYS
#undef KEYNAME
#undef KEYUSED
#undef NUMBTNS
#undef BTNNAME
#undef BTNUSED
} /* end of IsComposeBtnOrKey */

/*
 * IsSameKeyEvent
 *
 */
/* ARGSUSED */
static Bool
IsSameKeyEvent (Display *d, XEvent *event, char *arg)
{
	XEvent * e = (XEvent *) arg;

	return(Bool)(event-> type         == e-> type &&
		     event-> xkey.window  == e-> xkey.window &&
		     event-> xkey.state   == e-> xkey.state &&
		     event-> xkey.keycode == e-> xkey.keycode);
} /* end of IsSameKeyEvent */

static Boolean
ParseKeysymList (String *p_str, Modifiers *p_modifiers, String *p_detail)
{
	String			p		= *p_str;
	String			lbra;
	String			rbra;
	String			token;

	Cardinal		i;


	if (!p)
		return (False);

	/*
	 * Skip over leading ``whitespace''.
	 */
	p += strspn(p, DEF_SEPS);
	if (!*p)
		return (False);

	/*
	 * General syntax:
	 *
	 *	modifiers <keysym> [ sep modifiers <keysym> ... ]
	 *
	 * Each item in the list MUST have a <keysym>, thus the
	 * '<' (LBRA) and '>' (RBRA) are dominant, and we can scan for
	 * them before looking for the "sep". This allows for separator
	 * characters within the brackets, such as <,>, and allows one
	 * to use the same separator characters between modifiers as
	 * between items in the list. In fact, it allows one to forego
	 * using any separators whatsoever, as in:
	 *
	 *	Shift<a>Ctrl<b>		(identical to Shift<a>,Ctrl<b>)
	 */
	lbra = strchr(p, LBRA);
	if (!lbra)
		return (False);
	rbra = strchr(lbra, RBRA);
	if (!rbra)
		return (False);

	/*
	 * Allow for this: <>>
	 */
	if (rbra[1] == RBRA)
		rbra++;

	/*
	 * Skip over trailing ``whitespace'', to align with next
	 * item in the list.
	 */
	*p_str = rbra + strspn(rbra+1, DEF_SEPS);

	/*
	 * Set up returned detail:
	 */
	*p_detail = lbra + 1;
	*rbra = 0;

	/*
	 * Convert string of modifiers to bit-masks.
	 */
	*lbra = 0;
	*p_modifiers = 0;
	for (
		token = strtok(p, MOD_SEPS);
		token;
		token = strtok((String)0, MOD_SEPS)
	)
 		for (i = 0; i < XtNumber(mappings); i++)
			if (STREQU(mappings[i].s, token)) {
				*p_modifiers |= (Modifiers)mappings[i].m;
				break;
			}

	return (True);
} /* end of ParseKeysymList */


/**
 ** _OlStringToButton()
 **/
BtnSym
_OlStringToButton (String str)
{
	Cardinal		i;

	BtnSym			ret	= 0;


	if (str)
		for (i = 0; i < XtNumber(mappings); i++)
			if (STREQU(mappings[i].s, str)) {
				ret = (BtnSym)mappings[i].m;
				break;
			}
	return (ret);
} /* end of StringToButton */


static void
StringToKeyDefOrBtnDef (Display *display, Boolean is_keydef,
			String from, XtPointer result)
{
    OlKeyDef *		kd	= (OlKeyDef *)result;
    OlBtnDef *		bd	= (OlBtnDef *)result;
    
    Boolean		parsed;
    String		copy	= Strdup(from);
    String		p;
    String		comma_ptr;
    int			i;
    
    p = copy;
    for (i = 0; i < MAXDEFS; i++) {
	if (comma_ptr = strchr(p, ','))
	    *comma_ptr = '\0';
	
	if (is_keydef) {
	    parsed = _OlParseKeyOrBtnSyntax(display, p, is_keydef, NULL,
					    &kd->keysym[i], &kd->modifier[i]);
	} else {
	    parsed = _OlParseKeyOrBtnSyntax(display, p, is_keydef,
					    &bd->button[i], NULL,
					    &bd->modifier[i]);
	}

	if (!parsed) {
	    char buff[256];
	    
	    (void)sprintf(buff, dgettext(OlMsgsDomain,
			  "StringToKeyDefOrBtnDef: invalid syntax %s"),
			  p);
	    OlWarning (buff);

	    /* don't let the erroneous parse interfere with later attempts */
	    if (is_keydef)
		kd->keysym[i] = kd->modifier[i] = 0;
	    else
		bd->button[i] = bd->modifier[i] = 0;

	    --i;	/* still try other entries */
	}

	else if (is_keydef) {
				kd->keysym[i] = _OlCanonicalKeysym(display, kd->keysym[i],
								   (KeyCode *)0, &kd->modifier[i]);
			}

			if (comma_ptr == NULL) {
				++i;
				break;
			}
			else p = comma_ptr + 1;
			}
			if (is_keydef)
			kd->used = i;
			else
			bd->used = i;
			
			XtFree(copy);
			
		#ifdef DEBUG1K
			if (is_keydef == True)
			{
				fprintf (stderr, "%22s ", from);
				for (i = 0; i < kd->used; i++)
				fprintf (stderr, "(0x%x, 0x%x)",
					 kd->modifier[i], kd->keysym[i]);
				fprintf (stderr, "\n");
			}
		#endif
		#ifdef DEBUG1B
			if (is_keydef == False)
			{
				fprintf (stderr, "%22s ", from);
				for (i = 0; i < bd->used; i++)
				fprintf (stderr, "(0x%x, 0x%x)",
					 bd->modifier[i], bd->button[i]);
				fprintf (stderr, "\n");
			}
		#endif
		} /* end of StringToKeyDefOrBtnDef */

		/*
		 * this routine is called to update a given DB
		 */
		static void
		UpdateIEDB (Widget w, OlVirtualEventTable db)
		{
			register int		i = 0;
			XtResource		r[100];

			if (db == NULL)
			{
		#ifdef DEBUG4
		 fprintf (stderr, "UpdateIEDB: NULL db\n");
		#endif
				return;
			}

			if (_OlMax(db->num_key_bindings, db->num_btn_bindings) > XtNumber(r))
			{
				OlError(dgettext(OlMsgsDomain,
					"UpdateIEDB: internal stack too small, maximum is 100"));
			}

			for (i = 0; i < db->num_key_bindings; i++)
			{
				r[i].resource_name   = (String)db->key_bindings[i].name;
				r[i].resource_class  = (String)db->key_bindings[i].name;
				r[i].resource_type   = XtROlKeyDef;
				r[i].resource_size   = sizeof(OlKeyDef);
				r[i].resource_offset = (Cardinal) ((char *)
					&(db->key_bindings[i].def)-(char *)db->key_bindings);
				r[i].default_type    = XtRString;
				r[i].default_addr    = (XtPointer)
							db->key_bindings[i].default_value;
			}
			XtGetApplicationResources(w, (XtPointer)db->key_bindings,r,i,NULL,0);

			for (i = 0; i < db->num_btn_bindings; i++)
			{
				r[i].resource_name   = (String)db->btn_bindings[i].name;
				r[i].resource_class  = (String)db->btn_bindings[i].name;
				r[i].resource_type   = XtROlBtnDef;
				r[i].resource_size   = sizeof(OlBtnDef);
				r[i].resource_offset =  (Cardinal)((char *)
					&(db->btn_bindings[i].def)-(char *)db->btn_bindings);
				r[i].default_type    = XtRString;
				r[i].default_addr    = (XtPointer)
							db->btn_bindings[i].default_value;
			}
			XtGetApplicationResources(w, (XtPointer)db->btn_bindings,r,i,NULL,0);

		#ifdef DEBUG3
		 {
		#define BINDINGS	db->key_bindings
		#define TOTAL		db->num_key_bindings
		#define NAME		BINDINGS[i].name
		#define OLCMD		BINDINGS[i].ol_event
		#define DEFUSED		BINDINGS[i].def.used
		#define MODIFIER	BINDINGS[i].def.modifier[j]
		#define KEYSYM		BINDINGS[i].def.keysym[j]

		 int	i, j;

		 fprintf (stderr, "\n");
		 for (i = 0; i < TOTAL; i++)
		 {
			fprintf (stderr, "%20s %d %d ", NAME, OLCMD, DEFUSED);
			for (j = 0; j < DEFUSED; j++)
				fprintf (stderr, "(0X%x 0X%x) ", MODIFIER, KEYSYM);
			fprintf (stderr, "\n");
		 }
		 fprintf (stderr, "\n");

		#undef BINDINGS
		#undef TOTAL
		#undef KEYSYM

		#define BINDINGS	db->btn_bindings
		#define TOTAL		db->num_btn_bindings
		#define BUTTON		BINDINGS[i].def.button[j]

		 fprintf (stderr, "\n");
		 for (i = 0; i < TOTAL; i++)
		 {
			fprintf (stderr, "%20s %d %d ", NAME, OLCMD, DEFUSED);
			for (j = 0; j < DEFUSED; j++)
				fprintf (stderr, "(0X%x 0X%x) ", MODIFIER, BUTTON);
			fprintf (stderr, "\n");
		 }
		 fprintf (stderr, "\n");

		#undef BINDINGS
		#undef TOTAL
		#undef NAME
		#undef OLCMD
		#undef DEFUSED
		#undef MODIFIER
		#undef BUTTON
		 }
		#endif

			InitSortedKeyDB (db);

			db->db_type |= WidgetDB;
		} /* end of UpdateIEDB */

		/*
		 * WhatOlBtn - this routine returns an O/L button command by
		 *	using "button" and "state".
		 *	if "button" == 0, the query is from a motion, enter, or
		 *	leave event. it will always do a perfect match.
		 *
		 *	note: this routine assumes the BuildIEDBstack has been called
		 *		before getting here.
		 */
		static OlVirtualName
		WhatOlBtn (Display *dpy, unsigned int state, unsigned int button)
		{
			int		i, j, k,
					button_type = 0;	/* button detail */
			register	PerDisplayVEDBInfoPtr	ve_db;

		#ifdef DEBUG2B
		 if (button)
			printf ("(state, button): (0x%x, %d)", state, button);
		#endif

			state &= ~dont_care_bits;
			
			if (button != 0)	/* button event */
				button_type = button;
						/* motion, or crossing event, so	*/
			else			/* extract ButtonMask(s) from state	*/
			{
				for (i = 0; i < XtNumber(btn_mappings); i++)
					if ((state & btn_mappings[i].button_mask) != 0)
					{
						button_type = btn_mappings[i].button;
							/* rm it from state */
						state &= ~btn_mappings[i].button_mask;
						break;
					}
			}
			if (button_type == 0)	/* a crossing or motion event with no btn dn */
				return (OL_UNKNOWN_INPUT);

		#define BTNBINDINGS	ve_db->db_stack[i]->btn_bindings
		#define DEFUSED		BTNBINDINGS[j].def.used
		#define MODIFIER	BTNBINDINGS[j].def.modifier[k]
		#define BUTTON		BTNBINDINGS[j].def.button[k]
		#define OLCMD		BTNBINDINGS[j].ol_event

			ve_db = _AllocPDVEDBInfo(dpy);

			for (i = 0; i < ve_db->db_stack_entries; i++)
				for (j = 0; j < ve_db->db_stack[i]->num_btn_bindings; j++)
					for (k = 0; k < DEFUSED; k++)
						if (MODIFIER == (state & 0xff) &&
							BUTTON == button_type)
							{
		#ifdef DEBUG2B
		 if (button)
			printf ("\tgot %s(%d)\n", BTNBINDINGS[j].name, OLCMD);
		#endif
								return (OLCMD);
							}

		#ifdef DEBUG2B
		 if (button)
			printf ("\tgot unknown\n");
		#endif

			return (OL_UNKNOWN_INPUT);

		#undef BTNBINDINGS
		#undef DEFUSED
		#undef MODIFIER
		#undef BUTTON
		#undef OLCMD
		} /* end of WhatOlBtn */

		static OlVirtualName
		WhatOlKey (Widget w, XEvent *xevent, Boolean textedit_flag)
		{
		#define SORTEDDB	ve_db->db_stack[i]->sorted_key_db
		#define NUMKEYS		SORTEDDB[0].i
		#define KEYBINDINGS	ve_db->db_stack[i]->key_bindings
		#define INDEX		SORTEDDB[index].i

			int		i, index, per;
			KeySym		keysym_return, lower, upper;
			KeySym *	syms;
			KeyCode		min_keycode;
			Modifiers	modifiers;
			OlVirtualName	retval = OL_UNKNOWN_KEY_INPUT;

			modifiers = xevent->xkey.state & ~dont_care_bits &
						~(Button1Mask | Button2Mask | Button3Mask |
					  Button4Mask | Button5Mask);

				/* see Accelerate.c:OlEventToKeyEvent() for detailts */
			syms = XtGetKeysymTable(XtDisplayOfObject(w), &min_keycode, &per);
			syms += (xevent->xkey.keycode - min_keycode) * per;

			if (per > 1 && syms[1] != NoSymbol)
			{
				lower = syms[0];
				upper = syms[1];
			}
			else
				XtConvertCase (XtDisplayOfObject(w), syms[0], &lower, &upper);

			if ((modifiers & ShiftMask) && isascii(upper) && !isalpha(upper))
			{
				keysym_return = upper;
				modifiers &= ~ShiftMask;
			}
			else
				keysym_return = lower;

		#ifdef DEBUG2K
		 printf ("(mod, state, keysym): (0x%x, 0x%x, 0x%x)",
				modifiers, xevent->xkey.state, keysym_return);
		#endif
				if (CanBeBound(textedit_flag, keysym_return, xevent->xkey.state))
				{
				register PerDisplayVEDBInfoPtr	ve_db;

				ve_db = _AllocPDVEDBInfo(XtDisplayOfObject(w));

				for (i = 0; i < ve_db->db_stack_entries; i++)
					if ((index = BinarySearch(
							keysym_return,
							modifiers,
							SORTEDDB,
							(int) NUMKEYS,
							KEYBINDINGS)) != -1)
					{
						retval = KEYBINDINGS[INDEX].ol_event;
						break;
					}
				}
		#ifdef DEBUG2K
		 if (retval == OL_UNKNOWN_KEY_INPUT)
			printf ("\tgot unknown key(%d)\n", retval);
		 else
			printf ("\tgot %s(%d)\n", KEYBINDINGS[INDEX].name, retval);
		#endif
				return (retval);

		#undef SORTEDDB
		#undef NUMKEYS
		#undef KEYBINDINGS
		#undef INDEX
		} /* end of WhatOlKey */



		static OlInputEvent
		DoLookupForTextEdit (
			 Widget		w,
			 XEvent *	event,
			 KeySym *	keysym,
			 String *	buffer,
			 Cardinal *	length,
			 Boolean        textedit_flag,
			 XtPointer	db_flag)	/* OL_CORE_IE, OL_TEXT_IE,
							   OL_DEFAULT_IE, or db ptr */
		{
			OlInputEvent	retval = OL_UNKNOWN_INPUT;
			XEvent    newevent;
			static Buffer *buffer_return = NULL; 
			static WBuffer *wbuffer_return = NULL; 
			static unsigned char compose_buf[BUFFER_SIZE];
			KeySym		keysym_return, save_keysym_return;
			int		length_return;
			Modifiers	num_lock;
			Display		*dpy;
			Window		win;
			Status          status = XLookupNone;
			XIC      ic;
			OlStrRep	rep = ((PrimitiveWidget)w)->primitive.text_format;
			static	Boolean	led_on = False;	/* yet another static! */
			Boolean  is_numeric_key = False;
			unsigned long is_preedit_on = False ;
			static XComposeStatus  compose_return = { 
						(XPointer)compose_buf, 
						0};
			if (XtIsSubclass(w, textEditWidgetClass)) {
						TextEditWidget tw = (TextEditWidget)w;
						ic = ((tw->textedit.ic_id != (OlInputContextID)NULL)
		?
										OlXICOfIC(tw->textedit.ic_id) :
		(XIC)NULL);
				} else
				if (XtIsSubclass(w, textLineWidgetClass)) {
						TextLineWidget tw = (TextLineWidget)w;
						ic = ((tw->textLine.ic_id != (OlInputContextID)NULL)
		?
										OlXICOfIC(tw->textLine.ic_id) :
		(XIC)NULL);
				}
			if(buffer_return == NULL) { /* assume others are NULL too */ 
				buffer_return = 
					(Buffer *)AllocateBuffer(sizeof(char),4*BUFFER_SIZE);
				wbuffer_return =
				(WBuffer *)AllocateBuffer(sizeof(wchar_t),4*BUFFER_SIZE);
			}

			BuildIEDBstack(w, db_flag);

			switch (event->type)
			{
				case KeyPress:
			/*
			 * To conform with 'standard' Xlib, we now need to do our own
			 * processing of NumLock. If this key is down AND the keyevent is
			 * from a keypad key, we assume the user wants unmodified numeric
			 * keypad behaviour and we simply return OL_UNKNOWN_KEY_INPUT.
			 * This bypasses the binding mechanisms for such keys.
			 
			 * Additional Comments by jmk:
			 *		The basic problem is that X*LookupString does not recogonize
			 * the NumLock key. Hence the KeySym, status & the string returned are
			 * not correct. And so, we need to fabricate these .... thru
			 * IsKPKey (for KeySym) & _OlTranslateKPKeySym (for the string)
			 *
			 * The new keyboard spec as proposed in R6 deals with the NumLock
			 * key properly ... and if so we can remove all these hacks !
			 */
			
			/*
			 * A KeyPress event with a KeyCode of zero is used
			 * exclusively as a signal that an input method has
			 * composed input which can be returned by XmbLookupString
			 * and XwcLookupString. No other use is made of a KeyPress
			 * event with keycode zero. Such event may be generated
			 * by a front end or a back end input method.
			 */
				if(event->xkey.keycode != 0) {

						num_lock = _OlGetModifierBinding(
									event->xany.display,
							NumLock);
       		 	if ((num_lock & event->xkey.state) &&
               	        	IsKPKey(event->xany.display,
					event->xkey.keycode, &keysym_return)) {

					is_numeric_key = True;
					save_keysym_return = keysym_return;
		    /*
		     * Special case the KP Enter key so that we don't get
		     * a ^M displayed
		     */
			    	if (keysym_return == XK_KP_Enter)
						retval = OL_RETURN;
			    	else 
			        	retval = OL_UNKNOWN_KEY_INPUT;
				} else 		
      			    retval = WhatOlKey(w, event, textedit_flag);
		}

		if(ic != (XIC)NULL) {	/* Input method is present .. */

			wchar_t wc;

		   do { /* do until status is not over flow */
			switch(rep) {
			case OL_MB_STR_REP:
				length_return = XmbLookupString(ic,
					(XKeyPressedEvent *)event,
							buffer_return->p,
						buffer_return->size,
							&keysym_return,
							&status);
				if(status == XBufferOverflow)
					GrowBuffer(buffer_return,
							length_return -
							buffer_return->size+1);
				else
					buffer_return->p[length_return] = '\0';
				break;
			case OL_WC_STR_REP:
				length_return = XwcLookupString(ic,
					(XKeyPressedEvent *)	event,
							(wchar_t *)
							wbuffer_return->p,
						wbuffer_return->size,
							&keysym_return,
							&status);
				if(status == XBufferOverflow)
					GrowBuffer((Buffer *)wbuffer_return,
							length_return -
							wbuffer_return->size+1);
				else
				    wbuffer_return->p[length_return] = L'\0';
				break;
			  }
			} while(status == XBufferOverflow);

			/* Since X*LookupString is unable to process the NumLock
			 * key appropriately, it returns XLookupKeySym currently.
			 * If this is fixed, it would return XLookupBoth & our
			 * hack would be bypassed !
			 */
			/* If we are in preedit_mode, we wash our hands off !
			 * Note that we are using a XIM extension ...
			 */
			if ((XGetICValues(ic, 
					XNExtXimp_Conversion,
					&is_preedit_on, 
					NULL) == NULL) && is_preedit_on)
					is_numeric_key = False;

			switch(status) {
				case XLookupKeySym:
					break;
				case XLookupBoth:
					if(length_return == 1) {
						if(rep == OL_WC_STR_REP)
       			                                 wc = *((wchar_t *)
								wbuffer_return->p);
                                		else
                                    			mbtowc(&wc, (char *)
								buffer_return->p,1);
 
						if (is_preedit_on) {
						    if (iswprint(wc))
							retval = OL_UNKNOWN_KEY_INPUT;
						}
						else {
                                 		    if(!(event->xkey.state & Mod1Mask) &&  
 						       !(event->xkey.state & Mod2Mask) &&  
 						       !(event->xkey.state & Mod3Mask) &&  
 						       !(event->xkey.state & Mod4Mask) &&  
 						       !(event->xkey.state & Mod5Mask) &&  
 						       !(event->xkey.state & ControlMask) &&  iswprint(wc))
                                        		retval = OL_UNKNOWN_KEY_INPUT;
						}
					}
					break;
				case XLookupChars:
					retval = OL_UNKNOWN_KEY_INPUT;
					break;
				default:
					break;
			}
				
		} else  {	/* Input method is NOT present */
			length_return = XLookupString(
					(XKeyEvent *)event,
					buffer_return->p,
					buffer_return->size,
					&keysym_return,
					&compose_return);
			
			buffer_return->p[length_return] = '\0';
			if(rep == OL_WC_STR_REP) { 
				length_return = (int)
					mbstowcs((wchar_t *)wbuffer_return->p, 
					buffer_return->p, length_return);
					wbuffer_return->p[length_return] = L'\0';
			}
		}

		if (is_numeric_key) {
		/* Fabricate the correct KeySym & string_to_be_printed */
			keysym_return = save_keysym_return;
			length_return = _OlTranslateKPKeySym(keysym_return,
							buffer_return->p, buffer_return->size);
			buffer_return->p[length_return] = '\0';
			if(rep == OL_WC_STR_REP) {
				(void)mbstowcs((wchar_t *) wbuffer_return->p, 
								buffer_return->p,
								length_return);
				wbuffer_return->p[length_return] = L'\0';
			}
		}

  		if (keysym != NULL)
      		*keysym = keysym_return;
   		if (buffer != NULL)
	 		*buffer = (rep == OL_MB_STR_REP ||
					rep == OL_SB_STR_REP ? 
						(String)buffer_return->p:
						(String)wbuffer_return->p);
   		if (length != NULL)
       		*length = length_return;
   		if (IsDampableKey(textedit_flag, keysym_return) &&
			status == XLookupKeySym)
       		while (XCheckIfEvent(
          				XtDisplayOfObject(w),
					&newevent, IsSameKeyEvent,
					(char *) event))
           			;

     	  break;
   	    case ButtonPress:
   	    case ButtonRelease:
			/* WhatOlBtn returns OL_UNKNOWN_INPUT because*/
			/* crossing/motion aren't the button events  */

		retval = WhatOlBtn (	event->xany.display,
					event->xbutton.state,
					event->xbutton.button);
		if (retval == OL_UNKNOWN_INPUT)
      			retval = OL_UNKNOWN_BTN_INPUT;
      		break;
	    case EnterNotify:
		 break;
	    case LeaveNotify:
		 break;
	    case MotionNotify:
		 break;
   	    default:
      		break;
   	}
	return (retval);
}
 /* end of DoLookup */

