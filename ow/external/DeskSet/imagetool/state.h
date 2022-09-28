
/*
 * @(#)state.h 1.9 94/03/14
 *
 * Copyright (c) 1992 - Sun Microsystems Inc.
 *
 */

#ifndef STATE_H
#define STATE_H

#include <xview/xview.h>
#include <math.h>

#define PI            M_PI
#define RADIANS(a)    ((a * PI) / 180.0 )
#define DEGREES(a)    ((a * 180.0) / PI)


typedef enum {
	HFLIP,
	VFLIP,
	ROTATE_R,
	ROTATE_L,
	ZOOM,
	NO_UNDO
} UndoTypes;

typedef struct {
	UndoTypes	 op;		/* Last operation peformed	*/
	float		 zoom_amt;	/* Old zoom value		*/
} UndoInfo;
	
typedef struct {
	int		 top_x;
	int		 top_y;
	int		 new_x;
	int		 new_y;
	Rect		 rect;
} SelectedRect;

typedef struct {
	float		 zoom;		 /* Total zoom factor		 */
	float		 zoom_amt;	 /* Current zoom factor		 */
	int		 rotate_amt;	 /* Amount to rotate (relative)  */
	int		 angle;		 /* Current angle of rotation	 */
	int		 frontside;	 /* Front side or back side      */
	int		 hflip;		 /* Flip image horizontally	 */
	int		 vflip;		 /* Flip image vertically	 */
	UndoInfo	 undo;		 /* Last operation info		 */
	int		 pan;		 /* Panning chosen		 */
	int		 select;	 /* Select chosen		 */
	int		 image_selected; /* Portion has been selected    */
	int              set_roi;	 /* Set ROI on display win       */
	Rect		 sel_rect;	 /* Rect of selected portion     */
	SelectedRect	 save_rect;	 /* Rect of old selected portion */
	int		 old_x;		 /* Old x (where origin was)     */
	int		 old_y;		 /* Old y (where origin was)     */
	int		 currentx;	 /* Current x where origin is	 */
	int		 currenty;	 /* Current y where origin is	 */
	int		 current_page;	 /* Current page displayed	 */
	int		 next_page;	 /* Page number on pixmap2 	 */
	int		 reversed;	 /* Reverse pages of ps doc	 */
	int		 using_dsc;	 /* True if using dsc for ps     */
	int		 timeout_hit;	 /* True if timeout occured      */

	/* xform is an affine matrix representing current geometry */
	float		 xform[6];

	/* old_xform is the affine matrix for undo */
	float		 old_xform[6];
} StateInfo;

extern	StateInfo       *current_state;

/* Function prototypes */
extern	StateInfo	*init_state ();

#endif
