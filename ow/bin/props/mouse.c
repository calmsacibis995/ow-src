#pragma ident   "@(#)mouse.c	1.15    94/05/18 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/Nonexclusi.h>
#include <Xol/Notice.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>

#include "props.h"
#ifdef DBMALLOC
#	include <dbmalloc.h>
#endif


/***************************************************************************
* Defines
***************************************************************************/

#define MOUSE_UNKNOWN		  -1

#define MOUSE_ACCELERATION	0x01
#define MOUSE_THRESHOLD		0x02
#define MOUSE_POINTER_MAPPING	0x04


/***************************************************************************
* String Constants
***************************************************************************/

/*
 * Resource Names
 */

strconst MOUSE_SCROLLBAR_JUMP_CURSOR_RES = "OpenWindows.ScrollbarJumpCursor",
	 MOUSE_POPUP_JUMP_CURSOR_RES     = "OpenWindows.PopupJumpCursor",
	 MOUSE_MULTI_CLICK_TIMEOUT_RES   = "OpenWindows.MultiClickTimeout",
	 MOUSE_ACCELERATION_RES          = "OpenWindows.MouseAcceleration",
	 MOUSE_THRESHOLD_RES             = "OpenWindows.MouseThreshold",
	 MOUSE_POINTER_MAPPING_RES       = "OpenWindows.PointerMapping";

/*
 * Error Message Information
 */

typedef struct _MouseErrors {
    String	noItemInfo;
    String	badPtrMapping;
    String	buttonDown;
} MouseErrors, *MouseErrorsPtr;

MouseErrors	mouseErrors;

static XtResource appResources[] = {
	{ "noItemInfo", "NoItemInfo", XtRString, sizeof(String),
	  XtOffset(MouseErrorsPtr, noItemInfo), XtRImmediate,
	  "Missing itemInfo for widget \"%s\"."
	},
	{ "badPtrMapping", "BadPtrMapping", XtRString, sizeof(String),
	  XtOffset(MouseErrorsPtr, badPtrMapping), XtRImmediate,
	  "Bad return value %d from XSetPointerMapping()."
	},
	{ "buttonDown", "ButtonDown", XtRString, sizeof(String),
	  XtOffset(MouseErrorsPtr, buttonDown), XtRImmediate,
	  "Unable to switch mouse buttons.\nPerhaps you were holding down\n"
          "one of the mouse buttons?"
	}
};


/*
 * Other Strings
 */

strconst MOUSE_LEFT  = "left",
	 MOUSE_RIGHT = "right";


/***************************************************************************
* Function Declarations
***************************************************************************/

/*
 * Mouse category functions
 */

CategoryInfo   *mouseRegisterCategory(void);
static Widget	mouseCreateCategory(Widget);
static Boolean	mouseRealizeSettings(Widget);
static void	mouseInitializeServer(int);
static Boolean	mouseSyncWithServer(int, int);

/*
 * Mouse supporting functions
 */

static void	mouseGetChangedSettings(Widget, int*, int*, int*, int*);
static Boolean	mousePointerMapping(Widget, int);
static void	mousePointerMappingBusy(Display*, int);
static void	mouseSet(int, int, int, int);


/***************************************************************************
* Registration Data & Function
***************************************************************************/

static CategoryInfo info = {
   "mouse",
    mouseCreateCategory,
    wspCreateChangeBars,
    wspDeleteChangeBars,
    wspBackupSettings,
    wspRestoreSettings,
    wspReadDbSettings,
    wspSaveDbSettings,
    mouseRealizeSettings,
    mouseInitializeServer,
    mouseSyncWithServer,
    wspNoopShowCategory,
    wspNoopHideCategory
};


CategoryInfo*
mouseRegisterCategory(void)
{
    return &info;
}


