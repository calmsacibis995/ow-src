/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)holdbutton.c	1.5	91/07/18 SMI"

#include <xview/xview.h>
#include <xview/panel.h>
#include "holdbutton.h"

void			set_xvtimer();

/*
 * Test if button is still held down
 */
int
Button_helddown(
	void*		bdp)
{
	Button_state*	bp;

	bp = (Button_state*) bdp;

	return (bp->held_down == TRUE);
}


/*
 * Get xview handle to button
 */
Xv_opaque
Button_xvid(
	void*		bdp)
{
	Button_state*	bp;

	bp = (Button_state*) bdp;

	return ((Xv_opaque) bp->xvid);
}

/*
 * Get button data structure stored with Xview handle
 */
void*
Button_data(
	Xv_opaque		xvid)
{
	return ((void*) xv_get(xvid, XV_KEY_DATA, _BUTTON_KEY));
}

/*
 * Initialize and return a button structure (alloc if one not given).
 * Store button data with Xview handle.
 */
void *
Button_init(
	void*			sp,	/* button state pointer */
	Xv_opaque		xvid,	/* xview button handle */
	void			(*heldCallback)(), /* button held callback */
	void			(*letgoCallback)()) /* button let go callback */
{
	Button_state		*bp;

	/* alloc a struct if one is not given */
	if (sp == NULL) {
		bp = (Button_state *) calloc(1, sizeof (Button_state));
		if (bp == (Button_state *) NULL)
			return ((void *) NULL);
	} else {
		bp = (Button_state *) sp;
	}

	bp->xvid = xvid;
	bp->held_down = FALSE;
	bp->held_proc = heldCallback;
	bp->letgo_proc = letgoCallback;

	/* replace button's event handler */
	xv_set(bp->xvid, PANEL_EVENT_PROC, button_event, NULL);

	/* init key to panel data */
	if (_BUTTON_KEY == 0) {
		_BUTTON_KEY = xv_unique_key();
		_Multi_click_time =(int)defaults_get_integer("MultiClickTimeout",
		    "MultiClickTimeout", 4) * 100000;
	}

	/* save button data handle with xview data  */
	xv_set(bp->xvid, XV_KEY_DATA, _BUTTON_KEY, bp, NULL);

	return ((void *) bp);
}

/*
 * Countdown timer for detecting a button held down.
 */
/*ARGSUSED*/
Notify_value
timer_countdown(
	Notify_client		client,
	int			which)
{
	Button_state*		bp;

	/* get button data handle */
	bp = Button_data(client);

	bp->held_down = TRUE;
	if (bp->held_proc) (bp->held_proc)(bp);
}


/*
 * Generic button event handler.  Detect button held down, and swallows down
 * event.
 */
Notify_value
button_event(
	Panel_item		item,
	Event			*event)
{
	Button_state*		bp;
	struct itimerval	timer;

	/* get button data handle */
	bp = Button_data(item);

	switch (event_action(event)) {
	case ACTION_SELECT:
	case ACTION_ADJUST:
	case PANEL_EVENT_CANCEL:
		/*
		 * Generally this means the user never let up on the 
		 * button, but dragged off it.  Two things could happen:
		 * 1.) Button was not held down yet, do nothing, clear timer.
		 * 2.) Button was held down, so assume some action needs
		 *     to be stopped, notify with the letgo_proc, because
		 *     notify proc will not be called.
		 */
		if (event_is_down(event) && 
		    (event_action(event) == PANEL_EVENT_CANCEL)) {
			/* clear hold down timer */
			set_xvtimer(bp->xvid, timer_countdown, 
				(struct itimerval*) 0);

			/* notify that button was let go */
			if ((bp->letgo_proc) && (bp->held_down))
				(bp->letgo_proc)(bp);
			break;
		}

		/*
		 * On down button set "hold down" timer, then perform
		 * normal button feedback with panel_default_handle_event.
		 * If time expires, button was held down, bp->held_down 
		 * gets set TRUE, and the bp->held_proc is called.
		 */
		if (event_is_down(event)) {
			bp->held_down = FALSE;

			/* count down to _Multi_click_time once */
			timer.it_value.tv_usec = _Multi_click_time;
			timer.it_value.tv_sec = 0;
			timer.it_interval.tv_usec = 0;
			timer.it_interval.tv_sec = 0;
			set_xvtimer(bp->xvid, timer_countdown, &timer);

			break;
		}

		/*
		 * On up button,  if button was "held down", 
		 * call letgo_proc(), then perform normal button feedback,
		 * otherwise cancel hold down timer.
		 */
		if (event_is_up(event)) {
			if (bp->held_down) {
				if (bp->letgo_proc) 
					(bp->letgo_proc)(bp);

				/* 
				 * bp->held_down REMAINS TRUE so notify_proc
				 * can distinguish single click from held down.
				 * panel_default_handle_event must be called
				 * so button image will be updated properly.
				 */
			} else {
				/* stop hold down timer */
				set_xvtimer(bp->xvid, timer_countdown, 
					(struct itimerval*) 0);
			}
			break;
		}

		break;
	}

	/* regardless of event, perform normal button feedback */
	panel_default_handle_event(item, event);
}


/* convenience function for Enable/Disable timer */
void
set_xvtimer(
	Xv_opaque		xvid,
	Notify_value		(*timer_func)(),
	struct itimerval*	timer)
{

	if (timer != NULL) {
		/* Set up to catch interval timer */
		(void) notify_set_itimer_func(xvid, timer_func, 
			ITIMER_REAL, timer, (struct itimerval *)0);

	} else {
		/* Cancel timer */
		(void) notify_set_itimer_func(xvid, timer_func, ITIMER_REAL,
		    (struct itimerval *)0, (struct itimerval *)0);
	}
}

