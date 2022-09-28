#pragma ident	"@(#)color.c	1.42	93/08/02 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/DrawArea.h>
#include <Xol/Exclusives.h>
#include <Xol/Nonexclusi.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/Slider.h>
#include <olgx/olgx.h>
#include <assert.h>
#include "props.h"
#include "preview.h"
#include "color.h"
#ifdef DBMALLOC
	#include <dbmalloc.h>
#endif

#define COLOR_WORKSPACE_FOREGROUND_FORMAT "OpenWindows.%s.WorkspaceBitmapFg"
#define COLOR_WORKSPACE_BACKGROUND_FORMAT "OpenWindows.%s.WorkspaceBitmapBg"
#define COLOR_WORKSPACE_RANK_FORMAT       "OpenWindows.%s.Rank"


/* Other Strings */

static const char
    *const COLOR_FOREGROUND_BITMAP_FILE      = "foreground.xbm",
    *const COLOR_BACKGROUND_BITMAP_FILE      = "background.xbm",
 
    *const COLOR_PATTERN_ATTR_FILE	     = "attributes",
    *const COLOR_PATTERN_DIR		     = "etc/workspace/patterns",

    *const COLOR_WINDOWS_NAME		     = "colorWindows",
    *const COLOR_DATA_AREAS_NAME	     = "colorDataAreas",
    *const COLOR_WORKSPACE_NAME	     	     = "colorWorkspace",
    *const COLOR_FG_NAME	     	     = "colorFg",
    *const COLOR_BG_NAME	     	     = "colorBg";


/* Error Message Information */

typedef struct _ColorGlobals {
	String noAllocCells;
	String noAllocColor;
	String noAllocRgb;
	String noColorUpdate;
	String noColorUseBlack;
	String noDeskSetColors;
	String noPaletteUpdate;
	String noPatternDb;
	String noPatternDir;
	String noPatternUpdate;
	String borderColorString;
} ColorGlobals, *ColorGlobalsPtr;

static ColorGlobals	colorGlobals;

static XtResource globalAppResources[] = {
        { "noAllocCells", "NoAllocCells", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noAllocCells), XtRImmediate,
          "Cannot allocate read/write colorcells."
        },
        { "noAllocColor", "NoAllocColor", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noAllocColor), XtRImmediate,
          "Cannot allocate colorcell for color \"%s\". Using black."
        },
        { "noAllocRgb", "NoAllocRgb", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noAllocRgb), XtRImmediate,
          "Cannot allocate colorcell for RGB %d %d %d."
        },
        { "noColorUpdate", "NoColorUpdate", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noColorUpdate), XtRImmediate,
          "Cannot convert color \"%s\". Unable to update color."
        },
        { "noColorUseBlack", "NoColorUseBlack", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noColorUseBlack), XtRImmediate,
          "Cannot convert color \"%s\". Using black."
        },
        { "noDeskSetColors", "NoDeskSetColors", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noDeskSetColors), XtRImmediate,
          "Cannot get _SUN_DESKSET_COLORS from root window."
        },
        { "noPaletteUpdate", "NoPaletteUpdate", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noPaletteUpdate), XtRImmediate,
          "Cannot set color palette button number %d."
        },
        { "noPatternDb", "NoPatternDb", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noPatternDb), XtRImmediate,
          "Cannot load color pattern database \"%s\"."
        },
        { "noPatternDir", "NoPatternDir", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noPatternDir), XtRImmediate,
          "Cannot find pattern directory \"%s\"."
        },
        { "noPatternUpdate", "NoPatternUpdate", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, noPatternUpdate), XtRImmediate,
          "Cannot convert color \"%s\". Unable to update pattern."
        },
        { "borderColorString", "BorderColorString", XtRString, sizeof(String),
          XtOffset(ColorGlobalsPtr, borderColorString), XtRImmediate,
          "#bbbbbb"
        }
};


/* Static Globals */

extern Widget		propsPopup;		/* Global to props */

static XtWorkProcId	workProcId;
static XColor		olColors[OLTotal];
static DualColor	paletteColors[COLOR_PALETTE_CHIPS];
static Widget		colorChoosers[ColorTotal],
			colorCategory;
static TileInfo	       *tileInfo,
		       *currentTile = NULL,
		       *backupTile = NULL;
static char	       *pattDir = NULL;
static Graphics_info   *olgxInfo = NULL;
static XFontStruct     *olgxGlyphFont = NULL;
static XFontSet		olgxButtonFontSet = NULL;
static Pixel		huePixels[COLOR_BLOCK_TOTAL],
			satPixels[COLOR_BLOCK_TOTAL],
			briPixels[COLOR_BLOCK_TOTAL];
static XrmDatabase	pattDb;
static Colormap		colormap = NULL;
static Boolean		staticVisual;
static ColorTypeInfo	colorTypes[ColorTotal] = {
    {COLOR_WINDOW_FOREGROUND_RES,   "#000000", {{0L,0,0,0,0,0}, {0L,0,0,0,0,0},
	{0,0,0}, 0, 0}, 0, 0},
    {COLOR_WINDOW_BACKGROUND_RES,   "#cccccc", {{0L,0,0,0,0,0}, {0L,0,0,0,0,0},
        {0,0,0}, 0, 0}, 0, 0},
    {COLOR_DATA_FOREGROUND_RES,     "#000000", {{0L,0,0,0,0,0}, {0L,0,0,0,0,0},
        {0,0,0}, 0, 0}, 0, 0},
    {COLOR_DATA_BACKGROUND_RES,     "#ffffff", {{0L,0,0,0,0,0}, {0L,0,0,0,0,0},
        {0,0,0}, 0, 0}, 0, 0},
    {COLOR_WORKSPACE_BITMAP_FG_RES, "#000000", {{0L,0,0,0,0,0}, {0L,0,0,0,0,0},
        {0,0,0}, 0, 0}, 0, 0},
    {COLOR_WORKSPACE_BITMAP_BG_RES, "#40a0c0", {{0L,0,0,0,0,0}, {0L,0,0,0,0,0},
        {0,0,0}, 0, 0}, 0, 0}
};
static PreviewInfo     *previewInfo;

/* Registration Data & Function */

static CategoryInfo info = {
    "color",
    colorCreateCategory,
    colorCreateChangeBars,
    colorDeleteChangeBars,
    colorBackupSettings,
    colorRestoreSettings,
    colorReadDbSettings,
    colorSaveDbSettings,
    wspNoopRealizeSettings,
    wspNoopInitializeServer,
    wspNoopSyncWithServer,
    colorShowCategory,
    colorHideCategory
};

CategoryInfo*
colorRegisterCategory(void)
{
    XVisualInfo	*visualInfo;

    /*
     * Check to see if color category can handle the available visual
     */
    visualInfo = colorGetVisualInfo();
    if (visualInfo != NULL) { /* Doesn't handle DirectColor visuals */
    	if (visualInfo->class == DirectColor) {
	    return(NULL);
	}
    }
    return(&info);
}


/*
 *  Color Category Widget Tree
 *  --------------------------
 *
 *  propsMain (bulletinBoard)
 *  	colorCategory (controlArea)
 *			
 *  		colorWindows (caption)
 *  			previewButton (oblongButton)
 *  		colorType (controlArea)
 *  			colorFg (caption)
 *  				control (controlArea)
 *  					chooserButton (rectButton)
 *  					colorPatch (drawArea)
 *  			colorBg (caption)
 *  				control (controlArea)
 *  					chooserButton (rectButton)
 *  					colorPatch (drawArea)
 *  		colorDataAreas (caption)
 *  		colorType (controlArea)
 *  			colorFg (caption)
 *  				control (controlArea)
 *  					chooserButton (rectButton)
 *  					colorPatch (drawArea)
 *  			colorBg (caption)
 *  				control (controlArea)
 *  					chooserButton (rectButton)
 *  					colorPatch (drawArea)
 *  		colorWorkspace (caption)
 *  		colorType (controlArea)
 *  			colorFg (caption)
 *  				control (controlArea)
 *  					chooserButton (rectButton)
 *  					colorPatch (drawArea)
 *  			colorBg (caption)
 *  				control (controlArea)
 *  					chooserButton (rectButton)
 *  					colorPatch (drawArea)
 *  			pattern (caption)
 *  				control (controlArea)
 *  					abbrev (abbrevMenuButton)
 *  					exclusive (exclusives)
 *  						button00 (rectButton)
 *  						.  .  .
 *  						.  .  .
 *  						.  .  .
 *  						buttonxx (rectButton)
 *  					patternPatch (drawArea)
 *
 *  		colorChooser00 (popupWindowShell)
 *  			choosers (bulletinBoard)
 *  				palette (exclusives)
 *  					button00 (rectButton)
 *  					.  .  .
 *  					.  .  .
 *  					.  .  .
 *  					button71 (rectButton)
 *  				customChoice (controlArea)
 *  					exclusive (exclusives)
 *  						button00 (rectButton)
 *  						button01 (rectButton)
 *  					colorPatch (drawArea)
 *  				custom (controlArea)
 *  					hue (caption)
 *  						control (controlArea)
 *  							slider (slider)
 *  							draw (drawArea)
 *  					saturation (caption)
 *  						control (controlArea)
 *  							slider (slider)
 *  							draw (drawArea)
 *  					brightness (caption)
 *  						control (controlArea)
 *  							slider (slider)
 *  							draw (drawArea)
 *  		.			.			.
 *  		.			.			.
 *  		.			.			.
 *  		colorChooser06 (popupWindowShell)
 *  			same as above
 *
 *  		previewPopup (popupWindowShell)
 *  			draw (drawArea)
 *  	
 */                                   

/* Public Color Functions */

void
colorGetColors(
	PreviewInfo    *previewInfo)
{
	static Boolean	firstTime = TRUE;

#if 0
	if (!colorsInitialized) {
		/* 
		 * If another category is created before the color category
		 * and it uses colorGetColors(), then colormap needs to be set.
		 */
		colormap = DefaultColormapOfScreen(screen);
		colorsInitialized = colorInitializeColorCells();
	}
#endif
	if (firstTime) {
		ColorType	ii;
		XColor		xColor;

		/* 
		 * If another category is created before the color category
		 * and it uses colorGetColors(), then colormap needs to be set
		 * and the color cells need to be initialized.
		 */
		if (colormap == NULL) {
		    colormap = DefaultColormapOfScreen(screen);
		    (void) colorInitializeColorCells();
		}

		/* Get xcolors from resources */

		for (ii = 0; ii < ColorTotal; ii++) {
		    colorResourceToXColor(colorTypes[ii].resourceName,
		        colorTypes[ii].defaultColorName, &xColor,
		        (ACCESS_RWIN_DB | ACCESS_USER_DB | ACCESS_DFLT_DB));
            	    colorCopyXColorRgb(&colorTypes[ii].color.xColor, &xColor);
            	    colorAllocateDynamicColor(&xColor,
                	&colorTypes[ii].color.screenColor,
                	BlackPixelOfScreen(screen));
            	    colorXColorToHsv(&xColor, &colorTypes[ii].color.hsv);
		}
		colorUpdateOlColors();

		firstTime = FALSE;
	}
	previewInfo->windowFg =
		colorTypes[ColorWindowForeground].color.screenColor.pixel;
	previewInfo->windowBg =
		colorTypes[ColorWindowBackground].color.screenColor.pixel;
	previewInfo->dataFg =
		colorTypes[ColorDataForeground].color.screenColor.pixel;
	previewInfo->dataBg =
		colorTypes[ColorDataBackground].color.screenColor.pixel;
	previewInfo->workspaceBg =
		colorTypes[ColorWorkspaceBackground].color.screenColor.pixel;
	previewInfo->workspaceFg =
		colorTypes[ColorWorkspaceForeground].color.screenColor.pixel;
	previewInfo->olbg2 = olColors[OLBG2].pixel;
	previewInfo->olbg3 = olColors[OLBG3].pixel;
	previewInfo->olhl  = olColors[OLHighlight].pixel;
}

XVisualInfo*
colorGetVisualInfo(void)
{
    static XVisualInfo *visualInfo = NULL;
    XVisualInfo		visualTemp;
    int			ii;

    if (visualInfo == NULL) {
        if (visual != NULL) {
            visualTemp.visualid = XVisualIDFromVisual(visual);
            visualInfo = XGetVisualInfo(display,
		VisualIDMask, &visualTemp, &ii);
        }
    }
    return(visualInfo);
}

void
colorGetWorkspacePattern(
	PreviewInfo    *previewInfo)
{
	if (pattDir == NULL) {
		colorGetPatternDirectory();
	}
	if (currentTile == NULL) {
		currentTile = colorGetWorkspaceBackground();
		/* currentTile can still be NULL */
	}

	if (currentTile != NULL) {
		previewInfo->pattern = currentTile->pixmap;
	} else {
		previewInfo->pattern = NULL;
	}
}

