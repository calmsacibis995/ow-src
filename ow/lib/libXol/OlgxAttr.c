#pragma ident	"@(#)OlgxAttr.c	1.12	97/03/26 lib/libXol SMI"	/* olg:OlgAttr.c 302.6	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <libintl.h>

#include <Xol/OlgxP.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ConvertI.h>
#include <X11/CoreP.h>
#include <Xol/EventObjP.h>      /* For gadgets  */
#include <Xol/FlatP.h>


static OlgxAttrs	*attrHead;

/* LookupColorTuple: Determine if matching color tuple exists
 *
 * Check to see if the application resource, XtNcolorTupleList
 * was set and matches  "bg1"; if so, return a pointer to the tuple,
 * else return 0.
 */
static OlColorTuple *
LookupColorTuple (Screen *screen, Colormap cmap, Pixel bg1)
{
	static OlColorTupleList	*ctl	= 0;
	Cardinal		i;


	if (!ctl) {
		Display *		display = DisplayOfScreen(screen);
		XrmName			xrm_name[2];
		XrmClass		xrm_class[2];
		XrmRepresentation	type;
		XrmValue		value;
		XrmValue		args[2];

		static XrmValue		to   = {
			sizeof(OlColorTupleList),
			(XtPointer)&ctl
		};

		extern XtTypeConverter	_OlCvtStringToColorTupleList;


		/*
		 * Fetch value for "*colorTupleList" from resource
		 * database.
		 */
		xrm_name[0]  = XrmStringToName("colorTupleList");
		xrm_name[1]  = 0;
		xrm_class[0] = XrmStringToClass("ColorTupleList");
		xrm_class[1] = 0;
		if (!XrmQGetResource(XtDatabase(display), xrm_name, xrm_class,  &type, &value))
			return (0);

		/*
		 * Only type of value we expect is String.
		 */
		if (type != _XtQString)
			return (0);

		args[0].addr = (caddr_t) &screen;
		args[0].size = sizeof(Screen *);
		args[1].addr = (caddr_t) &cmap;
		args[1].size = sizeof(Colormap);
		if (!XtCallConverter(
			display,
			_OlCvtStringToColorTupleList,
			args,
			2,
			&value,
			&to,
			(XtCacheRef *)0
		))
			return (0);
	}

	for (i = 0; i < ctl->size; i++)
		if (bg1 == ctl->list[i].bg1)
			return (&(ctl->list[i]));

	return (0);

} /* LookupColorTuple */

/* Define strings to build glyph fontname for existing scales.
 */
#define	GLYPHFONT_PREFIX	"-sun-open look glyph-----"
#define	GLYPHFONT10		"10-100-75-75-p-106"
#define	GLYPHFONT12		"12-120-75-75-p-116"
#define	GLYPHFONT14		"14-140-75-75-p-136"
#define	GLYPHFONT16		"16-160-75-75-p-137"
#define	GLYPHFONT19		"19-190-75-75-p-163"
#define	GLYPHFONT20		"20-200-75-75-p-172"
#define	GLYPHFONT24		"24-240-75-75-p-206"
#define	GLYPHFONT_SUFFIX	"-sunolglyph-1"

/* OlgxCreateAttrs: Get an attributes structure for OLGX rendering
 *
 * Look for an existing attributes structure with the same background,
 * foreground, scale and textFont.  If not found, allocate a new one and
 * initialize an OLGX Graphics_info structure.  
 *
 */
