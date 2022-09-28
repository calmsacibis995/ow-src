/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)meter_xv.c	1.5	92/10/30 SMI"

#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/xv_i18n.h>
#include <olgx/olgx.h>

#include "meter_impl.h"

static Panel_ops ops = {
    panel_default_handle_event,		/* handle_event() */
    NULL,				/* begin_preview() */
    NULL,				/* update_preview() */
    NULL,				/* cancel_preview() */
    NULL,				/* accept_preview() */
    NULL,				/* accept_menu() */
    NULL,				/* accept_key() */
    meter_clear,			/* clear() */
    meter_paint,			/* paint() */
    NULL,				/* resize() */
    NULL,				/* remove() */
    NULL,				/* restore() */
    meter_layout,			/* layout() */
    NULL,				/* accept_kbd_focus() */
    NULL,				/* yield_kbd_focus() */
    NULL				/* extension: reserved for future use */
};

Xv_pkg          xv_panel_meter_pkg = {
    "Meter Item",
    ATTR_METER,
    sizeof(Xv_panel_meter),
    &xv_panel_item_pkg,
    meter_init,
    meter_set_avlist,
    meter_get_attr,
    meter_destroy,
    NULL			/* no find proc */
};

/* ========================================================================= */

/* -------------------- XView Functions  -------------------- */
/*ARGSUSED*/
Pkg_private int
meter_init(Panel panel_public, Panel_item item_public, Attr_avlist avlist)
{
	Xv_panel_meter *item_object = (Xv_panel_meter *) item_public;
	register Meter_info *dp;
	Display		    *dpy;
	XGCValues	    values;
	XID		    xid;
	Graphics_info 	    *ginfo;
	Pixmap		    pm;
	/* grey stipple pattern for LED's */
	unsigned char grey_bits[] = {
   0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
   0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
   0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55};

	dp = xv_alloc(Meter_info);

	/* link to object */
	item_object->private_data = (Xv_opaque) dp;
	dp->public_self = item_public;
	dp->panel = panel_public;

	dpy = (Display *) XV_DISPLAY_FROM_WINDOW(panel_public);
	xid = (XID) xv_get(panel_public, XV_XID);
	ginfo = (Graphics_info*) xv_get(panel_public, PANEL_GINFO);

	/* init these gc's to black - just to hold 'em */
	values.foreground = BlackPixel(dpy, 0);
	dp->led_gc = XCreateGC(dpy, xid, GCForeground, &values);
	dp->overld_gc = XCreateGC(dpy, xid, GCForeground, &values);

	if (ginfo->depth == 1) {
		pm = XCreatePixmapFromBitmapData(dpy, xid, (char *)grey_bits,
						   16, 16,
						   BlackPixel(dpy, 0),
						   WhitePixel(dpy, 0),
						   1);
		values.fill_style = FillOpaqueStippled;
		values.stipple = pm;
		XChangeGC(dpy, dp->led_gc, (GCFillStyle | GCStipple), 
			  & values);
	}

	/* Initialize non-zero dp fields */
	dp->nleds = DEF_METER_LEDS;
	dp->width = DEF_METER_WIDTH;
	dp->height = DEF_METER_HEIGHT;
	dp->min_value = DEF_METER_MIN;
	dp->max_value = DEF_METER_MAX;
	dp->vertical = DEF_METER_ORIENTATION;
	dp->debug = 0;
	dp->update_only = 0;
	dp->value = DEF_METER_MIN -1;
	dp->last_value = DEF_METER_MIN -1;
	dp->hold = DEF_METER_MIN - 1;
	dp->last_hold = DEF_METER_MIN - 1;
	dp->cutoff = DEF_METER_CUTOFF;
	dp->led_color = -1;
	dp->overld_color = -1;

	xv_set(item_public,
	       PANEL_OPS_VECTOR, &ops,
	       NULL);

	return XV_OK;
}


