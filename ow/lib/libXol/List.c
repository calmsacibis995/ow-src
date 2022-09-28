#pragma ident	"@(#)List.c	302.13	97/03/26 lib/libXol SMI"	/* scrolllist:src/List.c 1.57	*/

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

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/ListPaneP.h>		/* fixed pane */
#include <Xol/OpenLookP.h>
#include <Xol/Scrollbar.h>		/* fixed scrollbar */
#include <Xol/ScrollingP.h>		/* my private header file */


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
						/* class procedures */
static Boolean	AcceptFocus(Widget w, Time *time);
static void	Destroy(Widget w);
static void	GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void ClassInitialize(void);
static Widget	RegisterFocus(Widget w);
static void	Realize(Widget w, Mask *value_mask, XSetWindowAttributes *attributes);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static Widget	TraversalHandler (Widget, Widget, OlVirtualName, Time);
						/* action procedures */
						/* public procedures */

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

#define LPART(w)	( &(((ListWidget)(w))->list) )

	/* Spacing for List layout.  Values are in points */
#define PANE_BW		1	/* in pixels */


/* these resources actually belong to the ListPane.  Used in */
/* Initialize, SetValues, GetValuesHook */

static MaskArg PaneArgs[] = {
    { XtNapplAddItem, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplDeleteItem, NULL, OL_SOURCE_PAIR },	/* GetValues only */
    { XtNapplEditClose, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplEditOpen, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplTouchItem, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNapplUpdateView, NULL, OL_SOURCE_PAIR },	/* GetValues only */
    { XtNapplViewItem, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNfont, NULL, OL_SOURCE_PAIR },			/* and for textField */
    { XtNfontColor, NULL, OL_SOURCE_PAIR },		/* and for textField */
    { XtNforeground, NULL, OL_SOURCE_PAIR },		/* and for textField */
    { XtNinputFocusColor, NULL, OL_SOURCE_PAIR },	/* and for ListPane */
    { XtNmaximumSize, NULL, OL_SOURCE_PAIR },		/* for textField */
    { XtNrecomputeWidth, NULL, OL_SOURCE_PAIR },
    { XtNselectable, NULL, OL_SOURCE_PAIR },
    { XtNstring, NULL, OL_SOURCE_PAIR },		/* for textField */
    { XtNtextField, NULL, OL_SOURCE_PAIR },		/* GetValues only */
    { XtNtraversalOn, NULL, OL_SOURCE_PAIR },
    { XtNverification, NULL, OL_SOURCE_PAIR },		/* for textField */
    { XtNviewHeight, NULL, OL_SOURCE_PAIR },
    { XtNheight, NULL, OL_SOURCE_PAIR },		/* Redirect height
							   to ListPane child */
    { XtNtextFormat, NULL, OL_SOURCE_PAIR },
    { XtNimPreeditStyle, NULL, OL_SOURCE_PAIR },	/* for textField */
/*
 * This is to accommodate 2 resources added to the ListPane
 * widget which will allow the programmer to specify a
 * preferred maximum width and preferred minimum width of
 * the ScrollingList.  If these equal 0 (default value)
 * then they are ignored and the ScrollingList behaves
 * as usual.
 */
    { XtNprefMaxWidth, NULL, OL_SOURCE_PAIR },          /* for ListPane  */
    { XtNprefMinWidth, NULL, OL_SOURCE_PAIR },          /* for ListPane  */
    { XtNspace, NULL,   OL_SOURCE_PAIR },
    { XtNalign, NULL,   OL_SOURCE_PAIR },
    { XtNitemHeight, NULL, OL_SOURCE_PAIR },
    { XtNposition, NULL, OL_SOURCE_PAIR },
    { XtNscrollingListMode, NULL, OL_SOURCE_PAIR },
    { XtNfirstViewableItem, NULL, OL_SOURCE_PAIR },
    { XtNlastViewableItem, NULL, OL_SOURCE_PAIR },
    { XtNnumItems, NULL, OL_SOURCE_PAIR },
    { XtNcurrentItems, NULL, OL_SOURCE_PAIR },
    { XtNnumCurrentItems, NULL, OL_SOURCE_PAIR },
    { XtNscrollingListItems, NULL, OL_SOURCE_PAIR },
    { XtNviewableItems, NULL, OL_SOURCE_PAIR },
    };


/* these resources actually belong to the Scrollbar.  Used in
 * Initialize, SetValues
 */

static MaskArg ScrollbarArgs[] = {
    { XtNforeground, NULL, OL_SOURCE_PAIR },
    };

#define BYTE_OFFSET	XtOffsetOf(ListRec, list.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_LIST_FG, NULL },
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_LIST_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/***********************widget*translations*actions***********************
 *
 * Translations and Actions
 *
 */