OlgxAttrs *
OlgxCreateAttrs (Widget w, Pixel fg, OlgxBG *bg, unsigned int bgIsPixmap, 
		 unsigned int scale, OlStrRep textFormat, OlFont textFont)
{
    register Screen		*scr = XtScreenOfObject(w);
    register OlgxAttrs		*current;
    register _OlgxDevice	*pDev;
    Display		*display;
    XFontStruct	*glyphFont;
    char		*glyphFontname;
    unsigned long		pixvals[OLGX_NUM_COLORS];
    int			validScale;
    char		*errorbuf;


	if (!(errorbuf = malloc(256)))
		return(NULL);

    /* Verify scale passed in is one supported by OLGX */
    validScale = OlgxGetValidScale(scale);
    if (scale != validScale) {
	scale = validScale;
        snprintf(errorbuf, 256, dgettext(OlMsgsDomain,
                "OlgxCreateAttrs: invalid scale %1$d, scale set to %2$d"),
                        scale, validScale);
        OlWarning(errorbuf);
    }

    /* Get the device structure for the screen and scale */
    pDev = _OlgxGetDeviceData (w, scale);

    /* If NULL passed in for textFont, set textFormat to SingleByte */
    if (!textFont)
	textFormat = OL_SB_STR_REP;

    /* Look for an existing entry, comparing: foreground, pDev(scale),
     * textFont, background_pixmap or background_pixel.
     */ 
    current = attrHead;
    while (current) {
	if (current->fg == fg && current->pDev == pDev && 
               (!textFont || 
		  ((textFormat == OL_SB_STR_REP && 
		    Olgx_Flags(current->ginfo) != OLGX_FONTSET &&
                      TextFont_Struct(current->ginfo) == (XFontStruct*)textFont) ||
                   (textFormat != OL_SB_STR_REP && 
                    Olgx_Flags(current->ginfo) == OLGX_FONTSET &&
                      TextFont_Set(current->ginfo) == (XFontSet)textFont)))) 
	    if (current->flags & OLGX_BGPIXMAP) {
		if (bgIsPixmap && (current->bg.pixmap == bg->pixmap))
		    break;	/* winner! */
	    } else
		if (!bgIsPixmap && (current->bg.pixel == bg->pixel))
		    break;	/* winner! */

	current = current->next;
    }

    /* If we found a match, bump the reference count and return it */
    if (current) {
	current->refCnt++;

	current->refWidgets = (Widget *)XtRealloc((char *)current->refWidgets,
					          sizeof(Widget) *
					          current->refCnt);

	current->refWidgets[current->refCnt - 1] = w;

	free(errorbuf);
	return current;
    }

    /* No luck.  Create a new one. */

    current = (OlgxAttrs *) XtMalloc (sizeof (OlgxAttrs));
    current->next = attrHead;
    attrHead = current;

    current->refCnt = 1;
    *(current->refWidgets = (Widget *)XtCalloc(1, sizeof (Widget))) = w;
    current->pDev = pDev;
    current->fg = fg;
    current->bg = *bg;
    current->flags = bgIsPixmap ? OLGX_BGPIXMAP : 0;


	if (glyphFontname = malloc(60)) {
		strncpy(glyphFontname, GLYPHFONT_PREFIX, 60);
		switch(validScale) {
			case 10: strncat(glyphFontname, GLYPHFONT10, 60 - strlen(glyphFontname));
			 break;
			case 12: strncat(glyphFontname, GLYPHFONT12, 60 - strlen(glyphFontname));
			 break;
			case 14: strncat(glyphFontname, GLYPHFONT14, 60 - strlen(glyphFontname));
			 break;
			case 16: strncat(glyphFontname, GLYPHFONT16, 60 - strlen(glyphFontname));
			 break;
			case 19: strncat(glyphFontname, GLYPHFONT19, 60 - strlen(glyphFontname));
			 break;
			case 20: strncat(glyphFontname, GLYPHFONT20, 60 - strlen(glyphFontname));
			 break;
			case 24: strncat(glyphFontname, GLYPHFONT24, 60 - strlen(glyphFontname));
			 break;
		default: strncat(glyphFontname, GLYPHFONT12, 60 - strlen(glyphFontname));
		}
		strncat(glyphFontname, GLYPHFONT_SUFFIX, 60 - strlen(glyphFontname));
	 
		display = (Display *)DisplayOfScreen(scr);
		if ((glyphFont = XLoadQueryFont(display, glyphFontname)) == NULL) {
		/* OLGX_TODO: should we try a different scale here? */
			snprintf(errorbuf, 256, dgettext(OlMsgsDomain,
					"Could not load Glyph Font:  %1$s"),
							glyphFontname);
			OlError(errorbuf);
		}
		free(glyphFontname);
	}

    if (pDev->threed) {
	OlColorTuple *colorTuple;

	/* Determine if XtNcolorTupleList application resource was set
	 * and matches this bg color; if so, use those pixel values, else
	 * calculate new ones using OLGX.
	 */
        if ((colorTuple = LookupColorTuple(scr, OlColormapOfObject(w), bg->pixel))) {
            pixvals[OLGX_WHITE] = colorTuple->bg0;
	    pixvals[OLGX_BG1]   = colorTuple->bg1;
	    pixvals[OLGX_BG2]   = colorTuple->bg2;
	    pixvals[OLGX_BG3]   = colorTuple->bg3;

	} else { 
	    XrmValue src, dst;
	    XColor   colorFG, colorBG1, colorBG2, colorBG3, colorHighlight;

	    src.size = sizeof(Pixel);
	    src.addr = (XtPointer)&fg;
	    dst.size = sizeof(XColor);
	    dst.addr = (XtPointer)&colorFG;
	    XtConvertAndStore(w, XtRPixel, &src, XtRColor, &dst);

	    src.addr = (XtPointer)&(bg->pixel);
	    dst.addr = (XtPointer)&colorBG1;
	    XtConvertAndStore(w, XtRPixel, &src, XtRColor, &dst);
	 
	    olgx_calculate_3Dcolors(&colorFG, &colorBG1, &colorBG2, &colorBG3, 
	         &colorHighlight);

	    /* If XAllocColor fails, use Black & White (since we are pretty much
	     * guaranteed they will exist) and print a warning so the user knows
	     * why the 3D colors do not look correct.
	     */
	    if (XAllocColor(display, pDev->cmap, &(colorBG2))) {
	        pixvals[OLGX_BG2] = colorBG2.pixel;
		current->flags |= OLGX_ALLOCBG2;
	    } else {
		pixvals[OLGX_BG2] = WhitePixelOfScreen(scr);
		OlWarning("OlgxCreateAttrs: Cannot allocate colormap entry for BG2 - setting BG2=White");
	    }
	    if (XAllocColor(display, pDev->cmap, &(colorBG3))) {
	        pixvals[OLGX_BG3] = colorBG3.pixel;
		current->flags |= OLGX_ALLOCBG3;
	    } else {
		pixvals[OLGX_BG3] = BlackPixelOfScreen(scr);
                OlWarning("OlgxCreateAttrs: Cannot allocate colormap entry for BG3 - setting BG3=Black"); 
            }
	    if (XAllocColor(display, pDev->cmap, &(colorHighlight))) {
	        pixvals[OLGX_WHITE] = colorHighlight.pixel;
		current->flags |= OLGX_ALLOCWHITE;
	    } else {
		pixvals[OLGX_WHITE] = WhitePixelOfScreen(scr);
                OlWarning("OlgxCreateAttrs: Cannot allocate colormap entry for Highlight - setting Highlight=White");  
	    }
	    pixvals[OLGX_BG1] = bg->pixel;
	}
	pixvals[OLGX_BLACK] = fg; 

    } else { /* 2D */
	pixvals[OLGX_WHITE] = bg->pixel;
	pixvals[OLGX_BLACK] = fg;
    }

    if (textFormat == OL_SB_STR_REP)
	current->ginfo = olgx_main_initialize(display,
	    /* note: OLGX wants a screen *number*, not the data structure ptr */
		/* Yes ... and we CAN get the screen number from the Screen* -jmk */
	    XScreenNumberOfScreen(scr),
	    pDev->depth, (pDev->threed ? OLGX_3D_COLOR : OLGX_2D),
	    glyphFont, (XFontStruct *)textFont, pixvals, NULL);
    else
	current->ginfo = olgx_i18n_initialize(display, XScreenNumberOfScreen(scr),
	    pDev->depth, (pDev->threed ? OLGX_3D_COLOR : OLGX_2D),
	    glyphFont, (XFontSet)textFont, pixvals, NULL);
    /* OLGX_TODO: should fail gracefully in some way if (!current->ginfo) */

	free(errorbuf);
    return current;

} /* OlgxCreateAttrs */


