#ifndef lint
static 	char sccsid[] = "@(#)ds_dblclick.c 3.1 92/04/03 Copyr 1991 Sun Micro";
#endif

#include <xview/xview.h>

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/******************************************************************************
*
*	Function:	ds_is_double_click
*
*	Description:	Returns TRUE if the time and distance between
*			2 events are close enough to constitute a double
*			click.  The following X resources are used to
*			determine the thresholds for time and distances:
*
*			OpenWindows.MultiClickTimeout 
*			OpenWindows.DragThreshold
*
*			This code was taken from the following document
*			and slightly modified:
*
*		"Detecting and handling mouse double click with XView"
*			Jim Becker -- Feb 8, 1991
*
*
*	Parameters:	last_event	Previous event
*			event		Current event
*
*	Returns:	TRUE	Double click
*			FALSE	Not a double click
*
******************************************************************************/
int
ds_is_double_click(last_event, event )
Event		*last_event;
Event		*event;
{
	static	int		time_threshold;
	static	int		dist_threshold;
	static	short		first_time	= TRUE;
	short			ret_value	= FALSE;
	int			delta_time;
	int			delta_x, delta_y;

	if (last_event == NULL || event == NULL)
		return ret_value;

	/* first time this is called init the thresholds */
	if( first_time ) {
		/* Get time threshold in miliseconds */
		time_threshold	= 100 *
			defaults_get_integer("OpenWindows.MultiClickTimeout",
					     "OpenWindows.MultiClickTimeout",
					     4);
		dist_threshold	=
			defaults_get_integer("OpenWindows.DragThreshold",
					     "OpenWindows.DragThreshold", 4);

		first_time	= FALSE;
	}

	/* only deal with the down events */
	if( event_is_up(event) )
		return ret_value;

	if( event_action(event)       == ACTION_SELECT &&
	    event_action(last_event) == ACTION_SELECT ) {

		delta_time    	 = (event->ie_time.tv_sec -
				    last_event->ie_time.tv_sec) * 1000;
		delta_time	+= event->ie_time.tv_usec / 1000;
		delta_time 	-= last_event->ie_time.tv_usec / 1000;

		/* is the time within bounds? */
		if( delta_time <= time_threshold ) {

			/* check to see if the distance is ok */
			delta_x	= (last_event->ie_locx > event->ie_locx ?
				   last_event->ie_locx - event->ie_locx :
				   event->ie_locx - last_event->ie_locx);

			delta_y	= (last_event->ie_locy > event->ie_locy ?
				   last_event->ie_locy - event->ie_locy :
				   event->ie_locy - last_event->ie_locy);

			if( delta_x <= dist_threshold &&
			    delta_y <= dist_threshold )
				ret_value	= TRUE;
		}
	}
					
	return ret_value;
}
