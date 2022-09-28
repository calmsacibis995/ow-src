#pragma ident	"@(#)wkspmenu.c	1.37	94/04/21 SMI"

/* Copyright */


/* Includes */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>	/* For system(3S)  */
#include <sys/wait.h>	/* For system(3S). */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/ScrollingL.h>
#include <Xol/TextLine.h>
#include <Xol/StaticText.h>

#include "props.h"
#include "wksplist.h"
#include "wkspmenuentry.h"
#include "wkspreadmenu.h"
#include "wkspwritemenu.h"

#ifdef DBMALLOC
#    include <dbmalloc.h>
#endif


/* Glyphs used by the scrolling list. */

#include "empty.xbm"
#include "default.xbm"


/* Static Module globals.  Mostly things that callbacks need access to. */

static MenuEntry	 menuTitle,
			 menuClipBoard;

static HeadTail		*menuEntryList = NULL;

typedef struct {
	String	programsMenuNotIncluded;
} MenuErrorMsg, *MenuErrorMsgPtr;

static MenuErrorMsg	 menuErrorMsg;
	
static XtResource	 appResources[] = {
				{
				       "programsMenuNotIncluded",/*Res. name. */
				       "ProgramsMenuNotIncluded",/*Res. class.*/
					XtRString,		 /*Res. type. */
					sizeof(String),		 /*Res. size  */
					XtOffset(		 /*Res. Offset*/
						MenuErrorMsgPtr,
						programsMenuNotIncluded),
					XtRImmediate,		 /* Def. type */
"Changes to the Workspace Programs menu are stored in the file\n"/* Def. str. */
"\".openwin-menu-programs\" in your home directory.  Your\n"
"\".openwin-menu\" file does not include this file so any\n"
"changes you make will not be avaliable via the Workspace menu.\n"
"See the olwm(1) man page for information on how to include your\n"
"\".openwin-menu-programs\" file in your \".openwin-menu\" file.\n"
				}
			};


static OlListToken	 currentlySetToken	= NULL;

static Widget		 wkspMenuControlArea,


			 wkspMenuListControlArea,
		
			 scrollingListCaption,
			 scrollingList,

			 labelTextLine,
 			 commandTextLine,

    		
			 wkspMenuButtonControlArea,

			 changeButton,
			 addButton,
			 cutButton,
			 copyButton,
			 pasteBeforeButton,
			 pasteAfterButton,
			 deleteButton,
			 defaultButton;

enum {
			 OPENWINHOME_MENU,
			 PREFER_USERS_MENU
};


/* Category functions */

static Widget		 wkspMenuCreateCategory( Widget);
static void		 wkspMenuRestoreSettings(Widget);
static void		 wkspMenuReadDBSettings( Widget, int, Boolean);
static Boolean		 wkspMenuRealizeSettings(Widget);
static void		 wspMenuShowCategory(    Widget);


/* Pointers to Olit's scrolling list built-in functions */

static OlListToken    (*AddItem)(   Widget, OlListToken,OlListToken,OlListItem);
static void           (*DeleteItem)(Widget, OlListToken);
static void           (*TouchItem)( Widget, OlListToken);
static void           (*UpdateView)(Widget, Boolean);
static void           (*ViewItem)(  Widget, OlListToken);


/* Private functions */

static HeadTail	       *loadWorkspaceMenuIntoScrollingList(Widget, int);

static void             createTextLineHelper(Widget, Widget*,
					     strconst, strconst);

static void		textLineCommitCallback(Widget, XtPointer, XtPointer);

static void		createButtonHelper(Widget, Widget*, strconst,
					   void (*)(Widget,XtPointer,XtPointer),
					   Boolean);

static void		currentCallback(          Widget, XtPointer, XtPointer);
static void		changeButtonCallback(     Widget, XtPointer, XtPointer);
static void		addButtonCallback(        Widget, XtPointer, XtPointer);
static void		cutButtonCallback(        Widget, XtPointer, XtPointer);
static void		copyButtonCallback(       Widget, XtPointer, XtPointer);
static void		pasteBeforeButtonCallback(Widget, XtPointer, XtPointer);
static void		pasteAfterButtonCallback( Widget, XtPointer, XtPointer);
static void	 	deleteButtonCallback(     Widget, XtPointer, XtPointer);
static void		defaultButtonCallback(    Widget, XtPointer, XtPointer);

static ListElement     *getListElement(         const OlListItem  *const);
static MenuEntry       *getMenuEntryFromElement(const ListElement *const);
static MenuEntry       *getMenuEntryFromItem(   const OlListItem  *const);
static MenuEntry       *getMenuEntryFromToken(        OlListToken);
static OlListItem      *getScrollingListItem(   const ListElement *const);

static char	       *skipLeadingWhiteSpace(const char*);
static Boolean		emptyString(strconst);

static char	       *allocPathBuffer(int *const);

static void		sendRereadMessage(const Display *const);

static void		freeStringIfNeedTo(char *);

static XImage*		getScreenGlyph(
				const unsigned char*,
				const unsigned int,
				const unsigned int);

static void		appendLocale(char *const);
static int		programsMenuIncluded(void);



/* Category registration data */

static CategoryInfo info = {
	"wkspMenu",
	 wkspMenuCreateCategory,
	 wspCreateChangeBars,
	 wspDeleteChangeBars,
	 wspBackupSettings,
	 wkspMenuRestoreSettings,
	 wkspMenuReadDBSettings,
	 wspSaveDbSettings,
	 wkspMenuRealizeSettings,
	 wspNoopInitializeServer,
	 wspNoopSyncWithServer,
	 wspMenuShowCategory,
	 wspNoopHideCategory
};


/* Category Functions */

CategoryInfo*
wkspMenuRegisterCategory(void)
{
	return &info;
}



/*
 * Programs Menu editor Widget Tree
 *
 * mainControl
 *	wkspMenuControlArea
 *		wkspMenuListControlArea
 *			scrollingListCaption
 *				scrollingList
 *			labelTextLineCaption
 *				labelTextLine
 *			commandTextLineCaption
 *				commandTextLine
 *		wkspMenuButtonControlArea
 *			changeButton
 *			addButton
 *			cutButton
 *			copyButton
 *			pasteBeforeButton
 *			pasteAfterButton
 *			deleteButton
 *			defaultButton
 */

