/*****************************************************************************
 * Row.c: Methods for the Row widget
 *
 *         From:
 *                   The X Window System, 
 *            Programming and Applications with Xt
 *                   OPEN LOOK Edition
 *         by
 *              Douglas Young & John Pew
 *              Prentice Hall, 1993
 *
 *              Example described on pages: 
 *
 *
 *  Copyright 1993 by Prentice Hall
 *  All Rights Reserved
 *
 * This code is based on the OPEN LOOK Intrinsics Toolkit (OLIT) and 
 * the X Window System
 *
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation.
 *
 * Prentice Hall and the authors disclaim all warranties with regard to 
 * this software, including all implied warranties of merchantability and 
 * fitness.
 * In no event shall Prentice Hall or the authors be liable for any special,
 * indirect or consequential damages or any damages whatsoever resulting from 
 * loss of use, data or profits, whether in an action of contract, negligence 
 * or other tortious action, arising out of or in connection with the use 
 * or performance of this software.
 *
 * OPEN LOOK is a trademark of UNIX System Laboratories.
 * X Window System is a trademark of the Massachusetts Institute of Technology
 ****************************************************************************/

#include    <X11/IntrinsicP.h>
#include    <X11/Intrinsic.h>
#include    <X11/Composite.h>
#include    <X11/CompositeP.h>
#include    "RowP.h"
#include    "Row.h"
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

static void              Initialize();
static void              Resize();
static void              ChangeManaged();
static Boolean           SetValues();
static XtGeometryResult  GeometryManager();
static XtGeometryResult  PreferredSize();
static XtGeometryResult  try_layout();

XsRowClassRec XsrowClassRec = {
  {
    /* core_class members      */
    (WidgetClass) &compositeClassRec, /* superclass         */
    "Row",                            /* class_name         */
    sizeof(XsRowRec),                 /* widget_size        */
    NULL,                             /* class_initialize   */
    NULL,                             /* class_part_init    */  
    FALSE,                            /* class_inited       */  
    Initialize,                       /* initialize         */
    NULL,                             /* initialize_hook    */  
    XtInheritRealize,                 /* realize            */
    NULL,                             /* actions            */
    0,                                /* num_actions        */  
    NULL,                             /* resources          */
    0,                                /* num_resources      */
    NULLQUARK,                        /* xrm_class          */
    TRUE,                             /* compress_motion    */  
    XtExposeCompressMaximal,          /* compress_exposure  */  
    TRUE,                             /* compress_enterleave*/  
    FALSE,                            /* visible_interest   */
    NULL,                             /* destroy            */
    Resize,                           /* resize             */
    NULL,                             /* expose             */
    NULL,                             /* set_values         */
    NULL,                             /* set_values_hook    */
    XtInheritSetValuesAlmost,         /* set_values_almost  */
    NULL,                             /* get_values_hook    */  
    NULL,                             /* accept_focus       */
    XtVersion,                        /* version            */  
    NULL,                             /* callback_private   */
    NULL,                             /* tm_table           */
    PreferredSize,                    /* query_geometry     */  
    NULL,                             /* display_accelerator*/
    NULL,                             /* extension          */
  },
  {
    /* composite_class members */
    GeometryManager,                  /* geometry_manager   */
    ChangeManaged,                    /* change_managed     */
    XtInheritInsertChild,             /* insert_child       */  
    XtInheritDeleteChild,             /* delete_child       */  
    NULL,                             /* extension          */
  },
  {
    /* Row class members */
    0,                                /* empty              */  
  }
};
WidgetClass XsrowWidgetClass = (WidgetClass) &XsrowClassRec;

static void Initialize(request, new)
  XsRowWidget request, new;
{
  if (request -> core.width <= 0)
    new -> core.width = 5;
  if (request -> core.height <= 0)
    new -> core.height = 5;
} 

static void Resize(w)
  XsRowWidget    w;
{
  do_layout(w);
} 

