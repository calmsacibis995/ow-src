#pragma ident	"@(#)FlatExpand.c	302.14	97/03/26 lib/libXol SMI"	/* flat:FlatExpand.c 1.25	**/

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

/*
 *************************************************************************
 *
 * Description:
 *	This file contains various support and convenience routines
 *	for the flat widgets.
 *
 ******************************file*header********************************
 */


#include <stdio.h>

#include <X11/IntrinsicP.h>

#include <Xol/Error.h>
#include <Xol/EventObj.h>
#include <Xol/FlatP.h>
#include <Xol/Font.h>
#include <Xol/OpenLookP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private  Procedures 
 *		2. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void nullProc (Screen* scr, Drawable win, OlgxAttrs *pInfo,
		      Position x, Position y,
		      Dimension width, Dimension height, XtPointer labeldata,
		      OlDefine buttonType, int flags);
static void nullSize (Screen *scr, OlgxAttrs *pInfo, XtPointer labeldata,
		      Dimension *pWidth, Dimension *pHeight);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define CVT_JUSTIFY(j)	((j) == OL_LEFT ? TL_LEFT_JUSTIFY :\
		((j) == OL_CENTER ? TL_CENTER_JUSTIFY : TL_RIGHT_JUSTIFY))

#define W_CNAME(w)	(XtClass(w)->core_class.class_name)
#define PIXELS(s,n)	OlScreenPointToPixel(OL_HORIZONTAL,n,s)

#define ALLOC_STACK_ITEM(fcptr,iptr,stk)\
	auto char	stk[512];\
	FlatItem	iptr = (FlatItem)(fcptr->rec_size>sizeof(stk)?\
				XtMalloc(fcptr->rec_size) : stk)
#define FREE_STACK_ITEM(iptr,stk) if ((char*)iptr!=stk){XtFree((char *)iptr);}

			/* Define some constants to reflect visual
			 * sizes for the default point size.
			 * This will have to be updated when scaling
			 * is supported					*/

#define MENU_MARK	8				/* in points	*/
#define OBLONG_RADIUS	10				/* in points	*/

#define FPART(w)	(&((FlatWidget)(w))->flat)
#define FCPART(w)	(&((FlatWidgetClass)XtClass(w))->flat_class)

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * nullProc - Do not draw a label; do not pass GO; do not collect $200.
 ****************************procedure*header*****************************
 */
static void
nullProc (Screen* scr, Drawable win, OlgxAttrs *pInfo, Position x, Position y,
	  Dimension width, Dimension height, XtPointer labeldata,
	  OlDefine buttonType, int flags)
{
} /* END OF nullProc() */

/*
 *************************************************************************
 * nullSize - Size procedure for null labels.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
nullSize (Screen *scr, OlgxAttrs *pAttrs, XtPointer lbl, Dimension *pWidth, Dimension *pHeight)
{
    *pWidth = *pHeight = 0;
} /* END OF nullSize() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlDoGravity - this routine calculates the offsets for a particular
 * container.  Offsets are determined using the gravity resource.
 * The following code never lets the offsets be less than zero.
 ****************************procedure*header*****************************
 */
void
_OlDoGravity (int gravity, Dimension c_width, Dimension c_height,
	      Dimension width, Dimension height,
	      Position *x, Position *y)
{
	Dimension	dw;		/* difference in widths		*/
	Dimension	dh;		/* difference in heights	*/

	dw = (Dimension) (c_width > width ? c_width - width : 0);
	dh = (Dimension) (c_height > height ? c_height - height : 0);

	if (dh != 0 || dw != 0)
	{
		switch(gravity)
		{
		case EastGravity:
			dh /= 2;			/* (dw, dh/2)	*/
			break;
		case WestGravity:
			dw  = 0;			/* (0, dh/2)	*/
			dh /= 2;
			break;
		case CenterGravity:
			dw /= 2;			/* (dw/2, dh/2)	*/
			dh /= 2;
			break;
		case NorthGravity:
			dw /= 2;			/* (dw/2, 0)	*/
			dh  = 0;
			break;
		case NorthEastGravity:
			dh = 0;				/* (dw, 0)	*/
			break;
		case NorthWestGravity:
			dw = 0;				/* (0,0)	*/
			dh = 0;
			break;
		case SouthGravity:
			dw /= 2;			/* (dw/2, dh)	*/
			break;
		case SouthEastGravity:
			break;				/* (dw, dh)	*/
		case SouthWestGravity:
			dw = 0;				/* (0, dh)	*/
			break;
		default:
			OlVaDisplayWarningMsg((Display *)NULL,
				OleNinvalidParameters,
				OleTolDoGravity, OleCOlToolkitWarning,
				OleMinvalidParameters_olDoGravity, gravity);
			 break;
		}
	}

	*x = (Position) dw;		/* return the values	*/
	*y = (Position) dh;
} /* END OF _OlDoGravity() */