static Widget
wkspMenuCreateCategory(
	Widget	mainControl)
{
	wkspMenuControlArea = XtVaCreateManagedWidget(
		"wkspMenuControlArea",	controlAreaWidgetClass,
		 mainControl,
		 XtNmappedWhenManaged,	FALSE,
		 NULL);
	OlRegisterHelp(
		 OL_WIDGET_HELP,	wkspMenuControlArea,
		 NULL,
		 OL_INDIRECT_SOURCE,	(XtPointer)wspHelp);


	/*
	 * Create the Programs Menu scrolling list
	 * and the textfields used to edit it with.
	 */

	wkspMenuListControlArea = XtVaCreateManagedWidget(
		"wkspMenuListControlArea",	controlAreaWidgetClass,
		 wkspMenuControlArea,
		 NULL);

	scrollingListCaption = XtVaCreateManagedWidget(
		"scrollingListCaption", captionWidgetClass,
		 wkspMenuListControlArea,
		 NULL);
	OlRegisterHelp(
		 OL_WIDGET_HELP,	scrollingListCaption,
		 NULL,
		 OL_INDIRECT_SOURCE,	(XtPointer)wspHelp);
	scrollingList = XtVaCreateManagedWidget(
		"scrollingList",	scrollingListWidgetClass,
		 scrollingListCaption, 
		 XtNposition,		OL_RIGHT,
		 NULL);				
	XtVaGetValues(
		 scrollingList,
		 XtNapplAddItem,    &AddItem,
		 XtNapplTouchItem,  &TouchItem,
		 XtNapplUpdateView, &UpdateView,
		 XtNapplDeleteItem, &DeleteItem,
		 XtNapplViewItem,   &ViewItem,
		 NULL);
	XtAddCallback(
		scrollingList,
		XtNuserMakeCurrent, currentCallback,
		NULL);

	createTextLineHelper(
		 wkspMenuListControlArea,
		&labelTextLine,   "labelTextLine", "labelTextLineCaption");
	
	createTextLineHelper(
		 wkspMenuListControlArea,
		&commandTextLine, "commandTextLine", "commandTextLineCaption");

	XtAddCallback(
		labelTextLine,
		XtNcommitCallback, textLineCommitCallback,
		commandTextLine);
	
	XtAddCallback(
		commandTextLine,
		XtNcommitCallback, textLineCommitCallback,
		labelTextLine);


        menuEntryList = loadWorkspaceMenuIntoScrollingList(
                                scrollingList,
                                PREFER_USERS_MENU);


	/* Create editing buttons. */
    
	wkspMenuButtonControlArea = XtVaCreateManagedWidget(
		"wkspMenuButtonControlArea", controlAreaWidgetClass,
		 wkspMenuControlArea,
		 NULL);
            		 
	createButtonHelper(
		 wkspMenuButtonControlArea,
		&changeButton,		"wkspMenuChange",
		 changeButtonCallback,
		 FALSE);

	createButtonHelper(
		 wkspMenuButtonControlArea,
		&addButton,		"wkspMenuAdd",
		 addButtonCallback,
		 TRUE);

	createButtonHelper(
		 wkspMenuButtonControlArea,
		&cutButton,		"wkspMenuCut",
		 cutButtonCallback,
		 FALSE);

	createButtonHelper(
		 wkspMenuButtonControlArea,
		&copyButton,		"wkspMenuCopy",
		 copyButtonCallback,
		 FALSE);

	createButtonHelper(
		 wkspMenuButtonControlArea,
		&pasteBeforeButton,	"wkspMenuPasteBefore",
		 pasteBeforeButtonCallback,
		 FALSE);

	createButtonHelper(
		 wkspMenuButtonControlArea,
		&pasteAfterButton,	"wkspMenuPasteAfter",
		 pasteAfterButtonCallback,
		 FALSE);

	createButtonHelper(
		 wkspMenuButtonControlArea,
		&deleteButton,		"wkspMenuDelete",
		 deleteButtonCallback,
		 FALSE);

	/*
	 * This widget is only used to put a space between the
	 * default button and the rest of the editing buttons.
	 */
	 
	(void) XtVaCreateManagedWidget(
		"", staticTextWidgetClass,
		wkspMenuButtonControlArea,
		XtNstring, "",
		NULL);

	createButtonHelper(
		wkspMenuButtonControlArea,
	       &defaultButton,		"wkspMenuDefault",
		defaultButtonCallback,
		FALSE);

	return wkspMenuControlArea;
}


/* Called whenever the catagory is shown. */

static void
wspMenuShowCategory(
	Widget	wkspMenuTopWidget)
{
	int		 pathLength;
	char		*customOpenwinMenuPath;
	strconst	 HOME = getenv("HOME");


	XtGetApplicationResources(
		wkspMenuTopWidget,	/* Widget resources are assoc. with */
	       &menuErrorMsg,		/* Structure to fill in. */
		appResources,		/* Resource info. */
		XtNumber(appResources),	/* Number of resources. */
		NULL,			/* Unused, args to override res. with.*/
		0);			/* Number of args. */

	customOpenwinMenuPath = allocPathBuffer(&pathLength);
	(void) sprintf(customOpenwinMenuPath, "%s/.openwin-menu", HOME);
	
	if (access(customOpenwinMenuPath, R_OK) == 0) {
		if (programsMenuIncluded() != 0) {
			wspErrorPopupConfirm(
				menuErrorMsg.programsMenuNotIncluded, NULL);
		}
	}
}



/* Called when the Apply button is pressed. */

