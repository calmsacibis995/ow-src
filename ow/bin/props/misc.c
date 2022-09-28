#pragma ident	"@(#)misc.c	1.18	93/05/19 SMI"

/* Copyright */

/*
 * R5_SCREENSAVER: All code which supports the X server's ability to use an
 * image instead of video blanking for screen saving is hard code off via
 * ifdefs.
 *
 * These features were not supported in the X11/NeWS server and it was decided
 * not to support them in the early releases of VNext.
 *
 * NOTE: The code in this module is not sufficent to support this feature.
 * Enabling this code will result in program crashes.  For code which fully
 * this feature see the currently unused file, screensaver.c.
 */


#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>
#include <Xol/Slider.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>

#include "props.h"
#ifdef DBMALLOC
#	include <dbmalloc.h>
#endif


/***************************************************************************
* Defines
***************************************************************************/

strconst MISC_SCRSAVER_ONOFF_RES      = "OpenWindows.ScreenSaver.OnOff",
	 MISC_SCRSAVER_IDLE_RES       = "OpenWindows.ScreenSaver.IdleTime",
#ifdef ndef	/* See R5_SCREENSAVER in this file */
	 MISC_SAVE_METHOD_RES         = "OpenWindows.ScreenSaver.SaveMethod",
	 MISC_IMAGE_CYCLE_TIME_RES    ="OpenWindows.ScreenSaver.ImageCycleTime",
#endif
	 MISC_BEEP_RES                = "OpenWindows.Beep",
	 MISC_BEEP_DURATION_RES       = "OpenWindows.BeepDuration",
	 MISC_SET_INPUT_RES           = "OpenWindows.SetInput",
	 MISC_SCROLLBAR_PLACEMENT_RES = "OpenWindows.ScrollbarPlacement",
	 MISC_ICON_LOCATION_RES       = "OpenWindows.IconLocation";

enum {
    ON_OFF_ID,
    SAVE_METHOD_ID,
    IDLE_TIME_ID,
    IMAGE_CYCLE_TIME_ID,
    NUMBER_OF_ITEMS
};


/***************************************************************************
* Error Reporting
***************************************************************************/

typedef struct _MiscErrors {
        String badTimeoutValue;
} MiscErrors, *MiscErrorsPtr;
 
MiscErrors     miscErrors;
 
static XtResource errorAppResources[] = {
  { "badTimeoutValue", "BadTimeoutValue", XtRString, sizeof(String),
    XtOffset(MiscErrorsPtr, badTimeoutValue), XtRImmediate,
    "Please use numerals to specify minutes for the screen saver.\n"
    "Since the value \"%s\" you entered was blank or contained non-numeric\n"
    "characters, the screen saver timeout was reset to its previous value."
  }
};

/***************************************************************************
* Function Declarations
***************************************************************************/

/* strcasecmp(3C) is not in string.h, several bugs have been filed. */

extern int	 strcasecmp(const char *, const char *);
	

static void	 miscReadDbSettings(Widget, int, Boolean);
CategoryInfo    *miscRegisterCategory(void);
static void 	 miscRestoreSettings(Widget);
static Widget    miscCreateCategory(Widget);
static void	 miscInitializeServer(int);
static void 	 miscInitializeServerBeepDuration(int);
static void 	 miscInitializeServerScreensaver(int);
static Boolean	 miscRealizeSettings(Widget);
static Boolean	 miscRealizeSettingsBeepDuration(Widget);
static Boolean	 miscRealizeSettingsScreensaver(Widget);
static Boolean	 miscSyncWithServer(int, int);
static Boolean	 miscSyncWithServerBeepDuration(int, int);
static Boolean	 miscSyncWithServerScreensaver(int, int);

static Widget	 miscCategory;
static ItemInfo *ItemInfoArray[NUMBER_OF_ITEMS];


/***************************************************************************
* Registration Data & Function
***************************************************************************/

static CategoryInfo info = {
   "misc",
    miscCreateCategory,
    wspCreateChangeBars,
    wspDeleteChangeBars,
    wspBackupSettings,
    miscRestoreSettings,
    miscReadDbSettings,
    wspSaveDbSettings,
    miscRealizeSettings,
    miscInitializeServer,
    miscSyncWithServer,
    wspNoopShowCategory,
    wspNoopHideCategory
};


/* module global to allow duration slider inteminfo structure to be
   accessed in the realize settings subroutime */

static ItemInfo *durationInfo;


