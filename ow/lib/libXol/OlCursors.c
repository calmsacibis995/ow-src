#pragma ident	"@(#)OlCursors.c	302.5	97/03/26 lib/libXol SMI"	/* olmisc:OlCursors.c 1.5	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
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


#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/olcursor.h>	/* for OL Cursor Font */

#include <Xol/OlCursors.h>
#include <Xol/OlCursorsP.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>	/* multiple display support */


/*
 * Forward declarations for static functions
 */

static Cursor _GetOlMoveCursor(Screen *screen, Colormap cmap);
static Cursor _GetOlDuplicateCursor(Screen *screen, Colormap cmap);
static Cursor _GetOlBusyCursor(Screen *screen, Colormap cmap);
static Cursor _GetOlPanCursor(Screen *screen, Colormap cmap);
static Cursor _GetOlQuestionCursor(Screen *screen, Colormap cmap);
static Cursor _GetOlTargetCursor(Screen *screen, Colormap cmap);
static Cursor _GetOlStandardCursor(Screen *screen, Colormap cmap);


/*
 * _GetBandWXColors
 *
 * The \fI_GetBandWXColors\fR procedure is used to retrieve the XColor
 * structures for the \fIblack\fR and \fIwhite\fR colors of \fIscreen\fR.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static void
_GetBandWXColors (Screen *screen, Colormap cmap, XColor *black, XColor *white)
{
static XColor rblack;
static XColor rwhite;

XColor        xblack;
XColor        xwhite;
Display *     dpy  = DisplayOfScreen(screen);

   XAllocNamedColor(dpy, cmap, "black", &rblack, &xblack);
   XAllocNamedColor(dpy, cmap, "white", &rwhite, &xwhite);

   *black = rblack;
   *white = rwhite;

} /* end of _GetBandWXColors */
/*
 * _CreateCursorFromFiles
 *
 * The \fI_OlCreateCursorFromData\fR function is used to construct a cursor
 * for \fIscreen\fR using the data in \fIsourcefile\fR and \fImaskfile\fR.
 *
 * See also:
 * 
 * _OlCreateCursorFromData(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_CreateCursorFromFiles (Screen *screen, Colormap cmap, char *sourcefile, char *maskfile)
{
Pixmap        source;
Pixmap        mask;
XColor        white;
XColor        black;
Display *     dpy     = DisplayOfScreen(screen);
Window        root    = RootWindowOfScreen(screen);
Cursor        cursor  = 0;
unsigned int  w;
unsigned int  h;
int           xhot;
int           yhot;
static char * errmsg = "Can't read '%s' for cursor\n";

_GetBandWXColors(screen, cmap, &black, &white);

if (XReadBitmapFile(dpy, root, sourcefile, &w, &h, &source, &xhot, &yhot)
    != BitmapSuccess)
   fprintf(stderr, errmsg, sourcefile);

if (XReadBitmapFile(dpy, root, maskfile, &w, &h, &mask, &xhot, &yhot)
    != BitmapSuccess)
   fprintf(stderr, errmsg, maskfile);

cursor = XCreatePixmapCursor(dpy, source, mask, &black, &white, xhot, yhot);

XFreePixmap(dpy, source);
XFreePixmap(dpy, mask);

return (cursor);

} /* end of _CreateCursorFromFiles */
/*
 * _OlCreateCursorFromData
 *
 * The \fI_OlCreateCursorFromData\fR function is used to construct a cursor
 * for \fIscreen\fR using the source data \fIsbits\fR and mask \fImbits\fR
 * whose dimensions are defined by \fIw\fR and \fIh\fR and whose hot-spot
 * position is defined by \fIxhot\fR and \fIyhot\fR.
 *
 * See also:
 * 
 * _CreateCursorFromFiles(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
_OlCreateCursorFromData (Screen *screen, Colormap cmap, const unsigned char *sbits, const unsigned char *mbits, int w, int h, int xhot, int yhot)
{
Pixmap        source;
Pixmap        mask;
XColor        white;
XColor        black;
Display *     dpy    = DisplayOfScreen(screen);
Window        root   = RootWindowOfScreen(screen);
Cursor        cursor = 0;

_GetBandWXColors(screen, cmap, &black, &white);

source = XCreateBitmapFromData(dpy, root, (char *)sbits, w, h);
mask   = XCreateBitmapFromData(dpy, root, (char *)mbits, w, h);

cursor = XCreatePixmapCursor(dpy, source, mask, &black, &white, xhot, yhot);

XFreePixmap(dpy, source);
XFreePixmap(dpy, mask);

return (cursor);

} /* end of _OlCreateCursorFromData */

/*
 * _CreateCursorFromFont
 *
 * The \fI_CreateCursorFromFont\fR function is used to construct a cursor
 * for \fIscreen\fR using the index into the OL Cursor font, screen and
 * cmap.
 *
 * See also:
 * 
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */
static Cursor
_CreateCursorFromFont (unsigned int cursor, Screen *screen, Colormap cmap)
{
	XColor	white;
	XColor	black;
	Widget	w;
	Arg	arg;
	XFontStruct *font;
	unsigned char byte1; /* for 2 byte/char fonts */
	unsigned char byte2;
	Cursor retval;

	GetToken();
	w = _OlGetScreenShellOfScreen(screen);
	XtSetArg(arg, XtNolCursorFontData, (XtPointer)&font);
	XtGetValues(w, &arg, 1);

	if(font == (XFontStruct *)NULL) {
		ReleaseToken();
		return ((Cursor)NULL);
	}

	if( !(font->min_byte1 || font->max_byte1) ) {
		/* one byte font */

		if( (cursor < font->min_char_or_byte2) ||
			(cursor+1 > font->max_char_or_byte2) ) {
			ReleaseToken();
			return((Cursor)NULL); /* cursor index is BadValue */ 
		}

	} else {
		/* two byte font */

		byte2 = cursor & 0xff; /* ls byte */
		byte1 = (cursor >> sizeof(char)) & 0xff; /* ms byte */

		if(!( (byte1 >= font->min_byte1) && 
			(byte1 <=  font->max_byte1) ) ) {
			ReleaseToken();
			return((Cursor)NULL); /* cursor index is BadValue */ 
		}

		if(!( (byte2 >= font->min_char_or_byte2) && 
			(byte2 <=  font->max_char_or_byte2) ) ) {
			ReleaseToken();
			return((Cursor)NULL); /* cursor index is BadValue */ 
		}

		byte2 = (cursor+1) & 0xff; /* ls byte */
		byte1 = ((cursor+1) >> sizeof(char)) & 0xff; /* ms byte */

		if(!( (byte1 >= font->min_byte1) && 
			(byte1 <=  font->max_byte1) ) ) {
			ReleaseToken();
			return((Cursor)NULL); 
		}
			/* cursor+1 index is BadValue */ 

		if(!( (byte2 >= font->min_char_or_byte2) && 
			(byte2 <=  font->max_char_or_byte2) ) ) {
			ReleaseToken();
			return((Cursor)NULL); 
		}
			/* cursor+1 index is BadValue */ 

	} /* else */

	/* if we got here, cursor and cursor + 1 are in the font */

	/* check if cursor and cursor+1 have non zero bounding boxes */

	if( font->all_chars_exist == False ){
		/* some characters have zero bounding boxes */

		if( !( (font->per_char[cursor].lbearing ||
	            font->per_char[cursor].rbearing) && 
		   (font->per_char[cursor].ascent ||
	            font->per_char[cursor].descent)  ) ) {
		    ReleaseToken();
		    return((Cursor)NULL);
		}

		if( !( (font->per_char[cursor+1].lbearing ||
	            font->per_char[cursor+1].rbearing) && 
		   (font->per_char[cursor+1].ascent ||
	            font->per_char[cursor+1].descent)  ) ) {
		    ReleaseToken();
		    return((Cursor)NULL);
		}

	 } /* if */

	_GetBandWXColors(screen, cmap, &black, &white);

	retval = XCreateGlyphCursor(DisplayOfScreen(screen),font->fid,
			font->fid, cursor, cursor + 1, &black, &white);
	ReleaseToken();
	return retval;
}

