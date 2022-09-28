#pragma ident	"@(#)EventObj.c	302.12	93/05/10 lib/libXol SMI"	/* eventobj:EventObj.c 1.53	*/

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

/******************************file*header********************************
 *
 * Description:
 *	Source code for the OPEN LOOK (Tm - AT&T) EventObj (Meta) Class.
 *
 */


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/EventI.h>
#include <X11/TranslateI.h>

#include <Xol/Error.h>
#include <Xol/EventObjP.h>
#include <Xol/Manager.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgxP.h>
#include <Xol/Menu.h>


/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 */

#ifndef	NonMaskableMask
#define	NonMaskableMask	((EventMask)0x80000000L)
#endif

typedef enum {
	eRemoveGrab, eAddGrab, eQueryGrabs, eQuerySiblingGrabs
} GrabCode;
typedef struct {
	Widget	object;
	Boolean	exclusive;
} GrabRec, *GrabList;

#define ON_GRAB_LIST(w)	HandleGrabs(eQueryGrabs, w, False, False)

#define CREATE		True
#define DONT_CREATE	False
#define OleTdestroy	"destroy"

#define HASH_MASK	0x7F				/* 2^n - 1 */
#define TABLE_SIZE	(HASH_MASK + 1)
#define HASH(entry)	((((int)(entry)) >> 3) & HASH_MASK)

typedef struct  _GadgetData {
    WidgetList  gadget_children;/* array of gadget children	*/
    Cardinal    num_gadgets;	/* total number of gadget children */
    Cardinal    num_slots;	/* number of slots in gadget array */
    int         last_used;	/* index into the gadget_children */
    struct _GadgetData * next;	/* next GadgetData in hash table */
} GadgetData;

typedef enum _GrabType {pass, ignore, remap} GrabType;
    
#define PointInRect(rect_x, rect_y, rect_width, rect_height, x, y) \
    ( \
     (int)(rect_x) <= (int)(x) && \
     (int)((rect_x) + (rect_width)) >= (int)(x) && \
     (int)(rect_y) <= (int)(y) && \
     (int)((rect_y) + (rect_height)) >= (int)(y) \
     ) 

#define EOP(w) (((EventObj)w)->event)

#define BYTE_OFFSET	XtOffsetOf(EventObjRec, event.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_EVENT_BG, NULL },
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_EVENT_FG, NULL },
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_EVENT_FONTCOLOR, NULL },
{ { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, "Red" }, BYTE_OFFSET, OL_B_EVENT_FOCUSCOLOR, NULL },
};
#undef BYTE_OFFSET

/**************************forward*declarations***************************
 *
 * Forward Procedure declarations listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 */
						/* private procedures */
static Boolean		AcceptFocus(Widget w, Time *time);
static Boolean		AllowGadgetGrabs (Widget);
static EventMask	BuildParentMask(Widget w, Boolean *nonmaskable);
static void		ConvertTypeToMask(int eventType, EventMask *mask, GrabType *grabType);
static void		DispatchEvent(Widget w, XEvent *event);
static void		DispatchGadgetEvent(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);
static void		FreeGadgetData(Widget w);
static GadgetData *	GetGadgetData(Widget w, Boolean create);
static GadgetData **	GetHashTblEntry(Widget w);
static void		GrabDestroyCallback (Widget,
					XtPointer, XtPointer);
static Boolean		HandleGrabs (GrabCode,Widget,Boolean,Boolean);
static Boolean		IntersectRect(Position x1, Position y1, Dimension rect_width, Dimension rect_height, Position x3, Position y3, int width, int height);
static int		WidgetToGadget(int x, int y, GadgetData *gadget_data);
static Cardinal		WidgetToGadgets(int x, int y, int width, int height, GadgetData *gadget_data, WidgetList *list_return);
						/* class procedures */
static void ClassInitialize(void);
static void ClassPartInitialize(WidgetClass wc);
static void Destroy(Widget w);
static void GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static Boolean SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static void 	EventObjQuerySCLocnProc(const Widget		target,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return);
						/* action procedures */


/***********************widget*translations*actions***********************
 *
 * Define Translations and Actions
 *
 */


/* There are no Translations or Action Tables for the eventObj widget */


/****************************widget*resources*****************************
 *
 * Define Resource list associated with the Widget Instance
 *
 */
#define OFFSET(field) XtOffsetOf(EventObjRec, event.field)

static XtResource resources[] =
{
	/* this should be the first resource */
  { XtNtextFormat, XtCTextFormat, XtROlStrRep,sizeof(OlStrRep),
    OFFSET(text_format),XtRCallProc,(XtPointer)_OlGetDefaultTextFormat
  },
  { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffset(EventObj, rectangle.border_width), XtRImmediate, (XtPointer)0
  },
  { XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
    OFFSET(accelerator), XtRString, (XtPointer) NULL
  },
  { XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
    OFFSET(accelerator_text), XtRString, (XtPointer) NULL
  },
  { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
    OFFSET(consume_event), XtRCallback, (XtPointer)NULL
  },
  { XtNfont, XtCFont, XtROlFont, sizeof(OlFont),
    OFFSET(font), XtRString, XtDefaultFont 
  },
  { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
    OFFSET(font_color), XtRString, XtDefaultForeground
  },
  { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(foreground), XtRString, XtDefaultForeground
  },
  { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
    OFFSET(input_focus_color), XtRString, "Red"
  },
  { XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
    OFFSET(mnemonic), XtRImmediate, (XtPointer) '\0'
  },
  { XtNreferenceName, XtCReferenceName, XtRString, sizeof(String),
    OFFSET(reference_name), XtRString, (XtPointer)NULL
  },
  { XtNreferenceWidget, XtCReferenceWidget, XtRWidget, sizeof(Widget),
    OFFSET(reference_widget), XtRWidget, (XtPointer)NULL
  },
  { XtNscale, XtCScale, XtROlScale, sizeof(int),
    OFFSET(scale), XtRImmediate, (XtPointer)OL_DEFAULT_POINT_SIZE
  },
  { XtNtraversalOn, XtCTraversalOn, XtRBoolean, sizeof(Boolean),
    OFFSET(traversal_on), XtRImmediate, (XtPointer)True
  },
  { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
    OFFSET(user_data), XtRPointer, (XtPointer)NULL
  }
};

