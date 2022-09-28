#pragma ident   "@(#)ListPane.c	302.43    97/03/26 lib/libXol SMI" /* scrolllist:src/ListPane.c 1.141
*/

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


#include <stdio.h>
#include <libintl.h>
#include <widec.h>

#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/ListPaneP.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShellP.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrollingP.h>
#include <Xol/TextFieldP.h>


/**************************forward*declarations***************************
 *
 * Forward function definitions listed by category:
 *		1. Private functions
 *		2. Class   functions
 *		3. Action  functions
 *		4. Public  functions
 *
 */
						/* private procedures */
static OlListToken ApplAddItem (
			Widget, OlListToken, OlListToken, OlListItem);
static void	ApplDeleteItem (Widget, OlListToken);
static void	ApplEditClose (Widget);
static void	ApplEditOpen (Widget, Boolean, OlListToken);
static void	ApplTouchItem (Widget, OlListToken);
static void	ApplUpdateView (Widget, Boolean);
static void	ApplViewItem (Widget, OlListToken);
static void	BuildClipboard (ListPaneWidget);
static void	BuildDeleteList (ListPaneWidget, OlListDelete *);
static void	ClearSelection (ListPaneWidget);
static void	CreateGCs (ListPaneWidget);
static void	CutOrCopy (ListPaneWidget, Boolean);
static void	DeleteNode (ListPaneWidget, int);
static void	DrawItem (ListPaneWidget, int, Boolean);
static void	FreeList (ListPaneWidget);
static int	InsertNode (ListPaneWidget, int);
static void	MaintainWidth (ListPaneWidget, OlListItem *);
static void	ScrollView (ListPaneWidget);
static void	Search (ListPaneWidget, char);
static void	SelectOrAdjustItem (
			ListPaneWidget, int, OlVirtualName, Time, XEvent *, int);
static void	ToggleItemState (ListPaneWidget, int);
static void	UpdateSBar (ListPaneWidget);
static void 	UpdateSuperCaret(ListPaneWidget w);

static OlgxTextLbl * Item2TextLbl (ListPaneWidget, OlListItem *);
static OlgxPixmapLbl * Item2PixmapLbl(ListPaneWidget, OlListItem *);

						/* class procedures */
static Boolean	AcceptFocus(Widget w, Time *time);
static Boolean	Activate (Widget, OlVirtualName, XtPointer);
static void	Destroy(Widget w);
static void	HighlightHandler (Widget, OlDefine);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void 	ListPaneQuerySCLocnProc(const   Widget          w,
                        const   Widget          supercaret,
                        const   Dimension       width,
                        const   Dimension       height,
                        Cardinal        *const  scale,
                        SuperCaretShape *const  shape,
                        Position        *const  x_center,
                        Position        *const  y_center);
static void     Resize(Widget w);
static void	Redisplay(Widget w, XEvent *event, Region region);
static Widget	RegisterFocus (Widget);
static Widget	TraversalHandler (Widget, Widget, OlVirtualName, Time);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static void GetValuesHook(Widget current, ArgList args, Cardinal *num_args);
						/* action procedures */
static void	AutoScrollCB(XtPointer closure, XtIntervalId *id);
static void	ButtonDown(Widget widget, OlVirtualEvent ve);
static void	ButtonMotion(Widget widget, OlVirtualEvent ve);
static void	ButtonUp(Widget widget, OlVirtualEvent ve);
static Boolean	ConvertSelection(Widget w, Atom *selection, Atom *target, Atom *type_return, XtPointer *value_return, long unsigned int *length_return, int *format_return);
static void	KeyDown(Widget w, OlVirtualEvent ve);
static void	LoseSelection(Widget w, Atom *selection);
static void	SelectionDone(Widget w, Atom *selection, Atom *target);
static void	SBarMovedCB(Widget sbw, XtPointer closure, XtPointer callData);
static void copy_olstring(OlStrRep tf, OlStr * ol_str);
static void UpdateItemFocus(ListPaneWidget w, int node, Boolean item_has_focus);

						/* public procedures */

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

/* Since there is only one selection at any one time, it is satisfactory
   to have a single pointer for the contents defined here local to this
   file (a single "array" for all list widgets in the process space).
 */
static String	Clipboard;
static Cardinal	ClipboardCnt;		/* byte size */

/* Global List: a single structure can be shared between all lists in
   the process space.  Another reason for declaring TheList (below) so
   that it's visible in this file is for OlListItemPointer().  R1.0 of
   Open Look specified OlListItemPointer() as a macro taking a single
   (token) argument.  With dynamic storage, the list structure would
   have to be passed in as well (via the widget: (w, token)).  Having
   the list visible to the procedure gets around this problem.
 */

typedef struct {
    OlListItem	item;
    int		indx;
	Boolean item_sensitive;  /* new fields */
	Boolean item_current;
	OlFont  item_font;
	Pixmap  item_pixmap;
	Pixel   item_foreground;
	Pixel   item_background;
	XtPointer user_data;
} Node;

typedef struct _List {
    Node *	nodes;			/* array of nodes */
    Cardinal	num_nodes;		/* number of elements */
    Cardinal	num_slots;		/* number of allocated slots */
    _OlArrayRec	free;			/* array of free elements */
} * List;

static struct _List TheList = { 0 };

#define SLISTW(w)	( XtParent(w) )
#define NULL_OFFSET	-1
#define PANEW(w)	( (ListPaneWidget)(w) )
#define PANEPTR(w)	( &(PANEW(w)->list_pane) )
#define PRIMPTR(w)	( &(((PrimitiveWidget)(w))->primitive) )
#define SBAR(w)		( _OlListSBar(SLISTW(w)) )

#define THE_LIST		( &TheList )
#define HEADPTR(w)		( &(PANEPTR(w)->head) )
#define NODES			( TheList.nodes )
#define AboveView(w, node)	( IndexOf(node) < TopIndex(w) )
#define AfterBottom(w)		( NumItems(w) - TopIndex(w) - ViewHeight(w) )
#define BelowView(w, node)	( IndexOf(node) >= \
					(TopIndex(w) + ViewHeight(w)) ) 
#define EmptyList(w)		( NumItems(w) == 0 )
#define FocusItem(w)		( PANEPTR(w)->focus_item )
#define FontHeight(font)	( font->max_bounds.ascent + \
					font->max_bounds.descent )
#define FontWidth(font)		( font->max_bounds.rbearing - \
					font->max_bounds.lbearing )
#define HeadOffset(w)		( HEADPTR(w)->offset )
#define IndexOf(node)		( NodePtr(node)->indx )
#define InView(w, node)		( !AboveView(w, node) && \
					!BelowView(w, node) )
#define IsCurrent(node)		( ItemPtr(node)->attr & OL_B_LIST_ATTR_CURRENT )
#define IsFocused(node)		( ItemPtr(node)->attr & OL_B_LIST_ATTR_FOCUS )
#define IsSelected(node)	( ItemPtr(node)->attr & OL_B_LIST_ATTR_SELECTED)
#define IsTop(w, indx)		( TopIndex(w) == (indx) )
#define ItemPtr(node)		( &(NodePtr(node)->item) )
#define Next(w, node)		( OffsetNode(w, node, 1) )
#define NodePtr(node)		( &(NODES[node]) )
#define NumItems(w)		( HEADPTR(w)->offsets.num_elements )
#define OffsetNode(w, node, offset) ( OffsetOf(w, IndexOf(node) + (offset)) )
#define OffsetOf(w, indx)	( (int)HEADPTR(w)->offsets.array[indx] )
#define PaneIndex(w, node)	( IndexOf(node) - TopIndex(w) )
#define Prev(w, node)		( OffsetNode(w, node, -1) )
#define SearchItem(w)		( PANEPTR(w)->search_item )
#define TextField(w)		( PANEPTR(w)->text_field )
#define Top(w)			( PANEPTR(w)->top_item )
#define Last(w)			( PANEPTR(w)->last_item )
#define TopIndex(w)		( IndexOf(Top(w)) )
#define ValidSListWidget(w)	( ((w) != NULL) && \
				    XtIsSubclass(w, scrollingListWidgetClass) )
#define ViewHeight(w)		( PANEPTR(w)->actualViewHeight )

/* Function aliases: */
#define FreeGCs(w)		{ XtReleaseGC(w, PANEPTR(w)->gc_normal);    \
				  XtReleaseGC(w, PANEPTR(w)->gc_inverted);  \
                                  OlgxDestroyAttrs((w),			    \
						  PANEPTR(w)->attr_normal); \
                                  OlgxDestroyAttrs((w),                     \
                                                  PANEPTR(w)->attr_focus); \
				}

#define MakeFocused(node)	ItemPtr(node)->attr |= OL_B_LIST_ATTR_FOCUS
#define MakeUnfocused(node)	ItemPtr(node)->attr &= ~OL_B_LIST_ATTR_FOCUS
#define SetInputFocus(w, time)	OlSetInputFocus(w, RevertToNone, time)
#define LatestTime(w)		XtLastTimestampProcessed(XtDisplay(w))

#define CurrentNotify(w, node)	\
	    if (XtHasCallbacks(SLISTW(w), XtNuserMakeCurrent) == \
	      XtCallbackHasSome) \
		XtCallCallbacks(SLISTW(w), XtNuserMakeCurrent, (XtPointer)node);

#define ItemCurrentCallback(w, cbdata)	\
	    if (XtHasCallbacks(SLISTW(w), XtNitemCurrentCallback) == \
	      XtCallbackHasSome) \
		XtCallCallbacks(SLISTW(w), XtNitemCurrentCallback, (XtPointer)cbdata);

#define ItemNotCurrentCallback(w, cbdata)	\
	    if (XtHasCallbacks(SLISTW(w), XtNitemNotCurrentCallback) == \
	      XtCallbackHasSome) \
		XtCallCallbacks(SLISTW(w), XtNitemNotCurrentCallback, (XtPointer)cbdata);

#define MultiClickCallback(w, cbdata)	\
	    if (XtHasCallbacks(SLISTW(w), XtNmultiClickCallback) == \
	      XtCallbackHasSome) \
		XtCallCallbacks(SLISTW(w), XtNmultiClickCallback, (XtPointer)cbdata);

#define ITEMSENSITIVE(w, node) \
		(XtIsSensitive((Widget)w) && (PANEPTR(w)->slist_mode != OL_NONE) && \
						!(NodePtr(node)->item_sensitive)) ? False : True 

#ifdef DEBUG
    Boolean SLdebug = False;
#   define DPRINT(x)  if (SLdebug == True) (void)fprintf x
#else
#   define DPRINT(x)
#endif

#define INITIAL_INDEX	-1
#define NO_MOTION	0
#define SCROLL_UP	1
#define SCROLL_DOWN	( -SCROLL_UP )

/* ITEM SPACING:
 *
 * starting from the top (left) of the pane:
 *	margin surrounding entire list of items		(ITEM_MARGIN)
 *	(beginning of item)
 *	beginning of item to current-item border	(ITEM_BORDER)
 *	current-item border				(ITEM_BORDER)
 *	current-item border to label			(ITEM_BORDER)
 *	(label)
 *	same for bottom (right)
 */
					/* item spacing (in points) */
#define ITEM_MARGIN		4
#define ITEM_BORDER		1

	/*
	 * padding: constants to define padding around the label.
	 *	padding + label width (height) = item width (height)
	 *
	 * in addition, since HORIZ_PAD includes the left & right margins:
	 *	PaneWidth = ItemWidth  (see macros below):
	 */
#define VERT_PADDING	(2*ITEM_BORDER + ITEM_BORDER)
#define HORIZ_PADDING	(2*VERT_PADDING + 2*ITEM_MARGIN)

			/* conversion macros: convert 'value' to pixels. */
#define ConvertVert(w, value) \
		(OlScreenPointToPixel(OL_VERTICAL, value, XtScreen(w)))
#define ConvertHoriz(w, value) \
		(OlScreenPointToPixel(OL_HORIZONTAL, value, XtScreen(w)))

/* Calculate width and height of item including padding */

#define ItemWidth(w, item_width) ( (item_width) + PANEPTR(w)->horiz_pad )
#define ItemHeight(w)	( PANEPTR(w)->max_height + PANEPTR(w)->vert_pad )

#ifdef sun
/* Define a macro to calculate the preferred min/max widths of the ListPane
 * once we subtract the width of the scrollbar and it's xOffset from
 * the ScrollingList preferred min/max widths.
 */
#define PrefListPaneMaxWidth(w) ((int)(PANEPTR(w)->pref_max_width - PANEPTR(w)->sb_width_pad))
#define PrefListPaneMinWidth(w) ((int)(PANEPTR(w)->pref_min_width - PANEPTR(w)->sb_width_pad))
#endif

/* calculate an index into the pane based on 'y' value */
#define IndexFromY(w, y) ( (Dimension)((y) - PANEPTR(w)->vert_margin) / \
						(Dimension)ItemHeight(w) )
#define HasGlyph(item)  ((item->label_type == OL_IMAGE) ||      \
                         (item->label_type == OL_BOTH))
#define HasString(item) ((item->label_type == OL_STRING) ||     \
                         (item->label_type == OL_BOTH))
#define Align(w)        (PANEPTR(w)->align)
#define TextPosition(w) (PANEPTR(w)->position)

/* Resource shared with textField */
static MaskArg TFieldArgs[] = {
    { XtNfont, NULL, OL_SOURCE_PAIR },
    { XtNfontColor, NULL, OL_SOURCE_PAIR },
    { XtNforeground, NULL, OL_SOURCE_PAIR },
    };

#define BYTE_OFFSET	XtOffsetOf(ListPaneRec, list_pane.dyn_flags)
static _OlDynResource dyn_res[] = {
#ifdef COLORED_LIKE_TEXT
{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_LISTPANE_BG, NULL },
#endif
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0, XtRString,
	XtDefaultForeground }, BYTE_OFFSET, OL_B_LISTPANE_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/***********************widget*translations*actions***********************
 *
 * Translations and Actions
 *
 */
#ifdef XT_AUGMENT_WORKS_RIGHT

/* the GenericTranslations must be augmented with BtnMotion */

static char extendedTranslations[] = "\
    #augment				\n\
    <BtnMotion>	:	OlAction()	\n";	/* see MotionHandler */

#else

/* everything but BtnMotion is taken from GenericTranslations (Action.c) */

static char extendedTranslations[] = "\
    <FocusIn>	:	OlAction()	\n\
    <FocusOut>	:	OlAction()	\n\
    <Key>	:	OlAction()	\n\
    <BtnDown>	:	OlAction()	\n\
    <BtnUp>	:	OlAction()	\n\
    <BtnMotion>	:	OlAction()	\n";	/* see MotionHandler */

#endif

static OlEventHandlerRec event_procs[] = {
    { ButtonPress,	ButtonDown },
    { ButtonRelease,	ButtonUp },
    { KeyPress,		KeyDown },		/* for MENUKEY */
    { MotionNotify,	ButtonMotion },
};

/****************************widget*resources*****************************
 *
 * ListPane Resources
 */

#define OFFSET(field)	XtOffsetOf(ListPaneRec, field)
#define POFFSET(field)	OFFSET(list_pane.field)

static XtResource resources[] = {
					/* core resources: */
    { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
	OFFSET(core.border_width), XtRImmediate, (XtPointer) 1
    },
#ifdef COLORED_LIKE_TEXT
    { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
	OFFSET(core.background_pixel), XtRString, XtDefaultBackground
    },
#endif
    { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
	OFFSET(primitive.font_color), XtRString, XtDefaultForeground
    },
					/* list pane resources: */
	{ XtNapplAddItem, XtCApplAddItem, XtRFunction,
		sizeof (OlApplAddItemProc), POFFSET(applAddItem),
		XtRFunction, NULL
	},
	{ XtNapplDeleteItem, XtCApplDeleteItem, XtRFunction,
		sizeof (OlApplDeleteItemProc), POFFSET(applDeleteItem),
		XtRFunction, NULL
	},
	{ XtNapplEditClose, XtCApplEditClose, XtRFunction,
		sizeof (OlApplEditCloseProc), POFFSET(applEditClose), 
		XtRFunction, NULL
	},
	{ XtNapplEditOpen, XtCApplEditOpen, XtRFunction,
		sizeof (OlApplEditOpenProc), POFFSET(applEditOpen),
		XtRFunction, NULL
	},
	{ XtNapplTouchItem, XtCApplTouchItem, XtRFunction,
		sizeof (OlApplTouchItemProc), POFFSET(applTouchItem),
		XtRFunction, NULL
	},
	{ XtNapplUpdateView, XtCApplUpdateView, XtRFunction,
		sizeof (OlApplUpdateViewProc), POFFSET(applUpdateView),
		XtRFunction, NULL
	},
	{ XtNapplViewItem, XtCApplViewItem, XtRFunction,
		sizeof (OlApplViewItemProc), POFFSET(applViewItem),
		XtRFunction, NULL
	},
    { XtNrecomputeWidth, XtCRecomputeWidth, XtRBoolean, sizeof(Boolean),
	POFFSET(recompute_width), XtRImmediate, (XtPointer)True
    },
    { XtNselectable, XtCSelectable, XtRBoolean, sizeof(Boolean),
	POFFSET(selectable), XtRImmediate, (XtPointer)False
    },
    { XtNtextField, XtCTextField, XtRWidget, sizeof(Widget),
	POFFSET(text_field), XtRWidget, NULL
    },
    { XtNviewHeight, XtCViewHeight, XtRCardinal, sizeof(Cardinal),
	POFFSET(view_height), XtRImmediate, (XtPointer) 0
    },
#ifdef sun
    { XtNprefMaxWidth, XtCPrefMaxWidth, XtRDimension, sizeof(Dimension),
        POFFSET(pref_max_width), XtRImmediate, (XtPointer) 0
    },
    { XtNprefMinWidth, XtCPrefMinWidth, XtRDimension, sizeof(Dimension),
        POFFSET(pref_min_width), XtRImmediate, (XtPointer) 0
    },
#endif
    { XtNspace, XtCSpace, XtRDimension, sizeof(Dimension),
        POFFSET(space), XtRImmediate, (XtPointer) 4
    },
    { XtNalign, XtCAlign, XtRBoolean, sizeof(Boolean),
        POFFSET(align), XtRString, "True"
    },
    { XtNitemHeight, XtCItemHeight, XtRDimension, sizeof(Dimension),
        POFFSET(item_height), XtRImmediate, (XtPointer) 0
    },
    { XtNposition, XtCPosition, XtROlDefine, sizeof(OlDefine),
        POFFSET(position), XtRImmediate, (XtPointer)((OlDefine)OL_LEFT)
    },
    { XtNscrollingListMode, XtCScrollingListMode, XtROlDefine, sizeof(OlDefine),
        POFFSET(slist_mode), XtRImmediate, (XtPointer)((OlDefine)OL_NONE)
    },
    { XtNfirstViewableItem, XtCFirstViewableItem, XtRPointer, sizeof(XtPointer),
        POFFSET(first_item), XtRImmediate, (XtPointer)NULL
    },
    { XtNviewableItems, XtCViewableItems, XtRPointer, sizeof(XtPointer),
        POFFSET(viewable_items), XtRImmediate, (XtPointer)NULL
    },
    { XtNscrollingListItems, XtCScrollingListItems, XtRPointer, sizeof(XtPointer),
        POFFSET(scrolling_list_items), XtRImmediate, (XtPointer)NULL
    },
    { XtNlastViewableItem, XtCLastViewableItem, XtRPointer, sizeof(XtPointer),
        POFFSET(last_item), XtRImmediate, (XtPointer)NULL
    },
    { XtNnumItems, XtCNumItems, XtRInt, sizeof(int),
        POFFSET(num_items), XtRImmediate, (XtPointer)0
    },
    { XtNcurrentItems, XtCCurrentItems, XtRPointer, sizeof(XtPointer),
        POFFSET(current_items), XtRImmediate, (XtPointer)0
    },
    { XtNnumCurrentItems, XtCNumCurrentItems, XtRInt, sizeof(int),
        POFFSET(num_current_items), XtRImmediate, (XtPointer)0
    },
};

#undef OFFSET
#undef POFFSET

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

ListPaneClassRec listPaneClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &primitiveClassRec,
    /* class_name		*/	"ListPane",
    /* widget_size		*/	sizeof(ListPaneRec),
    /* class_initialize		*/	NULL,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	False,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	False,
    /* compress_exposure	*/	True,
    /* compress_enterleave	*/	True,
    /* visible_interest		*/	False,
    /* destroy			*/	Destroy,
#ifdef sun
    /* resize			*/	Resize,
#else
    /* resize			*/	XtInheritResize,
#endif
    /* expose			*/	Redisplay,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	GetValuesHook,
    /* accept_focus		*/	AcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	extendedTranslations,
    /* query geometry		*/	NULL
  },
  { /* primitive_class fields */
    /* reserved			*/	NULL,
    /* highlight_handler	*/	HighlightHandler,
    /* traversal_handler	*/	TraversalHandler,
    /* register_focus		*/	RegisterFocus,
    /* activate			*/	Activate,
    /* event_procs		*/	event_procs,
    /* num_event_procs		*/	XtNumber(event_procs),
    /* version			*/	OlVersion,
    /* extension		*/	NULL,
    /* dyn_data			*/	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc		*/	NULL,
    /* query_sc_locn_proc       */   	ListPaneQuerySCLocnProc,
  },
};

WidgetClass listPaneWidgetClass = (WidgetClass)&listPaneClassRec;

/*************************************************************************
 * 
 * Public Function
 *
 *************************************************************************/

void
OlSlistTouchItem(Widget widget, OlSlistItemPtr item)
{
	GetToken();
	ApplTouchItem (widget, item);
	ReleaseToken();
}

void
OlSlistUpdateView(Widget widget, Boolean ok)
{
	GetToken();
	ApplUpdateView (widget, ok);
	ReleaseToken();
}

void
OlSlistViewItem(Widget widget, OlSlistItemPtr item)
{
	GetToken();
	ApplViewItem (widget, item);
	ReleaseToken();
}