/*ARGSUSED*/
static Boolean
wkspMenuRealizeSettings(
	Widget		 category)
{
	int		 pathLength;
	char		*customProgramsMenuPath,
			*customProgramsMenuPathBackup;
	
	FILE		*customProgramsMenuFILE;

	strconst	 HOME = getenv("HOME");


	customProgramsMenuPath       = allocPathBuffer(&pathLength);
	customProgramsMenuPathBackup = allocPathBuffer(&pathLength);

	(void) sprintf(customProgramsMenuPath,
		"%s/.openwin-menu-programs",     HOME);
	appendLocale(customProgramsMenuPath);
	(void) sprintf(customProgramsMenuPathBackup,
		"%s/.openwin-menu-programs.BAK", HOME);
	appendLocale(customProgramsMenuPathBackup);

	rename(customProgramsMenuPath, customProgramsMenuPathBackup);
	/* REMIND: What to do if rename fails? */

	customProgramsMenuFILE = fopen(customProgramsMenuPath, "w");

	writeMenu(customProgramsMenuFILE, menuEntryList, &menuTitle);
	
	(void) fclose(customProgramsMenuFILE);
	
	
	sendRereadMessage(display); /* Tell olwm to reread programs menu file.*/
	
	return TRUE;
}



/* Called when the Reset button is pressed. */

/*ARGSUSED*/
static void
wkspMenuRestoreSettings(
	Widget		 widget)
{
	ListElement	*menuListElement,
			*menuListElementToDelete;

	MenuEntry	*menuEntry;
	
	OlListItem	*scrollingListItem;
	
	ListStatus	 status;


	/* Empty the scrolling list. */
 
	(*UpdateView)(scrollingList, FALSE);

	for (menuListElement = menuEntryList->head;
	     menuListElement != NULL;
	     menuListElement = menuListElement->next)
	{
		scrollingListItem = getScrollingListItem(menuListElement);
		menuEntry         = getMenuEntryFromElement(menuListElement);

		XtFree(scrollingListItem->label);
		(*DeleteItem)(scrollingList, menuEntry->token);
	}

	/* Empty the menu entry list. */

	menuListElement = menuEntryList->head;
	while (menuListElement != NULL) {
		menuEntry = getMenuEntryFromElement(menuListElement);
		freeStringIfNeedTo(menuEntry->label);
		freeStringIfNeedTo(menuEntry->command);
		XtFree((char *)menuEntry);

		/* Get the next element before deleting the current one. */
		menuListElementToDelete = menuListElement;
		menuListElement         = menuListElement->next;
		status = deleteFromList(menuEntryList, menuListElementToDelete);
		assert(status == SUCCESS);
	}

	/* Clear globals. */

	freeStringIfNeedTo(menuClipBoard.label);
	freeStringIfNeedTo(menuClipBoard.command);
	menuClipBoard.label   = "";
        menuClipBoard.command = "";
        menuClipBoard.type    = UNKNOWN;
        menuClipBoard.token   = NULL;

	freeStringIfNeedTo(menuTitle.label);
	freeStringIfNeedTo(menuTitle.command);
        menuTitle.label       = "";
        menuTitle.command     = "";
        menuTitle.type        = UNKNOWN;
        menuTitle.token       = NULL;
       
	currentlySetToken     = NULL;

	free(menuEntryList);
	menuEntryList = NULL;


	/* Reload programs menu. */

        menuEntryList = loadWorkspaceMenuIntoScrollingList(
        			scrollingList,
        			PREFER_USERS_MENU);

	
        /* Set correct button sensitivities. */

        XtSetSensitive(changeButton,      FALSE);
	XtSetSensitive(addButton,         TRUE);
	XtSetSensitive(cutButton,         FALSE);
	XtSetSensitive(copyButton,        FALSE);
	XtSetSensitive(pasteBeforeButton, FALSE);
	XtSetSensitive(pasteAfterButton,  FALSE);
	XtSetSensitive(deleteButton,      FALSE);
	XtSetSensitive(defaultButton,     FALSE);


	/* Clear textfields. */

	XtVaSetValues(labelTextLine,   XtNstring, "", NULL);
	XtVaSetValues(commandTextLine, XtNstring, "", NULL);
}



/* Called when the Standard button is pressed. */

/*ARGSUSED*/
static void
wkspMenuReadDBSettings(
	Widget		 unused1,
	int		 dbFlag,
	Boolean		 unused3)
{
	ListElement	*menuListElement,
			*menuListElementToDelete;

	MenuEntry	*menuEntry;
	
	OlListItem	*scrollingListItem;
	
	ListStatus	 status;


	/* Empty the scrolling list. */
 
	(*UpdateView)(scrollingList, FALSE);

	for (menuListElement = menuEntryList->head;
	     menuListElement != NULL;
	     menuListElement = menuListElement->next)
	{
		scrollingListItem = getScrollingListItem(menuListElement);
		menuEntry         = getMenuEntryFromElement(menuListElement);

		XtFree(scrollingListItem->label);
		(*DeleteItem)(scrollingList, menuEntry->token);
	}

	/* Empty the menu entry list. */

	menuListElement = menuEntryList->head;
	while (menuListElement != NULL) {
		menuEntry = getMenuEntryFromElement(menuListElement);
		freeStringIfNeedTo(menuEntry->label);
		freeStringIfNeedTo(menuEntry->command);
		XtFree((char *)menuEntry);

		/* Get the next element before deleting the current one. */
		menuListElementToDelete = menuListElement;
		menuListElement         = menuListElement->next;
		status = deleteFromList(menuEntryList, menuListElementToDelete);
		assert(status == SUCCESS);
	}

	/* Clear globals. */

	freeStringIfNeedTo(menuClipBoard.label);
	freeStringIfNeedTo(menuClipBoard.command);
	menuClipBoard.label   = "";
        menuClipBoard.command = "";
        menuClipBoard.type    = UNKNOWN;
        menuClipBoard.token   = NULL;

	freeStringIfNeedTo(menuTitle.label);
	freeStringIfNeedTo(menuTitle.command);
        menuTitle.label       = "";
        menuTitle.command     = "";
        menuTitle.type        = UNKNOWN;
        menuTitle.token       = NULL;
       
	currentlySetToken     = NULL;

	free(menuEntryList);
	menuEntryList = NULL;


	/* Load the programs menu. */

        if (dbFlag == ACCESS_DFLT_DB) {
        	menuEntryList = loadWorkspaceMenuIntoScrollingList(
        			scrollingList,
        			OPENWINHOME_MENU);
        } else {
        	menuEntryList = loadWorkspaceMenuIntoScrollingList(
        			scrollingList,
        			PREFER_USERS_MENU);
	}

	
        /* Set correct button sensitivities. */

        XtSetSensitive(changeButton,      FALSE);
	XtSetSensitive(addButton,         TRUE);
	XtSetSensitive(cutButton,         FALSE);
	XtSetSensitive(copyButton,        FALSE);
	XtSetSensitive(pasteBeforeButton, FALSE);
	XtSetSensitive(pasteAfterButton,  FALSE);
	XtSetSensitive(deleteButton,      FALSE);
	XtSetSensitive(defaultButton,     FALSE);


	/* Clear textfields. */

	XtVaSetValues(labelTextLine,   XtNstring, "", NULL);
	XtVaSetValues(commandTextLine, XtNstring, "", NULL);
}