/***************************widget*class*record***************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 */

EventObjClassRec eventObjClassRec = {
  {					/* core class */
      (WidgetClass) &rectObjClassRec,	/* superclass			*/
      "EventObj",			/* class_name			*/
      sizeof(EventObjRec),		/* widget_size			*/
      ClassInitialize,			/* class_initialize		*/
      ClassPartInitialize,		/* class_part_initialize	*/
      FALSE,				/* class_inited			*/
      Initialize,			/* initialize			*/
      NULL,				/* initialize_hook		*/
      NULL,				/* (unused) realize		*/
      NULL,				/* (unused) actions		*/
      NULL,				/* (unused) num_actions		*/
      resources,			/* resources			*/
      XtNumber(resources),		/* num_resources		*/
      NULLQUARK,			/* xrm_class			*/
      FALSE,				/* (unused) compress_motion	*/
      TRUE,				/* (unused) compress_exposure	*/
      FALSE,				/* (unused) compress_enterleave	*/
      FALSE,				/* (unused) visible_interest	*/
      Destroy,				/* destroy			*/
      NULL,				/* resize			*/
      NULL,				/* expose			*/
      SetValues,			/* set_values			*/
      NULL,				/* set_values_hook		*/
      NULL,				/* set_values_almost		*/
      GetValuesHook,			/* get_values_hook		*/
      (XtProc)AcceptFocus,		/* (unused by Xt!) accept_focus	*/
      XtVersion,			/* version			*/
      NULL,				/* callback_private		*/
      NULL,				/* (unused) tm_table		*/
      NULL,				/* query_geometry		*/
      NULL,				/* (unused) display your accelerator */
      NULL,				/* pointer to extension record	*/
  },
  {					/* eventObj class		*/
			      /* fields for Primitive Class equivalence */
      NULL,				/* reserved			*/
      NULL,				/* highlight_handler		*/
      NULL,				/* traversal_handler		*/
      NULL,				/* register_focus		*/
      NULL,				/* activate			*/
      NULL,				/* event_procs			*/
      0,				/* num_event_procs		*/
      OlVersion,			/* version			*/
      NULL,				/* extension			*/
      { dyn_res, XtNumber(dyn_res) },	/* dyn_data			*/
      NULL,				/* transparent_proc		*/
      EventObjQuerySCLocnProc,		/* query_sc_locn_proc		*/
  },
}; 

/*************************public*class*definition*************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 */

WidgetClass eventObjClass = (WidgetClass) &eventObjClassRec;
    
/***************************private*procedures****************************
 *
 * Private Procedures
 *
 */
    
/***************************function*header*******************************
  AcceptFocus- this function is pointed to by accept_focus field in Core.
	Xt does not make use of this field (since objects cannot take
	focus) but Xol makes use of it (OlCallAcceptFocus).  The assumption
	is that Xt will simply leave the field alone and not NULLify it,
	for instance.
 */

static Boolean
AcceptFocus(Widget w, Time *time)
{

    if (OlCanAcceptFocus(w, *time))
    {
     	OlSetInputFocus(w, RevertToNone, *time);
	return (True);
    }

    return (False);

} /* AcceptFocus() */

static EventMask
BuildParentMask(Widget w, Boolean *nonmaskable)
{
    GadgetData *	gd = GetGadgetData(XtParent(w), DONT_CREATE);
    Cardinal		num_gadgets;
    EventMask		mask;
    
    mask = (EventMask) 0L;
    *nonmaskable = FALSE;
    
    for (num_gadgets = 0; num_gadgets < gd->num_gadgets; num_gadgets++)
    {
	Widget widget = gd->gadget_children[num_gadgets];
	
	/* taken from XtBuildEventMask (Event.c).  XtBuildEventMask does
	 * not help to get 'nonmaskable', however.
	 */
	register XtEventTable ev;
	for (ev = widget->core.event_table; ev != NULL; ev = ev->next) {
	    if (ev->select) mask |= ev->mask;
	    *nonmaskable = *nonmaskable ||
			   (ev->mask & NonMaskableMask) == NonMaskableMask;
	}
	if (widget->core.widget_class->core_class.expose != NULL)
	    mask |= ExposureMask;
	if (widget->core.widget_class->core_class.visible_interest)
	    mask |= VisibilityChangeMask;
    }
    
    /* if any child wants Enter/Leaves, must have parent get Motion */
    if (mask & (EnterWindowMask | LeaveWindowMask))
	mask |= ButtonMotionMask;

    return(mask);
}

static void
ConvertTypeToMask (int eventType, EventMask *mask, GrabType *grabType)
{
    
    static const struct {
	EventMask   mask;
	GrabType    grabType;
    } masks[] = {
	{0,				pass},  /* shouldn't see 0 */
	{0,				pass},  /* shouldn't see 1 */
	{KeyPressMask,		        remap},	/* KeyPress	   */
	{KeyReleaseMask,		remap},	/* KeyRelease      */
	{ButtonPressMask,		remap},	/* ButtonPress     */
	{ButtonReleaseMask,		remap},	/* ButtonRelease   */
	{PointerMotionMask
	  | Button1MotionMask
	  | Button2MotionMask
	  | Button3MotionMask
	  | Button4MotionMask
	  | Button5MotionMask
	  | ButtonMotionMask,		ignore}, /* MotionNotify */
        {EnterWindowMask,		ignore}, /* EnterNotify	 */
	{LeaveWindowMask,		ignore}, /* LeaveNotify	 */
	{FocusChangeMask,		pass},   /* FocusIn	 */
	{FocusChangeMask,		pass},   /* FocusOut	 */
	{KeymapStateMask,		pass},   /* KeymapNotify */
	{ExposureMask,			pass},   /* Expose	 */
        {NonMaskableMask,            	pass}, /* GraphicsExpose, in GC    */
        {NonMaskableMask,            	pass}, /* NoExpose, in GC          */
	{VisibilityChangeMask,      	pass}, /* VisibilityNotify  */
	{SubstructureNotifyMask,    	pass}, /* CreateNotify	    */
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* DestroyNotify	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* UnmapNotify	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* MapNotify	*/
	{SubstructureRedirectMask,  	pass}, /* MapRequest	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* ReparentNotify  */
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* ConfigureNotify  */
	{SubstructureRedirectMask,  	pass}, /* ConfigureRequest */
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* GravityNotify	*/
	{ResizeRedirectMask,		pass}, /* ResizeRequest	*/
	{StructureNotifyMask
	     | SubstructureNotifyMask,  pass}, /* CirculateNotify  */
	{SubstructureRedirectMask,  	pass}, /* CirculateRequest */
	{PropertyChangeMask,		pass}, /* PropertyNotify   */
        {NonMaskableMask,            	pass}, /* SelectionClear   */
        {NonMaskableMask,            	pass}, /* SelectionRequest */
        {NonMaskableMask,            	pass}, /* SelectionNotify  */
	{ColormapChangeMask,	pass}, /* ColormapNotify   */
        {NonMaskableMask,            	pass}, /* ClientMessage */
        {NonMaskableMask,             	pass}, /* MappingNotify */
    };
    
    eventType &= 0x7f;	/* Events sent with XSendEvent have high bit set. */

    if (eventType >= (sizeof(masks) / sizeof(masks[0])))
	eventType = 0;

    (*mask)      = masks[eventType].mask;
    (*grabType)  = masks[eventType].grabType;
}