/*
 * GetOlMoveCursor
 *
 * The \fIGetOlMoveCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMove\fR cursor.
 *
 * See also:
 *
 * GetOlDuplicateCursor(3), GetOlBusyCursor(3),
 * GetOlPanCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
OlGetMoveCursor (Widget widget)
{
	return (_GetOlMoveCursor(XtScreen(widget), widget->core.colormap));
}

Cursor
GetOlMoveCursor (Screen *screen)
{
	return (_GetOlMoveCursor(screen, DefaultColormapOfScreen(screen)));
}

static Cursor
_GetOlMoveCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movcurs.h>
#include <Xol/bitmaps/movmask.h>
#undef char

Cursor MoveCursor;

	MoveCursor = _CreateCursorFromFont(OLC_move, screen, cmap);

	if (MoveCursor == (Cursor)NULL)
		MoveCursor = _OlCreateCursorFromData(screen,
						   cmap,
						   movcurs_bits,
						   movmask_bits,
						   movcurs_width,
						   movcurs_height,
						   movcurs_x_hot,
						   movcurs_y_hot);

return (MoveCursor);

} /* end of _GetOlMoveCursor */

/*
 * GetOlDuplicateCursor
 *
 * The \fIGetOlDuplicateCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDuplicate\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlBusyCursor(3),
 * GetOlPanCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
OlGetDuplicateCursor (Widget widget)
{
        return (_GetOlDuplicateCursor(XtScreen(widget), widget->core.colormap));
}

Cursor
GetOlDuplicateCursor (Screen *screen)
{
        return (_GetOlDuplicateCursor(screen, DefaultColormapOfScreen(screen)));
}


static Cursor
_GetOlDuplicateCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupcurs.h>
#include <Xol/bitmaps/dupmask.h>
#undef char

Cursor DuplicateCursor;

	DuplicateCursor = _CreateCursorFromFont(OLC_copy, screen, cmap);

	if (DuplicateCursor == (Cursor)NULL)
		DuplicateCursor = _OlCreateCursorFromData(screen,
							cmap,
							dupcurs_bits,
							dupmask_bits,
							dupcurs_width,
							dupcurs_height,
							dupcurs_x_hot,
							dupcurs_y_hot);

return (DuplicateCursor);

} /* end of _GetOlDuplicateCursor */
/*
 * GetOlBusyCursor
 *
 * The \fIGetOlBusyCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIBusy\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3),
 * GetOlPanCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
OlGetBusyCursor (Widget widget)
{
        return (_GetOlBusyCursor(XtScreen(widget), widget->core.colormap));
}

Cursor
GetOlBusyCursor (Screen *screen)
{
        return (_GetOlBusyCursor(screen, DefaultColormapOfScreen(screen)));
}

static Cursor
_GetOlBusyCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/busycurs.h>
#include <Xol/bitmaps/busymask.h>
#undef char

Cursor BusyCursor;

	BusyCursor = _CreateCursorFromFont(OLC_busy, screen, cmap);

	if (BusyCursor == (Cursor)NULL)
		BusyCursor = _OlCreateCursorFromData(screen,
						   cmap,
						   busycurs_bits,
						   busymask_bits,
						   busycurs_width,
						   busycurs_height,
						   busycurs_x_hot,
						   busycurs_y_hot);

return (BusyCursor);

} /* end of _GetOlBusyCursor */
/*
 * GetOlPanCursor
 *
 * The \fIGetOlPanCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIPan\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlQuestionCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
OlGetPanCursor (Widget widget)
{
        return (_GetOlPanCursor(XtScreen(widget), widget->core.colormap));
}

Cursor
GetOlPanCursor (Screen *screen)
{
        return (_GetOlPanCursor(screen, DefaultColormapOfScreen(screen)));
}


static Cursor
_GetOlPanCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/pancurs.h>
#include <Xol/bitmaps/panmask.h>
#undef char

Cursor PanCursor;

	PanCursor = _CreateCursorFromFont(OLC_panning, screen, cmap);

	if (PanCursor == (Cursor)NULL) 
		PanCursor = _OlCreateCursorFromData(screen,
						  cmap,
						  pancurs_bits,
						  panmask_bits, 
						  pancurs_width,
						  pancurs_height,
						  pancurs_x_hot,
						  pancurs_y_hot);

return (PanCursor);

} /* end of _GetOlPanCursor */
/*
 * GetOlQuestionCursor
 *
 * The \fIGetOlQuestionCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIQuestion\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlPanCursor(3), 
 * GetOlTargetCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
OlGetQuestionCursor (Widget widget)
{
        return (_GetOlQuestionCursor(XtScreen(widget), widget->core.colormap));
}

Cursor
GetOlQuestionCursor (Screen *screen)
{
        return (_GetOlQuestionCursor(screen, DefaultColormapOfScreen(screen)));
}


static Cursor
_GetOlQuestionCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/quescurs.h>
#include <Xol/bitmaps/quesmask.h>
#undef char

Cursor QuestionCursor;

	QuestionCursor = _OlCreateCursorFromData(screen,
					       cmap,
					       quescurs_bits,
					       quesmask_bits,
					       quescurs_width,
					       quescurs_height,
					       quescurs_x_hot,
					       quescurs_y_hot);

return (QuestionCursor);

} /* end of _GetOlQuestionCursor */
/*
 * GetOlTargetCursor
 *
 * The \fIGetOlTargetCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITarget\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlPanCursor(3), 
 * GetOlQuestionCursor(3), GetOlStandardCursor(3)
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
OlGetTargetCursor (Widget widget)
{
        return (_GetOlTargetCursor(XtScreen(widget), widget->core.colormap));
}

Cursor
GetOlTargetCursor (Screen *screen)
{
        return (_GetOlTargetCursor(screen, DefaultColormapOfScreen(screen)));
}


static Cursor
_GetOlTargetCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/targcurs.h>
#include <Xol/bitmaps/targmask.h>
#undef char

Cursor TargetCursor;

	TargetCursor = _CreateCursorFromFont(OLC_target, screen, cmap);

	if (TargetCursor == (Cursor)NULL)
		TargetCursor = _OlCreateCursorFromData(screen,
						     cmap,
						     targcurs_bits,
						     targmask_bits,
						     targcurs_width,
						     targcurs_height,
						     targcurs_x_hot,
						     targcurs_y_hot);

return (TargetCursor);

} /* end of _GetOlTargetCursor */
/*
 * GetOlStandardCursor
 *
 * The \fIGetOlStandardCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIStandard\fR cursor.
 *
 * See also:
 *
 * GetOlMoveCursor(3), GetOlDuplicateCursor(3), 
 * GetOlBusyCursor(3), GetOlPanCursor(3), 
 * GetOlQuestionCursor(3), GetOlTargetCursor(3),
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

Cursor
OlGetStandardCursor (Widget widget)
{
        return (_GetOlStandardCursor(XtScreen(widget), widget->core.colormap));
}

Cursor
GetOlStandardCursor (Screen *screen)
{
        return (_GetOlStandardCursor(screen, DefaultColormapOfScreen(screen)));
}


static Cursor
_GetOlStandardCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/stdcurs.h>
#include <Xol/bitmaps/stdmask.h>
#undef char

Cursor StandardCursor;

	StandardCursor = _CreateCursorFromFont(OLC_basic, screen, cmap);

	if (StandardCursor == (Cursor)NULL)
		StandardCursor = _OlCreateCursorFromData(screen,
						       cmap,
						       stdcurs_bits,
						       stdmask_bits,
						       stdcurs_width,
						       stdcurs_height,
						       stdcurs_x_hot,
						       stdcurs_y_hot);

return (StandardCursor);

} /* end of _GetOlStandardCursor */

