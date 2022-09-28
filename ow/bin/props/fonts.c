#pragma ident	"@(#)fonts.c	1.53	93/08/27 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>
#include <Xol/ScrollingL.h>
#include "props.h"
#include "preview.h"
#include "fonts.h"
#include "OWFsetDB.h"

#ifdef DBMALLOC
# include <dbmalloc.h>
#endif

/***************************************************************************
* Resource Names
***************************************************************************/
static const char  FONTS_SCALE_RES[]       =	"OpenWindows.Scale";
static const char  FONTS_TITLEFONT_RES[]   =	"OpenWindows.TitleFont";
static const char  FONTS_BUTTONFONT_RES[]  =	"OpenWindows.ButtonFont";
static const char  FONTS_TEXTFONT_RES[]    =	"OpenWindows.TextFont";
static const char  FONTS_REGULARFONT_RES[] =	"OpenWindows.RegularFont";
static const char  FONTS_BOLDFONT_RES[]    =	"OpenWindows.BoldFont";
static const char  FONTS_MONOSPFONT_RES[]  =	"OpenWindows.MonospaceFont";

static const char *DEFAULT_FONTS_RES[]     = {
    "*fontsCategory*defaultFont0",
    "*fontsCategory*defaultFont1",
    "*fontsCategory*defaultFont2",
    "*fontsCategory*defaultFont3",
    NULL
};

static const char *FONTS_POINTSIZE_RES[]   = {
    "*fontsCategory*pointSizeSmall",
    "*fontsCategory*pointSizeMedium",
    "*fontsCategory*pointSizeLarge",
    "*fontsCategory*pointSizeXLarge"
};

/***************************************************************************
* Function Declarations
***************************************************************************/
/*
 * Fonts category functions
 */
static Widget    fontsCreateCategory(Widget);
CategoryInfo    *fontsRegisterCategory(void);
static void      fontsSaveDbSettings(Widget, int);
static void      fontsReadDbSettings(Widget, int, Boolean);
static void      fontsCreateChangeBars(Widget);
static void      fontsDeleteChangeBars(Widget);
static void      fontsBackupSettings(Widget, Boolean);
static void      fontsRestoreSettings(Widget);
/*
 * Fonts category supporting functions
 */
static void      changeChoicesCB(Widget, XtPointer, XtPointer);
static void      changePointSizeCB(Widget, XtPointer, XtPointer);
static void      selectFontFamilyCB(Widget, XtPointer, XtPointer);
static void      unselectFontFamilyCB(Widget, XtPointer, XtPointer);
static void      updateSlistContents(void);
static Boolean   updateSlistSensitivity(void);
static void      updatePreviewWindow(void);
static void      updateButtonSensitivity(void);
static void      initFontData(void);

/***************************************************************************
* Registration Data & Function
***************************************************************************/
static CategoryInfo info = {
    "fonts",
    fontsCreateCategory,
    fontsCreateChangeBars,
    fontsDeleteChangeBars,
    fontsBackupSettings,
    fontsRestoreSettings,
    fontsReadDbSettings,
    fontsSaveDbSettings,
    wspNoopRealizeSettings,
    wspNoopInitializeServer,
    wspNoopSyncWithServer,
    wspNoopShowCategory,
    wspNoopHideCategory
};

CategoryInfo*
fontsRegisterCategory(void)
{
    return(&info);
}

/***************************************************************************
* Typedefs
***************************************************************************/

/*
 * Used to keep track of the font names associated with each font family,
 * and which are the best to use for each ScaleType.
 */
typedef struct _fontInfo {
    char             *familyLabel,

	             *bestRegular[ScaleMax],
	             *bestBold[ScaleMax],
	             *bestMonosp[ScaleMax];

    int               scoreBestRegular[ScaleMax],
	              scoreBestBold[ScaleMax],
	              scoreBestMonosp[ScaleMax];

    Boolean	      isStandardP; /* Is this a "standard" font? */
    OlSlistItemPtr    slistItem;   /* The scrolling list item for this font */
    struct _fontInfo *next;        /* The next record in the list */
} fontInfo;

/***************************************************************************
* Module Static Data
***************************************************************************/
/*
 * Sizes of arrays
 */
#define BIGNUM 32767
#define MAX_XLFD_LEN 255

/*
 * Avoid walking the widget tree to find these.
 */
static Widget    choicesButton,
		 typefaceScList,
	         typefaceCaption,
	         scaleCaption,
		 scaleButton[ScaleMax];
static ItemInfo *scaleItemInfo;

/*
 * Various internal state variables
 *
 * Note that currentPointSize must be initialized to an invalid value,
 * otherwise fontsReadDbSettings() won't invoke updateSlistSensitivity() when
 * the fonts panel is first created.
 */
static int       currentPointSize                   = 0,
		 backupPointSize                    = 0;
static ScaleType currentScale			    = ScaleMax,
		 backupScale			    = ScaleMax;
static Boolean   allowAllChoices                    = False,
		 useFontSets                        = False;
static char     *currentFamily                      = NULL,
		*backupFamily                       = NULL,
		 currentFontRegular[MAX_XLFD_LEN+1] = "",
	         currentFontBold[MAX_XLFD_LEN+1]    = "",
	         currentFontMonoSp[MAX_XLFD_LEN+1]  = "";
static PreviewInfo *previewInfo;

/*
 * Persistent databases
 */
static OWFsetDB	 fontSetDB;
static fontInfo *fontInfoList                       = NULL;
static fontInfo *defaultFont                        = NULL;

/*
 * Error Message Information
 */

typedef struct _FontsErrors {
    String usingDefaultFont;
    String usingDefaultDataFont;
    String fontNotFound;
    String badPointSize;
} FontsErrors, *FontsErrorsPtr;

static FontsErrors fontsErrors;

static XtResource appResources[] = {
    {"usingDefaultFont", "UsingDefaultFont", XtRString, sizeof(String),
     XtOffset(FontsErrorsPtr, usingDefaultFont), XtRImmediate,
     "Using default font."
    },
    {"usingDefaultDataFont", "UsingDefaultDataFont", XtRString, sizeof(String),
     XtOffset(FontsErrorsPtr, usingDefaultDataFont), XtRImmediate,
     "Using default font in Data Areas."
    },
    {"fontNotFound", "FontNotFound", XtRString, sizeof(String),
     XtOffset(FontsErrorsPtr, fontNotFound), XtRImmediate,
     "Font no longer available: %s"
    },
    {"badPointSize", "BadPointSize", XtRString, sizeof(String),
     XtOffset(FontsErrorsPtr, badPointSize), XtRImmediate,
     "Invalid point size (%d) encountered."
    }
};

/***************************************************************************

	       Fonts Category Widget Tree
	       --------------------------

	mainControl
		fontsCategory
			choices
				exclusive
					choicesButton x2
			typefaceCaption
				typefaceScList
			scaleCaption
				exclusive
					scaleButton[ii] x4
			preview

***************************************************************************/

/***************************************************************************
* Private Fonts Category Functions
***************************************************************************/

static const char *buttonNames[] = {
    "button00",
    "button01",
    "button02",
    "button03"
};

static int pointSizeFromScale[] = {
    10,
    12,
    14,
    19
};

/*
 * scaleFromPointSize returns
 *	ScaleScalable			  pointSize == 0
 *	ScaleSmall, ..., ScaleExtraLarge  pointSize matches a standard pointsize
 *	ScaleMax			  any other value for pointSize
 */
static ScaleType
scaleFromPointSize(
    int       pointSize)
{
    ScaleType sc;

    if (pointSize == 0) {
	return(ScaleScalable);
    }
    for (sc = ScaleSmall; sc < ScaleScalable; sc++) {
	if (pointSizeFromScale[sc] == pointSize) {
	    return(sc);
	}
    }
    return(ScaleMax);
}