/* Private Color Category Functions */

static Widget
colorCreateCategory(
    Widget	 propsMain)
{
    char        *pattDbFile;

    /* Create color control area */

    colorCategory = XtVaCreateManagedWidget("colorCategory",
            controlAreaWidgetClass, propsMain,
	    	XtNmappedWhenManaged,   FALSE,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)colorCategory, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    /* Get application resources */

    XtGetApplicationResources(colorCategory, &colorGlobals,
	    globalAppResources,   XtNumber(globalAppResources), NULL, 0);

    colorGetPatternDirectory();

    pattDbFile = XtMalloc(strlen(pattDir)+strlen(COLOR_PATTERN_ATTR_FILE)+2);
    (void) sprintf(pattDbFile, "%s/%s", pattDir, COLOR_PATTERN_ATTR_FILE);
    pattDb = XrmGetFileDatabase(pattDbFile);
    if (pattDb == NULL) {
	wspErrorFooter(colorGlobals.noPatternDb, pattDbFile, NULL);
    }

    colormap = DefaultColormapOfScreen(screen);

    /* Create color type areas with foreground & background choices */

    colorCreateTypeWindows(COLOR_WINDOWS_NAME);
    colorCreateTypeDataareas(COLOR_DATA_AREAS_NAME);
    colorCreateTypeWorkspace(COLOR_WORKSPACE_NAME);

    if (!colorInitializeColorCells()) {
	return NULL;
    }
    previewInfo = previewCreate(colorCategory);
    previewGetColors(previewInfo, TRUE);
    colorInitializeOlgx(previewInfo);

    return(colorCategory);
}

/* ARGSUSED */
static void
colorBackupSettings(
    Widget  widget,
    Boolean preserve)
{
    int		ii;

    for (ii = 0; ii < ColorTotal; ii++) {
        if (preserve && colorTypes[ii].color.changed) {
            continue;
        } else {
            colorCopyXColorRgb(&colorTypes[ii].color.backXColor,
                                  &colorTypes[ii].color.xColor);
        }
    }
    colorBackupTiles(preserve);

    if (!preserve)
	backupTile = currentTile;
}

static void
colorBackupTiles(
    Boolean preserve)
{
    int		ii;
    TileInfo   *current;

    for (current = tileInfo; current != NULL; current = current->next) {
	for (ii = Fg; ii <= Bg; ii++ ) {
	    if (preserve && current->colors[ii].changed) {
		continue;
	    } else {
		colorCopyXColorRgb(&current->colors[ii].backXColor,
				      &current->colors[ii].xColor);
	    }
	}
    }
}

/* ARGSUSED */
static void
colorCreateChangeBars(
    Widget 	widget)
{
    int    	ii;
    TileInfo   *current;
    Widget      caption;

    for (ii = 0; ii < ColorTotal; ii++) {
        if (!colorEqualXColorRgb(&colorTypes[ii].color.xColor, 
                                    &colorTypes[ii].color.backXColor)) {
            colorTypes[ii].color.changed = TRUE;
	    XtVaSetValues(colorTypes[ii].caption,
		XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);
        }
    }

    /*
     * Since all tiles have the same caption, it is not stored
     * with each tile, but retrieved from the widget tree instead.
     */

    caption = XtNameToWidget(colorCategory, "*colorType*pattern");

    /* Set the pattern change bar if a tile's fg or bg color changed */

    for (current = tileInfo; current != NULL; current = current->next) {
	for (ii = Fg; ii <= Bg; ii++) {
	    if (!colorEqualXColorRgb(&current->colors[ii].xColor,
					&current->colors[ii].backXColor)) {
		if (caption != NULL) {
		    XtVaSetValues(caption,
			XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);
		}
		current->colors[ii].changed = TRUE;
	    }
	}
    }

    /* Set the pattern change bar if the current tile has changed */

    if (currentTile != backupTile) {
	if (caption != NULL) {
	    XtVaSetValues(caption,
		XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);
	}
    }
}

/* ARGSUSED */
static void
colorDeleteChangeBars(
    Widget	widget)
{
    int    	ii;
    TileInfo   *current;
    Widget      caption;

    for (ii = 0; ii < ColorTotal; ii++) {
        colorTypes[ii].color.changed = FALSE;
	XtVaSetValues(colorTypes[ii].caption,
		XtNchangeBar, (XtArgVal)OL_NONE, NULL);
    }
    for (current = tileInfo; current != NULL; current = current->next) {
	for (ii = Fg; ii <= Bg; ii++) {
	    current->colors[ii].changed = FALSE;
	}
    }

    /*
     * Since all tiles have the same caption, it is not stored with each tile,
     * but retrieved from the widget tree instead and need only be set once.
     */

    caption = XtNameToWidget(colorCategory, "*colorType*pattern");

    if (caption != NULL) {
        XtVaSetValues(caption, XtNchangeBar, (XtArgVal)OL_NONE, NULL);
    }
}

/* ARGSUSED */
static void
colorHideCategory(
    Widget	widget)
{
    Widget	chooserPopup;
    char	name[80];
    int		ii;

    for (ii = 0; ii < ColorTotal; ii++) {
	(void) sprintf(name, "*colorChooser%02d", ii);
	chooserPopup = XtNameToWidget(colorCategory, name);
	if (chooserPopup != NULL)
		XtUnmapWidget(chooserPopup);
    }
}

/* ARGSUSED */
static void
colorReadDbSettings(
    Widget	widget,
    int		dbAccess,
    Boolean	preserve)
{
    int		ii;
    XrmValue	value;
    char       *type;
    XColor	xColor, xColors[2];
    TileInfo   *current, *tile, *foregroundTile, *backgroundTile;
    Boolean	colorSpecified,
		updateTile[2];	/* foreground & background */
    char	name[80];
    Widget	tileButton;
    char       *resourceNames[]   = { COLOR_WORKSPACE_BITMAP_FG_RES, 
				       COLOR_WORKSPACE_BITMAP_BG_RES };
    char       *resourceFormats[] = { COLOR_WORKSPACE_FOREGROUND_FORMAT, 
				       COLOR_WORKSPACE_BACKGROUND_FORMAT };
    XColor	workspacePlane[2];
    XVisualInfo *visualInfo = colorGetVisualInfo();

    /* Read color type information */

    for (ii = 0; ii < ColorTotal; ii++) {
        if (preserve && colorTypes[ii].color.changed) {
            continue;
        } else {
            if (!wspGetResource(colorTypes[ii].resourceName, dbAccess,
                    &value)) {
                value.addr = colorTypes[ii].defaultColorName;
                value.size = strlen(colorTypes[ii].defaultColorName) + 1;
            }
            if (!colorStringToXColor(value.addr, &xColor)) {
		wspErrorFooter(colorGlobals.noColorUpdate, value.addr, NULL);

                continue;
            }

	    colorCopyXColorRgb(&colorTypes[ii].color.xColor, &xColor);
	    colorAllocateDynamicColor(&xColor,
		&colorTypes[ii].color.screenColor,
		BlackPixelOfScreen(screen));
	    colorXColorToHsv(&xColor, &colorTypes[ii].color.hsv);

	    colorUpdateChoosers(ii);
	    colorUpdateColorPatch(ii);
        }
    }

    colorUpdateOlColors();

    /* Read color tile information */

    if (visualInfo->depth > 4) { /* Skip for 4-bit or less visuals */
        for (current = tileInfo; current != NULL; current = current->next) {
	    updateTile[Fg] = updateTile[Bg] = FALSE;
	    for (ii = Fg; ii <= Bg; ii++) {
		if (preserve && current->colors[ii].changed) {
		    continue;
		}
	        (void)sprintf(name, resourceFormats[ii], current->fileName);
	        if (!wspGetResource(name, dbAccess, &value)) {
	            XrmGetResource(pattDb, name, name, &type, &value);
	        }
	        if (value.size != 0) {
	            if (colorStringToXColor(value.addr, &xColors[ii])) {
			if (!colorEqualXColorRgb(&xColors[ii],
				&(current->colors[ii].xColor))) {
		            updateTile[ii] = TRUE;
			}
		    } else {
		        wspErrorFooter(colorGlobals.noPatternUpdate, value.addr,
			    NULL);
		    }
	        }
	    }
	    if (updateTile[Fg] && updateTile[Bg]) {
 	        colorUpdateTile(current, &xColors[Fg], &xColors[Bg]);
	    } else if (updateTile[Fg]) {
 	        colorUpdateTile(current, &xColors[Fg], NULL);
	    } else if (updateTile[Bg]) {
 	        colorUpdateTile(current, NULL, &xColors[Bg]);
	    }
        }
    }

    /*
     * Update the workspace background pattern
     */

    tile = NULL;
    colorSpecified = FALSE;
    xColor.flags = DoRed | DoGreen | DoBlue;

    /* Get a pointer to the current tile */

    if (preserve &&
	(currentTile != backupTile ||
         colorTypes[ColorWorkspaceForeground].color.changed ||
         colorTypes[ColorWorkspaceBackground].color.changed) ) {

	/*
	 * In this situation the user has changed the tile or one of its 
	 * colors but hasn't yet applied the change so don't update the tile
	 * with the values in the database.
	 */

	tile = currentTile;

    } else {


    if (wspGetResource(COLOR_WORKSPACE_STYLE_RES, dbAccess, &value) &&
	(strcmp(value.addr, "tilebitmap") == 0)) {

	if (wspGetResource(COLOR_WORKSPACE_BITMAP_FILE_RES, dbAccess, &value)) {

	    tile = colorGetTileByName(value.addr);

	    if (tile == NULL) { /* This tile not yet loaded - load it */
		char      name[MAXPATHLEN];

                (void) sprintf(name, "%s/%s", pattDir, value.addr);
		tile = colorCreateSingleTile(value.addr, name);
		if (backupTile == NULL) {
		    backupTile = tile;
		}
	    }
        }
    }

    foregroundTile = colorGetTileByName(COLOR_FOREGROUND_BITMAP_FILE);
    backgroundTile = colorGetTileByName(COLOR_BACKGROUND_BITMAP_FILE);

    /* Get workspace bitmap foreground and background colors from resources */

    if (tile != NULL) {
	for (ii = Fg; ii <= Bg; ii++) {
	    if (wspGetResource(resourceNames[ii], dbAccess, &value) &&
		value.size > 0 &&
		colorStringToXColor(value.addr, &workspacePlane[ii])) {

			  colorSpecified = TRUE;
	    } else {
	        if (ii == Fg) {
		    workspacePlane[Fg].pixel = BlackPixelOfScreen(screen);
	        }
	        if (ii == Bg) {
		    workspacePlane[Bg].pixel = WhitePixelOfScreen(screen);
	        }
	        XQueryColor(display, colormap, &workspacePlane[ii]);
	    }
	}
     } else { /* No valid bitmap tile is specified in the resources */
	XColor workspaceColor;

	/* Get workspace color defaulting to white */

	if (wspGetResource(COLOR_WORKSPACE_COLOR_RES, dbAccess, &value) &&
	    value.size > 0 && colorStringToXColor(value.addr, &workspaceColor)){

		colorExactXColorRgb(&workspaceColor);
	} else {
		workspaceColor.pixel = WhitePixelOfScreen(screen);
		XQueryColor(display, colormap, &workspaceColor);
	}

	/* Get workspace background & foreground defaulting to white & black */

	for (ii = Fg; ii <= Bg; ii++) {
	    if (wspGetResource(resourceNames[ii], dbAccess, &value) &&
		value.size > 0 &&
		colorStringToXColor(value.addr, &workspacePlane[ii])) {

		    colorExactXColorRgb(&workspacePlane[ii]);
	    } else {
		if (ii == Fg) {
		    workspacePlane[Fg].pixel = BlackPixelOfScreen(screen);
		}
		if (ii == Bg) {
		    workspacePlane[Bg].pixel = WhitePixelOfScreen(screen);
		}
		XQueryColor(display, colormap, &workspacePlane[ii]);
	    }
	}

	/*
	 * If the workspace color matches with the foreground tile's color
	 * use the foreground tile. If the workspace color matches with the
	 * background tile's color, use the background tile. Otherwise, use
	 * the background tile and set its color to the workspace color.
	 */

	tile = backgroundTile;
	if (colorEqualXColorRgb(&workspaceColor, &workspacePlane[Fg])) {
		tile = foregroundTile;
	} else if (!colorEqualXColorRgb(&workspaceColor,
	             			   &workspacePlane[Bg])) {
	    colorCopyXColorRgb(&workspacePlane[Bg], &workspaceColor);
	    colorSpecified = TRUE;
	}
     }

}

    previewCalculate(previewInfo);

    (void) sprintf(name, "*pattern*button%02d", tile->id);
    tileButton = XtNameToWidget(colorCategory, name);
    if (tileButton != NULL) {
        XtVaSetValues(tileButton, XtNset, TRUE, NULL);
    }
    currentTile = tile;
    if (colorSpecified) {
        colorUpdateTile(currentTile, &workspacePlane[Fg], &workspacePlane[Bg]);
    }
    colorUpdateWorkspaceColors(currentTile);
    colorUpdateWorkspaceChoosers();
    colorUpdateTileChooser();
    colorUpdateWorkspacePreviews();

    previewGetColors(previewInfo, FALSE);
    previewRefreshWindowBg(previewInfo);
    previewRefreshWindowFg(previewInfo);
    previewRefreshDataBg(previewInfo);
    previewRefreshDataFg(previewInfo);
}