CategoryInfo*
miscRegisterCategory(void)
{
    return &info;
}

/*
 *  Miscellaneous Category Widget Tree
 *  ----------------------------------
 * mainControl
 *     miscCategory
 *	  screensaver
 *		control
 *		    exclusive
 *			button0
 *			button1
 *		    numeric
 *        beep
 *            exclusive
 *                button0
 *                button1
 *                button2
 *        beepDuration
 *            control
 *                slider
 *                textfield
 *        setInputArea
 *            exclusive
 *                button0
 *                button1
 *        iconLocation
 *            exclusive
 *                button0
 *                button1
 *                button2
 *                button3
 *        scrollbarPlacement
 *            exclusive
 *                button0
 *                button1
 */


/***************************************************************************
* Private Miscellaneous Category Functions
***************************************************************************/

/* ARGSUSED */
static void
miscScreensaverNumVerify(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    int		value,
		count;
    char	character;
    String	text;

    XtVaGetValues(widget, XtNstring, (XtArgVal)&text, NULL);
    count = sscanf(text, "%d%c", &value, &character);
    if (count == 1) {
        wspChangeSettingCb(widget, (XtPointer)value, (XtPointer)NULL);
    } else {
	ItemInfo *itemInfo;
	char      oldText[80];
	
	XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
	(void) sprintf(oldText, "%d", itemInfo->currVal);
	XtVaSetValues(widget, XtNstring, (XtArgVal)&oldText, NULL);

	wspErrorPopupConfirm(miscErrors.badTimeoutValue, text, NULL);
    }
}


/* ARGSUSED */
static void
miscScreensaverOff(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    Widget	wid = (Widget)clientData;

    XtSetSensitive(wid, FALSE);
    wspChangeSettingCb(widget, (XtPointer)0, (XtPointer)NULL);
}


/* ARGSUSED */
static void
miscScreensaverAuto(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    Widget	wid = (Widget)clientData;

    XtSetSensitive(wid, TRUE);
    wspChangeSettingCb(widget, (XtPointer)1, (XtPointer)NULL);
}