Pkg_private     Xv_opaque
meter_set_avlist(Panel_item item, register Attr_avlist avlist)
{
    register Meter_info *dp = METER_PRIVATE(item);
    register Attr_attribute attr;
    int		    adjust_values = FALSE;
    int		    end_create = FALSE;
    Xv_opaque	    result;
    int		    size_changed = FALSE;
    int		    leds_set = FALSE;

    /* if a client has called panel_item_parent this item may not */
    /* have a parent -- do nothing in this case */
    if (dp->panel == NULL) {
	return ((Xv_opaque) XV_ERROR);
    }

    if (*avlist != XV_END_CREATE) {
	    /* Call generic item set code to handle layout attributes.
	     * Prevent panel_redisplay_item from being called in 
	     * item_set_avlist.
	     */
	    xv_set(dp->panel, PANEL_NO_REDISPLAY_ITEM, TRUE, 0);
	    result = xv_super_set_avlist(item, &xv_panel_meter_pkg, avlist);
	    xv_set(dp->panel, PANEL_NO_REDISPLAY_ITEM, FALSE, 0);
	    if (result != XV_OK)
		return result;
    }
 
    while (attr = *avlist++) {
	switch (attr) {
	  case PANEL_METER_DEBUG:
	    dp->debug = (int) *avlist++;
	    break;

	  case PANEL_VALUE:
	    dp->last_value = dp->value;
	    dp->value = (int) *avlist++;
	    adjust_values = TRUE;
	    if (dp->debug) {
		    fprintf(stderr,"meter: new value = %d\n", dp->value);
	    }
	    break;

	  case PANEL_MIN_VALUE:
	    dp->min_value = (int) *avlist++;
	    /* re-calc cutoff as 80% of Max */
	    dp->cutoff = (int) ((float)(dp->max_value - dp->min_value) * 0.8);
	    adjust_values = TRUE;
	    size_changed = TRUE;
	    break;

	  case PANEL_MAX_VALUE:
	    dp->max_value = (int) *avlist++;
	    /* re-calc cutoff as 80% of Max */
	    dp->cutoff = (int) ((float)(dp->max_value - dp->min_value) * 0.8);
	    adjust_values = TRUE;
	    size_changed = TRUE;
	    break;

	  case PANEL_DIRECTION:
	    if (*avlist++ == PANEL_VERTICAL)
		dp->vertical = TRUE;
	    else
		dp->vertical = FALSE;
	    size_changed = TRUE;
	    break;
		
	  case PANEL_METER_LEDS:
	    dp->nleds = (int) *avlist++;
	    if (dp->nleds < 1)
		dp->nleds = 1;
	    size_changed = TRUE;
	    leds_set = TRUE;
	    break;

	  case PANEL_METER_WIDTH:
	    dp->width = (int) *avlist++;
	    size_changed = TRUE;
	    break;

	  case PANEL_METER_HEIGHT:
	    dp->height = (int) *avlist++;
	    size_changed = TRUE;
	    break;

	  case PANEL_METER_PEAK:
#ifdef notdef
	    dp->last_peak = dp->peak;
	    dp->peak = (int) *avlist++;
	    adjust_values = TRUE;
#endif
	    break;

	  case PANEL_METER_CUTOFF:
	    /* reset cutoff */
	    dp->cutoff = (int) *avlist++;
	    adjust_values = TRUE;
	    break;

	  case PANEL_METER_SAMPLE_HOLD:
	    /* hold is just current value */
	    dp->last_hold = dp->hold;
	    dp->hold = dp->value;
	    adjust_values = TRUE;
	    break;

	  case PANEL_METER_LED_COLOR:
	    dp->led_color = (int) *avlist++;
	    if (dp->led_color > 0)
		meter_set_color(item, dp->led_color, dp->led_gc);
	    adjust_values = TRUE;
	    break;

	  case PANEL_METER_OVERLD_COLOR:
	    dp->overld_color = (int) *avlist++;
	    if (dp->overld_color > 0)
		meter_set_color(item, dp->overld_color, dp->overld_gc);
	    adjust_values = TRUE;
	    break;

	  case XV_END_CREATE:
	    end_create = TRUE;
	    break;

	  default:
	    /* skip past what we don't care about */
	    avlist = attr_skip(attr, avlist);
	    break;
	}
    }

    /* Set external (client unit) and internal (pixel) values */
    if (size_changed || leds_set || end_create) {
	    /* repaint the whole sucker */
	    update_rects(item);
	    /* meter_clear(item);	/* clear & repaint */
	    dp->update_only = 0;
    } else if (adjust_values) {
	    /* make sure value & hold are sane */
	    if (dp->value < dp->min_value)
		dp->value = dp->min_value;
	    else if (dp->value > dp->max_value)
		dp->value = dp->max_value;
	    if (dp->hold > dp->max_value)
		dp->hold = dp->max_value;

	    /* reset the hold */
	    if ((dp->hold >= dp->min_value) && (dp->value > dp->hold)) {
		    dp->last_hold = dp->hold;
		    dp->hold = dp->value;
	    }

	    dp->update_only = 1;
    }

    return XV_OK;
}

