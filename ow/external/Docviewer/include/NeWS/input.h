#ifndef	_NEWS_INPUT_H
#define _NEWS_INPUT_H


#ident "@(#)input.h	1.2 06/11/93 NEWS SMI"

#include <nucleus/time.h>

/*
 * Definitions having to do with input events.
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */


extern int		 distribution_count,
			 news_has_cursor,
			 raw_keyboard,
			 raw_mouse;


extern struct object	but1_keyword,
			but2_keyword,
			but3_keyword,
			but4_keyword,
			but5_keyword,
			damage_keyword,
			down_keyword,
			enter_keyword,
			exit_keyword,
			motion_keyword,
			raw_but1_keyword,
			raw_but2_keyword,
			raw_but3_keyword,
			raw_motion_keyword,
			up_keyword,
			redistribute_keyword;

extern REF		eventq;

extern REF		current_keystate;

enum event_action	{ neither, down, up };
extern enum event_action last_action;
extern struct object	 last_name;

extern fract		 lastEventX,
			 lastEventY;

extern struct object	createevent();

extern enum error_type	return_interests(/* prelist, postlist, obj */);

extern void		 determine_keystate(/* event */),
			 post_crossings(/* CANVAS from, to;
					   corpus *fr_name, *to_name; (kwds)
					   fract x, y; TimeStamp time;
					   int is_grab_event */),
			 tangle_event(/* ee, event */),
			 untangle_event(/* event */),
			 enqueueEvent(/* event */),
			 dequeueEvent(/* event */),
			 enqueueDamageEvent(/* canvas */),
			 enqueueUnmapEvent(/* canvas, int from-configure */),
			 enqueueGravityEvent(/* canvas */),
			 enqueueColormapEvent(/* cv, mid, new, state */),
			 enqueueVisibilityEvent(/* canvas, enum visibility */);

#define is_local_root(cv)   (is_ref_zero(cs_parent(cv)))
extern RCANVAS GlobalRoot;
#define global_root() GlobalRoot->opaqueparent
#define GlobalInterestList  \
	(((XNCANVAS) entity_addr(global_root()))->preInterest)

struct queue_lock {
    int		count;
    TimeStamp   timer;
};

extern struct queue_lock    input_queue_lock;

#define DEFAULT_INPUT_QUEUE_TIMEOUT time_halfsec

#define DEVIDKBD	0x6F00
#define DEVIDMS		0x6E00
#define DEVIDMSLBUT	DEVIDMS+1
#define DEVIDMSMBUT	DEVIDMS+2
#define DEVIDMSRBUT	DEVIDMS+3
#define DEVIDSERA	0x6D00
#define DEVIDSERB	0x6C00


#define createevent(objp) \
    {\
    REF ce_tmp;\
    ce_tmp = pnew(NO_POOL, event_type, 0);\
    set_object_from_ref((objp), ce_tmp);\
    }



REF	CanvasUnderMouse,	/* The canvas currently under the mouse */
	LastCanvasUnderMouse;	/* The canvas under the mouse the last time
				   it moved */

 /*
  * Server's internal view of the motion structure. 
  */
typedef struct _MotionBuf {
    TimeStamp	time;
    short	x, y;
} MotionBuf;


/*
 * interest list cons cell
 */
typedef struct _eventconscell {
    struct _eventconscell       *next;
    ENTITY			*interest;
    unsigned int                serial;
} EvConsCell;

#endif /* _NEWS_INPUT_H */
