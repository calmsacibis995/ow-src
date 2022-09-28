#pragma ident	"@(#)props.c	1.50	93/08/06 SMI"

/* Copyright */


/***************************************************************************
* Includes
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>

#include <Xol/OpenLook.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/FooterPane.h>
#include <Xol/Form.h>
#include <Xol/Notice.h>
#include <Xol/OblongButt.h>
#include <Xol/OlCursors.h>
#include <Xol/PopupWindo.h>
#include <Xol/Pushpin.h>
#include <Xol/RectButton.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/RubberTile.h>
#include <Xol/Form.h>

#include "props.h"
#include "propsP.h"
#include "helpfile.h"

#ifdef DBMALLOC
	#include <dbmalloc.h>
#endif


/**************************************************************************
* Defines
**************************************************************************/

#define HELP_BUFFER_SIZE 	 128

/***************************************************************************
* Global Variables
***************************************************************************/

Display		*display;             /* X connection for use by Xlib calls   */
Screen		*screen;              /* Current screen for various calls     */
Visual		*visual;              /* Visual being used by the application */
XtAppContext	 appContext;	      /* For use by workprocs */

/***************************************************************************
* String Constants
***************************************************************************/

/*
 * File Names
 */

const char	*const OWDEFAULTS_FILE = ".OWdefaults",
		*const XDEFAULTS_FILE = "lib/Xdefaults",
		*const HELP_FILE = "props.info",
		*const HELP_DEFAULT_DIRECTORY = "/usr/lib/help";

/*
 * Error Message Information
 */

typedef struct _PropsErrors {
	String	noAbbrevExclusive;
	String	noAbbrevParent;
	String	noCategoryCreation;
	String	noCategoryInfo;
	String	noCategoryName;
	String	noDefaultDatabase;
	String	noGetResourceValue;
	String  noHelpAvailable;
	String	noMenuPane;
	String	noPutResourceValue;
	String	noResourceManager;
	String	noStringConversion;
	String	noUpdateAbbrev;
	String	noUpdateSetting;
	String	noUpdateXrm;
	String	noUpdateXrmPopup;
	String	unknownWidgetClass;
	String	valueTooLarge;
} PropsErrors, *PropsErrorsPtr;

PropsErrors	propsErrors;

static XtResource appResources[] = {
	{ "noAbbrevExclusive", "NoAbbrevExclusive", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noAbbrevExclusive), XtRImmediate,
	  "Abbreviated button \"%s\" missing exclusive."
	},
	{ "noAbbrevParent", "NoAbbrevParent", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noAbbrevParent), XtRImmediate,
	  "Unable to get parent of abbreviated button \"%s\"."
	},
	{ "noCategoryCreation", "NoCategoryCreation", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noCategoryCreation), XtRImmediate,
	  "Unable to create category."
	},
	{ "noCategoryInfo", "NoCategoryInfo", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noCategoryInfo), XtRImmediate,
	  "Missing category info. Can't change categories."
	},
	{ "noCategoryName", "NoCategoryName", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noCategoryName), XtRImmediate,
	  "Missing category name. Can't change categories."
	},
	{ "noDefaultDatabase", "NoDefaultDatabase", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noDefaultDatabase), XtRImmediate,
	  "Can't find default database \"%s\"\nIs $OPENWINHOME set properly?"
	  " Is OpenWindows installed correctly?"
	},
	{ "noGetResourceValue", "NoGetResourceValue", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noGetResourceValue), XtRImmediate,
	  "Internal error. Cannot retrieve new resource value."
	},
	{ "noHelpAvailable", "NoHelpAvailable", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noHelpAvailable), XtRImmediate,
	  "There is no help available for this object."
	},
	{ "noMenuPane", "NoMenuPane", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noMenuPane), XtRImmediate,
	  "No menu pane for abbreviated menu button \"%s\"."
	},
	{ "noPutResourceValue", "NoPutResourceValue", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noPutResourceValue), XtRImmediate,
	  "Internal error. Unable to store new resource value."
	},
	{ "noResourceManager", "NoResourceManager", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noResourceManager), XtRImmediate,
	  "Can't get RESOURCE_MANAGER string. XGetWindowProperty() returned %d."
	},
	{ "noStringConversion", "NoStringConversion", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noStringConversion), XtRImmediate,
	  "Bad string value \"%s\" for \"%s\"."
	},
	{ "noUpdateAbbrev", "NoUpdateAbbrev", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noUpdateAbbrev), XtRImmediate,
	  "Internal error. Unable to update abbreviated menu button."
	},
	{ "noUpdateSetting", "NoUpdateSetting", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noUpdateSetting), XtRImmediate,
	  "Internal error. Unable to update setting."
	},
	{ "noUpdateXrm", "NoUpdateXrm", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noUpdateXrm), XtRImmediate,
	  "Unable to update resource manager."
	},
	{ "noUpdateXrmPopup", "NoUpdateXrmPopup", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, noUpdateXrmPopup), XtRImmediate,
	  "Unable to update resource manager.\nPlease try again."
	},
	{ "unknownWidgetClass", "UnknownWidgetClass", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, unknownWidgetClass), XtRImmediate,
	  "Attempting to set state of widget \"%s\" of unknown class %d."
	},
	{ "valueTooLarge", "ValueTooLarge", XtRString, sizeof(String),
	  XtOffset(PropsErrorsPtr, valueTooLarge), XtRImmediate,
	  "Value %d too large for exclusive choice. Choice not set."
	}
};


/***************************************************************************
* Static Global Variables
***************************************************************************/

static char		*userDbFile;

static XrmDatabase	 rwinDb,
			 userDb,
			 dfltDb;

static Widget		 topLevel,	/* Busy state, realizing widget tree */
			 propsMain;	/* For notices & category (un)mapping */

       Widget		 propsPopup = NULL; /* Preview window sizing, errors */

static PropsCatInfo	*currentCategory;


/***************************************************************************
* Public Supporting Functions
***************************************************************************/

/*
 * wspChangeBusyState
 *
 * Set the busy state for the top level widget and the cursor.
 */

void
wspChangeBusyState(
    Boolean     busyState)
{
    Cursor      cursor;
    Window      window;

    /* Update busy pattern on window */

    XtVaSetValues(propsPopup,
            XtNbusy, busyState,
            NULL);

    /* Update busy cursor */

    window = XtWindow(propsPopup);

    if (window != NULL) {
        if (busyState) {
            cursor = OlGetBusyCursor(propsPopup);
            XDefineCursor(display, window, cursor);
        } else {
            XUndefineCursor(display, window);
        }
    }    
}


/*
 * wspChangeSettingCb
 *
 * Set the change bar in the widget's associated caption. Set the current
 * value to the value passed into this function as client data. If the widget 
 * is a slider, set the current value to the slider's value.
 */

void
wspChangeSettingCb(
    Widget	 widget,
    XtPointer	 clientData,
    XtPointer	 callData)
{
    int           value = (int)clientData;
    ItemInfo     *itemInfo;
    WidgetClass   widgetClass;
    OlDefine      changeBar;

    wspClearFooter();

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }
    
    XtVaGetValues(itemInfo->caption,
            XtNchangeBar, (XtArgVal)&changeBar,
            NULL);
    if (changeBar != OL_NORMAL) {
        XtVaSetValues(itemInfo->caption,
                XtNchangeBar, (XtArgVal)OL_NORMAL,
                NULL);
    }
    
    widgetClass = XtClass(widget);
    if (widgetClass == sliderWidgetClass) {
        OlSliderVerify *sliderInfo = (OlSliderVerify*)callData;
        
        itemInfo->currVal = sliderInfo->new_location;
    } else {
        itemInfo->currVal = value;
    }
}