/*
 * GetOlDocCursor
 *
 * The \fIGetOlDocCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDoc\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDocCursor (Screen *screen, Colormap cmap)
{
/*
 * The following trick gets the bitmap data into the read-only text segment,
 * thus making it shared. We *could* have directly added the 'const' keyword
 * to the output of the bitmap program, but then the bitmap program barfs
 * when attempting to re-read the bitmap.
 * The bitmap program creates the data definition as:
 *     static char foo_bits[] = { ...
 * ..but we want it to be:
 *     static const unsigned char foo_bits[] = { ...
 *
 * We achieve this by effectively temporarily redefining 'char' to be
 * 'const unsigned char'.
 * To avoid macro recursion, this is done 'via' the _OlSpecialBitmapType
 * which is typedef'd to be 'const unsigned char'
 */

#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/doc.xbm>
#include <Xol/bitmaps/doc_mask.xbm>
#undef char

	Cursor DocCursor;

	DocCursor = _CreateCursorFromFont(OLC_doc, screen, cmap);

	if (DocCursor == (Cursor)NULL) { /* fallback */
		DocCursor = _OlCreateCursorFromData(screen, cmap,
						         doc_bits,
							 doc_mask_bits,
							 doc_width,
							 doc_height,
							 doc_x_hot,
							 doc_y_hot);
	}

	return DocCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDocCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDocCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDocCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDocCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDocStackCursor
 *
 * The \fIGetOlDocStackCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDocStack\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDocStackCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/docstack.xbm>
#include <Xol/bitmaps/docstack_mask.xbm>
#undef char

	Cursor DocStackCursor;

	DocStackCursor = _CreateCursorFromFont(OLC_docstack, screen, cmap);

	if (DocStackCursor == (Cursor)NULL) { /* fallback */
		DocStackCursor = _OlCreateCursorFromData(screen, cmap,
							 docstack_bits,
							 docstack_mask_bits,
							 docstack_width,
							 docstack_height,
							 docstack_x_hot,
							 docstack_y_hot);
	}

	return DocStackCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDocStackCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDocStackCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDocStackCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDocStackCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDropCursor
 *
 * The \fIGetOlDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/drop.xbm>
#include <Xol/bitmaps/drop_mask.xbm>
#undef char

	Cursor DropCursor;

	DropCursor = _CreateCursorFromFont(OLC_drop, screen, cmap);

	if (DropCursor == (Cursor)NULL) { /* fallback */
		DropCursor = _OlCreateCursorFromData(screen, cmap,
							 drop_bits,
							 drop_mask_bits,
							 drop_width,
							 drop_height,
							 drop_x_hot,
							 drop_y_hot);
	}

	return DropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

#if	0
/*
 * GetOlDupe32Cursor
 *
 * The \fIGetOlDupe32Cursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupe32\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupe32Cursor (screen, cmap)
 Screen *		screen;
 Colormap		cmap;
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupe32.xbm>
#include <Xol/bitmaps/dupe32_mask.xbm>
#undef char

	Cursor Dupe32Cursor;

	Dupe32Cursor = _CreateCursorFromFont(OLC_dupe32, screen, cmap);

	if (Dupe32Cursor == (Cursor)NULL) { /* fallback */
		Dupe32Cursor = _OlCreateCursorFromData(screen, cmap,
							 dupe32_bits,
							 dupe32_mask_bits,
							 dupe32_width,
							 dupe32_height,
							 dupe32_x_hot,
							 dupe32_y_hot);
	}

	return Dupe32Cursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupe32Cursor (widget)
 Widget			widget;