static Widget
fontsCreateCategory(
    Widget    mainControl)
{
    Widget    fontsCategory,
	      choices,
	      exclusive;
    int       ii;

    /*
     * Init fonts data and scrolling list
     */
    initFontData();

    /*
     * Create fonts control panel and subpanels
     */
    fontsCategory = XtVaCreateManagedWidget("fontsCategory",
            controlAreaWidgetClass, mainControl,
	    XtNmappedWhenManaged,   FALSE,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)fontsCategory, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtGetApplicationResources(fontsCategory, &fontsErrors, appResources,
	    XtNumber(appResources), NULL, 0);

    /*
     * Create "Choices" exclusive choice
     */
    choices = XtVaCreateManagedWidget("fontChoices",
	    captionWidgetClass, fontsCategory,
	    NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)choices, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    exclusive = XtVaCreateManagedWidget("exclusive",
	    exclusivesWidgetClass, choices,
	    NULL);

    for (ii = 0; ii < 2; ii++) {
        choicesButton = XtVaCreateManagedWidget(buttonNames[ii],
	        rectButtonWidgetClass, exclusive,
	        NULL);
        XtAddCallback(choicesButton, XtNselect,
		changeChoicesCB, (XtPointer)ii);
    }

    /*
     * Create "Typeface" scrolling list
     */
    typefaceCaption = XtVaCreateManagedWidget("fontTypefaces",
	    captionWidgetClass, fontsCategory,
	    NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)typefaceCaption, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    typefaceScList = XtVaCreateManagedWidget("slist",
	    scrollingListWidgetClass, typefaceCaption,
	    XtNscrollingListMode, OL_EXCLUSIVE_NONESET,
	    XtNselectable, FALSE,
	    NULL);
    XtAddCallback(typefaceScList, XtNitemCurrentCallback,
		  selectFontFamilyCB, NULL);
    XtAddCallback(typefaceScList, XtNitemNotCurrentCallback,
		  unselectFontFamilyCB, NULL);

    /*
     * Create "Scale" exclusive choice
     */
    scaleCaption = XtVaCreateManagedWidget("fontScale",
	    captionWidgetClass, fontsCategory,
	    NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)scaleCaption, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    scaleItemInfo = wspNewItemInfo(0, FONTS_SCALE_RES, dString,
	    "small:medium:large:extra_large", 1, scaleCaption);

    exclusive = XtVaCreateManagedWidget("exclusive",
	    exclusivesWidgetClass, scaleCaption,
	    NULL);

    for (ii = ScaleSmall; ii < ScaleScalable; ii++) {
        scaleButton[ii] = XtVaCreateManagedWidget(buttonNames[ii],
	        rectButtonWidgetClass, exclusive,
	        XtNuserData,	(XtArgVal)scaleItemInfo,
	        NULL);
        XtAddCallback(scaleButton[ii], XtNselect,
		wspChangeSettingCb, (XtPointer)ii);
        XtAddCallback(scaleButton[ii], XtNselect,
		changePointSizeCB, (XtPointer)pointSizeFromScale[ii]);
    }

    /*
     * Create preview area
     */

    previewInfo = previewCreate(fontsCategory);
    previewGetColors(previewInfo, TRUE);

    /*
     * Init fonts scrolling list
     */
    updateSlistContents();

    return(fontsCategory);
}

/***************************************************************************
* Private Fonts Category Supporting Functions
***************************************************************************/

/*
 * burstXLFDName takes XLFDName as input; it returns TRUE if it was able to
 * successfully burst the XLFDName into fields, and FALSE if it was unable
 * to do so.
 *
 * If burstXLFDName succeeds, then *XLFDNameFields will be an array of
 * (char *), each element pointing to a single field of the original
 * XLFDName.  Also, *freeMe will be the block of memory pointed to by the
 * elements of *XLFDNameFields.  The caller is then responsible for
 * XtFree'ing both *XLFDNameFields and *freeMe.
 *
 * If burstXLFDName fails, then *XLFDNameFields and *freeMe will be NULL
 * and the caller need not XtFree'ing them.
 */

/*
 * Definition of the fields of XLFD names
 */
#define XLFD_FOUNDRY	0
#define XLFD_FAMILY	1
#define XLFD_WEIGHT	2
#define XLFD_SLANT	3
#define XLFD_SETWIDTH	4
#define XLFD_ADD_STYLE	5
#define XLFD_PIXEL_SIZE	6
#define XLFD_POINT_SIZE	7
#define XLFD_RES_X	8
#define XLFD_RES_Y	9
#define XLFD_SPACING	10
#define XLFD_AVG_WIDTH	11
#define XLFD_REGISTRY	12
#define XLFD_ENCODING	13
#define XLFD_NUM_FIELDS	14

static Boolean
burstXLFDName(
    char   *XLFDName,
    char ***XLFDNameFields,
    char  **freeMe)
{
    int     i;
    char   *ch = XtNewString(XLFDName);

    *freeMe         = ch;
    /* LINTED XtCalloc should return a (void *) instead of a (char *) */
    *XLFDNameFields = (char **) XtCalloc(XLFD_NUM_FIELDS, (unsigned)sizeof(char **));

    if (*ch != '-') {
	/* XLFD name is malformed */
	XtFree((char *)*XLFDNameFields);
	XtFree(*freeMe);
	return(FALSE);
    }

    for (i=0; i < XLFD_NUM_FIELDS; i++) {
	(*XLFDNameFields)[i] = ++ch;
	ch = strchr(ch, '-');
	if (ch == NULL) {
	    break;
	} else {
	    *ch = '\0';
	}
    }

    if ((ch != NULL) || (++i != XLFD_NUM_FIELDS)) {
	/* XLFD name is malformed */
	XtFree((char *)*XLFDNameFields);
	XtFree(*freeMe);
	return(FALSE);
    }

    return (TRUE);
}

/*
 * Once the user has selected a font family from the scrolling list, we need
 * to determine which specific fonts belonging to that family should be used.
 *
 * The main complication in finding the specific fonts is that different font
 * families need different levels of "strictness" in the XLFD name.  For
 * example, fonts like Helvetica must specify the weight (medium or bold), the
 * slant (r), and the sWidth (normal); unless these are all specified, one is
 * likely to get the wrong font.  A contrasting example is ZapfChancery, which
 * comes in only one weight (medium) and slant (i); specifying "bold" for the
 * weight or "r" for the slant with ZapfChancery will yield the name of a
 * nonexistant font.
 *
 * Doing this via protocol requests after the user has selected the font
 * family is painfully slow.  It is much more efficient to query the server
 * once and then analyze the fontnames, building a database of which fonts are
 * best for each font family.  Since we have to query the server initially to
 * determine which font families are available, and since we have to analyze
 * the fontnames already to determine the pointsize and font family, there is
 * very little impact in creating a database of which fonts are most
 * appropriate for each font family.
 *
 * For any font, the pointsize must match the selected scale.  Thus, we keep
 * track of the best fonts for each scale independently, as well as keeping
 * track of which scalable font is best (for use when a bitmapped font for a
 * specific scale is not available).
 *
 * In addition, we must keep track of the best "standard" and "non-standard"
 * fonts separately.  If only the "standard" choices are being made available,
 * then only fonts that are "standard" (ie, sans-serif) can be used to
 * represent that font family.
 *
 * In addition, the monospace fonts must have a spacing of "m" or "c".  Only
 * fonts with such spacing are considered for monospace fonts.
 *
 * If for some reason, there isn't a font for the current font family that
 * also has the correct pointsize (or is scalable), the correct "standardness"
 * (if appropriate), and correct spacing (if appropriate), then the default
 * fonts will be used instead.  The common usages of the default fonts are--
 *
 * + No monospace font exists for the chosen font family; the default
 *   monospace font is used instead.
 * + The user changed the scale, and no font exists in that scale for the
 *   chosen font family; all the the default fonts are used instead.
 * + The user changed the "choices" setting to only allow "standard" fonts,
 *   and no "standard" font exists for the chosen font family; all the the
 *   default fonts are used instead.
 *
 * A simple scoring system is used to determine how desirable a given font is
 * as a representative of its font family.  The highest scoring font of a
 * given scale, etc. is recorded in the database.
 *
 * The following requirements for the fonts are implemented via the
 * font "scoring system".
 *
 *	Regular and Monospace fonts:
 *		Weight should be medium, light, or book
 *		Weight should NOT be bold, demibold, or demi
 *		Slant should be r
 *		sWidth should be normal
 *		Weight should be medium, otherwise
 *		Weight should be light, otherwise
 *		Weight should be book
 *	Bold fonts:
 *		Weight should be bold, demibold, or demi
 *		Weight should NOT be medium, light, or book
 *		Slant should be r
 *		sWidth should be normal
 *		Weight should be bold, otherwise
 *		Weight should be demibold, otherwise
 *		Weight should be demi
 */