/*
 *************************************************************************
 * _OlFlatCheckItems - this is a convenience routine for re-initializing
 * the sub-object items and for doing a relayout.
 *
 * In order for a relayout to be done, the relayout hint must be true
 * or the items_touched flag must be true.
 * If either of these are true and the calling WidgetClass matches
 * the layout_container, the items will be re-initialized (if necessary)
 * and a relayout will be done.
 ****************************procedure*header*****************************
 */
Boolean
_OlFlatCheckItems (WidgetClass wc, Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	FlatPart *	fp = FPART(new);
	FlatClassPart *	fcp = FCPART(new);
	Boolean		redisplay = False;

	if ((fcp->layout_class == wc)
		&&
	    ((fp->flags & OL_B_FLAT_RELAYOUT_HINT) ||
	      fp->items_touched == True))
	{
					/* First, initialize the items	*/

		if (fp->items_touched == True)
		{
			_OlFlatInitializeItems(current, request, new,
						args, num_args);
		}

				/* Now, set the redisplay flag and
				 * reset the internal state flags	*/

		redisplay		= True;
		fp->flags		&= ~OL_B_FLAT_RELAYOUT_HINT;
		fp->items_touched	= False;

		_OlFlatLayout(new, &new->core.width, &new->core.height);

		if (new->core.width == (Dimension)0 ||
		    new->core.height == (Dimension)0)
		{
			OlVaDisplayWarningMsg(XtDisplay(new),
				OleNinvalidDimension,
				OleTwidgetSize, OleCOlToolkitWarning,
				OleMinvalidDimension_widgetSize,
				XtName(new),W_CNAME(new));

			if (new->core.width == (Dimension)0)
			{
				new->core.width = (Dimension)1;
			}
			if (new->core.height == (Dimension)0)
			{
				new->core.height = (Dimension)1;
			}
		}
	}

	return(redisplay);
} /* END OF _OlFlatCheckItems() */

/*
 *************************************************************************
 * _OlFlatPreviewItem - this routine previews a flat widget's sub-object
 * into another widget.  If the preview widget (i.e., the destination
 * widget) is a flat widget, the parameter preview_item is used to 
 * determine which sub-object is the destination sub-object for
 * previewing.
 ****************************procedure*header*****************************
 */
void
_OlFlatPreviewItem (Widget w, Cardinal item_index, Widget preview, Cardinal preview_item)
{
	FlatPart *	fp = FPART(w);
	FlatClassPart *	fcp = FCPART(w);

	if (item_index < fp->num_items &&
	    preview != (Widget)NULL &&
	    XtIsRealized(preview) == (Boolean)True)
	{
		Display *	dpy;
		OlFlatDrawInfo	item_di;	/* item to be previewed	*/
		OlFlatDrawInfo	di;		/* preview draw info	*/
		ALLOC_STACK_ITEM(fcp, item_rec, stack);
		Boolean		is_gadget = _OlIsGadget(preview);

				/* Get the required width and height
				 * for the item to be previewed.	*/

		_OlFlatGetDrawInfo(w, item_index, &item_di);

				/* Obtain information about the
				 * destination.				*/

		if (preview_item != (Cardinal)OL_NO_ITEM &&
			 XtIsSubclass(preview, flatWidgetClass) == True)
		{
			_OlFlatGetDrawInfo(preview, preview_item, &di);
		}
		else
		{
			di.x = (Position) (is_gadget == (Boolean )True ?
				(di.y = preview->core.y, preview->core.x) :
				(di.y = (Position)0, 0));

			di.width	= preview->core.width;
			di.height	= preview->core.height;
		}

				/* Set the other draw information
				 * parameters.				*/

		di.screen	= XtScreen(preview);
		dpy		= DisplayOfScreen(di.screen);
		di.drawable	= (Drawable) XtWindow(preview);
		di.background	= (is_gadget == (Boolean)True ?
				XtParent(preview)->core.background_pixel :
					preview->core.background_pixel);

				/* Set the clipping information.  Always
				 * set the clip_width and clip_height to
				 * that of the destination.		*/

		di.clipped	= False;
		di.clip_width	= di.width;
		di.clip_height	= di.height;

					/* Clear the Destination area	*/

		(void) XClearArea(dpy, (Window)di.drawable,
					(int)di.x, (int)di.y,
					(unsigned int)di.width,
					(unsigned int)di.height, (Bool)False);

		_OlFlatExpandItem(w, item_index, item_rec);

		if (item_rec->flat.managed == True &&
		    item_rec->flat.mapped_when_managed == True)
		{
			_OlFlatDrawItem(w, item_rec, &di);
		}
		FREE_STACK_ITEM(item_rec,stack);
	}
} /* END OF _OlFlatPreviewItem() */