/* ARGSUSED */
static void
colorRestoreSettings(
    Widget	widget)
{
    int 	ii;
    TileInfo   *current;
    Boolean	needToUpdate;
    XColor     *tileColors[2];
    Widget	tileButton;
    char	name[40];

    /* Restore color type values */

    for (ii = 0; ii < ColorTotal; ii++) {
        if (colorTypes[ii].color.changed) {
	    colorAllocateDynamicColor(&colorTypes[ii].color.backXColor,
		&colorTypes[ii].color.screenColor, BlackPixelOfScreen(screen));
	    colorCopyXColorRgb(&colorTypes[ii].color.xColor,
		&colorTypes[ii].color.backXColor);
	    colorXColorToHsv(&colorTypes[ii].color.xColor, 
		&colorTypes[ii].color.hsv);

            colorTypes[ii].color.paletteIndex =
                colorPaletteGetButton(&colorTypes[ii].color.xColor);
	    colorUpdateChoosers(ii);
	    colorUpdateColorPatch(ii);
        }
    }
    colorUpdateOlColors();

    /* Restore tile fg/bg colors & current tile */

    for (current = tileInfo; current != NULL; current = current->next) {
	needToUpdate = FALSE;
	for (ii = Fg; ii <= Bg; ii++) {
	    tileColors[ii] = NULL;
	    if (current->colors[ii].changed) {
	    	current->colors[ii].paletteIndex =
	            colorPaletteGetButton(&current->colors[ii].backXColor);
		tileColors[ii] = &current->colors[ii].backXColor;
		needToUpdate = TRUE;
	    }
	}
	if (needToUpdate) {
	    colorUpdateTile(current, tileColors[Fg], tileColors[Bg]);
	}
    }
    if (backupTile != NULL) {
	(void)sprintf(name, "*pattern*button%02d", backupTile->id);
	tileButton = XtNameToWidget(colorCategory, name);
	if (tileButton != NULL)
	    XtVaSetValues(tileButton, XtNset, TRUE, NULL);
    }
    currentTile = backupTile;
    if (currentTile != NULL) {
	colorUpdateWorkspaceColors(currentTile);
	colorUpdateWorkspaceChoosers();
	colorUpdateTileChooser();
	colorUpdateWorkspacePreviews();
    }
    if (staticVisual) {
	/* ColorWindowBackground causes all parts of window to be refreshed */
	previewGetColors(previewInfo, FALSE);
	previewRefreshAsRequired(previewInfo, ColorWindowBackground);
    }
}

/* ARGSUSED */
static void
colorSaveDbSettings(
    Widget	widget,
    int		dbAccess)
{
    int 	ii;
    TileInfo   *current, *foregroundTile, *backgroundTile;
    char	name[80], *colorName;

    /* Save current colors */

    for (ii = 0; ii < ColorTotal; ii++) {
        if (colorTypes[ii].color.changed) {
            char *string;

            string = colorXColorToString(&colorTypes[ii].color.xColor);
            wspPutResource(colorTypes[ii].resourceName, string, dbAccess);
            XtFree(string);
        }
    }

    /* Save color tile foregrounds and backgrounds */

    foregroundTile = colorGetTileByName(COLOR_FOREGROUND_BITMAP_FILE);
    backgroundTile = colorGetTileByName(COLOR_BACKGROUND_BITMAP_FILE);

    for (current = tileInfo; current != NULL; current = current->next) {
	if (current == foregroundTile || current == backgroundTile) {
	    continue;
	}
	for (ii = Fg; ii <= Bg; ii++) {
	    if (current->colors[ii].changed) {
		if (ii == Fg) {
		    (void)sprintf(name, COLOR_WORKSPACE_FOREGROUND_FORMAT,
			current->fileName);
		} else {
		    (void)sprintf(name, COLOR_WORKSPACE_BACKGROUND_FORMAT,
			current->fileName);
		}
		colorName= colorXColorToString(&current->colors[ii].xColor);
		wspPutResource(name, colorName, dbAccess);
		XtFree(colorName);
	    }
	}
    }

    /* Save resources specifying how to decorate the workspace */

    if (currentTile != backgroundTile && currentTile != foregroundTile &&
	    currentTile != NULL) {
	wspPutResource(COLOR_PAINT_WORKSPACE_RES, "True", dbAccess);
	wspPutResource(COLOR_WORKSPACE_STYLE_RES, "tilebitmap", dbAccess);
	wspPutResource(COLOR_WORKSPACE_BITMAP_FILE_RES,
	    currentTile->fileName, dbAccess);
	colorName = colorXColorToString(&currentTile->colors[Fg].xColor);
	wspPutResource(COLOR_WORKSPACE_BITMAP_FG_RES, colorName, dbAccess);
	colorName = colorXColorToString(&currentTile->colors[Bg].xColor);
	wspPutResource(COLOR_WORKSPACE_BITMAP_BG_RES, colorName, dbAccess);
	wspPutResource(COLOR_WORKSPACE_COLOR_RES, colorName, dbAccess);
    } else if (currentTile == backgroundTile || currentTile == NULL) {
	wspPutResource(COLOR_PAINT_WORKSPACE_RES, "True", dbAccess);
	wspPutResource(COLOR_WORKSPACE_STYLE_RES, "paintcolor", dbAccess);
	colorName = colorXColorToString(
		&colorTypes[ColorWorkspaceBackground].color.xColor);
	wspPutResource(COLOR_WORKSPACE_COLOR_RES, colorName, dbAccess);
    } else if (currentTile == foregroundTile) {
	wspPutResource(COLOR_PAINT_WORKSPACE_RES, "True", dbAccess);
	wspPutResource(COLOR_WORKSPACE_STYLE_RES, "paintcolor", dbAccess);
	colorName = colorXColorToString(&currentTile->colors[Fg].xColor);
	wspPutResource(COLOR_WORKSPACE_COLOR_RES, colorName, dbAccess);
    }
}

/* ARGSUSED */
static void
colorShowCategory(
    Widget	widget)
{
    Widget	chooserPopup;
    char	name[80];
    int		ii, present;

    for (ii = 0; ii < ColorTotal; ii++) {
	(void)sprintf(name, "*colorChooser%02d", ii);
	chooserPopup = XtNameToWidget(colorCategory, name);
	if (chooserPopup != NULL) {
	    XtVaGetValues(chooserPopup, XtNuserData, (XtArgVal)&present, NULL);
	    if (present) {
		XtMapWidget(chooserPopup);
	    }
	}
    }
}


/*
 * Color Category Callbacks
 */

static void
colorCustomSliderCb(
    Widget		widget,
    XtPointer		clientData,
    XtPointer		callData)
{
    OlSliderVerify     *sliderInfo = (OlSliderVerify*)callData;
    int		        colorId = (int)clientData;
    Widget		parent, grandparent, caption;
    String		name;

    wspClearFooter();

    parent = XtParent(widget);
    grandparent = XtParent(parent);
    name = XtName(grandparent);

    if (strcmp(name, "hue") == 0) {
       colorTypes[colorId].color.hsv.h = sliderInfo->new_location;
    } else if (strcmp(name, "saturation") == 0) {
       colorTypes[colorId].color.hsv.s = sliderInfo->new_location;
    } else if (strcmp(name, "brightness") == 0) {
       colorTypes[colorId].color.hsv.v = sliderInfo->new_location;
    }
    colorHsvToXColor(&colorTypes[colorId].color.hsv,
			&colorTypes[colorId].color.xColor);
    colorTypes[colorId].color.changed = TRUE;
    XtVaSetValues(colorTypes[colorId].caption,
	XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);

    /*
     * Since all tiles have the same caption, it is not stored
     * with each tile, but retrieved from the widget tree instead.
     */

    caption = XtNameToWidget(colorCategory, "*colorType*pattern");
    if (colorId == ColorWorkspaceForeground && currentTile != NULL) {
	if (caption != NULL) {
	    XtVaSetValues(caption, XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);
	}
	currentTile->colors[Fg].changed = TRUE;
    } else if (colorId == ColorWorkspaceBackground && currentTile != NULL) {
	if (caption != NULL) {
	    XtVaSetValues(caption, XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);
        }
	currentTile->colors[Bg].changed = TRUE;
    }
    colorAllocateDynamicColor(&colorTypes[colorId].color.xColor,
	&colorTypes[colorId].color.screenColor, BlackPixelOfScreen(screen));

    if (colorId == ColorWindowBackground || colorId == ColorWindowForeground) {
    	colorUpdateOlColors();
    }
    if (staticVisual) {
	colorUpdateColorPatch(colorId);
	colorUpdateColorChooserPatch(colorId);
	previewGetColors(previewInfo, FALSE);
	previewRefreshAsRequired(previewInfo, colorId);
    }
}

/* ARGSUSED */
static void
colorDrawHsv(
    Widget    	widget,
    XtPointer 	clientData,
    XtPointer 	callData)
{
    GC		gc = DefaultGCOfScreen(screen);
    Window  	window;
    int		ii;
    Pixel      *pixels = (Pixel*)clientData;

    window = XtWindow(widget);

    for (ii = 0; ii < 160/COLOR_BLOCK_WIDTH; ii++) {
	XSetForeground(display, gc, pixels[ii]);
	XFillRectangle(display, window, gc, ii * COLOR_BLOCK_WIDTH, 0,
	    COLOR_BLOCK_WIDTH, COLOR_BLOCK_HEIGHT);
    }
}

/* ARGSUSED */
static void
colorPaletteCb(
    Widget    	widget,
    XtPointer 	clientData,
    XtPointer 	callData)
{
    int		colorId = (int)clientData;
    Widget	caption;
    int		index;

    wspClearFooter();

    XtVaGetValues(widget, XtNuserData, (XtArgVal)&index, NULL);
    colorCopyXColorRgb(&colorTypes[colorId].color.xColor,
	&paletteColors[index].xColor);
    colorTypes[colorId].color.xColor.flags = DoRed | DoGreen | DoBlue;
    colorTypes[colorId].color.changed = TRUE;
    XtVaSetValues(colorTypes[colorId].caption,
	XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);

    /*
     * Since all tiles have the same caption, it is not stored
     * with each tile, but retrieved from the widget tree instead.
     */

    caption = XtNameToWidget(colorCategory, "*colorType*pattern");
    if (colorId == ColorWorkspaceForeground && currentTile != NULL) {
	if (caption != NULL) {
	    XtVaSetValues(caption, XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);
	}
	currentTile->colors[Fg].changed = TRUE;
    } else if (colorId == ColorWorkspaceBackground && currentTile != NULL) {
	if (caption != NULL) {
	    XtVaSetValues(caption, XtNchangeBar, (XtArgVal)OL_NORMAL, NULL);
	}
	currentTile->colors[Bg].changed = TRUE;
    }
    colorAllocateDynamicColor(&colorTypes[colorId].color.xColor,
	&colorTypes[colorId].color.screenColor,
	BlackPixelOfScreen(screen));
    colorXColorToHsv(&colorTypes[colorId].color.xColor,
	&colorTypes[colorId].color.hsv);

    if (colorId == ColorWindowBackground || colorId == ColorWindowForeground) {
    	colorUpdateOlColors();
    }

    if (staticVisual) {
	colorUpdateColorPatch(colorId);
	colorUpdateColorChooserPatch(colorId);
	previewGetColors(previewInfo, FALSE);
	previewRefreshAsRequired(previewInfo, colorId);
    }
}

/* ARGSUSED */
static void
colorPatchCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    int		colorId = (int)clientData;
    Window	window;

    window = XtWindow(widget);
    if (window == NULL)
	return;
    XSetWindowBackground(display, window,
	colorTypes[colorId].color.screenColor.pixel);
    XClearWindow(display, window);
}
	
/* ARGSUSED */
static void
colorPatternPatchCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    Window	window;

    window = XtWindow(widget);
    if (window != NULL) {
        if (currentTile != NULL) {
	    colorTileWindow(window, currentTile->pixmap);
        } else {
	    colorPaintWindow(window,
		  colorTypes[ColorWorkspaceBackground].color.screenColor.pixel);
	}
    }
}