/*
 *  Mouse Category Widget Tree
 *  --------------------------
 * mainControl
 *	mouseCategory
 *		acceleration
 *			slider
 *		threshold
 *			slider
 *			statictext
 *		multiClickInterval
 *			control
 *				text
 *				slider
 *				statictext
 *		buttonMapping
 *			exclusive
 *				button0
 *				button1
 *		scrollbarJumping
 *			nonexclusive
 *			checkbox
 *		popupJumping
 *			nonexclusive
 *			checkbox
 */              


/***************************************************************************
* Private Mouse Category Functions
***************************************************************************/

static Widget
mouseCreateCategory(
    Widget	mainControl)
{
    Widget	mouseCategory,
		scrollbarJumping,
		popupJumping,
		multiClickInterval,
		acceleration,
		threshold,
		buttonMapping,
		button,
		checkbox,
		control,
		exclusive,
		nonexclusive,
		slider;

    ItemInfo   *itemInfo;


    /*
     * Create mouse control area
     */
    mouseCategory = XtVaCreateManagedWidget("mouseCategory",
            controlAreaWidgetClass, mainControl,
	    XtNmappedWhenManaged,   FALSE,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)mouseCategory, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtGetApplicationResources(mouseCategory, &mouseErrors, appResources,
	    XtNumber(appResources), NULL, 0);

    /*
     * Create "Mouse Acceleration" slider item
     */
    acceleration = XtVaCreateManagedWidget("acceleration",
            captionWidgetClass, mouseCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)acceleration, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(MOUSE_ACCELERATION, MOUSE_ACCELERATION_RES,
            dNumber, NULL, 2, acceleration);
          
    slider = XtVaCreateManagedWidget("slider",
            sliderWidgetClass, acceleration,
            XtNuserData,    (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(slider, XtNsliderMoved, wspChangeSettingCb,
            (XtPointer)NULL);

    /*
     * Create "Mouse Threshold" slider item
     */
    threshold = XtVaCreateManagedWidget("threshold",
            captionWidgetClass, mouseCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)threshold, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(MOUSE_THRESHOLD, MOUSE_THRESHOLD_RES,
            dNumber, NULL, 15, threshold);

    control = XtVaCreateManagedWidget("control",
	    controlAreaWidgetClass,  threshold,
	    NULL);
          
    slider = XtVaCreateManagedWidget("slider",
            sliderWidgetClass, control,
            XtNuserData,    (XtArgVal)itemInfo,
            NULL);

    (void)  XtVaCreateManagedWidget("units",
	    staticTextWidgetClass, control,
            XtNuserData,    (XtArgVal)itemInfo,
	    NULL);

    XtAddCallback(slider, XtNsliderMoved, wspChangeSettingCb,
            (XtPointer)NULL);

    /*
     * Create "Multi-click Timeout" slider item
     */
    multiClickInterval = XtVaCreateManagedWidget("multiClickInterval",
            captionWidgetClass, mouseCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)multiClickInterval, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MOUSE_MULTI_CLICK_TIMEOUT_RES,
            dNumber, NULL, 4, multiClickInterval);

    control = XtVaCreateManagedWidget("control",
	    controlAreaWidgetClass,  multiClickInterval,
	    NULL);
          
    slider = XtVaCreateManagedWidget("slider",
            sliderWidgetClass, control,
            XtNuserData,    (XtArgVal)itemInfo,
            XtNorientation, (XtArgVal)OL_HORIZONTAL,
            NULL);

    (void)  XtVaCreateManagedWidget("units",
	    staticTextWidgetClass, control,
	    NULL);

    XtAddCallback(slider, XtNsliderMoved, wspChangeSettingCb,
            (XtPointer)NULL);

    /*
     * Create "Mouse Buttons" exclusive choice
     */
    buttonMapping = XtVaCreateManagedWidget("buttonMapping",
            captionWidgetClass, mouseCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)buttonMapping, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(MOUSE_POINTER_MAPPING,
            MOUSE_POINTER_MAPPING_RES,
            dString, "right:left", 0, buttonMapping);

    exclusive = XtVaCreateManagedWidget("exclusive", 
            exclusivesWidgetClass, buttonMapping,
            NULL);

    button = XtVaCreateManagedWidget("button0",
            rectButtonWidgetClass, exclusive,
            XtNuserData, (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)0);
    button = XtVaCreateManagedWidget("button1",
            rectButtonWidgetClass, exclusive,
            XtNuserData, (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);

    /*
     * Create "Scrollbar Pointer Jumping" checkbox
     */
    scrollbarJumping = XtVaCreateManagedWidget("scrollbarJumping",
            captionWidgetClass, mouseCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)scrollbarJumping, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MOUSE_SCROLLBAR_JUMP_CURSOR_RES,
            dBoolean, NULL, TRUE, scrollbarJumping);

    nonexclusive = XtVaCreateManagedWidget("nonexclusive",
            nonexclusivesWidgetClass, scrollbarJumping,
            NULL);
    checkbox = XtVaCreateManagedWidget("checkbox",
            checkBoxWidgetClass, nonexclusive,
            XtNuserData, (XtArgVal)itemInfo,
            NULL);
             
    XtAddCallback(checkbox, XtNunselect, wspChangeSettingCb, (XtPointer)0);
    XtAddCallback(checkbox, XtNselect,   wspChangeSettingCb, (XtPointer)1);

    /*
     * Create "Pop-up Pointer Jumping" checkbox
     */
    popupJumping = XtVaCreateManagedWidget("popupJumping",
            captionWidgetClass, mouseCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)popupJumping, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MOUSE_POPUP_JUMP_CURSOR_RES,
            dBoolean, NULL, TRUE, popupJumping);

    nonexclusive = XtVaCreateManagedWidget("nonexclusive",
            nonexclusivesWidgetClass, popupJumping,
            NULL);

    checkbox = XtVaCreateManagedWidget("checkbox",
            checkBoxWidgetClass, nonexclusive,
            XtNuserData, (XtArgVal)itemInfo,
            NULL);
             
    XtAddCallback(checkbox, XtNunselect, wspChangeSettingCb, (XtPointer)0);
    XtAddCallback(checkbox, XtNselect,   wspChangeSettingCb, (XtPointer)1);

    return mouseCategory;
}


