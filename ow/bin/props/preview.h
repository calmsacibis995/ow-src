#ifndef _PREVIEW_H
#define _PREVIEW_H

#pragma ident "@(#)preview.h	1.0      93/06/10 SMI"

#include <olgx/olgx.h>

#ifndef COLOR_TYPE
#define COLOR_TYPE
typedef enum {
    ColorWindowForeground,      /* 0 */
    ColorWindowBackground,      /* 1 */
    ColorDataForeground,        /* 2 */
    ColorDataBackground,        /* 3 */
    ColorWorkspaceForeground,   /* 4 */
    ColorWorkspaceBackground,   /* 5 */
    ColorTotal                  /* 6 */
} ColorType;
#endif /* COLOR_TYPE */

/* Scale for color preview */

#define PREVIEW_SCALE_RES	"OpenWindows.Scale"

typedef enum {
    ScaleSmall,         /* 0 */
    ScaleMedium,        /* 1 */
    ScaleLarge,         /* 2 */
    ScaleExtraLarge,    /* 3 */
    ScaleScalable,	/* 4 */
    ScaleMax		/* 5 */
} ScaleType;


/*
 * Structures
 */

/* Each user of a preview should have a separate one of each of these */

typedef struct _PreviewInfo {

	Widget	preview;	/* draw area widget for the preview */
	Graphics_info *olgxInfo; /* graphics context for olgx rendering */

	/* Dimensions */

        int     xl; int yl;     /* upper left of left workspace preview area */
        int     wl; int hl;     /* dimensions of left workspace preview area */
        int     xr; int yr;     /* upper left of right workspace preview area */        int     wr; int hr;     /* dimensions of right workspace preview area */        int     xt; int yt;     /* upper left of top workspace preview area */
        int     wt; int ht;     /* dimensions of top workspace preview area */
        int     xb; int yb;     /* upper left of bottom workspace prev area */
        int     wb; int hb;     /* dimensions of bottom workspace prev area */
        int     xsize;          /* width of window in preview */
        int     ysize;          /* height of window in preview */
        int     xleft;          /* left edge of window in preview */
        int     xrite;          /* right edge of window in preview */
        int     ytopp;          /* top edge of window in preview */
        int     ybott;          /* bottom edge of window in preview */
        int     xwinl;          /* inner left edge of window in preview */
        int     xwinr;          /* inner right edge of window in preview */
        int     xabbr;          /* left edge of abbreviated menu button */
        int     xtitl;          /* left edge of window title */
        int     ytitl;          /* bottom edge of window title */
        int     ybart;          /* top edge of window highlight bar */
        int     ybarb;          /* bottom edge of window highlight bar */
        int     yline;          /* top edge of title line separator */
        int     xcont;          /* left edge of control area text */
        int     ycont;          /* bottom edge of control area text */
        int     xbutt;          /* left edge of button in control area */
        int     ybutt;          /* top edge of button in control area */
        int     wbutt;          /* width of button in control area */
        int     ydatt;          /* top edge of data area */
        int     ydatb;          /* bottom edge of data area */
        int     xdata;          /* left edge of data area text */
        int     ydata;          /* bottom edge of data area text */
        ScaleType scale;	/* small, medium, large, extra_large */

	/* Fonts */

        XFontSet        regularFontSet;	/* fonts for buttons, menus, etc. */
        XFontSet        boldFontSet;	/* fonts for window title, captions */
        XFontSet        monospaceFontSet; /* fonts for data areas */
        XFontStruct    *glyphFont;	/* font used by olgx to draw glyphs */
 
	/* Colors */

        Pixel	windowFg;
        Pixel	windowBg;
        Pixel	dataFg;
        Pixel	dataBg;
        Pixel	workspaceFg;
        Pixel	workspaceBg;
        Pixel	olbg2;
        Pixel	olbg3;
        Pixel	olhl;
        Pixmap	pattern;	/* pattern used on workspace background */
	Boolean staticVisual;
	unsigned int depth;

} PreviewInfo;

/* This structure is private & its contents do not change */

typedef struct _PreviewPrivate {
        int     percentWindowHeight[4];	/* percent of preview height & width */
        int     percentWindowWidth;	/* used by the window in the preview */
        String	titleString;		/* window title */
        String	controlAreaString;	/* control area label */
        String	buttonTextString;	/* button label */
        String	dataAreaString;		/* data area text */
} PreviewPrivate, *PreviewPrivatePtr;
 
/*
 * Functions
 */

void		previewCalculate(PreviewInfo*);
PreviewInfo    *previewCreate(Widget);
void		previewGetColors(PreviewInfo*, Boolean);
void		previewGetFonts(PreviewInfo*);
void		previewRefreshAsRequired(PreviewInfo*, ColorType);
void        	previewRefreshDataBg(PreviewInfo*);
void        	previewRefreshDataFg(PreviewInfo*);
void        	previewRefreshWindowBg(PreviewInfo*);
void        	previewRefreshWindowFg(PreviewInfo*);
void        	previewRefreshWorkspace(PreviewInfo*);

/*
 * Private functions
 */

static void	previewCalculateWindow(PreviewInfo*);
static void	previewCalculateWorkspace(PreviewInfo*);
static void	previewDrawCb(Widget, XtPointer, XtPointer);

#endif /* _PREVIEW_H */