/* Private function definitions */


/*ARGSUSED*/
static void
currentCallback(
        Widget              scrollingList,
        XtPointer	    clientData,
        XtPointer           callData)
{
	OlListToken	    justClickedOnToken;
	
	OlListItem	   *currentlySetItem,
			   *justClickedOnItem;

	MenuEntry	   *menuEntry;


	justClickedOnToken  = (OlListToken)callData;
	justClickedOnItem   = OlListItemPointer(justClickedOnToken);
	
	if (currentlySetToken == justClickedOnToken) {
		/* A set scrolling list item was SELECTED so unset it */

		currentlySetItem        = justClickedOnItem;
		/* Turn off set bit. */
		currentlySetItem->attr &= ~OL_LIST_ATTR_CURRENT;
		(*TouchItem)(scrollingList, currentlySetToken);
		currentlySetToken       = NULL;
		
		XtVaSetValues(labelTextLine,   XtNstring, "", NULL);
		XtVaSetValues(commandTextLine, XtNstring, "", NULL);

		XtSetSensitive(changeButton,  FALSE);
		XtSetSensitive(addButton,     TRUE);
		XtSetSensitive(cutButton,     FALSE);
		XtSetSensitive(copyButton,    FALSE);
		XtSetSensitive(deleteButton,  FALSE);
		XtSetSensitive(defaultButton, FALSE);
	} else {
		/* A non-set scrolling list item was SELECTED so set it. */

		/* Unset currently set item if there is one. */
		if (currentlySetToken != NULL) {
			currentlySetItem = OlListItemPointer(currentlySetToken);
			if (currentlySetItem->attr & OL_LIST_ATTR_CURRENT) {
				/* Turn off set bit. */
				currentlySetItem->attr &= ~OL_LIST_ATTR_CURRENT;
			}
			(*TouchItem)(scrollingList, currentlySetToken);
		}
	
		/* Set the item that was just clicked on. */
		justClickedOnItem->attr |= OL_LIST_ATTR_CURRENT;
		(*TouchItem)(scrollingList, justClickedOnToken);
		currentlySetToken = justClickedOnToken;

		menuEntry = getMenuEntryFromItem(justClickedOnItem);

		if (menuEntry->type == SEPARATOR) {
			XtVaSetValues(
				labelTextLine,
				XtNstring, "",
				NULL);
			XtVaSetValues(
				commandTextLine,
				XtNstring, "",
				NULL);
			XtSetSensitive(defaultButton, FALSE);
		} else {
			XtVaSetValues(
				labelTextLine,
				XtNstring, menuEntry->label,
				NULL);
			XtVaSetValues(
				commandTextLine,
				XtNstring, menuEntry->command,
				NULL);
			XtSetSensitive(defaultButton, TRUE);
		}
		
		XtSetSensitive(changeButton,  TRUE);
		XtSetSensitive(addButton,     FALSE);
		XtSetSensitive(cutButton,     TRUE);
		XtSetSensitive(copyButton,    TRUE);
		XtSetSensitive(deleteButton,  TRUE);
	}
}



/*ARGSUSED*/
static void
changeButtonCallback(
	Widget		 changeButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	MenuEntry	*menuEntry;

	OlListItem	*scrollingListItem;
	
	OlStr		 tempString;


	/* Change shouldn't be active if nothing in the scrolling is set. */
	assert(currentlySetToken != NULL);

	menuEntry = getMenuEntryFromToken(currentlySetToken);
	
	freeStringIfNeedTo(menuEntry->label);
	XtVaGetValues(labelTextLine,   XtNstring, &tempString, NULL);
	menuEntry->label = XtNewString(tempString);

	freeStringIfNeedTo(menuEntry->command);
	XtVaGetValues(commandTextLine, XtNstring, &tempString, NULL);
	menuEntry->command = XtNewString(tempString);
		
	menuEntry->type    = UNKNOWN;

	if (   emptyString(menuEntry->label)
	    || emptyString(menuEntry->command)
	    || (strcmp(menuEntry->command, "SEPARATOR") == 0))
	{
		freeStringIfNeedTo(menuEntry->label);
		freeStringIfNeedTo(menuEntry->command);
		menuEntry->label   = "";
		menuEntry->command = "SEPARATOR";
		menuEntry->type    =  SEPARATOR;
		
		XtVaSetValues(labelTextLine,   XtNstring, "", NULL);
		XtVaSetValues(commandTextLine, XtNstring, "", NULL);
	} else {
		XtVaSetValues(
			labelTextLine,
			XtNstring, menuEntry->label,
			NULL);
		XtVaSetValues(
			commandTextLine,
			XtNstring, menuEntry->command,
			NULL);
	}

	scrollingListItem        = OlListItemPointer(currentlySetToken);
	scrollingListItem->label = XtNewString(menuEntry->label);

	(*TouchItem)(scrollingList, currentlySetToken);
	(*ViewItem)( scrollingList, currentlySetToken);
}