/*
 *************************************************************************
 * _OlFlatRefreshItem - this routine optionally clears an item and then
 * makes a request to draw it.
 *	This routine performs the above differently in the case where
 * all items have a non-zero overlap value and the item being refreshed
 * is managed, but is not mapped when managed.  In this case, the item
 * to the left and the item to the right needs to be refreshed as well,
 * so we generate a synthetic exposure event and call the refresh routine.
 ****************************procedure*header*****************************
 */
void
_OlFlatRefreshItem (Widget w, Cardinal item_index, Boolean clear_area)
{
	FlatPart *	fp = FPART(w);
	FlatClassPart *	fcp = FCPART(w);

	if (XtIsRealized(w) == (Boolean)True &&
	   fp->items_touched == False)
	{
		Display *	dpy;
		OlFlatDrawInfo	di;
		ALLOC_STACK_ITEM(fcp, item_rec, stack);

		_OlFlatGetDrawInfo(w, item_index, &di);

		di.screen	= XtScreen(w);
		di.background	= w->core.background_pixel;
		di.drawable	= (Drawable) XtWindow(w);
		dpy		= DisplayOfScreen(di.screen);

		if (clear_area == True)
		{
			(void) XClearArea(dpy, (Window)di.drawable,
				(int)di.x, (int)di.y,
			(unsigned int)di.width, (unsigned int)di.height,
			(Bool)FALSE);
		}

		_OlFlatExpandItem(w, item_index, item_rec);

		if (item_rec->flat.mapped_when_managed == (Boolean)False &&
		    fp->overlap != (Dimension)0 &&
		    item_rec->flat.managed == (Boolean)True)
		{
			XExposeEvent	exposure;

			exposure.display	= dpy;
			exposure.type		= Expose;
			exposure.serial		= (unsigned long) 0;
			exposure.send_event	= (Bool) False;
			exposure.window		= (Window) di.drawable;
			exposure.x		= (int) di.x;
			exposure.y		= (int) di.y;
			exposure.width		= (int) di.width;
			exposure.height		= (int) di.height;
			exposure.count		= (int) 0;

			(* XtClass(w)->core_class.expose)
				(w, (XEvent *)&exposure, (Region)NULL);
		}
		else if (item_rec->flat.mapped_when_managed == True)
		{
			_OlFlatDrawItem(w, item_rec, &di);
		}
		FREE_STACK_ITEM(item_rec,stack);
	}
} /* END OF _OlFlatRefreshItem() */

/*
 *************************************************************************
 * ScreenManager - this routine caches information on a screen basis.
 * For each created widget instance, a reference is added to the count.
 * As an instance is destroyed, the reference count is decremented.
 * When the reference count is zero, the node is deleted.
 ****************************procedure*header*****************************
 */