/*FTNPROTOE*/
{
	return (_GetOlDupe32Cursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupe32Cursor (screen)
 Screen *		screen;
/*FTNPROTOE*/
{
	return (_GetOlDupe32Cursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeBoxCursor
 *
 * The \fIGetOlDupeBoxCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeBox\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeBoxCursor (screen, cmap)
 Screen *		screen;
 Colormap		cmap;
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupe_box.xbm>
#include <Xol/bitmaps/dupe_box_mask.xbm>
#undef char

	Cursor DupeBoxCursor;

	DupeBoxCursor = _CreateCursorFromFont(OLC_dupe_box, screen, cmap);

	if (DupeBoxCursor == (Cursor)NULL) { /* fallback */
		DupeBoxCursor = _OlCreateCursorFromData(screen, cmap,
							 dupe_box_bits,
							 dupe_box_mask_bits,
							 dupe_box_width,
							 dupe_box_height,
							 dupe_box_x_hot,
							 dupe_box_y_hot);
	}

	return DupeBoxCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeBoxCursor (widget)
 Widget			widget;
/*FTNPROTOE*/
{
	return (_GetOlDupeBoxCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeBoxCursor (screen)
 Screen *		screen;
/*FTNPROTOE*/
{
	return (_GetOlDupeBoxCursor(screen,
				       DefaultColormapOfScreen(screen)));
}
#endif
/*
 * GetOlDupeDocCursor
 *
 * The \fIGetOlDupeDocCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeDoc\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeDocCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupedoc.xbm>
#include <Xol/bitmaps/dupedoc_mask.xbm>
#undef char

	Cursor DupeDocCursor;

	DupeDocCursor = _CreateCursorFromFont(OLC_dupedoc, screen, cmap);

	if (DupeDocCursor == (Cursor)NULL) { /* fallback */
		DupeDocCursor = _OlCreateCursorFromData(screen, cmap,
							 dupedoc_bits,
							 dupedoc_mask_bits,
							 dupedoc_width,
							 dupedoc_height,
							 dupedoc_x_hot,
							 dupedoc_y_hot);
	}

	return DupeDocCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeDocCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeDocCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeDocDragCursor
 *
 * The \fIGetOlDupeDocDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeDocDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeDocDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupedoc_drag.xbm>
#include <Xol/bitmaps/dupedoc_drag_mask.xbm>
#undef char

	Cursor DupeDocDragCursor;

	DupeDocDragCursor = _CreateCursorFromFont(OLC_dupedoc_drag, screen, cmap);

	if (DupeDocDragCursor == (Cursor)NULL) { /* fallback */
		DupeDocDragCursor = _OlCreateCursorFromData(screen, cmap,
							 dupedoc_drag_bits,
							 dupedoc_drag_mask_bits,
							 dupedoc_drag_width,
							 dupedoc_drag_height,
							 dupedoc_drag_x_hot,
							 dupedoc_drag_y_hot);
	}

	return DupeDocDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeDocDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeDocDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeDocDropCursor
 *
 * The \fIGetOlDupeDocDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeDocDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeDocDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupedoc_drop.xbm>
#include <Xol/bitmaps/dupedoc_drop_mask.xbm>
#undef char

	Cursor DupeDocDropCursor;

	DupeDocDropCursor = _CreateCursorFromFont(OLC_dupedoc_drop, screen, cmap);

	if (DupeDocDropCursor == (Cursor)NULL) { /* fallback */
		DupeDocDropCursor = _OlCreateCursorFromData(screen, cmap,
							 dupedoc_drop_bits,
							 dupedoc_drop_mask_bits,
							 dupedoc_drop_width,
							 dupedoc_drop_height,
							 dupedoc_drop_x_hot,
							 dupedoc_drop_y_hot);
	}

	return DupeDocDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeDocDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeDocDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeDocNoDropCursor
 *
 * The \fIGetOlDupeDocNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeDocNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeDocNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupedoc_nodrop.xbm>
#include <Xol/bitmaps/dupedoc_nodrop_mask.xbm>
#undef char

	Cursor DupeDocNoDropCursor;

	DupeDocNoDropCursor = _CreateCursorFromFont(OLC_dupedoc_nodrop, screen, cmap);

	if (DupeDocNoDropCursor == (Cursor)NULL) { /* fallback */
		DupeDocNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 dupedoc_nodrop_bits,
							 dupedoc_nodrop_mask_bits,
							 dupedoc_nodrop_width,
							 dupedoc_nodrop_height,
							 dupedoc_nodrop_x_hot,
							 dupedoc_nodrop_y_hot);
	}

	return DupeDocNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeDocNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeDocNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeDocNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeStackCursor
 *
 * The \fIGetOlDupeStackCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeStack\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeStackCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupestack.xbm>
#include <Xol/bitmaps/dupestack_mask.xbm>
#undef char

	Cursor DupeStackCursor;

	DupeStackCursor = _CreateCursorFromFont(OLC_dupestack, screen, cmap);

	if (DupeStackCursor == (Cursor)NULL) { /* fallback */
		DupeStackCursor = _OlCreateCursorFromData(screen, cmap,
							 dupestack_bits,
							 dupestack_mask_bits,
							 dupestack_width,
							 dupestack_height,
							 dupestack_x_hot,
							 dupestack_y_hot);
	}

	return DupeStackCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeStackCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeStackCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeStackDragCursor
 *
 * The \fIGetOlDupeStackDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeStackDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeStackDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupestack_drag.xbm>
#include <Xol/bitmaps/dupestack_drag_mask.xbm>
#undef char

	Cursor DupeStackDragCursor;

	DupeStackDragCursor = _CreateCursorFromFont(OLC_dupestack_drag, screen, cmap);

	if (DupeStackDragCursor == (Cursor)NULL) { /* fallback */
		DupeStackDragCursor = _OlCreateCursorFromData(screen, cmap,
							 dupestack_drag_bits,
							 dupestack_drag_mask_bits,
							 dupestack_drag_width,
							 dupestack_drag_height,
							 dupestack_drag_x_hot,
							 dupestack_drag_y_hot);
	}

	return DupeStackDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeStackDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeStackDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeStackDropCursor
 *
 * The \fIGetOlDupeStackDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeStackDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeStackDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupestack_drop.xbm>
#include <Xol/bitmaps/dupestack_drop_mask.xbm>
#undef char

	Cursor DupeStackDropCursor;

	DupeStackDropCursor = _CreateCursorFromFont(OLC_dupestack_drop, screen, cmap);

	if (DupeStackDropCursor == (Cursor)NULL) { /* fallback */
		DupeStackDropCursor = _OlCreateCursorFromData(screen, cmap,
							 dupestack_drop_bits,
							 dupestack_drop_mask_bits,
							 dupestack_drop_width,
							 dupestack_drop_height,
							 dupestack_drop_x_hot,
							 dupestack_drop_y_hot);
	}

	return DupeStackDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeStackDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeStackDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlDupeStackNoDropCursor
 *
 * The \fIGetOlDupeStackNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDupeStackNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDupeStackNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/dupestack_nodrop.xbm>
#include <Xol/bitmaps/dupestack_nodrop_mask.xbm>
#undef char

	Cursor DupeStackNoDropCursor;

	DupeStackNoDropCursor = _CreateCursorFromFont(OLC_dupestack_nodrop, screen, cmap);

	if (DupeStackNoDropCursor == (Cursor)NULL) { /* fallback */
		DupeStackNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 dupestack_nodrop_bits,
							 dupestack_nodrop_mask_bits,
							 dupestack_nodrop_width,
							 dupestack_nodrop_height,
							 dupestack_nodrop_x_hot,
							 dupestack_nodrop_y_hot);
	}

	return DupeStackNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDupeStackNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDupeStackNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDupeStackNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

#if	0
/*
 * GetOlMove32Cursor
 *
 * The \fIGetOlMove32Cursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMove32\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMove32Cursor (screen, cmap)
 Screen *		screen;
 Colormap		cmap;
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/move32.xbm>
#include <Xol/bitmaps/move32_mask.xbm>
#undef char

	Cursor Move32Cursor;

	Move32Cursor = _CreateCursorFromFont(OLC_move32, screen, cmap);

	if (Move32Cursor == (Cursor)NULL) { /* fallback */
		Move32Cursor = _OlCreateCursorFromData(screen, cmap,
							 move32_bits,
							 move32_mask_bits,
							 move32_width,
							 move32_height,
							 move32_x_hot,
							 move32_y_hot);
	}

	return Move32Cursor;
}

Cursor
/*FTNPROTOB*/
OlGetMove32Cursor (widget)
 Widget			widget;
/*FTNPROTOE*/
{
	return (_GetOlMove32Cursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMove32Cursor (screen)
 Screen *		screen;
/*FTNPROTOE*/
{
	return (_GetOlMove32Cursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveBoxCursor
 *
 * The \fIGetOlMoveBoxCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveBox\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveBoxCursor (screen, cmap)
 Screen *		screen;
 Colormap		cmap;
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/move_box.xbm>
#include <Xol/bitmaps/move_box_mask.xbm>
#undef char

	Cursor MoveBoxCursor;

	MoveBoxCursor = _CreateCursorFromFont(OLC_move_box, screen, cmap);

	if (MoveBoxCursor == (Cursor)NULL) { /* fallback */
		MoveBoxCursor = _OlCreateCursorFromData(screen, cmap,
							 move_box_bits,
							 move_box_mask_bits,
							 move_box_width,
							 move_box_height,
							 move_box_x_hot,
							 move_box_y_hot);
	}

	return MoveBoxCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveBoxCursor (widget)
 Widget			widget;
/*FTNPROTOE*/
{
	return (_GetOlMoveBoxCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveBoxCursor (screen)
 Screen *		screen;
/*FTNPROTOE*/
{
	return (_GetOlMoveBoxCursor(screen,
				       DefaultColormapOfScreen(screen)));
}
#endif
/*
 * GetOlMoveDocCursor
 *
 * The \fIGetOlMoveDocCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveDoc\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveDocCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movedoc.xbm>
#include <Xol/bitmaps/movedoc_mask.xbm>
#undef char

	Cursor MoveDocCursor;

	MoveDocCursor = _CreateCursorFromFont(OLC_movedoc, screen, cmap);

	if (MoveDocCursor == (Cursor)NULL) { /* fallback */
		MoveDocCursor = _OlCreateCursorFromData(screen, cmap,
							 movedoc_bits,
							 movedoc_mask_bits,
							 movedoc_width,
							 movedoc_height,
							 movedoc_x_hot,
							 movedoc_y_hot);
	}

	return MoveDocCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveDocCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveDocCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveDocDragCursor
 *
 * The \fIGetOlMoveDocDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveDocDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveDocDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movedoc_drag.xbm>
#include <Xol/bitmaps/movedoc_drag_mask.xbm>
#undef char

	Cursor MoveDocDragCursor;

	MoveDocDragCursor = _CreateCursorFromFont(OLC_movedoc_drag, screen, cmap);

	if (MoveDocDragCursor == (Cursor)NULL) { /* fallback */
		MoveDocDragCursor = _OlCreateCursorFromData(screen, cmap,
							 movedoc_drag_bits,
							 movedoc_drag_mask_bits,
							 movedoc_drag_width,
							 movedoc_drag_height,
							 movedoc_drag_x_hot,
							 movedoc_drag_y_hot);
	}

	return MoveDocDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveDocDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveDocDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveDocDropCursor
 *
 * The \fIGetOlMoveDocDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveDocDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveDocDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movedoc_drop.xbm>
#include <Xol/bitmaps/movedoc_drop_mask.xbm>
#undef char

	Cursor MoveDocDropCursor;

	MoveDocDropCursor = _CreateCursorFromFont(OLC_movedoc_drop, screen, cmap);

	if (MoveDocDropCursor == (Cursor)NULL) { /* fallback */
		MoveDocDropCursor = _OlCreateCursorFromData(screen, cmap,
							 movedoc_drop_bits,
							 movedoc_drop_mask_bits,
							 movedoc_drop_width,
							 movedoc_drop_height,
							 movedoc_drop_x_hot,
							 movedoc_drop_y_hot);
	}

	return MoveDocDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveDocDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveDocDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveDocNoDropCursor
 *
 * The \fIGetOlMoveDocNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveDocNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveDocNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movedoc_nodrop.xbm>
#include <Xol/bitmaps/movedoc_nodrop_mask.xbm>
#undef char

	Cursor MoveDocNoDropCursor;

	MoveDocNoDropCursor = _CreateCursorFromFont(OLC_movedoc_nodrop, screen, cmap);

	if (MoveDocNoDropCursor == (Cursor)NULL) { /* fallback */
		MoveDocNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 movedoc_nodrop_bits,
							 movedoc_nodrop_mask_bits,
							 movedoc_nodrop_width,
							 movedoc_nodrop_height,
							 movedoc_nodrop_x_hot,
							 movedoc_nodrop_y_hot);
	}

	return MoveDocNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveDocNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveDocNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveDocNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveStackCursor
 *
 * The \fIGetOlMoveStackCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveStack\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveStackCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movestack.xbm>
#include <Xol/bitmaps/movestack_mask.xbm>
#undef char

	Cursor MoveStackCursor;

	MoveStackCursor = _CreateCursorFromFont(OLC_movestack, screen, cmap);

	if (MoveStackCursor == (Cursor)NULL) { /* fallback */
		MoveStackCursor = _OlCreateCursorFromData(screen, cmap,
							 movestack_bits,
							 movestack_mask_bits,
							 movestack_width,
							 movestack_height,
							 movestack_x_hot,
							 movestack_y_hot);
	}

	return MoveStackCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveStackCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveStackCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveStackDragCursor
 *
 * The \fIGetOlMoveStackDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveStackDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveStackDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movestack_drag.xbm>
#include <Xol/bitmaps/movestack_drag_mask.xbm>
#undef char

	Cursor MoveStackDragCursor;

	MoveStackDragCursor = _CreateCursorFromFont(OLC_movestack_drag, screen, cmap);

	if (MoveStackDragCursor == (Cursor)NULL) { /* fallback */
		MoveStackDragCursor = _OlCreateCursorFromData(screen, cmap,
							 movestack_drag_bits,
							 movestack_drag_mask_bits,
							 movestack_drag_width,
							 movestack_drag_height,
							 movestack_drag_x_hot,
							 movestack_drag_y_hot);
	}

	return MoveStackDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveStackDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveStackDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveStackDropCursor
 *
 * The \fIGetOlMoveStackDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveStackDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveStackDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movestack_drop.xbm>
#include <Xol/bitmaps/movestack_drop_mask.xbm>
#undef char

	Cursor MoveStackDropCursor;

	MoveStackDropCursor = _CreateCursorFromFont(OLC_movestack_drop, screen, cmap);

	if (MoveStackDropCursor == (Cursor)NULL) { /* fallback */
		MoveStackDropCursor = _OlCreateCursorFromData(screen, cmap,
							 movestack_drop_bits,
							 movestack_drop_mask_bits,
							 movestack_drop_width,
							 movestack_drop_height,
							 movestack_drop_x_hot,
							 movestack_drop_y_hot);
	}

	return MoveStackDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveStackDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveStackDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlMoveStackNoDropCursor
 *
 * The \fIGetOlMoveStackNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIMoveStackNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlMoveStackNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/movestack_nodrop.xbm>
#include <Xol/bitmaps/movestack_nodrop_mask.xbm>
#undef char

	Cursor MoveStackNoDropCursor;

	MoveStackNoDropCursor = _CreateCursorFromFont(OLC_movestack_nodrop, screen, cmap);

	if (MoveStackNoDropCursor == (Cursor)NULL) { /* fallback */
		MoveStackNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 movestack_nodrop_bits,
							 movestack_nodrop_mask_bits,
							 movestack_nodrop_width,
							 movestack_nodrop_height,
							 movestack_nodrop_x_hot,
							 movestack_nodrop_y_hot);
	}

	return MoveStackNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetMoveStackNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlMoveStackNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlMoveStackNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlNoDropCursor
 *
 * The \fIGetOlNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fINoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/nodrop.xbm>
#include <Xol/bitmaps/nodrop_mask.xbm>
#undef char

	Cursor NoDropCursor;

	NoDropCursor = _CreateCursorFromFont(OLC_nodrop, screen, cmap);

	if (NoDropCursor == (Cursor)NULL) { /* fallback */
		NoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 nodrop_bits,
							 nodrop_mask_bits,
							 nodrop_width,
							 nodrop_height,
							 nodrop_x_hot,
							 nodrop_y_hot);
	}

	return NoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

#if	0
/*
 * GetOlTextDupeBoxCursor
 *
 * The \fIGetOlTextDupeBoxCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextDupeBox\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextDupeBoxCursor (screen, cmap)
 Screen *		screen;
 Colormap		cmap;
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textdupe_box.xbm>
#include <Xol/bitmaps/textdupe_box_mask.xbm>
#undef char

	Cursor TextDupeBoxCursor;

	TextDupeBoxCursor = _CreateCursorFromFont(OLC_textdupe_box, screen, cmap);

	if (TextDupeBoxCursor == (Cursor)NULL) { /* fallback */
		TextDupeBoxCursor = _OlCreateCursorFromData(screen, cmap,
							 textdupe_box_bits,
							 textdupe_box_mask_bits,
							 textdupe_box_width,
							 textdupe_box_height,
							 textdupe_box_x_hot,
							 textdupe_box_y_hot);
	}

	return TextDupeBoxCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextDupeBoxCursor (widget)
 Widget			widget;
/*FTNPROTOE*/
{
	return (_GetOlTextDupeBoxCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextDupeBoxCursor (screen)
 Screen *		screen;
/*FTNPROTOE*/
{
	return (_GetOlTextDupeBoxCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

#endif
/*
 * GetOlTextDupeDragCursor
 *
 * The \fIGetOlTextDupeDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextDupeDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextDupeDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textdupe_drag.xbm>
#include <Xol/bitmaps/textdupe_drag_mask.xbm>
#undef char

	Cursor TextDupeDragCursor;

	TextDupeDragCursor = _CreateCursorFromFont(OLC_textdupe_drag, screen, cmap);

	if (TextDupeDragCursor == (Cursor)NULL) { /* fallback */
		TextDupeDragCursor = _OlCreateCursorFromData(screen, cmap,
							 textdupe_drag_bits,
							 textdupe_drag_mask_bits,
							 textdupe_drag_width,
							 textdupe_drag_height,
							 textdupe_drag_x_hot,
							 textdupe_drag_y_hot);
	}

	return TextDupeDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextDupeDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextDupeDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlTextDupeDropCursor
 *
 * The \fIGetOlTextDupeDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextDupeDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextDupeDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textdupe_drop.xbm>
#include <Xol/bitmaps/textdupe_drop_mask.xbm>
#undef char

	Cursor TextDupeDropCursor;

	TextDupeDropCursor = _CreateCursorFromFont(OLC_textdupe_drop, screen, cmap);

	if (TextDupeDropCursor == (Cursor)NULL) { /* fallback */
		TextDupeDropCursor = _OlCreateCursorFromData(screen, cmap,
							 textdupe_drop_bits,
							 textdupe_drop_mask_bits,
							 textdupe_drop_width,
							 textdupe_drop_height,
							 textdupe_drop_x_hot,
							 textdupe_drop_y_hot);
	}

	return TextDupeDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextDupeDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextDupeDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlTextDupeNoDropCursor
 *
 * The \fIGetOlTextDupeNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextDupeNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextDupeNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textdupe_nodrop.xbm>
#include <Xol/bitmaps/textdupe_nodrop_mask.xbm>
#undef char

	Cursor TextDupeNoDropCursor;

	TextDupeNoDropCursor = _CreateCursorFromFont(OLC_textdupe_nodrop, screen, cmap);

	if (TextDupeNoDropCursor == (Cursor)NULL) { /* fallback */
		TextDupeNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 textdupe_nodrop_bits,
							 textdupe_nodrop_mask_bits,
							 textdupe_nodrop_width,
							 textdupe_nodrop_height,
							 textdupe_nodrop_x_hot,
							 textdupe_nodrop_y_hot);
	}

	return TextDupeNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextDupeNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextDupeNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

#if	0
/*
 * GetOlTextMoveBoxCursor
 *
 * The \fIGetOlTextMoveBoxCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextMoveBox\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextMoveBoxCursor (screen, cmap)
 Screen *		screen;
 Colormap		cmap;
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textmove_box.xbm>
#include <Xol/bitmaps/textmove_box_mask.xbm>
#undef char

	Cursor TextMoveBoxCursor;

	TextMoveBoxCursor = _CreateCursorFromFont(OLC_textmove_box, screen, cmap);

	if (TextMoveBoxCursor == (Cursor)NULL) { /* fallback */
		TextMoveBoxCursor = _OlCreateCursorFromData(screen, cmap,
							 textmove_box_bits,
							 textmove_box_mask_bits,
							 textmove_box_width,
							 textmove_box_height,
							 textmove_box_x_hot,
							 textmove_box_y_hot);
	}

	return TextMoveBoxCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextMoveBoxCursor (widget)
 Widget			widget;
/*FTNPROTOE*/
{
	return (_GetOlTextMoveBoxCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextMoveBoxCursor (screen)
 Screen *		screen;
/*FTNPROTOE*/
{
	return (_GetOlTextMoveBoxCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

#endif
/*
 * GetOlTextMoveDragCursor
 *
 * The \fIGetOlTextMoveDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextMoveDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextMoveDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textmove_drag.xbm>
#include <Xol/bitmaps/textmove_drag_mask.xbm>
#undef char

	Cursor TextMoveDragCursor;

	TextMoveDragCursor = _CreateCursorFromFont(OLC_textmove_drag, screen, cmap);

	if (TextMoveDragCursor == (Cursor)NULL) { /* fallback */
		TextMoveDragCursor = _OlCreateCursorFromData(screen, cmap,
							 textmove_drag_bits,
							 textmove_drag_mask_bits,
							 textmove_drag_width,
							 textmove_drag_height,
							 textmove_drag_x_hot,
							 textmove_drag_y_hot);
	}

	return TextMoveDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextMoveDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextMoveDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlTextMoveDropCursor
 *
 * The \fIGetOlTextMoveDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextMoveDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextMoveDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textmove_drop.xbm>
#include <Xol/bitmaps/textmove_drop_mask.xbm>
#undef char

	Cursor TextMoveDropCursor;

	TextMoveDropCursor = _CreateCursorFromFont(OLC_textmove_drop, screen, cmap);

	if (TextMoveDropCursor == (Cursor)NULL) { /* fallback */
		TextMoveDropCursor = _OlCreateCursorFromData(screen, cmap,
							 textmove_drop_bits,
							 textmove_drop_mask_bits,
							 textmove_drop_width,
							 textmove_drop_height,
							 textmove_drop_x_hot,
							 textmove_drop_y_hot);
	}

	return TextMoveDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextMoveDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextMoveDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlTextMoveNoDropCursor
 *
 * The \fIGetOlTextMoveNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextMoveNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextMoveNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textmove_nodrop.xbm>
#include <Xol/bitmaps/textmove_nodrop_mask.xbm>
#undef char

	Cursor TextMoveNoDropCursor;

	TextMoveNoDropCursor = _CreateCursorFromFont(OLC_textmove_nodrop, screen, cmap);

	if (TextMoveNoDropCursor == (Cursor)NULL) { /* fallback */
		TextMoveNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 textmove_nodrop_bits,
							 textmove_nodrop_mask_bits,
							 textmove_nodrop_width,
							 textmove_nodrop_height,
							 textmove_nodrop_x_hot,
							 textmove_nodrop_y_hot);
	}

	return TextMoveNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextMoveNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextMoveNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlTextMoveInsertCursor
 *
 * The \fIGetOlTextMoveInsertCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextMoveInsert\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextMoveInsertCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textmove_insert.xbm>
#include <Xol/bitmaps/textmove_insert_mask.xbm>
#undef char

	Cursor TextMoveInsertCursor;

	TextMoveInsertCursor = _CreateCursorFromFont(OLC_textmove_insert, screen, cmap);

	if (TextMoveInsertCursor == (Cursor)NULL) { /* fallback */
		TextMoveInsertCursor = _OlCreateCursorFromData(screen, cmap,
							 textmove_insert_bits,
							 textmove_insert_mask_bits,
							 textmove_insert_width,
							 textmove_insert_height,
							 textmove_insert_x_hot,
							 textmove_insert_y_hot);
	}

	return TextMoveInsertCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextMoveInsertCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveInsertCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextMoveInsertCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextMoveInsertCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * GetOlTextDupeInsertCursor
 *
 * The \fIGetOlTextDupeInsertCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fITextDupeInsert\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlTextDupeInsertCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/textdupe_insert.xbm>
#include <Xol/bitmaps/textdupe_insert_mask.xbm>
#undef char

	Cursor TextDupeInsertCursor;

	TextDupeInsertCursor = _CreateCursorFromFont(OLC_textdupe_insert, screen, cmap);

	if (TextDupeInsertCursor == (Cursor)NULL) { /* fallback */
		TextDupeInsertCursor = _OlCreateCursorFromData(screen, cmap,
							 textdupe_insert_bits,
							 textdupe_insert_mask_bits,
							 textdupe_insert_width,
							 textdupe_insert_height,
							 textdupe_insert_x_hot,
							 textdupe_insert_y_hot);
	}

	return TextDupeInsertCursor;
}

Cursor
/*FTNPROTOB*/
OlGetTextDupeInsertCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeInsertCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlTextDupeInsertCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlTextDupeInsertCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataDupeDragCursor
 *
 * The \fIGetOlDataDupeDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataDupeDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataDupeDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datadupe_drag.xbm>
#include <Xol/bitmaps/datadupe_drag_mask.xbm>
#undef char

	Cursor DataDupeDragCursor;

#ifdef	OLC_datadupe_drag
	DataDupeDragCursor = _CreateCursorFromFont(OLC_datadupe_drag, screen, cmap);

	if (DataDupeDragCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataDupeDragCursor = _OlCreateCursorFromData(screen, cmap,
							 datadupe_drag_bits,
							 datadupe_drag_mask_bits,
							 datadupe_drag_width,
							 datadupe_drag_height,
							 datadupe_drag_x_hot,
							 datadupe_drag_y_hot);
#ifdef	OLC_datadupe_drag
	}
#endif

	return DataDupeDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataDupeDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataDupeDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataDupeDropCursor
 *
 * The \fIGetOlDataDupeDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataDupeDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataDupeDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datadupe_drop.xbm>
#include <Xol/bitmaps/datadupe_drop_mask.xbm>
#undef char

	Cursor DataDupeDropCursor;

#ifdef	OLC_datadupe_drop
	DataDupeDropCursor = _CreateCursorFromFont(OLC_datadupe_drop, screen, cmap);

	if (DataDupeDropCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataDupeDropCursor = _OlCreateCursorFromData(screen, cmap,
							 datadupe_drop_bits,
							 datadupe_drop_mask_bits,
							 datadupe_drop_width,
							 datadupe_drop_height,
							 datadupe_drop_x_hot,
							 datadupe_drop_y_hot);
#ifdef	OLC_datadupe_drop
	}
#endif

	return DataDupeDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataDupeDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataDupeDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataDupeInsertCursor
 *
 * The \fIGetOlDataDupeInsertCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataDupeInsert\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataDupeInsertCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datadupe_insert.xbm>
#include <Xol/bitmaps/datadupe_insert_mask.xbm>
#undef char

	Cursor DataDupeInsertCursor;

#ifdef	OLC_datadupe_insert
	DataDupeInsertCursor = _CreateCursorFromFont(OLC_datadupe_insert, screen, cmap);

	if (DataDupeInsertCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataDupeInsertCursor = _OlCreateCursorFromData(screen, cmap,
							 datadupe_insert_bits,
							 datadupe_insert_mask_bits,
							 datadupe_insert_width,
							 datadupe_insert_height,
							 datadupe_insert_x_hot,
							 datadupe_insert_y_hot);
#ifdef	OLC_datadupe_insert
	}
#endif

	return DataDupeInsertCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataDupeInsertCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeInsertCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataDupeInsertCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeInsertCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataDupeNoDropCursor
 *
 * The \fIGetOlDataDupeNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataDupeNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataDupeNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datadupe_nodrop.xbm>
#include <Xol/bitmaps/datadupe_nodrop_mask.xbm>
#undef char

	Cursor DataDupeNoDropCursor;

#ifdef	OLC_datadupe_nodrop
	DataDupeNoDropCursor = _CreateCursorFromFont(OLC_datadupe_nodrop, screen, cmap);

	if (DataDupeNoDropCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataDupeNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 datadupe_nodrop_bits,
							 datadupe_nodrop_mask_bits,
							 datadupe_nodrop_width,
							 datadupe_nodrop_height,
							 datadupe_nodrop_x_hot,
							 datadupe_nodrop_y_hot);
#ifdef	OLC_datadupe_nodrop
	}
#endif

	return DataDupeNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataDupeNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataDupeNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataDupeNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataMoveDragCursor
 *
 * The \fIGetOlDataMoveDragCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataMoveDrag\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataMoveDragCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datamove_drag.xbm>
#include <Xol/bitmaps/datamove_drag_mask.xbm>
#undef char

	Cursor DataMoveDragCursor;

#ifdef	OLC_datamove_drag
	DataMoveDragCursor = _CreateCursorFromFont(OLC_datamove_drag, screen, cmap);

	if (DataMoveDragCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataMoveDragCursor = _OlCreateCursorFromData(screen, cmap,
							 datamove_drag_bits,
							 datamove_drag_mask_bits,
							 datamove_drag_width,
							 datamove_drag_height,
							 datamove_drag_x_hot,
							 datamove_drag_y_hot);
#ifdef	OLC_datamove_drag
	}
#endif

	return DataMoveDragCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataMoveDragCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveDragCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataMoveDragCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveDragCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataMoveDropCursor
 *
 * The \fIGetOlDataMoveDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataMoveDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataMoveDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datamove_drop.xbm>
#include <Xol/bitmaps/datamove_drop_mask.xbm>
#undef char

	Cursor DataMoveDropCursor;

#ifdef	OLC_datamove_drop
	DataMoveDropCursor = _CreateCursorFromFont(OLC_datamove_drop, screen, cmap);

	if (DataMoveDropCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataMoveDropCursor = _OlCreateCursorFromData(screen, cmap,
							 datamove_drop_bits,
							 datamove_drop_mask_bits,
							 datamove_drop_width,
							 datamove_drop_height,
							 datamove_drop_x_hot,
							 datamove_drop_y_hot);
#ifdef	OLC_datamove_drop
	}
#endif

	return DataMoveDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataMoveDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataMoveDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataMoveInsertCursor
 *
 * The \fIGetOlDataMoveInsertCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataMoveInsert\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataMoveInsertCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datamove_insert.xbm>
#include <Xol/bitmaps/datamove_insert_mask.xbm>
#undef char

	Cursor DataMoveInsertCursor;

#ifdef	OLC_datamove_insert
	DataMoveInsertCursor = _CreateCursorFromFont(OLC_datamove_insert, screen, cmap);

	if (DataMoveInsertCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataMoveInsertCursor = _OlCreateCursorFromData(screen, cmap,
							 datamove_insert_bits,
							 datamove_insert_mask_bits,
							 datamove_insert_width,
							 datamove_insert_height,
							 datamove_insert_x_hot,
							 datamove_insert_y_hot);
#ifdef	OLC_datamove_insert
	}
#endif

	return DataMoveInsertCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataMoveInsertCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveInsertCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataMoveInsertCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveInsertCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlDataMoveNoDropCursor
 *
 * The \fIGetOlDataMoveNoDropCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIDataMoveNoDrop\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlDataMoveNoDropCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/datamove_nodrop.xbm>
#include <Xol/bitmaps/datamove_nodrop_mask.xbm>
#undef char

	Cursor DataMoveNoDropCursor;

#ifdef	OLC_datamove_nodrop
	DataMoveNoDropCursor = _CreateCursorFromFont(OLC_datamove_nodrop, screen, cmap);

	if (DataMoveNoDropCursor == (Cursor)NULL) { /* fallback */
#endif	
		DataMoveNoDropCursor = _OlCreateCursorFromData(screen, cmap,
							 datamove_nodrop_bits,
							 datamove_nodrop_mask_bits,
							 datamove_nodrop_width,
							 datamove_nodrop_height,
							 datamove_nodrop_x_hot,
							 datamove_nodrop_y_hot);
#ifdef	OLC_datamove_nodrop
	}
#endif

	return DataMoveNoDropCursor;
}

Cursor
/*FTNPROTOB*/
OlGetDataMoveNoDropCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveNoDropCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlDataMoveNoDropCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlDataMoveNoDropCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlFolderCursor
 *
 * The \fIGetOlFolderCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIFolder\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlFolderCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/folder.xbm>
#include <Xol/bitmaps/folder_mask.xbm>
#undef char

	Cursor FolderCursor;

#ifdef	OLC_folder
	FolderCursor = _CreateCursorFromFont(OLC_folder, screen, cmap);

	if (FolderCursor == (Cursor)NULL) { /* fallback */
#endif	
		FolderCursor = _OlCreateCursorFromData(screen, cmap,
							 folder_bits,
							 folder_mask_bits,
							 folder_width,
							 folder_height,
							 folder_x_hot,
							 folder_y_hot);
#ifdef	OLC_folder
	}
#endif

	return FolderCursor;
}

Cursor
/*FTNPROTOB*/
OlGetFolderCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlFolderCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlFolderCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlFolderCursor(screen,
				       DefaultColormapOfScreen(screen)));
}


