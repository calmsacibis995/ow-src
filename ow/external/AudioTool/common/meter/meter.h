/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_METER_H
#define	_MULTIMEDIA_METER_H

#ident	"@(#)meter.h	1.4	96/02/20 SMI"

#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)meter.h 1.1 91/02/28";
#endif
#endif

#include <xview/panel.h>

typedef Xv_panel_extension_item Xv_panel_meter;

typedef Panel_item Panel_meter;

extern Xv_pkg	    xv_panel_meter_pkg;

#define PANEL_METER_ITEM    &xv_panel_meter_pkg

/* Pick an Attribute ID between ATTR_PKG_UNUSED_FIRST and
 * ATTR_PKG_UNUSED_LAST.  The Attribute ID need only be
 * unique within the heirarchy for this object.
 */
#define ATTR_METER ATTR_PKG_UNUSED_FIRST

#define METER_ATTR(type, ordinal)	ATTR(ATTR_METER, type, ordinal)

typedef enum {
    PANEL_METER_LEDS	= METER_ATTR(ATTR_INT,		1),
    PANEL_METER_HEIGHT	= METER_ATTR(ATTR_INT,		2),
    PANEL_METER_WIDTH	= METER_ATTR(ATTR_INT,		3),
    PANEL_METER_PEAK	= METER_ATTR(ATTR_INT,		4),
    PANEL_METER_SAMPLE_HOLD = METER_ATTR(ATTR_INT,	5),
    PANEL_METER_LED_COLOR = METER_ATTR(ATTR_INT,	6),
    PANEL_METER_OVERLD_COLOR = METER_ATTR(ATTR_INT,	7),
    PANEL_METER_CUTOFF = METER_ATTR(ATTR_INT,		8),
    PANEL_METER_DEBUG = METER_ATTR(ATTR_BOOLEAN,	9)
} Meter_attr;

#endif /* !_MULTIMEDIA_METER_H */
