#ifndef _COLOR_CAT_H
#define _COLOR_CAT_H

#pragma ident "@(#)color.h	1.4      92/10/01 SMI"


/*
 * Defines, structures & enums
 */

/* Resource names */

#define COLOR_WORKSPACE_COLOR_RES       "OpenWindows.WorkspaceColor"
#define COLOR_WINDOW_BACKGROUND_RES     "OpenWindows.WindowColor"
#define COLOR_WINDOW_FOREGROUND_RES     "OpenWindows.WindowForeground"
#define COLOR_SELECTION_COLOR_RES       "OpenWindows.SelectionColor"
#define COLOR_DATA_BACKGROUND_RES       "OpenWindows.DataBackground"
#define COLOR_DATA_FOREGROUND_RES       "OpenWindows.DataForeground"
#define COLOR_FOREGROUND_RES            "OpenWindows.Foreground"
#define COLOR_PAINT_WORKSPACE_RES       "OpenWindows.PaintWorkspace"
#define COLOR_WORKSPACE_STYLE_RES       "OpenWindows.WorkspaceStyle"
#define COLOR_WORKSPACE_BITMAP_FG_RES   "OpenWindows.WorkspaceBitmapFg"
#define COLOR_WORKSPACE_BITMAP_BG_RES   "OpenWindows.WorkspaceBitmapBg"
#define COLOR_WORKSPACE_BITMAP_FILE_RES "OpenWindows.WorkspaceBitmapFile"
#if 0
#define COLOR_WINDOW_SCALE_RES	        "OpenWindows.Scale"
#endif

/* Color Formats */

#define MAXRGB 0xff	/* Maximum value for RGB red, green, and blue */
#define MAXSV  MAXRGB	/* Maximum value for HSV saturation & brightness */
#define MAXH   360	/* Maximum value for HSV hue */
 
#define VMUL            12              /* Brighten by 20% (12 = 1.2*10) */
#define SDIV            2               /* Unsaturate by 50% (divide by 2) */
#define VMIN            ((4*MAXSV)/10)  /* Highlight brightness 40% minimum */
 
typedef struct _HSV {	/* Hue, saturation, brightness color model */
    int h,
        s,
        v;
} HSV;
 
typedef struct _RGB {	/* Red, green, blue color model */
    int r,
        g,
        b;
} RGB;

/* Color information used for each color type and the fg/bg of patterns */

typedef struct _DualColor {
    XColor  xColor;
    XColor  screenColor;
} DualColor;

typedef struct _ColorInfo {
    XColor  xColor;
    XColor  backXColor;
    XColor  screenColor;
    HSV     hsv;
    short   paletteIndex;
    Boolean changed;
} ColorInfo;

/* Colors set from props */

#ifndef COLOR_TYPE
#define COLOR_TYPE
typedef enum {
    ColorWindowForeground,	/* 0 */
    ColorWindowBackground,	/* 1 */
    ColorDataForeground, 	/* 2 */
    ColorDataBackground, 	/* 3 */
    ColorWorkspaceForeground,	/* 4 */
    ColorWorkspaceBackground,	/* 5 */
    ColorTotal           	/* 6 */
} ColorType;
#endif /* COLOR_TYPE */
 
typedef struct _ColorTypeInfo {
    const char *resourceName;
    char       *defaultColorName;
    ColorInfo	color;
    Widget      caption;
    short       tileIndex;
} ColorTypeInfo;

/* Information for color patterns (tiles) */

typedef struct _TileInfo {
    int		id;
    int		rank;
    char       *filePath;
    char       *fileName;
    Pixmap	bitmap;
    Pixmap	pixmap;
    XImage     *ximage;
    int     	width;
    int     	height;
    ColorInfo	colors[2];
    struct _TileInfo *next;
} TileInfo;

typedef enum {
    Fg,	/* 0 */
    Bg	/* 1 */
} FgBgType;

#define COLOR_PATTERN_SIZE	32	/* Width & height of tile */

/* Color Chooser Characteristics */

#define COLOR_PALETTE_CHIPS     72	/* Color chips on palette   */
#define COLOR_BLOCK_WIDTH	20	/* Used in custom chooser hsv images */
#define COLOR_BLOCK_HEIGHT	16	/* Used in custom chooser hsv images */
#define COLOR_BLOCK_TOTAL	 8
 
/* OPEN LOOK 3D Colors */

typedef struct _CustomInfo {
    XColor  xColor;
    HSV     hsv;
} CustomInfo;
 
typedef enum {
    OLHighlight,/* 0 */
    OLBG2,      /* 1 */
    OLBG3,      /* 2 */
    OLTotal	/* 3 */
} OLColorType;
 
/*
 * Function Declarations
 */
 
/* Color public functions */

void		colorGetColors(PreviewInfo*);
XVisualInfo    *colorGetVisualInfo(void);
void		colorGetWorkspacePattern(PreviewInfo*);
#if 0
void		fontsGetFontSets(PreviewInfo*);
#endif
void		colorInitializeOlgx(PreviewInfo*);

