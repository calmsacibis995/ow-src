#pragma ident	"@(#)locale.c	1.37	97/01/09 SMI"

/* Copyright */


/* Includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/StaticText.h>
#include "props.h"

#ifdef DBMALLOC
#include <dbmalloc.h>
#endif

#undef DEBUGMSGS     /* print debugging messages?  */
#undef DEBUGMSGS_2   /* print even more debugging messages?  */


/***************************************************************************
* Typedefs
***************************************************************************/

#define MAX_LINE_LENGTH 4096

typedef enum {
    COMMENT,
    BASIC,
    INPUT,
    DISPLAY,
    TIME,
    NUMERIC
} LineType;
 
typedef struct _LocaleLabel {
    char                *locale;  /* two letter name of the locale */
    char                *label;   /* text displayed on the menu button */
    Boolean		 defaultItem; /* is this item the default setting ? */
    Boolean		 isAvailable; /* is this locale actually installed? */
    struct _LocaleLabel *next;    /* next item in list */
} LocaleLabel;
 
typedef struct _LocaleErrors {
    String	noLocaleFile;
} LocaleErrors, *LocaleErrorsPtr;

LocaleErrors	localeErrors;

static XtResource appResources[] = {
    { "noLocaleFile", "NoLocaleFile", XtRString, sizeof(String),
      XtOffset(LocaleErrorsPtr, noLocaleFile), XtRImmediate,
      "Missing locale file for \"%s\" locale. Using C locale."
    }
};


/***************************************************************************
* Resource Names and module static data
***************************************************************************/

static const char
    BASIC_LOCALE_RES[]		= "OpenWindows.BasicLocale",
    DISPLAY_LANGUAGE_RES[]	= "OpenWindows.DisplayLang",
    INPUT_LANGUAGE_RES[]	= "OpenWindows.InputLang",
    TIME_FORMAT_RES[]		= "OpenWindows.TimeFormat",
    NUMERIC_FORMAT_RES[]	= "OpenWindows.NumericFormat";


static Widget	 localeCategory;

static Widget	 localeMenuPaneArray[5];  /* storage for keeping a handle to
				 	     the menuPane widgets */
Widget		*panePtr;

static Widget	 localeAbbrevArray[5],    /* to store the five abbrev menu
					     widgets, so we can move the old
					     menuPanes out and create new ones
					     when the buttonSetting callback
					     happens  */

		 localePreviewArray[5],
		 localeCaptionArray[5];	  /* store these too */
    
static char	 basicLocale[20];	  /* can only support one basic_setting
					     locale per execution of props. */


/***************************************************************************
* Function Declarations
***************************************************************************/

static void localeCreateChangeBar(Widget, int, Boolean);

/*
 * Forward declarations - these had to be declared because of circular
 * dependencies. 
 */

static Widget	localeCreateCategory(Widget);

static void	basicSettingCallback(
    Widget, 
    XtPointer,
    XtPointer);

static void	localeAbbreviatedMenuCallback(
    Widget,
    XtPointer,
    XtPointer);

static void	localeReadDbSettings(Widget, int, Boolean);

/***************************************************************************
* Registration Data & Function
***************************************************************************/

static CategoryInfo info = {
    "locale",
    localeCreateCategory,
    wspCreateChangeBars,
    wspDeleteChangeBars,
    wspBackupSettings,
    wspRestoreSettings,
    localeReadDbSettings,
    wspSaveDbSettings,
    wspNoopRealizeSettings,
    wspNoopInitializeServer,
    wspNoopSyncWithServer,
    wspNoopShowCategory,
    wspNoopHideCategory
};

CategoryInfo*
localeRegisterCategory(void)
{
    return &info;
} 