/* ARGSUSED */
static void
colorPopdownCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    wspClearFooter();

    /* Set flag to indicate that the popup has been popped down */
    XtVaSetValues(widget, XtNuserData, (XtArgVal)FALSE, NULL);
    XtUnmapWidget(widget);
}

/* ARGSUSED */
static void
colorPopupCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    /* Set flag to indicate that the popup has been popped up */
    XtVaSetValues(widget, XtNuserData, (XtArgVal)TRUE, NULL);
    XtMapWidget(widget);
}

/* ARGSUSED */
static void
colorPopupChooserCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    int		colorId = (int)clientData;
    Widget	button;
    char	name[80];

    wspClearFooter();

    if (colorChoosers[colorId] == NULL) {
	colorChoosers[colorId] = colorCreateColorChooser(colorId);
	XtRealizeWidget(colorChoosers[colorId]);

	/* Set the appropriate palette button */

	colorTypes[colorId].color.paletteIndex = colorPaletteGetButton(
		&colorTypes[colorId].color.xColor);
	if (colorTypes[colorId].color.paletteIndex == -1) {
	    Widget	palette,
			custom,
			customButton;

	    /* Set the custom choice button to custom */

	    customButton = XtNameToWidget(colorChoosers[colorId],
			"*customChoice*button01");
	    XtVaSetValues(customButton, XtNset, (XtArgVal)TRUE, NULL);

	    /* Show the custom chooser */

	    palette = XtNameToWidget(colorChoosers[colorId], "*palette");
   	    custom = colorCreateChooserCustom(colorId);
   	    colorCustomSetSliders(colorId);
	    XtUnmapWidget(palette);
	    XtMapWidget(custom);
	} else {

	    /* Set the custom choice button to palette */

	    button = XtNameToWidget(colorChoosers[colorId],
		"*customChoice*button00");
	    XtVaSetValues(button, XtNset, (XtArgVal)TRUE, NULL);

	    /* Set the palette button */

	    (void)sprintf(name, "*palette*button%02d",
		colorTypes[colorId].color.paletteIndex);
	    button = XtNameToWidget(colorChoosers[colorId], name);
	    XtVaSetValues(button, XtNset, (XtArgVal)TRUE, NULL);
	}
    }
    XtPopup(colorChoosers[colorId], XtGrabNone);
}

/* ARGSUSED */
static void
colorTileChooserCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    int		tileId = (int)clientData;
    int		ii;
    TileInfo   *current;

    wspClearFooter();

    for (current = tileInfo, ii = 0;
	 current != NULL;
	 current = current->next, ii++) {

	if (current->id == tileId)
	    break;
    }
    if (current != NULL) {
    	colorUpdateWorkspaceColors(current);
        colorUpdateWorkspaceChoosers();
        colorUpdateTileChooser();
	currentTile = current;
        colorUpdateWorkspacePreviews();
    }
    colorCreateChangeBars(NULL);
}

/* ARGSUSED */
static void
colorTileVerifyMenuCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    OlVirtualEvent	vEvent = (OlVirtualEvent)callData;
    Widget		exclusive = (Widget)clientData;
    int			ii;

    /* Only perform check if this is a ButtonPress event */

    if (vEvent->xevent->type == ButtonPress) {
        for (ii = 0; !colorCreateNextTile((XtPointer)exclusive); ii++);
    }
}

/* ARGSUSED */
static void
colorUpdateTileCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    int		colorId = (int)clientData;
    TileInfo   *current, *foregroundTile, *backgroundTile;
    XColor     *foregroundColor, *backgroundColor;

    if (colorId < ColorWorkspaceForeground)
	return;

    foregroundTile = backgroundTile = NULL;
    for (current = tileInfo; current != NULL; current = current->next) {
	if (strcmp(current->fileName, COLOR_BACKGROUND_BITMAP_FILE) == 0) {
	    backgroundTile = current;
	} else if (strcmp(current->fileName, COLOR_FOREGROUND_BITMAP_FILE)==0){
	    foregroundTile = current;
	}
    }
    if (colorId == ColorWorkspaceForeground) {
	foregroundColor = &colorTypes[colorId].color.xColor;
	backgroundColor = NULL;
    } else {
	foregroundColor = NULL;
	backgroundColor = &colorTypes[colorId].color.xColor;
    }
    if (foregroundTile != NULL) {
	colorUpdateTile(foregroundTile, foregroundColor, backgroundColor);
    }
    if (backgroundTile != NULL) {
	colorUpdateTile(backgroundTile, foregroundColor, backgroundColor);
    }
    if (currentTile != foregroundTile && currentTile != backgroundTile) {
	colorUpdateTile(currentTile, foregroundColor, backgroundColor);
    }
    colorUpdateTilePreview();
    previewRefreshWorkspace(previewInfo);
}

/* ARGSUSED */
static void
colorUseCustomCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    int		colorId = (int)clientData;
    Widget      custom,
		palette;

    wspClearFooter();

    palette  = XtNameToWidget(colorChoosers[colorId], "*palette");
    custom   = XtNameToWidget(colorChoosers[colorId], "*custom");

    /* Set custom sliders for current color */

    if (custom == NULL)
        custom = colorCreateChooserCustom(colorId);
    colorCustomSetSliders(colorId);

    if (palette != NULL)
    	XtUnmapWidget(palette);	/* Hide palette chooser */
    if (custom != NULL)
    	XtMapWidget(custom);	/* Show custom chooser */
}

/* ARGSUSED */
static void
colorUsePaletteCb(
    Widget	widget,
    XtPointer	clientData,
    XtPointer	callData)
{
    int		colorId = (int)clientData;
    Widget      custom,
		palette;

    wspClearFooter();

    palette = XtNameToWidget(colorChoosers[colorId], "*palette");
    custom  = XtNameToWidget(colorChoosers[colorId], "*custom");

    colorTypes[colorId].color.paletteIndex = 
	colorPaletteGetButton(&colorTypes[colorId].color.xColor);
    if (colorTypes[colorId].color.paletteIndex != -1) {
   	colorPaletteSetButton(colorId,
		colorTypes[colorId].color.paletteIndex);
    } else {
	WidgetList	children;
	Cardinal	numChildren, ii;

	XtVaGetValues(palette,
		XtNchildren, &children,
		XtNnumChildren, &numChildren,
		NULL);
	for (ii = 0; ii < numChildren; ii++) {
		XtVaSetValues(children[ii], XtNset, (XtArgVal)FALSE, NULL);
	}
    }
    if (custom != NULL)
	XtUnmapWidget(custom);	/* Hide custom chooser */
    if (palette != NULL)
	XtMapWidget(palette);	/* Show palette chooser */
}

static void
colorUpdateChoosers(
    int		colorId)
{
    colorTypes[colorId].color.paletteIndex = colorPaletteGetButton(
	&colorTypes[colorId].color.xColor);
    colorPaletteSetButton(colorId,
	colorTypes[colorId].color.paletteIndex);
    colorCustomSetSliders(colorId);
    if (staticVisual) {
	colorUpdateColorChooserPatch(colorId);
    }
}

static void
colorUpdateColorChooserPatch(
    int		colorId)
{
    Widget	colorPatch;
    char	name[80];

    /* If the color chooser doesn't exist then return */

    if (colorChoosers[colorId] == NULL) {
	return;
    }

    (void) sprintf(name, "*colorChooser%02d*colorPatch", colorId);
    colorPatch = XtNameToWidget(colorCategory, name);
    if (colorPatch != NULL) {
        Window	window;

	window = XtWindow(colorPatch);
	if (window != NULL) {
    	    XSetWindowBackground(display, window,
		colorTypes[colorId].color.screenColor.pixel);
    	    XClearWindow(display, window);
	}
    }
}

static void
colorUpdateColorPatch(
    int		colorId)
{
    Widget	colorPatch;
    char	name[80];

    switch (colorId) {
    case ColorWindowForeground:
	(void) sprintf(name, "*%s*%s*colorPatch",
	    COLOR_WINDOWS_NAME, COLOR_FG_NAME);
	break;
    case ColorWindowBackground:
	(void) sprintf(name, "*%s*%s*colorPatch",
	    COLOR_WINDOWS_NAME, COLOR_BG_NAME);
	break;
    case ColorDataForeground:
	(void) sprintf(name, "*%s*%s*colorPatch",
	    COLOR_DATA_AREAS_NAME, COLOR_FG_NAME);
	break;
    case ColorDataBackground:
	(void) sprintf(name, "*%s*%s*colorPatch",
	    COLOR_DATA_AREAS_NAME, COLOR_BG_NAME);
	break;
    case ColorWorkspaceForeground:
	(void) sprintf(name, "*%s*%s*colorPatch",
	    COLOR_WORKSPACE_NAME, COLOR_FG_NAME);
	break;
    case ColorWorkspaceBackground:
	(void) sprintf(name, "*%s*%s*colorPatch",
	    COLOR_WORKSPACE_NAME, COLOR_BG_NAME);
	break;
    }

    colorPatch = XtNameToWidget(colorCategory, name);
    if (colorPatch != NULL) {
        Window	window;

	window = XtWindow(colorPatch);
	if (window != NULL) {
    	    XSetWindowBackground(display, window,
		colorTypes[colorId].color.screenColor.pixel);
    	    XClearWindow(display, window);
	}
    }
}

static void
colorUpdateTile(
    TileInfo   *tile,
    XColor     *foreground,
    XColor     *background)
{
    Pixel	oldPixels[2];
    int		pixelCount = 0;
    Boolean	needToUpdate = FALSE;

    if (tile == NULL)
	return;

    if (foreground != NULL) {
	oldPixels[pixelCount] = tile->colors[Fg].screenColor.pixel;
	pixelCount++;
	if (!colorAllocateFixedColor(foreground, &tile->colors[Fg].screenColor,
		tile->colors[Fg].screenColor.pixel)) {

            wspErrorFooter(colorGlobals.noAllocRgb, foreground->red,
                foreground->green, foreground->blue,
                NULL);
        } else {
	    colorCopyXColorRgb(&tile->colors[Fg].xColor, foreground);
	    colorXColorToHsv(&tile->colors[Fg].xColor, &tile->colors[Fg].hsv);
            needToUpdate = TRUE;
        }
    }
    if (background != NULL) {
	oldPixels[pixelCount] = tile->colors[Bg].screenColor.pixel;
	pixelCount++;
	if (!colorAllocateFixedColor(background, &tile->colors[Bg].screenColor,
		tile->colors[Bg].screenColor.pixel)) {

            wspErrorFooter(colorGlobals.noAllocRgb, background->red,
                background->green, background->blue,
                NULL);
        } else {
	    colorCopyXColorRgb(&tile->colors[Bg].xColor, background);
	    colorXColorToHsv(&tile->colors[Bg].xColor, &tile->colors[Bg].hsv);
            needToUpdate = TRUE;
        }
    }
    if (needToUpdate) {
	Pixmap	oldPixmap;
	XImage *oldImage;
	Widget	tileButton;
	char	name[80];

	oldPixmap = tile->pixmap;
	oldImage = tile->ximage;
	colorCreateTilePixmap(tile->bitmap, &tile->pixmap,
	    tile->width, tile->height,
	    tile->colors[Fg].screenColor.pixel,
	    tile->colors[Bg].screenColor.pixel);
	tile->ximage = XGetImage(display, tile->pixmap, 0, 0,
	    COLOR_PATTERN_SIZE, COLOR_PATTERN_SIZE, 0xffffffL, ZPixmap);
	XFreeColors(display, colormap, oldPixels, pixelCount, 0L);
	XFreePixmap(display, oldPixmap);
	XDestroyImage(oldImage);
	(void)sprintf(name, "*pattern*button%02d", tile->id);
	tileButton = XtNameToWidget(colorCategory, name);
	if (tileButton != NULL) {
	    XtVaSetValues(tileButton, XtNlabelImage, tile->ximage, NULL);
	}
    }
}

static void
colorUpdateTileChooser()
{
    TileInfo   *current,
	       *foregroundTile,
	       *backgroundTile;
    XColor     *foregroundColor,
	       *backgroundColor;

    foregroundTile = backgroundTile = NULL;
    for (current = tileInfo; current != NULL; current = current->next) {
	if (strcmp(current->fileName, COLOR_BACKGROUND_BITMAP_FILE) == 0) {
	    backgroundTile = current;
	} else if (strcmp(current->fileName, COLOR_FOREGROUND_BITMAP_FILE)==0){
	    foregroundTile = current;
	}
    }
    foregroundColor = &colorTypes[ColorWorkspaceForeground].color.xColor;
    backgroundColor = &colorTypes[ColorWorkspaceBackground].color.xColor;

    if (foregroundTile != NULL) {
	colorUpdateTile(foregroundTile, foregroundColor, backgroundColor);
    }
    if (backgroundTile != NULL) {
	colorUpdateTile(backgroundTile, foregroundColor, backgroundColor);
    }
}