static void
DispatchEvent(Widget w, XEvent *event)
          	  			/* really the gadget */
            	      
{
    register XtEventRec * p;
    EventMask		mask;
    GrabType		grabType;
    int			numprocs, i;
    XtEventHandler	proc[100];
    XtPointer		closure[100];
    Boolean		continue_to_dispatch;
    
    /* Have to copy the procs into an array, because calling one of them */
    /* might call XtRemoveEventHandler, which would break our linked list.*/
    
    ConvertTypeToMask(event->type, &mask, &grabType);
    numprocs = 0;
    
    if (grabType != pass && ON_GRAB_LIST(w) == False)
	return;

    for (p=w->core.event_table; p != NULL; p = p->next) {
	if ((mask & p->mask) != 0) {
	    proc[numprocs] = p->proc;
	    closure[numprocs++] = p->closure;
	}
    }
    
    for (i = 0; i < numprocs; i++)  {
	(*(proc[i]))(w, closure[i], event,&continue_to_dispatch);
    }
}				/*  DispatchEvent  */

/* ARGSUSED */
static void
DispatchGadgetEvent(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
    GadgetData *	gadget_data = GetGadgetData(w, DONT_CREATE);
    int			last_index;
    WidgetList		child;
    Widget		gadget;
    int			i;
    int			current_index = 0;
    
    /* No gadget_data or no gadget children */
    if ((gadget_data == NULL) || (gadget_data->num_gadgets == 0))
	return;
    
    child      = gadget_data->gadget_children;
    last_index = gadget_data->last_used;

    /* This code does motion, enter/leave compression if specified. */
    /* For other types of events, it distributes them to one or more */
    /* gadget.  The switch must handle any type of X event that a */
    /* gadget can register an event handler for. */

    switch (event->type)  {
    case ButtonPress:
    case ButtonRelease:
    case KeyPress:
    case KeyRelease:
	/* get index of gadget over which this event occurred */
	current_index = WidgetToGadget(event->xkey.x,
				      event->xkey.y, gadget_data);
	if (current_index != -1) {
	    /* get gadget child */
	    gadget = child[current_index];

	    if (XtIsSensitive(gadget)) /* if sensitive, Dispatch event */
		DispatchEvent(gadget, event);

	    gadget_data->last_used = current_index;
	}
	break;
	
    case EnterNotify:
	/* look for a border gadget & generate Enter for it */
	current_index =
	    WidgetToGadget(event->xkey.x, event->xkey.y, gadget_data);

	if (current_index != last_index && current_index != -1) {
	    gadget = child[current_index];

	    /* Dispatch event only if gadget is Sensitive */
            if (XtIsSensitive(gadget) == True)
		DispatchEvent(gadget, event);

	    gadget_data->last_used = current_index;
	}
	break;
	
    case LeaveNotify:
	/* Generate Leave on gadget (if one) */
	if (last_index != -1) {
	    gadget = child[last_index];
	    DispatchEvent(gadget, event);
	    gadget_data->last_used = -1;
	}
	break;

    case MotionNotify:
	/* get gadget over which motion occurred */
	current_index =
	    WidgetToGadget(event->xkey.x, event->xkey.y, gadget_data);

	/* consider delivering Enter/Leaves */
	if (current_index != last_index) {
	    XCrossingEvent new_event;
	    
	    new_event.serial		= event->xmotion.serial;
	    new_event.send_event	= TRUE;
	    new_event.display		= event->xmotion.display;
	    new_event.window		= event->xmotion.window;
	    new_event.root		= event->xmotion.root;
	    new_event.subwindow		= event->xmotion.subwindow;
	    new_event.time		= event->xmotion.time;
	    new_event.x			= event->xmotion.x;
	    new_event.y			= event->xmotion.y;
	    new_event.x_root		= event->xmotion.x_root;
	    new_event.y_root		= event->xmotion.y_root;
	    new_event.mode		= NotifyNormal;
	    new_event.detail		= NotifyAncestor;
	    new_event.same_screen	= event->xmotion.same_screen;
	    new_event.focus		= FALSE;
	    new_event.state		= event->xmotion.state;
	    
	    /* generate Leave event on last gadget (if any) */
	    if (last_index != -1) {
		Widget last_w = child[last_index];
		new_event.type = LeaveNotify;
		DispatchEvent(last_w, (XEvent *)&new_event);
		gadget_data->last_used = -1;
	    }

	    /* now generate Enter on gadget we're over (if any) */
	    if (current_index != -1)  {
		Widget current_w = child[current_index];
		if (XtIsSensitive(current_w) == True) {
                    new_event.type = EnterNotify;
                    DispatchEvent(current_w, (XEvent *)&new_event);
		    gadget_data->last_used = current_index;
		}
            }
	}

	/* Now decide if Motion event itself should be delivered */
	if (current_index != -1) {

	    gadget = child[current_index];
#if 0
	    /*  Check if compression should be done  */
	    if (XtClass(gadget)->core_class.compress_motion) {
		/*  compress motion through this gadget  */
	    }
	    if (XtClass(gadget)->core_class.compress_enterleave) {
		/*  compress enter/leaves in this gadget  */
	    }
#endif

	    /* Dispatch event only if gadget is Sensitive */
	    if (XtIsSensitive(gadget) == True)
		DispatchEvent(gadget, event);
	}
	break;

    case Expose:
    case GraphicsExpose:
	/*  Can generate events to many gadgets  */
	{
	    WidgetList	gadget_list;

	    i = WidgetToGadgets(event->xexpose.x, event->xexpose.y,
			    event->xexpose.width, event->xexpose.height,
			    gadget_data, &gadget_list);

	    while(current_index < i)
	    {
		    DispatchEvent(*gadget_list++, event);
		    ++current_index;
	    }
	}
	break;
	
    case ClientMessage:
    case SelectionClear:
    case SelectionNotify:
    case SelectionRequest:
	/*  Send these events to all gadgets  */
	for (i = 0; i < gadget_data->num_gadgets; i++)
	    DispatchEvent(child[i], event);
	break;
	
    case CirculateNotify:
    case ColormapNotify:
    case ConfigureNotify:
    case CreateNotify:
    case DestroyNotify:
    case FocusIn:
    case FocusOut:
    case GravityNotify:
    case KeymapNotify:
    case MapNotify:
    case MappingNotify:
    case NoExpose:
    case PropertyNotify:
    case ReparentNotify:
    case ResizeRequest:
    case UnmapNotify:
    case VisibilityNotify:
	/*  These events do not apply to gadgets  */
	break;
    }
}				/*  DispatchGadgetEvent  */