static Widget
miscCreateCategory(mainControl)
    Widget	mainControl;
{
    Widget	screenSaver,
		scrSaverOff,
		scrSaverAuto,
		beep,
		beepDuration,
		setInputArea,
		iconLocation,
		scrollbarPlacement,
		control,
		exclusive,
		button,
		slider,
		numcontrol,
		numeric;

    ItemInfo   *itemInfo;


    /*
     * Create control panel for miscellaneous category
     */
 
    miscCategory = XtVaCreateManagedWidget("miscCategory",
            controlAreaWidgetClass, mainControl,
	    XtNmappedWhenManaged,   FALSE,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)miscCategory, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    /*
     * Get application resources
     */

    XtGetApplicationResources(miscCategory, &miscErrors, errorAppResources,
	    XtNumber(errorAppResources), NULL, 0);

    /*
     * Create "Screen Saver" exclusive choice & numeric
     */

    screenSaver = XtVaCreateManagedWidget("screenSaver",
            captionWidgetClass, miscCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)screenSaver, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    ItemInfoArray[ON_OFF_ID] = wspNewItemInfo(0, MISC_SCRSAVER_ONOFF_RES,
	    dString, "off:auto", 0, screenSaver);

    control = XtVaCreateManagedWidget("control",
	    controlAreaWidgetClass, screenSaver,
	    NULL);

    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, control,
            NULL);

    scrSaverOff = XtVaCreateManagedWidget("button0",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)ItemInfoArray[ON_OFF_ID],
            NULL);

    scrSaverAuto = XtVaCreateManagedWidget("button1",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)ItemInfoArray[ON_OFF_ID],
            NULL);

    /* 
     * REMIND -- should be a real numeric widget, and should write
     *		 the correct resources.
     */
    ItemInfoArray[IDLE_TIME_ID] = wspNewItemInfo(0, MISC_SCRSAVER_IDLE_RES,
	    dNumber, NULL, 10, screenSaver);

    numcontrol = XtVaCreateManagedWidget("numcontrol",
	    controlAreaWidgetClass, control,
	    NULL);

    numeric = XtVaCreateManagedWidget("numeric",
	    textFieldWidgetClass, numcontrol,
            XtNuserData,   (XtArgVal)ItemInfoArray[IDLE_TIME_ID],
	    NULL);

    (void)  XtVaCreateManagedWidget("units",
	    staticTextWidgetClass, numcontrol,
	    NULL);

    XtAddCallback(numeric, XtNverification, miscScreensaverNumVerify, NULL);
    XtAddCallback(scrSaverOff,  XtNselect, miscScreensaverOff,  numcontrol);
    XtAddCallback(scrSaverAuto, XtNselect, miscScreensaverAuto, numcontrol);

    /*
     * Create "Beep" exclusive choice
     */

    beep = XtVaCreateManagedWidget("beep",
            captionWidgetClass, miscCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)beep, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MISC_BEEP_RES, dString,
            "always:notices:never", 0, beep );

    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, beep,
            NULL);

    button = XtVaCreateManagedWidget("button0",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)0);

    button = XtVaCreateManagedWidget("button1",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);

    button = XtVaCreateManagedWidget("button2",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)2);


    /*
     *  Create "Beep Duration" slider/textfield combination
     */
 
    beepDuration = XtVaCreateManagedWidget("beepDuration",
	    captionWidgetClass, miscCategory,
	    NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)beepDuration, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    durationInfo = wspNewItemInfo(0, MISC_BEEP_DURATION_RES, dNumber,
            NULL, 100, beepDuration);

    control = XtVaCreateManagedWidget("control",
            controlAreaWidgetClass, beepDuration,
            NULL);

    slider = XtVaCreateManagedWidget("slider",
            sliderWidgetClass, control,
            XtNuserData,     (XtArgVal)durationInfo,
            NULL);
    XtAddCallback(slider, XtNsliderMoved, wspChangeSettingCb,
	    (XtPointer)NULL);

    (void)  XtVaCreateManagedWidget("units",
	    staticTextWidgetClass, control,
            XtNuserData,   (XtArgVal)durationInfo,
	    NULL);


    /*
     * Create "Set Input Area" exclusive choice
     */

    setInputArea = XtVaCreateManagedWidget("setInputArea",
            captionWidgetClass, miscCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)setInputArea, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MISC_SET_INPUT_RES, dString,
            "select:followmouse", 0, setInputArea);

    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, setInputArea,
            NULL);

    button = XtVaCreateManagedWidget("button0",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)0);

    button = XtVaCreateManagedWidget("button1",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);


    /*
     * Create "Icon Location" exclusive choice
     */
 
    iconLocation = XtVaCreateManagedWidget("iconLocation",
            captionWidgetClass, miscCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)iconLocation, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MISC_ICON_LOCATION_RES, dString,
            "top:bottom:left:right", 1, iconLocation);

    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, iconLocation,
            NULL);

    button = XtVaCreateManagedWidget("button0",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)0);

    button = XtVaCreateManagedWidget("button1",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);

    button = XtVaCreateManagedWidget("button2",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)2);

    button = XtVaCreateManagedWidget("button3",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)3);


    /*
     * Create "Scrollbar Placement" exclusive choice 
     */
 
    scrollbarPlacement = XtVaCreateManagedWidget("scrollbarPlacement",
            captionWidgetClass, miscCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)scrollbarPlacement, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MISC_SCROLLBAR_PLACEMENT_RES,
            dString, "left:right", 1, scrollbarPlacement);

    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, scrollbarPlacement,
            NULL);

    button = XtVaCreateManagedWidget("button0",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)0);

    button = XtVaCreateManagedWidget("button1",
            rectButtonWidgetClass, exclusive,
            XtNuserData,   (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);

    return miscCategory;
}


/*
 * REMIND - need to add XGetScreenSaver() & XSetScreenSaver() to these
 */
static void
miscInitializeServer(
    int		dbAccess)
{
    miscInitializeServerBeepDuration(dbAccess);
    miscInitializeServerScreensaver(dbAccess);
}


static void
miscInitializeServerBeepDuration(
    int			dbAccess)
{
    XrmValue		value;

    XKeyboardControl	Keybd;


    if (wspGetResource(MISC_BEEP_DURATION_RES, dbAccess, &value) == TRUE){
    	Keybd.bell_duration = atoi((char*)value.addr);
 	XChangeKeyboardControl(display, KBBellDuration, &Keybd); 
    }
    /*
     * REMIND - what do we do if the returned value is not a
     * reasonable duration value ?
     */
}