/* OlgxDestroyAttrs: Destroy Attributes
 *
 * If no one else is using an attributes structure, free all resources
 * associated with the structure and free it.
 */
void
OlgxDestroyAttrs (Widget w, register OlgxAttrs *pInfo)
{
    register OlgxAttrs	*prev, *current;
    unsigned long	pixvals[3];
    int			nalloc;

    _OlgxFreeDeviceDataRef(pInfo->pDev); /* decrement ref count for Device */
    pInfo->refCnt--;

    if (pInfo->refCnt > 0) {
	register int i;

	for (i = 0; i <= pInfo->refCnt; i++)
		if (pInfo->refWidgets[i] == w) break;

	if (i < pInfo->refCnt) { /* not at end */
		memcpy((char *)(pInfo->refWidgets + i),
		       (char *)(pInfo->refWidgets + i + 1),
		       (pInfo->refCnt - i) * sizeof(Widget));
	}

	pInfo->refWidgets = (Widget *)XtRealloc((char *)pInfo->refWidgets,
						sizeof(Widget) * pInfo->refCnt);
	return;
    }

    nalloc = 0;
    if (pInfo->flags & OLGX_ALLOCBG2)
	pixvals[nalloc++] = olgx_get_single_color(pInfo->ginfo, OLGX_BG2);
    if (pInfo->flags & OLGX_ALLOCBG3)
	pixvals[nalloc++] = olgx_get_single_color(pInfo->ginfo, OLGX_BG3);
    if (pInfo->flags & OLGX_ALLOCWHITE)
	pixvals[nalloc++] = olgx_get_single_color(pInfo->ginfo, OLGX_WHITE);
    if (nalloc)
	XFreeColors(XtDisplayOfObject(w), OlColormapOfObject(w), pixvals,
	    nalloc, 0);

    XFreeFont(XtDisplayOfObject(w), GlyphFont_Struct(pInfo->ginfo));
    olgx_destroy(pInfo->ginfo);

    /* Find the element in the list just before this one */
    current = attrHead;
    prev = (OlgxAttrs *) 0;
    while (current && current != pInfo) {
	prev = current;
	current = current->next;
    }

    if (!current)
	OlError (dgettext(OlMsgsDomain, "Invalid attributes pointer"));

    if (prev)
	prev->next = current->next;
    else
	attrHead = current->next;

    XtFree ((char *)current->refWidgets);

    XtFree ((char *)current);
}