/* None */

/****************************widget*resources*****************************
 *
 * List Resources
 */

#define LOFFSET(member)	XtOffsetOf(ListRec, list.member)

static XtResource resources[] = {
    { XtNtextFormat, XtCTextFormat, XtROlStrRep, sizeof(OlStrRep),
	LOFFSET(text_format), XtRCallProc, (XtPointer)_OlGetDefaultTextFormat
    },
    { XtNuserDeleteItems, XtCCallback, XtRCallback, sizeof(XtPointer),
	LOFFSET(userDeleteItems), XtRCallback, (XtPointer)NULL
    },
    { XtNuserMakeCurrent, XtCCallback, XtRCallback, sizeof(XtPointer),
	LOFFSET(userMakeCurrent), XtRCallback, (XtPointer)NULL
    },
    { XtNmultiClickCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	LOFFSET(multiClickCallback), XtRCallback, (XtPointer)NULL
    },
    { XtNitemCurrentCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	LOFFSET(itemCurrentCallback), XtRCallback, (XtPointer)NULL
    },
    { XtNitemNotCurrentCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	LOFFSET(itemNotCurrentCallback), XtRCallback, (XtPointer)NULL
    },
    { XtNlistPane, XtCReadOnly, XtRWidget, sizeof(Widget),
	LOFFSET(list_pane), XtRImmediate, (XtPointer)NULL
    }
};

#undef LOFFSET

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

ListClassRec listClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &formClassRec,
    /* class_name		*/	"ScrollingList",
    /* widget_size		*/	sizeof(ListRec),
    /* class_initialize		*/	ClassInitialize,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	False,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	Realize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	True,
    /* compress_exposure	*/	True,
    /* compress_enterleave	*/	True,
    /* visible_interest		*/	False,
    /* destroy			*/	Destroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	NULL,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	GetValuesHook,
    /* accept_focus		*/	AcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query geometry		*/	NULL
  },
  { /* composite class		*/
    /* geometry_manager		*/	XtInheritGeometryManager,
    /* change_managed		*/	XtInheritChangeManaged,
    /* insert_child		*/	XtInheritInsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension		*/	NULL
  },
  { /* constraint class		*/
    /* resources		*/	NULL,
    /* num_resources		*/	0,
    /* constraint_size		*/	sizeof(ListConstraintRec),
    /* initialize		*/	NULL,
    /* destroy			*/      NULL,
    /* set_values		*/	NULL,
    /* extension		*/	NULL,
  },
  { /* manager_class fields	*/
    /* highlight_handler  	*/	NULL,
    /* reserved			*/	NULL,
    /* reserved			*/	NULL,
    /* traversal_handler	*/	TraversalHandler,
    /* activate			*/	NULL,
    /* event_procs		*/	NULL,
    /* num_event_proc		*/	0,
    /* register_focus		*/	RegisterFocus,
    /* reserved			*/	NULL,
    /* version			*/	OlVersion,
    /* extension		*/	NULL,
    /* dyn_data			*/	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc		*/	_OlDefaultTransparentProc,
    /* query_sc_locn_proc	*/	NULL,
  },
  { /* form class		*/
					0,
  },
  { /* list class		*/
					0,
  },

};

WidgetClass scrollingListWidgetClass	= (WidgetClass)&listClassRec;


/***************************private*procedures****************************
 *
 * Private Functions
 *
 */


/*************************************************************************
 *
 * Class Procedures
 *
 */

/*
 ************************************************************
 *
 *  ClassInitialize - Register OlDefine string values
 *
 *********************function*header************************
 */

static void ClassInitialize(void)
{
	_OlAddOlDefineType ("none", OL_NONE);
	_OlAddOlDefineType ("exclusive", OL_EXCLUSIVE);
	_OlAddOlDefineType ("exclusive_noneset", OL_EXCLUSIVE_NONESET);
	_OlAddOlDefineType ("nonexclusive", OL_NONEXCLUSIVE);
}


/******************************function*header****************************
 * AcceptFocus - pass along request to ListPane
 */
static Boolean
AcceptFocus(Widget w, Time *time)
	      	  		/* List Widget	*/
	    	     
{
	return( OlCanAcceptFocus(w, *time) && OlCallAcceptFocus(_OlListPane(w), *time) );
} /* END OF AcceptFocus() */

/******************************function*header****************************
 * Destroy():  free private storage.
 */

static void
Destroy(Widget w)
{
    XtRemoveAllCallbacks(w, XtNuserDeleteItems);
    XtRemoveAllCallbacks(w, XtNuserMakeCurrent);
}

/******************************function*header****************************
 * GetValuesHook(): get resource values aimed at List but belong to Pane
 */