/*ARGSUSED*/
static void
addButtonCallback(
	Widget		 addButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	ListElement	*newMenuListElement;

	MenuEntry	*newMenuEntry;
	
	OlListItem	 newScrollingListItem;

	ListStatus	 status;
	
	OlStr		 tempString;
	

	/* Add shouldn't be active if something in the scrolling is set. */
	assert(currentlySetToken == NULL);

	newMenuListElement       = createListElement();
	newMenuEntry             = createMenuEntry();
	newMenuListElement->data = newMenuEntry;

	XtVaGetValues(labelTextLine,   XtNstring, &tempString, NULL);
	newMenuEntry->label      = XtNewString(tempString);

	XtVaGetValues(commandTextLine, XtNstring, &tempString, NULL);
	newMenuEntry->command    = XtNewString(tempString);
	
	newMenuEntry->type       = UNKNOWN;

	if (   emptyString(newMenuEntry->label)
	    || emptyString(newMenuEntry->command)
	    || (strcmp(newMenuEntry->command, "SEPARATOR") == 0))
	{
		XtFree(newMenuEntry->label);
		XtFree(newMenuEntry->command);
		newMenuEntry->label   = "";
		newMenuEntry->command = "SEPARATOR";
		newMenuEntry->type    =  SEPARATOR;
	}

	newScrollingListItem.label_type = OL_BOTH;
	newScrollingListItem.label      = XtNewString(newMenuEntry->label);
	newScrollingListItem.glyph      = getScreenGlyph(
							empty_bits,
							empty_width,
							empty_height);
	newScrollingListItem.attr       = 0;
	newScrollingListItem.user_data  = newMenuListElement;
	newScrollingListItem.mnemonic   = 0;
 
	status = appendAfter(
			menuEntryList,
			menuEntryList->tail,
			newMenuListElement);

	assert(status == SUCCESS);

	newMenuEntry->token = (*AddItem)(
			scrollingList, 0,
			0,	/* Append item to end of scrolling list. */
			newScrollingListItem);

	(*ViewItem)(scrollingList, newMenuEntry->token);
}



/*ARGSUSED*/
static void
cutButtonCallback(
	Widget		 cutButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	ListElement	*menuListElement;

	MenuEntry	*menuEntry;

	ListStatus	 status;

	OlListItem	*scrollingListItem;


	/* Cut shouldn't be active if nothing in the scrolling list is set. */
	assert(currentlySetToken != NULL);

	scrollingListItem   = OlListItemPointer(currentlySetToken);
	menuListElement     = getListElement(scrollingListItem);
	menuEntry           = getMenuEntryFromElement(menuListElement);
	
	freeStringIfNeedTo(menuClipBoard.label);
	freeStringIfNeedTo(menuClipBoard.command);
	menuClipBoard       = *menuEntry;
	menuClipBoard.token =  NULL;

	XtFree(scrollingListItem->label);
	(*DeleteItem)(scrollingList, menuEntry->token);

	XtFree((char *)menuEntry);
	status = deleteFromList(menuEntryList, menuListElement);
	assert(status == SUCCESS);

	currentlySetToken = NULL;

	XtVaSetValues(labelTextLine,   XtNstring, "", NULL);
	XtVaSetValues(commandTextLine, XtNstring, "", NULL);

	XtSetSensitive(changeButton,      FALSE);
	XtSetSensitive(addButton,         TRUE);
	XtSetSensitive(cutButton,         FALSE);
	XtSetSensitive(copyButton,        FALSE);
	XtSetSensitive(pasteBeforeButton, TRUE);
	XtSetSensitive(pasteAfterButton,  TRUE);
	XtSetSensitive(deleteButton,      FALSE);
	XtSetSensitive(defaultButton,     FALSE);
}



/*ARGSUSED*/
static void
copyButtonCallback(
	Widget	 copyButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	MenuEntry	*menuEntry;
	
	
	/* Copy shouldn't be active if nothing in the scrolling list is set. */
	assert(currentlySetToken != NULL);

	menuEntry = getMenuEntryFromToken(currentlySetToken);
    
	freeStringIfNeedTo(menuClipBoard.label);
	freeStringIfNeedTo(menuClipBoard.command);
	menuClipBoard.label   = XtNewString(menuEntry->label);
	menuClipBoard.command = XtNewString(menuEntry->command);
	menuClipBoard.type    = menuEntry->type;
	menuClipBoard.token   = NULL;

	XtSetSensitive(pasteBeforeButton, TRUE);
	XtSetSensitive(pasteAfterButton,  TRUE);
}



/*ARGSUSED*/
static void
pasteBeforeButtonCallback(
	Widget		 pasteBeforeButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	ListElement	*newMenuListElement,
			*menuListElement;
	
	MenuEntry	*newMenuEntry,
			*menuEntry;
	
	OlListItem	 newScrollingListItem;
	OlListToken	 tokenToPrePendTo;

	ListStatus	 status;


	newMenuListElement              = createListElement();

	newMenuEntry                    = createMenuEntry();
	newMenuEntry->label             = XtNewString(menuClipBoard.label);
	newMenuEntry->command           = XtNewString(menuClipBoard.command);
	newMenuEntry->type              = menuClipBoard.type;

	newMenuListElement->data        = newMenuEntry;
	
	newScrollingListItem.label_type = OL_BOTH;
	newScrollingListItem.label      = XtNewString(newMenuEntry->label);
	newScrollingListItem.glyph      = getScreenGlyph(
						empty_bits,
						empty_width,
						empty_height);
	newScrollingListItem.attr       = 0;
	newScrollingListItem.user_data  = newMenuListElement;
	newScrollingListItem.mnemonic   = 0;

	if (currentlySetToken == NULL) {
		menuListElement = menuEntryList->head;

		if (menuListElement == NULL) {
			/* Handle empty menu entry list. */
			tokenToPrePendTo = 0;
		} else {
			menuEntry = getMenuEntryFromElement(menuListElement);
			tokenToPrePendTo = menuEntry->token;
		}
	} else {
		menuListElement  = getListElement(
					OlListItemPointer(currentlySetToken));
		tokenToPrePendTo = currentlySetToken;
	}

	status = prependBefore(
			menuEntryList,
			newMenuListElement,
			menuListElement);
	
	assert(status == SUCCESS);

	newMenuEntry->token = (*AddItem)(
				scrollingList, 0,
				tokenToPrePendTo,
				newScrollingListItem);

	(*ViewItem)(scrollingList, newMenuEntry->token);
}



