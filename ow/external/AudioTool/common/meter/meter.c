/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)meter.c	1.5	92/10/23 SMI"

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

#include "meter_impl.h"

/* --------------------  Local Routines  -------------------- */

/*
 * Convert external value (client units) to internal value (pixels).
 */
static int
scale_value(Meter_info *dp, int value)
{
    if (value <= dp->min_value)
	return (0);

    if (value >= dp->max_value)
	return (dp->nleds);

    return ((value * dp->nleds) / (dp->max_value - dp->min_value));
}

/* is this LED on or off? */
LED_State
led_state(Meter_info *dp, int led, int value, int hold)
{
	int val, hval, cval;
	LED_State state;

	val = scale_value(dp, value);
	hval = scale_value(dp, hold);
	cval = scale_value(dp, dp->cutoff);

	if ((dp->hold >= dp->min_value) && (hval == led)) {
		state = (L_On | L_Hold);
		state |= (hval >= cval) ? L_Overld : 0;
	} else if (led <= val) {
		state = L_On;
		state |= (led >= cval) ? L_Overld : 0;
	} else {
		state  = L_Off;
	}

	return state; 
}

/* return start pixel of this led (checks orientation) */
int
led_pos(Meter_info *dp, int led)
{
	if (led <= 0)
	    return 0;
	if (led > dp->nleds)
	    led = dp->nleds;

	if (dp->vertical) {
		return ((dp->meter_rect.r_top + dp->meter_rect.r_height) -
			((led * dp->led_rect.r_height) + 
			((led-1) * MIN_METER_GAP)));
	} else {
		return (dp->meter_rect.r_left +
			((dp->led_rect.r_width * (led-1)) +
			 (MIN_METER_GAP * (led-2))));
	}
}