static void
miscInitializeServerScreensaver(
    int         dbAccess)
{
    XrmValue    value;
 
    int         saveMethod,            /* Hardware blanking or image?        */
		allowExposures,        /* Ok to expose windows after saving? */
		idleTime,              /* Mins. to wait before saving screen */
		imageCycleTime;        /* Secs. between changing save image  */ 

    /*   
     * XSetScreeSaver() requires all screen saver values to be set when it is
     * called so we read the current screen saver values and use them as the
     * default values if the resource is not set.  If the resource is set we
     * over ride the values read from the server.
     */  
      
    XGetScreenSaver(display,
                &idleTime, &imageCycleTime, &saveMethod, &allowExposures);
 
#ifdef ndef	/* See R5_SCREENSAVER in this file */
    if (VendorRelease(display) >= 5000) {
        if (wspGetResource(MISC_SAVE_METHOD_RES, dbAccess, &value)) {
            if (strcasecmp("VideoBlanking",   (char *)value.addr) == 0) {
                saveMethod = PreferBlanking;
            } else if (strcasecmp("UseImage", (char *)value.addr) == 0) {
                saveMethod = DontPreferBlanking;
            }
        }      
    }
#endif

    if (wspGetResource(MISC_SCRSAVER_IDLE_RES, dbAccess, &value)) {
        /* Props represents idle time in minutes, but the server uses seconds.*/
        idleTime = atoi((char*)value.addr) * 60;
    }
 
#ifdef ndef	/* See R5_SCREENSAVER in this file */
    if (VendorRelease(display) >= 5000) {
        if (wspGetResource(MISC_IMAGE_CYCLE_TIME_RES, dbAccess, &value)) {
            imageCycleTime = atoi((char*)value.addr);
        }      
    }
#endif
 
    /*
     * Though the server combines disabling/enabling the screen saver with the
     * the idle time setting by using zero to mean disabled, seperate resources
     * are used so that a value for each can be stored.  Therefore the
     * MISC_SCRSAVER_ONOFF_RES over rides the MISC_SCRSAVER_IDLE_RES by setting
     * idleTime to zero if need be.
     */
 
    if (wspGetResource(MISC_SCRSAVER_ONOFF_RES, dbAccess, &value)) {
        if (strcasecmp("OFF", (char *)value.addr) == 0) {
            idleTime = 0;
        }      
    }
    
    XSetScreenSaver(display,
                    idleTime, imageCycleTime, saveMethod, allowExposures);
} 


static void
miscUpdateScreensaverNumeric(
    void)
{
    Widget	screenSaver,
    		numcontrol;


    screenSaver = XtNameToWidget(miscCategory, "*screenSaver*exclusive");
    numcontrol  = XtNameToWidget(miscCategory, "*numcontrol");
    
    if ((screenSaver != NULL) && (numcontrol != NULL)) {
	WidgetList	children;
	Cardinal	numChildren;
	int		ii;
	Boolean		buttonSet;


	XtVaGetValues(screenSaver,
	    XtNchildren,    (XtArgVal)&children,
	    XtNnumChildren, (XtArgVal)&numChildren,
	    NULL);
	for (ii = 0; ii < numChildren; ii++) {
	    XtVaGetValues(children[ii], XtNset, (XtArgVal)&buttonSet, NULL);
	    if (buttonSet) {
		break;
	    }
	}
	if (ii == 0) { /* Screensaver is off */
	    XtSetSensitive(numcontrol, FALSE);
	}
	if (ii == 1) { /* Screensaver is on */
	    XtSetSensitive(numcontrol, TRUE);
	}
    }
}


static void
miscRestoreSettings(
    Widget	widget)
{
    wspRestoreSettings(widget);
    miscUpdateScreensaverNumeric();
}


static void
miscReadDbSettings(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    wspReadDbSettings(widget, dbAccess, preserve);
    miscUpdateScreensaverNumeric();
}


static Boolean
miscRealizeSettings(
    Widget	 category)
{
    Boolean	 successful = TRUE;


    if (!miscRealizeSettingsBeepDuration(category)) {
	successful = FALSE;
    }
    if (!miscRealizeSettingsScreensaver(category)) {
	successful = FALSE;
    }
    
    return successful;
}


static Boolean
miscRealizeSettingsBeepDuration(
    Widget		 category)
{
    WidgetClass		widgetClass;
    XKeyboardControl	Keybd;


    widgetClass = XtClass(category);
    if (widgetClass != controlAreaWidgetClass) {
	return FALSE;
    }

    Keybd.bell_duration	= durationInfo->currVal; 
    XChangeKeyboardControl(display, KBBellDuration, &Keybd);
    
    return TRUE;
}


