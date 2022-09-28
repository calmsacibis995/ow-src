/*****************************************************************************
 * Dial.c: The Dial Widget
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

#include <stdio.h>
#include <math.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include "DialP.h"
#include "Dial.h"

#define  RADIANS(x)  (M_PI * 2.0 * (x) / 360.0)
#define  DEGREES(x)  ((x) / (M_PI * 2.0) * 360.0)
#define  MIN_ANGLE   225.0
#define  MAX_ANGLE   270.0
#define  MIN(a,b)    (((a) < (b)) ? (a) :  (b))

static void    select_dial ();
static void    Initialize();
static void    Redisplay();
static void    Resize();
static void    Destroy();
static Boolean SetValues();

static char defaultTranslations[] = "<Btn1Down>: select()";

static XtActionsRec actionsList[] = {
  { "select",   (XtActionProc) select_dial},
};

static XtResource resources[] = {
  {XtNmarkers, XtCMarkers, XtRInt, sizeof (int),
    XtOffset(XsDialWidget, dial.markers), XtRString, "10"  },
  {XtNminimum, XtCMin, XtRInt, sizeof (int),
    XtOffset(XsDialWidget, dial.minimum), XtRString, "0"   },
  {XtNmaximum, XtCMax, XtRInt, sizeof (int),
    XtOffset(XsDialWidget, dial.maximum), XtRString, "100" },
  {XtNindicatorColor, XtCColor, XtRPixel, sizeof (Pixel),
    XtOffset(XsDialWidget, dial.indicator_color), 
    XtRString, "Black"                                     },
  {XtNposition, XtCPosition, XtRPosition, sizeof (Position),
    XtOffset(XsDialWidget, dial.position), XtRString, "0"  },
  {XtNmarkerLength,XtCLength,XtRDimension,sizeof (Dimension),
    XtOffset(XsDialWidget, dial.marker_length),
    XtRString, "5"                                         },
  {XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
    XtOffset(XsDialWidget, dial.foreground), 
    XtRString, "Black"                                     },
  {XtNselectCallback,XtCCallback,XtRCallback,sizeof(XtPointer),
    XtOffset (XsDialWidget, dial.select), 
    XtRCallback, NULL                                      },
 };

XsDialClassRec  XsdialClassRec = {
     /* CoreClassPart */
  {
   (WidgetClass) &widgetClassRec,  /* superclass            */
   "Dial",                         /* class_name            */
   sizeof(XsDialRec),              /* widget_size           */
   NULL,                           /* class_initialize      */
   NULL,                           /* class_part_initialize */
   FALSE,                          /* class_inited          */
   Initialize,                     /* initialize            */
   NULL,                           /* initialize_hook       */
   XtInheritRealize,               /* realize               */
   actionsList,                    /* actions               */
   XtNumber(actionsList),          /* num_actions           */
   resources,                      /* resources             */
   XtNumber(resources),            /* num_resources         */
   NULLQUARK,                      /* xrm_class             */
   TRUE,                           /* compress_motion       */
   XtExposeCompressMaximal,        /* compress_exposure     */
   TRUE,                           /* compress_enterleave   */
   TRUE,                           /* visible_interest      */
   Destroy,                        /* destroy               */
   Resize,                         /* resize                */
   Redisplay,                      /* expose                */
   SetValues,                      /* set_values            */
   NULL,                           /* set_values_hook       */
   XtInheritSetValuesAlmost,       /* set_values_almost     */
   NULL,                           /* get_values_hook       */
   NULL,                           /* accept_focus          */
   XtVersion,                      /* version               */
   NULL,                           /* callback private      */
   defaultTranslations,            /* tm_table              */
   NULL,                           /* query_geometry        */
   NULL,                           /* display_accelerator   */
   NULL,                           /* extension             */
   },
      /* Dial class fields */
  {
   0,                              /* ignore                */
   }
};

WidgetClass XsdialWidgetClass = (WidgetClass) &XsdialClassRec;