Pkg_private     Xv_opaque
meter_get_attr(Panel_item item_public,
               int *status,
               Attr_attribute which_attr,
               va_list valist)	/*ARGSUSED*/
{
    register Meter_info *dp = METER_PRIVATE(item_public);

    switch (which_attr) {
      case PANEL_VALUE:
	return (Xv_opaque) dp->value;

      case PANEL_METER_CUTOFF:
	return (Xv_opaque) dp->cutoff;

      case PANEL_METER_PEAK:
	return (Xv_opaque) dp->hold;

      case PANEL_MIN_VALUE:
	return (Xv_opaque) dp->min_value;

      case PANEL_MAX_VALUE:
	return (Xv_opaque) dp->max_value;

      case PANEL_DIRECTION:
	return (Xv_opaque) (dp->vertical ? PANEL_VERTICAL : PANEL_HORIZONTAL);

      case PANEL_METER_LEDS:
	return (Xv_opaque) dp->nleds;

      case PANEL_METER_WIDTH:
	return (Xv_opaque) dp->width;

      case PANEL_METER_HEIGHT:
	return (Xv_opaque) dp->height;
	
      default:
	*status = XV_ERROR;
	return (Xv_opaque) 0;
    }
}


Pkg_private int
meter_destroy(Panel_item item_public, Destroy_status status)
{
    Meter_info    *dp = METER_PRIVATE(item_public);

    if ((status == DESTROY_CHECKING) || (status == DESTROY_SAVE_YOURSELF))
	return XV_OK;

    free((char *) dp);

    return XV_OK;
}



/* --------------------  Panel Item Operations  -------------------- */

static void
meter_set_color(Panel_item item, int colind, GC gc)
{
	Meter_info 	*dp = METER_PRIVATE(item);
	Graphics_info 	*ginfo;
	unsigned long	pixval;

	ginfo = (Graphics_info*) xv_get(dp->panel, PANEL_GINFO);

	pixval = xv_get(xv_get(dp->panel, WIN_CMS), CMS_PIXEL, colind);
	XSetForeground((Display*)xv_get(dp->panel, XV_DISPLAY), gc, pixval);
} 

static void
meter_paint(Panel_item item)
{
	meter_update(item);
} 

static void
meter_update(Panel_item item)
{
    Meter_info	   *dp = METER_PRIVATE(item);
    GC             *gc_list;	/* Graphics Context list */
    Panel_paint_window *ppw;	/* ptr to Panel_paint_window structure */
    Xv_Window       pw;		/* Paint Window */
    Graphics_info  *ginfo;
    int		    height;
    int		    led;
    int		    width;
    int		    x;
    Drawable	    xid;
    int		    y;
    int		    inactive;
    int		    is_hold;
    LED_State	    state;
    LED_State	    last_state;
    int		    graphic_state;
    int		    refresh;

    /* XXX */
    refresh = (dp->update_only == 0);
    dp->update_only = 0;	/* reset fo next call */

    if (refresh)
	panel_paint_label(item);

    ginfo = (Graphics_info*) xv_get(dp->panel, PANEL_GINFO);
    inactive = xv_get(item, PANEL_INACTIVE);

    for (ppw = (Panel_paint_window *)
	     xv_get(dp->panel, PANEL_FIRST_PAINT_WINDOW);
	 ppw;
	 ppw = ppw->next) {

	    pw = ppw->pw;
	    xid = (XID) xv_get(pw, XV_XID);

	    width = dp->led_rect.r_width;
	    height = dp->led_rect.r_height;

	    if (dp->vertical) {
		    x = dp->meter_rect.r_left;
	    } else {
		    y = dp->meter_rect.r_top;
	    }

	    for(led=1; led <= dp->nleds; led++) {
		    if (dp->vertical) {
			    y = led_pos(dp, led);
		    } else {
			    x = led_pos(dp, led);
		    }

		    last_state = led_state(dp, led, dp->last_value,
					   dp->last_hold);
		    state = led_state(dp, led, dp->value, dp->hold);

		    if (!refresh && (dp->last_value >= dp->min_value)
			&& (last_state==state))
			continue; /* don't need to repaint this ... */

/* #ifdef notdef */
		    /* XXX - OLGX_ERASE is broken, do a clear area... */
		    if (!refresh) {
			    XClearArea((Display*) 
				       xv_get(dp->panel, XV_DISPLAY), xid,
				       x+2, y+2, width-4, height-4, False);
		    }
/* #endif */
		    /* idea of "invoked" different for mono & color */
		    if (ginfo->depth > 1)
			graphic_state = state ? OLGX_NORMAL : OLGX_INVOKED;
		    else
			graphic_state = OLGX_NORMAL; /* always for mono */

		    graphic_state |= refresh ? 0 : OLGX_ERASE;
		    graphic_state |= inactive ? OLGX_INACTIVE : 0;

		    olgx_draw_box(ginfo, xid,
				  x, y, width, height,
				  graphic_state, 0);

		    /* fill in with colors/stipple */
		    if (state != L_Off)
			XFillRectangle((Display*) 
				       xv_get(dp->panel, 
					      XV_DISPLAY),
				       xid,
				       (state & L_Overld) ? dp->overld_gc
				       : dp->led_gc,
				       x+2, y+2, 
				       width-4, height-4);
	    }
    }
} 