#define SCORE_GOOD_WEIGHT	(1 << 7)
#define SCORE_NOT_BAD_WEIGHT	(1 << 6)
#define SCORE_GOOD_SLANT	(1 << 5)
#define SCORE_GOOD_SWIDTH	(1 << 4)
#define SCORE_BEST_WEIGHT1	(1 << 3)
#define SCORE_BEST_WEIGHT2	(1 << 2)
#define SCORE_BEST_WEIGHT3	(1 << 1)
#define SCORE_NOT_INVALID	(1 << 0)

/*
 * scoreXLFDName takes XLFDName as input; all other parameters are
 * output.
 *
 * Note that we can't use the font properties to get this info
 * because getting all the font properties is painfully slow, and in
 * addition, scalable fonts (pointsize 0 in XLFD name) have a
 * pointsize font property of 12.
 */

static Boolean
scoreXLFDName(
    char    **XLFDNameFields,	/* XLFD font name, burst into fields */
    int      *pointSize,	/* the pointsize */
    int      *scoreRegular,	/* score (see above) as regular font */
    int      *scoreBold,	/* score (see above) as bold font */
    int      *scoreMonosp,	/* score (see above) as monospace font */
    Boolean  *monospaceP,	/* is it monospace? */
    Boolean  *sansSerifP)	/* is it sans serif? */
{
    char     *ch;

    *scoreRegular = 0;
    *scoreBold    = 0;
    *scoreMonosp  = 0;

    if (strcmp(XLFDNameFields[XLFD_WEIGHT], "medium") == 0) {
	*scoreRegular |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT1;
	*scoreMonosp  |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT1;
    } else if (strcmp(XLFDNameFields[XLFD_WEIGHT], "light") == 0) {
	*scoreRegular |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT2;
	*scoreMonosp  |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT2;
    } else if (strcmp(XLFDNameFields[XLFD_WEIGHT], "book") == 0) {
	*scoreRegular |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT3;
	*scoreMonosp  |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT3;
    } else if (strcmp(XLFDNameFields[XLFD_WEIGHT], "bold") == 0) {
	*scoreBold    |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT1;
    } else if (strcmp(XLFDNameFields[XLFD_WEIGHT], "demibold") == 0) {
	*scoreBold    |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT2;
    } else if (strcmp(XLFDNameFields[XLFD_WEIGHT], "demi") == 0) {
	*scoreBold    |= SCORE_GOOD_WEIGHT | SCORE_BEST_WEIGHT3;
    } else {
	*scoreRegular |= SCORE_NOT_BAD_WEIGHT;
	*scoreBold    |= SCORE_NOT_BAD_WEIGHT;
	*scoreMonosp  |= SCORE_NOT_BAD_WEIGHT;
    }

    if (strcmp(XLFDNameFields[XLFD_SLANT], "r") == 0) {
	*scoreRegular |= SCORE_GOOD_SLANT;
	*scoreBold    |= SCORE_GOOD_SLANT;
	*scoreMonosp  |= SCORE_GOOD_SLANT;
    }

    if ((strcmp(XLFDNameFields[XLFD_SETWIDTH], "normal") == 0) ||
	    (strcmp(XLFDNameFields[XLFD_SETWIDTH], "") == 0)) {
	*scoreRegular |= SCORE_GOOD_SWIDTH;
	*scoreBold    |= SCORE_GOOD_SWIDTH;
	*scoreMonosp  |= SCORE_GOOD_SWIDTH;
    }

    *sansSerifP = (strcmp(XLFDNameFields[XLFD_ADD_STYLE], "sans") == 0);

    *pointSize = (int) strtol(XLFDNameFields[XLFD_POINT_SIZE], &ch, 10);
    if (XLFDNameFields[XLFD_POINT_SIZE] == ch) {
	/*
	 * Can't allow garbage in the XLFD_POINT_SIZE field to be confused
	 * with scalable fonts that have 0 in the XLFD_POINT_SIZE field.
	 */
	*scoreRegular = 0;
	*scoreBold    = 0;
	*scoreMonosp  = 0;
	return(FALSE);
    }

    *monospaceP = ((strcmp(XLFDNameFields[XLFD_SPACING], "m") == 0) ||
		   (strcmp(XLFDNameFields[XLFD_SPACING], "c") == 0));

    *scoreRegular    |= SCORE_NOT_INVALID;
    *scoreBold       |= SCORE_NOT_INVALID;
    if (*monospaceP) {
	*scoreMonosp |= SCORE_NOT_INVALID;
    } else {
	*scoreMonosp = 0;
    }

    return(TRUE);
}

/*
 * fontsGetBasicLocale returns a char* which is the name of the current
 * basicLocale, or displayLang if there is no basicLocale
 */

static const char *basicLocaleResources[] = {
    "OpenWindows.BasicLocale",
    "OpenWindows.basicLocale",
    "OpenWindows.DisplayLang",
    "OpenWindows.displayLang",
    NULL
};

const char *
fontsGetBasicLocale(void)
{
    XrmValue     value;
    const char  *basicLocale = NULL;
    const char	*basicLocaleResource;
    int		 index = 0;

    basicLocaleResource = basicLocaleResources[index];

    while ((basicLocale == NULL) && (basicLocaleResource != NULL)) {
	if (wspGetResource(basicLocaleResource, ACCESS_ALL_DB, &value)) {
	    basicLocale = value.addr;
	} else {
	    basicLocaleResource = basicLocaleResources[++index];
	}
    }

    if (basicLocale == NULL) {
	basicLocale = getenv("LANG");
    }

    if (basicLocale == NULL) {
	basicLocale = "C";
    }

    return XtNewString(basicLocale);
}

/*
 * listFontSets returns a char** pointing to a list of font set names.
 *	the number of font set names found is returned in *listLength.
 *	locale is used to look up the font set names.
 *
 * listFontSets has the side effect of creating fontSetDB.
 */
static char **
listFontSets(
    int   	*listLength,
    const char  *locale)
{
    char **fontSetNames;
    char   fontSetDBPath[MAXPATHLEN];

    (void) sprintf(fontSetDBPath, "%s/lib/locale/%s/OW_FONT_SETS/OpenWindows.fs",
	    wspGetOpenwinhome(), locale);

    fontSetDB = CreateOWFsetDB(fontSetDBPath);
    if (fontSetDB == (OWFsetDB) NULL) {
	*listLength = 0;
	return(NULL);
    }

    fontSetNames = ListOWFsets(fontSetDB, listLength);
    return(fontSetNames);
}

