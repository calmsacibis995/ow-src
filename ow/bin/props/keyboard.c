#pragma ident	"@(#)keyboard.c	1.19	93/01/21 SMI"

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>
#include <Xol/CheckBox.h>
#include <Xol/Nonexclusi.h>
#include "props.h"

#ifdef DBMALLOC
	#include <dbmalloc.h>
#endif

/***************************************************************************
* Defines
***************************************************************************/

/*
 * Resource Names
 */

static const char KEYBOARD_MOUSELESS_RES[]    =	"OpenWindows.KeyboardCommands";
static const char KEYBOARD_WINMENUACCEL_RES[] =	"OpenWindows.WindowMenuAccelerators";
static const char KEYBOARD_MENUACCEL_RES[]    =	"OpenWindows.MenuAccelerators";
static const char KEYBOARD_KEYCLICK_RES[]     =	"OpenWindows.KeyClick";
static const char KEYBOARD_KEYREPEAT_RES[]    =	"OpenWindows.KeyRepeat";

typedef enum {
    MenuAccelAppNWin,
    MenuAccelAppOnly,
    MenuAccelNone
} MenuAccelType;

/***************************************************************************
* Function Declarations
***************************************************************************/

/* strcasecmp(3C) is not in string.h, several bugs have been filed. */

extern int       strcasecmp(const char *, const char *);

CategoryInfo  *keyboardRegisterCategory(void);
static Widget  keyboardCreateCategory(Widget);
static void    keyboardReadDbSettings(Widget, int, Boolean);
static void    keyboardSaveDbSettings(Widget, int);
static Boolean keyboardRealizeSettings(Widget);
static void    keyboardInitializeServer(int);
static Boolean keyboardSyncWithServer(int, int);

/***************************************************************************
* Registration Data & Function
***************************************************************************/

static CategoryInfo info = {
    "keyboard",
    keyboardCreateCategory,
    wspCreateChangeBars,
    wspDeleteChangeBars,
    wspBackupSettings,
    wspRestoreSettings,
    keyboardReadDbSettings,
    keyboardSaveDbSettings,
    keyboardRealizeSettings,
    keyboardInitializeServer,
    keyboardSyncWithServer,
    wspNoopShowCategory,
    wspNoopHideCategory
};

CategoryInfo*
keyboardRegisterCategory(void)
{
    return(&info);
}

/***************************************************************************

           Keyboard Category Widget Tree
           -----------------------------

mainControl
	keyboardCategory
		mouseless
			exclusive
				button00
				button01
		menuAccel
			exclusive
				button00
				button01
				button02
		keyClick
			nonexclusive
				checkbox
		repeatKeys
			nonexclusive
				checkbox

***************************************************************************/

/***************************************************************************
* Private Keyboard Category Functions
***************************************************************************/

/*
 * Keep this stuff around to avoid having to find it again.
 */
static ItemInfo *menuAccelItemInfo,
		*keyClickItemInfo,
		*keyRepeatItemInfo;
static Widget    menuAccelButton;

