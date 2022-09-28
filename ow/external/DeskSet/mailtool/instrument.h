/* @(#)instrument.h	3.1 - 92/04/03 */

/* instrument.h -- track user objects */



#ifdef INSTRUMENT

extern void track_button( /* Menu menu, Menu_item item, char *title */ );
extern void track_button2( /* Menu menu, Menu_item item, char *title */ );
extern void track_panel( /* Panel_item item, Event *ie, char *title */ );
extern void track_drag_select( /* int start, int stop, int status */ );
extern void track_toggle_select( /* int msg, int onoff */ );
extern void track_message( /* char *message */ );
extern void track_exit( /* int status */ );
extern void track_startup();
extern void track_timer();
extern void track_showmsg( /* int msg, char *how */ );

#define TRACK_BUTTON(menu, item, title)	track_button(menu, item, title)
#define TRACK_BUTTON2(menu, item, title) track_button2(menu, item, title)
#define TRACK_PANEL(item, ie, title)	track_panel(item, ie, title)
#define TRACK_DRAG_SELECT(start, stop)	track_drag_select(start, stop)
#define TRACK_TOGGLE_SELECT(msg, onoff)	track_toggle_select(msg, onoff)
#define TRACK_MESSAGE(message)		track_message(message)
#define TRACK_EXIT(status)		track_exit(status)
#define TRACK_STARTUP()			track_startup()
#define TRACK_TIMER()			track_timer()
#define TRACK_SHOWMSG(msg, how)		track_showmsg(msg, how)

#else

#define TRACK_BUTTON(menu, item, title)
#define TRACK_BUTTON2(menu, item, title)
#define TRACK_PANEL(item, ie, title)
#define TRACK_DRAG_SELECT(start, stop)
#define TRACK_TOGGLE_SELECT(msg, onoff)
#define TRACK_MESSAGE(msg)
#define TRACK_EXIT(status)
#define TRACK_STARTUP()
#define TRACK_TIMER()
#define TRACK_SHOWMSG(msg, how)

#endif INSTRUMENT