static void
FreeGadgetData(Widget w)
{
    GadgetData ** gd = GetHashTblEntry(w);
    
    if (*gd != NULL) {
	GadgetData * tmp = *gd;			/* save pointer to entry */

	*gd = (*gd)->next;		/* point to next; may be NULL */

	if (tmp->gadget_children != NULL)
	    XtFree((char *)tmp->gadget_children); /* free list of children */
    
	XtFree((char *) tmp);				/* free gadget data */
    }
}

static GadgetData **
GetHashTblEntry(Widget w)
             			/* composite/parent of gadgets */
{

    static GadgetData * HashTbl[TABLE_SIZE];
    register GadgetData ** ptr;

    for (ptr = &HashTbl[ HASH(w) ]; *ptr != NULL; ptr = &((*ptr)->next))
	if (XtParent((*ptr)->gadget_children[0]) == w)
	    break;

    return (ptr);
}


static GadgetData *
GetGadgetData(Widget w, Boolean create)
          	  		/* composite/parent of gadgets */
           	       		/* make one if not pre-existing */
{
    GadgetData ** gd = GetHashTblEntry(w);

    if (*gd == NULL && create == CREATE) {

	/* GadgetData was not found for this composite so alloc one. */
	/* 'gd' points to correct link in list */

	*gd = (GadgetData *) XtMalloc(sizeof(GadgetData));
	(*gd)->gadget_children	= (WidgetList) NULL;
	(*gd)->num_gadgets	= 0;
	(*gd)->num_slots	= 0;
	(*gd)->last_used	= -1;
	(*gd)->next		= NULL;
    }
    return (*gd);
}

static Boolean
IntersectRect(Position x1, Position y1, Dimension rect_width, Dimension rect_height, Position x3, Position y3, int width, int height)
{
    int partIn = FALSE;
    int x2 = x1 + rect_width;
    int y2 = y1 + rect_height;
    int x4 = x3 + width;
    int y4 = y3 + height;
    
    if (x1 > x3)  {
	if (x4 >= x1)  {
	    partIn = TRUE;
	}
    } else {
	if (x2 >= x3)  {
	    partIn = TRUE;
	}
    }
    
    if (partIn)  {
	if (y1 > y3)  {
	    if (y4 >= y1)  {
		return(TRUE);
	    }
	} else  {
	    if (y2 >= y3)  {
		return(TRUE);
	    }
	}
    }
    return(FALSE);	
}				/*  IntersectRect  */

static int
WidgetToGadget(int x, int y, GadgetData *gadget_data)
{
    Cardinal		num_gadgets;
    Cardinal		current;
    WidgetList		child;
    
    /* If the position of the event does not intersect the
       bounding box of all gadgets, then give up
       if (!PointInRect(gadget_data->bounding_x,
       gadget_data->bounding_y,
       gadget_data->bounding_width,
       gadget_data->bounding_height, x, y))
       return(-1);
       */
    
    current	= (gadget_data->last_used == -1) ? 0 : gadget_data->last_used;
    child	= gadget_data->gadget_children;

    for (num_gadgets = gadget_data->num_gadgets;
	 num_gadgets != 0; current++, num_gadgets--) {
	if (current == gadget_data->num_gadgets)
	    current = 0;

	if (XtIsManaged(child[current])) /* only consider managed gadgets */
	{
	    Widget gadget = child[current];

	    if (PointInRect(gadget->core.x, gadget->core.y,
			gadget->core.width, gadget->core.height, x, y))
		return(current);
	}
    }
    return(-1);
}				/*  WidgetToGadget  */

static Cardinal
WidgetToGadgets(int x, int y, int width, int height, GadgetData *gadget_data, WidgetList *list_return)
{
#define MORESLOTS	4

    register Cardinal	num_gadgets, i;
    register WidgetList	child;

    static Widget *	gadget_list = NULL;
    static int		gadget_list_slots_left = 0;
    static int		gadget_list_alloced = 0;
    
    /*  If the position of the event does not intersect the
	bounding box of all gadgets, then give up
	if (!IntersectRect(gadget_data->bounding_x,
	gadget_data->bounding_y,
	gadget_data->bounding_width,
	gadget_data->bounding_height,
	x, y, width, height))
	return;
	*/

    gadget_list_slots_left = gadget_list_alloced;
    child		   = gadget_data->gadget_children;
    
    for (num_gadgets = gadget_data->num_gadgets, i = 0;
	 num_gadgets != 0; num_gadgets--, child++)
    {
	if (!XtIsManaged(*child))	/* only consider managed gadgets */
	    continue;

	if (IntersectRect((*child)->core.x, (*child)->core.y,
			  (*child)->core.width, (*child)->core.height,
			  x, y, width, height))
	{
	    if (gadget_list_slots_left == 0)
	    {
	       gadget_list_alloced += MORESLOTS;
	       gadget_list_slots_left += MORESLOTS;
	       gadget_list = (Widget *) XtRealloc((char *)gadget_list,
						gadget_list_alloced *
						sizeof (Widget));
	    }
	    gadget_list[i++] = *child;
	    gadget_list_slots_left--;
	}
    }

    *list_return = (i == (Cardinal)0 ? NULL : gadget_list);
    return(i);
#undef MORESLOTS
}				/*  WidgetToGadgets  */