OlFlatScreenCache *
_OlFlatScreenManager (Widget w, Cardinal point_size, OlDefine flag)
{
	static OlFlatScreenCacheList	screen_cache_head = NULL;
	OlFlatScreenCache *		self = screen_cache_head;
	Screen *			screen = XtScreen(w);
	Colormap			cmap = w->core.colormap;
	Display *			dpy = DisplayOfScreen(screen);

				/* If we didn't find a node, make one	*/

	if (flag == OL_ADD_REF || flag == OL_JUST_LOOKING)
	{
		while(self != (OlFlatScreenCache *)NULL &&
				self->screen != screen && self->cmap != cmap)
		{
			self = self->next;
		}

		if (self == (OlFlatScreenCacheList)NULL)
		{
			self			= XtNew(OlFlatScreenCache);
			self->next		= screen_cache_head;
			screen_cache_head	= self;
			self->screen		= screen;
			self->cmap		= cmap;
			self->count		= 0;

			self->alt_bg		= OlWhitePixel(w);
			self->alt_fg		= OlBlackPixel(w);
                        self->font              = OlGetDefaultFont(w);
			self->alt_attrs		= OlgxCreateAttrs(w,
							self->alt_fg,
							(OlgxBG *)&(self->alt_bg),
							False, point_size,
							((FlatWidget)w)->primitive.text_format,
							((FlatWidget)w)->primitive.font);

			self->check_width = (Dimension)
			    CheckBox_Width(self->alt_attrs->ginfo);
			self->check_height = (Dimension)
			    CheckBox_Height(self->alt_attrs->ginfo);

		}
		if (flag == OL_ADD_REF)
		{
			++self->count;
		}
		else if (self->count == 0)
		{
			OlVaDisplayWarningMsg(dpy, OleNinternal,
				OleTbadNodeReference, OleCOlToolkitWarning,
				OleMinternal_badNodeReference,
				XtName(w), W_CNAME(w), "screenCache");
		}
	}
	else
	{
		OlFlatScreenCache * prev = (OlFlatScreenCache *)NULL;

		for(;self != (OlFlatScreenCacheList)NULL &&
			self->screen != screen; self = self->next)
		{
			prev = self;
		}

		if (self != (OlFlatScreenCacheList)NULL)
		{
			if (--self->count == 0)
			{
				if (prev != (OlFlatScreenCache *)NULL)
				{
					prev->next = self->next;
				}
				else
				{
					screen_cache_head = self->next;
				}

				if (self->alt_attrs != (OlgxAttrs *)NULL)
				{
					OlgxDestroyAttrs(w, self->alt_attrs);
				}

				XtFree((char *) self);
			}
		}
		else
		{
			OlVaDisplayWarningMsg(dpy, OleNinternal,
				OleTcorruptedList, OleCOlToolkitWarning,
				OleMinternal_corruptedList,
				XtName(w), W_CNAME(w), "screenCache");
		}

		self = (OlFlatScreenCache *)NULL;
	}
	return(self);
} /* END OF _OlFlatScreenManager() */

/*
 *************************************************************************
 * _OlFlatSetupAttributes - this routine selects attributes and label
 * drawing functions for a flat item.  Note that there is only one
 * static label structure.  Subsequent calls to this function will destroy
 * previous contents.
 ****************************procedure*header*****************************
 */