Boolean
OlSlistIsValidItem(Widget widget, OlSlistItemPtr item)
{
ListPaneWidget w;
Boolean result = False;
int num_items, i;

	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistIsValidItem- widget is not scrollingListWidget"));
	   ReleaseToken();
	   return False;
    }
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistIsValidItem- NULL item"));
	   ReleaseToken();
	   return False;
    }
    w = PANEW(_OlListPane(widget));	/* get list pane widget */
	num_items = NumItems(w);

	for (i = 0; i < num_items; i++) {
	   if ((int)PANEPTR(w)->head.offsets.array[i] == (int)item) { /* found it */
		  result = True;
		  break;
	   }
	}

	ReleaseToken();
	return result;
}

OlSlistItemPtr
OlSlistGetNextItem(Widget widget, OlSlistItemPtr item)
{
ListPaneWidget w;
int retval;

	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetNextItem- widget is not scrollingListWidget"));
	   return (OlSlistItemPtr)NULL;
    }
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetNextItem- NULL item"));
	   return (OlSlistItemPtr)NULL;
    }
	if (!OlSlistIsValidItem(widget, item)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistGetNextItem- not a valid item"));
	   return (OlSlistItemPtr)NULL;
	}
    w = PANEW(_OlListPane(widget));	/* get list pane widget */
	if ((int)IndexOf((int)item) < (int)(NumItems(w) - 1))
	   retval = Next(w, (int)item);
	else
	   retval = (int)NULL;

	ReleaseToken();
	return (OlSlistItemPtr)retval;
}

OlSlistItemPtr
OlSlistGetPrevItem(Widget widget, OlSlistItemPtr item)
{
ListPaneWidget w;
int retval;

	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetPrevItem- widget is not scrollingListWidget"));
	   ReleaseToken();
	   return (OlSlistItemPtr)NULL;
    }
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetPrevItem- NULL item"));
	   ReleaseToken();
	   return (OlSlistItemPtr)NULL;
    }
	if (!OlSlistIsValidItem(widget, item)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistGetPrevItem- not a valid item"));
	   ReleaseToken();
	   return (OlSlistItemPtr)NULL;
	}
    w = PANEW(_OlListPane(widget));	/* get list pane widget */
	if ((int)IndexOf((int)item) > 0)
	   retval = Prev(w, (int)item);
	else
	   retval = (int)NULL;

	ReleaseToken();
	return (OlSlistItemPtr)retval;
}

OlStr 
OlSlistGetItemLabel(Widget widget, OlSlistItemPtr item)
{
OlStr retval;

	GetToken();
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetItemLabel- NULL item"));
	   ReleaseToken();
	   return (OlStr)NULL;
    }
	retval = ItemPtr((int)item)->label;
	ReleaseToken();
	return retval;
}

XImage *
OlSlistGetItemImage(Widget widget, OlSlistItemPtr item)
{
XImage *retval;

	GetToken();
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetItemImage- NULL item"));
	   ReleaseToken();
	   return (XImage *)NULL;
    }
	retval= ItemPtr((int)item)->glyph;
	ReleaseToken();
	return retval;
}

OlDefine
OlSlistGetItemType(Widget widget, OlSlistItemPtr item)
{
OlDefine retval;

	GetToken();
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetItemType- NULL item"));
	   ReleaseToken();
	   return (OlDefine)NULL;
    }
	retval= ItemPtr((int)item)->label_type;
	ReleaseToken();
	return retval;
}

OlDefine
OlSlistGetMode(Widget widget)
{
OlDefine retval;

	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetMode- widget is not scrollingListWidget"));
	   ReleaseToken();
	   return (OlDefine)NULL;
    }

	retval= PANEPTR(PANEW(_OlListPane(widget)))->slist_mode;
	ReleaseToken();
	return retval;
}

Boolean
OlSlistIsItemCurrent(Widget widget, OlSlistItemPtr item)
{
Boolean retval;

	GetToken();
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistIsItemCurrent- NULL item"));
	   ReleaseToken();
	   return False;
    }
	retval= NodePtr((int)item)->item_current;
	ReleaseToken();
	return retval;
}

Boolean
OlSlistGetItemSensitivity(Widget widget, OlSlistItemPtr item)
{
Boolean retval;

	GetToken();
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetItemSensitivity- NULL item"));
	   ReleaseToken();
	   return False;
    }
	retval = NodePtr((int)item)->item_sensitive;
	ReleaseToken();
	return retval;
}

XtPointer
OlSlistGetItemUserData(Widget widget, OlSlistItemPtr item)
{
XtPointer retval;

	GetToken();
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetItemSensitivity- NULL item"));
	   ReleaseToken();
	   return (XtPointer)NULL;
    }
	retval = NodePtr((int)item)->user_data;
	ReleaseToken();
	return retval;
}

OlSlistItemPtr
OlSlistAddItem(Widget widget, OlSlistItemAttrsPtr item_attr,
					   OlSlistItemPtr refNode)
{
ListPaneWidget	w;
int	node;		/* index of new node */
static Boolean item_cur = False;

	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistAddItem- widget is not scrollingListWidget"));
	   ReleaseToken();
	   return (OlSlistItemPtr)NULL;
    }

    if ((item_attr->label_type != OL_STRING) &&
        (item_attr->label_type != OL_IMAGE)  &&
        (item_attr->label_type != OL_BOTH)) {
	   OlWarning(dgettext(OlMsgsDomain,
	    	"ScrollingList: OlSlistAddItem- illegal label_type"));
	   ReleaseToken();
	   return (OlSlistItemPtr)NULL;
    }

    w = PANEW(_OlListPane(widget));	/* get list pane widget */

    /*
     * Bump the start_index if you insert an item
     * below of the current start_index.
     *
     * If the refNode is NULL the item is getting added
     * above everything so the start_index is ok.
     *
     * Prev_index should be the same as the start_index.
     */
    if ((refNode != 0) &&
	(IndexOf((int)refNode) <= PANEPTR(w)->start_index)) {
	    ++(PANEPTR(w)->start_index);
	    PANEPTR(w)->prev_index = PANEPTR(w)->start_index;
    }

    node = InsertNode(w, (int)refNode);
	ItemPtr(node)->attr = 0;
	ItemPtr(node)->label_type = OL_STRING;
	ItemPtr(node)->mnemonic = '\0';
	ItemPtr(node)->label = (OlStr) NULL;
	ItemPtr(node)->glyph = (XImage *) NULL;
	NodePtr(node)->item_sensitive = True;
	NodePtr(node)->user_data = (XtPointer) NULL;
	/* by default have the first item current in exclusive mode */
	if ((PANEPTR(w)->slist_mode == OL_EXCLUSIVE) && 
		     !PANEPTR(w)->current_item) {
	   PANEPTR(w)->current_item = node;
	   ItemPtr(node)->attr |= OL_LIST_ATTR_CURRENT;
	   NodePtr(node)->item_current = True; 
	   item_cur = True;
	}
	else 
	   NodePtr(node)->item_current = False;

	if (item_attr->flags & OlItemLabelType)
		ItemPtr(node)->label_type = item_attr->label_type;
	if (item_attr->flags & OlItemLabel) {
		ItemPtr(node)->label = item_attr->item_label;
    	copy_olstring(PRIMPTR(w)->text_format, &(ItemPtr(node)->label));
	}
	if (item_attr->flags & OlItemImage) 
		ItemPtr(node)->glyph = item_attr->item_image;
	if (item_attr->flags & OlItemMnemonic) 
		ItemPtr(node)->mnemonic = item_attr->item_mnemonic;
	if ((item_attr->flags & OlItemCurrent) && (item_attr->item_current)) {
		Boolean set_sel = False;

		if (PANEPTR(w)->slist_mode == OL_NONEXCLUSIVE) {
		   if (PANEPTR(w)->num_current_items > 0) {
		      PANEPTR(w)->current_items = (int *) XtRealloc(
		            (char *)PANEPTR(w)->current_items, 
		            (PANEPTR(w)->num_current_items + 1) /* allocate 1 more */
		            * sizeof(int));
		      PANEPTR(w)->current_items[PANEPTR(w)->num_current_items] = node;
		      PANEPTR(w)->num_current_items++;
		   }
		   else {
		      PANEPTR(w)->current_items = (int *) XtMalloc(sizeof(int));
		      PANEPTR(w)->current_items[PANEPTR(w)->num_current_items] = node;
		      PANEPTR(w)->num_current_items++;
		   }
		   set_sel = True;
		}
		else if (PANEPTR(w)->slist_mode == OL_EXCLUSIVE && item_cur) 
		   set_sel = True;
		else if (PANEPTR(w)->slist_mode == OL_EXCLUSIVE_NONESET &&
				 !PANEPTR(w)->current_item) 
		   set_sel = True;

		if ((PANEPTR(w)->slist_mode == OL_EXCLUSIVE) && set_sel && item_cur) { 
			 /* means application has set an item current */
		   int cur_node = PANEPTR(w)->current_item;
		   ItemPtr(cur_node)->attr &= ~OL_LIST_ATTR_CURRENT;
		   NodePtr(cur_node)->item_current = False; 
		   item_cur = False;
		}

		if (set_sel) {
		   PANEPTR(w)->current_item = node;
		   ItemPtr(node)->attr |= OL_LIST_ATTR_CURRENT;
		   NodePtr(node)->item_current = item_attr->item_current;
		}
	}

	if (item_attr->flags & OlItemSensitive) {
		OlDefine mode = OlSlistGetMode(widget);

		if (((mode == OL_EXCLUSIVE) || (mode == OL_EXCLUSIVE_NONESET)) 
		         && NodePtr(node)->item_current && !item_attr->item_sensitive) {
		   item_cur = False;
	   	   PANEPTR(w)->current_item = 0;
		   ItemPtr(node)->attr &= ~OL_LIST_ATTR_CURRENT;
		   NodePtr(node)->item_current = False;
		   NodePtr(node)->item_sensitive = item_attr->item_sensitive;
		}
		else 
		   NodePtr(node)->item_sensitive = item_attr->item_sensitive;
	}
	if (item_attr->flags & OlItemUserData)
		NodePtr(node)->user_data = item_attr->user_data;

    if (ItemPtr(node)->mnemonic != '\0')
		_OlAddMnemonic((Widget)w, (XtPointer)node, item_attr->item_mnemonic);

    MaintainWidth(w, &(NodePtr(node)->item));

    /* If this is the 1st item in the list or inserting above view and
	view is unfilled, make it the top_item.
    */
    if ((Top(w) == NULL_OFFSET) ||
            (AboveView(w, node) && (NumItems(w) <= ViewHeight(w))))
		Top(w) = node;

    /* If we have focus but focus_item is NULL, make this the focus_item.
	This happens when entire list is deleted, then new item added.
    */
    if (PRIMPTR(w)->has_focus && (FocusItem(w) == NULL_OFFSET)) {
		FocusItem(w) = node;
		MakeFocused(FocusItem(w));
    }

    /* # of items increased, recalibrate SBar */
    if (PANEPTR(w)->update_view) {
		UpdateSBar(w);

		if (InView(w, node) && XtIsRealized((Widget)w))
					/* scroll view if appropriate */
			ScrollView(w);
    }
	PANEPTR(w)->num_items = NumItems(w); 

	ReleaseToken();
    return ((OlSlistItemPtr)node );
}

void 
OlSlistDeleteItem(Widget widget, OlSlistItemPtr item)
{
ListPaneWidget w;
int	node;
int num_items;
Boolean		in_view;
List list = THE_LIST;

	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistDeleteItem- widget is not scrollingListWidget"));
	   ReleaseToken();
	   return;
    }

    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistDeleteItem- NULL item"));
	   ReleaseToken();
	   return;
    }
	if (!OlSlistIsValidItem(widget, item)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistDeleteItem- not a valid item"));
	   ReleaseToken();
	   return;
	}

	w = PANEW(_OlListPane(widget));	/* get list pane widget */
    /* safety check for deletion from NULL list */
    if (EmptyList(w)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistDeleteItem- Empty List"));
		ReleaseToken();
		return;
	}

	num_items = NumItems(w);
    node = (int)item;			/* make item an offset into list */
	if (ItemPtr(node)->attr & OL_LIST_ATTR_CURRENT) {
	   switch(PANEPTR(w)->slist_mode) {
	   case OL_EXCLUSIVE:  /* cannot delete current item in this mode */
	        OlWarning(dgettext(OlMsgsDomain,
		        "ScrollingList: OlSlistDeleteItem- Cannot delete current item"));
			ReleaseToken();
	        return;

	   case OL_EXCLUSIVE_NONESET:
			PANEPTR(w)->current_item = 0;
	        break;
	   case OL_NONEXCLUSIVE:
		   if (PANEPTR(w)->num_current_items > 0) {
		   int i;

		   if (node == PANEPTR(w)->current_item)
		      PANEPTR(w)->current_item = 0;
		   for (i = 0; i < PANEPTR(w)->num_current_items; i++) {
			/* First try to find it in the current list */
			  if (PANEPTR(w)->current_items[i] == node) { /* found it */
				/* Is it the last current item ?? */
				 if (PANEPTR(w)->num_current_items == 1) {
				 	XtFree((char *)PANEPTR(w)->current_items);
			   		PANEPTR(w)->num_current_items = 0;
					break; 
				 }

				 { /* Otherwise move everything up one slot */
				 int j;
			        
				 for (j = i; j < (PANEPTR(w)->num_current_items - 1); j++)
					PANEPTR(w)->current_items[j] = 
						            PANEPTR(w)->current_items[j + 1];

			        PANEPTR(w)->current_items = (int *) XtRealloc(
						     (char *)PANEPTR(w)->current_items, 
						     (PANEPTR(w)->num_current_items - 1) 
						     * sizeof(int));  /* allocate 1 less */
			   		PANEPTR(w)->num_current_items--;
					break; 

				 }

				 } /* end if */
			  } /* end for */
	       } /* end if */
	   break;

	   default:
	   break;
	   }
	}

    in_view = InView(w, node);

    /* if item being deleted is in view, consider scrolling in new item */
    if (in_view) {
	   /* Deleting Top */
	   if (node == Top(w)) {
	      /* Move Top back.  If at head, move forward */
	      Top(w) = (Top(w) == HeadOffset(w)) ?
			  (NumItems(w) == 1) ? NULL_OFFSET :	/* last item */
			  Next(w, Top(w)) : Prev(w, Top(w));

	  /* If node is in upper half of view, back up Top if possible.
	     If node is in lower half and there is an item below view,
	     it will 'naturally' scroll into view when the pane is redrawn.
	     However, if there is no item below the view, attempt to bring
	     in item from above.
	  */
	  } else if ((Top(w) != HeadOffset(w)) &&
	      ((PaneIndex(w, node) < ViewHeight(w)/2) || (AfterBottom(w) < 1))) {
		  Top(w) = Prev(w, Top(w));
	  }
    }

    /* update count of selected items */
    if (IsSelected(node))
	  PANEPTR(w)->items_selected--;

    /* if the focus_item is being deleted, move it to prev (or next) */
    if (node == FocusItem(w)) {

	   FocusItem(w) = (node == HeadOffset(w)) ?
			(NumItems(w) == 1) ? NULL_OFFSET :	/* last item */
			Next(w, FocusItem(w)) : Prev(w, FocusItem(w));

	   if (PRIMPTR(w)->has_focus && (FocusItem(w) != NULL_OFFSET))
	       MakeFocused(FocusItem(w));
    }

    /* If last-searched item deleted, make it invalid */
    if (node == SearchItem(w))
	   SearchItem(w) = NULL_OFFSET;

    /* If there is a mnemonic, remove from global list of mnemonics */
    if (ItemPtr(node)->mnemonic)
	   _OlRemoveMnemonic((Widget)w, (XtPointer)node, False,
			  ItemPtr(node)->mnemonic);

    /*
     * Reset the start_index to nothing if you delete
     * the item that the start_index points to.
     *
     * Decrement the start_index if an item is going to
     * be deleted below the current start_index.
     *
     * Prev_index should be the same as the start_index.
     */
    if (IndexOf(node) == PANEPTR(w)->start_index) {
	   PANEPTR(w)->start_index = INITIAL_INDEX;
	   PANEPTR(w)->prev_index = PANEPTR(w)->start_index;
    } else if (IndexOf(node) < PANEPTR(w)->start_index) {
	   --(PANEPTR(w)->start_index);
	   PANEPTR(w)->prev_index = PANEPTR(w)->start_index;
    }
    
	if (ItemPtr(node)->label != (OlStr)NULL) 
		XtFree((char *) (ItemPtr(node)->label));


    DeleteNode(w, node);		/* delete it from list */

	PANEPTR(w)->num_items = NumItems(w);

	if (PANEPTR(w)->num_items == 0){
       PANEPTR(w)->own_clipboard		= False;
       PANEPTR(w)->own_selection		= False;
       PANEPTR(w)->items_selected	= 0;
	   PANEPTR(w)->current_item = 0;
	}

    /* # of items decreased, recalibrate SBar */
    if (PANEPTR(w)->update_view) {
	   UpdateSBar(w);

	   /* redisplay pane if appropriate */
	   if (in_view && XtIsRealized((Widget)w)) {
	       Redisplay((Widget)w, (XEvent *)NULL, (Region)NULL);
	   }
    }
	ReleaseToken();
}

void
OlSlistDeleteAllItems(Widget widget)
{
ListPaneWidget w; 
int i; 
int num_items;
Boolean save;

	GetToken();
    if (!ValidSListWidget(widget)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistDeleteAllItems- widget is not scrollingListWidget"));
		ReleaseToken();
		return;
    }
	w = PANEW(_OlListPane(widget));
	num_items = NumItems(w);
	if (EmptyList(w)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistDeleteAllItems- Empty List"));
	   ReleaseToken();
	   return;
	}

    save = PANEPTR(w)->update_view;

	PANEPTR(w)->update_view = False;
	for (i = num_items; i > 0; i--) { /* delete from end of the list */
	   ItemPtr((int)PANEPTR(w)->head.offsets.array[i - 1])->attr = 0;
	   OlSlistDeleteItem(widget, (OlSlistItemPtr) PANEPTR(w)->head.offsets.array[i - 1]);
	}

	if (PANEPTR(w)->num_current_items > 0) {
	   XtFree((char *)PANEPTR(w)->current_items);
	   PANEPTR(w)->num_current_items = 0;
	}
	PANEPTR(w)->num_items = (int)NumItems(w);
	PANEPTR(w)->update_view = save;

	
	/* update the view if allowed */
	if (PANEPTR(w)->update_view)
	{
	    MaintainWidth(w, NULL);/* pass in NULL to used max_width*/

	    if (XtIsRealized((Widget)w))
		Redisplay((Widget)w, (XEvent *)NULL, (Region)NULL);

	    UpdateSBar(w);
	}

	ReleaseToken();


}

void
OlSlistDeleteItems(Widget widget, OlSlistItemPtr *items, 
						   int num_items)
{
int i;

	GetToken();
	for (i = 0; i < num_items; i++) {
		OlSlistDeleteItem(widget, items[i]);
	}
	ReleaseToken();
}

void
OlSlistSetItemAttrs(Widget widget, OlSlistItemPtr item, OlSlistItemAttrsPtr item_attr)
{
ListPaneWidget lpw;

	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistSetItemAttrs- widget is not scrollingListWidget"));
	   ReleaseToken();
	   return;
    }
	lpw = PANEW(_OlListPane(widget));
	if (EmptyList(lpw)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistSetItemAttrs- Empty List"));
	   ReleaseToken();
	   return;
	}
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistSetItemAttrs- NULL item"));
	   ReleaseToken();
	   return;
    }
	if (!OlSlistIsValidItem(widget, item)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistSetItemAttrs- not a valid item"));
	   ReleaseToken();
	   return;
	}

	if (item_attr->flags & OlItemLabelType) 
		ItemPtr((int)item)->label_type = item_attr->label_type;
	if (item_attr->flags & OlItemLabel) {
		if (ItemPtr((int)item)->label != (OlStr) NULL) {
		OlStr temp;

		  temp = ItemPtr((int)item)->label;
		  ItemPtr((int)item)->label = (XtPointer)item_attr->item_label;
		  XtFree((char *) temp);
    	  copy_olstring(PRIMPTR(widget)->text_format, &(ItemPtr((int)item)->label));
		}
	}
	if (item_attr->flags & OlItemImage)
		ItemPtr((int)item)->glyph = (XImage *)item_attr->item_image;
	if (item_attr->flags & OlItemMnemonic) {
    	if (ItemPtr((int)item)->mnemonic != '\0') 
	       _OlRemoveMnemonic((Widget)lpw, (XtPointer)item, False,
			         ItemPtr((int)item)->mnemonic);

		ItemPtr((int)item)->mnemonic = item_attr->item_mnemonic;
		_OlAddMnemonic((Widget)lpw, (XtPointer)item, item_attr->item_mnemonic);
	}
	if (item_attr->flags & OlItemCurrent) {
		if (item_attr->item_current)
		   OlSlistMakeItemCurrent(widget, item, False);
		else 
		   OlSlistMakeItemNotCurrent(widget, item, False);
	}
	if (item_attr->flags & OlItemSensitive) {
		OlDefine mode = OlSlistGetMode(widget);

		if (((mode == OL_EXCLUSIVE) || (mode == OL_EXCLUSIVE_NONESET)) 
		   && NodePtr((int)item)->item_current)
		   ;
		else 
		   NodePtr((int)item)->item_sensitive = item_attr->item_sensitive;
	}
	if (item_attr->flags & OlItemUserData)
		NodePtr((int)item)->user_data = item_attr->user_data;

	ReleaseToken();

}

