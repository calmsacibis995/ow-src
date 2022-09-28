#pragma ident	"@(#)Margin.c	302.3	97/03/26 lib/libXol SMI"	/* textedit:Margin.c 1.6	*/

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


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Dynamic.h>
#include <Xol/Margin.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextEdit.h>
#include <Xol/TextEditP.h>
#include <Xol/Util.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>


#define ITEMHT(ctx)             (ctx-> textedit.lineHeight)
#define FONT(ctx)               ((XFontStruct *)ctx-> primitive.font)
#define NORMALGC(ctx)           ctx-> textedit.gc


static int     LocateItem(TextEditWidget ctx, int y);


/*
 * _OlRegisterTextLineNumberMargin
 *
 */
extern void
_OlRegisterTextLineNumberMargin(TextEditWidget ctx)
{

XtAddCallback((Widget)ctx, XtNmargin,
	      _OlDisplayTextEditLineNumberMargin, NULL);

if (XtIsRealized((Widget)ctx))
   {
   XClearArea(XtDisplay(ctx), XtWindow(ctx), 
       0, PAGE_T_MARGIN(ctx), PAGE_L_MARGIN(ctx), PAGEHT(ctx), False);

   _OlDisplayTextEditLineNumberMargin((Widget)ctx, NULL, NULL);
   }

} /* end of _OlRegisterTextLineNumberMargin */
/*
 * _OlUnregisterTextLineNumberMargin
 *
 */

extern void
_OlUnregisterTextLineNumberMargin(TextEditWidget ctx)
{

if (XtIsRealized((Widget)ctx))
   XClearArea(XtDisplay(ctx), XtWindow(ctx), 
       0, PAGE_T_MARGIN(ctx), PAGE_L_MARGIN(ctx), PAGEHT(ctx), False);

XtRemoveCallback((Widget)ctx, XtNmargin,
		 _OlDisplayTextEditLineNumberMargin, NULL);

} /* end of _OLUnregisterTextLineNumberMargin */
/*
 * _OlDisplayTextEditLineNumberMargin
 *
 * Note: This is defined as an extern procedure to allow users to
 *       simply add is as a callback without using the provided
 *       Register/Unregister functions.
 */

extern void
_OlDisplayTextEditLineNumberMargin(Widget w,
				   XtPointer client_data,
				   XtPointer call_data)
{
TextEditWidget         ctx          = (TextEditWidget)w;
Display *              dpy          = XtDisplay(ctx);
Window                 win          = XtWindow(ctx);
XFontStruct *          font         = FONT(ctx);
GC                     gc           = NORMALGC(ctx);
int                    fontht       = ITEMHT(ctx);
int                    ascent       = FONT(ctx)->ascent;
int                    lmargin      = PAGE_L_MARGIN(ctx) - FONTWID(ctx);
DisplayTable *         DT           = ctx-> textedit.DT;
OlTextMarginCallData * cd           = (OlTextMarginCallData *) call_data;
XRectangle *           cd_rect      = cd-> rect;
XRectangle             rect;
int                    start;
int                    end;
int                    len;
int                    x;
int                    y;
int                    i;
char                   *buffer;

if (cd_rect &&
   (cd_rect-> x > PAGE_L_MARGIN(ctx) || 
    cd_rect-> y + (Position)cd_rect-> height < PAGE_T_MARGIN(ctx) || 
    cd_rect-> y > PAGE_B_MARGIN(ctx)))
   return;

	if (!(buffer = malloc(20)))
		return;
if (cd_rect == NULL)
   {
   start = 0;
   end   = LINES_VISIBLE(ctx) - 1;
   }
else
   {
   y = cd_rect-> y + cd_rect-> height;
   start = LocateItem(ctx, MAX(cd_rect-> y, PAGE_T_MARGIN(ctx)));
   end   = LocateItem(ctx, MIN(y, PAGE_B_MARGIN(ctx)));
   }

rect.x      = 0;
rect.width  = PAGE_L_MARGIN(ctx);
rect.y      = PAGE_T_MARGIN(ctx) + fontht * start;
rect.height = PAGE_T_MARGIN(ctx) + fontht * end;

XClearArea(dpy, win, rect.x, rect.y, rect.width, rect.height, False);

for (i = start, y = PAGE_T_MARGIN(ctx) + (i * fontht) + ascent; 
     i <= end; i++, y += fontht)
   {
   if (DT[i].p == NULL)
      break;
   else
      if (DT[i].wraploc.offset == 0)
         {
         snprintf(buffer, 20, "%d", DT[i].wraploc.line + 1);
	 len = strlen(buffer);
         x   = lmargin - XTextWidth(font, buffer, len);
         XDrawImageString(dpy, win, gc, x, y, buffer, len);
         }
   }
	free(buffer);

} /* end of _OlDisplayTextEditLineNumberMargin */
/*
 * LocateItem
 *
 */

static int
LocateItem(TextEditWidget ctx, int y)
{
int i;

if (y < PAGE_T_MARGIN(ctx))
   i = 0;
else
   if (y > PAGE_B_MARGIN(ctx))
      i = LINES_VISIBLE(ctx) - 1;
   else
      i = (y - PAGE_T_MARGIN(ctx)) / ITEMHT(ctx);

return (i);

} /* end of LocateItem */