void
_OlFlatSetupAttributes(
	Widget		w,		/* flat widget container */
	FlatItem	item,		/* expanded item */
	OlFlatDrawInfo*	di,		/* drawing information */
	OlgxAttrs**	ppAttrs,	/* returned pointer to attributes */
	FlatLabel**	ppLbl,		/* returned pointer to label */
	OlDrawProc*     ppDrawProc      /* returned ptr to lbl drawer */
)
{
    FlatPart*		fp = FPART(w);
    Cardinal		psize = ((FlatWidget)w)->primitive.scale;
    OlFlatScreenCache *	sc = _OlFlatScreenManager(w, psize, OL_JUST_LOOKING);
    void		(*sizeProc)();
    FlatLabel		*plbl;
    int			has_focus = (fp->focus_item == item->flat.item_index);
    int			isSensitive = (item->flat.sensitive == True &&
				     item->flat.ancestor_sensitive == True);
    static OlgxAttrs	*new_attrs = NULL;

    /* if new_attrs was created earlier it should be destroyed. */
    if (new_attrs) {
	OlgxDestroyAttrs(w, new_attrs);
	new_attrs = NULL;
    }

    /* Let the label routine do most of the work for us.	*/

    _OlFlatSetupLabelSize(w, item, ppLbl, &sizeProc);
    plbl = *ppLbl;

	/* Now get attributes suitable for drawing the button	*/

    if (has_focus || item->flat.background_pixel != di->background)
    {
        Pixel	bg = (has_focus 	&& 
			_OlInputFocusFeedback(w) == OL_INPUT_FOCUS_COLOR ? 
					item->flat.input_focus_color :
					item->flat.background_pixel);
	Pixel fg = item->flat.font_color;

	/* Invert fg & bg if focus_color, font_color & bg clash ..*/
	if (has_focus && 
		(_OlInputFocusFeedback(w) == OL_INPUT_FOCUS_COLOR) &&
	   (item->flat.font_color==bg || item->flat.background_pixel==bg)) {
		   /* reverse fg & bg for the Button */
		   bg = fg;
		   fg = item->flat.input_focus_color;
        }

			/* Try and use previously allocated attributes. */
	if (sc->alt_attrs == (OlgxAttrs *)NULL ||
	    sc->alt_bg != bg || sc->alt_fg != fg ||
	    sc->alt_attrs->pDev->scale != psize)
	{
		if (sc->alt_attrs != (OlgxAttrs *)NULL && 
		    sc->alt_attrs->refCnt > 1)
		    /* don't ever want to free this; it's wrong to free
		     * individual item colourcells XAlloc'd for this "scratch
		     * attrs"
		     */
			OlgxDestroyAttrs(w, sc->alt_attrs);

		sc->alt_bg = bg;
		sc->alt_fg = fg;
		sc->alt_attrs = 
		    OlgxCreateAttrs (w, sc->alt_fg,
				    (OlgxBG *)&(sc->alt_bg), False, psize,
                                     item->flat.text_format,
                                     item->flat.font);
	}
	*ppAttrs = sc->alt_attrs;
    }
    else
    {
	if (item->flat.font != (OlFont)fp->pAttrs->ginfo->utextfont.fontset ||
	    item->flat.font_color != ((PrimitiveWidget)w)->primitive.font_color)
	{
	    new_attrs = OlgxCreateAttrs(w, item->flat.font_color,
					(OlgxBG *)&(item->flat.background_pixel),
					False, psize, item->flat.text_format, item->flat.font);
	    *ppAttrs = new_attrs;
	}
	else
	{
	    *ppAttrs = fp->pAttrs;
	}
    }

    if (item->flat.label != (String) NULL)
	*ppDrawProc = OlgxDrawTextButton;
    else if (item->flat.label_image != (XImage *) NULL ||
	     (item->flat.background_pixmap != (Pixmap)None &&
	      item->flat.background_pixmap != (Pixmap)ParentRelative &&
	      item->flat.background_pixmap != XtUnspecifiedPixmap))
	*ppDrawProc = OlgxDrawImageButton;
    else
	*ppDrawProc = nullProc;

} /* END OF _OlFlatSetupAttributes() */

/*
 *************************************************************************
 * _OlFlatSetupLabelSize - this routine populates a label structure with
 * the elements needed to calculate the size of the label.  It also selects
 * a function to calculate the size.  Note that this function destroys the
 * contents of the previous label.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
_OlFlatSetupLabelSize(
	Widget		w,		/* Widget making request */
	FlatItem	item,		/* expanded item */
	FlatLabel**	ppLbl,		/* pointer to label structure */
	OlSizeProc*	ppSizeProc      /* pointer to size procedure */
)
{
	static FlatLabel	lbl;
	unsigned char	justification = CVT_JUSTIFY(item->flat.label_justify);

	*ppLbl = &lbl;

	if (item->flat.label != (String)NULL)
	{
		lbl.text.label		= item->flat.label;
		lbl.text.mnemonic	= item->flat.mnemonic;
		lbl.text.qualifier	= (unsigned char *)
		    item->flat.qualifier_text;
		lbl.text.meta		= item->flat.meta_key;
		lbl.text.accelerator	= (unsigned char *)
		    item->flat.accelerator_text;
		lbl.text.font		= item->flat.font;
		lbl.text.text_format	= item->flat.text_format;
		lbl.text.flags		= 0;
		lbl.text.justification	= justification;
		*ppSizeProc		= OlgxSizeTextLabel;
	}
	else if (item->flat.label_image != (XImage *) NULL ||
	     (item->flat.background_pixmap != (Pixmap)None &&
	      item->flat.background_pixmap != (Pixmap)ParentRelative &&
	      item->flat.background_pixmap != XtUnspecifiedPixmap))
	{
		lbl.pixmap.flags		= 0;
		lbl.pixmap.justification	= justification;
		*ppSizeProc			= OlgxSizePixmapLabel;

		if (item->flat.label_tile)
		{
			lbl.pixmap.flags |= PL_TILED;
		}

		if (item->flat.label_image != (XImage *)NULL)
		{
			lbl.pixmap.type		= PL_IMAGE;
			lbl.pixmap.label.image	= item->flat.label_image;
		}
		else
		{
			lbl.pixmap.type		= PL_PIXMAP;
			lbl.pixmap.label.pixmap	= item->flat.background_pixmap;
		}
	}
	else
	{
	        *ppSizeProc = nullSize;
	}
} /* END OF _OlFlatSetupLabelSize */