/****************************class*procedures*****************************
 *
 * Class Procedures
 *
 */

/*****************************procedure*header*****************************
 * ClassInitialize- 
 * 
 */

static void
ClassInitialize(void)
{
#define EOC ((EventObjClass)eventObjClass)

    EOC->event_class.event_procs	= (OlEventHandlerList)
						_OlGenericEventHandlerList;
    EOC->event_class.num_event_procs	= _OlGenericEventHandlerListSize;

#undef EOC
}

/*************************************************************************
 * ClassPartInitialize - Provides for inheritance of the class procedures
 ***************************function*header*******************************/

static void
ClassPartInitialize(WidgetClass wc)
{
#define EC(wc)	((EventObjClass)wc)
#define SC(wc)	(EC(wc)->rect_class.superclass)
#define ECP(wc)	(EC(wc)->event_class)

#ifdef SHARELIB
	void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif
    /* Generate warning if version is less than 3.0	*/
    if (ECP(wc).version != OlVersion && ECP(wc).version < 3000)
    {
    	OlVaDisplayWarningMsg((Display *)NULL,
    			OleNinternal, OleTbadVersion,
    			OleCOlToolkitWarning, OleMinternal_badVersion,
    			EC(wc)->rect_class.class_name,
    			ECP(wc).version, OlVersion);
    }

    if (ECP(wc).highlight_handler == XtInheritHighlightHandler)
	ECP(wc).highlight_handler = ECP(SC(wc)).highlight_handler;

    if (ECP(wc).register_focus == XtInheritRegisterFocus)
	ECP(wc).register_focus = ECP(SC(wc)).register_focus;

    if (ECP(wc).traversal_handler == XtInheritTraversalHandler)
	ECP(wc).traversal_handler = ECP(SC(wc)).traversal_handler;

    if (ECP(wc).activate == XtInheritActivateFunc)
	ECP(wc).activate = ECP(SC(wc)).activate;

    if (ECP(wc).transparent_proc == XtInheritTransparentProc)
	ECP(wc).transparent_proc = ECP(SC(wc)).transparent_proc;
    if (ECP(wc).query_sc_locn_proc == XtInheritSuperCaretQueryLocnProc)
	ECP(wc).query_sc_locn_proc = ECP(SC(wc)).query_sc_locn_proc;

    /*	Since Xt does not define accept_focus for gadgets, Core.c will
	not do inheritance so it must be done here.
     */
    if (wc->core_class.accept_focus == XtInheritAcceptFocus)
	wc->core_class.accept_focus = SC(wc)->core_class.accept_focus;

    if (wc == eventObjClass)
	return;

    if (ECP(wc).dyn_data.num_resources == 0) {
	/* give the superclass's resource list to this class */
	ECP(wc).dyn_data = ECP(SC(wc)).dyn_data;
    }
    else {
	/* merge the two lists */
	_OlMergeDynResources(&(ECP(wc).dyn_data), &(ECP(SC(wc)).dyn_data));
    }

    if (ECP(wc).query_sc_locn_proc == XtInheritSuperCaretQueryLocnProc)
	ECP(wc).query_sc_locn_proc = ECP(SC(wc)).query_sc_locn_proc;

} /* ClassPartInitialize() */

/*****************************procedure*header*****************************
 * Destroy- 
 *   1	decrement number of gadget children in GadgetData.
 *   2	remove gadget pointer from gadget_children by moving down
 *	those that follow. 
 *   3	if this is the last gadget...
 * 
 * This is taken from CompositeDeleteChild in Composite.c
 */

static void
Destroy(Widget w)
{
    register Cardinal		position;
    register Cardinal		i;
    Widget			parent = XtParent(w);
    register GadgetData *	gd = GetGadgetData(parent, DONT_CREATE);
    register WidgetList		child = gd->gadget_children;
    
    /* none of the following (taken from CompositeDeleteChild in */
    /* Composite.c) really needs to be done if the parent is being */
    /* destoyed.  There is a check, however, in CompositeDeleteChild */
    /* for a child not found (this has been maintained in the code */
    /* below). */
    if (gd == NULL)
	return;

    for (position = 0; position < gd->num_gadgets; position++)
	if (child[position] == w)
	    break;
    
    if (position == gd->num_gadgets) {	/* not found ! */
	OlVaDisplayWarningMsg(XtDisplay(w), OleNinternal,
			      OleTdestroy,
			      OleCOlToolkitWarning,
			      "gadget to be destroyed not found");
	return;
    }

    gd->num_gadgets--;		/* one less gadget child */

    /* Ripple children down one space from "position" */
    for (i = position; i < gd->num_gadgets; i++)
	child[i] = child[i+1];

    if (gd->num_gadgets == 0)	/* last gadget child! */
	FreeGadgetData(parent);
    
    /* from WindObjDestroy in WindowObj.c */
    _XtFreeEventTable(&w->core.event_table);

    _OlDestroyKeyboardHooks(w);

    if (EOP(w).accelerator)
	XtFree(EOP(w).accelerator);
    if (EOP(w).qualifier_text)
	XtFree(EOP(w).qualifier_text);
    if (EOP(w).accelerator_text)
	XtFree(EOP(w).accelerator_text);
    if (EOP(w).reference_name)
	XtFree(EOP(w).reference_name);
}

/*****************************procedure*header*****************************
 * GetValuesHook - check for XtNreferenceWidget and XtNreferenceName
 *		and return info according to the traversal list
 */
static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	_OlGetRefNameOrWidget(w, args, num_args);
}

