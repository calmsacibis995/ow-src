/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)segment.c	1.41	93/02/18 SMI"


#include "segment_canvas_impl.h"
#include "segment_canvas.h"

#include "atool_panel.h"		/* to get at SECS_GT #define */
#include "undolist_c.h"
#include "atool_i18n.h"
#include "atool_debug.h"

int
void_proc(void)
{
}

ptr_t
SegCanvas_Init(
	ptr_t owner,				/* handle of canvas owner */
	ptr_t canvas_id,			/* xview handle of canvas */
	int (*cursor_position_cb)(), 		/* cursor position changed */
	int (*pointer_position_cb)(), 		/* pointer position changed */
	int (*insert_cb)(),			/* callback for inserts */
	int (*update_select_cb)(),		/* update select call back */
	int (*done_select_cb)())		/* done select call back */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	/* alloc memory for data */
	if ((sp = (struct segment_canvas_data *)calloc(1, sizeof (*sp)))
	    == NULL)
		return (NULL);

	/* save handles */
	sp->segcanvas = canvas_id;
	sp->segowner = owner;

	/* Init the canvas */
	SegCanvas_INIT((ptr_t)sp, canvas_id, owner);

	/* set call backs */
	sp->cursor_position_proc = (cursor_position_cb) ? 
				cursor_position_cb : void_proc;
	sp->pointer_position_proc = (pointer_position_cb) ? 
				pointer_position_cb : void_proc;
	sp->insert_proc = (insert_cb) ? insert_cb : void_proc;
	sp->update_select_proc = (update_select_cb) ? 
				update_select_cb : void_proc;
	sp->done_select_proc = (done_select_cb) ? done_select_cb : void_proc;

	/* return segment data pointer */
	return((ptr_t) sp);
}

/*
 * Update display list with new points by translating the AudioDetect Points 
 * to a new list of display points. Free old display list, and set new one.
 */
void
SegCanvas_Updatelist(ptr_t spd, AudioDetectPts *dpts)
	       		    		/* segment data handle */
	              	      		/* detection points */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	Display_point	*dpi;		/* index to display point list */
	Display_point	*dpl;		/* display point list */
	AudioDetectPts	*dpts_end;	/* end of detect point list */
	AudioDetectPts	*dpts_index;	/* index into detect point list */
	int		dpts_cnt;	/* number of display points */
	int		not_done;	/* loop condition */

	sp = (struct segment_canvas_data *) spd;

	/* if there are no display points, clear current list */
	if (dpts == NULL) {
		free(sp->Pts);
		sp->Pts = NULL;
		return;
	}

	/* get head of display point list */
	dpts_end = dpts;

	/* count eof element */
	dpts_cnt = 1;

	while ((dpts_end)->type != DETECT_EOF) {
		(dpts_end)++;		/* next point */
		dpts_cnt++;
	}

	/* make space for new list */
	dpl = (Display_point *) calloc((unsigned) dpts_cnt, 
	    sizeof (Display_point));
	if (dpl == NULL) {
		return;
	}

	/* free old list, and point to new one */
	if (sp->Pts) free(sp->Pts);
	sp->Pts = dpl;
	sp->Pts_n = dpts_cnt;

	/* 
	 * copy display list to segment point list
	 */
	not_done = TRUE;
	for (dpts_index = dpts, dpi = dpl; (not_done); dpts_index++, dpi++) {
		/* copy time */
		dpi->secs = dpts_index->pos;

		/* copy type */
		switch (dpts_index->type) {
		case DETECT_SOUND:
			dpi->type = SEG_SOUND_START;
			break;
		case DETECT_SILENCE:
			dpi->type = SEG_SILENT_START;
			break;
		case DETECT_EOF:
			dpi->type = SEG_EOF;
			not_done = FALSE;
			break;
		default:
			DBGOUT((1, "Bad display point type\n"));
			break;
		}
		/* check for memory errors */
		if (dpi >= (dpl + dpts_cnt)) {
			DBGOUT((1, "overran seglist memory\n"));
		}
	}
	/* initialize constants */
	SegCanvas_INITCONST(sp);
}

void
SegCanvas_Clearfile(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* switch back to open look pointer */
	cursor_normal(sp);

	/* clear the current display list, free memory */
	SegCanvas_Updatelist(sp, NULL);
	SegCanvas_CLEAR(sp);
}


/*
 * Display the graph.
 */