/*
 * wspClearFooter
 *
 * Removes the message printed in the footer of the props popup window.
 * This should be called whenever the user performs an action in props.
 * The assumption is that by performing another action, the user has read
 * the message printed in the footer and is ready to continue. The toolkit
 * should probably handle this. Since it doesn't we use wspClearFooter().
 */

void
wspClearFooter(
    void)
{
    Widget	propsError;

    propsError = XtNameToWidget(propsPopup, "*propsError");
    if (propsError != NULL) {
	XtVaSetValues(propsError, XtNstring, (XtArgVal)NULL, NULL);
    }
}


/*
 * wspErrorFooter
 *
 * Displays an error in the footer of the props popup window.
 * Use wspClearFooter() to remove the message. The message should be
 * removed as soon as the user performs another action within the props
 * application.
 */

void
wspErrorFooter(
    const char *messageFormat,
    ...)
{
    va_list	args;
    char	errorBuffer[160];
    Widget	propsError = NULL;

    va_start(args, messageFormat);
    
    if (messageFormat == NULL) {
	return;
    }

    if (propsPopup != NULL) {
  	propsError = XtNameToWidget(propsPopup, "*propsError");
    }

    if (propsError != NULL) {
	(void) vsprintf(errorBuffer, messageFormat, args);
	XtVaSetValues(propsError, XtNstring, (XtArgVal)errorBuffer, NULL);

    } else { /* There is no footer in which to put the message */

	wspErrorStandard("wspErrorFooter()", messageFormat, args);
    }
}

/*
 * wspErrorPopupConfirm
 *
 * Creates a popup notice with the specified message displayed in it and a
 * single confirmation button.
 *
 * Use when:
 *
 *      - There is an error which requires the user to take an action.
 *      - There is a catastrophic error which requires exiting the program.
 */

void
wspErrorPopupConfirm(
    const char *messageFormat,
    ...)
{
    va_list	args;
    char 	errorBuffer[1024];
    Widget	errorNotice,
		staticText,
		controlArea,
		confirmButton;

    va_start(args, messageFormat);

    if (messageFormat == NULL) {
	return;
    }

    if (propsPopup != NULL) {

	/* Create notice */

	errorNotice = XtVaCreatePopupShell("errorNotice",
	    noticeShellWidgetClass, propsPopup,
	    NULL);

	/* Place the text */

	(void) vsprintf(errorBuffer, messageFormat, args);
	XtVaGetValues(errorNotice, XtNtextArea, (XtArgVal)&staticText, NULL);
	XtVaSetValues(staticText, XtNstring, (XtArgVal)errorBuffer, NULL);

	/* Create the button */

	XtVaGetValues(errorNotice, XtNcontrolArea, (XtArgVal)&controlArea,NULL);
        confirmButton = XtVaCreateManagedWidget("confirmButton",
	    oblongButtonWidgetClass, controlArea,
	    NULL);

	/* Pop up the notice */

	XtPopup(errorNotice, XtGrabExclusive);

    } else { /* There's no window to use for the popup notice */

	wspErrorStandard("wspErrorPopupConfirm()", messageFormat, args);
    }
}


/*
 * wspErrorPopupChoice
 *
 * Creates a popup notice with the specified message displayed in it and
 * a pair of buttons. Button 0 means yes and button 1 means no. The function
 * attaches the given callbacks to the buttons passing the button value as
 * described here.
 *
 * Use when:
 *
 *      - There is an error which requires the user to make a choice.
 */

void
wspErrorPopupChoice(
    void       (*choiceCallback)(Widget, XtPointer, XtPointer),
    const char *messageFormat,
    ...)
{
    va_list	args;
    char	errorBuffer[160];
    Widget	errorNotice,
		staticText,
		controlArea,
		yesButton,
		noButton;

    va_start(args, messageFormat);

    if (messageFormat == NULL) {
	return;
    }

    if (propsPopup != NULL) {

	/* Create notice */

	errorNotice = XtVaCreatePopupShell("errorNotice",
	    noticeShellWidgetClass, propsPopup,
	    NULL);

	/* Place the text */

	(void) vsprintf(errorBuffer, messageFormat, args);
	XtVaGetValues(errorNotice, XtNtextArea, (XtArgVal)&staticText, NULL);
	XtVaSetValues(staticText, XtNstring, (XtArgVal)messageFormat, NULL);

	/* Create the buttons */

	XtVaGetValues(errorNotice, XtNcontrolArea, (XtArgVal)&controlArea,NULL);
       	yesButton = XtVaCreateManagedWidget("yesButton",
	    oblongButtonWidgetClass, controlArea,
	    NULL);
	XtAddCallback(yesButton, XtNselect, choiceCallback, (XtPointer)1);

       	noButton = XtVaCreateManagedWidget("noButton",
	    oblongButtonWidgetClass, controlArea,
	    NULL);
	XtAddCallback(noButton, XtNselect, choiceCallback, (XtPointer)0);

	/* Pop up the notice */

	XtPopup(errorNotice, XtGrabExclusive);

    } else { /* There's no window to use for the popup notice */

	wspErrorStandard("wspErrorPopupChoice()", messageFormat, args);
    }
}


/*
 * wspErrorStandard
 *
 * Create the string "props: caller(): error message" and print to stderr
 */

void
wspErrorStandard(
    const char *caller,
    ...)
{
    va_list	args;
    char       *messageFormat;
    char	errorBuffer[160];
    
    va_start(args, caller);
    messageFormat = va_arg(args, char *);

    if (messageFormat == NULL) {
	errorBuffer[0] = NULL;
    } else {
    	(void) vsprintf(errorBuffer, messageFormat, args);
    }
    (void) fprintf(stderr, "props: %s: %s\n", caller, errorBuffer);
}


/*
 * wspGetOpenwinhome
 *
 * Find the value of the environment variable OPENWINHOME. If OPENWINHOME
 * is not set, this function returns a pointer to the string "/usr/openwin".
 */

const char *
wspGetOpenwinhome(
    void)
{
    static const char	*openwinhome = NULL;

    if (openwinhome == NULL) {
        openwinhome = getenv("OPENWINHOME");
        if (openwinhome == NULL) {
            openwinhome = "/usr/openwin";
	}
    }

    return openwinhome;
}


/*
 * wspGetLocale returns a char* which is the name of the current locale
 */

static const char *localeResources[] = {
    "OpenWindows.DisplayLang",
    "OpenWindows.BasicLocale",
    "OpenWindows.displayLang",
    "OpenWindows.basicLocale",
    NULL
};

const char *
wspGetLocale(void)
{
    XrmValue     value;
    const char  *locale = NULL;
    const char	*localeResource;
    int		 index = 0;


    localeResource = localeResources[index];

    while (   (locale         == NULL)  /* Haven't found a valid locale. */
	   && (localeResource != NULL)) /* Haven't run out of res. to check. */
    {
	if (wspGetResource(localeResource, ACCESS_ALL_DB, &value)) {
	    locale = value.addr;
	} else {
	    localeResource = localeResources[++index];
	}
    }

    if (locale == NULL) {
	locale = getenv("LANG");
    }

    if (locale == NULL) {
	locale = "C";
    }

    return XtNewString(locale);
}


/*
 * wspGetResource
 *
 * Query databases specified by dbAccess. The precidence order of the
 * databases from highest to lowest is: root window, user, default.
 * The value of the resource specification is set for the first successful
 * query and TRUE is returned. If the resource specification is not found
 * in any database, FALSE is returned.
 */

Boolean
wspGetResource(
    const char	*name,
    int		 dbAccess,
    XrmValue	*value)
{
    char	*type;

    if ((name == NULL) || (value == NULL)) {
	wspErrorFooter(propsErrors.noGetResourceValue, NULL);

        return FALSE;
    }
    if (dbAccess & ACCESS_RWIN_DB)
        if (XrmGetResource(rwinDb, name, name, &type, value))
            if (value->size > 1) /* value.addr is null-terminated string */
                return TRUE;   /* size <= 1 means only null or no null */
    if (dbAccess & ACCESS_USER_DB)
        if (XrmGetResource(userDb, name, name, &type, value))
            if (value->size > 1)
                return TRUE;
    if (dbAccess & ACCESS_DFLT_DB)
        if (XrmGetResource(dfltDb, name, name, &type, value))
            if (value->size > 1)
                return TRUE;

    return FALSE;
}