/*****************************procedure*header*****************************
 * Initialize- 
 *   *	install dispatcher on parent
 *   *	add destroy callback to parent so GadgetData can be deallocated
 *	when parent is destroyed
 *   *  allocate GadgetData for parent/composite & attach to heap
 *   *	add this child to gadget data
 */

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    GadgetData *	gd = GetGadgetData(XtParent(new), CREATE);
    int			validScale;
    
    /* initialize the event_table field  */
    new->core.event_table = NULL;
    
    /* add this child to gadget data (from CompositeInsertChild) */
    if (gd->num_gadgets == gd->num_slots) {
	/* Allocate more space */
	gd->num_slots += (gd->num_slots / 2) + 2;
	gd->gadget_children = (WidgetList)
	    XtRealloc((char *)gd->gadget_children,
		      gd->num_slots * sizeof(Widget));
    }
    gd->gadget_children[gd->num_gadgets] = new;
    gd->num_gadgets++;

    /* DEBUG */
    if (gd->num_gadgets > 1) {
	Widget w = gd->gadget_children[gd->num_gadgets - 2];
	if (XtParent(w) != XtParent(new))
	    OlVaDisplayWarningMsg(XtDisplay(new), OleNinternal,
				  OleTinitialize,
				  OleCOlToolkitWarning,
 "gadgets have different parents: current=%s, previous=%s",
				  XtParent(new)->core.name,
				  XtParent(w)->core.name); 
    }

    /* Make sure scale is supported by OLGX */
    validScale = OlgxGetValidScale(EOP(new).scale);
    EOP(new).scale = validScale;

    /* Initialize non-resource fields */

    EOP(new).has_focus = False;

    if (EOP(new).mnemonic)
	if (_OlAddMnemonic(new, (XtPointer)0, EOP(new).mnemonic) != OL_SUCCESS)
	    EOP(new).mnemonic = NULL;

    if (EOP(new).accelerator)
	if (_OlAddAccelerator(new, (XtPointer)0, EOP(new).accelerator) != OL_SUCCESS) {
	    EOP(new).accelerator = (String)NULL;
	    EOP(new).accelerator_text = (String)NULL;
	}

    EOP(new).qualifier_text = (String)NULL;
    EOP(new).meta_key = False;
    if (EOP(new).accelerator) {
	EOP(new).accelerator = XtNewString(EOP(new).accelerator);
	if (!(EOP(new).accelerator_text))
	    _OlMakeAcceleratorText(new, EOP(new).accelerator, 
		&EOP(new).qualifier_text, &EOP(new).meta_key, 
		&EOP(new).accelerator_text);
	else
	    EOP(new).accelerator_text = XtNewString(EOP(new).accelerator_text);
    } else if (EOP(new).accelerator_text)
	EOP(new).accelerator_text = XtNewString(EOP(new).accelerator_text);

		/* add me to the traversal list */
    if (EOP(new).reference_name)
	EOP(new).reference_name = XtNewString(EOP(new).reference_name);

    _OlUpdateTraversalWidget(new, EOP(new).reference_name,
			     EOP(new).reference_widget, True);

    _OlInitDynResources(new, &(((EventObjClass)(new->core.widget_class))->
		event_class.dyn_data));
    _OlCheckDynResources(new, &(((EventObjClass)(new->core.widget_class))->
		event_class.dyn_data), args, *num_args);
}

/*************************************************************************
 * SetValues - Sets up internal values based on changes made to external
 *	       ones
 ***************************function*header*******************************/

/* ARGSUSED */
static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	Boolean		redisplay = False;
	EventObj neweo = (EventObj)new;
	EventObj curreo = (EventObj)current;

	/* always reset text format */
	neweo->event.text_format = curreo->event.text_format;

#define CHANGED(field)	(EOP(new).field != EOP(current).field)

        /* Make sure scale is supported by OLGX */
	if (CHANGED(scale)) {
	    int validScale = OlgxGetValidScale(EOP(new).scale);
	    EOP(new).scale = validScale;
	    redisplay = True;
        }

	if (CHANGED(reference_name))	/* this has higher preference */
	{
		if (EOP(new).reference_name)
		{
			EOP(new).reference_name = XtNewString(
					EOP(new).reference_name);

			_OlUpdateTraversalWidget(new, EOP(new).reference_name,
					 NULL, False);
		}
		if (EOP(current).reference_name != NULL)
		{
			XtFree(EOP(current).reference_name);
			EOP(current).reference_name = NULL;
		}
	}
    	else if (CHANGED(reference_widget))
    	{
			/* no need to keep this around */
		if (EOP(current).reference_name != NULL)
		{
			XtFree(EOP(current).reference_name);
			EOP(current).reference_name = NULL;
		}

		_OlUpdateTraversalWidget(new, NULL,
				 EOP(new).reference_widget, False);
    	}

    if (CHANGED(mnemonic))
    {
	if (EOP(current).mnemonic != NULL)
	    _OlRemoveMnemonic(new, 0, False, EOP(current).mnemonic);

	if ((EOP(new).mnemonic != NULL) &&
	  (_OlAddMnemonic(new, 0, EOP(new).mnemonic) != OL_SUCCESS))
	    EOP(new).mnemonic = NULL;
	redisplay = True;
    }

    {
	Boolean changed_accelerator = CHANGED(accelerator);
	Boolean changed_accelerator_text = CHANGED(accelerator_text);

    if (changed_accelerator) {
	/*
	 * Remove the old accelerator.
	 */
	if (EOP(current).accelerator) {
	    _OlRemoveAccelerator(new, (XtPointer)0,
		False, EOP(current).accelerator);
        }

	/*
	 * Install the new accelerator
	 */
	if (EOP(new).accelerator) {
	    if (_OlAddAccelerator(new, (XtPointer)0,
		    EOP(new).accelerator) == OL_SUCCESS) {
		/*
		 * succeeds, reset the accelerator text
		 */
		EOP(new).accelerator =
		    XtNewString(EOP(new).accelerator);
		if (!changed_accelerator_text)
		    EOP(new).accelerator_text = (String)NULL;
	     } else {
		EOP(new).accelerator = (String)NULL;
		EOP(new).accelerator_text = (String)NULL;
	    }
        }

	/*
	 * Free the old accelerator string
	 */
	if (EOP(current).accelerator)
	    XtFree(EOP(current).accelerator);

    }

    if (changed_accelerator || changed_accelerator_text) {
	   /*
	    * Add the new accelerator text.
	    */
	   EOP(new).qualifier_text = (String)NULL;
	   EOP(new).meta_key = False;

	   if (changed_accelerator_text && EOP(new).accelerator_text)
	       EOP(new).accelerator_text = 
		   XtNewString(EOP(new).accelerator_text);
	   else if (EOP(new).accelerator)
	       _OlMakeAcceleratorText(new, EOP(new).accelerator, 
		   &EOP(new).qualifier_text, &EOP(new).meta_key, 
		   &EOP(new).accelerator_text);

	   /*
	    * Free the old accelerator text.
	    */
           if (EOP(current).qualifier_text)
			XtFree(EOP(current).qualifier_text);

           if (EOP(current).accelerator_text)
		        XtFree(EOP(current).accelerator_text);

	   redisplay = True;
    }
    }

	if (!XtIsSensitive(new) &&
	    (OlGetCurrentFocusWidget(new) == new)) {
		/*
		 * When it becomes insensitive, need to move focus elsewhere,
		 * if this widget currently has focus.
		 */
		OlMoveFocus(new, OL_IMMEDIATE, CurrentTime);
	}