/*
 * GetOlFolderStackCursor
 *
 * The \fIGetOlFolderStackCursor\fR function is used to retrieve the Cursor id
 * for \fIscreen\fR which complies with the OPEN LOOK(tm) specification
 * of the \fIFolderStack\fR cursor.
 *
 * Return value:
 *
 * The Cursor id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static Cursor
_GetOlFolderStackCursor (Screen *screen, Colormap cmap)
{
#define char _OlReadOnlyBitmapType
#include <Xol/bitmaps/folderstack.xbm>
#include <Xol/bitmaps/folderstack_mask.xbm>
#undef char

	Cursor FolderStackCursor;

#ifdef	OLC_folderstack
	FolderStackCursor = _CreateCursorFromFont(OLC_folderstack, screen, cmap);

	if (FolderStackCursor == (Cursor)NULL) { /* fallback */
#endif	
		FolderStackCursor = _OlCreateCursorFromData(screen, cmap,
							 folderstack_bits,
							 folderstack_mask_bits,
							 folderstack_width,
							 folderstack_height,
							 folderstack_x_hot,
							 folderstack_y_hot);
#ifdef	OLC_folderstack
	}
#endif

	return FolderStackCursor;
}

Cursor
/*FTNPROTOB*/
OlGetFolderStackCursor (Widget widget)
       			       