static Boolean
mouseRealizeSettings(
    Widget	mouseWidget)
{
    int         flag,
    		acceleration,
    		threshold,
    		lefty;


    mouseGetChangedSettings(mouseWidget, &flag, &acceleration,
             &threshold, &lefty);

    if ((flag & MOUSE_ACCELERATION) || (flag & MOUSE_THRESHOLD)) {
        mouseSet(True, True, acceleration, threshold);
    }

    if (flag & MOUSE_POINTER_MAPPING) {
        if (!mousePointerMapping(mouseWidget, lefty)) {
            return FALSE;
        }
    }
    
    return TRUE;
}


static void
mouseInitializeServer(
    int		dbAccess)
{
    int		acceleration	= 1,
		threshold	= 1,
		doAcceleration,
		doThreshold;

    XrmValue	value;


    if (!wspGetResource(MOUSE_ACCELERATION_RES, dbAccess, &value)) {
        doAcceleration = False;
    } else {
        acceleration   = atoi((char*)value.addr);
        doAcceleration = True;
    }

    if (!wspGetResource(MOUSE_THRESHOLD_RES, dbAccess, &value)) {
        doThreshold = False;
    } else {
        threshold   = atoi((char*)value.addr);
        doThreshold = True;
    }
    mouseSet(doAcceleration, doThreshold, acceleration, threshold);
 
    if (!wspGetResource(MOUSE_POINTER_MAPPING_RES, dbAccess, &value)) {
        return;
    }
    if (!strcmp((char*)value.addr, "left")) {
        (void) mousePointerMapping(NULL, 1);		/* Lefty */
    } else if (!strcmp((char*)value.addr, "right")) {
        (void) mousePointerMapping(NULL, 0);		/* Righty */
    }
}