/*
 * getFontInfo returns a pointer to the fontInfo struct in fontInfoList which
 *	corresponds to the familyLabel passed to it.
 * If such a fontInfo struct isn't in fontInfoList yet, and createInfo is
 *	TRUE, then one is created and inserted into fontInfoList.
 * If such a fontInfo struct isn't in fontInfoList yet, and createInfo is
 *	FALSE, then NULL is returned.
 *
 * getFontInfo performs a linear search to find the requested fontInfo
 *	struct.  New fontInfo structs are added via an insertion sort,
 *	which keeps the scrolling list alphabetized, and improves
 *	performance of failed searches by making it possible to know that a
 *	given fontInfo struct doesn't exist before reaching the end of the
 *	fontInfoList.
 *
 * createFontInfo is used by getFontInfo to create a new fontInfo struct and
 *	insert it into fontInfoList
 */

static fontInfo *
createFontInfo(
    char     *familyLabel,
    fontInfo *parent,
    fontInfo *child)
{
    /* LINTED XtCalloc should return a (void *) instead of a (char *) */
    fontInfo *newPtr = (fontInfo*) XtCalloc(1, (unsigned)sizeof(fontInfo));

    newPtr->familyLabel = XtNewString(familyLabel);
    newPtr->next       = child;
    if (parent == NULL) {
    	fontInfoList   = newPtr;
    } else {
    	parent->next   = newPtr;
    }

    return(newPtr);
}

static fontInfo *
getFontInfo(
    char     *familyLabel,
    Boolean   createInfo)
{
    fontInfo *ptr  = fontInfoList,
	     *prev = NULL;
    int       comparison;

    if (familyLabel == NULL) {
	return(NULL);
    }

    while (ptr != NULL) {
        comparison = strcmp(ptr->familyLabel, familyLabel);
	if (comparison > 0) {				/* not found */
	    break;
	} else if (comparison == 0) {			/* found */
	    return(ptr);
	} else { /* (comparison < 0) */			/* keep looking */
	    prev = ptr;
	    ptr = ptr->next;
	}
    }

    /*
     * need to add the new one
     */
    if (createInfo) {
	return(createFontInfo(familyLabel, prev, ptr));
    } else {
	return(NULL);
    }
}

/*
 * getFamilyLabel uses XLFDNameFields to generate the appropriate family
 * label for a given font.  The labels are intended to match the format
 * used by the OLIT font chooser.
 */

static char *
getFamilyLabel(
    char        **XLFDNameFields)
{
    static char   ch[MAX_XLFD_LEN];
    char         *familyLabel,
		 *ptr;
    Boolean       useSetwidth,
	          useAddStyle,
	          useFoundry,
		  wasLower;
 
    useSetwidth = ((strcmp(XLFDNameFields[XLFD_SETWIDTH], "normal") != 0) &&
		   (strcmp(XLFDNameFields[XLFD_SETWIDTH], "") != 0));
    useAddStyle = (strcmp(XLFDNameFields[XLFD_ADD_STYLE], "") != 0);
    useFoundry  = (strcmp(XLFDNameFields[XLFD_FOUNDRY], "") != 0);

    (void) sprintf(ch, "%s%s%s%s%s%s%s%s",
		XLFDNameFields[XLFD_FAMILY],
		useSetwidth ? " " : "",
		useSetwidth ? XLFDNameFields[XLFD_SETWIDTH] : "",
		useAddStyle ? "-" : "",
		useAddStyle ? XLFDNameFields[XLFD_ADD_STYLE] : "",
		useFoundry  ? " (" : "",
		XLFDNameFields[XLFD_FOUNDRY],
		useFoundry  ? ")" : "");
 
    familyLabel = XtNewString(ch);
    for (wasLower = FALSE, ptr = (char *)familyLabel; *ptr != NULL; ptr++) {
	if (islower(*ptr) && !wasLower) {
	    *ptr = toupper(*ptr);
	    wasLower = TRUE;
	} else {
	    wasLower = islower(*ptr);
	}
    }

    return(familyLabel);
}

/*
 * initPointSizeFromScale initializes
 *	pointSizeFromScale[]   point size corresponding to each scale
 */

static void
initPointSizeFromScale(void)
{
    ScaleType    sc;
    XrmDatabase  xtdb = XtDatabase(display);
    char        *type,
		*ch;
    XrmValue     value;
    int		 ii;

    for (sc = ScaleSmall; sc < ScaleScalable; sc++) {
	if (XrmGetResource(xtdb, FONTS_POINTSIZE_RES[sc],
			   FONTS_POINTSIZE_RES[sc], &type, &value)) {
	    ii = (int) strtol(value.addr, &ch, 10);
	    if (value.addr != ch) {
	        pointSizeFromScale[sc] = ii;
	    }
	}
    }
}

/*
 * initFontInfoList initializes
 *	 fontSetDB      (font set database, via listFontSets)
 *	 useFontSets    (whether font set data was found)
 *	*fontInfoList   (list of all fontInfo structs)
 *
 * REMIND: use meaningful loop variable names
 */

static void
initFontInfoList(void)
{
    char       **fontlist,
	        *familyLabel;
    const char  *locale = fontsGetBasicLocale();
    int	         ii,nfonts,pointSize;
    int          scoreRegular,	/* score (see above) as regular font */
	         scoreBold,	/* score (see above) as bold font */
	         scoreMonosp;	/* score (see above) as monospace font */
    Boolean      monospaceP,	/* is it monospace? */
    	         sansSerifP;	/* is it sans serif? */

    fontlist    = listFontSets(&nfonts, locale);
    useFontSets = (nfonts > 0);
    if (!useFontSets) {
        fontlist = XListFonts(display, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*",
			      BIGNUM, &nfonts);
    }
    /*
     * Figure out which font families exist, and which fonts in each family
     * are the best ones to use.
     */
    for(ii = 0; ii < nfonts; ii++) {
        char     **XLFDNameFields,
	          *freeMe;
        fontInfo  *fi;
        ScaleType  sc;

        if (!burstXLFDName(fontlist[ii], &XLFDNameFields, &freeMe))
	    continue;

	if (!scoreXLFDName(XLFDNameFields, &pointSize,
			   &scoreRegular, &scoreBold, &scoreMonosp,
			   &monospaceP, &sansSerifP)) {
            XtFree((char *)XLFDNameFields);
            XtFree(freeMe);
	    continue;
	}

	familyLabel = getFamilyLabel(XLFDNameFields);
        fi = getFontInfo(familyLabel, TRUE);

        XtFree((char *)XLFDNameFields);
        XtFree(freeMe);
        XtFree(familyLabel);

	sc = scaleFromPointSize(pointSize / 10);

	if (sc == ScaleMax) continue;

        /*
         * Score all fonts
         */
        if (scoreRegular > fi->scoreBestRegular[sc]) {
	    if (fi->bestRegular[sc] != NULL) XtFree(fi->bestRegular[sc]);
	    fi->bestRegular[sc]      = XtNewString(fontlist[ii]);
	    fi->scoreBestRegular[sc] = scoreRegular;
        }
        if (scoreBold > fi->scoreBestBold[sc]) {
	    if (fi->bestBold[sc] != NULL) XtFree(fi->bestBold[sc]);
	    fi->bestBold[sc]      = XtNewString(fontlist[ii]);
	    fi->scoreBestBold[sc] = scoreBold;
        }
        if ((monospaceP) && (scoreMonosp > fi->scoreBestMonosp[sc])) {
	    if (fi->bestMonosp[sc] != NULL) XtFree(fi->bestMonosp[sc]);
	    fi->bestMonosp[sc]      = XtNewString(fontlist[ii]);
	    fi->scoreBestMonosp[sc] = scoreMonosp;
        }

	fi->isStandardP = sansSerifP;
    }

    if (useFontSets) {
        FreeOWFsetList(fontlist);
    } else {
        XFreeFontNames(fontlist);
    }
}