/***************************************************************************
 * Locale Category Widget Tree
 * --------------------------
 * mainControl
 *   localeCategory
 *	basicCaption
 *	    basicControl
 *		basicPreview
 *		basicAbbrev
 *		   basicExclusive
 *			locale00
 *			 . . .
 *			localenn
 *	(blankLine)
 *	displayCaption
 *	    displayControl
 *		displayPreview
 *		displayAbbrev
 *		   displayExclusive
 *			locale00
 *			 . . .
 *			localenn
 *	inputCaption
 *	    inputControl
 *		inputPreview
 *		inputAbbrev
 *		   inputExclusive
 *			locale00
 *			 . . .
 *			localenn
 *	timeCaption
 *	    timeControl
 *		timePreview
 *		timeAbbrev
 *		   timeExclusive
 *			locale00
 *			 . . .
 *			localenn
 *	numericCaption
 *	    numericControl
 *		numericPreview
 *		numericAbbrev
 *		   numericExclusive
 *			locale00
 *			 . . .
 *			localenn
 *
 **************************************************************************/

/* ARGSUSED */
static void
localeReadDbSettings(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    WidgetList	children;
    String	label;
    Cardinal	numChildren;
    Widget	menuPane;	/* Looks like localeMenuPaneArray isn't set up*/
    Boolean	sensitive;
    int		ii, jj, unAvailableLocaleIndex;
    ItemInfo   *itemInfo;
    char        tmpDomain[MAX_LINE_LENGTH];
    char       *p1;
    char       *unAvailableLocale;

    wspReadDbSettings(widget, dbAccess, preserve);

    for (ii = 0; ii < 5; ii++) { /* REMIND: 5 should be a define */
	XtVaGetValues(localeAbbrevArray[ii],
	    XtNuserData,    (XtArgVal)&itemInfo,
	    NULL);
        if (itemInfo != NULL) {
	    XtVaGetValues(localeAbbrevArray[ii],
		XtNmenuPane,    (XtArgVal)&menuPane,
		NULL);
	    if (menuPane != NULL) {
                XtVaGetValues(menuPane, /* Could use localeMenuPaneArray here?*/
		    XtNchildren,    (XtArgVal)&children,
		    XtNnumChildren, (XtArgVal)&numChildren,
		    NULL);
		if (numChildren == 1 && children[0] != NULL) {
		    XtVaGetValues(children[0], /* exclusive widget */
			XtNchildren,    (XtArgVal)&children,
			XtNnumChildren, (XtArgVal)&numChildren,
			NULL);
	            if (itemInfo->currVal < numChildren &&
		        children[itemInfo->currVal] != NULL) {
		
		        XtVaGetValues(children[itemInfo->currVal],
		            XtNsensitive, (XtArgVal)&sensitive,
		            NULL);
		        if (!sensitive) { /* Current selection is grayed out */
		            /*
		             * Default to C locale - assuming last setting for
			     * strtok thru the itemIndo domain string
			     * until we find "C" and set currVal to the
			     * index at that point.
		             */
				unAvailableLocaleIndex = itemInfo-> currVal;
				jj = 0;
				(void) strcpy(tmpDomain, itemInfo->domain);
				p1 = strtok(tmpDomain, ":\n");
				while (p1 != NULL) {
				    if (strcmp(p1, "C") == 0) {
					itemInfo->currVal = jj;
					XtVaSetValues(children[jj],
					     XtNset, (XtArgVal)TRUE, NULL);
			    		XtVaGetValues(children[jj], XtNlabel,
					    (XtArgVal)&label, NULL);
			    		XtVaSetValues(localePreviewArray[ii], 
					    XtNstring, label, NULL);
				    }
				    if (jj == unAvailableLocaleIndex)
					unAvailableLocale = XtNewString(p1);
				    p1 = strtok(NULL, ":\n");
				    jj++;
				}

		           /*
		            * REMIND: Need to provide informative error message
		            * using the foundation error mechanism.
		            */

		/*
		 *      wspErrorFooter("Locale unavailable. Using C.", NULL);
		 */

                    wspErrorFooter(localeErrors.noLocaleFile, 
					unAvailableLocale, NULL);
			}
		    }
	        }
	    }
	}
    }
}