static Boolean
mouseSyncWithServer(
    int		getDbAccess,
    int		putDbAccess)
{
    int		resourceAcceleration, serverAcceleration, /* acceleration */
		resourceThreshold,    serverThreshold,	  /* threshold */
		resourceLeft,         serverLeft,    /* lefty button mapping */
		nn,
		sync;
    XrmValue	value;
    char	valueString[20];
    
    unsigned char map[256];


    sync = FALSE;
 
    /* Get mouse acceleration and threshold values from the databases */

    if (!wspGetResource(MOUSE_ACCELERATION_RES, getDbAccess, &value)) {
        resourceAcceleration = -1;	/* Resource value not found in databases */
    } else {
        resourceAcceleration = atoi((char*)value.addr);
    }

    if (!wspGetResource(MOUSE_THRESHOLD_RES, getDbAccess, &value)) {
        resourceThreshold = -1;	/* Resource value not found in databases */
    } else {
        resourceThreshold = atoi((char*)value.addr);
    }
 
 
    /* Get current mouse acceleration and threshold values from the server */

    XGetPointerControl(display, &serverAcceleration, &nn, &serverThreshold);
 
    /* Update resource manager db to contain resources matching server state */
    if (serverAcceleration != resourceAcceleration) {
        (void) sprintf(valueString, "%d", serverAcceleration);
        wspPutResource(MOUSE_ACCELERATION_RES, valueString, putDbAccess);
        sync = TRUE;
    }
    if (serverThreshold != resourceThreshold) {
        (void) sprintf(valueString, "%d", serverThreshold);
        wspPutResource(MOUSE_THRESHOLD_RES, valueString, putDbAccess);
        sync = TRUE;
    }


    /* Get mouse pointer setting from resource databases */

    resourceLeft = serverLeft = MOUSE_UNKNOWN;
    if (wspGetResource(MOUSE_POINTER_MAPPING_RES, getDbAccess, &value)) {
        /* Determine if ptr setting is right- or left-handed */
        if (!strcmp((char*)value.addr, "left")) {
            resourceLeft = TRUE;
        } else if (!strcmp((char*)value.addr, "right")) {
            resourceLeft = FALSE;
        }
    }


    /* Get the current pointer mapping from the server */

    nn = XGetPointerMapping(display, map, 256);
    if (nn != 3) {	/* We only handle a 3-button mouse */
        return sync;
    }

    if ((map[0] == 3) && (map[1] == 2) && (map[2] == 1)) {
        serverLeft = TRUE;
    } else if ((map[0] == 1) && (map[1] == 2) && (map[2] == 3)) {
        serverLeft = FALSE;
    }


    /* Update resource manager db to contain resources matching server state */

    if (serverLeft != resourceLeft) {
        if (serverLeft == TRUE) {
            wspPutResource(MOUSE_POINTER_MAPPING_RES, "left", putDbAccess);
            sync = TRUE;
        } else if (serverLeft == FALSE) {
            wspPutResource(MOUSE_POINTER_MAPPING_RES, "right", putDbAccess);
            sync = TRUE;
        }
    }

    return sync;
}


/***************************************************************************
* Private Supporting Functions
***************************************************************************/
 