static void
colorUpdateTilePreview()
{
    Widget patternPatch;

    patternPatch = XtNameToWidget(colorCategory, "*patternPatch");
    if (patternPatch == NULL)
	return;

    if (currentTile != NULL) {
	colorTileWindow(XtWindow(patternPatch), currentTile->pixmap);
    } else {
	colorPaintWindow(XtWindow(patternPatch),
		colorTypes[ColorWorkspaceBackground].color.screenColor.pixel);
    }
}
    
static void
colorUpdateWorkspaceColors(
    TileInfo   *tileInfo)
{

    /* Update workspace foreground & background colors */

    colorAllocateDynamicColor(&tileInfo->colors[Fg].xColor,
	&colorTypes[ColorWorkspaceForeground].color.screenColor,
	BlackPixelOfScreen(screen));
    colorCopyXColorRgb(&colorTypes[ColorWorkspaceForeground].color.xColor,
	&tileInfo->colors[Fg].xColor);
    colorXColorToHsv(&colorTypes[ColorWorkspaceForeground].color.xColor,
	      &colorTypes[ColorWorkspaceForeground].color.hsv);

    if (!colorEqualXColorRgb(
		&colorTypes[ColorWorkspaceForeground].color.xColor,
		&colorTypes[ColorWorkspaceForeground].color.backXColor)) {
	colorTypes[ColorWorkspaceForeground].color.changed = TRUE;
    }
    colorAllocateDynamicColor(&tileInfo->colors[Bg].xColor,
	&colorTypes[ColorWorkspaceBackground].color.screenColor,
	BlackPixelOfScreen(screen));
    colorCopyXColorRgb(&colorTypes[ColorWorkspaceBackground].color.xColor,
	&tileInfo->colors[Bg].xColor);
    colorXColorToHsv(&colorTypes[ColorWorkspaceBackground].color.xColor,
	      &colorTypes[ColorWorkspaceBackground].color.hsv);

    if (!colorEqualXColorRgb(
		&colorTypes[ColorWorkspaceBackground].color.xColor,
		&colorTypes[ColorWorkspaceBackground].color.backXColor)) {
	colorTypes[ColorWorkspaceBackground].color.changed = TRUE;
    }
}

static void
colorUpdateWorkspaceChoosers()
{
    colorUpdateChoosers(ColorWorkspaceBackground);
    colorUpdateChoosers(ColorWorkspaceForeground);
}

static void
colorUpdateWorkspacePreviews()
{
    colorUpdateTilePreview();
    previewRefreshWorkspace(previewInfo);
    if (staticVisual) {
	colorUpdateColorPatch(ColorWorkspaceForeground);
	colorUpdateColorPatch(ColorWorkspaceBackground);
    }
}


/*
 * Color category supporting functions
 */

static void
colorTileListInsert(
    TileInfo   **newInfo)
{
    TileInfo		*previous,
			*current;
    char		 resource[MAXPATHLEN];

    /* Query database for tile's rank */
 
    (void) sprintf(resource, COLOR_WORKSPACE_RANK_FORMAT, (*newInfo)->fileName);    colorGetTileRank(*newInfo, resource);
 
    /* If the list is empty the new entry goes at the beginning */
 
    if (tileInfo == NULL) {
        tileInfo = *newInfo;
        (*newInfo)->next = NULL;
        return;
    }    
 
    /* Find position for new entry - lowest rank first, highest rank last */
 
    for (previous = current = tileInfo;
         current != NULL &&
	 ((*newInfo)->rank >= current->rank || ((*newInfo)->rank == 0));
         previous = current, current = current->next) {

        /* Don't insert a duplicate entry */

	if ((*newInfo)->rank == current->rank &&
	    strcmp((*newInfo)->filePath, current->filePath) == 0) {

	    return;
	}
    }
 
    /* Insert at head of list */
 
    if (current == tileInfo) {
        (*newInfo)->next = tileInfo;
        tileInfo = *newInfo;
        return;
    }    
 
    /* Insert within list */
 
    if (current != NULL) {
        (*newInfo)->next = current;
        previous->next = *newInfo;
        return;
    }    
  
    /* Insert at end of list */
 
    if (current == NULL) {
        previous->next = *newInfo;
        (*newInfo)->next = NULL;
        return;
    }
}

static void
colorTileWindow(
    Window	window,
    Pixmap	pixmap)
{
    XSetWindowBackgroundPixmap(display, window, pixmap);
    XClearWindow(display, window);
}

static void
colorAllocateDynamicColor(
    XColor     *original,
    XColor     *onscreen,
    Pixel	defaultPixel)
{
    if (staticVisual) {
	(void) colorAllocateFixedColor(original, onscreen, defaultPixel);
    } else {
	colorCopyXColorRgb(onscreen, original);
	XStoreColor(display, colormap, onscreen);
    }
}

static Boolean
colorAllocateFixedColor(
    XColor     *original,
    XColor     *onscreen,
    Pixel	defaultPixel)
{
    int		status;

    colorCopyXColorRgb(onscreen, original);
    status = XAllocColor(display, colormap, onscreen);
    if (status == 0 ) {
	onscreen->pixel = defaultPixel;
        XQueryColor(display, colormap, onscreen);
	return(FALSE); /* Unable to allocate requested color, used default */
    } else {
	return(TRUE);  /* Successfully allocated requested color */
    }
}

static Widget
colorCreateChooserPalette(
    int		colorId,
    Widget	choosers,
    char       *dscolorString)
{
    Widget	palette,
		button,
		customChoice,
		exclusive,
		colorPatch;
    char	buttonName[20];
    XImage     *chipImage;
    static unsigned char chipData[] = {
                  0x7F, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFE };
    char       *colorName;
    int         ii, hueIndex, satIndex, briIndex, row;
    XVisualInfo *visualInfo = colorGetVisualInfo();
    Boolean	firstPass = TRUE;

    hueIndex = satIndex = briIndex = 0;

    palette = XtVaCreateManagedWidget("palette",
            exclusivesWidgetClass, choosers,
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)palette, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    chipImage = XCreateImage(display, visual, 1, XYBitmap, 0,
            (char*)chipData, 16, 16, 8, 0);

    /* Read the deskset colors into array of paletteColors */

    for (ii = 0, colorName = strtok((char*)dscolorString, " ");
         colorName != NULL && ii < COLOR_PALETTE_CHIPS;
         colorName = strtok(NULL, " ")) {

	if (firstPass) { /* Skip first entry in colors which is colormap id */
	    firstPass = FALSE;
	    continue;
	}
        if (!colorStringToXColor(colorName, &paletteColors[ii].xColor)) {
	    wspErrorFooter(colorGlobals.noColorUseBlack, colorName, NULL);

	    paletteColors[ii].xColor.red   = 0;
	    paletteColors[ii].xColor.green = 0;
	    paletteColors[ii].xColor.blue  = 0;
	}
	/* 
	 * In the case of a 4-bit dynamic visual, don't use up the colormap
	 * by allocating new colors. For colors which are not already allocated
	 * just use black. This should leave two colorcells for changing the
	 * workspace pattern colors via the custom chooser.
	 */
	if (visualInfo->depth == 4 && !staticVisual) {
	    if (!colorIsCurrentlyUsed(&paletteColors[ii].xColor)) {
		paletteColors[ii].xColor.red   = 0;
		paletteColors[ii].xColor.green = 0;
		paletteColors[ii].xColor.blue  = 0;
	    }
	}
	if (!colorAllocateFixedColor(&paletteColors[ii].xColor,
	    &paletteColors[ii].screenColor, BlackPixelOfScreen(screen))) {

	    wspErrorFooter(colorGlobals.noAllocColor, colorName, NULL);
	}

	if (visualInfo->depth == 4) {
	    row = 3;	/* Colors in the 3rd row appear better on 4-bit */
	} else {
	    row = 5;	/* Colors in the 5th row appear better with more bits */
	}
        if (ii > 7 && ((ii - row) % 8 == 0)) {
            huePixels[hueIndex++] = paletteColors[ii].screenColor.pixel;
	}
        if (ii > 51 && ii < 56) {
            satPixels[satIndex++] = paletteColors[ii].screenColor.pixel;
            satPixels[satIndex++] = paletteColors[ii].screenColor.pixel;
        }
        if (ii < 8) {
            briPixels[briIndex++] = paletteColors[ii].screenColor.pixel;
	}
        ii++;
    }

    /* Create the palette buttons for the canned deskset colors */

    for (ii = 0; ii < COLOR_PALETTE_CHIPS; ii++) {
        (void) sprintf(buttonName, "button%02d", ii);
        button = XtVaCreateManagedWidget(buttonName,
            rectButtonWidgetClass, palette,
            XtNlabelImage, chipImage,
	    XtNfontColor, paletteColors[ii].screenColor.pixel,
	    XtNuserData, ii,
            NULL);
        XtAddCallback(button, XtNselect, colorPaletteCb, (XtPointer)colorId);
	XtAddCallback(button, XtNselect, colorUpdateTileCb,
	    (XtPointer)colorId);
    }
    customChoice = XtVaCreateManagedWidget("customChoice",
	controlAreaWidgetClass, choosers,
	NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)customChoice, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    exclusive = XtVaCreateManagedWidget("exclusive",
	exclusivesWidgetClass, customChoice,
	NULL);
    for (ii = 0; ii < 2; ii++) {
	(void) sprintf(buttonName, "button%02d", ii);
	button = XtVaCreateManagedWidget(buttonName,
	    rectButtonWidgetClass, exclusive,
	    NULL);
	if (ii == 0) {
	    XtAddCallback(button, XtNselect, colorUsePaletteCb,
		(XtPointer)colorId);
	} else {
	    XtAddCallback(button, XtNselect, colorUseCustomCb,
		(XtPointer)colorId);
	}
    }
    colorPatch = XtVaCreateManagedWidget("colorPatch",
	drawAreaWidgetClass, customChoice,
	NULL);
    /*
     * Use the border color specified in the app-defaults file unless the
     * visual has a depth 4 or less in which case use black to save a colorcell.
     */
    if (visualInfo->depth > 4) {
	XColor	xColor;

	if (!colorStringToXColor(colorGlobals.borderColorString, &xColor)) {
	    wspErrorFooter(colorGlobals.noColorUseBlack,
		colorGlobals.borderColorString, NULL);

	    xColor.red = xColor.green = xColor.blue = 0;
	}
        if (!colorAllocateFixedColor(&xColor, &xColor,
	    BlackPixelOfScreen(screen))) {

	    wspErrorFooter(colorGlobals.noAllocColor,
		colorGlobals.borderColorString, NULL);
        }
	XtVaSetValues(colorPatch, XtNborderColor, xColor.pixel, NULL);
    } else { /* Save a colorcell by using black */
	XtVaSetValues(colorPatch,
	    XtNborderColor, BlackPixelOfScreen(screen),
	    NULL);
    }
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)colorPatch, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(colorPatch, XtNexposeCallback, colorPatchCb,
	(XtPointer)colorId);

    return(palette);
}

static Widget
colorCreateChooserCustom(
    int    colorId)
{
    Widget custom,
           choosers,
           hue,
           saturation,
           brightness;

    choosers = XtNameToWidget(colorChoosers[colorId], "*choosers");
    
    custom = XtVaCreateManagedWidget("custom",
            controlAreaWidgetClass, choosers,
            XtNmappedWhenManaged, FALSE, /* Map after unmapping current chsr */
            NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)custom, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    hue        = colorCreateCustomSlider(colorId, custom, huePixels,
				"hue");
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)hue, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    saturation = colorCreateCustomSlider(colorId, custom, satPixels,
				"saturation");
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)saturation, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    brightness = colorCreateCustomSlider(colorId, custom, briPixels,
				"brightness");
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)brightness, NULL,
            OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    
    XtRealizeWidget(custom);
    return(custom);
}

static Widget
colorCreateCustomSlider(
    int		colorId,
    Widget	parent,
    Pixel      *pixels,
    const char *name)
{
    Widget	caption,
              	control,
              	slider,
              	draw;

    caption = XtVaCreateManagedWidget(name,
            captionWidgetClass, parent,
            NULL);

    control = XtVaCreateManagedWidget("control",
            controlAreaWidgetClass, caption,
            NULL);

    slider = XtVaCreateManagedWidget("slider",
            sliderWidgetClass, control,
            NULL);
    XtAddCallback(slider, XtNsliderMoved, colorCustomSliderCb,
            (XtPointer)colorId);
    XtAddCallback(slider, XtNsliderMoved, colorUpdateTileCb,
            (XtPointer)colorId);

    draw = XtVaCreateManagedWidget("draw",
            drawAreaWidgetClass, control,
            NULL);

    XtAddCallback(draw, XtNexposeCallback, colorDrawHsv, (XtPointer)pixels);

    return(caption);
}

