#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/DrawArea.h>
#include <olgx/olgx.h>
#include <assert.h>
#include "props.h"
#include "preview.h"
#include "fonts.h"
#include "color.h"

static PreviewPrivate privateInfo;

static XtResource previewAppResources[] = {
	    { "percentHeightSmall", "PercentHeightSmall", XtRInt, sizeof(int),
	      XtOffset(PreviewPrivatePtr, percentWindowHeight[0]), XtRImmediate,
	      "84"
	    },
	    { "percentHeightMedium", "PercentHeightMedium", XtRInt, sizeof(int),
	      XtOffset(PreviewPrivatePtr, percentWindowHeight[1]), XtRImmediate,
	      "86"
	    },
	    { "percentHeightLarge", "PercentHeightLarge", XtRInt, sizeof(int),
	      XtOffset(PreviewPrivatePtr, percentWindowHeight[2]), XtRImmediate,
	      "90"
	    },
	    { "percentHeightXLarge", "PercentHeightXLarge", XtRInt, sizeof(int),
	      XtOffset(PreviewPrivatePtr, percentWindowHeight[3]), XtRImmediate,
	      "98"
	    },
	    { "percentWindowWidth", "PercentWindowWidth", XtRInt, sizeof(int),
	      XtOffset(PreviewPrivatePtr, percentWindowWidth), XtRImmediate,
	      "64"
	    },
	    { "titleString", "TitleString", XtRString, sizeof(String),
	      XtOffset(PreviewPrivatePtr, titleString), XtRImmediate,
	      "Window Title"
	    },
	    { "controlAreaString", "ControlAreaString",XtRString,sizeof(String),
	      XtOffset(PreviewPrivatePtr, controlAreaString), XtRImmediate,
	      "Control Area:"
	    },
	    { "buttonTextString", "ButtonTextString", XtRString, sizeof(String),
	      XtOffset(PreviewPrivatePtr, buttonTextString), XtRImmediate,
	      "Button"
	    },
	    { "dataAreaString", "DataAreaString", XtRString, sizeof(String),
	      XtOffset(PreviewPrivatePtr, dataAreaString), XtRImmediate,
	      "Data Area"
	    }
};

/*
 * previewCalculate
 *
 * Calculates the values required to draw the preview image.
 */
void
previewCalculate(
	PreviewInfo     *previewInfo)
{
	if (previewInfo->olgxInfo == NULL) {
		colorInitializeOlgx(previewInfo);
	}
	previewCalculateWindow(previewInfo);
	previewCalculateWorkspace(previewInfo);
}


/*
 * previewCreate
 *
 * Preconditions:  - parent widget is valid
 *
 * Postconditions: - previewDraw widget is created with callback & help
 *		   - privateInfo is initialized if required
 *		   - staticVisual & depth are initialized if required
 */
PreviewInfo*
previewCreate(
	Widget		parent)
{
	PreviewInfo    *previewInfo = XtNew(PreviewInfo);
	static Boolean  firstTime = TRUE;
	XVisualInfo    *visualInfo = colorGetVisualInfo();

	previewInfo->staticVisual = ((visualInfo->class % 2) == 0);
	previewInfo->depth = visualInfo->depth;
		
	previewInfo->olgxInfo = NULL;
	previewInfo->glyphFont = NULL;
	previewInfo->pattern = NULL;

	previewInfo->preview = XtVaCreateManagedWidget("previewDraw",
		drawAreaWidgetClass, parent,
		NULL);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)previewInfo->preview, NULL,
	        	OL_INDIRECT_SOURCE, (XtPointer)wspHelp);
	XtAddCallback(previewInfo->preview, XtNexposeCallback, previewDrawCb,
	    	(XtPointer)previewInfo);

	if (firstTime) {
		XtGetApplicationResources(previewInfo->preview, &privateInfo,
			previewAppResources, XtNumber(previewAppResources),
			NULL, 0);
		
		firstTime = FALSE;
	}
	return previewInfo;
}

/*
 * previewGetColors
 *
 * Retrieves OPEN LOOK colors, foreground and background colors for windows,
 * data areas, and workspace pattern.
 *
 * Preconditions:  previewInfo is a valid PreviewInfo pointer
 *
 * Postconditions: previewInfo color information is valid
 */