static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
    ArgList	mergedArgs;
    Cardinal	mergedCnt;

    _OlComposeArgList(args, *num_args, PaneArgs, XtNumber(PaneArgs),
		      &mergedArgs, &mergedCnt);

    if (mergedCnt != 0) {
	XtGetValues(_OlListPane(w), mergedArgs, mergedCnt);
	XtFree((char *)mergedArgs);
    }
}

/******************************function*header****************************
 * Initialize()
 */

static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    Arg		zArgs[10];
    ArgList	mergedArgs;
    Cardinal	mergedCnt;
    ArgList     finalArgs;
    Cardinal	cnt;

    /***************************************************************
		create sub-widgets

	Order of child creation is important.  See macros _OlListSBar,
	_OlListTextF & PANE.
	List pane gets created below after scrollbar.
    */
    /* create scrollbar */
    _OlComposeArgList(args, *num_args, ScrollbarArgs,
	XtNumber(ScrollbarArgs), &mergedArgs, &mergedCnt);
    cnt = 0;
    XtSetArg(zArgs[cnt], XtNbackground, new->core.background_pixel); cnt++;
    finalArgs = XtMergeArgLists(mergedArgs,mergedCnt,zArgs,cnt);
    (void) XtCreateManagedWidget("scrollbar",
	scrollbarWidgetClass, new, finalArgs, mergedCnt+cnt);

    XtFree((char *)mergedArgs);
    XtFree((char *)finalArgs);

    /* Pane must be created after SBar so pane can add callbacks to SBar.  */
    _OlComposeArgList(args, *num_args, PaneArgs, XtNumber(PaneArgs),
					&mergedArgs, &mergedCnt);
    cnt = 0;
    XtSetArg(zArgs[cnt], XtNbackground, new->core.background_pixel); cnt++;
    XtSetArg(zArgs[cnt], XtNbackgroundPixmap,
			 new->core.background_pixmap); cnt++;
    XtSetArg(zArgs[cnt], XtNborderColor, new->core.border_pixel); cnt++;
    XtSetArg(zArgs[cnt], XtNborderPixmap, new->core.border_pixmap); cnt++;
    XtSetArg(zArgs[cnt], XtNborderWidth, new->core.border_width); cnt++;
    XtSetArg(zArgs[cnt], XtNtextFormat, ((ScrollingListWidget)(new))->list.text_format); cnt++;
    finalArgs = XtMergeArgLists(mergedArgs,mergedCnt,zArgs,cnt);

     ((ScrollingListWidget)(new))->list.list_pane = 
			XtCreateManagedWidget("pane",listPaneWidgetClass, 
					 new, finalArgs, mergedCnt+cnt);
    XtFree((char *)mergedArgs);
    XtFree((char *)finalArgs);

    _OlDeleteDescendant(_OlListPane(new));/* delete pane from traversal list */
					  /*  and add myself instead */
    _OlUpdateTraversalWidget(new, MGRPART(new)->reference_name,
			     MGRPART(new)->reference_widget, True);

/* Introduce new set of constraint resources for ListPane -JMK */
    cnt = 0;
    XtSetArg(zArgs[cnt], XtNyResizable, True); cnt++;
    XtSetArg(zArgs[cnt], XtNxResizable, True); cnt++;
    XtSetArg(zArgs[cnt], XtNyAttachBottom, True); cnt++;
    XtSetValues(_OlListPane(new), zArgs, cnt);

   /* now go back and make scrollbar position relative to pane */
    cnt = 0;
    XtSetArg(zArgs[cnt], XtNyAttachOffset,
		PANE_BW); cnt++;		/* WORKAROUND Form bug */
    XtSetArg(zArgs[cnt], XtNxRefWidget, _OlListPane(new)); cnt++;
    XtSetArg(zArgs[cnt], XtNxAddWidth, True); cnt++;
    XtSetArg(zArgs[cnt], XtNxOffset,
		OlScreenPointToPixel(OL_HORIZONTAL, 2, XtScreen(new))); cnt++;

/* Impose additional constraints for Scrolbar too -JMK */
    XtSetArg(zArgs[cnt], XtNyResizable, True); cnt++;
    XtSetArg(zArgs[cnt], XtNxResizable, False); cnt++;
    XtSetArg(zArgs[cnt], XtNxAttachRight, True); cnt++;
    XtSetArg(zArgs[cnt], XtNyAddHeight, False); cnt++;  /* default */
    XtSetArg(zArgs[cnt], XtNyAttachBottom, True); cnt++;
    XtSetArg(zArgs[cnt], XtNyOffset, PANE_BW); cnt++;


    XtSetValues(_OlListSBar(new), zArgs, cnt);

					/* associate pane with container */
    OlAssociateWidget(new, _OlListPane(new), False);
					/* associate sbar with pane */
    OlAssociateWidget(_OlListPane(new), _OlListSBar(new), True);

    new->core.background_pixmap = ParentRelative;
}