static void
mouseGetChangedSettings(
    Widget	 widget,
    int		*which,
    int		*acceleration,
    int		*threshold,
    int		*lefty)
{
    Cardinal     numberOfChildren, ii;
    WidgetList   children;
    WidgetClass  widgetClass;
    ItemInfo    *itemInfo;


    widgetClass = XtClass(widget);
    if (wspSettableWidgetClass(widgetClass)) {
        OlDefine changeBar;

        XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
        if (itemInfo == NULL) {
	    wspErrorStandard("mouseGetChangedSettings()",
		mouseErrors.noItemInfo, XtName(widget), NULL);

            return;
        }

        XtVaGetValues(itemInfo->caption,
                XtNchangeBar, (XtArgVal)&changeBar,
                NULL);

        switch(itemInfo->id) {
        case MOUSE_ACCELERATION:
            *acceleration = (int)itemInfo->currVal;
            if (changeBar == OL_NORMAL) {
                *which |= MOUSE_ACCELERATION;
            }
            break;
        case MOUSE_THRESHOLD:
            *threshold = (int)itemInfo->currVal;
            if (changeBar == OL_NORMAL) {
                *which |= MOUSE_THRESHOLD;
            }
            break;
        case MOUSE_POINTER_MAPPING:
            *lefty = (int)itemInfo->currVal;
            if (changeBar == OL_NORMAL) {
                *which |= MOUSE_POINTER_MAPPING;
            }
            break;
        }
        
        return; /* Don't need to traverse subtree of settable widget */
    }

    if (XtIsSubclass(widget, compositeWidgetClass)) {
        XtVaGetValues(widget,
                      XtNnumChildren, &numberOfChildren,
                      XtNchildren,    &children,
                      NULL);
        for (ii = 0; ii < numberOfChildren; ii++) {
            mouseGetChangedSettings(children[ii], which, acceleration,
                                       threshold, lefty);
        }
    }
}


static Boolean
mousePointerMapping(
    Widget	mouseWidget,
    int		lefty)  /* Non-zero value means switch to lefty, else righty */
{
    int		retries,
		timeout,
    		result, nmap;
		
    unsigned char map[3];
 
 
    nmap = XGetPointerMapping(display, map, 3);
    if (nmap == 2) {
	if (lefty == TRUE) {
            map[0] = 2;
            map[1] = 1;
        } else {
            map[0] = 1;
            map[1] = 2;
        }
    } else if (nmap == 3) {
        map[1] = 2;
        if (lefty == TRUE) {
            map[0] = 3;
            map[2] = 1;
        } else {
            map[0] = 1;
            map[2] = 3;
        }
    } else {
	return FALSE;
    }

    for (retries = 3, timeout  = 1;
         retries > 0;
         retries--,   timeout *= 2)
    {
 
	result = XSetPointerMapping(display, map, nmap);
        switch(result) {
        case MappingSuccess:
            return TRUE;
        case MappingBusy:
            mousePointerMappingBusy(display, timeout);
            continue;
        default:
	    if (retries == 1) {
		wspErrorStandard("mousePointerMapping()",
	            mouseErrors.badPtrMapping, result, NULL);

		return FALSE;
	    }
        }
    }

    wspErrorPopupConfirm(mouseErrors.buttonDown, NULL);

    return FALSE;
}


static void
mousePointerMappingBusy(
    Display	*display,
    int		 timeout) /* Seconds to wait for user to release a button */
{
    Window		root, child;		/* dummy variables */
    int			rx, ry, wx, wy;		/* dummy variables */
    unsigned int	mask;

 
    if (!XQueryPointer(display, RootWindow(display,DefaultScreen(display)),
                       &root, &child, &rx, &ry, &wx, &wy, &mask))
    {
        mask = 0;
    }
    (void) sleep(timeout);
}


static void
mouseSet(
    int doAcc,       /* Non-zero value indicates set the mouse acceleration */
    int doThr,       /* Non-zero value indicates set the mouse threshold    */
    int acceleration, /* Acceleration value to set */
    int threshold)    /* Threshold value to set    */
{
    if (acceleration < 0) {	/* shouldn't happen */
        acceleration = 1;
    }

    if (threshold <= 0) {	/* shouldn't happen */
        threshold = 1;
    }

    XChangePointerControl(display, doAcc, doThr, acceleration, 1, threshold);
}