/* ARGSUSED */
void
wspHelp(
        OlDefine        idType,
        XtPointer       id,
        Cardinal        srcX,
        Cardinal        srcY,
        OlDefine        *srcType,
        XtPointer       *src)
{
        char            *key;
        FILE            *fp;
        char            *moreHelp;
        char            *helpLine;
        char            *helpBuf = NULL;
        int             pos, len;

        *srcType = OL_STRING_SOURCE;

        key = XtName(id);
 
        if ((fp = HelpFindFile(HELP_FILE,NULL)) == NULL) {
                *src = XtNewString(propsErrors.noHelpAvailable);
                return;
        }
 
        if (!HelpSearchFile(fp, key, &moreHelp)) {
                fclose(fp);
                *src = XtNewString(propsErrors.noHelpAvailable);
                return;
        }
 
        for (helpLine = HelpGetText(fp), pos = 0;
             helpLine != NULL;
             helpLine = HelpGetText(fp)) {
 
                len = strlen(helpLine);

                helpBuf = XtRealloc(helpBuf, pos + len + 1);
                strcpy(&helpBuf[pos], helpLine);

                pos += len;                      
        }

        fclose(fp);
         
        *src = helpBuf;
}
 
/*
 * wspNewItemInfo
 *
 * Allocate a new ItemInfo structure and fill in the fields with the
 * parameter values. Each setting in a category which effects a resource
 * gets one of these. The memory allocated here remains in use for the 
 * life of the program.
 */

ItemInfo*
wspNewItemInfo(
    int		 id,
    const char	*name,
    Deftype	 type,
    const char	*domain,
    int		 value,
    Widget	 caption)
{
    ItemInfo	*newItem;

    newItem = XtNew(ItemInfo);

    if (newItem) {
        newItem->name    = name;
        newItem->id      = id;
        newItem->type    = type;
        newItem->domain  = domain;
        newItem->currVal = value;
        newItem->backVal = value;
        newItem->caption = caption;
    }
    
    return newItem;
}


/*
 * wspPutResource
 *
 * Put the resource specification:value pair into each specified database.
 */

void
wspPutResource(
    const char	*name,
    const char	*value,
    int		 dbAccess)
{
    if ((name == NULL) || (value == NULL)) {
	wspErrorFooter(propsErrors.noPutResourceValue, NULL);

        return;
    }
    
    if (dbAccess & ACCESS_RWIN_DB)
        XrmPutStringResource(&rwinDb, name, value);
    if (dbAccess & ACCESS_USER_DB)
        XrmPutStringResource(&userDb, name, value);
}


/*
 * wspSettableWidgetClass
 *
 * Returns TRUE if the widget is of a class props knows how to set.
 */

Boolean
wspSettableWidgetClass(WidgetClass widgetClass)
{
    if (widgetClass == rectButtonWidgetClass       ||
        widgetClass == textFieldWidgetClass        ||
        widgetClass == sliderWidgetClass           ||
        widgetClass == abbrevMenuButtonWidgetClass ||
        widgetClass == checkBoxWidgetClass)
    {
        return TRUE;
    }

    return FALSE;
}


/*
 * wspSetWidgetState
 *
 * Sets the state of the widget based on the value. This works for 
 * widgets which are settable as determined by wspSettableWidgetClass().
 */

void
wspSetWidgetState(
    Widget	widget,
    int		value)
{
    WidgetClass	widgetClass;

    widgetClass = XtClass(widget);
    if (widgetClass == rectButtonWidgetClass) {
        Widget		parent;
        Cardinal	numChildren;
        WidgetList	children;

        parent = XtParent(widget);
        XtVaGetValues(parent,
                XtNnumChildren, (XtArgVal)&numChildren,
                XtNchildren,    (XtArgVal)&children,
                NULL);
        if (value < (int)numChildren) {
            XtVaSetValues(children[value],
                    XtNset, (XtArgVal)TRUE,
                    NULL);
        } else {
	    wspErrorFooter(propsErrors.valueTooLarge, value, NULL);
        }

        return;
    }
    
    /* REMIND: This should be a separate function */
    if (widgetClass == abbrevMenuButtonWidgetClass) {
	Widget		menuPane,
			exclusive,
			parent;
	Cardinal	numChildren;
	WidgetList	children;
	String		label;


	/* Get the menu pane from the abbreviated menu button */

	XtVaGetValues(widget, XtNmenuPane, (XtArgVal)&menuPane, NULL);
	if (menuPane == NULL) {
	    wspErrorStandard("wspSetWidgetState()", propsErrors.noMenuPane,
		XtName(widget), NULL);
	    wspErrorFooter(propsErrors.noUpdateAbbrev, NULL);

	    return;
	}


	/* Get the exclusive choice in the menu pane */

	XtVaGetValues(menuPane,
		XtNchildren, (XtArgVal)&children,
		XtNnumChildren, (XtArgVal)&numChildren,
		NULL);
	if (numChildren != 1) {
	    wspErrorStandard("wspSetWidgetState()",
		propsErrors.noAbbrevExclusive, XtName(widget), NULL);
	    wspErrorFooter(propsErrors.noUpdateAbbrev, NULL);

	    return;

	} else {
	    exclusive = children[0];
	}


	/* Get the buttons in the exclusive choice & set the correct button */

	XtVaGetValues(exclusive,
		XtNchildren,    (XtArgVal)&children,
		XtNnumChildren, (XtArgVal)&numChildren,
		NULL);
	if (value < (int)numChildren) {
	    XtVaSetValues(children[value], XtNset,   (XtArgVal)TRUE,   NULL);
	    XtVaGetValues(children[value], XtNlabel, (XtArgVal)&label, NULL);
	} else {
	    wspErrorFooter(propsErrors.valueTooLarge, value, NULL);

            return;
	}


	/* Get the menu button preview and update it */

	parent = XtParent(widget);
	if (parent != NULL) {
	    XtVaGetValues(parent,
		XtNchildren,    (XtArgVal)&children,
		XtNnumChildren, (XtArgVal)&numChildren,
		NULL);
	    XtVaSetValues(children[1], XtNstring, (XtArgVal)label, NULL);
	} else {
	    wspErrorStandard("wspSetWidgetState()", propsErrors.noAbbrevParent,
		XtName(widget), NULL);
	    wspErrorFooter(propsErrors.noUpdateAbbrev, NULL);

            return;
	}

	return;
    }
    
    if (widgetClass == textFieldWidgetClass) {
        char	numstr[20];

        (void) sprintf(numstr, "%d", value);
        XtVaSetValues(widget,
                XtNstring, (XtArgVal)numstr,
                NULL);
        return;
    }
    
    if (widgetClass == sliderWidgetClass) {
        XtVaSetValues(widget,
                XtNsliderValue, (XtArgVal)value,
                NULL);
        return;
    }
    
    if (widgetClass == checkBoxWidgetClass) {
        XtVaSetValues(widget,
                XtNset, (XtArgVal)value,
                NULL);
        return;
    }
    
    wspErrorStandard("wspSetWidgetState()", propsErrors.unknownWidgetClass,
	XtName(widget), widgetClass, NULL);
    wspErrorFooter(propsErrors.noUpdateSetting, NULL);

    return;
}


/*
 * wspUpdateResourceManager
 *
 * Write the new resource database onto the root window property
 * XA_RESOURCE_MANAGER.
 */