/* Set Visuals Mode
 *
 * Set the visuals mode to 2- or 3-D drawing mode.  All previously allocated
 * GCs are freed and re-allocated in the new style.
 */

static void
_expose(register Widget w)
{
	if (!(XtIsRealized(w)					       &&
		((XtIsWidget((w))				       &&
		 (((w)->core.widget_class->core_class.visible_interest &&
		  (w)->core.visible)				       ||
		 (XtIsManaged(w) && (w)->core.mapped_when_managed)))   ||
		XtIsManaged(w)))) return;

	if (XtIsComposite(w)) {
		int		i;
		CompositeWidget	cw = (CompositeWidget)w;

		for (i = cw->composite.num_children - 1; i >= 0; i--)
			_expose(cw->composite.children[i]);
	}

	if (XtIsWidget(w)) {
		XClearArea(XtDisplay(w), XtWindow(w),
			   0, 0,
			   (unsigned int)w->core.width,
			   (unsigned int)w->core.height,
			   True);
	} else {
		XClearArea(XtDisplayOfObject(w),
			   XtWindowOfObject(w),
			   w->core.x - w->core.border_width,
			   w->core.y - w->core.border_width,
			   (unsigned int)w->core.width +
					 w->core.border_width,
			   (unsigned int)w->core.height +
					 w->core.border_width,
			   True);
	}

} /* OlgxDestroyAttrs */