/*ARGSUSED*/
static void
pasteAfterButtonCallback(
	Widget		 pasteAfterButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	ListElement	*newMenuListElement,
			*menuListElement;
	
	MenuEntry	*newMenuEntry,
			*menuEntry;

	OlListItem	 newScrollingListItem;
	OlListToken	 tokenToPrePendTo;

	ListStatus	 status;


	newMenuListElement              = createListElement();

	newMenuEntry                    = createMenuEntry();
	newMenuEntry->label             = XtNewString(menuClipBoard.label);
	newMenuEntry->command           = XtNewString(menuClipBoard.command);
	newMenuEntry->type              = menuClipBoard.type;

	newMenuListElement->data        = newMenuEntry;
	
	newScrollingListItem.label_type = OL_BOTH;
	newScrollingListItem.label      = XtNewString(newMenuEntry->label);
	newScrollingListItem.glyph      = getScreenGlyph(
						empty_bits,
						empty_width,
						empty_height);
	newScrollingListItem.attr       = 0;
	newScrollingListItem.user_data  = newMenuListElement;
	newScrollingListItem.mnemonic   = 0;

	if (currentlySetToken == NULL) {
		menuListElement  = menuEntryList->tail;
		tokenToPrePendTo = 0;
	} else {
		menuListElement = getListElement(
					OlListItemPointer(currentlySetToken));

		if (menuListElement->next == NULL) {
			/* Handle appending to end of list. */
			tokenToPrePendTo = 0;
		} else {
			/*
			 * Olit scrolling lists only support prepending to an
			 * item in the list or to the end of the list so we get
			 * the menu entry after the menu entry list insertion
			 * point and use it's token as the scrolling list
			 * insertion point.
			 */
			
			menuEntry = getMenuEntryFromElement(
					menuListElement->next);
			
			tokenToPrePendTo = menuEntry->token;
		}
	}

 	status = appendAfter(
			menuEntryList,
			menuListElement,
			newMenuListElement);

	assert(status == SUCCESS);

	newMenuEntry->token = (*AddItem)(
				scrollingList, 0,
				tokenToPrePendTo,
				newScrollingListItem);

	(*ViewItem)(scrollingList, newMenuEntry->token);
}



/*ARGSUSED*/
static void
deleteButtonCallback(
	Widget		 deleteButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	ListElement	*menuListElement;
	
	MenuEntry	*menuEntry;

	OlListItem	*scrollingListItem;

	ListStatus	 status;


	/* Delete shouldn't be active if nothing in the scrolling is set. */
	assert(currentlySetToken != NULL);
	
	scrollingListItem = OlListItemPointer(currentlySetToken);
	menuListElement   = getListElement(scrollingListItem);	
	menuEntry         = getMenuEntryFromElement(menuListElement);

	XtFree(scrollingListItem->label);
	(*DeleteItem)(scrollingList, menuEntry->token);

	freeStringIfNeedTo(menuEntry->label);
	freeStringIfNeedTo(menuEntry->command);
	XtFree((char *)menuEntry);
	
	status = deleteFromList(menuEntryList, menuListElement);
	assert(status == SUCCESS);
	
	currentlySetToken = NULL;

	XtVaSetValues(labelTextLine,   XtNstring, "", NULL);
	XtVaSetValues(commandTextLine, XtNstring, "", NULL);
	
	XtSetSensitive(changeButton,  FALSE);
	XtSetSensitive(addButton,     TRUE);
	XtSetSensitive(cutButton,     FALSE);
	XtSetSensitive(copyButton,    FALSE);
	XtSetSensitive(deleteButton,  FALSE);
	XtSetSensitive(defaultButton, FALSE);
}
	

/*ARGSUSED*/
static void
defaultButtonCallback(
	Widget		 defaultButton,
	XtPointer	 clientData,
	XtPointer	 callData)
{
	ListElement	*menuListElement;
	
	MenuEntry	*menuEntry;

	OlListItem	*scrollingListItem;
	
	char		*tmp;


	/* Delete shouldn't be active if nothing in the scrolling is set. */
	
	assert(currentlySetToken != NULL);


	/* SEPARATOR can't be a default. */
	/* REMIND - remove this from 10/93.  currentCallback deals with this. */
	menuEntry = getMenuEntryFromToken(currentlySetToken);
	if (strcmp("SEPARATOR", menuEntry->command) == 0) {
		return;
	}
	
	
	/* Clear current default entry if there is one. */

	(*UpdateView)(scrollingList, FALSE);

	for (menuListElement =  menuEntryList->head;
	     menuListElement != NULL;
	     menuListElement =  menuListElement->next)
	{
		menuEntry = getMenuEntryFromElement(menuListElement);
		
		if (menuEntry->isDefault == TRUE) {
			menuEntry->isDefault = FALSE;

			tmp = menuEntry->command + strlen("DEFAULT");
			tmp = skipLeadingWhiteSpace(tmp);
			tmp = XtNewString(tmp);
			XtFree(menuEntry->command);
			menuEntry->command = tmp;
	
			scrollingListItem = OlListItemPointer(menuEntry->token);
			scrollingListItem->glyph = getScreenGlyph(
							empty_bits,
							empty_width,
							empty_height);
			(*TouchItem)(scrollingList, menuEntry->token);
		}
	}

	/* Set currently set item to be the default entry. */

	menuEntry = getMenuEntryFromToken(currentlySetToken);
	menuEntry->isDefault = TRUE;
	
	tmp = XtMalloc(strlen("DEFAULT") + strlen(menuEntry->command) + 2);
	(void) sprintf(tmp, "%s%s", "DEFAULT ", menuEntry->command);
	freeStringIfNeedTo(menuEntry->command);
	menuEntry->command = tmp;
	
	scrollingListItem = OlListItemPointer(menuEntry->token);
	scrollingListItem->glyph = getScreenGlyph(
					default_bits,
					default_width,
					default_height);
	(*TouchItem)(scrollingList, menuEntry->token);

	(*UpdateView)(scrollingList, TRUE);

	XtVaSetValues(labelTextLine,   XtNstring, menuEntry->label,   NULL);
	XtVaSetValues(commandTextLine, XtNstring, menuEntry->command, NULL);
}