/***************************************************************************
* Private Locale Category Functions
***************************************************************************/

/*
 * prints out all the key/value pairs from a linked list of LocaleLabel
 * structures.  Used for debugging only.
 */
#ifdef DEBUGMSGS
void
printBasic(LocaleLabel *list)
{
    LocaleLabel *thing;

    thing = list;
    while (thing != NULL) {
        (void) printf("locale: %s    label: %s\n", thing->locale, thing->label);
	(void) printf("    defaultItem: %d\n", thing->defaultItem);
	(void) printf("    isAvailable: %d\n", thing->isAvailable);
	thing = thing->next;
    }
}
#endif


static char *
getDefault(char *line)
{
    char  *tmp, *p1, *p2;

    tmp = XtNewString(line);
    for (p1 = tmp; *p1 != '='; p1++) {
	/* empty loop */ ;
    }
    p1++; /* move past "=" */
    p2 = strchr(p1, ';');
   *p2 = NULL;
#ifdef DEBUGMSGS_2
    (void) printf("getDefaults: line = %s, tmp = %s, p1 = %s\n", line, tmp, p1);
#endif
    p1 = XtNewString(p1);
    XtFree(tmp);
    return p1;
} 


static LineType
getLineType(const char *line)
{
        LineType        lineType;
 
 
	if (strncmp(line, "basic_setting", 13) == 0) {
                lineType = BASIC;
        } else if (strncmp(line, "input_language", 14) == 0) {
                lineType = INPUT;
        } else if (strncmp(line, "display_language", 16) == 0) {
                lineType = DISPLAY;
        } else if (strncmp(line, "time_format", 11) == 0) {
                lineType = TIME;
        } else if (strncmp(line, "numeric_format", 14) == 0) {
                lineType = NUMERIC;
        } else {
		lineType = COMMENT;  /* we don't care what it is */
	}

        return lineType;
}


/*
 * takes string as argument, and decomposes it into key/value
 * pairs for use as menu labels.  String is of the format
 * "C|USA;ja|Japan;ko|Korean" etc.  "|" separates the locale
 * name from the string used in the label, and ";" separates
 * the multiple key/value pairs.
 */
static LocaleLabel *
readData(char *line)
{
    char 	*p1, *p2, *p3, *tmp, path[MAXPATHLEN];
    const char  *openwinDir;
    LocaleLabel *list, *prevItem, *head;
    struct stat	 osBuffer, windowsBuffer;	     /* stat() buffers */
    int		 osRet, windowslibRet, windowsRet;   /* stat() return values */


    /*
     * now decompose the string into pairs with strtok, then pop 'em 
     * into the link list. 
     */

    openwinDir = wspGetOpenwinhome();

    p1 = line;
    p2 = strtok(p1, ";\n"); /* skip over first token. separate on either item */
    prevItem = NULL;
    head = NULL;
    while ((p2 = strtok(NULL, ";\n")) != NULL) {
        list = malloc(sizeof(LocaleLabel));
        p3 = strchr(p2, '|');
       *p3++ = NULL;

	tmp = XtMalloc(strlen(p2) + strlen(p3) + 4);  /* lv room for " ()\n" */
	(void) strcpy(tmp, p3);
	(void) strcat(tmp, " (");
	(void) strcat(tmp, p2);
	(void) strcat(tmp, ")");
        list->locale = XtNewString(p2);
        list->label  = XtNewString(tmp);
	list->defaultItem = FALSE;
	list->isAvailable = FALSE; 
	/*
	 * Look in
	 *	/usr/lib/locale/<locale>
	 *	/usr/openwin/lib/locale/<locale>
	 *	/usr/openwin/share/locale/<locale>
	 * to verfiy the locale is installed.
	 */
        (void) sprintf(path, "%s/share/locale/%s", openwinDir, p2);
	windowsRet = stat(path, &windowsBuffer);
        (void) sprintf(path, "/usr/lib/locale/%s", p2);
	osRet = stat(path, &osBuffer);
	(void) sprintf(path, "%s/lib/locale/%s", openwinDir, p2);
	windowslibRet = stat(path, &windowsBuffer);
	if ((osRet == 0) && (windowsRet == 0) && (windowslibRet == 0)) {
	    if (S_ISDIR(osBuffer.st_mode) && S_ISDIR(windowsBuffer.st_mode)) { 
		list->isAvailable = TRUE;
	    }
	}
	if (prevItem == NULL){
	    head = list;  /* first time thru, save this address  */
	} else {
	    prevItem->next = list;
	}
	list->next = NULL;
	prevItem = list;
    }

    p2 = strchr((list->label), '\n');
    if (p2 != NULL)
	*p2 = NULL;  /* Terminate string with <NULL>, instead of <\n><NULL>. */

    return head;
}