Boolean
wspUpdateResourceManager(
    void)
{
    unsigned char	*buffer;
    FILE		*filePtr;
    struct stat		 fileStatus;
 
    XrmPutFileDatabase(rwinDb, RWIN_DB_FILE);
    if (stat(RWIN_DB_FILE, &fileStatus)) {
        (void) unlink(RWIN_DB_FILE);

        return FALSE;
    }

    buffer = (unsigned char *)XtMalloc((Cardinal)fileStatus.st_size);
    if (buffer == NULL) {
        (void) unlink(RWIN_DB_FILE);
        return FALSE;
    }

    filePtr = fopen(RWIN_DB_FILE, "r");
    if (filePtr == NULL) {
        (void) unlink(RWIN_DB_FILE);
        return FALSE;
    }
    
    (void) unlink(RWIN_DB_FILE);
    if (  fread(buffer, 1, (unsigned)fileStatus.st_size, filePtr)
        < (unsigned)(fileStatus.st_size))
    {
        return FALSE;
    }
    
    XChangeProperty(display, RootWindow(display,0), XA_RESOURCE_MANAGER,
                    XA_STRING, 8, PropModeReplace, buffer, fileStatus.st_size);
    XSync(display, 0);
 
    if (filePtr != NULL) {
        (void) fclose(filePtr);
    }
    
    if (buffer != NULL) {
        XtFree((char *)buffer);
    }
    
    return TRUE;
}


/***************************************************************************
* Public Generic Category Functions
***************************************************************************/

/*
 * wspBackupSettings
 *
 * Update the backup values with the current values. If preserve is TRUE and
 * an item's change bar is set, do not set the backup value. This allows a
 * user to make a change to item 1 in category A without applying it, move
 * to category B, have the database change, the panels update accordingly,
 * and return to category A, and still be able to reset to the previous value
 * of item 1.
 */

void
wspBackupSettings(
    Widget	widget,
    Boolean	preserve)
{
    wspUpdateSettableWidgets(propsBackupSetting, widget, NULL, preserve);
}


/*
 * wspCreateChangeBars
 *
 * Set the change bars for items where the backup value is different from
 * the current value.
 */

void
wspCreateChangeBars(Widget widget)
{
    wspUpdateSettableWidgets(propsCreateChangeBar, widget, NULL, NULL);
}


/*
 * wspDeleteChangeBars
 *
 * Turn off the change bars for each item in the category.
 */

void
wspDeleteChangeBars(Widget widget)
{
    wspUpdateSettableWidgets(propsDeleteChangeBar, widget, NULL, NULL);
}


/*
 * wspNoopHideCategory
 *
 * Used by categories which don't need to do anything upon being hidden
 * after they were used.
 */

/* ARGSUSED */
void
wspNoopHideCategory(Widget widget)
{}


/*
 * wspNoopInitializeServer
 *
 * Used by categories which don't need to do anything to make their
 * resource settings take effect when the window system starts up.
 */

/* ARGSUSED */
void
wspNoopInitializeServer(int dbAccess)
{}


/*
 * wspNoopRealizeSetting
 *
 * Used by categories which don't need to do anything to make their
 * resource settings take effect when apply is pressed.
 */

/* ARGSUSED */
Boolean
wspNoopRealizeSettings(Widget widget)
{
    return TRUE;
}


/*
 * wspNoopShowCategory
 *
 * Used by categories which don't need to do anything upon being shown.
 * This function is called by the foundation each time a category is shown.
 */

/* ARGSUSED */
void
wspNoopShowCategory(Widget widget)
{}


/*
 * wspNoopSyncWithServer
 *
 * Used by categories which don't need to do anything to ensure their
 * resource settings are in sync with the window server's state.
 */

/* ARGSUSED */
Boolean
wspNoopSyncWithServer(
    int	getDbAccess,
    int	putDbAccess)
{
    return FALSE;
}


/*
 * wspReadDbSettings
 *
 * Update each item to reflect the current database values. If preserve is
 * TRUE and an item's change bar is set, the databases are not queried and
 * item's state is not changed. This allows a user to change item 1 in
 * category A, move to category B, have the databases change, return to
 * category A, and still see the change made to item 1.
 */

void
wspReadDbSettings(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    wspUpdateSettableWidgets(propsReadDbSetting, widget, dbAccess,
            preserve);
}


/*
 * wspRestoreSettings
 *
 * Set the current value to the backup value for each item with a change
 * bar that is set.
 */

void
wspRestoreSettings(Widget widget)
{
    wspUpdateSettableWidgets(propsRestoreSetting, widget, NULL, NULL);
}


/*
 * wspSaveDbSettings
 *
 * For each item for which the change bar is set, convert the current
 * value of the item into a string and put the resource name and this
 * string value into the database.
 */ 

void
wspSaveDbSettings(
    Widget	widget,
    int		dbAccess)
{
    wspUpdateSettableWidgets(propsSaveDbSetting, widget, dbAccess, NULL);
}


/*
 * wspUpdateSettableWidgets
 *
 * Recursively traverse the subtree of the widget, updating the state of
 * settable widgets by applying the given process. This is used to update
 * widgets after reading the database or restoring backup values. Settable
 * widgets are defined as those widgets which cause wspSettableWidget()
 * to return TRUE.
 */

void
wspUpdateSettableWidgets(
    void      (*process)(Widget, int, Boolean),
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    WidgetClass	widgetClass;

    widgetClass = XtClass(widget);
    if (wspSettableWidgetClass(widgetClass)) {
        (*process)(widget, dbAccess, preserve);
        return;
    }
    
    if (XtIsSubclass(widget, compositeWidgetClass)) {
        Cardinal	numChildren,
        		ii;
        WidgetList	children;

        XtVaGetValues(widget,
                XtNnumChildren, (XtArgVal)&numChildren,
                XtNchildren,    (XtArgVal)&children,
                NULL);
        for (ii = 0; ii < numChildren; ii++) {
            wspUpdateSettableWidgets(process, children[ii], dbAccess,
                    preserve);
        }
    }
}

 
/***************************************************************************
* Private Generic Category Supporting Functions
***************************************************************************/

/*
 * propsBackupSetting
 *
 * Set the widget's backup value to the current value. If preserve is TRUE
 * and the widget's change bar is set, do not set the backup value. This
 * allows a user to make a change to item 1 in category A without applying
 * it, move to category B, have the database change, the panels update
 * accordingly, and return to category A, and still be able to reset to
 * the previous value of item 1.
 */

/* ARGSUSED */
static void
propsBackupSetting(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    ItemInfo   *itemInfo;

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }
    
    if (preserve) { /* preserve current backup val if change bar on */
        OlDefine	changeBar;

        XtVaGetValues(itemInfo->caption,
                XtNchangeBar, (XtArgVal)&changeBar, NULL);
        if (changeBar != OL_NONE) {
            return;
        }
    }
    itemInfo->backVal = itemInfo->currVal;
    XtVaSetValues(widget, XtNuserData, (XtArgVal)itemInfo, NULL);
}


/*
 * propsCreateChangeBar
 * 
 * Turn on the change bar for the widget if the current value is different
 * from the backup value.
 */

/* ARGSUSED */
static void
propsCreateChangeBar(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    ItemInfo   *itemInfo;

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }
    if (itemInfo->currVal != itemInfo->backVal) {
        OlDefine	changeBar;

        XtVaGetValues(itemInfo->caption,
                XtNchangeBar, (XtArgVal)&changeBar,
                NULL);
        if (changeBar != OL_NORMAL) {	/* If change bar is not on */
            XtVaSetValues(itemInfo->caption,
                    XtNchangeBar, (XtArgVal)OL_NORMAL,
                    NULL);
        }
    }
}


/*
 * propsDeleteChangeBar
 *
 * Turn off the change bar for the widget.
 */