void
OlSlistGetItemAttrs(Widget widget, OlSlistItemPtr item, OlSlistItemAttrsPtr item_attr)
{
	GetToken();
    if (!ValidSListWidget(widget)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistGetItemAttrs- widget is not scrollingListWidget"));
	   ReleaseToken();
	   return;
    }
	if (EmptyList(PANEW(_OlListPane(widget)))) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistGetItemAttrs- Empty List"));
	   ReleaseToken();
	   return;
	}

    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistGetItemAttrs- NULL item"));
	   ReleaseToken();
	   return;
    }
	if (!OlSlistIsValidItem(widget, item)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: OlSlistGetItemAttrs- not a valid item"));
	   ReleaseToken();
	   return;
	}

	if (item_attr->flags & OlItemLabelType)
		item_attr->label_type = ItemPtr((int)item)->label_type;
	if (item_attr->flags & OlItemLabel)
		item_attr->item_label = ItemPtr((int)item)->label;
	if (item_attr->flags & OlItemImage)
		item_attr->item_image = ItemPtr((int)item)->glyph;
	if (item_attr->flags & OlItemMnemonic)
		item_attr->item_mnemonic = ItemPtr((int)item)->mnemonic;
	if (item_attr->flags & OlItemCurrent)
		item_attr->item_current = NodePtr((int)item)->item_current;
	if (item_attr->flags & OlItemSensitive)
		item_attr->item_sensitive = NodePtr((int)item)->item_sensitive;
	if (item_attr->flags & OlItemUserData)
		item_attr->user_data = NodePtr((int)item)->user_data;

	ReleaseToken();
}

void
OlSlistMakeItemCurrent(Widget widget, OlSlistItemPtr item, Boolean issue_callback)
{
ListPaneWidget w;
int node;
int unsel_node;

	GetToken();
    if (!ValidSListWidget(widget)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeItemCurrent- widget is not scrollingListWidget"));
		ReleaseToken();
		return;
    }
	w = PANEW(_OlListPane(widget));
	if (EmptyList(w)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeItemCurrent- Empty List"));
	   ReleaseToken();
	   return;
	}
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistMakeItemCurrent- NULL item"));
	   ReleaseToken();
	   return;
    }
	if (ItemPtr((int)item)->attr & OL_LIST_ATTR_CURRENT) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeItemCurrent- item already current"));
	   ReleaseToken();
	   return;
	}

	if ((PANEPTR(w)->slist_mode == OL_EXCLUSIVE) || 
			(PANEPTR(w)->slist_mode == OL_EXCLUSIVE_NONESET)) {
	   unsel_node = PANEPTR(w)->current_item;
	   if (unsel_node)
	      OlSlistMakeItemNotCurrent(widget, (OlSlistItemPtr)unsel_node, issue_callback);
	}

	ItemPtr((int)item)->attr |= OL_LIST_ATTR_CURRENT;
	NodePtr((int)item)->item_current = True;
	node = (int)item;
	PANEPTR(w)->current_item = node;
	OlSlistTouchItem(widget, item);
	if (PANEPTR(w)->slist_mode == OL_NONEXCLUSIVE) {
	   if (PANEPTR(w)->num_current_items > 0) {
		   PANEPTR(w)->current_items = (int *) XtRealloc(
				 (char *)PANEPTR(w)->current_items, 
				 (PANEPTR(w)->num_current_items + 1) /* allocate 1 more */
				 * sizeof(int));
	   } /* end if */
	   else {
		   PANEPTR(w)->current_items = (int *) XtMalloc(sizeof(int));
	   }
	   PANEPTR(w)->current_item = node;
	   PANEPTR(w)->current_items[PANEPTR(w)->num_current_items] = node;
	   PANEPTR(w)->num_current_items++;
	}
    if (issue_callback) {
	   switch (PANEPTR(w)->slist_mode) {
	   case OL_EXCLUSIVE:
	   case OL_EXCLUSIVE_NONESET:
	   {
	      OlSlistCallbackStruct cbdata;

		  cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
	      cbdata.reason = OL_REASON_ITEM_CURRENT; 
	      cbdata.mode = PANEPTR(w)->slist_mode;
	      cbdata.event = (XEvent *) NULL;
	      cbdata.item = (OlSlistItemPtr)item;
	      cbdata.item_pos = IndexOf((int)item) + 1;
		  *(cbdata.cur_items) = (OlSlistItemPtr)item;
		  cbdata.num_cur_items = 1;
	      cbdata.cur_items_pos = (int *) NULL;
	      ItemCurrentCallback(w, &cbdata);
		  XtFree((char *)cbdata.cur_items);
	   }
		  break;
	   case OL_NONEXCLUSIVE:
	   {
	      OlSlistCallbackStruct nonex_cbdata;

	      nonex_cbdata.reason = OL_REASON_ITEM_CURRENT; 
	      nonex_cbdata.mode = PANEPTR(w)->slist_mode;
	      nonex_cbdata.event = (XEvent *) NULL;
	      nonex_cbdata.item = (OlSlistItemPtr)item;
	      nonex_cbdata.item_pos = IndexOf((int)item) + 1;
		  nonex_cbdata.cur_items = (OlSlistItemPtr *)PANEPTR(widget)->current_items;
		  nonex_cbdata.num_cur_items = PANEPTR(widget)->num_current_items;
	      nonex_cbdata.cur_items_pos = (int *) NULL;
	      ItemCurrentCallback(w, &nonex_cbdata);
	   }
		  break;
	   default:
		  break;
	   }
    }
	ReleaseToken();
}

void
OlSlistMakeItemNotCurrent(Widget widget, OlSlistItemPtr item, Boolean issue_callback)
{
ListPaneWidget w;
int node;

	GetToken();
    if (!ValidSListWidget(widget)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeItemNotCurrent- widget is not scrollingListWidget"));
		ReleaseToken();
		return;
    }
	w = PANEW(_OlListPane(widget));
	if (EmptyList(w)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeItemNotCurrent- Empty List"));
	   ReleaseToken();
	   return;
	}
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistMakeItemNotCurrent- NULL item"));
	   ReleaseToken();
	   return;
    }
	if (!(ItemPtr((int)item)->attr & OL_LIST_ATTR_CURRENT)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeItemNotCurrent- item not current"));
	   ReleaseToken();
	   return;
	}
	if (PANEPTR(widget)->slist_mode == OL_EXCLUSIVE) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeItemNotCurrent- item cannot be made not current in exclusive mode"));
	   ReleaseToken();
	   return;
	}

	node = (int)item;
	ItemPtr(node)->attr &= ~OL_LIST_ATTR_CURRENT;
	NodePtr(node)->item_current = False;
	OlSlistTouchItem(widget, item);
	if (node == PANEPTR(w)->current_item)
	   PANEPTR(w)->current_item = 0;
	if (PANEPTR(w)->slist_mode == OL_NONEXCLUSIVE) {
	   if (PANEPTR(w)->num_current_items > 0) {
	   int i;

	   for (i = 0; i < PANEPTR(w)->num_current_items; i++) {
		/* First try to find it in the current list */
		  if (PANEPTR(w)->current_items[i] == node) { /* found it */
			/* Is it the last current item ?? */
			 if (PANEPTR(w)->num_current_items == 1) {
				XtFree((char *)PANEPTR(w)->current_items);
				PANEPTR(w)->num_current_items = 0;
				break; 
			 }

			 { /* Otherwise move everything up one slot */
			 int j;
				
			 for (j = i; j < (PANEPTR(w)->num_current_items - 1); j++)
				PANEPTR(w)->current_items[j] = 
								PANEPTR(w)->current_items[j + 1];

				PANEPTR(w)->current_items = (int *) XtRealloc(
						 (char *)PANEPTR(w)->current_items, 
						 (PANEPTR(w)->num_current_items - 1) 
						 * sizeof(int));  /* allocate 1 less */
				PANEPTR(w)->num_current_items--;
				break; 

			 }

			 } /* end if */
		  } /* end for */
	   } /* end if */
	}
	if (issue_callback) {
		OlSlistCallbackStruct cbdata;

		cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
		cbdata.reason = OL_REASON_ITEM_NOT_CURRENT; 
		cbdata.event = (XEvent *) NULL;
		cbdata.mode = PANEPTR(widget)->slist_mode;
		cbdata.item = (OlSlistItemPtr)item;
		cbdata.item_pos = IndexOf((int)item) + 1;
		*(cbdata.cur_items) = (OlSlistItemPtr)item;
		cbdata.num_cur_items = 1;
	    cbdata.cur_items_pos = (int *) NULL;
		ItemNotCurrentCallback(w, &cbdata);
		XtFree((char *)cbdata.cur_items);
	}
	ReleaseToken();
}

void 
OlSlistMakeAllItemsNotCurrent(Widget widget)
{
int i, node;
ListPaneWidget w;

	GetToken();
    if (!ValidSListWidget(widget)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeAllItemsNotCurrent- widget is not scrollingListWidget"));
		ReleaseToken();
		return;
    }
	w = PANEW(_OlListPane(widget));
	if (EmptyList(w)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeAllItemsNotCurrent- Empty List"));
	   ReleaseToken();
	   return;
	}
	switch (PANEPTR(w)->slist_mode) {
	case OL_NONEXCLUSIVE: 
	   if (PANEPTR(w)->num_current_items) {
	    for (i = 0; i < PANEPTR(w)->num_current_items; i++) {
	      ItemPtr(PANEPTR(w)->current_items[i])->attr &= ~OL_LIST_ATTR_CURRENT; 
	      NodePtr(PANEPTR(w)->current_items[i])->item_current = False;
	    }
	    XtFree((char *)PANEPTR(w)->current_items);
	    PANEPTR(w)->num_current_items = 0;
	    Redisplay((Widget)w, (XEvent *)NULL, (Region)NULL);
	   }
	   break;
	case OL_EXCLUSIVE: 
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistMakeAllItemsNotCurrent- item cannot be not current in exclusive mode"));
	   break;
	case OL_EXCLUSIVE_NONESET:
	   if (PANEPTR(w)->current_item) {
	     ItemPtr(PANEPTR(w)->current_item)->attr &= ~OL_LIST_ATTR_CURRENT; 
	     NodePtr(PANEPTR(w)->current_item)->item_current = False;
	     OlSlistTouchItem(widget, (OlSlistItemPtr)PANEPTR(w)->current_item);
	   }
	   break;
	default:
	   break;
	}
	PANEPTR(w)->current_item = 0;
	ReleaseToken();
}

void
OlSlistFirstViewableItem(Widget widget, OlSlistItemPtr item)
{
ListPaneWidget	lpw;

	GetToken();
    if (!ValidSListWidget(widget)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistFirstViewableItem- widget is not scrollingListWidget"));
		ReleaseToken();
		return;
    }
	lpw = PANEW(_OlListPane(widget));
	if (EmptyList(lpw)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistFirstViewableItem- Empty List"));
	   ReleaseToken();
	   return;
	}
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistFirstViewableItem- NULL item"));
	   ReleaseToken();
	   return;
    }

    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistFirstViewableItem- NULL item"));
	   ReleaseToken();
	   return;
    }
	if (Top(lpw) == (int)item) {
		ReleaseToken();
		return;
	}

	if (!(IndexOf((int)item) <= (NumItems(lpw) - ViewHeight(lpw)))) {
		ReleaseToken();
		return;
	}

	Top(lpw) = (int)item;
	PANEPTR(lpw)->first_item = item;
	PANEPTR(lpw)->last_item = (OlSlistItemPtr) (OffsetNode(lpw, (int)item, ViewHeight(lpw)));
	ScrollView(lpw);		/* do scroll specific stuff */
	UpdateSBar(lpw);		/* item moved into view: adjust scrollbar! */

	ReleaseToken();
}

void
OlSlistLastViewableItem(Widget widget, OlSlistItemPtr item)
{
ListPaneWidget	lpw;
int new_top_item;

	GetToken();
    if (!ValidSListWidget(widget))
    {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistLastViewableItem- widget is not scrollingListWidget"));
		ReleaseToken();
		return;
    }
	lpw = PANEW(_OlListPane(widget));
	if (EmptyList(lpw)) {
		OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: OlSlistLastViewableItem- Empty List"));
	   ReleaseToken();
	   return;
	}
    if (item == (OlSlistItemPtr)NULL) {
	   OlWarning(dgettext(OlMsgsDomain,
		   "ScrollingList: OlSlistLastViewableItem- NULL item"));
	   ReleaseToken();
	   return;
    }
	
	if (!(IndexOf((int)item) >= (ViewHeight(lpw) - 1))) {
		ReleaseToken();
		return;
	}

	new_top_item = OffsetNode(lpw, Top(lpw), 
                     IndexOf((int)item) - TopIndex(lpw) - ViewHeight(lpw) + 1);

	if (new_top_item == Top(lpw)) { /* Every thing is the same - so return */
		ReleaseToken();
		return;
	}

	Top(lpw) = new_top_item;
	PANEPTR(lpw)->first_item = (OlSlistItemPtr) new_top_item;
	PANEPTR(lpw)->last_item = (OlSlistItemPtr) item;
	ScrollView(lpw);		/* do scroll specific stuff */
	UpdateSBar(lpw);		/* item moved into view: adjust scrollbar! */

	ReleaseToken();
}


void
OlSlistEditItem(Widget widget, Boolean insert, OlSlistItemPtr ref_item)
{
	GetToken();
	ApplEditOpen (widget, insert, ref_item);
	ReleaseToken();
}

void
OlSlistEndEdit(Widget widget)
{
	GetToken();
	ApplEditClose (widget);
	ReleaseToken();
}

/***************************private*procedures****************************
 *
 * Private Functions
 *
 */
static void
UpdateSuperCaret(ListPaneWidget lpw)
{
        PrimitivePart   *pp = &(lpw->primitive);
        OlVendorPartExtension ext_part = pp->ext_part;

	if(pp->has_focus == FALSE)
		return;
        if(ext_part == (OlVendorPartExtension)NULL ||
            ext_part->supercaret == (Widget)NULL    ||
            pp->input_focus_feedback != OL_SUPERCARET)
                return;

        _OlCallUpdateSuperCaret(ext_part->supercaret, (Widget)lpw);
}

static void
copy_olstring(OlStrRep tf, OlStr * ol_str)
{
    if(tf != OL_WC_STR_REP)
       *ol_str = (*ol_str != (OlStr) NULL) ?
           XtNewString((XtPointer)(*ol_str)) : (OlStr) NULL;
     else {
       wchar_t *ws;
       if (*ol_str != (OlStr) NULL) {
           ws = (wchar_t *)XtMalloc((wslen((wchar_t *)*ol_str)
                                     +1)*sizeof(wchar_t));
           wscpy(ws, (wchar_t *) *ol_str); 
           *ol_str = (OlStr)ws;
       }
     }
}

/* 
 * UpdateItemFocus - used to set focus or unfocus on an item with flag
 * 'item_has_focus'
 */

static void
UpdateItemFocus(ListPaneWidget w, int node, Boolean item_has_focus)
{
	if (item_has_focus) { 
	  FocusItem(w) = node;
	  MakeFocused(node);
	}
	else
	  MakeUnfocused(node);
}

/******************************function*header****************************
 * ApplAddItem- called by appl to insert item into list
 */