static void
createTextLineHelper(
	Widget		 parent,
	Widget		*textLine,
	strconst	 textLineName,
	strconst	 textLineCaptionName)
{
	Widget		 textLineCaption;
	
	
	textLineCaption = XtVaCreateManagedWidget(
		textLineCaptionName,	captionWidgetClass,
		parent,
		NULL);

	*textLine = XtVaCreateManagedWidget(
		textLineName,		textLineWidgetClass,
		textLineCaption,
		NULL);

	 OlRegisterHelp(
		 OL_WIDGET_HELP,	*textLine,
		 NULL,
		 OL_INDIRECT_SOURCE,	(XtPointer)wspHelp);
}



static void
textLineCommitCallback(
	Widget		currentWidget,
	XtPointer	clientData,
	XtPointer	callData)
{
	OlTLCommitCallbackStruct *commitInfo   = (OlTLCommitCallbackStruct *)
					         callData;
	Widget			  nextTextLine = (Widget)clientData;
	
	KeySym			  keysym;
	
	
	if (commitInfo->reason == OL_REASON_COMMIT) {
		commitInfo->valid = TRUE;
		
		keysym = XLookupKeysym(&(commitInfo->event->xkey), 0);
		if (keysym == XK_Return) {
			OlCallAcceptFocus(nextTextLine, CurrentTime);
		}
	}
}