static void Initialize (request, new)
  XsDialWidget request, new;
{
  XGCValues values;
  XtGCMask  valueMask;
  /*
   * Make sure the window size is not zero. The Core 
   * Initialize() method doesn't do this.
   */
  if (request->core.width == 0)
    new->core.width = 100;
  if (request->core.height == 0)
    new->core.height = 100;
  /*
   * Make sure the min and max dial settings are valid.
   */
  if (new->dial.minimum >= new->dial.maximum) {
    XtWarning ("Maximum must be greater than the Minimum");
    new->dial.minimum = new->dial.maximum - 1;
  }
  if (new->dial.position > new->dial.maximum) {
    XtWarning ("Position exceeds the Dial Maximum");
    new->dial.position =  new->dial.maximum;
  }
  if (new->dial.position < new->dial.minimum) {
    XtWarning ("Position is less than the Minimum");
    new->dial.position =  new->dial.minimum;
  }
  /*
   * Allow only MAXSEGMENTS / 2 markers
   */
  if(new->dial.markers > MAXSEGMENTS / 2){
    XtWarning ("Too many markers");
    new->dial.markers = MAXSEGMENTS / 2;
  }
  /*
   * Create the graphics contexts used for the dial face 
   * and the indicator.
   */
  valueMask = GCForeground | GCBackground;
  values.foreground = new->dial.foreground;
  values.background = new->core.background_pixel;
  new->dial.dial_GC = XtGetGC ((Widget)new, valueMask, &values);  

  values.foreground = new->dial.indicator_color;
  new->dial.indicator_GC = XtGetGC ((Widget)new, valueMask,&values);  

  valueMask = GCForeground | GCBackground;
  values.foreground = new->core.background_pixel;
  values.background = new->dial.indicator_color;
  new->dial.inverse_GC = XtGetGC ((Widget)new, valueMask, &values);   

  Resize (new);
}

static void Destroy (w)
  XsDialWidget w;
{
  XtReleaseGC ((Widget)w, w->dial.indicator_GC);
  XtReleaseGC ((Widget)w, w->dial.inverse_GC);
  XtReleaseGC ((Widget)w, w->dial.dial_GC);
  XtRemoveAllCallbacks ((Widget)w, XtNselectCallback);
}

static void Resize (w)
  XsDialWidget w;
{
  double    angle, cosine, sine, increment;
  int       i; 
  XSegment *ptr;
  /*
   * Get the address of the first line segment.
   */
  ptr = w->dial.segments;
  /*
   * calculate the center of the widget
   */
  w->dial.center_x = w->core.width/2; 
  w->dial.center_y = w->core.height/2;   
  /* 
   *  Generate the segment array containing the    
   *  face of the dial.    
   */ 
  increment = RADIANS(MAX_ANGLE) /(float)(w->dial.markers -1);
  w->dial.outer_diam = (Position)MIN(w->core.width, w->core.height) / 2;
  w->dial.inner_diam=w->dial.outer_diam-w->dial.marker_length;
  angle = RADIANS(MIN_ANGLE);  

  for (i = 0; i < w->dial.markers;i++){   
    cosine = cos(angle);   
    sine   = sin(angle); 
    ptr->x1 = w->dial.center_x + w->dial.outer_diam * sine; 
    ptr->y1 = w->dial.center_y - w->dial.outer_diam * cosine;
    ptr->x2 = w->dial.center_x + w->dial.inner_diam * sine; 
    ptr->y2 = w->dial.center_y - w->dial.inner_diam * cosine;
    ptr++;
    angle += increment; 
  }  
 calculate_indicator_pos(w); 
} 

static calculate_indicator_pos(w)
  XsDialWidget w;
{
  double   normalized_pos, angle;
  Position indicator_length;
  /*
   * Make the indicator two pixels shorter than the  
   * inner edge of the markers.
   */
  indicator_length=w->dial.outer_diam-w->dial.marker_length-2;
  /*
   * Normalize the indicator position to lie between zero
   * and 1, and then convert it to an angle.
   */
  normalized_pos = (w->dial.position - w->dial.minimum)/
                 (float)(w->dial.maximum - w->dial.minimum);
  angle = RADIANS(MIN_ANGLE + MAX_ANGLE  * normalized_pos);  
   /*
    * Find the x,y coordinates of the tip of the indicator.   
    */ 
   w->dial.indicator_x = w->dial.center_x + 
                               indicator_length * sin(angle); 
   w->dial.indicator_y = w->dial.center_y - 
                               indicator_length  * cos(angle);
} 

static void Redisplay (w, event, region)
  XsDialWidget  w;
  XEvent       *event;
  Region        region;
{
  if(w->core.visible){
    /*
     * Draw the markers used for the dial face.
     */
    XDrawSegments(XtDisplay(w), XtWindow(w),
                  w->dial.dial_GC, 
                  w->dial.segments,
                  w->dial.markers);
    /*
     * Draw the indicator at its current position.
     */
    XDrawLine(XtDisplay(w), XtWindow(w),
              w->dial.indicator_GC, 
              w->dial.center_x, 
              w->dial.center_y,   
              w->dial.indicator_x,  
              w->dial.indicator_y);   
    }
 } 

