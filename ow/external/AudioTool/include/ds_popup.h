/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ifndef _DS_POPUP_H
#define _DS_POPUP_H

#ident	"@(#)ds_popup.h	1.5	92/11/10 SMI"

#include <xview/xview.h>

/*
 * Location ops 
 */
#define DS_POPUP_RIGHT 0	/* Place popup to right of baseframe */
#define DS_POPUP_LEFT 1		/* Place popup to left of baseframe */
#define DS_POPUP_ABOVE 2	/* Place popup above baseframe */
#define DS_POPUP_BELOW 3	/* Place popup below baseframe */
#define DS_POPUP_LOR 4		/* Place popup to right or left of baseframe */
#define DS_POPUP_AOB 5		/* Place popup above or below baseframe */

#ifdef __cplusplus
extern "C" {
#endif
extern int	ds_position_popup(Frame, Frame, int);
extern int	ds_position_popup_rect(Rect*, Frame, int);
extern int	ds_force_popup_on_screen(int*, int*, Frame, Frame);
extern int	ds_get_screen_size(Frame, int*, int*);
extern int	ds_center_items(Panel, int, ...);
#ifdef __cplusplus
}
#endif

#endif /* !_DS_POPUP_H */