static void
createButtonHelper(
	Widget		 parent,
	Widget		*button,
	strconst	 buttonName,
	void	       (*callback)(Widget, XtPointer, XtPointer),
	Boolean		 sensitivity
)
{
	*button = XtVaCreateManagedWidget(
		buttonName,	 oblongButtonWidgetClass,
		parent,
		NULL);

	 XtAddCallback(
	       *button,
		XtNselect,	 callback,
		NULL);

	 OlRegisterHelp(
		OL_WIDGET_HELP,	*button,
		NULL,
		OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

	 XtSetSensitive(*button, sensitivity);
}



static HeadTail*
loadWorkspaceMenuIntoScrollingList(
	Widget		 scrollingList,
	int		 whichMenuFileToLoad)
{
	strconst	 HOME        = getenv("HOME"),
			 OPENWINHOME = wspGetOpenwinhome();

	int		 pathLength;
	char		*programsMenuPath;	

	int		 lineNumber = 0;

	OlListItem	 scrollingListItem;
	OlListToken	 token;

	ListElement	*menuListElement;
	MenuEntry	*menuEntry;
	
	int		 status;


	assert(OPENWINHOME != NULL);
	assert(HOME        != NULL);

	programsMenuPath = allocPathBuffer(&pathLength);

	if (whichMenuFileToLoad == PREFER_USERS_MENU) {
		/* Check for a customized menu first. */
	
		(void) sprintf(programsMenuPath,
			"%s/.openwin-menu-programs", HOME);
		appendLocale(programsMenuPath);

		status = access(programsMenuPath, R_OK);
		
		if (status != 0) {
			(void) sprintf(programsMenuPath,
				"%s/.openwin-menu-programs", HOME);

			status = access(programsMenuPath, R_OK);
		}
	}

	if (   (whichMenuFileToLoad == OPENWINHOME_MENU)
	    || (status              != 0))
	{
		(void) sprintf(
			programsMenuPath,
		       "%s/share/locale/%s/olwm/openwin-menu-programs",
			OPENWINHOME, wspGetLocale());
		
		status = access(programsMenuPath, R_OK);

		if (status != 0) {
			(void) sprintf(programsMenuPath,
				"%s/lib/openwin-menu-programs", OPENWINHOME);
			appendLocale(programsMenuPath);

			status = access(programsMenuPath, R_OK);
			
			if (status != 0) {
				(void) sprintf(
					programsMenuPath,
				       "%s/lib/openwin-menu-programs",
					OPENWINHOME);

				status = access(programsMenuPath, R_OK);
			}
		}
	}

	assert(status == 0);

	menuEntryList = parseMenu(programsMenuPath, &lineNumber, &menuTitle);
	
	assert(menuEntryList != NULL);
	
	(*UpdateView)(scrollingList, FALSE);

	token = 0;
	for (menuListElement  = menuEntryList->tail;
	     menuListElement != NULL;
	     menuListElement  = menuListElement->previous)
	{
		menuEntry = getMenuEntryFromElement(menuListElement);

		scrollingListItem.label      = XtNewString(menuEntry->label);
		scrollingListItem.label_type = OL_BOTH;
		if (menuEntry->isDefault == FALSE) {
			scrollingListItem.glyph = getScreenGlyph(
							empty_bits,
							empty_width,
							empty_height);
		} else {
			scrollingListItem.glyph      = getScreenGlyph(
							default_bits,
							default_width,
							default_height);
		}
		scrollingListItem.attr       = 0;
		scrollingListItem.user_data  = menuListElement;
		scrollingListItem.mnemonic   = 0;
		
		token = (*AddItem)(scrollingList, 0, token, scrollingListItem);
		
		menuEntry->token = token;
	}

	(*ViewItem)(  scrollingList, token);
	(*UpdateView)(scrollingList, TRUE);

	return menuEntryList;
}



static ListElement*
getListElement(
	const OlListItem	*const	item)
{
	assert(item != NULL);

	return (ListElement *)(item->user_data);
}


static MenuEntry*
getMenuEntryFromElement(
	const ListElement	*const	menuListElement)
{
	assert(menuListElement != NULL);
	
	return (MenuEntry *)(menuListElement->data);
}


static MenuEntry*
getMenuEntryFromItem(
	const OlListItem	*const item)
{	
	assert(item != NULL);
	
	return getMenuEntryFromElement(getListElement(item));
}


static MenuEntry*
getMenuEntryFromToken(
	OlListToken	 token)
{
	assert(token != NULL);
	
	return getMenuEntryFromItem(OlListItemPointer(token));
}


static OlListItem*
getScrollingListItem(
	const ListElement	*const	menuListElement)
{
	MenuEntry	*menuEntry;


	assert(menuListElement != NULL);
	
	menuEntry = getMenuEntryFromElement(menuListElement);
	
	return OlListItemPointer(menuEntry->token);
}


static char *
allocPathBuffer(
	int		*const pathSize)
{
#ifdef PATH_MAX
static	int		 pathMax = PATH_MAX;
#else
static	int		 pathMax = 0;
#endif
	const int	 PATH_MAX_GUESS = 1024;
	
	char		*pathSpace;
	
	
	/*
	 * Find the longest path on the system.
	 */
	if (pathMax == 0) {
		errno = 0;
		pathMax = pathconf("/", _PC_PATH_MAX);
		if (pathMax < 0) {
		/* Invalid or unsupported symbolic const. passed to pathconf */
			if (errno == 0) { /* Unsupported symbolic constant. */
				pathMax = PATH_MAX_GUESS;
			} else {
				/* REMIND, error */
			}
		} else {
			pathMax++;	/* Add one for root "/". */
		}
	}
	
	pathMax++;	/* Add one for terminating null */
	pathSpace = XtMalloc(pathMax);

	if (pathSize != NULL) {
		*pathSize = pathMax;
	}
	
	return pathSpace;
}


/* Tell olwm that the programs menu has changed. */

static void
sendRereadMessage(
	const Display	*const	 display)
{
	XClientMessageEvent	 cm;
static	Atom			 atomReReadMenuFile;
	Window			 rootwin;


	if (atomReReadMenuFile == None) {
		atomReReadMenuFile = XInternAtom(
					 (Display *)display,
					"_SUN_WM_REREAD_MENU_FILE", False);
	}

	rootwin	        = RootWindow(display, DefaultScreen(display));

	cm.type		= ClientMessage;
	cm.window	= rootwin;
	cm.message_type	= atomReReadMenuFile;
	cm.format	= 32;
	cm.data.l[0]	= atomReReadMenuFile;

	XSendEvent(
		(Display *)display, rootwin, False,
		SubstructureRedirectMask|SubstructureNotifyMask,
		(XEvent *)&cm);
}


static void
freeStringIfNeedTo(
	char	*str)
{
	if (str != NULL) {
		if ((strcmp(str, "") != 0) && (strcmp(str, "SEPARATOR") != 0)) {
			XtFree((char *)str);
		}
	}	
}


static XImage*
getScreenGlyph(
	const unsigned char	*bits,
	const unsigned int	 width,
	const unsigned int	 height)
{
	XImage			*image;
	Pixmap			 pixmap;
	const Screen  const	*screen  = XtScreen(scrollingList);
	const Display const	*display = XDisplayOfScreen((Screen *)screen);
	
	
	pixmap = XCreateBitmapFromData(
			(Display *)display,
			RootWindowOfScreen(screen),
			(char *)bits,
			width, height);
	
	image = XGetImage(
			(Display *)display,
			pixmap,
			0, 0,
			width, height,
			1,
			XYPixmap);
	
	XFreePixmap((Display *)display, pixmap);
	
	image->format = XYBitmap;
	
	return image;
}


static char *
skipLeadingWhiteSpace(
	const char	*currentCharacter)
{	
	while (isspace(*currentCharacter)) {
		currentCharacter++;
	}
	
	return (char *)currentCharacter;
}


static void
appendLocale(
	char *const	 path)
{
	const char	*locale = wspGetLocale();


	if (strcmp(locale, "C") != 0) {
		(void) strcat(path, ".");
		(void) strcat(path, locale);
	}
	
	XtFree((char *)locale);
}


/*
 * Check to see if the user's .openwin-menu file includes
 * .openwin-menu-programs.
 *
 * Precondition: Local .openwin-menu exists and is accesible.
 *
 * Returns:
 *	-1	fork() or waitpid() (called by system()) failed or
 *		command killed by signal.
 *	 
 *	 0	Local .openwin-menu-programs is included in local .openwin-menu.
 *	 	 
 *	 1	Local .openwin-menu-programs is not included in local
 *	 	 .openwin-menu.
 */

static int
programsMenuIncluded(void)
{
	int	egrepStatus,
		systemStatus,
		returnValue;


	systemStatus = system(
		"/bin/egrep -s"			  	  /* Work silently */
		"'^[^#].*INCLUDE[ 	]+openwin-menu-programs$' "
		"$HOME/.openwin-menu "			  /* Custom root menu.*/
		"2> /dev/null");			  /* Ignore error msgs*/

	if (systemStatus < 0) {
		/* fork or waitpid failed. */
		returnValue = -1;

	} else {
		if (WIFEXITED(systemStatus)) {
			/*Normal termination of processes started by system().*/
			egrepStatus = WEXITSTATUS(systemStatus);
			if (egrepStatus == 0) {
				/* Found a match. */
				returnValue = 0;
				
			} else if (egrepStatus == 1) {
				/* Didn't find a match. */
				returnValue = 1;
				
			} else {
				/*
				 * Should never get here.
				 *
				 * $HOME/.openwin-menu inaccessible or
				 * not there.
				 */
				returnValue = -1;
			}
	
		} else {
			/* Process started by system() killed by signal. */
			returnValue = -1;
		}
	}

	return returnValue;
}


/*
 * Check if a string is empty or not.  An empty string contains zero or more 
 * whitespaces and a null terminator.
 *
 * Precondition: string is null-terminated.
 *
 * Returns:
 *	FALSE	String is not empty.
 *
 *	TRUE	String is empty.
 */

static Boolean
emptyString(
	strconst	string)
{
	Boolean		isEmpty = FALSE;	/* Assume string isn't empty. */
	int		ii = 0;


	/*
	 * Walk down the string until we come to a non-whitespace character
	 * or we get to the end of the string.
	 */
        while (   isspace(string[ii])
               && (string[ii] != NULL))
        {
		ii++;
        }

	/*
	 * If current character is NULL then we are at the string's end and
	 * found only whitespace so the string is empty.  Otherwise we found a
	 * non-whitespace character and the string is not empty.
	 */
	if (string[ii] == NULL) {
		isEmpty = TRUE;
	}
	
	return isEmpty;
}