/*FTNPROTOE*/
{
	return (_GetOlFolderStackCursor(XtScreen(widget),
				       widget->core.colormap));
}

Cursor
/*FTNPROTOB*/
GetOlFolderStackCursor (Screen *screen)
         		       
/*FTNPROTOE*/
{
	return (_GetOlFolderStackCursor(screen,
				       DefaultColormapOfScreen(screen)));
}

/*
 * OlGet50PercentGrey
 *
 * The \fIOlGet50PercentGrey\fR function is used to retrieve the id
 * of a 50 percent grey Pixmap for \fIscreen\fR.
 *
 * Return value:
 *
 * The Pixmap id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static  _Ol50PercentGrey        *Ol50PercentGrey = NULL;

extern Pixmap
OlGet50PercentGrey (Screen *screen)
{

#define grey_width 2
#define grey_height 2
static const char grey_bits[] = { 0x01, 0x02};

/* Search the cache for our screen */
register _Ol50PercentGrey *pPix;
for (pPix = Ol50PercentGrey; pPix ; pPix = pPix->next) {
	if (pPix->scr == screen)
		return pPix->pixmap;
}
/* No entry - make one ...and insert it at head of list */
pPix = (_Ol50PercentGrey *)XtMalloc(sizeof(_Ol50PercentGrey));
pPix->scr = screen;
pPix->pixmap = XCreateBitmapFromData(DisplayOfScreen(screen),
		    RootWindowOfScreen(screen),
		    (char *)grey_bits, grey_width, grey_height);