/* Color category functions */

static void    	colorBackupSettings(Widget, Boolean);
static void	colorBackupTiles(Boolean);
static Widget  	colorCreateCategory(Widget);
static void    	colorCreateChangeBars(Widget);
static void    	colorDeleteChangeBars(Widget);
static void	colorHideCategory(Widget);
static void    	colorReadDbSettings(Widget, int, Boolean);
CategoryInfo   *colorRegisterCategory(void);
static void    	colorRestoreSettings(Widget);
static void    	colorSaveDbSettings(Widget, int);
static void	colorShowCategory(Widget);
 
/* Color category callbacks */
 
static void    	colorCustomSliderCb(Widget, XtPointer, XtPointer);
static void    	colorDrawHsv(Widget, XtPointer, XtPointer);
static void	colorPaletteCb(Widget, XtPointer, XtPointer);
static void	colorPatchCb(Widget, XtPointer, XtPointer);
static void	colorPatternPatchCb(Widget, XtPointer, XtPointer);
static void	colorPopdownCb(Widget, XtPointer, XtPointer);
static void	colorPopupCb(Widget, XtPointer, XtPointer);
static void	colorPopupChooserCb(Widget, XtPointer, XtPointer);
static void	colorTileChooserCb(Widget, XtPointer, XtPointer);
static void	colorTileVerifyMenuCb(Widget, XtPointer, XtPointer);
static void	colorUpdateTileCb(Widget, XtPointer, XtPointer);
static void    	colorUseCustomCb(Widget, XtPointer, XtPointer);
static void    	colorUsePaletteCb(Widget, XtPointer, XtPointer);

/* Color category creation functions */

static Widget   colorCreateChooserPalette(int, Widget, char*);
static Widget   colorCreateChooserCustom(int);
static Widget   colorCreateCustomSlider(int, Widget, Pixel*, const char*);
static Widget	colorCreateColorChooser(int);
static Boolean	colorCreateNextTile(XtPointer);
static Widget	colorCreateSubtype(int, const char*, Widget);
static TileInfo *colorCreateSingleTile(const char*, const char*);
static void	colorCreateTileMenu(const char*, Widget);
static void	colorCreateTileButtons(Widget);
static void     colorCreateTilePixmap(Pixmap, Pixmap*, int, int, Pixel,
			Pixel);
static void	colorCreateTypeWindows(const char*);
static void	colorCreateTypeDataareas(const char*);
static void	colorCreateTypeWorkspace(const char*);

/* Color category supporting functions */
 
static void	colorAllocateDynamicColor(XColor*, XColor*, Pixel);
static Boolean	colorAllocateFixedColor(XColor*, XColor*, Pixel);
static void    	colorCopyXColorRgb(XColor*, XColor*);
static void     colorCustomSetSliders(int);
static Boolean  colorEqualXColorRgb(XColor*, XColor*);
static void  	colorExactXColorRgb(XColor*);
static void	colorGetPatternDirectory(void);    
static TileInfo	*colorGetTileByName(const char*);
static void	colorGetTileColor(TileInfo*, FgBgType, const char*);
static Boolean	colorGetTilePixmap(TileInfo*);
static void	colorGetTileRank(TileInfo*, const char*);
static TileInfo	*colorGetWorkspaceBackground(void);
static Boolean	colorInitializeColorCells(void);
static Boolean  colorIsCurrentlyUsed(XColor*);
static void	colorPaintWindow(Window, Pixel);
static int     	colorPaletteGetButton(XColor*);
static void    	colorPaletteSetButton(int, int);
static Boolean	colorRegularFile(const char*);
static void	colorTileListInsert(TileInfo**);
static void    	colorTileWindow(Window, Pixmap);
static void	colorUpdateChoosers(int);
static void	colorUpdateColorChooserPatch(int);
static void	colorUpdateColorPatch(int);
static void	colorUpdateTile(TileInfo*, XColor*, XColor*);
static void	colorUpdateTileChooser(void);
static void	colorUpdateTilePreview(void);
static void    	colorUpdateOlColors(void);
static void	colorUpdateWorkspaceColors(TileInfo*);
static void	colorUpdateWorkspaceChoosers(void);
static void 	colorUpdateWorkspacePreviews(void);
 
/* Color format manipulation functions */
 
static void    	colorHsvToRgb(HSV*, RGB*);
static void    	colorHsvToXColor(HSV*, XColor*);
static Boolean	colorResourceToXColor(const char*, const char*, XColor*, int);
static void    	colorRgbToHsv(RGB*, HSV*);
static void    	colorRgbToXColor(RGB*, XColor*);
static Boolean 	colorStringToXColor(const char*, XColor*);
static void    	colorXColorToHsv(XColor*, HSV*);
static char    *colorXColorToString(XColor*);
 
#endif /* _COLOR_CAT_H */