void
previewGetColors(
	PreviewInfo*	previewInfo,
	Boolean		initializing)
{
	assert(previewInfo != NULL);

	if (initializing || previewInfo->staticVisual) {
		colorGetColors(previewInfo);
	}
}

/*
 * previewGetFonts
 *
 * Retrieves font sets for regular, bold, and monospace fonts.
 *
 * Preconditions:  previewInfo is a valid PreviewInfo pointer
 *
 * Postconditions: previewInfo font set information is valid
 */
void
previewGetFonts(
	PreviewInfo      *previewInfo)
{
	fontsUpdatePreviewInfo(previewInfo);
}

/*
 * previewRefreshAsRequired
 *
 * Refreshes the part of the preview specified by colorType and any other parts
 * of the preview the specified refresh obscures.
 */
void
previewRefreshAsRequired(
	PreviewInfo*	previewInfo,
	ColorType	colorType)
{
	switch (colorType) {

	case ColorWorkspaceForeground:
	case ColorWorkspaceBackground:
		previewRefreshWorkspace(previewInfo);
		break;

	case ColorWindowForeground:
		previewRefreshWindowFg(previewInfo);
		break;

	case ColorWindowBackground:
		previewRefreshWindowBg(previewInfo);
		previewRefreshWindowFg(previewInfo);
		/* Fallthrough */
 
	case ColorDataBackground:
		previewRefreshDataBg(previewInfo);
		/* Fallthrough */
 
	case ColorDataForeground:
		previewRefreshDataFg(previewInfo);
		break;
	}
}

/*
 * previewRefreshDataBg
 *
 * Redraws the data area background.
 */
void
previewRefreshDataBg(
	PreviewInfo    *previewInfo)
{
	GC		gc = DefaultGCOfScreen(screen);
	Window		window;

	assert(previewInfo->preview != NULL);

	if ((window = XtWindow(previewInfo->preview)) == NULL) {
		return;
	}

		/* Fill the data area background with color */

	XSetForeground(display, gc, previewInfo->dataBg);
	XFillRectangle(display, window, gc,
		previewInfo->xwinl+1, previewInfo->ydatt+1,
		previewInfo->xwinr-previewInfo->xwinl-1,
		previewInfo->ydatb-previewInfo->ydatt-1);
}

/*
 * previewRefreshDataFg
 *
 * Redraws the data area text.
 */
void
previewRefreshDataFg(
	PreviewInfo    *previewInfo)
{
	GC		gc = DefaultGCOfScreen(screen);
	Window		window;
	int		textLength;	/* Number of bytes in text string */

	assert(previewInfo->preview != NULL);

	if ((window = XtWindow(previewInfo->preview)) == NULL) {
		return;
	}

	/* Draw data area text */

	textLength = strlen(privateInfo.dataAreaString);
	XSetForeground(display, gc, previewInfo->dataFg);
	if (previewInfo->monospaceFontSet != NULL) {
	    	XmbDrawString(display, window,
			previewInfo->monospaceFontSet, gc,
			previewInfo->xdata, previewInfo->ydata,
			privateInfo.dataAreaString, textLength);
	}
}

/*
 * previewRefreshWindowBg
 *
 * Redraws the window background area.
 */