do_layout(parent)
  XsRowWidget parent;
{
  Widget     child;
  int        i;
  Dimension  childwidth = 0;
  Position   xpos = 0;
  Dimension  pad = 0;
  int    n_managed_children = 0;

  /*
   * Compute the total width of all managed children and
   * determine how many children are managed.
   */
  for (i = 0; i < parent -> composite.num_children; i++) {
    child = parent -> composite.children[i];
    if(child->core.managed) {
      n_managed_children++;
      childwidth += child->core.width + child->core.border_width * 2;
    }
 }
  /*
   *  Divide any remaining space by the number 
   *  of children.
   */
  if((n_managed_children > 1) && 
          (parent->core.width > childwidth))
    pad = (int)(parent->core.width - childwidth) /
                                  (n_managed_children - 1);
  /*
   * Position all children.
   */
  for (i = 0; i < parent -> composite.num_children; i++) {
    child = parent -> composite.children[i];
    if(child->core.managed) {
       XtMoveWidget (child, xpos, 0);
       xpos += pad + child->core.width + 
                             child->core.border_width * 2;
     }
  }
}
static XtGeometryResult PreferredSize(w, request, preferred)
  XsRowWidget w;
  XtWidgetGeometry *request, *preferred;
{
  Widget child;
  int i;

  /*
   * If no changes are being made to width or height, just agree. 
   */
  if(!(request->request_mode & CWWidth) &&
     !(request->request_mode & CWHeight))
    return (XtGeometryYes);
  /*
   * Calculate our minimum size.
   */
  preferred->width = 0;
  preferred->height = 0;       
  for (i = 0; i < w -> composite.num_children; i++) {
    child = w -> composite.children[i];
    if(child->core.managed) {
       preferred->width += child->core.width + 
                                child->core.border_width * 2;
       if(preferred->height < (Dimension)(child->core.height + 
               child->core.border_width * 2))
         preferred->height = child->core.height +
                                child->core.border_width * 2;
     }
  }
  preferred->request_mode = CWWidth | CWHeight;
  /* 
   * If both width and height are requested.
   */
  if((request->request_mode & CWWidth) && 
     (request->request_mode & CWHeight)) {
    /* 
     * If we are to be the same or bigger, say ok.
     */
    if(preferred->width <= request->width &&            /* Error in first printing */
            preferred->height <= request->height) {      /* Signs were reversed     */
      preferred->width  = request->width;
      preferred->height = request->height;
      return (XtGeometryYes);
    }
    /*
     * If both dimensions are too small, say no.
     */
    else
      if(preferred->width > request->width && 
         preferred->height > request->height)
        return (XtGeometryNo);
    /*
     * Otherwise one must be right, so say almost.
     */
      else
        return (XtGeometryAlmost);
  }
  /*
   * If only the width is requested, either it's 
   * OK or it isn't. Same for height.
   */
  else
    if(request->request_mode & CWWidth) {
      if(preferred->width <= request->width) {
         preferred->width = request->width;
         return (XtGeometryYes);
      } 
      else
                return (XtGeometryNo);
    }
    else if(request->request_mode & CWHeight) {
      if(preferred->height <= request->height) {
                preferred->height = request->height;
                return (XtGeometryYes);
      }
      else
                return (XtGeometryNo);
    }
    return (XtGeometryYes);
}