void
SegCanvas_Displayfile(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int				select;	/* true, if selection exists */
	int				cursor_x;/* true, if cursor is active */
	double				start;	/* start of selection */
	double				end;	/* end of selection */
	Display_point			svptr;	/* saved position pointer */

	sp = (struct segment_canvas_data *) spd;

	/* if there's no display list, return */
	if (sp->Pts == NULL)
		return;

	select = FALSE;

	/* if graph active, save selection, and clear graph before redraw */
	if (sp->Graph_active) {
		/* save selection and position, if any */
		select = SegCanvas_Getselect(sp, &start, &end);
		svptr = sp->Pointer;

		/* clear selection, graph, hash marks, cursor, and pointer */
		SegCanvas_CLEAR(sp);
	} else {
		/* turn on hairline cursor and fake a position point */
		cursor_crosshair(sp);
		svptr.secs = 0.;
		svptr.x = 0;
		svptr.type = SEG_POSITION;
	}

	/* draw the hash marks */
	SegCanvas_HASHDRAW(sp);

	/* draw new graph, turn on cursor and pointer */
	SegCanvas_GRAPHDRAW(sp);

	/* if mouse is over canvas, turn on the vertical cursor */
	if (mouse_overCanvas(sp, &cursor_x))
		cursor_on(sp, cursor_x);

	/* update selection */
	if (select) {
		SegCanvas_Setselect(sp, start, end);
	}
	pointer_on(sp, svptr);
}

/*
 * Return insert position in seconds.
 */
double
SegCanvas_Getinsert(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->S == INSERT) {
		return (sp->Anchor.secs);
	} else
		return (sp->Pointer.secs);
}

/*
 * Return pointer position in seconds.
 */
double
SegCanvas_Getpointer(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* return pointer position converted to time */
	return (sp->Pointer.secs);
}

/*
 * Move pointer to specified position.
 */
void
SegCanvas_Setpointer(ptr_t spd, double pos)
	       		    	/* segment data handle */
	      			    	/* position in seconds */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	Display_point		pt;	/* current position */

	sp = (struct segment_canvas_data *) spd;

	/* save position */
	pt.secs = pos;
	pt.x = SECS_TO_X(sp, pt.secs);
	pt.type = SEG_POSITION;

	/* set pixel position */
	pointer_on(sp, pt);
}

/*
 * clear pointer image, position is unchanged
 */
void
SegCanvas_Clearpointer(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	pointer_off(sp);
}

/*
 * return time position of vertical cursor
 */
double
SegCanvas_Getcursor(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	Display_point		pt;	/* current position */

	sp = (struct segment_canvas_data *) spd;

	return (X_TO_SECS(sp, sp->Cursor_x));
}

/*
 * Set pointer position to one pixel beyond the current request.
 */
void
SegCanvas_Setendpointer(ptr_t spd, double pos)
	       		    	/* segment data handle */
	      			    	/* position in seconds */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	Display_point			pt;	/* position to move to */

	sp = (struct segment_canvas_data *) spd;

	/* save position */
	pt.secs = pos;
	pt.x = SECS_TO_X(sp, pt.secs) + 1;
	pt.type = SEG_POSITION;

	pointer_on(sp, pt);
}

/*
 * Return true if there is a selection
 */
int
SegCanvas_Selection(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	return((sp->S == MAKING_SELECT) || (sp->S == SELECT_EXISTS));
}


/*
 * Set st and end, to start and end of selection.
 * Return FALSE if no selection exists.
 */
int
SegCanvas_Getselect(ptr_t spd, double *st, double *end)
	       		    	/* segment data handle */
	      		    		/* start of selection */
	      		     		/* end of selection */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	switch (sp->S) {
	case NOTHING:
		*st = 0.0;
		*end = 0.0;
		return (FALSE);
	break;
	case INSERT:
		*st = sp->Ext_left.secs;
		*end = *st;
		return (FALSE);
	break;
	case MAKING_SELECT:
	case SELECT_EXISTS:
		*st = sp->Ext_left.secs;
		*end = sp->Ext_right.secs;
		return (TRUE);
	break;
	default:
		return (FALSE);
	}
}

/*
 * Set the selection
 */