void
previewRefreshWindowBg(
	PreviewInfo    *previewInfo)
{
	GC		gc = DefaultGCOfScreen(screen);
	Window		window;
	XPoint      	points[10]; /* For drawing highlight, shadow, borders */
	static int  	cornerSize[] = {10, 11, 12, 14}; /* Resize corner HxW */

	assert(previewInfo->preview != NULL);

	if ((window = XtWindow(previewInfo->preview)) == NULL) {
		return;
	}

	/* Draw window background */
 
	XSetForeground(display, gc, previewInfo->windowBg);
	XFillRectangle(display, window, gc,
	    previewInfo->xleft, previewInfo->ytopp,
	    previewInfo->xsize, previewInfo->ysize);

	/* Draw input focus bar and header dividing line */  
	    
	/* Background - input focus bar */
	   
	XSetForeground(display, gc, previewInfo->olbg2);
	if (previewInfo->depth > 1) { /* Skip for 1-bit visuals */
	    XFillRectangle(display, window, gc,
	        previewInfo->xwinl, previewInfo->ybart,
	        previewInfo->xwinr - previewInfo->xwinl+1,
	        previewInfo->ybarb - previewInfo->ybart+1);
	}   

	/* Shadow - input focus bar */
	 
	XSetForeground(display, gc, previewInfo->olbg3);
	if (previewInfo->depth > 1) { /* Skip for 1-bit visuals */
	    points[0].x = previewInfo->xwinl;
	    points[0].y = previewInfo->ybarb;
	    points[1].x = previewInfo->xwinl;
	    points[1].y = previewInfo->ybart;
	    points[2].x = previewInfo->xwinr;
	    points[2].y = previewInfo->ybart;
	    XDrawLines(display, window, gc, points, 3, CoordModeOrigin);
	}
 
	/* Shadow - dividing line */
 
	points[0].x = previewInfo->xwinr;
	points[0].y = previewInfo->yline;
	points[1].x = previewInfo->xwinl;
	points[1].y = previewInfo->yline;
	if (previewInfo->depth > 1) {
	    points[2].x = previewInfo->xwinl;
	    points[2].y = previewInfo->yline+1;
	} else { /* Skip left hand shadow pixel for 1-bit visuals */
	    XSetForeground(display, gc, previewInfo->windowFg);
	    points[2].x = previewInfo->xwinl;
	    points[2].y = previewInfo->yline;
	}
	XDrawLines(display, window, gc, points, 3, CoordModeOrigin);
 
	/* Highlight - dividing line */
 
	XSetForeground(display, gc, previewInfo->olhl);
	if (previewInfo->depth > 1) { /* Skip for 1-bit visuals */
	    points[0].x = previewInfo->xwinl+1;
	    points[0].y = previewInfo->yline+1;
	    points[1].x = previewInfo->xwinr;
	    points[1].y = previewInfo->yline+1;
	    points[2].x = previewInfo->xwinr;
	    points[2].y = previewInfo->yline;
	    XDrawLines(display, window, gc, points, 3, CoordModeOrigin);
	}
 
	/* Highlight - input focus bar */
 
	if (previewInfo->depth > 1) { /* Skip for 1-bit visuals */
	    points[0].x = previewInfo->xwinl+1;
	    points[0].y = previewInfo->ybarb;
	    points[1].x = previewInfo->xwinr;
	    points[1].y = previewInfo->ybarb;
	    points[2].x = previewInfo->xwinr;
	    points[2].y = previewInfo->ybart;
	    XDrawLines(display, window, gc, points, 3, CoordModeOrigin);
	}
 
	/* Draw 2-pixel outline around window frame */
 
	XSetForeground(display, gc, BlackPixelOfScreen(screen));
 
	points[0].x = previewInfo->xleft;
	points[0].y = previewInfo->ytopp;
	points[1].x = previewInfo->xrite;
	points[1].y = previewInfo->ytopp;
	points[2].x = previewInfo->xrite;
	points[2].y = previewInfo->ybott;
	points[3].x = previewInfo->xleft;
	points[3].y = previewInfo->ybott;
	points[4].x = previewInfo->xleft;
	points[4].y = previewInfo->ytopp;
	points[5].x = previewInfo->xleft+1;
	points[5].y = previewInfo->ytopp+1;
	points[6].x = previewInfo->xrite-1;
	points[6].y = previewInfo->ytopp+1;
	points[7].x = previewInfo->xrite-1;
	points[7].y = previewInfo->ybott-1;
	points[8].x = previewInfo->xleft+1;
	points[8].y = previewInfo->ybott-1;
	points[9].x = previewInfo->xleft+1;
	points[9].y = previewInfo->ytopp;
	XDrawLines(display, window, gc, points, 10, CoordModeOrigin);
 
	/* Draw 1-pixel outline around data pane */
 
	points[0].x = previewInfo->xwinl;
	points[0].y = previewInfo->ydatt;
	points[1].x = previewInfo->xwinr;
	points[1].y = previewInfo->ydatt;
	points[2].x = previewInfo->xwinr;
	points[2].y = previewInfo->ydatb;
	points[3].x = previewInfo->xwinl;
	points[3].y = previewInfo->ydatb;
	points[4].x = previewInfo->xwinl;
	points[4].y = previewInfo->ydatt;
	XDrawLines(display, window, gc, points, 5, CoordModeOrigin);
 
	/* Draw buttons & resize corners */
 
	if (previewInfo->scale < ScaleSmall ||
	    previewInfo->scale > ScaleExtraLarge) {

		previewInfo->scale = ScaleMedium;
	}
	olgx_draw_resize_corner(previewInfo->olgxInfo, window,
	    previewInfo->xleft,
	    previewInfo->ytopp,
	    OLGX_UPPER_LEFT, OLGX_NORMAL);
	olgx_draw_resize_corner(previewInfo->olgxInfo, window,
	    previewInfo->xrite - cornerSize[previewInfo->scale] + 1,
	    previewInfo->ytopp,
	    OLGX_UPPER_RIGHT, OLGX_NORMAL);
	olgx_draw_resize_corner(previewInfo->olgxInfo, window,
	    previewInfo->xrite - cornerSize[previewInfo->scale] + 1,
	    previewInfo->ybott - cornerSize[previewInfo->scale] + 1,
	    OLGX_LOWER_RIGHT, OLGX_NORMAL);
	olgx_draw_resize_corner(previewInfo->olgxInfo, window,
	    previewInfo->xleft,
	    previewInfo->ybott - cornerSize[previewInfo->scale] + 1,
	    OLGX_LOWER_LEFT, OLGX_NORMAL);
	olgx_draw_abbrev_button(previewInfo->olgxInfo, window,
	    previewInfo->xabbr,
	    previewInfo->ybart,
	    OLGX_NORMAL);
}

