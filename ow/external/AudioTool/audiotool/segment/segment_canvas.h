/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_SEGMENT_CANVAS_H
#define	_MULTIMEDIA_SEGMENT_CANVAS_H

#ident	"@(#)segment_canvas.h	1.10	91/06/01 SMI"

#include <atool_types.h>

ptr_t	SegCanvas_Init(ptr_t owner,
       	               ptr_t canvas_id,
       	               int (*cursor_position_cb) (/*XXX*/),
		       int (*pointer_position_cb) (/*XXX*/),
		       int (*insert_cb) (/*XXX*/),
		       int (*update_select_cb) (/*XXX*/),
		       int (*done_select_cb) (/*XXX*/));
ptr_t SegCanvas_Getowner(ptr_t spd);

void	SegCanvas_Displayfile(ptr_t spd);
void	SegCanvas_Clearfile(ptr_t spd);
void	SegCanvas_Updatefile();

double	SegCanvas_Getinsert(ptr_t spd);
double	SegCanvas_Getpointer(ptr_t spd);
void	SegCanvas_Setpointer(ptr_t spd, double pos);
void	SegCanvas_Clearpointer(ptr_t spd);

int	SegCanvas_Getselect(ptr_t spd, double *st, double *end);
void	SegCanvas_Setselect(ptr_t spd, double start, double end);
int	SegCanvas_Selection(ptr_t spd);
void	SegCanvas_Clearselect(ptr_t spd);

void	SegCanvas_Hashmarkson();
void	SegCanvas_Defaultsize();

double	SegCanvas_Getcursor(ptr_t spd);
double	SegCanvas_Findsound(ptr_t spd, double pos, int forward);

#endif /* !_MULTIMEDIA_SEGMENT_CANVAS_H */