static Widget
colorCreateColorChooser(
    int		colorId)
{
    Widget	colorChooser,
		upper,
		choosers,
		palette,
		customChoice;
    Dimension	width, height;
    Atom	retType, atom;
    int         retFormat,
		status;
    unsigned long retItems,
		retRemain;
    unsigned char *dscolorString;
    char	choosername[20];
    static char *DefaultColorString = "%ld "
	"#000000 #000000 #4c4c4c #666666 #999999 #b2b2b2 #cccccc #ffffff "
	"#bf9898 #bf7272 #bf4c4c #bf2626 #e5b7b7 #e58989 #e55b5b #e52d2d "
	"#bfb498 #bfaa72 #bf9e4c #bf9426 #e5d8b7 #e5cb89 #e5be5b #e5b12d "
	"#bdbf98 #bbbf72 #b9bf4c #b7bf26 #e3e5b7 #e0e589 #dee55b #dce52d "
	"#98bfa2 #72bf86 #4cbf69 #26bf4c #b7e5c2 #89e5a0 #5be57e #2de55b "
	"#98bfbf #72bfbf #4cbfbf #26bfbf #b7e5e5 #89e5e5 #5be5e5 #2de5e5 "
	"#98a2bf #7286bf #4c69bf #264cbf #b7c2e5 #89a0e5 #5b7ee5 #2d5be5 "
	"#b298bf #a572bf #984cbf #8c26bf #d5b7e5 #c689e5 #b75be5 #a72de5 "
	"#bf98b6 #bf72ac #bf4ca2 #bf2698 #e5b7da #e589ce #e55bc2 #e52db7";

    (void) sprintf(choosername, "colorChooser%02d", colorId);

    colorChooser = XtVaCreatePopupShell(choosername,
	popupWindowShellWidgetClass, colorCategory,
	XtNmappedWhenManaged, FALSE,
	NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)colorChooser, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(colorChooser, XtNpopupCallback, colorPopupCb,
	(XtPointer)NULL);
    XtAddCallback(colorChooser, XtNpopdownCallback, colorPopdownCb,
	(XtPointer)NULL);
    XtVaGetValues(colorChooser, XtNupperControlArea, (XtArgVal)&upper, NULL);

    choosers = XtVaCreateManagedWidget("choosers",
        bulletinBoardWidgetClass, upper,
        NULL);

    /* Get the DeskSet colors */

    atom = XInternAtom(display, "_SUN_DESKSET_COLORS", False);
    status = XGetWindowProperty(display, RootWindowOfScreen(screen),
            atom, 0L, 100000000L, False, XA_STRING,
            &retType, &retFormat, &retItems, &retRemain,
            &dscolorString);

    if (status != Success || retType != XA_STRING || retItems == 0 ||
	retRemain != 0 || dscolorString == NULL) {
	    wspErrorStandard("colorCreateColorChooser()",
		colorGlobals.noDeskSetColors, NULL);
	    
	    /* Allocate space for default color string plus colormap id */
 	    dscolorString = (unsigned char*)XtMalloc(
		strlen(DefaultColorString) + 20);
	    (void) sprintf((char*)dscolorString, DefaultColorString,
		DefaultColormapOfScreen(screen));
    }
    palette = colorCreateChooserPalette(colorId, choosers,
	(char*)dscolorString);
    XtRealizeWidget(colorChooser);
    XtVaSetValues(palette, XtNnoneSet, TRUE, NULL);

    /* Place the custom color choice on the color chooser */

    customChoice = XtNameToWidget(colorChooser, "*customChoice");
    XtVaGetValues(palette, XtNheight, (XtArgVal)&height, NULL);
    XtVaSetValues(customChoice, XtNy, height + 4, NULL);

    /* Place the color chooser on the screen */

    XtVaGetValues(propsPopup, XtNwidth, (XtArgVal)&width, NULL);
    XtVaSetValues(colorChooser, /* Don't put all choosers in the same place */
	XtNx, (width + 10),
	XtNy, (20 * colorId),
	NULL);
    return(colorChooser);
}

static Boolean
colorCreateNextTile(
    XtPointer       exclusive)
{
    static Boolean  firstTime = TRUE;
    static DIR     *dp;
    struct dirent  *de;
    
    if (firstTime) {
	dp = opendir(pattDir);
	firstTime = FALSE;
    }
    if (dp != NULL) {
        de = readdir(dp);
        if (de != NULL) {
            char      name[MAXPATHLEN];

            (void) sprintf(name, "%s/%s", pattDir, de->d_name);
            if (colorRegularFile(name)) {
		/* Create a new tile in the list */
		(void) colorCreateSingleTile(de->d_name, name);
            }
        } else { /* No more files in the directory so close it */
	    char 	name[80];
            Widget	tileButton;

	    (void) closedir(dp);
	    dp = NULL;
            colorCreateTileButtons((Widget)exclusive);
	    colorBackupTiles(FALSE);
	    (void) sprintf(name, "*pattern*button%02d", currentTile->id);
	    tileButton = XtNameToWidget(colorCategory, name);
	    if (tileButton != NULL) {
		XtVaSetValues(tileButton, XtNset, TRUE, NULL);
	    }
        }
    }
    if (dp == NULL) {
	return TRUE; /* Done */
    } else {
        return FALSE; /* Not done */
    }
}

static Widget
colorCreateSubtype(
    int		colorId,
    const char *name,
    Widget	parent)
{
    Widget	caption,
		control,
		colorPatch,
		chooserButton;
    XVisualInfo *visualInfo = colorGetVisualInfo();

    caption = XtVaCreateManagedWidget(name,
	captionWidgetClass, parent,
	NULL);

    control = XtVaCreateManagedWidget("control",
	controlAreaWidgetClass, caption,
	NULL);

    chooserButton = XtVaCreateManagedWidget("chooserButton",
	oblongButtonWidgetClass, control,
	NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)chooserButton, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(chooserButton, XtNselect, colorPopupChooserCb,
	(XtPointer)colorId);

    colorPatch = XtVaCreateManagedWidget("colorPatch",
	drawAreaWidgetClass, control,
	NULL);
    /*
     * Use the border color specified in the app-defaults file unless the
     * visual has a depth 4 or less in which case use black to save a colorcell.
     */
    if (visualInfo->depth > 4) {
	XColor	xColor;

	if (!colorStringToXColor(colorGlobals.borderColorString, &xColor)) {
	    wspErrorFooter(colorGlobals.noColorUseBlack,
		colorGlobals.borderColorString, NULL);

	    xColor.red = xColor.green = xColor.blue = 0;
	}
        if (!colorAllocateFixedColor(&xColor, &xColor,
	    BlackPixelOfScreen(screen))) {

	    wspErrorFooter(colorGlobals.noAllocColor,
		colorGlobals.borderColorString, NULL);
        }
	XtVaSetValues(colorPatch, XtNborderColor, xColor.pixel, NULL);
    } else { /* Save a colorcell by using black */
	XtVaSetValues(colorPatch,
	    XtNborderColor, BlackPixelOfScreen(screen),
	    NULL);
    }
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)colorPatch, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(colorPatch, XtNexposeCallback, colorPatchCb,
	(XtPointer)colorId);

    return(caption);
}

static TileInfo*
colorCreateSingleTile(
    const char *fileName,
    const char *filePath)
{
    TileInfo   *newInfo;

    newInfo = XtNew(TileInfo);

    if (newInfo != NULL) {

        newInfo->fileName = XtNewString(fileName);
        newInfo->filePath = XtNewString(filePath);

        if (colorGetTilePixmap(newInfo)) {
            newInfo->next = NULL;
            colorTileListInsert(&newInfo);
            newInfo->ximage = XGetImage(display, newInfo->pixmap, 0, 0,
	        COLOR_PATTERN_SIZE, COLOR_PATTERN_SIZE, 0xffffffL, ZPixmap);
            newInfo->colors[Fg].changed = FALSE;
            newInfo->colors[Bg].changed = FALSE;
        } else {
            XtFree(newInfo->fileName);
            XtFree(newInfo->filePath);
            XtFree((char*)newInfo);
	    newInfo = NULL;
        }
    }
    return newInfo;
}

static void
colorCreateTileMenu(
    const char *thename,
    Widget	parent)
{
    Widget	pattern,
		abbrev,
		menuPane,
		exclusive,
		control,
		patternPatch;
    DIR	       *dp;
    Boolean	noPatterns = FALSE;
    XVisualInfo *visualInfo = colorGetVisualInfo();

    pattern = XtVaCreateManagedWidget(thename,
	captionWidgetClass, parent,
	NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)pattern, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);

    control = XtVaCreateManagedWidget("control",
	controlAreaWidgetClass, pattern,
	NULL);

    abbrev = XtVaCreateManagedWidget("abbrev",
	abbrevMenuButtonWidgetClass, control,
	NULL);

    XtVaGetValues(abbrev, XtNmenuPane, (XtArgVal)&menuPane, NULL);

    exclusive = XtVaCreateManagedWidget("exclusive",
	exclusivesWidgetClass, menuPane,
	NULL);

    XtAddCallback(abbrev, XtNconsumeEvent, colorTileVerifyMenuCb,
	(XtPointer)exclusive);

    /* Start workproc here to create color patterns in the background */

    workProcId = XtAppAddWorkProc(appContext, colorCreateNextTile, exclusive);

    dp = opendir(pattDir);

    if (dp == NULL) {
	wspErrorFooter(colorGlobals.noPatternDir, pattDir, NULL);

	noPatterns = TRUE;

    } else { /* Load foreground & background tiles */
	char      name[MAXPATHLEN],
		 *planeStrings[] = { "foreground.xbm", "background.xbm" };
	int	  ii;

	for (ii = Fg; ii <= Bg; ii++) {
            (void) sprintf(name, "%s/%s", pattDir, planeStrings[ii]);
	    /* Create a new tile in the list */
	    (void) colorCreateSingleTile(planeStrings[ii], name);
        }
	(void) closedir(dp);
    }
    patternPatch = XtVaCreateManagedWidget("patternPatch",
	drawAreaWidgetClass, control,
	NULL);
    /*
     * Use the border color specified in the app-defaults file unless the
     * visual has a depth 4 or less in which case use black to save a colorcell.
     */
    if (visualInfo->depth > 4) {
	XColor	xColor;

	if (!colorStringToXColor(colorGlobals.borderColorString, &xColor)) {
	    wspErrorFooter(colorGlobals.noColorUseBlack,
		colorGlobals.borderColorString, NULL);

	    xColor.red = xColor.green = xColor.blue = 0;
	}
        if (!colorAllocateFixedColor(&xColor, &xColor,
	    BlackPixelOfScreen(screen))) {

	    wspErrorFooter(colorGlobals.noAllocColor,
		colorGlobals.borderColorString, NULL);
        }
	XtVaSetValues(patternPatch, XtNborderColor, xColor.pixel, NULL);
    } else { /* Save a colorcell by using black */
	XtVaSetValues(patternPatch,
	    XtNborderColor, BlackPixelOfScreen(screen),
	    NULL);
    }
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)patternPatch, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    XtAddCallback(patternPatch, XtNexposeCallback, colorPatternPatchCb,
	    (XtPointer)NULL);

    /* Grey out the pattern chooser if no patterns are available */

    if (noPatterns) {
	XtVaSetValues(pattern,
 	    XtNsensitive, (XtArgVal)FALSE,
	    XtNdim,       (XtArgVal)TRUE,
	    NULL);
	XtVaSetValues(abbrev,
	    XtNsensitive, (XtArgVal)FALSE,
	    XtNdim,       (XtArgVal)TRUE,
	    NULL);
	XtVaSetValues(patternPatch,
	    XtNsensitive, (XtArgVal)FALSE,
	    XtNdim,       (XtArgVal)TRUE,
	    NULL);
     }
}

static void
colorCreateTileButtons(
    Widget	parent)
{
    int		ii;
    Widget	button;
    char	name[10];
    TileInfo   *current;

    for (current = tileInfo, ii = 0;
	 current != NULL;
	 current = current->next, ii++) {

	current->id = ii;
	(void) sprintf(name, "button%02d", ii);
	button = XtVaCreateManagedWidget(name,
		rectButtonWidgetClass, parent,
		XtNlabelImage, current->ximage,
		NULL);
	XtAddCallback(button, XtNselect, colorTileChooserCb,
		(XtPointer)ii);
    }
}