/*
 * previewRefreshWindowFg
 *
 * Redraws text for title, control area, and button.
 */
void
previewRefreshWindowFg(
	PreviewInfo    *previewInfo)
{
	GC		gc = DefaultGCOfScreen(screen);
	Window		window;
	int		textLength;     /* Number of bytes in text string */

	assert(previewInfo->preview != NULL);

	if ((window = XtWindow(previewInfo->preview)) == NULL) {
		return;
	}

	/* Display title text centered in the window header */
 
	XSetForeground(display, gc, previewInfo->windowFg);
	textLength = strlen(privateInfo.titleString);
	if (previewInfo->boldFontSet != NULL) {
		XmbDrawString(display, window,
			previewInfo->boldFontSet, gc,
			previewInfo->xtitl, previewInfo->ytitl,
			privateInfo.titleString, textLength);
	}
 
	/* Display caption text and button with label in the control area */
 
	textLength = strlen(privateInfo.controlAreaString);
	if (previewInfo->boldFontSet != NULL) {
		XmbDrawString(display, window,
			previewInfo->boldFontSet, gc,
			previewInfo->xcont, previewInfo->ycont,
			privateInfo.controlAreaString, textLength);
	}
 
	if (previewInfo->regularFontSet != NULL) {
		olgx_draw_button(previewInfo->olgxInfo, window,
	    		previewInfo->xbutt, previewInfo->ybutt,
	    		previewInfo->wbutt, 0,
	    		privateInfo.buttonTextString, OLGX_NORMAL);
	}
}

/*
 * previewRefreshWorkspace
 *
 * Retiles workspace area with current tile without drawing on the window
 * area in the center of the preview.
 */
void
previewRefreshWorkspace(
	PreviewInfo    *previewInfo)
{
	GC		gc = DefaultGCOfScreen(screen);
	Window		window;

	assert(previewInfo->preview != NULL);

	if ((window = XtWindow(previewInfo->preview)) == NULL) {
		return;
	}

	/* Set the background pixmap or color */
 
	colorGetWorkspacePattern(previewInfo);

	if (previewInfo->pattern != NULL) {
	    XGCValues       values;
 
	    values.foreground = previewInfo->workspaceFg;
	    values.background = previewInfo->workspaceBg;
	    values.fill_style = FillTiled;
	    values.tile       = previewInfo->pattern;
 
	    XChangeGC(display, gc,
	        (GCForeground | GCBackground | GCFillStyle | GCTile), &values);
	} else {
	    XSetForeground(display, gc, previewInfo->workspaceBg);
	}
 
	/* Draw the workspace area of the preview using the new background */
 
	XFillRectangle(display, window, gc,
		previewInfo->xl, previewInfo->yl,
		previewInfo->wl, previewInfo->hl);
	XFillRectangle(display, window, gc,
		previewInfo->xr, previewInfo->yr,
		previewInfo->wr, previewInfo->hr);
	XFillRectangle(display, window, gc,
		previewInfo->xt, previewInfo->yt,
		previewInfo->wt, previewInfo->ht);
	XFillRectangle(display, window, gc,
		previewInfo->xb, previewInfo->yb,
		previewInfo->wb, previewInfo->hb);
	if (previewInfo->pattern != NULL) {
	    XSetFillStyle(display, gc, FillSolid);
	}
}


