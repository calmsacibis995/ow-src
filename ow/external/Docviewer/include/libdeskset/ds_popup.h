#ifndef	_ds_popup_h
#define	_ds_popup_h

#ident "@(#)ds_popup.h	1.1 07/08/92 Copyright 1992 Sun Microsystems, Inc."

/*	@(#)ds_popup.h 1.4 IEI SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <xview/xview.h>
#include <xview/panel.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Location ops 
 */
#define DS_POPUP_RIGHT 0	/* Place popup to right of baseframe */
#define DS_POPUP_LEFT 1		/* Place popup to left of baseframe */
#define DS_POPUP_ABOVE 2	/* Place popup above baseframe */
#define DS_POPUP_BELOW 3	/* Place popup below baseframe */
#define DS_POPUP_LOR 4		/* Place popup to right or left of baseframe */
#define DS_POPUP_AOB 5		/* Place popup above or below baseframe */

#if	defined(__STDC__)

extern int	ds_position_popup(Frame		base,
				  Frame		popup,
				  int		location_op);

extern int	ds_position_popup_rect(Rect    *base_rect_p,
				       Frame	popup,
				       int	location_op);

extern int	ds_force_popup_on_screen(int   *popup_x_p,
					 int   *popup_y_p,
					 Frame	base,
					 Frame	popup);

extern int	ds_get_screen_size(Frame	frame,
				   int	       *width_p,
				   int	       *height_p);

extern int	ds_center_items(Panel		panel,
				int		row,
				...);

#else	/* ! defined(__STDC__) */

int	ds_position_popup();
int	ds_position_popup_rect();
int	ds_force_popup_on_screen();
int	ds_get_screen_size();
int	ds_center_items();

#endif	/* defined(__STDC__) */

#ifdef	__cplusplus
}
#endif

#endif /*!_ds_popup_h*/
