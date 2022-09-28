/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_HOLDBUTTON_H
#define	_AUDIOTOOL_HOLDBUTTON_H

#ident	"@(#)holdbutton.h	1.3	92/12/14 SMI"

/* Internal Interface */

#include <xview/notify.h>

static int 		_Multi_click_time;		/* Xview default */
static int 		_BUTTON_KEY;			/* Xview default */

typedef struct button_state {
	int		held_down;	/* true when button has been held down */
	Xv_opaque	xvid;		/* xview button handle, used as unique
					 * id for timer
					 */
	void		(*held_proc)();		/* button held call back */
	void		(*letgo_proc)();	/* button let go call back */
} Button_state;

Notify_value 		button_event();


/* External Interface */

void* 			Button_init(void*, Xv_opaque, void(*)(), void(*)());
void* 			Button_data(Xv_opaque);
Xv_opaque 		Button_xvid(void*);
int 			Button_helddown(void*);

#endif /* !_AUDIOTOOL_HOLDBUTTON_H */
