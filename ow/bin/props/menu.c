#pragma ident	"@(#)menu.c	1.18	93/03/10 SMI"

/***************************************************************************
* Includes
***************************************************************************/

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
#include <Xol/Slider.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/Nonexclusi.h>
#include <Xol/CheckBox.h>

#include "props.h"
#ifdef DBMALLOC
#	include <dbmalloc.h>
#endif


/***************************************************************************
* Defines
***************************************************************************/

strconst MENU_DRAG_RIGHT_DISTANCE_RES  = "OpenWindows.DragRightDistance",
	 MENU_SELECT_DISPLAYS_MENU_RES = "OpenWindows.SelectDisplaysMenu",

	 LEFT_POINTER_BUTTON_IS_SELECT  =
			"*menuCategory*selectMousePress.leftButtonIsSelect",
	 RIGHT_POINTER_BUTTON_IS_SELECT	=
	 		"*menuCategory*selectMousePress.rightButtonIsSelect";


/***************************************************************************
* Module Static Data
***************************************************************************/

/*
 * The rest of the widget tree is local to menuCreateCategory(), but
 * menuSyncWithServer() must be able to modify selectMousePress's caption.
 */

Widget	selectMousePress;

typedef struct _SelectMousePressLabels {
    String      leftButtonIsSelect;
    String      rightButtonIsSelect;
} SelectMousePressLabels, *SelectMousePressLabelsPtr;

SelectMousePressLabels     selectMousePressLabels;

static XtResource appResources[] = {
        { "leftButtonIsSelect", "LeftButtonIsSelect",
	  XtRString, sizeof(String),
          XtOffset(SelectMousePressLabelsPtr, leftButtonIsSelect),XtRImmediate,
          "Left Mouse Press:"
        },
        { "rightButtonIsSelect", "RightButtonIsSelect",
	  XtRString, sizeof(String),
          XtOffset(SelectMousePressLabelsPtr,rightButtonIsSelect),XtRImmediate,
          "Right Mouse Press:"
        }
};

/***************************************************************************
* Function Declarations
***************************************************************************/

CategoryInfo	*menuRegisterCategory(void);
static Widget	 menuCreateCategory(Widget);
static Boolean	 menuSyncWithServer(int, int);


/***************************************************************************
* Registration Data & Function
***************************************************************************/

static CategoryInfo info = {
   "menu",
    menuCreateCategory,
    wspCreateChangeBars,
    wspDeleteChangeBars,
    wspBackupSettings,
    wspRestoreSettings,
    wspReadDbSettings,
    wspSaveDbSettings,
    wspNoopRealizeSettings,
    wspNoopInitializeServer,
    menuSyncWithServer,
    wspNoopShowCategory,
    wspNoopHideCategory
};

CategoryInfo*
menuRegisterCategory(void)
{
    return( &info );
}


/***************************************************************************
Menu Category Widget Tree
-------------------------
menuCategory
	dragRightDistance
		control
			slider
			units
	selectMousePress
		exclusive
			button00
			button01

***************************************************************************/


/***************************************************************************
* Private Menu Category Functions
***************************************************************************/

static Boolean
menuSyncWithServer(
    int			getDbAccess,
    int			putDbAccess)
{
    unsigned char	map[256];

    XrmValue		value;

    int			pointerButtonCount;


    pointerButtonCount = XGetPointerMapping(display, map, 256);

    /*
     * Check to see if mouse is mapped left-handed or right-handed.
     * If check below is checking for left-handed and defaults to
     * right-handed.
     */

    if ((map[0] == pointerButtonCount) && (map[pointerButtonCount - 1] == 1)) {
    	XtVaSetValues(
    		selectMousePress,
    		XtNlabel, selectMousePressLabels.rightButtonIsSelect,
    		NULL);
    } else {
    	XtVaSetValues(
    		selectMousePress,
    		XtNlabel, selectMousePressLabels.leftButtonIsSelect,
    		NULL);
    }

    return TRUE;
}


static Widget
menuCreateCategory(
    Widget	 mainControl)
{
    Widget	 menuCategory,
		 dragRightDistance,
		 button,
		 control,
		 exclusive,
		 slider;
		 /* selectMousePress */

    ItemInfo 	*itemInfo;


    /*
     * Create menu control area
     */

    menuCategory = XtVaCreateManagedWidget("menuCategory",
            controlAreaWidgetClass, mainControl,
	    XtNmappedWhenManaged,   FALSE,
            NULL);

    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)menuCategory, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    XtGetApplicationResources(
    		menuCategory, &selectMousePressLabels,
    		appResources, XtNumber(appResources), NULL, 0);


    /*
     * Create "Drag-Right Distance" slider
     */

    dragRightDistance = XtVaCreateManagedWidget("dragRightDistance",
            captionWidgetClass, menuCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)dragRightDistance, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MENU_DRAG_RIGHT_DISTANCE_RES, dNumber,
            NULL, 100, dragRightDistance);

    control = XtVaCreateManagedWidget("control",
            controlAreaWidgetClass, dragRightDistance,
	    NULL);

    slider = XtVaCreateManagedWidget("slider",
            sliderWidgetClass, control,
            XtNuserData,     (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(slider, XtNsliderMoved, wspChangeSettingCb, 
	    (XtPointer)NULL);

    (void) XtVaCreateManagedWidget("units",
	    staticTextWidgetClass, control,
	    NULL);


    /*
     * Create "SELECT Mouse Press" exclusive choice
     * 
     * The value of "Selects Default" is False. The value of "Displays Menu"
     * is True. So, the name of the boolean resource is "SelectDisplaysMenu".
     */

    selectMousePress = XtVaCreateManagedWidget("selectMousePress",
            captionWidgetClass, menuCategory,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)selectMousePress, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    itemInfo = wspNewItemInfo(0, MENU_SELECT_DISPLAYS_MENU_RES, dString,
            "True:False", 0, selectMousePress);

    exclusive = XtVaCreateManagedWidget("exclusive", 
            exclusivesWidgetClass, selectMousePress,
            NULL);

    button = XtVaCreateManagedWidget("button00",
            rectButtonWidgetClass, exclusive,
            XtNuserData,     (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)0);

    button = XtVaCreateManagedWidget("button01",
            rectButtonWidgetClass, exclusive,
            XtNuserData,     (XtArgVal)itemInfo,
            NULL);
    XtAddCallback(button, XtNselect, wspChangeSettingCb, (XtPointer)1);

    return(menuCategory);
}