static LocaleLabel *
getSetting(char *locale, char *filename, LineType setting)
{
	const char     *openwinDir;
        char            fullpath[MAXPATHLEN];
        FILE           *configFile;
        char            line[MAX_LINE_LENGTH+1];
        LineType        lineType;
        LocaleLabel    *bs,
		       *labelptr;
        char           *defaultLocale;
	Boolean		found;
 
	openwinDir = wspGetOpenwinhome();

        if (strcmp(locale, basicLocale) != 0 )
		strcpy(locale, basicLocale);
 
        (void) sprintf(fullpath, "%s/share/locale/%s/props/%s",
		       openwinDir, locale, filename);
 	configFile = fopen(fullpath, "r");
        if (configFile == NULL) {
		wspErrorFooter(localeErrors.noLocaleFile, locale, NULL);
		/*
		 * Didn't find requested locale. So we default to C.
		 * if the filename is "basic_setting", all is okay, since
		 * this is the right file to read for the basic setting
		 * menu. For all the other menus, however, thr right name
		 * is someting like usr/openwin/share/locale/<locale>/props/C.
		 * unless we only *have* the C locale. So we need to try to
		 * open the <locale> version, and if that fails we hardwire
		 * it to the C locale.  If that fails, we just bail out.
		 */
		(void) sprintf(fullpath, "%s/share/locale/C/props/%s",
			       openwinDir, filename);
		configFile = fopen(fullpath, "r");
		if (configFile == NULL) {	
		    if (strncmp(filename, "basic_setting", 13) != 0) 
			(void) strcpy(filename, "C");
		    (void) sprintf(fullpath, "%s/share/locale/C/props/%s",
				   openwinDir, filename);
		    configFile = fopen(fullpath, "r");
                    if (configFile == NULL) {
			/* Real trouble.  If C doesn't exist, bail out. */
		        wspErrorStandard("getSetting()",
		        		 localeErrors.noLocaleFile, "C", NULL);
			/* Should try to get back to aother panel - how? */
			exit(1);
		    }
		}

        }
 
        /* file opened. Now read some stuff out of it.  */
 
	bs 	= NULL;
        while ( fgets(line, MAX_LINE_LENGTH, configFile) != NULL) {
 
                lineType = getLineType(line);
 
                switch(lineType) {
 
                    case COMMENT:
                        break;
  
                    case BASIC:
                    case INPUT:
                    case DISPLAY:
                    case TIME:
                    case NUMERIC:
			if(lineType == setting) {
                             defaultLocale = getDefault(line);
                             bs = readData(line);
			}
                        break;
 
                    default:
                        (void)fprintf(stderr,"  Syntax error in file %s: Unrecognized line: %s\n", fullpath, line);
                        break;
                } /* switch */
 
 
        } /* while */
 
        /* Close the file, please. */
 
        (void)fclose(configFile);
 
	found = FALSE;
#ifdef DEBUGMSGS_2
	if (strcmp(defaultLocale, "de") == 0 )
		(void)strcpy(defaultLocale, "C");
	printf("getSetting: defaultLocale is %s\n", defaultLocale);
	fflush(stdout);
#endif

	for (labelptr = bs; labelptr != NULL; labelptr = labelptr->next) {
#ifdef DEBUGMSGS_2
	    (void)printf("defaultLocale = |%s|, labelptr->locale == |%s|\n", defaultLocale, labelptr->locale);
#endif
	    if ((strcmp(defaultLocale, labelptr->locale) == 0) && 
		labelptr->isAvailable) {
		    labelptr-> defaultItem = TRUE;
		    found = TRUE;
#ifdef DEBUGMSGS_2
		    printf("Found == true\n");
		    fflush(stdout);
#endif
	    }
	}

	if (!found) { 
#ifdef DEBUGMSGS_2
	    printf("found not true : Basic Locale: %s\n", basicLocale);
	    fflush(stdout);
#endif
	    for (labelptr = bs; labelptr != NULL; labelptr = labelptr->next) {
		if (strcmp(basicLocale, labelptr->locale) == 0) 
			labelptr-> defaultItem = TRUE;
	    }
	}

	if (!found) {
	  (void)strcpy(defaultLocale,"C");
	  (void)strcpy(basicLocale,"C");
	  for (labelptr = bs; labelptr != NULL; labelptr = labelptr->next) {
	    if ((strcmp(defaultLocale, labelptr->locale) == 0) && 
		labelptr->isAvailable) {
	      labelptr-> defaultItem = TRUE;
	      found = TRUE;
	    }
	  }
	}

#ifdef DEBUGMSGS
	printBasic(bs);
#endif
        return (bs);
 
}