/*
 * initDefaultFont initializes
 *	*defaultFont	(extra fontInfo struct for default fonts)
 *
 * REMIND: use meaningful loop variable names
 */

static void
initDefaultFont(void)
{

    /*
     * Figure out now what the default fonts should be
     *
     * REMIND: need to change search pattern for default font.
     */
    if (defaultFont == NULL) {
        fontInfo     *fi;
	char         *familyLabel,
		     *type;
	char const  **ch;
	XrmValue      value;
	XrmDatabase   xtdb = XtDatabase(display);
	ScaleType     sc;

	/* LINTED XtCalloc should return a (void *) instead of a (char *) */
        defaultFont = (fontInfo*) XtCalloc(1, (unsigned)sizeof(fontInfo));
	/*
	 * Try the fonts in DEFAULT_FONTS_RES first
	 */
	for (ch = DEFAULT_FONTS_RES; *ch != NULL; ch++) {
	    if (XrmGetResource(xtdb, *ch, *ch, &type, &value)) {
		familyLabel = value.addr;
	    } else {
		familyLabel = "Lucida-Sans (B&H)";
	    }

            fi = getFontInfo(familyLabel, FALSE);
	    if (fi == NULL) continue;

	    if (defaultFont->familyLabel == NULL) {
	        defaultFont->familyLabel = fi->familyLabel;
	    }
	    for (sc = ScaleSmall; sc < ScaleMax; sc++) {
	        if ((defaultFont->bestRegular[sc] == NULL) &&
		        (fi->bestRegular[sc] != NULL)) {
		    defaultFont->bestRegular[sc] = fi->bestRegular[sc];
	        }
	        if ((defaultFont->bestBold[sc] == NULL) &&
		        (fi->bestBold[sc] != NULL)) {
		    defaultFont->bestBold[sc] = fi->bestBold[sc];
	        }
	        if ((defaultFont->bestMonosp[sc] == NULL) &&
		        (fi->bestMonosp[sc] != NULL)) {
		    defaultFont->bestMonosp[sc] = fi->bestMonosp[sc];
	        }
	    }
	}

	/*
	 * If the default fonts in DEFAULT_FONTS_RES fail, try anything
	 */
        for (sc = ScaleSmall; sc < ScaleMax; sc++) {
	    if ((defaultFont->bestRegular[sc] == NULL) &&
		    (defaultFont->bestRegular[ScaleScalable] == NULL)) {
	        for (fi = fontInfoList; fi != NULL; fi = fi->next) {
		    if (fi->bestRegular[sc] != NULL) {
		        defaultFont->bestRegular[sc] = fi->bestRegular[sc];
		        break;
		    }
	        }
	    }
	    if ((defaultFont->bestBold[sc] == NULL) &&
		    (defaultFont->bestBold[ScaleScalable] == NULL)) {
	        for (fi = fontInfoList; fi != NULL; fi = fi->next) {
		    if (fi->bestBold[sc] != NULL) {
		        defaultFont->bestBold[sc] = fi->bestBold[sc];
		        break;
		    }
	        }
	    }
	    if ((defaultFont->bestMonosp[sc] == NULL) &&
		    (defaultFont->bestMonosp[ScaleScalable] == NULL)) {
	        for (fi = fontInfoList; fi != NULL; fi = fi->next) {
		    if (fi->bestMonosp[sc] != NULL) {
		        defaultFont->bestMonosp[sc] = fi->bestMonosp[sc];
		        break;
		    }
	        }
	    }
        }
    }
}

/*
 * initFontData invokes
 *	initPointSizeFromScale()
 *	initFontInfoList()
 *	initDefaultFont()
 * to initialize various font data.
 */

static void
initFontData(void)
{
    static Boolean initNeeded = True;

    if (initNeeded) {
        initPointSizeFromScale();
	initFontInfoList();
	initDefaultFont();
	initNeeded = False;
    }
}



/*
 * Callbacks must take a Widget and two XtPointer's, even if they
 * don't need them.  Use ARGSUSED to keep lint happy.
 *
 * changeChoicesCB services the "choices" buttons
 * changePointSizeCB services the "scale" buttons
 * selectFontFamilyCB and unselectFontFamilyCB service the "typeface"
 *	scrolling list
 */

/*ARGSUSED*/
static void
changeChoicesCB(
    Widget     widget,
    XtPointer  clientData,
    XtPointer  callData)
{
    wspClearFooter();
    allowAllChoices = (Boolean)clientData;
    updateSlistContents();
    (void) updateSlistSensitivity();
    updateButtonSensitivity();
    updatePreviewWindow();
}

/*ARGSUSED*/
static void
changePointSizeCB(
    Widget    widget,
    XtPointer clientData,
    XtPointer callData)
{
    ScaleType scale = scaleFromPointSize((int) clientData);

    wspClearFooter();

    if ((scale == ScaleScalable) || (scale == ScaleMax)) {
	wspErrorFooter(fontsErrors.badPointSize, currentPointSize, NULL);
    } else {
        currentPointSize = (int) clientData;
	currentScale     = scale;
    }

    (void) updateSlistSensitivity();
    updateButtonSensitivity();
    updatePreviewWindow();
}