/*
 * Private Preview Functions
 */


/*
 * previewCalculateWindow
 *
 * Preconditions:  - Font set information in previewInfo is valid.
 *		     Font sets are obtained by calling previewGetFonts().
 *
 * Postconditions: - Window coordinates in previewInfo are valid.
 *		   - Scale in previewInfo is valid.
 *		   - Glyph font in previewInfo is valid. 
 */
static void
previewCalculateWindow(PreviewInfo *previewInfo)
{
	XFontStruct    *previousFont;
	Dimension	width, height;	/* Dimensions of preview window */
	int		textWidth;	/* Width of multi-byte text */
	int         	textLength;     /* Number of bytes in text */
	XRectangle  	inkArea0,       /* Bounding boxes for multi-byte */
	            	inkArea1,	/* strings */
	            	logicalArea;

	/*
	 * OPEN LOOK spacings dependant upon glyph scale which may equal
	 * small, medium, large, or extra_large.
	 */

	int         	abbrLeftMargin[]  = { 12, 14, 16, 21 };
	int         	lineToppMargin[]  = { 22, 24, 28, 34 };
	int         	titlLineSpacing[] = {  7,  7,  8, 11 };
	int         	abbrTitlSpacing[] = {  8,  9, 10, 14 };
	int         	colnButtSpacing[] = { 13, 14, 15, 16 };

	/*
	 * Sizes of OPEN LOOK glyphs. Olgx macros return the wrong values
	 * in some cases. The values here are based on the actual dimensions
	 * of the glyphs in the glyph fonts.
	 */

	int		resizeArmWidth[]       = {  5,  5,  6,  6 };
	int		abbrMenuButtonHeight[] = { 12, 15, 16, 21 };

	previewGetFonts(previewInfo);

	/* Determine glyph font */

	previousFont = previewInfo->glyphFont;

	switch (previewInfo->scale) {
 
	case ScaleSmall:
	    	previewInfo->glyphFont = XLoadQueryFont(display,
	           		"-sun-open look glyph-----*-100-*-*-*-*-*-*");
	    break;
 
	case ScaleLarge:
	    	previewInfo->glyphFont = XLoadQueryFont(display,
	           		"-sun-open look glyph-----*-140-*-*-*-*-*-*");
	    break;
 
	case ScaleExtraLarge:
	    	previewInfo->glyphFont = XLoadQueryFont(display,
	           		"-sun-open look glyph-----*-190-*-*-*-*-*-*");
	    break;
 
	default: /* Defaults to medium scale (12 point) */
	    	previewInfo->glyphFont = XLoadQueryFont(display,
	           		"-sun-open look glyph-----*-120-*-*-*-*-*-*");
	    	break;
	}
	   
	if (previewInfo->glyphFont != NULL) {
	    	olgx_set_glyph_font(previewInfo->olgxInfo,
			previewInfo->glyphFont, OLGX_NORMAL);
		if (previousFont != NULL) {
	    		XFreeFont(display, previousFont);
		}
	}

	/* Determine outlines of preview window */

	if (previewInfo->preview == NULL) {
		/* Can't calculate dimensions without size of preview */
		return;
	}

	XtVaGetValues(previewInfo->preview,
		XtNwidth, 	(XtArgVal)&width,
		XtNheight,	(XtArgVal)&height,
		NULL);

	if (previewInfo->scale < ScaleSmall ||
	    previewInfo->scale > ScaleExtraLarge) {

	    	previewInfo->scale = ScaleMedium;
	}
	previewInfo->xsize = (int)width * privateInfo.percentWindowWidth/100;
	previewInfo->xleft = ((int)width - previewInfo->xsize)/2;
	previewInfo->xrite = previewInfo->xleft + previewInfo->xsize;
	previewInfo->xwinl = previewInfo->xleft +
		resizeArmWidth[previewInfo->scale];
	previewInfo->xwinr = previewInfo->xrite -
		resizeArmWidth[previewInfo->scale];

	/* Detour to determine width of control area text/button */

	textLength = strlen(privateInfo.controlAreaString);
	if (previewInfo->boldFontSet != NULL) {
		textWidth = XmbTextEscapement(previewInfo->boldFontSet,
			privateInfo.controlAreaString, textLength);
	}
#if 0
	XmbTextExtents(previewInfo->boldFontSet,
		privateInfo.controlAreaString, textLength,
		&inkArea0, &logicalArea);
#endif

	textLength = strlen(privateInfo.buttonTextString);
	if (previewInfo->regularFontSet != NULL) {
		XmbTextExtents(previewInfo->regularFontSet,
			privateInfo.buttonTextString, textLength,
			&inkArea1, &logicalArea);

		/* Adjust width with XmbTextEscapement() because olgx uses it */
		inkArea1.width = XmbTextEscapement(previewInfo->regularFontSet,
			privateInfo.buttonTextString, textLength);
	}

	previewInfo->wbutt = (int)inkArea1.width +
	    	2*ButtonEndcap_Width(previewInfo->olgxInfo);
 
	previewInfo->xcont =
	    	(previewInfo->xwinr - previewInfo->xwinl -
	     	 ((int)textWidth + previewInfo->wbutt +
#if 0
	     	 ((int)inkArea0.width + previewInfo->wbutt +
#endif
	      	  colnButtSpacing[previewInfo->scale]
	             )
	            )/2 + previewInfo->xwinl;

	if (previewInfo->xcont < previewInfo->xwinl) {
		/*
		 * Control area text/button is wider than preview window
		 * so adjust the preview window to be wide enough.
		 */
		int	difference = previewInfo->xwinl - previewInfo->xcont;
		previewInfo->xsize += 2 * difference;
		previewInfo->xleft -= difference;
		previewInfo->xrite = previewInfo->xleft + previewInfo->xsize;
		previewInfo->xwinl = previewInfo->xleft +
			resizeArmWidth[previewInfo->scale];
		previewInfo->xwinr = previewInfo->xrite -
			resizeArmWidth[previewInfo->scale];
	}
	previewInfo->xbutt = previewInfo->xcont + textWidth +
#if 0
	previewInfo->xbutt = previewInfo->xcont + inkArea0.width +
#endif
	    	colnButtSpacing[previewInfo->scale];
	
	/* Continue determining the outlines of the preview window */

	previewInfo->ysize = (int)height *
		privateInfo.percentWindowHeight[previewInfo->scale]/100;
	previewInfo->ytopp = ((int)height - previewInfo->ysize)/2;
	previewInfo->ybott = previewInfo->ytopp + previewInfo->ysize;

	previewInfo->ybart = previewInfo->ytopp +
		resizeArmWidth[previewInfo->scale];
	previewInfo->ybarb = previewInfo->ybart +
		abbrMenuButtonHeight[previewInfo->scale] - 1;

	/* Determine title placement */

	textLength = strlen(privateInfo.titleString);
	if (previewInfo->boldFontSet != NULL) {
		textWidth = XmbTextEscapement(previewInfo->boldFontSet,
			privateInfo.titleString, textLength);
	}
#if 0
	XmbTextExtents(previewInfo->boldFontSet, privateInfo.titleString,
		textLength, &inkArea0, &logicalArea);
#endif

	previewInfo->xtitl = (previewInfo->xsize - textWidth)/2 +
#if 0
	previewInfo->xtitl = (previewInfo->xsize - (int)inkArea0.width)/2 +
#endif
	    	previewInfo->xleft;
 
	previewInfo->xabbr = previewInfo->xleft +
	    	abbrLeftMargin[previewInfo->scale];
	previewInfo->yline = previewInfo->ytopp +
	    	lineToppMargin[previewInfo->scale];
	previewInfo->ytitl = previewInfo->yline -
	    	titlLineSpacing[previewInfo->scale];
	if (previewInfo->xtitl <
		Abbrev_MenuButton_Width(previewInfo->olgxInfo) +
	    	abbrTitlSpacing[previewInfo->scale]) {
 
	    	previewInfo->xtitl =
			Abbrev_MenuButton_Width(previewInfo->olgxInfo) +
	        		abbrTitlSpacing[previewInfo->scale];
	}

	/* Determine control area text placement - y coordinate */

	previewInfo->ybutt = previewInfo->yline + 4;
 
	if (previewInfo->regularFontSet != NULL) {
		olgx_set_text_fontset(previewInfo->olgxInfo,
			previewInfo->regularFontSet, OLGX_NORMAL);
	}
 
	/* Centered on button */
	previewInfo->ycont = previewInfo->ybutt +
	    	Button_Height(previewInfo->olgxInfo)/2 + inkArea1.height/2;
 
	/* Determine data area text placement */

	previewInfo->ydatt = previewInfo->ybutt +
		Button_Height(previewInfo->olgxInfo) + 4;
	previewInfo->ydatb = previewInfo->ybott -
		resizeArmWidth[previewInfo->scale];
	textLength = strlen(privateInfo.dataAreaString);
	if (previewInfo->regularFontSet != NULL) {
		XmbTextExtents(previewInfo->regularFontSet,
			privateInfo.dataAreaString, textLength,
			&inkArea0, &logicalArea);
	}
	previewInfo->xdata =
	    	(previewInfo->xwinr - previewInfo->xwinl -
	    	 (int)inkArea0.width)/2 + previewInfo->xwinl;
	previewInfo->ydata = previewInfo->ydatt + inkArea0.height + 3;
}

/*
 * previewCalculateWorkspace
 *
 * Preconditions:  call previewCalculateWindow() first to set up the following:
 *		   - previewInfo->preview widget exists
 *		   - privateInfo contains valid values  
 *		   - previewInfo window coordinates are valid
 *		   - previewInfo->scale is current
 *
 * Postconditions: - previewInfo workspace quadrant coordinates are valid
 */
static void
previewCalculateWorkspace(
	PreviewInfo    *previewInfo)
{
	Dimension   	wpre, hpre,     /* Preview dimensions */
	            	wwin, hwin;     /* Window dimensions */

	/*   
	 *  -----------------------------------------------
	 *  |          !       Top Area       !           |  
	 *  |          ------------------------           |  
	 *  |  Left    |                      |  Right    |
	 *  |  Area    |                      |   Area    |
	 *  |          ------------------------           |  
	 *  |          !      Bottom Area     !           |  
	 *  -----------------------------------------------
	 */
 
	/* Get preview area & dimensions */
 
	if (previewInfo->preview == NULL) {
	    	return;
	}
	XtVaGetValues(previewInfo->preview,
		XtNwidth,  (XtArgVal)&wpre,
		XtNheight, (XtArgVal)&hpre,
		NULL);

	/* Calculate x and y coordinates */
 
	wwin = previewInfo->xsize;
 
	/* Left area */
 
	previewInfo->xl = 0;
	previewInfo->yl = 0;
	previewInfo->hl = hpre;
	previewInfo->wl = ((int)(wpre - wwin))/2;
	if (previewInfo->wl < 0) {
		previewInfo->wl = 0;
	}
	 
	/* Right area */
 
	previewInfo->xr = previewInfo->wl + wwin + 1;
	previewInfo->yr = 0;
	previewInfo->hr = hpre;
	previewInfo->wr = previewInfo->wl;
	 
	/* Make scale safe */

	if (previewInfo->scale < ScaleSmall ||
	    previewInfo->scale > ScaleExtraLarge) {

		previewInfo->scale = ScaleMedium;
	}

	/* Top area */
 
	hwin = (int)hpre *
		privateInfo.percentWindowHeight[previewInfo->scale]/100;
	previewInfo->xt = previewInfo->wl;
	previewInfo->yt = 0;
	previewInfo->ht = ((int)(hpre - hwin))/2;
	previewInfo->wt = wwin + 1;
	if (previewInfo->xt < 0) {
		previewInfo->xt = 0;
	}
	if (previewInfo->ht < 0) {
		previewInfo->ht = 0;
	}
	 
	/* Bottom area */
	previewInfo->xb = previewInfo->wl;
	previewInfo->yb = previewInfo->ht + hwin + 1;
	previewInfo->hb = previewInfo->ht;
	previewInfo->wb = wwin + 1;
}

/*
 * previewDrawCb
 *
 * Refreshes all parts of the preview window whenever an expose event
 * is sent to the preview window.
 */
/* ARGSUSED */
static void
previewDrawCb(
	Widget		widget,
	XtPointer	clientData,
	XtPointer	callData)
{
	PreviewInfo    *previewInfo = (PreviewInfo*) clientData;

	previewRefreshWorkspace(previewInfo);
	previewRefreshWindowBg(previewInfo);
	previewRefreshWindowFg(previewInfo);
	previewRefreshDataBg(previewInfo);
	previewRefreshDataFg(previewInfo);
}