static Widget
createButtons(
	LocaleLabel    *buttons, 
	Widget		abbrev, 
	Widget		preview,
	Widget		caption,
	char           *label, 
	const char     *Resource) 
{
	char		domain[MAX_LINE_LENGTH],
			name[MAXPATHLEN];
	int		ii,
			defaultItem,
			numButtons,
			screen_height,
			max_rows,
			num_columns;
	ItemInfo       *itemInfo;
	LocaleLabel    *ptr;
	Widget		menuButton, 
			menuPane, 
			exclusive;
	String		previewLabel;
	WidgetList	children;
	Cardinal	numChildren;
	Dimension	bheight = 0;

	domain[0] = NULL;
	numButtons = 0;
	defaultItem = -1;  /*  REMIND - test for this and handle error. */
	for (ptr = buttons; ptr != NULL; ptr = ptr->next) {
	    numButtons++;
	    (void) strcat(domain, ptr->locale);
	    (void) strcat(domain, ":");
	    if (ptr->defaultItem)
		defaultItem = numButtons-1;
	}
	/* Slice off the last ":" from the string. */
	domain[strlen(domain) -1] = NULL;
#ifdef DEBUGMSGS
	    (void) printf("create buttons: domainstring = |%s|\n", domain);
	    (void) printf("create buttons: numButtons = |%d|\n", numButtons);
#endif
 
	/* See if there is already an iteminfo on the caption */
	/* If there isn't, we'll create one. */
	XtVaGetValues(caption, XtNuserData, &itemInfo, NULL);
	if (itemInfo == NULL) {
	    itemInfo = wspNewItemInfo(0, Resource, dString,
                    XtNewString(domain), 0, caption); 
	    XtVaSetValues(caption, XtNuserData, (XtArgVal)itemInfo, NULL);
	    XtVaSetValues(abbrev,  XtNuserData, (XtArgVal)itemInfo, NULL);
	} else {
	    XtFree((char *)itemInfo->domain);
	    itemInfo->domain = XtNewString(domain);
	}

	itemInfo->currVal = defaultItem;

	XtVaGetValues(abbrev, XtNmenuPane, (XtArgVal)&menuPane, NULL);

	XtVaGetValues(menuPane, XtNchildren,    &children, 
				XtNnumChildren, &numChildren,
				NULL);
	if (numChildren == 1)
		XtDestroyWidget(children[0]);
	/*
	 * We assume that the child is the exclusive, and that we are 
	 * destroying it just before we create another one.
	 */

        (void) sprintf(name, "%sExclusive", label);
        exclusive = XtVaCreateManagedWidget(name,
                    exclusivesWidgetClass, menuPane,
                    NULL);

	ptr = buttons;
	for (ii = 0; ii < numButtons ; ii++) {
	    Dimension theight;

	    (void) sprintf(name, "locale%02d", ii);
	    menuButton = XtVaCreateManagedWidget(name,
	        rectButtonWidgetClass, exclusive,
	        XtNlabel, ptr->label,
	        XtNuserData, (XtArgVal)itemInfo,
	        NULL);
	    XtAddCallback(menuButton, XtNselect, wspChangeSettingCb,
	    		  (XtPointer)ii);
	    XtAddCallback(menuButton, XtNselect, localeAbbreviatedMenuCallback,
	    		  (XtPointer)label);
	    if (strcmp(label,"basic") == 0) {
	        XtAddCallback(menuButton, XtNselect,
	        	      (XtCallbackProc)basicSettingCallback,
	        	      (XtPointer)preview);
	    }
	    if (ptr->isAvailable == FALSE)
	        XtSetSensitive(menuButton, FALSE);
	    if (ii == defaultItem) {
	        XtVaGetValues(menuButton, XtNlabel,
	            (XtArgVal)&previewLabel, NULL);
	        XtVaSetValues(preview, XtNstring,
	            (XtArgVal)previewLabel, NULL);
		XtVaSetValues(menuButton, XtNset, TRUE, NULL);
	    }
	    ptr = ptr->next;
   	    XtVaGetValues (menuButton,
		XtNheight, &theight,
		NULL);
	    if (theight > bheight)
		bheight = theight;
	}

	/*
	   The following code is to prevent the locales menu from
	   extending off the screen due to there being too many choices to
	   display on the screen in a single column. This is probably an
	   OLIT menuPane bug. This workaround relies on getting reliable
	   values for the button heights (bheight). The MAX_ROWS value defined
	   below is totally arbitrary and is done to protect against unreliable 
	   bheight values. Bug # 1249899.
	*/
#define MAX_ROWS 35
	screen_height = DisplayHeight (XtDisplay (exclusive),
		DefaultScreen (XtDisplay (exclusive) ) );
	max_rows = screen_height / bheight - 1;
	if (max_rows < 1) max_rows = 1;
	if (max_rows > MAX_ROWS) max_rows = MAX_ROWS;
#undef MAX_ROWS

	/* We specify the number of columns to get an even distribution
	   of buttons. */
	num_columns = numButtons / max_rows;
	if ( (num_columns * max_rows) < numButtons) num_columns++;
	XtVaSetValues (exclusive,
		XtNlayoutType, OL_FIXEDCOLS,
		XtNmeasure, num_columns,
		NULL);
	return exclusive;
}