static void
meter_layout(Panel_item item, Rect *deltas)
{
    Meter_info	   *dp = METER_PRIVATE(item);

    dp->meter_rect.r_left += deltas->r_left;
    dp->meter_rect.r_top += deltas->r_top;
    dp->led_rect.r_left += deltas->r_left;
    dp->led_rect.r_top += deltas->r_top;
}


/* we *could* let the default clear/repaint proc do this, but it
 * appears that it's not really doing it's job, so we'll do it
 *
 */
static void
meter_clear(Panel_item item)
{
	Meter_info *dp = METER_PRIVATE(item);

#ifdef notdef
	Display *dpy;
	Drawable xid;
	Xv_Window pw;

	dpy = (Display*) xv_get(item, XV_DISPLAY);

	PANEL_EACH_PAINT_WINDOW(dp->panel, pw)
	    xid = (XID) xv_get(pw, XV_XID);
	    XClearArea(dpy, xid, dp->meter_rect.r_left, dp->meter_rect.r_top,
               dp->meter_rect.r_width, dp->meter_rect.r_height, False);
	PANEL_END_EACH_PAINT_WINDOW

	meter_update(item);
#else
	if (dp->update_only)
	    meter_update(item);
	else
	    panel_default_clear_item(item);
#endif
}

static void
update_rects(Panel_item item)
{
    register Meter_info *dp = METER_PRIVATE(item);
    Rect value_rect;

    value_rect = *(Rect *) xv_get(item, PANEL_ITEM_VALUE_RECT);

    if (dp->vertical) {
	    rect_construct(&dp->meter_rect,
			   value_rect.r_left,
			   value_rect.r_top,
			   dp->width, dp->height);

	    rect_construct(&dp->led_rect,
			   value_rect.r_left,
			   value_rect.r_top,
			   dp->meter_rect.r_width,
			   ((dp->meter_rect.r_height - 
			     (MIN_METER_GAP * dp->nleds)) / dp->nleds));
    } else {
	    rect_construct(&dp->meter_rect,
			   value_rect.r_left,
			   value_rect.r_top,
			   dp->height, dp->width);

	    rect_construct(&dp->led_rect,
			   value_rect.r_left,
			   value_rect.r_top,
			   ((dp->meter_rect.r_width - 
			     (MIN_METER_GAP * dp->nleds)) / dp->nleds),
			   dp->meter_rect.r_height);
    }	    

    value_rect = rect_bounding(&value_rect, &dp->meter_rect);

    /* Note: Setting the value rect will cause the item rect to be
     * recalculated as the enclosing rect containing both the label
     * and value rects.
     */
    xv_set(item,
	   PANEL_ITEM_VALUE_RECT, &value_rect,
	   0);

}