static Widget
keyboardCreateCategory(Widget mainControl)
{
    Widget    keyboardCategory,
              mouseless,
              keyClick,
              repeatKeys,
	      menuAccel,
              button,
              checkbox,
              exclusive,
              nonexclusive;
    ItemInfo *itemInfo;

    /*
     * Create control area for keyboard category
     */
    keyboardCategory = XtVaCreateManagedWidget("keyboardCategory",
            controlAreaWidgetClass, mainControl,
	    XtNmappedWhenManaged,   FALSE,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)keyboardCategory, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    /*
     * Create "Mouseless" exclusive choice
     */

/*
 * REMIND -- KeyboardCommands can have a value of SunView1 which should
 *	be forced to Basic and not presented to the user since that value
 *	had been deprecated.  The props foundation is not equipped to
 *	handle ItemInfo's that include values that don't correspond to
 *	buttons; we'll have to handle it separately.  Currently,
 *	propsReadDbSetting will complain (noStringConversion) and default
 *	to Basic.
 */
    mouseless = XtVaCreateManagedWidget("mouseless",
            captionWidgetClass, keyboardCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)mouseless, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, KEYBOARD_MOUSELESS_RES,
	    dString, "Basic:Full", 0, mouseless);
    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, mouseless,
            NULL);

    button = XtVaCreateManagedWidget("button00",
            rectButtonWidgetClass, exclusive,
            XtNuserData, (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)0);

    button = XtVaCreateManagedWidget("button01",
            rectButtonWidgetClass, exclusive,
            XtNuserData, (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);

    /*
     * Create "Menu Accelerators" exclusive choice
     */
    menuAccel = XtVaCreateManagedWidget("menuAccel",
		captionWidgetClass, keyboardCategory,
		NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)menuAccel, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    menuAccelItemInfo = wspNewItemInfo(0, NULL, dPrivate, NULL, TRUE, menuAccel);

    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, menuAccel,
            NULL);

    menuAccelButton = XtVaCreateManagedWidget("button00",
            rectButtonWidgetClass, exclusive,
            XtNuserData, (XtArgVal)menuAccelItemInfo,
            NULL);
    XtAddCallback(menuAccelButton, XtNselect, wspChangeSettingCb, (XtPointer)0);

    button = XtVaCreateManagedWidget("button01",
            rectButtonWidgetClass, exclusive,
            XtNuserData, (XtArgVal)menuAccelItemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);

    button = XtVaCreateManagedWidget("button02",
            rectButtonWidgetClass, exclusive,
            XtNuserData, (XtArgVal)menuAccelItemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)2);


    /*
     * Create "Key Click" exclusive choice
     */
    keyClick = XtVaCreateManagedWidget("keyClick",
            captionWidgetClass, keyboardCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)keyClick, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    keyClickItemInfo = wspNewItemInfo(0, KEYBOARD_KEYCLICK_RES,
	    dBoolean, NULL, FALSE, keyClick);

    nonexclusive = XtVaCreateManagedWidget("nonexclusive",
            nonexclusivesWidgetClass, keyClick,
            NULL);
    checkbox = XtVaCreateManagedWidget("checkbox",
            checkBoxWidgetClass, nonexclusive,
            XtNuserData,   (XtArgVal)keyClickItemInfo,
            NULL);

    XtAddCallback(checkbox, XtNunselect, wspChangeSettingCb, (XtPointer)0);
    XtAddCallback(checkbox, XtNselect, wspChangeSettingCb, (XtPointer)1);

    /*
     * Create "Repeat Key" exclusive choice
     */
    repeatKeys = XtVaCreateManagedWidget("repeatKeys",
            captionWidgetClass, keyboardCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)repeatKeys, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    keyRepeatItemInfo = wspNewItemInfo(0, KEYBOARD_KEYREPEAT_RES,
	    dBoolean, NULL, TRUE, repeatKeys);

    nonexclusive = XtVaCreateManagedWidget("nonexclusive",
            nonexclusivesWidgetClass, repeatKeys,
            NULL);
    checkbox = XtVaCreateManagedWidget("checkbox",
            checkBoxWidgetClass, nonexclusive,
            XtNuserData, (XtArgVal)keyRepeatItemInfo,
            NULL);

    XtAddCallback(checkbox, XtNunselect, wspChangeSettingCb, (XtPointer)0);
    XtAddCallback(checkbox, XtNselect, wspChangeSettingCb, (XtPointer)1);

    return(keyboardCategory);
}

static void
keyboardReadDbSettings(
    Widget	keyboardCategory,
    int		dbAccess,
    Boolean	preserve)
{
    XrmValue	menuAccelVal, winMenuAccelVal;

    wspReadDbSettings(keyboardCategory, dbAccess, preserve);
    wspGetResource(KEYBOARD_MENUACCEL_RES, dbAccess, &menuAccelVal);
    wspGetResource(KEYBOARD_WINMENUACCEL_RES, dbAccess, &winMenuAccelVal);

    if (preserve) {
	OlDefine changeBar;

	XtVaGetValues(menuAccelItemInfo->caption,
		XtNchangeBar, (XtArgVal)&changeBar,
		NULL);
	if (changeBar != OL_NONE)
		return;
    }
    menuAccelItemInfo->currVal = MenuAccelNone; /* Set to "None" by default */
    if (menuAccelVal.size != 0) {
	if (strcasecmp(menuAccelVal.addr, "TRUE") == 0) {
	    menuAccelItemInfo->currVal = MenuAccelAppOnly;
  	    if (winMenuAccelVal.size != 0 &&
		strcasecmp(winMenuAccelVal.addr, "TRUE") == 0) {
		    menuAccelItemInfo->currVal = MenuAccelAppNWin;
	    }
	}
    }
    wspSetWidgetState(menuAccelButton, menuAccelItemInfo->currVal);
}