static void
colorCreateTilePixmap(
    Pixmap      bitmap,
    Pixmap     *pixmap,
    int         width,
    int         height,
    Pixel       foreground,
    Pixel       background)
{
    GC		gc = DefaultGCOfScreen(screen);
    int		xx,
		yy;
    int		actualWidth,
		actualHeight;
    XVisualInfo *visualInfo = colorGetVisualInfo();

    XSetForeground(display, gc, foreground);
    XSetBackground(display, gc, background);

    if (width < COLOR_PATTERN_SIZE) {
	actualWidth = COLOR_PATTERN_SIZE;
    } else {
	actualWidth = width;
    }
    if (height < COLOR_PATTERN_SIZE) {
	actualHeight = COLOR_PATTERN_SIZE;
    } else {
	actualHeight = height;
    }
    *pixmap = XCreatePixmap(display, RootWindowOfScreen(screen),
        actualWidth, actualHeight, visualInfo->depth);
    for(yy = 0; yy < COLOR_PATTERN_SIZE; yy += height) {
	for(xx = 0; xx < COLOR_PATTERN_SIZE; xx += width) {
		XCopyPlane(display, bitmap, *pixmap, gc, 0, 0, width, height,
			xx, yy, 1);
	}
    }
}

static void
colorCreateTypeDataareas(
    const char *name)
{
    Widget	caption,
    		control,
    		subtype;
			
    caption = XtVaCreateManagedWidget(name,
    	captionWidgetClass, colorCategory,
    	NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)caption, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
	
    control = XtVaCreateManagedWidget("colorType",
    	controlAreaWidgetClass, caption,
    	NULL);

    subtype = colorCreateSubtype(ColorDataForeground, COLOR_FG_NAME, control);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)subtype, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    colorTypes[ColorDataForeground].caption = subtype;

    subtype = colorCreateSubtype(ColorDataBackground, COLOR_BG_NAME, control);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)subtype, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    colorTypes[ColorDataBackground].caption = subtype;
}

static void
colorCreateTypeWindows(
    const char *name)
{
    Widget	caption,
    		control,
    		subtype;
			
    caption = XtVaCreateManagedWidget(name,
    	captionWidgetClass, colorCategory,
    	NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)caption, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
	
    control = XtVaCreateManagedWidget("colorType",
    	controlAreaWidgetClass, caption,
    	NULL);

    subtype = colorCreateSubtype(ColorWindowForeground, COLOR_FG_NAME, control);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)subtype, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    colorTypes[ColorWindowForeground].caption = subtype;

    subtype = colorCreateSubtype(ColorWindowBackground, COLOR_BG_NAME, control);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)subtype, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    colorTypes[ColorWindowBackground].caption = subtype;

}

static void
colorCreateTypeWorkspace(
    const char *name)
{
    Widget      caption,
    		control,
    		subtype;
			
    caption = XtVaCreateManagedWidget(name,
    	captionWidgetClass, colorCategory,
    	NULL);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)caption, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
	
    control = XtVaCreateManagedWidget("colorType",
    	controlAreaWidgetClass, caption,
    	NULL);

    subtype = colorCreateSubtype(ColorWorkspaceForeground, COLOR_FG_NAME,
	control);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)subtype, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    colorTypes[ColorWorkspaceForeground].caption = subtype;

    subtype = colorCreateSubtype(ColorWorkspaceBackground, COLOR_BG_NAME,
	control);
    OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)subtype, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
    colorTypes[ColorWorkspaceBackground].caption = subtype;

    colorCreateTileMenu("pattern", control);
}

static void
colorCopyXColorRgb(
    XColor *dstx,
    XColor *srcx)
{
    dstx->red   = srcx->red;
    dstx->green = srcx->green;
    dstx->blue  = srcx->blue;
}

static void
colorCustomSetSliders(
    int		colorId)
{
    Widget 	hueSlider,
           	satSlider,
           	briSlider;
    char	name[80];

    (void) sprintf(name, "*colorChooser%02d*hue*slider", colorId);
    hueSlider = XtNameToWidget(colorCategory, name); 

    (void) sprintf(name, "*colorChooser%02d*saturation*slider", colorId);
    satSlider = XtNameToWidget(colorCategory, name);

    (void) sprintf(name, "*colorChooser%02d*brightness*slider", colorId);
    briSlider = XtNameToWidget(colorCategory, name);

    if (hueSlider != NULL) {
    	XtVaSetValues(hueSlider,
            XtNsliderValue, colorTypes[colorId].color.hsv.h,
            NULL);
    }
    if (satSlider != NULL) {
    	XtVaSetValues(satSlider,
            XtNsliderValue, colorTypes[colorId].color.hsv.s,
            NULL);
    }
    if (briSlider != NULL) {
    	XtVaSetValues(briSlider,
            XtNsliderValue, colorTypes[colorId].color.hsv.v,
            NULL);
    }
}

static Boolean
colorEqualXColorRgb(
    XColor *x1,
    XColor *x2)
{
    if (x1->red   == x2->red   &&
        x1->green == x2->green &&
        x1->blue  == x2->blue) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}

static void
colorExactXColorRgb(
    XColor     *xColor)
{
    XColor  	exactXColor;
    int		status;

    colorCopyXColorRgb(&exactXColor, xColor);
    status = XAllocColor(display, colormap, &exactXColor);
    if (status == 0) {
	wspErrorStandard("colorExactXColorRgb()", colorGlobals.noAllocRgb,
		xColor->red, xColor->green, xColor->blue, NULL);
    } else {
        colorCopyXColorRgb(xColor, &exactXColor);
        XFreeColors(display, colormap, &exactXColor.pixel, 1, 0L);
    }
}

void
colorGetPatternDirectory()
{
#ifdef DEBUG
    char	*debugPattDir;
#endif /* DEBUG */
    const char	*owHome;

    owHome = wspGetOpenwinhome();
    pattDir = XtMalloc(strlen(owHome)+strlen(COLOR_PATTERN_DIR)+2);
    (void) sprintf(pattDir, "%s/%s", owHome, COLOR_PATTERN_DIR);
#ifdef DEBUG
    debugPattDir = getenv("PROPSPATTERNS");
    if (debugPattDir != NULL) {
        if ((int)strlen(pattDir) < (int)strlen(debugPattDir)) {
            XtFree(pattDir);
            pattDir = XtMalloc(strlen(debugPattDir));
        }
        (void) sprintf(pattDir, "%s", debugPattDir);
    }
#endif /* DEBUG */
}

static TileInfo*
colorGetTileByName(
	const char     *name)
{
	TileInfo       *current;

	if (name == NULL)
		return(NULL);

	for (current = tileInfo; current != NULL; current = current->next) {
		if (strcmp(current->fileName, name) == 0) {
			break;
		}
	}
	return(current);
}

static void
colorGetTileColor(
    TileInfo   *tile,
    FgBgType    index,
    const char *resource)
{
    Boolean     setDefault;
    char       *type;
    XrmValue    value;
    ColorInfo  *colorInfo;
    Pixel	defaultPixel;
    XVisualInfo *visualInfo = colorGetVisualInfo();

    colorInfo = &tile->colors[index];

    setDefault = TRUE;

    /*
     * For 4-bit visuals it's better to use black & white for the patterns
     * than attempt to allocate colors for them. Allocating the colors in a
     * dynamic colormap uses up the colormap very quickly. Allocating the
     * colors in a static colormap often results in patterns with the same
     * foreground and background color. Hence, skip to using the default
     * black and white colors for visuals of 4 bits or less.
     */
 
    if (visualInfo->depth > 4) {

        if (!wspGetResource(resource, ACCESS_RWIN_DB | ACCESS_USER_DB, &value)){
	    XrmGetResource(pattDb, resource, resource, &type, &value);
        }
        if (value.size != 0) {
            if (colorStringToXColor(value.addr, &colorInfo->xColor)) {
                setDefault = FALSE;
            }
        }
    }
    if (index == Fg) {
	defaultPixel = BlackPixelOfScreen(screen);
    } else {
	defaultPixel = WhitePixelOfScreen(screen);
    }
    if (setDefault) {
        colorInfo->xColor.pixel = defaultPixel;
        XQueryColor(display, colormap, &colorInfo->xColor);
    }
    if (!colorAllocateFixedColor(&colorInfo->xColor, &colorInfo->screenColor,
	defaultPixel)) {

	wspErrorFooter(colorGlobals.noAllocColor, value.addr, NULL);
    }
    colorXColorToHsv(&colorInfo->xColor, &colorInfo->hsv);
    colorInfo->paletteIndex =
        colorPaletteGetButton(&colorInfo->xColor);
}
 
static Boolean
colorGetTilePixmap(
    TileInfo           *tile)
{
    int         	xHot,
			yHot;
    unsigned int    	width,
			height;
    char       	    	name[80];

    if (XReadBitmapFile(display, RootWindowOfScreen(screen),
            tile->filePath, &width, &height, &tile->bitmap,
            &xHot, &yHot) == BitmapSuccess) {
        tile->width = width;
        tile->height = height;

        /* Get foreground color */

        if (strcmp(tile->fileName, COLOR_FOREGROUND_BITMAP_FILE) == 0 ||
            strcmp(tile->fileName, COLOR_BACKGROUND_BITMAP_FILE) == 0 ) {

            (void) sprintf(name, "%s", COLOR_WORKSPACE_BITMAP_FG_RES);
        } else {
            (void) sprintf(name, COLOR_WORKSPACE_FOREGROUND_FORMAT,
		tile->fileName);
        }
        colorGetTileColor(tile, Fg, name);
 
        /* Get background color */

        if (strcmp(tile->fileName, COLOR_FOREGROUND_BITMAP_FILE) == 0 ||
            strcmp(tile->fileName, COLOR_BACKGROUND_BITMAP_FILE) == 0 ) {
 
            (void) sprintf(name, "%s", COLOR_WORKSPACE_BITMAP_BG_RES);
        } else {
            (void) sprintf(name, COLOR_WORKSPACE_BACKGROUND_FORMAT,
		tile->fileName);
        }
        colorGetTileColor(tile, Bg, name);
 
        colorCreateTilePixmap(tile->bitmap, &tile->pixmap,
            tile->width, tile->height,
            tile->colors[Fg].screenColor.pixel,
	    tile->colors[Bg].screenColor.pixel);
        return(TRUE);
    }
    return(FALSE);
}

static void
colorGetTileRank(
    TileInfo   *tile,
    const char *resource)
{
    XrmValue    value;
    char       *type;

    if (!wspGetResource(resource, ACCESS_RWIN_DB | ACCESS_USER_DB, &value)) {
        XrmGetResource(pattDb, resource, resource, &type, &value);
    }
    if (value.size == 0) {
        tile->rank = 0;
    } else {
        tile->rank = atoi(value.addr);
    }
}      

static TileInfo*
colorGetWorkspaceBackground()
{
    XrmValue	value;
    int		dbAccess = ACCESS_RWIN_DB | ACCESS_USER_DB | ACCESS_DFLT_DB;
    TileInfo   *tile = NULL;

    if (wspGetResource(COLOR_WORKSPACE_STYLE_RES, dbAccess, &value)
	    && (strcmp(value.addr, "tilebitmap") == 0)) {

	if (wspGetResource(COLOR_WORKSPACE_BITMAP_FILE_RES, dbAccess, &value)) {

	    tile = colorGetTileByName(value.addr);

	    if (tile == NULL) { /* This tile not yet loaded - load it */
		char      name[MAXPATHLEN];

                (void) sprintf(name, "%s/%s", pattDir, value.addr);
		tile = colorCreateSingleTile(value.addr, name);
		if (backupTile == NULL) {
		    backupTile = tile;
		}
	    }
        }
    }
    return tile;
}

static Boolean
colorInitializeColorCells()
{
    XVisualInfo *visualInfo = colorGetVisualInfo();
    Pixel	 pixels[ColorTotal];
    int		 status,
		 ii;

    assert(visualInfo != NULL);

    staticVisual = ((visualInfo->class % 2) == 0); /* No remainder = TRUE */

    /* Allocate r/w color cells for color types */

    if (!staticVisual) {
    	status = XAllocColorCells(display, colormap, False, NULL, 0, pixels,
    	    ColorTotal);
    	if (status == 0) {
	    wspErrorPopupConfirm(colorGlobals.noAllocCells, NULL);

	    XtRemoveWorkProc(workProcId);
	    return FALSE;
    	}
    }

    /* Initialize color type information */

    for (ii = 0; ii < ColorTotal; ii++) {
	if (!staticVisual) {
    	    colorTypes[ii].color.screenColor.pixel = pixels[ii];
    	    colorTypes[ii].color.screenColor.flags = DoRed | DoGreen | DoBlue;
	}
    	colorTypes[ii].color.paletteIndex = -1;
	colorTypes[ii].color.changed = FALSE;
    }

    /* Allocate r/w color cells for OPEN LOOK 3d colors */

    if (!staticVisual && visualInfo->depth > 4) {
    	status = XAllocColorCells(display, colormap, False, NULL, 0, pixels,
	    OLTotal);
    	if (status == 0) {
	    wspErrorPopupConfirm(colorGlobals.noAllocCells, NULL);

	    XtRemoveWorkProc(workProcId);
	    return FALSE;
    	}
        for (ii = 0; ii < OLTotal; ii++) {
    	    olColors[ii].pixel = pixels[ii];
    	    olColors[ii].flags = DoRed | DoGreen | DoBlue;
        }
    } else {
	olColors[OLHighlight].pixel = WhitePixelOfScreen(screen);
	olColors[OLBG2].pixel =
		colorTypes[ColorWindowBackground].color.screenColor.pixel;
	olColors[OLBG3].pixel = BlackPixelOfScreen(screen);
    }

    return TRUE;
}