static Widget
localeCreateAbbreviatedMenu(
	Widget		parent,
	char	       *label,
	LocaleLabel    *buttons,  /* data structure containing the labels */
	int	       *buttonSet,
	const char     *Resource
)
{
	Widget		caption,
			control,
			abbrev,
			preview;
	char		name[80];
	static int	Index = 0;
	
	*buttonSet = -1;
	(void) sprintf(name, "%sCaption", label);

    	caption = XtVaCreateManagedWidget(name,
		    captionWidgetClass, parent,
		    NULL);
	localeCaptionArray[Index] = caption;

	(void) sprintf(name, "%sControl", label);
    	control = XtVaCreateManagedWidget(name,
		    controlAreaWidgetClass, caption,
		    NULL);
	(void) sprintf(name, "%sAbbrev", label);
    	abbrev = XtVaCreateManagedWidget(name,
		    abbrevMenuButtonWidgetClass, control,
		    NULL);
	localeAbbrevArray[Index] = abbrev;
	(void) sprintf(name, "%sPreview", label);
    	preview = XtVaCreateManagedWidget(name,
		    staticTextWidgetClass, control,
		    NULL);
	localePreviewArray[Index] = preview;

	/* Pass off the rest of the creation duties to createButtons. */

	(void)createButtons(buttons, abbrev, preview, caption, label, Resource);
	Index++;

	/* Done: now return the top of this subtree of widgets. */

	return caption;
}