static Boolean SetValues (current, request, new)
  XsDialWidget current, request, new;
{
  XGCValues  values;
  XtGCMask   valueMask;
  Boolean    redraw = FALSE;
  Boolean    redraw_indicator = FALSE;

  /*
   * Make sure the new dial values are reasonable.
   */
  if (new->dial.minimum >= new->dial.maximum) {
    XtWarning ("Minimum must be less than Maximum");
    new->dial.minimum = 0;
    new->dial.maximum = 100;
  }
  if (new->dial.position > new->dial.maximum) {
    XtWarning("Dial position is greater than the Maximum");
    new->dial.position = new->dial.maximum;
  }
  if (new->dial.position < new->dial.minimum) {
    XtWarning("Dial position is less than the Minimum");
    new->dial.position = new->dial.minimum;
  }
  /*
   * If the indicator color or background color 
   * has changed, generate the GC's.
   */
 if(new->dial.indicator_color!=current->dial.indicator_color||
  new->core.background_pixel !=current->core.background_pixel){
    valueMask = GCForeground | GCBackground;
    values.foreground = new->dial.indicator_color;
    values.background = new->core.background_pixel;
    XtReleaseGC((Widget)new, new->dial.indicator_GC);
    new->dial.indicator_GC = XtGetGC((Widget)new, valueMask,&values);
    values.foreground = new->core.background_pixel;
    values.background = new->dial.indicator_color;
    XtReleaseGC((Widget)new, new->dial.inverse_GC);
    new->dial.inverse_GC = XtGetGC((Widget)new, valueMask, &values);
    redraw_indicator = TRUE;     
  }
  /*
   * If the marker color has changed, generate the GC.
   */
  if (new->dial.foreground != current->dial.foreground){
    valueMask = GCForeground | GCBackground;
    values.foreground = new->dial.foreground;
    values.background = new->core.background_pixel;
    XtReleaseGC((Widget)new, new->dial.dial_GC);
    new->dial.dial_GC = XtGetGC ((Widget)new, valueMask, &values);   
    redraw = TRUE;     
  }
  /*
   * If the indicator position has changed, or if the min/max
   * values have changed, recompute the indicator coordinates.
   */
  if (new->dial.position != current->dial.position ||
      new->dial.minimum != current->dial.minimum ||
      new->dial.maximum != current->dial.maximum){
    calculate_indicator_pos(new);
    redraw_indicator = TRUE;
  }
  /*
   * If only the indicator needs to be redrawn and
   * the widget is realized, erase the current indicator
   * and draw the new one.
   */
  if(redraw_indicator && ! redraw &&
     XtIsRealized((Widget)new) && new->core.visible){
    XDrawLine(XtDisplay(current), XtWindow(current),
              current->dial.inverse_GC, 
              current->dial.center_x, 
              current->dial.center_y,
              current->dial.indicator_x, 
              current->dial.indicator_y);    
    XDrawLine(XtDisplay(new), XtWindow(new),  
              new->dial.indicator_GC,  
              new->dial.center_x, 
              new->dial.center_y,
              new->dial.indicator_x,
              new->dial.indicator_y); 
      } 
  return (redraw); 
} 

static void select_dial (w, event, args, n_args)
  XsDialWidget   w;
  XEvent        *event;
  char          *args[];
  int            n_args;
{
  Position   pos;
  double     angle;
  xsdialCallbackStruct cb;
  
  pos = w->dial.position;
  if(event->type == ButtonPress || 
         event->type == MotionNotify){
    /* 
     * Get the angle in radians.
     */
   angle=atan2((double)(event->xbutton.y - w->dial.center_y),
               (double)(event->xbutton.x - w->dial.center_x));
   /*
    * Convert to degrees from the MIN_ANGLE.
    */ 
   angle = DEGREES(angle) - (MIN_ANGLE - 90.0); 
   if (angle < 0)
     angle = 360.0 + angle;
   /*  
    * Convert the angle to a position. 
    */ 
   pos = w->dial.minimum + (angle / 
             MAX_ANGLE * (w->dial.maximum - w->dial.minimum));
 }  
 /*
   * Invoke the callback, report the position in the call_data
   * structure
   */  
  cb.event    = event;
  cb.position = pos;
  XtCallCallbacks ((Widget)w, XtNselectCallback, &cb); 
} 