static void
keyboardSaveDbSettings(
    Widget	keyboardCategory,
    int		dbAccess)
{
    char       *menuAccel, *winMenuAccel;
    OlDefine    changeBar;

    wspSaveDbSettings(keyboardCategory, dbAccess);

    XtVaGetValues(menuAccelItemInfo->caption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if (changeBar == OL_NORMAL) {
        switch(menuAccelItemInfo->currVal) {
        case MenuAccelAppNWin:
	    menuAccel    = "True";
	    winMenuAccel = "True";
	    break;
        case MenuAccelAppOnly:
	    menuAccel    = "True";
	    winMenuAccel = "False";
	    break;
        case MenuAccelNone:
        default:
	    menuAccel    = "False";
	    winMenuAccel = "False";
	    break;
        }
        wspPutResource(KEYBOARD_MENUACCEL_RES, menuAccel, dbAccess);
        wspPutResource(KEYBOARD_WINMENUACCEL_RES, winMenuAccel, dbAccess);
    }
}

/*ARGSUSED*/
static Boolean
keyboardRealizeSettings(
    Widget           keyboardWidget)
{
    int              flag = 0;
    OlDefine         changeBar;
    XKeyboardControl kbControl;

    XtVaGetValues(keyClickItemInfo->caption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if (changeBar == OL_NORMAL) {
	flag |= KBKeyClickPercent;
        kbControl.key_click_percent = ((keyClickItemInfo->currVal) ? 100 : 0);
    }

    XtVaGetValues(keyRepeatItemInfo->caption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if (changeBar == OL_NORMAL) {
	flag |= KBAutoRepeatMode;
        kbControl.auto_repeat_mode = (keyRepeatItemInfo->currVal);
    }

    if (flag) {
    	XChangeKeyboardControl(display, flag, &kbControl);
    }

    return(TRUE);
}

static void
keyboardInitializeServer(int dbAccess)
{
    XKeyboardControl kbControl;
    XrmValue         value;
    Boolean	     resourceValue;

    if (wspGetResource(KEYBOARD_KEYCLICK_RES, dbAccess, &value)) {
	resourceValue = (strcmp((char*)value.addr, "True") == 0);
        kbControl.key_click_percent = ((resourceValue) ? 100 : 0);
    } else {
        kbControl.key_click_percent = 0;
    }

    if (wspGetResource(KEYBOARD_KEYREPEAT_RES, dbAccess, &value)) {
	resourceValue = (strcmp((char*)value.addr, "True") == 0);
        kbControl.auto_repeat_mode = (resourceValue);
    } else {
        kbControl.auto_repeat_mode = (TRUE);
    }

    XChangeKeyboardControl(display, KBKeyClickPercent | KBAutoRepeatMode,
				    &kbControl);
}

static Boolean
keyboardSyncWithServer(
    int            getDbAccess,
    int            putDbAccess)
{
    Boolean        resKeyclick  = FALSE,
                   resKeyrepeat = TRUE,
	           svrKeyclick,
	           svrKeyrepeat,
                   sync = FALSE;
    XrmValue       value;
    XKeyboardState kbState;

    /* Get keyboard setting from resource databases */
    if (wspGetResource(KEYBOARD_KEYCLICK_RES, getDbAccess, &value)) {
        resKeyclick = (strcmp((char*)value.addr, "True") == 0);
    }
    if (wspGetResource(KEYBOARD_KEYREPEAT_RES, getDbAccess, &value)) {
        resKeyrepeat = (strcmp((char*)value.addr, "True") == 0);
    }

    /* Get the current keyboard setting from the server */
    XGetKeyboardControl(display, &kbState);
    svrKeyclick = (kbState.key_click_percent > 0);
    svrKeyrepeat = (kbState.global_auto_repeat != 0);

    /* Update resource manager db to contain resources matching server state */
    if (resKeyclick != svrKeyclick) {
	wspPutResource(KEYBOARD_KEYCLICK_RES,
		(svrKeyclick ? "True" : "False"), putDbAccess);
        sync = TRUE;
    }
    if (resKeyrepeat != svrKeyrepeat) {
	wspPutResource(KEYBOARD_KEYREPEAT_RES,
		(svrKeyrepeat ? "True" : "False"), putDbAccess);
        sync = TRUE;
    }

    return(sync);
}