pPix->next = Ol50PercentGrey;
Ol50PercentGrey = pPix;

#undef grey_width
#undef grey_height

return (pPix->pixmap);

} /* end of OlGet50PercentGrey */

void
_OlFlush50PercentGreyCache(void)
{
	register _Ol50PercentGrey *pPix, *tmpPix;

	for (pPix = Ol50PercentGrey; pPix ; ) {
		tmpPix = pPix->next;
		XtFree((char *)pPix);

		pPix = tmpPix;
	}

	Ol50PercentGrey = (_Ol50PercentGrey *)NULL;
}

/*
 * OlGet75PercentGrey
 *
 * The \fIOlGet75PercentGrey\fR function is used to retrieve the id
 * of a 75 percent grey Pixmap for \fIscreen\fR.
 *
 * Return value:
 *
 * The Pixmap id is returned.
 *
 * Synopsis:
 *
 *#include <OlCursors.h>
 * ...
 */

static  _Ol75PercentGrey        *Ol75PercentGrey = NULL;

extern Pixmap
OlGet75PercentGrey (Screen *screen)
{

#define grey_width 4
#define grey_height 2
static const char grey_bits[] = { 0x0d, 0x07};

register _Ol75PercentGrey *pPix;
for (pPix = Ol75PercentGrey; pPix ; pPix = pPix->next) {
	if (pPix->scr == screen)
		return pPix->pixmap;
}
pPix = (_Ol75PercentGrey *)XtMalloc(sizeof(_Ol75PercentGrey));
pPix->scr = screen;
pPix->pixmap = XCreateBitmapFromData(DisplayOfScreen(screen),
		    RootWindowOfScreen(screen),
		    (char *)grey_bits, grey_width, grey_height);
pPix->next = Ol75PercentGrey;
Ol75PercentGrey = pPix;

#undef grey_width
#undef grey_height

return (pPix->pixmap);

} /* end of OlGet75PercentGrey */

void
_OlFlush75PercentGreyCache(void)
{
	register _Ol75PercentGrey *pPix, *tmpPix;

	for (pPix = Ol75PercentGrey; pPix ; ) {
		tmpPix = pPix->next;
		XtFree((char *)pPix);

		pPix = tmpPix;
	}

	Ol75PercentGrey = (_Ol75PercentGrey *)NULL;
}