/******************************function*header****************************
 * RegisterFocus():  return widget to register on Shell
 */

static Widget
RegisterFocus(Widget w)
{
    return (w);
}

/******************************function*header****************************
 * Realize():
 */

static void
Realize(Widget w, Mask *value_mask, XSetWindowAttributes *attributes)
{
	XtRealizeProc super_realize;

	attributes->background_pixmap = ParentRelative;
	*value_mask |= CWBackPixmap;
	*value_mask &= ~CWBackPixel;

	super_realize = scrollingListWidgetClass->core_class.superclass->core_class.realize;

        if (super_realize != (XtRealizeProc)NULL) {
                (*super_realize)(w, value_mask, attributes);
	} else {
		OlError(dgettext(OlMsgsDomain,
			"Scrolling List's superclass has no Realize Proc."));
	}

} /* END of Realize() */

/******************************function*header****************************
 * SetValues():  pass on SetValues aimed at the List but destined
 * for a subwidget.
 */

static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    ArgList	mergedArgs;
    Cardinal	mergedCnt;
    Arg         Args[10];
    Cardinal    cnt;
    ArgList     finalArgs;

    /* Redirect XtNheight changes to ListPane. Let it decide whether
     * to honour it, based on the XtNviewHeight (== 0 ?) ... And finally
     * the ListPane will set its own height appropriately, causing a
     * resize request to us - which will set our own height too .
     * To prevent myself from resizing the ListPane, I reset my height
     * here -JMK
     */
    new->core.height = current->core.height;

#define CHANGED(f) 	(new->core.f != current->core.f)
    cnt = 0;
    if (CHANGED(background_pixel)) {
	XtSetArg(Args[cnt], XtNbackground, new->core.background_pixel); 
	cnt++;
    }
    if (CHANGED(background_pixmap)) {
	XtSetArg(Args[cnt], XtNbackgroundPixmap, new->core.background_pixmap); 
        cnt++;
    }
    if (CHANGED(border_pixel)) {
	XtSetArg(Args[cnt], XtNborderColor, new->core.border_pixel);
	cnt++;
    }
    if (CHANGED(border_pixmap)) {
	XtSetArg(Args[cnt], XtNborderPixmap, new->core.border_pixmap);
	cnt++;
    }
    if (CHANGED(border_width)) {
	XtSetArg(Args[cnt], XtNborderWidth, new->core.border_width); 
	cnt++;
    }
    _OlComposeArgList(args, *num_args, PaneArgs, XtNumber(PaneArgs),
		      &mergedArgs, &mergedCnt);
    finalArgs = XtMergeArgLists(mergedArgs,mergedCnt,Args,cnt);

    if ((mergedCnt+cnt) != 0) {
	XtSetValues(_OlListPane(new), finalArgs, mergedCnt+cnt);
	XtFree((char *)mergedArgs);
	XtFree((char *)finalArgs);
    }

    cnt = 0;
    if (CHANGED(background_pixel)) {
	 XtSetArg(Args[cnt], XtNbackground, new->core.background_pixel);
	 cnt++;
    }
    _OlComposeArgList(args, *num_args, ScrollbarArgs,
	XtNumber(ScrollbarArgs), &mergedArgs, &mergedCnt);
    finalArgs = XtMergeArgLists(mergedArgs,mergedCnt,Args,cnt);

    if ((mergedCnt+cnt) != 0) {
	XtSetValues(_OlListSBar(new), finalArgs, mergedCnt+cnt);
	XtFree((char *)mergedArgs);
	XtFree((char *)finalArgs);
    }

    new->core.border_width = 0;		/* always */
    new->core.background_pixmap = ParentRelative; /* always */
    ((ScrollingListWidget)(new))->list.text_format = 
		((ScrollingListWidget)(current))->list.text_format;

    return False;
#undef CHANGED
}

/******************************function*header****************************
   TraversalHandler- If 'w' is ScrollingList, defer to ListPane's
	TraversalHandler, else (w is Scrollbar), return NULL
*/
static Widget
TraversalHandler (Widget mgr, Widget w, OlVirtualName direction, Time time)
{
	/* macro makes things more palettable */
#define PrimClass(w) \
	    ( ((PrimitiveWidgetClass)w->core.widget_class)->primitive_class )

    return ( (w == mgr) ?
	(*PrimClass(_OlListPane(mgr)).traversal_handler)
		(_OlListPane(mgr), _OlListPane(mgr), direction, time) : NULL);

#undef PrimClass
}

/*************************************************************************
 *
 * Action Procedures
 *
 */

/*************************************************************************
 *
 * Public Procedures
 *
 */