void
SegCanvas_Setselect(ptr_t spd, double start, double end)
	       		    	/* segment data handle */
	      		      
	      		    
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	Display_point			pt;	/* position to move to */

	sp = (struct segment_canvas_data *) spd;

	/* clear old selection */
	SegCanvas_Clearselect(sp);

	/* create start point */
	pt.secs = start;
	pt.x = SECS_TO_X(sp, pt.secs);
	pt.type = SEG_POSITION;

	/* set start of selection */
	Insert_set(sp, pt);

	/* set end and display selection */
	pt.secs = end;
	pt.x = SECS_TO_X(sp, pt.secs);
	select_on(sp, pt, FALSE);

	/* become owner of primary AUDIO selection, etc... */
	Select_done(sp);
}



/*
 * Clear the selection image and state, leaving pointer in same position.
 */
void
SegCanvas_Clearselect(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* clear selection image */
	select_clear(sp);

	/* change state to insert */
	SET_STATE(sp, INSERT);
}

SegCanvas_SETINCREMENTTIME(ptr_t spd, double incr)
	       		    	/* segment data handle */
	      		     	/* time display length increment */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* if increment is zero, set to default */
	if (incr <= 0.0) {
		sp->Boxds_time_incr = 30.0;
	} else {
		sp->Boxds_time_incr = incr;
	}
}

/*
 * Turn display of hash marks on or off.
 */
void
SegCanvas_Hashon(ptr_t spd, int value)
	       		    	/* segment data handle */
	   		       /* TRUE/FALSE (on/off) */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* no change */
	if (value == sp->Hash_on)
		return;

	if (value == TRUE) {
		/* turning hash marks on */
		sp->Hash_on = TRUE;
		SegCanvas_HASHDRAW(sp);
	} else {
		/* turning hash marks off */
		SegCanvas_HASHCLEAR(sp);
		sp->Hash_on = FALSE;
	}
}

/*
 * Return true if hairline cursor is active
 */
SegCanvas_Cursoron(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	return(sp->Cursor_active);
}

/*
 * Return owner of segment canvas
 */
ptr_t
SegCanvas_Getowner(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	return (sp->segowner);
}




/*
 *	SegCanvas_Findsound()
 *		Walk through segment point list, and find two segment points
 *	surrounding given position.  Then return the start of next appropriate 
 *	(forward or back) sound segement.  If no sound is found in the current
 *	direction, the start or end of file is returned for the forward and
 *	reverse directions respectively.
 */
double
SegCanvas_Findsound(ptr_t spd, double pos, int forward)
	       		    		/* segment data handle */
	      		    		/* current position in secs */
	   		        	/* if TRUE, search forward in time */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int		fwd;		/* segment point index to forward to*/

	sp = (struct segment_canvas_data *) spd;

	/* return 0 if it's a bad point list */
	if (sp->Pts_n < 2)
		return(0);

	/* find first marker beyond current position */
	fwd = 0;
	while ((fwd < sp->Pts_n - 1) && (!SECS_GT(sp->Pts[fwd].secs, pos)))
		fwd++;

	/* Make sure fwd is a SEG_SOUND_START point.  */
	if (fwd == sp->Pts_n - 1) {
		/*
		 * if last segment point, and last segment is sound...
		 * bump fwd beyond end of array to simulate a start of sound.
		 * (bounds checking is done below).
		 */
		if (sp->Pts[fwd - 1].type == SEG_SOUND_START)
			fwd++;
	} else {
		/* if start of silence, skip to start of sound */
		if (sp->Pts[fwd].type == SEG_SILENT_START)
			fwd++;
	}

	/* 
	 * Return start of sound segment.
	 * Since we have marked the first start of sound beyond the given 
	 * position, we check if the given point is on a start of sound
	 * boundary by looking back two points and comparing.
	 */
	if ((fwd >= 2) && (SECS_EQUAL(sp->Pts[fwd - 2].secs, pos))) {
		/* 
		 * given position is at start of a sound segment.
		 * jump to segment back of this one, or to the fwd position
		 */
		return ((forward) ?
		    sp->Pts[(fwd >= sp->Pts_n) ? sp->Pts_n - 1 : fwd].secs :
		    sp->Pts[(fwd - 4 < 0) ? 0 : fwd - 4].secs); 
	} else {
		/* 
		 * given position is *not* at start of sound segment
		 * jump back to nearest one or to fwd position
		 */
		return ((forward) ? 
		    sp->Pts[(fwd >= sp->Pts_n) ? sp->Pts_n - 1 : fwd].secs :
		    sp->Pts[(fwd - 2 < 0) ? 0 : fwd - 2].secs); 
	}
}
