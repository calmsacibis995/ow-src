/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_METER_IMPL_H
#define	_MULTIMEDIA_METER_IMPL_H

#ident	"@(#)meter_impl.h	1.2	91/06/14 SMI"

#include <xview/frame.h>
#include <xview/panel.h>

#include "meter.h"

typedef enum { 
	L_Off = 0, L_On = 1, L_Overld = 2, L_Hold = 4, L_Peak = 8,
} LED_State;

#define METER_PRIVATE(item)	\
	XV_PRIVATE(Meter_info, Xv_panel_meter, item)
#define METER_PUBLIC(item)	XV_PUBLIC(item)

#define MIN_METER_GAP	2	/* minimum gap between meter LED's */

#define DEF_METER_HEIGHT 100    /* default height in pixels */
#define DEF_METER_WIDTH  20	/* default height in pixels */
#define DEF_METER_LEDS  10	/* default # LED's */
#define DEF_METER_MAX	 100	/* default max val */
#define DEF_METER_MIN	 1	/* default min val */
#define DEF_METER_CUTOFF 80	/* default cutoff */
#define DEF_METER_ORIENTATION 1; /* vertical */

typedef struct {	/* data for a meter */
    Panel_item      public_self;/* back pointer to object */

    Panel	    panel;	/* Panel this item is owned by */

    Rect            meter_rect;	/* rect containing meter */
    Rect            led_rect;	/* rect containing LED */

    int             max_value;
    int             min_value;
    int		    nleds;	/* nbr of led marks on meter */
    int		    width;
    int		    height;

    int		    value;	/* value of meter in client units
				 * ("external") */
    int		    last_value; /* to repaint only the LEDs that change */
    int		    hold;	/* peak value of meter in client units
				 * ("external") */
    int		    last_hold;

    int		    cutoff;	/* cutoff value for red LED's */

    int		    led_color;	/* color index for normal (on) led */
    int		    overld_color; /* color index for peak/sample (on) led */
    GC		    led_gc;	/* gc's for painting these things */
    GC		    overld_gc;

    /* flags */
    unsigned int vertical:1;
    unsigned int update_only:1;	/* XXX - next repaint is an update only. */
    unsigned int debug:1;	/* debugging output ... */
} Meter_info;

/* XView functions */
Pkg_private int meter_init(Panel panel_public,
                           Panel_item item_public,
                           Attr_avlist avlist);
Pkg_private Xv_opaque meter_set_avlist(Panel_item item,
                                       register Attr_avlist avlist);
Pkg_private Xv_opaque meter_get_attr(Panel_item item_public,
                                     int *status,
                                     Attr_attribute which_attr,
                                     va_list valist);
Pkg_private int meter_destroy(Panel_item item_public, Destroy_status status);

/* Panel Item Operations */
Pkg_private void     meter_paint(Panel_item item);
Pkg_private void     meter_update(Panel_item item);
Pkg_private void     meter_clear(Panel_item item);
Pkg_private void	meter_layout(Panel_item item, Rect *deltas);
Pkg_private void	meter_set_color(Panel_item item, int colind, GC gc);

/* Local functions */
Pkg_private void     update_rects(Panel_item item);
Pkg_private LED_State led_state(Meter_info *, int, int, int);

#endif /* !_MULTIMEDIA_METER_IMPL_H */