/* ARGSUSED */
static void
propsDeleteChangeBar(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    ItemInfo   *itemInfo;
    
    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }
    XtVaSetValues(itemInfo->caption, XtNchangeBar, (XtArgVal)OL_NONE, NULL);
}


/*
 * propsReadDbSetting
 *
 * Query the resource databases for the widget's resource name given in
 * itemInfo. Apply the resulting value to the widget accordingly. If
 * preserve is TRUE and an item's change bar is set, the databases are
 * not queried and the widget's state is not changed.
 */

static void
propsReadDbSetting(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    ItemInfo   *itemInfo;

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }
    
    if (preserve) { /* preserve value of item with change bar */
        OlDefine	changeBar;

        XtVaGetValues(itemInfo->caption,
                XtNchangeBar, (XtArgVal)&changeBar,
                NULL);
        if (changeBar != OL_NONE) {
            return;
        }
    }
    
    /*lint -e787 */
    switch(itemInfo->type) {
        XrmValue	value;

    case dNumber: {
        int	intVal;

        if (wspGetResource(itemInfo->name, dbAccess, &value)) {
            intVal = atoi((char*)value.addr);
            itemInfo->currVal = intVal;
            XtVaSetValues(widget, XtNuserData, (XtArgVal)itemInfo, NULL);
        } else { /* Use hard-coded value from startup if resource not found */
            intVal = itemInfo->currVal;
        }
        wspSetWidgetState(widget, intVal);
        
    } break;
    
    case dBoolean: {
        int	boolVal;
        
        if (wspGetResource(itemInfo->name, dbAccess, &value)) {
            boolVal = !strcmp(value.addr, "True") ||
                      !strcmp(value.addr, "true");
            itemInfo->currVal = boolVal;
            XtVaSetValues(widget, XtNuserData, (XtArgVal)itemInfo, NULL);
        } else { /* Use hard-coded value from startup if resource not found */
            boolVal = itemInfo->currVal;
        }
        if (   itemInfo->domain
            && (   !strcmp(itemInfo->domain, "1:0")
                || !strcmp(itemInfo->domain, "True:False")))
        {
            boolVal = !boolVal;
        }
        wspSetWidgetState(widget, boolVal);
    
    } break;
    
    case dString: {
        int	ii;
        
        if (wspGetResource(itemInfo->name, dbAccess, &value)) {
            char	*domain,
                 	*string;

            domain = XtNewString(itemInfo->domain);
            string = strtok(domain, ":");
            for (ii = 0;
                 (string != NULL) && strcmp(string,(char*)value.addr);
                 ii++)
            {
                string = strtok(NULL, ":");
            }
            if (string != NULL) {
                itemInfo->currVal = ii;
                XtVaSetValues(widget, XtNuserData, (XtArgVal)itemInfo, NULL);
            } else {
		wspErrorFooter(propsErrors.noStringConversion,
		    value.addr, itemInfo->name, NULL);

                ii = itemInfo->currVal;
            }
            XtFree(domain);
        } else { /* Use hard-coded value from startup if resource not found */
            ii = itemInfo->currVal;
        }
        wspSetWidgetState(widget, ii);
    } break;
    
    } /* switch */
    /*lint +e787 */
}


/*
 * propsRestoreSetting
 *
 * If the widget's change bar is set, set the current value to the backup value.
 * Update the state of the widget accordingly.
 */

/* ARGSUSED */
static void
propsRestoreSetting(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    OlDefine	changeBar;
    ItemInfo   *itemInfo;

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }
    XtVaGetValues(itemInfo->caption,
            XtNchangeBar, (XtArgVal)&changeBar,
            NULL);
    if (changeBar == OL_NONE) {
        return;
    } else {
        itemInfo->currVal = itemInfo->backVal;
        XtVaSetValues(itemInfo->caption,
                XtNuserData, (XtArgVal)itemInfo,
                NULL);
    }
    if (   (itemInfo->type == dBoolean)
        &&  itemInfo->domain
        && (   !strcmp(itemInfo->domain, "1:0")
            || !strcmp(itemInfo->domain, "True:False")))
    {
        wspSetWidgetState(widget, !itemInfo->currVal);
    } else {
        wspSetWidgetState(widget,  itemInfo->currVal);
    }
}


/*
 * propsSaveDbSetting
 *
 * If the widget's change bar is set, convert the current value of the
 * widget into a string and put the resource name and this string value
 * into the database.
 */
 
/* ARGSUSED */
static void
propsSaveDbSetting(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    OlDefine	changeBar;
    ItemInfo   *itemInfo;

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }
    
    XtVaGetValues(itemInfo->caption,
            XtNchangeBar, (XtArgVal)&changeBar,
            NULL);
    if (changeBar == OL_NORMAL) {
        const char	*string = NULL;
	      char	*domain = NULL;
              char	 numstr[20];

        switch(itemInfo->type) {
        case dNumber:
            (void) sprintf(numstr, "%d", itemInfo->currVal);
            string = numstr;
            break;
            
        case dBoolean:
            if (itemInfo->currVal) {
                string = "True";
            } else {
                string = "False";
            }
            break;
            
        case dString: {
            int	ii;

            domain = XtNewString(itemInfo->domain);
            string = strtok(domain, ":");
            for (ii = 0; (ii != itemInfo->currVal) && (string != NULL); ii++)
                string = strtok(NULL, ":");
            }
            break;
            
        case dPrivate:
	    return;

        }
        
        wspPutResource(itemInfo->name, string, dbAccess);
        if (domain != NULL) {
            XtFree(domain);
        }
    }	/* else no update is needed */
}

 
/***************************************************************************
* Private Initialization & Shutdown Functions
***************************************************************************/

/*
 *  Props Widget Tree
 *  -----------------
 *
 * topLevel
 *     propsPopup
 *         propsUpper (controlArea)
 *             topRow (form)
 *	           categoryChoice (caption)
 *		       control
 *		           abbrevMenuButton
 *			   preview
 *			   menuPane
 *			       exclusive
 *			           button (named after category)
 *				   .  .  .
 *				   .  .  .
 *				   .  .  .
 *			           button (named after category)
 *                 standButton
 *             propsBorder      (controlArea)
 *                 propsMain    (bulletinBoard)      
 *                     .		  .		    .
 *                     .		  .		    .
 *		       [ Individual Categories Plug In Here ]
 *                     .		  .		    .
 *                     .		  .		    .
 *                 propsButtons (rubberTile)
 *		       space    (staticText)
 *                     buttons  (controlArea)
 *			   applyButton
 *			   resetButton
 *		       space    (staticText)
 *         propsFooter
 *             propsError	(staticText)
 */