#undef CHANGED

    _OlCheckDynResources(new, &(((EventObjClass)(new->core.widget_class))->
		event_class.dyn_data), args, *num_args);

    return (redisplay);
}	/* SetValues() */

static void
EventObjQuerySCLocnProc(const Widget		w,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return)
{
	EventObj	target = (EventObj)w;
	Widget		fwp	   = (XtIsShell(w) ? w : XtParent(w));
	EventObjClass fwwc = (EventObjClass)w->core.widget_class;
	SuperCaretShape rs = *shape;
	Dimension	fwp_width, fwp_height;

	if(!XtIsSubclass(w,eventObjClass) ||
		fwwc->event_class.query_sc_locn_proc == NULL) {
		*shape = SuperCaretNone;
		return;
	}
		
	fwp_width  = fwp->core.width,
	fwp_height = fwp->core.height;

	*x_center_return = (Position)0;
	*y_center_return = (Position)0;

	if (!(XtIsSubclass(w, managerWidgetClass) || XtIsShell(w))) {
		Widget	shell = _OlGetShellOfWidget(w);

		if(shell != NULL && XtIsSubclass(shell,menuShellWidgetClass))
			*shape = SuperCaretLeft;
		else
			*shape = (((int)fwp_width > (int)fwp_height)
				? SuperCaretBottom
				: SuperCaretLeft);
	} else 
		*shape = SuperCaretNone;

	if(target->event.scale != *scale || rs != *shape) {
		*scale = target->event.scale;
		return; /* try again */
	}

	switch (*shape) {
		case SuperCaretBottom:
			*x_center_return += w->core.width / 2;
			*y_center_return += w->core.height;
			break;
		case SuperCaretLeft:
			*y_center_return += w->core.height / 2;
			break;
	}
}

/****************************action*procedures****************************
 *
 * Action Procedures
 *
 */

/****************************public*procedures****************************
 *
 * Public Procedures
 *
 */

void
_OlAddEventHandler(Widget widget, EventMask eventMask,
	Boolean other, XtEventHandler proc, XtPointer closure)
{
    register XtEventRec	*p,**pp;
    EventMask		mask;
    Boolean		nonmaskable;
    
    if (!_OlIsGadget(widget)) {
	XtAddEventHandler(widget, eventMask, other, proc, closure);
	return;
    }
    
    if (eventMask == 0 && other == FALSE) return;
    
    pp = & widget->core.event_table;
    p = *pp;
    while (p != NULL &&
	   (p->proc != proc || p->closure != closure)) {
	pp = &p->next;
	p = *pp;
    }
    
    eventMask &= ~NonMaskableMask;
    if (other) eventMask |= NonMaskableMask;

    if (p == NULL) {
	/* new proc to add to list */
	p = XtNew(XtEventRec);
	p->proc = proc;
	p->closure = closure;
	p->mask = eventMask;
	p->select = TRUE;
	
	p->next = widget->core.event_table;
	widget->core.event_table = p;
	
    } else {
	/* update existing proc */
	p->mask |= eventMask;
	p->select |= TRUE;
    }
    
    /* build dispatching mask for parent from all gadget children */
    mask = BuildParentMask(widget, &nonmaskable);
    
    /* update entry for Dispatcher on parent */
    XtAddEventHandler(XtParent(widget), mask, nonmaskable,
		      DispatchGadgetEvent, NULL);
    
} /*  _OlAddEventHandler  */

void
_OlRemoveEventHandler(Widget widget,
	EventMask eventMask, Boolean other, XtEventHandler proc,
	XtPointer closure)
{
    register XtEventRec *p,**pp;
    
    if (!_OlIsGadget(widget)) {
	XtRemoveEventHandler(widget, eventMask, other, proc, closure);
	return;
    }
    pp = &widget->core.event_table;
    p = *pp;
    
    /* find it */
    while (p != NULL &&
	   (p->proc != proc || (p->closure != closure))) {
	pp = &p->next;
	p = *pp;
    }
    if (p == NULL) return;	/* couldn't find it */
    p->select = FALSE;
    /* un-register it */
    p->mask &= ~eventMask;
    if (p->mask == 0) {
	/* delete it entirely */
	*pp = p->next;
	XtFree((char *)p);
    }
    
    /* update entry for Dispatcher on parent */
    XtRemoveEventHandler(XtParent(widget), eventMask, other,
			 DispatchGadgetEvent, NULL);
}

Widget
_OlWidgetToGadget (Widget w, Position x, Position y)
{
    int			child_index;
    GadgetData *	gadget_data = GetGadgetData(w, DONT_CREATE);

    if (gadget_data == NULL)
	return(NULL);

    child_index = WidgetToGadget(x, y, gadget_data);

    if (child_index == -1)
	return(NULL);

    return(gadget_data->gadget_children[child_index]);
}

/* GrabDestroyCallback - removes the object from the grab list
 */
/* ARGSUSED */
static void
GrabDestroyCallback (Widget w, XtPointer client_data, XtPointer call_data)
{
	(void)HandleGrabs(eRemoveGrab, w, False, False);
} /* END OF GrabDestroyCallback() */