static Widget
localeCreateCategory(Widget mainControl)
{
    Widget	basicSetting,
    		displayLocale,
		inputLocale,
		timeFormat,
		numericFormat;
    int		buttonNumberSet,
    		dbAccess;
    char	locale[20];
    LocaleLabel	     *bs,	/* basic setting */
    		     *dl,	/* display language */
    		     *il,	/* input language */
    		     *tf,	/* time format */
    		     *nf;	/* numeric format */
    XrmValue	value;

    localeCategory = XtVaCreateManagedWidget("localeCategory",
            controlAreaWidgetClass, mainControl,
	    XtNmappedWhenManaged,   FALSE,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)localeCategory, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtGetApplicationResources(localeCategory, &localeErrors, appResources,
	    XtNumber(appResources), NULL, 0);

    dbAccess = ACCESS_ALL_DB ;


    if (wspGetResource(BASIC_LOCALE_RES, dbAccess, &value))
	(void) strcpy(locale, (char *)value.addr);
    else
        strcpy (locale, wspGetLocale());
    (void) strcpy(basicLocale, locale); /* cache this locale value */
    bs = getSetting(locale, "basic_setting", BASIC);
    basicSetting = localeCreateAbbreviatedMenu (localeCategory,
		"basic", bs, &buttonNumberSet, BASIC_LOCALE_RES); 
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)basicSetting, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    /* Create a blank space, separating Basic Setting from the next 4 settings*/
    (void) XtVaCreateManagedWidget("blankLine",
	    captionWidgetClass, localeCategory,
	    NULL);

    if (wspGetResource(DISPLAY_LANGUAGE_RES, dbAccess, &value))
	(void) strcpy(locale, (char *)value.addr);
    else
	(void) strcpy(locale, basicLocale);
    dl = getSetting(locale, locale, DISPLAY);
    displayLocale = localeCreateAbbreviatedMenu (localeCategory,
                "display", dl, &buttonNumberSet, DISPLAY_LANGUAGE_RES);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)displayLocale, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
 
    if (wspGetResource(INPUT_LANGUAGE_RES, dbAccess, &value))
	(void) strcpy(locale, (char *)value.addr);
    else
	(void) strcpy(locale, basicLocale);
    il = getSetting(locale, locale, INPUT);
    inputLocale = localeCreateAbbreviatedMenu (localeCategory,
                "input", il, &buttonNumberSet, INPUT_LANGUAGE_RES);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)inputLocale, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
 
    if (wspGetResource(TIME_FORMAT_RES, dbAccess, &value))
	(void) strcpy(locale, (char *)value.addr);
    else
	(void) strcpy(locale, basicLocale);
    tf = getSetting(locale, locale, TIME);
    timeFormat = localeCreateAbbreviatedMenu (localeCategory,
                "time", tf, &buttonNumberSet, TIME_FORMAT_RES);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)timeFormat, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
 
    if (wspGetResource(NUMERIC_FORMAT_RES, dbAccess, &value))
	(void) strcpy(locale, (char *)value.addr);
    else
	(void) strcpy(locale, basicLocale);
    nf = getSetting(locale, locale, NUMERIC);
    numericFormat = localeCreateAbbreviatedMenu (localeCategory,
                "numeric", nf, &buttonNumberSet, NUMERIC_FORMAT_RES);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)numericFormat, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
 
    return localeCategory;
} 


/*
 ******************************
 *  Callback functions
 ******************************
 */

/*ARGSUSED*/
static void
localeAbbreviatedMenuCallback(
	Widget		 widget,
	XtPointer	 clientData,
	XtPointer	 callData)
{
        char       	*namePrefix = (char*)clientData;
        String           label;
        Widget           preview;
        char             name[80];
 
        XtVaGetValues(widget, XtNlabel, (XtArgVal)&label, NULL);
        (void) sprintf(name, "*%sPreview", namePrefix);
        preview = XtNameToWidget(localeCategory, name);
        XtVaSetValues(preview, XtNstring, label, NULL);
}