void
main(
    int			 argc,
    char		*argv[])
{
    Widget		 propsFooter,
			 propsBorder,
			 categoryFirst,
			 categoryChoice,
			 exclusive,
			 topRow,
			 control,
			 abbrevMenuButton,
			 button,
			 menuPane,
			 standButton;
			 
    PropsCatInfo	*categories,
    			*categoryInfo;
    
    int			 ii;
    
    PropsMode		 propsMode;
    
    char		*resourceString,
    			*dfltDbFile;
    			
    const char		*owHome,
    			*userHome;
    			

    /*
     * If props was started to set certain server values upon window system
     * startup, just open a connection. Otherwise, initialize OLIT & create
     * the base popup window.
     */

    if ((argc == 2) && !strcmp(argv[1], "-init")) {
        propsMode = ModeSystemInitialize;
        display    = XOpenDisplay(NULL);
	if (display == NULL) {
	    	wspErrorStandard("main()", "Can't connect to X server.", NULL);
		exit(1);
	}
    } else {
        propsMode = ModeRun;

        OlToolkitInitialize((XtPointer)NULL);
	OlSetDefaultTextFormat(OL_MB_STR_REP);
	XtToolkitInitialize();

	appContext = XtCreateApplicationContext();

	/*
	 * The following error messages should be localized but can't be
	 * using the app-defaults method currently employed.
     	 */

	if (appContext == NULL) {
	    	wspErrorStandard("main()", "Can't create application context.",
			NULL);
		exit(1);
	}

	display = XtOpenDisplay(appContext, (String)NULL, "props", "Props",
		(XrmOptionDescList)NULL, (Cardinal)0,
		(int *)&argc, (String *)argv);
		
	if (display == NULL) {
	    	wspErrorStandard("main()", "Can't connect to X server.", NULL);
		exit(1);
	}
		
	topLevel = XtAppCreateShell("props", "Props",
		applicationShellWidgetClass, display,
		(ArgList)NULL, (Cardinal)0);

	if (topLevel == NULL) {
	    	wspErrorStandard("main()", "Can't create application shell.",
			NULL);
		exit(1);
	}
		
	OlAddCallback(topLevel, (String)XtNwmProtocol, propsExit, NULL);

        screen  = XtScreen(topLevel);
        visual  = OlVisualOfObject(topLevel);

        propsPopup = XtVaCreateManagedWidget("propsPopup",
		controlAreaWidgetClass, topLevel,
		NULL);
    
	/* Register generic help describing workspace props */

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)propsPopup, NULL,
		OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

	/* Get error message text. */

        XtGetApplicationResources(propsPopup, &propsErrors, appResources,
		XtNumber(appResources), NULL, 0);
    }

    /* Load resource databases into memory. */

    XrmInitialize();

    resourceString = propsGetRootWindowProperty();
    if (resourceString != NULL) {
        rwinDb = XrmGetStringDatabase(resourceString);
        XFree(resourceString);
    }

    userHome = getenv("HOME");
    if (userHome == NULL) {
        userHome = ".";
    }

    /* This memory should not be freed */

    userDbFile = XtMalloc(strlen(userHome) + strlen(OWDEFAULTS_FILE) + 2);
    (void) sprintf(userDbFile, "%s/%s", userHome, OWDEFAULTS_FILE);

    owHome = wspGetOpenwinhome();

    /* This memory should not be freed */

    dfltDbFile = XtMalloc(strlen(owHome) + strlen(XDEFAULTS_FILE) +2 );
    (void) sprintf(dfltDbFile, "%s/%s", owHome, XDEFAULTS_FILE);

    /* If the user db is NULL, it will be created when a resource is set. */

    userDb = XrmGetFileDatabase(userDbFile);
    
    /* If the default db is NULL, there is something seriously wrong. */

    dfltDb = XrmGetFileDatabase(dfltDbFile);
    
    if (dfltDb == NULL) { /* No popup window available at this point */
        wspErrorStandard("main()", propsErrors.noDefaultDatabase, dfltDbFile,
	    NULL);
    }


    /*
     * Get the information from each category
     */
     
    categories = propsBuildCategoryList();

    /* If props was started to set server values, do so and exit. */

    if (propsMode == ModeSystemInitialize) {
        propsSystemInitialize(categories);
	XFlush(display);
        exit(0);
    }


    /*
     * Register a callback so props can update its internal values when
     * the resource manager is updated.
     */
     
    OlRegisterDynamicCallback(propsDynamicUpdate, (XtPointer)categories);

    /* Create category abbreviated menu widget in top row of widgets. */

    topRow = XtVaCreateManagedWidget("topRow",
            formWidgetClass, propsPopup,
            NULL);

    categoryChoice = XtVaCreateManagedWidget("categoryChoice",
            captionWidgetClass, topRow,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP,
            (XtPointer)categoryChoice, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    control = XtVaCreateManagedWidget("control",
            controlAreaWidgetClass, categoryChoice,
            NULL);
    abbrevMenuButton = XtVaCreateManagedWidget("abbrevMenuButton",
            abbrevMenuButtonWidgetClass, control,
            NULL);
    (void) XtVaCreateManagedWidget("preview",
            staticTextWidgetClass, control,
            NULL);
    XtVaGetValues(abbrevMenuButton,
            XtNmenuPane, (XtArgVal)&menuPane,
            NULL);
    exclusive = XtVaCreateManagedWidget("exclusive",
            exclusivesWidgetClass, menuPane, 
            NULL);

    /* Add category names to the abbreviated menu widget. */

    /* Create footer for error messages */

    for (ii = 0, categoryInfo = propsGetCategoryInfo(categories, ii);
         categoryInfo && categoryInfo->info;
         ii++, categoryInfo = propsGetCategoryInfo(categories, ii))
    {
        button = XtVaCreateManagedWidget(categoryInfo->info->name,
                rectButtonWidgetClass, exclusive,
                NULL);
        XtAddCallback(button, XtNselect, propsShowCategory,
                (XtPointer)categoryInfo);
        if (ii == 0) {
            categoryFirst = button;
        }
    }

    /* Add the standard settings button to the upper right of propsPopup */

    standButton = XtVaCreateManagedWidget("standButton",
            oblongButtonWidgetClass, topRow,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)standButton, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(standButton, XtNselect, propsStandCb,
        (XtPointer)categories);

    /* Create main area with border where categories appear */

    propsBorder = XtVaCreateManagedWidget("propsBorder",
	    controlAreaWidgetClass, propsPopup,
	    NULL);
    propsMain = XtVaCreateManagedWidget("propsMain",
            bulletinBoardWidgetClass, propsBorder,
            NULL);

    /* Create apply and reset buttons */

    propsCreateButtons(propsBorder, categories);

    /* Create footer for error messages */

    propsFooter = XtVaCreateManagedWidget("propsFooter", 
	    footerPanelWidgetClass, propsPopup,
	    NULL);
    (void) XtVaCreateManagedWidget("propsError",
	staticTextWidgetClass, propsFooter,
	NULL);

    /* Create, realize, & display widgets for the first category. */

    currentCategory         = categories;
    currentCategory->widget = NULL;
    currentCategory->id     = 0;
    propsShowCategory(categoryFirst, (XtPointer)categories, (XtPointer)NULL);

    XtAppMainLoop(appContext);
}


/*
 * propsBuildCategoryList
 *
 * Register each category by placing its registration information in a
 * linked list. The order of registration determines the order in which
 * the category names appear on the abbreviated category menu.
 */

static PropsCatInfo*
propsBuildCategoryList(
    void)
{
    PropsCatInfo	*categoryList = NULL;

    propsPutCategoryInfo(&categoryList, colorRegisterCategory());
    propsPutCategoryInfo(&categoryList, fontsRegisterCategory());
    propsPutCategoryInfo(&categoryList, keyboardRegisterCategory());
    propsPutCategoryInfo(&categoryList, mouseRegisterCategory());
    propsPutCategoryInfo(&categoryList, menuRegisterCategory());
    propsPutCategoryInfo(&categoryList, wkspMenuRegisterCategory());
    propsPutCategoryInfo(&categoryList, miscRegisterCategory());
    propsPutCategoryInfo(&categoryList, localeRegisterCategory());

    return categoryList;
}


/*
 * propsCreateButtons
 *
 * Create the "Apply", "Standard", and "Reset" buttons on a control area.
 */

static void
propsCreateButtons(
    Widget		 parent,
    PropsCatInfo	*categories)
{
    Widget		 propsButtons,
			 applyButton,
			 resetButton,
			 buttons;

    propsButtons = XtVaCreateManagedWidget("propsButtons",
            rubberTileWidgetClass, parent,
            NULL);

    (void) XtVaCreateManagedWidget("space",
            staticTextWidgetClass, propsButtons,
	    XtNlabel,		"",
	    XtNweight,		1,
            NULL);

    buttons = XtVaCreateManagedWidget("buttons",
            controlAreaWidgetClass, propsButtons,
	    XtNweight,		0,
            NULL);

    applyButton = XtVaCreateManagedWidget("applyButton",
            oblongButtonWidgetClass, buttons,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)applyButton, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(applyButton, XtNselect, propsApplyCb,
        (XtPointer)categories);
            
    resetButton = XtVaCreateManagedWidget("resetButton",
            oblongButtonWidgetClass, buttons,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)resetButton, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(resetButton, XtNselect, propsResetCb,
        (XtPointer)categories);
            
    (void) XtVaCreateManagedWidget("space",
            staticTextWidgetClass, propsButtons,
	    XtNlabel,		"",
	    XtNweight,		1,
            NULL);
}