/* AllowGadgetGrabs -
 * flag that an application can set to permit gadgets to be used on
 * grab lists.  By Default, gadgets are not allowed to have grabs to
 * maintain backwards functionality.  If this value is set to TRUE,
 * the user will be able to drag the MENU button over menuButton gadgets
 * in a control area to preview the submenus.  When this is false,
 * once one submenu is displayed the user must dismiss the submenu before
 * being able to post another one.
 */
static Boolean
AllowGadgetGrabs (Widget w)
{
	static Boolean	first_time = True;
	static Boolean	allow_gadget_grabs;

	if (first_time == True)
	{
		XtResource	rsc;

		first_time = False;

		rsc.resource_name	= "allowGadgetGrabs";
		rsc.resource_class	= "AllowGadgetGrabs";
		rsc.resource_type	= XtRBoolean;
		rsc.resource_size	= sizeof(Boolean);
		rsc.resource_offset	= (Cardinal)0;
		rsc.default_type	= XtRImmediate;
		rsc.default_addr	= (XtPointer)False;

		XtGetApplicationResources(w, (XtPointer)&allow_gadget_grabs,
				&rsc, 1, NULL, (Cardinal)0);
	}
	return(allow_gadget_grabs);
} /* END OF AllowGadgetGrabs() */

/* HandleGrabs - 
 * Shadow the Intrinsic's grabList.  This routine uses a stack
 * where new elements are put onto the end of the stack.
 */
static Boolean
HandleGrabs (GrabCode opcode, Widget w,
	     Boolean exclusive, Boolean spring_loaded)
{
	static Cardinal	slots = 0;
	static Cardinal	num_grabs = 0;
	static GrabList	grablist = (GrabList)NULL;
	extern Boolean  _OlWidgetOnXtGrabList();

	if (AllowGadgetGrabs(w) == False)
	{
		if (_OlIsGadget(w) == False)
		{
			switch(opcode) {
			case eAddGrab:
				XtAddGrab(w, exclusive, spring_loaded);
				break;
			case eRemoveGrab:
				if (_OlWidgetOnXtGrabList(w))
					XtRemoveGrab(w);
				break;
			}
		}
		return (True);
	}

	if (opcode == eQueryGrabs ||
	    opcode == eQuerySiblingGrabs)
	{
		Cardinal	i;
		Widget		orig = w;

		if (num_grabs == 0)
		{
			return(True);
		}

		while (w != (Widget)NULL)
		{
			i = num_grabs;
			while (i)
			{
				--i;
				if (grablist[i].object == w)
					return True;
				else if (grablist[i].exclusive == True)
					break;
			}
			w = XtParent(w);
		}


		/* If we reach here, the requested object is not on
		 * the grabList.  However, since the Intrinsics maintains
		 * the real grab list we should check to see if the
		 * parent of the original object (if it was a gadget)
		 * is on the grab list or if any gadget siblings 
		 * are on the grablist.  If neither of these is true,
		 * chances are that the application has created another
		 * XtGrab (e.g., using XtPopupWidget with XtGrabExclusive)
		 * or with XtAddGrab explicitly and we can't detect that.
		 * We can be pretty sure of this since the event would
		 * not have gotten here in the first place if the Instrinsics
		 * didn't think the gadget's parent wasn't on the grablist.
		 * So we'll consider this a degenerate case and return TRUE.
		 *
		 * However, check to see if we're here because we're checking
		 * for a sibling grab.  If so, return FALSE.
		 */

		if (opcode == eQuerySiblingGrabs ||
		    _OlIsGadget(orig) == FALSE)
		{
			return (FALSE);
		}
		else
		{
			/* Check for siblings on the list.  If one's on
			 * the list return FALSE since this means the
			 * original gadget is not permitted to receive
			 * the event, else, check for the parent being on
			 * the list.
			 */

		    Cardinal		i;
		    GadgetData *	gd = GetGadgetData(XtParent(orig),
							DONT_CREATE);

		    for (i = 0; i < gd->num_gadgets; ++i)
		    {
			w = gd->gadget_children[i];

			if (w != orig &&
			    HandleGrabs(eQuerySiblingGrabs, w, False, False)
				== True)
			{
				return(FALSE);
			}
		    }

		    return ((ON_GRAB_LIST(XtParent(orig)) == FALSE ?
				TRUE : FALSE));
		}
	}
	else if (opcode == eAddGrab)
	{
		XtAddCallback(w, XtNdestroyCallback, GrabDestroyCallback,
					(XtPointer)NULL);

		if (num_grabs == slots)
		{
			slots += 4;
			grablist = (GrabList)XtRealloc((char *)grablist,
						slots * sizeof(GrabRec));
		}
		grablist[num_grabs].object	= w;
		grablist[num_grabs].exclusive	= exclusive;
		++num_grabs;
	}
	else /* eRemoveGrab */
	{
		Cardinal	i = num_grabs;

		while (i)
		{
			--i;
			if (grablist[i].object == w)
			{
				Cardinal	old_num = num_grabs;

				num_grabs = i;

				for(; i< old_num; ++i)
				{
					XtRemoveCallback(grablist[i].object,
							XtNdestroyCallback,
							GrabDestroyCallback,
							(XtPointer)NULL);
				}
				break;
			}
		}
	}

	if (_OlClass(w) == eventObjClass)
	{
		w = XtParent(w);
	}

	if (opcode == eAddGrab)
	{
		XtAddGrab(w, exclusive, spring_loaded);
	}
	else
	{
		/* Don't bother removing the XtGrab when this object is
		 * being destroyed since Xt will do it for us.
		 */
		if (w->core.being_destroyed == False)
			XtRemoveGrab(w);
	}
	return True;
} /* END OF HandleGrabs() */

/*
 * _OlAddGrab - must use our own grab routine since XtAddGrab doesn't work
 * for Gadgets.
 */
void
_OlAddGrab (Widget w, Boolean exclusive, Boolean spring_loaded)
{
	(void)HandleGrabs(eAddGrab, w, exclusive, spring_loaded);
} /* END OF _OlAddGrab() */

void
_OlRemoveGrab (Widget w)
{
	(void)HandleGrabs(eRemoveGrab, w, False, False);
} /* END OF _OlRemoveGrab() */