void
OlgSetStyle3D (Screen *scr, unsigned int draw3d)
{
    register OlgxAttrs	*current;

    if (DefaultDepthOfScreen(scr) == 1)
		draw3d = False;

    /* For each attributes structure in the system, free the existing GCs
     * and get fresh ones.
     */
    /* OLGX_TODO: Need to re-write this for OLGX (after alpha!)
    for (current = attrHead; current; current = current->next) {
	if (current->pDev->scr == scr) {
		int 	i;

		if (OlgIs3d(scr) != current->pDev->threed ||
		    current->pDev->threed != draw3d) {
			if (current->flags & OLG_ALLOCBG0)
			    XFreeGC (DisplayOfScreen(scr), current->bg0);
			if (current->flags & OLG_ALLOCBG1)
			    XFreeGC (DisplayOfScreen(scr), current->bg1);
			if (current->flags & OLG_ALLOCBG2)
			    XFreeGC (DisplayOfScreen(scr), current->bg2);
			if (current->flags & OLG_ALLOCBG3)
			    XFreeGC (DisplayOfScreen(scr), current->bg3);
			current->flags &= ~(OLG_ALLOCBG0 | OLG_ALLOCBG1 |
					    OLG_ALLOCBG2 | OLG_ALLOCBG3);

			allocGCs (current->pDev->scr,
				  current->pDev->cmap,
				  current->pDev->depth,
				  current->refWidgets[0], current);

			current->pDev->threed = draw3d;
		}

		for (i = 0; i < current->refCnt; i++) {
			_expose(current->refWidgets[i]);
		}
	}
    }
    END OLGX_TODO */
}
#ifdef DEBUG
void
dumpOlgxAttr(OlgxAttrs *pAttr)
{
    int i;

    fprintf(stderr,"---------------------------------------------------------------\n");
    fprintf(stderr,"  next       = %x\n",pAttr->next);
    fprintf(stderr,"  refCnt     = %d\n",pAttr->refCnt);
    fprintf(stderr,"  refWidgets = ");
    for(i=0; i < pAttr->refCnt; i++) {
        fprintf(stderr,"%d:%s ",i,pAttr->refWidgets[i]->core.name);
        if (!((i+1)%5))
            fprintf(stderr,"\n");
    }
    fprintf(stderr,"\n");
    fprintf(stderr,"  fg         = %d\n",pAttr->fg);
    fprintf(stderr,"  bg         = %d\n",pAttr->bg.pixel);
    fprintf(stderr,"  flags      = %x\n",pAttr->flags);
    fprintf(stderr,"  ginfo *    = %x\n",pAttr->ginfo);
    fprintf(stderr,"  pDev => %x \n",pAttr->pDev);
    fprintf(stderr,"       next     = %x\n",pAttr->pDev->next);
    fprintf(stderr,"       refCnt   = %d\n",pAttr->pDev->refCnt);
    fprintf(stderr,"       threed   = %s\n",pAttr->pDev->threed?"TRUE":"FALSE");
    fprintf(stderr,"       scale    = %d\n",pAttr->pDev->scale);
    fprintf(stderr,"       Colormap = %x\n",pAttr->pDev->cmap);
    fprintf(stderr,"       depth    = %d\n",pAttr->pDev->depth);
    fprintf(stderr,"\n");
}

void
dumpOlgxAttrCache()
{
    OlgxAttrs *current;
    int i = 0;

    fprintf(stderr,"OlgxAttrs CACHE --->>>>\n");
    for (current = attrHead; current; current = current->next) {
        fprintf(stderr,"%d : ",++i);
        dumpOlgxAttr(current);
    }
}
#endif /* DEBUG */