/*
 * propsExit
 *
 * Exits the program. Called when the popup's pushpin is pulled or when the
 * popup is dismissed from the window menu.
 */

/* ARGSUSED */
static void
propsExit(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    OlWMProtocolVerify *pv = (OlWMProtocolVerify *)callData;
    
    if (pv->msgtype == OL_WM_DELETE_WINDOW) {
	XFlush(display);
    	exit(0);
    }
}


/*
 * propsSystemInitialize
 *
 * Call the initializeServer() function for each category. This function
 * is called upon window system startup only.
 */

static void
propsSystemInitialize(
    PropsCatInfo	*categories)
{
    int			 ii;
    PropsCatInfo	*categoryInfo;

    for (ii = 0, categoryInfo = propsGetCategoryInfo(categories, ii);
         categoryInfo && categoryInfo->info;
         ii++, categoryInfo = propsGetCategoryInfo(categories, ii))
    {
        InitializeServer(categoryInfo, (ACCESS_ALL_DB));
    }
}


/***************************************************************************
* Private Category List Handling Functions
***************************************************************************/

/*
 * propsGetCategoryCurrentInfo
 *
 * Return the category information for the current category.
 */

static PropsCatInfo*
propsGetCategoryCurrentInfo(
    PropsCatInfo	*categories)
{
    PropsCatInfo	*current;

    for (current = categories; current != NULL; current = current->next) {
	if (   (currentCategory         != NULL)
	    && (currentCategory->widget == current->widget))
	{
            break;
        }
    }
    
    return current;
}


/*
 * propsGetCategoryInfo
 *
 * Return the category info for the category with the given id.
 */

static PropsCatInfo*
propsGetCategoryInfo(
    PropsCatInfo	*categories,
    int			 id)
{
    PropsCatInfo	*current;

    for (current = categories; current != NULL; current = current->next) {
        if (current->id == id) {
            break;
        }
    }
    
    return current;
}


/*
 * propsGetNextId
 *
 * Provide a unique integer identifier.
 */

static int
propsGetNextId(
    void)
{
    static int	id = 0;
 
    return id++;
}


/*
 * propsPutCategoryInfo
 * 
 * Add a CategoryInfo structure to the linked list containing such structures.
 * Assign the category an id and store it in the list. The memory allocated
 * here is used for the life of the program and should not be freed.
 */

static void
propsPutCategoryInfo(
    PropsCatInfo       **infoList,
    CategoryInfo	*infoObject)
{
    PropsCatInfo	*current,
    			*newListNode;
 
    if (infoObject == NULL) {
        return;
    }
    
    newListNode = XtNew(PropsCatInfo);

    newListNode->info   = infoObject;
    newListNode->id     = propsGetNextId();
    newListNode->widget = NULL;
    newListNode->next   = NULL;
 
    if (*infoList == NULL) {
        *infoList = newListNode;
    } else {
        for (current= *infoList; current->next != NULL; current=current->next){
        	;	/* Loop until find list's end. */
        }
        current->next = newListNode;
    }
}


/*
 * propsShowCategory
 *
 * Hide the current category and show the chosen category. If the newly 
 * chosen category widget tree has not been created yet, it is created and
 * realized here. When a category is chosen, its SyncWithServer() function
 * is called to ensure the displayed settings correspond to the current
 * state of the server. If the category is newly created, the sizes of its
 * container widgets are saved. If the category existed previously, it is
 * sized according to the previously stored dimensions.
 */

/* ARGSUSED */
static void
propsShowCategory(
    Widget		 widget,
    XtPointer		 clientData,
    XtPointer		 callData)
{
    Widget		 preview;
    String		 categoryLabel;
    PropsCatInfo	*categoryInfo  = (PropsCatInfo*)clientData;

    wspClearFooter();

    if ((categoryInfo == NULL) || (categoryInfo->info == NULL)) {
	wspErrorFooter(propsErrors.noCategoryInfo, NULL);

        return;
    }


    /* If the requested category is already displayed just return */
    
    if ( (currentCategory != NULL) && (currentCategory->widget != NULL)) {
        if (categoryInfo->widget == currentCategory->widget) {
            return;
        }
    }

    wspChangeBusyState(TRUE);


    /* Set category preview text to chosen menu item */
    
    preview = XtNameToWidget(propsPopup, "*categoryChoice*preview");

    if (preview == NULL) {

	(void) propsSetCategoryMenu(widget, currentCategory->id);
	wspErrorFooter(propsErrors.noCategoryName, NULL);
	wspChangeBusyState(FALSE);

        return;
    }
    
    XtVaGetValues(widget,  XtNlabel,  (XtArgVal)&categoryLabel, NULL);
    XtVaSetValues(preview, XtNstring, (XtArgVal) categoryLabel, NULL);
    /*
     * Create the category widget tree if it doesn't exist, reading values
     * from the resource databases and saving the category widths.
     */
    if (categoryInfo->widget == NULL) {
        categoryInfo->widget = CreateCategory(categoryInfo, propsMain);
        if (categoryInfo->widget == NULL) {
	    Widget	setButton;

	    setButton = propsSetCategoryMenu(widget, currentCategory->id);
	    if (setButton != NULL) {
		XtVaGetValues(setButton,
		    XtNlabel,  (XtArgVal)&categoryLabel, NULL);
		XtVaSetValues(preview,
		    XtNstring, (XtArgVal) categoryLabel, NULL);
	    }
	    wspErrorFooter(propsErrors.noCategoryCreation, NULL);
	    wspChangeBusyState(FALSE);

	    XtRealizeWidget(topLevel); /* In case creation fails for 1st */
            return;
        }

	/*
	 * Realize the entire widget tree. When the program starts, this
	 * ensures the first category is created before the foundation
	 * widgets are realized, a performance win. When the foundation
	 * widgets are already realized, this simply realizes the widgets
	 * for the newly created category.
	 */
	XtRealizeWidget(topLevel);
        
	ShowCategory(categoryInfo);
        if (SyncWithServer(categoryInfo, (ACCESS_ALL_DB), ACCESS_RWIN_DB))
        {
            if (!wspUpdateResourceManager()) {
		wspErrorFooter(propsErrors.noUpdateXrm, NULL);
	    }
        }
        ReadDBSettings(categoryInfo, (ACCESS_ALL_DB), FALSE);
        BackupSettings(categoryInfo, FALSE);
        if ((currentCategory != NULL) && (currentCategory->widget != NULL)) {
            HideCategory(currentCategory);
            XtUnmapWidget(currentCategory->widget);
        }
        XtMapWidget(categoryInfo->widget);
	propsCenterCategory(categoryInfo->widget);
    /*
     * If the category already exists, synchronize the resource settings in 
     * the root window RESOURCE_MANAGER property with the state of the server
     * & read category settings from the resource databases.
     */
    } else {
        ShowCategory(categoryInfo);
        if (SyncWithServer(categoryInfo, (ACCESS_ALL_DB), ACCESS_RWIN_DB))
        {
            if (!wspUpdateResourceManager()) {
		wspErrorFooter(propsErrors.noUpdateXrm, NULL);
	    }
	}
        ReadDBSettings(categoryInfo, (ACCESS_ALL_DB), TRUE);
        BackupSettings(categoryInfo, TRUE);

        HideCategory(currentCategory);
        XtUnmapWidget(currentCategory->widget);
        XtMapWidget(categoryInfo->widget);
	propsCenterCategory(categoryInfo->widget);
    }
    currentCategory = categoryInfo;
    wspChangeBusyState(FALSE);
}