/*ARGSUSED*/
static void
unselectFontFamilyCB(
    Widget       widget,
    XtPointer    clientData,
    XtPointer    callData)
{
    OlDefine     changeBar;

    OlSlistItemPtr *currentItem;
    int		    numCurrentItems;

    wspClearFooter();
    XtVaGetValues(typefaceScList,
	    XtNnumCurrentItems, &numCurrentItems,
	    XtNcurrentItems,    &currentItem,
	    NULL);
    switch (numCurrentItems) {
    case 0:
    case 1:
    	if (currentFamily != NULL) XtFree(currentFamily);
	currentFamily = NULL;
	break;
    default:
	/* REMIND: any other value is an error! */
	break;
    }
    updateButtonSensitivity();

    XtVaGetValues(typefaceCaption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if (changeBar != OL_NORMAL) {
	XtVaSetValues(typefaceCaption,
		XtNchangeBar, (XtArgVal)OL_NORMAL,
		NULL);
    }
}

/*ARGSUSED*/
static void
selectFontFamilyCB(
    Widget       widget,
    XtPointer    clientData,
    XtPointer    callData)
{
    OlDefine     changeBar;

    OlSlistItemPtr *currentItem;
    int		    numCurrentItems;

    wspClearFooter();
    XtVaGetValues(typefaceScList,
	    XtNnumCurrentItems, &numCurrentItems,
	    XtNcurrentItems,    &currentItem,
	    NULL);
    switch (numCurrentItems) {
    case 0:
    	if (currentFamily != NULL) XtFree(currentFamily);
	currentFamily = NULL;
	break;
    case 1:
    	if (currentFamily != NULL) XtFree(currentFamily);
	currentFamily = OlSlistGetItemLabel(typefaceScList, *currentItem);
	currentFamily = XtNewString(currentFamily);
	break;
    default:
	/* REMIND: any other value is an error! */
	break;
    }
    updateButtonSensitivity();
    updatePreviewWindow();

    XtVaGetValues(typefaceCaption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if (changeBar != OL_NORMAL) {
	XtVaSetValues(typefaceCaption,
		XtNchangeBar, (XtArgVal)OL_NORMAL,
		NULL);
    }
}

/*
 * setWidgetSensitivity is a wrapper for XtVaSetValues that sets the
 *	widget's XtNsensitive resource to the specified state.
 */
static void
setWidgetSensitivity(
    Widget	   widget,
    Boolean	   state)
{
    XtVaSetValues(widget,
	    XtNsensitive, (XtArgVal)state,
	    NULL);
}

/*
 * setItemSensitivity is a wrapper for OlSlistSetItemAttrs that sets the
 *	item's item_sensitive attribute to the specified state.
 */
static void
setItemSensitivity(
    Widget	   widget,
    OlSlistItemPtr item,
    Boolean	   state)
{
    OlSlistItemAttrs itemAttrs;

    itemAttrs.flags	     = OlItemSensitive;
    itemAttrs.item_sensitive = state;
    OlSlistSetItemAttrs(widget, item, &itemAttrs);
    OlSlistTouchItem(widget, item);
}

/*
 * updateButtonSensitivity updates the sensitivity of the scaleButton[]'s
 *	after the currentFamily has been changed.
 */
static void
updateButtonSensitivity(void)
{
    fontInfo   *fi = getFontInfo(currentFamily, False);
    ScaleType   sc;

    if (fi != NULL) {
        for (sc = ScaleSmall; sc < ScaleScalable; sc++) {
	    setWidgetSensitivity(scaleButton[sc],
				 ((fi->bestRegular[sc] != NULL) ||
			     	  (fi->bestRegular[ScaleScalable] != NULL)));
	}
    } else {
        for (sc = ScaleSmall; sc < ScaleScalable; sc++) {
	    setWidgetSensitivity(scaleButton[sc], True);
	}
    }
}

/*
 * updateSlistContents updates the contents of the "typeface" scrolling
 *	list after allowAllChoices has been changed (via the "choices"
 *	buttons).
 *
 * REMIND: use descriptive variable names!
 */

static void
updateSlistContents(void)
{
    fontInfo         *fi = fontInfoList;
    OlSlistItemAttrs  item_attr;
    int		      num;

    XtVaGetValues(typefaceScList,   XtNnumItems, (XtArgVal)&num,   NULL);
    if (num > 0) OlSlistDeleteAllItems(typefaceScList);

    while (fi != NULL) {
	if (allowAllChoices) {
	    item_attr.flags = OlItemLabelType | OlItemLabel | OlItemUserData;
	    item_attr.label_type = OL_STRING;
	    item_attr.item_label = fi->familyLabel;
	    item_attr.user_data  = (XtPointer) fi;

	    fi->slistItem = OlSlistAddItem(typefaceScList, &item_attr, NULL);
	} else {

	    if (fi->isStandardP) {
	    	item_attr.flags = OlItemLabelType | OlItemLabel | OlItemUserData;
	    	item_attr.label_type = OL_STRING;
	    	item_attr.item_label = fi->familyLabel;
	    	item_attr.user_data  = (XtPointer) fi;

	    	fi->slistItem = OlSlistAddItem(typefaceScList, &item_attr, NULL);
	    } else {
		fi->slistItem = NULL;
	    }
	}

	fi = fi->next;
    }
}

/*
 * updateSlistSensitivity updates the sensitivity of all the items of the
 *	"typeface" scrolling list after the currentPointSize has been
 *	changed (via the "scale" buttons), or after the contents of the
 *	"typeface" scrolling list has been updated by updateSlistContents.
 *
 * updateSlistSensitivity returns True if the currentFamily is found in the
 *	resulting scrolling list (currentFamily will be selected), and
 *	False otherwise.
 *
 * REMIND: use descriptive variable names!
 */

static Boolean
updateSlistSensitivity(void)
{
    fontInfo       *fi;
    ScaleType       sc = currentScale;
    Boolean         found = False,
		    sensitive;
    OlSlistItemPtr  item;

    OlSlistUpdateView(typefaceScList,False);
    for (fi = fontInfoList; fi != NULL; fi = fi->next) {
	if (fi->slistItem == NULL) continue;

	sensitive = ((allowAllChoices || fi->isStandardP) &&
		     ((fi->bestRegular[sc] != NULL) ||
		      (fi->bestRegular[ScaleScalable] != NULL)));
	setItemSensitivity(typefaceScList, fi->slistItem, sensitive);
	if (sensitive && (currentFamily != NULL) &&
		    (strcmp(fi->familyLabel, currentFamily) == 0)) {
	    if (!OlSlistIsItemCurrent(typefaceScList, fi->slistItem)) {
	        OlSlistMakeItemCurrent(typefaceScList, fi->slistItem, False);
	    }

	    XtVaGetValues(typefaceScList,
		    XtNfirstViewableItem, (XtArgVal)&item,
		    NULL);
	    if (strcmp(OlSlistGetItemLabel(typefaceScList, item),
		       currentFamily) > 0) {
	        OlSlistFirstViewableItem(typefaceScList, fi->slistItem);
	    }

	    XtVaGetValues(typefaceScList,
		    XtNlastViewableItem, (XtArgVal)&item,
		    NULL);
	    if (OlSlistIsValidItem(typefaceScList, item) &&
	        (strcmp(OlSlistGetItemLabel(typefaceScList, item),
		        currentFamily) < 0)) {
		OlSlistLastViewableItem(typefaceScList, fi->slistItem);
	    }

	    OlSlistTouchItem(typefaceScList, fi->slistItem);
	    found = True;
	}
    }
    OlSlistUpdateView(typefaceScList,True);

    return(found);
}

/*
 * getFontSet returns an XFontSet that corresponds to requestedName, or
 *	NULL if such an XFontSet can't be created.
 */

static XFontSet
getFontSet(
    char      *requestedName)
{
    char      *fontSetName;
    char      *fontSetSpec;
    XFontSet   returnValue;
    char     **missingCharList;
    int        missingCharCount;
    char      *defaultString;

    if (useFontSets) {
	/* look up font set spec and use it */
        fontSetName = GetOWFsetForSpec(fontSetDB, requestedName);
        if (fontSetName == NULL) return(NULL);

        fontSetSpec = GetFsetForOWFset(fontSetDB, fontSetName);
        if (fontSetSpec == NULL) return(NULL);

        returnValue = XCreateFontSet(display, fontSetSpec,
	        &missingCharList, &missingCharCount, &defaultString);
    } else {
	/* create minimal font set using requestedName itself */
        returnValue = XCreateFontSet(display, requestedName,
	        &missingCharList, &missingCharCount, &defaultString);
    }

    return(returnValue);
}

/*
 * addWildcard copies the XLFD name sourceFontName into destFontName (which
 *	is assumed to be "big enough"), converting the pointsize field in
 *	destFontName to match the currentPointSize, and converting the
 *	pixelsize, resx, resy, and avgWdth fields to asterisks (the
 *	wildcard character that matches any value).  This is most useful
 *	for converting the XLFD names of scalable fonts into XLFD names of
 *	specific pointsizes of those scalable fonts, but it is also used to
 *	avoid being overly specific when writing bitmapped font names.
 *
 * REMIND: the fontset code can't handle wildcarding, so if (useFontSets)
 *	is true, we merely use strcpy instead of wildcarding everything.
 */

static void
addWildcard(
    char         *destFontName,
    char         *sourceFontName)
{
    char        **XLFDNameFields,
	         *freeMe;
    static char  *wildcard = "*";

    if (useFontSets) {
	(void) strcpy(destFontName,sourceFontName);
	return;
    }

    if (!burstXLFDName(sourceFontName, &XLFDNameFields, &freeMe)) {
	/* REMIND: print error */
	return;
    }

    (void) sprintf(destFontName, "-%s-%s-%s-%s-%s-%s-%s-%d0-%s-%s-%s-%s-%s-%s",
		XLFDNameFields[XLFD_FOUNDRY],
		XLFDNameFields[XLFD_FAMILY],
		XLFDNameFields[XLFD_WEIGHT],
		XLFDNameFields[XLFD_SLANT],
		XLFDNameFields[XLFD_SETWIDTH],
		XLFDNameFields[XLFD_ADD_STYLE],
		wildcard,		/* XLFDNameFields[XLFD_PIXEL_SIZE] */
		currentPointSize,	/* XLFDNameFields[XLFD_POINT_SIZE] */
		wildcard,		/* XLFDNameFields[XLFD_RES_X] */
		wildcard,		/* XLFDNameFields[XLFD_RES_Y] */
		XLFDNameFields[XLFD_SPACING],
		wildcard,		/* XLFDNameFields[XLFD_AVG_WIDTH] */
		XLFDNameFields[XLFD_REGISTRY],
		XLFDNameFields[XLFD_ENCODING]);

    XtFree((char *)XLFDNameFields);
    XtFree(freeMe);
}

/*
 * updatePreviewWindow updates the preview window to reflect changes in
 *	either currentPointSize or currentFamily.
 *
 * if there is no regular font, then we bail out and don't change anything.
 */

static void
updatePreviewWindow(void)
{
    wspChangeBusyState(TRUE);

    fontsUpdatePreviewInfo(previewInfo);
    previewCalculate(previewInfo);
    previewRefreshAsRequired(previewInfo, ColorWindowBackground);
    previewRefreshWorkspace(previewInfo);

    wspChangeBusyState(FALSE);
}

/*
 * fontsetPutResource
 */
static void
fontsetPutResource(
    const char *fontResourceName,
    char       *resourceValue,
    int	        dbAccess)
{
    const char *locale = fontsGetBasicLocale();
    char       *fontsetResourceName;

    fontsetResourceName = XtMalloc(2 * strlen(fontResourceName));
    if (useFontSets) {
	(void) sprintf(fontsetResourceName, "%s.%s", fontResourceName, locale);
	wspPutResource(fontsetResourceName, resourceValue, dbAccess);
    } else {
	wspPutResource(fontResourceName, resourceValue, dbAccess);
    }

    XtFree(fontsetResourceName);
}

/*
 * private versions of standard props.c functions
 *	fontsSaveDbSettings   (see wspSaveDbSettings)
 *	fontsReadDbSettings   (see wspReadDbSettings)
 *	fontsCreateChangeBars (see wspCreateChangeBars)
 *	fontsDeleteChangeBars (see wspDeleteChangeBars)
 *	fontsBackupSettings    (see wspBackupSettings)
 *	fontsRestoreSettings   (see wspRestoreSettings)
 *
 * scaleItemInfo is still being handled by the wsp* functions, which are
 * called by the corresponding fonts* functions.
 */

static void
fontsSaveDbSettings(
    Widget   widget,
    int      dbAccess)
{
    OlDefine typefaceChangeBar,
	     scaleChangeBar;

    XtVaGetValues(typefaceCaption,
	    XtNchangeBar, (XtArgVal)&typefaceChangeBar,
	    NULL);
    XtVaGetValues(scaleCaption,
	    XtNchangeBar, (XtArgVal)&scaleChangeBar,
	    NULL);
    if ((typefaceChangeBar == OL_NORMAL) || (scaleChangeBar == OL_NORMAL)) {
	fontsetPutResource(FONTS_REGULARFONT_RES, currentFontRegular, dbAccess);
	fontsetPutResource(FONTS_BUTTONFONT_RES,  currentFontRegular, dbAccess);
	fontsetPutResource(FONTS_TEXTFONT_RES,    currentFontRegular, dbAccess);
	fontsetPutResource(FONTS_BOLDFONT_RES,    currentFontBold,    dbAccess);
	fontsetPutResource(FONTS_TITLEFONT_RES,   currentFontBold,    dbAccess);
	fontsetPutResource(FONTS_MONOSPFONT_RES,  currentFontMonoSp,  dbAccess);

	XtVaSetValues(typefaceCaption,
		XtNchangeBar, (XtArgVal)OL_NONE,
		NULL);
    }

    wspSaveDbSettings(widget, dbAccess);
}

static void
fontsReadDbSettings(
    Widget     widget,
    int        dbAccess,
    Boolean    preserve)
{
    OlDefine  changeBar;
    XrmValue  value;
    Boolean   familyChanged = FALSE,
	      pointSizeChanged;
    char     *origFamily = currentFamily;
    int       origPointSize = currentPointSize;
    static Boolean firstTime = TRUE;

    XtVaGetValues(typefaceCaption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if ((!preserve) || (changeBar == OL_NONE)) {
	if (wspGetResource(FONTS_REGULARFONT_RES, dbAccess, &value)) {
            char **currentXLFDName,
	          *freeMe;
	    
            if (burstXLFDName(value.addr, &currentXLFDName, &freeMe)) {
            	currentFamily = getFamilyLabel(currentXLFDName);

        	XtFree((char *)currentXLFDName);
        	XtFree(freeMe);
	    } else {
		currentFamily = XtNewString(defaultFont->familyLabel);
	    }
	} else {
	    currentFamily = XtNewString(defaultFont->familyLabel);
	}

	if ((origFamily != NULL) && (currentFamily != NULL)) {
	    familyChanged = (strcmp(currentFamily, origFamily) != 0);
	    XtFree(origFamily);
	} else {
	    familyChanged = TRUE;
	}
    }

    wspReadDbSettings(widget, dbAccess, preserve);

    if ((scaleItemInfo->currVal == ScaleScalable) ||
	(scaleItemInfo->currVal == ScaleMax)) {
	wspErrorFooter(fontsErrors.badPointSize, currentPointSize, NULL);
    } else {
    	currentScale     = scaleItemInfo->currVal;
        currentPointSize = pointSizeFromScale[currentScale];
    }

    pointSizeChanged = (origPointSize != currentPointSize);

    if (pointSizeChanged || familyChanged || firstTime) {
	firstTime = FALSE;
	if (! updateSlistSensitivity()) {
	    if (! allowAllChoices) {
		/* if user's font isn't shown in the standard typeface
		 * scrolling list, then try the custom typefaces.  If it
		 * isn't there either, go back to the standard typefaces.
		 * Skip this search though if we're already using the
		 * custom typeface list.
		 */
	    	wspSetWidgetState(choicesButton, 1);
	    	allowAllChoices = True;
	    	updateSlistContents();
	    	if (! updateSlistSensitivity()) {
	    	    wspSetWidgetState(choicesButton, 0);
	    	    allowAllChoices = False;
	    	    updateSlistContents();
	    	    (void) updateSlistSensitivity();
		}
	    }
	}
	updateButtonSensitivity();
    }
    updatePreviewWindow();
}

static void
fontsCreateChangeBars(
    Widget widget)
{
    if ((backupFamily != NULL) && (currentFamily != NULL) &&
	    (strcmp(backupFamily, currentFamily) != 0)) {
        OlDefine changeBar;

        XtVaGetValues(typefaceCaption,
	        XtNchangeBar, (XtArgVal)&changeBar,
	        NULL);
        if (changeBar != OL_NORMAL) {
	    XtVaSetValues(typefaceCaption,
		    XtNchangeBar, (XtArgVal)OL_NORMAL,
		    NULL);
        }
    }
    wspCreateChangeBars(widget);
}

static void
fontsDeleteChangeBars(
    Widget widget)
{
    XtVaSetValues(typefaceCaption,
	    XtNchangeBar, (XtArgVal)OL_NONE,
	    NULL);
    wspDeleteChangeBars(widget);
}

static void
fontsBackupSettings(
    Widget   widget,
    Boolean  preserve)
{
    OlDefine changeBar;

    XtVaGetValues(typefaceCaption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if ((!preserve) || (changeBar == OL_NONE)) {
	if (backupFamily != NULL) XtFree(backupFamily);
        backupFamily    = XtNewString(currentFamily);
    }

    XtVaGetValues(scaleCaption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if ((!preserve) || (changeBar == OL_NONE)) {
	backupPointSize = currentPointSize;
	backupScale     = currentScale;
    }

    wspBackupSettings(widget,preserve);
}

static void
fontsRestoreSettings(
    Widget   widget)
{
    OlDefine changeBar;
    Boolean  pointSizeChanged = FALSE,
	     familyChanged = FALSE;

    XtVaGetValues(typefaceCaption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if (changeBar != OL_NONE) {
	if (backupFamily != NULL) {
	    if (currentFamily != NULL) XtFree(currentFamily);
            currentFamily = XtNewString(backupFamily);
	}
	familyChanged = TRUE;
    }

    XtVaGetValues(scaleCaption,
	    XtNchangeBar, (XtArgVal)&changeBar,
	    NULL);
    if (changeBar != OL_NONE) {
	currentPointSize = backupPointSize;
	currentScale     = backupScale;
	pointSizeChanged = TRUE;
    }

    if (pointSizeChanged || familyChanged) {
	if (! updateSlistSensitivity()) {
	    if (! allowAllChoices) {
		/* if user's font isn't shown in the standard typeface
		 * scrolling list, then try the custom typefaces.  If it
		 * isn't there either, go back to the standard typefaces.
		 * Skip this search though if we're already using the
		 * custom typeface list.
		 */
	    	wspSetWidgetState(choicesButton, 1);
	    	allowAllChoices = True;
	    	updateSlistContents();
	    	if (! updateSlistSensitivity()) {
	    	    wspSetWidgetState(choicesButton, 0);
	    	    allowAllChoices = False;
	    	    updateSlistContents();
	    	    (void) updateSlistSensitivity();
		}
	    }
	}
	updateButtonSensitivity();
	updatePreviewWindow();
    }

    wspRestoreSettings(widget);
}

static XFontSet
getFontSets(
    char     *currentFontName,
    char     *fontNameBitmapRequested,
    char     *fontNameScaledRequested,
    char     *fontNameBitmapDefault,
    char     *fontNameScaledDefault,
    Boolean  *useDefault)
{
    char     *currentFontNameSaved = XtNewString(currentFontName);
    XFontSet  newFontSet           = NULL;

    if (fontNameBitmapRequested != NULL) {
	addWildcard(currentFontName,fontNameBitmapRequested);
        newFontSet = getFontSet(currentFontName);
    } else if (fontNameScaledRequested != NULL) {
	addWildcard(currentFontName,fontNameScaledRequested);
        newFontSet = getFontSet(currentFontName);
    } else if (fontNameBitmapDefault != NULL) {
    	*useDefault = TRUE;
	addWildcard(currentFontName,fontNameBitmapDefault);
        newFontSet = getFontSet(currentFontName);
    } else if (fontNameScaledDefault != NULL) {
    	*useDefault = TRUE;
	addWildcard(currentFontName,fontNameScaledDefault);
        newFontSet = getFontSet(currentFontName);
    }

    if (newFontSet == NULL) {
	/*
	 * font no longer exists, so put everything back (fontpath probably
	 * changed, or something like that)
	 *
	 * REMIND: it would be nice to requery the server and update the
	 * font database
	 */
	wspErrorFooter(fontsErrors.fontNotFound, currentFontName, NULL);
	(void) strcpy(currentFontName,currentFontNameSaved);
    }
    XtFree(currentFontNameSaved);
    return(newFontSet);
}

/***************************************************************************
* Public Functions
***************************************************************************/

void
fontsUpdatePreviewInfo(
    PreviewInfo *previewInfo)
{
    XFontSet    newFontset;
    fontInfo   *fi;
    ScaleType   sc;
    Boolean     useDefault,
                usingDefault;
    static Boolean firstTime = TRUE;

    initFontData();

    if (firstTime) {
	XrmValue   value;

	/* Get font family specifications */

	if (wspGetResource(FONTS_REGULARFONT_RES, ACCESS_ALL_DB, &value)) {
	    char **currentXLFDName,
	          *freeMe;

	    if (burstXLFDName(value.addr, &currentXLFDName, &freeMe)) {
		currentFamily = getFamilyLabel(currentXLFDName);

		XtFree((char *)currentXLFDName);
		XtFree(freeMe);
	    } else {
		currentFamily = XtNewString(defaultFont->familyLabel);
	    }
	} else {
	    currentFamily = XtNewString(defaultFont->familyLabel);
	}

	/* Get scale specifications */

        currentScale = ScaleMedium;

        if (wspGetResource(PREVIEW_SCALE_RES, ACCESS_ALL_DB, &value)) {

                if (strcmp(value.addr, "large") == 0) {
                        currentScale = ScaleLarge;
                } else if (strcmp(value.addr, "small") == 0) {
                        currentScale = ScaleSmall;
                } else if (strcmp(value.addr, "extra_large") == 0) {
                        currentScale = ScaleExtraLarge;
                }
        }
	currentPointSize = pointSizeFromScale[currentScale];

	firstTime = FALSE;
    }
    fi           = getFontInfo(currentFamily, True);
    sc           = currentScale;
    useDefault   = False,
    usingDefault = False;

    newFontset = getFontSets(currentFontRegular,
	    (fi != NULL) ? fi->bestRegular[sc] : NULL,
	    (fi != NULL) ? fi->bestRegular[ScaleScalable] : NULL,
	    defaultFont->bestRegular[sc],
	    defaultFont->bestRegular[ScaleScalable], &useDefault);
    if (newFontset == NULL) {
	previewInfo->regularFontSet   = NULL;
	previewInfo->boldFontSet      = NULL;
	previewInfo->monospaceFontSet = NULL;
        return;
    }
    if (useDefault) {
	wspErrorFooter(fontsErrors.usingDefaultFont, NULL);
	usingDefault = True;
    }
    previewInfo->regularFontSet = newFontset;

    newFontset = getFontSets(currentFontBold,
	    (fi != NULL) ? fi->bestBold[sc] : NULL,
	    (fi != NULL) ? fi->bestBold[ScaleScalable] : NULL,
	    defaultFont->bestBold[sc],
	    defaultFont->bestBold[ScaleScalable], &useDefault);
    if (useDefault) {
	wspErrorFooter(fontsErrors.usingDefaultFont, NULL);
	usingDefault = True;
    }
    previewInfo->boldFontSet = newFontset;

    newFontset = getFontSets(currentFontMonoSp,
	    (fi != NULL) ? fi->bestMonosp[sc] : NULL,
	    (fi != NULL) ? fi->bestMonosp[ScaleScalable] : NULL,
	    defaultFont->bestMonosp[sc],
	    defaultFont->bestMonosp[ScaleScalable], &useDefault);
    if (useDefault && !usingDefault) {
	wspErrorFooter(fontsErrors.usingDefaultDataFont, NULL);
    }
    previewInfo->monospaceFontSet = newFontset;

    previewInfo->scale = currentScale;
}