static XtGeometryResult GeometryManager(w, request, reply)
  Widget                            w;
  XtWidgetGeometry        *request;
  XtWidgetGeometry        *reply;
{
  XsRowWidget      rw = (XsRowWidget) w -> core.parent;
  Mask             mask;
  XtGeometryResult result;
  Dimension        wdelta, hdelta;

  /*
   * If the widget wants to move, just say no.
   */
  if ((request->request_mode & CWX && request->x != w->core.x) ||
      (request->request_mode & CWY && request->y != w->core.y))
    return (XtGeometryNo);
  /*
   *  Otherwise, grant all requests if they fit.
   */
  if (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)) {
    /*
     * Save the original widget size, and set the 
     * corresponding widget fields to the requested sizes.
     */
    Dimension savewidth       = w->core.width;
    Dimension saveheight      = w->core.height;
    Dimension saveborderwidth = w->core.border_width;

    if (request->request_mode & CWWidth)
      w->core.width  = request->width;
    if (request->request_mode & CWHeight)
      w->core.height = request->height;
    if (request->request_mode & CWBorderWidth)
      w->core.border_width = request->border_width;
    /*
     * See if we can still handle all the children 
     * if the request is granted.
     */
    result = try_layout(rw, &mask, &wdelta, &hdelta);
    /*
     * If the children won't fit, restore the widget to its
     * original size, and return no.
     */
    if(result == XtGeometryNo) {
      w->core.width  = savewidth;
      w->core.height = saveheight;
      w->core.border_width = saveborderwidth;
      return (XtGeometryNo);
    }
    /*
     * If only one dimension fits, restore the one that
     * doesn't fit and return "almost".
     */
    if(result == XtGeometryAlmost) {    
      reply->request_mode = request->request_mode;
      if(!(mask & CWWidth)) {
      reply->width = w->core.width = savewidth;
       reply->border_width  = saveborderwidth;
       w->core.border_width = saveborderwidth;
      }
      if(!(mask & CWHeight))
        reply->height = w->core.height = saveheight;
     
      return (XtGeometryAlmost);  
    }
    /*
     *  If we got here, everything must fit, so reposition
     *  all children based on the new size, and return "yes".
     */
    do_layout(rw);
    return (XtGeometryYes); 
  }
  return (XtGeometryYes);
}

static XtGeometryResult 
try_layout(parent, mask, w_delta, h_delta)
  XsRowWidget parent;
  Mask       *mask;
  Dimension  *w_delta, *h_delta;
{
  int  i;
  Dimension total_width = 0, max_height = 0;

  /*
   * Get the bounding width and height of all children.
   */
  for (i = 0; i < parent -> composite.num_children; i++) {
   Widget    child;
   Dimension width, height;

  child  = parent -> composite.children[i];
  if(child->core.managed) {
    height = child->core.height + child->core.border_width * 2;
    width  = child->core.width + child->core.border_width * 2;
    total_width += width;
    max_height = MAX(max_height, height);
  }
 }
 /*
  *  If everyone doesn't fit, ask if we can grow. Return the 
  *  result, after setting the mask to indicate which (if 
  *  any) dimension is ok.
  */
 if(total_width > parent->core.width || 
     max_height > parent->core.height) {
   XtGeometryResult result;
   Dimension replyWidth, replyHeight;
   Dimension width  =  MAX(total_width, parent->core.width);
   Dimension height = MAX(max_height, parent->core.height);

   result = XtMakeResizeRequest ((Widget)parent, width, height,
                                 &replyWidth, &replyHeight);
   *mask = NULL;
   if(total_width == replyWidth)
     *mask  = CWWidth;
   if(max_height == replyHeight)        
    *mask |= CWHeight;

   if(result == XtGeometryAlmost)
     XtMakeResizeRequest ((Widget)parent, replyWidth, replyHeight, NULL, NULL);
   *w_delta = total_width - parent->core.width;
   *h_delta = max_height - parent->core.height;
   return (result);
 }
 /*
  * If everybody fits, just return yes.
  */
 *mask = CWWidth | CWHeight;
 return (XtGeometryYes);
}

static void ChangeManaged(w)
  XsRowWidget w; 
{
  XtGeometryResult result;
  Dimension        width, height, delta;
  int              i;
  Mask             mask;
  Widget           child;

  /*
   * See if all children fit.
   */
  result = try_layout(w, &mask, &width, &height);
  /*
   * If they don't, resize all children to be smaller.
   */
  if(result != XtGeometryYes) {
    if(w->composite.num_children > 0) {
     delta = width / w->composite.num_children;
     for(i=0;i<w->composite.num_children;i++) {
        child = w->composite.children[i];
       height = MIN(child->core.height,
                    (Dimension)(w->core.height -child->core.border_width));
        if(child->core.managed)
                 XtResizeWidget(child,
                                child->core.width - delta,
                                height,
                                child->core.border_width);
     }
   }
  }
  /*
   * Move all children to their new positions.
   */
  do_layout(w);
}