/* ARGSUSED */
static Boolean
miscRealizeSettingsScreensaver(
    Widget	 category)
{
    int          onOff,                /* Turn screen saving on or off.      */
                 saveMethod,           /* Hardware blanking or image?        */
                 allowExposures,       /* Ok to expose windows after saving? */
                 idleTime,             /* Mins. to wait before saving screen */
                 imageCycleTime;      /* Secs. between changing save image  */
    Widget	 screenSaverNumeric;

    /* First make sure the textField has been verified */

    screenSaverNumeric = XtNameToWidget(miscCategory, "*screenSaver*numeric");

    if (screenSaverNumeric != NULL) {
        miscScreensaverNumVerify(screenSaverNumeric, NULL, NULL);
    }

    XGetScreenSaver(display,
                &idleTime, &imageCycleTime, &saveMethod, &allowExposures);

    onOff                      = ItemInfoArray[ON_OFF_ID          ]->currVal;
    idleTime                   = ItemInfoArray[IDLE_TIME_ID       ]->currVal; 
#ifdef ndef	/* See R5_SCREENSAVER in this file */
    if (VendorRelease(display) >= 5000) {
        saveMethod             = ItemInfoArray[SAVE_METHOD_ID     ]->currVal;
    }
 
    if (VendorRelease(display) >= 5000) {
        imageCycleTime        = ItemInfoArray[IMAGE_CYCLE_TIME_ID]->currVal;
    }
#endif
 
    /*
     * The server uses an idle time of zero to turn the screen save off, so
     * override the MISC_SCRSAVER_IDLE_RES if the MISC_SCRSAVER_ONOFF_RES is
     * set to off.
     */
 
    if (onOff == 0) {
        idleTime = 0;
    }
 
    /* Props represents idle time in minutes, but the server uses seconds. */
    idleTime *= 60;
 
    XSetScreenSaver(display,
                    idleTime, imageCycleTime, saveMethod, AllowExposures);
 
    return TRUE;
}


static Boolean
miscSyncWithServer(
    int		 getDbAccess,
    int		 putDbAccess)
{
    Boolean 	 syncWithServer = FALSE;


    if (miscSyncWithServerBeepDuration(getDbAccess, putDbAccess)) {
	syncWithServer = TRUE;
    }
    if (miscSyncWithServerScreensaver(getDbAccess, putDbAccess)) {
	syncWithServer = TRUE;
    }
    
    return syncWithServer;
}


static Boolean
miscSyncWithServerBeepDuration(
    int		 	getDbAccess,
    int		 	putDbAccess)
{
    XKeyboardState	State;   
    Boolean	 	needToSyncWithServer = FALSE;
    XrmValue		value;
    unsigned int	resourceDuration, 
			serverDuration;	 	/* duration values */

    char		valueString[20]; /* space for new resource string */


    /* Get current keyboard settings from the server */
    XGetKeyboardControl(display, &State);
 
    if (wspGetResource(MISC_BEEP_DURATION_RES, getDbAccess, &value)) {
	resourceDuration = atoi((char *)value.addr);
    } else {

	/*
	 * Resource value not found in databases, so there
	 * is no need to continue checking.  Returns "false".
	 */

	return needToSyncWithServer;
    }
 
    /* set up convenience value, so test below makes sense */
    serverDuration = State.bell_duration;

    /* Update resource manager db to contain resources matching server state */
    if (serverDuration != resourceDuration) {
	(void) sprintf(valueString, "%d", serverDuration);
	wspPutResource(MISC_BEEP_DURATION_RES, valueString, putDbAccess);
	needToSyncWithServer = TRUE;
    }

    return needToSyncWithServer;
}