/*
 * Basic_Setting has been changed, so we must re-read the data files for
 * the other four menus, and change the menus accordingly.
 */

/*ARGSUSED*/
static void
basicSettingCallback(
	Widget		widget,
	XtPointer	 clientData,
	XtPointer	 callData)

{
	int 	 	 jj;
	char 		*newLocale,
		 	*locales;
	ItemInfo    	*itemInfo;
	LocaleLabel	*bs;


	/*
	 * Build LocaleLabel structs for each of the settings
	 * create four new itemInfos, and four new Exclusives
	 * and as many rectButtons as we need, then hang them 
	 * off of the existing Abbrev widgets. whew!
	 */

#ifdef DEBUGMSGS_2
	(void) printf("in basicSettingCallback\n");
	(void) fflush(stdout);
#endif

	for (jj = 0; jj < 5; jj++) {
            XtVaGetValues(localeAbbrevArray[jj], 
                    XtNmenuPane, (XtArgVal)&localeMenuPaneArray[jj],
                    NULL);
	}

	/*
	 * Get iteminfo from our basicsetting widget.  Find new locale
	 * to switch menus to via currVal & domain string.
	 */
	XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
	locales = XtNewString(itemInfo->domain);
	newLocale = strtok(locales, ":");
	for (jj = 0; jj < (itemInfo->currVal); jj++)
		newLocale = strtok(NULL, ":");

	bs = getSetting(basicLocale, newLocale, DISPLAY);
	(void) createButtons(bs, localeAbbrevArray[1], 
		localePreviewArray[1], localeCaptionArray[1],
		"display", DISPLAY_LANGUAGE_RES);
	localeCreateChangeBar(localeCaptionArray[1], ACCESS_ALL_DB, True);

	bs = getSetting(basicLocale, newLocale, INPUT);
	(void) createButtons(bs, localeAbbrevArray[2], 
		localePreviewArray[2], localeCaptionArray[2],
		"input", INPUT_LANGUAGE_RES);
	localeCreateChangeBar(localeCaptionArray[2], ACCESS_ALL_DB, True);

	bs = getSetting(basicLocale, newLocale, TIME);
	(void) createButtons(bs, localeAbbrevArray[3], 
		localePreviewArray[3], localeCaptionArray[3],
		"time", TIME_FORMAT_RES);
	localeCreateChangeBar(localeCaptionArray[3], ACCESS_ALL_DB, True);

	bs = getSetting(basicLocale, newLocale, NUMERIC);
	(void) createButtons(bs, localeAbbrevArray[4], 
		localePreviewArray[4], localeCaptionArray[4],
		"numeric", NUMERIC_FORMAT_RES);
	localeCreateChangeBar(localeCaptionArray[4], ACCESS_ALL_DB, True);
	
	XtFree(locales);

	return;
}

/* End callback functions. */


/*
 * Convenience function, stolen from props.c,
 * where it is called propsCreateChangeBar.
 */

/*
 * localeCreateChangeBar
 * 
 * Turn on the change bar for the widget if the current value is different
 * from the backup value.
 */

/*ARGSUSED*/
static void
localeCreateChangeBar(
    Widget  widget,
    int     dbAccess,
    Boolean preserve)
{
    ItemInfo *itemInfo;
    OlDefine  changeBar;

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&itemInfo, NULL);
    if (itemInfo == NULL) {
        return;
    }   
 
    XtVaGetValues(itemInfo->caption,
            XtNchangeBar, (XtArgVal)&changeBar,
            NULL);
    if (changeBar != OL_NORMAL) /* If change bar is not on */
        XtVaSetValues(itemInfo->caption,
                XtNchangeBar, (XtArgVal)OL_NORMAL,
                NULL);
}