/***************************************************************************
* Private Database Handling Functions
**************************************************************************/
 
/*
 * propsApplyCb
 *
 * This is the callback for the Apply button. Call category's RealizeSettings()
 * function. If the RealizeSettings() function fails, restore settings to the
 * previous values to let the user know the setting did not work. Backup
 * settings, update resource databases, and remove change bars.
 */

/* ARGSUSED */
static void
propsApplyCb(
    Widget		 widget,
    XtPointer		 clientData,
    XtPointer		 callData)
{
    PropsCatInfo	*categories = (PropsCatInfo*)clientData;
    PropsCatInfo	*categoryInfo;

    wspClearFooter();

    categoryInfo = propsGetCategoryCurrentInfo(categories);
    if ((categoryInfo == NULL) || (categoryInfo->info == NULL)) {
        return;
    }


    /* If the settings can't be realized, reset them */
    
    if (!RealizeSettings(categoryInfo)) {
        RestoreSettings(categoryInfo);
        DeleteChangeBars(categoryInfo);
        
        return;
    }
    
    
    /* Otherwise, backup settings, update resource dbs, and category widgets */
    
    BackupSettings(categoryInfo, FALSE);
    SaveDBSettings(categoryInfo, (ACCESS_RWIN_DB | ACCESS_USER_DB));
    propsWriteDb();
    DeleteChangeBars(categoryInfo);
}


/*
 * propsDynamicUpdate
 *
 * Get the XA_RESOURCE_MANAGER property from the root window and convert
 * it to a Xrm database. Update individual category values by calling
 * propsUpdateCategories(). This function is called whenever the resource
 * database changes.
 */

static void
propsDynamicUpdate(
    XtPointer		 data)
{
    PropsCatInfo	*categories = (PropsCatInfo*)data;
    char		*resourceString;

    resourceString = propsGetRootWindowProperty();
    if (resourceString != NULL) { /* Error reported by GetRootWindowProperty */
        rwinDb = XrmGetStringDatabase(resourceString);
        propsUpdateCategories(categories);
        XFree(resourceString);
    }
}


/*
 * propsGetRootWindowProperty
 *
 * Get the RESOURCE_MANAGER property from the root window. The caller is
 * responsible for freeing the resource strings using XFree().
 */

static char*
propsGetRootWindowProperty(
    void)
{
    Atom		 retType;
    int			 retFormat;
    unsigned long	 retItems;
    unsigned long	 retRemain;
    unsigned char	*resourceString;
    int			 status;

    status = XGetWindowProperty(display, RootWindow(display,0),
            XA_RESOURCE_MANAGER, 0L, 100000000L, False, XA_STRING,
            &retType, &retFormat, &retItems, &retRemain, &resourceString);
    if (   (status     != Success)
        || (retType   != XA_STRING)
        || (retRemain != 0)
        || (resourceString == NULL))
    {
	wspErrorStandard("propsGetRootWindowProperty()",
	    propsErrors.noResourceManager, status, NULL);

        return NULL;
    }
    
    return (char*)resourceString;
}


/*
 * propsResetCb
 *
 * This is the callback for the "Reset" button. Set the current value to
 * the backup value for each item in the category.
 */

/* ARGSUSED */
static void
propsResetCb(
    Widget		 widget,
    XtPointer		 clientData,
    XtPointer		 callData)
{
    PropsCatInfo	*categories = (PropsCatInfo*)clientData;
    PropsCatInfo	*categoryInfo;

    wspClearFooter();

    categoryInfo = propsGetCategoryCurrentInfo(categories);
    if ((categoryInfo == NULL) || (categoryInfo->info == NULL)) {
    
        return;
    }
    RestoreSettings(categoryInfo);
    DeleteChangeBars(categoryInfo);
}


/*
 * propsStandCb
 *
 * This is the callback for the "Standard" button. Set the current value for
 * each item to that found in the system default resource database, dfltDb.
 */

/* ARGSUSED */
static void
propsStandCb(
    Widget		 widget,
    XtPointer		 clientData,
    XtPointer		 callData)
{
    PropsCatInfo	*categories = (PropsCatInfo*)clientData;
    PropsCatInfo	*categoryInfo;

    wspClearFooter();

    categoryInfo = propsGetCategoryCurrentInfo(categories);
    if ((categoryInfo == NULL) || (categoryInfo->info == NULL)) {
    
        return;
    }
    
    
    /* Read settings from default database & update the category widgets */
    
    ReadDBSettings(categoryInfo, ACCESS_DFLT_DB, FALSE);
    CreateChangeBars(categoryInfo);
}


/*
 * propsUpdateCategories
 *
 * For each category, read the current values from the resource databases and
 * backup those settings.
 */

static void
propsUpdateCategories(
    PropsCatInfo	*categories)
{
    PropsCatInfo	*categoryInfo;
    int			 ii;

    for (ii = 0, categoryInfo = propsGetCategoryInfo(categories, ii);
         categoryInfo && categoryInfo->info;
         ii++,   categoryInfo = propsGetCategoryInfo(categories, ii)) {

        if (categoryInfo->widget != NULL) {
            (void) SyncWithServer(categoryInfo, (ACCESS_ALL_DB),
                    ACCESS_RWIN_DB);
            /*
             * It would be appropriate to update the root window property
             * at this point if a SyncWithServer really occured. However,
             * to do so would cause this function to be called again.
             */
            ReadDBSettings(categoryInfo, ACCESS_RWIN_DB, TRUE);
            BackupSettings(categoryInfo, TRUE);
        }    
    }    
}


/*
 * propsWriteDb
 *
 * Write the user database, ~/.OWdefaults to disk and update the
 * RESOURCE_MANAGER property on the root window.
 */

static void
propsWriteDb(void)
{
    XrmPutFileDatabase(userDb, userDbFile);
    if (!wspUpdateResourceManager()) {
	wspErrorPopupConfirm(propsErrors.noUpdateXrmPopup, NULL);
    }
}

/*
 * Supporting Functions
 */

static void
propsCenterCategory(
    Widget	category)
{
    Dimension	mainW,
    		mainH,
    		catW,
    		catH,
    		catX,
    		catY;

    XtVaGetValues(propsMain,
	XtNwidth,  (XtArgVal)&mainW,
        XtNheight, (XtArgVal)&mainH,
        NULL);
    XtVaGetValues(category,
        XtNwidth,  (XtArgVal)&catW,
        XtNheight, (XtArgVal)&catH,
        NULL);
    catX = ((Dimension)(mainW - catW) / (Dimension)2) - 10; /* 10 = Chng Bars */
    catY =  (Dimension)(mainH - catH) / (Dimension)4;
    XtVaSetValues(category,
        XtNx,	catX,
        XtNy,	catY,
        NULL);
}

/*
 * propsSetCategoryMenu
 *
 * Attempt to set the requested button in the menu category. Simply return
 * without doing anything if any failure conditions are encountered. This
 * function is used by propsShowCategory() to reset the category menu when
 * the user attempts to change to a category which cannot be created.
 */

static Widget
propsSetCategoryMenu(
    Widget	currentButton,
    int		setButton)
{
    Widget	exclusive;
    WidgetList	children;
    Cardinal	numChildren;

    if (currentButton == NULL) {
	return NULL;
    }

    exclusive = XtParent(currentButton);
    if (exclusive == NULL) {
	return NULL;
    }

    XtVaGetValues(exclusive,
	XtNchildren,    (XtArgVal)&children,
	XtNnumChildren, (XtArgVal)&numChildren,
	NULL);

    if (setButton < numChildren) {
	XtVaSetValues(children[setButton], XtNset, TRUE, NULL);
	return(children[setButton]);
    } else {
	return NULL;
    }
}