void
colorInitializeOlgx(PreviewInfo *previewInfo)
{
    if (olgxInfo == NULL) {
        Pixel           olgxPixels[5];
        Pixmap          olgxPixmaps[1];
	XrmValue	value;
        XVisualInfo    *visualInfo = colorGetVisualInfo();
	char	      **missingCharList;
	int		missingCharCount;
	char           *defaultString;
 
        if (!wspGetResource("OpenWindows.RegularFont",
            ACCESS_RWIN_DB | ACCESS_USER_DB | ACCESS_DFLT_DB, &value)) { 
                value.addr = "-*-lucida sans-medium-r-*-*-*-120-*-*-*-*-*-*"; 
        }      
        olgxButtonFontSet = XCreateFontSet(display, value.addr,
		&missingCharList, &missingCharCount, &defaultString); 
        olgxGlyphFont = XLoadQueryFont(display, 
            "-sun-open look glyph-----*-120-*-*-*-*-*-*"); 
  
        olgxPixels[0] = olColors[OLHighlight].pixel; 
        olgxPixels[1] =
                colorTypes[ColorWindowForeground].color.screenColor.pixel; 
        olgxPixels[2] =
                colorTypes[ColorWindowBackground].color.screenColor.pixel; 
        olgxPixels[3] = olColors[OLBG2].pixel; 
        olgxPixels[4] = olColors[OLBG3].pixel; 
	if (visualInfo->depth > 1) {
            olgxInfo = olgx_i18n_initialize(display, 
                XScreenNumberOfScreen(screen), visualInfo->depth, OLGX_3D_COLOR,
                olgxGlyphFont, olgxButtonFontSet, olgxPixels, olgxPixmaps); 
	} else {
            olgxInfo = olgx_i18n_initialize(display, 
                XScreenNumberOfScreen(screen), visualInfo->depth, OLGX_2D,
                olgxGlyphFont, olgxButtonFontSet, olgxPixels, olgxPixmaps); 
	}
    }
    assert(previewInfo != NULL);

    previewInfo->olgxInfo = olgxInfo;
}

static Boolean
colorIsCurrentlyUsed(
    XColor     *newColor)
{
    int		ii;

    for (ii = 0; ii < ColorTotal; ii++) {
	if (colorEqualXColorRgb(newColor, &colorTypes[ii].color.xColor)) {
	    return TRUE;
	}
    }
    return FALSE;
}

static void
colorPaintWindow(
    Window	window,
    Pixel	pixel)
{
    XSetWindowBackground(display, window, pixel);
    XClearWindow(display, window);
}

static int
colorPaletteGetButton(
    XColor *xColor)
{
    int     ii;

    for(ii = 0; ii < COLOR_PALETTE_CHIPS; ii++) {
        if (colorEqualXColorRgb(xColor, &paletteColors[ii].xColor)) {
            return(ii);
        }
    }
    return(-1);
}

static void
colorPaletteSetButton(
    int		colorId,
    int 	buttonNumber)
{
    char   	name[80];
    Widget 	button;

    /* If the color chooser doesn't exist then return */

    if (colorChoosers[colorId] == NULL) {
	return;
    }

    /* If the color is a custom color then unset the current palette button */

    if (buttonNumber < 0 || buttonNumber > COLOR_PALETTE_CHIPS) {
	WidgetList	children;
	Widget		palette;
	int		ii;
	Boolean		buttonIsSet;
	Cardinal	numChildren;
		
	(void) sprintf(name, "*colorChooser%02d*palette", colorId);
	palette = XtNameToWidget(colorCategory, name);
	if (palette != NULL) {
	    XtVaGetValues(palette,
		XtNchildren, (XtArgVal)&children,
		XtNnumChildren, (XtArgVal)&numChildren,
		NULL);
	    for (ii = 0; ii < numChildren && children[ii] != NULL; ii++) {
		XtVaGetValues(children[ii],
		    XtNset, (XtArgVal)&buttonIsSet, NULL);
		if (buttonIsSet) {
		    XtVaSetValues(children[ii], XtNset, (XtArgVal)FALSE, NULL);
		    return;
		}
	    }
	}
	return;
    }

    /* Otherwise set the requested palette button */

    (void) sprintf(name, "*palette*button%02d", buttonNumber);
    button = XtNameToWidget(colorChoosers[colorId], name);
    if (button != NULL) {
        XtVaSetValues(button, XtNset, (XtArgVal)TRUE, NULL);
    } else {
	wspErrorFooter(colorGlobals.noPaletteUpdate, buttonNumber, NULL);
    }
}

static Boolean
colorRegularFile(
    const char *filePath)
{
    struct stat	fileStatus;

    if (stat(filePath, &fileStatus))
        return(FALSE);
    if (S_ISREG(fileStatus.st_mode))
        return(TRUE);

    return(FALSE);
}

static void
colorUpdateOlColors()
{
    XColor	olBg2,
		olBg3,
		olHighlight;
    XVisualInfo *visualInfo = colorGetVisualInfo();
	
    /*
     * There are not enough colorcells in 4-bit dynamic visuals to use
     * the true OL colors along with the other colors in props and those
     * colors used by the system. In this case the OL colors are perminantly
     * set as follows:
     *		bg2       = window background
     *		bg3       = black
     *		highlight = white
     * In this case there is no need to update the colors so simply return.
     */

    if (visualInfo->depth <= 4 && !staticVisual) {
	return;
    }

    olgx_calculate_3Dcolors(
	&colorTypes[ColorWindowForeground].color.screenColor,
	&colorTypes[ColorWindowBackground].color.screenColor,
	&olBg2, &olBg3, &olHighlight); 
    colorAllocateDynamicColor(&olBg2, &olColors[OLBG2],
	BlackPixelOfScreen(screen));
    colorAllocateDynamicColor(&olBg3, &olColors[OLBG3],
	BlackPixelOfScreen(screen));
    colorAllocateDynamicColor(&olHighlight, &olColors[OLHighlight],
	WhitePixelOfScreen(screen));

    if (staticVisual && olgxInfo != NULL) {
	olgx_set_single_color(olgxInfo, OLGX_WHITE,
		olColors[OLHighlight].pixel,
		OLGX_SPECIAL);
        olgx_set_single_color(olgxInfo, OLGX_BLACK,
	        colorTypes[ColorWindowForeground].color.screenColor.pixel, 
	        OLGX_SPECIAL);
	olgx_set_single_color(olgxInfo, OLGX_BG1,
		colorTypes[ColorWindowBackground].color.screenColor.pixel,
		OLGX_SPECIAL);
	olgx_set_single_color(olgxInfo, OLGX_BG2,
		olColors[OLBG2].pixel,
		OLGX_SPECIAL);
	olgx_set_single_color(olgxInfo, OLGX_BG3,
		olColors[OLBG3].pixel,
		OLGX_SPECIAL);
    }
}


/*
 * Color format manipulation functions
 */

static int
max3(
    int x,
    int y,
    int z)
{
    if (y > x)
        x = y;
    if (z > x)
        x = z;
    return x;
}

static int
min3(
    int x,
    int y,
    int z)
{
    if (y < x)
        x = y;
    if (z < x)
        x = z;
    return x;
}      

static void
colorHsvToRgb(
    HSV        *hsv,
    RGB        *rgb)
{
    int         h = hsv->h,
                s = hsv->s,
                v = hsv->v,
                r, g, b,
                i, f,
                p, q, t;
 
    s = (s * MAXRGB) / MAXSV;
    v = (v * MAXRGB) / MAXSV;
    if (h == 360)
        h = 0;
 
    if (s == 0) {

        h = 0;
        r = g = b = v;
    }
    i = h / 60;
    f = h % 60;
    p = v * (MAXRGB - s) / MAXRGB;
    q = v * (MAXRGB - s * f / 60) / MAXRGB;
    t = v * (MAXRGB - s * (60 - f) / 60) / MAXRGB;
 
    switch (i) {
    case 0:
        r = v, g = t, b = p;
        break;
    case 1:
        r = q, g = v, b = p;
        break;
    case 2:
        r = p, g = v, b = t;
        break;
    case 3:
        r = p, g = q, b = v;
        break;
    case 4:
        r = t, g = p, b = v;
        break;
    case 5:
        r = v, g = p, b = q;
        break;
    }
    rgb->r = r;
    rgb->g = g;
    rgb->b = b;
}

static void
colorHsvToXColor(
    HSV    *hsv,
    XColor *x)
{
    RGB     rgb;

    colorHsvToRgb(hsv, &rgb);
    colorRgbToXColor(&rgb, x);
}

static Boolean
colorResourceToXColor(
    const char *resourceName,
    const char *defaultColorName,
    XColor     *xColor,
    int		dbAccess)
{
    XrmValue	value;

    if (!wspGetResource(resourceName, dbAccess, &value)) {
        value.addr = (char *)defaultColorName;
        value.size = strlen(defaultColorName) + 1;
    }
    if (!colorStringToXColor(value.addr, xColor)) {
	return FALSE;
    } else {
	return TRUE;
    }
}

static void
colorRgbToHsv(
    RGB        *rgb,
    HSV        *hsv)
{
    int         r = rgb->r,
                g = rgb->g,
                b = rgb->b,
                h, s, v;
    register int maxv = max3(r, g, b);
    register int minv = min3(r, g, b);

    v = maxv;

    if (maxv) {
        s = (maxv - minv) * MAXRGB / maxv;
    } else {
        s = 0;
    }

    if (s == 0) {
        h = 0;
    } else {
        int         rc;
        int         gc;
        int         bc;
        int         hex;
 
        rc = (maxv - r) * MAXRGB / (maxv - minv);
        gc = (maxv - g) * MAXRGB / (maxv - minv);
        bc = (maxv - b) * MAXRGB / (maxv - minv);
 
        if (r == maxv) {
            h = bc - gc, hex = 0;
        } else if (g == maxv) {
            h = rc - bc, hex = 2;
        } else if (b == maxv) {
            h = gc - rc, hex = 4;
        }
        h = hex * 60 + (h * 60 / MAXRGB);
        if (h < 0)
            h += 360;
    }
    hsv->h = h;
    hsv->s = (s * MAXSV) / MAXRGB;
    hsv->v = (v * MAXSV) / MAXRGB;
}

static void
colorRgbToXColor(
    RGB        *rgb,
    XColor     *x)
{
    x->red = (unsigned short) rgb->r << 8;
    x->green = (unsigned short) rgb->g << 8;
    x->blue = (unsigned short) rgb->b << 8;
    x->flags = DoRed | DoGreen | DoBlue;
}

static Boolean
colorStringToXColor(
    const char *s,
    XColor     *xColor)
{
    if (!XParseColor(display, colormap, s, xColor)) {
        int         red, 
                    green,
                    blue;
        if (sscanf(s, "%d %d %d", &red, &green, &blue) == 3) {
            xColor->red = red << 8;  
            xColor->green = green << 8;
            xColor->blue = blue << 8;
            xColor->flags = DoRed | DoGreen | DoBlue;
        } else { 
            return(FALSE);
        }
    }    

    /* XParseColor may not have multiplied by 257... */  

    xColor->red &= 0xff00;
    xColor->green &= 0xff00;
    xColor->blue &= 0xff00;
    return(TRUE);
}

static void
colorXColorToHsv(
    XColor *x,
    HSV        *hsv)
{
    RGB		rgb;

    rgb.r = (int) x->red >> 8;
    rgb.g = (int) x->green >> 8;
    rgb.b = (int) x->blue >> 8;
    colorRgbToHsv(&rgb, hsv);
}

static char*
colorXColorToString(
    XColor     *x)
{
    char	s[8];

    (void)sprintf(s, "#%02x%02x%02x", x->red >> 8, x->green >> 8, x->blue >> 8);
    return(XtNewString(s));
}