static OlListToken
ApplAddItem (Widget slw, OlListToken parent, OlListToken refNode, OlListItem item)
{
    ListPaneWidget	w;
    int			node;		/* index of new node */

	GetToken();
    if (!ValidSListWidget(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: addItem- widget is not scrollingListWidget"));
	ReleaseToken();
	return (NULL);
    }

    if ((item.label_type != OL_STRING) &&
        (item.label_type != OL_IMAGE)  &&
        (item.label_type != OL_BOTH))
    {
	OlWarning(dgettext(OlMsgsDomain,
	    	"ScrollingList: addItem- illegal label_type"));
	ReleaseToken();
	return(NULL);
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    /*
     * Bump the start_index if you insert an item
     * below of the current start_index.
     *
     * If the refNode is NULL the item is getting added
     * above everything so the start_index is ok.
     *
     * Prev_index should be the same as the start_index.
     */
    if ((refNode != 0) &&
	(IndexOf((int)refNode) <= PANEPTR(w)->start_index)) {
	    ++(PANEPTR(w)->start_index);
	    PANEPTR(w)->prev_index = PANEPTR(w)->start_index;
    }

    node = InsertNode(w, (int)refNode);
	ItemPtr(node)->attr = 0;

    *ItemPtr(node) = item;		/* copy in item data */

    if (item.mnemonic != '\0')
	_OlAddMnemonic((Widget)w, (XtPointer)node, item.mnemonic);

    MaintainWidth(w, &item);

    /* If this is the 1st item in the list or inserting above view and
	view is unfilled, make it the top_item.
    */
    if ((Top(w) == NULL_OFFSET) ||
      (AboveView(w, node) && (NumItems(w) <= ViewHeight(w))))
	Top(w) = node;

    /* If we have focus but focus_item is NULL, make this the focus_item.
	This happens when entire list is deleted, then new item added.
    */
    if (PRIMPTR(w)->has_focus && (FocusItem(w) == NULL_OFFSET))
    {
	FocusItem(w) = node;
	MakeFocused(FocusItem(w));
    }

    /* # of items increased, recalibrate SBar */
    if (PANEPTR(w)->update_view)
    {
	UpdateSBar(w);

	if (InView(w, node) && XtIsRealized((Widget)w))
	    		/* scroll view if appropriate */
	{
	    ScrollView(w);
	}
    }

	ReleaseToken();
    return ( (OlListToken)node );
}

/*****************************function*header****************************
 * ApplDeleteItem- called by appl to delete item.
 */
static void
ApplDeleteItem (Widget slw, OlListToken token)
{
    ListPaneWidget	w;
    int			node;
    Boolean		in_view;

	GetToken();
    if (!ValidSListWidget(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: deleteItem- widget is not scrollingListWidget"));
	ReleaseToken();
	return;
    }

    if (token == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: deleteItem- NULL token"));
	ReleaseToken();
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    /* safety check for deletion from NULL list */
    if (EmptyList(w)) {
	ReleaseToken();
	return;
	}

    node = (int)token;			/* make token an offset into list */

    in_view = InView(w, node);

    /* if item being deleted is in view, consider scrolling in new item */
    if (in_view)
    {
	/* Deleting Top */
	if (node == Top(w))
	{
	    /* Move Top back.  If at head, move forward */
	    Top(w) = (Top(w) == HeadOffset(w)) ?
			(NumItems(w) == 1) ? NULL_OFFSET :	/* last item */
			Next(w, Top(w)) : Prev(w, Top(w));

	/* If node is in upper half of view, back up Top if possible.
	   If node is in lower half and there is an item below view,
	   it will 'naturally' scroll into view when the pane is redrawn.
	   However, if there is no item below the view, attempt to bring
	   in item from above.
	*/
	} else if ((Top(w) != HeadOffset(w)) &&
	  ((PaneIndex(w, node) < ViewHeight(w)/2) || (AfterBottom(w) < 1))) {
		Top(w) = Prev(w, Top(w));
	}
    }

    /* update count of selected items */
    if (IsSelected(node))
	PANEPTR(w)->items_selected--;

    /* if the focus_item is being deleted, move it to prev (or next) */
    if (node == FocusItem(w))
    {
	FocusItem(w) = (node == HeadOffset(w)) ?
			(NumItems(w) == 1) ? NULL_OFFSET :	/* last item */
			Next(w, FocusItem(w)) : Prev(w, FocusItem(w));
	
	if (PRIMPTR(w)->has_focus && (FocusItem(w) != NULL_OFFSET))
	    MakeFocused(FocusItem(w));
    }

    /* If last-searched item deleted, make it invalid */
    if (node == SearchItem(w))
	SearchItem(w) = NULL_OFFSET;

    /* If there is a mnemonic, remove from global list of mnemonics */
    if (ItemPtr(node)->mnemonic)
	_OlRemoveMnemonic((Widget)w, (XtPointer)node, False,
			  ItemPtr(node)->mnemonic);

    /*
     * Reset the start_index to nothing if you delete
     * the item that the start_index points to.
     *
     * Decrement the start_index if an item is going to
     * be deleted below the current start_index.
     *
     * Prev_index should be the same as the start_index.
     */
    if (IndexOf(node) == PANEPTR(w)->start_index) {
	PANEPTR(w)->start_index = INITIAL_INDEX;
	PANEPTR(w)->prev_index = PANEPTR(w)->start_index;
    } else if (IndexOf(node) < PANEPTR(w)->start_index) {
	--(PANEPTR(w)->start_index);
	PANEPTR(w)->prev_index = PANEPTR(w)->start_index;
    }
    
    DeleteNode(w, node);		/* delete it from list */

    /* # of items decreased, recalibrate SBar */
    if (PANEPTR(w)->update_view)
    {
	UpdateSBar(w);

	/* redisplay pane if appropriate */
	if (in_view && XtIsRealized((Widget)w))
	{
	    Redisplay((Widget)w, (XEvent *)NULL, (Region)NULL);
	}
    }
	ReleaseToken();
}

/*****************************function*header****************************
 * ApplEditClose-
 */
static void
ApplEditClose (Widget slw)
{
    ListPaneWidget w;

	GetToken();
    if (!ValidSListWidget(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: editClose- widget is not scrollingListWidget"));
	ReleaseToken();
	return;
    }

    w = PANEW(_OlListPane(slw));		/* get list pane widget */

    if (!PANEPTR(w)->editing) {			/* if we're not editing */
	ReleaseToken();
	return;					/*  return silently */
	}

    PANEPTR(w)->editing = False;

    SetInputFocus((Widget)w, LatestTime(w));	/* set focus back to pane */

    XtUnmapWidget(TextField(w));		/* unmap text field */

    if (NumItems(w) > ViewHeight(w))
	XtSetSensitive(SBAR(w), True);		/* turn scrollbar back on */

	ReleaseToken();
}

/*****************************function*header****************************
 * ApplEditOpen-
 */
static void
ApplEditOpen (Widget slw, Boolean insert, OlListToken reference)
{
    ListPaneWidget	w;
    int			node;
    Arg			args[10];
    Cardinal		cnt;

	GetToken();
    if (!ValidSListWidget(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: editOpen- widget is not scrollingListWidget"));
	ReleaseToken();
	return;
    }

    if (reference == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: editOpen- NULL reference"));
	ReleaseToken();
	return;
    }

    /* editing only makes sense if we're Realized */
    if (!XtIsRealized(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: editOpen- widget is not realized"));
	ReleaseToken();
	return;
    }

    if (insert) {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: editOpen- 'insert' currently not supported"));
	ReleaseToken();
	return;
    }
	if (!OlSlistIsValidItem(slw, reference)) {
	   OlWarning(dgettext(OlMsgsDomain,
	      "ScrollingList: EditOpen- not a valid reference item"));
	   ReleaseToken();
	   return;
	}

    ApplViewItem(slw, reference);	/* be sure item is in view */

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    XtSetSensitive(SBAR(w), False);	/* shut off scrollbar */

    PANEPTR(w)->editing = True;		/* we're now editing */
    node = (int)reference;		/* make reference an offset into list */
    XtRealizeWidget(TextField(w));	/* must be realized before resize */

    /* set geometry and string of text field */
    cnt = 0;
    XtSetArg(args[cnt], XtNx, PANEPTR(w)->horiz_margin); cnt++;
    XtSetArg(args[cnt], XtNy, PANEPTR(w)->vert_margin +
			PaneIndex(w, node) * ItemHeight(w)); cnt++; 
    XtSetArg(args[cnt], XtNwidth,
			w->core.width - 2*PANEPTR(w)->horiz_margin); cnt++;
    XtSetArg(args[cnt], XtNstring, ItemPtr(node)->label); cnt++;
    XtSetValues(TextField(w), args, cnt);

    /* map the text field and set focus to it */
    XtMapWidget(TextField(w));
    (void) OlCallAcceptFocus(TextField(w), LatestTime(w));

	ReleaseToken();
}
/*****************************function*header****************************
 * ApplTouchItem-
 */
static void
ApplTouchItem (Widget slw, OlListToken token)
{
    ListPaneWidget	w;
    int			node;

	GetToken();

    if (!ValidSListWidget(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: touchItem- widget is not scrollingListWidget"));
	ReleaseToken();
	return;
    }

    if (token == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: touchItem- NULL token"));
	ReleaseToken();
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */
    node = (int)token;			/* make token an offset into list */

    MaintainWidth(w, ItemPtr(node));	/* do bookkeeping on widest item */

    /* draw item if appropriate */
    if (InView(w, node) && XtIsRealized((Widget) w) &&
	    PANEPTR(w)->update_view) {

		DrawItem(w, node, ITEMSENSITIVE(w, node));
    }

	ReleaseToken();
}

/*****************************function*header****************************
 * ApplUpdateView-
 */
static void
ApplUpdateView (Widget slw, Boolean ok)
{
    ListPaneWidget w;

	GetToken();
    if (!ValidSListWidget(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: updateView- widget is not scrollingListWidget"));
	ReleaseToken();
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */

    PANEPTR(w)->update_view = ok;

    if (ok)
    {
	MaintainWidth(w, NULL);		/* pass in NULL so max_width is used */

	if (XtIsRealized((Widget)w))
	    Redisplay((Widget)w, (XEvent *)NULL, (Region)NULL);

	UpdateSBar(w);
    }

	ReleaseToken();
}

/*****************************function*header****************************
 * ApplViewItem-
 */
static void
ApplViewItem (Widget slw, OlListToken token)
{
    ListPaneWidget	w;
    int			node;

	GetToken();

    if (!ValidSListWidget(slw))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: viewItem- widget is not scrollingListWidget"));
	ReleaseToken();
	return;
    }

    if (token == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: viewItem- NULL token"));
	ReleaseToken();
	return;
    }

    w = PANEW(_OlListPane(slw));	/* get list pane widget */
    node = (int)token;			/* make token an offset into list */

    if (InView(w, node)) {		/* trivial: already in view */
	ReleaseToken();
	return;
	}

    /* adjust top_item according to whether node is above or below view */
    Top(w) = (AboveView(w, node)) ?
		node :			/* move node to top of view */
					/* move node to bottom of view: */
		OffsetNode(w, Top(w),
			IndexOf(node) - TopIndex(w) - ViewHeight(w) + 1);
		
    ScrollView(w);		/* do scroll specific stuff */
    UpdateSBar(w);		/* item moved into view: adjust scrollbar! */

	ReleaseToken();
}

/*****************************function*header****************************
 * BuildClipboard-
 */
static void
BuildClipboard (ListPaneWidget w)
{
    int		node;
    Cardinal	char_cnt = 0;
    Cardinal	item_cnt;
    String	ptr;

    /* first, accumulate byte count from labels of selected items */
    node = HeadOffset(w);
    for (item_cnt = PANEPTR(w)->items_selected; item_cnt > 0; )
    {
	if (IsSelected(node))
	{
	    item_cnt--;
	    char_cnt += _OlStrlen(ItemPtr(node)->label) + 1; /* + \n */
	}
	node = Next(w, node);
    }

    /* allocate storage (if necessary) for char_cnt bytes */
    if (char_cnt > ClipboardCnt)
    {
	ClipboardCnt	= char_cnt;
	Clipboard	= XtRealloc(Clipboard, ClipboardCnt);
    }

    /* copy labels (and '\n') into Clipboard */

    ptr = Clipboard;			/* point to Clipboard */

    node = HeadOffset(w);
    for (item_cnt = PANEPTR(w)->items_selected; item_cnt > 0; )
    {
	if (IsSelected(node))
	{
	    item_cnt--;
	    (void) strcpy(ptr, ItemPtr(node)->label);
	    ptr += _OlStrlen(ItemPtr(node)->label);
	    *ptr++ = '\n';
	}
	node = Next(w, node);
    }

    *(ptr - 1) = '\0';		/* replace last \n with terminator */
}

/*****************************function*header****************************
 * BuildDeleteList-
 */
static void
BuildDeleteList (ListPaneWidget w, OlListDelete *deleteList)
{
    Cardinal		cnt;
    int			node;
    OlListToken *	tokens;		/* array of token to be deleted */

    deleteList->num_tokens = PANEPTR(w)->items_selected;

    if (deleteList->num_tokens == 0)
    {
	deleteList->tokens = NULL;
	return;
    }

    deleteList->tokens = (OlListToken *)XtMalloc(
				deleteList->num_tokens * sizeof(OlListToken));
    tokens = deleteList->tokens;

    for (node = HeadOffset(w), cnt = deleteList->num_tokens;
      cnt > 0; node = Next(w, node))
	if (IsSelected(node))
	{
	    *tokens++ = (OlListToken)node;
	    cnt--;
	}
}

/*****************************function*header****************************
   SetPaneHeight- called at Initialize & SetValues to set actualViewHeight
	and Pane height based on settings in core.height &
	list_pane.view_height.  Scrollbar is resized to height of pane.

	If view_height > 0, make widget tall enough for that many items.
	Else, if height > 0, make pane height tall enough for an integral
	number of items.
	Else, make height & actualViewHeight size of (marginal) minimal
	scrollbar.

	SetPaneHeight depends on list_pane.max_height being set.
*/
static void
SetPaneHeight (ListPaneWidget w)
{
    if (PANEPTR(w)->view_height > 0)
    {
	ViewHeight(w) = PANEPTR(w)->view_height;

    } else {
	Widget model_widget = (w->core.height > 0) ? (Widget)w : SBAR(w);

/*
	ViewHeight(w) =
	    (Dimension)(model_widget->core.height + ItemHeight(w) - 1) /
						(Dimension)ItemHeight(w);
*/
	ViewHeight(w) = (Dimension)(model_widget->core.height - 
					2 * PANEPTR(w)->vert_margin) / 
					(Dimension) ItemHeight(w);

	if (ViewHeight(w) < 1)
	    ViewHeight(w) = 1;
    }

    /* Set pane height */
    w->core.height = ViewHeight(w) * ItemHeight(w) +
					2 * PANEPTR(w)->vert_margin;

}

/*****************************function*header****************************
    SetPaneWidth- called from SetValues to set the widget's width field
    to the maximum width of all items. We need to invoke MaintainWidth
    on each item to compute the max_width over all items; BUT we also
    must prevent any resize requests from within MaintainWidth. So we
    hack the recompute_width resource to False *temporarily* to prevent
    resize requests ....
*/
static void
SetPaneWidth (ListPaneWidget w)
{
        int node;
        Boolean recompute_width = PANEPTR(w)->recompute_width;
        Dimension width;
        int i;

        if (EmptyList(w) || (ViewHeight(w) == 0))
                return;
        node = HeadOffset(w);

        /* Reset everything as we start thru the itemlist again */
        PANEPTR(w)->aligned_pos           = 0;
        PANEPTR(w)->max_comp_width        = 0;

        /* HACK - to prevent MaintainWidth from issuing resize reqs */
        PANEPTR(w)->recompute_width = False;

        for (i = 0; i < NumItems(w); i++) {
                MaintainWidth(w, ItemPtr(node));
                node = Next(w, node);
        }

        PANEPTR(w)->recompute_width = recompute_width;
 
        width = ItemWidth(w, PANEPTR(w)->max_width);
 
        /* NOTE: the widget does'nt shrink  - Just following existing
         * practices ... !
         */
        if (PANEPTR(w)->recompute_width && width > w->core.width)
                 w->core.width = width;
}

/*****************************function*header****************************
 * ClearSelection-
 */
static void
ClearSelection (ListPaneWidget w)
{
    int		node;
    Cardinal	cnt;

    for (node = HeadOffset(w), cnt = PANEPTR(w)->items_selected;
      cnt > 0; node = Next(w, node))
	if ((node != NULL_OFFSET) && IsSelected(node)) {
	    cnt--;
	    ItemPtr(node)->attr &= ~OL_B_LIST_ATTR_SELECTED;	/* unselect */
	    if (InView(w, node) && PANEPTR(w)->update_view)
		   DrawItem(w, node, ITEMSENSITIVE(w, node));
	}

    PANEPTR(w)->items_selected = 0;
}

/*****************************function*header****************************
 * CreateGCs-
 */
static void
CreateGCs (ListPaneWidget w)
{
    unsigned long	valueMask;
    XGCValues		values;
    Pixel		tmp;

    if(PRIMPTR(w)->text_format == OL_SB_STR_REP){
	valueMask           = (GCForeground | GCBackground | GCFont);
	values.font         = ((XFontStruct *)PRIMPTR(w)->font)->fid;
    } else {
	valueMask           = (GCForeground | GCBackground);
    }

    values.foreground		= PRIMPTR(w)->font_color;
    values.background		= w->core.background_pixel;

    PANEPTR(w)->gc_normal	= XtGetGC((Widget)w, valueMask, &values);

    tmp				= values.foreground;
    values.foreground		= values.background;
    values.background		= tmp;

    PANEPTR(w)->gc_inverted	= XtGetGC((Widget)w, valueMask, &values);

    PANEPTR(w)->attr_normal	= OlgxCreateAttrs((Widget)w,
				   w->primitive.font_color,
				   (OlgxBG *)&(w->core.background_pixel), False,
				   (unsigned)_OlGetScale(XtScreen(w)),
				   PRIMPTR(w)->text_format, PRIMPTR(w)->font);

    PANEPTR(w)->attr_focus	= OlgxCreateAttrs((Widget)w,
				   w->primitive.font_color,
				   (OlgxBG *)&(PRIMPTR(w)->input_focus_color),
				   False, (unsigned)_OlGetScale(XtScreen(w)),
				   PRIMPTR(w)->text_format, PRIMPTR(w)->font);
}

/******************************function*header****************************
    CutOrCopy- copy selection contents to clipboard
*/
static void
CutOrCopy (ListPaneWidget w, Boolean cut)
{
    /* Do we beep the display if we own the selection but it is NULL? */
    if (PANEPTR(w)->items_selected == 0)
    {
	_OlBeepDisplay((Widget)w, 1);
	return;			/* return silently */
    }

    /* Do we beep when CUT and no appl delete callback? */
    if (cut &&
      (XtHasCallbacks(SLISTW(w), XtNuserDeleteItems) != XtCallbackHasSome))
    {
	_OlBeepDisplay((Widget)w, 1);
	return;			/* return silently */
    }

    /* get the clipboard if we don't already own it */
    if (!PANEPTR(w)->own_clipboard)
    {
	PANEPTR(w)->own_clipboard =
	    XtOwnSelection((Widget)w, XA_CLIPBOARD(XtDisplay(w)),
			   LatestTime(w), ConvertSelection, LoseSelection,
			   SelectionDone);

	if (!PANEPTR(w)->own_clipboard) {
	    OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: failed to gain ownership of Clipboard"));
	    return;
	}
    }

    BuildClipboard(w);

    DPRINT((stderr, "%s selection= %s \n", (cut) ? "cut" : "copy", Clipboard));

    if (cut)			/* call any user delete callbacks */
    {
	  OlListDelete items;
	  BuildDeleteList(PANEW(w), &items);
	if (PANEPTR(w)->slist_mode != OL_NONE) {
	  OlSlistUserDelCallbackStruct del_cbdata; 

	  del_cbdata.reason = OL_REASON_USER_DELETE_ITEMS;
	  del_cbdata.event = (XEvent *)NULL;
	  del_cbdata.user_del_items = items.tokens;
	  del_cbdata.num_del_items = items.num_tokens;
	  XtCallCallbacks((Widget)SLISTW(w), XtNuserDeleteItems, 
					  (XtPointer)&del_cbdata);
	}
	else
	  XtCallCallbacks((Widget)SLISTW(w), XtNuserDeleteItems, 
					  (XtPointer)&items);

	XtFree((char *)items.tokens);
    }

    ClearSelection(w);		/* OL says clear selection after CUT/COPY */
}

/******************************function*header****************************
    DeleteNode-

    Widget list specific first:
     1	decrement index of all nodes 'above' node to be deleted
     2	delete node from widget's list
     3	fix up HeadOffset if deleting head and/or last item

    List store specific:
     4	decrement number of total items
     5	put deleted node in free list or free everything if last item

     Note that inserting at the end of the free list is faster than at the
     beginning of the list.  Deleting is done at the beginning of the free
     list (during InsertNode) which was felt to be less frequent.  Optimizing
     DeleteNode was deemed appropriate since destroying a widget would
     result in many calls to DeleteNode.
*/
static void
DeleteNode (ListPaneWidget w, int node)
{
    _OlHead	head = HEADPTR(w);
    List	list = THE_LIST;
    int		i;

    /* Do work on behalf of widget first */

    /* Decrement index of all nodes 'above' deleted node */
    for (i = IndexOf(node) + 1; i < NumItems(w); i++)
	IndexOf(OffsetOf(w, i))--;

    /* Delete node from widget's list of offsets (nodes). */
    _OlArrayDelete(&(head->offsets), IndexOf(node));

    if (EmptyList(w))			/* free offset array when last item */
    {
	_OlArrayFree(&(head->offsets));
	HeadOffset(w) = NULL_OFFSET;

    } else if (node == HeadOffset(w)) {	/* if deleting the head, */
	HeadOffset(w) = OffsetOf(w, 0);	/*  new head is offset of index 0 */
    }

    /* Now delete node from list store */

    list->num_nodes--;			/* one less node */

    if (list->num_nodes == 0)	/* If all nodes deleted, free storage. */
    {
	XtFree((char *)list->nodes);
	list->nodes	= NULL;
	list->num_slots	= 0;

	_OlArrayFree(&(list->free));	/* free "free" list */

    } else {
				/* save node on free list */
	_OlArrayInsert(&(list->free), list->free.num_elements, node);
    }
}

/*
 ****************************************************************
    DrawItem-
        First, the background is painted.
        Then the current item border is drawn.
        Then the string is drawn.
*/
static void
DrawItem (ListPaneWidget w, int node, Boolean sensitive)
{
    Screen	*scr = XtScreenOfObject((Widget)w);
    Window	win = XtWindow((Widget)w);
    Display	*dpy = XtDisplay((Widget)w);
    Position	x, y;
    Dimension	width, height;
    Dimension	hBorder, vBorder;
    OlgxAttrs * attr;
    unsigned long saveFG; /* save FG pixel color */
    GC		fore_gc, back_gc;
    Boolean	releaseGCs = False, restoreFG = False;
    Position    offset = 0;
    int		state = 0;	/* OLGX rendering flag */

    if (!sensitive)
	state |= OLGX_INACTIVE;

    x		= PANEPTR(w)->horiz_margin;
    width	= w->core.width - 2*x;	/* ie., left & right margins */
    height	= ItemHeight(w);
    y		= PANEPTR(w)->vert_margin + PaneIndex(w, node) * height;
    /* This should really be equivalent to size of 3D indent drawn by OLGX */
    hBorder	= (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr);
    vBorder	= (Dimension)OlgxScreenPointToPixel(OL_VERTICAL, 1, scr);

    if ( _OlInputFocusFeedback((Widget)w) == OL_INPUT_FOCUS_COLOR &&
		IsFocused(node) && sensitive) {		/* drawing focus_item ? */

        if (PRIMPTR(w)->font_color == PRIMPTR(w)->input_focus_color) {
            /* when collision with colors, must reverse normal settings.
	     * but, note that we can no longer tell a selected item from
	     * an item with the input Focus, since we have only 2 colours
	     * to work with...
	     */
	    back_gc = PANEPTR(w)->gc_normal;
	    fore_gc = PANEPTR(w)->gc_inverted;
	    attr = PANEPTR(w)->attr_normal;
            /* We must temporarily set the fontcolor to the item bg color
             * so that we can see the label.  We'll restore it later.
             */  
            saveFG = olgx_get_single_color(attr->ginfo, OLGX_BLACK); 
	    restoreFG = True;
            olgx_set_single_color(attr->ginfo, OLGX_BLACK, 
                        (unsigned long)w->core.background_pixel, OLGX_SPECIAL); 
	} else {
	    unsigned long	valueMask;
	    XGCValues		values;
	    Pixel		tmp;

	    releaseGCs = True;
	    attr = PANEPTR(w)->attr_focus;

	    if (IsSelected(node)) {
		values.foreground = PRIMPTR(w)->font_color;
		values.background = PRIMPTR(w)->input_focus_color;

		/* We must temporarily set the fontcolor to the item bg color
		 * so that we can see the label.  We'll restore it later.
		 */
		saveFG = olgx_get_single_color(attr->ginfo, OLGX_BLACK);
		restoreFG = True;
		olgx_set_single_color(attr->ginfo, OLGX_BLACK, 
		    (unsigned long)PRIMPTR(w)->input_focus_color, OLGX_SPECIAL);
		
	    } else {
		values.foreground = PRIMPTR(w)->input_focus_color;
		values.background = PRIMPTR(w)->font_color;
	    }

	    if(w->primitive.text_format == OL_SB_STR_REP) {
		valueMask   = (GCForeground | GCBackground | GCFont);
		values.font = ((XFontStruct *)PRIMPTR(w)->font)->fid;
	    } else
		valueMask   = (GCForeground | GCBackground);

	    back_gc		= XtGetGC((Widget)w, valueMask, &values);

	    tmp			= values.foreground;
	    values.foreground	= values.background;
	    values.background	= tmp;
	    fore_gc		= XtGetGC((Widget)w, valueMask, &values);
	}

    } else {				/* not drawing focus_item */
        attr = PANEPTR(w)->attr_normal;

	if (IsSelected(node) && sensitive) { /* If insensitive, NO selection */
	    back_gc = PANEPTR(w)->gc_normal;
	    fore_gc = PANEPTR(w)->gc_inverted;

            /* We must temporarily set the fontcolor to the item bg color
             * so that we can see the label.  We'll restore it later.
             */  
            saveFG = olgx_get_single_color(attr->ginfo, OLGX_BLACK); 
	    restoreFG = True;
            olgx_set_single_color(attr->ginfo, OLGX_BLACK, 
                        (unsigned long)w->core.background_pixel, OLGX_SPECIAL); 

	} else {
	    back_gc = PANEPTR(w)->gc_inverted;
	    fore_gc = PANEPTR(w)->gc_normal;
	}
    }

    /* Paint background with background color */

    XFillRectangle(dpy, win, back_gc, x, y, width, height);

    x		+= hBorder;
    y		+= vBorder;
    width	-= 2 * hBorder;
    height	-= vBorder;

    if (IsCurrent(node)) {
	if (OlgIs3d(w->core.screen)) {
	    olgx_draw_box(attr->ginfo, win, x, y, width, height, 
			state | OLGX_INVOKED, 1);

	} else {
	    XRectangle	rects [4];

	    rects [0].x = x;
	    rects [0].y = y;
	    rects [0].width = width;
	    rects [0].height = vBorder;

	    rects [1].x = x + width - hBorder;
	    rects [1].y = y + vBorder;
	    rects [1].width = hBorder;
	    rects [1].height = height - vBorder;

	    rects [2].x = x;
	    rects [2].y = y + height - vBorder;
	    rects [2].width = width - hBorder;
	    rects [2].height = vBorder;

	    rects [3].x = x;
	    rects [3].y = y + vBorder;
	    rects [3].width = hBorder;
	    rects [3].height = height - vBorder * 2;

	    XFillRectangles(dpy, win, fore_gc, rects, 4);

	}
    }

    /* update bounding box for (re)drawing selected area inside of 3D item */

    x		+= hBorder * 2;
    y		+= vBorder * 2;
    width	-= hBorder * 4;
    height	-= vBorder * 4;

    if (OlgIs3d(w->core.screen) && IsSelected(node) && sensitive)
	XFillRectangle(dpy, win, back_gc, x, y, width, height);

    /* Now draw text / image */
    if (TextPosition(w) == OL_RIGHT) {  /* draw glyph first, then text */
        if (HasGlyph(ItemPtr(node))) {
            OlgxPixmapLbl *pixmapLbl = Item2PixmapLbl(w, ItemPtr(node));

            OlgxDrawImageButton(XtScreen(w), XtWindow(w), attr, x, y,
		width, height, (XtPointer) pixmapLbl, OL_LABEL,
		state | OLGX_LABEL_IS_XIMAGE);

            if (Align(w)) {
                offset = PANEPTR(w)->aligned_pos;
                x += offset;
            } else {
                Dimension width, h;

		OlgxSizePixmapLabel(scr, PANEPTR(w)->attr_normal, 
		    (XtPointer) pixmapLbl, &width, &h);
                offset = width + PANEPTR(w)->space;
                x += offset;
            }
        }
        if (HasString(ItemPtr(node)))
	    OlgxDrawTextButton(XtScreen(w), XtWindow(w), attr, x, y,
		width - offset, height,
		(XtPointer)Item2TextLbl(w, ItemPtr(node)), OL_LABEL, state);

    } else {      /* position == OL_LEFT ; text first, then glyph */
        if (HasString(ItemPtr(node))) {
            OlgxTextLbl *textLbl = Item2TextLbl(w, ItemPtr(node));

	    OlgxDrawTextButton(XtScreen(w), XtWindow(w), attr, x, y,
		width, height, (XtPointer) textLbl, OL_LABEL, state);

            if (Align(w)) {
                offset = PANEPTR(w)->aligned_pos;
                x += offset;
            } else {
                Dimension width, h;

		OlgxSizeTextLabel(XtScreen(w), PANEPTR(w)->attr_normal,
		    (XtPointer) textLbl, &width, &h);
                offset = width + PANEPTR(w)->space;
                x += offset;
            }
        }
        if (HasGlyph(ItemPtr(node)))
	    OlgxDrawImageButton(XtScreen(w), XtWindow(w), attr, x, y,
		width - offset, height,
		(XtPointer)Item2PixmapLbl(w, ItemPtr(node)),
		OL_LABEL, state | OLGX_LABEL_IS_XIMAGE);
    }

    if (releaseGCs) {		/* if temp GC's created, release them */
	XtReleaseGC((Widget)w, back_gc);
	XtReleaseGC((Widget)w, fore_gc);
    }

    if (restoreFG)	/* Restore Graphics_info FG color */
	olgx_set_single_color(attr->ginfo, OLGX_BLACK, saveFG, OLGX_SPECIAL);

    XFlush(XtDisplay(w));
}

/******************************function*header****************************
   FreeList- move any items to free list by deleting them.  Widget's offset
	array is freed when all items deleted.
*/
static void
FreeList (ListPaneWidget w)
{
    int i;

    for (i = NumItems(w) - 1; i >= 0; i--)
	DeleteNode(w, OffsetOf(w, i));
}

/******************************function*header****************************
    InsertNode-
     1	get node (either from free list of at end of 'store')
     2	inc number of nodes in 'store'
     3	inc indices in widget's portion of list above inserted node
     4	insert node (offset) into widget's list of offsets
     5	return node

	Node is inserted before refOffset where refOffset == 0 means append
	to list (as specified from OL R1.0).  Since '0' is reserved for
	this purpose, node returned must be 'overstated' by 1; that is,
	the very 1st offset returned is '1' rather than '0'.  As a result,
	Node '0' is unused space.
*/
static int
InsertNode (ListPaneWidget w, int refOffset)
{
    _OlHead	head = HEADPTR(w);
    List	list = THE_LIST;
    int		node;
    int		refIndex;

    /* inc num_nodes 1st so '0' is never used (see func header) */
    list->num_nodes++;

    /* Get node offset for new item */
    if (list->free.num_elements > 0)
    {
	node = (int)list->free.array[0]; /* take node off front of array */
	_OlArrayDelete(&(list->free), 0); /* delete it from front of array */

    } else {
	/* increase size of Node store if necessary */
	if (list->num_nodes >= list->num_slots)
	{
	    list->num_slots += (list->num_slots / 2) + 2;
	    list->nodes = (Node *)XtRealloc((char *)list->nodes,
					list->num_slots * sizeof(Node));
	}
	node = list->num_nodes;
    }

    /* If this is the 1st node or inserting before Head, make it the head */
    if ((HeadOffset(w) == NULL_OFFSET) || (HeadOffset(w) == refOffset))
	HeadOffset(w) = node;

    /* Increment indices in widget's portion of the list. */

    if (refOffset == 0)			/* inserting at end-of-list */
    {
	refIndex = NumItems(w);		/* 'beyond' end-of-list */

    } else {
	int i;

	refIndex = IndexOf(refOffset);

	/* Increment index of all nodes from reference node and up */
	for (i = refIndex; i < NumItems(w); i++)
	    IndexOf(OffsetOf(w, i))++;
    }

    IndexOf(node) = refIndex;		/* index of new node gets refIndex */

    /* Insert new node into widget's list of offsets (nodes). */
    _OlArrayInsert(&(head->offsets), refIndex, node);

    return(node);
}

/*
 *******************************************************************
  Item2TextLbl- populate a OlgxTextLbl struct with OlListItem data
 *******************************************************************
*/
static OlgxTextLbl *
Item2TextLbl (ListPaneWidget w, OlListItem *item)
{
    static OlgxTextLbl text_lbl;

    text_lbl.label		= item->label;
    text_lbl.text_format	= PRIMPTR(w)->text_format;
    text_lbl.font		= PRIMPTR(w)->font;
    text_lbl.qualifier          = NULL;
    text_lbl.meta               = False;
    text_lbl.accelerator	= NULL;
    text_lbl.mnemonic		= item->mnemonic;
    text_lbl.justification	= TL_LEFT_JUSTIFY;
    text_lbl.flags		= 0;

    return (&text_lbl);
}

/*
 ***********************************************************************
  Item2PixmapLbl-  populate a OlgxPixmapLbl struct with OlListItem data
 ***********************************************************************
*/
static OlgxPixmapLbl *
Item2PixmapLbl(ListPaneWidget w, OlListItem *item)
{
    static OlgxPixmapLbl pix_lbl;
 
    pix_lbl.label.image         = item->glyph;
    pix_lbl.justification       = TL_LEFT_JUSTIFY;
    pix_lbl.type                = PL_IMAGE;
    pix_lbl.flags           = 0;
    return (&pix_lbl);
}

/******************************function*header****************************
    MaintainWidth- the width of the widest item must be kept.  If this
		is larger than the current width of the widget, make
    geometry request.  If item == NULL, use established width in max_width.
    This is the case for updateView: when updateView goes from False to True,
    we must consider updating the geometry.
*/
static void
MaintainWidth (ListPaneWidget w, OlListItem *item)
{
    Dimension width = 0;
    Dimension height;		/* don't care */

    if (item == NULL)
    {
	width	= PANEPTR(w)->max_width;

    } else {
        Dimension string_w = 0;
        Dimension glyph_w = 0;
 
        if (HasGlyph(item))
                OlgxSizePixmapLabel(XtScreen(w), PANEPTR(w)->attr_normal,
                        (XtPointer)Item2PixmapLbl(w, item), &glyph_w, &height);
        if (HasString(item))
                OlgxSizeTextLabel(XtScreen(w), PANEPTR(w)->attr_normal,
			(XtPointer)Item2TextLbl(w, item), &string_w, &height);
        width = string_w + glyph_w ;
        if (string_w && glyph_w) {      /* both string & image do exist */
                if (TextPosition(w) == OL_RIGHT) {
                        _OlAssignMax(PANEPTR(w)->aligned_pos,
                                (Dimension)(glyph_w + PANEPTR(w)->space));
                        _OlAssignMax(PANEPTR(w)->max_comp_width,
                                string_w);
                }
                else {
                        _OlAssignMax(PANEPTR(w)->aligned_pos,
                                (Dimension)(string_w + PANEPTR(w)->space));
                        _OlAssignMax(PANEPTR(w)->max_comp_width,
                                glyph_w);
                }
                if (Align(w))
                        width = PANEPTR(w)->aligned_pos +
                                PANEPTR(w)->max_comp_width;
                else
                        width += PANEPTR(w)->space;
        }
        /* keep width of widest item: */
        _OlAssignMax(PANEPTR(w)->max_width, width);
    }

    /* consider adjusting widget width */
    if (PANEPTR(w)->recompute_width && PANEPTR(w)->update_view)
    {
	width = ItemWidth(w, width);		/* include padding */
	if (width > w->core.width)		/* grow, never shrink */
	{
	    XtWidgetGeometry request;
#ifdef sun
        /* Check to see if prefMaxWidth is zero (than use old behavior and
         * grow to accomodate widest item)  OR if prefMaxWidth is NONzero
         * AND the width of this item will fit into the maxWidth.
         */
          if ( !(PANEPTR(w)->pref_max_width) ||
              (PANEPTR(w)->pref_max_width &&
                        (width < (PANEPTR(w)->pref_max_width))) )
              {
                /* Request a size change...*/

                request.request_mode = CWWidth;
                request.width        = width;
                (void)XtMakeGeometryRequest((Widget)w, &request, NULL);
            }
#else

	    request.request_mode = CWWidth;
	    request.width	 = width;
	    (void)XtMakeGeometryRequest(w, &request, NULL);
#endif
	}
    }
}

/*****************************function*header****************************
   ScollView- called when view scrolls.  Maintain focus_item in view.
*/
static void
ScrollView (ListPaneWidget w)
{
    if ((FocusItem(w) != NULL_OFFSET) && !InView(w, FocusItem(w)))
    {
	/* turn off current focus_item, adjust it & make it focus'ed */

	if (PRIMPTR(w)->has_focus)
	    MakeUnfocused(FocusItem(w));
	
	/* set it to Top or Bottom */
	FocusItem(w) = AboveView(w, FocusItem(w)) ? Top(w) :
				OffsetOf(w, TopIndex(w) + ViewHeight(w) - 1);

	if (PRIMPTR(w)->has_focus)
	    MakeFocused(FocusItem(w));
    }
	if (XtIsRealized((Widget) w))
       Redisplay((Widget)w, (XEvent *)NULL, (Region)NULL);
}

/*****************************function*header****************************
   Search- 
*/
static void
Search (ListPaneWidget w, char ch)
{
    int node;
    int cnt;

    DPRINT((stderr, "Searching for '%c'\n", ch));

    /* Interpret empty list as failed search */
    if (EmptyList(w))
    {
	_OlBeepDisplay((Widget)w, 1);
	return;
    }

    if ((SearchItem(w) == NULL_OFFSET) ||
      (IndexOf(SearchItem(w)) + 1 == NumItems(w)))
    {
	node = HeadOffset(w);

    } else {
	node = Next(w, SearchItem(w));
    }

    /* Loop thru all items.  This means that the current search_item (if
	any) will be visited again (as it should be.  It's as though
	selected item has been reselected).
    */
    for (cnt = NumItems(w); cnt > 0; cnt--)
    {
	if ((ItemPtr(node)->label != NULL) &&
	    (((char *)(ItemPtr(node)->label))[0] == ch))
	{
	    SearchItem(w) = node;	/* update search_item */
	    CurrentNotify(w, node);	/* call any callbacks */
	    return;
	}

	node = (IndexOf(node) + 1 == NumItems(w)) ?
					HeadOffset(w) : Next(w, node);
    }

    /* Falling thru means search failed */
    _OlBeepDisplay((Widget)w, 1);
}

/*****************************function*header****************************
   SelectOrAdjustItem- called when item is SELECTed or ADJUSTed (from
	mouse of keyboard)

     If editing, finish up by calling verification callback.  Then:

     1	consider selection ownership
     2	consider highlighting
     3	consider input focus
     4	call any callbacks

     +	starting place is always important
     +	selection ownership is only meaningful if we're selectable
     +	input focus is always set on SELECT (regardless of selectability)
*/
static void
SelectOrAdjustItem (ListPaneWidget w, int node, OlVirtualName type, 
					Time time, XEvent *xevent, int event_type)
{
    int prev_focus_node = FocusItem(w); 
#define IsSELECT(type) ( (type) == OL_SELECT )

    DPRINT((stderr, "selected node= %s \n", ItemPtr(node)->label));

    if (PANEPTR(w)->editing && (IsSELECT(type) || PANEPTR(w)->selectable)) {
		OlTextFieldVerify verify;
		Arg arg;

		XtSetArg(arg, XtNstring, &(verify.string));
		XtGetValues(TextField(w), &arg, 1);		/* get string */

		verify.ok = True;				/* although not used */

		XtCallCallbacks(TextField(w), XtNverification, &verify);
		XtFree(verify.string);				/* free string */
    }

    /* Consider selection ownership & highlighting:
	1st, only meaningful if selectable.
	Then, if this is the only highlighted item, don't reselect it.
	This prevents highlighting an already highlighted item.
    */
    if (PANEPTR(w)->selectable && ((event_type != MotionNotify) || (event_type == 0))&&
      !(IsSELECT(type) && IsSelected(node) &&
	  (PANEPTR(w)->items_selected == 1))) {
		/* consider selection-ownership */
		if (PANEPTR(w)->items_selected > 0)	{ /* ie. we own the selection */
			if (IsSELECT(type))			/* clears any/all others */
			ClearSelection(w);

		} else if (!PANEPTR(w)->own_selection) {
			PANEPTR(w)->own_selection =
			XtOwnSelection((Widget)w, XA_PRIMARY, time,
				ConvertSelection, LoseSelection, SelectionDone);
		}
		/* Item Highlighting:
		   test for ownership again in case attempt above fails
		*/
		if (PANEPTR(w)->own_selection)
			ToggleItemState(w, node);
    }

    if (IsSELECT(type)) {				/* call any callbacks */
		if (PANEPTR(w)->slist_mode == OL_NONE) {
		   if (_OlMouseless((Widget)w)) {
			  if (FocusItem(w) != NULL_OFFSET) {
				 MakeUnfocused(FocusItem(w));
				 DrawItem(w, FocusItem(w), XtIsSensitive((Widget)w));
			  }
			  FocusItem(w) = node;
			  MakeFocused(node);
			  DrawItem(w, node, XtIsSensitive((Widget)w));
		   }
		   CurrentNotify(w, node);
		   if(prev_focus_node != FocusItem(w))
			UpdateSuperCaret(w);
		   return;
		}
	    if (FocusItem(w) != NULL_OFFSET && _OlMouseless((Widget)w)) {
		   UpdateItemFocus(w, FocusItem(w), False);
		   DrawItem(w, FocusItem(w), ITEMSENSITIVE(w, FocusItem(w)));
	    }

		/* remember to write a function to determine if item is valid */
		/* also ex and non-ex mode */
		switch  (PANEPTR(w)->slist_mode) {

		case OL_EXCLUSIVE_NONESET:
			if (PANEPTR(w)->current_item) {
			  if (PANEPTR(w)->current_item == node &&
				  NodePtr(PANEPTR(w)->current_item)->item_sensitive) {
			    OlSlistCallbackStruct cbdata;

			    ItemPtr(node)->attr &= ~OL_LIST_ATTR_CURRENT;
			    NodePtr(node)->item_current = False;
				if (_OlMouseless((Widget)w))
		   		   UpdateItemFocus(w, PANEPTR(w)->current_item, True);
				DrawItem(w, PANEPTR(w)->current_item, ITEMSENSITIVE(w, PANEPTR(w)->current_item));
			    cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
			    cbdata.reason = OL_REASON_ITEM_NOT_CURRENT; 
			    cbdata.mode = OL_EXCLUSIVE_NONESET;
			    cbdata.event = xevent;
			    cbdata.item = (OlSlistItemPtr)node;
			    cbdata.item_pos = IndexOf(node) + 1;
			    *(cbdata.cur_items) = (OlSlistItemPtr)node;
			    cbdata.num_cur_items = 1;
			    cbdata.cur_items_pos = (int *) NULL;
			    ItemNotCurrentCallback(w, &cbdata);
			    PANEPTR(w)->current_item = 0;
			    XtFree((char *)cbdata.cur_items);
				break;
			  }
			  else {
			    OlSlistCallbackStruct cbdata;

			    if (PANEPTR(w)->current_item &&
					NodePtr(PANEPTR(w)->current_item)->item_sensitive) {
			      ItemPtr(PANEPTR(w)->current_item)->attr &= ~OL_LIST_ATTR_CURRENT;
			      NodePtr(PANEPTR(w)->current_item)->item_current = False;
			      if (IsFocused(PANEPTR(w)->current_item) && 
								_OlMouseless((Widget)w))
		   		     UpdateItemFocus(w, PANEPTR(w)->current_item, False);
				  DrawItem(w, PANEPTR(w)->current_item, ITEMSENSITIVE(w, PANEPTR(w)->current_item));
			      cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
			      cbdata.reason = OL_REASON_ITEM_NOT_CURRENT; 
			      cbdata.mode = OL_EXCLUSIVE_NONESET;
			      cbdata.event = xevent;
			      cbdata.item = (OlSlistItemPtr)PANEPTR(w)->current_item;
			      cbdata.item_pos = IndexOf(PANEPTR(w)->current_item) + 1;
			      *(cbdata.cur_items) = (OlSlistItemPtr)PANEPTR(w)->current_item;
			      cbdata.num_cur_items = 1;
			      cbdata.cur_items_pos = (int *) NULL;
			      ItemNotCurrentCallback(w, &cbdata);
			      XtFree((char *)cbdata.cur_items);
			      
			    } /* end if */
			  } /* end else */
			} /* end if */

		    if (NodePtr(node)->item_sensitive) {
			  OlSlistCallbackStruct cbdata;

			  ItemPtr(node)->attr |= OL_LIST_ATTR_CURRENT;
			  NodePtr(node)->item_current = True;
			  if (_OlMouseless((Widget)w))
		   	     UpdateItemFocus(w, node, True);
			  DrawItem(w, node, ITEMSENSITIVE(w, node));
			  PANEPTR(w)->current_item = node;
			  cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
			  cbdata.reason = OL_REASON_ITEM_CURRENT; 
			  cbdata.mode = OL_EXCLUSIVE_NONESET;
			  cbdata.event = xevent;
			  cbdata.item = (OlSlistItemPtr)node;
			  cbdata.item_pos = IndexOf(node) + 1;
			  *(cbdata.cur_items) = (OlSlistItemPtr)node;
			  cbdata.num_cur_items = 1;
			  cbdata.cur_items_pos = (int *) NULL;
			  ItemCurrentCallback(w, &cbdata);
			  XtFree((char *)cbdata.cur_items);
			}
			break;
		case OL_EXCLUSIVE:
			if (PANEPTR(w)->current_item && 
					   PANEPTR(w)->current_item == node) {
			   if (_OlMouseless((Widget)w)) {
		   	      UpdateItemFocus(w, node, True);
				  DrawItem(w, node, ITEMSENSITIVE(w, node));
			   }
			   break;
			}

			if (PANEPTR(w)->current_item && PANEPTR(w)->current_item != node
				&& NodePtr(PANEPTR(w)->current_item)->item_sensitive) {
			   OlSlistCallbackStruct cbdata;

			   ItemPtr(PANEPTR(w)->current_item)->attr &= ~OL_LIST_ATTR_CURRENT;
			   NodePtr(PANEPTR(w)->current_item)->item_current = False;
			   if (IsFocused(PANEPTR(w)->current_item) && 
							 _OlMouseless((Widget)w))
		   	      UpdateItemFocus(w, PANEPTR(w)->current_item, False);
			   DrawItem(w, PANEPTR(w)->current_item, ITEMSENSITIVE(w, PANEPTR(w)->current_item));
			   cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
			   cbdata.reason = OL_REASON_ITEM_NOT_CURRENT; 
			   cbdata.mode = OL_EXCLUSIVE;
			   cbdata.event = xevent;
			   cbdata.item = (OlSlistItemPtr)PANEPTR(w)->current_item;
			   cbdata.item_pos = IndexOf(PANEPTR(w)->current_item) + 1;
			   *(cbdata.cur_items) = (OlSlistItemPtr)PANEPTR(w)->current_item;
			   cbdata.num_cur_items = 1;
			   cbdata.cur_items_pos = (int *) NULL;
			   ItemNotCurrentCallback(w, &cbdata);
			   XtFree((char *)cbdata.cur_items);
			}

		    if (NodePtr(node)->item_sensitive) {
			  OlSlistCallbackStruct cbdata;

			  ItemPtr(node)->attr |= OL_LIST_ATTR_CURRENT;
			  NodePtr(node)->item_current = True;
			  if (_OlMouseless((Widget)w))
		   	     UpdateItemFocus(w, node, True);
			  DrawItem(w, node, ITEMSENSITIVE(w, node));
			  PANEPTR(w)->current_item = node;
			  cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
			  cbdata.reason = OL_REASON_ITEM_CURRENT; 
			  cbdata.mode = OL_EXCLUSIVE;
			  cbdata.event = xevent;
			  cbdata.item = (OlSlistItemPtr)node;
			  cbdata.item_pos = IndexOf(node) + 1;
			  *(cbdata.cur_items) = (OlSlistItemPtr)node;
			  cbdata.num_cur_items = 1;
			  cbdata.cur_items_pos = (int *) NULL;
			  ItemCurrentCallback(w, &cbdata);
			  XtFree((char *)cbdata.cur_items);
			}
			break;
		case OL_NONEXCLUSIVE:
		/* check if its not current first */
		   if (PANEPTR(w)->num_current_items > 0) {
		   int i;
		   Boolean done = False;

		   for (i = 0; i < PANEPTR(w)->num_current_items; i++) {
			/* First try to find it in the current list */
			  if (PANEPTR(w)->current_items[i] == node) { /* found it */
			   	 PANEPTR(w)->current_item = 0;
				/* Is it the last current item ?? */
				 if (PANEPTR(w)->num_current_items == 1) {
				 	XtFree((char *)PANEPTR(w)->current_items);
			   		PANEPTR(w)->num_current_items = 0;
					done = True;
					break; 
				 }

				 { /* Otherwise move everything up one slot */
				 int j;
			        
				 for (j = i; j < (PANEPTR(w)->num_current_items - 1); j++)
					PANEPTR(w)->current_items[j] = 
						            PANEPTR(w)->current_items[j + 1];

			        PANEPTR(w)->current_items = (int *) XtRealloc(
						     (char *)PANEPTR(w)->current_items, 
						     (PANEPTR(w)->num_current_items - 1) 
						     * sizeof(int));  /* allocate 1 less */
			   		PANEPTR(w)->num_current_items--;
					done = True;
					break; 

					}
				 } /* end if */
			  } /* end for */

			  /* Not found in current list so, Add this to the current list */
			  if (!done) {
			     OlSlistCallbackStruct nonex_cbdata;

			     PANEPTR(w)->current_items = (int *) XtRealloc(
					 (char *)PANEPTR(w)->current_items, 
					 (PANEPTR(w)->num_current_items + 1) /* allocate 1 more */
					 * sizeof(int));
			     PANEPTR(w)->current_item = node;
			     PANEPTR(w)->current_items[PANEPTR(w)->num_current_items] = node;
			     PANEPTR(w)->num_current_items++;
				 ItemPtr(node)->attr |= OL_LIST_ATTR_CURRENT;
				 NodePtr(node)->item_current = True;
				 if (_OlMouseless((Widget)w))
		   	        UpdateItemFocus(w, node, True);
				 DrawItem(w, node, ITEMSENSITIVE(w, node));
			     nonex_cbdata.reason = OL_REASON_ITEM_CURRENT; 
			     nonex_cbdata.mode = OL_NONEXCLUSIVE;
			     nonex_cbdata.event = xevent;
			     nonex_cbdata.item = (OlSlistItemPtr)node;
			     nonex_cbdata.item_pos = IndexOf(node) + 1;
			     nonex_cbdata.cur_items = (OlSlistItemPtr *)PANEPTR(w)->current_items;
			     nonex_cbdata.num_cur_items = PANEPTR(w)->num_current_items;
			     nonex_cbdata.cur_items_pos = (int *) NULL;
			     ItemCurrentCallback(w, &nonex_cbdata);
			  }
			  else {
				 OlSlistCallbackStruct cbdata;

				 ItemPtr(node)->attr &= ~OL_LIST_ATTR_CURRENT;
				 NodePtr(node)->item_current = False;
				 if (_OlMouseless((Widget)w))
		   	        UpdateItemFocus(w, node, True);
				 DrawItem(w, node, ITEMSENSITIVE(w, node));
				 cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
			     cbdata.reason = OL_REASON_ITEM_NOT_CURRENT; 
			     cbdata.mode = OL_NONEXCLUSIVE;
			     cbdata.event = xevent;
			     cbdata.item = (OlSlistItemPtr)node;
			     cbdata.item_pos = IndexOf(node) + 1;
			     *(cbdata.cur_items) = (OlSlistItemPtr)node;
			     cbdata.num_cur_items = 1;
			     cbdata.cur_items_pos = (int *) NULL;
			     ItemNotCurrentCallback(w, &cbdata);
				 XtFree((char *)cbdata.cur_items);
			  }

		   } 
		   else {
			   OlSlistCallbackStruct nonex_cbdata;

			   PANEPTR(w)->current_items = (int *) XtMalloc(sizeof(int));
			   PANEPTR(w)->current_item = node;
			   PANEPTR(w)->current_items[PANEPTR(w)->num_current_items] = node;
			   PANEPTR(w)->num_current_items++;
			   ItemPtr(node)->attr |= OL_LIST_ATTR_CURRENT;
			   NodePtr(node)->item_current = True;
			   if (_OlMouseless((Widget)w))
		   	      UpdateItemFocus(w, node, True);
			   DrawItem(w, node, ITEMSENSITIVE(w, node));
			   nonex_cbdata.reason = OL_REASON_ITEM_CURRENT; 
			   nonex_cbdata.mode = OL_NONEXCLUSIVE;
			   nonex_cbdata.event = xevent;
			   nonex_cbdata.item = (OlSlistItemPtr)node;
			   nonex_cbdata.item_pos = IndexOf(node) + 1;
			   nonex_cbdata.cur_items = (OlSlistItemPtr *)PANEPTR(w)->current_items;
			   nonex_cbdata.num_cur_items = PANEPTR(w)->num_current_items;
			   nonex_cbdata.cur_items_pos = (int *) NULL;
			   ItemCurrentCallback(w, &nonex_cbdata);
		   }
	       break;
		default:
	       break;
		}
	}


	if(prev_focus_node != FocusItem(w))
		UpdateSuperCaret(w);
#undef IsSELECT
}

/*****************************function*header****************************
 * ToggleItemState- [un]highlights item & counts selected items.
 *		item state is toggled & select count updated accordingly
 *
 * 'node' is viewable when called from the Button event handler (since
 * they react to user interaction (ie. the items are in view).  But the
 * AutoScroll callback function desires to Toggle the state 1st, then make
 * the item viewable (then Redisplay) to reduce flashes.
 */
static void
ToggleItemState (ListPaneWidget w, int node)
{
    if (IsSelected(node))
    {
		ItemPtr(node)->attr &= ~OL_B_LIST_ATTR_SELECTED;
		PANEPTR(w)->items_selected--;
    } else {
		ItemPtr(node)->attr |= OL_B_LIST_ATTR_SELECTED;
		PANEPTR(w)->items_selected++;
    }

    if (InView(w, node))
	   DrawItem(w, node, ITEMSENSITIVE(w, node));
}

/*****************************function*header****************************
   UpdateSBar- update scrollbar values.
    
    If view in unfilled, set scrollbar to insensitive.
    sliderMax = num of items (sliderMin is always default 0).
    sliderValue is "array" index of top item.
    proportionLength is smaller of view & number of item.
*/
static void
UpdateSBar (ListPaneWidget w)
{
    Arg		args[4];
    Cardinal	cnt = 0;

    if (NumItems(w) > ViewHeight(w))
    {
	XtSetArg(args[cnt], XtNsensitive, True); cnt++;
	XtSetArg(args[cnt], XtNproportionLength, ViewHeight(w)); cnt++;
	XtSetArg(args[cnt], XtNsliderMax, NumItems(w)); cnt++;
	XtSetArg(args[cnt], XtNsliderValue, TopIndex(w)); cnt++;

    } else {				/* Unfilled view: */
	XtSetArg(args[cnt], XtNsensitive, False); cnt++;
	if (EmptyList(w))
	{
	    XtSetArg(args[cnt], XtNproportionLength, 1); cnt++;
	    XtSetArg(args[cnt], XtNsliderMax, 1); cnt++;
	    XtSetArg(args[cnt], XtNsliderValue, 0); cnt++;
	} else {
	    XtSetArg(args[cnt], XtNproportionLength, NumItems(w)); cnt++;
	    XtSetArg(args[cnt], XtNsliderMax, NumItems(w)); cnt++;
	    XtSetArg(args[cnt], XtNsliderValue, TopIndex(w)); cnt++;
	}
    }

    XtSetValues(SBAR(w), args, cnt);
}

/*************************************************************************
 *
 * Class Procedures
 *
 */

/****************************function*header*******************************
    AcceptFocus- if editing, have TextField take focus.  Else, have
	superclass do the work.
*/
static Boolean
AcceptFocus(Widget w, Time *time)
{
    XtAcceptFocusProc accept_focus;

    /* If editing, have TextField take focus */
    if (PANEPTR(w)->editing)
	return ( OlCallAcceptFocus(TextField(w), *time) );

    accept_focus =
	listPaneWidgetClass->core_class.superclass->core_class.accept_focus;

    return ( (accept_focus != NULL) && (*accept_focus)(w, time) );
}

/******************************function*header****************************
 * Destroy-
 */
static void
Destroy(Widget w)
{
    FreeGCs(w);
	if (PANEPTR(w)->slist_mode == OL_NONE)
       FreeList(PANEW(w));
	else {
	   OlSlistUpdateView (XtParent (w), False);
	   OlSlistDeleteAllItems(XtParent(w));
	}

    /* Free clipboard store if we own it.
	(Xt disowns XA_CLIPBOARD  & XA_PRIMARY on our behalf)
    */
    if (PANEPTR(w)->own_clipboard)
    {
	XtFree(Clipboard);
	Clipboard = NULL;
	ClipboardCnt = 0;
    }

    /* shut off auto-scroll timer if it's set */
    if (PANEPTR(w)->timer_id)
	XtRemoveTimeOut(PANEPTR(w)->timer_id);

    /* Since ListPane is a Primitive parent, it's child is
     * not destroyed automatically.
     */
    XtDestroyWidget(TextField(w));
}

/******************************function*header****************************
 * HighlightHandler-
 */
static void
HighlightHandler (Widget w, OlDefine type)
{

    switch (type) {
    case OL_IN :

	if (EmptyList(w))
	    break;
	
	/* if no focus node, make it the top_item */
	if (FocusItem(w) == NULL_OFFSET)
	    FocusItem(w) = Top(w);

	/* highlight the focus_item and draw it */
	MakeFocused(FocusItem(w));
	DrawItem(PANEW(w), FocusItem(w), ITEMSENSITIVE(PANEW(w), FocusItem(w)));
	break;

    case OL_OUT :
	if ((FocusItem(w) != NULL_OFFSET) && 
	 OlSlistIsValidItem(SLISTW(w), FocusItem(w)) && IsFocused(FocusItem(w)))
	{
	    /* turn off highlighting and redraw item */
	    MakeUnfocused(FocusItem(w));
		DrawItem(PANEW(w), FocusItem(w), ITEMSENSITIVE(PANEW(w), FocusItem(w)));
	}
	break;
    }
}

/******************************function*header****************************
 * Initialize-
 */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    Screen	*scr = XtScreenOfObject(new);
    MaskArg	mArgs[10];
    ArgList	mergedArgs = NULL;
    Cardinal	mergedCnt;
    Cardinal	cnt;

    /* Can't easily specify default values for function pointers
	so initialize these here.
    */
    PANEPTR(new)->applAddItem		= ApplAddItem;
    PANEPTR(new)->applDeleteItem	= ApplDeleteItem;
    PANEPTR(new)->applEditClose		= ApplEditClose;
    PANEPTR(new)->applEditOpen		= ApplEditOpen;
    PANEPTR(new)->applTouchItem		= ApplTouchItem;
    PANEPTR(new)->applUpdateView	= ApplUpdateView;
    PANEPTR(new)->applViewItem		= ApplViewItem;

    /* CLIPBOARD / SELECTION / INPUT FOCUS / SEARCHING  RELATED */

    PANEPTR(new)->own_clipboard		= False;
							/* selection */
    PANEPTR(new)->own_selection		= False;
    PANEPTR(new)->items_selected	= 0;
    PANEPTR(new)->first_item	    = (OlSlistItemPtr)NULL;
    PANEPTR(new)->last_item	        = (OlSlistItemPtr)NULL;
    PANEPTR(new)->current_item	    = 0;
    PANEPTR(new)->current_items	= NULL;
    PANEPTR(new)->num_current_items = 0;
    PANEPTR(new)->num_items         = 0;
    PANEPTR(new)->start_index		= INITIAL_INDEX;
    PANEPTR(new)->prev_index		= INITIAL_INDEX;
    PANEPTR(new)->initial_motion	= NO_MOTION;
							/* auto-scroll */
    PANEPTR(new)->scroll		= 0;
    PANEPTR(new)->repeat_rate		= 0;
    PANEPTR(new)->timer_id		= NULL;
							/* input focus */
    FocusItem(new)			= NULL_OFFSET;

							/* searching */
    SearchItem(new)			= NULL_OFFSET;

    PANEPTR(new)->aligned_pos           = 0;
    PANEPTR(new)->max_comp_width        = 0;

    /* COMPUTE ITEM SPACING IN PIXELS

	Offsets for drawing an item are fixed and specified in device-
	independent units.  They are converted to (device-dependent)
	pixel dimensions here.  Conversion is only needed once so is
	done here.
    */
    /* margin: space between pane and item */
    PANEPTR(new)->vert_margin	= ConvertVert(new, ITEM_MARGIN);
    PANEPTR(new)->horiz_margin	= ConvertHoriz(new, ITEM_MARGIN);

    /* padding: total vertical/horizontal padding surrounding label
     * pad + label width (height) = item width (height)
     */
    PANEPTR(new)->vert_pad	= ConvertVert(new, VERT_PADDING);
    PANEPTR(new)->horiz_pad	= ConvertHoriz(new, HORIZ_PADDING);

#ifdef sun
   /* To keep track of the TOTAL width of the ScrollingList, we
    * must know the width of the Scrollbar plus it's offset from
    * the ListPane:
    *   new->core.width + PANEPTR(new)->sb_width_pad = ScrollingList width
    *
    * Note: The offset value was derived from the XtNxOffset resource
    *       which is set in for the Scrollbar in List's Initialize() in List.c
    */
    PANEPTR(new)->sb_width_pad = SBAR(new)->core.width +
                        OlgxScreenPointToPixel(OL_HORIZONTAL, 2, XtScreen(new));
#endif


    /* WIDEST / TALLEST ITEM BOOKKEEPING */

	if(PRIMPTR(new)->text_format == OL_SB_STR_REP){
	    PANEPTR(new)->max_width	= 
			FontWidth(((XFontStruct *)(PRIMPTR(new)->font)));
	    PANEPTR(new)->max_height	= PANEPTR(new)->item_height ?
			PANEPTR(new)->item_height :
			FontHeight(((XFontStruct *)(PRIMPTR(new)->font)));
	}
	else{
		XFontSetExtents *fset_extents;

		fset_extents =
                        XExtentsOfFontSet((XFontSet)
					(PRIMPTR(new)->font));
		PANEPTR(new)->max_width     = 
			fset_extents->max_logical_extent.width;
		PANEPTR(new)->max_height	= PANEPTR(new)->item_height ?
			PANEPTR(new)->item_height :
			fset_extents->max_logical_extent.height;
	}

    /* DRAWING RELATED */

    CreateGCs(PANEW(new));

    /* Can't use a window border for 3-D.  Remove it and adjust the sizes
     * to account for the wider, chiseled line.
     */
    if (OlgIs3d(new->core.screen))
    {
	PANEPTR(new)->vert_margin +=
		(Dimension)OlgxScreenPointToPixel(OL_VERTICAL, 1, scr);
	PANEPTR(new)->vert_pad +=
		(Dimension)(2 * OlgxScreenPointToPixel(OL_VERTICAL, 1, scr));
	PANEPTR(new)->horiz_margin +=
                (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr);
	PANEPTR(new)->horiz_pad +=
                (Dimension)(2 * OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr)); 
	new->core.border_width = 0;
    }

    /* LIST RELATED */

    HEADPTR(new)->offset	= NULL_OFFSET;
    _OlArrayInit(&(HEADPTR(new)->offsets));	/* init array of offsets */
  
    /* VIEW RELATED */

    Top(new)			= NULL_OFFSET;
    PANEPTR(new)->editing	= False;
    PANEPTR(new)->update_view	= True;

    /* PANE WIDTH
     *
     * make initial width equal to the widest character in the font
     */
#ifdef sun

    /* If prefMinWidth and prefMaxWidth are being set to non-zero,
     * then verify that their values are acceptable.
     */
    if (PANEPTR(new)->pref_min_width && PANEPTR(new)->pref_max_width)
        if (PANEPTR(new)->pref_min_width > PANEPTR(new)->pref_max_width)
           {
            OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: prefMinWidth cannot be greater than prefMaxWidth"));
            PANEPTR(new)->pref_min_width = PANEPTR(request)->pref_min_width;
            PANEPTR(new)->pref_max_width = PANEPTR(request)->pref_max_width;
           }

    /*
     * If prefMinWidth is non-zero, then set initial ScrollingList
     * width equal to it.
     */
    if (PANEPTR(new)->pref_min_width && new->core.width == 0)
        /* Verify that prefMinWidth is large enough to accomodate the
         * width of the scrollbar and it's xOffset.
         */
        if (PrefListPaneMinWidth(new) > 0)
            new->core.width = PrefListPaneMinWidth(new);
        else
           {
           OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: prefMinWidth value too small"));
           PANEPTR(new)->pref_min_width = PANEPTR(request)->pref_min_width;
           }
    else
#endif
    if (new->core.width == 0)
	new->core.width = ItemWidth(new, PANEPTR(new)->max_width);

    /* PANE HEIGHT (and SBar height) */
    SetPaneHeight(PANEW(new));

    /* calibrate SBAR for empty list (after ViewHeight & NumItems set) */
    UpdateSBar(PANEW(new));

    /* Add callback to SBar */
    XtAddCallback(SBAR(new), XtNsliderMoved, SBarMovedCB, (XtPointer)new);

    /* create editable text field */
    cnt = 0;
    _OlSetMaskArg(mArgs[cnt], XtNtextFormat, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNimPreeditStyle, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNfont, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNfontColor, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNforeground, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNmaximumSize, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNstring, 0, OL_SOURCE_PAIR); cnt++;
    _OlSetMaskArg(mArgs[cnt], XtNverification, 0, OL_SOURCE_PAIR); cnt++;
    _OlComposeArgList(args, *num_args, mArgs, cnt, &mergedArgs, &mergedCnt);
    TextField(new) = XtCreateWidget("textfield", textFieldWidgetClass,
			new, mergedArgs, mergedCnt);
    XtFree((char *)mergedArgs);

    /* If mouseless is enabled, the TextField widget is removed from the
     * traversal list; it's parent (ListPane) gets focus and sets it to the
     * TextField if required.
     * If mouseless is disabled, we leave the TextField widget on the
     * traversal list, since its removal could leave us with a NULL traversal
     * list. Furthermore, the ListPane will not be on the traversal list,
     * so we don't have the problem that tabbing from the TextField moves
     * to the ListPane, which then sets focus back to the TextField!
     */
    if (_OlMouseless(new))
	_OlDeleteDescendant(TextField(new)); /* delete it from traversal list */
}

/******************************function*header****************************
 * Redisplay-
 */
static void
Redisplay(Widget w, XEvent *event, Region region)
{
    int	i;
    int	node;
	Window win = XtWindow(w);

    /* Must use 2 olgx_draw_box() calls in OLGX to get chiseled look */
    olgx_draw_box(PANEPTR(w)->attr_normal->ginfo, win, 0, 0,
		w->core.width, w->core.height, OLGX_INVOKED, FALSE);
    olgx_draw_box(PANEPTR(w)->attr_normal->ginfo, win, 1 , 1, 
                w->core.width-2, w->core.height-2, OLGX_NORMAL, FALSE); 

    /* check for unfilled view and empty list.  When last item is
	deleted, it must be cleared from the pane here
    */
    if (NumItems(w) < ViewHeight(w))
    {
	XClearArea(XtDisplay(w), XtWindow(w),
	    PANEPTR(w)->horiz_margin,				   /* x */
	    PANEPTR(w)->vert_margin + NumItems(w) * ItemHeight(w), /* y */
	    w->core.width - 2 * PANEPTR(w)->horiz_margin,	/* width */
	    ItemHeight(w) * (ViewHeight(w) - NumItems(w)),	/* height */
	    False);						/* expose's */

    }

    if (EmptyList(w))
       return;

    node = Top(w);			/* start at Top of pane */
    for (i = _OlMin(ViewHeight(w), NumItems(w) - TopIndex(w)); i > 0; i--) {
		Boolean item_sens;
		/* if widget is sensitive and item is not then set sensitive to false */
		if (XtIsSensitive(w) && PANEPTR(w)->slist_mode != OL_NONE && 
							 !(NodePtr(node)->item_sensitive))
	       DrawItem(PANEW(w), node, False);
	    else 
	       DrawItem(PANEW(w), node, XtIsSensitive(w));

	   node = Next(w, node);
    }

}

/******************************function*header****************************
 * RegisterFocus-
 */
static Widget
RegisterFocus (Widget w)
{
    return(SLISTW(w));
}

/******************************function*header****************************
 * GetValuesHook-
 */
static void
GetValuesHook(Widget widget, ArgList args, Cardinal *num_args)
{
ListPaneWidget w = (ListPaneWidget)widget;
int i;
	for (i = 0; i < *num_args; i++) {
		if (strcmp(XtNnumItems, args[i].name) == 0) {
			*((int *)args[i].value) = NumItems(w);
		}
		if (strcmp(XtNscrollingListMode, args[i].name) == 0) {
			*((OlDefine *)args[i].value) = PANEPTR(w)->slist_mode;
		}
		if (strcmp(XtNcurrentItems, args[i].name) == 0) {
		   if (PANEPTR(w)->slist_mode == OL_EXCLUSIVE ||
			   (PANEPTR(w)->slist_mode == OL_EXCLUSIVE_NONESET))
			*((OlSlistItemPtr **)args[i].value) = (OlSlistItemPtr *) &(PANEPTR(w)->current_item);
			else if (PANEPTR(w)->slist_mode == OL_NONEXCLUSIVE)
			*((OlSlistItemPtr **)args[i].value) = (OlSlistItemPtr *)PANEPTR(w)->current_items;
		}
		if (strcmp(XtNnumCurrentItems, args[i].name) == 0) {
			if (PANEPTR(w)->slist_mode == OL_EXCLUSIVE ||
				(PANEPTR(w)->slist_mode == OL_EXCLUSIVE_NONESET))
			*((int *)args[i].value) = 1;
			else if (PANEPTR(w)->slist_mode == OL_NONEXCLUSIVE)
			*((int *)args[i].value) = PANEPTR(w)->num_current_items;
		}
		if (strcmp(XtNfirstViewableItem, args[i].name) == 0) {
			int j;
			for (j = 0; j < NumItems(w); j++) 
			    if (IndexOf((int)PANEPTR(w)->head.offsets.array[j]) 
						   == TopIndex(w)) {
			      *((OlSlistItemPtr *)args[i].value) = 
			         (OlSlistItemPtr)(PANEPTR(w)->head.offsets.array[j]);
				  break;
				}
		}
		if (strcmp(XtNlastViewableItem, args[i].name) == 0) {
			int j;
			for (j = 0; j < NumItems(w); j++) 
			   if (IndexOf((int)PANEPTR(w)->head.offsets.array[j]) 
						  == TopIndex(w)) {
				if ((j + ViewHeight(w)) < NumItems(w))
				{
			            *((OlSlistItemPtr *)args[i].value) = 
					(OlSlistItemPtr)(PANEPTR(w)->head.offsets.array[j + (ViewHeight(w) - 1)]);
				}
				else
				{
			            *((OlSlistItemPtr *)args[i].value) = 
					(OlSlistItemPtr)(PANEPTR(w)->head.offsets.array[NumItems(w) - 1]);
				}
				  break;
			    }
		}
		if (strcmp(XtNscrollingListItems, args[i].name) == 0) {
			*((OlSlistItemPtr **)args[i].value) = 
			   (OlSlistItemPtr *)(PANEPTR(w)->head.offsets.array);
		}
		if (strcmp(XtNviewableItems, args[i].name) == 0) {
			int j;
			for (j = 0; j < NumItems(w); j++) 
			   if (IndexOf((int)PANEPTR(w)->head.offsets.array[j]) 
						   == TopIndex(w)) {
			      *((OlSlistItemPtr **)args[i].value) = 
			         (OlSlistItemPtr *)(&(PANEPTR(w)->head.offsets.array[j]));
				  break;
			   }
		}
	}
}
/******************************function*header****************************
 * SetValues-
 */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
Boolean	redisplay = False;
ListPaneWidget lpw = (ListPaneWidget) new;
Boolean update_view_sb = False;

    if ((PANEPTR(new)->view_height != PANEPTR(current)->view_height) ||
      (new->core.height != current->core.height))
    {
	SetPaneHeight(PANEW(new));
    }

#ifdef sun
/* OLGX_TODO: I think we may want to get rid of all this pref* code */

        /* Let's check to see if the prefMaxWidth resource is being changed...
         */
     if (PANEPTR(new)->pref_max_width != PANEPTR(current)->pref_max_width)
        {
        /* It's changed, now let's check whether it's been set to zero..
         */
        if (PANEPTR(new)->pref_max_width)
           {
           /* It's non zero, Let's make sure it's greater than prefMinWidth..
            */
           if (PANEPTR(new)->pref_max_width >= PANEPTR(new)->pref_min_width)
              {
              /* It is, now check if the new prefMaxWidth is LESS than
               * the current width of the ListPaneWidth
               */
              if ((int)PrefListPaneMaxWidth(new) <
		  (int)current->core.width)
                 {
                 /* It's less, so we must shrink the ListPane width such
                  * that it meets the new, smaller prefMaxWidth constraint.
                  */
                 new->core.width = PrefListPaneMaxWidth(new) ;
                 redisplay = True;
                 }
              else if ((int)current->core.width <
		       (int)ItemWidth(new,PANEPTR(new)->max_width))
                 {
                 /* The new prefMaxWidth is greater than the current ListPane
                  * width, we only want to grow IF there is a list item
                  * which is longer than the current ListPane (so it's being
                  * obscured).  If all items are in fullview, make no size
                  * change.
                  */
                 new->core.width =
		     ((int)ItemWidth(new,PANEPTR(new)->max_width) >
		      (int)PrefListPaneMaxWidth(new)?
		             PrefListPaneMaxWidth(new) :
		             ItemWidth(new, PANEPTR(new)->max_width));
                 redisplay = True;
                 }
              }
           else  
              {
                /* The prefMaxWidth was being set SMALLER than prefMinWidth,
                 * which is not allowed: set prefMaxWidth back to previous
                 * value.
                 */
              OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: prefMaxWidth cannot be less than\
 prefMinWidth")); 
              PANEPTR(new)->pref_max_width = PANEPTR(current)->pref_max_width;
              }
	   }
        else  
           {
           /* prefMaxWidth has been set to 0, revert to old size behavior,
            * where the list will always be the width of the largest item.
            */
            new->core.width = ItemWidth(new, PANEPTR(new)->max_width);
            redisplay = True;
           }
        }   
           
        /* Let's check to see if the prefMinWidth resource is being changed...
         */
 
    if (PANEPTR(new)->pref_min_width != PANEPTR(current)->pref_min_width)
        {
        /* It's being changed, check to see if it's being set to zero...
         */
        if  (PANEPTR(new)->pref_min_width)
           {
           /* It's not, check to make sure that the prefMinWidth is at
            * least large enough such that the ListPaneWidth is greater
            * than zero (non negative!) once the Scrollbar width and offset
            * has been subtracted from the prefMinWidth.
            */
           if (PrefListPaneMinWidth(new) > 0)
              {
              if (PANEPTR(new)->pref_min_width <= PANEPTR(new)->pref_max_width)                 {
                 if ((int)PrefListPaneMinWidth(new) > (int)current->core.width)
                    {
                     new->core.width = PrefListPaneMinWidth(new);
                     redisplay = True;
                    }
                 }   
              else  
                 {
                 OlWarning(dgettext(OlMsgsDomain,
			"ScrollingList: prefMinWidth cannot be greater \
than prefMaxWidth"));
                 PANEPTR(new)->pref_min_width = PANEPTR(current)->pref_min_width;
                 }
              }
           else  
              {
              OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: prefMinWidth value too small"));
              PANEPTR(new)->pref_min_width = PANEPTR(current)->pref_min_width;
              }
          }
        else /* prefMinWidth has been set to 0, revert to old size behavior */
           {
           new->core.width = ItemWidth(new, PANEPTR(new)->max_width);
           redisplay = True;
           }
        }
#endif


    if ((new->core.background_pixel != current->core.background_pixel) ||
      (PRIMPTR(new)->font != PRIMPTR(current)->font) ||
      (PRIMPTR(new)->font_color != PRIMPTR(current)->font_color) ||
      (PRIMPTR(new)->foreground != PRIMPTR(current)->foreground) ||
      (PRIMPTR(new)->input_focus_color != PRIMPTR(current)->input_focus_color))
    {
	/* pass on changes to TextField */
	ArgList		mergedArgs;
	Cardinal	mergedCnt;

	_OlComposeArgList(args, *num_args, TFieldArgs, XtNumber(TFieldArgs),
		      &mergedArgs, &mergedCnt);

	if (mergedCnt != 0) {
	    XtSetValues(TextField(new), mergedArgs, mergedCnt);
	    XtFree((char *)mergedArgs);
	}

	/* re-create ListPane GC's */
	FreeGCs(new);
	CreateGCs(PANEW(new));

	redisplay = True;
    }

/* If font or itemHeight changed, recompute max_height .. */
    if ((PRIMPTR(new)->font != PRIMPTR(current)->font) ||
        (PANEPTR(new)->item_height != PANEPTR(current)->item_height)) {
          if (PANEPTR(new)->item_height == 0) {
	  /* Use font's height .... */
                if(PRIMPTR(new)->text_format == OL_SB_STR_REP) {
                    	PANEPTR(new)->max_height =
                                FontHeight(((XFontStruct *)
                                        (PRIMPTR(new)->font)));
                }
                else {
                        XFontSetExtents *fset_extents;
 
                        fset_extents =
                        XExtentsOfFontSet((XFontSet) (PRIMPTR(new)->font));
                        PANEPTR(new)->max_height        =
                                fset_extents->max_logical_extent.height;
                }
	  }
          else
                PANEPTR(new)->max_height = PANEPTR(new)->item_height;
          SetPaneHeight(PANEW(new));
    }

    if ((PANEPTR(new)->space != PANEPTR(current)->space) ||
        (PANEPTR(new)->align != PANEPTR(current)->align) ||
        (PANEPTR(new)->position != PANEPTR(current)->position)) {
                SetPaneWidth(PANEW(new));
                redisplay = True; /* needed if new width < core width */
    }

    if (PANEPTR(new)->viewable_items != PANEPTR(current)->viewable_items) {
		PANEPTR(new)->viewable_items = PANEPTR(current)->viewable_items;
        OlWarning(dgettext(OlMsgsDomain, 
			"ScrollingList: cannot change XtNviewableItems"));
	}
    if (PANEPTR(new)->scrolling_list_items != PANEPTR(current)->scrolling_list_items) {
		PANEPTR(new)->scrolling_list_items = PANEPTR(current)->scrolling_list_items;
        OlWarning(dgettext(OlMsgsDomain, 
			"ScrollingList: cannot change XtNscrollingListItems"));
	}
    if (PANEPTR(new)->slist_mode != PANEPTR(current)->slist_mode) {
		PANEPTR(new)->slist_mode = PANEPTR(current)->slist_mode;
        OlWarning(dgettext(OlMsgsDomain, 
			"ScrollingList: cannot change XtNscrollinglistMode"));
	}
    if (PANEPTR(new)->first_item != PANEPTR(current)->first_item) {
	  if ((IndexOf((int)PANEPTR(new)->first_item) <= 
					   (NumItems(lpw) - ViewHeight(lpw)))) {
		Top(new) = (int)PANEPTR(new)->first_item;
		Last(new) = (OlSlistItemPtr) (OffsetNode(lpw, Top(new), 
						(ViewHeight(lpw) - 1)));
		update_view_sb = True;
		redisplay = True;
	  }
	  else 
		PANEPTR(new)->first_item = PANEPTR(current)->first_item;
	}
    if (PANEPTR(new)->last_item != PANEPTR(current)->last_item) {
	  if ((IndexOf((int)PANEPTR(new)->last_item) >= (ViewHeight(lpw) - 1))) {
	     int new_top_item = OffsetNode(lpw, Top(lpw), 
               IndexOf((int)PANEPTR(new)->last_item) - TopIndex(lpw) 
			   - ViewHeight(lpw) + 1);

	     Top(lpw) = new_top_item;
	     PANEPTR(lpw)->first_item = (OlSlistItemPtr) new_top_item;
	     PANEPTR(lpw)->last_item = (OlSlistItemPtr) PANEPTR(new)->last_item;
	     update_view_sb = True;
	     redisplay = True;
	  }
	  else 
		PANEPTR(new)->last_item = PANEPTR(current)->last_item;
	}

	if (update_view_sb) {
      if ((FocusItem(lpw) != NULL_OFFSET) && !InView(lpw, FocusItem(lpw))) {
		/* turn off current focus_item, adjust it & make it focus'ed */

		if (PRIMPTR(lpw)->has_focus)
			MakeUnfocused(FocusItem(lpw));
		
		/* set it to Top or Bottom */
		FocusItem(lpw) = AboveView(lpw, FocusItem(lpw)) ? Top(lpw) :
					OffsetOf(lpw, TopIndex(lpw) + ViewHeight(lpw) - 1);

		if (PRIMPTR(lpw)->has_focus)
			MakeFocused(FocusItem(lpw));
	  }
	  UpdateSBar(lpw);		/* item moved into view: adjust scrollbar! */
	}
    if (XtIsSensitive(new) != XtIsSensitive(current))
	    redisplay = True;

    /* update_view must always be considered */
    return(redisplay && PANEPTR(new)->update_view);
}

/******************************function*header****************************
 * TraversalHandler-
 */
static Widget
TraversalHandler (Widget mgr, Widget w, OlVirtualName direction, Time time)
{
    if (EmptyList(w))
	return(NULL);

    switch(direction)
    {
    case OL_MOVEUP :
    case OL_MOVEDOWN :
    case OL_MULTIUP :
    case OL_MULTIDOWN :
	Activate(w,
	    (direction == OL_MULTIUP) ? OL_PAGEUP :
	    (direction == OL_MULTIDOWN) ? OL_PAGEDOWN : direction,
	    NULL);
	break;

    default :
	break;
    }
    return(w);
}


/* ----------------------------------------------------------------------
 * RESIZE proc for ListPane widget. 
 *
 * The resize policy is to basically show/hide items from the bottom
 * of the view. If there are no further items below the view, then we
 * scroll down, and include items above the current top-item. Thus we try
 * to keep the view full always ... 
 *
 *
 * Things to be done: 
 *	When ViewHeight == 0, FocusItem should be set to  NULL_OFFSET.
 * In which case ,we'll have to reset the FocusItem, when the  height 
 * increases .. later !! (no one's gonna notice this !)  
 *	Resize the editable TextField widget ,if we have a horizontal
 * resize .. probably I'll need to check which of 
 * width/height/both have changed.
 *	-JMK
 * ---------------------------------------------------------------------
 */
static void
Resize(Widget w)
{
	int scroll;

	/* Update ViewHeight to an integral no: of items .. */
	ViewHeight(w) = (Dimension)
			(w->core.height - 2 * PANEPTR(w)->vert_margin)
				 / (Dimension)ItemHeight(w);

	if (EmptyList(w) || (ViewHeight(w) == 0))
		return;

	if (ViewHeight(w) > NumItems(w))
		Top(w) = HeadOffset(w);
	else
	if ((scroll = AfterBottom(w)) < 0)
	/* No more items that can be shown at the bottom .Lets bring in
	 * some from above ...basically,just update Top(w)
	*/
		Top(w) = OffsetNode(w,Top(w),scroll);

	/* Adjust the Focus item - it should be visible always .. */
    	if ((FocusItem(w) != NULL_OFFSET) && !InView(w, FocusItem(w))) {
	/* turn off current focus_item, adjust it & make it focus'ed */
		if (PRIMPTR(w)->has_focus)
		    MakeUnfocused(FocusItem(w));
	/* set it to Top or Bottom */
		FocusItem(w) = AboveView(w, FocusItem(w)) ? Top(w) :
			OffsetOf(w, TopIndex(w) + ViewHeight(w) - 1);
		if (PRIMPTR(w)->has_focus)
		    MakeFocused(FocusItem(w));
    	}

	UpdateSBar((ListPaneWidget)w);
}

/*
 *************************************************************************
 * ListPaneQuerySCLocnProc: handles the positioning of the supercaret
 * over the current focus item.
 ****************************procedure*header*****************************
 */
static void
ListPaneQuerySCLocnProc(const   Widget          w,
                        const   Widget          supercaret,
                        const   Dimension       width,
                        const   Dimension       height,
                        Cardinal        *const  scale,
                        SuperCaretShape *const  shape,
                        Position        *const  x_center,
                        Position        *const  y_center)
{
	ListPaneWidget              lpw  = (ListPaneWidget)w;
        ListPanePart      *const    lpp  = PANEPTR(lpw);
	SuperCaretShape		    rs   = *shape;
	int node = lpp->focus_item;
	Dimension item_height = (Dimension)ItemHeight(w);

        if (node == NULL_OFFSET || !InView(w,node)) {
                *shape = SuperCaretNone;
                return;
        } else
		*shape = SuperCaretLeft;
 
        if (lpw->primitive.scale != *scale || rs != *shape) {
                *scale = lpw->primitive.scale;
                return; /* try again */
        }
 
        *x_center = lpp->horiz_margin;
        *y_center = lpp->vert_margin + 
			(Position)(PaneIndex(w, node)*item_height) + 
			(Position)(item_height/2);
 
}



/*************************************************************************
 *
 * Action Procedures
 *
 */

/*****************************function*header****************************
   Activate- call to activate widget "remotely"
*/
static Boolean
Activate (Widget w, OlVirtualName name, XtPointer data)
{

    Boolean	consumed;
    int		delta;
    Boolean	update_super_caret = TRUE;

    if (!XtIsSensitive(w) || EmptyList(w))
	return (True);

    switch(name)
    {
    case OL_CUT :
    case OL_COPY :
	consumed = True;

	if ((PANEPTR(w)->slist_mode != OL_NONE) && 
		!(NodePtr(FocusItem(w))->item_sensitive))
	   break;

	CutOrCopy(PANEW(w), (name == OL_CUT));
	break;

    case OL_PASTE :
	consumed = True;

	_OlBeepDisplay(w, 1);		/* can't paste into list */
	break;

    case OL_ADJUST :
    case OL_ADJUSTKEY :
    case OL_SELECT :
    case OL_SELECTKEY :
	consumed = True;

	/* FocusItem is not NULL since we have focus
	   Fire FocusItem or item passed as 'data' (mnemonic)
	*/
	if ((PANEPTR(w)->slist_mode != OL_NONE) && 
		!(NodePtr(FocusItem(w))->item_sensitive))
	   break;

	SelectOrAdjustItem(PANEW(w),
	    (data == NULL) ? FocusItem(w) : (int)data,
	    (name == OL_SELECTKEY) ? OL_SELECT :
	    (name == OL_ADJUSTKEY) ? OL_ADJUST : name,
	    LatestTime(w), (XEvent *) NULL, 0);
	update_super_caret = FALSE; 
	break;


    /* these involve scrolling the view (and potentially the focus_item): */
    case OL_SCROLLUP :
    case OL_SCROLLTOP :
    case OL_PAGEUP :
	consumed = True;

	if (Top(w) == HeadOffset(w))		/* can't back up any farther */
	    break;

	if (name == OL_SCROLLUP)
	{
	    Top(w) = Prev(w, Top(w));		/* update top_item */

	} else if (name == OL_SCROLLTOP) {
	    Top(w) = HeadOffset(w);		/* update top_item */

	} else {
	    delta = _OlMin(_OlMax(ViewHeight(w) - 1, 1), TopIndex(w));
	    Top(w) = OffsetNode(w, Top(w), -delta); /* update top_item */
	}

	ScrollView(PANEW(w));			/* update view */
	UpdateSBar(PANEW(w));			/* update scrollbar */
	break;


    case OL_SCROLLDOWN :
    case OL_SCROLLBOTTOM :
    case OL_PAGEDOWN :
	consumed = True;
	delta = AfterBottom(w);

	if (delta <= 0)			/* can't scroll forward any farther */
	    break;

	if (name == OL_SCROLLDOWN)
	{
	    Top(w) = Next(w, Top(w));			/* update top_item */
	
	} else if (name == OL_SCROLLBOTTOM) {
	    Top(w) = OffsetOf(w, NumItems(w) - ViewHeight(w)); /* update top */

	} else {
	    int		d;

	    d = _OlMin(_OlMax(ViewHeight(w) - 1, 1), delta);
	    Top(w) = OffsetNode(w, Top(w), d);
	}

	ScrollView(PANEW(w));			/* update view */
	UpdateSBar(PANEW(w));			/* update scrollbar */
	break;


    /* these involve moving the focus_item (and potentially the view): */
    case OL_MOVEUP :
    case OL_MOVEDOWN :
    case OL_PANESTART :
    case OL_PANEEND :
	consumed = True;

	/* disregard extremes */

	if (((name == OL_MOVEUP) && (FocusItem(w) == HeadOffset(w))) ||
	  ((name == OL_MOVEDOWN) &&
				(IndexOf(FocusItem(w)) + 1 == NumItems(w))) ||
	  ((name == OL_PANESTART) && (FocusItem(w) == Top(w))))
	{
	    break;

	} else if (name == OL_PANEEND) {
	    delta = _OlMin(TopIndex(w) + ViewHeight(w) - 1, NumItems(w) - 1);

	    if (FocusItem(w) == OffsetOf(w, delta))
		break;				/* can't go any farther down */
	}

	/* From here on it's all the same:
	   Unfocus the current focus_item and redraw it.
	   Establish new focus_item and redraw it or view it
	*/
	if ( OlSlistIsValidItem( SLISTW(w), FocusItem(w) ) )
	{
	    MakeUnfocused(FocusItem(w));
	    DrawItem(PANEW(w), FocusItem(w), ITEMSENSITIVE(PANEW(w), 
		     FocusItem(w)));
	}

	FocusItem(w) = (name == OL_MOVEUP) ? Prev(w, FocusItem(w)) :
			(name == OL_MOVEDOWN) ? Next(w, FocusItem(w)) :
			(name == OL_PANESTART) ? Top(w) :
			OffsetOf(w, delta);	/* OL_PANEEND */
	MakeFocused(FocusItem(w));

	if (InView(w, FocusItem(w)))
	{
		DrawItem(PANEW(w), FocusItem(w), ITEMSENSITIVE(PANEW(w), FocusItem(w)));
	} else {
	    ApplViewItem(SLISTW(w), (OlListToken)FocusItem(w));
	}
	break;
	
    default :
	consumed = False;
	break;
    }

    if(update_super_caret == TRUE)
	UpdateSuperCaret((ListPaneWidget)w);
    return (consumed);
}

/*****************************function*header****************************
 * AutoScrollCB-
 */
static void
AutoScrollCB(XtPointer closure, XtIntervalId *id)
{
    ListPaneWidget	w = PANEW(closure);
    XtAppContext	ac = XtWidgetToApplicationContext(closure);

    if (!PANEPTR(w)->scroll)		/* no need to scroll */
    {
	PANEPTR(w)->timer_id = NULL;	/* indicates scrolling has stopped */
	return;
    }

    if (PANEPTR(w)->timer_id == NULL)	/* 1st time thru? */
    {
	Arg arg;
	int initial_delay;

	XtSetArg(arg, XtNinitialDelay, &initial_delay);
	XtGetValues(SBAR(w), &arg, 1);			/* set initial delay */

	PANEPTR(w)->timer_id = XtAppAddTimeOut(ac, initial_delay,
					       AutoScrollCB, w);

    } else {
	if (PANEPTR(w)->scroll == SCROLL_UP)
	{
	    if (Top(w) != HeadOffset(w))
	    {
		ToggleItemState(PANEW(w), Prev(w, Top(w)));	/* SELECT it */
		Activate((Widget)w, OL_SCROLLUP, NULL);		/* scroll up */
		PANEPTR(w)->prev_index--;
	    }

	} else if (AfterBottom(w) > 0) {
	    ToggleItemState(PANEW(w), OffsetOf(w, TopIndex(w) + ViewHeight(w)));
	    Activate((Widget)w, OL_SCROLLDOWN, NULL);	/* scroll down */
	    PANEPTR(w)->prev_index++;
	}

	if (PANEPTR(w)->repeat_rate == 0)
	{
	    Arg arg;
	    XtSetArg(arg, XtNrepeatRate, &(PANEPTR(w)->repeat_rate));
	    XtGetValues(SBAR(w), &arg, 1);
	}
	PANEPTR(w)->timer_id = XtAppAddTimeOut(ac, PANEPTR(w)->repeat_rate,
							AutoScrollCB, w);
    }
}

/*****************************function*header****************************
   ButtonDown- handle buttonPress event.
	For MENU, dispatch to List.
	For SELECT or ADJUST see if event occurred over valid
	item and dispatch to SelectOrAdjustItem.
*/
static void
ButtonDown (Widget widget, OlVirtualEvent ve)
{
ListPaneWidget	w = PANEW(widget);
XButtonEvent *	buttonEvent;
int			indx;
static int prev_sel_node = 0;
Cardinal    mouse_damping_factor, multi_click_timeout;
static Boolean double_click = False;
#define IsSELECT(ve) ( (ve->virtual_name) == OL_SELECT )


    if (!XtIsSensitive(widget))
	return;

    switch(ve->virtual_name) {
    case OL_MENU :
		ve->consumed = True;

		OlAction(SLISTW(w), ve->xevent, NULL, NULL);
		break;
    case OL_SELECT :
    case OL_ADJUST :
		ve->consumed = True;

		/* ignore presses when empty list */
		if (EmptyList(w))
			break;

		buttonEvent = (XButtonEvent *) ve->xevent;

		/* ignore press in top or bottom margins */
		if ((buttonEvent->y < (Position)PANEPTR(w)->vert_margin) ||
		  (buttonEvent->y >=
				(Position)(w->core.height - PANEPTR(w)->vert_margin)))
		{
			break;
		}

		/* compute index based on y offset in pane and index of top_item */
		indx = IndexFromY(w, buttonEvent->y) + TopIndex(w);

		/* Call SelectOrAdjustItem only if index in pane is valid; that
		   is, list may not extend down as far as where button was pressed
		   (ie. list does not fill pane and pointer is in whitespace below
		   last item).
		*/
		if (indx < NumItems(w)) {
		int node;

			DPRINT((stderr, "index = %d \n", indx));

			/* For SELECT, take focus if don't already have it. */
			if ((ve->virtual_name == OL_SELECT) && !PRIMPTR(w)->has_focus)
			SetInputFocus((Widget)w, buttonEvent->time);

			/* track values in case of motion */
			PANEPTR(w)->start_index = indx;	/* must track starting pos */
			PANEPTR(w)->prev_index = indx;	/* any motion started here */
			PANEPTR(w)->initial_motion = NO_MOTION;	/* no motion yet */

			node = OffsetOf(w, indx);

			if (PANEPTR(w)->slist_mode == OL_NONE) {
			   SelectOrAdjustItem(PANEW(w), node,
					ve->virtual_name, buttonEvent->time, ve->xevent, ve->xevent->type);
			   break;
			}

			if (PANEPTR(w)->slist_mode != OL_NONE && 
					   !NodePtr(node)->item_sensitive)
			   break;


		   mouse_damping_factor = _OlGetMouseDampingFactor((Widget)w);
		   multi_click_timeout  = _OlGetMultiClickTimeout((Widget)w);
		 
		   if (IsSELECT(ve) && (buttonEvent->time - PANEPTR(w)->last_click_time <=
			   multi_click_timeout) &&
			   (PANEPTR(w)->last_root == buttonEvent->root) &&
			/* These checks for the mouse damping factor may
			 * not be necessary (the above code checks the
			 * motionnotify event) but just in case....
			 */
			   (ABS_DELTA(buttonEvent->x_root, PANEPTR(w)->last_x_root) <
					mouse_damping_factor) &&
			   (ABS_DELTA(buttonEvent->y_root, PANEPTR(w)->last_y_root) <
					mouse_damping_factor))
				  if (node == prev_sel_node) {
			      	double_click = True;
				  }

			if (double_click) {
			    OlSlistCallbackStruct cbdata;

				cbdata.cur_items = (OlSlistItemPtr *) XtMalloc(sizeof(int));
			    cbdata.reason = OL_REASON_DOUBLE_CLICK; 
			    cbdata.mode = PANEPTR(w)->slist_mode;
			    cbdata.event = ve->xevent;
			    cbdata.item = (OlSlistItemPtr)node;
			    cbdata.item_pos = IndexOf(node) + 1;
			    *(cbdata.cur_items) = (OlSlistItemPtr)node;
			    cbdata.num_cur_items = 1;
			    cbdata.cur_items_pos = (int *) NULL;
			    MultiClickCallback(w, &cbdata);
				double_click = False;
				XtFree((char *)cbdata.cur_items);
			}
			else {
				SelectOrAdjustItem(PANEW(w), node,
					ve->virtual_name, buttonEvent->time, ve->xevent, ve->xevent->type);
			}
		    prev_sel_node = node;
	    }
		break;

    default :
		break;
    }
}

/*****************************function*header****************************
   ButtonMotion- for wipe-thru selections & auto-scrolling

   dismiss OL_SELECT or OL_ADJUST outright if:
     *  not selectable OR
     *  motion w/o down-press (enter with button down) OR
     *  empty list
 */
static void
ButtonMotion (Widget widget, OlVirtualEvent ve)
{
#define Up		( delta < 0 )
#define Down		( delta > 0 )
#define WentUp(w)	( PANEPTR(w)->initial_motion < 0 )
#define WentDown(w)	( PANEPTR(w)->initial_motion > 0 )
#define ScrollUp(w)	( PANEPTR(w)->scroll == SCROLL_UP )
#define ScrollDown(w)	( PANEPTR(w)->scroll == SCROLL_DOWN )

XPointerMovedEvent * motionEvent;
ListPaneWidget	w = PANEW(widget);
int	indx;
int	delta;
int node;
Boolean update_prev_index = True;
Boolean cross_start_index = False;

    if (!XtIsSensitive(widget))
	return;

    switch(ve->virtual_name)
    {
    case OL_SELECT :
    case OL_ADJUST :
	ve->consumed = True;

	if (EmptyList(w) || (PANEPTR(w)->start_index == INITIAL_INDEX))
	    break;

	motionEvent = (XPointerMovedEvent *) ve->xevent;

	/* If pointer is above or below pane, consider auto-scrolling.
	   If scrolling (in same direction) is in progress, return.
	   Otherwise, compute indx of item under the pointer and continue
	   (selection will be wiped-thru and auto-scroll kicked off).
	*/
	if (motionEvent->y < (Position)PANEPTR(w)->vert_margin)
	{
	    if ((PANEPTR(w)->timer_id != NULL) &&
	      (PANEPTR(w)->scroll == SCROLL_UP))
	    {
		break;

	    } else {
		indx = TopIndex(w);
		PANEPTR(w)->scroll = (indx > 0) ?
		    SCROLL_UP :			/* scroll in items from top */
		    0;				/* no scrolling */
	    }

	} else if (motionEvent->y >=
	  (Position)(w->core.height - PANEPTR(w)->vert_margin)) {
	    if ((PANEPTR(w)->timer_id != NULL) &&
	        (PANEPTR(w)->scroll == SCROLL_DOWN)) {
			break;
	    } else {
			indx = TopIndex(w) + ViewHeight(w) - 1;

			/* If list does not fill view, pointer may be in whitespace
			   below end of list.  If so, adjust index to last item
			*/
			if (indx >= NumItems(w)) {
				indx = NumItems(w) - 1;
				PANEPTR(w)->scroll = 0;	/* no scrolling */
			} else {
				PANEPTR(w)->scroll = SCROLL_DOWN;
			}
		}
	} else {				/* y is in pane */
	    PANEPTR(w)->scroll = 0;		/* no scrolling */

	    /* compute index based on y offset in pane and index of top_item */
	    indx = IndexFromY(w, motionEvent->y) + TopIndex(w);

	    /* If list does not fill view, pointer may be in whitespace
		below end of list.  If so, adjust index to last item
	    */
		if (indx >= NumItems(w))
		    indx = NumItems(w) - 1;
	}

	/* compute # of items between previous and current indices */
	delta = indx - PANEPTR(w)->prev_index;

	/* disregard "jitter": motion within item */
	if (delta != 0)
	{
	    int i, cnt;

	    DPRINT((stderr, "index = %d \n", indx));

	    /* set initial motion if not already set */
	    if (PANEPTR(w)->initial_motion == 0)
		PANEPTR(w)->initial_motion = delta;

	    /* Now toggle ([un]highlight) items.  The prev_index
		must be adjusted (not included) if:
		    we're above start_index & moving up -OR-
		    we're below start_index & moving down
	    */

	    if ((indx < PANEPTR(w)->start_index) && Up)
		   i = PANEPTR(w)->prev_index - 1;

		else if ((PANEPTR(w)->slist_mode != OL_NONE) && 
				 (indx < PANEPTR(w)->start_index) && Down)
		   i = PANEPTR(w)->prev_index + 1;

		else if ((PANEPTR(w)->slist_mode != OL_NONE) && 
				 (indx == PANEPTR(w)->start_index) && Up) {
		   i = PANEPTR(w)->prev_index - 1;
		   update_prev_index = False;
		}

	    else if ((indx > PANEPTR(w)->start_index) && Down)
		   i = PANEPTR(w)->prev_index + 1;
	
		else if ((PANEPTR(w)->slist_mode != OL_NONE) && 
				 (indx == PANEPTR(w)->start_index) && Down) {
		   i = PANEPTR(w)->prev_index + 1;
		   update_prev_index = False;
		}

		else if ((PANEPTR(w)->slist_mode != OL_NONE) && 
				 (indx > PANEPTR(w)->start_index) && Up)
		   i = PANEPTR(w)->prev_index - 1;

	    else
		   i = PANEPTR(w)->prev_index;

	    for (cnt = (delta < 0) ? -delta : delta; cnt > 0; cnt--) {
		/* if direction changed and crossing start_index, toggle it */
			if (((i == PANEPTR(w)->start_index + 1) && WentUp(w) && Down) ||
				((i == PANEPTR(w)->start_index - 1) && WentDown(w) && Up)) {

				node = OffsetOf(w, PANEPTR(w)->start_index);

				if (PANEPTR(w)->slist_mode == OL_NONE) {
				   if (PANEPTR(w)->selectable) 
					 ToggleItemState(w, node);
				}
				else {
				   if (NodePtr(node)->item_sensitive) {
					  if (PANEPTR(w)->selectable)
						ToggleItemState(w, node);
					  SelectOrAdjustItem(PANEW(w), node, ve->virtual_name, 
								   motionEvent->time, ve->xevent, ve->xevent->type);
				   }
				}
			}

			node = OffsetOf(w, i);
			if (PANEPTR(w)->slist_mode == OL_NONE) {
			   if (PANEPTR(w)->selectable) 
				 ToggleItemState(w, node);
			}
			else {
			   if (NodePtr(node)->item_sensitive) {
				  if (PANEPTR(w)->selectable)
					ToggleItemState(w, node);
				  SelectOrAdjustItem(PANEW(w), node, ve->virtual_name, 
							   motionEvent->time, ve->xevent, ve->xevent->type);
			   }
			}
			i = Up ? i - 1 : i + 1;
	    }

	    if ((indx == PANEPTR(w)->start_index) &&
		    ((WentUp(w) && Up) || (WentDown(w) && Down))) {
			node = OffsetOf(w, PANEPTR(w)->start_index);
			if (PANEPTR(w)->slist_mode == OL_NONE) {
			   if (PANEPTR(w)->selectable) 
				 ToggleItemState(w, node);
			}
			else {
			   if (NodePtr(node)->item_sensitive) {
				  if (PANEPTR(w)->selectable)
					ToggleItemState(w, node);
				  SelectOrAdjustItem(PANEW(w), node, ve->virtual_name, 
							   motionEvent->time, ve->xevent, ve->xevent->type);
			   }
			}
	    }
	    
		if (update_prev_index)
	       PANEPTR(w)->prev_index = indx;	/* update previous index */
		else  /* We're goin' over from where we started so reset */
	       PANEPTR(w)->prev_index = PANEPTR(w)->start_index;
	}

	/* finally, Toggle Top or Bottom item if the start_index has
	   scrolled out of view, we've changed direction and
	    +	we're at the Top and scrolling up -OR-
	    +	at the bottom and scrolling down
	*/
	if (!InView(w, OffsetOf(w, PANEPTR(w)->start_index)) &&

	  /* we're at the Top and about to ScrollUp */
	  (((indx == TopIndex(w)) && WentDown(w) && ScrollUp(w)) ||

	  /* we're at the bottom and about to scroll down */
	  ((indx == _OlMin(TopIndex(w) + ViewHeight(w) - 1,
			NumItems(w) - 1)) && WentUp(w) && ScrollDown(w)))) {

		node = OffsetOf(w, indx);
		if (PANEPTR(w)->selectable) 
		   ToggleItemState(w, node);
	}

	/* if we need to scroll and we aren't already, then do so */
	if (PANEPTR(w)->selectable) 
	   if ((PANEPTR(w)->scroll != 0) && (PANEPTR(w)->timer_id == NULL))
	       AutoScrollCB((XtPointer)w, (XtIntervalId *)NULL);

	break;
    
    default :
	break;
    }

#undef Up
#undef Down
#undef WentUp
#undef WentDown
#undef ScrollUp
#undef ScrollDown
}

/*****************************function*header****************************
 * ButtonUp-
 */
static void
ButtonUp (Widget widget, OlVirtualEvent ve)
{
ListPaneWidget	w = PANEW(widget);
XButtonEvent *	buttonEvent;
#define IsSELECT(ve) ( (ve->virtual_name) == OL_SELECT )

	/* Save:
	 * Button Release, position (x,y), and root window for double-click.
	 */
	if (IsSELECT(ve)) {
		buttonEvent = (XButtonEvent *) ve->xevent;
		PANEPTR(w)->last_click_time = buttonEvent->time;
		PANEPTR(w)->last_x_root = buttonEvent->x_root;
		PANEPTR(w)->last_y_root = buttonEvent->y_root;
		PANEPTR(w)->last_root = buttonEvent->root;
	}

    if (!XtIsSensitive(widget) || EmptyList(w) || !PANEPTR(w)->selectable)
	return;

    switch(ve->virtual_name)
    {
    case OL_SELECT :
    case OL_ADJUST :
	PANEPTR(w)->start_index		= INITIAL_INDEX;
	PANEPTR(w)->initial_motion	= NO_MOTION;

	/* if auto-scrolling is going on, stop it */
	PANEPTR(w)->scroll		= 0;

	/* initial state: for 1st time thru auto-scroll */
	PANEPTR(w)->timer_id		= NULL;

	break;

    default :
	break;
    }
}

/*****************************function*header****************************
 * ConvertSelection-
 */
static Boolean
ConvertSelection(Widget w, Atom *selection, Atom *target, Atom *type_return, XtPointer *value_return, long unsigned int *length_return, int *format_return)
{
    Boolean		status;

    if (*selection == XA_PRIMARY)
    {
	if ((*target == _OL_COPY) || (*target  == _OL_CUT)) {

	    CutOrCopy(PANEW(w), (*target == _OL_CUT));

	    *format_return	= 0;
	    *length_return	= NULL;
	    *value_return	= NULL;
	    *type_return	= *target;
	    status		= False;


	} else if (*target == XA_STRING) {
	    *format_return	= 8;
	    *length_return	= ClipboardCnt;
	    status		= (ClipboardCnt > 0);
	    *value_return	= (status) ? Clipboard : NULL;
	    *type_return	= XA_STRING;

	}
	else if (*target == OlInternAtom(XtDisplay(w), LENGTH_NAME)) {
     	    int *intbuffer;
	    intbuffer = (int *) XtMalloc(sizeof(int));
		/* Why don't I just use ClipboardCnt below ?! */
	    *intbuffer = (int) (strlen (Clipboard));
	    *value_return = (XtPointer)intbuffer;
	    *length_return = 1;
	    *format_return = sizeof(int) * 8;
	    *type_return = (Atom) *target;
	    status =  True;
	}
	else {
	    char *atom;
	    static char prefix[] = "_SUN_SELN";

	    atom = XGetAtomName(XtDisplay(w), *target);
	    if (strncmp(prefix,atom,strlen(prefix)) != 0) 
	        OlWarning(dgettext(OlMsgsDomain,
	    			"ScrollingList: Can't convert PRIMARY"));
	    status = False;
	    XFree(atom);
	}

    } else if (*selection == XA_CLIPBOARD(XtDisplay(w))) {
	if (*target == XA_STRING) {
	    *format_return	= 8;
	    *length_return	= ClipboardCnt;
	    status		= (ClipboardCnt > 0);
	    *value_return	= (status) ? Clipboard : NULL;
	    *type_return	= XA_STRING;
	} 
	else if (*target == OlInternAtom (XtDisplay(w), LENGTH_NAME)) {
     	    int *intbuffer;
	    intbuffer = (int *) XtMalloc(sizeof(int));
		/* Why don't I just use ClipboardCnt below ?! */
	    *intbuffer = (int) (strlen (Clipboard));
	    *value_return = (XtPointer)intbuffer;
	    *length_return = 1;
	    *format_return = sizeof(int) * 8;
	    *type_return = (Atom) *target;
	    status =  True;
	}
	else {
	    char *atom;
	    static char prefix[] = "_SUN_SELN";

	    atom = XGetAtomName(XtDisplay(w), *target);
	    if (strncmp(prefix,atom,strlen(prefix)) != 0) 
	        OlWarning(dgettext(OlMsgsDomain,
	    			"ScrollingList: Can't convert CLIPBOARD"));
	    status = False;
	    XFree(atom);
	}

    } else {
	status			= False;
	OlWarning(dgettext(OlMsgsDomain,
		"ScrollingList: Unknown selection target ignored"));
    }
    return(status);
}

/*****************************function*header****************************
   KeyDown- pass MENUKEY to ListWidget in case appl looking for it.  For
	UNKNOWN_KEY, search list for it if it's printable.
*/
static void
KeyDown (Widget w, OlVirtualEvent ve)
{
    if (!XtIsSensitive(w))
	return;

    switch(ve->virtual_name)
    {
    case OL_MENUKEY :
	ve->consumed = True;

	OlAction(SLISTW(w), ve->xevent, NULL, NULL);
	break;

    case OL_UNKNOWN_KEY_INPUT :
#ifdef DEBUG
	{
#include <ctype.h>
#include <X11/keysym.h>
	    String keyName;

	    if (IsCursorKey(ve->keysym))
		keyName = "Cursor";

	    else if (IsFunctionKey(ve->keysym))
		keyName = "Function";
	
	    else if (IsKeypadKey(ve->keysym))
		keyName = "Keypad";

	    else if (IsMiscFunctionKey(ve->keysym))
		keyName = "MiscFunction";

	    else if (IsModifierKey(ve->keysym))
		keyName = "Modifier";

	    else if (IsPFKey(ve->keysym))
		keyName = "PF";

	    else
		keyName = "Key";

	    DPRINT((stderr, "%s: ", keyName));

	    if (ve->length)
	    {
		if (isprint(ve->buffer[0]))
		{
		    DPRINT((stderr, "'%s'\n", ve->buffer));

		} else {
		    DPRINT((stderr, "%#x\n", ve->buffer[0]));
		}

	    } else {
		DPRINT((stderr, "buffer length is 0\n"));
	    }
	}
#endif /*DEBUG */

	/* interested in un-modified keys (Modifier press results in
	   length == 0).  Check for mnemonics & accelerators, too.
	*/
	if (ve->length != 0 && !(((XKeyEvent *)(ve->xevent))->state &
	  (Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask)) &&
	  (_OlFetchMnemonicOwner(w, NULL, ve) == NULL) &&
	  (_OlFetchAcceleratorOwner(w, NULL, ve) == NULL))
	{
	    ve->consumed = True;

	    Search(PANEW(w), ve->buffer[0]);

	}

	break;

    default:
	break;
    }
}

/****************************procedure*header*****************************
 *  LoseSelection - 
 */
static void
LoseSelection(Widget w, Atom *selection)
{
    if (*selection == XA_CLIPBOARD(XtDisplay(w)))
    {
	PANEPTR(w)->own_clipboard = False;

    } else if (*selection == XA_PRIMARY) {
	ClearSelection(PANEW(w));
	PANEPTR(w)->own_selection = False;
    }
}

/*****************************function*header****************************
    SelectionDone- this is registered for Clipboard ownership.  When a
		SelectionDoneProc is registered, the selection owner
	owns the memory allocated for the selection.  Otherwise, new memory
	must be allocated which the Intrinsics frees on owners behalf.
	This is undesirable so SelectionDone is registered for the sole
	purpose of being able to own the clipboard memory (Clipboard).
*/
static void
SelectionDone(Widget w, Atom *selection, Atom *target)
{
}

/*****************************function*header****************************
 * SBarMovedCB-
 */
static void
SBarMovedCB(Widget sbw, XtPointer closure, XtPointer callData)
          	    			/* scrollbar widget */
             	        
             	         
{
    OlScrollbarVerify * verifyData = (OlScrollbarVerify *)callData;
    ListPaneWidget	w;		/* pane widget */
    int			new_index;
	int prev_focus_node;

    if (verifyData->ok == False)
	return;

    w = (ListPaneWidget)closure;
	prev_focus_node = FocusItem(w);

    new_index = verifyData->new_location;

    DPRINT((stderr, "SBarMoved- old= %d, new= %d \n", TopIndex(w), new_index));

    if ((new_index == TopIndex(w)) ||			/* no change */
      (new_index < 0) || (new_index >= NumItems(w)))	/* bad values */
	return;

    Top(w) = OffsetOf(w, new_index);	/* update top_item */
    ScrollView(PANEW(w));		/* update view */
	UpdateSuperCaret(w);
}

/*************************************************************************
 *
 * Public Procedures
 *
 */

OlListItem *
OlListItemPointer (OlListToken token)
{
    int indx = (int)token;

    if ((indx < 1) || (indx >= TheList.num_slots))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"Scrolling List: OlListItemPointer- bad token value"));
	return (NULL);
    }

    return ( ItemPtr(indx) );
}

#ifdef DEBUG
#include <stdio.h>

void
_OlPrintList(slw)
    Widget slw;
{
    Cardinal	i;
    Widget	w;

    if (slw == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
		"_OlPrintList- NULL widget passed in"));
	return;
    }

    w = _OlListPane(slw);

    if (EmptyList(w))
    {
	OlWarning(dgettext(OlMsgsDomain,
		"_OlPrintList- Empty list"));
	return;
    }

    fprintf(stderr, "index\toffset\tlabel\n");

    for (i = 0; i < NumItems(w); i++)
	fprintf(stderr, "%d\t%d,\t%s\n", i, OffsetOf(w, i),
					ItemPtr(OffsetOf(w, i))->label);
    fprintf(stderr, "NumItems= %d, TopIndex= %d\n", NumItems(w), TopIndex(w));
}
#endif /* DEBUG */