static Boolean
miscSyncWithServerScreensaver(
    int		 getDbAccess,
    int		 putDbAccess)
{
    int          saveMethod,           /* Hardware blanking or image?        */
                 allowExposures,       /* Ok to expose windows after saving? */
                 idleTime,             /* Mins. to wait before saving screen */
                 imageCycleTime;      /* Secs. between changing save image  */

    char         valueString[20];
    int          minutes;

    XrmValue     value;
   
    Boolean      needToSyncWithServer = FALSE;


    XGetScreenSaver(display,
                &idleTime, &imageCycleTime, &saveMethod, &allowExposures);
 
    /* The server uses an idle time of zero to turn the screen save off. */
    
    if (wspGetResource(MISC_SCRSAVER_ONOFF_RES, getDbAccess, &value)) {
        if (   (strcasecmp("OFF", (char *)value.addr) == 0)
            && (idleTime > 0))
        {
            wspPutResource(MISC_SCRSAVER_ONOFF_RES, "auto",  putDbAccess);
            needToSyncWithServer = TRUE;
        } else if (   (strcasecmp("ON", (char *)value.addr) == 0)
                   && (idleTime == 0))
        {
            wspPutResource(MISC_SCRSAVER_ONOFF_RES, "off", putDbAccess);
            needToSyncWithServer = TRUE;
        }
    } else { /* MISC_SCRSAVER_ONOFF_RES was not set. */
        if (idleTime > 0) {
            wspPutResource(MISC_SCRSAVER_ONOFF_RES, "auto",  putDbAccess);
            needToSyncWithServer = TRUE;
        } else {
            wspPutResource(MISC_SCRSAVER_ONOFF_RES, "off", putDbAccess);
            needToSyncWithServer = TRUE;
        }
    }
 
#ifdef ndef	/* See R5_SCREENSAVER in this file */
    if (VendorRelease(display) >= 5000) {
        if (wspGetResource(MISC_SAVE_METHOD_RES, getDbAccess, &value)) {
             if (   (strcasecmp("VideoBlanking", (char *)value.addr) == 0)
                 && (saveMethod == DontPreferBlanking))
             {
                wspPutResource(MISC_SAVE_METHOD_RES, "UseImage",
                                 putDbAccess);
                needToSyncWithServer = TRUE;
             } else if (   (strcasecmp("UseImage", (char *)value.addr) == 0)
                        && (saveMethod == PreferBlanking))
             {
                wspPutResource(MISC_SAVE_METHOD_RES, "VideoBlanking",
                                 putDbAccess);
                needToSyncWithServer = TRUE;
             }
        } else { /* MISC_SAVE_METHOD_RES was not set. */
            if (saveMethod == DontPreferBlanking) {
                wspPutResource(MISC_SAVE_METHOD_RES, "UseImage",
                                 putDbAccess);
                needToSyncWithServer = TRUE;
            } else {
                wspPutResource(MISC_SAVE_METHOD_RES, "VideoBlanking",
                                 putDbAccess);
                needToSyncWithServer = TRUE;
            }
        }      
    }
#endif
 
    /*
     * Idle time may have been set outside of props to a value less then one (1)
     * minute.  Since the screen saver interface only deals in minutes any value
     * less then 60, except zero is set to one minute.
     *
     * Times greater than 60 seconds are rounded to the minute.
     */
 
    if ((idleTime > 0) && (idleTime <= 60)) {
        minutes = 1;
    } else if ( (idleTime % 60) > 30 ) {       /* %: remainder */
        minutes = (idleTime / 60) + 1;         /* /: pos int div == truncate */
    } else {
        minutes = (idleTime / 60);
    }
 
    /*
     * An idle time of zero means that the screen saver is turned off.  This is
     * shown in the GUI via the Screen Saver On/Off setting.
     */
    if (idleTime > 0) {
        if (wspGetResource(MISC_SCRSAVER_IDLE_RES, getDbAccess, &value)) {
            if (atoi((char *)value.addr) != minutes) {
                (void) sprintf(valueString, "%d", minutes);
		wspPutResource(
		    MISC_SCRSAVER_IDLE_RES, valueString, putDbAccess);
                needToSyncWithServer = TRUE;
            }
        } else { /* MISC_SCRSAVER_IDLE_RES was not set. */
            (void) sprintf(valueString, "%d", minutes);
            wspPutResource(
                MISC_SCRSAVER_IDLE_RES, valueString, putDbAccess);
            needToSyncWithServer = TRUE;
        }
    }
    
#ifdef ndef	/* See R5_SCREENSAVER in this file */
    if (VendorRelease(display) >= 5000) {
        if (wspGetResource(MISC_IMAGE_CYCLE_TIME_RES, getDbAccess, &value)){
            if (atoi((char *)value.addr) != imageCycleTime) {
                (void) sprintf(valueString, "%d", imageCycleTime);
                wspPutResource(
                    MISC_IMAGE_CYCLE_TIME_RES, valueString, putDbAccess);
                needToSyncWithServer = TRUE;
            }
        } else { /* MISC_IMAGE_CYCLE_TIME_RES was not set. */
           (void) sprintf(valueString, "%d", imageCycleTime);
           wspPutResource(
               MISC_IMAGE_CYCLE_TIME_RES, valueString, putDbAccess);
           needToSyncWithServer = TRUE;
        }
    }
#endif
 
    return needToSyncWithServer;
}
